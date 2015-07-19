#ifndef REVOLC_RTS_MINION_H
#define REVOLC_RTS_MINION_H

#include "core/vector.h"
#include "build.h"

struct WArchive;
struct RArchive;
typedef struct Minion {
	V3d pos;
	F32 health;
} Minion;

MOD_API void upd_minion(Minion *begin, Minion *end);
MOD_API void pack_minion(struct WArchive *ar, const Minion *begin, const Minion *end);
MOD_API void unpack_minion(struct RArchive *ar, Minion *begin, Minion *end);

#endif // REVOLC_RTS_MINION_H
