#include <cstdlib>
#include <ctime>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include <string>
#include <memory>
#include <unistd.h>

#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "application.h"
#include "image.h"
#include "shader.h"
#include "model.h"
#include "utils.h"

void error_callback(int error, const char *description) { puts(description); }

float brickwall_vertex_attributes = {
  -1.0,  0.0,  1.0,  0.0,  0.0,
  -1.0,  0.0, -1.0,  0.0,  1.0,
   1.0,  0.0, -1.0,  1.0,  1.0,
   1.0,  0.0,  1.0,  1.0,  0.0,
};

const int kDefaultWidth = 1920;
const int kDefaultHeight = 1080;
const double kMaxFov = 60.0;

double last_xpos, last_ypos;
float pitch = 0, yaw = -90;
float fov = 60.0;

void WindowResizeCallback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void MouseCallback(GLFWwindow *window, double xpos, double ypos) {
  double sensitivity = 0.05;
  static bool first_mouse = true;
  if (first_mouse) {
    last_xpos = xpos;
    last_ypos = ypos;
    first_mouse = false;
    return;
  }

  double xdiff = (xpos - last_xpos) * sensitivity;
  double ydiff = (ypos - last_ypos) * sensitivity;
  yaw += xdiff;
  pitch += ydiff;
  pitch = std::max<float>(std::min<float>(89.0, pitch), -89.0);

  last_xpos = xpos;
  last_ypos = ypos;
}

void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  double sensitivity = 2;
  fov -= yoffset * sensitivity;
  fov = std::max<float>(0.1, std::min<float>(kMaxFov, fov));
}

class ModelApp : public Application {
 public:
  ModelApp() : Application("Simple"), window_(nullptr) {};

