#ifndef REVOLC_VISUAL_MESH_H
#define REVOLC_VISUAL_MESH_H

#include "build.h"
#include "core/vector.h"
#include "core/json.h"
#include "resources/resource.h"
#include "platform/gl.h"

typedef enum { MeshType_tri, MeshType_point } MeshType;
typedef U32 MeshIndexType;
#define MESH_INDEX_GL_TYPE GL_UNSIGNED_INT

typedef struct {
	const GLchar *name;
	GLint size;
	GLenum type;	
	GLboolean normalized;
	U64 offset;
} VertexAttrib;

REVOLC_API void vertex_attributes(MeshType type, const VertexAttrib **attribs, U32 *count);
REVOLC_API bool is_indexed_mesh(MeshType type);
REVOLC_API U32 vertex_size(MeshType type);

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
	U32 v_count;
	U32 i_count;
	BlobOffset v_offset;
	BlobOffset i_offset;
} PACKED Mesh;

REVOLC_API void * mesh_vertices(const Mesh *m);
REVOLC_API MeshIndexType * mesh_indices(const Mesh *m);

REVOLC_API WARN_UNUSED
int json_mesh_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_VISUAL_MESH_H
