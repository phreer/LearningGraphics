#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


GLuint CreateTextureFromImage(const std::string &filename) {
  GLuint texture_id;
  int width, height, num_channels;
  stbi_set_flip_vertically_on_load(
      true); // tell stb_image.h to flip loaded texture's on the y-axis.
  unsigned char *data =
      stbi_load(filename.c_str(), &width, &height, &num_channels, 0);
  GLenum format = num_channels == 3 ? GL_RGB : (num_channels == 4 ? GL_RGBA : (num_channels == 1 ? GL_RED : 0));
  if (data == nullptr) {
    LOG_ERROR("Failed to load texture %s\n", filename.c_str());
    exit(EXIT_FAILURE);
  }
  if (format == 0) {
    LOG_ERROR("Unsupported format\n");
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

GLuint CreateTextureFromCubeImage(const std::vector<std::string> &faces) {
  GLuint textureID;
  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(
      false); // tell stb_image.h to flip loaded texture's on the y-axis.
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
  for (unsigned int i = 0; i < faces.size(); i++) {
    unsigned char *data =
        stbi_load((std::string("../skybox/") + faces[i]).c_str(), &width,
                  &height, &nrChannels, 0);
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

