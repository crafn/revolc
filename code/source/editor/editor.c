#include "core/malloc.h"
#include "editor.h"
#include "global/env.h"
#include "platform/device.h"
#include "ui/uicontext.h"
#include "visual/renderer.h"

typedef struct EditorBoxState {
	bool hover;
	bool down;
	bool pressed;
	bool released;
} EditorBoxState;

internal
void toggle_bool(bool *b)
{ *b = !*b; }

internal
void destroy_rt_mesh(Resource *res);

// Creates modifiable substitute for static mesh resource
internal
Mesh *create_rt_mesh(Mesh *src)
{
	Mesh *rt_mesh= dev_malloc(sizeof(*rt_mesh));
	*rt_mesh= *src;
	substitute_res(&src->res, &rt_mesh->res, destroy_rt_mesh);

	TriMeshVertex *verts= dev_malloc(sizeof(*verts)*src->v_count);
	memcpy(verts, mesh_vertices(src), sizeof(*verts)*src->v_count);

	MeshIndexType *inds= dev_malloc(sizeof(*inds)*src->i_count);
	memcpy(inds, mesh_indices(src), sizeof(*inds)*src->i_count);

	rt_mesh->v_offset= blob_offset(&rt_mesh->res, verts);
	rt_mesh->i_offset= blob_offset(&rt_mesh->res, inds);

	recache_ptrs_to_meshes();

	return rt_mesh;
}

internal
void destroy_rt_mesh(Resource *res)
{
	Mesh *m= (Mesh*)res;
	dev_free(blob_ptr(res, m->v_offset));
	dev_free(blob_ptr(res, m->i_offset));
	dev_free(m);
}

internal
void editor_free_res_state()
{
	Editor *e= g_env.editor;
	free(e->stored.vertices);
	free(e->stored.indices);
	e->stored.vertices= NULL;
	e->stored.indices= NULL;
}

// Store currently selected resources for cancellation of edit
internal
void editor_store_res_state()
{
	Editor *e= g_env.editor;
	if (e->stored.vertices)
		editor_free_res_state();

	if (e->cur_model_h != NULL_HANDLE) {
		ModelEntity *m= get_modelentity(e->cur_model_h);
		Mesh *mesh= model_mesh((Model*)res_by_name(	g_env.resblob,
													ResType_Model,
													m->model_name));

		const U32 v_size= sizeof(*e->stored.vertices)*mesh->v_count;
		e->stored.vertices= dev_malloc(v_size);
		memcpy(e->stored.vertices, mesh_vertices(mesh), v_size);
		e->stored.v_count= mesh->v_count;

		const U32 i_size= sizeof(*e->stored.indices)*mesh->i_count;
		e->stored.indices= dev_malloc(i_size);
		memcpy(e->stored.indices, mesh_indices(mesh), i_size);
		e->stored.i_count= mesh->i_count;
	}

	if (e->cur_comp_h != NULL_HANDLE) {
		CompEntity *c= get_compentity(e->cur_comp_h);
		Armature *a= c->armature;

		for (U32 i= 0; i < a->joint_count; ++i)
			e->stored.bind_pose.tf[i]= a->joints[i].bind_pose;
		e->stored.joint_count= a->joint_count;
	}
}

// Revert currenty select resources to state before edit action
internal
void editor_revert_res_state()
{
	Editor *e= g_env.editor;
	if (e->stored.vertices && e->cur_model_h != NULL_HANDLE) {
		ModelEntity *m= get_modelentity(e->cur_model_h);
		Mesh *mesh= model_mesh((Model*)res_by_name(	g_env.resblob,
													ResType_Model,
													m->model_name));
		if (mesh->res.is_runtime_res) {
			// Free old mesh
			dev_free(blob_ptr(&mesh->res, mesh->v_offset));
			dev_free(blob_ptr(&mesh->res, mesh->i_offset));

			// Move stored mesh
			mesh->v_offset= blob_offset(&mesh->res, e->stored.vertices);
			mesh->i_offset= blob_offset(&mesh->res, e->stored.indices);
			mesh->v_count= e->stored.v_count;
			mesh->i_count= e->stored.i_count;
			e->stored.vertices= NULL;
			e->stored.indices= NULL;

			recache_ptrs_to_meshes();
		}
	}

	if (e->cur_comp_h != NULL_HANDLE) {
		CompEntity *c= get_compentity(e->cur_comp_h);
		Armature *a= c->armature;

		for (U32 i= 0; i < a->joint_count; ++i)
			a->joints[i].bind_pose= e->stored.bind_pose.tf[i];
		a->joint_count= e->stored.joint_count;
	}
}

