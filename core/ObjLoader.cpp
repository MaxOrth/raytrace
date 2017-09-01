
#include "ObjLoader.h"
#include <fstream>
#include <stdio.h>

#define log(a) printf("%s\n", a)


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
      else
      {
        result.push_back("");
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

void LoadObj(std::istream &file, std::vector<vec3> &verts, std::vector<cl_float4> &norms, std::vector<cl_uint8> &indices, MtlLib &mtllib, std::string folder)
{
  if (!file)
  {
    printf("stream empty\n");
    return;
  }

  std::vector<Tri> model;
  std::vector<vec3> vertices;
  std::vector<vec3> uvws;
  std::vector<cl_float4> normals;
  std::vector<cl_uint8> faces;
  int curr_mat_index = 0;
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
          vert.x = static_cast<float>(atof(tokens[1].c_str()));
          vert.y = static_cast<float>(atof(tokens[2].c_str()));
          vert.z = static_cast<float>(atof(tokens[3].c_str()));
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
          texcoord.x = static_cast<float>(atof(tokens[1].c_str()));
          texcoord.y = static_cast<float>(atof(tokens[2].c_str()));
          if (tokens.size() > 3)
            texcoord.z = static_cast<float>(atof(tokens[3].c_str()));
          texcoord.w = 0;
        }
        uvws.push_back(texcoord);
      }
      else if (line_type == "vn")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);
        cl_float4 nvec;
        if (tokens.size() < 4)
        {
          log("incomplete texture coordinate record, using <1,0,0>");
          nvec.x = 1;
          nvec.y = 0;
          nvec.z = 0;
          nvec.w = 0;
        }
        else
        {
          nvec.x = static_cast<float>(atof(tokens[1].c_str()));
          nvec.y = static_cast<float>(atof(tokens[2].c_str()));
          nvec.z = static_cast<float>(atof(tokens[3].c_str()));
          nvec.w = 0;
        }
        normals.push_back(nvec);
      }
      else if (line_type == "f")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);
        cl_uint8 face = { 0,0,0,0,0,0,0,0 };
        //uint3 faceTexIndices;
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
            face.s[0] = static_cast<unsigned>(atoi(v1par[0].c_str())) - 1;
            face.s[1] = static_cast<unsigned>(atoi(v2par[0].c_str())) - 1;
            face.s[2] = static_cast<unsigned>(atoi(v3par[0].c_str())) - 1;
            face.s[3] = curr_mat_index;
          }

          // TODO use these texture indices
          //if (v1par.size() > 1 && v2par.size() > 1 && v3par.size() > 1)
          //{
          //  if (!v1par[1].empty() && !v2par[1].empty() && !v3par[1].empty())
          //  {
          //    faceTexIndices[0] = static_cast<unsigned>(atoi(v1par[1].c_str())) - 1;
          //    faceTexIndices[1] = static_cast<unsigned>(atoi(v2par[1].c_str())) - 1;
          //    faceTexIndices[2] = static_cast<unsigned>(atoi(v3par[1].c_str())) - 1;
          //  }
          //}

          // vertex normals.
          if (v1par.size() > 2 && v2par.size() > 2 && v3par.size() > 2)
          {
            if (!v1par[2].empty() && !v2par[2].empty() && !v3par[2].empty())
            {
              face.s[4] = static_cast<unsigned>(atol(v1par[2].c_str())) - 1;
              face.s[5] = static_cast<unsigned>(atol(v2par[2].c_str())) - 1;
              face.s[6] = static_cast<unsigned>(atol(v3par[2].c_str())) - 1;
            }
          }
          faces.push_back(face);
        }
      }
      else if (line_type == "mtllib")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);

        LoadObjMaterials(std::ifstream(folder + tokens[1]), mtllib);
      }
      else if (line_type == "usemtl")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);

        curr_mat_index = mtllib.MaterialIndex(tokens[1]);
      }
      else
      {
        // unsuported record. ignoring
      }
    }
  }
  verts = vertices;
  indices = faces;
  norms = normals;
}

void ResetMaterial(Material &mat)
{
  mat.color.s[0] = 0;
  mat.color.s[1] = 0;
  mat.color.s[2] = 0;
  mat.color.s[3] = 0;

  mat.uv.s[0] = 0;
  mat.uv.s[1] = 0;
  mat.uv.s[2] = 0;
  mat.uv.s[3] = 0;


  mat.Si = 1;
  mat.Rf = 1;
  mat.texId = 0;
}

void LoadObjMaterials(std::istream &file, MtlLib &matlib)
{
  if (!file)
  {
    return;
  }

  Material mat;
  std::string name;

  while (file)
  {
    std::string line;
    std::getline(file, line);
    std::string line_type = line.substr(0, line.find_first_of(' '));
    if (line.size())
    {
      if (line_type == "newmtl")
      {
        if (!name.size())
        {
          ; // do nothing
        }
        else if (matlib.indices.find(name) != matlib.indices.end())
        {
          log("Material already defined, using previous definition");
        }
        else
        {
          matlib.InsertMaterial(name, mat);
        }
        ResetMaterial(mat);
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);
        name = tokens[1];
      }
      else if (line_type == "color")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);
        mat.color.s[0] = static_cast<float>(atof(tokens[1].c_str()));
        mat.color.s[1] = static_cast<float>(atof(tokens[2].c_str()));
        mat.color.s[2] = static_cast<float>(atof(tokens[3].c_str()));
      }
      else if (line_type == "rough")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);
        mat.Rf = static_cast<float>(atof(tokens[1].c_str()));
      }
      else if (line_type == "ind_ref")
      {
        std::vector<std::string> tokens;
        splitstring(line, ' ', tokens);
        mat.Si = static_cast<float>(atof(tokens[1].c_str()));
      }

      //else if (line_type == "Ka")
      //{
      //  // https://en.wikipedia.org/wiki/Specular_highlight#Beckmann_distribution
      //  // ambient.  
      //  // aparently reflectivity is partially exported through ambient in blender
      //  // interpreted as intensity of the reflection, not roughness (% light that gets reflected by mirror vs refracted or absorbed and re-emitted
      //
      //}
      //else if (line_type == "d")
      //{
      //  // "dissolved". oposite of transparency. d = 1 - tr
      //}
      //else if (line_type == "Tr")
      //{
      //  // transparency.
      //}
      //else if (line_type == "Kd")
      //{
      //  // diffuse
      //  std::vector<std::string> tokens;
      //  splitstring(line, ' ', tokens);
      //  mat.color.s[0] = static_cast<float>(atof(tokens[1].c_str()));
      //  mat.color.s[1] = static_cast<float>(atof(tokens[2].c_str()));
      //  mat.color.s[2] = static_cast<float>(atof(tokens[3].c_str()));
      //}
      //else if (line_type == "Ks")
      //{
      //  // specular color
      //}
      //else if (line_type == "Ns")
      //{
      //  // specular power
      //}
      //else if (line_type == "Ni")
      //{
      //  // index of refraction
      //  std::vector<std::string> tokens;
      //  splitstring(line, ' ', tokens);
      //  mat.N = static_cast<float>(atof(tokens[1].c_str()));
      //}
      



    }

  }


}

