#ifndef REVOLC_VISUAL_MESH_H
#define REVOLC_VISUAL_MESH_H

#include "build.h"
#include "core/color.h"
#include "core/basic.h"
#include "core/math.h"
#include "core/cson.h"
#include "resources/resource.h"
#include "core/gl.h"

typedef enum { MeshType_tri } MeshType_enum;
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
	V2f outline_uv;
	// @todo Colors could be U8[4], or at least U16[4]
	Color color;
	Color outline_color;
	F32 color_exp; // Controls gradient over area of triangle. 1.0 == linear (in linear color space)
	F32 outline_exp; // Controls gradient over the area of outline width. 1.0 == linear
	F32 outline_width; // In pixels
	F32 emission;
	bool selected; // Editor
	bool pad[47];
} PACKED TriMeshVertex; // 128 bytes

REVOLC_API TriMeshVertex default_vertex();

typedef struct Mesh {
	Resource res;
	MeshType mesh_type;
	U32 v_count;
	U32 i_count;
	REL_PTR(TriMeshVertex) vertices;
	REL_PTR(MeshIndexType) indices;
} PACKED Mesh;

REVOLC_API TriMeshVertex * mesh_vertices(const Mesh *m);
REVOLC_API MeshIndexType * mesh_indices(const Mesh *m);

REVOLC_API WARN_UNUSED Mesh *blobify_mesh(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_mesh(WCson *c, struct RArchive *ar);

// Only for runtime resources
REVOLC_API void add_rt_mesh_vertex(Mesh *mesh, TriMeshVertex vertex);
REVOLC_API void add_rt_mesh_index(Mesh *mesh, MeshIndexType index);
REVOLC_API void remove_rt_mesh_vertex(Mesh *mesh, MeshIndexType index); // Removes also faces

#endif // REVOLC_VISUAL_MESH_H
