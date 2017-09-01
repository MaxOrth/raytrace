#pragma once

#ifdef __OPENCL_VERSION__
#include "core/raydata.h"
#else
#include "raydata.h"
#include <CL/cl.h>
//#pragma pack(push, 16) // not working for some reason
#endif

#define TRIS_PER_LEAF (4)

struct CentroidBVHLeafNode
{
  cl_uint tris[TRIS_PER_LEAF];
  cl_uint count;
  cl_uint padding_1;
  cl_uint padding_2;
  cl_uint padding_3;
};
#ifndef __cplusplus
typedef struct CentroidBVHLeafNode CentroidBVHLeafNode;
#endif

struct CentroidBVHInnerNode
{
  cl_float3 plane_norm; // axis this children were seperated on.  Points tword children[0]
  cl_uint children_index[2];
  cl_uint padding[2]; // pack(16) should fix this but doesnt
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
  cbvhINNER = 0, cbvhLEAF = 1
};

struct CentroidBVHNode
{
  union CentroidBVHNodeUnion node;
  AABB aabb;
  cl_uint type;
  cl_uint parent_index;
  cl_uint padding[2]; // pack(16) should fix this
};
#ifndef __cplusplus
typedef struct CentroidBVHNode CentroidBVHNode;
#endif

#ifndef __OPENCL_VERSION__
//#pragma pack(pop)
#endif

/*
  Root is the first node. nodeList[0].  
  A CentroidBVH is build by referencing a triangle list.
  It does this by using indices into the triangle list.
  This is so it is compatible with opencl.
*/
#ifndef __OPENCL_VERSION__
struct CentroidBVH
{
  CentroidBVHNode *nodeList;
  size_t listSize;
  size_t listCapacity;
  int depth;
  int tricount;
  int leafcount;
};
#endif
#ifndef __cplusplus
typedef struct CentroidBVH CentroidBVH;
#endif

#ifndef __OPENCL_VERSION__
void CentroidBVHInit(CentroidBVH *bvh);
void CentroidBVHFree(CentroidBVH *bvh);

// make sure to delete the old one before you build a new one
void CentroidBVHBuild(CentroidBVH *bvh, cl_uint8 const *triangles, vec3 const *verts, size_t tricount, size_t vertcount);
#endif
