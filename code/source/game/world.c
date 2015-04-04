#include "core/file.h"
#include "core/malloc.h"
#include "game/world.h"
#include "global/env.h"
#include "platform/math.h"

internal
void * node_impl(World *w, U32 *size, NodeInfo *node)
{
	if (size)
		*size= node->type->size;

	U32 offset= node->type->size*node->impl_handle;
	if (node->type->auto_impl_mgmt) {
		ensure(node->type->auto_storage_handle < w->auto_storage_count);
		U8 *storage= w->auto_storages[node->type->auto_storage_handle].storage;
		return storage + offset;
	} else {
		return (U8*)node->type->storage() + offset;
	}
}

internal
void resurrect_node_impl(World *w, NodeInfo *n, void *dead_impl_bytes)
{
	if (n->type->auto_impl_mgmt) {
		// Automatically manage impl memory
		ensure(n->type->auto_storage_handle < w->auto_storage_count);
		AutoNodeImplStorage *st= &w->auto_storages[n->type->auto_storage_handle];

		if (st->count >= st->max_count)
			fail("Too many %s", n->type->res.name);

		while (st->allocated[st->next])
			st->next= (st->next + 1) % st->max_count;
		++st->count;

		const U32 h= st->next;
		void *e= (U8*)st->storage + h*st->size;
		memcpy(e, dead_impl_bytes, st->size);
		st->allocated[h]= true;

		// In-place resurrection
		if (n->type->resurrect) {
			U32 ret= n->type->resurrect(e);
			ensure(ret == NULL_HANDLE);
		}
		n->impl_handle= h;
	} else {
		// Manual memory management
		n->impl_handle= n->type->resurrect(dead_impl_bytes);
	}
}

internal
U32 alloc_node_without_impl(World *w, NodeType *type, U64 group_id)
{
	if (w->node_count == MAX_NODE_COUNT)
		fail("Too many nodes");

	NodeInfo info= {
		.allocated= true,
		.type= type,
		.group_id= group_id,
	};
	fmt_str(info.type_name, sizeof(info.type_name), "%s", type->res.name);

	while (w->nodes[w->next_node].allocated)
		w->next_node= (w->next_node + 1) % MAX_NODE_COUNT;

	ensure(w->next_node < MAX_NODE_COUNT);
	ensure(!w->nodes[w->next_node].allocated);
	++w->node_count;
	w->nodes[w->next_node]= info;
	return w->next_node;
}

World * create_world()
{
	World *w= zero_malloc(sizeof(*w));

	// Initialize storage for automatically allocated node impls

	U32 ntypes_count;
	NodeType **ntypes=
		(NodeType **)all_res_by_type(	&ntypes_count,
										g_env.resblob,
										ResType_NodeType);

	U32 ntype_count_with_auto_mgmt= 0;
	for (U32 i= 0; i < ntypes_count; ++i) {
		const NodeType *type= ntypes[i];
		if (type->auto_impl_mgmt)
			++ntype_count_with_auto_mgmt;
	}

	w->auto_storages=
		zero_malloc(sizeof(*w->auto_storages)*ntype_count_with_auto_mgmt);
	w->auto_storage_count= ntype_count_with_auto_mgmt;

	U32 st_i= 0;
	for (U32 i= 0; i < ntypes_count; ++i) {
		NodeType *type= ntypes[i];
		if (!type->auto_impl_mgmt)
			continue;

		AutoNodeImplStorage *st= &w->auto_storages[st_i];
		st->storage= zero_malloc(type->size*type->auto_impl_max_count);
		st->allocated=
			zero_malloc(sizeof(*st->allocated)*type->auto_impl_max_count);
		st->size= type->size;
		st->max_count= type->auto_impl_max_count;

		// Cache handle to storage in NodeType itself for fastness
		type->auto_storage_handle= st_i++;
	}

	return w;
}

void destroy_world(World *w)
{
	debug_print("destroy_world: %i nodes", w->node_count);
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		if (w->nodes[i].allocated)
			free_node(w, i);
	}
	for (U32 i= 0; i < w->auto_storage_count; ++i) {
		free(w->auto_storages[i].storage);
		free(w->auto_storages[i].allocated);
	}
	free(w->auto_storages);
	free(w);
}

