
#ifndef __OPENCL_VERSION__
#define __kernel 
#define __global 
#define __read_only 
#define __write_only 
#define __local 
#define __constant 
#endif

#define COLOR_INCR 0.0001


#include "core/raydata.h"
#include "core/MtlLib.h"
#include "core/CentroidBVH.h"

typedef CentroidBVHNode __constant *CBVH_GPTR;

// #include "raytri.h"
// uv coordinate detail: (1-u-v)*a + u*b + u*c
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

// project v onto p
float4 proj(float4 v, float4 p)
{
  return (dot(v, p) / dot(p, p)) * p;
}

// reflect v over n
float4 reflect(float4 v, float4 n)
{
  return v - 2.0f * dot(v,n) * n;
}

typedef float4(fmat4x4)[4];

enum TraverseState
{
  tsFromParent,
  tsFromSibling,
  tsFromChild
};

unsigned near_child_impl(unsigned i, CentroidBVHNode __constant *accel, struct Ray ray)
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

unsigned parent_impl(unsigned i, CentroidBVHNode __constant *accel)
{
  return accel[i].parent_index;
}

unsigned sibling_impl(unsigned i, CentroidBVHNode __constant *accel)
{
  // if i is same index as 0th child, it becomes 1 which is the index of the other child
  unsigned test = accel[accel[i].parent_index].node.inner.children_index[0] == i;
  return accel[accel[i].parent_index].node.inner.children_index[test];
}

#define nearChild(i, r) (near_child_impl((i), accel, (r)))
#define farChild(i, r) (far_child_impl((i), accel, (r)))
#define parent(i) (parent_impl((i), accel))
#define sibling(i) (sibling_impl((i), accel))

