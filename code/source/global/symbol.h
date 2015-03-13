#ifndef REVOLC_GLOBAL_SYMBOL_H
#define REVOLC_GLOBAL_SYMBOL_H

#include "build.h"
#include "global/cfg.h"

typedef struct Symbol {
	char module_name[RES_NAME_SIZE];
	char name[SYM_NAME_SIZE];
	void *addr;
	void *old_addr; // Address before last dll reload
} Symbol;

typedef struct SymbolTable {
	Symbol symbols[MAX_SYM_COUNT];
	U32 symbol_count;
} SymbolTable;

#endif // REVOLC_GLOBAL_SYMBOL_H
