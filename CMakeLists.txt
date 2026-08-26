cmake_minimum_required(VERSION 4.3)
project(tame_compiler)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(
    metal_cpp
    GIT_REPOSITORY https://github.com/apple/metal-cpp.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(metal_cpp)

add_library(metal-cpp INTERFACE
        includes/tame/backend/metal_engine.h)
target_include_directories(metal-cpp INTERFACE ${metal_cpp_SOURCE_DIR})

target_link_libraries(metal-cpp INTERFACE
    "-framework Metal"
    "-framework Foundation"
    "-framework QuartzCore"
)

add_subdirectory(lib)
add_executable(tame_compiler main.cpp)

target_link_libraries(tame_compiler PRIVATE frontend backend support metal-cpp)

enable_testing()

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.18.0
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(tests)
