#version 150 core
in vec3 a_pos;
in vec3 a_uv;

uniform mat4 u_cam;

out vec3 v_uv;

void main()
{
	v_uv= a_uv;
	vec4 p= u_cam*vec4(a_pos, 1.0);
	gl_Position= vec4(p.xy, 0.0, p.w);
}
