add_library(support STATIC lexer.cpp)

target_include_directories(support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../includes)
