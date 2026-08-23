cmake_minimum_required(VERSION 4.3)
project(tame_compiler)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib)
add_executable(tame_compiler main.cpp)

target_link_libraries(tame_compiler PRIVATE frontend support)

enable_testing()

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.18.0
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(tests)
