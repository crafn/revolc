void load_layout(GuiContext *ctx)
{
	ctx->layout_count = 0;
	{
		GuiElementLayout l = {0};
		l.id = 266850159;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "btn_in_list+10|button_10");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 0;
		l.padding[2] = 5;
		l.padding[3] = 3;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 329413595;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_bar");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 0;
		l.size[1] = 25;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 536052831;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "none");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 5;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 753432749;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "armature_overlay_box");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 1;
		l.align_bottom = 1;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 807068630;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_client:win");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 20;
		l.padding[1] = 20;
		l.padding[2] = 20;
		l.padding[3] = 20;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 944829634;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "win");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 9;
		l.offset[1] = 8;
		l.has_size = 1;
		l.size[0] = 310;
		l.size[1] = 421;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1060175294;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Radio 1");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1076952913;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Radio 2");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1093730532;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Radio 3");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1228705237;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_layout_list+name|  name:");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 0;
		l.padding[2] = 5;
		l.padding[3] = 1;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1261779323;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "armature_overlay_box");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 1;
		l.align_bottom = 1;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1490755955;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Model: <none>");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1546540848;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_layoutwin");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 446;
		l.offset[1] = 5;
		l.has_size = 1;
		l.size[0] = 329;
		l.size[1] = 537;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1552461022;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_client:Gui components");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 5;
		l.padding[2] = 5;
		l.padding[3] = 5;
		l.gap[0] = 0;
		l.gap[1] = 4;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2401638985;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "clip_button+list|Clip: playerch_run");
		l.on_same_row = 1;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 0;
		l.padding[2] = 5;
		l.padding[3] = 4;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2584612413;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Show layout editor");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3124663356;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "panel");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 0;
		l.offset[1] = 474;
		l.has_size = 1;
		l.size[0] = 792;
		l.size[1] = 99;
		l.prevent_resizing = 1;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 1;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3154500777;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "mesh_overlay_box");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 1;
		l.align_bottom = 1;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3281346948;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "res_info");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 320;
		l.size[1] = 25;
		l.prevent_resizing = 1;
		l.align_left = 1;
		l.align_right = 0;
		l.align_top = 1;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3299356898;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "combo|none");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 5;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3322826958;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "timeline");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 150;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 1;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3394987476;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Slider");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 0;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3582334811;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "button");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 10;
		l.size[1] = 10;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 0;
		l.padding[2] = 10;
		l.padding[3] = 5;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3740805772;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_client:gui_layoutwin");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 5;
		l.padding[2] = 10;
		l.padding[3] = 5;
		l.gap[0] = 0;
		l.gap[1] = 4;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3755050329;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Textfield");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 0;
		l.padding[2] = 5;
		l.padding[3] = 4;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3880415376;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_slider");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 15;
		l.size[1] = 15;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3965537002;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_client:panel");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 5;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3977359180;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "clip_timeline");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 120;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 1;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 4016554794;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Gui components");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 167;
		l.offset[1] = 199;
		l.has_size = 1;
		l.size[0] = 214;
		l.size[1] = 320;
		l.prevent_resizing = 1;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 4153449906;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "uvbox_box");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 300;
		l.size[1] = 300;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 1;
		l.align_top = 1;
		l.align_bottom = 0;
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 4243896424;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_client:panel");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 5;
		l.padding[2] = 5;
		l.padding[3] = 5;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

}
