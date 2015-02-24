#version 150 core
in vec3 a_pos;
in vec3 a_uv;
in vec4 a_color;

uniform vec2 u_cursor;
out vec3 v_uv;
out vec3 v_pos;
out vec4 v_color;
void main()
{
	v_uv= a_uv;
	v_pos= a_pos;
	v_color= a_color;
	gl_Position= vec4((v_pos.xy + u_cursor)/(5.0 + a_pos.z), 0.0, 1.0);
}
