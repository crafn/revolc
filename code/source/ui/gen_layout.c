void load_layout(GuiContext *ctx)
{
	gui_update_layout_property(ctx, "gui_button", "padding_bottom", 4);
	gui_update_layout_property(ctx, "gui_button", "padding_right", 5);
	gui_update_layout_property(ctx, "gui_button", "padding_left", 5);
	gui_update_layout_property(ctx, "gui_button", "padding_top", 2);

	gui_update_layout_property(ctx, "gui_textfield", "size_y", 22);
	gui_update_layout_property(ctx, "gui_textfield", "gap_x", 4);

	gui_update_layout_property(ctx, "gui_bar", "size_y", 25);

	gui_update_layout_property(ctx, "gui_checkbox", "size_x", 22);
	gui_update_layout_property(ctx, "gui_checkbox", "size_y", 22);
	gui_update_layout_property(ctx, "gui_checkbox", "gap_x", 4);

	gui_update_layout_property(ctx, "datatree", "align_right", 1);
	gui_update_layout_property(ctx, "datatree", "align_left", 1);

	gui_update_layout_property(ctx, "world_node_list_item", "align_right", 1);
	gui_update_layout_property(ctx, "world_node_list_item", "align_left", 1);

	gui_update_layout_property(ctx, "model_settings", "size_x", 243);
	gui_update_layout_property(ctx, "model_settings", "size_y", 492);
	gui_update_layout_property(ctx, "model_settings", "offset_x", 0);
	gui_update_layout_property(ctx, "model_settings", "offset_y", 249);
	gui_update_layout_property(ctx, "model_settings", "align_bottom", 1);
	gui_update_layout_property(ctx, "model_settings", "prevent_resizing", 1);
	gui_update_layout_property(ctx, "model_settings", "align_left", 1);

	gui_update_layout_property(ctx, "win", "size_x", 133);
	gui_update_layout_property(ctx, "win", "size_y", 479);
	gui_update_layout_property(ctx, "win", "offset_x", 120);
	gui_update_layout_property(ctx, "win", "offset_y", 151);

	gui_update_layout_property(ctx, "create_cmd", "size_x", 566);
	gui_update_layout_property(ctx, "create_cmd", "size_y", 363);
	gui_update_layout_property(ctx, "create_cmd", "offset_x", 572);
	gui_update_layout_property(ctx, "create_cmd", "offset_y", 105);

	gui_update_layout_property(ctx, "gui_layout_list", "align_right", 1);
	gui_update_layout_property(ctx, "gui_layout_list", "align_left", 1);
	gui_update_layout_property(ctx, "gui_layout_list", "gap_x", 3);

	gui_update_layout_property(ctx, "world_tools", "size_x", 220);
	gui_update_layout_property(ctx, "world_tools", "size_y", 741);
	gui_update_layout_property(ctx, "world_tools", "offset_x", 0);
	gui_update_layout_property(ctx, "world_tools", "offset_y", 0);
	gui_update_layout_property(ctx, "world_tools", "align_bottom", 1);
	gui_update_layout_property(ctx, "world_tools", "prevent_resizing", 1);
	gui_update_layout_property(ctx, "world_tools", "align_top", 1);
	gui_update_layout_property(ctx, "world_tools", "align_left", 0);

	gui_update_layout_property(ctx, "gui_layoutwin", "size_x", 400);
	gui_update_layout_property(ctx, "gui_layoutwin", "size_y", 700);
	gui_update_layout_property(ctx, "gui_layoutwin", "offset_x", 607);
	gui_update_layout_property(ctx, "gui_layoutwin", "offset_y", 21);

	gui_update_layout_property(ctx, "gui_window", "size_x", 100);
	gui_update_layout_property(ctx, "gui_window", "size_y", 100);

	gui_update_layout_property(ctx, "outline_uvbox_box", "align_right", 1);
	gui_update_layout_property(ctx, "outline_uvbox_box", "size_x", 100);
	gui_update_layout_property(ctx, "outline_uvbox_box", "size_y", 100);
	gui_update_layout_property(ctx, "outline_uvbox_box", "align_bottom", 1);

	gui_update_layout_property(ctx, "gui_treenode", "padding_left", 20);
	gui_update_layout_property(ctx, "gui_treenode", "gap_y", 2);

	gui_update_layout_property(ctx, "cmd_list", "size_x", 378);
	gui_update_layout_property(ctx, "cmd_list", "size_y", 546);
	gui_update_layout_property(ctx, "cmd_list", "offset_x", 640);
	gui_update_layout_property(ctx, "cmd_list", "offset_y", 102);

	gui_update_layout_property(ctx, "clip_button", "on_same_row", 1);

	gui_update_layout_property(ctx, "model_setting", "align_right", 1);
	gui_update_layout_property(ctx, "model_setting", "align_left", 1);

	gui_update_layout_property(ctx, "gui_layout_list_prop", "size_x", 300);
	gui_update_layout_property(ctx, "gui_layout_list_prop", "on_same_row", 1);

	gui_update_layout_property(ctx, "node_list_item", "align_right", 1);
	gui_update_layout_property(ctx, "node_list_item", "align_left", 1);

	gui_update_layout_property(ctx, "editor_overlay_box", "align_right", 1);
	gui_update_layout_property(ctx, "editor_overlay_box", "align_bottom", 1);
	gui_update_layout_property(ctx, "editor_overlay_box", "align_top", 1);
	gui_update_layout_property(ctx, "editor_overlay_box", "align_left", 1);

	gui_update_layout_property(ctx, "gui_client", "padding_bottom", 5);
	gui_update_layout_property(ctx, "gui_client", "align_right", 1);
	gui_update_layout_property(ctx, "gui_client", "padding_right", 10);
	gui_update_layout_property(ctx, "gui_client", "offset_x", 0);
	gui_update_layout_property(ctx, "gui_client", "offset_y", 0);
	gui_update_layout_property(ctx, "gui_client", "align_left", 1);
	gui_update_layout_property(ctx, "gui_client", "padding_left", 10);
	gui_update_layout_property(ctx, "gui_client", "padding_top", 5);
	gui_update_layout_property(ctx, "gui_client", "gap_x", 0);
	gui_update_layout_property(ctx, "gui_client", "gap_y", 4);

	gui_update_layout_property(ctx, "nodegroupdef_list", "size_x", 173);
	gui_update_layout_property(ctx, "nodegroupdef_list", "size_y", 381);
	gui_update_layout_property(ctx, "nodegroupdef_list", "offset_x", 838);
	gui_update_layout_property(ctx, "nodegroupdef_list", "offset_y", 54);

	gui_update_layout_property(ctx, "panel", "size_x", 100);
	gui_update_layout_property(ctx, "panel", "size_y", 741);
	gui_update_layout_property(ctx, "panel", "offset_x", 0);
	gui_update_layout_property(ctx, "panel", "offset_y", 0);
	gui_update_layout_property(ctx, "panel", "align_bottom", 1);
	gui_update_layout_property(ctx, "panel", "prevent_resizing", 1);
	gui_update_layout_property(ctx, "panel", "align_top", 1);
	gui_update_layout_property(ctx, "panel", "align_left", 1);

	gui_update_layout_property(ctx, "node_list", "size_x", 687);
	gui_update_layout_property(ctx, "node_list", "size_y", 671);
	gui_update_layout_property(ctx, "node_list", "offset_x", 227);
	gui_update_layout_property(ctx, "node_list", "offset_y", 9);

	gui_update_layout_property(ctx, "timeline", "align_right", 1);
	gui_update_layout_property(ctx, "timeline", "size_y", 150);
	gui_update_layout_property(ctx, "timeline", "align_bottom", 1);
	gui_update_layout_property(ctx, "timeline", "align_left", 1);
	gui_update_layout_property(ctx, "timeline", "padding_left", 10);
	gui_update_layout_property(ctx, "timeline", "gap_x", 2);

	gui_update_layout_property(ctx, "nodegroupdef_list_item", "align_right", 1);
	gui_update_layout_property(ctx, "nodegroupdef_list_item", "align_left", 1);

	gui_update_layout_property(ctx, "Slider", "align_right", 1);
	gui_update_layout_property(ctx, "Slider", "align_left", 1);

	gui_update_layout_property(ctx, "world_tool_elem", "align_right", 1);
	gui_update_layout_property(ctx, "world_tool_elem", "align_left", 1);

	gui_update_layout_property(ctx, "res_tools", "size_x", 114);
	gui_update_layout_property(ctx, "res_tools", "size_y", 94);
	gui_update_layout_property(ctx, "res_tools", "offset_x", 0);
	gui_update_layout_property(ctx, "res_tools", "offset_y", 0);
	gui_update_layout_property(ctx, "res_tools", "prevent_resizing", 1);

	gui_update_layout_property(ctx, "gui_bg_window", "align_right", 1);
	gui_update_layout_property(ctx, "gui_bg_window", "align_bottom", 1);
	gui_update_layout_property(ctx, "gui_bg_window", "align_top", 1);
	gui_update_layout_property(ctx, "gui_bg_window", "align_left", 1);

	gui_update_layout_property(ctx, "Textfield", "align_right", 1);
	gui_update_layout_property(ctx, "Textfield", "align_left", 1);

	gui_update_layout_property(ctx, "gui_slider", "size_x", 15);
	gui_update_layout_property(ctx, "gui_slider", "size_y", 15);
	gui_update_layout_property(ctx, "gui_slider", "gap_x", 4);
	gui_update_layout_property(ctx, "gui_slider", "gap_y", 4);

	gui_update_layout_property(ctx, "program_state", "size_x", 417);
	gui_update_layout_property(ctx, "program_state", "size_y", 670);
	gui_update_layout_property(ctx, "program_state", "offset_x", 330);
	gui_update_layout_property(ctx, "program_state", "offset_y", 61);

	gui_update_layout_property(ctx, "clip_timeline", "align_right", 1);
	gui_update_layout_property(ctx, "clip_timeline", "size_y", 130);
	gui_update_layout_property(ctx, "clip_timeline", "align_bottom", 1);
	gui_update_layout_property(ctx, "clip_timeline", "align_left", 1);

	gui_update_layout_property(ctx, "Gui components", "size_x", 280);
	gui_update_layout_property(ctx, "Gui components", "size_y", 463);
	gui_update_layout_property(ctx, "Gui components", "offset_x", 277);
	gui_update_layout_property(ctx, "Gui components", "offset_y", 84);

	gui_update_layout_property(ctx, "uvbox_box", "align_right", 1);
	gui_update_layout_property(ctx, "uvbox_box", "size_x", 300);
	gui_update_layout_property(ctx, "uvbox_box", "size_y", 300);
	gui_update_layout_property(ctx, "uvbox_box", "align_top", 1);

	gui_update_layout_property(ctx, "cmd_list_item", "align_right", 1);
	gui_update_layout_property(ctx, "cmd_list_item", "align_left", 1);
}
