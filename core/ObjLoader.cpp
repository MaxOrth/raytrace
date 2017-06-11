
#include "ObjLoader.h"
#include <stdio.h>

#define log(a) printf("%s\n", a)

void LoadObj(std::istream &file, std::vector<vec3> &verts, std::vector<uint3> &indices)
{
  if (!file)
  {
    printf("stream empty\n");
  }

  std::vector<Tri> model;

  std::vector<vec3> vertices;
  std::vector<vec3> uvws;
  std::vector<uint3> faces;

  /*
  Obj format. Lines starting with:
  v:  vertex coordinate
  vt: texture coordinate
  f:  poly face.  We will only accept triangles.

  Poly face formatting:
  f vert_index/vert_texcoordindex/vertex_normalindex

  we dont support index offsets
  */

  while (file)
  {
    std::string line;
    std::getline(file, line);
    std::string line_type = line.substr(0, line.find_first_of(' '));
    if (line.size())
    {
      if (line_type == "#")
      {
        // is comment
      }
      else if (line_type == "v")
      {
        std::vector<std::string> tokens;
        vec3 vert;
        splitstring(line, ' ', tokens);
        if (tokens.size() < 4)
        {
          log("incomplete vertex record, using (0,0,0)\n");
          vert.x = 0;
          vert.y = 0;
          vert.z = 0;
          vert.w = 1;
        }
        else
        {
          vert.x = atof(tokens[1].c_str());
          vert.y = atof(tokens[2].c_str());
          vert.z = atof(tokens[3].c_str());
          vert.w = 1;
        }
        vertices.push_back(vert);
      }
      else if (line_type == "vt")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);
        vec3 texcoord;
        if (tokens.size() < 3)
        {
          log("incomplete texture coordinate record, using (0,0)");
          texcoord.x = 0;
          texcoord.y = 0;
          texcoord.z = 0;
          texcoord.w = 0;
        }
        else
        {
          texcoord.x = atof(tokens[1].c_str());
          texcoord.y = atof(tokens[2].c_str());
          if (tokens.size() > 3)
            texcoord.z = atof(tokens[3].c_str());
          texcoord.w = 0;
        }
        uvws.push_back(texcoord);
      }
      else if (line_type == "vn")
      {
        // not supported
      }
      else if (line_type == "f")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);
        uint3 face;
        uint3 faceTexIndices;
        if (tokens.size() < 4)
        {
          log("incomplete face record, ignoring");
        }
        else if (tokens.size() > 4)
        {
          log("Quads and ngons not supported, ignoring.");
        }
        else
        {
          std::vector<std::string> v1par;
          std::vector<std::string> v2par;
          std::vector<std::string> v3par;
          splitstring(tokens[1], '/', v1par);
          splitstring(tokens[2], '/', v2par);
          splitstring(tokens[3], '/', v3par);

          if (!v1par.size() || v1par[0].empty() || !v2par.size() || v2par[0].empty() || !v3par.size() || v3par[0].empty())
          {
            log("face record missing vertex index, skipping");
            continue;
          }
          else
          {
            face[0] = static_cast<unsigned>(atoi(v1par[0].c_str()));
            face[1] = static_cast<unsigned>(atoi(v2par[0].c_str()));
            face[2] = static_cast<unsigned>(atoi(v3par[0].c_str()));
          }

          if (v1par.size() > 1 && v2par.size() > 1 && v3par.size() > 1)
          {
            if (!v1par[1].empty() && !v2par[1].empty() && !v3par[1].empty())
            {
              faceTexIndices[0] = static_cast<unsigned>(atoi(v1par[1].c_str()));
              faceTexIndices[1] = static_cast<unsigned>(atoi(v2par[1].c_str()));
              faceTexIndices[2] = static_cast<unsigned>(atoi(v3par[1].c_str()));
            }
          }

          // vertex normals, not supported. will probably be using normal maps instead
          //if (v1par.size() > 2 && v2par.size() > 2 && v3par.size() > 2)
          //{
          //  if (!v1par[2].empty() && !v2par[2].empty() && !v3par[2].empty())
          //  {
          //    faceTexIndices[0] = static_cast<unsigned>(atol(v1par[1].c_str()));
          //    faceTexIndices[1] = static_cast<unsigned>(atol(v2par[1].c_str()));
          //    faceTexIndices[2] = static_cast<unsigned>(atol(v3par[1].c_str()));
          //  }
          //}
          faces.push_back(face);
        }
      }
      else
      {
        // unsuported record. ignoring
      }
    }
  }
  verts = vertices;
  indices = faces;
}

