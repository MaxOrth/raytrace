

#include "clerrorcheck.h"
#include "clerrortext.h"
#include <stdio.h>
#include <Windows.h>

extern "C"
{

  void clerrchk_impl(cl_int err, int line)
  {
    if (err != CL_SUCCESS)
    {
      //DebugBreak();
      printf("%i: %s (%i)\n", line, getCLErrorString(err), err);
    }
  }
}
