#ifndef QC_CORE_H
#define QC_CORE_H

/* Commonly used utils */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

typedef uint32_t QC_U32;
typedef uint64_t QC_U64;
typedef void *QC_Void_Ptr; /* Just for some macro fiddling */

/* Usage: QC_FAIL(("Something %i", 10)) */
#define QC_FAIL(args) do { printf("INTERNAL FAILURE: "); printf args; printf("\n"); abort(); } while(0)
#define QC_ASSERT(x) assert(x)

#define QC_NONULL(x) qc_nonull_impl(x)
void *qc_nonull_impl(void *ptr);

#define QC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define QC_MAX(a, b) ((a) > (b) ? (a) : (b))

typedef enum { QC_false, QC_true } QC_Bool;

#define QC_INTERNAL static
#define QC_LOCAL_PERSIST static

#define QC_JOIN2_IMPL(A, B) A##B
#define QC_JOIN2(A, B) QC_JOIN2_IMPL(A, B)

#define QC_JOIN3_IMPL(A, B, C) A##B##C
#define QC_JOIN3(A, B, C) QC_JOIN3_IMPL(A, B, C)

/* Not terminated by NULL! */
typedef struct QC_Buf_Str {
	const char *buf;
	int len;
} QC_Buf_Str;

QC_Bool qc_buf_str_equals(QC_Buf_Str a, QC_Buf_Str b);
QC_Buf_Str qc_c_str_to_buf_str(const char* str);

/* Args for printf %.*s specifier */
#define QC_BUF_STR_ARGS(str) str.len, str.buf


/* Dynamic array */

#define QC_Array(V) QC_JOIN2(V, _Array)
#define qc_create_array(V) QC_JOIN3(qc_create_, V, _array)
#define qc_destroy_array(V) QC_JOIN3(qc_destroy_, V, _array)
#define qc_release_array(V) QC_JOIN3(release_, V, _array)
#define qc_push_array(V) QC_JOIN3(qc_push_, V, _array)
#define qc_pop_array(V) QC_JOIN3(pop_, V, _array)
#define qc_insert_array(V) QC_JOIN3(insert_, V, _array)
#define qc_erase_array(V) QC_JOIN3(erase_, V, _array)
#define qc_copy_array(V) QC_JOIN3(qc_copy_, V, _array)
#define qc_clear_array(V) QC_JOIN3(clear_, V, _array)
/* Internal */
#define qc_increase_array_capacity(V) QC_JOIN3(qc_increase_array_capacity_, V, _array)

#define QC_DECLARE_ARRAY(V)\
typedef struct QC_Array(V) {\
	V *data;\
	int size;\
	int capacity;\
} QC_Array(V);\
\
QC_Array(V) qc_create_array(V)(int init_capacity);\
void qc_destroy_array(V)(QC_Array(V) *arr);\
V *qc_release_array(V)(QC_Array(V) *arr);\
void qc_push_array(V)(QC_Array(V) *arr, V value);\
V qc_pop_array(V)(QC_Array(V) *arr);\
void qc_insert_array(V)(QC_Array(V) *arr, int at_place, V *values, int value_count);\
void qc_erase_array(V)(QC_Array(V) *arr, int at_place, int erase_count);\
QC_Array(V) qc_copy_array(V)(QC_Array(V) *arr);\
void qc_clear_array(V)(QC_Array(V) *arr);\

