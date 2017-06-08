#pragma once

#include "raydata.h"

void InitAABB(Tri const *elem, AABB *dest);

/*
  Grows the aabb as required to keep elem in it.
*/
void GrowAABB(vec3 const *elem, AABB *dest);

/*
  Grows the aabb to keep the list of triangles inside.
*/
void GrowAABB(Tri const *triangles, size_t tricount, AABB *aabb);


/*
  Calculate centroid of triangle
*/
void Centroid(Tri const *triangle, vec3 *coord);

bool PointInAABB(vec3 const *p, AABB const *aabb);

/*
  If the triangle has at least a coordinate in the bb.
  Useful for splitting bounding volumes b/c the triangle
  cant be entirely outside the volume.
*/
bool TriangleInAABBSimple(Tri const *tri, AABB const *aabb);

