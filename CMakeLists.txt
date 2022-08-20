cmake_minimum_required(VERSION 3.10)
SET(ASMFLAGS "-x assembler-with-cpp")
enable_language(C ASM)

project(smc)
include_directories(
    vendor/raylib/src
    vendor/raygui/src
)
add_subdirectory(vendor/raylib)

add_executable(smc 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/res/res.s
    ${CMAKE_CURRENT_SOURCE_DIR}/rc/embeds.s
)
target_link_libraries(smc raylib)
