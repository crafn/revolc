#include "core/debug.h"
#include "gl.h"

void gl_check_shader_status(GLuint shd, const char *msg)
{
	GLint status;
	glGetShaderiv(shd, GL_COMPILE_STATUS, &status);
	if (!status) {
		const GLsizei max_len = 512;
		GLchar log[max_len];
		glGetShaderInfoLog(shd, max_len, NULL, log);
		fail("Shader compilation failed (%i, %s): %s", shd, msg, log);
	}
}

void gl_check_program_status(GLuint prog)
{
	GLint link_status;
	glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
	if (!link_status) {
		const GLsizei size = 512;
		GLchar log[size];
		glGetProgramInfoLog(prog, size, NULL, log);
		fail("Program link failed: %s", log);
	}
}

void gl_check_errors(const char *tag)
{
	GLenum error = glGetError();
	if (error!= GL_NO_ERROR)
		debug_print("GL Error (%s): %i\n", tag, error);
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

	*prog = *vs = *gs = *fs = 0;
}

