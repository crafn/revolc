#include "core/file.h"
#include "core/malloc.h"
#include "game/world.h"
#include "global/env.h"

/// TEMP
#include "visual/renderer.h"
#include "physics/physworld.h"
#include "resources/resblob.h"

#include <math.h>

internal
T3d temptest_t3d_storage[MAX_NODE_COUNT];
internal
U32 next_t3d= 0;

internal
void * node_impl(U32 *size, NodeInfo *node)
{
	switch (node->type) {
		case NodeType_ModelEntity:
			if (size) *size= sizeof(ModelEntity);
			return &g_env.renderer->entities[node->impl_handle];
		break;
		case NodeType_T3d:
			if (size) *size= sizeof(T3d);
			return &temptest_t3d_storage[node->impl_handle];
		break;
		case NodeType_RigidBody:
			if (size) *size= sizeof(RigidBody);
			return &g_env.phys_world->bodies[node->impl_handle];
		break;
		default:
			fail("node_impl: Unhandled type: %i", node->type);
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

		switch (node->type) {
			case NodeType_ModelEntity:
			break;
			case NodeType_T3d:
				upd_t3d_nodes(w, node_impl(NULL, node), 1);
			break;
			case NodeType_RigidBody:
			break;
			default: fail("upd_world: Unhandled type: %i", node->type);
		}

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

typedef struct {
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
		// Create node from dump

		NodeInfo dead_node;
		fread(&dead_node, 1, sizeof(dead_node), file);

		U32 node_h= alloc_node(w, dead_node.type);
		ensure(node_h == i); // Must retain handles because of routing

		NodeInfo *node= &w->nodes[node_h];
		memcpy(node->routing, dead_node.routing, sizeof(node->routing));

		switch (dead_node.type) {
			case NodeType_ModelEntity: {
				ModelEntity dead_impl;
				fread(&dead_impl, 1, sizeof(dead_impl), file);
				set_modelentity(
						g_env.renderer,
						node->impl_handle,
						(Model*)res_by_name(
							g_env.res_blob,
							ResType_Model,
							dead_impl.model_name));
				ModelEntity *impl= &g_env.renderer->entities[node->impl_handle];
				impl->pos= dead_impl.pos;
				impl->rot= dead_impl.rot;
			}
			break;
			case NodeType_T3d: {
				T3d dead_impl;
				fread(&dead_impl, 1, sizeof(dead_impl), file);
				T3d *impl= &temptest_t3d_storage[node->impl_handle];
				*impl= dead_impl;
			 }
			break;
			case NodeType_RigidBody: {
				RigidBody dead_impl;
				fread(&dead_impl, 1, sizeof(dead_impl), file);
				set_rigidbody(
						g_env.phys_world,
						node->impl_handle,
						(V2d) {dead_impl.pos.x, dead_impl.pos.y},
						rot_z_qd(dead_impl.rot),
						(RigidBodyDef*)res_by_name(
							g_env.res_blob,
							ResType_RigidBodyDef,
							dead_impl.def_name));
			}
			break;
			default: fail("load_world: Unhandled type: %i", node->type);
		}
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
		case NodeType_RigidBody:
			info.impl_handle= alloc_rigidbody(g_env.phys_world);
		break;
		default: fail("create_node: Unhandled type: %i", type);
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
		case NodeType_RigidBody:
			free_rigidbody(g_env.phys_world, impl_handle);
		break;
		default: fail("destroy_node: Unhandled type: %i", type);
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
