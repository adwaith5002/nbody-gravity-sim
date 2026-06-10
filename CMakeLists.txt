cmake_minimum_required(VERSION 3.16)
project(gravity_sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Tell CMake where to find the libraries ---
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/libs/glfw")
set(GLEW_DIR "${CMAKE_SOURCE_DIR}/libs/glew")
set(GLM_DIR  "${CMAKE_SOURCE_DIR}/libs/glm")

# --- Triangle test executable ---
add_executable(triangle_test
    src/triangle_test.cpp
)

target_include_directories(triangle_test PRIVATE
    "${GLFW_DIR}/include"
    "${GLEW_DIR}/include"
    "${GLM_DIR}"
)

target_link_libraries(triangle_test PRIVATE
    "${GLFW_DIR}/lib-vc2022/glfw3.lib"
    "${GLEW_DIR}/lib/Release/x64/glew32.lib"
    opengl32
)

add_custom_command(TARGET triangle_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLEW_DIR}/bin/Release/x64/glew32.dll"
        $<TARGET_FILE_DIR:triangle_test>
)

# --- Main gravity sim executable ---
add_executable(gravity_sim
    src/main.cpp
)

target_include_directories(gravity_sim PRIVATE
    "${GLFW_DIR}/include"
    "${GLEW_DIR}/include"
    "${GLM_DIR}"
)

target_link_libraries(gravity_sim PRIVATE
    "${GLFW_DIR}/lib-vc2022/glfw3.lib"
    "${GLEW_DIR}/lib/Release/x64/glew32.lib"
    opengl32
)

add_custom_command(TARGET gravity_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLEW_DIR}/bin/Release/x64/glew32.dll"
        $<TARGET_FILE_DIR:gravity_sim>
)