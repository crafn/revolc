#include "dll.h"

#include <dlfcn.h>

DllHandle load_dll(const char *path)
{ return dlopen(path, RTLD_LAZY | RTLD_GLOBAL); }

void unload_dll(DllHandle dll)
{ dlclose(dll); }

void* query_dll_sym(DllHandle dll, const char *sym)
{ return dlsym(dll, sym); }

