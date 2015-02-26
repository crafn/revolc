#ifndef REVOLC_GLOBAL_RTTI_H
#define REVOLC_GLOBAL_RTTI_H

#include "build.h"

/// @warning These can be slow as they search the symbol table!
REVOLC_API
U32 struct_size(const char *struct_name);

REVOLC_API
void * func_ptr(const char *func_name);

REVOLC_API
U32 member_size(const char *struct_name, const char *member_name);

REVOLC_API
U32 member_offset(const char *struct_name, const char *member_name);

#endif // REVOLC_GLOBAL_RTTI_H
