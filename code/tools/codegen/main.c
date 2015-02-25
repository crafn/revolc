#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sparse/lib.h>
#include <sparse/allocate.h>
#include <sparse/token.h>
#include <sparse/parse.h>
#include <sparse/symbol.h>
#include <sparse/expression.h>
#include <sparse/linearize.h>

static void print_symbols(struct symbol_list *list)
{
	struct symbol *sym;
	int symbol_count= 0;

	FOR_EACH_PTR(list, sym) {
		struct entrypoint *ep;

		expand_symbol(sym);
		ep = linearize_symbol(sym);
		if (ep) {
			printf("symbol: %s\n", ep->name->ident->name);
			++symbol_count;
		}
	} END_FOR_EACH_PTR(sym);

	printf("symbol_count: %i\n", symbol_count);
}

int main(int argc, char **argv)
{
	/// @todo Don't hard-code
	char *sparse_argv[]= {
		"codegen",
		"-I/usr/include/x86_64-linux-gnu/",
		"-I/usr/lib/gcc/x86_64-linux-gnu/4.8/include/",
		"-I./source/",
		"-I./deps/common/",
		"-I./deps/common/chipmunk/include/",
		"./source/unity.c",
		NULL
	};
	const int sparse_argc= sizeof(sparse_argv)/sizeof(*sparse_argv);

	struct string_list *filelist = NULL;
	char *file;

	// Expand, linearize and show it.
	sparse_initialize(sparse_argc, sparse_argv, &filelist);
	FOR_EACH_PTR_NOTAG(filelist, file) {
		print_symbols(sparse(file));
	} END_FOR_EACH_PTR_NOTAG(file);
	return 0;
}
