#pragma once

#ifdef __OPENCL_VERSION__
#include "core/raydata.h"
#else
#include "raydata.h"
#endif

/*
  Intersect ray and triangle
*/

int intersect_ray_triangle(struct vec3 const *a, struct vec3 const *b, struct vec3 const *c, struct Ray const *r, struct vec2 *uvout, float *tout);

int intersect_ray_aabb(AABB const *aabb, struct Ray const *ray, float *t);


