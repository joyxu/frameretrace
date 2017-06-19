// Copyright (C) Intel Corp.  2015.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#ifndef RETRACE_DAEMON_BARGRAPH_GLFRAME_GLHELPER_H_
#define RETRACE_DAEMON_BARGRAPH_GLFRAME_GLHELPER_H_

#include <stdio.h>

#include <GLES2/gl2.h>
#include <GL/gl.h>

#include <vector>

namespace glretrace {

class GlFunctions {
 public:
  static void Init(void *lookup_fn = NULL);
  static GLuint CreateProgram(void);
  static GLuint CreateShader(GLenum type);
  static void ShaderSource(GLuint shader, GLsizei count,
                            const GLchar *const*str,
                            const GLint *length);
  static void CompileShader(GLuint shader);
  static void AttachShader(GLuint program, GLuint shader);
  static void LinkProgram(GLuint program);
  static void UseProgram(GLuint program);
  static GLenum GetError();
  static void GetShaderiv(GLuint shader, GLenum pname, GLint *params);
  static void GetShaderInfoLog(GLuint shader, GLsizei bufSize,
                               GLsizei *length, GLchar *infoLog);
  static void GenBuffers(GLsizei n, GLuint *buffers);
  static void BindBuffer(GLenum target, GLuint buffer);
  static GLint GetAttribLocation(GLuint program, const GLchar *name);
  static GLint GetUniformLocation(GLuint program, const GLchar *name);

  static void ClearColor(GLclampf red, GLclampf green, GLclampf blue,
                         GLclampf alpha);
  static void Clear(GLbitfield mask);
  static void Uniform1f(GLint location, GLfloat v0);
  static void Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                        GLfloat v3);
  static void BufferData(GLenum target, GLsizeiptr size, const void *data,
                         GLenum usage);
  static void EnableVertexAttribArray(GLuint index);
  static void VertexAttribPointer(GLuint index, GLint size, GLenum type,
                                  GLboolean normalized, GLsizei stride,
                                  const void *pointer);
  static void DrawArrays(GLenum mode, GLint first, GLsizei count);
  static void DisableVertexAttribArray(GLuint index);
  static void Enable(GLenum cap);

  static void ReadPixels(GLint x, GLint y,
                           GLsizei width, GLsizei height,
                           GLenum format, GLenum type,
                           GLvoid *pixels);

 private:
  GlFunctions();
  static bool m_is_initialized;
};

typedef GlFunctions GL;

inline void CheckError(const char * file, int line) {
  const int error = GL::GetError();
  if ( error == GL_NO_ERROR)
    return;
  printf("ERROR: %x %s:%i\n", error, file, line);
}

inline void PrintCompileError(GLint shader) {
  GLint status;
  GL::GetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE)
    return;
  static const int MAXLEN = 1024;
  std::vector<char> log(MAXLEN);
  GLsizei len;
  GL::GetShaderInfoLog(shader,  MAXLEN,  &len, log.data());
  printf("ERROR -- compile failed: %s\n", log.data());
}

}  // namespace glretrace

#define GL_CHECK() CheckError(__FILE__, __LINE__)

#endif  // RETRACE_DAEMON_BARGRAPH_GLFRAME_GLHELPER_H_
