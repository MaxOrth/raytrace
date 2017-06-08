/*
  for choosing a gpu to run on. They have (just different by wgl vs glx name) identical apis i think
    https://www.khronos.org/registry/OpenGL/extensions/AMD/GLX_AMD_gpu_association.txt
    https://www.khronos.org/registry/OpenGL/extensions/AMD/WGL_AMD_gpu_association.txt
*/

/*
  run build.bat
  then run img_main.exe, exit the window, see what platform and device are nvidia
  run img_main a b, where a is platform index and b is device index
*/

#include <CL/cl.h>
#include <CL/cl_gl.h>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include <Windows.h>
#include <stdlib.h>

#include <vector>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <cmath>

#include "clerrortext.h"
#include "clerrorcheck.h"
#include "raydata.h"
#include "ObjLoader.h"
#include "CentroidBVH.h"

#define IMG_X 1920/2
#define IMG_Y 1080/2

void glerrchk_impl(int line)
{
  int err = glGetError();
  if (err != GL_NO_ERROR)
  {
    printf("%i : ", line);
    if (err == GL_INVALID_OPERATION)
      printf("GL_INVALID_OPERATION\n");
    else if (err == GL_INVALID_VALUE)
      printf("GL_INVALID_VALUE\n");
    else if (err == GL_INVALID_ENUM)
      printf("GL_INVALID_ENUM\n");
    else
      printf("Other - %i\n", err);
  }
}

#define glerrchk() glerrchk_impl(__LINE__)

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
  clerrchk(error);
  elog = new char[retsize + 1];
  error = clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, retsize, elog, &retsize);
  clerrchk(error);
  printf("%s\n", elog);
  delete[] elog;
}

GLFWwindow *window;

cl_context context;
cl_command_queue cmdQueue;
cl_kernel kernel;
cl_mem gl_tex;
cl_mem cam_mat;
cl_mem tri_buff;
cl_mem tri_ind_buff;
cl_program program[2];
GLuint texId;
GLuint vao;
GLuint vertBuff;
GLuint shader_prog;
int texunif;

int use_platform = 0;
int use_device = 0;
float timer = 0;

typedef cl_mem t_gltexfunc(cl_context,cl_mem_flags,cl_GLenum,cl_GLuint,cl_GLuint,cl_int *);

// cl_init must go after gl_init
void cl_init(void)
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
    
    clGetPlatformInfo(platformIds[i], CL_PLATFORM_EXTENSIONS, 0, nullptr, &psize);
    std::string exts(psize, ' ');
    clGetPlatformInfo(platformIds[i], CL_PLATFORM_EXTENSIONS, psize, (void *)(exts.data()), nullptr);
    printf("Extension support: %s\n", exts.c_str());
    
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
  
  HGLRC glcontext = wglGetCurrentContext();
  HDC devcontext = wglGetCurrentDC();
  
  const cl_context_properties contextProps[] =
  {
    CL_GL_CONTEXT_KHR, (cl_context_properties)(glcontext),
    CL_WGL_HDC_KHR, (intptr_t)(devcontext),
    CL_CONTEXT_PLATFORM, (intptr_t)(platformIds[use_platform]),
    0,
    0
  };
  cl_int error;
  context = clCreateContext(contextProps, deviceIdMasterList[use_platform].size(), deviceIdMasterList[use_platform].data(), nullptr, nullptr, &error);
  clerrchk(error);
  
  
  std::string psource;
  load_file("render_client/tracekernel.ck", psource);
  
  char const *src = psource.c_str();
  
  program[0] = clCreateProgramWithSource(context, 1, &src, nullptr, &error);
  clerrchk(error);
  error = clBuildProgram(program[0], deviceIdMasterList[use_platform].size(), deviceIdMasterList[use_platform].data(), nullptr, nullptr, nullptr);
  clerrchk(error);
  printprogrambuildinfo(program[0], deviceIdMasterList[use_platform][use_device]);
  kernel = clCreateKernel(program[0], "trace", &error);
  
  clerrchk(error);
  
