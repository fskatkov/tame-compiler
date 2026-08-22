add_library(support STATIC
    diagnostic_engine.cpp
    driver.cpp
)

target_include_directories(support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../includes)