#define QC_DEFINE_ARRAY(V)\
QC_Array(V) qc_create_array(V)(int init_capacity)\
{\
	QC_Array(V) arr = {0};\
	if (init_capacity > 0) {\
		arr.data = (V*)QC_MALLOC(init_capacity*sizeof(*arr.data));\
		arr.capacity = init_capacity;\
	}\
	return arr;\
}\
void qc_destroy_array(V)(QC_Array(V) *arr)\
{\
	QC_ASSERT(arr);\
	QC_FREE(arr->data);\
}\
V *qc_release_array(V)(QC_Array(V) *arr)\
{\
	V *data = arr->data;\
	arr->data = NULL;\
	arr->size = 0;\
	arr->capacity = 0;\
	return data;\
}\
QC_INTERNAL void qc_increase_array_capacity(V)(QC_Array(V) *arr, int min_size)\
{\
	if (min_size <= arr->capacity)\
		return;\
	if (arr->capacity == 0)\
		arr->capacity = QC_MAX(min_size, 1);\
	else\
		arr->capacity = QC_MAX(min_size, arr->capacity*2);\
	arr->data = (V*)realloc(arr->data, arr->capacity*sizeof(*arr->data));\
}\
void qc_push_array(V)(QC_Array(V) *arr, V value)\
{\
	QC_ASSERT(arr);\
	qc_increase_array_capacity(V)(arr, arr->size + 1);\
	arr->data[arr->size++] = value;\
}\
void qc_insert_array(V)(QC_Array(V) *arr, int at_place, V *values, int value_count)\
{\
	int move_count = arr->size - at_place;\
	QC_ASSERT(arr);\
	QC_ASSERT(at_place >= 0 && at_place <= arr->size);\
	QC_ASSERT(move_count >= 0);\
	qc_increase_array_capacity(V)(arr, arr->size + value_count);\
	memmove(arr->data + at_place + value_count, arr->data + at_place, sizeof(*arr->data)*move_count);\
	memcpy(arr->data + at_place, values, sizeof(*arr->data)*value_count);\
	arr->size += value_count;\
}\
void qc_erase_array(V)(QC_Array(V) *arr, int at_place, int erase_count)\
{\
	QC_ASSERT(arr);\
	QC_ASSERT(at_place >= 0 && at_place < arr->size);\
	QC_ASSERT(at_place + erase_count <= arr->size);\
	QC_ASSERT(erase_count >= 0);\
	memmove(arr->data + at_place, arr->data + at_place + erase_count, sizeof(*arr->data)*(arr->size - at_place - erase_count));\
	arr->size -= erase_count;\
}\
V qc_pop_array(V)(QC_Array(V) *arr)\
{\
	QC_ASSERT(arr);\
	QC_ASSERT(arr->size > 0);\
	--arr->size;\
	return arr->data[arr->size];\
}\
QC_Array(V) qc_copy_array(V)(QC_Array(V) *arr)\
{\
	QC_Array(V) copy = {0};\
	copy.data = (V*)QC_MALLOC(arr->capacity*sizeof(*arr->data));\
	memcpy(copy.data, arr->data, arr->size*sizeof(*arr->data));\
	copy.size = arr->size;\
	copy.capacity = arr->capacity;\
	return copy;\
}\
void qc_clear_array(V)(QC_Array(V) *arr)\
{\
	QC_ASSERT(arr);\
	arr->size = 0;\
}\


/* Hashing */

/* Hash "template" */
#define qc_hash(V) QC_JOIN2(qc_hash_, V)

/* Hash functions should avoid generating neighbouring qc_hashes easily (linear probing) */
static QC_U32 qc_hash(QC_Void_Ptr)(QC_Void_Ptr value) { return (QC_U32)(((QC_U64)value)/2); }



/* Hash table */
/* @todo Automatic resizing */

/* Key_Value */
#define QC_KV(K, V) QC_JOIN3(K, _, V)

#define qc_create_tbl(K, V) QC_JOIN3(qc_create_, QC_KV(K, V), _tbl)
#define qc_destroy_tbl(K, V) QC_JOIN3(qc_destroy_, QC_KV(K, V), _tbl)
#define qc_get_tbl(K, V) QC_JOIN3(get_, QC_KV(K, V), _tbl)
#define qc_set_tbl(K, V) QC_JOIN3(set_, QC_KV(K, V), _tbl)
#define qc_null_tbl_entry(K, V) QC_JOIN3(null_, QC_KV(K, V), _tbl_entry)
#define QC_Hash_Table(K, V) QC_JOIN2(QC_KV(K, V), _Tbl)
#define QC_Hash_Table_Entry(K, V) QC_JOIN2(QC_KV(K, V), _Tbl_Entry)

#define QC_DECLARE_HASH_TABLE(K, V)\
struct QC_Hash_Table_Entry(K, V);\
\
typedef struct QC_Hash_Table(K, V) {\
	struct QC_Hash_Table_Entry(K, V) *array;\
	int array_size;\
	int count;\
	K null_key;\
	V null_value;\
} QC_Hash_Table(K, V);\
typedef struct QC_Hash_Table_Entry(K, V) {\
	K key;\
	V value;\
} QC_Hash_Table_Entry(K, V);\
\
QC_Hash_Table(K, V) qc_create_tbl(K, V)(	K null_key, V null_value, int max_size);\
void qc_destroy_tbl(K, V)(QC_Hash_Table(K, V) *tbl);\
\
V qc_get_tbl(K, V)(QC_Hash_Table(K, V) *tbl, K key);\
void qc_set_tbl(K, V)(QC_Hash_Table(K, V) *tbl, K key, V value);\

