#pragma once

#include "raydata.h"

#define TRIS_PER_LEAF 2

struct CentroidBVHLeafNode
{
  AABB aabb;
  size_t count;
  size_t tris[TRIS_PER_LEAF];
};
#ifndef __cplusplus
typedef struct CentroidBVHLeafNode CentroidBVHLeafNode;
#endif

struct CentroidBVHInnerNode
{
  size_t children_index[2];
  AABB aabb;
};
#ifndef __cplusplus
typedef struct CentroidBVHInnerNode CentroidBVHInnerNode;
#endif

union CentroidBVHNodeUnion
{
  CentroidBVHLeafNode leaf;
  CentroidBVHInnerNode inner;
#ifdef __cplusplus
  CentroidBVHNodeUnion() {};
#endif
};
#ifndef __cplusplus
typedef union CentroidBVHNodeUnion CentroidBVHNodeUnion;
#endif

enum CentroidBVHNodeType
{
  cbvhINNER, cbvhLEAF
};

struct CentroidBVHNode
{
  CentroidBVHNodeType type;
  CentroidBVHNodeUnion node;
};
#ifndef __cplusplus
typedef struct CentroidBVHNode CentroidBVHNode;
#endif



/*
  Root is the first node. nodeList[0].  Will be structured like a max-heap.
  A CentroidBVH is build on top of a triangle list. If the triangle list 
  changes or gets deleted then this bvh is useless.
  It does this by using indices into the triangle list.
  This is so it is compatible with opencl.
*/
struct CentroidBVH
{
  CentroidBVHNode *nodeList;
  size_t listSize;
  size_t listCapacity;
};
#ifndef __cplusplus
typedef struct CentroidBVH CentroidBVH;
#endif

void CentroidBVHInit(CentroidBVH *bvh);

// make sure to delete the old one before you build a new one
void CentroidBVHBuild(CentroidBVH *bvh, Tri *triangles, size_t tricount);
