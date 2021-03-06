#include "font.h"

Font *blobify_font(struct WArchive *ar, Cson c, bool *err)
{
	Font *ptr = warchive_ptr(ar);
	U8 *ttf_data = NULL;
	U8 *bitmap = NULL;

	Cson c_file = cson_key(c, "file");
	if (cson_is_null(c_file))
		RES_ATTRIB_MISSING("file");

	char rel_path[MAX_PATH_SIZE];
	fmt_str(rel_path, sizeof(rel_path), "%s", blobify_string(c_file, err));

	char total_path[MAX_PATH_SIZE];
	joined_path(total_path, c.dir_path, rel_path);

	ttf_data = read_file(gen_ator(), total_path, NULL);

	// @todo No guarantee this is enough!
	const int reso = 512;
	bitmap = malloc(reso*reso);
	Font font = {
		.bitmap_reso = {reso, reso},
		.px_height = 13,
	};
	fmt_str(font.rel_file, sizeof(font.rel_file), "%s", rel_path);

	stbtt_pack_context ctx;
	stbtt_PackBegin(&ctx, bitmap, reso, reso, 0, 0, NULL);
	stbtt_PackSetOversampling(&ctx, 1, 1);
	stbtt_PackFontRange(&ctx, ttf_data, 0, font.px_height,
						FONT_CHAR_BEGIN,
						FONT_CHAR_COUNT,
						font.chars);
	stbtt_PackEnd(&ctx);

	if (err && *err)
		goto error;

	U32 bitmap_ptr_buf_offset = ar->data_size + offsetof(Font, bitmap_offset);
	pack_buf(ar, &font, sizeof(font));
	pack_patch_rel_ptr(ar, bitmap_ptr_buf_offset);
	pack_buf(ar, bitmap, reso*reso);

cleanup:
	free(bitmap);
	free(ttf_data);
	return ptr;

error:
	ptr = NULL;
	goto cleanup;
}

void deblobify_font(WCson *c, struct RArchive *ar)
{
	Font *f = rarchive_ptr(ar, sizeof(*f));
	unpack_advance(ar, f->res.size);

	wcson_begin_compound(c, "Font");

	wcson_designated(c, "name");
	deblobify_string(c, f->res.name);

	wcson_designated(c, "file");
	deblobify_string(c, f->rel_file);

	wcson_end_compound(c);
}

internal
U8 *font_bitmap(const Font *font)
{ return rel_ptr(&font->bitmap_offset); }

Texel * malloc_rgba_font_bitmap(const Font *font)
{
	U32 texel_count =
			font->bitmap_reso.x*font->bitmap_reso.y;
	Texel *texels = malloc(sizeof(*texels)*texel_count);
	U8 *bitmap = font_bitmap(font);
	for (U32 i = 0; i < texel_count; ++i) {
		texels[i].r = texels[i].g = texels[i].b = 255;
		texels[i].a = bitmap[i];
	}
	return texels;
}

U32 text_mesh(	V2i *size,
				TriMeshVertex *verts,
				MeshIndexType *inds,
				const Font *font,
				const char *text)
{
	F32 x = 0, y = font->px_height;
	U32 count = 0;
	U32 v_i = 0;
	*size = (V2i) {};
	while (*text) {
		U32 ch = *text;
		if (ch == '\n') {
			x = 0;
			y += font->px_height;
		}

		if (ch >= FONT_CHAR_BEGIN && ch < FONT_CHAR_END) {
			stbtt_aligned_quad q;
			stbtt_GetPackedQuad((void *)font->chars,
								font->bitmap_reso.x,
								font->bitmap_reso.y,
								*text - FONT_CHAR_BEGIN,
								&x, &y,
								&q,
								1);
			size->x = MAX(size->x, MAX(q.x0, q.x1));
			size->y = MAX(size->y, MAX(q.y0, q.y1));
			size->x = MAX(size->x, x);
			size->y = MAX(size->y, y);

			for (U32 k = 0; k < 4; ++k)
				verts[v_i + k] = default_vertex();

			verts[v_i + 0].pos = (V3f) {q.x0, q.y1};
			verts[v_i + 1].pos = (V3f) {q.x1, q.y1};
			verts[v_i + 2].pos = (V3f) {q.x1, q.y0};
			verts[v_i + 3].pos = (V3f) {q.x0, q.y0};

			// Chars are upside-down in the gl texture
			verts[v_i + 0].uv = (V3f) {q.s0, q.t1};
			verts[v_i + 1].uv = (V3f) {q.s1, q.t1};
			verts[v_i + 2].uv = (V3f) {q.s1, q.t0};
			verts[v_i + 3].uv = (V3f) {q.s0, q.t0};

			*(inds++) = v_i;
			*(inds++) = v_i + 1;
			*(inds++) = v_i + 2;

			*(inds++) = v_i;
			*(inds++) = v_i + 2;
			*(inds++) = v_i + 3;

			v_i += 4;
			++count;
		}
		++text;
	}
	return count;
}

V2i calc_text_mesh_size(const Font *font, const char *text)
{
	V2i size = {};
	F32 x = 0, y = font->px_height;
	while (*text) {
		U32 ch = *text;
		if (ch == '\n') {
			x = 0;
			y += font->px_height;
		}

		if (ch >= FONT_CHAR_BEGIN && ch < FONT_CHAR_END) {
			stbtt_aligned_quad q;
			stbtt_GetPackedQuad((void *)font->chars,
								font->bitmap_reso.x,
								font->bitmap_reso.y,
								*text - FONT_CHAR_BEGIN,
								&x, &y,
								&q,
								1);
			size.x = MAX(size.x, MAX(q.x0, q.x1));
			size.y = MAX(size.y, MAX(q.y0, q.y1));
			size.x = MAX(size.x, x);
			size.y = MAX(size.y, y);
		}
		++text;
	}
	return size;
}
