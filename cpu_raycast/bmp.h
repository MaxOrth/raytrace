
#ifndef BMPHEADER
#define BMPHEADER

#define MS_HEADER true


  
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char pixel[4];

#ifdef __cplusplus
extern "C" 
{
#endif
  
  /* do not have padding in the pixel array */
  void printImage(pixel *data, int width, int height, FILE *file);

#ifdef __cplusplus
}
#endif



#endif
