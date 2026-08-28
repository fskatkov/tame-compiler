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
        includes/tame/backend/metal/msl_codegen.h
        includes/tame/backend/metal/lowering_engine.h)
target_include_directories(metal-cpp INTERFACE ${metal_cpp_SOURCE_DIR})

target_link_libraries(metal-cpp INTERFACE
    "-framework Metal"
    "-framework Foundation"
    "-framework QuartzCore"
)

set(METAL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/includes/tame/backend/metal/kernels/matmul_op_kernel.metal)
set(METAL_AIR ${CMAKE_CURRENT_BINARY_DIR}/tensor_ops.air)
set(METAL_LIB ${CMAKE_CURRENT_BINARY_DIR}/general.metallib)

add_custom_command(
    OUTPUT ${METAL_LIB}
    COMMAND xcrun -sdk macosx metal -c ${METAL_SRC} -o ${METAL_AIR}
    COMMAND xcrun -sdk macosx metallib ${METAL_AIR} -o ${METAL_LIB}
    DEPENDS ${METAL_SRC}
    COMMENT "Compiling Metal shaders"
)
add_custom_target(MetalShaders ALL DEPENDS ${METAL_LIB})

add_subdirectory(lib)
add_executable(tame_compiler main.cpp)

target_link_libraries(tame_compiler PRIVATE frontend backend support metal-cpp)

target_compile_options(tame_compiler PRIVATE -Wno-elaborated-enum-base)
add_dependencies(tame_compiler MetalShaders)

add_custom_command(TARGET tame_compiler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${METAL_LIB} $<TARGET_FILE_DIR:tame_compiler>/general.metallib
)

enable_testing()

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.18.0
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(tests)
