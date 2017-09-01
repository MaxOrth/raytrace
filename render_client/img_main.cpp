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

// cause I dont know how to get the gl context in linux
#ifndef _WIN32
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#endif
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
#include "Affine.h"

#define IMG_X 512
#define IMG_Y 512

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
cl_mem gl_tex[2];
cl_mem cam_mat;
cl_mem tri_buff;
cl_mem tri_ind_buff;
cl_mem accel_buff;
cl_mem cam_buff;
cl_mem mtl_buff;
cl_mem normal_buff;
cl_mem ray_isc_buff[2]; // need 2, one for reading and one for writing to. will read from 0 and write to 1
cl_program program[2];
GLuint texId[2];
GLuint vao;
GLuint vertBuff;
GLuint shader_prog;
int texunif;

int tex_curr_draw = 0;

int use_platform = 0;
int use_device = 0;
//float timer = 0;

double cam_r_x = 0;
double cam_r_y = 0;
vec3 cam_trans = { -0,0,-100 };

fmat4x4 cam_matrix;
CentroidBVH bvh;

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

  cl_int error;

  cl_uint computeCount = 0;
  error = clGetDeviceInfo(deviceIdMasterList[use_platform][use_device], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &computeCount, nullptr);
  printf("Selected device has %i \"compute units\"\n", computeCount);
  clerrchk(error);
#ifdef _WIN32
  HGLRC glcontext = wglGetCurrentContext();
  HDC devcontext = wglGetCurrentDC();
  cl_context_properties contextProps[] = {
    CL_GL_CONTEXT_KHR, (cl_context_properties)glcontext,
    CL_WGL_HDC_KHR, (cl_context_properties)devcontext,
    0,
    0
  };
#else
  cl_context_properties contextProps[] = {
    CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetGLXContext(window),
    CL_GLX_DISPLAY_KHR, (cl_context_properties)glfwGetX11Display(),
    0,
    0
  };
