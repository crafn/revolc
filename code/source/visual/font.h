#ifndef REVOLC_VISUAL_FONT_H
#define REVOLC_VISUAL_FONT_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"

#include <stb/stb_truetype.h>

#define FONT_BAKEDCHAR_COUNT 96 // ASCII 32..126

typedef struct Font {
	Resource res;

	// Renderer sets
	V3f atlas_uv;
	V2f scale_to_atlas_uv;

	stbtt_bakedchar chars[FONT_BAKEDCHAR_COUNT];
	// @todo Should probably have rgba bitmap, or ttf data.
	//       Monochrome bitmap is no good for anyone.
	BlobOffset bitmap_offset; // U8
	V2i bitmap_reso;
} PACKED Font;

REVOLC_API WARN_UNUSED
int json_font_to_blob(struct BlobBuf *buf, JsonTok j);

Texel * malloc_rgba_font_bitmap(const Font *font);

// Returns written char count
REVOLC_API WARN_UNUSED
U32 text_mesh(	TriMeshVertex *verts, // len(text)*4
				MeshIndexType *inds, // len(text)*6
				const Font *font,
				V2i p,
				const char *text);

#endif // REVOLC_VISUAL_FONT_H