#include "generated_rtti.h"

const U32 V3d_size= sizeof(V3d);
const char *V3d_member_names[]= {
	"x",
	"y",
	"z",
	NULL
};
const U32 V3d_member_sizes[]= {8, 8, 8, };
const U32 V3d_member_offsets[]= {
	offsetof(V3d, x),
	offsetof(V3d, y),
	offsetof(V3d, z),
};
const U32 V2d_size= sizeof(V2d);
const char *V2d_member_names[]= {
	"x",
	"y",
	NULL
};
const U32 V2d_member_sizes[]= {8, 8, };
const U32 V2d_member_offsets[]= {
	offsetof(V2d, x),
	offsetof(V2d, y),
};
const U32 Color_size= sizeof(Color);
const char *Color_member_names[]= {
	"r",
	"g",
	"b",
	"a",
	NULL
};
const U32 Color_member_sizes[]= {4, 4, 4, 4, };
const U32 Color_member_offsets[]= {
	offsetof(Color, r),
	offsetof(Color, g),
	offsetof(Color, b),
	offsetof(Color, a),
};
const U32 V3f_size= sizeof(V3f);
const char *V3f_member_names[]= {
	"x",
	"y",
	"z",
	NULL
};
const U32 V3f_member_sizes[]= {4, 4, 4, };
const U32 V3f_member_offsets[]= {
	offsetof(V3f, x),
	offsetof(V3f, y),
	offsetof(V3f, z),
};
const U32 V2f_size= sizeof(V2f);
const char *V2f_member_names[]= {
	"x",
	"y",
	NULL
};
const U32 V2f_member_sizes[]= {4, 4, };
const U32 V2f_member_offsets[]= {
	offsetof(V2f, x),
	offsetof(V2f, y),
};
const U32 V2i_size= sizeof(V2i);
const char *V2i_member_names[]= {
	"x",
	"y",
	NULL
};
const U32 V2i_member_sizes[]= {4, 4, };
const U32 V2i_member_offsets[]= {
	offsetof(V2i, x),
	offsetof(V2i, y),
};
const U32 JsonTok_size= sizeof(JsonTok);
const char *JsonTok_member_names[]= {
	"json_path",
	"json",
	"tok",
	NULL
};
const U32 JsonTok_member_sizes[]= {8, 8, 8, };
const U32 JsonTok_member_offsets[]= {
	offsetof(JsonTok, json_path),
	offsetof(JsonTok, json),
	offsetof(JsonTok, tok),
};
const U32 M44f_size= sizeof(M44f);
const char *M44f_member_names[]= {
	"e",
	NULL
};
const U32 M44f_member_sizes[]= {64, };
const U32 M44f_member_offsets[]= {
	offsetof(M44f, e),
};
const U32 AiTest_size= sizeof(AiTest);
const char *AiTest_member_names[]= {
	"input_pos",
	"force",
	"allocated",
	NULL
};
const U32 AiTest_member_sizes[]= {16, 16, 1, };
const U32 AiTest_member_offsets[]= {
	offsetof(AiTest, input_pos),
	offsetof(AiTest, force),
	offsetof(AiTest, allocated),
};
const U32 World_size= sizeof(World);
const char *World_member_names[]= {
	"nodes",
	"next_node",
	"node_count",
	"sort_space",
	NULL
};
const U32 World_member_sizes[]= {5734400, 4, 4, 5734400, };
const U32 World_member_offsets[]= {
	offsetof(World, nodes),
	offsetof(World, next_node),
	offsetof(World, node_count),
	offsetof(World, sort_space),
};
const U32 Qd_size= sizeof(Qd);
const char *Qd_member_names[]= {
	"cs",
	"sn",
	NULL
};
const U32 Qd_member_sizes[]= {8, 8, };
const U32 Qd_member_offsets[]= {
	offsetof(Qd, cs),
	offsetof(Qd, sn),
};
const U32 ResBlob_size= sizeof(ResBlob);
const char *ResBlob_member_names[]= {
	"version",
	"res_count",
	"first_missing_res",
	"res_offsets",
	NULL
};
const U32 ResBlob_member_sizes[]= {4, 4, 8, 1, };
const U32 ResBlob_member_offsets[]= {
	offsetof(ResBlob, version),
	offsetof(ResBlob, res_count),
	offsetof(ResBlob, first_missing_res),
	offsetof(ResBlob, res_offsets),
};
const U32 BlobBuf_size= sizeof(BlobBuf);
const char *BlobBuf_member_names[]= {
	"data",
	"offset",
	"is_file",
	NULL
};
const U32 BlobBuf_member_sizes[]= {8, 4, 1, };
const U32 BlobBuf_member_offsets[]= {
	offsetof(BlobBuf, data),
	offsetof(BlobBuf, offset),
	offsetof(BlobBuf, is_file),
};
const U32 Resource_size= sizeof(Resource);
const char *Resource_member_names[]= {
	"type",
	"name",
	"blob",
	"is_missing_res",
	NULL
};
const U32 Resource_member_sizes[]= {4, 16, 8, 1, };
const U32 Resource_member_offsets[]= {
	offsetof(Resource, type),
	offsetof(Resource, name),
	offsetof(Resource, blob),
	offsetof(Resource, is_missing_res),
};
const U32 MissingResource_size= sizeof(MissingResource);
const char *MissingResource_member_names[]= {
	"res",
	"next",
	NULL
};
const U32 MissingResource_member_sizes[]= {8, 8, };
const U32 MissingResource_member_offsets[]= {
	offsetof(MissingResource, res),
	offsetof(MissingResource, next),
};
const U32 NodeType_size= sizeof(NodeType);
const char *NodeType_member_names[]= {
	"res",
	"alloc_func_name",
	"free_func_name",
	"upd_func_name",
	"storage_func_name",
	"resurrect_func_name",
	"alloc",
	"free",
	"storage",
	"upd",
	"resurrect",
	"size",
	NULL
};
const U32 NodeType_member_sizes[]= {40, 64, 64, 64, 64, 64, 8, 8, 8, 8, 8, 4, };
const U32 NodeType_member_offsets[]= {
	offsetof(NodeType, res),
	offsetof(NodeType, alloc_func_name),
	offsetof(NodeType, free_func_name),
	offsetof(NodeType, upd_func_name),
	offsetof(NodeType, storage_func_name),
	offsetof(NodeType, resurrect_func_name),
	offsetof(NodeType, alloc),
	offsetof(NodeType, free),
	offsetof(NodeType, storage),
	offsetof(NodeType, upd),
	offsetof(NodeType, resurrect),
	offsetof(NodeType, size),
};
const U32 Texel_size= sizeof(Texel);
const char *Texel_member_names[]= {
	"r",
	"g",
	"b",
	"a",
	NULL
};
const U32 Texel_member_sizes[]= {1, 1, 1, 1, };
const U32 Texel_member_offsets[]= {
	offsetof(Texel, r),
	offsetof(Texel, g),
	offsetof(Texel, b),
	offsetof(Texel, a),
};
const U32 Texture_size= sizeof(Texture);
const char *Texture_member_names[]= {
	"res",
	"reso",
	"atlas_uv",
	"texels",
	NULL
};
const U32 Texture_member_sizes[]= {40, 8, 12, 1, };
const U32 Texture_member_offsets[]= {
	offsetof(Texture, res),
	offsetof(Texture, reso),
	offsetof(Texture, atlas_uv),
	offsetof(Texture, texels),
};
const U32 VertexAttrib_size= sizeof(VertexAttrib);
const char *VertexAttrib_member_names[]= {
	"name",
	"size",
	"type",
	"normalized",
	"offset",
	NULL
};
const U32 VertexAttrib_member_sizes[]= {8, 4, 4, 1, 8, };
const U32 VertexAttrib_member_offsets[]= {
	offsetof(VertexAttrib, name),
	offsetof(VertexAttrib, size),
	offsetof(VertexAttrib, type),
	offsetof(VertexAttrib, normalized),
	offsetof(VertexAttrib, offset),
};
const U32 TriMeshVertex_size= sizeof(TriMeshVertex);
const char *TriMeshVertex_member_names[]= {
	"pos",
	"uv",
	"color",
	"normal",
	"tangent",
	NULL
};
const U32 TriMeshVertex_member_sizes[]= {12, 12, 16, 12, 12, };
const U32 TriMeshVertex_member_offsets[]= {
	offsetof(TriMeshVertex, pos),
	offsetof(TriMeshVertex, uv),
	offsetof(TriMeshVertex, color),
	offsetof(TriMeshVertex, normal),
	offsetof(TriMeshVertex, tangent),
};
const U32 Mesh_size= sizeof(Mesh);
const char *Mesh_member_names[]= {
	"res",
	"mesh_type",
	"v_count",
	"i_count",
	"v_offset",
	"i_offset",
	NULL
};
const U32 Mesh_member_sizes[]= {40, 4, 4, 4, 8, 8, };
const U32 Mesh_member_offsets[]= {
	offsetof(Mesh, res),
	offsetof(Mesh, mesh_type),
	offsetof(Mesh, v_count),
	offsetof(Mesh, i_count),
	offsetof(Mesh, v_offset),
	offsetof(Mesh, i_offset),
};
const U32 Model_size= sizeof(Model);
const char *Model_member_names[]= {
	"res",
	"textures",
	"mesh",
	NULL
};
const U32 Model_member_sizes[]= {40, 48, 16, };
const U32 Model_member_offsets[]= {
	offsetof(Model, res),
	offsetof(Model, textures),
	offsetof(Model, mesh),
};
const U32 ModelEntity_size= sizeof(ModelEntity);
const char *ModelEntity_member_names[]= {
	"allocated",
	"pos",
	"rot",
	"model_name",
	"atlas_uv",
	"scale_to_atlas_uv",
	"vertices",
	"indices",
	"mesh_v_count",
	"mesh_i_count",
	NULL
};
const U32 ModelEntity_member_sizes[]= {1, 24, 16, 16, 12, 8, 8, 8, 4, 4, };
const U32 ModelEntity_member_offsets[]= {
	offsetof(ModelEntity, allocated),
	offsetof(ModelEntity, pos),
	offsetof(ModelEntity, rot),
	offsetof(ModelEntity, model_name),
	offsetof(ModelEntity, atlas_uv),
	offsetof(ModelEntity, scale_to_atlas_uv),
	offsetof(ModelEntity, vertices),
	offsetof(ModelEntity, indices),
	offsetof(ModelEntity, mesh_v_count),
	offsetof(ModelEntity, mesh_i_count),
};
const U32 SlotRouting_size= sizeof(SlotRouting);
const char *SlotRouting_member_names[]= {
	"allocated",
	"src_offset",
	"dst_offset",
	"size",
	"dst_node",
	NULL
};
const U32 SlotRouting_member_sizes[]= {1, 1, 1, 1, 4, };
const U32 SlotRouting_member_offsets[]= {
	offsetof(SlotRouting, allocated),
	offsetof(SlotRouting, src_offset),
	offsetof(SlotRouting, dst_offset),
	offsetof(SlotRouting, size),
	offsetof(SlotRouting, dst_node),
};
const U32 NodeInfo_size= sizeof(NodeInfo);
const char *NodeInfo_member_names[]= {
	"allocated",
	"type",
	"type_name",
	"impl_handle",
	"routing",
	"group_id",
	NULL
};
const U32 NodeInfo_member_sizes[]= {1, 8, 16, 4, 64, 8, };
const U32 NodeInfo_member_offsets[]= {
	offsetof(NodeInfo, allocated),
	offsetof(NodeInfo, type),
	offsetof(NodeInfo, type_name),
	offsetof(NodeInfo, impl_handle),
	offsetof(NodeInfo, routing),
	offsetof(NodeInfo, group_id),
};
const U32 NodeGroupDef_size= sizeof(NodeGroupDef);
const char *NodeGroupDef_member_names[]= {
	"res",
	"nodes",
	"node_count",
	NULL
};
const U32 NodeGroupDef_member_sizes[]= {40, 4224, 4, };
const U32 NodeGroupDef_member_offsets[]= {
	offsetof(NodeGroupDef, res),
	offsetof(NodeGroupDef, nodes),
	offsetof(NodeGroupDef, node_count),
};
const U32 Device_size= sizeof(Device);
const char *Device_member_names[]= {
	"cursor_pos",
	"win_size",
	"quit_requested",
	"dt",
	"lmb_down",
	"key_down",
	"key_pressed",
	"key_released",
	"data",
	NULL
};
const U32 Device_member_sizes[]= {8, 8, 1, 4, 1, 32, 32, 32, 8, };
const U32 Device_member_offsets[]= {
	offsetof(Device, cursor_pos),
	offsetof(Device, win_size),
	offsetof(Device, quit_requested),
	offsetof(Device, dt),
	offsetof(Device, lmb_down),
	offsetof(Device, key_down),
	offsetof(Device, key_pressed),
	offsetof(Device, key_released),
	offsetof(Device, data),
};
const U32 PhysWorld_size= sizeof(PhysWorld);
const char *PhysWorld_member_names[]= {
	"debug_draw",
	"bodies",
	"next_body",
	"body_count",
	"space",
	"ground",
	NULL
};
const U32 PhysWorld_member_sizes[]= {1, 1310720, 4, 4, 8, 8, };
const U32 PhysWorld_member_offsets[]= {
	offsetof(PhysWorld, debug_draw),
	offsetof(PhysWorld, bodies),
	offsetof(PhysWorld, next_body),
	offsetof(PhysWorld, body_count),
	offsetof(PhysWorld, space),
	offsetof(PhysWorld, ground),
};
const U32 Renderer_size= sizeof(Renderer);
const char *Renderer_member_names[]= {
	"cam_pos",
	"cam_fov",
	"entities",
	"next_entity",
	"entity_count",
	"ddraw_v",
	"ddraw_i",
	"ddraw_v_count",
	"ddraw_i_count",
	"atlas_gl_id",
	"sort_space",
	NULL
};
const U32 Renderer_member_sizes[]= {24, 4, 5734400, 4, 4, 3276800, 614400, 4, 4, 4, 5734400, };
const U32 Renderer_member_offsets[]= {
	offsetof(Renderer, cam_pos),
	offsetof(Renderer, cam_fov),
	offsetof(Renderer, entities),
	offsetof(Renderer, next_entity),
	offsetof(Renderer, entity_count),
	offsetof(Renderer, ddraw_v),
	offsetof(Renderer, ddraw_i),
	offsetof(Renderer, ddraw_v_count),
	offsetof(Renderer, ddraw_i_count),
	offsetof(Renderer, atlas_gl_id),
	offsetof(Renderer, sort_space),
};
const U32 Env_size= sizeof(Env);
const char *Env_member_names[]= {
	"device",
	"phys_world",
	"renderer",
	"res_blob",
	"world",
	NULL
};
const U32 Env_member_sizes[]= {8, 8, 8, 8, 8, };
const U32 Env_member_offsets[]= {
	offsetof(Env, device),
	offsetof(Env, phys_world),
	offsetof(Env, renderer),
	offsetof(Env, res_blob),
	offsetof(Env, world),
};
const U32 SaveHeader_size= sizeof(SaveHeader);
const char *SaveHeader_member_names[]= {
	"version",
	"node_count",
	NULL
};
const U32 SaveHeader_member_sizes[]= {4, 4, };
const U32 SaveHeader_member_offsets[]= {
	offsetof(SaveHeader, version),
	offsetof(SaveHeader, node_count),
};
const U32 Circle_size= sizeof(Circle);
const char *Circle_member_names[]= {
	"pos",
	"rad",
	NULL
};
const U32 Circle_member_sizes[]= {16, 8, };
const U32 Circle_member_offsets[]= {
	offsetof(Circle, pos),
	offsetof(Circle, rad),
};
const U32 Poly_size= sizeof(Poly);
const char *Poly_member_names[]= {
	"v",
	"v_count",
	NULL
};
const U32 Poly_member_sizes[]= {128, 4, };
const U32 Poly_member_offsets[]= {
	offsetof(Poly, v),
	offsetof(Poly, v_count),
};
const U32 RigidBodyDef_size= sizeof(RigidBodyDef);
const char *RigidBodyDef_member_names[]= {
	"res",
	"circles",
	"circle_count",
	"polys",
	"poly_count",
	NULL
};
const U32 RigidBodyDef_member_sizes[]= {40, 96, 4, 544, 4, };
const U32 RigidBodyDef_member_offsets[]= {
	offsetof(RigidBodyDef, res),
	offsetof(RigidBodyDef, circles),
	offsetof(RigidBodyDef, circle_count),
	offsetof(RigidBodyDef, polys),
	offsetof(RigidBodyDef, poly_count),
};
const U32 RigidBody_size= sizeof(RigidBody);
const char *RigidBody_member_names[]= {
	"allocated",
	"input_force",
	"def_name",
	"pos",
	"rot",
	"cp_shapes",
	"cp_shape_count",
	"cp_body",
	NULL
};
const U32 RigidBody_member_sizes[]= {1, 16, 16, 24, 16, 32, 1, 8, };
const U32 RigidBody_member_offsets[]= {
	offsetof(RigidBody, allocated),
	offsetof(RigidBody, input_force),
	offsetof(RigidBody, def_name),
	offsetof(RigidBody, pos),
	offsetof(RigidBody, rot),
	offsetof(RigidBody, cp_shapes),
	offsetof(RigidBody, cp_shape_count),
	offsetof(RigidBody, cp_body),
};
const U32 DevicePlatformData_size= sizeof(DevicePlatformData);
const char *DevicePlatformData_member_names[]= {
	"dpy",
	"win",
	"ctx",
	"ts",
	NULL
};
const U32 DevicePlatformData_member_sizes[]= {8, 8, 8, 16, };
const U32 DevicePlatformData_member_offsets[]= {
	offsetof(DevicePlatformData, dpy),
	offsetof(DevicePlatformData, win),
	offsetof(DevicePlatformData, ctx),
	offsetof(DevicePlatformData, ts),
};
const U32 ShaderSource_size= sizeof(ShaderSource);
const char *ShaderSource_member_names[]= {
	"res",
	"vs_src_offset",
	"gs_src_offset",
	"fs_src_offset",
	"mesh_type",
	"vs_gl_id",
	"gs_gl_id",
	"fs_gl_id",
	"prog_gl_id",
	NULL
};
const U32 ShaderSource_member_sizes[]= {40, 8, 8, 8, 4, 4, 4, 4, 4, };
const U32 ShaderSource_member_offsets[]= {
	offsetof(ShaderSource, res),
	offsetof(ShaderSource, vs_src_offset),
	offsetof(ShaderSource, gs_src_offset),
	offsetof(ShaderSource, fs_src_offset),
	offsetof(ShaderSource, mesh_type),
	offsetof(ShaderSource, vs_gl_id),
	offsetof(ShaderSource, gs_gl_id),
	offsetof(ShaderSource, fs_gl_id),
	offsetof(ShaderSource, prog_gl_id),
};
const U32 ParsedJsonFile_size= sizeof(ParsedJsonFile);
const char *ParsedJsonFile_member_names[]= {
	"tokens",
	"json",
	"root",
	NULL
};
const U32 ParsedJsonFile_member_sizes[]= {8, 8, 24, };
const U32 ParsedJsonFile_member_offsets[]= {
	offsetof(ParsedJsonFile, tokens),
	offsetof(ParsedJsonFile, json),
	offsetof(ParsedJsonFile, root),
};
const U32 ResInfo_size= sizeof(ResInfo);
const char *ResInfo_member_names[]= {
	"header",
	"tok",
	NULL
};
const U32 ResInfo_member_sizes[]= {40, 24, };
const U32 ResInfo_member_offsets[]= {
	offsetof(ResInfo, header),
	offsetof(ResInfo, tok),
};
const U32 Vao_size= sizeof(Vao);
const char *Vao_member_names[]= {
	"vao_id",
	"vbo_id",
	"ibo_id",
	"mesh_type",
	"v_count",
	"v_capacity",
	"v_size",
	"i_count",
	"i_capacity",
	NULL
};
const U32 Vao_member_sizes[]= {4, 4, 4, 4, 4, 4, 4, 4, 4, };
const U32 Vao_member_offsets[]= {
	offsetof(Vao, vao_id),
	offsetof(Vao, vbo_id),
	offsetof(Vao, ibo_id),
	offsetof(Vao, mesh_type),
	offsetof(Vao, v_count),
	offsetof(Vao, v_capacity),
	offsetof(Vao, v_size),
	offsetof(Vao, i_count),
	offsetof(Vao, i_capacity),
};
const U32 TexInfo_size= sizeof(TexInfo);
const char *TexInfo_member_names[]= {
	"tex",
	"reso",
	"atlas_uv",
	"texels",
	NULL
};
const U32 TexInfo_member_sizes[]= {8, 8, 8, 8, };
const U32 TexInfo_member_offsets[]= {
	offsetof(TexInfo, tex),
	offsetof(TexInfo, reso),
	offsetof(TexInfo, atlas_uv),
	offsetof(TexInfo, texels),
};
