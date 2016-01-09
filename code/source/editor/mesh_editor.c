#include "mesh_editor.h"
#include "editor_util.h"

internal
V3d vertex_world_pos(ModelEntity *m, U32 i)
{
	TriMeshVertex *v = &m->vertices[i];
	T3d v_t = identity_t3d();
	v_t.pos = v3f_to_v3d(v->pos);

	T3d t = mul_t3d(m->tf, v_t);
	return t.pos;
}

internal
V2i uv_to_pix(V2f uv, V2i pix_pos, V2i pix_size)
{ return (V2i) {uv.x*pix_size.x + pix_pos.x, (1 - uv.y)*pix_size.y + pix_pos.y}; }

internal
V3d pix_to_uv(V2i p, V2i pix_pos, V2i pix_size)
{ return (V3d) {(F64)(p.x - pix_pos.x)/pix_size.x,
				1 - (F64)(p.y - pix_pos.y)/pix_size.y,
				0.0};
}

internal
Mesh *editable_model_mesh(const char *name)
{
	Mesh *mesh = model_mesh((Model*)res_by_name(	g_env.resblob,
													ResType_Model,
													name));
	mesh = (Mesh*)substitute_res(&mesh->res);
	return mesh;
}

typedef enum {
	MeshTransformType_pos,
	MeshTransformType_uv,
	MeshTransformType_outline_uv,
} MeshTransformType;

internal
void transform_mesh(ModelEntity *m, T3f tf, MeshTransformType ttype)
{
	if (m->has_own_mesh) {
		debug_print("@todo Modify unique mesh");
		return;
	}

	Mesh *mesh = editable_model_mesh(m->model_name);

	for (U32 i = 0; i < mesh->v_count; ++i) {
		TriMeshVertex *v = &mesh_vertices(mesh)[i];
		if (!v->selected)
			continue;

		if (ttype == MeshTransformType_pos) {
			v->pos = transform_v3f(tf, v->pos);
		} else if (ttype == MeshTransformType_uv) {
			F32 uvz = v->uv.z;
			v->uv = transform_v3f(tf, v->uv);
			v->uv.x = CLAMP(v->uv.x, 0.0, 1.0);
			v->uv.y = CLAMP(v->uv.y, 0.0, 1.0);
			v->uv.z = uvz;
		} else if (ttype == MeshTransformType_outline_uv) {
			V3f uv = {v->outline_uv.x, v->outline_uv.y, 0.0};
			v->outline_uv = v3f_to_v2f(transform_v3f(tf, uv));
			v->outline_uv.x = CLAMP(v->outline_uv.x, 0.0, 1.0);
			v->outline_uv.y = CLAMP(v->outline_uv.y, 0.0, 1.0);
		}
	}

	resource_modified(&mesh->res);
}