internal
F64 editor_vertex_size()
{ return screen_to_world_size((V2i) {5, 0}).x; }

internal
V3f cursor_delta_in_tf_coords(T3d tf)
{
	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3d cur= mul_t3d(	inv_t3d(tf),
						(T3d) {	{1, 1, 1},
								identity_qd(),
								cur_wp}).pos;
	V3d prev= mul_t3d(	inv_t3d(tf),
						(T3d) {	{1, 1, 1},
								identity_qd(),
								prev_wp}).pos;
	return v3d_to_v3f(sub_v3d(cur, prev));
}

internal
Qf cursor_rot_delta_in_tf_coords(T3d tf)
{
	V3d center= tf.pos;
	/// @todo Correct return with 3d rot

	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3f v1= v3d_to_v3f(sub_v3d(prev_wp, center));
	V3f v2= v3d_to_v3f(sub_v3d(cur_wp, center));
	return qf_by_from_to(v1, v2);
}

internal
V3f cursor_scale_delta_in_tf_coords(T3d tf)
{
	V3d center= tf.pos;
	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3f v1= v3d_to_v3f(sub_v3d(prev_wp, center));
	V3f v2= v3d_to_v3f(sub_v3d(cur_wp, center));

	F32 s= length_v3f(v2)/length_v3f(v1);
	return (V3f) {s, s, s};
}

internal
V3d vertex_world_pos(ModelEntity *m, U32 i)
{
	TriMeshVertex *v= &m->vertices[i];
	T3d v_t= identity_t3d();
	v_t.pos= v3f_to_v3d(v->pos);

	T3d t= mul_t3d(m->tf, v_t);
	return t.pos;
}

internal
void gui_wrap(V2i *p, V2i *s)
{
	const V2i win_size= g_env.device->win_size;
	// Wrap around screen
	while (p->x < 0)
		p->x += win_size.x;
	while (p->x > win_size.x)
		p->x -= win_size.x;
	while (p->y < 0)
		p->y += win_size.y;
	while (p->y > win_size.y)
		p->y -= win_size.y;
}

// Draws single-color quad
internal
void gui_quad(V2i pix_pos, V2i pix_size, Color c)
{
	V3d pos= v2d_to_v3d(screen_to_world_point(pix_pos)); 
	V3d size= v2d_to_v3d(screen_to_world_size(pix_size));

	ModelEntity init;
	init_modelentity(&init);
	init.tf.pos= pos;
	init.tf.scale= size;
	init.free_after_draw= true;
	snprintf(init.model_name, sizeof(init.model_name), "guibox_singular");

	U32 handle= resurrect_modelentity(&init);
	ModelEntity *e= get_modelentity(handle);
	e->color= c;
}

// Draws texture of a model
internal
void gui_model_image(V2i pix_pos, V2i pix_size, ModelEntity *src_model)
{
	ensure(src_model);

	V3d pos= v2d_to_v3d(screen_to_world_point(pix_pos)); 
	V3d size= v2d_to_v3d(screen_to_world_size(pix_size));

	ModelEntity init;
	init_modelentity(&init);
	init.tf.pos= pos;
	init.tf.scale= size;
	init.free_after_draw= true;
	snprintf(init.model_name, sizeof(init.model_name), "guibox");

	U32 handle= resurrect_modelentity(&init);
	ModelEntity *e= get_modelentity(handle);
	e->atlas_uv= src_model->atlas_uv;
	e->scale_to_atlas_uv= src_model->scale_to_atlas_uv;
}

