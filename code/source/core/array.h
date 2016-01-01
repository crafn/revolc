#ifndef REVOLC_CORE_ARRAY_H
#define REVOLC_CORE_ARRAY_H

#include "build.h"
#include "memory.h"

// @todo Remove this unnerving api, use Array(type) instead

REVOLC_API WARN_UNUSED
void * enlarge_array(void *array, U32 *old_count, U32 elem_size);

/// Append to array, possibly enlargening it
REVOLC_API WARN_UNUSED
void * push_dyn_array(
		void *array, U32 *capacity, U32 *count, U32 elem_size, void *elem);

// Dynamic array

#define Array(V) JOIN2(V, _Array)
#define create_array(V) JOIN3(create_, V, _array)
#define destroy_array(V) JOIN3(destroy_, V, _array)
#define release_array(V) JOIN3(release_, V, _array)
#define push_array(V) JOIN3(push_, V, _array)
#define pop_array(V) JOIN3(pop_, V, _array)
#define insert_array(V) JOIN3(insert_, V, _array)
#define copy_array(V) JOIN3(copy_, V, _array)
#define clear_array(V) JOIN3(clear_, V, _array)
// Internal
#define increase_array_capacity(V) JOIN3(increase_array_capacity_, V, _array)

#define DECLARE_ARRAY(V)\
typedef struct Array(V) {\
	V *data;\
	U32 size;\
	U32 capacity;\
	Ator *ator;\
} Array(V);\
\
REVOLC_API Array(V) create_array(V)(Ator *ator, U32 init_capacity);\
REVOLC_API void destroy_array(V)(Array(V) *arr);\
REVOLC_API V *release_array(V)(Array(V) *arr);\
REVOLC_API void push_array(V)(Array(V) *arr, V value);\
REVOLC_API V pop_array(V)(Array(V) *arr);\
REVOLC_API void insert_array(V)(Array(V) *arr, U32 at_place, V *values, U32 value_count);\
REVOLC_API Array(V) copy_array(V)(Array(V) *arr);\
REVOLC_API void clear_array(V)(Array(V) *arr);\

#define DEFINE_ARRAY(V)\
Array(V) create_array(V)(Ator *ator, U32 init_capacity)\
{\
	Array(V) arr = {0};\
	arr.ator = ator;\
	if (init_capacity > 0) {\
		arr.data = (V*)ALLOC(ator, init_capacity*sizeof(*arr.data), "create_array");\
		arr.capacity = init_capacity;\
	}\
	return arr;\
}\
void destroy_array(V)(Array(V) *arr)\
{\
	ensure(arr);\
	FREE(arr->ator, arr->data);\
}\
V *release_array(V)(Array(V) *arr)\
{\
	V *data = arr->data;\
	arr->data = NULL;\
	arr->size = 0;\
	arr->capacity = 0;\
	return data;\
}\
internal void increase_array_capacity(V)(Array(V) *arr, U32 min_size)\
{\
	if (min_size <= arr->capacity)\
		return;\
	if (arr->capacity == 0)\
		arr->capacity = MAX(min_size, 1);\
	else\
		arr->capacity = MAX(min_size, arr->capacity*2);\
	arr->data = (V*)REALLOC(arr->ator, arr->data, arr->capacity*sizeof(*arr->data), "increase_array_capacity");\
}\
void push_array(V)(Array(V) *arr, V value)\
{\
	ensure(arr);\
	increase_array_capacity(V)(arr, arr->size + 1);\
	arr->data[arr->size++] = value;\
}\
void insert_array(V)(Array(V) *arr, U32 at_place, V *values, U32 value_count)\
{\
	U32 move_count = arr->size - at_place;\
	ensure(arr);\
	ensure(at_place <= arr->size);\
	increase_array_capacity(V)(arr, arr->size + value_count);\
	memmove(arr->data + at_place + value_count, arr->data + at_place, sizeof(*arr->data)*move_count);\
	memcpy(arr->data + at_place, values, sizeof(*arr->data)*value_count);\
	arr->size += value_count;\
}\
V pop_array(V)(Array(V) *arr)\
{\
	ensure(arr);\
	ensure(arr->size > 0);\
	--arr->size;\
	return arr->data[arr->size];\
}\
Array(V) copy_array(V)(Array(V) *arr)\
{\
	Array(V) copy = {0};\
	copy.ator = arr->ator;\
	copy.data = (V*)ALLOC(arr->ator, arr->capacity*sizeof(*arr->data), "copy_array");\
	memcpy(copy.data, arr->data, arr->size*sizeof(*arr->data));\
	copy.size = arr->size;\
	copy.capacity = arr->capacity;\
	return copy;\
}\
void clear_array(V)(Array(V) *arr)\
{\
	ensure(arr);\
	arr->size = 0;\
}\


#endif // REVOLC_CORE_ARRAY_H
