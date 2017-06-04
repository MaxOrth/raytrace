
#include "raydata.h"

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
  
  SUB(*b, *a, e1);
  SUB(*c, *a, e2);
  CROSS(r->dir, e2, P);
  
  det = DOT(e1, P);
  if (NEAR_ZERO(det))
    return 0;
  
  det_inv = 1.f / det;
  
  SUB(r->origin, *a, T);
  
  uv.u = DOT(T, P) * det_inv;
  if (uv.u < 0.f || uv.u > 1.f)
    return 0;
  
  CROSS(T, e1, Q);
  
  uv.v = DOT(r->dir, Q) * det_inv;
  if (uv.v < 0.f || uv.v + uv.u > 1.f)
    return 0;
  
  t = DOT(e2, Q) * det_inv;
  
  if (t > EPSILON)
  {
    *tout = t;
    *uvout = uv;
    return 1;
  }
  
  return 0;
  
}


__kernel void trace(__write_only image2d_t destimg)
{
  int2 pix = { get_global_id(0), get_global_id(1) };
  
  
  float4 color = {0,0,0,1};
  
  /*
  Tri testTri = {
    {50, 20, 0},
    {600, 400, 0},
    {400, 100, 0}
  };
  */
  
  
  Tri testTri = {
    {1, 1, 0},
    {50, 50, 0},
    {0, 50, 0}
  };
  
  struct vec2 uv;
  float t;
  struct Ray ray;
  ray.origin.x = pix.x;
  ray.origin.y = pix.y;
  ray.origin.z = -1;
  ray.dir.x = 0;
  ray.dir.y = 0;
  ray.dir.z = 1;
  
  if (intersect_ray_triangle(&testTri[0], &testTri[1], &testTri[2], &ray, &uv, &t))
  {
    color.x = 1 - uv.u - uv.v;
    color.y = uv.u;
    color.z = uv.v;
  }
  
  write_imagef(destimg, pix, color);
}
