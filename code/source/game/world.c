#include "core/basic.h"
#include "core/memory.h"
#include "core/math.h"
#include "game/world.h"
#include "global/env.h"

typedef struct SaveHeader {
	U16 version;
	U32 node_count;
	U32 cmd_count;
	bool delta;

	// For non-delta
	Id next_entity_id;
	Id next_node_id;
	Id next_cmd_id;
} PACKED SaveHeader;

typedef struct DeadCmd {
	CmdType type;
	Id cmd_id;

	bool has_condition;
	Id cond_node_id;
	U16 cond_offset;
	U16 cond_size;

	union {
		struct { // memcpy
			U16 src_offset;
			U16 dst_offset;
			U16 size;
			Id src_node_id;
			Id dst_node_id;
		};
		struct { // call
			char func_name[MAX_FUNC_NAME_SIZE];

			Id p_node_ids[MAX_CMD_CALL_PARAMS];
			U16 p_node_count;
		};
	};

	// Used on delta
	bool created;
	bool destroyed;
} PACKED DeadCmd;

typedef struct DeadNode {
	char type_name[RES_NAME_SIZE];
	char group_def_name[RES_NAME_SIZE];
	Id node_id;
	Id group_id;
	U8 peer_id;
	U8 node_ix_in_group;

	void *packed_impl; // frame-allocated
	U32 packed_impl_size;

	// Used on delta
	bool destroyed;
} PACKED DeadNode;

internal void make_deadnode(DeadNode *dead_node, World *w, NodeInfo *node);
internal void unpack_deadnode_impl(void **dead_impl, const DeadNode *dead_node);
internal void resurrect_deadnode_impl(World *w, U32 handle, const DeadNode *dead_node);
internal void resurrect_deadnode(World *w, const DeadNode *dead_node);
internal void save_deadnode(WArchive *ar, const DeadNode *n);
internal void load_deadnode(RArchive *ar, DeadNode *dead_node);

internal void make_deadcmd(DeadCmd *dead_cmd, World *w, NodeCmd *cmd);
internal void resurrect_deadcmd(World *w, const DeadCmd *dead_cmd);
internal void save_deadcmd(WArchive *ar, const DeadCmd *dead_cmd);
internal void load_deadcmd(RArchive *ar, DeadCmd *dead_cmd);

internal void overwrite_node_impl(World *w, Handle node_h, const DeadNode *dead);
internal void free_node_impl(World *w, U32 handle);

internal void add_node_assoc_cmd(World *w, Handle node_h, Handle cmd_h)
{
	ensure(node_h != NULL_HANDLE);
	ensure(cmd_h != NULL_HANDLE);
	NodeInfo *node = &w->nodes[node_h];
	for (U32 i = 0; i < MAX_NODE_ASSOC_CMD_COUNT; ++i) {
		if (node->assoc_cmds[i] == NULL_HANDLE) {
			node->assoc_cmds[i] = cmd_h;
			return;
		}
	}
	fail("Node is full of cmds");
}

internal void remove_node_assoc_cmd(World *w, Handle node_h, Handle cmd_h)
{
	ensure(node_h != NULL_HANDLE);
	ensure(cmd_h != NULL_HANDLE);
	NodeInfo *node = &w->nodes[node_h];
	for (U32 i = 0; i < MAX_NODE_ASSOC_CMD_COUNT; ++i) {
		if (node->assoc_cmds[i] == cmd_h) {
			node->assoc_cmds[i] = NULL_HANDLE;
			break;
		}
	}
}

World * create_world()
{
	World *w = ZERO_ALLOC(gen_ator(), sizeof(*w), "world");

	// Initialize storage for automatically allocated node impls

	U32 ntypes_count;
	NodeType **ntypes =
		(NodeType **)all_res_by_type(	&ntypes_count,
										g_env.resblob,
										ResType_NodeType);

	U32 ntype_count_with_auto_mgmt = 0;
	for (U32 i = 0; i < ntypes_count; ++i) {
		const NodeType *type = ntypes[i];
		if (type->auto_impl_mgmt)
			++ntype_count_with_auto_mgmt;
	}

	w->auto_storages =
		ZERO_ALLOC(	gen_ator(),
						(sizeof(*w->auto_storages)*ntype_count_with_auto_mgmt),
						"auto_storage");
	w->auto_storage_count = ntype_count_with_auto_mgmt;

	U32 st_i = 0;
	for (U32 i = 0; i < ntypes_count; ++i) {
		NodeType *type = ntypes[i];
		if (!type->auto_impl_mgmt)
			continue;

		AutoNodeImplStorage *st = &w->auto_storages[st_i];
		st->storage = ZERO_ALLOC(gen_ator(),
								(type->size*type->max_count),
								"auto_storage.storage");
		st->allocated =
			ZERO_ALLOC(	gen_ator(),
						sizeof(*st->allocated)*type->max_count,
						"auto_storage.allocated");
		st->size = type->size;
		st->max_count = type->max_count;

		// Cache handle to storage in NodeType itself for fastness
		type->auto_storage_handle = st_i++;
	}

	w->node_id_to_handle = create_tbl(Id, Handle)(NULL_ID, NULL_HANDLE, gen_ator(), MAX_NODE_COUNT);
	w->cmd_id_to_handle = create_tbl(Id, Handle)(NULL_ID, NULL_HANDLE, gen_ator(), MAX_NODE_CMD_COUNT);

	if (!g_env.netstate || g_env.netstate->authority) { // Builtin engine nodes
		SlotVal init_vals[] = {};
		NodeGroupDef *def =
			(NodeGroupDef*)res_by_name(	g_env.resblob,
										ResType_NodeGroupDef,
										"builtin");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), 0, AUTHORITY_PEER);
	}

	return w;
}

