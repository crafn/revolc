#version 150 core
in vec3 a_pos;
in vec3 a_uv;
uniform vec2 u_cursor;
out vec3 v_uv;
void main()
{
	v_uv= a_uv;
	gl_Position= vec4((a_pos.xy + u_cursor)/(1.0 + a_pos.z), 0.0, 1.0);
}
