#include "armature_editor.h"
#include "core/basic.h"
#include "core/device.h"
#include "editor_util.h"
#include "global/env.h"
#include "ui/uicontext.h"
#include "visual/ddraw.h"
#include "visual/renderer.h"

internal
Clip *get_modifiable_clip(const char *name)
{
	return (Clip*)substitute_res(res_by_name(	g_env.resblob,
												ResType_Clip,
												name));
}

// Armature editing on world
// Returns true if editing is actively happening 
internal
bool ogui_armature_overlay(ArmatureEditor *state, bool is_edit_mode)
{
	bool editing_happening = false;
	UiContext *ctx = g_env.uicontext;
	V3d cur_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));

	const char *box_label = "armature_overlay_box";
	EditorBoxState bstate =
		gui_editorbox(g_env.uicontext->gui, NULL, NULL, box_label, true);

	if (!is_edit_mode) {
		if (bstate.down)
			state->comp_h = find_compentity_at_pixel(ctx->dev.cursor_pos);
	}
	
	CompEntity *entity = NULL;
	if (state->comp_h != NULL_HANDLE)
		entity = get_compentity(state->comp_h);
	else
		return editing_happening;
	Armature *a = (Armature*)substitute_res(&entity->armature->res);
	T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
	calc_global_pose(global_pose, entity);

	if (ctx->dev.toggle_select_all) {
		if (is_edit_mode) {
			bool some_selected = false;
			for (U32 i = 0; i < a->joint_count; ++i) {
				if (a->joints[i].selected)
					some_selected = true;
			}
			for (U32 i = 0; i < a->joint_count; ++i)
				a->joints[i].selected = !some_selected;
		} else {
			state->comp_h = NULL_HANDLE;
			return editing_happening;
		}
	}

	if (is_edit_mode) {
		for (U32 i = 0; i < a->joint_count; ++i) {
			if (!a->joints[i].selected)
				continue;

			CursorDeltaMode m = cursor_delta_mode(box_label);
			if (!m)
				continue;

			editing_happening = true;

			if (state->clip_is_bind_pose) {
				resource_modified(&a->res);
				// Modify bind pose

				T3d coords = entity->tf;
				U32 super_i = a->joints[i].super_id;
				if (super_i != NULL_JOINT_ID) {
					coords = global_pose[super_i];
					coords.pos = global_pose[i].pos; 
				}

				T3f delta;
				cursor_transform_delta_world(&delta, box_label, coords);
				V3f translation = delta.pos;
				{ // `translation` from cur pose coords to bind pose coords
					T3f to_bind = inv_t3f(entity->pose.tf[i]);
					V3f a = transform_v3f(to_bind, (V3f) {0, 0, 0});
					V3f b = transform_v3f(to_bind, translation);
					translation = sub_v3f(b, a);
				}
				delta.pos = translation;

				T3f *bind_tf = &a->joints[i].bind_pose;
				bind_tf->pos = add_v3f(delta.pos, bind_tf->pos);
				bind_tf->rot = mul_qf(delta.rot, bind_tf->rot);
				bind_tf->scale = mul_v3f(delta.scale, bind_tf->scale);
			} else {
				// Modify/create keyframe

				T3d coords = global_pose[i];
				T3f delta;
				cursor_transform_delta_world(&delta, box_label, coords);
				V3f translation = delta.pos;
				{ // Not sure what happens here. It's almost the same as bind pose modifying, but with inverse transform...
					T3f from_bind = entity->pose.tf[i];
					V3f a = transform_v3f(from_bind, (V3f) {0, 0, 0});
					V3f b = transform_v3f(from_bind, translation);
					translation = sub_v3f(b, a);
				}
				delta.pos = translation;

				const T3f base = entity->pose.tf[i];

				Clip_Key key = {
					.joint_id = i,
					.time = state->clip_time,
				};
				switch (m) {
				case CursorDeltaMode_scale:
					key.type = Clip_Key_Type_scale;
					key.value.scale = mul_v3f(delta.scale, base.scale);
					entity->pose.tf[i].scale = key.value.scale;
				break;
				case CursorDeltaMode_rotate:
					key.type = Clip_Key_Type_rot;
					key.value.rot = mul_qf(delta.rot, base.rot);
					entity->pose.tf[i].rot = key.value.rot;
				break;
				case CursorDeltaMode_translate:
					key.type = Clip_Key_Type_pos;
					key.value.pos = add_v3f(delta.pos, base.pos);
					entity->pose.tf[i].pos = key.value.pos;
				break;
				default: fail("Unknown CursorDeltaMode: %i", m);
				}

				Clip *clip = get_modifiable_clip(state->clip_name);
				update_rt_clip_key(clip, key);
			}
		}
	}

	if (is_edit_mode && bstate.pressed) {
		// Control joint selection
		F64 closest_dist = 0;
		U32 closest_i = NULL_HANDLE;
		for (U32 i = 0; i < a->joint_count; ++i) {
			V3d pos = global_pose[i].pos;

			F64 dist = dist_sqr_v3d(pos, cur_wp);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i = i;
				closest_dist = dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i = 0; i < a->joint_count; ++i)
				a->joints[i].selected = false;
		}

		if (closest_dist < 2.0) {
			ensure(closest_i != NULL_HANDLE);
			toggle_bool(&a->joints[closest_i].selected);
		}
	}
	return editing_happening;
}

