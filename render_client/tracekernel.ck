
#include "core/raydata.h"
#include "core/CentroidBVH.h"

// #include "raytri.h"
int intersect_ray_triangle(VEC3 const *a, VEC3 const *b, VEC3 const *c, struct Ray const *r, struct vec2 *uvout, float *tout)
{
  VEC3 e1;
  VEC3 e2;

  VEC3 P;
  VEC3 Q;
  VEC3 T;
  struct vec2 uv;

  float det;
  float det_inv;
  float t;

  e1 = *b - *a;
  e2 = *c - *a;
  P = cross(r->dir, e2);

  det = dot(e1, P);
  if (NEAR_ZERO(det))
    return 0;

  det_inv = 1.f / det;

  T = r->origin - *a;

  uv.u = dot(T, P) * det_inv;
  if (uv.u < 0.f || uv.u > 1.f)
    return 0;

  Q = cross(T, e1);

  uv.v = dot(r->dir, Q) * det_inv;
  if (uv.v < 0.f || uv.v + uv.u > 1.f)
    return 0;

  t = dot(e2, Q) * det_inv;

  if (t > EPSILON)
  {
    *tout = t;
    *uvout = uv;
    return 1;
  }

  return 0;

}

int intersect_ray_aabb(AABB const *aabb, struct Ray const *ray, float *t)
{
  float3 dir_inv = { 1 / ray->dir.x, 1 / ray->dir.y, 1 / ray->dir.z };

  float t1 = (aabb->a.x - ray->origin.x) * dir_inv.x;
  float t2 = (aabb->b.x - ray->origin.x) * dir_inv.x;
  float tmin = fmin(t1, t2);
  float tmax = fmax(t1, t2);

  t1 = (aabb->a.y - ray->origin.y) * dir_inv.y;
  t2 = (aabb->b.y - ray->origin.y) * dir_inv.y;
  tmin = fmax(tmin, fmin(t1, t2));
  tmax = fmin(tmax, fmax(t1, t2));

  t1 = (aabb->a.z - ray->origin.z) * dir_inv.z;
  t2 = (aabb->b.z - ray->origin.z) * dir_inv.z;
  tmin = fmax(tmin, fmin(t1, t2));
  tmax = fmin(tmax, fmax(t1, t2));

  *t = tmin;
  return tmax > fmax(tmin, 0);
}

typedef float4(fmat4x4)[4];

void mult_m_f4(fmat4x4 const *mat, float4 const *vec, float4 *out)
{
  out->x = dot((*mat)[0], *vec);
  out->y = dot((*mat)[1], *vec);
  out->z = dot((*mat)[2], *vec);
  out->w = dot((*mat)[3], *vec);
}

