add_executable(tame_compiler_tests frontend_tests/lexer_tests.cpp)

target_link_libraries(tame_compiler_tests PRIVATE GTest::gmock GTest::gtest_main frontend support)

include(GoogleTest)
gtest_discover_tests(tame_compiler_tests)
