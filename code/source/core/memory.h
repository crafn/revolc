#ifndef REVOLC_CORE_MEMORY_H
#define REVOLC_CORE_MEMORY_H

#include "build.h"

// Allocator types
typedef enum AtorType {
	AtorType_none,
	AtorType_stack, // alloca
	AtorType_gen, 	// malloc/calloc/realloc/free
	AtorType_frame, // alloc_frame // @todo change to general linear
	AtorType_dev,
} AtorType;

typedef struct Ator {
	AtorType type;
} Ator;

// @todo Reserve/commit

// General allocation functions
#define ALLOC(ator, size, tag) \
	( ator.type == AtorType_stack ? \
			STACK_ALLOC(size) : \
			alloc_impl(ator, size, NULL, tag) )
#define REALLOC(ator, ptr, size, tag) \
	( ator.type == AtorType_stack ? \
			STACK_ALLOC(size) : \
			alloc_impl(ator, size, ptr, tag) )
#define ZERO_ALLOC(ator, size, tag) \
	( ator.type == AtorType_stack ? \
			ZERO_STACK_ALLOC(size) : \
			zero_alloc_impl(ator, size, NULL, tag) )
#define ZERO_REALLOC(ator, ptr, size, tag) \
	( ator.type == AtorType_stack ? \
			ZERO_STACK_ALLOC(size) : \
			zero_alloc_impl(ator, size, ptr, tag) )
#define FREE(ator, ptr) (free_impl(ator, ptr))

// Specific allocators to be used with general allocation functions
REVOLC_API WARN_UNUSED Ator stack_ator(); // No need to free
REVOLC_API WARN_UNUSED Ator gen_ator();
REVOLC_API WARN_UNUSED Ator dev_ator();
REVOLC_API WARN_UNUSED Ator frame_ator(); // No need to free (valid for this frame)

REVOLC_API WARN_UNUSED
void * alloc_impl(Ator ator, U32 size, void *realloc_ptr, const char *tag);
REVOLC_API WARN_UNUSED
void * zero_alloc_impl(Ator ator, U32 size, void *realloc_ptr, const char *tag);
REVOLC_API void free_impl(Ator ator, void *ptr);



// @todo Hide things below -- implementation

#define STACK_ALLOC(size) alloca(size)
#define ZERO_STACK_ALLOC(size) memset(alloca(size), 0, size)

REVOLC_API WARN_UNUSED
void * dev_malloc(U32 size);
REVOLC_API WARN_UNUSED
void * dev_realloc(void *ptr, U32 size);
REVOLC_API void dev_free(void *mem);

REVOLC_API void * frame_alloc(U32 size);
REVOLC_API void reset_frame_alloc();

#endif // REVOLC_CORE_MEMORY_H