__kernel void trace(
  __write_only image2d_t destimg,
  const unsigned tricount,
  __read_only __global float *vertices,
  __read_only __global unsigned *indices,
  __read_only __global float4 *cam_mat,
  __read_only __global CentroidBVHNode *accel
  )
{
  int2 pix = { get_global_id(0), get_global_id(1) };
  int2 dim = get_image_dim(destimg);
  float2 dimf = convert_float2(dim);
  float2 pixf = convert_float2(pix);
  float2 raydest = (pixf - (dimf * 0.5f)) / dimf;

  float4 color = { 0,0,0,1 };

  struct Ray ray;
  ray.origin.x = 0;
  ray.origin.y = 0;
  ray.origin.z = -1;

  float3 rayend = { raydest.x, raydest.y, 0 };
  ray.dir = normalize(rayend - ray.origin);

  fmat4x4 camera_matrix = { cam_mat[0], cam_mat[1], cam_mat[2], cam_mat[3] };

  float4 out;
  float4 raytmp = { ray.origin.x, ray.origin.y, ray.origin.z, 1 };
  mult_m_f4(&camera_matrix, &raytmp, &out);
  ray.origin = out.xyz;

  raytmp.x = ray.dir.x;
  raytmp.y = ray.dir.y;
  raytmp.z = ray.dir.z;
  raytmp.w = 0;
  mult_m_f4(&camera_matrix, &raytmp, &out);
  ray.dir = out.xyz;
  float3 light_dir = { 0, 0, -1 };

  float t;
  float t_near = MAXFLOAT;
  struct vec2 uv;

  __global float3 *vert_buff = (__global __read_only float3 *)vertices;
  __global uint3 *ind_buff = (__global __read_only uint3 *)indices;

  //list of internal nodes with potential intersections
  __global __read_only CentroidBVHNode * __local nodeStack[80];
  __global __read_only CentroidBVHNode * __local leafNodeStack[40];
  unsigned nodeStackSize = 1; // 1 bc we push root on at beginning
  unsigned leafStackSize = 0;
  
  // push root onto stack
  nodeStack[0] = accel;

  struct AABB bb = accel[0].aabb;
  if (intersect_ray_aabb(&bb, &ray, &t))
  {
    color.x = 1;
  }

  // find all leaf node with potential intersections
  //while (nodeStackSize)
  //{
  //  // pop top of stack
  //  __global __read_only CentroidBVHNode *curr = nodeStack[nodeStackSize - 1];
  //  nodeStackSize--;
  //  
  //  unsigned child1 = curr->node.inner.children_index[0];
  //  struct AABB bb = accel[child1].aabb;
  //  if (intersect_ray_aabb(&bb, &ray, &t))
  //  {
  //    color.x = 1;
  //    //if (accel[child1].type == cbvhINNER)
  //    //{
  //    //  nodeStack[nodeStackSize] = accel + child1;
  //    //  nodeStackSize++;
  //    //}
  //    //else
  //    //{
  //    //  color.z = 1;
  //    //  //leafNodeStack[leafStackSize] = child1;
  //    //  //leafStackSize++;
  //    //}
  //  }
  //
  //  unsigned child2 = curr->node.inner.children_index[0];
  //  bb = accel[child2].aabb;
  //  if (intersect_ray_aabb(&bb, &ray, &t))
  //  {
  //    color.x = 1;
  //    //if (accel[child2].type == cbvhINNER)
  //    //{
  //    //  nodeStack[nodeStackSize] = accel + child2;
  //    //  nodeStackSize++;
  //    //}
  //    //else
  //    //{
  //    //  color.z = 1;
  //    //  //leafNodeStack[leafStackSize] = child2;
  //    //  //leafStackSize++;
  //    //}
  //  }
  //}

  //float t_min = 99999999;
  //float t_tmp;
  //// intersect ray with leaf node contents
  //for (unsigned i = 0; i < leafStackSize; ++i)
  //{
  //  __global __read_only CentroidBVHNode *curr = leafNodeStack[i];
  //  for (unsigned j = 0; j < curr->node.leaf.count; ++j)
  //  {
  //    uint3 tri_index = curr->node.leaf.tris[j];
  //    float3 v1 = vertices[indices[tri_index.x]];
  //    float3 v2 = vertices[indices[tri_index.y]];
  //    float3 v3 = vertices[indices[tri_index.z]];
  //    // VEC3 const *a, VEC3 const *b, VEC3 const *c, struct Ray const *r, struct vec2 *uvout, float *tout
  //    if (intersect_ray_triangle(&v1, &v2, &v3, &ray, &uv, &t_tmp) && t_tmp < t_near)
  //    {
  //      color.x = 1 - uv.u - uv.v;
  //      color.y = uv.u;
  //      color.z = uv.v;
  //      t_near = t_tmp;
  //    }
  //  }
  //}

  for (unsigned i = 0; i < tricount; ++i)
  {
    uint3 tri_ind = ind_buff[i];
    float3 a = vert_buff[tri_ind.x];
    float3 b = vert_buff[tri_ind.y];
    float3 c = vert_buff[tri_ind.z];
    float3 n = cross(a - b, a - c);
    float u = fabs((float)(dot(n, ray.dir) * 0.7f));
    if (intersect_ray_triangle(&a, &b, &c, &ray, &uv, &t) && t < t_near)
    {
      color.y = 1;
      //color.x = 0.3f + u;//1 - uv.u - uv.v;
      //color.y = 0.3f + u;//uv.u;
      //color.z = 0.3f + u;//uv.v;
      t_near = t;
    }
  }


  write_imagef(destimg, pix, color);
}