// Some common features with different resources
internal
EditorBoxState gui_editorbox(const char *label, V2i pix_pos, V2i pix_size, bool invisible)
{
	gui_wrap(&pix_pos, &pix_size);
	UiContext *ctx= g_env.uicontext;
	const V2i c_p= ctx->dev.cursor_pos;
	const Color c= (Color) {0.25, 0.25, 0.3, 0.9};

	EditorBoxState state= {};

	if (gui_is_active(label)) {
		state.pressed= false;
		if (	!ctx->dev.rmb.down &&
				!ctx->dev.grabbing && !ctx->dev.rotating && !ctx->dev.scaling) {
			state.released= true;
			gui_set_inactive(label);
		} else if (ctx->dev.rmb.down) {
			state.down= true;
		}

		if (	ctx->dev.rmb.pressed &&
				(ctx->dev.grabbing || ctx->dev.rotating || ctx->dev.scaling)) {
			// Cancel
			editor_revert_res_state();
			ctx->dev.grabbing= 0;
			ctx->dev.rotating= 0;
			ctx->dev.scaling= 0;
			gui_set_inactive(label);
		}

		if (ctx->dev.lmb.pressed) {
			ctx->dev.grabbing= 0;
			ctx->dev.rotating= 0;
			ctx->dev.scaling= 0;
			gui_set_inactive(label);
		}
	} else if (gui_is_hot(label)) {
		if (ctx->dev.rmb.pressed) {
			state.pressed= true;
			state.down= true;
			gui_set_active(label);
		} else if (ctx->dev.g_pressed) {
			ctx->dev.grabbing= gui_id(label);
			gui_set_active(label);
		} else if (ctx->dev.r_pressed) {
			ctx->dev.rotating= gui_id(label);
			gui_set_active(label);
		} else if (ctx->dev.s_pressed) {
			ctx->dev.scaling= gui_id(label);
			gui_set_active(label);
		}

		if (gui_is_active(label))
			editor_store_res_state();
	}

	if (	c_p.x >= pix_pos.x &&
			c_p.y >= pix_pos.y &&
			c_p.x < pix_pos.x + pix_size.x &&
			c_p.y < pix_pos.y + pix_size.y) {
		state.hover= true;
		gui_set_hot(label);
	}

	if (!invisible)
		gui_quad(pix_pos, pix_size, c);

	return state;
}
internal
void transform_mesh(ModelEntity *m, T3f tf, bool uv)
{
	if (m->has_own_mesh) {
		debug_print("@todo Modify unique mesh");
		return;
	}

	Mesh *mesh= model_mesh((Model*)res_by_name(	g_env.resblob,
												ResType_Model,
												m->model_name));
	if (!mesh->res.is_runtime_res)
		mesh= create_rt_mesh(mesh);

	for (U32 i= 0; i < mesh->v_count; ++i) {
		TriMeshVertex *v= &mesh_vertices(mesh)[i];
		if (!v->selected)
			continue;
		if (uv) {
			v->uv= transform_v3f(tf, v->uv);
			v->uv.x= CLAMP(v->uv.x, 0.0, 1.0);
			v->uv.y= CLAMP(v->uv.y, 0.0, 1.0);
		} else {
			v->pos= transform_v3f(tf, v->pos);
		}
	}

	mesh->res.needs_saving= true;
}

internal
void destroy_rt_armature(Resource *res);

// Creates modifiable substitute for static armature resource
internal
Armature *create_rt_armature(Armature *src)
{
	Armature *rt_armature= dev_malloc(sizeof(*rt_armature));
	*rt_armature= *src;
	substitute_res(&src->res, &rt_armature->res, NULL);
	recache_ptrs_to_armatures();
	return rt_armature;
}

internal
V2i uv_to_pix(V3f uv, V2i pix_pos, V2i pix_size)
{ return (V2i) {uv.x*pix_size.x + pix_pos.x, (1 - uv.y)*pix_size.y + pix_pos.y}; }

internal
V3d pix_to_uv(V2i p, V2i pix_pos, V2i pix_size)
{ return (V3d) {(F64)(p.x - pix_pos.x)/pix_size.x,
				1 - (F64)(p.y - pix_pos.y)/pix_size.y,
				0.0};
}

