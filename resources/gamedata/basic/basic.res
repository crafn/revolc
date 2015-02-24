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
]
