#include "hashtable.h"

internal
Id_Handle_Tbl_Entry null_id_handle_tbl_entry()
{ return (Id_Handle_Tbl_Entry) { .key= NULL_ID, .value= NULL_HANDLE }; }

Id_Handle_Tbl create_id_handle_tbl(Ator ator, U32 max_size)
{
	Id_Handle_Tbl tbl= {};
	tbl.ator= ator;
	tbl.array_size= max_size;
	tbl.array= ALLOC(ator, sizeof(*tbl.array)*max_size, "tbl.array");
	for (U32 i= 0; i < max_size; ++i)
		tbl.array[i]= null_id_handle_tbl_entry();
	return tbl;
}

void destroy_id_handle_tbl(Id_Handle_Tbl *tbl)
{
	FREE(tbl->ator, tbl->array);
	tbl->array= NULL;
}

Handle get_id_handle_tbl(Id_Handle_Tbl *tbl, Id key)
{
	U32 ix= hash_u64(key) % tbl->array_size;
	// Should not be infinite because set_id_handle_tbl asserts if table is full
	while (tbl->array[ix].key != key && tbl->array[ix].key != NULL_ID)
		ix= (ix + 1) % tbl->array_size;

	if (tbl->array[ix].key == NULL_ID)
		ensure(tbl->array[ix].value);

	return tbl->array[ix].value;
}

void set_id_handle_tbl(Id_Handle_Tbl *tbl, Id key, Handle value)
{
	U32 ix= hash_u64(key) % tbl->array_size;

	while (tbl->array[ix].key != key && tbl->array[ix].key != NULL_ID)
		ix= (ix + 1) % tbl->array_size;

	Id_Handle_Tbl_Entry *entry= &tbl->array[ix];
	bool modify_existing= 	value != NULL_HANDLE && entry->key != NULL_ID;
	bool insert_new=		value != NULL_HANDLE && entry->key == NULL_ID;
	bool remove_existing=	value == NULL_HANDLE && entry->key != NULL_ID;
	bool remove_new=		value == NULL_HANDLE && entry->key == NULL_ID;

	if (modify_existing) {
		entry->value= value;
	} else if (insert_new) {
		entry->key= key;
		entry->value= value;
		++tbl->count;
	} else if (remove_existing) {
		entry->key= NULL_ID;
		entry->value= NULL_HANDLE;
		--tbl->count;

		// Rehash
		ix= (ix + 1) % tbl->array_size;
		while (tbl->array[ix].key != NULL_ID) {
			Id_Handle_Tbl_Entry e= tbl->array[ix];
			tbl->array[ix]= null_id_handle_tbl_entry();
			set_id_handle_tbl(tbl, e.key, e.value);

			ix= (ix + 1) % tbl->array_size;
		}
	} else if (remove_new) {
		// Nothing to be removed
	} else {
		fail("Hash table logic failed");
	}

	ensure(tbl->count < tbl->array_size);

}