void do_armature_editor(	ArmatureEditor *state,
							bool is_edit_mode,
							bool active)
{
	UiContext *ctx = g_env.uicontext;
	GuiContext *gui = ctx->gui;

	if (active) {
		bool editing_happening = ogui_armature_overlay(state, is_edit_mode);

		CompEntity *entity = NULL;
		Armature *a = NULL;
		if (state->comp_h != NULL_HANDLE) {
			entity = get_compentity(state->comp_h);
			a = entity->armature;	
		}

		gui_res_info(ResType_Armature, a ? &a->res : NULL);

		{ // Timeline box
			gui_begin(gui, "timeline");
			V2i px_pos, px_size;
			gui_turtle_pos(gui, &px_pos.x, &px_pos.y);
			gui_turtle_size(gui, &px_size.x, &px_size.y);

			drawcmd_px_quad(px_pos, px_size, 0.0, panel_color(), outline_color(panel_color()), ogui_next_draw_layer());

			ogui_set_turtle_pos(px_pos);

			if (strlen(state->clip_name) == 0)
				fmt_str(state->clip_name, RES_NAME_SIZE,
						"%s", "bind_pose");

			U32 clip_count;
			Clip **clips = (Clip **)all_res_by_type(	&clip_count,
													g_env.resblob,
													ResType_Clip);

			// Listbox containing all animation clips
			if (ogui_begin_combobox(frame_str("Clip: %s", state->clip_name))) {
				for (U32 i = 0; i < clip_count + 1; ++i) {
					const char *name = "bind_pose";
					if (i < clip_count)
						name = clips[i]->res.name;
					if (ogui_combobox_item(name))
						fmt_str(state->clip_name, RES_NAME_SIZE, "%s", name);
				}
				ogui_end_combobox();
			}

			state->clip_is_bind_pose =
				!strcmp(state->clip_name, "bind_pose");

			// "Make looping", "Delete" and "Play" -buttons
			if (!state->clip_is_bind_pose) {
				if (ogui_button("Make looping", NULL, NULL)) {
					Clip *clip = get_modifiable_clip(state->clip_name);
					make_rt_clip_looping(clip);
				}

				if (	ogui_button("Delete key <x>", NULL, NULL)
						|| ctx->dev.delete) {

					Clip *clip = get_modifiable_clip(state->clip_name);

					// Delete all keys of selected joints at current time
					bool key_deleted;
					do {
						key_deleted = false;
						for (U32 i = 0; i < clip->key_count; ++i) {
							Clip_Key key = clip_keys(clip)[i];
							if (	key.time == state->clip_time &&
									a->joints[key.joint_id].selected) {
								delete_rt_clip_key(clip, i);
								key_deleted = true;
								break;
							}
						}
					} while (key_deleted);
				}

				if (	ogui_button(	state->is_playing ?
										"Pause <space>" : "Play <space>",
									NULL, NULL)
						|| ctx->dev.toggle_play)
					toggle_bool(&state->is_playing);
			}

			// Selected joints
			if (a && is_edit_mode) {
				ogui_text(" Selected joints: ");
				int count = 0;
				for (U32 i = 0; i < a->joint_count; ++i) {
					if (!a->joints[i].selected)
						continue;
					if (count > 0)
						ogui_text(", ");
					ogui_text(a->joint_names[i]);
					++count;
				}
			}

			// Interior of timeline
			px_pos.x += 10;
			px_pos.y += 27;
			px_size.x -= 20;
			px_size.y -= 27;
			Color c = darken_color(panel_color());
			drawcmd_px_quad(px_pos, px_size, 0.0, c, outline_color(c), ogui_next_draw_layer());
			const char *clip_timeline_label = "clip_timeline";
			EditorBoxState bstate =
				gui_editorbox(g_env.uicontext->gui, NULL, NULL, clip_timeline_label, true);
			if (entity && a) {
				if (state->clip_is_bind_pose) {
					entity->pose = identity_pose();
					state->is_playing = false;
				} else {
					const Clip *clip =
						(Clip*)substitute_res(res_by_name(	g_env.resblob,
															ResType_Clip,
															state->clip_name));

					if (bstate.ldown) { // LMB
						// Set time
						F64 lerp = (F64)(g_env.uicontext->dev.cursor_pos.x -
										px_pos.x)/px_size.x;
						F64 t = lerp*clip->duration;
						if (ctx->dev.snap_to_closest) {
							F64 closest_t = -1000000;
							for (U32 i = 0; i < clip->key_count; ++i) {
								F64 key_t = clip_keys(clip)[i].time;
								if (	ABS(t - key_t) <
										ABS(t - closest_t))
									closest_t = key_t;
							}
							t = closest_t;
						}
						state->clip_time = CLAMP(t, 0, clip->duration);
					}

					// Move keys
					CursorDeltaMode m = cursor_delta_mode(clip_timeline_label);
					if (m == CursorDeltaMode_translate) {
						Clip *clip = get_modifiable_clip(state->clip_name);

						T3d coords = {{1, 1, 1}, identity_qd(), {0, 0, 0}};
						T3f delta;
						cursor_transform_delta_pixels(	&delta,
														clip_timeline_label,
														coords);
						const F64 dt = clip->duration*delta.pos.x/px_size.x;
						const F64 target_t =
							CLAMP(state->clip_time + dt, 0, clip->duration);
						move_rt_clip_keys(clip, state->clip_time, target_t);
						state->clip_time = target_t;
					}

					// Show keys
					for (U32 key_i = 0; key_i < clip->key_count; ++key_i) {
						Clip_Key key = clip_keys(clip)[key_i];

						F64 lerp_x = key.time/clip->duration;
						F64 lerp_y = (F64)key.joint_id/clip->joint_count;
						V2i pos = {
							px_pos.x + px_size.x*lerp_x - 3,
							px_pos.y + px_size.y*lerp_y - 6 + 4*key.type
						};
						V2i size = {6, 3};

						Color color = (Color [4]) {
							{}, // none
							{1.0, 0.0, 0.0, 1.0}, // scale
							{0.0, 1.0, 0.0, 1.0}, // rot
							{0.0, 0.0, 1.0, 1.0}, // pos
						}[key.type];
						if (key.time == state->clip_time && a->joints[key.joint_id].selected) {
							color = (Color) {1.0, 1.0, 1.0, 1.0};
							size.y += 5;
						}
						drawcmd_px_quad(pos, size, 0.0, color, color, ogui_next_draw_layer());
					}

					// Update animation to CompEntity when not actively editing
					// This because calculated pose doesn't exactly match
					// with keys (discretization error) and causes feedback loop
					if (state->is_playing)
						state->clip_time += g_env.device->dt;
					while (state->clip_time > clip->duration)
						state->clip_time -= clip->duration;
					if (!editing_happening)
						entity->pose = calc_clip_pose(clip, state->clip_time);

					// Show timeline cursor
					F64 lerp = state->clip_time/clip->duration;
					V2i time_cursor_pos = {
						px_pos.x + px_size.x*lerp - 1, px_pos.y
					};
					Color c = {1, 1, 0, 0.8};
					drawcmd_px_quad(time_cursor_pos, (V2i){2, px_size.y}, 0.0,
									c, c, ogui_next_draw_layer());
				}
			}

			gui_end(gui);
		}
	}

	// Draw armature
	if (state->comp_h != NULL_HANDLE){
		CompEntity *entity = get_compentity(state->comp_h);
		Armature *a = entity->armature;
		T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
		calc_global_pose(global_pose, entity);

		Color default_color = {0.6, 0.6, 0.8, 0.8};
		Color selected_color = {1.0, 0.8, 0.5, 0.7};
		Color line_color = {0.0, 0.0, 0.0, 1.0};
		Color orientation_color = {1.0, 1.0, 1.0, 0.8};
		if (!is_edit_mode)
			line_color = selected_color;
		if (!active) {
			default_color = inactive_color();
			selected_color = inactive_color();
			line_color = inactive_color();
		}

		F64 rad = editor_vertex_size()*3;
		for (U32 i = 0; i < a->joint_count; ++i) {
			V3d p = global_pose[i].pos;

			const U32 v_count = 15;
			V3d v[v_count];
			for (U32 i = 0; i < v_count; ++i) {
				F64 a = i*3.141*2.0/v_count;
				v[i].x = p.x + cos(a)*rad;
				v[i].y = p.y + sin(a)*rad;
				v[i].z = 0.0;
			}

			Color c = default_color;
			if (a->joints[i].selected || !is_edit_mode)
				c = selected_color;
			ddraw_poly(c, v, v_count);

			V3d end_p = transform_v3d(global_pose[i], (V3d) {rad, 0, 0});
			ddraw_line(orientation_color, p, end_p);

			if (a->joints[i].super_id != NULL_JOINT_ID) {
				ddraw_line(	line_color,
							p,
							global_pose[a->joints[i].super_id].pos);
			}
		}
	}
}
