add_library(frontend STATIC
    lexer.cpp
    parser.cpp
)

target_include_directories(frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../includes)

target_compile_options(frontend PRIVATE -Wno-elaborated-enum-base)
