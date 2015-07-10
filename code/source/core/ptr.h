#ifndef REVOLC_PTR_H
#define REVOLC_PTR_H

#include "build.h"

// Declares a "pointer" relative to its own address, handy in binary blobs. 
// Macro because we'll want to have type-safe custom preprocessing soon
#define REL_PTR(type) RelPtr

typedef struct RelPtr {
	U64 value;
} RelPtr;

static void set_rel_ptr(RelPtr *rel_ptr, const void *ptr)
{ rel_ptr->value= (U8*)ptr - (U8*)rel_ptr; }

static void * rel_ptr(const RelPtr *rel_ptr)
{ return (U8*)rel_ptr + rel_ptr->value; }


#endif // REVOLC_PTR_H