 private:
  void StartUp_() override {
    glfwSetErrorCallback(error_callback);
    if (glfwInit() != GLFW_TRUE) {
      fprintf(stderr, "failed to initialize GLFW\n");
      exit(EXIT_FAILURE);
    }
    /* Create a windowed mode window and its OpenGL context */
    // glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    window_ = glfwCreateWindow(kDefaultWidth, kDefaultHeight,
                               app_name().c_str(), NULL, NULL);
    if (!window_) {
      fprintf(stderr, "failed to create window\n");
      exit(EXIT_FAILURE);
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window_);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window_, WindowResizeCallback);
    glfwSetCursorPosCallback(window_, MouseCallback);
    glfwSetScrollCallback(window_, ScrollCallback);

    // GLenum err = glewInit();
    // if (err != GLEW_OK) {
    //   fprintf(stderr, "failed to init glew\n");
    //   exit(EXIT_FAILURE); // or handle the error in a nicer way
    // }
    // if (!GLEW_VERSION_2_1) // check that the machine supports the 2.1 API.
    //   exit(EXIT_FAILURE);  // or handle the error in a nicer way
    gladLoadGL();

    // get version info
    const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte *version = glGetString(GL_VERSION);   // version as a string
    LOG("Renderer: %s\n", renderer);
    LOG("OpenGL version supported %s\n", version);

    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(
        GL_LESS); // depth-testing interprets a smaller value as "closer"

    scene_shader_ = Shader();
    scene_shader_.SetVertexShader("../shader.vert")
                 .SetFragmentShader("../shader.frag")
                 .Build().Use();

    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    width_ = width;
    height_ = height;
    glViewport(0, 0, width, height);

    glGenVertexArrays(sizeof(vertex_array_objects_) / sizeof(vertex_array_objects_[0]),
                      vertex_array_objects_);
    glGenBuffers(sizeof(vertex_buffers_) / sizeof(vertex_buffers_[0]), vertex_buffers_);

    // Prepare data for scene shader.
    camera_pos_ = glm::vec3(0.0, 0.0, 3.0);
    camera_up_ = glm::vec3(0.0, 1.0, 0.0);
    camera_front_ = glm::vec3(0.0, 0.0, -1);
    camera_target_ = glm::vec3(0.0, 0.0, 0.0);

    light_pos_ = glm::vec3(1.0, 1.0, 1.0);
    light_ambient_ = glm::vec3(1.0, 1.0, 0.8);
    light_diffuse_ = glm::vec3(1.0, 1.0, 0.8);
    light_specular_ = glm::vec3(1.0, 1.0, 0.8);

    scene_shader_.Use()
                 .SetUniform3fv("light.position", 1, glm::value_ptr(light_pos_))
                 .SetUniform3fv("light.ambient", 1, glm::value_ptr(light_ambient_))
                 .SetUniform3fv("light.diffuse", 1, glm::value_ptr(light_diffuse_))
                 .SetUniform3fv("light.specular", 1, glm::value_ptr(light_specular_));

    model_ = std::shared_ptr<Model>(new Model("../../texture/backpack/backpack.obj"));

    brickwall_texture_ = CreateTextureFromImage("../../texture/brickwall.jpg");
    brickwall_normal_ = CreateTexutreFromImage("../../texture/brickwall_normal.jpg");
    brickwall_shader_ = Shader();
    brickwall_shader.SetVertexShader("../brickwall.vert")
                    .SetFragmentShader("../brickwall.frag")
                    .Build().Use();
  }

  void ShutDown_() override {
    glDeleteVertexArrays(2, &vertex_array_objects_[0]);
    glfwTerminate();
  }

  void Render_() override {
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window_)) {
      ProcessInput__(window_);
      /* Render here */
      struct timeval tv;
      gettimeofday(&tv, nullptr);
      double current_time = (double)tv.tv_sec * 100 + (double)tv.tv_usec / 10000.f;
      float angle = 0.0;
      // Setup projection matrix
      glm::mat4 projection;
      projection = glm::perspective(glm::radians(fov), (float)width_ / height_,
                                    0.1f, 100.0f);

      glm::vec3 direction;
      direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
      direction.y = sin(glm::radians(pitch));
      direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
      camera_front_ = glm::normalize(direction);
      camera_target_ = camera_pos_ + camera_front_;
      // Setup view matrix
      glm::mat4 view = glm::mat4(1.0f);
      float view_angle = angle / 10;
      view = glm::lookAt(camera_pos_, camera_target_, camera_up_);

      const GLfloat background_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
      glClearBufferfv(GL_COLOR, 0, background_color);

      // Start drawing triangles
      glClear(GL_DEPTH_BUFFER_BIT);
      Draw__(current_time, view, projection);
      /* Swap front and back buffers */
      glfwSwapBuffers(window_);
      /* Poll for and process events */
      glfwPollEvents();
    }
  }

  void DrawBrickwall__(double current_time, const glm::mat4 &view, const glm::mat4 &projection) {
    scene_shader_.Use();
    glDepthFunc(GL_LESS);

    glm::mat4 model = glm::identity<glm::mat4>();
    brickwall_shader.SetUniformMatrix4fv("u_projection",
                                         1, GL_FALSE, glm::value_ptr(projection))
                    .SetUniform3fv("u_camera_pos", 1, glm::value_ptr(camera_pos_))
                    .SetUniformMatrix4fv("u_view", 1, GL_FALSE, glm::value_ptr(view))
                    .SetUniformMatrix4fv("u_model", 1, GL_FALSE, glm::value_ptr(model));
    glActiveTexture();

  }

  void Draw__(double current_time, const glm::mat4 &view, const glm::mat4 &projection) {
    scene_shader_.Use();
    glDepthFunc(GL_LESS);

    glm::mat4 model = glm::identity<glm::mat4>();
    scene_shader_.SetUniformMatrix4fv("u_projection",
                                      1, GL_FALSE, glm::value_ptr(projection))
                 .SetUniform3fv("u_camera_pos", 1, glm::value_ptr(camera_pos_))
                 .SetUniformMatrix4fv("u_view", 1, GL_FALSE, glm::value_ptr(view))
                 .SetUniformMatrix4fv("u_model", 1, GL_FALSE, glm::value_ptr(model));

    model_->Draw(scene_shader_);
  }

  void ProcessInput__(GLFWwindow *window) {
    const float camera_speed = 0.05f; // adjust accordingly
    glm::vec3 movement = glm::vec3(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      movement = camera_speed * camera_front_;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      movement = -camera_speed * camera_front_;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      movement =
          -glm::normalize(glm::cross(camera_front_, camera_up_)) * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      movement =
          glm::normalize(glm::cross(camera_front_, camera_up_)) * camera_speed;
    camera_pos_ += movement;
    camera_target_ += movement;
  }

  GLFWwindow *window_;
  Shader scene_shader_;
  Shader brickwall_shader_;
  GLuint vertex_array_objects_[1];
  GLuint vertex_buffers_[1];
  GLuint brickwall_texture_;
  GLuint brickwall_normal_;
  std::shared_ptr<Model> model_;
  glm::vec3 camera_pos_;
  glm::vec3 camera_front_;
  glm::vec3 camera_up_;
  glm::vec3 camera_target_;

  glm::vec3 light_pos_;
  glm::vec3 light_ambient_;
  glm::vec3 light_diffuse_;
  glm::vec3 light_specular_;

  unsigned int width_;
  unsigned int height_;
};

int main() {
  ModelApp app{};
  app.Run();
  return 0;
}
