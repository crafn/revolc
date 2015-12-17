#ifndef REVOLC_CORE_ARCHIVE_H
#define REVOLC_CORE_ARCHIVE_H

#include "build.h"
#include "core/memory.h"

typedef enum ArchiveType {
	ArchiveType_measure, // Doesn't write anything
	ArchiveType_binary, // Binary
	// @todo Think about json archive (replacing hand-written json serialization)
} ArchiveType;

typedef struct WArchive {
	ArchiveType type;
	Ator *ator;
	U8 *data;
	U32 data_size; // @todo Rename to offset
	U32 data_capacity;
} WArchive;

typedef struct RArchive {
	ArchiveType type;
	U32 offset;
	const U8 *data;
	U32 data_size;
} RArchive;

REVOLC_API WArchive create_warchive(ArchiveType t, Ator *ator, U32 capacity);
REVOLC_API void destroy_warchive(WArchive *ar);

REVOLC_API RArchive create_rarchive(ArchiveType t, const void *data, U32 data_size);
REVOLC_API void destroy_rarchive(RArchive *ar);

// @todo Options parameters (range, precision, ...)
REVOLC_API void pack_u32(WArchive *ar, const U32 *value);
REVOLC_API void pack_u64(WArchive *ar, const U64 *value);
REVOLC_API void pack_s32(WArchive *ar, const S32 *value);
REVOLC_API void pack_s64(WArchive *ar, const S64 *value);
REVOLC_API void pack_f32(WArchive *ar, const F32 *value);
REVOLC_API void pack_f64(WArchive *ar, const F64 *value);
REVOLC_API void pack_buf(WArchive *ar, const void *data, U32 data_size);
REVOLC_API void pack_strbuf(WArchive *ar, const char *str, U32 str_max_size);

REVOLC_API void pack_buf_patch(WArchive *ar, U32 offset, const void *data, U32 data_size);

REVOLC_API void unpack_u32(RArchive *ar, U32 *value);
REVOLC_API void unpack_u64(RArchive *ar, U64 *value);
REVOLC_API void unpack_s32(RArchive *ar, S32 *value);
REVOLC_API void unpack_s64(RArchive *ar, S64 *value);
REVOLC_API void unpack_f32(RArchive *ar, F32 *value);
REVOLC_API void unpack_f64(RArchive *ar, F64 *value);
REVOLC_API void unpack_buf(RArchive *ar, void *data, U32 data_size);
REVOLC_API void unpack_strbuf(RArchive *ar, char *str, U32 str_max_size);

// Compound types

struct T3d;
REVOLC_API void pack_t3d(WArchive *ar, const struct T3d *tf);
REVOLC_API void unpack_t3d(RArchive *ar, struct T3d *tf);

#endif // REVOLC_CORE_ARCHIVE_H
