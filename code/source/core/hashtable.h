#ifndef REVOLC_CORE_HASHTABLE_H
#define REVOLC_CORE_HASHTABLE_H

#include "build.h"
#include "core/hash.h"
#include "core/memory.h"

struct Id_Handle_Tbl_Entry;

// Maps U64 keys to U32 values
// @todo Collision statistics
typedef struct Id_Handle_Tbl {
	Ator ator;
	struct Id_Handle_Tbl_Entry *array;
	U32 array_size;
	U32 count;
} Id_Handle_Tbl;

typedef struct Id_Handle_Tbl_Entry {
	Id key;
	Handle value;
} Id_Handle_Tbl_Entry;

REVOLC_API Id_Handle_Tbl create_id_handle_tbl(Ator ator, U32 max_size);
REVOLC_API void destroy_id_handle_tbl(Id_Handle_Tbl *tbl);

REVOLC_API Handle get_id_handle_tbl(Id_Handle_Tbl *tbl, Id key);
REVOLC_API void set_id_handle_tbl(Id_Handle_Tbl *tbl, Id key, Handle value);

#endif // REVOLC_CORE_HASHTABLE_H
