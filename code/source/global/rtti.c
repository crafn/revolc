#include "rtti.h"
#include "platform/dll.h"

void * query_sym_concat(const char* a, const char *b)
{
	ensure(g_env.resblob && "Can't use RTTI before resources are loaded");

	U32 mod_start_i;
	U32 mod_count;
	all_res_by_type(&mod_start_i, &mod_count,
					g_env.resblob, ResType_Module);

	for (U32 i= mod_start_i; i < mod_start_i + mod_count; ++i) {
		const Module *mod= (Module*)res_by_index(g_env.resblob, i);
		DllHandle h= mod->dll;
		char name[strlen(a) + strlen(b) + 1];
		snprintf(name, sizeof(name), "%s%s", a, b);
		void *sym= query_dll_sym(h, name);
		if (sym)
			return sym;
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
