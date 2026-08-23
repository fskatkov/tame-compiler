#include <gtest/gtest.h>

#include "tame/frontend/lexer.h"

using namespace tame::frontend;
using namespace tame::diagnostics;

static std::vector<Token> tokenize(const std::string &input) {
    DiagnosticEngine diagnostics_engine;
    Lexer lexer(input, diagnostics_engine);
    return lexer.tokenize();
}

TEST(LexerTest, HandlesEmptyInput) {
    const auto tokens = tokenize("");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens.at(0).type, TokenType::EOF_TOKEN);
}
