#include <gtest/gtest.h>

#include "tame/frontend/parser.h"

using namespace tame::frontend;
using namespace tame::ast;
using namespace tame::diagnostics;

namespace {
    class ParserTest : public ::testing::Test {
    protected:
        DiagnosticEngine diagnostic_engine;

        std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token> &tokens) {
            Parser parser(diagnostic_engine);
            return parser.run(tokens);
        }
    };
}

TEST_F(ParserTest, ParsesComplexNestedExpressions) {
    const auto statement = parse({
        Token(TokenType::PRINT_TOKEN, "print"),
        Token(TokenType::LEFT_PARENTHESIS_TOKEN, "("),
        Token(TokenType::INT_TOKEN, "1", 1),
        Token(TokenType::PLUS_TOKEN, "+"),
        Token(TokenType::INT_TOKEN, "2", 2),
        Token(TokenType::STAR_TOKEN, "*"),
        Token(TokenType::INT_TOKEN, "3", 3),
        Token(TokenType::RIGHT_PARENTHESIS_TOKEN, ")"),
        Token(TokenType::EOF_TOKEN, ""),
    });

    auto *print_statement = dynamic_cast<PrintStmt *>(statement.front().get());
    ASSERT_NE(print_statement, nullptr) << "expected `print`";

    const auto *addition_operation = dynamic_cast<BinaryExpr *>(print_statement->expression.get());
    EXPECT_EQ(addition_operation->operator_token.type, TokenType::PLUS_TOKEN);

    const auto *multiplication_operation = dynamic_cast<BinaryExpr *>(addition_operation->rhs.get());
    EXPECT_EQ(multiplication_operation->operator_token.type, TokenType::STAR_TOKEN);
}

TEST_F(ParserTest, ParsesVariableDeclaration) {
    const auto statement = parse({
        Token(TokenType::VAR_TOKEN, "var"),
        Token(TokenType::IDENTIFIER_TOKEN, "kernelCounter"),
        Token(TokenType::COLON_TOKEN, ":"),
        Token(TokenType::INT_TYPE_TOKEN, "i32"),
        Token(TokenType::EQUALS_TOKEN, "="),
        Token(TokenType::INT_TOKEN, "15", 15),
        Token(TokenType::EOF_TOKEN, "")
    });

    auto *variable_statement = dynamic_cast<VarStmt *>(statement.front().get());
    ASSERT_NE(variable_statement, nullptr) << "expected `kernelCounter` variable";

    EXPECT_EQ(variable_statement->name.lexeme, "kernelCounter");

    auto *variable_type = dynamic_cast<IntTypeAnnotation *>(variable_statement->type.get());
    ASSERT_NE(variable_type, nullptr) << "expected `int` variable type";

    auto *literal = dynamic_cast<LiteralExpr *>(variable_statement->initializer.get());
    EXPECT_EQ(literal->value.get<int>(), 15);
}

TEST_F(ParserTest, ParsesTensorDeclaration) {
    const auto statement = parse({
        Token(TokenType::VAR_TOKEN, "var"),
        Token(TokenType::IDENTIFIER_TOKEN, "basicMatrix"),
        Token(TokenType::COLON_TOKEN, ":"),
        Token(TokenType::TENSOR_TYPE_TOKEN, "tensor"),
        Token(TokenType::LEFT_ANGLE_TOKEN, "<"),
        Token(TokenType::INT_TOKEN, "1", 1),
        Token(TokenType::COMMA_TOKEN, ","),
        Token(TokenType::INT_TOKEN, "3", 3),
        Token(TokenType::RIGHT_ANGLE_TOKEN, ">"),
        Token(TokenType::EQUALS_TOKEN, "="),
        Token(TokenType::LEFT_BRACKET_TOKEN, "["),
        Token(TokenType::LEFT_BRACKET_TOKEN, "["),
        Token(TokenType::INT_TOKEN, "1", 1),
        Token(TokenType::COMMA_TOKEN, ","),
        Token(TokenType::INT_TOKEN, "2", 2),
        Token(TokenType::COMMA_TOKEN, ","),
        Token(TokenType::INT_TOKEN, "3", 3),
        Token(TokenType::RIGHT_BRACKET_TOKEN, "]"),
        Token(TokenType::RIGHT_BRACKET_TOKEN, "]"),
        Token(TokenType::EOF_TOKEN, "")
    });

    const auto *variable_statement = dynamic_cast<VarStmt *>(statement.front().get());
    ASSERT_NE(variable_statement, nullptr) << "expected `basicMatrix` variable";

    const auto *tensor_type = dynamic_cast<TensorTypeAnnotation *>(variable_statement->type.get());
    ASSERT_NE(tensor_type, nullptr) << "expected `tensor` type";

    const auto *tensor_literal = dynamic_cast<TensorLiteralExpr *>(variable_statement->initializer.get());
    ASSERT_NE(tensor_literal, nullptr) << "expected `tensor` literal";

    const std::vector<int> expected_shape{1, 3};
    EXPECT_EQ(tensor_literal->shape, expected_shape);
}