internal
int node_cmp(const void * a_, const void * b_)
{
	const NodeInfo *a= a_, *b= b_;
	// Sort for batching
	// Non-allocated should be at the end
	/// @todo Prioritize by cmds, but still clumping by type
	int alloc_cmp= b->allocated - a->allocated;
	if (alloc_cmp) {
		return alloc_cmp;
	} else { 
		int str_cmp= strcmp(a->type_name, b->type_name);
		if (str_cmp)
			return str_cmp;
		else
			return	(a->impl_handle < b->impl_handle) -
					(a->impl_handle > b->impl_handle);
	}
}
 
void upd_world(World *w, F64 dt)
{
	w->dt= dt;
	w->time += dt;

	memcpy(w->sort_space, w->nodes, sizeof(w->nodes));
	/// @todo	Optimize sorting -- this qsort causes a major fps drop.
	///			Maybe don't sort everything again every frame?
	///			Maybe calculate priorities beforehand for fast cmp?
	///			See clover/code/source/nodes/updateline.cpp for priorization
	//qsort(w->sort_space, MAX_NODE_COUNT, sizeof(*w->nodes), node_cmp);

	U32 node_i= 0;
	U32 updated_count= 0;
	U32 batch_count= 0;
	U32 signal_count= 0;
	while (node_i < MAX_NODE_COUNT) {
		NodeInfo *node= &w->sort_space[node_i];
		if (!node->allocated) {
			++node_i;
			continue;
		}

		const U32 batch_begin_i= node_i;
		const U32 batch_begin_impl_handle= node->impl_handle;
		const NodeType *batch_begin_type= node->type;

		while (	node_i < MAX_NODE_COUNT &&
				node->type == batch_begin_type &&
				node->impl_handle ==	batch_begin_impl_handle +
										batch_begin_i - node_i) {
			ensure(node->allocated);
			++node_i;
			++node;
		}

		// Update batch
		const U32 batch_size= node_i - batch_begin_i;
		updated_count += batch_size; 
		if (batch_begin_type->upd) {
			U8 *begin= node_impl(w, NULL, &w->sort_space[batch_begin_i]);
			batch_begin_type->upd(
					begin,
					begin + batch_size*batch_begin_type->size);
		}
		++batch_count;

		// Perform commands stated in NodeGroupDefs
		for (U32 dst_i= batch_begin_i; dst_i < node_i; ++dst_i) {
			NodeInfo *dst_node= &w->sort_space[dst_i];
			for (U32 r_i= 0; r_i < dst_node->cmd_count; ++r_i) {
				SlotCmd *r= &dst_node->cmds[r_i];
				if (r->has_condition) {
					NodeInfo *c_node= &w->nodes[r->cond_node_h];
					U8 *cond_bytes= node_impl(w, NULL, c_node) + r->cond_offset;
					bool cond_fullfilled= false;
					for (U32 i= 0; i < r->cond_size; ++i) {
						if (cond_bytes[i]) {
							cond_fullfilled= true;
							break;
						}
					}

					if (!cond_fullfilled)
						continue; // Skip command
				}
				/// @todo Batching?
				switch (r->type) {
				case CmdType_memcpy: {
					ensure(r->src_node < MAX_NODE_COUNT);
					NodeInfo *src_node= &w->nodes[r->src_node];
					ensure(dst_node->allocated && src_node->allocated);

					U8 *dst= (U8*)node_impl(w, NULL, dst_node) + r->dst_offset;
					U8 *src= (U8*)node_impl(w, NULL, src_node) + r->src_offset;
					for (U32 i= 0; i < r->size; ++i)
						dst[i]= src[i];
				} break;
				case CmdType_call: {
					// This is ugly. Call function pointer with corresponding node parameters
					/// @todo Generate this
					void *dst_impl= node_impl(w, NULL, dst_node);
					switch (r->p_node_count) {
					case 0:
						((void (*)(void *, void *))r->fptr)(
							dst_impl, dst_impl + dst_node->type->size);
					break;
					case 1: {
						void * p1_impl=
							node_impl(w, NULL, &w->sort_space[r->p_nodes[0]]);
						U32 p1_size= w->sort_space[r->p_nodes[0]].type->size;
						((void (*)(void *, void *,
								   void *, void *))r->fptr)(
							dst_impl, dst_impl + dst_node->type->size,
							p1_impl, p1_impl + p1_size);
					} break;
					case 2: {
						void * p1_impl=
							node_impl(w, NULL, &w->sort_space[r->p_nodes[0]]);
						U32 p1_size= w->sort_space[r->p_nodes[0]].type->size;
						void * p2_impl=
							node_impl(w, NULL, &w->sort_space[r->p_nodes[1]]);
						U32 p2_size= w->sort_space[r->p_nodes[1]].type->size;
						((void (*)(void *, void *,
								   void *, void *,
								   void *, void *))r->fptr)(
							dst_impl, dst_impl + dst_node->type->size,
							p1_impl, p1_impl + p1_size,
							p2_impl, p2_impl + p2_size);
					} break;
					default: fail("Too many node params");
					}
				} break;
				default: fail("Unknown cmd type: %i", r->type);
				}
				++signal_count;
			}
		}
	}

	//debug_print("upd signal count: %i", signal_count);
	//debug_print("upd batch count: %i", batch_count);
}

