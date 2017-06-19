#include "CentroidBVH.h"
#include "GeomUtil.h"
#include "raydata.h"
// hehe... cheating
#include <vector>
#include <algorithm>

#include <assert.h>
#include "ray_tri.h"

void CentroidBVHInit(CentroidBVH *bvh)
{
  bvh->listSize = 0;
  bvh->listCapacity = 0;
  bvh->nodeList = nullptr;
  bvh->depth = 0;
  bvh->tricount = 0;
  bvh->leafcount = 0;
}

void CentroidBVHFree(CentroidBVH *bvh)
{
  delete[] bvh->nodeList;
}

namespace
{

  void CBVHCreateLeaf(CentroidBVH *bvh, uint3 const *triangles, vec3 const *verts, size_t const *set, size_t setsize, CentroidBVHNode *node)
  {
    bvh->leafcount++;
    //InitAABB(&verts[triangles[set[0]].x], &node->aabb);
    //GrowAABB(verts, triangles, set, setsize, &node->aabb);
    node->node.leaf.count = setsize;
    for (size_t i = 0; i < setsize; ++i)
    {
      node->node.leaf.tris[i] = set[i];
    }
    //printf("leaf bb: %f %f %f, %f %f %f\n", node->aabb.a.x, node->aabb.a.y, node->aabb.a.z, node->aabb.b.x, node->aabb.b.y, node->aabb.b.z);
  }

