#ifndef REVOLC_CORE_SPARSETABLE_H
#define REVOLC_CORE_SPARSETABLE_H

#include "build.h"

#define SparseTbl(T) JOIN2(ORIG_TYPE(T), _SparseTbl)
#define create_stbl(T) JOIN3(create_, ORIG_TYPE(T), _stbl)
#define destroy_stbl(T) JOIN3(destroy_, ORIG_TYPE(T), _stbl)
#define insert_stbl(T) JOIN3(insert_, ORIG_TYPE(T), _stbl)
#define remove_stbl(T) JOIN3(remove_, ORIG_TYPE(T), _stbl)
#define get_stbl(T) JOIN3(get_, ORIG_TYPE(T), _stbl)
#define begin_stbl(T) JOIN3(begin_, ORIG_TYPE(T), _stbl)
#define next_stbl(T) JOIN3(next_, ORIG_TYPE(T), _stbl)
#define end_stbl(T) JOIN3(end_, ORIG_TYPE(T), _stbl)

// Sparse only logically.
// Properties:
//  - cache-friendly iterating (<- @todo)
//  - permanent handles to elements
//  - fast (< O(n)) insertion and removal (<- @todo)
#define DECLARE_SPARSETABLE(T)\
typedef struct SparseTbl(T) {\
	Ator *ator;\
	T *array;\
	U32 *status;\
	U32 capacity;\
	U32 count;\
} SparseTbl(T);\
\
REVOLC_API SparseTbl(T) create_stbl(T)(Ator *ator, U32 capacity);\
REVOLC_API void destroy_stbl(T)(SparseTbl(T) *tbl);\
REVOLC_API Handle insert_stbl(T)(SparseTbl(T) *tbl, T elem);\
REVOLC_API void remove_stbl(T)(SparseTbl(T) *tbl, Handle h);\
REVOLC_API T *get_stbl(T)(SparseTbl(T) *tbl, Handle h);\
REVOLC_API T *begin_stbl(T)(SparseTbl(T) *tbl);\
REVOLC_API T *next_stbl(T)(SparseTbl(T) *tbl, T* it);\
REVOLC_API T *end_stbl(T)(SparseTbl(T) *tbl);\

// Impl
// @todo Use status to mark next used index for skipping free regions

#define SPARSETABLE_STATUS_USED 0
#define SPARSETABLE_STATUS_FREE (U32)(-1)

#define DEFINE_SPARSETABLE(T)\
SparseTbl(T) create_stbl(T)(Ator *ator, U32 capacity)\
{\
	SparseTbl(T) tbl = {\
		.ator = ator,\
		.array = ALLOC(ator, sizeof(*tbl.array)*capacity, "stbl_array"),\
		.status = ALLOC(ator, sizeof(*tbl.status)*capacity, "stbl_status"),\
		.capacity = capacity,\
	};\
	for (U32 i = 0; i < tbl.capacity; ++i)\
		tbl.status[i] = SPARSETABLE_STATUS_FREE;\
	return tbl;\
}\
\
void destroy_stbl(T)(SparseTbl(T) *tbl)\
{\
	FREE(tbl->ator, tbl->array);\
	FREE(tbl->ator, tbl->status);\
	tbl->array = NULL;\
	tbl->status = NULL;\
}\
\
Handle insert_stbl(T)(SparseTbl(T) *tbl, T elem)\
{\
	/* @todo Keep leftmost free index stored */\
	for (U32 i = 0; i < tbl->capacity; ++i) {\
		if (tbl->status[i] != SPARSETABLE_STATUS_FREE)\
			continue;\
		tbl->array[i] = elem;\
		tbl->status[i] = SPARSETABLE_STATUS_USED;\
		++tbl->count;\
		return i;\
	}\
	fail("SparseTbl too small: %i", tbl->capacity);\
}\
\
void remove_stbl(T)(SparseTbl(T) *tbl, Handle h)\
{\
	ensure(h < tbl->capacity);\
	ensure(tbl->status[h] == SPARSETABLE_STATUS_USED);\
	tbl->status[h] = SPARSETABLE_STATUS_FREE;\
	ensure(tbl->count > 0);\
	--tbl->count;\
}\
\
T *get_stbl(T)(SparseTbl(T) *tbl, Handle h)\
{\
	ensure(h < tbl->capacity);\
	if (tbl->status[h] == SPARSETABLE_STATUS_FREE)\
		return NULL;\
	return &tbl->array[h];\
}\
T *begin_stbl(T)(SparseTbl(T) *tbl)\
{\
	U32 i = 0;\
	while (i < tbl->capacity && tbl->status[i] != SPARSETABLE_STATUS_USED)\
		++i;\
	return tbl->array + i;\
}\
T *next_stbl(T)(SparseTbl(T) *tbl, T* it)\
{\
	/* @todo Jump over empty regions */\
	++it;\
	while (	it < tbl->array + tbl->capacity &&\
			tbl->status[it - tbl->array] != SPARSETABLE_STATUS_USED)\
		++it;\
	return it;\
}\
T *end_stbl(T)(SparseTbl(T) *tbl)\
{ return tbl->array + tbl->capacity; }\


#endif // REVOLC_CORE_SPARSETABLE
