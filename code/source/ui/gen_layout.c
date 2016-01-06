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
		l.id = 318080033;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "show_cmd_list|Show cmds");
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
		l.padding[3] = 0;
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
		l.id = 370987631;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "datatree+Editor_editor|Editor editor >");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 0;
		l.size[0] = 200;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 10;
		l.padding[1] = 0;
		l.padding[2] = 10;
		l.padding[3] = 2;
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
		l.id = 915859504;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "model_settings");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 765;
		l.offset[1] = 331;
		l.has_size = 1;
		l.size[0] = 251;
		l.size[1] = 410;
		l.prevent_resizing = 1;
		l.align_left = 0;
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
		l.id = 1010623428;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "create_cmd|Create node command");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 24;
		l.offset[1] = 258;
		l.has_size = 1;
		l.size[0] = 931;
		l.size[1] = 467;
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
		l.id = 1071369692;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "show_node_list|Show nodes");
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
		l.id = 1099133688;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_client:world_tools");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 10;
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
		l.gap[1] = 2;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 1228705237;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_layout_list");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 10;
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
		l.id = 1529018108;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "world_tools|World tools");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 278;
		l.size[1] = 741;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 0;
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
		l.id = 1546540848;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_layoutwin");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 668;
		l.offset[1] = 19;
		l.has_size = 1;
		l.size[0] = 339;
		l.size[1] = 699;
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
		l.id = 1955553667;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "create_cmd_list_2");
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
		l.padding[2] = 0;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 3;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2005886524;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "create_cmd_list_1");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 200;
		l.size[1] = 20;
		l.prevent_resizing = 0;
		l.align_left = 0;
		l.align_right = 0;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 5;
		l.padding[2] = 5;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 3;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2172173380;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_client:model_settings");
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
		l.gap[1] = 3;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2357176526;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "cmd_list");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 460;
		l.offset[1] = 92;
		l.has_size = 1;
		l.size[0] = 251;
		l.size[1] = 479;
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
		l.id = 2420476340;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "model_setting+r|R");
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
		l.padding[3] = 0;
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
		l.id = 2651403700;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "show_prog_state|Show program state");
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
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2751042306;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "node_list_item+4|ModelEntity id 4");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 0;
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
		l.padding[3] = 1;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2821238409;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_treenode:node_list_item");
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
		l.padding[0] = 0;
		l.padding[1] = 0;
		l.padding[2] = 30;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 2949743699;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "nodegroupdef_list|NodeGroupDef list");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 846;
		l.offset[1] = 68;
		l.has_size = 1;
		l.size[0] = 148;
		l.size[1] = 591;
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
		l.id = 3124663356;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "panel");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 0;
		l.offset[1] = 642;
		l.has_size = 1;
		l.size[0] = 1016;
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
		l.id = 3134070952;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_treenode:create_cmd_list_item");
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
		l.padding[0] = 5;
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
		l.id = 3309950839;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "node_list");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 472;
		l.offset[1] = 45;
		l.has_size = 1;
		l.size[0] = 372;
		l.size[1] = 665;
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
		l.padding[0] = 5;
		l.padding[1] = 0;
		l.padding[2] = 5;
		l.padding[3] = 0;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3517619157;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "world_tool_elem+prog|Show program state");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 100;
		l.size[1] = 25;
		l.prevent_resizing = 0;
		l.align_left = 1;
		l.align_right = 1;
		l.align_top = 0;
		l.align_bottom = 0;
		l.padding[0] = 5;
		l.padding[1] = 0;
		l.padding[2] = 5;
		l.padding[3] = 2;
		l.gap[0] = 0;
		l.gap[1] = 0;
		append_element_layout(ctx, l);
	}

	{
		GuiElementLayout l = {0};
		l.id = 3576810726;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "vertex_attributes");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 777;
		l.offset[1] = 372;
		l.has_size = 1;
		l.size[0] = 239;
		l.size[1] = 369;
		l.prevent_resizing = 0;
		l.align_left = 0;
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
		l.id = 3769276755;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "create_cmd_list_item+label|Selected nodes");
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
		l.padding[3] = 5;
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
		l.id = 3880832514;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "program_state|Program state");
		l.on_same_row = 0;
		l.has_offset = 1;
		l.offset[0] = 624;
		l.offset[1] = 21;
		l.has_size = 1;
		l.size[0] = 379;
		l.size[1] = 632;
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
		l.offset[0] = 361;
		l.offset[1] = 185;
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
		l.id = 4047493188;
		GUI_FMT_STR(l.str, sizeof(l.str), "%s", "gui_treenode:datatree");
		l.on_same_row = 0;
		l.has_offset = 0;
		l.offset[0] = 0;
		l.offset[1] = 0;
		l.has_size = 1;
		l.size[0] = 500;
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
