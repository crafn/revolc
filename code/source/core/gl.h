#ifndef REVOLC_GL_H
#define REVOLC_GL_H

#include "build.h"

#ifndef CODEGEN
#	include <GL/gl.h>
#endif


// Required GL 2 features

#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_GEOMETRY_SHADER 0x8DD9
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C
#define GL_MULTISAMPLE  0x809D

#if PLATFORM == PLATFORM_WINDOWS
#	define GL_CLAMP_TO_EDGE 0x812F
#	define GL_TEXTURE_BASE_LEVEL 0x813C
#	define GL_TEXTURE_MAX_LEVEL 0x813D
#	define GL_TEXTURE_MIN_LOD 0x813A
#	define GL_TEXTURE_MAX_LOD 0x813B
#	define GL_TEXTURE0 0x84C0
#	define GL_TEXTURE1 0x84C1
#	define GL_TEXTURE2 0x84C2
#	define GL_TEXTURE3 0x84C3
#	define GL_STATIC_DRAW 0x88E4
#	define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif

typedef char GLchar;
typedef intptr_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef GLuint (*GlCreateShader)(GLenum);
GlCreateShader glCreateShader;
typedef void (*GlShaderSource)(GLuint, GLsizei, const GLchar**, const GLint*);
GlShaderSource glShaderSource;
typedef void (*GlCompileShader)(GLuint);
GlCompileShader glCompileShader;
typedef GLuint (*GlCreateProgram)();
GlCreateProgram glCreateProgram;
typedef void (*GlAttachShader)(GLuint, GLuint);
GlAttachShader glAttachShader;
typedef void (*GlLinkProgram)(GLuint);
GlLinkProgram glLinkProgram;
typedef void (*GlUseProgram)(GLuint);
GlUseProgram glUseProgram;
typedef void (*GlGetShaderiv)(GLuint, GLenum, GLint*);
GlGetShaderiv glGetShaderiv;
typedef void (*GlGetProgramiv)(GLuint, GLenum, GLint*);
GlGetProgramiv glGetProgramiv;
typedef void (*GlGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
GlGetShaderInfoLog glGetShaderInfoLog;
typedef void (*GlGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
GlGetProgramInfoLog glGetProgramInfoLog;
typedef void (*GlDetachShader)(GLuint, GLuint);
GlDetachShader glDetachShader;
typedef void (*GlDeleteShader)(GLuint);
GlDeleteShader glDeleteShader;
typedef void (*GlDeleteProgram)(GLuint);
GlDeleteProgram glDeleteProgram;
typedef GLint (*GlGetUniformLocation)(GLuint, const GLchar*);
GlGetUniformLocation glGetUniformLocation;
typedef void (*GlUniform1f)(GLuint, GLfloat);
GlUniform1f glUniform1f;
typedef void (*GlUniform2f)(GLuint, GLfloat, GLfloat);
GlUniform2f glUniform2f;
typedef void (*GlUniform3f)(GLuint, GLfloat, GLfloat, GLfloat);
GlUniform3f glUniform3f;
typedef void (*GlUniform4f)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
GlUniform4f glUniform4f;
typedef void (*GlUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
GlUniformMatrix4fv glUniformMatrix4fv;
typedef void (*GlUniform1i)(GLint, GLint);
GlUniform1i glUniform1i;
typedef void (*GlUniform2i)(GLint, GLint, GLint);
GlUniform2i glUniform2i;
typedef void (*GlGenBuffers)(GLsizei, GLuint*);
GlGenBuffers glGenBuffers;
typedef void (*GlBindBuffer)(GLenum, GLuint);
GlBindBuffer glBindBuffer;
typedef void (*GlBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
GlBufferData glBufferData;
typedef void (*GlBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*);
GlBufferSubData glBufferSubData;
typedef void (*GlDeleteBuffers)(GLsizei, const GLuint*);
GlDeleteBuffers glDeleteBuffers;
typedef void (*GlEnableVertexAttribArray)(GLuint);
GlEnableVertexAttribArray glEnableVertexAttribArray;
typedef void (*GlDisableVertexAttribArray)(GLuint);
GlDisableVertexAttribArray glDisableVertexAttribArray;
typedef void (*GlVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
GlVertexAttribPointer glVertexAttribPointer;
typedef void (*GlVertexAttribIPointer)(GLuint, GLint, GLenum, GLsizei, const GLvoid*);
GlVertexAttribIPointer glVertexAttribIPointer;
typedef void (*GlBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
GlBindAttribLocation glBindAttribLocation;
typedef void (*GlDrawBuffers)(GLsizei n, const GLenum *buffs);
GlDrawBuffers glDrawBuffers;

// Required GL 3 features

#define GL_FRAMEBUFFER 0x8D40
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT4 0x8CE4
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_RGB16F 0x881B
#define GL_RG16F 0x822F
#define GL_RGB32F 0x8815
#define GL_R16UI 0x8234
#define GL_RED_INTEGER 0x8D94
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_RASTERIZER_DISCARD 0x8C89
#define GL_INTERLEAVED_ATTRIBS 0x8C8C
#define GL_TRANSFORM_FEEDBACK_BUFFER 0x8C8E
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9

typedef void (*GlGenFramebuffers)(GLsizei, GLuint*);
GlGenFramebuffers glGenFramebuffers;
typedef void (*GlBindFramebuffer)(GLenum, GLuint);
GlBindFramebuffer glBindFramebuffer;
typedef void (*GlFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
GlFramebufferTexture2D glFramebufferTexture2D;
typedef void (*GlDeleteFramebuffers)(GLsizei, GLuint*);
GlDeleteFramebuffers glDeleteFramebuffers;
typedef GLenum (*GlCheckFramebufferStatus)(GLenum);
GlCheckFramebufferStatus glCheckFramebufferStatus;
typedef void (*GlGenVertexArrays)(GLsizei, GLuint*);
GlGenVertexArrays glGenVertexArrays;
typedef void (*GlDeleteVertexArrays)(GLsizei, const GLuint*);
GlDeleteVertexArrays glDeleteVertexArrays;
typedef void (*GlBindVertexArray)(GLuint);
GlBindVertexArray glBindVertexArray;
typedef void (*GlTexStorage3D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
GlTexStorage3D glTexStorage3D;
typedef void (*GlBindFragDataLocation)(GLuint, GLuint, const char *);
GlBindFragDataLocation glBindFragDataLocation;
typedef void (*GlBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
typedef void (*GlTransformFeedbackVaryings)(GLuint program, GLsizei count, const char **varyings, GLenum bufferMode);
GlTransformFeedbackVaryings glTransformFeedbackVaryings;
GlBindBufferBase glBindBufferBase;
typedef void (*GlBeginTransformFeedback)(GLenum primitiveMode);
GlBeginTransformFeedback glBeginTransformFeedback;
typedef void (*GlEndTransformFeedback)();
GlEndTransformFeedback glEndTransformFeedback;
typedef void (*GlTexImage2DMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLboolean);
GlTexImage2DMultisample glTexImage2DMultisample;
typedef void (*GlBlitFramebuffer)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
GlBlitFramebuffer glBlitFramebuffer;


#if PLATFORM == PLATFORM_WINDOWS
	typedef void (*GlTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
	GlTexSubImage3D glTexSubImage3D;
	typedef void (*GlActiveTexture)(GLenum);
	GlActiveTexture glActiveTexture;
	typedef void (*GlDrawRangeElements)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);
	GlDrawRangeElements glDrawRangeElements;
#endif

// Useful utilities wrapping multiple OpenGL commands

#if BUILD == BUILD_DEBUG
// Wrap every gl call with this
#	define GL(x) do {\
		gl_check_errors("pre " __FILE__ " line " TO_STRING(__LINE__));\
		x;\
		gl_check_errors("post " __FILE__ " line " TO_STRING(__LINE__));\
		} while(0)
#else
#	define GL(x) x
#endif

REVOLC_API void gl_check_shader_status(GLuint shd, const char *msg);
REVOLC_API void gl_check_program_status(GLuint prog);
REVOLC_API void gl_check_errors(const char* tag);
REVOLC_API void gl_destroy_shader_prog(GLuint *prog, GLuint *vs, GLuint *gs, GLuint *fs);

#endif // REVOLC_GL_H
