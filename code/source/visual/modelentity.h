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

	// Local
	T3d smoothing_delta;
	F32 smoothing_phase; // 1 at start of smoothing, 0 when done 

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


// Visual smoothing for discontinuities caused by net sync
REVOLC_API void upd_smoothing_phase(F32 *phase, F64 dt);
REVOLC_API T3d smoothed_tf(T3d tf, F32 phase, T3d delta);

// Rest of node-API or ModelEntity is in renderer

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
