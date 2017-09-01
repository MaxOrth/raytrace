#pragma once

#include "raydata.h"

void RAYAPI InitAABB(Tri const *elem, AABB *dest);
void RAYAPI InitAABB(vec3 const *p, AABB *dest);

/*
  Makes sure no dimension in aabb is 0. adds a little bit to dims that are
*/
void MinSizeAABB(AABB *dest);

/*
  Grows the aabb as required to keep elem in it.
*/
void RAYAPI GrowAABB(vec3 const *elem, AABB *dest);

/*
Grows the aabb as required to keep elem in it.
*/
void RAYAPI GrowAABB(vec3 const *elems, size_t count, AABB *dest);

/*
  Grows the aabb to keep the list of triangles inside.
*/
void RAYAPI GrowAABB(Tri const *triangles, size_t tricount, AABB *aabb);

void RAYAPI GrowAABB(vec3 const *verts, cl_uint8 const *tris, size_t const *which, size_t count, AABB *aabb);

/*
  Calculate centroid of triangle
*/
void Centroid(Tri const *triangle, vec3 *coord);

bool PointInAABB(vec3 const *p, AABB const *aabb, float e = 0.000001f);

/*
  If the triangle has at least a coordinate in the bb.
  Useful for splitting bounding volumes b/c the triangle
  cant be entirely outside the volume.
*/
bool TriangleInAABBSimple(Tri const *tri, AABB const *aabb, float e = 0.000001f);

void TriFromIndices(vec3 const *verts, uint3 const *indices, Tri *dest);
