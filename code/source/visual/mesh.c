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

TriMeshVertex * mesh_vertices(const Mesh *m)
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

WJson *wjson_create()
{
	WJson *new_member= dev_malloc(sizeof(*new_member));
	*new_member= (WJson) {};
	return new_member;
}

void wjson_destroy(WJson *j)
{
	WJson *member= j->last_member;
	while (member) {
		WJson* prev= member->prev;
		wjson_destroy(member);
		member= prev;
	}

	if (j->type == WJsonType_string) {
		dev_free(j->string);
		j->string= NULL;
	}

	dev_free(j);
}

void * wjson_init_string(WJson *j_str, const char *str)
{
	ensure(j_str->string == NULL);
	j_str->type= WJsonType_string;
	U32 size= strlen(str) + 1;
	j_str->string= dev_malloc(size);
	snprintf(j_str->string, size, "%s", str);
	return j_str;
}

void wjson_append(WJson *j, WJson *item)
{
	item->prev= j->last_member;
	j->last_member= item;
}

WJson * wjson_new_member(WJson *j)
{
	WJson *new_member= wjson_create();
	wjson_append(j, new_member);
	return new_member;
}

WJson *wjson_named_member(WJson *j, WJsonType t, const char *name)
{
	WJson *j_str= wjson_new_member(j);
	wjson_init_string(j_str, name);
	WJson *j_member= wjson_new_member(j_str);
	j_member->type= t;
	return j_member;
}

void wjson_move(WJson *dst, WJson *src)
{
	WJson *prev= dst->prev;
	*dst= *src;
	dst->prev= prev;
}

WJson *wjson_number(F64 n)
{
	WJson *j_number= wjson_create();
	j_number->type= WJsonType_number;
	j_number->number= n;
	return j_number;
}

WJson *wjson_v3(V3d vec)
{
	WJson *j_v3= wjson_create();
	j_v3->type= WJsonType_array;
	
	wjson_append(j_v3, wjson_number(vec.x));
	wjson_append(j_v3, wjson_number(vec.y));
	wjson_append(j_v3, wjson_number(vec.z));
	return j_v3;
}

internal
void wjson_dump_recurse(WJson *j, U32 depth)
{
	for (U32 i= 0; i < depth; ++i)
		printf("  ");
	switch (j->type) {
	case WJsonType_object: debug_print("{"); break;
	case WJsonType_array: debug_print("["); break;
	case WJsonType_string:
		if (j->last_member)
			debug_print("\"%s\" : ", j->string);
		else
			debug_print("\"%s\",", j->string);
	break;
	case WJsonType_number: debug_print("%g,", j->number); break;
	default: fail("Unknown value");
	}

	WJson *member= j->last_member;
	while (member) {
		wjson_dump_recurse(member, depth + 1);
		member= member->prev;
	}

	if (	j->type == WJsonType_object ||
			j->type == WJsonType_array) {
		for (U32 i= 0; i < depth; ++i)
			printf("  ");

		if (j->type == WJsonType_object)
			debug_print("},");
		else
			debug_print("],");
	}
}

void wjson_dump(WJson *j)
{
	debug_print("WJson dump");
	wjson_dump_recurse(j, 0);
}

void mesh_to_json(WJson *j, const Mesh *m)
{
	WJson *j_pos= wjson_named_member(j, WJsonType_array, "pos");
	WJson *j_uv= wjson_named_member(j, WJsonType_array, "uv");
	WJson *j_ind= wjson_named_member(j, WJsonType_array, "ind");

	for (U32 i= 0; i < m->v_count; ++i) {
		wjson_append(	j_pos,
						wjson_v3(v3f_to_v3d(mesh_vertices(m)[i].pos)));
		wjson_append(	j_uv,
						wjson_v3(v3f_to_v3d(mesh_vertices(m)[i].uv)));
	}

	for (U32 i= 0; i < m->i_count; ++i) {
		wjson_append(	j_ind,
						wjson_number(mesh_indices(m)[i]));
	}
}
