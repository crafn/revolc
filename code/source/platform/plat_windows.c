#include "platform/device.h"
#include "platform/dll.h"

DllHandle load_dll(const char *path)
{
	fail("@todo load_dll");
}

void unload_dll(DllHandle dll)
{ fail("@todo unload_dll"); }

void* query_dll_sym(DllHandle dll, const char *sym)
{ fail("@todo query_dll_sym"); }

const char* dll_error()
{ fail("@todo dll_error"); }
