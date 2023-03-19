#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <string>
#include <vector>

#include "glad/glad.h"

#include "log.h"

GLuint CreateTextureFromCubeImage(const std::vector<std::string> &faces);

GLuint CreateTextureFromImage(const std::string &filename);

#endif // !__IMAGE_H__
