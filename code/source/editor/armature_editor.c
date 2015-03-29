#include "armature_editor.h"
#include "core/misc.h"
#include "editor_util.h"
#include "global/env.h"
#include "platform/device.h"
#include "ui/uicontext.h"
#include "visual/ddraw.h"
#include "visual/renderer.h"

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

	gui_text((V2i) {100, 100}, a->res.name);

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

	if (*is_edit_mode) {
		for (U32 i= 0; i < a->joint_count; ++i) {
			if (!a->joints[i].selected)
				continue;

			T3d coords= entity->tf;
			U32 super_i= a->joints[i].super_id;
			if (super_i != NULL_JOINT_ID) {
				coords= global_pose[super_i];
				coords.pos= global_pose[i].pos; 
			}

			T3f delta;
			if (cursor_transform_delta_world(&delta, box_label, coords)) {
				if (!a->res.is_runtime_res)
					a= create_rt_armature(a);

				V3f translation= delta.pos;
				{ // `translation` from cur pose coords to bind pose coords
					T3f to_bind= inv_t3f(entity->pose.tf[i]);
					V3f a= transform_v3f(to_bind, (V3f) {0, 0, 0});
					V3f b= transform_v3f(to_bind, translation);
					translation= sub_v3f(b, a);
				}
				delta.pos= translation;

				T3f *bind_tf= &a->joints[i].bind_pose;
				bind_tf->pos= add_v3f(bind_tf->pos, delta.pos);
				bind_tf->rot= mul_qf(delta.rot, bind_tf->rot);
				bind_tf->scale= mul_v3f(delta.scale, bind_tf->scale);

				a->res.needs_saving= true;
			}
		}

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

void do_armature_editor(	U32 *comp_h,
							bool *is_edit_mode,
							bool active)
{
	if (active)
		gui_armature_overlay(comp_h, is_edit_mode);

	// Draw armature
	if (*comp_h != NULL_HANDLE){
		CompEntity *entity= get_compentity(*comp_h);
		Armature *a= entity->armature;
		T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
		calc_global_pose(global_pose, entity);

		Color default_color= {0.6, 0.6, 0.8, 0.8};
		Color selected_color= {1.0, 0.8, 0.5, 0.7};
		Color line_color= {0.0, 0.0, 0.0, 1.0};
		Color orientation_color= {1.0, 1.0, 1.0, 0.8};
		if (!*is_edit_mode)
			line_color= selected_color;
		if (!active) {
			default_color= inactive_color();
			selected_color= inactive_color();
			line_color= inactive_color();
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
			if (a->joints[i].selected || !*is_edit_mode)
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
