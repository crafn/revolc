#ifndef REVOLC_GLOBAL_RTTI_H
#define REVOLC_GLOBAL_RTTI_H

#include "build.h"

typedef struct MemberRtti {
	const char *name;
	const char *base_type_name;
	U32 ptr_depth;
	U32 array_depth;

	U32 offset;
	U32 size;
} MemberRtti;

typedef struct StructRtti {
	const char *name;
	U32 size;

	MemberRtti *members;
	U32 member_count;
} StructRtti;

// Lookup is done from all loaded Modules
// These are currently quite slow, but that can be easily fixed

// @todo Rename functions rtti_* -> *_rtti
REVOLC_API
StructRtti *rtti_struct(const char *struct_name);

REVOLC_API
void *rtti_func_ptr(const char *func_name);

REVOLC_API
U32 rtti_member_index(const char *struct_name, const char *member_name);

// e.g. rtti_sym_name(rtti_func_ptr("foo")) == "foo"
REVOLC_API
const char *rtti_sym_name(void *ptr);


// Give a valid pointer corresponding to a pointer invalidated by dll reload
// This is prohibited: rtti_relocate(rtti_relocate(ptr)), as symbols can
// theoretically switch addresses with each other on the process of dll reload
REVOLC_API WARN_UNUSED
void * rtti_relocate_sym(void *possibly_invalidated_ptr);

// Example:
// sym = rtti_func_ptr("foo");
// while (1) {
//   reload_some_dlls();
//   rtti_requery_syms();
//   sym = rtti_relocate_sym(sym);
// }
// Be very careful where to call, as every previously invalidated symbol
// should've been resolved as in the example code above
// So this is prohibited:
//   reload_some_dlls();
//   rtti_requery_syms();
//   // sym = rtti_relocate_sym(sym);
//   reload_some_dlls();
//   rtti_requery_syms();
//   sym = rtti_relocate_sym(sym); // Probably fails
REVOLC_API
void rtti_requery_syms();

#endif // REVOLC_GLOBAL_RTTI_H
