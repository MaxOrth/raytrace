#pragma once


#include "raydata.h"


struct Material
{
  cl_float3 color;
  cl_float4 uv; // where uv coords are in their texture buffer. Linear mapping. uv[0][1] => (0,0), uv[2][3] => (1,1)
  float Si;  // index of refraction
  float Rf;  // roughness
  int texId;
  int padding;
};


#ifdef __cplusplus
#include <map>
#include <vector>

struct MtlLib
{
  std::map<std::string, int> indices;
  std::vector<Material> materials;
  
  void InsertMaterial(std::string const &name, Material const &mtl);
  int MaterialIndex(std::string const &name);
};
#else
typedef struct Material Material;
#endif