void destroy_world(World *w)
{
	debug_print("destroy_world: %i nodes", w->node_count);

	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		if (w->nodes[i].allocated)
			free_node(w, i);
	}
	for (U32 i = 0; i < w->auto_storage_count; ++i) {
		FREE(gen_ator(), w->auto_storages[i].storage);
		FREE(gen_ator(), w->auto_storages[i].allocated);
	}
	FREE(gen_ator(), w->auto_storages);

	destroy_tbl(Id, Handle)(&w->node_id_to_handle);
	destroy_tbl(Id, Handle)(&w->cmd_id_to_handle);
	FREE(gen_ator(), w);
}

void clear_world_nodes(World *w)
{
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		if (w->nodes[i].allocated)
			free_node(w, i);
	}
}

internal
int node_cmp(const void * a_, const void * b_)
{
	const NodeInfo *a = a_, *b = b_;
	// Sort for batching
	// Non-allocated should be at the end
	/// @todo Prioritize by cmds, but still clumping by type
	int alloc_cmp = b->allocated - a->allocated;
	if (alloc_cmp) {
		return alloc_cmp;
	} else { 
		int str_cmp = strcmp(a->type_name, b->type_name);
		if (str_cmp)
			return str_cmp;
		else
			return	(a->impl_handle < b->impl_handle) -
					(a->impl_handle > b->impl_handle);
	}
}
 
void upd_world(World *w, F64 dt)
{
	w->dt = dt;

	memcpy(w->sort_space, w->nodes, sizeof(w->nodes));
	/// @todo	Optimize sorting -- this qsort causes a major fps drop.
	///			Maybe don't sort everything again every frame?
	///			Maybe calculate priorities beforehand for fast cmp?
	///			See clover/code/source/nodes/updateline.cpp for priorization
	//qsort(w->sort_space, MAX_NODE_COUNT, sizeof(*w->nodes), node_cmp);

	U32 node_i = 0;
	U32 updated_count = 0;
	U32 batch_count = 0;
	U32 signal_count = 0;
	while (node_i < MAX_NODE_COUNT) {
		NodeInfo *node = &w->sort_space[node_i];
		if (!node->allocated) {
			++node_i;
			continue;
		}

		const U32 batch_begin_i = node_i;
		const U32 batch_begin_impl_handle = node->impl_handle;
		const NodeType *batch_begin_type = node->type;

		while (	node_i < MAX_NODE_COUNT &&
				node->type == batch_begin_type &&
				node->impl_handle ==	batch_begin_impl_handle +
										batch_begin_i - node_i) {
			ensure(node->allocated);
			++node_i;
			++node;
		}

		// Update batch
		const U32 batch_size = node_i - batch_begin_i;
		updated_count += batch_size; 
		if (batch_begin_type->upd) {
			U8 *it = node_impl(w, NULL, &w->sort_space[batch_begin_i]);
			U32 size = batch_begin_type->size;
			U8 *end = it + batch_size*size;
			while (it < end) {
				batch_begin_type->upd(it);
				it += size;
			}
		}
		++batch_count;
	}

	// @todo update-function should be a command. Batching/sorting happends then at command level.
	// Perform commands stated in NodeGroupDefs
	for (U32 i = 0; i < MAX_NODE_CMD_COUNT; ++i) {
		NodeCmd cmd = w->cmds[i];
		if (!cmd.allocated)
			continue;
		if (cmd.has_condition) {
			NodeInfo *c_node = &w->nodes[cmd.cond_node_h];
			U8 *cond_bytes = node_impl(w, NULL, c_node) + cmd.cond_offset;
			bool cond_fullfilled = false;
			for (U32 k = 0; k < cmd.cond_size; ++k) {
				if (cond_bytes[i]) {
					cond_fullfilled = true;
					break;
				}
			}

			if (!cond_fullfilled)
				continue; // Skip command
		}

		switch (cmd.type) {
		case CmdType_memcpy: {
			if (w->editor_disable_memcpy_cmds)
				break; // Allow editing of individual struct members without overwriting
			ensure(cmd.memcpy.src_node < MAX_NODE_COUNT);
			NodeInfo *src_node = &w->nodes[cmd.memcpy.src_node];
			NodeInfo *dst_node = &w->nodes[cmd.memcpy.dst_node];
			ensure(dst_node->allocated && src_node->allocated);

			U8 *dst = (U8*)node_impl(w, NULL, dst_node) + cmd.memcpy.dst_offset;
			U8 *src = (U8*)node_impl(w, NULL, src_node) + cmd.memcpy.src_offset;
			for (U32 i = 0; i < cmd.memcpy.size; ++i) 
				dst[i] = src[i];
		} break;
		case CmdType_call: {
			// This is ugly. Call function pointer with corresponding node parameters
			switch (cmd.call.p_node_count) {
			case 0:
				((void (*)())cmd.call.fptr)();
			break;
			case 1: {
				void * p1_impl = node_impl(w, NULL, &w->nodes[cmd.call.p_nodes[0]]);
				((void (*)(void *))cmd.call.fptr)(
					p1_impl);
			} break;
			case 2: {
				void * p1_impl = node_impl(w, NULL, &w->nodes[cmd.call.p_nodes[0]]);
				void * p2_impl = node_impl(w, NULL, &w->nodes[cmd.call.p_nodes[1]]);
				((void (*)(void *, void *))cmd.call.fptr)(
					p1_impl, p2_impl);
			} break;
			case 3: {
				void * p1_impl = node_impl(w, NULL, &w->nodes[cmd.call.p_nodes[0]]);
				void * p2_impl = node_impl(w, NULL, &w->nodes[cmd.call.p_nodes[1]]);
				void * p3_impl = node_impl(w, NULL, &w->nodes[cmd.call.p_nodes[2]]);
				((void (*)(void *, void *, void *))cmd.call.fptr)(
					p1_impl,  p2_impl, p3_impl);
			} break;
			default: fail("Too many node params");
			}
		} break;
		default: fail("Unknown cmd type: %i", cmd.type);
		}
		++signal_count;
	}

	//debug_print("upd signal count: %i", signal_count);
	//debug_print("upd batch count: %i", batch_count);

	// Free remove-flagged nodes
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		if (w->nodes[i].allocated && w->nodes[i].remove)
			free_node(w, i);
	}
}


