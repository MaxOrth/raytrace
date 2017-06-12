#pragma once

#ifdef __OPENCL_VERSION__
#include "core/raydata.h"
#define cl_uint unsigned
#define PACK_TIGHTLY __attribute__ ((aligned(1)))
#else
#include "raydata.h"
#include <CL/cl.h>
#define PACK_TIGHTLY
#pragma pack(push, 1)
#endif

#define TRIS_PER_LEAF 2

struct CentroidBVHLeafNode
{
  cl_uint count;
  cl_uint tris[TRIS_PER_LEAF];
} PACK_TIGHTLY;
#ifndef __cplusplus
typedef struct CentroidBVHLeafNode CentroidBVHLeafNode;
#endif

struct CentroidBVHInnerNode
{
  cl_uint children_index[2];
} PACK_TIGHTLY;
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
} PACK_TIGHTLY;
#ifndef __cplusplus
typedef union CentroidBVHNodeUnion CentroidBVHNodeUnion;
#endif

enum CentroidBVHNodeType
{
  cbvhINNER = 0, cbvhLEAF = 1
};

struct CentroidBVHNode
{
  AABB aabb;
  union CentroidBVHNodeUnion node;
  unsigned type;
} PACK_TIGHTLY;
#ifndef __cplusplus
typedef struct CentroidBVHNode CentroidBVHNode;
#endif

#ifndef __OPENCL_VERSION__
#pragma pack(pop)
#endif

/*
  Root is the first node. nodeList[0].  
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
  int depth;
  int tricount;
  int leafcount;
};
#ifndef __cplusplus
typedef struct CentroidBVH CentroidBVH;
#endif

#ifndef __OPENCL_VERSION__
void CentroidBVHInit(CentroidBVH *bvh);
void CentroidBVHFree(CentroidBVH *bvh);

// make sure to delete the old one before you build a new one
void CentroidBVHBuild(CentroidBVH *bvh, uint3 const *triangles, vec3 const *verts, size_t tricount, size_t vertcount);
#endif
