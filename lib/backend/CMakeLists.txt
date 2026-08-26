add_library(backend STATIC
    compiler.cpp
    virtual_machine.cpp
    metal_engine.cpp
)

target_include_directories(backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../includes)

target_compile_options(backend PRIVATE -Wno-elaborated-enum-base)
