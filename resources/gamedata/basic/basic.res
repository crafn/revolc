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
		"type" : "NodeType",
		"name" : "ModelEntity",
		"init_func" : "init_modelentity",
		"resurrect_func" : "resurrect_modelentity",
		"free_func" : "free_modelentity",
		"upd_func" : "",
		"storage_func" : "storage_modelentity",
	},
	{
		"type" : "NodeType",
		"name" : "RigidBody",
		"init_func" : "",
		"resurrect_func" : "resurrect_rigidbody",
		"free_func" : "free_rigidbody",
		"upd_func" : "",
		"storage_func" : "storage_rigidbody",
	},
]
