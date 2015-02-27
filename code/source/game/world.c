#include "core/file.h"
#include "core/malloc.h"
#include "game/world.h"
#include "global/env.h"

#include <math.h>

internal
void * node_impl(U32 *size, NodeInfo *node)
{
	if (size)
		*size= node->type->size;
	return (U8*)node->type->storage() + node->type->size*node->impl_handle;
}

internal
void resurrect_node_impl(NodeInfo *n, void *dead_impl_bytes)
{
	n->impl_handle= n->type->resurrect(dead_impl_bytes);
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
	snprintf(info.type_name, sizeof(info.type_name), "%s", type->res.name);

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
	return w;
}

void destroy_world(World *w)
{
	debug_print("destroy_world: %i nodes", w->node_count);
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		if (w->nodes[i].allocated)
			free_node(w, i);
	}
	free(w);
}

internal
int node_cmp(const void * a_, const void * b_)
{
	const NodeInfo *a= a_, *b= b_;
	// Sort for batching
	// Non-allocated should be at the end
	/// @todo Prioritize by routing, but still clumping by type
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
			batch_begin_type->upd(
					w,
					node_impl(NULL, &w->sort_space[batch_begin_i]),
					batch_size);
		}
		++batch_count;

		// Propagate values
		for (U32 src_i= batch_begin_i; src_i < node_i; ++src_i) {
			NodeInfo *src_node= &w->sort_space[src_i];
			for (U32 r_i= 0; r_i < MAX_NODE_ROUTING_COUNT; ++r_i) {
				if (!src_node->routing[r_i].allocated)
					continue;

				SlotRouting *r= &src_node->routing[r_i];
				ensure(r->dst_node < MAX_NODE_COUNT);
				NodeInfo *dst_node= &w->nodes[r->dst_node];
				ensure(src_node->allocated && dst_node->allocated);

				U8 *dst= (U8*)node_impl(NULL, dst_node) + r->dst_offset;
				U8 *src= (U8*)node_impl(NULL, src_node) + r->src_offset;
				for (U32 i= 0; i < r->size; ++i)
					dst[i]= src[i];
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
	for (U32 i= 0; i < header.node_count;
			++i, w->next_node= (w->next_node + 1) % MAX_NODE_COUNT) {
		ensure(w->next_node == i);
		// New NodeInfo from binary
		NodeInfo dead_node;
		fread(&dead_node, 1, sizeof(dead_node), file);
		if (!dead_node.allocated)
			continue;

		U32 node_h= alloc_node_without_impl(
				w,
				(NodeType*)res_by_name(
					g_env.res_blob,
					ResType_NodeType,
					dead_node.type_name),
				dead_node.group_id);
		ensure(node_h == i); // Must retain handles because of routing
		++node_count;

		NodeInfo *n= &w->nodes[node_h];
		memcpy(n->type_name, dead_node.type_name, sizeof(n->type_name));
		memcpy(n->routing, dead_node.routing, sizeof(n->routing));
		n->group_id= dead_node.group_id;

		// New Node implementation from binary
		U8 dead_impl_bytes[n->type->size]; /// @todo Alignment!!!
		fread(dead_impl_bytes, 1, n->type->size, file);
		resurrect_node_impl(n, dead_impl_bytes);
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
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node= &w->nodes[i];
		// Save all, even non-allocated nodes
		// Easy way to keep routing handles valid
		fwrite(node, 1, sizeof(*node), file);
		if (!node->allocated)
			continue;

		U32 impl_size;
		void *impl= node_impl(&impl_size, node);
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
							(NodeType*)res_by_name(	g_env.res_blob,
													ResType_NodeType,
													node_def->type_name),
							group_id);
		handles[node_i]= h;
		NodeInfo *node= &w->nodes[h];

		U8 default_struct[sizeof(node_def->default_struct)]= {};
		if (node->type->init)
			node->type->init(default_struct);

		// Default values in NodeGroupDef override struct init value
		for (U32 i= 0; i < node_def->default_struct_size; ++i) {
			if (node_def->default_struct_set_bytes[i])
				default_struct[i]= node_def->default_struct[i];
		}

		// Passed init values override default values of NodeGroupDef
		for (U32 i= 0; i < init_vals_count; ++i) {
			const SlotVal *val= &init_vals[i];
			if (strcmp(val->node_name, node_def->name))
				continue;

			/// @todo Don't do this. Slow. Or make RTTI fast.
			U32 size= member_size(node_def->type_name, val->member_name);
			U32 offset= member_offset(node_def->type_name, val->member_name);
			ensure(val->size <= size);
			memcpy(default_struct + offset, val->data, val->size);
		}

		// Resurrect impl from constructed value
		resurrect_node_impl(&w->nodes[h], default_struct);
	}

	// Route slots
	for (U32 node_i= 0; node_i < def->node_count; ++node_i) {
		const NodeGroupDef_Node *node= &def->nodes[node_i];
		for (U32 out_i= 0; out_i < node->output_count; ++out_i) {
			const NodeGroupDef_Node_Output *out= &node->outputs[out_i];
			add_routing(	w,
							handles[node_i],			out->src_offset,
							handles[out->dst_node_i],	out->dst_offset,
							out->size);
		}
	}
}

void free_node(World *w, U32 handle)
{
	ensure(handle < MAX_NODE_COUNT);
	NodeInfo *n= &w->nodes[handle];
	ensure(n->allocated);

	U32 impl_handle= n->impl_handle;
	n->type->free(impl_handle);

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

void add_routing(	World *w,
					U32 src_node_h, U32 src_offset,
					U32 dst_node_h, U32 dst_offset,
					U32 size)
{
	ensure(src_node_h < MAX_NODE_COUNT);
	NodeInfo *src_node= &w->nodes[src_node_h];

	U32 routing_i= 0;
	while (	src_node->routing[routing_i].allocated &&
			routing_i < MAX_NODE_ROUTING_COUNT)
		++routing_i;
	if (routing_i >= MAX_NODE_ROUTING_COUNT)
		fail("Too many node routings");

	src_node->routing[routing_i]= (SlotRouting) {
		.allocated= true,
		.src_offset= src_offset,
		.dst_offset= dst_offset,
		.size= size,
		.dst_node= dst_node_h
	};
}

void world_on_res_reload(struct ResBlob* blob)
{
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *n= &g_env.world->nodes[i];
		if (!n->allocated)
			continue;

		n->type= (NodeType*)res_by_name(blob, ResType_NodeType, n->type_name);
	}
}
