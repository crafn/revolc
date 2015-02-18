#include "core/debug_print.h"
#include "core/ensure.h"
#include "gl.h"

void gl_check_shader_status(GLuint shd)
{
	GLint status;
	glGetShaderiv(shd, GL_COMPILE_STATUS, &status);
	if (!status) {
		const GLsizei max_len= 512;
		GLchar log[max_len];
		glGetShaderInfoLog(shd, max_len, NULL, log);
		debug_print("Shader compilation failed (%i): %s", shd, log);
		fail("");
	}
}

void gl_check_program_status(GLuint prog)
{
	GLint link_status;
	glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
	if (!link_status) {
		const GLsizei size= 512;
		GLchar log[size];
		glGetProgramInfoLog(prog, size, NULL, log);
		debug_print("Program link failed: %s", log);
		fail("");
	}
}

void gl_check_errors(const char *tag)
{
	GLenum error= glGetError();
	if (error!= GL_NO_ERROR)
		debug_print("GL Error (%s): %i\n", tag, error);
}

void gl_create_shader_prog(	GLuint* prog, GLuint *vs, GLuint *gs, GLuint *fs,
							GLsizei vs_count, const GLchar **vs_src,
							GLsizei gs_count, const GLchar **gs_src,
							GLsizei fs_count, const GLchar **fs_src)
{
	{ // Vertex
		*vs= glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(*vs, vs_count, vs_src, NULL);
		glCompileShader(*vs);
		gl_check_shader_status(*vs);
	}
	if (gs_count > 0) { // Geometry
		*gs= glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(*gs, gs_count, gs_src, NULL);
		glCompileShader(*gs);
		gl_check_shader_status(*gs);
	}
	{ // Fragment
		*fs= glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(*fs, fs_count, fs_src, NULL);
		glCompileShader(*fs);
		gl_check_shader_status(*fs);
	}
	{ // Shader program
		*prog= glCreateProgram();
		glAttachShader(*prog, *vs);
		if (gs_count > 0)
			glAttachShader(*prog, *gs);
		glAttachShader(*prog, *fs);
		glLinkProgram(*prog);
		gl_check_program_status(*prog);
	}
}

void gl_destroy_shader_prog(GLuint *prog, GLuint *vs, GLuint *gs, GLuint *fs)
{
	glDetachShader(*prog, *vs);
	glDeleteShader(*vs);

	if (*gs) {
		glDetachShader(*prog, *gs);
		glDeleteShader(*gs);
	}

	glDetachShader(*prog, *fs);
	glDeleteShader(*fs);

	glDeleteProgram(*prog);

	*prog= *vs= *gs= *fs= 0;
}

