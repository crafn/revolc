#ifndef REVOLC_VISUAL_SHADERSOURCE_H
#define REVOLC_VISUAL_SHADERSOURCE_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"
#include "mesh.h" // MeshType

typedef struct ShaderSource {
	Resource res;
	// NULL-terminated strings
	REL_PTR(char) vs_src_offset;
	REL_PTR(char) gs_src_offset;
	REL_PTR(char) fs_src_offset;
	MeshType mesh_type;
	char feedback_varyings[MAX_SHADER_VARYING_COUNT][RES_NAME_SIZE];

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
