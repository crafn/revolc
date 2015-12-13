#ifndef REVOLC_CORE_MEMORY_H
#define REVOLC_CORE_MEMORY_H

#include "build.h"

/// @todo Other compilers
#define PLAT_FULL_MEMORY_BARRIER() __sync_synchronize()
#define PLAT_ACQUIRE_FENCE() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define PLAT_RELEASE_FENCE() __atomic_thread_fence(__ATOMIC_RELEASE)

// Allocator types
typedef enum AtorType {
	AtorType_none,
	AtorType_stack, // alloca
	AtorType_gen, 	// malloc/realloc/free
	AtorType_linear, // allocates linearly from buffer. free == nop
	AtorType_dev,
	AtorType_leakable_dev, // Use for one-off allocations if lazy.
} AtorType;

typedef struct Ator {
	AtorType type;
	// Only used for linear
	U8 *buf;
	U32 offset;
	U32 capacity;

	const char *tag;
} Ator;

// General allocation functions
#define ALLOC(ator, size, tag) \
	( ator->type == AtorType_stack ? \
			STACK_ALLOC(size) : \
			alloc_impl(ator, size, NULL, tag, false) )
#define REALLOC(ator, ptr, size, tag) \
	( ator->type == AtorType_stack ? \
			STACK_ALLOC(size) : \
			alloc_impl(ator, size, ptr, tag, false) )
#define ZERO_ALLOC(ator, size, tag) \
	( ator->type == AtorType_stack ? \
			ZERO_STACK_ALLOC(size) : \
			alloc_impl(ator, size, NULL, tag, true) )
#define ZERO_REALLOC(ator, ptr, size, tag) \
	( ator->type == AtorType_stack ? \
			ZERO_STACK_ALLOC(size) : \
			alloc_impl(ator, size, ptr, tag, true) )
#define FREE(ator, ptr) (free_impl(ator, ptr))

// Specific allocators to be used with general allocation functions
REVOLC_API WARN_UNUSED Ator *stack_ator(); // No need to free
REVOLC_API WARN_UNUSED Ator *gen_ator();
REVOLC_API WARN_UNUSED Ator linear_ator(void *buf, U32 size, const char *tag);
REVOLC_API WARN_UNUSED Ator *dev_ator();
REVOLC_API WARN_UNUSED Ator *leakable_dev_ator();
REVOLC_API WARN_UNUSED Ator *frame_ator(); // No need to free (valid for this frame)

REVOLC_API WARN_UNUSED
void * alloc_impl(Ator *ator, U32 size, void *realloc_ptr, const char *tag, bool zero);
REVOLC_API void free_impl(Ator *ator, void *ptr);


#define STACK_ALLOC(size) alloca(size)
#define ZERO_STACK_ALLOC(size) memset(alloca(size), 0, size)

// @todo Remove
REVOLC_API WARN_UNUSED
void * dev_malloc(U32 size);
REVOLC_API WARN_UNUSED
void * dev_realloc(void *ptr, U32 size);
REVOLC_API void dev_free(void *mem);

REVOLC_API void * frame_alloc(U32 size);
REVOLC_API void reset_frame_alloc();

#endif // REVOLC_CORE_MEMORY_H