internal
void gui_uvbox(GuiContext *gui, ModelEntity *m, bool outline_uv)
{
	const char *box_label = "uvbox_box";
	if (outline_uv)
		box_label = gui_str(gui, "outline_uvbox_box");
	UiContext *ctx = g_env.uicontext;

	V2i pix_pos, pix_size;
	EditorBoxState state = gui_editorbox(gui, &pix_pos, &pix_size, box_label, false);

	if (!m)
		return;

	if (state.pressed) {
		// Control vertex selection
		F64 closest_dist = 0;
		U32 closest_i = NULL_HANDLE;
		for (U32 i = 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v = &m->vertices[i];
			V2f uv = outline_uv ? v->outline_uv : v3f_to_v2f(v->uv);
			V2i pos = uv_to_pix(uv, pix_pos, pix_size);

			F64 dist = dist_sqr_v2i(pos, ctx->dev.cursor_pos);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i = i;
				closest_dist = dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i = 0; i < m->mesh_v_count; ++i)
				m->vertices[i].selected = false;
		}

		if (closest_dist < 100*100) {
			ensure(closest_i != NULL_HANDLE);
			toggle_bool(&m->vertices[closest_i].selected);
		}
	}

	T3d coords = {
		{pix_size.x, -pix_size.y, 1},
		identity_qd(),
		{	pix_pos.x,
			pix_pos.y,
			0.0} // @todo
	};
	T3f delta;
	if (cursor_transform_delta_pixels(&delta, box_label, coords)) {
		transform_mesh(m, delta, outline_uv ? MeshTransformType_outline_uv : MeshTransformType_uv);
	}

	V2i padding = {20, 20};
	pix_pos = add_v2i(pix_pos, padding);
	pix_size = sub_v2i(pix_size, scaled_v2i(2, padding));

	if (!outline_uv)
		drawcmd_px_model_image(pix_pos, pix_size, m, gui_layer(gui) + 2);

	for (U32 i = 0; i < m->mesh_v_count; ++i) {
		TriMeshVertex *v = &m->vertices[i];
		V2f uv = outline_uv ? v->outline_uv : v3f_to_v2f(v->uv);

		V2i pix_uv = uv_to_pix(uv, pix_pos, pix_size);
		V2d p = screen_to_world_point(pix_uv);
		const F64 v_size = editor_vertex_size();
		V3d poly[4] = {
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

	Color fill_color = {0.6, 0.6, 0.8, 0.4};
	V3d poly[3];
	for (U32 i = 0; i < m->mesh_i_count; ++i) {
		U32 v_i = m->indices[i];
		TriMeshVertex *v = &m->vertices[v_i];
		V2f uv = outline_uv ? v->outline_uv : v3f_to_v2f(v->uv);
		V2i pix_uv = uv_to_pix(uv, pix_pos, pix_size);
		V2d p = screen_to_world_point(pix_uv);
		poly[i%3] = (V3d) {p.x, p.y, 0};

		if (i % 3 == 2)
			ddraw_poly(fill_color, poly, 3);
	}
}

// Mesh editing on world
internal
void gui_mesh_overlay(U32 *model_h, bool *is_edit_mode)
{
	UiContext *ctx = g_env.uicontext;
	V3d cur_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	F64 v_size = editor_vertex_size();

	const char *box_label = "mesh_overlay_box";
	EditorBoxState state = gui_editorbox(ctx->gui, NULL, NULL, box_label, true);

	if (!*is_edit_mode) { // Mesh select mode
		if (state.down)
			*model_h = find_modelentity_at_pixel(ctx->dev.cursor_pos);
	}

	ModelEntity *m = NULL;
	if (*model_h != NULL_HANDLE)
		m = get_modelentity(*model_h);
	else
		return;

	if (ctx->dev.toggle_select_all) {
		if (*is_edit_mode) {
			bool some_selected = false;
			for (U32 i = 0; i < m->mesh_v_count; ++i) {
				if (m->vertices[i].selected)
					some_selected = true;
			}
			for (U32 i = 0; i < m->mesh_v_count; ++i) {
				m->vertices[i].selected = !some_selected;
			}
		} else {
			*model_h = NULL_HANDLE;
			return;
		}
	}

	if (*is_edit_mode && g_env.device->key_pressed['e']) {
		ctx->dev.grabbing = gui_id(box_label);
		gui_set_active(ctx->gui, box_label);
		editor_store_res_state();

		// Extrude selected vertices
		Mesh *mesh = editable_model_mesh(m->model_name);
		U32 old_v_count = mesh->v_count;
		for (U32 i = 0; i < old_v_count; ++i) {
			TriMeshVertex v = mesh_vertices(mesh)[i];
			if (!v.selected)
				continue;
			add_rt_mesh_vertex(mesh, v); // Duplicate selected vertices for extrude
			mesh_vertices(mesh)[i].selected = false;
		}

		// @todo Create indices for extruded faces

		recache_ptrs_to_meshes();
	}

	if (*is_edit_mode && g_env.device->key_pressed['f']) {
		// Create face between three vertices
		Mesh *mesh = editable_model_mesh(m->model_name);
		U32 selected_count = 0;
		MeshIndexType indices[3];
		for (U32 i = 0; i < mesh->v_count; ++i) {
			TriMeshVertex v = mesh_vertices(mesh)[i];
			if (!v.selected)
				continue;
			if (selected_count < ARRAY_COUNT(indices))
				indices[selected_count++] = i;
		}
		if (selected_count == 3) {
			add_rt_mesh_index(mesh, indices[0]);
			add_rt_mesh_index(mesh, indices[1]);
			add_rt_mesh_index(mesh, indices[2]);
		} else {
			debug_print("Select 3 vertices to make a face");
		}
		recache_ptrs_to_meshes();
	}

	if (*is_edit_mode && g_env.device->key_pressed['x']) {
		// Delete selected vertices (and corresponding faces)
		Mesh *mesh = editable_model_mesh(m->model_name);
		for (U32 i = 0; i < mesh->v_count;) {
			TriMeshVertex v = mesh_vertices(mesh)[i];
			if (v.selected)
				remove_rt_mesh_vertex(mesh, i);
			else
				++i;
		}
		recache_ptrs_to_meshes();
	}

	T3f delta;
	if (*is_edit_mode && cursor_transform_delta_world(&delta, box_label, m->tf)) {
		transform_mesh(m, delta, MeshTransformType_pos);
	}

	if (*is_edit_mode && state.pressed) {
		// Control vertex selection
		F64 closest_dist = 0;
		U32 closest_i = NULL_HANDLE;
		for (U32 i = 0; i < m->mesh_v_count; ++i) {
			V3d pos = vertex_world_pos(m, i);

			F64 dist = dist_sqr_v3d(pos, cur_wp);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i = i;
				closest_dist = dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i = 0; i < m->mesh_v_count; ++i)
				m->vertices[i].selected = false;
		}

		if (closest_i != NULL_HANDLE && closest_dist < 2.0)
			toggle_bool(&m->vertices[closest_i].selected);
	}

	// Draw vertices
	if (*is_edit_mode) {
		for (U32 i = 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v = &m->vertices[i];
			V3d p = vertex_world_pos(m, i);
			V3d poly[4] = {
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

void do_mesh_editor(U32 *model_h, bool *is_edit_mode, bool active)
{
	GuiContext *ctx = g_env.uicontext->gui;
	bool changed = false;

	if (active) {
		gui_mesh_overlay(model_h, is_edit_mode);

		ModelEntity *m = NULL;
		if (*model_h != NULL_HANDLE)
			m = get_modelentity(*model_h);

		gui_res_info(	ResType_Model,
						m ? res_by_name(g_env.resblob,
										ResType_Model,
										m->model_name) : NULL);

		gui_uvbox(g_env.uicontext->gui, m, false);
		gui_uvbox(g_env.uicontext->gui, m, true);

		gui_begin_panel(ctx, "model_settings");
		if (m) {
			Model *model = (Model*)substitute_res(res_by_name(g_env.resblob, ResType_Model, m->model_name));
			Mesh *mesh = editable_model_mesh(m->model_name);

			bool col_changed = false;
			{ // Model settings
				gui_label(ctx, "model_setting+l1|Model settings");
				col_changed |= gui_slider(ctx, "model_setting+r|R", &model->color.r, 0.0, 1.0);
				col_changed |= gui_slider(ctx, "model_setting+g|G", &model->color.g, 0.0, 1.0);
				col_changed |= gui_slider(ctx, "model_setting+b|B", &model->color.b, 0.0, 1.0);
				col_changed |= gui_slider(ctx, "model_setting+a|A", &model->color.a, 0.0, 1.0);

				if (col_changed)
					resource_modified(&model->res);
			}

			{ // Vertex attributes
				V3f pos = {};
				Color col = white_color();
				Color outline_col = white_color();
				F32 col_exp = 1.0;
				F32 outline_exp = 1.0;
				F32 outline_width = 1.0;
				for (U32 i = 0; i < mesh->v_count; ++i) {
					TriMeshVertex *v = &mesh_vertices(mesh)[i];
					if (!v->selected)
						continue;
					pos = v->pos;
					col = v->color;
					outline_col = v->outline_color;
					col_exp = v->color_exp;
					outline_exp = v->outline_exp;
					outline_width = v->outline_width;
					break;
				}

				gui_label(ctx, "model_setting+l2|Vertex attributes");

				bool v_x_changed = gui_slider(ctx, "model_setting+vx|X", &pos.x, -1.0, 1.0);
				bool v_y_changed = gui_slider(ctx, "model_setting+vy|Y", &pos.y, -1.0, 1.0);
				bool v_z_changed = gui_slider(ctx, "model_setting+vz|Z", &pos.z, -1.0, 1.0);

				gui_label(ctx, "model_setting+l3|Color");
				bool v_col_changed = false;
				v_col_changed |= gui_slider(ctx, "model_setting+vr|R", &col.r, 0.0, 1.0);
				v_col_changed |= gui_slider(ctx, "model_setting+vg|G", &col.g, 0.0, 1.0);
				v_col_changed |= gui_slider(ctx, "model_setting+vb|B", &col.b, 0.0, 1.0);
				v_col_changed |= gui_slider(ctx, "model_setting+va|A", &col.a, 0.0, 1.0);

				bool col_exp_changed = gui_slider(ctx, "model_setting+vexp|Color exp", &col_exp, 0.0, 5.0);

				gui_label(ctx, "model_setting+l4|Outline color");
				bool v_out_col_changed = false;
				v_out_col_changed |= gui_slider(ctx, "model_setting+vor|R", &outline_col.r, 0.0, 1.0);
				v_out_col_changed |= gui_slider(ctx, "model_setting+vog|G", &outline_col.g, 0.0, 1.0);
				v_out_col_changed |= gui_slider(ctx, "model_setting+vob|B", &outline_col.b, 0.0, 1.0);
				v_out_col_changed |= gui_slider(ctx, "model_setting+voa|A", &outline_col.a, 0.0, 1.0);

				bool outline_exp_changed = gui_slider(ctx, "model_setting+voe|Outline exp", &outline_exp, 0.0, 5.0);
				bool outline_width_changed = gui_slider(ctx, "model_setting+vow|Outline width", &outline_width, 0.0, 50.0);

				for (U32 i = 0; i < mesh->v_count; ++i) {
					TriMeshVertex *v = &mesh_vertices(mesh)[i];
					if (!v->selected)
						continue;

					if (v_x_changed)
						v->pos.x = pos.x;
					if (v_y_changed)
						v->pos.y = pos.y;
					if (v_z_changed)
						v->pos.z = pos.z;
					if (v_col_changed)
						v->color = col;
					if (v_out_col_changed)
						v->outline_color = outline_col;
					if (col_exp_changed)
						v->color_exp = col_exp;
					if (outline_exp_changed)
						v->outline_exp = outline_exp;
					if (outline_width_changed)
						v->outline_width = outline_width;
				}

				changed |= col_changed || v_x_changed || v_y_changed || v_z_changed || v_col_changed || v_out_col_changed || col_exp_changed || outline_exp_changed || outline_width_changed;
			}

			if (changed) {
				resource_modified(&mesh->res);
				recache_ptrs_to_meshes();
			}
		}
		gui_end_panel(ctx);
	}

	// Draw mesh
	if (*model_h != NULL_HANDLE) {
		ModelEntity	*m = get_modelentity(*model_h);
		Color fill_color = {0.6, 0.6, 0.8, 0.4};
		if (!*is_edit_mode)
			fill_color = (Color) {1.0, 0.8, 0.5, 0.6};

		Color poly_color = fill_color;
		if (!active)
			poly_color = inactive_color();

		if (active)
			ddraw_circle((Color) {1, 1, 1, 1}, m->tf.pos, editor_vertex_size()*0.5);

		V3d poly[3];
		for (U32 i = 0; i < m->mesh_i_count; ++i) {
			U32 v_i = m->indices[i];
			V3d p = vertex_world_pos(m, v_i);
			poly[i%3] = p;

			if (i % 3 == 2 && !changed)
				ddraw_poly(poly_color, poly, 3);
		}
	}
}
