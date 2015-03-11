#ifndef REVOLC_GLOBAL_RTTI_H
#define REVOLC_GLOBAL_RTTI_H

#include "build.h"

/// @warning These can be slow as they search the symbol table!
// Lookup is done from all loaded Modules

REVOLC_API
U32 rtti_struct_size(const char *struct_name);

REVOLC_API
void * rtti_func_ptr(const char *func_name);

REVOLC_API
U32 rtti_member_size(const char *struct_name, const char *member_name);

REVOLC_API
U32 rtti_member_offset(const char *struct_name, const char *member_name);

REVOLC_API
const char * rtti_member_type_name(const char *struct_name, const char *member_name);

#endif // REVOLC_GLOBAL_RTTI_H
