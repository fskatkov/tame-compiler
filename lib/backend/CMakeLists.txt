add_library(backend STATIC
    compiler.cpp
)

target_include_directories(backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../includes)
