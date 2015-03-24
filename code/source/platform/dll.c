#include "dll.h"

#if PLATFORM == PLATFORM_LINUX
#include <dlfcn.h>

DllHandle load_dll(const char *path)
{
	if (dlopen(path, RTLD_NOLOAD))
		fail("DLL already opened: %s", path);
	return dlopen(path, RTLD_NOW);
}

void unload_dll(DllHandle dll)
{ dlclose(dll); }

void* query_dll_sym(DllHandle dll, const char *sym)
{ return dlsym(dll, sym); }

const char* dll_error()
{ return dlerror(); }

#endif
