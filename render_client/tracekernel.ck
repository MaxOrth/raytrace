
#ifndef __OPENCL_VERSION__
#define __global __global
#define __read_only __read_only
#define __write_only __write_only
#define __local __local
#endif

#include "core/raydata.h"
#include "core/CentroidBVH.h"

typedef __global __read_only CentroidBVHNode *CBVH_GPTR;

// #include "raytri.h"
int intersect_ray_triangle(VEC3 a, VEC3 b, VEC3 c, struct Ray r, struct vec2 *uvout, float *tout)
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

  e1 = b - a;
  e2 = c - a;
  P = cross(r.dir, e2);

  det = dot(e1, P);
  if (NEAR_ZERO(det))
    return 0;

  det_inv = 1.f / det;

  T = r.origin - a;

  uv.u = dot(T, P) * det_inv;
  if (uv.u < 0.f || uv.u > 1.f)
    return 0;

  Q = cross(T, e1);

  uv.v = dot(r.dir, Q) * det_inv;
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

int intersect_ray_aabb(AABB aabb, struct Ray ray, float *t)
{
  float3 dir_inv = { 1 / ray.dir.x, 1 / ray.dir.y, 1 / ray.dir.z };

  float t1 = (aabb.a.x - ray.origin.x) * dir_inv.x;
  float t2 = (aabb.b.x - ray.origin.x) * dir_inv.x;
  float tmin = fmin(t1, t2);
  float tmax = fmax(t1, t2);

  t1 = (aabb.a.y - ray.origin.y) * dir_inv.y;
  t2 = (aabb.b.y - ray.origin.y) * dir_inv.y;
  tmin = fmax(tmin, fmin(t1, t2));
  tmax = fmin(tmax, fmax(t1, t2));

  t1 = (aabb.a.z - ray.origin.z) * dir_inv.z;
  t2 = (aabb.b.z - ray.origin.z) * dir_inv.z;
  tmin = fmax(tmin, fmin(t1, t2));
  tmax = fmin(tmax, fmax(t1, t2));

  *t = tmin;
  return tmax > fmax(tmin, 0);
}

typedef float4(fmat4x4)[4];

enum TraverseState
{
  tsFromParent,
  tsFromSibling,
  tsFromChild
};

unsigned near_child_impl(unsigned i, __global __read_only CentroidBVHNode *accel, struct Ray ray)
{
  // test is 1 if in same direction
  unsigned test = dot(ray.dir, accel[i].node.inner.plane_norm) > 0;
  return accel[i].node.inner.children_index[test];
}

//unsigned far_child_impl(unsigned i, __global __read_only CentroidBVHNode *accel, struct Ray ray)
//{
//  unsigned test = dot(ray.dir, accel[i].node.inner.plane_norm) <= 0;
//  return accel[i].node.inner.children_index[test];
//}

unsigned parent_impl(unsigned i, __global __read_only CentroidBVHNode *accel)
{
  return accel[i].parent_index;
}

unsigned sibling_impl(unsigned i, __global __read_only CentroidBVHNode *accel)
{
  // if i is same index as 0th child, it becomes 1 which is the index of the other child
  unsigned test = accel[accel[i].parent_index].node.inner.children_index[0] == i;
  return accel[accel[i].parent_index].node.inner.children_index[test];
}

void ray_leaf_intersection(
  CBVH_GPTR leaf, 
  struct Ray ray, 
  __global __read_only float3 *vert_buff, 
  __global __read_only uint3 *ind_buff, 
  struct vec2 *uvout, 
  float *t_update,
  unsigned *tri_ind_update
)
{
  float t_tmp;
  struct vec2 uv;
  for (unsigned j = 0; j < leaf->node.leaf.count; ++j)
  {
    uint3 tri_index = ind_buff[leaf->node.leaf.tris[j]];
    float3 a = vert_buff[tri_index.x];
    float3 b = vert_buff[tri_index.y];
    float3 c = vert_buff[tri_index.z];
    //  VEC3 const a, VEC3 const b, VEC3 const c, struct Ray const r, struct vec2 *uvout, float *tout
    if (
      intersect_ray_triangle(
        a,
        b,
        c,
        ray,
        &uv,
        &t_tmp
      ) // v1, v2, v3, ray, &uv, &t_tmp) 
      && t_tmp < *t_update)
    {
      *t_update = t_tmp;
      *uvout = uv;
      *tri_ind_update = leaf->node.leaf.tris[j];
    }
  }
}

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
  //printf("size of node: %i\n", sizeof(struct CentroidBVHNode));
  //printf("Size of union: %i\n", sizeof(union CentroidBVHNodeUnion));
  //printf("Leaf size: %i\n", sizeof(struct CentroidBVHLeafNode));
  //printf("Inner size: %i\n", sizeof(struct CentroidBVHInnerNode));
  //return;
  
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

  __global float3 *vert_buff = (__global __read_only float3 *)vertices;
  __global uint3 *ind_buff = (__global __read_only uint3 *)indices;


