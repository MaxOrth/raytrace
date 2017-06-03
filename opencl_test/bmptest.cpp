
#include "bmp.h"

#include <string.h>

int main(void)
{
  pixel *img = new pixel[256];
  memset((unsigned char *)img, 255, 256);
  for (unsigned i = 0; i < 256; ++i)
  {
    img[i][0] = static_cast<unsigned char>(i);
    img[i][1] = 0;
    img[i][2] = 0;
    img[i][3] = 0;
  }
  
  FILE *file = fopen("bmpout.bmp", "w");
  printImage(img, 256, 1, file);
  fclose(file);
  return 0;
}

