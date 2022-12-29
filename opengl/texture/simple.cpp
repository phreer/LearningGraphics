#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "application.h"
#include "utils.h"

void error_callback(int error, const char *description) { puts(description); }

const float vertex_data[] = {
  // positions     colors         texture coordinates
  -0.5, -0.5, 0.8, 1.0, 0.0, 0.0, -1.0, -1.0,
  +0.5, -0.5, 0.8, 0.0, 1.0, 0.0, +2.0, -1.0,
  +0.0, +0.5, 0.8, 0.0, 0.0, 1.0, +0.5, +2.0,
};

void WindowResizeCallback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

char *LoadVertexShader() {
  char filename[] = "../shader.vert";
  struct FileContent content;
  int ret = 0;

  ret = LoadFile(filename, &content);
  if (ret) {
    return NULL;
  }

  return content.content;
}

char *LoadFragmentShader() {
  char filename[] = "../shader.frag";
  struct FileContent content;
  int ret = 0;

  ret = LoadFile(filename, &content);
  if (ret) {
    return NULL;
  }

  return content.content;
}

GLuint CompileShaders() {
  GLuint vert_shader;
  GLuint frag_shader;
  GLuint program;
  GLint is_compiled = 0;

  char *vert_shader_source = LoadVertexShader();
  if (vert_shader_source == NULL) {
    fprintf(stderr, "failed to load vertex shader file\n");
    exit(EXIT_FAILURE);
  }
  vert_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert_shader, 1, &vert_shader_source, NULL);
  glCompileShader(vert_shader);
  glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &is_compiled);
  if (is_compiled == GL_FALSE) {
    GLint maxLength = 0;
    glGetShaderiv(vert_shader, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(vert_shader, maxLength, &maxLength, &errorLog[0]);

    // Provide the infolog in whatever manor you deem best.
    printf("%s: failed to compile fragment shader\n", __func__);
    puts(errorLog.data());
    // Exit with failure.
    glDeleteShader(vert_shader); // Don't leak the shader.
    exit(EXIT_FAILURE);
  } else {
    printf("vertex shader compiled\n");
  }

  char *frag_shader_source = LoadFragmentShader();
  if (frag_shader_source == NULL) {
    fprintf(stderr, "failed to load fragment shader file\n");
    exit(EXIT_FAILURE);
  }
  frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag_shader, 1, &frag_shader_source, NULL);
  glCompileShader(frag_shader);
  glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &is_compiled);

  if (is_compiled == GL_FALSE) {
    GLint maxLength = 0;
    glGetShaderiv(frag_shader, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(frag_shader, maxLength, &maxLength, &errorLog[0]);

    // Provide the infolog in whatever manor you deem best.
    printf("%s: failed to compile vertex shader\n", __func__);
    puts(errorLog.data());
    // Exit with failure.
    glDeleteShader(frag_shader); // Don't leak the shader.
    exit(EXIT_FAILURE);
  } else {
    printf("fragment shader compiled\n");
  }

  program = glCreateProgram();
  printf("program created\n");
  glAttachShader(program, vert_shader);
  printf("vertex shader attached\n");
  glAttachShader(program, frag_shader);
  printf("program created\n");
  glLinkProgram(program);
  printf("fragment shader attached\n");

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  free(vert_shader_source);
  free(frag_shader_source);

  return program;
}

