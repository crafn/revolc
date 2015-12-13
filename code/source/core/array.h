#ifndef REVOLC_CORE_ARRAY_H
#define REVOLC_CORE_ARRAY_H

#include "build.h"

// @todo Copy dynamic array "template" from some other project. This is unnerving.

REVOLC_API WARN_UNUSED
void * enlarge_array(void *array, U32 *old_count, U32 elem_size);

/// Append to array, possibly enlargening it
REVOLC_API WARN_UNUSED
void * push_dyn_array(
		void *array, U32 *capacity, U32 *count, U32 elem_size, void *elem);

#endif // REVOLC_CORE_ARRAY_H
