#pragma once

#include <string>
#include <sstream>
#include <istream>
#include <vector>

#include "raydata.h"

// TODO move to utilities
namespace
{
  void splitstring(std::string const &str, char token, std::vector<std::string> &out)
  {
    std::vector<std::string> result;
    size_t token_location = 0;
    size_t substr_begin = 0;
    while ((token_location = str.find(token, substr_begin)) != std::string::npos)
    {
      if (token_location != substr_begin)
      {
        result.push_back(str.substr(substr_begin, token_location - substr_begin));
      }
      substr_begin = token_location + 1;
    }
    if (substr_begin < str.size())
    {
      result.push_back(str.substr(substr_begin));
    }
    out = result;
  }
}

void LoadObj(std::istream &file, std::vector<vec3> &verts, std::vector<uint3> &indices);