#endif

  context = clCreateContext(contextProps, deviceIdMasterList[use_platform].size(), deviceIdMasterList[use_platform].data(), nullptr, nullptr, &error);
  clerrchk(error);
  
  
  std::string psource;
  load_file("render_client/tracekernel.ck", psource);
  
  char const *src = psource.c_str();
  
  program[0] = clCreateProgramWithSource(context, 1, &src, nullptr, &error);
  clerrchk(error);
  error = clBuildProgram(program[0], deviceIdMasterList[use_platform].size(), deviceIdMasterList[use_platform].data(), "-I .", nullptr, nullptr);
  clerrchk(error);
  printprogrambuildinfo(program[0], deviceIdMasterList[use_platform][use_device]);
  kernel = clCreateKernel(program[0], "trace", &error);

  cl_ulong kernel_mem_req;
  error = clGetKernelWorkGroupInfo(kernel, deviceIdMasterList[use_platform][use_device], CL_KERNEL_LOCAL_MEM_SIZE, sizeof(cl_ulong), &kernel_mem_req, nullptr);
  printf("Kernel requires %lld bytes\n", kernel_mem_req);

  clerrchk(error);
  
  size_t workGroupSize = 0;
  error = clGetKernelWorkGroupInfo(kernel, deviceIdMasterList[use_platform][use_device], CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &workGroupSize, nullptr);
  printf("Recommened work group size: %zi\n", workGroupSize);

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
  gl_tex[0] = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texId[0], &error);
  clerrchk(error);
  gl_tex[1] = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texId[1], &error);
  clerrchk(error);
  
  cam_mat = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cam_matrix), cam_matrix, &error);
  clerrchk(error);
  
  printf("Loading model\n");
  std::vector<vec3> vertices;
  std::vector<cl_float4> normals;
  std::vector<cl_uint8> indices;
  MtlLib mtllib;
  mtllib.InsertMaterial("SKY_DEFAULT", { {0,0,0,0}, {0,0,0,0}, 0, 0, 0, 0 });
  LoadObj(std::ifstream("models/cube_mirror_test.obj"), vertices, normals, indices, mtllib, "./models/");

  tri_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(vec3) * vertices.size(), vertices.data(), &error);
  clerrchk(error);
  tri_ind_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint8) * indices.size(), indices.data(), &error);
  clerrchk(error);
  normal_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * normals.size(), normals.data(), &error);
  clerrchk(error);
  mtl_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Material) * mtllib.materials.size(), mtllib.materials.data(), &error);
  clerrchk(error);

  // create camera's RayIntersection buffer
  RayIntersection *ribuff = new RayIntersection[IMG_X * IMG_Y];
  vec3 o{ IMG_X / 2.f, IMG_Y / 2.f, -5 * IMG_X };
  for (int i = 0; i < IMG_X; ++i)
  {
    for (int j = 0; j < IMG_Y; ++j)
    {
      vec3 end{ i, j, 0 };
      vec3 dir = normalize(end - o);
      ribuff[i + j * IMG_Y].direction = { dir.x, dir.y, dir.z, 0 };
      ribuff[i + j * IMG_Y].origin = { 0,0,-5,1 };
      ribuff[i + j * IMG_Y].normal = { 0,0,1,0 };
      ribuff[i + j * IMG_Y].triangle_id = -1;
      ribuff[i + j * IMG_Y].uv = { 0,0 };
    }
  }
  cam_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(RayIntersection) * IMG_X * IMG_Y, ribuff, &error);
  delete[] ribuff;
  clerrchk(error);

  // create ray isec buffers
  ray_isc_buff[0] = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, sizeof(RayIntersection) * IMG_X * IMG_Y, nullptr, &error);
  clerrchk(error);
  ray_isc_buff[1] = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, sizeof(RayIntersection) * IMG_X * IMG_Y, nullptr, &error);
  clerrchk(error);

  //printf("Node size: %zi\n", sizeof(CentroidBVHNode));
  //printf("Union size: %zi\n", sizeof(CentroidBVHNodeUnion));
  //printf("Leaf size: %zi\n", sizeof(CentroidBVHLeafNode));
  //printf("Inner size: %zi\n", sizeof(CentroidBVHInnerNode));
  printf("model loaded\n");
  printf("creating bvh\n");
  CentroidBVHInit(&bvh);
  CentroidBVHBuild(&bvh, indices.data(), vertices.data(), indices.size(), vertices.size());
  printf("bvh created\n");
  printf("Triangles: %i\n", bvh.tricount);
  printf("Max depth: %i\n", bvh.depth);
  printf("Node count: %zx\n", bvh.listSize);
  printf("Avg tris per leaf: %f\n", static_cast<float>(bvh.tricount) / bvh.leafcount);
  printf("=======================\n\n\n");
  printf("buff size: %zx\n", sizeof(*(bvh.nodeList)) * bvh.listSize);
  accel_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(*(bvh.nodeList)) * bvh.listSize, bvh.nodeList, &error);
  clerrchk(error);

  unsigned tricount = indices.size();

  error = clSetKernelArg(kernel, 3, sizeof(unsigned), &tricount);
  clerrchk(error);
  error = clSetKernelArg(kernel, 4, sizeof(cl_mem), &tri_buff);
  clerrchk(error);
  error = clSetKernelArg(kernel, 5, sizeof(cl_mem), &normal_buff);
  clerrchk(error);
  error = clSetKernelArg(kernel, 6, sizeof(cl_mem), &tri_ind_buff);
  clerrchk(error);
  error = clSetKernelArg(kernel, 7, sizeof(cl_mem), &cam_mat);
  clerrchk(error);
  error = clSetKernelArg(kernel, 8, sizeof(cl_mem), &accel_buff);
  clerrchk(error);
  error = clSetKernelArg(kernel, 9, sizeof(cl_mem), &mtl_buff);
  clerrchk(error);

  cmdQueue = clCreateCommandQueue(context, deviceIdMasterList[use_platform][use_device], 0, &error);
  clerrchk(error);
  
}

