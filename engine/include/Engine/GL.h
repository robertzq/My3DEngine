#pragma once
#include <cstddef>

// 最小 OpenGL 3.3 Core 加载器（通过 SDL_GL_GetProcAddress）。
// 不依赖任何 GL 头文件（避免与平台头冲突），只声明引擎用到的子集。
// 所有 GL 函数以函数指针形式命名为 glXxx，调用点写法与常规 GL 一致。
// gameplay 不得直接 include 本文件。

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef char GLchar;
typedef std::ptrdiff_t GLsizeiptr;

// constants (core subset)
#define GL_NO_ERROR            0x0000
#define GL_TRIANGLES           0x0004
#define GL_COLOR_BUFFER_BIT    0x00004000
#define GL_BLEND               0x0BE2
#define GL_SRC_ALPHA           0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_TEXTURE_2D          0x0DE1
#define GL_TEXTURE0            0x84C0
#define GL_RGBA                0x1908
#define GL_RGBA8               0x8058
#define GL_UNSIGNED_BYTE       0x1401
#define GL_FLOAT               0x1406
#define GL_FALSE               0
#define GL_TEXTURE_MAG_FILTER  0x2800
#define GL_TEXTURE_MIN_FILTER  0x2801
#define GL_TEXTURE_WRAP_S      0x2802
#define GL_TEXTURE_WRAP_T      0x2803
#define GL_NEAREST             0x2600
#define GL_LINEAR              0x2601
#define GL_CLAMP_TO_EDGE       0x812F
#define GL_ARRAY_BUFFER        0x8892
#define GL_STATIC_DRAW         0x88E4
#define GL_FRAMEBUFFER         0x8D40
#define GL_COLOR_ATTACHMENT0   0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_VERTEX_SHADER       0x8B31
#define GL_FRAGMENT_SHADER     0x8B30
#define GL_COMPILE_STATUS      0x8B81
#define GL_LINK_STATUS         0x8B82
#define GL_INFO_LOG_LENGTH     0x8B84

#define ENGINE_GL_FUNCTIONS(X) \
    X(void,   glGenTextures,           (GLsizei, GLuint*)) \
    X(void,   glDeleteTextures,        (GLsizei, const GLuint*)) \
    X(void,   glBindTexture,           (GLenum, GLuint)) \
    X(void,   glTexImage2D,            (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) \
    X(void,   glTexParameteri,         (GLenum, GLenum, GLint)) \
    X(void,   glActiveTexture,         (GLenum)) \
    X(void,   glGenVertexArrays,       (GLsizei, GLuint*)) \
    X(void,   glDeleteVertexArrays,    (GLsizei, const GLuint*)) \
    X(void,   glBindVertexArray,       (GLuint)) \
    X(void,   glGenBuffers,            (GLsizei, GLuint*)) \
    X(void,   glDeleteBuffers,         (GLsizei, const GLuint*)) \
    X(void,   glBindBuffer,            (GLenum, GLuint)) \
    X(void,   glBufferData,            (GLenum, GLsizeiptr, const void*, GLenum)) \
    X(void,   glEnableVertexAttribArray,(GLuint)) \
    X(void,   glVertexAttribPointer,   (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) \
    X(GLuint, glCreateShader,          (GLenum)) \
    X(void,   glShaderSource,          (GLuint, GLsizei, const GLchar* const*, const GLint*)) \
    X(void,   glCompileShader,         (GLuint)) \
    X(void,   glGetShaderiv,           (GLuint, GLenum, GLint*)) \
    X(void,   glGetShaderInfoLog,      (GLuint, GLsizei, GLsizei*, GLchar*)) \
    X(void,   glDeleteShader,          (GLuint)) \
    X(GLuint, glCreateProgram,         (void)) \
    X(void,   glAttachShader,          (GLuint, GLuint)) \
    X(void,   glLinkProgram,           (GLuint)) \
    X(void,   glGetProgramiv,          (GLuint, GLenum, GLint*)) \
    X(void,   glGetProgramInfoLog,     (GLuint, GLsizei, GLsizei*, GLchar*)) \
    X(void,   glDeleteProgram,         (GLuint)) \
    X(void,   glUseProgram,            (GLuint)) \
    X(GLint,  glGetUniformLocation,    (GLuint, const GLchar*)) \
    X(void,   glUniform1i,             (GLint, GLint)) \
    X(void,   glUniform1f,             (GLint, GLfloat)) \
    X(void,   glUniform2f,             (GLint, GLfloat, GLfloat)) \
    X(void,   glUniform3f,             (GLint, GLfloat, GLfloat, GLfloat)) \
    X(void,   glUniform4f,             (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
    X(void,   glViewport,              (GLint, GLint, GLsizei, GLsizei)) \
    X(void,   glClearColor,            (GLfloat, GLfloat, GLfloat, GLfloat)) \
    X(void,   glClear,                 (GLuint)) \
    X(void,   glEnable,                (GLenum)) \
    X(void,   glDisable,               (GLenum)) \
    X(void,   glBlendFunc,             (GLenum, GLenum)) \
    X(void,   glDrawArrays,            (GLenum, GLint, GLsizei)) \
    X(void,   glReadPixels,            (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*)) \
    X(void,   glGenFramebuffers,       (GLsizei, GLuint*)) \
    X(void,   glDeleteFramebuffers,    (GLsizei, const GLuint*)) \
    X(void,   glBindFramebuffer,       (GLenum, GLuint)) \
    X(void,   glFramebufferTexture2D,  (GLenum, GLenum, GLenum, GLuint, GLint)) \
    X(GLenum, glCheckFramebufferStatus,(GLenum)) \
    X(GLenum, glGetError,              (void)) \
    X(const unsigned char*, glGetString, (GLenum))

#define ENGINE_GL_DECLARE(ret, name, args) extern ret (*name) args;
ENGINE_GL_FUNCTIONS(ENGINE_GL_DECLARE)
#undef ENGINE_GL_DECLARE

// 在 GL context 创建后调用一次；成功后上面的函数指针才可用。
bool LoadGLFunctions();