#define nearChild(i, r) (near_child_impl((i), accel, (r)))
#define farChild(i, r) (far_child_impl((i), accel, (r)))
#define parent(i) (parent_impl((i), accel))
#define sibling(i) (sibling_impl((i), accel))

  unsigned curr_node = nearChild(0, ray);
  enum TraverseState traverse_state = tsFromParent;

  // t for aabb intersection. not used yet
  float t_quit_early;

  struct vec2 uv;
  float t_near = MAXFLOAT;
  float t_tmp = 0;
  unsigned intersected_triangle = -1;

  bool go = true;
  // stackless bvh traversal from https://graphics.cg.uni-saarland.de/2011/hapalasccg2011/
  
  do
  {
    if (traverse_state == tsFromChild)
    {
      // root is index 0
      if (curr_node == 0)
      {
        go = false;
      }
      else if (curr_node == nearChild(parent(curr_node), ray))
      {
        curr_node = sibling(curr_node);
        traverse_state = tsFromSibling;
      }
      else
      {
        curr_node = parent(curr_node);
        traverse_state = tsFromChild;
      }
    }
    else if (traverse_state == tsFromSibling)
    {
      if (!intersect_ray_aabb(accel[curr_node].aabb, ray, &t_quit_early))
      {
        curr_node = parent(curr_node);
        traverse_state = tsFromChild;
      }
      else if (accel[curr_node].type == cbvhLEAF)
      {
        ray_leaf_intersection(
          accel + curr_node,
          ray,
          vert_buff,
          ind_buff,
          &uv,
          &t_near,
          &intersected_triangle
        );

        curr_node = parent(curr_node);
        traverse_state = tsFromChild;
      }
      else
      {
        curr_node = nearChild(curr_node, ray);
        traverse_state = tsFromParent;
      }
    }
    else if (traverse_state == tsFromParent)
    {
      if (!intersect_ray_aabb(accel[curr_node].aabb, ray, &t_quit_early))
      {
        curr_node = sibling(curr_node);
        traverse_state = tsFromSibling;
      }
      else if (accel[curr_node].type == cbvhLEAF)
      {
        ray_leaf_intersection(
          accel + curr_node,
          ray,
          vert_buff,
          ind_buff,
          &uv,
          &t_near,
          &intersected_triangle
        );
        curr_node = sibling(curr_node);
        traverse_state = tsFromSibling;
      }
      else
      {
        curr_node = nearChild(curr_node, ray);
        traverse_state = tsFromParent;
      }
    }
  } while (go);
  
  // calculate final color
  if (intersected_triangle != -1)
  {
    uint3 tri_ind = ind_buff[intersected_triangle];
    float3 a = vert_buff[tri_ind.x];
    float3 b = vert_buff[tri_ind.y];
    float3 c = vert_buff[tri_ind.z];
    float3 n = normalize(cross(a - c, a - b));
    float u = 0.7 * dot(n, ray.dir);
    color.x = 0.3 + u;
    color.y = 0.3 + u;
    color.z = 0.3 + u;
    color.w = 1;
  }

  //// iterate through all tris (naive method)
  //t_near = MAXFLOAT;
  //for (unsigned i = 0; i < tricount; ++i)
  //{
  //  uint3 tri_ind = ind_buff[i];
  //  float3 a = vert_buff[tri_ind.x];
  //  float3 b = vert_buff[tri_ind.y];
  //  float3 c = vert_buff[tri_ind.z];
  //  float3 n = cross(a - b, a - c);
  //  float u = fabs((float)(dot(n, ray.dir) * 0.7f));
  //  if (intersect_ray_triangle(a, b, c, ray, &uv, &t_tmp) && t_tmp < t_near)
  //  {
  //    //color.y = 0.1;
  //    color.x = 1;//u;//1 - uv.u - uv.v;
  //    //color.y = 1;//u;//uv.u;
  //    //color.z = 1;//u;//uv.v;
  //    t_near = t_tmp;
  //  }
  //}

  write_imagef(destimg, pix, color);
}
