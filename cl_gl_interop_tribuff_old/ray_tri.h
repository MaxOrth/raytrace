#pragma once


/*
  Intersect ray and triangle
*/

int intersect_ray_triangle(struct vec3 const *a, struct vec3 const *b, struct vec3 const *c, struct Ray const *r, struct vec2 *uvout, float *tout);




