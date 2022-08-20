cmake_minimum_required(VERSION 3.10)
SET(ASMFLAGS "-x assembler-with-cpp")
enable_language(C ASM)

project(smc)
include_directories(
    vendor/glfw/include
    vendor/glad/include
    vendor/raylib/include
    vendor/raygui/include
)
add_subdirectory(
    vendor/glfw
)

add_executable(smc 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/res/res.s
    ${CMAKE_CURRENT_SOURCE_DIR}/rc/embeds.s
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/src/gl.c
)
target_link_libraries(smc glfw)
