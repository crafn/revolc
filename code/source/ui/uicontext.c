#include "global/env.h"
#include "platform/device.h"
#include "uicontext.h"

void create_uicontext()
{
	UiContext *ctx= zero_malloc(sizeof(*ctx));
	g_env.uicontext= ctx;
}

void destroy_uicontext()
{
	free(g_env.uicontext);
	g_env.uicontext= NULL;
}

void upd_uicontext()
{
	UiContext *ctx= g_env.uicontext;

	ctx->dev.prev_cursor_pos= ctx->dev.cursor_pos;
	ctx->dev.cursor_pos= g_env.device->cursor_pos;
	ctx->dev.cursor_delta= sub_v2i(ctx->dev.cursor_pos, ctx->dev.prev_cursor_pos);

	ctx->dev.lmb.pressed= g_env.device->key_pressed[KEY_LMB];
	ctx->dev.lmb.down= g_env.device->key_down[KEY_LMB];
	ctx->dev.lmb.released= g_env.device->key_released[KEY_LMB];

	ctx->dev.rmb.pressed= g_env.device->key_pressed[KEY_RMB];
	ctx->dev.rmb.down= g_env.device->key_down[KEY_RMB];
	ctx->dev.rmb.released= g_env.device->key_released[KEY_RMB];

	ctx->dev.shift_down= g_env.device->key_down[KEY_LSHIFT];
	ctx->dev.g_pressed= g_env.device->key_pressed['g'];
	ctx->dev.r_pressed= g_env.device->key_pressed['r'];
	ctx->dev.s_pressed= g_env.device->key_pressed['s'];
	ctx->dev.toggle_select_all= g_env.device->key_pressed['a'];

	ctx->last_hot_id= ctx->hot_id;
	ctx->hot_id= 0;
}

internal
U32 hash32(const U8* buf, U32 size)
{
	// Modified FNV-1a
	U32 hash= 2166136261;
	for (U32 i= 0; i < size; ++i)
		hash= ((hash ^ buf[i]) + 379721)*16777619;
	return hash;
}

GuiId gui_id(const char *label)
{ return hash32((U8*)label, strlen(label)); }

void gui_set_hot(const char *label)
{
	UiContext *ctx= g_env.uicontext;
	if (ctx->active_id == 0) {
		//debug_print("set_hot %s", label);
		ctx->hot_id= gui_id(label);
	}
}

bool gui_is_hot(const char *label)
{
	UiContext *ctx= g_env.uicontext;
	return ctx->last_hot_id == gui_id(label);
}

void gui_set_active(const char *label)
{
	//debug_print("set_active %s", label);
	UiContext *ctx= g_env.uicontext;
	ctx->active_id= gui_id(label);
}

void gui_set_inactive(const char *label)
{
	//debug_print("set_inactive %s", label);
	UiContext *ctx= g_env.uicontext;
	if (ctx->active_id == gui_id(label))
		ctx->active_id= 0;
}

bool gui_is_active(const char *label)
{
	UiContext *ctx= g_env.uicontext;
	return ctx->active_id == gui_id(label);
}