typedef struct SaveHeader {
	U32 version;
	U32 node_count;
} PACKED SaveHeader;

typedef struct SaveCmd {
	char func_name[MAX_FUNC_NAME_SIZE];
} PACKED SaveCmd;
typedef struct SaveNode {
	NodeInfo info;
	SaveCmd cmds[MAX_NODE_CMD_COUNT];
	U32 cmd_count;
} PACKED SaveNode;

void load_world(World *w, const char *path)
{
	debug_print("load_world: %s", path);
	if (w->node_count > 0) {
		debug_print("load_world: destroying current world");
		for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
			if (w->nodes[i].allocated)
				free_node(w, i);
		}
	}
	ensure(w->node_count == 0);

	FILE *file= fopen(path, "rb");
	if (!file)
		fail("load_world: Couldn't open: %s", path);

	SaveHeader header;
	int len= fread(&header, 1, sizeof(header), file);
	if (len != sizeof(header))
		fail("load_world: Read error");

	U32 node_count= 0;
	for (U32 node_i= 0; node_i < header.node_count;
			++node_i, w->next_node= (w->next_node + 1) % MAX_NODE_COUNT) {
		ensure(w->next_node == node_i);
		// New NodeInfo from binary
		SaveNode dead_node;
		fread(&dead_node, 1, sizeof(dead_node), file);
		if (!dead_node.info.allocated)
			continue;

		U32 node_h= alloc_node_without_impl(
				w,
				(NodeType*)res_by_name(
					g_env.resblob,
					ResType_NodeType,
					dead_node.info.type_name),
				dead_node.info.group_id);
		ensure(node_h == node_i); // Must retain handles because of cmds
		++node_count;

		NodeInfo *n= &w->nodes[node_h];
		memcpy(n->type_name, dead_node.info.type_name, sizeof(n->type_name));
		memcpy(n->cmds, dead_node.info.cmds, sizeof(n->cmds));
		n->cmd_count= dead_node.info.cmd_count;
		n->group_id= dead_node.info.group_id;

		// Set func pointers
		for (U32 i= 0; i < n->cmd_count; ++i) {
			if (n->cmds[i].type == CmdType_call) {
				n->cmds[i].fptr=
					rtti_func_ptr(dead_node.cmds[i].func_name);
			}
		}

		// New Node implementation from binary
		U8 dead_impl_bytes[n->type->size]; /// @todo Alignment!!!
		fread(dead_impl_bytes, 1, n->type->size, file);
		resurrect_node_impl(w, n, dead_impl_bytes);
	}

	ensure(node_count == w->node_count);
	fclose(file);
}

