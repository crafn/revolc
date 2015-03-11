#ifndef REVOLC_PLATFORM_DLL_H
#define REVOLC_PLATFORM_DLL_H

#include "build.h"

typedef void * DllHandle;
const DllHandle g_main_program_dll= NULL;

DllHandle load_dll(const char *path);
void unload_dll(DllHandle dll);
void * query_dll_sym(DllHandle dll, const char* sym);
const char* dll_error();

#endif // REVOLC_PLATFORM_DLL_H