void save_world(WArchive *ar, World *w)
{
	SaveHeader header = {
		.node_count = 0, // Patch afterwards
		.cmd_count = 0,
		.delta = false,
		.next_entity_id = w->next_entity_id,
		.next_node_id = w->next_node_id,
		.next_cmd_id = w->next_cmd_id,
	};

	U32 header_offset = ar->data_size;
	pack_buf(ar, &header, sizeof(header));

	for (U32 node_i = 0; node_i < MAX_NODE_COUNT; ++node_i) {
		NodeInfo *node = &w->nodes[node_i];
		if (!node->allocated)
			continue;

		DeadNode dead_node;
		make_deadnode(&dead_node, w, node);
		save_deadnode(ar, &dead_node);
		++header.node_count;
	}

	for (U32 cmd_i = 0; cmd_i < MAX_NODE_CMD_COUNT; ++cmd_i) {
		NodeCmd *cmd = &w->cmds[cmd_i];
		if (!cmd->allocated)
			continue;

		DeadCmd dead_cmd;
		make_deadcmd(&dead_cmd, w, cmd);
		save_deadcmd(ar, &dead_cmd);
		++header.cmd_count;
	}

	pack_buf_patch(ar, header_offset, &header, sizeof(header));

	//debug_print("save_world: nodes: %i, cmds: %i", header.node_count, header.cmd_count);
}

void load_world(RArchive *ar, World *w)
{
	//debug_print("load_world");
	if (w->node_count > 0) {
		//debug_print("load_world: cleaning current world");
		clear_world_nodes(w);
	}
	ensure(w->node_count == 0);

	SaveHeader header;
	unpack_buf(ar, &header, sizeof(header));
	w->next_entity_id = header.next_entity_id;
	w->next_node_id = header.next_node_id;
	w->next_cmd_id = header.next_cmd_id;

	U32 node_count = 0;
	for (U32 node_i = 0; node_i < header.node_count;
			++node_i, w->next_node = (w->next_node + 1) % MAX_NODE_COUNT) {
		DeadNode dead_node;
		load_deadnode(ar, &dead_node);
		resurrect_deadnode(w, &dead_node);
		++node_count;
	}
	ensure(node_count == w->node_count);

	U32 cmd_count = 0;
	for (U32 cmd_i = 0; cmd_i < header.cmd_count;
			++cmd_i, w->next_cmd = (w->next_cmd + 1) % MAX_NODE_CMD_COUNT) {
		DeadCmd dead_cmd;
		load_deadcmd(ar, &dead_cmd);
		resurrect_deadcmd(w, &dead_cmd);
		++cmd_count;
	}
	ensure(cmd_count == w->cmd_count);
}

void save_world_to_file(World *w, const char *path)
{
	WArchive measure = create_warchive(ArchiveType_measure, NULL, 0);
	save_world(&measure, w);

	WArchive ar = create_warchive(ArchiveType_binary, frame_ator(), measure.data_size);
	save_world(&ar, w);

	FILE *file = fopen(path, "wb");
	file_write(file, ar.data, ar.data_size);
	fclose(file);

	destroy_warchive(&ar);
	destroy_warchive(&measure);
}

void load_world_from_file(World *w, const char *path)
{
	U32 size;
	void *data = read_file(frame_ator(), path, &size);

	RArchive ar = create_rarchive(ArchiveType_binary, data, size);
	load_world(&ar, w);
	destroy_rarchive(&ar);
}

void save_world_delta(WArchive *ar, World *w, RArchive *base_ar)
{
	SaveHeader base_header;
	unpack_buf(base_ar, &base_header, sizeof(base_header));
	ensure(!base_header.delta);

	SaveHeader header = { .delta = true };
	U32 header_offset = ar->data_size;
	pack_buf(ar, &header, sizeof(header)); // Patch later

	//
	// Nodes
	//

	// Used to detect which nodes have been created
	Id *processed_nodes = ALLOC(	frame_ator(),
									sizeof(*processed_nodes)*MAX_NODE_COUNT,
									"processed_nodes");
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i)
		processed_nodes[i] = !w->nodes[i].allocated;

	// Go through base state and write deleted/modified nodes
	for (U32 node_i = 0; node_i < base_header.node_count; ++node_i) {
		// Read node from base state
		DeadNode dead_base;
		load_deadnode(base_ar, &dead_base);

		// Find node from world
		Handle node_h = node_id_to_handle(w, dead_base.node_id);

		if (node_h == NULL_HANDLE) {
			// Node destroyed
			DeadNode dead_node = dead_base;
			dead_node.destroyed = true;
			save_deadnode(ar, &dead_node);
		} else {
			processed_nodes[node_h] = true;
			NodeInfo delta_node = w->nodes[node_h];

			DeadNode dead_delta;
			make_deadnode(&dead_delta, w, &delta_node);

			// Check if dead node has different packed data.
			// @todo Allow ensure below in future when not a bug
			ensure(dead_delta.packed_impl_size == dead_base.packed_impl_size); 
			if (!memcmp(dead_delta.packed_impl,
						dead_base.packed_impl,
						dead_base.packed_impl_size)) {
				continue; // Identical
			} else {
				//debug_print("delta differ: %s", node_h == NULL_HANDLE ? "null" : w->nodes[node_h].type->res.name);
			}

			save_deadnode(ar, &dead_delta);
		}

		++header.node_count;
	}

	// Go through remaining handles (which are the created nodes)
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		if (processed_nodes[i])
			continue;

		NodeInfo *created_node = &w->nodes[i];

		DeadNode dead;
		make_deadnode(&dead, w, created_node);
		save_deadnode(ar, &dead);
		//debug_print("delta created node: %s", w->nodes[i].type->res.name);
		++header.node_count;
	}

	//
	// Cmds
	//


	// Used to detect which cmds have been created
	Id *processed_cmds = ALLOC(	frame_ator(),
								sizeof(*processed_cmds)*MAX_NODE_CMD_COUNT,
								"processed_cmds");
	for (U32 i = 0; i < MAX_NODE_CMD_COUNT; ++i)
		processed_cmds[i] = !w->cmds[i].allocated;

	// Go through base state and write deleted cmds
	for (U32 cmd_i = 0; cmd_i < base_header.cmd_count; ++cmd_i) {
		DeadCmd dead_base;
		load_deadcmd(base_ar, &dead_base);

		Handle cmd_h = cmd_id_to_handle(w, dead_base.cmd_id);
		if (cmd_h == NULL_HANDLE) {
			// Cmd destroyed
			DeadCmd dead_cmd = dead_base;
			dead_cmd.destroyed = true;
			save_deadcmd(ar, &dead_cmd);
			++header.cmd_count;
			debug_print("cmd destroyed");
		} else {
			// Cmd still alive
			processed_cmds[cmd_h] = true;
		}
	}

	// Go through remaining handles (which are the created cmds)
	for (U32 i = 0; i < MAX_NODE_CMD_COUNT; ++i) {
		if (processed_cmds[i])
			continue;

		NodeCmd *created_cmd = &w->cmds[i];

		DeadCmd dead;
		make_deadcmd(&dead, w, created_cmd);
		dead.created = true;
		save_deadcmd(ar, &dead);
		++header.cmd_count;
	}

	pack_buf_patch(ar, header_offset, &header, sizeof(header));

	debug_print("delta nodes: %i, cmds: %i", header.node_count, header.cmd_count);
}