/*
cl_mem clCreateFromGLTexture(   cl_context context,
                                cl_mem_flags flags,
                                GLenum texture_target,
                                GLint miplevel,
                                GLuint texture,
                                cl_int * errcode_ret)
*/
  //t_gltexfunc *gltexfunc = reinterpret_cast<t_gltexfunc *>(clGetExtensionFunctionAddressForPlatform(platformIds[use_platform], "clCreateFromGLTexture"));
  //gl_tex = gltexfunc(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texId, &error);
  gl_tex = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texId, &error);
  
  clerrchk(error);
  
  float cammat[4][4] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
  
  cam_mat = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * 16, cammat, &error);
  clerrchk(error);
  
  printf("Loading model\n");
  std::vector<vec3> vertices;
  std::vector<uint3> indices;

  LoadObj(std::ifstream("models/bunny.obj"), vertices, indices);

  tri_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(vec3) * vertices.size(), vertices.data(), &error);
  clerrchk(error);
  tri_ind_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint3) * indices.size(), indices.data(), &error);
  clerrchk(error);
  printf("model loaded\n");
  printf("creating bvh\n");
  CentroidBVH bvh;
  CentroidBVHInit(&bvh);
  CentroidBVHBuild(&bvh, vertices.data(), vertices.size());
  printf("bvh created\n");


  unsigned tricount = indices.size();

  error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &gl_tex);
  clerrchk(error);
  error = clSetKernelArg(kernel, 1, sizeof(unsigned), &tricount);
  clerrchk(error);
  error = clSetKernelArg(kernel, 2, sizeof(cl_mem), &tri_buff);
  clerrchk(error);
  error = clSetKernelArg(kernel, 3, sizeof(cl_mem), &tri_ind_buff);
  clerrchk(error);
  error = clSetKernelArg(kernel, 4, sizeof(cl_mem), &cam_mat);
  clerrchk(error);
  
  cmdQueue = clCreateCommandQueue(context, deviceIdMasterList[use_platform][use_device], 0, &error);
  clerrchk(error);
  
}

