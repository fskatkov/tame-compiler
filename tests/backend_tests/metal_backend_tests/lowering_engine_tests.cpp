#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tame/ast/stmt.h"
#include "tame/backend/metal/lowering_engine.h"

using namespace tame::ast;
using namespace tame::frontend;
using namespace tame::backend;

TEST(LoweringEngineTest, LowersBinaryAddition) {
    LoweringEngine lowering_engine;

    std::vector<std::unique_ptr<Stmt>> statements;
    statements.push_back(std::make_unique<ExprStmt>(std::make_unique<BinaryExpr>(
        std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "firstMatrix")),
        Token(TokenType::PLUS_TOKEN, "+"),
        std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "secondMatrix"))
    )));

    lowering_engine.process(statements);

    auto *expression_statement = dynamic_cast<ExprStmt *>(statements.front().get());
    ASSERT_NE(expression_statement, nullptr);

    auto *gpu_launch_expr = dynamic_cast<GPULaunchExpr *>(expression_statement->expression.get());
    ASSERT_NE(gpu_launch_expr, nullptr);

    EXPECT_THAT(gpu_launch_expr->source, testing::HasSubstr("kernel void tensor_op"));
    EXPECT_EQ(gpu_launch_expr->values.size(), 2);
}

TEST(LoweringEngineTest, PreservesMatmul) {
    LoweringEngine lowering_engine;

    std::vector<std::unique_ptr<Stmt>> statements;
    statements.push_back(std::make_unique<ExprStmt>(std::make_unique<BinaryExpr>(
        std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "firstMatrix")),
        Token(TokenType::STAR_TOKEN, "*"),
        std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "secondMatrix"))
    )));

    lowering_engine.process(statements);

    auto *expression_statement = dynamic_cast<ExprStmt *>(statements.front().get());
    ASSERT_NE(expression_statement, nullptr);

    auto *binary_expression = dynamic_cast<BinaryExpr *>(expression_statement->expression.get());
    EXPECT_NE(binary_expression, nullptr);
    EXPECT_EQ(binary_expression->operator_token.type, TokenType::STAR_TOKEN);
}
