project(darwin_opengl)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
enable_language(CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL)
find_package(GLEW CONFIG REQUIRED)

include_directories(${GLEW_INCLUDE_DIRS})
link_libraries(${GLEW_LIBRARIES})
link_libraries(GLEW::GLEW)

add_executable(simple simple.cpp)
target_link_libraries(simple glfw ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