void gl_init()
{
  float *zero = new float[IMG_X * IMG_Y * sizeof(float)];
  for (unsigned i = 0; i < IMG_X * IMG_Y; ++i)
  {
    zero[i] = 0;
  }

  glCreateTextures(GL_TEXTURE_2D, 2, texId);
  glBindTexture(GL_TEXTURE_2D, texId[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glerrchk();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, IMG_X, IMG_Y, 0, GL_RGBA, GL_FLOAT, zero);

  glBindTexture(GL_TEXTURE_2D, texId[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glerrchk();
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
  
  char cmplog[256];
  GLsizei size;
  glGetShaderInfoLog(vertpart, 255, &size, cmplog);
  printf("%s\n", cmplog);
  glGetShaderInfoLog(fragpart, 255, &size, cmplog);
  printf("%s\n", cmplog);
  
  glDeleteShader(vertpart);
  glDeleteShader(fragpart);
  
  glerrchk();
  
  glUseProgram(shader_prog);
  glerrchk();
  texunif = glGetUniformLocation(shader_prog, "rayimg");
  glerrchk();
  glUniform1i(texunif, 0);
  
  glerrchk();
  glViewport(0, 0, IMG_X, IMG_Y);
  glClearColor(0, 0, 0, 1);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void cl_update()
{
  //timer += 1;
  int error;

  error = clEnqueueWriteBuffer(cmdQueue, cam_mat, false, 0, sizeof(float) * 16, cam_matrix, 0, nullptr, nullptr);
  clerrchk(error);
  error = clEnqueueAcquireGLObjects(cmdQueue, 2, gl_tex, 0, nullptr, nullptr);
  clerrchk(error);

  error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &gl_tex[0]);
  clerrchk(error);
  error = clSetKernelArg(kernel, 1, sizeof(cl_mem), &cam_buff);
  clerrchk(error);
  error = clSetKernelArg(kernel, 2, sizeof(cl_mem), &ray_isc_buff[0]);
  clerrchk(error);
  unsigned top = 0;
  error = clSetKernelArg(kernel, 10, sizeof(unsigned), &top);
  clerrchk(error);

  size_t lworksize[3] = { 8,8,1 };
  size_t gworksize[3] = { IMG_X, IMG_Y, 1 };
  {
    size_t offset[3] = { 0, 0, 0 };
    error = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, offset, gworksize, lworksize, 0, nullptr, nullptr);
    clerrchk(error);
  }
  //{
  //  size_t offset[3] = { IMG_X / 2, 0, 0 };
  //  error = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, offset, gworksize, lworksize, 0, nullptr, nullptr);
  //  clerrchk(error);
  //}
  //{
  //  size_t offset[3] = { 0, IMG_Y / 2, 0 };
  //  error = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, offset, gworksize, lworksize, 0, nullptr, nullptr);
  //  clerrchk(error);
  //}
  //{
  //  size_t offset[3] = { IMG_X / 2, IMG_Y / 2, 0 };
  //  error = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, offset, gworksize, lworksize, 0, nullptr, nullptr);
  //  clerrchk(error);
  //}
  
  error = clFinish(cmdQueue);

  clerrchk(error);
  // reflection test
  error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &gl_tex[1]);
  clerrchk(error);
  error = clSetKernelArg(kernel, 1, sizeof(cl_mem), &ray_isc_buff[0]);
  clerrchk(error);
  error = clSetKernelArg(kernel, 2, sizeof(cl_mem), &ray_isc_buff[1]);
  clerrchk(error);
  top = 1;
  error = clSetKernelArg(kernel, 10, sizeof(unsigned), &top);
  clerrchk(error);

  {
    size_t offset[3] = { 0, 0, 0 };
    error = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, offset, gworksize, lworksize, 0, nullptr, nullptr);
    clerrchk(error);
  }
  // reflection test end

  
  clerrchk(error);
  clEnqueueReleaseGLObjects(cmdQueue, 2, gl_tex, 0, nullptr, nullptr);
  clerrchk(error);
  error = clFinish(cmdQueue);
  clerrchk(error);
}

