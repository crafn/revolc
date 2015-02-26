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

void upd_world(World *w, F64 dt)
{
	w->time += dt;
	NodeInfo *nodes= w->nodes;

	/// @todo Sorting, update batches
	for (U32 node_i= 0; node_i < MAX_NODE_COUNT; ++node_i) {
		NodeInfo *node= &nodes[node_i];
		if (!node->allocated)
			continue;

		if (node->type->upd)
			node->type->upd(w, node_impl(NULL, node), 1);

		// Propagate values
		for (U32 r_i= 0; r_i < MAX_NODE_ROUTING_COUNT; ++r_i) {
			if (!node->routing[r_i].allocated)
				continue;

			SlotRouting *r= &node->routing[r_i];
			ensure(r->dst_node < MAX_NODE_COUNT);
			NodeInfo *dst_node= &w->nodes[r->dst_node];

			U8 *dst= (U8*)node_impl(NULL, dst_node) + r->dst_offset;
			U8 *src= (U8*)node_impl(NULL, node) + r->src_offset;
			for (U32 i= 0; i < r->size; ++i)
				dst[i]= src[i];
		}
	}
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

	for (U32 i= 0; i < header.node_count; ++i) {
		// New NodeInfo from binary
		NodeInfo dead_node;
		fread(&dead_node, 1, sizeof(dead_node), file);
		U32 node_h= alloc_node(
				w,
				(NodeType*)res_by_name(
					g_env.res_blob,
					ResType_NodeType,
					dead_node.type_name));
		ensure(node_h == i); // Must retain handles because of routing

		NodeInfo *n= &w->nodes[node_h];
		memcpy(n->routing, dead_node.routing, sizeof(n->routing));
		memcpy(n->type_name, dead_node.type_name, sizeof(n->type_name));

		// New Node implementation from binary
		U8 dead_impl_bytes[n->type->size]; /// @todo Alignment!!!
		fread(dead_impl_bytes, 1, n->type->size, file);
		U8 *impl= node_impl(NULL, n);
		if (n->type->resurrect)
			n->type->resurrect(n->impl_handle, dead_impl_bytes);
		else
			memcpy(impl, dead_impl_bytes, n->type->size);
	}
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
		.node_count= w->node_count,
	};

	fwrite(&header, 1, sizeof(header), file);
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node= &w->nodes[i];
		if (!node->allocated)
			continue;

		U32 impl_size;
		void *impl= node_impl(&impl_size, node);

		fwrite(node, 1, sizeof(*node), file);
		fwrite(impl, 1, impl_size, file);
	}

	fclose(file);
}

U32 alloc_node(World *w, NodeType* type)
{
	if (w->node_count == MAX_NODE_COUNT)
		fail("Too many nodes");

	NodeInfo info= {
		.allocated= true,
		.type= type
	};
	info.impl_handle= type->alloc();
	snprintf(info.type_name, sizeof(info.type_name), "%s", type->res.name);

	while (w->nodes[w->next_node].allocated)
		w->next_node= (w->next_node + 1) % MAX_NODE_COUNT;

	++w->node_count;
	w->nodes[w->next_node]= info;
	return w->next_node;
}

void free_node(World *w, U32 handle)
{
	ensure(handle < MAX_NODE_COUNT);
	ensure(w->nodes[handle].allocated);

	U32 impl_handle= w->nodes[handle].impl_handle;
	w->nodes[handle].type->free(impl_handle);

	--w->node_count;
	w->nodes[handle].allocated= false;
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

void world_on_res_reload(World *w, struct ResBlob* blob)
{
	fail("@todo world res reload");
}
