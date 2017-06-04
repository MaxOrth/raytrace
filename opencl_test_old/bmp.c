


#include "bmp.h"
#include <stdio.h>

void printImage(pixel *data, int width, int height, FILE *file)
{
  
  int bitmapcoreheader_header_size = 12;
  int win_header_size = 40;
  int file_header_size = 14;
  int data_start = win_header_size + file_header_size + 4;
  int full_size;
  short swidth = (short)width;
  short sheight = (short)height;
  short one = 1;
  short twentyfour = 24;
  short thirtytwo = 32;
  int rowpadding = (width * sizeof(pixel)) % 4;
  char *padding = "\0\0\0\0";
  int rowIndex;
  int zero = 0;
  int resolution = 0xFFFFFF;
  
  printf("rowpad: %d\n", rowpadding);
  
  full_size = (width * sizeof(pixel) + rowpadding) * height + file_header_size + win_header_size + 4;
  
  /* file header, 14 bytes. */
  fwrite("BM", 2, 1, file);
  fwrite(&full_size, 4, 1, file);
  fwrite("\0\0\0\0", 4, 1, file);
  fwrite(&data_start, 4, 1, file);
  
  /* DiB header, 40 bytes, windows version extension */
  fwrite(&win_header_size, 4, 1, file);
  fwrite(&width, 4, 1, file);
  fwrite(&height, 4, 1, file);
  fwrite(&one, 2, 1, file);
  fwrite(&thirtytwo, 2, 1, file);
  fwrite(&zero, 4, 1, file); // no compression
  fwrite(&zero, 4, 1, file); // dumy zero, no compression
  fwrite(&resolution, 4, 1, file);
  fwrite(&resolution, 4, 1, file);
  fwrite(&zero, 4, 1, file);
  fwrite(&zero, 4, 1, file);
  
  /* color pallette? I probably just messed up */
  fwrite("\0\0\0\0", 4, 1, file);
  
  
  /* Pixle array */
  for (rowIndex = height - 1; rowIndex > -1; rowIndex--)
  {
    fwrite(data + rowIndex * width, sizeof(pixel) * width, 1, file);
    fwrite(padding, rowpadding, 1, file);
  }
  
}