void gl_init()
{
  glCreateTextures(GL_TEXTURE_2D, 1, &texId);
  glBindTexture(GL_TEXTURE_2D, texId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glerrchk();
  float *zero = new float[IMG_X * IMG_Y * sizeof(float)];
  for (unsigned i = 0; i < IMG_X * IMG_Y; ++i)
  {
    zero[i] = 0;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, IMG_X, IMG_Y, 0, GL_RGBA, GL_FLOAT, zero);
  delete[] zero;
  
  glerrchk();
  
  // create vert buffer
  // do buffer attribs
  glGenVertexArrays(1, &vao);
  glerrchk();
  glBindVertexArray(vao);
  glerrchk();
  glGenBuffers(1, &vertBuff);
  glerrchk();
  glBindBuffer(GL_ARRAY_BUFFER, vertBuff);
  glerrchk();
  float verts[] = {
    -1, -1,  0, 0,
     1,  1,  1, 1,
    -1,  1,  0, 1,
    
    -1, -1,  0, 0,
     1,  1,  1, 1,
     1, -1,  1, 0
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glerrchk();
  glEnableVertexAttribArray(0);
  glerrchk();
  glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);
  glerrchk();
  glBindVertexArray(0);
  glerrchk();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glerrchk();
  
  // make shader program
  GLuint vertpart;
  GLuint fragpart;
  
  std::string vsrc;
  std::string fsrc;
  load_file("render_client/shader.vs", vsrc);
  load_file("render_client/shader.fs", fsrc);
  
  vertpart = glCreateShader(GL_VERTEX_SHADER);
  fragpart = glCreateShader(GL_FRAGMENT_SHADER);
  
  const char *src = vsrc.c_str();
  int srclen = vsrc.size();
  glShaderSource(vertpart, 1, &src, &srclen);
  
  src = fsrc.c_str();
  srclen = fsrc.size();
  glShaderSource(fragpart, 1, &src, &srclen);
  
  glCompileShader(vertpart);
  glCompileShader(fragpart);
  
  shader_prog = glCreateProgram();
  
  glerrchk();
  
  glAttachShader(shader_prog, vertpart);
  glAttachShader(shader_prog, fragpart);
  glLinkProgram(shader_prog);
  glDeleteShader(vertpart);
  glDeleteShader(fragpart);
  
  char cmplog[256];
  GLsizei size;
  glGetShaderInfoLog(vertpart, 255, &size, cmplog);
  printf("%s\n", cmplog);
  glGetShaderInfoLog(fragpart, 255, &size, cmplog);
  printf("%s\n", cmplog);
  
  glerrchk();
  
  glUseProgram(shader_prog);
  glerrchk();
  texunif = glGetUniformLocation(shader_prog, "rayimg");
  glerrchk();
  glActiveTexture(GL_TEXTURE0);
  glerrchk();
  glUniform1i(texunif, 0);
  
  glerrchk();
}

void cl_update()
{
  timer += 1;
  size_t gworksize[3] = {IMG_X, IMG_Y, 1};
  size_t lworksize[3] = {1,1,1};
  size_t offset[3] = {0,0,0};
  int error;
  float cam_z = 1 + std::sin(timer / 100);
  float matrix[4][4] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, -100,
    0, 0, 0, 1
  };
  error = clEnqueueWriteBuffer(cmdQueue, cam_mat, false, 0, sizeof(float) * 16, matrix, 0, nullptr, nullptr);
  clerrchk(error);
  error = clEnqueueAcquireGLObjects(cmdQueue, 1, &gl_tex, 0, nullptr, nullptr);
  clerrchk(error);
  error = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, offset, gworksize, nullptr, 0, nullptr, nullptr);
  clerrchk(error);
  clEnqueueReleaseGLObjects(cmdQueue, 1, &gl_tex, 0, nullptr, nullptr);
  clerrchk(error);
  // sync point. is it needed?
  //clFinish(cmdQueue);
}

void gl_update()
{
  glBindVertexArray(vao);
  glUseProgram(shader_prog);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glfwSwapBuffers(window);
  //glFlush();
}

void cl_free()
{
  clReleaseCommandQueue(cmdQueue);
  clReleaseKernel(kernel);
  clReleaseProgram(program[0]);
  clReleaseMemObject(gl_tex);
  clReleaseMemObject(cam_mat);
  clReleaseMemObject(tri_buff);
  clReleaseMemObject(tri_ind_buff);
  clReleaseContext(context);
}

void gl_free()
{
  glDeleteTextures(1, &texId);
  glDeleteBuffers(1, &vertBuff);
  glDeleteProgram(shader_prog);
  glDeleteVertexArrays(1, &vao);
}

void init() {
  gl_init();
  cl_init();
}

void update()
{
  cl_update();
  gl_update();
}

void release()
{
  cl_free();
  gl_free();
}


int main(int argc, char *argv[])
{
  if (!glfwInit())
  {
    printf("glfw failed\n");
    return 0;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window = glfwCreateWindow(IMG_X, IMG_Y, "rays", nullptr, nullptr);
  if (!window)
  {
    glfwTerminate();
    printf("glfw failed to create window\n");
    return 0;
  }

  glfwMakeContextCurrent(window);
  
  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    printf("glew failed\n");
    printf("%s\n", glewGetErrorString(err));
    glfwTerminate();
    return 0;
  }
  
  if (argc > 1)
    use_platform = atoi(argv[1]);
  if (argc > 2)
    use_device = atoi(argv[2]);
  
  init();
  
  while (!glfwWindowShouldClose(window))
  {
    update();
    glfwPollEvents();
  }
  
  release();
  glfwTerminate();

  return 0;
}
