#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <math.h>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include "utils.h"
#include "application.h"


void error_callback(int error, const char* description) {
   puts(description);
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
  printf("vertex shader source: \n");
  printf(vert_shader_source);
  glShaderSource(vert_shader, 1, &vert_shader_source, NULL);
  glCompileShader(vert_shader);
  glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &is_compiled);
  if(is_compiled == GL_FALSE) {
    GLint maxLength = 0;
    glGetShaderiv(vert_shader, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(vert_shader, maxLength, &maxLength, &errorLog[0]);

    // Provide the infolog in whatever manor you deem best.
    printf("%s: failed to compile vertex shader\n", __func__);
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
  printf("fragment shader source: \n");
  printf(frag_shader_source);
  glShaderSource(frag_shader, 1, &frag_shader_source, NULL);
  glCompileShader(frag_shader);
  glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &is_compiled);

  if(is_compiled == GL_FALSE) {
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


class SimpleApp: public Application {
 public:
  SimpleApp(): Application("Simple"), window_(nullptr) {};

 private:
  void StartUp_() override {
    glfwSetErrorCallback(error_callback);
    if (glfwInit() != GLFW_TRUE) {
      fprintf(stderr, "failed to initialize GLFW\n");
      exit(EXIT_FAILURE);
    }
    /* Create a windowed mode window and its OpenGL context */
    window_ = glfwCreateWindow(640, 480, app_name().c_str(), NULL, NULL);
    if (!window_) {
      fprintf(stderr, "failed to create window\n");
      exit(EXIT_FAILURE);
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window_);

    GLenum err = glewInit();
    if (err != GLEW_OK) {
      fprintf(stderr, "failed to init glew");
      exit(EXIT_FAILURE); // or handle the error in a nicer way
    }
    if (!GLEW_VERSION_2_1)  // check that the machine supports the 2.1 API.
        exit(EXIT_FAILURE); // or handle the error in a nicer way

    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

    rendering_program_ = CompileShaders();

    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    glViewport(0, 0, width, height);

    glCreateVertexArrays(1, &vertex_array_object_);
    glBindVertexArray(vertex_array_object_);
  }

  void ShutDown_() override {
    glDeleteVertexArrays(1, &vertex_array_object_);
    glDeleteProgram(rendering_program_);
    glfwTerminate();
  }

  void Render_() override {
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window_))
    {
      /* Render here */
      auto current_time = time(nullptr);
      Draw__(current_time * 1000);
      /* Swap front and back buffers */
      glfwSwapBuffers(window_);
      /* Poll for and process events */
      glfwPollEvents();
    }
  }

  void Draw__(double current_time) {
    const GLfloat color[] = {
        (float)sin(current_time) * 0.5f + 0.5f,
        (float)cos(current_time) * 0.5f + 0.5f,
        0.0f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, color);
  }

  GLFWwindow *window_;
  GLuint rendering_program_;
  GLuint vertex_array_object_;
};

int main() {
  SimpleApp app{};
  app.Run();
  return 0;
}
