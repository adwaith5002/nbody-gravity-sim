cmake_minimum_required(VERSION 3.16)
project(gravity_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(triangle_test
    src/triangle_test.cpp
)

target_link_libraries(triangle_test PRIVATE
    glfw
    GLEW::GLEW
    OpenGL::GL
)

add_executable(gravity_sim
    src/main.cpp
)

target_link_libraries(gravity_sim PRIVATE
    glfw
    GLEW::GLEW
    OpenGL::GL
    glm::glm
)
