cmake_minimum_required(VERSION 3.16)
project(CompilerBackend)

set(CMAKE_CXX_STANDARD 14)

add_library(Tree ../Tree/Tree.cpp)

add_executable(CompilerBackend main.cpp)

target_link_libraries(CompilerBackend Tree)