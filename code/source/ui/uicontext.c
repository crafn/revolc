#include "core/device.h"
#include "global/env.h"
#include "uicontext.h"

void create_uicontext()
{
	UiContext *ctx = ZERO_ALLOC(gen_ator(), sizeof(*ctx), "uicontext");
	g_env.uicontext = ctx;
}

void destroy_uicontext()
{
	FREE(gen_ator(), g_env.uicontext);
	g_env.uicontext = NULL;
}

void upd_uicontext()
{
	UiContext *ctx = g_env.uicontext;

	ctx->dev.prev_cursor_pos = ctx->dev.cursor_pos;
	ctx->dev.cursor_pos = g_env.device->cursor_pos;
	ctx->dev.cursor_delta = sub_v2i(ctx->dev.cursor_pos, ctx->dev.prev_cursor_pos);

	ctx->dev.lmb.pressed = g_env.device->key_pressed[KEY_LMB];
	ctx->dev.lmb.down = g_env.device->key_down[KEY_LMB];
	ctx->dev.lmb.released = g_env.device->key_released[KEY_LMB];

	ctx->dev.rmb.pressed = g_env.device->key_pressed[KEY_RMB];
	ctx->dev.rmb.down = g_env.device->key_down[KEY_RMB];
	ctx->dev.rmb.released = g_env.device->key_released[KEY_RMB];

	ctx->dev.shift_down = g_env.device->key_down[KEY_LSHIFT];
	ctx->dev.snap_to_closest = g_env.device->key_down[KEY_LCTRL];
	ctx->dev.g_pressed = g_env.device->key_pressed['g'];
	ctx->dev.r_pressed = g_env.device->key_pressed['r'];
	ctx->dev.s_pressed = g_env.device->key_pressed['s'];
	ctx->dev.toggle_select_all = g_env.device->key_pressed['a'];
	ctx->dev.toggle_play = g_env.device->key_pressed[KEY_SPACE];
	ctx->dev.delete = g_env.device->key_pressed['x'];

	ensure(ctx->turtle_i == 0);
	ctx->turtles[0] = (UiContext_Turtle) {
		.dir = (V2i) {0, 1},
	};
	ctx->listbox_released = false;

	ctx->last_hot_id = ctx->hot_id;
	ctx->hot_id = 0;
}

void gui_set_turtle_pos(V2i pos)
{
	UiContext *ctx = g_env.uicontext;
	ctx->turtles[ctx->turtle_i].pos = pos;
}

V2i gui_turtle_pos()
{
	UiContext *ctx = g_env.uicontext;
	return ctx->turtles[ctx->turtle_i].pos;
}

void gui_advance_turtle(V2i elem_size)
{
	UiContext *ctx = g_env.uicontext;
	UiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_i];

	const V2i adv = mul_v2i(turtle->dir, elem_size);
	turtle->last_adv_size = elem_size;
	turtle->pos = add_v2i(turtle->pos, adv);
}

V2i gui_last_adv_size()
{
	UiContext *ctx = g_env.uicontext;
	UiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_i];
	return turtle->last_adv_size;
}

S32 gui_next_draw_layer()
{
	UiContext *ctx = g_env.uicontext;
	UiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_i];

	// Should return something > 0, as zero is the world layer
	return (turtle->draw_i++) + 1337;
}

void gui_begin(V2i turtle_dir)
{
	UiContext *ctx = g_env.uicontext;
	ensure(ctx->turtle_i < MAX_GUI_STACK_SIZE);

	UiContext_Turtle *prev = &ctx->turtles[ctx->turtle_i];
	UiContext_Turtle *cur = &ctx->turtles[ctx->turtle_i + 1];

	++ctx->turtle_i;
	*cur = (UiContext_Turtle) {
		.pos = prev->pos,
		.dir = turtle_dir,
		.draw_i = prev->draw_i,
	};
}

void gui_end()
{
	UiContext *ctx = g_env.uicontext;

	ensure(ctx->turtle_i > 0);
	--ctx->turtle_i;
}

internal
U32 hash32(const U8* buf, U32 size)
{
	// Modified FNV-1a
	U32 hash = 2166136261;
	for (U32 i = 0; i < size; ++i)
		hash = ((hash ^ buf[i]) + 379721)*16777619;
	return hash;
}

GuiId gui_id(const char *label)
{ return hash32((U8*)label, strlen(label)); }

void gui_set_hot(const char *label)
{
	UiContext *ctx = g_env.uicontext;
	if (ctx->active_id == 0) {
		//debug_print("set_hot %s", label);
		ctx->hot_id = gui_id(label);
	}
}

bool gui_is_hot(const char *label)
{
	UiContext *ctx = g_env.uicontext;
	return ctx->last_hot_id == gui_id(label);
}

void gui_set_active(const char *label)
{
	//debug_print("set_active %s", label);
	UiContext *ctx = g_env.uicontext;
	ctx->active_id = gui_id(label);
}

void gui_set_inactive(const char *label)
{
	//debug_print("set_inactive %s", label);
	UiContext *ctx = g_env.uicontext;
	if (ctx->active_id == gui_id(label))
		ctx->active_id = 0;
}

bool gui_is_active(const char *label)
{
	UiContext *ctx = g_env.uicontext;
	return ctx->active_id == gui_id(label);
}
