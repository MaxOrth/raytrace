

#include <Windows.h>


#ifdef ENABLE_OPTIMUS

extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

#endif
