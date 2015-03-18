#include "core/ensure.h"
#include "mesh.h"
#include "resources/resblob.h"

void vertex_attributes(MeshType type, const VertexAttrib **attribs, U32 *count)
{
	if (type == MeshType_tri) {
		local_persist VertexAttrib tri_attribs[]= {
			{ "a_pos", 3, GL_FLOAT, false, offsetof(TriMeshVertex, pos) },
			{ "a_uv", 3, GL_FLOAT, false, offsetof(TriMeshVertex, uv) },
			{ "a_color", 4, GL_FLOAT, false, offsetof(TriMeshVertex, color) },
			{ "a_dev_highlight", 1, GL_FLOAT, false, offsetof(TriMeshVertex, dev_highlight) },
		};
		if (attribs)
			*attribs= tri_attribs;
		*count= sizeof(tri_attribs)/sizeof(*tri_attribs);
	} else {
		fail("Unimplemented mesh type");
	}
}

bool is_indexed_mesh(MeshType type)
{ return type == MeshType_tri; }

U32 vertex_size(MeshType type)
{
	switch (type) {
		case MeshType_tri: return sizeof(TriMeshVertex);
		default: fail("Unhandled mesh type");
	}
	return 0;
}

void * mesh_vertices(const Mesh *m)
{ return blob_ptr(&m->res, m->v_offset); }

MeshIndexType * mesh_indices(const Mesh *m)
{ return (MeshIndexType*)blob_ptr(&m->res, m->i_offset); }

int json_mesh_to_blob(struct BlobBuf *buf, JsonTok j)
{
	MeshType type= MeshType_tri;
	BlobOffset v_offset= 0;
	BlobOffset i_offset= 0;

	JsonTok j_pos= json_value_by_key(j, "pos");
	JsonTok j_uv= json_value_by_key(j, "uv");
	JsonTok j_ind= json_value_by_key(j, "ind");

	if (json_is_null(j_pos))
		RES_ATTRIB_MISSING("pos");
	if (json_is_null(j_ind))
		RES_ATTRIB_MISSING("ind");

	if (	!json_is_null(j_uv) &&
			json_member_count(j_uv) != json_member_count(j_pos)) {
		critical_print("Attrib 'uv' size doesn't match with 'pos' for Mesh: %s",
				json_str(json_value_by_key(j, "name")));
		goto error;
	}

	{
		const U32 v_count= json_member_count(j_pos);
		const U32 i_count= json_member_count(j_ind);
		TriMeshVertex *vertices= zero_malloc(sizeof(*vertices)*v_count);
		MeshIndexType *indices= zero_malloc(sizeof(*indices)*i_count);
		for (U32 i= 0; i < v_count; ++i) {
			JsonTok j_p= json_member(j_pos, i);
			V3f p= {};
			for (U32 c= 0; c < json_member_count(j_p); ++c)
				(&p.x)[c]= json_real(json_member(j_p, c));
			vertices[i].pos= p;

			if (json_is_null(j_uv))
				continue;
			JsonTok j_u= json_member(j_uv, i);
			V3f u= {};
			for (U32 c= 0; c < json_member_count(j_u); ++c)
				(&u.x)[c]= json_real(json_member(j_u, c));
			vertices[i].uv= u;
		}
		for (U32 i= 0; i < i_count; ++i)
			indices[i]= json_integer(json_member(j_ind, i));

		blob_write(buf, &type, sizeof(type));
		blob_write(buf, &v_count, sizeof(v_count));
		blob_write(buf, &i_count, sizeof(i_count));

		v_offset= buf->offset + sizeof(v_offset) + sizeof(i_offset);
		blob_write(buf, &v_offset, sizeof(v_offset));

		i_offset= buf->offset + sizeof(i_offset) + sizeof(*vertices)*v_count;
		blob_write(buf, &i_offset, sizeof(i_offset));

		blob_write(buf, &vertices[0], sizeof(*vertices)*v_count);
		blob_write(buf, &indices[0], sizeof(*indices)*i_count);

		free(vertices);
		free(indices);
	}

	return 0;

error:
	return 1;
}

typedef enum {
	JsonType_object,
	JsonType_array,
	JsonType_number,
	JsonType_string
} JsonType;

// If complex manipulation of json files is needed, this should
// probably override JsonTok as read/write structure
typedef struct JsonNode {
	JsonType type;
	struct JsonNode *members; // Owns
	U32 member_count;

	F64 number;
	char *string; // Owns
} JsonNode;

JsonNode create_jsonnode(JsonTok input)
{
	JsonNode n= {};

	ensure(!json_is_null(input) && "@todo Null support");

	if (json_is_object(input) || json_is_array(input) || json_is_string(input)) {
		if (json_is_object(input)) {
			n.type= JsonType_object;
		} else if (json_is_array(input)) {
			n.type= JsonType_array;
		} else {
			n.type= JsonType_string;
			U32 string_len= strlen(json_str(input));
			n.string= dev_malloc(string_len + 1);
			json_strcpy(n.string, string_len, input);
		}
		n.member_count= json_member_count(input);
		n.members= dev_malloc(sizeof(*n.members)*n.member_count);
		for (U32 i= 0; i < n.member_count; ++i)
			n.members[i]= create_jsonnode(json_member(input, i));
	} else { // Number
		n.type= JsonType_number;
		n.number= json_real(input);
	}
	return n;
}

void destroy_jsonnode(JsonNode node)
{
	if (	node.type == JsonType_object ||
			node.type == JsonType_array ||
			node.type == JsonType_string) {
		for (U32 i= 0; i < node.member_count; ++i)
			destroy_jsonnode(node.members[i]);
		dev_free(node.members);
	}

	if (node.type == JsonType_string)
		dev_free(node.string);
}

/*
void blob_mesh_to_json(JsonOut *j, const Mesh *m)
{
	JsonOut* j_pos= jout_array(j, "pos");
	JsonOut* j_uv= jout_array(j, "uv");
	JsonOut* j_ind= jout_array(j, "ind");

	for (U32 i= 0; i < m->v_count; ++i) {
		jout_member(j_pos, i, jout_v3d(mesh_vertices(m)[i].pos));
		jout_member(j_uv, i, jout_v3d(mesh_vertices(m)[i].uv));
	}

	for (U32 i= 0; i < m->i_count; ++i) {
		jout_member(j_ind, i, jout_integer(mesh_indices(m)[i]));
	}
}
*/
