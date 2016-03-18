#include "archive.h"
#include "core/math.h" // Compound types

// @note No caring about endianness

internal U32 float_size(bool single)
{ return sizeof(float) + (!single)*sizeof(float); }

// ArchiveType_measure

void measure_pack_int(WArchive *ar, const void *value, U32 size)
{ ar->data_size += size; }
void measure_pack_float(WArchive *ar, const void *value, bool single)
{ ar->data_size += float_size(single); }
void measure_pack_buf(WArchive *ar, const void *data, U32 data_size)
{ ar->data_size += data_size; }
void measure_pack_strbuf(WArchive *ar, const char *str, U32 str_max_size)
{ ar->data_size += str_max_size; }



// ArchiveType_binary
// @todo Compression

void binary_pack_buf(WArchive *ar, const void *data, U32 data_size);
void binary_pack_int(WArchive *ar, const void *value, U32 size)
{ binary_pack_buf(ar, value, size); }
void binary_pack_float(WArchive *ar, const void *value, bool single)
{ binary_pack_buf(ar, value, float_size(single)); }
void binary_pack_buf(WArchive *ar, const void *data, U32 data_size)
{
	if (ar->data_size + data_size > ar->data_capacity)
		fail(	"WArchive capacity exceeded: %i > %i (@todo resize)",
				ar->data_size + data_size, ar->data_capacity);

	U8 *begin = ar->data + ar->data_size;
	memcpy(begin, data, data_size);
	ar->data_size += data_size;
}
void binary_pack_strbuf(WArchive *ar, const char *str, U32 str_max_size)
{ binary_pack_buf(ar, str, str_max_size); }


void binary_unpack_buf(RArchive *ar, void *data, U32 data_size);
void binary_unpack_int(RArchive *ar, void *value, U32 size)
{ binary_unpack_buf(ar, value, size); }
void binary_unpack_float(RArchive *ar, void *value, bool single)
{ binary_unpack_buf(ar, value, float_size(single)); }
void binary_unpack_buf(RArchive *ar, void *data, U32 data_size)
{
	if (ar->offset + data_size > ar->data_size)
		fail("RArchive capacity exceeded");

	memcpy(data, ar->data + ar->offset, data_size);
	ar->offset += data_size;
}
void binary_unpack_strbuf(RArchive *ar, char *str, U32 str_max_size)
{ binary_unpack_buf(ar, str, str_max_size); str[str_max_size - 1] = '\0'; }

void *unpack_peek(RArchive *ar, U32 data_size)
{
	ensure(ar->offset + data_size <= ar->data_size);
	return (void*)(ar->data + ar->offset);
}

void unpack_advance(RArchive *ar, U32 data_size)
{
	ar->offset += data_size;
	ensure(ar->offset <= ar->data_size);
}


// ArchiveType to pack-function -tables

internal
void (*pack_int_funcs[])(WArchive *, const void *, U32) = {
	measure_pack_int,
	binary_pack_int,
};

internal
void (*pack_float_funcs[])(WArchive *, const void *, bool) = {
	measure_pack_float,
	binary_pack_float,
};

internal
void (*pack_buf_funcs[])(WArchive *, const void *, U32) = {
	measure_pack_buf,
	binary_pack_buf,
};

internal
void (*pack_strbuf_funcs[])(WArchive *, const char *, U32) = {
	measure_pack_strbuf,
	binary_pack_strbuf,
};



internal
void (*unpack_int_funcs[])(RArchive *, void *, U32) = {
	NULL,
	binary_unpack_int,
};

internal
void (*unpack_float_funcs[])(RArchive *, void *, bool) = {
	NULL,
	binary_unpack_float,
};

internal
void (*unpack_buf_funcs[])(RArchive *, void *, U32) = {
	NULL,
	binary_unpack_buf,
};

internal
void (*unpack_strbuf_funcs[])(RArchive *, char *, U32) = {
	NULL,
	binary_unpack_strbuf,
};



// Public

WArchive create_warchive(ArchiveType t, Ator *ator, U32 capacity)
{
	return (WArchive) {
		.type = t,
		.ator = ator,
		.data = 	t == ArchiveType_measure ?
					NULL :
					ALLOC(ator, capacity, "warchive_data"),
		.data_size = 0,
		.data_capacity = capacity,
	};
}

