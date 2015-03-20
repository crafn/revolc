#ifndef REVOLC_VISUAL_ENTITY_MODEL_H
#define REVOLC_VISUAL_ENTITY_MODEL_H

#include "build.h"
#include "core/json.h"
#include "core/transform.h"
#include "core/vector.h"
#include "model.h"
#include "mesh.h"

typedef struct ModelEntity {
	char model_name[RES_NAME_SIZE];
	T3d tf;
	bool allocated;
	bool has_own_mesh; // If true, vertices and indices are free'd along this
	bool free_after_draw;

	// Cached
	Color color;
	V3f atlas_uv;
	V2f scale_to_atlas_uv;
	U32 mesh_v_count;
	U32 mesh_i_count;
	TriMeshVertex* vertices;
	MeshIndexType* indices;
} ModelEntity;

REVOLC_API void init_modelentity(ModelEntity *data);

#endif // REVOLC_VISUAL_ENTITY_MODEL_H
