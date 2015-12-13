#include "hashtable.h"

#define DEFINE_HASHTABLE(K, V)\
internal HashTbl_Entry(K, V) null_tbl_entry(K, V)(HashTbl(K, V) *tbl)\
{ return (HashTbl_Entry(K, V)) {\
		.key = tbl->null_key,\
		.value = tbl->null_value\
	};\
}\
\
HashTbl(K, V) create_tbl(K, V)(	K null_key, V null_value,\
								Ator *ator, U32 max_size)\
{\
	HashTbl(K, V) tbl = {};\
	tbl.null_key = null_key;\
	tbl.null_value = null_value;\
	tbl.ator = ator;\
	tbl.array_size = max_size;\
	tbl.array = ALLOC(ator, sizeof(*tbl.array)*max_size, "tbl.array");\
	for (U32 i = 0; i < max_size; ++i)\
		tbl.array[i] = null_tbl_entry(K, V)(&tbl);\
	return tbl;\
}\
\
void destroy_tbl(K, V)(HashTbl(K, V) *tbl)\
{\
	FREE(tbl->ator, tbl->array);\
	tbl->array = NULL;\
}\
\
V get_tbl(K, V)(HashTbl(K, V) *tbl, K key)\
{\
	U32 ix = hash(K)(key) % tbl->array_size;\
	/* Linear probing */\
	/* Should not be infinite because set_id_handle_tbl asserts if table is full */\
	while (tbl->array[ix].key != key && tbl->array[ix].key != tbl->null_key)\
		ix = (ix + 1) % tbl->array_size;\
\
	if (tbl->array[ix].key == tbl->null_key)\
		ensure(tbl->array[ix].value == tbl->null_value);\
\
	return tbl->array[ix].value;\
}\
\
void set_tbl(K, V)(HashTbl(K, V) *tbl, K key, V value)\
{\
	ensure(key != tbl->null_key);\
\
	U32 ix = hash(K)(key) % tbl->array_size;\
\
	/* Linear probing */\
	while (tbl->array[ix].key != key && tbl->array[ix].key != tbl->null_key)\
		ix = (ix + 1) % tbl->array_size;\
\
	HashTbl_Entry(K, V) *entry = &tbl->array[ix];\
	bool modify_existing = 	value != tbl->null_value && entry->key != tbl->null_key;\
	bool insert_new =		value != tbl->null_value && entry->key == tbl->null_key;\
	bool remove_existing =	value == tbl->null_value && entry->key != tbl->null_key;\
	bool remove_new =		value == tbl->null_value && entry->key == tbl->null_key;\
\
	if (modify_existing) {\
		entry->value = value;\
	} else if (insert_new) {\
		entry->key = key;\
		entry->value = value;\
		++tbl->count;\
	} else if (remove_existing) {\
		entry->key = key;\
		entry->key = tbl->null_key;\
		entry->value = NULL_HANDLE;\
		ensure(tbl->count > 0);\
		--tbl->count;\
\
		/* Rehash */\
		ix = (ix + 1) % tbl->array_size;\
		while (tbl->array[ix].key != tbl->null_key) {\
			HashTbl_Entry(K, V) e = tbl->array[ix];\
			tbl->array[ix] = null_tbl_entry(K, V)(tbl);\
			--tbl->count;\
			set_tbl(K, V)(tbl, e.key, e.value);\
\
			ix = (ix + 1) % tbl->array_size;\
		}\
	} else if (remove_new) {\
		/* Nothing to be removed */\
	} else {\
		fail("Hash table logic failed");\
	}\
\
	ensure(tbl->count < tbl->array_size);\
}\

DEFINE_HASHTABLE(U64, U32)
DEFINE_HASHTABLE(U32, U32)

