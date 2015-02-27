#include "rtti.h"
#include "platform/dll.h"

/// @todo Search all loaded dll's

U32 struct_size(const char *struct_name)
{
	char postfix[]= "_size";
	char size_sym[strlen(struct_name) + strlen(postfix) + 1];
	snprintf(size_sym, sizeof(size_sym), "%s%s", struct_name, postfix);
	U32 *size_ptr= (U32*)query_dll_sym(main_program_dll, size_sym);
	if (!size_ptr)
		fail("struct_size: Couldn't find struct: %s", struct_name);
	return *size_ptr;
}

void * func_ptr(const char *func_name)
{ return query_dll_sym(main_program_dll, func_name); }

internal
U32 member_size_by_index(const char *struct_name, U32 member_i)
{
	char postfix[]= "_member_sizes";
	char size_sym[strlen(struct_name) + strlen(postfix) + 1];
	snprintf(size_sym, sizeof(size_sym), "%s%s", struct_name, postfix);
	U32 *sizes= (U32*)query_dll_sym(main_program_dll, size_sym);
	if (!sizes)
		fail("member_size_by_index: Couldn't find member sizes: %s",
				struct_name);
	return sizes[member_i];
}

internal
U32 member_offset_by_index(const char *struct_name, U32 member_i)
{
	char postfix[]= "_member_offsets";
	char size_sym[strlen(struct_name) + strlen(postfix) + 1];
	snprintf(size_sym, sizeof(size_sym), "%s%s", struct_name, postfix);
	U32 *offsets= (U32*)query_dll_sym(main_program_dll, size_sym);
	if (!offsets)
		fail("member_offset_by_index: Couldn't find member offsets: %s",
				struct_name);
	return offsets[member_i];
}

internal
U32 member_index_by_name(const char *struct_name, const char *member_name)
{
	char postfix[]= "_member_names";
	char size_sym[strlen(struct_name) + strlen(postfix) + 1];
	snprintf(size_sym, sizeof(size_sym), "%s%s", struct_name, postfix);
	const char **names= (const char**)query_dll_sym(main_program_dll, size_sym);
	if (!names)
		fail("Couldn't find: %s::%s", struct_name, member_name);

	U32 i= 0;
	while (names[i] && strcmp(names[i], member_name))
		++i;
	if (!names[i])
		fail("Couldn't find: %s::%s", struct_name, member_name);
	return i;
}

U32 member_size(const char *struct_name, const char *member_name)
{
	return member_size_by_index(
				struct_name,
				member_index_by_name(struct_name, member_name));
}

U32 member_offset(const char *struct_name, const char *member_name)
{
	return member_offset_by_index(
				struct_name,
				member_index_by_name(struct_name, member_name));
}

