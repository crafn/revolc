#include "rtti.h"
#include "platform/dll.h"

void * query_sym_concat(const char* a, const char *b)
{
	ensure(g_env.resblob && "Can't use RTTI before resources are loaded");
	char name[strlen(a) + strlen(b) + 1];
	snprintf(name, sizeof(name), "%s%s", a, b);

	// Check first if symbol is already in use
	/// @todo O(1) search
	SymbolTable *tbl= g_env.used_rtti_symbols;
	for (U32 i= 0; i < tbl->symbol_count; ++i) {
		Symbol *sym= &tbl->symbols[i];
		if (!strcmp(name, sym->name)) {
			ensure(sym->addr);
			return sym->addr;
		}
	}

	U32 mod_start_i;
	U32 mod_count;
	all_res_by_type(&mod_start_i, &mod_count,
					g_env.resblob, ResType_Module);

	for (U32 i= mod_start_i; i < mod_start_i + mod_count; ++i) {
		const Module *mod= (Module*)res_by_index(g_env.resblob, i);
		DllHandle h= mod->dll;
		void *addr= query_dll_sym(h, name);
		if (addr) {
			// Add symbol to table
			ensure(tbl->symbol_count < MAX_SYM_COUNT);
			Symbol sym= {};
			snprintf(	sym.module_name, sizeof(sym.module_name), "%s",
						mod->res.name);
			snprintf(	sym.name, sizeof(sym.name), "%s",
						name);
			sym.addr= addr;
			tbl->symbols[tbl->symbol_count++]= sym;

			return addr;
		}
	}
	return NULL;
}

U32 rtti_struct_size(const char *struct_name)
{
	U32 *size_ptr= query_sym_concat(struct_name, "_size");
	if (!size_ptr)
		fail("struct_size: Couldn't find struct: %s", struct_name);
	return *size_ptr;
}

void * rtti_func_ptr(const char *func_name)
{ return query_sym_concat(func_name, ""); }

internal
U32 member_size_by_index(const char *struct_name, U32 member_i)
{
	U32 *sizes= query_sym_concat(struct_name, "_member_sizes");
	if (!sizes)
		fail("member_size_by_index: Couldn't find member sizes: %s",
				struct_name);
	return sizes[member_i];
}

internal
U32 member_offset_by_index(const char *struct_name, U32 member_i)
{
	U32 *offsets= query_sym_concat(struct_name, "_member_offsets");
	if (!offsets)
		fail("member_offset_by_index: Couldn't find member offsets: %s",
				struct_name);
	return offsets[member_i];
}

internal
const char * member_type_name_by_index(const char *struct_name, U32 member_i)
{
	const char **type_names= query_sym_concat(struct_name, "_member_type_names");
	if (!type_names)
		fail("member_type_name_by_index: Couldn't find member offsets: %s",
				struct_name);
	return type_names[member_i];
}

internal
U32 member_index_by_name(const char *struct_name, const char *member_name)
{
	const char **names= query_sym_concat(struct_name, "_member_names");
	if (!names)
		fail("Couldn't find: %s::%s", struct_name, member_name);

	U32 i= 0;
	while (names[i] && strcmp(names[i], member_name))
		++i;
	if (!names[i])
		fail("Couldn't find: %s::%s", struct_name, member_name);
	return i;
}

U32 rtti_member_size(const char *struct_name, const char *member_name)
{
	return member_size_by_index(
				struct_name,
				member_index_by_name(struct_name, member_name));
}

U32 rtti_member_offset(const char *struct_name, const char *member_name)
{
	return member_offset_by_index(
				struct_name,
				member_index_by_name(struct_name, member_name));
}

const char * rtti_member_type_name(const char *struct_name, const char *member_name)
{
	return member_type_name_by_index(
				struct_name,
				member_index_by_name(struct_name, member_name));
}

const char * rtti_sym_name(void *ptr)
{
	const SymbolTable *tbl= g_env.used_rtti_symbols;
	/// @todo O(1) search
	for (U32 i= 0; i < tbl->symbol_count; ++i) {
		const Symbol *sym= &tbl->symbols[i];
		if (sym->addr == ptr)
			return sym->name;
	}
	fail("rtti_sym_name: Symbol not found: %p", ptr);
}

void * rtti_relocate_sym(void *possibly_invalidated_ptr)
{
	const SymbolTable *tbl= g_env.used_rtti_symbols;
	/// @todo O(1) search
	for (U32 i= 0; i < tbl->symbol_count; ++i) {
		const Symbol *sym= &tbl->symbols[i];
		if (sym->old_addr == possibly_invalidated_ptr) {
			return sym->addr;
		}
	}

	fail("rtti_relocate_sym: Symbol not found: %p", possibly_invalidated_ptr);
}

void rtti_requery_syms()
{
	SymbolTable *tbl= g_env.used_rtti_symbols;
	for (U32 i= 0; i < tbl->symbol_count; ++i) {
		Symbol *sym= &tbl->symbols[i];

		sym->old_addr= sym->addr;
		const Module *mod= (Module*)res_by_name(	g_env.resblob,
													ResType_Module,
													sym->module_name);
		sym->addr= query_dll_sym(mod->dll, sym->name);
		ensure(sym->addr);
	}
}

