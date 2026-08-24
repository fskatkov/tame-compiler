add_executable(tame_compiler_tests
    frontend_tests/lexer_tests.cpp
    frontend_tests/parser_tests.cpp
    backend_tests/compiler_tests.cpp
    backend_tests/virtual_machine_tests.cpp
)

target_link_libraries(tame_compiler_tests PRIVATE
    GTest::gmock
    GTest::gtest_main
    frontend
    backend
    support
)

include(GoogleTest)
gtest_discover_tests(tame_compiler_tests)
