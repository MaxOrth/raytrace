// Affine.h
// -- points, vectors, transformations in 3D
// cs250 12/16

#ifndef CS250_AFFINE_H
#define CS250_AFFINE_H

#include <cmath>
#include <cassert>


struct Hcoord {
  float x, y, z, w;
  explicit Hcoord(float X=0, float Y=0, float Z=0, float W=0);
  float& operator[](int i) { return *(&x+i); }
  float operator[](int i) const { return *(&x+i); }
  static bool Near(float x, float y) { return std::abs(x-y) < 1e-5f; }
};


struct Point : Hcoord {
  explicit Point(float X=0, float Y=0, float Z=0);
  Point(const Hcoord& v) : Hcoord(v) {  }
};
  

struct Vector : Hcoord {
  explicit Vector(float X=0, float Y=0, float Z=0);
  Vector(const Hcoord& v) : Hcoord(v) {  }
};

typedef float (fmat4x4_t) [4][4];

struct Matrix {
  Hcoord row[4];
  Hcoord& operator[](int i) { return row[i]; }
  const Hcoord& operator[](int i) const { return row[i]; }
  operator fmat4x4_t *() {
    return reinterpret_cast<float (*)[4][4]>(&row[0]);
  };
};


struct Affine : Matrix {
  Affine(void);
  Affine(const Vector& Lx, const Vector& Ly, const Vector& Lz, const Point& D);
  Affine(const Matrix& M) : Matrix(M)                 
      { assert(Hcoord::Near(M[3][0],0) && Hcoord::Near(M[3][1],0)
               && Hcoord::Near(M[3][2],0) && Hcoord::Near(M[3][3],1)); }
};


Hcoord operator+(const Hcoord& u, const Hcoord& v);
Hcoord operator-(const Hcoord& u, const Hcoord& v);
Hcoord operator-(const Hcoord& v);
Hcoord operator*(float r, const Hcoord& v);
Hcoord operator*(const Matrix& A, const Hcoord& v);
Matrix operator*(const Matrix& A, const Matrix& B);
float abs(const Vector& v);
Vector normalize(const Vector& v);
Vector cross(const Vector& u, const Vector& v);
Affine Rot(float t, const Vector& v);
Affine Trans(const Vector& v);
Affine Scale(float r);
Affine Scale(float rx, float ry, float rz);
Affine Inverse(const Affine& A);


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
  return static_cast<T>(Hcoord(u.x / len, u.y / len, u.z / len, W * u.w / len));
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

Affine Inverse(Affine const & a)
{
  //float const (&in) [4][4] = *reinterpret_cast<float const(*)[4][4]>(&a);
  //float (&out) [4][4] = *reinterpret_cast<float(*)[4][4]>(&c);
  //inverse(in, out);
  //return c;
  
  Matrix c;
  float mat3x3[3][3];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      mat3x3[i][j] = a[i][j];
  
  inverse(mat3x3, mat3x3);
  
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      c[i][j] = mat3x3[i][j];
  
  c[3][3] = 1;
  
  Affine t_inv = Trans(Vector(-a[0][3], -a[1][3], -a[2][3]));
  
  return c * t_inv;
}

#undef AFFINE_MATELEM_SIGN

#endif

