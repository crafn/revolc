#ifndef REVOLC_GL_H
#define REVOLC_GL_H

#include "build.h"

#include <GL/gl.h>

// Required GL 2.1 features

#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82

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
typedef void (*GlUniform3f)(GLuint, GLfloat, GLfloat, GLfloat);
GlUniform3f glUniform3f;
typedef void (*GlUniform4f)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
GlUniform4f glUniform4f;
typedef void (*GlUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
GlUniformMatrix4fv glUniformMatrix4fv;
typedef void (*GlUniform1i)(GLint, GLint);
GlUniform1i glUniform1i;
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
typedef void (*GlVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
GlVertexAttribPointer glVertexAttribPointer;
typedef void (*GlBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
GlBindAttribLocation glBindAttribLocation;


// Required GL 3 features

#define GL_FRAMEBUFFER 0x8D40
#define GL_COLOR_ATTACHMENT0 0x8CE0

typedef void (*GlGenFramebuffers)(GLsizei, GLuint*);
GlGenFramebuffers glGenFramebuffers;
typedef void (*GlBindFramebuffer)(GLenum, GLuint);
GlBindFramebuffer glBindFramebuffer;
typedef void (*GlFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
GlFramebufferTexture2D glFramebufferTexture2D;
typedef void (*GlDeleteFramebuffers)(GLsizei, GLuint*);
GlDeleteFramebuffers glDeleteFramebuffers;

// Useful utilities wrapping multiple OpenGL commands

REVOLC_API void gl_check_shader_status(GLuint shd);
REVOLC_API void gl_check_program_status(GLuint prog);
REVOLC_API void gl_check_errors(const char* tag);
REVOLC_API void gl_create_shader_prog(	GLuint* prog, GLuint *vs, GLuint *gs, GLuint *fs,
										GLsizei vs_count, const GLchar **vs_src,
										GLsizei gs_count, const GLchar **gs_src,
										GLsizei fs_count, const GLchar **fs_src);
REVOLC_API void gl_destroy_shader_prog(GLuint *prog, GLuint *vs, GLuint *gs, GLuint *fs);

#endif // REVOLC_GL_H