  void CBVHRecurseSet(CentroidBVH *bvh, uint3 const *triangles, vec3 const *verts, size_t const *set, size_t setsize, std::vector<CentroidBVHNode> *nodes, size_t parent, unsigned axis, int depthcounter)
  {
    if (setsize <= TRIS_PER_LEAF)
    {
      printf("bounding volume broke...\n");
    }
    else
    {
      bvh->depth = std::max(bvh->depth, depthcounter);
      // calculate barycenter of triangle set (just realized it might be bad but might actually turn out well in practice? 
      //                                          teapot in stadium problem, or more like teapont in space)
      vec3 centroid{ 0,0,0 };
      float total_mass = 0;
      vec3 tri_c;
      float tri_m;
      vec3 *centroids = new vec3[setsize];
      for (size_t i = 0; i < setsize; ++i)
      {
        Tri t{ verts[triangles[i].x], verts[triangles[i].y], verts[triangles[i].z] }; // unaddressable access here. bad obj parser?
        Centroid(&t, &tri_c);
        centroids[i] = tri_c; // TODO calculate centroids and areas all upfront
        vec3 crossres;
        CROSS(t[0] - t[1], t[0] - t[2], crossres);
        tri_m = LENGTH(crossres) / 2.f;
        centroid = (centroid * total_mass + tri_c * tri_m) * (1.f / (total_mass + tri_m));
        total_mass += tri_m;
      }
      // split space: seperate on centroid coordinate
      // create two new lists of triangles
      std::vector<size_t> list1;
      std::vector<size_t> list2;
      unsigned axis_count = axis;
      while (list1.empty() || list2.empty())
      {
        list1.clear();
        list2.clear();
        for (size_t i = 0; i < setsize; ++i)
        {
          if (axis_count % 3 == 0)
          {
            if (centroids[i].x < centroid.x)
            {
              list1.push_back(set[i]);
            }
            else
            {
              list2.push_back(set[i]);
            }
          }
          else if (axis_count % 3 == 1)
          {
            if (centroids[i].y < centroid.y)
            {
              list1.push_back(set[i]);
            }
            else
            {
              list2.push_back(set[i]);
            }
          }
          else
          {
            if (centroids[i].z < centroid.z)
            {
              list1.push_back(set[i]);
            }
            else
            {
              list2.push_back(set[i]);
            }

          }

        }
        axis_count++;
      }
      delete[] centroids;
      // calculate bbs of sets
      //printf("child 1\n");
      AABB n1bb;
      InitAABB(&verts[triangles[list1[0]].x], &n1bb);
      GrowAABB(verts, triangles, list1.data(), list1.size(), &n1bb);
      MinSizeAABB(&n1bb);

      for (unsigned i = 0; i < list1.size(); ++i)
      {
        if (
          !PointInAABB(&verts[triangles[list1[i]].x], &n1bb) &&
          !PointInAABB(&verts[triangles[list1[i]].y], &n1bb) &&
          !PointInAABB(&verts[triangles[list1[i]].z], &n1bb)
        )
        {
          assert(0);
        }
      }
      //printf("child 2\n");
      AABB n2bb;
      InitAABB(&verts[triangles[list2[0]].x], &n2bb);
      GrowAABB(verts, triangles, list2.data(), list2.size(), &n2bb);
      MinSizeAABB(&n2bb);
      // second recursion through, index 0: f5 f5 step_into_pointinaabb
      for (unsigned i = 0; i < list2.size(); ++i)
      {
        if (
          !PointInAABB(&verts[triangles[list2[i]].x], &n2bb) &&
          !PointInAABB(&verts[triangles[list2[i]].y], &n2bb) &&
          !PointInAABB(&verts[triangles[list2[i]].z], &n2bb)
          )
        {
          assert(0);
        }
      }

      size_t n1i = nodes->size();
      CentroidBVHNode node1;
      (*nodes)[parent].node.inner.children_index[0] = n1i;
      node1.aabb = n1bb;

      if (list1.size() <= TRIS_PER_LEAF)
      {
        node1.type = cbvhLEAF;
        CBVHCreateLeaf(bvh, triangles, verts, list1.data(), list1.size(), &node1);
        nodes->push_back(node1);
        //printf("LEAF  node at index %3i with parent %3i", n1i, parent);
        //printf("  bb: %f %f %f, %f %f %f\n", n1bb.a.x, n1bb.a.y, n1bb.a.z, n1bb.b.x, n1bb.b.y, n1bb.b.z);
      }
      else
      {
        node1.type = cbvhINNER;
        (*nodes)[parent].node.inner.children_index[0] = n1i;
        nodes->push_back(node1);
        //printf("INNER node at index %3i with parent %3i", n1i, parent);
        //printf("  bb: %f %f %f, %f %f %f\n", n1bb.a.x, n1bb.a.y, n1bb.a.z, n1bb.b.x, n1bb.b.y, n1bb.b.z);
        CBVHRecurseSet(bvh, triangles, verts, list1.data(), list1.size(), nodes, n1i, axis_count, depthcounter + 1);
      }

      size_t n2i = nodes->size();
      CentroidBVHNode node2;
      (*nodes)[parent].node.inner.children_index[1] = n2i;
      node2.aabb = n2bb;

      if (list2.size() <= TRIS_PER_LEAF)
      {
        node2.type = cbvhLEAF;
        CBVHCreateLeaf(bvh, triangles, verts, list2.data(), list2.size(), &node2);
        nodes->push_back(node2);
        //printf("LEAF  node at index %3i with parent %3i", n2i, parent);
        //printf("  bb: %f %f %f, %f %f %f\n", n2bb.a.x, n2bb.a.y, n2bb.a.z, n2bb.b.x, n2bb.b.y, n2bb.b.z);
      }
      else
      {
        node2.type = cbvhINNER;
        nodes->push_back(node2);
        //printf("INNER node at index %3i with parent %3i", n2i, parent);
        //printf("  bb: %f %f %f, %f %f %f\n", n2bb.a.x, n2bb.a.y, n2bb.a.z, n2bb.b.x, n2bb.b.y, n2bb.b.z);
        CBVHRecurseSet(bvh, triangles, verts, list2.data(), list2.size(), nodes, n2i, axis_count, depthcounter + 1);
      }
    }
  }
}

void CentroidBVHBuild(CentroidBVH *bvh, uint3 const *triangles, vec3 const *verts, size_t tricount, size_t vertcount)
{
  if (tricount == 0)
    return;

  bvh->tricount = tricount;

  std::vector<CentroidBVHNode> nodeList;
  
  // create root node
  CentroidBVHNode root;
  root.type = cbvhINNER;
  InitAABB(verts, &root.aabb);
  GrowAABB(verts, vertcount, &root.aabb);
  MinSizeAABB(&root.aabb);
  size_t *initialSet = new size_t[tricount];
  for (size_t i = 0; i < tricount; ++i)
  {
    initialSet[i] = i;
  }
  nodeList.push_back(root);
  CBVHRecurseSet(bvh, triangles, verts, initialSet, tricount, &nodeList, 0, 0, 0);
  delete[] initialSet;
  bvh->nodeList = new CentroidBVHNode[nodeList.size()];
  memcpy(bvh->nodeList, nodeList.data(), sizeof(nodeList[0]) * nodeList.size());
  bvh->listSize = nodeList.size();
}
