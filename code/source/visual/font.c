#include "font.h"

int json_font_to_blob(struct BlobBuf *buf, JsonTok j)
{
	int return_value= 0;
	U8 *ttf_data= NULL;
	U8 *bitmap= NULL;

	JsonTok j_file= json_value_by_key(j, "file");
	if (json_is_null(j_file))
		RES_ATTRIB_MISSING("file");

	char total_path[MAX_PATH_SIZE];
	joined_path(total_path, j.json_path, json_str(j_file));

	ttf_data= malloc_file(total_path, NULL);

	// @todo No guarantee this is enough!
	const int reso= 512;
	bitmap= malloc(reso*reso);
	Font font= {
		.bitmap_reso= {reso, reso},
		.px_height= 13*16.0/12.0,
	};

	stbtt_pack_context ctx;
	stbtt_PackBegin(&ctx, bitmap, reso, reso, 0, 0, NULL);
	stbtt_PackSetOversampling(&ctx, 1, 1);
	stbtt_PackFontRange(&ctx, ttf_data, 0, font.px_height,
						FONT_CHAR_BEGIN,
						FONT_CHAR_COUNT,
						font.chars);
	stbtt_PackEnd(&ctx);

	U32 res_size= sizeof(font) - sizeof(Resource);
	font.bitmap_offset= buf->offset + res_size;
	blob_write(	buf,
				(U8*)&font + sizeof(Resource),
				res_size);
	blob_write(buf, bitmap, reso*reso);

cleanup:
	free(bitmap);
	free(ttf_data);
	return return_value;

error:
	return_value= 1;
	goto cleanup;
}

internal
U8 *font_bitmap(const Font *font)
{ return blob_ptr(&font->res, font->bitmap_offset); }

Texel * malloc_rgba_font_bitmap(const Font *font)
{
	U32 texel_count=
			font->bitmap_reso.x*font->bitmap_reso.y;
	Texel *texels= malloc(sizeof(*texels)*texel_count);
	U8 *bitmap= font_bitmap(font);
	for (U32 i= 0; i < texel_count; ++i) {
		texels[i].r= texels[i].g= texels[i].b= 255;
		texels[i].a= bitmap[i];
	}
	return texels;
}

U32 text_mesh(	TriMeshVertex *verts,
				MeshIndexType *inds,
				const Font *font,
				V2i p,
				const char *text)
{
	F32 x= p.x, y= p.y;
	U32 count= 0;
	U32 v_i= 0;
	while (*text) {
		if (*text == '\n') {
			x= p.x;
			y += font->px_height;
		}

		if (	*text >= FONT_CHAR_BEGIN &&
				*text < FONT_CHAR_END) {
			stbtt_aligned_quad q;
			stbtt_GetPackedQuad((void *)font->chars,
								font->bitmap_reso.x,
								font->bitmap_reso.y,
								*text - FONT_CHAR_BEGIN,
								&x, &y,
								&q,
								1);
			verts[v_i + 0].pos= (V3f) {q.x0, q.y0};
			verts[v_i + 1].pos= (V3f) {q.x1, q.y0};
			verts[v_i + 2].pos= (V3f) {q.x1, q.y1};
			verts[v_i + 3].pos= (V3f) {q.x0, q.y1};

			// Chars are upside-down in the gl texture
			verts[v_i + 0].uv= (V3f) {q.s0, q.t0};
			verts[v_i + 1].uv= (V3f) {q.s1, q.t0};
			verts[v_i + 2].uv= (V3f) {q.s1, q.t1};
			verts[v_i + 3].uv= (V3f) {q.s0, q.t1};

			*(inds++)= v_i;
			*(inds++)= v_i + 1;
			*(inds++)= v_i + 2;

			*(inds++)= v_i;
			*(inds++)= v_i + 2;
			*(inds++)= v_i + 3;

			v_i += 4;
			++count;
		}
		++text;
	}
	return count;
}
