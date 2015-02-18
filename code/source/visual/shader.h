#ifndef REVOLC_VISUAL_SHADER_H
#define REVOLC_VISUAL_SHADER_H

#include "build.h"
#include "resources/resource.h"
#include "mesh.h" // MeshType

typedef struct {
	Resource res;
	// NULL-terminated strings
	BlobOffset vs_src_offset;
	BlobOffset gs_src_offset;
	BlobOffset fs_src_offset;

	// Cached
	/// @todo	Store these somewhere else, as single shader source can
	///			produce multiple programs (macros)
	U32 vs_gl_id;
	U32 gs_gl_id;
	U32 fs_gl_id;
	U32 prog_gl_id;
} PACKED Shader;

REVOLC_API void init_Shader(Shader *shd);
REVOLC_API void deinit_Shader(Shader *shd);

#endif // REVOLC_VISUAL_SHADER_H
