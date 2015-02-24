#version 150 core
in vec3 a_pos;
in vec3 a_uv;
uniform vec2 u_cursor;
out vec3 v_uv;
out vec3 v_pos;
void main()
{
	v_uv= a_uv;
	v_pos= a_pos;
	gl_Position= vec4((v_pos.xy + u_cursor)/(1.0 + a_pos.z), 0.0, 1.0);
}