class SimpleApp : public Application {
public:
  SimpleApp() : Application("Simple"), window_(nullptr){};

private:
  void StartUp_() override {
    glfwSetErrorCallback(error_callback);
    if (glfwInit() != GLFW_TRUE) {
      fprintf(stderr, "failed to initialize GLFW\n");
      exit(EXIT_FAILURE);
    }
    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    window_ = glfwCreateWindow(640, 480, app_name().c_str(), NULL, NULL);
    if (!window_) {
      fprintf(stderr, "failed to create window\n");
      exit(EXIT_FAILURE);
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window_);
    glfwSetFramebufferSizeCallback(window_, WindowResizeCallback);

    GLenum err = glewInit();
    if (err != GLEW_OK) {
      fprintf(stderr, "failed to init glew");
      exit(EXIT_FAILURE); // or handle the error in a nicer way
    }
    if (!GLEW_VERSION_2_1) // check that the machine supports the 2.1 API.
      exit(EXIT_FAILURE);  // or handle the error in a nicer way

    // get version info
    const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte *version = glGetString(GL_VERSION);   // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    // glEnable(GL_DEPTH_TEST); // enable depth-testing
    // glDepthFunc(
    //     GL_LESS); // depth-testing interprets a smaller value as "closer"

    rendering_program_ = CompileShaders();

    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    width_ = width;
    height_ = height;
    glViewport(0, 0, width, height);

    // Setup vertex array and vertex buffers
    glGenVertexArrays(1, &vertex_array_object_);
    glBindVertexArray(vertex_array_object_);
    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Setup textures
    glGenTextures(sizeof(textures_) / sizeof(textures_[0]), textures_);
    int num_channels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load("../texture.jpg", &width, &height, &num_channels, 0);
    if (data == nullptr) {
      printf("Failed to load texture\n");
      exit(EXIT_FAILURE);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures_[0]);
    // set the texture wrapping/filtering options (on currently bound texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    data = stbi_load("../doge.png", &width, &height, &num_channels, 0);
    if (data == nullptr) {
      printf("Failed to load texutre (doge)\n");
      exit(EXIT_FAILURE);
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures_[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glUseProgram(rendering_program_);
    glUniform1i(glGetUniformLocation(rendering_program_, "u_tex0"), 0);
    glUniform1i(glGetUniformLocation(rendering_program_, "u_tex1"), 1);

  }

  void ShutDown_() override {
    glDeleteVertexArrays(1, &vertex_array_object_);
    glDeleteProgram(rendering_program_);
    glfwTerminate();
  }

  void Render_() override {
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window_)) {
      /* Render here */
      struct timeval tv;
      gettimeofday(&tv, nullptr);
      printf("sec = %ld, usec = %ld\n", tv.tv_sec, tv.tv_usec);
      Draw__((double) tv.tv_sec * 100 + (double) tv.tv_usec / 10000.f);
      /* Swap front and back buffers */
      glfwSwapBuffers(window_);
      /* Poll for and process events */
      glfwPollEvents();
    }
  }

  void Draw__(double current_time) {
    printf("Start drawing new frame, current_time = %lf\n", current_time);
    // Setup transform matrix
    unsigned int transform_loc = glGetUniformLocation(rendering_program_, "transform");
    glm::mat4 trans = glm::mat4(1.0f);
    double angle = current_time - (long) (current_time / 360) * 360;
    trans = glm::rotate(trans, glm::radians((float) angle), glm::vec3(0.0, 1.0, 1.0));
    trans = glm::translate(trans, glm::vec3(-0.5f, 0.1f, 0.0f));
    glUniformMatrix4fv(transform_loc, 1, GL_FALSE, glm::value_ptr(trans));

    // Start drawing triangles
    glUseProgram(rendering_program_);
    glBindVertexArray(vertex_array_object_);
    // const GLfloat color[] = {(float)sin(current_time) * 0.5f + 0.5f,
    //                          (float)cos(current_time) * 0.5f + 0.5f, 0.0f,
    //                          1.0f};
    const GLfloat color[] = {1, 1, 1, 1};
    glClearBufferfv(GL_COLOR, 0, color);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  GLFWwindow *window_;
  GLuint rendering_program_;
  GLuint vertex_array_object_;
  GLuint vertex_buffer_;
  GLuint textures_[2];
  unsigned int width_;
  unsigned int height_;
};

int main() {
  SimpleApp app{};
  app.Run();
  return 0;
}
