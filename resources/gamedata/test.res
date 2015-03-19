[
	{
		"type" : "Texture",
		"name" : "wbox_c",
		"file" : "./we/woodenbox_color.png",
	},
	{
		"type" : "Texture",
		"name" : "bg_cloud0",
		"file" : "./we/bg_cloud0.png",
	},
	{
		"type" : "Texture",
		"name" : "bg_meadow0",
		"file" : "./we/bg_meadow0.png",
	},
	{
		"type" : "Texture",
		"name" : "stone0",
		"file" : "./we/stone0.png",
	},
	{
		"type" : "Model",
		"name" : "wbox",
		"mesh" : "halfquad",
		"textures" : ["wbox_c"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},
	{
		"type" : "Model",
		"name" : "stone0",
		"mesh" : "halfquad",
		"textures" : ["stone0"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},

	{
		"type" : "Sound",
		"name" : "dev_beep0",
		"file" : "./dev_beep0.ogg",
	},
	{
		"type" : "Sound",
		"name" : "dev_beep1",
		"file" : "./dev_beep1.ogg",
	},


	{
		"type" : "Texture",
		"name" : "sky_day",
		"file" : "./we/day_test.png",
	},
	{
		"type" : "Model",
		"name" : "sky_day",
		"mesh" : "unitquad",
		"textures" : ["sky_day"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},


	{
		"type" : "Texture",
		"name" : "bg_mountain",
		"file" : "./we/bg_mountain0.png",
	},
	{
		"type" : "Model",
		"name" : "bg_mountain",
		"mesh" : "unitquad",
		"textures" : ["bg_mountain"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},


	{
		"type" : "Texture",
		"name" : "bg_meadow",
		"file" : "./we/bg_meadow0.png",
	},
	{
		"type" : "Model",
		"name" : "bg_meadow",
		"mesh" : "unitquad",
		"textures" : ["bg_meadow"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},

	{
		"type" : "Texture",
		"name" : "grassclump_f",
		"file" : "./we/grassclump_front0.png"
	},
	{
		"type" : "Model",
		"name" : "grassclump_f",
		"mesh" : "unitquad",
		"textures" : ["grassclump_f"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},


	{
		"type" : "Texture",
		"name" : "grassclump_b",
		"file" : "./we/grassclump_back0.png"
	},
	{
		"type" : "Model",
		"name" : "grassclump_b",
		"mesh" : "unitquad",
		"textures" : ["grassclump_b"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},


	{
		"type" : "Texture",
		"name" : "block_dirt",
		"file" : "./we/block_dirt_color.png",
	},
	{
		"type" : "Model",
		"name" : "block_dirt",
		"mesh" : "halfquad",
		"textures" : ["block_dirt"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},
	{
		"type" : "PhysMat",
		"name" : "dirt",
		"density" : "1.0",
		"friction" : "0.8",
		"restitution" : "0.1",
	},
	{
		"type" : "RigidBodyDef",
		"name" : "block_dirt",
		"mat" : "dirt",
		"shapes" : [
			{
				"type" : "Poly",
				"v" : [
					[-0.5, -0.5],
					[0.5, -0.5],
					[0.5, 0.5],
					[-0.5, 0.5],
				],
			}
		]
	},



	{
		"type" : "Texture",
		"name" : "wbarrel_c",
		"file" : "./we/barrel_color.png",
	},
	{
		"type" : "Model",
		"name" : "wbarrel",
		"mesh" : "halfquad",
		"textures" : ["wbarrel_c"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},
	{
		"type" : "PhysMat",
		"name" : "wood",
		"density" : "0.7",
		"friction" : "0.8",
		"restitution" : "0.5",
	},
	{
		"type" : "RigidBodyDef",
		"name" : "wbarrel",
		"mat" : "wood",
		"shapes" : [
			{
				"type" : "Circle",
				"pos" : [0.0, 0.0],
				"rad" : 0.45,
			},
		]
	},


	{
		"type" : "Texture",
		"name" : "rollbot_color",
		"file" : "./we/rollbot.png",
	},
	{
		"type" : "Model",
		"name" : "rollbot",
		"mesh" : "unitquad",
		"textures" : ["rollbot_color"],
		"color" : [1.0, 1.0, 1.0, 1.0],
	},
	{
		"type" : "PhysMat",
		"name" : "rock",
		"density" : "2.0",
		"friction" : "0.8",
		"restitution" : "0.1",
	},
	{
		"type" : "RigidBodyDef",
		"name" : "rollbot",
		"mat" : "rock",
		"shapes" : [
			{
				"type" : "Circle",
				"pos" : [0.0, 0.0],
				"rad" : 0.8,
			},
		]
	},

	{
		"type" : "RigidBodyDef",
		"name" : "wbox",
		"mat" : "wood",
		"shapes" : [
			{
				"type" : "Poly",
				"v" : [
					[-0.45, -0.45],
					[0.45, -0.45],
					[0.45, 0.45],
					[-0.45, 0.45],
				],
			}
		],
	},

	{
		"type" : "NodeGroupDef",
		"name" : "phys_prop",
		"nodes" : [
			{
				"type" : "RigidBody",
				"name" : "body",
				"defaults" : [
					{ "def_name" : "wbarrel" },
				],
			},
			{
				"type" : "ModelEntity",
				"name" : "visual",
				"defaults" : [
					{ "model_name" : "wbarrel" },
				],
			},
		],
		"cmds" : [
			"visual.tf= body.tf",
		]
	},


	{
		"type" : "NodeGroupDef",
		"name" : "block",
		"nodes" : [
			{
				"type" : "RigidBody",
				"name" : "body",
				"defaults" : [
					{ "def_name" : "block_dirt" },
				],
			},
			{
				"type" : "ModelEntity",
				"name" : "visual",
				"defaults" : [
					{ "model_name" : "block_dirt" },
				],
			},
		],
		"cmds" : [
			"if (body.tf_changed) visual.tf= body.tf",
			"if (body.shape_changed) poly_to_modelentity(visual, body)",
		]
	},


	{
		"type" : "NodeGroupDef",
		"name" : "visual_prop",
		"nodes" : [
			{
				"type" : "ModelEntity",
				"name" : "visual",
				"defaults" : [
					{ "model_name" : "wbox" },
				],
			},
		],
		"cmds" : [],
	},

	{
		"type" : "NodeGroupDef",
		"name" : "test_comp",
		"nodes" : [
			{
				"type" : "RigidBody",
				"name" : "body",
				"defaults" : [
					{ "def_name" : "wbox" },
				],
			},
			{
				"type" : "ClipInst",
				"name" : "anim",
				"defaults" : [
					{ "clip_name" : "test_clip" },
				]
			},
			{
				"type" : "CompEntity",
				"name" : "visual",
				"defaults" : [
					{ "def_name" : "test_comp" },
				],
			},
		],
		"cmds" : [
			"visual.pose= anim.pose",
			"visual.tf= body.tf",
		],
	},
	{
		"type" : "Armature",
		"name" : "test_armature",
		"joints" : [
			{
				"name" : "root",
				"super" : "",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				}
			},
			{
				"name" : "middle",
				"super" : "root",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 0.0, 1.0, 1.0 ],
					"pos" : [ 0.0, 1.0, 0.0 ],
				}
			},
			{
				"name" : "end",
				"super" : "middle",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 1.0, 0.0 ],
				}
			},
		],
	},
	{
		"type" : "CompDef",
		"name" : "test_comp",
		"armature" : "test_armature",
		"subs" : [
			{
				"entity" : "wbox",
				"joint" : "root",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
			{
				"entity" : "wbarrel",
				"joint" : "middle",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
			{
				"entity" : "wbox",
				"joint" : "end",
				"offset" : {
					"scale" : [ 1.0, 1.0, 1.0 ],
					"rot" : [ 0.0, 1.0, 0.0, 0.0 ],
					"pos" : [ 0.0, 0.0, 0.0 ],
				},
			},
		],
	},
	{
		"type" : "Clip",
		"name" : "test_clip",
		"armature" : "test_armature",
		"duration" : 2.0,
		"channels" : [

			{
				"joint" : "end",
				"type" : "rot",
				"keys" : [
					{ "t" : 0.0,	"v" : [ 0.0, 0.0, 1.0, 0.0 ] },
					{ "t" : 1.0,	"v" : [ 0.0, 0.0, 1.0, 1.0 ] },
					{ "t" : 2.0,	"v" : [ 0.0, 0.0, 1.0, 0.0 ] }
				],
			},

			{
				"joint" : "middle",
				"type" : "pos",
				"keys" : [
					{ "t" : 0.0,	"v" : [ 0.0, 0.0, 0.0 ] },
					{ "t" : 0.50,	"v" : [ 0.0, 0.50, 0.0 ] },
					{ "t" : 1.0,	"v" : [ 0.5, 0.50, 0.0 ] },
					{ "t" : 1.50,	"v" : [ 0.5, 0.0, 0.0 ] },
					{ "t" : 2.0,	"v" : [ 0.0, 0.0, 0.0 ] }
				],
			},
		],
	},

	{
		"type" : "Module",
		"name" : "mod",
		"file" : "./mod.so",
	},
	{
		"type" : "NodeType",
		"name" : "PlayerChar",
		"impl_mgmt" : "auto",
		"max_count" : 4,

		"init_func" : "",
		"resurrect_func" : "resurrect_playerchar",
		"upd_func" : "upd_playerchar",
		"free_func" : "free_playerchar",
	},
	{
		"type" : "PhysMat",
		"name" : "playerchar_legs",

		"density" : 2.0,
		"friction" : 5.0,
		"restitution" : 0.0,
	},
	{
		"type" : "RigidBodyDef",
		"name" : "playerchar_legs",
		"mat" : "playerchar_legs",
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
		"name" : "playerchar",
		"nodes" : [
			{
				"type" : "RigidBody",
				"name" : "body",
				"defaults" : [
					{ "def_name" : "playerchar_legs" },
				],
			},
			{
				"type" : "PlayerChar",
				"name" : "char",
				"defaults" : [ ],
			},
			{
				"type" : "ModelEntity",
				"name" : "visual",
				"defaults" : [
					{ "model_name" : "wbarrel" },
				],
			},
		],
		"cmds" : [
			"supply_playerchar(char, body)",
			"visual.tf.pos= char.pos",
			"visual.tf.rot= body.tf.rot",
		]
	},
]
