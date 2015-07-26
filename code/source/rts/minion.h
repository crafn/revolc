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

typedef struct Selection {
	bool selected;
	V3d pos;
} Selection;
// @todo Generate
#define ORIG_TYPE_Selection Selection

MOD_API Handle resurrect_selection(const Selection *dead);
MOD_API void free_selection(Handle h);
MOD_API void *storage_selection();

#endif // REVOLC_RTS_MINION_H
