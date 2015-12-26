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
		l.size[0] = 228;
		l.size[1] = 408;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
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
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1546540848;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_layoutwin");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 450;
		l.offset[1] = 23;
		l.has_size = 1;
		l.size[0] = 302;
		l.size[1] = 538;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
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
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3124663356;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "panel");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 0;
		l.offset[1] = 456;
		l.has_size = 1;
		l.size[0] = 782;
		l.size[1] = 99;
		l.prevent_resizing = 1;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 1;
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
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
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
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 4016554794;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Gui components");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 353;
		l.offset[1] = 70;
		l.has_size = 1;
		l.size[0] = 214;
		l.size[1] = 320;
		l.prevent_resizing = 1;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
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
		append_element_layout(ctx, l);
	}

}
