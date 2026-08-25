#include <gtest/gtest.h>

#include "tame/backend/compiler.h"
#include "tame/frontend/parser.h"

using namespace tame::frontend;
using namespace tame::ast;
using namespace tame::diagnostics;
using namespace tame::backend;

namespace {
    class CompilerTest : public ::testing::Test {
    protected:
        static std::unique_ptr<CodeBuffer> compile(const std::vector<std::unique_ptr<Stmt>> &statements) {
            Compiler compiler;
            return std::move(compiler.run(statements));
        }
    };
}

TEST_F(CompilerTest, EmitsBasicPrint) {
    std::vector<std::unique_ptr<Stmt>> statements;
    statements.push_back(std::move(std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(15, Token{}))));

    const auto code_buffer = compile(statements);

    ASSERT_NE(code_buffer, nullptr) << "code buffer should be initialized";
    EXPECT_EQ(code_buffer->data.front(), std::to_underlying(Instruction::OP_CONSTANT));
    EXPECT_EQ(code_buffer->data.at(2), std::to_underlying(Instruction::OP_PRINT));
}

TEST_F(CompilerTest, EmitsVariableDeclaration) {
    std::vector<std::unique_ptr<Stmt>> statements;
    statements.push_back(std::make_unique<VarStmt>(
        Token(TokenType::IDENTIFIER_TOKEN, "kernelSize"),
        std::make_unique<IntTypeAnnotation>(),
        std::make_unique<LiteralExpr>(42, Token{})
    ));
    statements.push_back(std::make_unique<PrintStmt>(std::make_unique<VarExpr>(
        Token(TokenType::IDENTIFIER_TOKEN, "kernelSize")
    )));

    const auto code_buffer = compile(statements);
    ASSERT_NE(code_buffer, nullptr) << "code buffer should be initialized";

    EXPECT_EQ(code_buffer->values.at(0).get<int>(), 42) << "expected kernelSize value in the constant pool";

    EXPECT_EQ(code_buffer->data.front(), std::to_underlying(Instruction::OP_CONSTANT));
    EXPECT_EQ(code_buffer->data.at(1), 0);
}

TEST_F(CompilerTest, EmitsTensorDeclaration) {
    std::vector<int> tensor_shape{2, 3};

    std::vector<std::unique_ptr<Expr>> tensor_data;
    tensor_data.push_back(std::make_unique<LiteralExpr>(1, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(2, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(3, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(4, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(5, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(6, Token{}));

    std::vector<std::unique_ptr<Stmt>> statements;
    statements.push_back(std::make_unique<VarStmt>(
        Token(TokenType::IDENTIFIER_TOKEN, "randomMatrix"),
        std::make_unique<TensorTypeAnnotation>(tensor_shape),
        std::make_unique<TensorLiteralExpr>(tensor_shape, std::move(tensor_data))
    ));
    statements.push_back(std::make_unique<PrintStmt>(std::make_unique<VarExpr>(
        Token(TokenType::IDENTIFIER_TOKEN, "randomMatrix")
    )));

    const auto code_buffer = compile(statements);
    ASSERT_NE(code_buffer, nullptr) << "code buffer should be initialized";

    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(code_buffer->values.at(i).get<int>(), i + 1);
    }
}