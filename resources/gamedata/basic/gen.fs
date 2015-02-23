#version 150 core
uniform sampler2DArray u_tex_color;
in vec3 v_uv;
out vec4 f_color;
void main()
{
	gl_FragColor= texture(u_tex_color, v_uv);
}
