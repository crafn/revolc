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
					"rot" : [ 0, 0, 1, 0.912025,],
					"pos" : [ 0.0266398, -0.00890695, 0,],
				}
			},
			{
				"name" : "ubody",
				"super" : "lbody",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 0.144343,],
					"pos" : [ 0.239605, 0.0966829, 0,],
				}
			},
			{
				"name" : "head",
				"super" : "ubody",
				"offset" : {
					"scale" : [ 1, 1, 1,],
					"rot" : [ 0, 0, -1, 0.346198,],
					"pos" : [ 0.299436, 0.0261988, 0,],
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
		"pos" : [[-0.282539, -0.336717, 0,], [-0.102059, 0.33558, 0,], [0.498949, 0.146529, 0,], [0.373269, -0.406827, 0,],],          
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
		"pos" : [[-0.424445, -0.0300199, 0,], [-0.164716, 0.645708, 0,], [0.596437, 0.150133, 0,], [0.258747, -0.557281, 0,],],          
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
