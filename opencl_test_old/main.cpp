
#include "CL/cl.h"

#include <vector>
#include <stdio.h>
#include <fstream>
#include <sstream>

#include "clerrortext.h"

#define TESTDATASIZE 5

void errchk(cl_int err)
{
  if (err != CL_SUCCESS)
  {
    printf("%s\n", getCLErrorString(err));
  }
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
    reinterpret_cast<cl_context_properties>(platformIds[0]),
    0,
    0
  };
  cl_int error;
  cl_context context = clCreateContext(contextProps, deviceIdMasterList[0].size(), deviceIdMasterList[0].data(), nullptr, nullptr, &error);
  errchk(error);
  
  float *f = new float[TESTDATASIZE];
  for (float *fp = f; fp < f + TESTDATASIZE; ++fp)
  {
    *fp = fp - f;
  }
  float *f2 = new float[TESTDATASIZE];
  cl_mem buff1 = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * TESTDATASIZE, f, &error);
  errchk(error);
  cl_mem buff2 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * TESTDATASIZE, nullptr, &error);
  
  errchk(error);
  
  std::ifstream file("kernal.c");
  std::stringstream input;
  input << file.rdbuf();
  std::string psource(input.str());
  
  char const *src = psource.c_str();
  cl_program program = clCreateProgramWithSource(context, 1, &src, nullptr, &error);
  errchk(error);
  error = clBuildProgram(program, deviceIdMasterList[0].size(), deviceIdMasterList[0].data(), nullptr, nullptr, nullptr);
  errchk(error);
  cl_kernel kernel = clCreateKernel(program, "doublevalue", &error);
  
  errchk(error);
  
  error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buff1);
  errchk(error);
  error = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buff2);
  errchk(error);
  
  cl_command_queue cmdQueue = clCreateCommandQueue(context, deviceIdMasterList[0][0], 0, &error);
  errchk(error);
  cl_event evnt;
  size_t gworksize = TESTDATASIZE;
  error = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, nullptr, &gworksize, nullptr, 0, nullptr, &evnt);
  errchk(error);
  error = clEnqueueReadBuffer(cmdQueue, buff2, true, 0, TESTDATASIZE * sizeof(float), f2, 1, &evnt, nullptr);
  errchk(error);
  
  clReleaseCommandQueue(cmdQueue);
  clReleaseProgram(program);
  clReleaseKernel(kernel);
  clReleaseMemObject(buff1);
  clReleaseMemObject(buff2);
  
  clReleaseContext(context);
  
  for (int i = 0; i < TESTDATASIZE; ++i)
  {
    printf("%f   ->   %f\n", f[i], f2[i]);
  }
  
  delete[] f;
  delete[] f2;
  
  return 0;
}




