#include <stdio.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "utils.h"
#include "application.h"

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

  char *vert_shader_source = LoadVertexShader();
  if (vert_shader_source == NULL) {
    fprintf(stderr, "failed to load vertex shader file\n");
    exit(EXIT_FAILURE);
  }
  vert_shader = glCreateShader(GL_VERTEX_SHADER);
  printf(vert_shader_source);
  glShaderSource(vert_shader, 1, &vert_shader_source, NULL);
  glCompileShader(vert_shader);
  printf("vertex shader compiled\n");

  char *frag_shader_source = LoadFragmentShader();
  if (frag_shader_source == NULL) {
    fprintf(stderr, "failed to load fragment shader file\n");
    exit(EXIT_FAILURE);
  }
  frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  printf(frag_shader_source);
  glShaderSource(frag_shader, 1, &frag_shader_source, NULL);
  glCompileShader(frag_shader);
  printf("fragment shader compiled\n");

  program = glCreateProgram();
  printf("rendering program created");
  glAttachShader(program, vert_shader);
  printf("rendering program created");
  glAttachShader(program, frag_shader);
  printf("rendering program created");
  glLinkProgram(program);
  printf("rendering program created");

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

    if (!glfwInit()) {
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
  }

  void ShutDown_() override {
    // glDeleteVertexArrays(1, &vertex_array_object_);
    glDeleteProgram(rendering_program_);
    glfwTerminate();
  }

  void Render_() override {
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window_))
    {
      /* Render here */
      Draw__();
      /* Swap front and back buffers */
      glfwSwapBuffers(window_);
      /* Poll for and process events */
      glfwPollEvents();
    }
  }

  void Draw__() {
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
