/*
  data primitives
*/

#pragma once

#ifdef __OPENCL_VERSION__ 
typedef float3 vec3;
#else

struct vec3
{
  float x;
  float y;
  float z;
#ifdef __cplusplus
  float *operator[](int i) { return (&x)[i] };
#endif
};
#endif

struct Ray
{
  struct vec3 origin;
  struct vec3 dir;
};

struct vec2
{
  float u;
  float v;
};

typedef struct vec3 Tri[3];

struct AABB
{
  vec3 a;
  vec3 b;
};

struct Model
{
  struct AABB vol;
  unsigned triangles[3];
};

struct SceneObject
{
  vec3[3] mat;
  unsigned mi;  // index into model list
};

#define EPSILON (0.000001)

#define SUB(a,b,out) do {\
out.x = (a).x - (b).x;   \
out.y = (a).y - (b).y;   \
out.z = (a).z - (b).z; } while (0)

#define DOT(a,b) ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)

#define CROSS(a,b,out) do {    \
(out).x = (a).y * (b).z - (a).z * (b).y; \
(out).y = (a).z * (b).x - (a).x * (b).z; \
(out).z = (a).x * (b).y - (a).y * (b).x; \
} while (0)
  
struct vec3 normalize(struct vec3 in);
  
#define NEAR_ZERO(a) ((a) > -EPSILON && (a) < EPSILON)


#ifdef __cplusplus
#ifndef __OPENCL_VERSION__

vec3 operator+(vec3 const &l, vec3 const &r)
{
  return vec3{ l.x + r.x, l.y + r.y, l.z + r.z };
}

vec3 operator-(vec3 const &l, vec3 const &r)
{
  return vec3{ l.x - r.x, l.y - r.y, l.z - r.z };
}

vec3 operator*(vec3 const &l, float r)
{
  return vec3{ l.x * r, l.y * r, l.z * r };
}

vec3 operator*(float l, vec3 const &r)
{
  return vec3{ r.x * l, r.y * l, r.z * l };
}

inline vec3 mult(vec3 const (&mat)[3], vec3 const &v)
{
  vec3 r;
  r.x = mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2];
  r.y = mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2];
  r.Z = mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2];
}

#endif
#endif





