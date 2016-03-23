#ifndef REVOLC_VISUAL_FONT_H
#define REVOLC_VISUAL_FONT_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"

#include <stb/stb_truetype.h>

#define FONT_CHAR_BEGIN 32
#define FONT_CHAR_COUNT 96 // ASCII 32..126
#define FONT_CHAR_END (FONT_CHAR_BEGIN + FONT_CHAR_COUNT)

typedef struct Font {
	Resource res;

	char rel_file[MAX_PATH_SIZE];

	// Renderer sets
	AtlasUv atlas_uv;

	stbtt_packedchar chars[FONT_CHAR_COUNT];
	// @todo Should probably have rgba bitmap, or ttf data.
	//       Monochrome bitmap is no good for anyone.
	REL_PTR(U8) bitmap_offset;
	V2i bitmap_reso;
	F32 px_height;
} PACKED Font;

REVOLC_API WARN_UNUSED
int json_font_to_blob(struct BlobBuf *buf, JsonTok j);

REVOLC_API WARN_UNUSED
Font *blobify_font(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_font(WCson *c, struct RArchive *ar);

Texel * malloc_rgba_font_bitmap(const Font *font);

// Mesh is in OpenGL-like coordinates (but px sized)
// Origin is at upper left corner of the text
REVOLC_API WARN_UNUSED
U32 text_mesh(	V2i *size,
				TriMeshVertex *verts, // len(text)*4
				MeshIndexType *inds, // len(text)*6
				const Font *font,
				const char *text);

V2i calc_text_mesh_size(const Font *font, const char *text);

#endif // REVOLC_VISUAL_FONT_H
