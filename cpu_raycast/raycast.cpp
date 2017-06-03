

#include "bmp.h"
#include "raydata.h"
#include "ray_tri.h"
#include <stdio.h>

int main0(int argc, const char **argv)
{
  // Single ray test
  // shoots ray down x axis at triangle entirely in plane x = 0
  Ray ray { {-1, 0, 0}, {1, 0, 0} };
  Tri tri { {0, 1, 1}, {0, -1, 0}, {0, 1, -1} };
  vec2 uv {0, 0};
  float t = -1;
  int intersect = intersect_ray_triangle(&tri[0], &tri[1], &tri[2], &ray, &uv, &t);
  printf("Intersect: %i\n", intersect);
  printf("t: %f\n", t);
  printf("uv: %.3f, %.3f\n", uv.u, uv.v);
  return 0;
}

int main1(int argc, const char **argv)
{
  const unsigned W = 360;
  const unsigned H = 20;
  // orthographic projection test
  // device is 100px by 100px
  // projects triangle in device coordinates.  Looking down -z axis (like cannonically oriented camera)
  Tri tri { {W * 0.1, H * 0.1, -200}, {W * 0.9, H * 0.5, -1}, {W * 0.3, H * 0.85, -1} };
  Ray ray;
  ray.dir = {0, 0, -1};
  vec2 uv;
  float t;
  pixel *fb = reinterpret_cast<pixel *>(new char[4 * W * H]);
  for (unsigned j = 0; j < H; ++j)
  {
    for (unsigned i = 0; i < W; ++i)
    {
      pixel *p = fb + i + j * W;
      ray.origin = {static_cast<float>(i), static_cast<float>(j), 0};
      if (intersect_ray_triangle(&tri[0], &tri[1], &tri[2], &ray, &uv, &t))
      {
        (*p)[0] = static_cast<unsigned char>(t);
        (*p)[1] = 0;//static_cast<unsigned char>(uv.u * 255);
        (*p)[2] = 0;//static_cast<unsigned char>(uv.v * 255);
        (*p)[3] = 0;
      }
      else
      {
        (*p)[0] = 0;
        (*p)[1] = 0;
        (*p)[2] = 255;
        (*p)[3] = 0;
      }
    }
  }
  FILE *file = fopen("testimg1.bmp", "w");
  if (file)
  {
    printImage(fb, W, H, file);
    fclose(file);
  }
  else
  {
    printf("Failed to open output file \"testimg.bmp\"\n");
  }
  delete fb;
  return 0;
}

int main2(int argc, const char **argv)
{
  vec3 o  = {0, 0, 0};
  vec3 bx = {1, 0, 0};
  vec3 by = {0, 1, 0};
  vec3 bz = {0, 0, 1};
  Tri *mesh = new Tri[4];
  
  mesh[0][0] = bx, mesh[0][1] = by, mesh[0][2] = bz;
  mesh[1][0] = o, mesh[1][1] = by, mesh[1][2] = bx;
  mesh[2][0] = o, mesh[2][1] = bz, mesh[2][2] = by;
  mesh[3][0] = o, mesh[3][1] = bx, mesh[3][2] = bz;
  
  
  vec3 origin {0,0,0};
  
  float xdim = 1;
  float ydim = 1;
  
  int ypix = 100;
  int xpix = 180;
  
  // [y][x]? meh
  pixel *fb = new pixel[xpix * ypix];
  
  Ray ray;
  vec2 uv;
  float closest = 99999;  // a bug number...
  float dist;
  ray.origin = {0, 0, -1};
  for (int j = 0; j < ypix; ++j)
  {
    for (int i = 0; i < xpix; ++i)
    {
      ray.dir;
      vec3 tmp;
      tmp.x = (i - xpix / 2.f) / xpix;
      tmp.y = (j - ypix / 2.f) / ypix;
      tmp.z = -1;
      SUB(tmp, ray.origin, ray.dir);
      ray.dir = normalize(ray.dir);
      closest = 99999;
      for (unsigned mi = 0; mi < 4; ++mi)
      {
        if (intersect_ray_triangle(&mesh[mi][0], &mesh[mi][1], &mesh[mi][2], &ray, &uv, &dist))
        {
          if (dist < closest)
          {
            closest = dist;
          }
        }
        if (closest < 99999)
        {
          pixel *p = ((fb + i) + j * ypix);
          (*p)[0] = 255;
          (*p)[1] = 255;
          (*p)[2] = 255;
          (*p)[3] = 255;
          printf("I");
        }
        else
        {
          pixel *p = ((fb + i) + j * xpix);
          (*p)[0] = 0;
          (*p)[1] = 0;
          (*p)[2] = 0;
          (*p)[3] = 0;
        }
      }
    }
  }
  
  FILE *file = fopen("testimg2.bmp", "w");
  if (file)
  {
    printImage(fb, xpix, ypix, file);
    fclose(file);
  }
  else
  {
    printf("Failed to open output file \"testimg.bmp\"\n");
  }
  delete fb;
  delete mesh;
  return 0;
}

int main(int argc, const char **argv)
{
  return main1(argc, argv);
}





