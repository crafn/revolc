#version 150 core
uniform sampler2DArray u_tex_color;
in vec3 v_uv;
in vec3 v_pos;
out vec4 f_color;
void main()
{
	vec4 c= texture(u_tex_color, v_uv);
	f_color= c;
}
