#ifndef REVOLC_VISUAL_MESH_H
#define REVOLC_VISUAL_MESH_H

#include "build.h"
#include "core/color.h"
#include "core/vector.h"
#include "core/json.h"
#include "resources/resource.h"
#include "platform/gl.h"

typedef enum { MeshType_tri, MeshType_brush } MeshType_enum;
typedef U32 MeshType;
typedef U32 MeshIndexType;
#define MESH_INDEX_GL_TYPE GL_UNSIGNED_INT

typedef struct VertexAttrib {
	const GLchar *name;
	GLint size;
	GLenum type;
	bool floating; // False for integer attribs
	GLboolean normalized;
	U64 offset;
} VertexAttrib;

REVOLC_API void vertex_attributes(MeshType type, const VertexAttrib **attribs, U32 *count);
REVOLC_API bool is_indexed_mesh(MeshType type);
REVOLC_API U32 vertex_size(MeshType type);

typedef struct TriMeshVertex {
	V3f pos;
	V3f uv;
	Color color; // @todo Could be U8[4]
	F32 emission;
	U8 pattern; // Overridden in rendering
	bool selected; // Editor
	bool pad[18];
} TriMeshVertex ALIGNED(64);

typedef struct BrushMeshVertex {
	V2f pos; // In GL coords
	F32 size;
	bool pad[4];
} BrushMeshVertex ALIGNED(16);

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
