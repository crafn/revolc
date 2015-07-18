#ifndef REVOLC_CORE_HASH_H
#define REVOLC_CORE_HASH_H

#include "build.h"

typedef U32 Hash;

// Hash functions should avoid generating neighbouring hashes easily (linear probing)
static Hash hash_u32(U32 value) { return value*2; }
static Hash hash_u64(U64 value) { return (value*2) % U32_MAX; }

#endif // REVOLC_CORE_HASH_H