void load_world_delta(RArchive *ar, World *w, RArchive *base_ar, U8 ignore_peer_id)
{
	SaveHeader base_header;
	unpack_buf(base_ar, &base_header, sizeof(base_header));
	ensure(base_header.delta == false);

	SaveHeader delta_header;
	unpack_buf(ar, &delta_header, sizeof(delta_header));
	ensure(delta_header.delta == true);

	// Merge base and delta state

	{ // Nodes
		U32 deadnode_max_count = base_header.node_count + delta_header.node_count;
		DeadNode *deadnodes = ALLOC(frame_ator(), sizeof(*deadnodes)*deadnode_max_count, "dead_nodes");
		U32 deadnode_count = 0;
		HashTbl(Id, Handle) id_to_deadnode_ix =
			create_tbl(Id, Handle)(NULL_ID, NULL_HANDLE, frame_ator(), delta_header.node_count*2 + 1);

		// Go through delta
		for (U32 i = 0; i < delta_header.node_count; ++i) {
			DeadNode dead_delta;
			load_deadnode(ar, &dead_delta);
			set_tbl(Id, Handle)(&id_to_deadnode_ix, dead_delta.node_id, deadnode_count);
			deadnodes[deadnode_count++] = dead_delta;
		}

		// Go through base and add nodes which were not in delta
		for (U32 i = 0; i < base_header.node_count; ++i) {
			DeadNode dead_base;
			load_deadnode(base_ar, &dead_base);
			
			Handle ix = get_tbl(Id, Handle)(&id_to_deadnode_ix, dead_base.node_id);
			if (ix != NULL_HANDLE)
				continue;
			deadnodes[deadnode_count++] = dead_base;
		}
		destroy_tbl(Id, Handle)(&id_to_deadnode_ix);

		// Go through current world and apply deadnodes
		for (U32 i = 0; i < deadnode_count; ++i) {
			DeadNode dead = deadnodes[i];
			if (dead.peer_id == ignore_peer_id)
				continue;

			// Find node from world
			Handle node_h = node_id_to_handle(w, dead.node_id);
			if (node_h == NULL_HANDLE) {
				// Created
				if (!dead.destroyed)
					resurrect_deadnode(w, &dead);
			} else if (dead.destroyed) {
				// Destroyed
				free_node(w, node_h);
			} else {
				// Updated
				overwrite_node_impl(w, node_h, &dead);
			}
		}
	}

	{ // Cmds
		U32 deadcmd_max_count = base_header.node_count + delta_header.node_count;
		DeadCmd *deadcmds = ALLOC(frame_ator(), sizeof(*deadcmds)*deadcmd_max_count, "dead_nodes");
		U32 deadcmd_count = 0;
		HashTbl(Id, Handle) id_to_deadcmd_ix =
			create_tbl(Id, Handle)(NULL_ID, NULL_HANDLE, frame_ator(), delta_header.cmd_count*2 + 1);

		// Go through delta
		for (U32 i = 0; i < delta_header.cmd_count; ++i) {
			DeadCmd dead_delta;
			load_deadcmd(ar, &dead_delta);
			set_tbl(Id, Handle)(&id_to_deadcmd_ix, dead_delta.cmd_id, deadcmd_count);
			deadcmds[deadcmd_count++] = dead_delta;
		}

		// Go through base and add cmds which were not in delta
		for (U32 i = 0; i < base_header.cmd_count; ++i) {
			DeadCmd dead_base;
			load_deadcmd(base_ar, &dead_base);
			
			Handle ix = get_tbl(Id, Handle)(&id_to_deadcmd_ix, dead_base.cmd_id);
			if (ix != NULL_HANDLE)
				continue;
			deadcmds[deadcmd_count++] = dead_base;
		}
		destroy_tbl(Id, Handle)(&id_to_deadcmd_ix);

		// Go through current world and apply deadcmds
		for (U32 i = 0; i < deadcmd_count; ++i) {
			DeadCmd dead = deadcmds[i];

			if (dead.created) {
				resurrect_deadcmd(w, &dead);
			} else if (dead.destroyed) {
				Handle cmd_h = cmd_id_to_handle(w, dead.cmd_id);
				if (cmd_h != NULL_HANDLE) // Cmd can be destroyed with node, so NULL_HANDLE is ok
					free_cmd(w, cmd_h);
			} else {
				// Maintain cmd (do nothing)
			}
		}
	}
}

