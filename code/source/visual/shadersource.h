#ifndef REVOLC_VISUAL_SHADERSOURCE_H
#define REVOLC_VISUAL_SHADERSOURCE_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"
#include "mesh.h" // MeshType

typedef struct {
	Resource res;
	// NULL-terminated strings
	BlobOffset vs_src_offset;
	BlobOffset gs_src_offset;
	BlobOffset fs_src_offset;
	MeshType mesh_type;

	// On init
	/// @todo	Store these somewhere else, as single shader source can
	///			produce multiple programs (macros)
	U32 vs_gl_id;
	U32 gs_gl_id;
	U32 fs_gl_id;
	U32 prog_gl_id;
} PACKED ShaderSource;

REVOLC_API void init_shadersource(ShaderSource *shd);
REVOLC_API void deinit_shadersource(ShaderSource *shd);

REVOLC_API WARN_UNUSED
int json_shadersource_to_blob(struct BlobBuf *buf, JsonTok j);


#endif // REVOLC_VISUAL_SHADERSOURCE_H
