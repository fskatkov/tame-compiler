#include <gtest/gtest.h>

#include "tame/frontend/lexer.h"

using namespace tame::frontend;
using namespace tame::diagnostics;

namespace {
    class LexerTest : public ::testing::Test {
    protected:
        DiagnosticEngine diagnostic_engine;

        std::vector<Token> tokenize(const std::string &source_str) {
            diagnostic_engine.init(source_str);
            Lexer lexer(source_str, diagnostic_engine);
            return lexer.tokenize();
        }
    };
}

TEST_F(LexerTest, HandlesEmptyInput) {
    const auto tokens = tokenize("");
    EXPECT_EQ(tokens.front().type, TokenType::EOF_TOKEN);
}

TEST_F(LexerTest, TokenizesSingleCharacterPunctuators) {
    const auto tokens = tokenize("()[]<>,.;:+-*/=");

    EXPECT_EQ(tokens.front().type, TokenType::LEFT_PARENTHESIS_TOKEN);
    EXPECT_EQ(tokens.at(4).type, TokenType::LEFT_ANGLE_TOKEN);
    EXPECT_EQ(tokens.at(8).type, TokenType::SEMICOLON_TOKEN);
    EXPECT_EQ(tokens.at(14).type, TokenType::EQUALS_TOKEN);
    EXPECT_EQ(tokens.back().type, TokenType::EOF_TOKEN);
}

TEST_F(LexerTest, IgnoresWhitespaceAndComments) {
    const auto tokens = tokenize("# This is a comment\n+");
    EXPECT_EQ(tokens.front().type, TokenType::PLUS_TOKEN);
    EXPECT_EQ(tokens.front().source_location.line, 2);
}

TEST_F(LexerTest, ScansNumericLiterals) {
    const auto tokens = tokenize("42 3.14");

    EXPECT_EQ(tokens.front().type, TokenType::INT_TOKEN);
    EXPECT_EQ(tokens.at(1).type, TokenType::FLOAT_TOKEN);
}

TEST_F(LexerTest, ReportsUnterminatedStringError) {
    const auto tokens = tokenize("\"hello");

    EXPECT_TRUE(diagnostic_engine.encountered_error);
    ASSERT_FALSE(diagnostic_engine.reports.empty());

    EXPECT_EQ(diagnostic_engine.reports.front().report_message, "unterminated string");
}

TEST_F(LexerTest, ReportsUnexpectedCharacterError) {
    const auto tokens = tokenize("&");

    EXPECT_TRUE(diagnostic_engine.encountered_error);
    ASSERT_FALSE(diagnostic_engine.reports.empty());

    EXPECT_EQ(diagnostic_engine.reports.front().report_message, "unexpected character");
}
