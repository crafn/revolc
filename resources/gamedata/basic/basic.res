[
	{
		"type" : "Texture",
		"name" : "white",
		"file" : "./white.png",
	},
	{
		"type" : "Mesh",
		"name" : "quad",
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
		"type" : "ShaderSource",
		"name" : "gen",
		"vs_file" : "./gen.vs",
		"fs_file" : "./gen.fs",
	},
	{
		"type" : "NodeType",
		"name" : "ModelEntity",
		"alloc_func" : "alloc_modelentity",
		"free_func" : "free_modelentity",
		"upd_func" : "",
		"storage_func" : "storage_modelentity",
		"resurrect_func" : "resurrect_modelentity",
	},
	{
		"type" : "NodeType",
		"name" : "RigidBody",
		"alloc_func" : "alloc_rigidbody",
		"free_func" : "free_rigidbody",
		"upd_func" : "",
		"storage_func" : "storage_rigidbody",
		"resurrect_func" : "resurrect_rigidbody",
	},
]
