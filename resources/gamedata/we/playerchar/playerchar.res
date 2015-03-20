[
	{
		"type" : "Armature",
		"name" : "playerch",
		"joints" : [
			{
				"name" : "lbody",
				"super" : "",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 0.260605,],
					"pos" : [ 0.00569886, -0.0372309, 0,],
				}
			},
			{
				"name" : "ubody",
				"super" : "lbody",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, 1, 0.702221,],
					"pos" : [ 0.201401, 0.203326, 0,],
				}
			},
			{
				"name" : "head",
				"super" : "ubody",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 0.271244,],
					"pos" : [ 0.216078, 0.0604476, 0,],
				}
			},
		],
	},
	{
		"type" : "CompDef",
		"name" : "playerch",
		"armature" : "playerch",
		"subs" : [
			{
				"entity" : "playerch_lbody",
				"joint" : "lbody",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
			{
				"entity" : "playerch_ubody",
				"joint" : "ubody",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
			{
				"entity" : "playerch_head",
				"joint" : "head",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
		],
	},

	{
		"type" : "Mesh",
		"name" : "playerch_lbody",
		"pos" : [[-0.372735, -0.869139, 0,], [-1.06127, 0.266171, 0,], [0.0626069, 0.6077, 0,], [0.581786, -0.13954, 0,],],          
		"uv" : [[0, 0.5825, 0,], [0.0025, 0.9375, 0,], [0.3625, 0.93, 0,], [0.4125, 0.625, 0,],],          
		"ind" : [0, 1, 2, 0, 2, 3,],
	},
	{
		"type" : "Model",
		"name" : "playerch_lbody",
		"mesh" : "playerch_lbody",
		"textures" : ["playerch"],
		"color" : [1, 1, 1, 1,],
	},

	{
		"type" : "Mesh",
		"name" : "playerch_ubody",
		"pos" : [[-0.282162, -0.407658, 0,], [-0.258846, 0.288052, 0,], [0.369376, 0.240221, 0,], [0.372459, -0.327219, 0,],],          
		"uv" : [[0.375, 0.6425, 0,], [0.405, 0.9, 0,], [0.5975, 0.9225, 0,], [0.6325, 0.7225, 0,],],          
		"ind" : [0, 1, 2, 0, 2, 3,],
	},
	{
		"type" : "Model",
		"name" : "playerch_ubody",
		"mesh" : "playerch_ubody",
		"textures" : ["playerch"],
		"color" : [1, 1, 1, 1,],
	},

	{
		"type" : "Mesh",
		"name" : "playerch_head",
		"pos" : [[-0.424445, -0.0300199, 0,], [-0.200212, 0.705004, 0,], [0.665099, 0.270719, 0,], [0.353257, -0.564259, 0,],],          
		"uv" : [[0.63, 0.6675, 0,], [0.5975, 0.97, 0,], [1, 0.925, 0,], [1, 0.6225, 0,],],          
		"ind" : [0, 1, 2, 0, 2, 3,],
	},
	{
		"type" : "Model",
		"name" : "playerch_head",
		"mesh" : "playerch_head",
		"textures" : ["playerch"],
		"color" : [1, 1, 1, 1,],
	},

	{
		"type" : "Texture",
		"name" : "playerch",
		"file" : "./tex.png",
	},
	{
		"type" : "NodeType",
		"name" : "PlayerCh",
		"impl_mgmt" : "auto",
		"max_count" : 4,

		"init_func" : "",
		"resurrect_func" : "resurrect_playerch",
		"upd_func" : "upd_playerch",
		"free_func" : "free_playerch",
	},
	{
		"type" : "PhysMat",
		"name" : "playerch_legs",

		"density" : 2.0,
		"friction" : 5.0,
		"restitution" : 0.0,
	},
	{
		"type" : "RigidBodyDef",
		"name" : "playerch_legs",
		"mat" : "playerch_legs",
		"shapes" : [
			{
				"type" : "Circle",
				"pos" : [0.0, 0.0],
				"rad" : 0.45,
			},
		]
	},
	{
		"type" : "NodeGroupDef",
		"name" : "playerch",
		"nodes" : [
			{
				"type" : "RigidBody",
				"name" : "body",
				"defaults" : [
					{ "def_name" : "playerch_legs" },
				],
			},
			{
				"type" : "PlayerCh",
				"name" : "char",
				"defaults" : [ ],
			},
			{
				"type" : "CompEntity",
				"name" : "visual",
				"defaults" : [
					{ "def_name" : "playerch" },
				],
			},
		],
		"cmds" : [
			"supply_playerch(char, body)",
			"visual.tf.pos= char.pos",
			"visual.tf.rot= body.tf.rot",
		]
	},
]
