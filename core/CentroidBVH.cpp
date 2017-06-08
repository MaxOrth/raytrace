#include "CentroidBVH.h"
#include "GeomUtil.h"
#include "raydata.h"
// hehe... cheating
#include <vector>

void CentroidBVHInit(CentroidBVH *bvh)
{
  bvh->listSize = 0;
  bvh->listCapacity = 0;
bvh->nodeList = nullptr;
}

namespace
{

  void CBVHCreateLeaf(CentroidBVH *bvh, Tri *triangles, size_t *set, size_t setsize, CentroidBVHNode *node)
  {
    InitAABB(triangles, &node->node.leaf.aabb);
    GrowAABB(triangles, setsize, &node->node.leaf.aabb);
    node->node.leaf.count = setsize;
    for (size_t i = 0; i < setsize; ++i)
    {
      node->node.leaf.tris[i] = set[i];
    }
  }

  void CBVHRecurseSet(CentroidBVH *bvh, Tri *triangles, size_t *set, size_t setsize, std::vector<CentroidBVHNode> *nodes, size_t parent, unsigned axis)
  {
    if (setsize <= TRIS_PER_LEAF)
    {
      printf("bounding volume broke...\n");
    }
    else
    {
      // calculate barycenter of triangle set (just realized it might be bad but might actually turn out well in practice? 
      //                                          teapot in stadium problem, or more like teapont in space)
      vec3 centroid{ 0,0,0 };
      float total_mass = 0;
      vec3 tri_c;
      float tri_m;

      for (size_t i = 0; i < setsize; ++i)
      {
        Tri *t = triangles + set[i];
        Centroid(t, &tri_c);
        vec3 crossres;
        CROSS((*t)[0] - (*t)[1], (*t)[0] - (*t)[2], crossres);
        tri_m = LENGTH(crossres) / 2.f;
        centroid = (centroid * total_mass + tri_c * tri_m) * (1.f / (total_mass + tri_m));
        total_mass += tri_m;
      }
      // split space: create new aabbs
      AABB n1((*nodes)[parent].node.inner.aabb);
      AABB n2(n1);
      if (axis % 3 == 0)
      {
        n1.a.x = centroid.x;
        n2.b.x = centroid.x;
      }
      else if (axis % 3 == 1)
      {
        n1.a.y = centroid.y;
        n2.b.y = centroid.y;
      }
      else
      {
        n1.a.z = centroid.z;
        n2.b.z = centroid.z;
      }
      // create two new lists of triangles
      std::vector<size_t> list1;
      std::vector<size_t> list2;
      for (size_t i = 0; i < setsize; ++i)
      {
        Tri *t = triangles + set[i];
        if (TriangleInAABBSimple(t, &n1))
        {
          list1.push_back(set[i]);
        }
        if (TriangleInAABBSimple(t, &n2))
        {
          list2.push_back(set[i]);
        }
      }
      size_t n1i = nodes->size();
      CentroidBVHNode node1;
      (*nodes)[parent].node.inner.children_index[0] = n1i;

      size_t n2i = nodes->size();
      CentroidBVHNode node2;
      (*nodes)[parent].node.inner.children_index[1] = n2i;

      if (list1.size() <= TRIS_PER_LEAF)
      {
        node1.type = cbvhLEAF;
        CBVHCreateLeaf(bvh, triangles, list1.data(), list1.size(), &node1);
        nodes->push_back(node1);
      }
      else
      {
        node1.type = cbvhINNER;
        node1.node.inner.aabb = n1;
        (*nodes)[parent].node.inner.children_index[0] = n1i;
        nodes->push_back(node1);
        CBVHRecurseSet(bvh, triangles, list1.data(), list1.size(), nodes, n1i, axis + 1);
      }
      if (list2.size() <= TRIS_PER_LEAF)
      {
        
        node2.type = cbvhINNER;
        node2.node.inner.aabb = n2;
        nodes->push_back(node2);
        CBVHRecurseSet(bvh, triangles, list2.data(), list2.size(), nodes, n2i, axis + 1);
      }
      else
      {
        node2.type = cbvhLEAF;
        CBVHCreateLeaf(bvh, triangles, list2.data(), list2.size(), &node2);
        nodes->push_back(node2);
      }
    }
  }
}

void CentroidBVHBuild(CentroidBVH *bvh, Tri *triangles, size_t tricount)
{
  if (tricount == 0)
    return;

  std::vector<CentroidBVHNode> nodeList;
  
  // create root node
  CentroidBVHNode root;
  root.type = cbvhINNER;
  InitAABB(triangles, &root.node.inner.aabb);
  GrowAABB(triangles, tricount, &root.node.inner.aabb);
  size_t *initialSet = new size_t[tricount];
  for (size_t i = 0; i < tricount; ++i)
  {
    initialSet[i] = i;
  }
  CBVHRecurseSet(bvh, triangles, initialSet, tricount, &nodeList, 0, 0);
}