void save_world(World *w, const char *path)
{
	debug_print("save_world: %s, %i nodes", path, w->node_count);
	FILE *file= fopen(path, "wb");
	if (!file)
		fail("save_world: Couldn't open: %s", path);

	SaveHeader header= {
		.version= 1,
		.node_count= MAX_NODE_COUNT,
	};

	fwrite(&header, 1, sizeof(header), file);
	for (U32 node_i= 0; node_i < MAX_NODE_COUNT; ++node_i) {
		NodeInfo *node= &w->nodes[node_i];
		SaveNode savenode= {.info= *node};

		// Must use names of func ptrs
		if (node->allocated) {
			for (U32 i= 0; i < node->cmd_count; ++i) {
				SlotCmd *cmd= &node->cmds[i];
				if (cmd->type != CmdType_call)
					continue;
				fmt_str(savenode.cmds[i].func_name,
						sizeof(savenode.cmds[i].func_name),
						"%s", rtti_sym_name(cmd->fptr));
			}
		}

		// Save all, even non-allocated nodes
		// Easy way to keep cmd handles valid
		fwrite(&savenode, 1, sizeof(savenode), file);
		if (!node->allocated)
			continue;

		U32 impl_size;
		void *impl= node_impl(w, &impl_size, node);
		fwrite(impl, 1, impl_size, file);
	}

	fclose(file);
}

