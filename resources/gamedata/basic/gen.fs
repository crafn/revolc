#version 150 core
uniform sampler2D u_tex_color;
in vec2 v_uv;
out vec4 f_color;
void main()
{
	gl_FragColor= texture2D(u_tex_color, v_uv);
}
