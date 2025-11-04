cmake_minimum_required(VERSION 3.10)
project(fluid_sim_)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(fluid_sim_ src/main.cpp)

target_link_libraries(fluid_sim_
    OpenGL::GL
    glfw3
    GLEW::GLEW
)