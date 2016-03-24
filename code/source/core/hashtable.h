#ifndef REVOLC_CORE_HASHTABLE_H
#define REVOLC_CORE_HASHTABLE_H

#include "build.h"
#include "core/hash.h"
#include "core/memory.h"

// @todo Change max_size to expected_item_count. This avoids problems with value 0, and allows load factor calculations
//       inside hash table.

// Key_Value
#define KV(K, V) JOIN3(ORIG_TYPE(K), _, ORIG_TYPE(V))
// key_value
#define LC_KV(K, V) JOIN3(LC(ORIG_TYPE(K)), _, LC(ORIG_TYPE(V))) 

#define create_tbl(K, V) JOIN3(create_, LC_KV(K, V), _tbl)
#define destroy_tbl(K, V) JOIN3(destroy_, LC_KV(K, V), _tbl)
#define get_tbl(K, V) JOIN3(get_, LC_KV(K, V), _tbl)
#define set_tbl(K, V) JOIN3(set_, LC_KV(K, V), _tbl)
#define clear_tbl(K, V) JOIN3(clear_tbl, LC_KV(K, V), _tbl)
#define null_tbl_entry(K, V) JOIN3(null_, LC_KV(K, V), _tbl_entry)
#define HashTbl(K, V) JOIN2(KV(K, V), _Tbl)
#define HashTbl_Entry(K, V) JOIN2(KV(K, V), _Tbl_Entry)

#define DECLARE_HASHTABLE(K, V)\
struct HashTbl_Entry(K, V);\
\
typedef struct HashTbl(K, V) {\
	Ator *ator;\
	struct HashTbl_Entry(K, V) *array;\
	U32 array_size;\
	U32 count;\
	K null_key;\
	V null_value;\
} HashTbl(K, V);\
typedef struct HashTbl_Entry(K, V) {\
	K key;\
	V value;\
} HashTbl_Entry(K, V);\
\
REVOLC_API HashTbl(K, V) create_tbl(K, V)(	K null_key, V null_value,\
											Ator *ator, U32 capacity);\
REVOLC_API void destroy_tbl(K, V)(HashTbl(K, V) *tbl);\
\
REVOLC_API V get_tbl(K, V)(HashTbl(K, V) *tbl, K key);\
REVOLC_API void set_tbl(K, V)(HashTbl(K, V) *tbl, K key, V value);\
REVOLC_API void clear_tbl(K, V)(HashTbl(K, V) *tbl);\


DECLARE_HASHTABLE(U64, U32)
DECLARE_HASHTABLE(U32, U32)

#endif // REVOLC_CORE_HASHTABLE_H
