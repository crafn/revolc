void load_layout(GuiContext *ctx)
{
	ctx->layout_count = 0;
	{
		GuiElementLayout l = {0};
		l.id = 266850159;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "btn_in_list+10|button_10");
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
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
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 0;
		l.size[1] = 25;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 944829634;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "win");
		l.has_offset = 1;
		l.offset[0] = 7;
		l.offset[1] = 23;
		l.has_size = 1;
		l.size[0] = 210;
		l.size[1] = 414;
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
		l.has_offset = 1;
		l.offset[0] = 527;
		l.offset[1] = 8;
		l.has_size = 1;
		l.size[0] = 246;
		l.size[1] = 424;
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
		l.has_offset = 1;
		l.offset[0] = 0;
		l.offset[1] = 456;
		l.has_size = 1;
		l.size[0] = 782;
		l.size[1] = 99;
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
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
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
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 20;
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
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 15;
		l.size[1] = 15;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 4016554794;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "Gui components");
		l.has_offset = 1;
		l.offset[0] = 256;
		l.offset[1] = 26;
		l.has_size = 1;
		l.size[0] = 253;
		l.size[1] = 306;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		append_element_layout(ctx, l);
	}

}
