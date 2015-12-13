#ifndef REVOLC_VISUAL_ENTITY_MODEL_H
#define REVOLC_VISUAL_ENTITY_MODEL_H

#include "atlas.h"
#include "build.h"
#include "core/json.h"
#include "core/math.h"
#include "model.h"
#include "mesh.h"

typedef struct ModelEntity {
	char model_name[RES_NAME_SIZE]; // @todo Replace with ResId
	T3d tf;
	S32 layer; // Overrides z-sorting. 0 for world stuff
	bool allocated;
	bool has_own_mesh; // If true, vertices and indices are free'd along this

	// Cached
	Color color;
	F32 emission;
	U8 pattern;
	V3f atlas_uv; // @todo Use AtlasUv
	V2f scale_to_atlas_uv;
	U32 mesh_v_count;
	U32 mesh_i_count;
	TriMeshVertex* vertices;
	MeshIndexType* indices;
} ModelEntity;

REVOLC_API void init_modelentity(ModelEntity *data);

struct WArchive;
struct RArchive;
REVOLC_API void pack_modelentity(	struct WArchive *ar,
									const ModelEntity *begin,
									const ModelEntity *end);
REVOLC_API void unpack_modelentity(	struct RArchive *ar,
									ModelEntity *begin,
									ModelEntity *end);

#endif // REVOLC_VISUAL_ENTITY_MODEL_H
