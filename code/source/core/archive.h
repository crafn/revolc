#ifndef REVOLC_CORE_ARCHIVE_H
#define REVOLC_CORE_ARCHIVE_H

#include "build.h"

typedef enum ArchiveType {
	ArchiveType_measure, // Doesn't write anything
	ArchiveType_binary, // Compressed binary
	// @todo Think about json archive (replacing hand-written json serialization)
} ArchiveType;

typedef struct WArchive {
	ArchiveType type;
	U8 *data;
	U32 data_size;
	U32 data_capacity;
} WArchive;

typedef struct RArchive {
	ArchiveType type;
	const U8 *read_ptr;
	const U8 *data;
	U32 data_size;
} RArchive;

// @todo Allocator, uses frame_alloc for now
WArchive create_warchive(ArchiveType t, U32 capacity);
void destroy_warchive(WArchive *ar);

RArchive create_rarchive(ArchiveType t, const void *data, U32 data_size);
void destroy_rarchive(RArchive *ar);

// @todo Options parameters (range, precision, ...)
void pack_u32(WArchive *ar, const U32 *value);
void pack_u64(WArchive *ar, const U64 *value);
void pack_f32(WArchive *ar, const F32 *value);
void pack_f64(WArchive *ar, const F64 *value);
void pack_buf(WArchive *ar, const void *data, U32 data_size);
void pack_strbuf(WArchive *ar, const char *str, U32 str_max_size);

void unpack_u32(RArchive *ar, U32 *value);
void unpack_u64(RArchive *ar, U64 *value);
void unpack_f32(RArchive *ar, F32 *value);
void unpack_f64(RArchive *ar, F64 *value);
void unpack_buf(RArchive *ar, void *data, U32 data_size);
void unpack_strbuf(RArchive *ar, char *str, U32 str_max_size);

#endif // REVOLC_CORE_ARCHIVE_H
