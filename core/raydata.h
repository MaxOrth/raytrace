/*
  data primitives
*/

#pragma once

#ifdef __OPENCL_VERSION__ 
typedef float3 vec3;

#define VEC3 vec3

#else

#define VEC3 struct vec3
  
struct vec3
{
  float x;
  float y;
  float z;
#ifdef __cplusplus
  float &operator[](int i) { return (&x)[i]; };
  float const &operator[](int i) const { return (&x)[i]; };
#endif
};

VEC3 normalize(VEC3 in);

#endif

struct Ray
{
  VEC3 origin;
  VEC3 dir;
};

struct vec2
{
  float u;
  float v;
};

typedef VEC3 Tri[3];

struct AABB
{
  VEC3 a;
  VEC3 b;
};

struct Model
{
  struct AABB vol;
  unsigned triangles[3];
};

struct SceneObject
{
  VEC3 mat[3];
  unsigned mi;  // index into model list
};

#define EPSILON (0.000001)

#ifndef __OPENCL_VERSION__

#define SUB(a,b,out) do {\
(out).x = (a).x - (b).x;   \
(out).y = (a).y - (b).y;   \
(out).z = (a).z - (b).z; } while (0)


  
#define DOT(a,b) ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)

#define CROSS(a,b,out) do {    \
(out).x = (a).y * (b).z - (a).z * (b).y; \
(out).y = (a).z * (b).x - (a).x * (b).z; \
(out).z = (a).x * (b).y - (a).y * (b).x; \
} while (0)

#else

#define SUB(a,b,out) (out) = (a) - (b)
#define CROSS(a,b) cross((a), (b))
#define DOT(a,b) dot((a), (b))

#endif
  
#define NEAR_ZERO(a) ((a) > -EPSILON && (a) < EPSILON)


#ifdef __cplusplus
#ifndef __OPENCL_VERSION__

VEC3 operator+(VEC3 const &l, VEC3 const &r)
{
  return { l.x + r.x, l.y + r.y, l.z + r.z };
}

VEC3 operator-(VEC3 const &l, VEC3 const &r)
{
  return { l.x - r.x, l.y - r.y, l.z - r.z };
}

VEC3 operator*(VEC3 const &l, float r)
{
  return { l.x * r, l.y * r, l.z * r };
}

VEC3 operator*(float l, VEC3 const &r)
{
  return { r.x * l, r.y * l, r.z * l };
}

inline VEC3 mult(VEC3 const (&mat)[3], VEC3 const &v)
{
  VEC3 r;
  r.x = mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2];
  r.y = mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2];
  r.z = mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2];
}

#endif
#endif





