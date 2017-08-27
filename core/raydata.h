/*
  data primitives
*/
#ifndef __OPENCL_VERSION__
#include <CL/cl.h>
#endif

#define RAYAPI __stdcall

#pragma once

#ifdef __OPENCL_VERSION__ 
//#define PACK_TIGHTLY __attribute__ ((aligned(1)))
typedef float cl_float;
typedef unsigned cl_uint;
typedef float3 vec3;
typedef float4 cl_float4;
typedef float3 cl_float3;
typedef float2 cl_float2;
#define VEC3 vec3

#else
//#define PACK_TIGHTLY

#define VEC3 struct vec3

struct uint3
{
  cl_uint x;
  cl_uint y;
  cl_uint z;
  cl_uint w; // same as vec3
#ifdef __cplusplus
  cl_uint &operator[](int i) { return (&x)[i]; };
  cl_uint const &operator[](int i) const { return (&x)[i]; };
  uint3() : w(0) {};
#endif
};

struct vec3
{
  cl_float x;
  cl_float y;
  cl_float z;
  cl_float w; // lol. see OpenCL 6.1.5: Alignment of Types
#ifdef __cplusplus
  cl_float &operator[](int i) { return (&x)[i]; };
  cl_float const &operator[](int i) const { return (&x)[i]; };
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
  cl_float u;
  cl_float v;
};

typedef VEC3 Tri[3];

struct RayIntersection
{
  cl_float4 origin;
  cl_float4 direction;
  cl_float4 normal;
  cl_float2 uv;
  unsigned triangle_id;
  unsigned p1;
};
#ifndef __cplusplus
typedef struct RayIntersection RayIntersection;
#endif

/*
  a is "min", b is "max".
  Using it differently will make the GeomUtil functions break.
*/
struct AABB
{
  VEC3 a;
  VEC3 b;
#ifdef __cplusplus
  AABB() : a { 0, 0, 0 }, b { 0, 0, 0 } {};
#endif
};

#ifndef __cplusplus
typedef struct AABB AABB;
#endif

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

#define LENGTH(a) (sqrt((a).x*(a).x + (a).y*(a).y + (a).z*(a).z))

#else

#define SUB(a,b,out) (out) = (a) - (b)
#define CROSS(a,b) cross((a), (b))
#define DOT(a,b) dot((a), (b))

#endif
  
#define NEAR_ZERO(a) ((a) > -EPSILON && (a) < EPSILON)


#ifdef __cplusplus
#ifndef __OPENCL_VERSION__

inline VEC3 operator+(VEC3 const &l, VEC3 const &r)
{
  return { l.x + r.x, l.y + r.y, l.z + r.z };
}

inline VEC3 operator-(VEC3 const &l, VEC3 const &r)
{
  return { l.x - r.x, l.y - r.y, l.z - r.z };
}

inline VEC3 operator*(VEC3 const &l, float r)
{
  return { l.x * r, l.y * r, l.z * r };
}

inline VEC3 operator*(float l, VEC3 const &r)
{
  return { r.x * l, r.y * l, r.z * l };
}

inline VEC3 mult(float const (&mat)[4][4], VEC3 const &v)
{
  VEC3 r;
  r.x = mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2];
  r.y = mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2];
  r.z = mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2];
  r.z = mat[3][0] * v[0] + mat[3][1] * v[1] + mat[3][2] * v[2];
  return r;
}

#endif
#endif





