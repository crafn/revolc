#version 150 core
in vec3 a_pos;
in vec3 a_uv;
in vec4 a_color;
in float a_dev_highlight;

uniform mat4 u_cam;
out vec3 v_uv;
out vec3 v_pos;
out vec4 v_color;
out float v_dev_highlight;
void main()
{
	v_uv= a_uv;
	v_pos= a_pos;
	v_color= a_color;
	v_dev_highlight= a_dev_highlight;

	vec4 p= u_cam*vec4(a_pos, 1.0);
	gl_Position= vec4(p.xy, 0.0, p.w);
}
