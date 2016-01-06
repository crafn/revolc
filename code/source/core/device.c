#include "device.h"
#include "core/gl.h"
#include "core/basic.h"
#include "core/memory.h"

// Define these functions in platform dependent code
VoidFunc plat_query_gl_func_impl(const char *name);
void plat_init_impl(Device* d, const char* title, V2i reso);
void plat_quit_impl(Device *d);
void plat_update_impl(Device *d);
void plat_sleep(int ms);
void plat_find_paths_with_end_impl(	char **path_table, U32 *path_count, U32 max_count,
									const char *name, int level, const char *end);

VoidFunc plat_query_gl_func(const char *name)
{
	VoidFunc f = NULL;
	f = plat_query_gl_func_impl(name);
	if (!f)
		fail("Failed to query gl function: %s\n", name);
	return f;
}

Device * plat_init(const char* title, V2i reso)
{
	debug_print("plat_init");
	{
		ensure(sizeof(U8) == 1);
		ensure(sizeof(U16) == 2);
		ensure(sizeof(U32) == 4);
		ensure(sizeof(U64) == 8);
	}
	plat_flush_denormals(true); // For perf

	Device *d = ZERO_ALLOC(gen_ator(), sizeof(*d), "device");
	if (g_env.device == NULL)
		g_env.device = d;

	plat_init_impl(d, title, reso);

	{
		glCreateShader = (GlCreateShader)plat_query_gl_func("glCreateShader");
		glShaderSource = (GlShaderSource)plat_query_gl_func("glShaderSource");
		glCompileShader = (GlCompileShader)plat_query_gl_func("glCompileShader");
		glCreateProgram = (GlCreateProgram)plat_query_gl_func("glCreateProgram");
		glAttachShader = (GlAttachShader)plat_query_gl_func("glAttachShader");
		glLinkProgram = (GlLinkProgram)plat_query_gl_func("glLinkProgram");
		glUseProgram = (GlUseProgram)plat_query_gl_func("glUseProgram");
		glGetShaderiv = (GlGetShaderiv)plat_query_gl_func("glGetShaderiv");
		glGetProgramiv = (GlGetProgramiv)plat_query_gl_func("glGetProgramiv");
		glGetShaderInfoLog = (GlGetShaderInfoLog)plat_query_gl_func("glGetShaderInfoLog");
		glGetProgramInfoLog = (GlGetProgramInfoLog)plat_query_gl_func("glGetProgramInfoLog");
		glDetachShader = (GlDetachShader)plat_query_gl_func("glDetachShader");
		glDeleteShader = (GlDeleteShader)plat_query_gl_func("glDeleteShader");
		glDeleteProgram = (GlDeleteProgram)plat_query_gl_func("glDeleteProgram");
		glGetUniformLocation = (GlGetUniformLocation)plat_query_gl_func("glGetUniformLocation");
		glUniform1f = (GlUniform1f)plat_query_gl_func("glUniform1f");
		glUniform2f = (GlUniform2f)plat_query_gl_func("glUniform2f");
		glUniform3f = (GlUniform3f)plat_query_gl_func("glUniform3f");
		glUniform4f = (GlUniform4f)plat_query_gl_func("glUniform4f");
		glUniformMatrix4fv = (GlUniformMatrix4fv)plat_query_gl_func("glUniformMatrix4fv");
		glUniform1i = (GlUniform1i)plat_query_gl_func("glUniform1i");
		glUniform2i = (GlUniform2i)plat_query_gl_func("glUniform2i");
		glGenBuffers = (GlGenBuffers)plat_query_gl_func("glGenBuffers");
		glBindBuffer = (GlBindBuffer)plat_query_gl_func("glBindBuffer");
		glBufferData = (GlBufferData)plat_query_gl_func("glBufferData");
		glBufferSubData = (GlBufferSubData)plat_query_gl_func("glBufferSubData");
		glDeleteBuffers = (GlDeleteBuffers)plat_query_gl_func("glDeleteBuffers");
		glCheckFramebufferStatus = (GlCheckFramebufferStatus)plat_query_gl_func("glCheckFramebufferStatus");
		glEnableVertexAttribArray = (GlEnableVertexAttribArray)plat_query_gl_func("glEnableVertexAttribArray");
		glDisableVertexAttribArray = (GlDisableVertexAttribArray)plat_query_gl_func("glDisableVertexAttribArray");
		glVertexAttribPointer = (GlVertexAttribPointer)plat_query_gl_func("glVertexAttribPointer");
		glVertexAttribIPointer = (GlVertexAttribIPointer)plat_query_gl_func("glVertexAttribIPointer");
		glBindAttribLocation = (GlBindAttribLocation)plat_query_gl_func("glBindAttribLocation");
		glDrawBuffers = (GlDrawBuffers)plat_query_gl_func("glDrawBuffers");

		glGenFramebuffers = (GlGenFramebuffers)plat_query_gl_func("glGenFramebuffers");
		glBindFramebuffer = (GlBindFramebuffer)plat_query_gl_func("glBindFramebuffer");
		glFramebufferTexture2D = (GlFramebufferTexture2D)plat_query_gl_func("glFramebufferTexture2D");
		glDeleteFramebuffers = (GlDeleteFramebuffers)plat_query_gl_func("glDeleteFramebuffers");
		glGenVertexArrays = (GlGenVertexArrays)plat_query_gl_func("glGenVertexArrays");
		glDeleteVertexArrays = (GlDeleteVertexArrays)plat_query_gl_func("glDeleteVertexArrays");
		glBindVertexArray = (GlBindVertexArray)plat_query_gl_func("glBindVertexArray");
		glTexStorage3D = (GlTexStorage3D)plat_query_gl_func("glTexStorage3D");
		glBindFragDataLocation = (GlBindFragDataLocation)plat_query_gl_func("glBindFragDataLocation");
		glTransformFeedbackVaryings = (GlTransformFeedbackVaryings)plat_query_gl_func("glTransformFeedbackVaryings");
		glBindBufferBase = (GlBindBufferBase)plat_query_gl_func("glBindBufferBase");
		glBeginTransformFeedback = (GlBeginTransformFeedback)plat_query_gl_func("glBeginTransformFeedback");
		glEndTransformFeedback = (GlEndTransformFeedback)plat_query_gl_func("glEndTransformFeedback");
		glTexImage2DMultisample = (GlTexImage2DMultisample)plat_query_gl_func("glTexImage2DMultisample");
		glBlitFramebuffer = (GlBlitFramebuffer)plat_query_gl_func("glBlitFramebuffer");

#		if PLATFORM == PLATFORM_WINDOWS
			glTexSubImage3D = (GlTexSubImage3D)plat_query_gl_func("glTexSubImage3D");
			glActiveTexture = (GlActiveTexture)plat_query_gl_func("glActiveTexture");
			glDrawRangeElements = (GlDrawRangeElements)plat_query_gl_func("glDrawRangeElements");
#		endif
	}

	return d;
}

void plat_quit(Device *d)
{
	if (g_env.device == d)
		g_env.device = NULL;

	plat_quit_impl(d);

	FREE(gen_ator(), d);
	debug_print("plat_quit successful");
}

void plat_update(Device *d)
{
	d->written_text_size = 0;
	plat_update_impl(d);
}

#define PATH_MAX_TABLE_SIZE 1024
char ** plat_find_paths_with_end(const char *path_to_dir, const char *end)
{
	U32 path_count = 0;
	char **path_table = ZERO_ALLOC(gen_ator(), sizeof(*path_table)*PATH_MAX_TABLE_SIZE, "path_table");

	plat_find_paths_with_end_impl(path_table, &path_count, PATH_MAX_TABLE_SIZE, path_to_dir, 0, end);
	return path_table;
}
