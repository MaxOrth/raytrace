#pragma once

#include <string>
#include <sstream>
#include <istream>
#include <vector>

#include "raydata.h"
#include "MtlLib.h"


void LoadObj(std::istream &file, std::vector<vec3> &verts, std::vector<uint3> &indices, MtlLib &mtllib, std::string folder = "./");

void LoadObjMaterials(std::istream &file, MtlLib &matlib);
