#ifndef REVOLC_VISUAL_MESH_H
#define REVOLC_VISUAL_MESH_H

#include "build.h"
#include "core/vector.h"
#include "resources/resource.h"

typedef enum { MeshType_tri, MeshType_point } MeshType;

typedef struct {
	V3f pos;
	V3f uv;
	F32 color[4];
	V3f normal;
	V3f tangent;

} TriMeshVertex ALIGNED(64);

typedef struct {
	Resource res;
	MeshType mesh_type;
	U32 vertex_count;
	U32 index_count;
	ResOffset vertices_offset;
	ResOffset indices_offset;
} PACKED Mesh;

#endif // REVOLC_VISUAL_MESH_H
