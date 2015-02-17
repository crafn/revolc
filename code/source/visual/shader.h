#ifndef REVOLC_VISUAL_SHADER_H
#define REVOLC_VISUAL_SHADER_H

#include "build.h"
#include "resources/resource.h"

typedef struct {
	Resource res;
	U32 vs_len;
	U32 gs_len;
	U32 fs_len;
	BlobOffset vs_src;
	BlobOffset gs_src;
	BlobOffset fs_src;

	// Cached
	U32 vs_gl_id;
	U32 gs_gl_id;
	U32 fs_gl_id;
	U32 prog_gl_id;
} PACKED Shader;

#endif // REVOLC_VISUAL_SHADER_H
