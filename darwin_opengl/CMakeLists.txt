project(darwin_opengl)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
enable_language(CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL)

add_executable(simple simple.cpp)
target_link_libraries(simple glfw ${OPENGL_LIBRARIES})
