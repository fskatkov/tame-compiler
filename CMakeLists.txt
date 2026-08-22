cmake_minimum_required(VERSION 4.3)
project(tame_compiler)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib)
add_executable(tame_compiler main.cpp
)

target_link_libraries(tame_compiler PRIVATE frontend)