void destroy_warchive(WArchive *ar)
{
	if (ar->type != ArchiveType_measure)
		FREE(ar->ator, ar->data);
	*ar = (WArchive) {};
}

void release_warchive(void **data, U32 *size, WArchive *ar)
{
	*data = ar->data;
	if (size)
		*size = ar->data_size;
	*ar = (WArchive) {};
}

RArchive create_rarchive(ArchiveType t, const void *data, U32 data_size)
{
	ensure(t == ArchiveType_binary);
	return (RArchive) {
		.type = t,
		.offset = 0,
		.data = data,
		.data_size = data_size,
	};
}

void destroy_rarchive(RArchive *ar)
{
	*ar = (RArchive) {};
}

void pack_u32(WArchive *ar, const U32 *value)
{ pack_int_funcs[ar->type](ar, value, sizeof(*value)); }
void pack_u64(WArchive *ar, const U64 *value)
{ pack_int_funcs[ar->type](ar, value, sizeof(*value)); }
void pack_s32(WArchive *ar, const S32 *value)
{ pack_int_funcs[ar->type](ar, value, sizeof(*value)); }
void pack_s64(WArchive *ar, const S64 *value)
{ pack_int_funcs[ar->type](ar, value, sizeof(*value)); }
void pack_f32(WArchive *ar, const F32 *value)
{ pack_float_funcs[ar->type](ar, value, true); }
void pack_f64(WArchive *ar, const F64 *value)
{ pack_float_funcs[ar->type](ar, value, false); }
void pack_buf(WArchive *ar, const void *data, U32 data_size)
{ pack_buf_funcs[ar->type](ar, data, data_size); }
void pack_strbuf(WArchive *ar, const char *str, U32 str_max_size)
{ pack_strbuf_funcs[ar->type](ar, str, str_max_size); }

void pack_buf_patch(WArchive *ar, U32 offset, const void *data, U32 data_size)
{
	U32 end = ar->data_size;
	ar->data_size = offset;
	pack_buf(ar, data, data_size);
	ar->data_size = end;
}

void pack_patch_rel_ptr(WArchive *ar, U32 offset_to_ptr)
{
	RelPtr ptr = { .value = ar->data_size - offset_to_ptr };
	pack_buf_patch(ar, offset_to_ptr, &ptr, sizeof(ptr));
}

void unpack_u32(RArchive *ar, U32 *value)
{ unpack_int_funcs[ar->type](ar, value, sizeof(*value)); }
void unpack_u64(RArchive *ar, U64 *value)
{ unpack_int_funcs[ar->type](ar, value, sizeof(*value)); }
void unpack_s32(RArchive *ar, S32 *value)
{ unpack_int_funcs[ar->type](ar, value, sizeof(*value)); }
void unpack_s64(RArchive *ar, S64 *value)
{ unpack_int_funcs[ar->type](ar, value, sizeof(*value)); }
void unpack_f32(RArchive *ar, F32 *value)
{ unpack_float_funcs[ar->type](ar, value, true); }
void unpack_f64(RArchive *ar, F64 *value)
{ unpack_float_funcs[ar->type](ar, value, false); }
void unpack_buf(RArchive *ar, void *data, U32 data_size)
{ unpack_buf_funcs[ar->type](ar, data, data_size); }
void unpack_strbuf(RArchive *ar, char *str, U32 str_max_size)
{ unpack_strbuf_funcs[ar->type](ar, str, str_max_size); }


// Compound types


void lossy_pack_t3d(WArchive *ar, const T3d *tf)
{
	T3f temp = t3d_to_t3f(*tf);
	pack_buf(ar, &temp, sizeof(temp));
}

void lossy_unpack_t3d(RArchive *ar, T3d *tf)
{
	T3f temp;
	unpack_buf(ar, &temp, sizeof(temp));
	*tf = t3f_to_t3d(temp);
	tf->rot = normalized_qd(tf->rot);
}

void lossy_pack_v2d(WArchive *ar, const struct V2d *v)
{
	V2f temp = v2d_to_v2f(*v);
	pack_buf(ar, &temp, sizeof(temp));
}

void lossy_unpack_v2d(RArchive *ar, struct V2d *v)
{
	V2f temp;
	unpack_buf(ar, &temp, sizeof(temp));
	*v = v2f_to_v2d(temp);
}

