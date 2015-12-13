#ifndef REVOLC_PLATFORM_DLL_H
#define REVOLC_PLATFORM_DLL_H

#include "build.h"

typedef void * DllHandle;
const DllHandle g_main_program_dll= NULL;

REVOLC_API DllHandle load_dll(const char *path);
REVOLC_API void unload_dll(DllHandle dll);
REVOLC_API void * query_dll_sym(DllHandle dll, const char* sym);
REVOLC_API const char * dll_error();
REVOLC_API const char * plat_dll_ext();

#endif // REVOLC_PLATFORM_DLL_H
