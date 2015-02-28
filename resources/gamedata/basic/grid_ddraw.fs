#version 150 core
uniform sampler2D u_tex_color;
in vec3 v_uv;

out vec4 f_color;
void main()
{
	f_color= texture(u_tex_color, v_uv.xy);
}
