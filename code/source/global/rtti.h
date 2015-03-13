#ifndef REVOLC_GLOBAL_RTTI_H
#define REVOLC_GLOBAL_RTTI_H

#include "build.h"

/// @warning These can be slow as they search the symbol table!
// Lookup is done from all loaded Modules

// Give a valid pointer corresponding to a pointer invalidated by dll reload
// This is prohibited: rtti_relocate(rtti_relocate(ptr)), as symbols can
// theoretically switch addresses with each other on the process of dll reload
REVOLC_API WARN_UNUSED
void * rtti_relocate_sym(void *possibly_invalidated_ptr);

// Example:
// sym= rtti_func_ptr("foo");
// while (1) {
//   reload_some_dlls();
//   rtti_requery_syms();
//   sym= rtti_relocate_sym(sym);
// }
// Be very careful where to call, as every previously invalidated symbol
// should've been resolved as in the example code above
// So this is prohibited:
//   reload_some_dlls();
//   rtti_requery_syms();
//   // sym= rtti_relocate_sym(sym);
//   reload_some_dlls();
//   rtti_requery_syms();
//   sym= rtti_relocate_sym(sym); // Probably fails
REVOLC_API
void rtti_requery_syms();

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
