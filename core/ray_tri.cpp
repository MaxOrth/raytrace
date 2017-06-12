
#include "raydata.h"

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm

int intersect_ray_triangle(struct vec3 const *a, struct vec3 const *b, struct vec3 const *c, struct Ray const *r, struct vec2 *uvout, float *tout)
{
  struct vec3 e1;
  struct vec3 e2;
  
  struct vec3 P;
  struct vec3 Q;
  struct vec3 T;
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

// https://tavianator.com/fast-branchless-raybounding-box-intersections/

#define min(a,b) ( (a) < (b) ? (a) : (b) )
#define max(a,b) ( (a) > (b) ? (a) : (b) )

int intersect_ray_aabb(AABB const *aabb, struct Ray const *ray, float *t)
{
  struct vec3 dir_inv = { 1 / ray->dir.x, 1 / ray->dir.y, 1 / ray->dir.z };
  
  float t1 = (aabb->a.x - ray->origin.x) * dir_inv.x;
  float t2 = (aabb->b.x - ray->origin.x) * dir_inv.x;

  float tmin = min(t1, t2);
  float tmax = max(t1, t2);

  for (int i = 1; i < 3; ++i)
  {
    t1 = (aabb->a[i] - ray->origin[i]) * dir_inv[i];
    t2 = (aabb->b[i] - ray->origin[i]) * dir_inv[i];
    tmin = max(tmin, min(t1, t2));
    tmax = min(tmax, max(t1, t2));
  }
  return tmax > max(tmin, 0);
}

