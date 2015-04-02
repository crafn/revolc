#ifndef REVOLC_VISUAL_MESH_H
#define REVOLC_VISUAL_MESH_H

#include "build.h"
#include "core/color.h"
#include "core/vector.h"
#include "core/json.h"
#include "resources/resource.h"
#include "platform/gl.h"

typedef enum { MeshType_tri, MeshType_point } MeshType;
typedef U32 MeshIndexType;
#define MESH_INDEX_GL_TYPE GL_UNSIGNED_INT

typedef struct VertexAttrib {
	const GLchar *name;
	GLint size;
	GLenum type;	
	GLboolean normalized;
	U64 offset;
} VertexAttrib;

REVOLC_API void vertex_attributes(MeshType type, const VertexAttrib **attribs, U32 *count);
REVOLC_API bool is_indexed_mesh(MeshType type);
REVOLC_API U32 vertex_size(MeshType type);

typedef struct TriMeshVertex {
	V3f pos;
	V3f uv;
	Color color;
	F32 dev_highlight;
	bool selected; // Editor
	bool pad[19];
} TriMeshVertex ALIGNED(64);

typedef struct Mesh {
	Resource res;
	MeshType mesh_type;
	U32 v_count;
	U32 i_count;
	BlobOffset v_offset;
	BlobOffset i_offset;
} PACKED Mesh;

REVOLC_API TriMeshVertex * mesh_vertices(const Mesh *m);
REVOLC_API MeshIndexType * mesh_indices(const Mesh *m);

REVOLC_API WARN_UNUSED
int json_mesh_to_blob(struct BlobBuf *buf, JsonTok j);
REVOLC_API
void mesh_to_json(WJson *j, const Mesh *m);

// Creates modifiable substitute for static mesh resource
Mesh *create_rt_mesh(Mesh *src);

#endif // REVOLC_VISUAL_MESH_H
