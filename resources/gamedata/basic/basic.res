[
	{
		"type" : "Texture",
		"name" : "white",
		"file" : "./white.png",
	},
	{
		"type" : "Mesh",
		"name" : "unitquad",
		"pos" : [	[-1.0, -1.0, 0.0], 
					[-1.0,  1.0, 0.0],
					[ 1.0,  1.0, 0.0],
					[ 1.0, -1.0, 0.0], ],
		"uv" : [	[0.0, 0.0],
					[0.0, 1.0],
					[1.0, 1.0],
					[1.0, 0.0], ],
		"ind" : [0, 1, 2,  0, 2, 3],
	},
	{
		"type" : "Mesh",
		"name" : "halfquad",
		"pos" : [	[-0.5, -0.5, 0.0], 
					[-0.5,  0.5, 0.0],
					[ 0.5,  0.5, 0.0],
					[ 0.5, -0.5, 0.0], ],
		"uv" : [	[0.0, 0.0],
					[0.0, 1.0],
					[1.0, 1.0],
					[1.0, 0.0], ],
		"ind" : [0, 1, 2,  0, 2, 3],
	},
	{
		"type" : "ShaderSource",
		"name" : "gen",
		"vs_file" : "./gen.vs",
		"fs_file" : "./gen.fs",
	},
	{
		"type" : "ShaderSource",
		"name" : "grid_ddraw",
		"vs_file" : "./grid_ddraw.vs",
		"fs_file" : "./grid_ddraw.fs",
	},
	{
		"type" : "NodeType",
		"name" : "ModelEntity",
		"impl_mgmt" : "manual",

		"init_func" : "init_modelentity",
		"resurrect_func" : "resurrect_modelentity",
		"free_func" : "free_modelentity",
		"upd_func" : "",
		"storage_func" : "storage_modelentity",
	},
	{
		"type" : "NodeType",
		"name" : "CompEntity",
		"impl_mgmt" : "manual",

		"init_func" : "init_compentity",
		"resurrect_func" : "resurrect_compentity",
		"free_func" : "free_compentity",
		"upd_func" : "",
		"storage_func" : "storage_compentity",
	},
	{
		"type" : "NodeType",
		"name" : "RigidBody",
		"impl_mgmt" : "manual",

		"init_func" : "",
		"resurrect_func" : "resurrect_rigidbody",
		"free_func" : "free_rigidbody",
		"upd_func" : "",
		"storage_func" : "storage_rigidbody",
	},
	{
		"type" : "NodeType",
		"name" : "ClipInst",
		"impl_mgmt" : "auto",
		"max_count" : 128,

		"init_func" : "init_clipinst",
		"resurrect_func" : "resurrect_clipinst",
		"upd_func" : "upd_clipinst",
		"free_func" : "",
	},
	{
		"type" : "Module",
		"name" : "main_prog",
		"file" : ""
	}
]