void create_nodes(	World *w,
					const NodeGroupDef *def,
					const SlotVal *init_vals, U32 init_vals_count,
					U64 group_id)
{
	U32 handles[MAX_NODES_IN_GROUP_DEF]= {};

	// Create nodes
	for (U32 node_i= 0; node_i < def->node_count; ++node_i) {
		const NodeGroupDef_Node *node_def= &def->nodes[node_i];
		/// @todo Don't query NodeType
		U32 h= alloc_node_without_impl(
							w,
							(NodeType*)res_by_name(	g_env.resblob,
													ResType_NodeType,
													node_def->type_name),
							group_id);
		handles[node_i]= h;
		NodeInfo *node= &w->nodes[h];

		U8 default_struct[sizeof(node_def->default_struct)]= {};
		if (node->type->init)
			node->type->init(default_struct);

		// Default values in NodeGroupDef override struct init values
		for (U32 i= 0; i < node_def->default_struct_size; ++i) {
			if (node_def->default_struct_set_bytes[i])
				default_struct[i]= node_def->default_struct[i];
		}

		// Passed init values override default values of NodeGroupDef
		for (U32 i= 0; i < init_vals_count; ++i) {
			const SlotVal *val= &init_vals[i];
			if (strcmp(val->node_name, node_def->name))
				continue;

			/// @todo Don't do this! Slow. Or make RTTI fast.
			U32 size= rtti_member_size(node_def->type_name, val->member_name);
			U32 offset= rtti_member_offset(node_def->type_name, val->member_name);
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
	for (U32 cmd_i= 0; cmd_i < def->cmd_count; ++cmd_i) {
		const NodeGroupDef_Cmd *cmd_def= &def->cmds[cmd_i];

		switch (cmd_def->type) {
			case CmdType_memcpy: {
				U32 src_node_h= handles[cmd_def->src_node_i];
				ensure(src_node_h < MAX_NODE_COUNT);

				U32 dst_node_h= handles[cmd_def->dst_node_i];

				NodeInfo *dst_node= &w->nodes[dst_node_h];
				U32 cmd_i= dst_node->cmd_count++;
				if (cmd_i >= MAX_NODE_CMD_COUNT)
					fail("Too many node cmds");

				dst_node->cmds[cmd_i]= (SlotCmd) {
					.type= CmdType_memcpy,
					.src_offset= cmd_def->src_offset,
					.dst_offset= cmd_def->dst_offset,
					.size= cmd_def->size,
					.src_node= src_node_h,
				};
				SlotCmd *cmd= &dst_node->cmds[cmd_i];
				cmd->has_condition= cmd_def->has_condition;
				cmd->cond_node_h= handles[cmd_def->cond_node_i];
				cmd->cond_offset= cmd_def->cond_offset;
				cmd->cond_size= cmd_def->cond_size;
			} break;
			case CmdType_call: {
				ensure(cmd_def->p_count > 0);

				// Convention: "Destination" node is the first parameter
				const U32 dst_node_i= cmd_def->p_node_i[0];
				const U32 dst_node_h= handles[dst_node_i];

				NodeInfo *dst_node= &w->nodes[dst_node_h];
				const U32 cmd_i= dst_node->cmd_count++;
				if (cmd_i >= MAX_NODE_CMD_COUNT)
					fail("Too many node cmds");

				SlotCmd *slot= &dst_node->cmds[cmd_i];
				*slot= (SlotCmd) {
					.type= CmdType_call,
					.fptr= cmd_def->fptr,
				};
				for (U32 i= 1; i < cmd_def->p_count; ++i) {
					slot->p_nodes[slot->p_node_count++]=
						handles[cmd_def->p_node_i[i]];
				}
				SlotCmd *cmd= &dst_node->cmds[cmd_i];
				cmd->has_condition= cmd_def->has_condition;
				cmd->cond_node_h= handles[cmd_def->cond_node_i];
				cmd->cond_offset= cmd_def->cond_offset;
				cmd->cond_size= cmd_def->cond_size;
			} break;
			default: fail("Invalid CmdType: %i", cmd_def->type);
		}
	}
}

void free_node(World *w, U32 handle)
{
	ensure(handle < MAX_NODE_COUNT);
	NodeInfo *n= &w->nodes[handle];
	ensure(n->allocated);

	U32 impl_handle= n->impl_handle;
	if (n->type->auto_impl_mgmt) {
		if (n->type->free)
			n->type->free(node_impl(w, NULL, n));

		ensure(n->type->auto_storage_handle < w->auto_storage_count);
		AutoNodeImplStorage *st= &w->auto_storages[n->type->auto_storage_handle];
		ensure(impl_handle < st->max_count);
		st->allocated[impl_handle]= false;
	} else {
		n->type->free(node_impl(w, NULL, n));
	}

	--w->node_count;
	*n= (NodeInfo) { .allocated= false };
}

void free_node_group(World *w, U64 group_id)
{
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node= &w->nodes[i];
		if (!node->allocated)
			continue;

		if (node->group_id == group_id)
			free_node(w, i);
	}
}

U32 node_impl_handle(World *w, U32 node_handle)
{
	ensure(node_handle < MAX_NODE_COUNT);
	return w->nodes[node_handle].impl_handle;
}

void world_on_res_reload(ResBlob *old)
{
	World *w= g_env.world;

	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *n= &w->nodes[i];
		if (!n->allocated)
			continue;

		n->type= (NodeType*)res_by_name(	g_env.resblob,
											ResType_NodeType,
											n->type_name);
	}


	U32 old_ntypes_count;
	all_res_by_type(&old_ntypes_count,
					g_env.resblob, ResType_NodeType);

	U32 ntypes_count;
	NodeType **ntypes=
		(NodeType **)all_res_by_type(	&ntypes_count,
										g_env.resblob,
										ResType_NodeType);

	// Not a rigorous solution, but probably catches 99% of bad cases
	// For a new NodeTypes during runtime one would need to
	// resize w->auto_storages, possibly reordering some handles
	if (ntypes_count != old_ntypes_count)
		fail("@todo Runtime load/unload for NodeTypes");

	U32 next_auto_storage_handle= 0;
	for (U32 i= 0; i < ntypes_count; ++i) {
		NodeType *ntype= ntypes[i];
		if (!ntype->auto_impl_mgmt)
			continue;
		// This is gonna break horribly some day.
		ntype->auto_storage_handle= next_auto_storage_handle++;
	}

	// Reinitialize every auto storage node so that cached pointers are updated
	// It's probably simplest just to free and resurrect them
	/// @todo No! Single nodes can make assumptions about other nodes in
	///       the group, and then resurrecting messes everything up.
	///       Maybe there should be an optional "on_recompile" func.
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node= &w->nodes[i];
		if (!node->allocated)
			continue;
		if (!node->type->auto_impl_mgmt)
			continue; // Manually managed have manual logic for res reload

		for (U32 slot_i= 0; slot_i < node->cmd_count; ++slot_i) {
			SlotCmd *cmd= &node->cmds[slot_i];
			if (cmd->type == CmdType_call)
				cmd->fptr= rtti_relocate_sym(cmd->fptr);
		}

		if (node->type->free) {
			node->type->free(node_impl(w, NULL, node));
		}
		if (node->type->resurrect) {
			U32 ret= node->type->resurrect(node_impl(w, NULL, node));
			ensure(ret == NULL_HANDLE);
		}
	}
}

