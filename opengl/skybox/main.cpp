#include <cstdlib>
#include <ctime>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "application.h"
#include "shader.h"
#include "utils.h"

void error_callback(int error, const char *description) { puts(description); }

const std::vector<std::string> faces = {"right.jpg",  "left.jpg",  "top.jpg",
                                        "bottom.jpg", "front.jpg", "back.jpg"};

const float vertices[] = {
    // positions // normals // texture coords
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.5f,  -0.5f, -0.5f,
    0.0f,  0.0f,  -1.0f, 1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
    1.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  1.0f,  -0.5f,
    0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, 0.0f,
    0.0f,  -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,
    0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.5f,  0.5f,
    0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,
    1.0f,  1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,
    -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,
    1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  -0.5f,
    -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.0f,
    0.0f,  0.0f,  0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,
    0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,
    -0.5f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,
    0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
    0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,
    1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,
    0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f,  1.0f,  0.5f,
    -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,
    -1.0f, 0.0f,  1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f,
    0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,
    -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,
    0.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,
    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,
    0.0f,  1.0f};

float skybox_vertices[] = {
  // positions          
  -1.0f,  1.0f, -1.0f,
  -1.0f, -1.0f, -1.0f,
   1.0f, -1.0f, -1.0f,
   1.0f, -1.0f, -1.0f,
   1.0f,  1.0f, -1.0f,
  -1.0f,  1.0f, -1.0f,

  -1.0f, -1.0f,  1.0f,
  -1.0f, -1.0f, -1.0f,
  -1.0f,  1.0f, -1.0f,
  -1.0f,  1.0f, -1.0f,
  -1.0f,  1.0f,  1.0f,
  -1.0f, -1.0f,  1.0f,

   1.0f, -1.0f, -1.0f,
   1.0f, -1.0f,  1.0f,
   1.0f,  1.0f,  1.0f,
   1.0f,  1.0f,  1.0f,
   1.0f,  1.0f, -1.0f,
   1.0f, -1.0f, -1.0f,

  -1.0f, -1.0f,  1.0f,
  -1.0f,  1.0f,  1.0f,
   1.0f,  1.0f,  1.0f,
   1.0f,  1.0f,  1.0f,
   1.0f, -1.0f,  1.0f,
  -1.0f, -1.0f,  1.0f,

  -1.0f,  1.0f, -1.0f,
   1.0f,  1.0f, -1.0f,
   1.0f,  1.0f,  1.0f,
   1.0f,  1.0f,  1.0f,
  -1.0f,  1.0f,  1.0f,
  -1.0f,  1.0f, -1.0f,

  -1.0f, -1.0f, -1.0f,
  -1.0f, -1.0f,  1.0f,
   1.0f, -1.0f, -1.0f,
   1.0f, -1.0f, -1.0f,
  -1.0f, -1.0f,  1.0f,
   1.0f, -1.0f,  1.0f
};

