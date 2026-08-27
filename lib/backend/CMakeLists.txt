add_library(backend STATIC
    compiler.cpp
    virtual_machine.cpp
    metal/metal_engine.cpp
    metal/msl_codegen.cpp
)

target_include_directories(backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../includes)

target_compile_options(backend PRIVATE -Wno-elaborated-enum-base)