internal
void gui_uvbox(V2i pix_pos, V2i pix_size, ModelEntity *m)
{
	const char *box_label= "uvbox_box";
	UiContext *ctx= g_env.uicontext;
	gui_wrap(&pix_pos, &pix_size);

	EditorBoxState state= gui_editorbox(box_label, pix_pos, pix_size, false);

	if (!m)
		return;

	if (state.pressed) {
		// Control vertex selection
		F64 closest_dist= 0;
		U32 closest_i= NULL_HANDLE;
		for (U32 i= 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v= &m->vertices[i];
			V2i pos= uv_to_pix(v->uv, pix_pos, pix_size);

			F64 dist= dist_sqr_v2i(pos, ctx->dev.cursor_pos);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i= i;
				closest_dist= dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i= 0; i < m->mesh_v_count; ++i)
				m->vertices[i].selected= false;
		}

		if (closest_dist < 100*100) {
			ensure(closest_i != NULL_HANDLE);
			toggle_bool(&m->vertices[closest_i].selected);
		}
	}

	if (ctx->dev.grabbing == gui_id(box_label)) {
		// Move selected uv coords
		V3d cur= pix_to_uv(ctx->dev.cursor_pos, pix_pos, pix_size);
		V3d prev= pix_to_uv(ctx->dev.prev_cursor_pos, pix_pos, pix_size);
		T3f delta= {{1, 1, 1}, identity_qf(), v3d_to_v3f(sub_v3d(cur, prev))};
		transform_mesh(m, delta, true);
	}

	if (ctx->dev.rotating == gui_id(box_label)) {
		debug_print("@todo Uv rotating");
		ctx->dev.rotating= 0;
	}

	if (ctx->dev.scaling == gui_id(box_label)) {
		debug_print("@todo Uv scaling");
		ctx->dev.scaling= 0;
	}


	V2i padding= {20, 20};
	pix_pos= add_v2i(pix_pos, padding);
	pix_size= sub_v2i(pix_size, scaled_v2i(2, padding));

	gui_model_image(pix_pos, pix_size, m);

	for (U32 i= 0; i < m->mesh_v_count; ++i) {
		TriMeshVertex *v= &m->vertices[i];

		V2i pix_uv= uv_to_pix(v->uv, pix_pos, pix_size);
		gui_wrap(&pix_uv, NULL);
		V2d p= screen_to_world_point(pix_uv);
		const F64 v_size= editor_vertex_size();
		V3d poly[4]= {
			{-v_size + p.x, -v_size + p.y, 0},
			{-v_size + p.x, +v_size + p.y, 0},
			{+v_size + p.x, +v_size + p.y, 0},
			{+v_size + p.x, -v_size + p.y, 0},
		};
		if (v->selected)
			ddraw_poly((Color) {1.0, 0.7, 0.2, 0.8}, poly, 4);
		else
			ddraw_poly((Color) {0.0, 0.0, 0.0, 0.8}, poly, 4);
	}

	Color fill_color= {0.6, 0.6, 0.8, 0.4};
	V3d poly[3];
	for (U32 i= 0; i < m->mesh_i_count; ++i) {
		U32 v_i= m->indices[i];
		TriMeshVertex *v= &m->vertices[v_i];
		V2i pix_uv= uv_to_pix(v->uv, pix_pos, pix_size);
		V2d p= screen_to_world_point(pix_uv);
		poly[i%3]= (V3d) {p.x, p.y, 0};

		if (i % 3 == 2)
			ddraw_poly(fill_color, poly, 3);
	}
}

// Mesh editing on world
internal
void gui_mesh_overlay(U32 *model_h, bool *is_edit_mode)
{
	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	F64 v_size= editor_vertex_size();

	const char *box_label= "mesh_overlay_box";
	EditorBoxState state=
		gui_editorbox(box_label, (V2i) {0, 0}, g_env.device->win_size, true);

	if (!*is_edit_mode) { // Mesh select mode
		if (state.down)
			*model_h= find_modelentity_at_pixel(ctx->dev.cursor_pos);
	}

	ModelEntity *m= NULL;
	if (*model_h != NULL_HANDLE)
		m= get_modelentity(*model_h);
	else
		return;

	if (ctx->dev.toggle_select_all) {
		if (*is_edit_mode) {
			bool some_selected= false;
			for (U32 i= 0; i < m->mesh_v_count; ++i) {
				if (m->vertices[i].selected)
					some_selected= true;
			}
			for (U32 i= 0; i < m->mesh_v_count; ++i) {
				m->vertices[i].selected= !some_selected;
			}
		} else {
			*model_h= NULL_HANDLE;
			return;
		}
	}

	if (*is_edit_mode && (ctx->dev.grabbing || ctx->dev.rotating || ctx->dev.scaling)) {
		T3f delta= {{1, 1, 1}, identity_qf(), {0, 0, 0}};
		if (ctx->dev.grabbing == gui_id(box_label)) {
			delta.pos= cursor_delta_in_tf_coords(m->tf);
		} else if (ctx->dev.rotating == gui_id(box_label)) {
			delta.rot= cursor_rot_delta_in_tf_coords(m->tf);
		} else if (ctx->dev.scaling == gui_id(box_label)) {
			delta.scale= cursor_scale_delta_in_tf_coords(m->tf);
		}
		transform_mesh(m, delta, false);
	}

	if (*is_edit_mode && state.pressed) {
		// Control vertex selection
		F64 closest_dist= 0;
		U32 closest_i= NULL_HANDLE;
		for (U32 i= 0; i < m->mesh_v_count; ++i) {
			V3d pos= vertex_world_pos(m, i);

			F64 dist= dist_sqr_v3d(pos, cur_wp);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i= i;
				closest_dist= dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i= 0; i < m->mesh_v_count; ++i)
				m->vertices[i].selected= false;
		}

		if (closest_i != NULL_HANDLE && closest_dist < 2.0)
			toggle_bool(&m->vertices[closest_i].selected);
	}

	// Draw vertices
	if (*is_edit_mode) {
		for (U32 i= 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v= &m->vertices[i];
			V3d p= vertex_world_pos(m, i);
			V3d poly[4]= {
				{-v_size + p.x, -v_size + p.y, p.z},
				{-v_size + p.x, +v_size + p.y, p.z},
				{+v_size + p.x, +v_size + p.y, p.z},
				{+v_size + p.x, -v_size + p.y, p.z},
			};

			if (v->selected)
				ddraw_poly((Color) {1.0, 0.7, 0.2, 0.8}, poly, 4);
			else
				ddraw_poly((Color) {0.0, 0.0, 0.0, 0.8}, poly, 4);
		}
	}
}

