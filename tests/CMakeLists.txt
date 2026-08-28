add_executable(tame_compiler_tests
    frontend_tests/lexer_tests.cpp
    frontend_tests/parser_tests.cpp
    backend_tests/compiler_tests.cpp
    backend_tests/virtual_machine_tests.cpp
    backend_tests/metal_backend_tests/metal_engine_tests.cpp
    backend_tests/metal_backend_tests/msl_codegen_tests.cpp
        backend_tests/metal_backend_tests/lowering_engine_tests.cpp
)

target_link_libraries(tame_compiler_tests PRIVATE
    GTest::gmock
    GTest::gtest_main
    frontend
    backend
    support
    metal-cpp
)

target_compile_options(tame_compiler_tests PRIVATE -Wno-elaborated-enum-base)
add_dependencies(tame_compiler_tests MetalShaders)

add_custom_command(TARGET tame_compiler_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${METAL_LIB} $<TARGET_FILE_DIR:tame_compiler_tests>/general.metallib
)

include(GoogleTest)
gtest_discover_tests(tame_compiler_tests)
