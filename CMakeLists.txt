cmake_minimum_required(VERSION 3.20)

project(cableTool LANGUAGES CXX)

add_executable(cableTool
    core/cableTool.cpp
    core/command_registry.cpp
)

target_compile_features(cableTool PRIVATE cxx_std_17)

target_include_directories(cableTool
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_options(cableTool
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O2 -Wall -Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:/O2 /W4>
)
