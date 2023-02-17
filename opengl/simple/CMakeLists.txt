project(darwin_opengl)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
enable_language(CXX)

find_package(glfw3 CONFIG REQUIRED)
link_libraries(glfw)

if (APPLE)
        find_package(OpenGL)
        find_package(GLEW CONFIG REQUIRED)

        include_directories(${GLEW_INCLUDE_DIRS})
        link_libraries(${GLEW_LIBRARIES})
        link_libraries(GLEW::GLEW)
endif()

if (UNIX)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(glew REQUIRED glew)
        link_libraries(${glew_LIBRARIES})
endif()

add_executable(simple simple.cpp)
target_link_libraries(simple)

add_executable(simple-fb simple-fb.cpp)
target_link_libraries(simple-fb)