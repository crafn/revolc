#include "game/world.h"
#include "global/env.h"
#include "visual/renderer.h"

#include <math.h>

/*
void init_visualnodes(VisualNode *node, VisualNode *end)
{
	while (node++ != end) {
		node->handle=
			create_modelentity(
					g_env.renderer,
					(Model*)resource_by_name(g_env.res_blob, node->model_input));
	}
}

void deinit_visualnodes(VisualNode *node, VisualNode *end)
{
	while (node++ != end) {
		destroy_modelentity(
				g_env.renderer,
				node->handle);
	}
}
*/

internal
T3d temptest_t3d_storage[1024];
internal
U32 next_t3d= 0;


internal
void * node_impl(NodeInfo *node)
{
	switch (node->type) {
		case NodeType_ModelEntity:
			return get_modelentity(g_env.renderer, node->impl_handle);
		break;
		case NodeType_T3d:
			return &temptest_t3d_storage[node->impl_handle];
		break;
		default:
			fail("node_impl: unhandled type: %s", node->type);
	}
	return NULL;
}

void upd_t3d_nodes(	World *w,
					T3d *t,
					U32 count)

{
	for (U32 i= 0; i < count; ++i, ++t) {
		int asd= (int)t;
		t->pos.x= sin(asd + w->time*0.7)*3.0;
		t->pos.y= sin(asd*asd + w->time*0.3427)*1.5;
		t->pos.z= sin(asd)*5 + 6;
	}
}

void upd_modelentity_nodes(	World *w,
							ModelEntity *e,
							U32 count)
{
	for (U32 i= 0; i < count; ++i, ++e) {
		/*if (!changed[i].model_name) {
			set_modelentity(
					&node[i],
					(Model*)res_by_name(g_env.res_blob,
										ResType_Model,
										node[i].model_name));
		}*/
	}
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

		switch (node->type) {
			case NodeType_ModelEntity:
				upd_modelentity_nodes(w, node_impl(node), 1);
			break;
			case NodeType_T3d:
				upd_t3d_nodes(w, node_impl(node), 1);
			break;
			default: fail("upd_world: unhandled type: %i", node->type);
		}

		// Propagate values
		for (U32 r_i= 0; r_i < MAX_NODE_ROUTING_COUNT; ++r_i) {
			if (!node->routing[r_i].allocated)
				continue;

			SlotRouting *r= &node->routing[r_i];
			ensure(r->dst_node < MAX_NODE_COUNT);
			NodeInfo *dst_node= &w->nodes[r->dst_node];

			for (U32 i= 0; i < r->size; ++i) {
				((U8*)node_impl(dst_node))[r->dst_offset + i]=
					((U8*)node_impl(node))[r->src_offset + i];
			}
		}
	}
}

U32 alloc_node(World *w, NodeType type)
{
	if (w->node_count == MAX_NODE_COUNT)
		fail("Too many nodes");

	NodeInfo info= {
		.allocated= true,
		.type= type
	};
	switch (type) {
		case NodeType_ModelEntity:
			info.impl_handle= alloc_modelentity(g_env.renderer);
		break;
		case NodeType_T3d:
			info.impl_handle= next_t3d++;
		break;
		default: fail("create_node: unhandled type: %i", type);
	}

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
	NodeType type= w->nodes[handle].type;
	switch (w->nodes[handle].type) {
		case NodeType_ModelEntity:
			free_modelentity(g_env.renderer, impl_handle);
		break;
		case NodeType_T3d:
		break;
		default: fail("destroy_node: unhandled type: %i", type);
	}
	--w->node_count;
	w->nodes[handle].allocated= false;
}

U32 node_impl_handle(World *w, U32 node_handle)
{
	ensure(node_handle < MAX_NODE_COUNT);
	return w->nodes[node_handle].impl_handle;
}

void add_routing(	World *w,
					U32 dst_node_h, U32 dst_offset,
					U32 src_node_h, U32 src_offset,
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
