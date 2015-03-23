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
					"rot" : [ 0, 0, 1, 0.509063,],
					"pos" : [ 0.0266398, -0.00890695, 0,],
				}
			},
			{
				"name" : "ubody",
				"super" : "lbody",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 0.0755983,],
					"pos" : [ 0.249917, 0.0837621, 0,],
				}
			},
			{
				"name" : "head",
				"super" : "ubody",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 0.606361,],
					"pos" : [ 0.308342, 0.0132817, 0,],
				}
			},

			{
				"name" : "r_arm",
				"super" : "ubody",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 1.60076,],
					"pos" : [ 0.151786, -0.06603, 0,],
				}
			},
			{
				"name" : "r_frm",
				"super" : "r_arm",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, 1, 1.7987,],
					"pos" : [ 0.398654, -0.0617989, 0,],
				}
			},
			{
				"name" : "r_hnd",
				"super" : "r_frm",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 0.683367,],
					"pos" : [ 0.257817, -0.0396217, 0,],
				}
			},

			{
				"name" : "r_thg",
				"super" : "lbody",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 1.23494,],
					"pos" : [ -0.0837665, -0.14209, 0,],
				}
			},
			{
				"name" : "r_leg",
				"super" : "r_thg",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 1.87576,],
					"pos" : [ 0.2786, -0.0155363, 0,],
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

			{
				"entity" : "playerch_r_arm",
				"joint" : "r_arm",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
			{
				"entity" : "playerch_r_frm",
				"joint" : "r_frm",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
			{
				"entity" : "playerch_r_hnd",
				"joint" : "r_hnd",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},

			{
				"entity" : "playerch_r_thg",
				"joint" : "r_thg",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
			{
				"entity" : "playerch_r_leg",
				"joint" : "r_leg",
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
		"pos" : [[-0.778737, -0.0923023, 0,], [-0.296338, 1.06002, 0,], [0.501087, 0.446215, 0,], [0.26322, -0.404482, 0,],],          
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
		"pos" : [[-0.297747, -0.33286, 0,], [-0.117267, 0.339437, 0,], [0.483741, 0.150386, 0,], [0.358061, -0.40297, 0,],],          
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
		"pos" : [[-0.399083, -0.147606, 0,], [-0.338843, 0.573808, 0,], [0.449763, 0.362903, 0,], [0.323545, -0.41075, 0,],],          
		"uv" : [[0.6275, 0.64, 0,], [0.6275, 0.9725, 0,], [0.98, 0.975, 0,], [1, 0.65, 0,],],          
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
		"type" : "Mesh",
		"name" : "playerch_r_arm",
		"pos" : [[-0.171912, -0.12409, 0,], [-0.210757, 0.206426, 0,], [0.648521, 0.00426166, 0,], [0.447001, -0.294498, 0,],],          
		"uv" : [[0.49, 0.6525, 0,], [0.5775, 0.6875, 0,], [0.655, 0.4925, 0,], [0.5575, 0.4375, 0,],],          
		"ind" : [0, 1, 2, 0, 2, 3,],
	},
	{
		"type" : "Model",
		"name" : "playerch_r_arm",
		"mesh" : "playerch_r_arm",
		"textures" : ["playerch"],
		"color" : [1, 1, 1, 1,],
	},

	{
		"type" : "Mesh",
		"name" : "playerch_r_frm",
		"pos" : [[-0.210718, -0.169424, 0,], [-0.091628, 0.139495, 0,], [0.39886, 0.105126, 0,], [0.412674, -0.133476, 0,],],          
		"uv" : [[0.5525, 0.3125, 0,], [0.55, 0.455001, 0,], [0.8175, 0.53, 0,], [0.835, 0.395, 0,],],          
		"ind" : [0, 1, 2, 0, 2, 3,],
	},
	{
		"type" : "Model",
		"name" : "playerch_r_frm",
		"mesh" : "playerch_r_frm",
		"textures" : ["playerch"],
		"color" : [1, 1, 1, 1,],
	},

	{
		"type" : "Mesh",
		"name" : "playerch_r_hnd",
		"pos" : [[-0.0502002, -0.161181, 0,], [-0.100248, 0.223665, 0,], [0.209456, 0.174018, 0,], [0.175765, -0.193925, 0,],],          
		"uv" : [[0.835, 0.3975, 0,], [0.8225, 0.545, 0,], [0.9975, 0.545, 0,], [0.9775, 0.3525, 0,],],          
		"ind" : [0, 1, 2, 0, 2, 3,],
	},
	{
		"type" : "Model",
		"name" : "playerch_r_hnd",
		"mesh" : "playerch_r_hnd",
		"textures" : ["playerch"],
		"color" : [1, 1, 1, 1,],
	},

	{
		"type" : "Mesh",
		"name" : "playerch_r_thg",
		"pos" : [[-0.0630128, -0.139302, 0,], [-0.0907968, 0.146211, 0,], [0.364702, 0.0879109, 0,], [0.391701, -0.155639, 0,],],          
		"uv" : [[0.115, 0.45, 0,], [0.125, 0.5675, 0,], [0.3375, 0.5275, 0,], [0.325, 0.4175, 0,],],          
		"ind" : [0, 1, 2, 0, 2, 3,],
	},
	{
		"type" : "Model",
		"name" : "playerch_r_thg",
		"mesh" : "playerch_r_thg",
		"textures" : ["playerch"],
		"color" : [1, 1, 1, 1,],
	},

	{
		"type" : "Mesh",
		"name" : "playerch_r_leg",
		"pos" : [[-0.141448, -0.0710303, 0,], [-0.0726191, 0.167753, 0,], [0.762413, 0.145966, 0,], [0.747751, -0.232937, 0,],],          
		"uv" : [[0.2825, 0.4125, 0,], [0.3875, 0.3725, 0,], [0.2325, 0.0325001, 0,], [0.1025, 0.1225, 0,],],          
		"ind" : [0, 1, 2, 0, 2, 3,],
	},
	{
		"type" : "Model",
		"name" : "playerch_r_leg",
		"mesh" : "playerch_r_leg",
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