void save_single_node(WArchive *ar, World *w, Handle handle)
{
	ensure(handle != NULL_HANDLE);

	DeadNode dead;
	make_deadnode(&dead, w, &w->nodes[handle]);
	save_deadnode(ar, &dead);
}

void load_single_node(RArchive *ar, World *w)
{
	DeadNode dead;
	load_deadnode(ar, &dead);
	Handle h = node_id_to_handle(w, dead.node_id);
	if (h != NULL_HANDLE) {
		overwrite_node_impl(w, h, &dead);
	} else {
		critical_print("load_single_node: node %i not found", dead.node_id);
	}
}

void make_deadnode(DeadNode *dead_node, World *w, NodeInfo *node)
{
	*dead_node = (DeadNode) {
		.node_id = node->node_id,
		.group_id = node->group_id,
		.peer_id = node->peer_id,
		.node_ix_in_group = node->node_ix_in_group,
	};
	fmt_str(dead_node->type_name, sizeof(dead_node->type_name), "%s", node->type_name);
	fmt_str(dead_node->group_def_name, sizeof(dead_node->group_def_name), "%s", node->group_def_name);

	NodeType *node_type = (NodeType*)res_by_name(
							g_env.resblob,
							ResType_NodeType,
							dead_node->type_name);
	if (node_type->packsync == PackSync_full) {
		void *impl = node_impl(w, NULL, node);
		// @todo Resizeable archive -- packed node might be larger than struct (ptrs)
		WArchive ar = create_warchive(ArchiveType_binary, frame_ator(), node_type->size);
		if (node_type->pack) {
			// @todo Multiple nodes
			node_type->pack(&ar, impl, (U8*)impl + node_type->size);
		} else {
			pack_buf(&ar, impl, node_type->size);
		}
		dead_node->packed_impl = ar.data;
		dead_node->packed_impl_size = ar.data_size;
		destroy_warchive(&ar); // Frame-allocator doesn't free. Relying on that.
	}
}

void unpack_deadnode_impl(void **dead_impl, const DeadNode *dead_node)
{
	NodeType *node_type = (NodeType*)res_by_name(
							g_env.resblob,
							ResType_NodeType,
							dead_node->type_name);

	if (node_type->packsync == PackSync_full) {
		*dead_impl = ALLOC(frame_ator(), node_type->size, "dead_impl");
		RArchive ar = create_rarchive(	ArchiveType_binary,
										dead_node->packed_impl, dead_node->packed_impl_size);
		if (node_type->unpack)
			node_type->unpack(&ar, *dead_impl, (U8*)*dead_impl + node_type->size);
		else
			unpack_buf(&ar, *dead_impl, node_type->size);
		destroy_rarchive(&ar);
	} else {
		*dead_impl = ZERO_ALLOC(frame_ator(), node_type->size, "dead_impl");
		// @todo Default values from node def and group def
		if (node_type->init)
			node_type->init(*dead_impl);
	}
}

void resurrect_deadnode_impl(World *w, U32 node_h, const DeadNode *dead_node)
{
	NodeInfo *n = &w->nodes[node_h];
	n->node_id = dead_node->node_id;
	n->group_id = dead_node->group_id;
	n->peer_id = dead_node->peer_id;

	//debug_print("resurrect_deadnode_impl %s, h %i, id %i", dead_node->type_name, node_h, n->node_id);

	void *dead_impl;
	unpack_deadnode_impl(&dead_impl, dead_node);
	resurrect_node_impl(w, n, dead_impl);
}

void resurrect_deadnode(World *w, const DeadNode *dead_node)
{
	U32 node_h = alloc_node_without_impl(
			w,
			(NodeType*)res_by_name(
				g_env.resblob,
				ResType_NodeType,
				dead_node->type_name),
			dead_node->node_id,
			dead_node->group_id,
			dead_node->peer_id,
			dead_node->group_def_name,
			dead_node->node_ix_in_group);

	resurrect_deadnode_impl(w, node_h, dead_node);
}

void save_deadnode(WArchive *ar, const DeadNode *dead_node)
{
	pack_buf(ar, WITH_DEREF_SIZEOF(dead_node));
	if (dead_node->destroyed)
		return;

	if (dead_node->packed_impl_size > 0)
		pack_buf(ar, dead_node->packed_impl, dead_node->packed_impl_size);
}

void load_deadnode(RArchive *ar, DeadNode *dead_node)
{
	unpack_buf(ar, WITH_DEREF_SIZEOF(dead_node));
	if (dead_node->destroyed)
		return;

	if (dead_node->packed_impl_size > 0) {
		// New Node implementation from binary
		dead_node->packed_impl =
			ALLOC(frame_ator(), dead_node->packed_impl_size, "packed_impl");
		unpack_buf(ar, dead_node->packed_impl, dead_node->packed_impl_size);
	}
}

void make_deadcmd(DeadCmd *dead_cmd, World *w, NodeCmd *cmd)
{
	*dead_cmd = (DeadCmd) {
		.type = cmd->type,
		.cmd_id = cmd->cmd_id,
	};

	if (cmd->has_condition) {
		dead_cmd->has_condition = true;
		dead_cmd->cond_node_id = node_handle_to_id(w, cmd->cond_node_h);
		dead_cmd->cond_offset = cmd->cond_offset;
		dead_cmd->cond_size = cmd->cond_size;
	}

	if (cmd->type == CmdType_memcpy)  {
		dead_cmd->src_node_id = node_handle_to_id(w, cmd->memcpy.src_node);
		dead_cmd->dst_node_id = node_handle_to_id(w, cmd->memcpy.dst_node);
		dead_cmd->src_offset = cmd->memcpy.src_offset;
		dead_cmd->dst_offset = cmd->memcpy.dst_offset;
		dead_cmd->size = cmd->memcpy.size;
	} else if (cmd->type == CmdType_call) {
		fmt_str(dead_cmd->func_name,
				sizeof(dead_cmd->func_name),
				"%s", rtti_sym_name(cmd->call.fptr));
		for (U32 k = 0; k < cmd->call.p_node_count; ++k)
			dead_cmd->p_node_ids[k] = node_handle_to_id(w, cmd->call.p_nodes[k]);
		dead_cmd->p_node_count = cmd->call.p_node_count;
	}
}

