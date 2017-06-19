// Affine.h
// -- points, vectors, transformations in 3D
// cs250 12/16

#ifndef CS250_AFFINE_H
#define CS250_AFFINE_H

#include <cmath>
#include <cassert>

typedef float fmat4x4[4][4];
typedef float fmat3x3[3][3];

#define AFFINE_MATELEM_SIGN(i,j) ((i + j) % 2 ? -1 : 1)


template <typename T>
T initlist(T t)
{
  return t;
}

template <typename T, typename B>
float dot(T const & a, B const & b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template <typename T, int W = 0>
float abs(T const & u)
{
  return std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z + W * u.w * u.w);
}

template <typename T, int W = 0>
T normalize(T const & u)
{
  float len = abs<T, W>(u);
  return T{ u.x / len, u.y / len, u.z / len, W * u.w / len };
}

void inline normalize(float const (&in)[4], float(&out)[4])
{
  float len = sqrt(in[0] * in[0] + in[1] * in[1] + in[2] * in[2] + in[3] * in[3]);
  out[0] = in[0] / len;
  out[1] = in[1] / len;
  out[2] = in[2] / len;
  out[3] = in[3];
}

template <unsigned Dim>
void extract(float const (&mat) [Dim][Dim], float (&out) [Dim-1][Dim-1], unsigned row, unsigned col)
{
  // top left paratition
  for (unsigned k = 0; k < row; ++k)
  {
    for (unsigned l = 0; l < col; ++l)
    {
      out[k][l] = mat[k][l];
    }
  }
  // top right partition
  for (unsigned k = 0; k < row; ++k)
  {
    for (unsigned l = col+1; l < Dim; ++l)
    {
      out[k][l-1] = mat[k][l];
    }
  }
  // bottom left partition
  for (unsigned k = row + 1; k < Dim; ++k)
  {
    for (unsigned l = 0; l < col; ++l)
    {
      out[k-1][l] = mat[k][l];
    }
  }
  // bottom right partition
  for (unsigned k = row+1; k < Dim; ++k)
  {
    for (unsigned l = col+1; l < Dim; ++l)
    {
      out[k-1][l-1] = mat[k][l];
    }
  }
}

template <unsigned Dim>
float determinant(float const (&mat) [Dim][Dim])
{
  float sm[Dim-1][Dim-1];
  float det = 0;
  for (unsigned i = 0; i < Dim; ++i)
  {
    extract(mat, sm, 0, i); // copy out matrix values
    det += determinant(sm) * mat[0][i] * AFFINE_MATELEM_SIGN(i, 0);
  }
  return det;
}

template <>
float inline determinant<1>(float const (&mat) [1][1])
{
  return mat[0][0];
}

template <unsigned Dim>
void cofactor(float const (&a) [Dim][Dim], float (&out)[Dim][Dim])
{
  float tmp1[Dim - 1][Dim - 1];
  for (int i = 0; i < Dim; ++i)
  {
    for (int j = 0; j < Dim; ++j)
    {
      int s = AFFINE_MATELEM_SIGN(i,j);
      extract(a, tmp1, i, j);
      out[i][j] = s * determinant(tmp1);
    }
  }
}

template <>
void inline cofactor<1>(float const (&in) [1][1], float (&out) [1][1])
{
  out[0][0] = in[0][0];
}

template <unsigned Dim>
void transpose(float (&in) [Dim][Dim], float (&out) [Dim][Dim])
{
  for (unsigned j = 0; j < Dim; ++j)
  {
    for (unsigned i = j; i < Dim; ++i)
    {
      float tmp = in[i][j]; // safe if in == out
      out[i][j] = in[j][i];
      out[j][i] = tmp;
    }
  }
}

template <unsigned Dim>
void inverse(float const (&in) [Dim][Dim], float (&out)[Dim][Dim])
{
  float cof[Dim][Dim];
  
  cofactor(in, cof);
  
  float det = determinant(in);
  
  det = 1 / det;
  
  for (unsigned i = 0; i < Dim; ++i)
  {
    for (unsigned j = 0; j < Dim; ++j)
    {
      cof[i][j] *= det;
    }
  }
  
  transpose(cof, out);
  
}

void inline rotate3(float (&out)[4][4], float const (&vec)[4], float angle)
{
  float cos_a = cos(angle);
  float sin_a = sin(angle);
  float v[4];
  normalize(vec, v);
  
  out[0][0] = cos_a + v[0] * v[0] * (1 - cos_a);
  out[0][1] = v[0] * v[1] * (1 - cos_a) - v[2] * sin_a;
  out[0][2] = v[0] * v[2] * (1 - cos_a) + v[1] * sin_a;
  out[0][3] = 0;

  out[1][0] = v[0] * v[1] * (1 - cos_a) + v[2] * sin_a;
  out[1][1] = cos_a + v[1] * v[1] * (1 - cos_a);
  out[1][2] = v[1] * v[2] * (1 - cos_a) - v[0] * sin_a;
  out[1][0] = 0;

  out[2][0] = v[0] * v[2] * (1 - cos_a) - v[1] * sin_a;
  out[2][1] = v[1] * v[2] * (1 - cos_a) + v[0] * sin_a;
  out[2][2] = cos_a + v[2] * v[2] * (1 - cos_a);
  out[2][3] = 0;

  out[3][0] = 0;
  out[3][1] = 0;
  out[3][2] = 0;
  out[3][3] = 1;

}

template <unsigned Dim1, unsigned Dim2, unsigned Dim3>
void multiply(float const (&a)[Dim1][Dim2], float const (&b)[Dim3][Dim1], float(&out)[Dim3][Dim2])
{
  float tmp_m[Dim3][Dim2];
  for (unsigned i = 0; i < Dim2; ++i)
  {
    for (unsigned j = 0; j < Dim3; ++j)
    {
      float tmp = 0;
      for (unsigned k = 0; k < Dim1; ++k)
      {
        tmp += a[k][i] * b[j][k];
      }
      tmp_m[j][i] = tmp;
    }
  }
  memcpy(out, tmp_m, sizeof(tmp_m));
}

#undef AFFINE_MATELEM_SIGN

#endif