void ray_leaf_intersection(
  CBVH_GPTR leaf, 
  struct Ray ray, 
  float3 __constant *vert_buff, 
  uint8 __constant *ind_buff, 
  struct vec2 *uvout, 
  float *t_update,
  unsigned *tri_ind_update
)
{
  float t_tmp;
  struct vec2 uv;
  for (unsigned j = 0; j < leaf->node.leaf.count; ++j)
  {
    uint3 tri_index = ind_buff[leaf->node.leaf.tris[j]].s012;
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

float4 mult_m_f4(fmat4x4 mat, float4 vec)
{
  float4 result;
  result.x = dot((mat)[0], vec);
  result.y = dot((mat)[1], vec);
  result.z = dot((mat)[2], vec);
  result.w = dot((mat)[3], vec);
  return result;
}

__kernel void trace(
  __write_only image2d_t color_img,

  __constant RayIntersection *ray_in,

  __global RayIntersection *ray_out,

  const unsigned tricount,
  __constant float *vertices,
  __constant float4 *normals,
  __constant uint8 *indices,
  __constant float4 *cam_mat,
  __constant CentroidBVHNode *accel,
  __constant Material *mtl_lib,
  const unsigned trace_operation
)
{
  __constant float3 *vert_buff = (__constant float3 *)vertices;
  __constant uint8 *ind_buff = indices;

  struct Ray ray;
  unsigned gworkid_x = get_global_id(0);
  unsigned gworkid_y = get_global_id(1);
  unsigned ray_in_buffer_index = gworkid_x + get_global_size(0) * gworkid_y;
  int2 pix;
  pix.x = get_global_id(0);
  pix.y = get_global_id(1);

  RayIntersection __constant * const work_item = ray_in + ray_in_buffer_index;
  RayIntersection __global * const ray_out_item = ray_out + ray_in_buffer_index;
  Material __constant *tri_material = mtl_lib + ind_buff[work_item->triangle_id].s3;

  ray.origin = work_item->origin.xyz;

  // uniform behavior on trace_operation
  if (trace_operation == 1) // reflection
  {
    // do we need to fudge the ray a bit away from the intersected triangle?
    // yes, we do
    ray.dir = reflect(work_item->direction, work_item->normal).xyz;
    ray.origin = ray.origin + work_item->normal.xyz * 0.00001f;
  }
  else if (trace_operation == 0)
  {
    // use given direction
    ray.dir = work_item->direction.xyz;
  }
  //else if (trace_operation == 2) // refraction
  //{
  //  /*
  //    sin(a) / sin(b) = nb / na
  //   */
  //  float sn = tri_material.Si;
  //}

  float4 color = { 0,0,0,0 };


  // if using direction, apply camera matrix
  if (trace_operation == 0)
  {
    float4 out;
    fmat4x4 camera_matrix = { cam_mat[0], cam_mat[1], cam_mat[2], cam_mat[3] };
    float4 raytmp = { ray.origin.x, ray.origin.y, ray.origin.z, 1 };
    out = mult_m_f4(camera_matrix, raytmp);
    ray.origin = out.xyz;

    raytmp.x = ray.dir.x;
    raytmp.y = ray.dir.y;
    raytmp.z = ray.dir.z;
    raytmp.w = 0;
    out = mult_m_f4(camera_matrix, raytmp);
    ray.dir = out.xyz;
  }

  float3 light_dir = { 0, 0, -1 };



  unsigned curr_node = nearChild(0, ray);
  enum TraverseState traverse_state = tsFromParent;

  // t for aabb intersection. not used yet
  float t_quit_early;

  struct vec2 uv;
  float t_near = MAXFLOAT;
  float t_tmp = 0;
  unsigned intersected_triangle = ~0;

  ray_out_item->origin = work_item->origin;
  ray_out_item->normal = work_item->direction;
  ray_out_item->uv.x = 0;
  ray_out_item->uv.y = 0;
  ray_out_item->triangle_id = ~0;

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
      if (!intersect_ray_aabb(accel[curr_node].aabb, ray, &t_quit_early) || t_quit_early > t_near)
      {
        curr_node = parent(curr_node);
        traverse_state = tsFromChild;
      }
      else if (accel[curr_node].type == cbvhLEAF)
      {
        //color.y += COLOR_INCR;
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
        //color.z += COLOR_INCR;
        curr_node = nearChild(curr_node, ray);
        traverse_state = tsFromParent;
      }
    }
    else if (traverse_state == tsFromParent)
    {
      if (!intersect_ray_aabb(accel[curr_node].aabb, ray, &t_quit_early) || t_quit_early > t_near)
      {
        curr_node = sibling(curr_node);
        traverse_state = tsFromSibling;
      }
      else if (accel[curr_node].type == cbvhLEAF)
      {
        //color.y += COLOR_INCR;
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
        //color.z += COLOR_INCR;
        curr_node = nearChild(curr_node, ray);
        traverse_state = tsFromParent;
      }
    }
  } while (go);
  
  // calculate final color and output vectors
  if (intersected_triangle != ~0)
  {
    uint8 tri_ind = ind_buff[intersected_triangle];
    float4 n = uv.u * normals[tri_ind.s4] + uv.v * normals[tri_ind.s5] + (1 - uv.u - uv.v) * normals[tri_ind.s6];

    ray_out_item->origin.xyz = (1 - uv.u - uv.v) * vert_buff[tri_ind.s0] + uv.u * vert_buff[tri_ind.s1] + uv.v * vert_buff[tri_ind.s2];
    ray_out_item->origin.w = 1;

    ray_out_item->normal = n;
    ray_out_item->normal.w = 0;

    // refraction turning into a bitch
    //if (dot(ray.dir, n.xyz) < 0) // entering volume
    //{
    //  float sn_old = mtl_lib[ind_buff[work_item->triangle_id].s3].Sn;
    //}
    //else // leaving volume
    //{
    //  
    //}
    

    ray_out_item->direction.xyz = ray.dir;
    ray_out_item->direction.w = 0;

    ray_out_item->uv.x = uv.u;
    ray_out_item->uv.y = uv.v;

    float u = fabs(dot(n.xyz, ray.dir));
    color.xyz = u * mtl_lib[tri_ind.s3].color;

    
    color.w = mtl_lib[tri_ind.s3].Rf;  // fresnel calculation here for refraction and reflections
    
    //color.xyz = (1 + n.xyz) * 0.5f;
    //color.w = 1;
  }
  


  write_imagef(color_img, pix, color);
}
