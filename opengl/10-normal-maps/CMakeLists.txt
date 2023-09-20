project(skybox)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)
include_directories(${PROJECT_SOURCE_DIR}/include)

macro(print_all_variables)
  message(STATUS "print_all_variables------------------------------------------{")
  get_cmake_property(_variableNames VARIABLES)
  foreach (_variableName ${_variableNames})
      message(STATUS "${_variableName}=${${_variableName}}")
  endforeach()
  message(STATUS "print_all_variables------------------------------------------}")
endmacro()

if (APPLE)
  include_directories(/usr/local/include/)
  find_package(OpenGL)
  link_libraries(${OPENGL_LIBRARIES})
endif()


find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
link_libraries(glfw assimp)

# lg means learning graphics
add_library(lg STATIC image.cpp)

# print_all_variables()
add_executable(skybox skybox.cpp ${PROJECT_SOURCE_DIR}/src/glad.c)
target_link_libraries(skybox lg)