void gl_update()
{
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_BLEND);
  glBindVertexArray(vao);
  glUseProgram(shader_prog);

  glBindTexture(GL_TEXTURE_2D, texId[0]);
  glActiveTexture(GL_TEXTURE0);
  glerrchk();
  glDrawArrays(GL_TRIANGLES, 0, 6);

  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D, texId[1]);
  glActiveTexture(GL_TEXTURE0);
  glerrchk();
  glDrawArrays(GL_TRIANGLES, 0, 6);

  glFinish();

  glfwSwapBuffers(window);
}

void cl_free()
{
  clReleaseCommandQueue(cmdQueue);
  clReleaseKernel(kernel);
  clReleaseProgram(program[0]);
  clReleaseMemObject(gl_tex[0]);
  clReleaseMemObject(gl_tex[1]);
  clReleaseMemObject(cam_mat);
  clReleaseMemObject(tri_buff);
  clReleaseMemObject(tri_ind_buff);
  clReleaseMemObject(accel_buff);
  clReleaseMemObject(mtl_buff);
  clReleaseMemObject(ray_isc_buff[0]);
  clReleaseMemObject(ray_isc_buff[1]);
  clReleaseMemObject(cam_buff);

  clReleaseContext(context);
}

void gl_free()
{
  glDeleteTextures(2, texId);
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

  double time = glfwGetTime();

  while (!glfwWindowShouldClose(window))
  {
    update();
    glfwPollEvents();
    glfwGetCursorPos(window, &cam_r_y, &cam_r_x);


    float axisy[4]{ 0, 1, 0, 0 };
    float axisx[4]{ 1, 0, 0, 0 };
    float roty[4][4]{ 0 };
    float rotx[4][4]{ 0 };
    fmat4x4 rotation;
    rotate3(roty, axisy, cam_r_y / 50);
    rotate3(rotx, axisx, cam_r_x / 50);
    multiply(roty, rotx, rotation);

    vec3 look = mult(rotation, { 0,0,-1 });
    vec3 left = mult(rotation, { -1,0,0 });
    vec3 up = mult(rotation, { 0,1,0 });

    if (glfwGetKey(window, GLFW_KEY_W))
    {
      cam_trans.z += 0.1;// = cam_trans + look * 0.5;
    }
    if (glfwGetKey(window, GLFW_KEY_S))
    {
      cam_trans.z -= 0.1;// = cam_trans - look * 0.5;
    }
    //if (glfwGetKey(window, GLFW_KEY_A))
    //{
    //  cam_trans = cam_trans + left * 0.5;
    //}
    //if (glfwGetKey(window, GLFW_KEY_D))
    //{
    //  cam_trans = cam_trans - left * 0.5;
    //}
    //if (glfwGetKey(window, GLFW_KEY_SPACE))
    //{
    //  cam_trans = cam_trans + up * 0.5;
    //}
    //if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
    //{
    //  cam_trans = cam_trans - up * 0.5;
    //}

    float trans[4][4] = {
      1, 0, 0, cam_trans.x,
      0, 1, 0, cam_trans.y,
      0, 0, 1, cam_trans.z,
      0, 0, 0, 1
    };
    multiply(trans, rotation, cam_matrix);

    if (glfwGetKey(window, GLFW_KEY_SPACE))
    {
      tex_curr_draw = 1;
    }
    else
    {
      tex_curr_draw = 0;
    }

    double t = glfwGetTime();
    char buff[32];
    sprintf(buff, "fps: %f", 1 / (t - time));
    time = t;
    glfwSetWindowTitle(window, buff);
   }
  
  release();
  glfwTerminate();

  return 0;
}
