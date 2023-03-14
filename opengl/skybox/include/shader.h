#ifndef __SHADER_H__
#define __SHADER_H__
#include <stdlib.h>
#include <string>

#include "glad/glad.h"

#include "utils.h"
#include "log.h"

static char *LoadShader(const char *filename) {
  struct FileContent content;
  int ret = 0;

  ret = LoadFile(filename, &content);
  if (ret) {
    LOG_ERROR("failed to load shader %s\n", filename);
    exit(EXIT_FAILURE);
  }

  return content.content;
}

static GLuint CompileShader(GLenum shader_type, const char *data) {
  if (shader_type != GL_VERTEX_SHADER && shader_type != GL_FRAGMENT_SHADER) {
    LOG_ERROR("unknown shader type: %d\n", shader_type);
    exit(EXIT_FAILURE);
  }
  GLint shader_id = glCreateShader(shader_type);
  glShaderSource(shader_id, 1, &data, NULL);
  glCompileShader(shader_id);

  int is_compiled = 0;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);
  if (is_compiled == GL_FALSE) {
    GLint maxLength = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shader_id, maxLength, &maxLength, &errorLog[0]);

    const char *shader_name = shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment";
    // Provide the infolog in whatever manor you deem best.
    LOG_ERROR("%s: failed to compile %s shader\n", __func__, shader_name);
    puts(errorLog.data());
    // Exit with failure.
    glDeleteShader(shader_id); // Don't leak the shader.
    exit(EXIT_FAILURE);
  } else {
    LOG("vertex shader compiled\n");
  }
  return shader_id;
}

static void HandleGLError(const char *context) {
  GLenum error = GL_NO_ERROR;
  while ((error = glGetError())) {
    LOG_ERROR("error occurred in OpenGL: %d, context: %s\n", error, context);
    exit(EXIT_FAILURE);
  }
}

class Shader {
 public:
  Shader(): program_id_(0), vertex_shader_id_(0), fragment_shader_id_(0) {}
  ~Shader() {
    glDeleteShader(vertex_shader_id_);
    glDeleteShader(fragment_shader_id_);
    glDeleteProgram(program_id_);
  }
  Shader& SetVertexShader(const char *filename) {
    const char *data = LoadShader(filename);
    vertex_shader_id_ = CompileShader(GL_VERTEX_SHADER, data);
    free(data);

    return *this;
  }
  Shader& SetFragmentShader(const char *filename) {
    const char *data = LoadShader(filename);
    vertex_shader_id_ = CompileShader(GL_FRAGMENT_SHADER, data);
    free(data);

    return *this;
  }
  Shader& Build() {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader_id_);
    glAttachShader(program, fragment_shader_id_);
    glLinkProgram(program);
    HandleGLError(__func__);
    LOG("shader program linked\n");
    program_id_ = program;

    return *this;
  }

  Shader& Use() {
    if (program_id_ == 0) {
      LOG_ERROR("program unavailable\n");
      exit(EXIT_FAILURE);
    }
    glUseProgram(program_id_);
    return *this;
  }

  Shader& SetVertexArray(GLuint vertex_array);
  Shader& SetUniform1i(const char *name, GLint value) {
    glUniform1i(glGetUniformLocation(program_id_, name), value);
    return *this;
  }
  Shader& SetUniform3fv(const char *name, GLsizei count, const GLfloat *value) {
    glUniform3fv(glGetUniformLocation(program_id_, name), count, value);
    return *this;
  }
  Shader& SetUniform4fv(const char *name, GLsizei count, const GLfloat *value) {
    glUniform4fv(glGetUniformLocation(program_id_, name), count, value);
    return *this;
  }
  Shader& SetUniformMatrix4fv(const char *name, GLsizei count,
                              GLboolean transpose, const GLfloat *value) {
    glUniformMatrix4fv(glGetUniformLocation(program_id_, name),
                       count, transpose, value);
    return *this;
  }

 private:
  GLuint program_id_;
  GLuint vertex_shader_id_;
  GLuint fragment_shader_id_;
};

#endif