F64 sign(F64 a)
{ return (0 < a) - (a < 0); }

// Armature editing on world
internal
void gui_armature_overlay(U32 *comp_h, bool *is_edit_mode)
{
	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));

	const char *box_label= "armature_overlay_box";
	EditorBoxState state=
		gui_editorbox(box_label, (V2i) {0, 0}, g_env.device->win_size, true);

	if (!*is_edit_mode) {
		if (state.down)
			*comp_h= find_compentity_at_pixel(ctx->dev.cursor_pos);
	}

	CompEntity *entity= NULL;
	if (*comp_h != NULL_HANDLE)
		entity= get_compentity(*comp_h);
	else
		return;
	Armature *a= entity->armature;
	T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
	calc_global_pose(global_pose, entity);

	if (ctx->dev.toggle_select_all) {
		if (*is_edit_mode) {
			bool some_selected= false;
			for (U32 i= 0; i < a->joint_count; ++i) {
				if (a->joints[i].selected)
					some_selected= true;
			}
			for (U32 i= 0; i < a->joint_count; ++i)
				a->joints[i].selected= false;
			a->joints[0].selected= !some_selected;
		} else {
			*comp_h= NULL_HANDLE;
			return;
		}
	}

	if (*is_edit_mode && (ctx->dev.grabbing || ctx->dev.rotating || ctx->dev.scaling)) {
		if (!a->res.is_runtime_res)
			a= create_rt_armature(a);

		for (U32 i= 0; i < a->joint_count; ++i) {
			if (!a->joints[i].selected)
				continue;

			if (ctx->dev.grabbing == gui_id(box_label)) {
				T3d coords= entity->tf;
				U32 super_i= a->joints[i].super_id;
				if (super_i != NULL_JOINT_ID)
					coords= global_pose[super_i];

				V3f translation= cursor_delta_in_tf_coords(coords);

				{ // `translation` from cur pose coords to bind pose coords
					T3f to_bind= inv_t3f(entity->pose.tf[i]);
					V3f a= transform_v3f(to_bind, (V3f) {0, 0, 0});
					V3f b= transform_v3f(to_bind, translation);
					translation= sub_v3f(b, a);
				}

				V3f *bind_pos= &a->joints[i].bind_pose.pos;
				*bind_pos= add_v3f(*bind_pos, translation);
			} else if (ctx->dev.rotating == gui_id(box_label)) {
				Qf rot= cursor_rot_delta_in_tf_coords(global_pose[i]);
				Qf *bind_rot= &a->joints[i].bind_pose.rot;
				*bind_rot= mul_qf(rot, *bind_rot);
			} else if (ctx->dev.scaling == gui_id(box_label)) {
				V3f s= cursor_scale_delta_in_tf_coords(global_pose[i]);
				V3f *bind_scale= &a->joints[i].bind_pose.scale;
				*bind_scale= mul_v3f(s, *bind_scale);
			}
		}

		a->res.needs_saving= true;
	}

	if (*is_edit_mode && state.pressed) {
		// Control joint selection
		F64 closest_dist= 0;
		U32 closest_i= NULL_HANDLE;
		for (U32 i= 0; i < a->joint_count; ++i) {
			V3d pos= global_pose[i].pos;

			F64 dist= dist_sqr_v3d(pos, cur_wp);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i= i;
				closest_dist= dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i= 0; i < a->joint_count; ++i)
				a->joints[i].selected= false;
		}

		if (closest_dist < 2.0) {
			ensure(closest_i != NULL_HANDLE);
			toggle_bool(&a->joints[closest_i].selected);
		}
	}
}

