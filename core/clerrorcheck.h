#pragma once

#include <CL/cl.h>


#define clerrchk(err) clerrchk_impl(err, __LINE__)

#ifdef __cplusplus
extern "C"
#endif
void clerrchk_impl(cl_int error, int line);

