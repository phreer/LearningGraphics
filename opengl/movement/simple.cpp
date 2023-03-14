#include <cstdlib>
#include <ctime>
#include <glm/ext/matrix_clip_space.hpp>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "application.h"
#include "utils.h"

void error_callback(int error, const char *description) { puts(description); }

const float vertex_data[] = {
  -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
  0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
  0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
  0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
  -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
  -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
  -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
  0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
  0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
  -0.5f, 0.5f, 0.5f, 0.0f, 1.0f,
  -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
  -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
  -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
  -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
  -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
  -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
  -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
  0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
  0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
  0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
  0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
  -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
  0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
  0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
  0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
  -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
  -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
  -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
  0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
  0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
  -0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
  -0.5f, 0.5f, -0.5f, 0.0f, 1.0f
};

glm::vec3 cube_positions[] = {
  glm::vec3( 0.0f, 0.0f, 0.0f),
  glm::vec3( 2.0f, 5.0f, -15.0f),
  glm::vec3(-1.5f, -2.2f, -2.5f),
  glm::vec3(-3.8f, -2.0f, -12.3f),
  glm::vec3( 2.4f, -0.4f, -3.5f),
  glm::vec3(-1.7f, 3.0f, -7.5f),
  glm::vec3( 1.3f, -2.0f, -2.5f),
  glm::vec3( 1.5f, 2.0f, -2.5f),
  glm::vec3( 1.5f, 0.2f, -1.5f),
  glm::vec3(-1.3f, 1.0f, -1.5f)
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
    printf("%s: failed to compile fragment shader\n", __func__);
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
    window_ = glfwCreateWindow(kDefaultWidth, kDefaultHeight, app_name().c_str(), NULL, NULL);
    if (!window_) {
      fprintf(stderr, "failed to create window\n");
      exit(EXIT_FAILURE);
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window_);
    glfwSetFramebufferSizeCallback(window_, WindowResizeCallback);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window_, MouseCallback);
    glfwSetScrollCallback(window_, ScrollCallback);

    // GLenum err = glewInit();
    // if (err != GLEW_OK) {
    //   fprintf(stderr, "failed to init glew");
    //   exit(EXIT_FAILURE); // or handle the error in a nicer way
    // }
    // if (!GLEW_VERSION_2_1) // check that the machine supports the 2.1 API.
    //   exit(EXIT_FAILURE);  // or handle the error in a nicer way
    int ret = gladLoadGL();
    if (ret == 0) {
      fprintf(stderr, "failed to load OpenGL functions\n");
      exit(EXIT_FAILURE);
    }

    // get version info
    const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte *version = glGetString(GL_VERSION);   // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

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

		camera_pos_ = glm::vec3(0.0, 0.0, 3.0);
		camera_up_ = glm::vec3(0.0, 1.0, 0.0);
		camera_front_ = glm::vec3(0.0, 0.0, -1);
		camera_target_ = glm::vec3(0.0, 0.0, 0.0);
  }

  void ShutDown_() override {
    glDeleteVertexArrays(1, &vertex_array_object_);
    glDeleteProgram(rendering_program_);
    glfwTerminate();
  }

  void Render_() override {
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window_)) {
			ProcessInput__(window_);
      /* Render here */
      struct timeval tv;
      gettimeofday(&tv, nullptr);
      Draw__((double) tv.tv_sec * 100 + (double) tv.tv_usec / 10000.f);
      /* Swap front and back buffers */
      glfwSwapBuffers(window_);
      /* Poll for and process events */
      glfwPollEvents();
    }
  }

  void Draw__(double current_time) {
    static double last_time = -1.0;

    if (last_time == -1.0) last_time = current_time;
    // printf("Start drawing new frame, current_time = %lf\n", current_time);
    // float angle = current_time - (long) (current_time / 360) * 360;
    float angle = 0.0;
    float camera_pos_rotate_angle = (current_time - last_time) * 0.36;
    unsigned int view_loc = glGetUniformLocation(rendering_program_, "u_view");

    glUniform3f(glGetUniformLocation(rendering_program_, "u_color"), 0.0, 0.0, sin(glm::radians(angle)));

    // Setup projection matrix
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(fov), (float) width_ / height_, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(rendering_program_, "u_projection"),
                       1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 camera_pos_rotate = glm::rotate(glm::mat4(1.0f), glm::radians(camera_pos_rotate_angle), glm::vec3(0.0, 1.0, 0.0));
    glm::vec3 pos_tmp = camera_pos_;
    pos_tmp.x = (camera_pos_rotate * glm::vec4(camera_pos_, 1)).x;
    pos_tmp.y = (camera_pos_rotate * glm::vec4(camera_pos_, 1)).y;
    pos_tmp.z = (camera_pos_rotate * glm::vec4(camera_pos_, 1)).z;
    camera_pos_ = pos_tmp;

		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		camera_front_ = glm::normalize(direction);
    camera_target_ = camera_pos_ + camera_front_;
    // Setup view matrix
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::lookAt(camera_pos_, camera_target_, camera_up_);
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));

    // Start drawing triangles
    glUseProgram(rendering_program_);
    glBindVertexArray(vertex_array_object_);
    const GLfloat color[] = {0.5f, 0.5f, 0.5f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, color);
    glClear(GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < 10; ++i) {
      // Setup model matrix
      unsigned int model_loc = glGetUniformLocation(rendering_program_, "u_model");
      glm::mat4 trans = glm::mat4(1.0f);
      trans = glm::rotate(trans, glm::radians(angle + i * 20), glm::vec3(0.0, 0.0, 1.0));
      trans = glm::translate(trans, cube_positions[i]);
      glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(trans));
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    last_time = current_time;
  }

	void ProcessInput__(GLFWwindow *window) {
		const float camera_speed = 0.05f; // adjust accordingly
		glm::vec3 movement = glm::vec3(0.0f);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			movement = camera_speed * camera_front_;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			movement = -camera_speed * camera_front_;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			movement = -glm::normalize(glm::cross(camera_front_, camera_up_)) * camera_speed;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			movement = glm::normalize(glm::cross(camera_front_, camera_up_)) * camera_speed;
		camera_pos_ += movement;
		camera_target_ += movement;
	}

  GLFWwindow *window_;
  GLuint rendering_program_;
  GLuint vertex_array_object_;
  GLuint vertex_buffer_;
  GLuint textures_[2];
	glm::vec3 camera_pos_;
	glm::vec3 camera_front_;
	glm::vec3 camera_up_;
	glm::vec3 camera_target_;
  unsigned int width_;
  unsigned int height_;
};

int main() {
  SimpleApp app{};
  app.Run();
  return 0;
}