glm::vec3 cube_positions[] = {
    glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
    glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
    glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

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

GLuint CreateTextureFromCubeImage(const std::vector<std::string> &faces) {
  GLuint textureID;
  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(false); // tell stb_image.h to flip loaded texture's on the y-axis.
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
  for (unsigned int i = 0; i < faces.size(); i++) {
    unsigned char *data =
        stbi_load((std::string("../skybox/") + faces[i]).c_str(), &width, &height, &nrChannels, 0);
    if (data) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                   0, GL_RGB, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    } else {
      LOG_ERROR("Cubemap failed to load at path: %s\n", faces[i].c_str());
      exit(EXIT_FAILURE);
    }
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  return textureID;
}

GLuint CreateTextureFromImage(const std::string &filename) {
  GLuint texture_id;
  int width, height, num_channels;
  stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
  unsigned char *data = stbi_load(filename.c_str(), &width, &height, &num_channels, 0);
  GLenum format = num_channels == 3 ? GL_RGB : (num_channels == 4 ? GL_RGBA : 0);
  if (format == 0) {
    LOG_ERROR("Unsupported format\n");
    exit(EXIT_FAILURE);
  }
  if (data == nullptr) {
    LOG_ERROR("Failed to load texture %s\n", filename.c_str());
    exit(EXIT_FAILURE);
  }
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(data);
  return texture_id;
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
    // Setup vertex array and vertex buffers
    glBindVertexArray(vertex_array_objects_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Setup textures
    glActiveTexture(GL_TEXTURE0);
    textures_[0] = CreateTextureFromImage("../texture.jpg");
    glBindTexture(GL_TEXTURE_2D, textures_[0]);
    glActiveTexture(GL_TEXTURE1);
    textures_[1] = CreateTextureFromImage("../doge.png");
    glBindTexture(GL_TEXTURE_2D, textures_[1]);

    camera_pos_ = glm::vec3(0.0, 0.0, 3.0);
    camera_up_ = glm::vec3(0.0, 1.0, 0.0);
    camera_front_ = glm::vec3(0.0, 0.0, -1);
    camera_target_ = glm::vec3(0.0, 0.0, 0.0);

    light_pos_ = glm::vec3(1.0, 1.0, 1.0);
    light_color_ = glm::vec3(1.0, 1.0, 0.8);

    scene_shader_.Use()
                 .SetUniform1i("u_tex0", 0).SetUniform1i("u_tex1", 1)
                 .SetUniform3fv("u_light_pos", 1, glm::value_ptr(light_pos_))
                 .SetUniform3fv("u_light_color", 1, glm::value_ptr(light_color_));

    // Prepare data for skybox shader.
    skybox_shader_ = Shader();
    skybox_shader_.SetVertexShader("../skybox.vert")
                  .SetFragmentShader("../skybox.frag")
                  .Build().Use();
    for (auto &v: skybox_vertices) {
      v *= 10.0;
    }
    glActiveTexture(GL_TEXTURE0);
    skybox_texture_ = CreateTextureFromCubeImage(faces);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_);
    skybox_shader_.SetUniform1i("u_skybox", skybox_texture_);
    glBindVertexArray(vertex_array_objects_[1]);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers_[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    skybox_shader_.SetUniform1i("u_skybox", 0);
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

      Draw__(current_time, view, projection);
      DrawSkybox__(current_time, view, projection);
      /* Swap front and back buffers */
      glfwSwapBuffers(window_);
      /* Poll for and process events */
      glfwPollEvents();
    }
  }

  void DrawSkybox__(double current_time, const glm::mat4 &view, const glm::mat4 &projection) {
    skybox_shader_.Use();
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(vertex_array_objects_[1]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_);
    glm::mat4 view1 = glm::mat4(glm::mat3(view));
    skybox_shader_.SetUniformMatrix4fv("u_projection",
                                       1, GL_FALSE, glm::value_ptr(projection))
                  .SetUniformMatrix4fv("u_view", 1, GL_FALSE, glm::value_ptr(view1));
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  void Draw__(double current_time, const glm::mat4 &view, const glm::mat4 &projection) {
    scene_shader_.Use();
    glDepthFunc(GL_LESS);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures_[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures_[1]);
    GLfloat color[3] = {0.0, 0.0, 0.0};

    scene_shader_.SetUniform3fv("u_color", 1, color)
                 .SetUniformMatrix4fv("u_projection",
                                      1, GL_FALSE, glm::value_ptr(projection))
                 .SetUniform3fv("u_camera_pos", 1, glm::value_ptr(camera_pos_))
                 .SetUniformMatrix4fv("u_view", 1, GL_FALSE, glm::value_ptr(view));

    // Start drawing triangles
    glBindVertexArray(vertex_array_objects_[0]);
    glClear(GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < 10; ++i) {
      // Setup model matrix
      glm::mat4 trans = glm::mat4(1.0f);
      trans = glm::rotate(trans, glm::radians(0.f + i * 20),
                          glm::vec3(0.0, 0.0, 1.0));
      trans = glm::translate(trans, cube_positions[i]);
      scene_shader_.SetUniformMatrix4fv("u_model", 1, GL_FALSE, glm::value_ptr(trans));
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
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
  Shader skybox_shader_;
  GLuint vertex_array_objects_[2];
  GLuint vertex_buffers_[2];
  GLuint textures_[2];
  GLuint skybox_texture_;
  glm::vec3 camera_pos_;
  glm::vec3 camera_front_;
  glm::vec3 camera_up_;
  glm::vec3 camera_target_;
  glm::vec3 light_pos_;
  glm::vec3 light_color_;
  unsigned int width_;
  unsigned int height_;
};

int main() {
  SimpleApp app{};
  app.Run();
  return 0;
}