void resurrect_deadcmd(World *w, const DeadCmd *dead_cmd)
{
	NodeCmd cmd = {
		.type = dead_cmd->type,
		.cmd_id = dead_cmd->cmd_id,
	};

	cmd.has_condition = dead_cmd->has_condition;
	if (dead_cmd->has_condition) {
		cmd.cond_node_h = node_id_to_handle(w, dead_cmd->cond_node_id);
		cmd.cond_offset = dead_cmd->cond_offset;
		cmd.cond_size = dead_cmd->cond_size;
	}

	if (dead_cmd->type == CmdType_memcpy) {
		cmd.memcpy.src_node = node_id_to_handle(w, dead_cmd->src_node_id);
		cmd.memcpy.dst_node = node_id_to_handle(w, dead_cmd->dst_node_id);
		ensure(cmd.memcpy.src_node != NULL_HANDLE);
		ensure(cmd.memcpy.dst_node != NULL_HANDLE);
		ensure(w->nodes[cmd.memcpy.src_node].allocated);
		ensure(w->nodes[cmd.memcpy.dst_node].allocated);
		cmd.memcpy.src_offset = dead_cmd->src_offset;
		cmd.memcpy.dst_offset = dead_cmd->dst_offset;
		cmd.memcpy.size = dead_cmd->size;
	} else if (dead_cmd->type == CmdType_call) {
		cmd.call.fptr = rtti_func_ptr(dead_cmd->func_name);
		ensure(cmd.call.fptr != NULL);
		cmd.call.p_node_count = dead_cmd->p_node_count;
		for (U32 k = 0; k < dead_cmd->p_node_count; ++k) {
			cmd.call.p_nodes[k] = node_id_to_handle(w, dead_cmd->p_node_ids[k]);
			if (cmd.call.p_nodes[k] == NULL_HANDLE) {
				fail("Not found: id: %i", dead_cmd->p_node_ids[k]);
			}
		}
	}

	resurrect_cmd(w, cmd);
}

void save_deadcmd(WArchive *ar, const DeadCmd *dead_cmd)
{ pack_buf(ar, dead_cmd, sizeof(*dead_cmd)); }

void load_deadcmd(RArchive *ar, DeadCmd *dead_cmd)
{ unpack_buf(ar, dead_cmd, sizeof(*dead_cmd)); }

void create_nodes(	World *w,
					const NodeGroupDef *def,
					const SlotVal *init_vals, U32 init_vals_count,
					U64 group_id, U8 peer_id)
{
	ensure(!g_env.netstate || g_env.netstate->authority);

	U32 handles[MAX_NODES_IN_GROUP_DEF] = {};

	// Create nodes
	for (U32 node_i = 0; node_i < def->node_count; ++node_i) {
		const NodeGroupDef_Node *node_def = &def->nodes[node_i];
		/// @todo Don't query NodeType
		U32 h = alloc_node_without_impl(
							w,
							(NodeType*)res_by_name(	g_env.resblob,
													ResType_NodeType,
													node_def->type_name),
							w->next_node_id++,
							group_id,
							peer_id,
							def->res.name,
							node_i);
		handles[node_i] = h;
		NodeInfo *node = &w->nodes[h];

		ensure(node_def->default_struct_size > 0);

		U8 *default_struct = ZERO_ALLOC(frame_ator(), node_def->default_struct_size, "default_struct");
		if (node->type->init)
			node->type->init(default_struct);

		// Default values in NodeGroupDef override struct init values
		for (U32 i = 0; i < node_def->default_struct_size; ++i) {
			U8 *set_bytes = node_def->default_struct_set_bytes;
			if (set_bytes[i]) {
				U8 *default_bytes = node_def->default_struct;
				default_struct[i] = default_bytes[i];
			}
		}

		// Passed init values override default values of NodeGroupDef
		for (U32 i = 0; i < init_vals_count; ++i) {
			const SlotVal *val = &init_vals[i];
			if (strcmp(val->node_name, node_def->name))
				continue;

			/// @todo Don't do this! Slow. Or make RTTI fast.
			StructRtti *s = rtti_struct(node_def->type_name);
			ensure(s);
			U32 member_ix = rtti_member_index(node_def->type_name, val->member_name);
			U32 size = s->members[member_ix].size;
			U32 offset = s->members[member_ix].offset;
			if (val->size > size) {
				fail("Node init value is larger than member (%s.%s): %i > %i",
						val->node_name, val->member_name, val->size, size);
			}
			ensure(val->size <= size);
			memcpy(default_struct + offset, val->data, val->size);
		}

		// Resurrect impl from constructed value
		resurrect_node_impl(w, &w->nodes[h], default_struct);
	}

	// Commands
	for (U32 cmd_i = 0; cmd_i < def->cmd_count; ++cmd_i) {
		const NodeGroupDef_Cmd *cmd_def = &def->cmds[cmd_i];
		NodeCmd cmd = {0};

		switch (cmd_def->type) {
			case CmdType_memcpy: {
				U32 src_node_h = handles[cmd_def->src_node_i];
				U32 dst_node_h = handles[cmd_def->dst_node_i];

				cmd = (NodeCmd) {
					.type = CmdType_memcpy,
					.memcpy = (NodeCmd_Memcpy) {
						.src_offset = cmd_def->src_offset,
						.dst_offset = cmd_def->dst_offset,
						.size = cmd_def->size,
						.src_node = src_node_h,
						.dst_node = dst_node_h,
					}
				};
			} break;
			case CmdType_call: {
				ensure(cmd_def->p_count > 0);

				cmd = (NodeCmd) {
					.type = CmdType_call,
					.call = (NodeCmd_Call) {
						.fptr = cmd_def->fptr,
					}
				};
				for (U32 i = 0; i < cmd_def->p_count; ++i) {
					cmd.call.p_nodes[cmd.call.p_node_count++] =
						handles[cmd_def->p_node_i[i]];
					ensure(w->nodes[cmd.call.p_nodes[i]].allocated);
					node_impl(w, NULL, &w->nodes[cmd.call.p_nodes[i]]);
				}
			} break;
			default: fail("Invalid CmdType: %i", cmd_def->type);
		}
		cmd.has_condition = cmd_def->has_condition;
		cmd.cond_node_h = handles[cmd_def->cond_node_i];
		cmd.cond_offset = cmd_def->cond_offset;
		cmd.cond_size = cmd_def->cond_size;

		cmd.cmd_id = w->next_cmd_id++;
		resurrect_cmd(w, cmd);
	}
}

