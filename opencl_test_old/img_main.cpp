
#include "CL/cl.h"

#include <vector>
#include <stdio.h>
#include <fstream>
#include <sstream>

#include "clerrortext.h"
#include "raydata.h"
#include "bmp.h"

#define IMG_X 12
#define IMG_Y 12

void errchk_impl(cl_int err, int line)
{
  if (err != CL_SUCCESS)
  {
    printf("%i: %s\n", line, getCLErrorString(err));
  }
}

#define errchk(err) errchk_impl(err, __LINE__)

void load_file(std::string fname, std::string &out)
{
  std::ifstream file(fname);
  std::stringstream input;
  input << file.rdbuf();
  out = input.str();
}

void printprogrambuildinfo(cl_program prog, cl_device_id dev)
{
  char *elog;
  size_t retsize;
  cl_int error = clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 0, nullptr, &retsize);
  errchk(error);
  elog = new char[retsize + 1];
  error = clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, retsize, elog, &retsize);
  errchk(error);
  printf("%s\n", elog);
  delete[] elog;
}

int main(void)
{
  cl_uint platformIdCount = 0;
  clGetPlatformIDs(0, nullptr, &platformIdCount);
  
  std::vector<cl_platform_id> platformIds(platformIdCount);
  std::vector<std::vector<cl_device_id>> deviceIdMasterList;
  clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);
  
  printf("Found %i platforms\n", platformIdCount);
  
  for (unsigned i = 0; i < platformIds.size(); ++i)
  {
    size_t psize = 0;
    clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, 0, nullptr, &psize);
    std::string pname(psize, ' ');
    clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, psize, (void *)(pname.data()), nullptr);
    printf("Platform:  %s\n", pname.c_str());
    
    cl_uint deviceIdCount = 0;
    clGetDeviceIDs(platformIds[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &deviceIdCount);
    std::vector<cl_device_id> deviceIds(deviceIdCount);
    clGetDeviceIDs(platformIds[i],  CL_DEVICE_TYPE_ALL, deviceIdCount, deviceIds.data(), nullptr);
    deviceIdMasterList.push_back(deviceIds);
    for (unsigned j = 0; j < deviceIds.size(); ++j)
    {
      size_t size = 0;
      
      clGetDeviceInfo(deviceIds[j], CL_DEVICE_NAME, 0, nullptr, &size);
      std::string name(size, ' ');
      clGetDeviceInfo(deviceIds[j], CL_DEVICE_NAME, size, (void *)name.data(), nullptr);
      
      clGetDeviceInfo(deviceIds[j], CL_DEVICE_OPENCL_C_VERSION, 0, nullptr, &size);
      std::string cname(size, ' ');
      clGetDeviceInfo(deviceIds[j], CL_DEVICE_OPENCL_C_VERSION, size, (void *)cname.data(), nullptr);
      
      printf("  Device:    %s  %s\n", name.c_str(), cname.c_str());
    }
  }
  
  const cl_context_properties contextProps[] =
  {
    CL_CONTEXT_PLATFORM,
    reinterpret_cast<cl_context_properties>(platformIds[1]),
    0,
    0
  };
  cl_int error;
  cl_context context = clCreateContext(contextProps, deviceIdMasterList[1].size(), deviceIdMasterList[1].data(), nullptr, nullptr, &error);
  errchk(error);
  
  cl_image_format format[] = { CL_RGBA, CL_FLOAT };
  cl_mem img = clCreateImage2D(context, CL_MEM_WRITE_ONLY, format, IMG_X, IMG_Y, 0, nullptr, &error);
  
  /*
  cl_image_desc imgdesc;
  imgdesc.image_type = CL_MEM_OBJECT_IMAGE2D;
  imgdesc.image_width = IMG_X;
  imgdesc.image_height = IMG_Y;
  imgdesc.image_depth = 1;
  imgdesc.image_array_size = 1;
  imgdesc.image_row_pitch = 0;
  imgdesc.image_slice_pitch = 0;
  imgdesc.num_mip_levels = 0;
  imgdesc.num_samples = 0;
  
  
  cl_image_format format[] = { CL_RGBA, CL_HALF_FLOAT };
  cl_mem img = clCreateImage(context, CL_MEM_WRITE_ONLY, format, &imgdesc, nullptr, &error);
  */
  errchk(error);
  
  
  std::string psource;
  load_file("tracekernel.c", psource);
  
  char const *src = psource.c_str();
  cl_program program[2];
  
  program[0] = clCreateProgramWithSource(context, 1, &src, nullptr, &error);
  errchk(error);
  error = clBuildProgram(program[0], deviceIdMasterList[1].size(), deviceIdMasterList[1].data(), nullptr, nullptr, nullptr);
  errchk(error);
  printprogrambuildinfo(program[0], deviceIdMasterList[1][1]);
  cl_kernel kernel = clCreateKernel(program[0], "trace", &error);
  
  errchk(error);
  
  error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &img);
  errchk(error);
  
  cl_command_queue cmdQueue = clCreateCommandQueue(context, deviceIdMasterList[1][1], 0, &error);
  errchk(error);
  cl_event evnt;
  size_t gworksize[3] = {IMG_X, IMG_Y, 1};
  size_t lworksize[3] = {1,1,1};
  size_t offset[3] = {0,0,0};
  error = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, offset, gworksize, nullptr, 0, nullptr, &evnt);
  errchk(error);
  size_t origin[3]{0,0,0};
  size_t region[3]{IMG_X,IMG_Y,1};
  pixel *pimg = new pixel[IMG_X*IMG_Y];
  float *fpimg = new float[IMG_X*IMG_Y*4];
  error = clEnqueueReadImage(cmdQueue, img, true, origin, region, 0, 0, fpimg, 0, nullptr, nullptr);
  errchk(error);
  
  clReleaseCommandQueue(cmdQueue);
  clReleaseKernel(kernel);
  clReleaseProgram(program[0]);
  clReleaseMemObject(img);
  
  clReleaseContext(context);
  
  pixel *pix = pimg;
  float *fpix = fpimg;
  for (unsigned i = 0; i < IMG_X * IMG_Y; ++i)
  {
    (*pix)[0] = static_cast<unsigned char>(fpix[0] * 255);
    (*pix)[1] = static_cast<unsigned char>(fpix[1] * 255);
    (*pix)[2] = static_cast<unsigned char>(fpix[2] * 255);
    (*pix)[3] = static_cast<unsigned char>(fpix[3] * 255);
    ++pix;
    fpix += 4;
  }
  //__debugbreak();
  FILE *file = fopen("outimg.bmp", "w");
  printImage(pimg, IMG_X, IMG_Y, file);
  fclose(file);
  
  delete pimg;
  delete fpimg;
  
  return 0;
}




