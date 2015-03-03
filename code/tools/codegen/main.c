#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sparse/lib.h>
#include <sparse/allocate.h>
#include <sparse/token.h>
#include <sparse/parse.h>
#include <sparse/scope.h>
#include <sparse/symbol.h>
#include <sparse/expression.h>
#include <sparse/linearize.h>

#define MAX_STRUCT_COUNT (1024*4)
#define MAX_STRUCT_MEMBER_COUNT 128

typedef struct StructInfo {
	struct symbol *sym;
	struct symbol *member_syms[MAX_STRUCT_MEMBER_COUNT];
	int member_count;
} StructInfo;

StructInfo struct_infos[MAX_STRUCT_COUNT];
int struct_info_count;

static void examine_namespace(struct symbol *sym)
{
	if (sym->ident && sym->ident->reserved)
		return;

	switch(sym->namespace) {
	case NS_MACRO:
		break;
	case NS_STRUCT: {
		if (sym->type == SYM_UNION)
			break;
		if (!sym->ident)
			break;
		if (!sym->ident->name)
			break;
		if (sym->ident->name[0] == '_')
			break;
		if (sym->ident->name[0] > 'Z')
			break;
		if (	sym->ident->name[0] == 'L' &&
				sym->ident->name[1] == 'o' &&
				sym->ident->name[2] == 'd' &&
				sym->ident->name[3] == 'e') // Lode png lib
			break;
		if (	sym->ident->name[0] == 'P' &&
				sym->ident->name[1] == 'a') // PortAudio
			break;

		{
			int illegal= 0;
			int count= 0;
			struct symbol *m= NULL;
			FOR_EACH_PTR(sym->symbol_list, m) {
				++count;
				if (!m->ident || !m->ident->name || m->ident->name[0] == '_') {
					illegal= 1;
					break;
				}
			} END_FOR_EACH_PTR(m);
			if (illegal || count == 0)
				break;
		}

		assert(struct_info_count < MAX_STRUCT_COUNT);
		StructInfo *s= &struct_infos[struct_info_count++];
		s->sym= sym;

		struct symbol *m;
		FOR_EACH_PTR(sym->symbol_list, m) {
			assert(s->member_count < MAX_STRUCT_MEMBER_COUNT);
			s->member_syms[s->member_count++]= m;
		} END_FOR_EACH_PTR(m);
		break;
	} case NS_TYPEDEF:
	case NS_SYMBOL:
	case NS_NONE:
	case NS_LABEL:
	case NS_ITERATOR:
	case NS_UNDEF:
	case NS_PREPROCESSOR:
	case NS_KEYWORD:
		break;
	default:
		die("Unrecognised namespace type %d",sym->namespace);
	}
}

static inline void examine_symbol_list(const char *file, struct symbol_list *list)
{
	struct symbol *sym;

	if (!list)
		return;
	FOR_EACH_PTR(list, sym) {
			examine_namespace(sym);
	} END_FOR_EACH_PTR(sym);
}

int main(int argc, char **argv)
{
	/// @todo Don't hard-code
	char *sparse_argv[]= {
		"codegen",
		"-I/usr/include/",
		"-I/usr/include/x86_64-linux-gnu/",
		"-I/usr/lib/gcc/x86_64-linux-gnu/4.8/include/",
		"-I./source/",
		"-I./deps/common/",
		"-I./deps/common/chipmunk/include/",
		"-DCODEGEN",
		"./source/unity.c",
		NULL
	};
	const int sparse_argc= sizeof(sparse_argv)/sizeof(*sparse_argv);

	struct string_list *filelist = NULL;
	char *file;
	sparse_initialize(sparse_argc, sparse_argv, &filelist);
	FOR_EACH_PTR_NOTAG(filelist, file) {
		sparse_keep_tokens(file);
		examine_symbol_list(file, file_scope->symbols);
	} END_FOR_EACH_PTR_NOTAG(file);

	//
	// Code generation
	//

	FILE *h= fopen("./source/global/generated_rtti.h", "wb");
	assert(h);
	FILE *c= fopen("./source/global/generated_rtti.c", "wb");
	assert(h);
	assert(c);

	fprintf(h, "#ifndef REVOLC_GLOBAL_RTTI_H\n");
	fprintf(h, "#define REVOLC_GLOBAL_RTTI_H\n\n");
	fprintf(h, "// This file is automatically generated (codegen)\n\n");

	fprintf(c, "#include \"generated_rtti.h\"\n\n");

	for (int i= 0; i < struct_info_count; ++i) {
		StructInfo *s= &struct_infos[i];

		// Struct size
		fprintf(h, "extern const U32 %s_size;\n", s->sym->ident->name);
		fprintf(c, "const U32 %s_size= sizeof(%s);\n", s->sym->ident->name, s->sym->ident->name);

		// Member names
		fprintf(h, "extern const char *%s_member_names[];\n", s->sym->ident->name);
		fprintf(c, "const char *%s_member_names[]= {\n", s->sym->ident->name);
		for (int k= 0; k < s->member_count; ++k) {
			fprintf(c, "	\"%s\",\n", s->member_syms[k]->ident->name);
		}
		fprintf(c, "	NULL\n};\n");

		// Member sizes
		fprintf(h, "extern const U32 %s_member_sizes[];\n", s->sym->ident->name);
		fprintf(c, "const U32 %s_member_sizes[]= {", s->sym->ident->name);
		for (int k= 0; k < s->member_count; ++k) {
			struct symbol *m= get_base_type(s->member_syms[k]);
			int byte_size= m->bit_size/8;
			if (byte_size == 0) // Happens with bools for some reason
				byte_size= 1;
			fprintf(c, "%i, ", byte_size);
		}
		fprintf(c, "};\n");

		// Member offsets
		fprintf(h, "extern const U32 %s_member_offsets[];\n", s->sym->ident->name);
		fprintf(c, "const U32 %s_member_offsets[]= {\n", s->sym->ident->name);
		for (int k= 0; k < s->member_count; ++k) {
			struct symbol *m= s->member_syms[k];
			fprintf(c, "	offsetof(%s, %s),\n",
					s->sym->ident->name,
					m->ident->name);
		}
		fprintf(c, "};\n");
	}
	fprintf(h, "\n#endif // REVOLC_GLOBAL_RTTI_H\n");

	fclose(h);
	fclose(c);
	return 0;
}

