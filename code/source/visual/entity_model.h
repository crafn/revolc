#ifndef REVOLC_VISUAL_ENTITY_MODEL_H
#define REVOLC_VISUAL_ENTITY_MODEL_H

#include "build.h"
#include "core/vector.h"
#include "model.h"
#include "mesh.h"

typedef struct {
	V3f pos;
	const Model* model;

	// Cached
	U32 tex_gl_ids[3];
	const TriMeshVertex* vertices;
	const MeshIndexType* indices;
	U32 mesh_v_count;
	U32 mesh_i_count;

} ModelEntity;

#endif // REVOLC_VISUAL_ENTITY_MODEL_H