void overwrite_node_impl(World *w, Handle node_h, const DeadNode *deadnode)
{
	ensure(node_h != NULL_HANDLE);
	NodeInfo *node = &w->nodes[node_h];
	if (node->type->overwrite) {
		void *dead_impl;
		unpack_deadnode_impl(&dead_impl, deadnode);
		node->type->overwrite(node_impl(w, NULL, node), dead_impl);
	} else {
		// Don't destroy the node (only impl), because that breaks cmds referring to that node
		free_node_impl(w, node_h);
		resurrect_deadnode_impl(w, node_h, deadnode);
	}
}


void free_node_impl(World *w, U32 handle)
{
	NodeInfo *n = &w->nodes[handle];
	U32 impl_handle = n->impl_handle;
	if (n->type->auto_impl_mgmt) {
		if (n->type->free)
			n->type->free(impl_handle, node_impl(w, NULL, n));

		ensure(n->type->auto_storage_handle < w->auto_storage_count);
		AutoNodeImplStorage *st = &w->auto_storages[n->type->auto_storage_handle];
		ensure(impl_handle < st->max_count);
		st->allocated[impl_handle] = false;
		--st->count;
	} else {
		if (n->type->free)
			n->type->free(impl_handle, node_impl(w, NULL, n));
	}
}

void free_node(World *w, U32 handle)
{
	ensure(handle < MAX_NODE_COUNT);
	NodeInfo *n = &w->nodes[handle];
	ensure(n->allocated);

	set_tbl(Id, Handle)(&w->node_id_to_handle, n->node_id, NULL_HANDLE);

	// Remove commands involving this node
	for (U32 i = 0; i < MAX_NODE_ASSOC_CMD_COUNT; ++i) {
		Handle cmd_h = n->assoc_cmds[i];
		if (cmd_h != NULL_HANDLE)
			free_cmd(w, cmd_h);
	}

	free_node_impl(w, handle);

	--w->node_count;
	*n = (NodeInfo) { .allocated = false };
}

void free_node_group(World *w, U64 group_id)
{
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node = &w->nodes[i];
		if (!node->allocated)
			continue;

		if (node->group_id == group_id)
			free_node(w, i);
	}
}

void remove_node_group(World *w, void *node_impl_in_group)
{
	// @todo It's horrible to scan like this

	U64 group_id = (U64)-1;
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node = &w->nodes[i];
		if (!node->allocated)
			continue;

		if (node_impl(w, NULL, node) == node_impl_in_group) {
			group_id = node->group_id;
			break;
		}
	}

	ensure(group_id != (U64)-1);
	free_node_group(w, group_id);
}

U32 node_impl_handle(World *w, U32 node_handle)
{
	ensure(node_handle < MAX_NODE_COUNT);
	return w->nodes[node_handle].impl_handle;
}

Id node_handle_to_id(World *w, Handle handle)
{ return w->nodes[handle].node_id; }

Handle node_id_to_handle(World *w, Id id)
{ return get_tbl(Id, Handle)(&w->node_id_to_handle, id); }

Id cmd_handle_to_id(World *w, Handle handle)
{ return w->cmds[handle].cmd_id; }

Handle cmd_id_to_handle(World *w, Id id)
{ return get_tbl(Id, Handle)(&w->cmd_id_to_handle, id); }

U32 resurrect_cmd(World *w, NodeCmd cmd)
{
	if (w->cmd_count == MAX_NODE_CMD_COUNT)
		fail("Too many cmds");

	while (w->cmds[w->next_cmd].allocated)
		w->next_cmd = (w->next_cmd + 1) % MAX_NODE_CMD_COUNT;
	Handle cmd_h = w->next_cmd;

	ensure(w->next_cmd < MAX_NODE_CMD_COUNT);
	ensure(!w->cmds[cmd_h].allocated);
	++w->cmd_count;
	cmd.allocated = true;
	w->cmds[cmd_h] = cmd;
	ensure(cmd_id_to_handle(w, cmd.cmd_id) == NULL_HANDLE);
	set_tbl(Id, Handle)(&w->cmd_id_to_handle, cmd.cmd_id, cmd_h);

	{ // Add cmd to all associated nodes
		NodeCmd *cmd = &w->cmds[cmd_h];
		if (cmd->has_condition)
			add_node_assoc_cmd(w, cmd->cond_node_h, cmd_h);
		switch (cmd->type) {
		case CmdType_memcpy:
			// Might add duplicates
			add_node_assoc_cmd(w, cmd->memcpy.src_node, cmd_h);
			add_node_assoc_cmd(w, cmd->memcpy.dst_node, cmd_h);
		break;
		case CmdType_call:
			// Might add duplicates
			for (U32 k = 0; k < cmd->call.p_node_count; ++k)
				add_node_assoc_cmd(w, cmd->call.p_nodes[k], cmd_h);
		break;
		default: fail("Unknown cmd type %i", cmd->type);
		}
	}

	return cmd_h;
}

