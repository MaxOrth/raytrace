
#pragma once


#include <vector>

#include "raydata.h"


/*
  Create a space partitioning tree suitable for use in opencl kernels.
  This means all memory must be as contiguous as possible and use 
  indexes for all pointer needs.
  The entire structure needs to be uploadable by one clCreateBuffer call.
  Actual geometry will not be present in this structure;  This will only 
  return indecies into the global geometry buffer.
*/

void CreatePartitionTree(PartitionTree *dest, Model *models, unsigned modelcount, SceneObject *objs, unsigned objcount);

union uf_union
{
  unsigned u;
  float f;
};

struct PartitionTree
{
  uf_union *data;
};
