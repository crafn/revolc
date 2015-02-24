#version 150 core
in vec3 a_pos;
in vec3 a_uv;
in vec4 a_color;

uniform mat4 u_cam;
out vec3 v_uv;
out vec3 v_pos;
out vec4 v_color;
void main()
{
	v_uv= a_uv;
	v_pos= a_pos;
	v_color= a_color;

	vec4 p= u_cam*vec4(a_pos, 1.0);
	gl_Position= vec4(p.xy, 0.0, p.w);
}
