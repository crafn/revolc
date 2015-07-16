#include "archive.h"

// @note No caring about endianness


// ArchiveType_measure

void measure_pack_u32(WArchive *ar, const U32 *value)
{ ar->data_size += sizeof(*value); }
void measure_pack_u64(WArchive *ar, const U64 *value)
{ ar->data_size += sizeof(*value); }
void measure_pack_f32(WArchive *ar, const F32 *value)
{ ar->data_size += sizeof(*value); }
void measure_pack_f64(WArchive *ar, const F64 *value)
{ ar->data_size += sizeof(*value); }
void measure_pack_buf(WArchive *ar, const void *data, U32 data_size)
{ ar->data_size += data_size; }
void measure_pack_strbuf(WArchive *ar, const char *str, U32 str_max_size)
{ ar->data_size += str_max_size; }




// ArchiveType_binary
// @todo Compression

void binary_pack_buf(WArchive *ar, const void *data, U32 data_size);
void binary_pack_u32(WArchive *ar, const U32 *value)
{ binary_pack_buf(ar, value, sizeof(*value)); }
void binary_pack_u64(WArchive *ar, const U64 *value)
{ binary_pack_buf(ar, value, sizeof(*value)); }
void binary_pack_f32(WArchive *ar, const F32 *value)
{ binary_pack_buf(ar, value, sizeof(*value)); }
void binary_pack_f64(WArchive *ar, const F64 *value)
{ binary_pack_buf(ar, value, sizeof(*value)); }
void binary_pack_buf(WArchive *ar, const void *data, U32 data_size)
{
	if (ar->data_size + data_size > ar->data_capacity)
		fail(	"WArchive capacity exceeded: %i > %i (@todo resize)",
				ar->data_size + data_size, ar->data_capacity);

	U8 *begin= ar->data + ar->data_size;
	memcpy(begin, data, data_size);
	ar->data_size += data_size;
}
void binary_pack_strbuf(WArchive *ar, const char *str, U32 str_max_size)
{ binary_pack_buf(ar, str, str_max_size); }


void binary_unpack_buf(RArchive *ar, void *data, U32 data_size);
void binary_unpack_u32(RArchive *ar, U32 *value)
{ binary_unpack_buf(ar, value, sizeof(*value)); }
void binary_unpack_u64(RArchive *ar, U64 *value)
{ binary_unpack_buf(ar, value, sizeof(*value)); }
void binary_unpack_f32(RArchive *ar, F32 *value)
{ binary_unpack_buf(ar, value, sizeof(*value)); }
void binary_unpack_f64(RArchive *ar, F64 *value)
{ binary_unpack_buf(ar, value, sizeof(*value)); }
void binary_unpack_buf(RArchive *ar, void *data, U32 data_size)
{
	if (ar->offset + data_size > ar->data_size)
		fail("RArchive capacity exceeded");

	memcpy(data, ar->data + ar->offset, data_size);
	ar->offset += data_size;
}
void binary_unpack_strbuf(RArchive *ar, char *str, U32 str_max_size)
{ binary_unpack_buf(ar, str, str_max_size); str[str_max_size - 1]= '\0'; }



// ArchiveType to pack-function -tables

internal
void (*pack_u32_funcs[])(WArchive *, const U32 *)= {
	measure_pack_u32,
	binary_pack_u32,
};

internal
void (*pack_u64_funcs[])(WArchive *, const U64 *)= {
	measure_pack_u64,
	binary_pack_u64,
};

internal
void (*pack_f32_funcs[])(WArchive *, const F32 *)= {
	measure_pack_f32,
	binary_pack_f32,
};

internal
void (*pack_f64_funcs[])(WArchive *, const F64 *)= {
	measure_pack_f64,
	binary_pack_f64,
};

internal
void (*pack_buf_funcs[])(WArchive *, const void *, U32)= {
	measure_pack_buf,
	binary_pack_buf,
};

internal
void (*pack_strbuf_funcs[])(WArchive *, const char *, U32)= {
	measure_pack_strbuf,
	binary_pack_strbuf,
};



internal
void (*unpack_u32_funcs[])(RArchive *, U32 *)= {
	NULL,
	binary_unpack_u32,
};

internal
void (*unpack_u64_funcs[])(RArchive *, U64 *)= {
	NULL,
	binary_unpack_u64,
};

internal
void (*unpack_f32_funcs[])(RArchive *, F32 *)= {
	NULL,
	binary_unpack_f32,
};

internal
void (*unpack_f64_funcs[])(RArchive *, F64 *)= {
	NULL,
	binary_unpack_f64,
};

internal
void (*unpack_buf_funcs[])(RArchive *, void *, U32)= {
	NULL,
	binary_unpack_buf,
};

internal
void (*unpack_strbuf_funcs[])(RArchive *, char *, U32)= {
	NULL,
	binary_unpack_strbuf,
};



// Public

WArchive create_warchive(ArchiveType t, U32 capacity)
{
	return (WArchive) {
		.type= t,
		.data= frame_alloc(capacity),
		.data_size= 0,
		.data_capacity= capacity,
	};
}

void destroy_warchive(WArchive *ar)
{
	*ar= (WArchive) {};
}

RArchive create_rarchive(ArchiveType t, const void *data, U32 data_size)
{
	ensure(t == ArchiveType_binary);
	return (RArchive) {
		.type= t,
		.offset= 0,
		.data= data,
		.data_size= data_size,
	};
}

void destroy_rarchive(RArchive *ar)
{
	*ar= (RArchive) {};
}

void pack_u32(WArchive *ar, const U32 *value)
{ pack_u32_funcs[ar->type](ar, value); }
void pack_u64(WArchive *ar, const U64 *value)
{ pack_u64_funcs[ar->type](ar, value); }
void pack_f32(WArchive *ar, const F32 *value)
{ pack_f32_funcs[ar->type](ar, value); }
void pack_f64(WArchive *ar, const F64 *value)
{ pack_f64_funcs[ar->type](ar, value); }
void pack_buf(WArchive *ar, const void *data, U32 data_size)
{ pack_buf_funcs[ar->type](ar, data, data_size); }
void pack_strbuf(WArchive *ar, const char *str, U32 str_max_size)
{ pack_strbuf_funcs[ar->type](ar, str, str_max_size); }

void pack_buf_patch(WArchive *ar, U32 offset, const void *data, U32 data_size)
{
	U32 end= ar->data_size;
	ar->data_size= offset;
	pack_buf(ar, data, data_size);
	ar->data_size= end;
}

void unpack_u32(RArchive *ar, U32 *value)
{ unpack_u32_funcs[ar->type](ar, value); }
void unpack_u64(RArchive *ar, U64 *value)
{ unpack_u64_funcs[ar->type](ar, value); }
void unpack_f32(RArchive *ar, F32 *value)
{ unpack_f32_funcs[ar->type](ar, value); }
void unpack_f64(RArchive *ar, F64 *value)
{ unpack_f64_funcs[ar->type](ar, value); }
void unpack_buf(RArchive *ar, void *data, U32 data_size)
{ unpack_buf_funcs[ar->type](ar, data, data_size); }
void unpack_strbuf(RArchive *ar, char *str, U32 str_max_size)
{ unpack_strbuf_funcs[ar->type](ar, str, str_max_size); }
