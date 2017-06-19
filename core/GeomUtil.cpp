
#include <cmath>
#include <algorithm>
#include "GeomUtil.h"

/*
  AABB.a will be "min", AABB.b will be "max"
*/

void RAYAPI InitAABB(Tri const *t, AABB *dest)
{
  dest->a.x = std::min(std::min((*t)[0].x, (*t)[1].x), (*t)[2].x);
  dest->a.y = std::min(std::min((*t)[0].y, (*t)[1].y), (*t)[2].y);
  dest->a.z = std::min(std::min((*t)[0].z, (*t)[1].z), (*t)[2].z);

  dest->b.x = std::max(std::max((*t)[0].x, (*t)[1].x), (*t)[2].x);
  dest->b.y = std::max(std::max((*t)[0].y, (*t)[1].y), (*t)[2].y);
  dest->b.z = std::max(std::max((*t)[0].z, (*t)[1].z), (*t)[2].z);
}

void InitAABB(vec3 const *v, AABB *dest)
{
  dest->a = *v;
  dest->b = *v;
}

void MinSizeAABB(AABB *dest)
{
  if (dest->b.x - dest->a.x < 0.01)
  {
    dest->b.x += 0.01;
  }
  if (dest->b.y - dest->a.y < 0.01)
  {
    dest->b.y += 0.01;
  }
  if (dest->b.z - dest->a.z < 0.01)
  {
    dest->b.z += 0.01;
  }
}

void RAYAPI GrowAABB(vec3 const *test, AABB *dest)
{
  //printf("Grow bb x. target: %3f bb:  %3f  %3f", test->x, dest->a.x, dest->b.x);
  dest->a.x = std::min(test->x, dest->a.x);
  dest->a.y = std::min(test->y, dest->a.y);
  dest->a.z = std::min(test->z, dest->a.z);

  dest->b.x = std::max(test->x, dest->b.x);
  dest->b.y = std::max(test->y, dest->b.y);
  dest->b.z = std::max(test->z, dest->b.z);
  //printf("  end: %3f  %3f\n", dest->a.x, dest->b.x);
}

void RAYAPI GrowAABB(vec3 const *elems, size_t count, AABB *dest)
{
  for (size_t i = 0; i < count; ++i)
  {
    GrowAABB(elems + i, dest);
  }
}

void GrowAABB(Tri const *triangles, size_t tricount, AABB *aabb)
{
  AABB bb;
  for (size_t i = 0; i < tricount; ++i)
  {
    GrowAABB(&triangles[i][0], &bb);
    GrowAABB(&triangles[i][1], &bb);
    GrowAABB(&triangles[i][2], &bb);
  }
}

void GrowAABB(vec3 const *verts, uint3 const *tris, size_t const *which, size_t count, AABB *dest)
{
  for (size_t i = 0; i < count; ++i)
  {
    //printf("index: %i\n", which[i]);
    GrowAABB(verts + tris[which[i]].x, dest);
    GrowAABB(verts + tris[which[i]].y, dest);
    GrowAABB(verts + tris[which[i]].z, dest);
  }
}

void Centroid(Tri const *t, vec3 *c)
{
  c->x = ((*t)[0].x + (*t)[1].x + (*t)[2].x) / 3.f;
  c->y = ((*t)[0].y + (*t)[1].y + (*t)[2].y) / 3.f;
  c->z = ((*t)[0].z + (*t)[1].z + (*t)[2].z) / 3.f;
}

// TODO epsilon
bool PointInAABB(vec3 const *p, AABB const *bb, float e)
{

  
  bool a = p->x - bb->a.x > -e && p->x - bb->b.x < e; //&&
  bool b = p->y - bb->a.y > -e && p->y - bb->b.y < e; //&&
  bool c = p->z - bb->a.z > -e && p->z - bb->b.z < e;
  return a && b && c;
  //return
  //  p->x >= bb->a.x && p->x <= bb->b.x &&
  //  p->y >= bb->a.y && p->y <= bb->b.y &&
  //  p->y >= bb->a.z && p->z <= bb->b.z;
}

bool TriangleInAABBSimple(Tri const *tri, AABB const *bb, float e)
{
  return
    PointInAABB(&(*tri)[0], bb, e) ||
    PointInAABB(&(*tri)[1], bb, e) ||
    PointInAABB(&(*tri)[2], bb, e);
}

void TriFromIndices(vec3 const *verts, uint3 const *is, Tri *dest)
{
  (*dest)[0] = verts[is->x];
  (*dest)[1] = verts[is->y];
  (*dest)[2] = verts[is->z];
}
