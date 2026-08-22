add_library(frontend STATIC lexer.cpp)

target_include_directories(frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../includes)
