#ifndef REVOLC_RTS_MINION_H
#define REVOLC_RTS_MINION_H

#include "core/vector.h"
#include "build.h"

struct WArchive;
struct RArchive;
typedef struct Minion {
	V3d pos;
} Minion;

MOD_API void upd_minion(Minion *minion_begin, Minion *minion_end);
MOD_API void pack_minion(struct WArchive *ar, Minion *minion);
MOD_API void unpack_minion(struct RArchive *ar, Minion *minion);

#endif // REVOLC_RTS_MINION_H
