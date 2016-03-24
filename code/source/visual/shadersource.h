#ifndef REVOLC_VISUAL_SHADERSOURCE_H
#define REVOLC_VISUAL_SHADERSOURCE_H

#include "build.h"
#include "core/cson.h"
#include "resources/resource.h"
#include "mesh.h" // MeshType

typedef struct ShaderSource {
	Resource res;

	char rel_vs_file[MAX_PATH_SIZE];
	char rel_gs_file[MAX_PATH_SIZE];
	char rel_fs_file[MAX_PATH_SIZE];

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
ShaderSource *blobify_shadersource(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_shadersource(WCson *c, struct RArchive *ar);

#endif // REVOLC_VISUAL_SHADERSOURCE_H
