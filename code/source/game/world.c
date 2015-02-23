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
		int asd= (int)e;
		e->pos.x= sin(asd + w->time*0.7)*3.0;
		e->pos.y= sin(asd*asd + w->time*0.3427)*1.5;
		e->pos.z= 2.0 + i*0.1 + sin(asd + w->time);
	}
}

void upd_world(World *w, F64 dt)
{
	w->time += dt;
	NodeInfo *nodes= w->nodes;

	/// @todo Sorting, update batches
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		if (!nodes[i].allocated)
			continue;

		switch (nodes[i].type) {
			case NodeType_ModelEntity:
				upd_modelentity_nodes(
						w,
						get_modelentity(g_env.renderer,
										nodes[i].impl_handle),
						1);
			break;
			default: fail("upd_world: unhandled type: %i", nodes[i].type);
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
