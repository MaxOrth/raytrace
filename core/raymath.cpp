
#include "raydata.h"
#include <math.h>


struct vec3 normalize(struct vec3 in)
{
  float lensq = DOT(in,in);
  float len = sqrt(lensq);
  struct vec3 ret;
  ret.x = in.x / len;
  ret.y = in.y / len;
  ret.z = in.z / len;
  return ret;
}