#define DEFINE_HASH_TABLE(K, V)\
QC_Hash_Table_Entry(K, V) qc_null_tbl_entry(K, V)(QC_Hash_Table(K, V) *tbl)\
{\
	QC_Hash_Table_Entry(K, V) e = {0};\
	e.key = tbl->null_key;\
	e.value = tbl->null_value;\
	return e;\
}\
\
QC_Hash_Table(K, V) qc_create_tbl(K, V)(K null_key, V null_value, int max_size)\
{\
	int i;\
	QC_Hash_Table(K, V) tbl = {0};\
	tbl.null_key = null_key;\
	tbl.null_value = null_value;\
	tbl.array_size = max_size;\
	tbl.array = QC_MALLOC(sizeof(*tbl.array)*max_size);\
	for (i = 0; i < max_size; ++i)\
		tbl.array[i] = qc_null_tbl_entry(K, V)(&tbl);\
	return tbl;\
}\
\
void qc_destroy_tbl(K, V)(QC_Hash_Table(K, V) *tbl)\
{\
	QC_FREE(tbl->array);\
	tbl->array = NULL;\
}\
\
V qc_get_tbl(K, V)(QC_Hash_Table(K, V) *tbl, K key)\
{\
	int ix = qc_hash(K)(key) % tbl->array_size;\
	/* Linear probing */\
	/* Should not be infinite because set_id_handle_tbl asserts if table is full */\
	while (tbl->array[ix].key != key && tbl->array[ix].key != tbl->null_key)\
		ix= (ix + 1) % tbl->array_size;\
\
	if (tbl->array[ix].key == tbl->null_key)\
		QC_ASSERT(tbl->array[ix].value == tbl->null_value);\
\
	return tbl->array[ix].value;\
}\
\
void qc_set_tbl(K, V)(QC_Hash_Table(K, V) *tbl, K key, V value)\
{\
	int ix = qc_hash(K)(key) % tbl->array_size;\
	QC_ASSERT(key != tbl->null_key);\
\
	/* Linear probing */\
	while (tbl->array[ix].key != key && tbl->array[ix].key != tbl->null_key)\
		ix = (ix + 1) % tbl->array_size;\
\
	{\
		QC_Hash_Table_Entry(K, V) *entry = &tbl->array[ix];\
		QC_Bool modify_existing = 	value != tbl->null_value && entry->key != tbl->null_key;\
		QC_Bool insert_new =		value != tbl->null_value && entry->key == tbl->null_key;\
		QC_Bool remove_existing =	value == tbl->null_value && entry->key != tbl->null_key;\
		QC_Bool remove_new =		value == tbl->null_value && entry->key == tbl->null_key;\
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
			entry->value = tbl->null_value;\
			QC_ASSERT(tbl->count > 0);\
			--tbl->count;\
	\
			/* Rehash */\
			ix= (ix + 1) % tbl->array_size;\
			while (tbl->array[ix].key != tbl->null_key) {\
				QC_Hash_Table_Entry(K, V) e = tbl->array[ix];\
				tbl->array[ix] = qc_null_tbl_entry(K, V)(tbl);\
				--tbl->count;\
				qc_set_tbl(K, V)(tbl, e.key, e.value);\
	\
				ix= (ix + 1) % tbl->array_size;\
			}\
		} else if (remove_new) {\
			/* Nothing to be removed */\
		} else {\
			QC_FAIL(("Hash table logic failed"));\
		}\
	}\
\
	QC_ASSERT(tbl->count < tbl->array_size);\
}\

QC_DECLARE_ARRAY(char)
QC_DECLARE_ARRAY(int)

/* @todo Make this safe.. */
void qc_safe_vsprintf(QC_Array(char) *buf, const char *fmt, va_list args);
void qc_append_str(QC_Array(char) *buf, const char *fmt, ...);

#endif