void create_editor()
{
	Editor* e= zero_malloc(sizeof(*e));
	e->cur_model_h= NULL_HANDLE;
	e->cur_comp_h= NULL_HANDLE;

	g_env.editor= e;
}

void destroy_editor()
{
	editor_free_res_state();
	free(g_env.editor);
	g_env.editor= NULL;
}

void upd_editor()
{
	Editor *e= g_env.editor;

	if (g_env.device->key_pressed[KEY_F1])
		e->state= EditorState_mesh;
	if (g_env.device->key_pressed[KEY_F2])
		e->state= EditorState_armature;
	if (g_env.device->key_pressed[KEY_ESC])
		e->state= EditorState_invisible;

	if (e->state == EditorState_invisible)
		return;

	bool tab_pressed= g_env.device->key_pressed[KEY_TAB];
	if (tab_pressed)
		toggle_bool(&e->is_edit_mode);
	if (e->state == EditorState_mesh && e->cur_model_h == NULL_HANDLE)
		e->is_edit_mode= false;
	if (e->state == EditorState_armature && e->cur_comp_h == NULL_HANDLE)
		e->is_edit_mode= false;

	switch (e->state) {
	case EditorState_mesh: {
		gui_mesh_overlay(&e->cur_model_h, &e->is_edit_mode);

		ModelEntity *m= NULL;
		if (e->cur_model_h != NULL_HANDLE)
			m= get_modelentity(e->cur_model_h);

		const S32 box_size= 400;
		gui_uvbox(	(V2i) {-box_size, 0},
					(V2i) {box_size, box_size},
					m);
	} break;
	case EditorState_armature: {
		gui_armature_overlay(&e->cur_comp_h, &e->is_edit_mode);
	} break;
	default: fail("Unhandled editor state: %i", e->state);
	}

	// Draw selected things on world
	const Color inactive_color= {0.5, 0.5, 0.5, 0.5};

	// Draw mesh
	if (e->cur_model_h != NULL_HANDLE) {
		ModelEntity	*m= get_modelentity(e->cur_model_h);
		Color fill_color= {0.6, 0.6, 0.8, 0.4};
		if (!e->is_edit_mode)
			fill_color= (Color) {1.0, 0.8, 0.5, 0.6};

		Color poly_color= fill_color;
		if (e->state != EditorState_mesh)
			poly_color= inactive_color;

		V3d poly[3];
		for (U32 i= 0; i < m->mesh_i_count; ++i) {
			U32 v_i= m->indices[i];
			V3d p= vertex_world_pos(m, v_i);
			poly[i%3]= p;

			if (i % 3 == 2)
				ddraw_poly(poly_color, poly, 3);
		}
	}

	// Draw armature
	if (e->cur_comp_h != NULL_HANDLE){
		CompEntity *entity= get_compentity(e->cur_comp_h);
		Armature *a= entity->armature;
		T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
		calc_global_pose(global_pose, entity);

		Color default_color= {0.6, 0.6, 0.8, 0.8};
		Color selected_color= {1.0, 0.8, 0.5, 0.7};
		Color line_color= {0.0, 0.0, 0.0, 1.0};
		Color orientation_color= {1.0, 1.0, 1.0, 0.8};
		if (!e->is_edit_mode)
			line_color= selected_color;
		if (e->state != EditorState_armature) {
			default_color= inactive_color;
			selected_color= inactive_color;
			line_color= inactive_color;
		}

		F64 rad= editor_vertex_size()*3;
		for (U32 i= 0; i < a->joint_count; ++i) {
			V3d p= global_pose[i].pos;

			const U32 v_count= 15;
			V3d v[v_count];
			for (U32 i= 0; i < v_count; ++i) {
				F64 a= i*3.141*2.0/v_count;
				v[i].x= p.x + cos(a)*rad;
				v[i].y= p.y + sin(a)*rad;
				v[i].z= 0.0;
			}

			Color c= default_color;
			if (a->joints[i].selected || !e->is_edit_mode)
				c= selected_color;
			ddraw_poly(c, v, v_count);

			V3d end_p= transform_v3d(global_pose[i], (V3d) {rad, 0, 0});
			ddraw_line(orientation_color, p, end_p);

			if (a->joints[i].super_id != NULL_JOINT_ID) {
				ddraw_line(	line_color,
							p,
							global_pose[a->joints[i].super_id].pos);
			}
		}
	}
}
