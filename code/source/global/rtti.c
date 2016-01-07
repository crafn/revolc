#include "core/dll.h"
#include "rtti.h"

void * query_sym_concat(const char* a, const char *b)
{
	ensure(g_env.resblob && "Can't use RTTI before resources are loaded");
	char name[strlen(a) + strlen(b) + 1];
	fmt_str(name, sizeof(name), "%s%s", a, b);

	// Check first if symbol is already in use
	/// @todo O(1) search
	SymbolTable *tbl = g_env.used_rtti_symbols;
	for (U32 i = 0; i < tbl->symbol_count; ++i) {
		Symbol *sym = &tbl->symbols[i];
		if (!strcmp(name, sym->name)) {
			ensure(sym->addr);
			return sym->addr;
		}
	}

	U32 mod_count;
	Module **modules =
		(Module **)all_res_by_type(	&mod_count,
									g_env.resblob,
									ResType_Module);

	for (U32 i = 0; i < mod_count; ++i) {
		const Module *mod = modules[i];
		DllHandle h = mod->dll;
		void *addr = query_dll_sym(h, name);
		if (addr) {
			// Add symbol to table
			ensure(tbl->symbol_count < MAX_SYM_COUNT);
			Symbol sym = {};
			fmt_str(	sym.module_name, sizeof(sym.module_name), "%s",
						mod->res.name);
			fmt_str(	sym.name, sizeof(sym.name), "%s",
						name);
			sym.addr = addr;
			tbl->symbols[tbl->symbol_count++] = sym;

			return addr;
		}
	}
	return NULL;
}

StructRtti *rtti_struct(const char *struct_name)
{
	StructRtti *s = query_sym_concat(struct_name, "_rtti");
	return s;
}

void * rtti_func_ptr(const char *func_name)
{ return query_sym_concat(func_name, ""); }

U32 rtti_member_index(const char *struct_name, const char *member_name)
{
	StructRtti *s = rtti_struct(struct_name);
	if (!s)
		return (U32)-1;
	U32 i = 0;
	while (i < s->member_count && strcmp(s->members[i].name, member_name))
		++i;
	if (i >= s->member_count)
		fail("Member not found: %s.%s", struct_name, member_name);
	return i;
}

const char * rtti_sym_name(void *ptr)
{
	const SymbolTable *tbl = g_env.used_rtti_symbols;
	/// @todo O(1) search
	for (U32 i = 0; i < tbl->symbol_count; ++i) {
		const Symbol *sym = &tbl->symbols[i];
		if (sym->addr == ptr)
			return sym->name;
	}
	fail("rtti_sym_name: Symbol not found: %p", ptr);
}

void * rtti_relocate_sym(void *possibly_invalidated_ptr)
{
	const SymbolTable *tbl = g_env.used_rtti_symbols;
	/// @todo O(1) search
	for (U32 i = 0; i < tbl->symbol_count; ++i) {
		const Symbol *sym = &tbl->symbols[i];
		if (sym->old_addr == possibly_invalidated_ptr) {
			return sym->addr;
		}
	}

	fail("rtti_relocate_sym: Symbol not found: %p", possibly_invalidated_ptr);
}

void rtti_requery_syms()
{
	SymbolTable *tbl = g_env.used_rtti_symbols;
	for (U32 i = 0; i < tbl->symbol_count; ++i) {
		Symbol *sym = &tbl->symbols[i];

		sym->old_addr = sym->addr;
		const Module *mod = (Module*)res_by_name(	g_env.resblob,
													ResType_Module,
													sym->module_name);
		sym->addr = query_dll_sym(mod->dll, sym->name);
		ensure(sym->addr);
	}
}

