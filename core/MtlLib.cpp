#include "MtlLib.h"

void MtlLib::InsertMaterial(std::string const &name, Material const &mtl)
{
  indices[name] = materials.size();
  materials.push_back(mtl);
}

int MtlLib::MaterialIndex(std::string const &name)
{
  return indices[name];
}