void free_cmd(World *w, U32 handle)
{
	ensure(handle < MAX_NODE_CMD_COUNT);

	NodeCmd *cmd = &w->cmds[handle];
	ensure(cmd->allocated);

	{ // Remove cmd from associated nodes
		if (cmd->has_condition)
			remove_node_assoc_cmd(w, cmd->cond_node_h, handle);
		switch (cmd->type) {
		case CmdType_memcpy:
			remove_node_assoc_cmd(w, cmd->memcpy.src_node, handle);
			remove_node_assoc_cmd(w, cmd->memcpy.dst_node, handle);
		break;
		case CmdType_call:
			for (U32 k = 0; k < cmd->call.p_node_count; ++k)
				remove_node_assoc_cmd(w, cmd->call.p_nodes[k], handle);
		break;
		default: fail("Unknown cmd type %i", cmd->type);
		}
	}

	set_tbl(Id, Handle)(&w->cmd_id_to_handle, cmd->cmd_id, NULL_HANDLE);

	--w->cmd_count;
	*cmd = (NodeCmd) { .allocated = false };
}

void * node_impl(World *w, U32 *size, NodeInfo *node)
{
	ensure(node->allocated);
	ensure(node->type);

	if (size)
		*size = node->type->size;

	U32 offset = node->type->size*node->impl_handle;
	if (node->type->auto_impl_mgmt) {
		ensure(node->type->auto_storage_handle < w->auto_storage_count);
		U8 *storage = w->auto_storages[node->type->auto_storage_handle].storage;
		return storage + offset;
	} else {
		return (U8*)node->type->storage() + offset;
	}
}

void resurrect_node_impl(World *w, NodeInfo *n, void *dead_impl_bytes)
{
	if (n->type->auto_impl_mgmt) {
		// Automatically manage impl memory
		ensure(n->type->auto_storage_handle < w->auto_storage_count);
		AutoNodeImplStorage *st = &w->auto_storages[n->type->auto_storage_handle];

		if (st->count >= st->max_count)
			fail(	"Too many nodes '%s': %i > %i",
					n->type->res.name, st->count + 1, st->max_count);

		while (st->allocated[st->next])
			st->next = (st->next + 1) % st->max_count;
		++st->count;

		const U32 h = st->next;
		void *e = (U8*)st->storage + h*st->size;
		memcpy(e, dead_impl_bytes, st->size);
		st->allocated[h] = true;

		// In-place resurrection
		if (n->type->resurrect) {
			U32 ret = n->type->resurrect(e);
			ensure(ret == NULL_HANDLE);
		}
		n->impl_handle = h;
	} else {
		// Manual memory management
		ensure(n->type->resurrect);
		n->impl_handle = n->type->resurrect(dead_impl_bytes);
	}
}

U32 alloc_node_without_impl(World *w, NodeType *type, U64 node_id, U64 group_id, U8 peer_id, const char *group_def_name, U8 node_ix_in_group)
{
	if (w->node_count == MAX_NODE_COUNT)
		fail("Too many nodes");

	NodeInfo info = {
		.allocated = true,
		.type = type,
		.node_id = node_id,
		.group_id = group_id,
		.peer_id = peer_id,
		.node_ix_in_group = node_ix_in_group,
	};
	fmt_str(info.type_name, sizeof(info.type_name), "%s", type->res.name);
	fmt_str(info.group_def_name, sizeof(info.group_def_name), "%s", group_def_name);
	for (U32 i = 0; i < MAX_NODE_ASSOC_CMD_COUNT; ++i)
		info.assoc_cmds[i] = NULL_HANDLE;

	while (w->nodes[w->next_node].allocated)
		w->next_node = (w->next_node + 1) % MAX_NODE_COUNT;

	ensure(w->next_node < MAX_NODE_COUNT);
	ensure(!w->nodes[w->next_node].allocated);
	++w->node_count;
	w->nodes[w->next_node] = info;
	set_tbl(Id, Handle)(&w->node_id_to_handle, node_id, w->next_node);
	return w->next_node;
}

void world_on_res_reload(ResBlob *old)
{
	World *w = g_env.world;

	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *n = &w->nodes[i];
		if (!n->allocated)
			continue;

		n->type = (NodeType*)res_by_name(	g_env.resblob,
											ResType_NodeType,
											n->type_name);
	}


	U32 old_ntypes_count;
	all_res_by_type(&old_ntypes_count,
					g_env.resblob, ResType_NodeType);

	U32 ntypes_count;
	NodeType **ntypes =
		(NodeType **)all_res_by_type(	&ntypes_count,
										g_env.resblob,
										ResType_NodeType);

	// Not a rigorous solution, but probably catches 99% of bad cases
	// For a new NodeTypes during runtime one would need to
	// resize w->auto_storages, possibly reordering some handles
	if (ntypes_count != old_ntypes_count)
		fail("@todo Runtime load/unload for NodeTypes");

	U32 next_auto_storage_handle = 0;
	for (U32 i = 0; i < ntypes_count; ++i) {
		NodeType *ntype = ntypes[i];
		if (!ntype->auto_impl_mgmt)
			continue;
		// This is gonna break horribly some day.
		ntype->auto_storage_handle = next_auto_storage_handle++;
	}

	for (U32 cmd_i = 0; cmd_i < w->cmd_count; ++cmd_i) {
		NodeCmd *cmd = &w->cmds[cmd_i];
		if (cmd->type == CmdType_call)
			cmd->call.fptr = rtti_relocate_sym(cmd->call.fptr);
	}

	// Nobody should have pointers to resources. Just ResIds.
#if 0
	// Reinitialize every auto storage node so that cached pointers are updated
	// It's probably simplest just to free and resurrect them
	/// @todo No! Single nodes can make assumptions about other nodes in
	///       the group, and then resurrecting messes everything up.
	///       Maybe there should be an optional "on_recompile" func.
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node = &w->nodes[i];
		if (!node->allocated)
			continue;
		if (!node->type->auto_impl_mgmt)
			continue; // Manually managed have manual logic for res reload

		if (node->type->free) {
			node->type->free(node->impl_handle, node_impl(w, NULL, node));
		}
		if (node->type->resurrect) {
			U32 ret = node->type->resurrect(node_impl(w, NULL, node));
			ensure(ret == NULL_HANDLE);
		}
	}
#endif
}

