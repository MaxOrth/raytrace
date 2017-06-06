
#include "core/raydata.h"

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

typedef float4 (fmat4x4)[4];

void mult_m_f4(fmat4x4 const *mat, float4 const *vec, float4 *out)
{
  out->x = dot((*mat)[0], *vec);
  out->y = dot((*mat)[1], *vec);
  out->z = dot((*mat)[2], *vec);
  out->w = dot((*mat)[3], *vec);
}

__kernel void trace(__write_only image2d_t destimg, __read_only __global float4 *cam_mat)
{
  int2 pix = { get_global_id(0), get_global_id(1) };
  int2 dim = get_image_dim(destimg);
  float2 dimf = convert_float2(dim);
  float2 pixf = convert_float2(pix);
  float2 raydest = (pixf - (dimf * 0.5f)) / dimf;
  
  float4 color = {0,0,0,1};
  
  /*
  Tri testTri = {
    {50, 20, 0},
    {600, 400, 0},
    {400, 100, 0}
  };
  */
  
  float3 v1 = { 0, 0, 2 };
  float3 v2 = { 0.3, 0.3,  2 };
  float3 v3 = { -0.5, 0.3, 2 };
  float3 v4 = { 0, 0.3, 2 };
  
  Tri testTri[2] = {
    {v1, v2, v4},
    {v1, v3, v4},
  //  {v1, v4, v2},
  //  {v2, v3, v4}
  };
  
  struct Ray ray;
  ray.origin.x = 0;
  ray.origin.y = 0;
  ray.origin.z = -1;
  
  float3 rayend = {raydest.x, raydest.y, 0};
  ray.dir = normalize(rayend - ray.origin);
  
  fmat4x4 camera_matrix = { cam_mat[0], cam_mat[1], cam_mat[2], cam_mat[3]  };
  
  float4 out;
  float4 raytmp = {ray.origin.x, ray.origin.y, ray.origin.z, 1};
  mult_m_f4(&camera_matrix, &raytmp, &out);
  ray.origin = out.xyz;
  
  raytmp.x = ray.dir.x;
  raytmp.y = ray.dir.y;
  raytmp.z = ray.dir.z;
  raytmp.w = 0;
  mult_m_f4(&camera_matrix, &raytmp, &out);
  ray.dir = out.xyz;
  
  float t;
  float t_near = MAXFLOAT;
  struct vec2 uv;
  
  for (unsigned i = 0; i < 2; ++i)
  {
    if (intersect_ray_triangle(&testTri[i][0], &testTri[i][1], &testTri[i][2], &ray, &uv, &t) && t < t_near)
    {
      color.x = 1 - uv.u - uv.v;
      color.y = uv.u;
      color.z = uv.v;
      t_near = t;
    }
  }
  
  
  write_imagef(destimg, pix, color);
}
