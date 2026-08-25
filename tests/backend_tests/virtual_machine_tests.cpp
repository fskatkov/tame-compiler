#include <gtest/gtest.h>

#include "tame/backend/virtual_machine.h"
#include "tame/backend/compiler.h"
#include "tame/ast/stmt.h"
#include "tame/structures/token.h"
#include "tame/support/diagnostic_engine.h"

using namespace tame::ast;
using namespace tame::frontend;
using namespace tame::backend;
using namespace tame::diagnostics;

namespace {
    class VirtualMachineTest : public ::testing::Test {
    protected:
        static std::unique_ptr<CodeBuffer> compile(const std::vector<std::unique_ptr<Stmt>> &statements) {
            Compiler compiler;
            return compiler.run(statements);
        }
    };
}

TEST_F(VirtualMachineTest, ComputeBasicTwoOperandsAddtion) {
    std::vector<std::unique_ptr<Stmt>> statements;

    statements.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(42, Token{}),
            Token(TokenType::PLUS_TOKEN, "+"),
            std::make_unique<LiteralExpr>(15, Token{})
        )
    ));

    DiagnosticEngine diagnostic_engine;
    VirtualMachine virtual_machine(diagnostic_engine);

    testing::internal::CaptureStdout();

    const auto vm_execution_result = virtual_machine.execute(std::move(compile(statements)));

    const std::string vm_output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(vm_execution_result, VirtualMachineResult::OK) << "VM executed with runtime-errors!";
    EXPECT_EQ(vm_output, "57\n") << "seems like 57 is not exactly what we wanted to calculate";
}

TEST_F(VirtualMachineTest, ComputeVariableDeclarations) {
    std::vector<std::unique_ptr<Stmt>> statements;

    statements.push_back(std::make_unique<VarStmt>(
        Token(TokenType::IDENTIFIER_TOKEN, "firstKernelSize"),
        std::make_unique<IntTypeAnnotation>(),
        std::make_unique<LiteralExpr>(42, Token{})
    ));

    statements.push_back(std::make_unique<VarStmt>(
        Token(TokenType::IDENTIFIER_TOKEN, "secondKernelSize"),
        std::make_unique<IntTypeAnnotation>(),
        std::make_unique<LiteralExpr>(15, Token{})
    ));

    statements.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "firstKernelSize")),
            Token(TokenType::PLUS_TOKEN, "+"),
            std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "secondKernelSize"))
        )
    ));

    DiagnosticEngine diagnostic_engine;
    VirtualMachine virtual_machine(diagnostic_engine);

    testing::internal::CaptureStdout();

    const auto vm_execution_result = virtual_machine.execute(std::move(compile(statements)));

    const std::string vm_output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(vm_execution_result, VirtualMachineResult::OK) << "VM executed with runtime-errors!";
    EXPECT_EQ(vm_output, "57\n") << "seems like 57 is not exactly what we wanted to calculate";
}

TEST_F(VirtualMachineTest, ComputeTensorDeclaration) {
    std::vector<int> tensor_shape{2, 3};

    std::vector<std::unique_ptr<Expr>> tensor_data;
    tensor_data.push_back(std::make_unique<LiteralExpr>(1.0f, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(2.0f, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(3.0f, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(4.0f, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(5.0f, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(6.0f, Token{}));

    std::vector<std::unique_ptr<Stmt>> statements;
    statements.push_back(std::make_unique<VarStmt>(
        Token(TokenType::IDENTIFIER_TOKEN, "randomMatrix"),
        std::make_unique<TensorTypeAnnotation>(tensor_shape),
        std::make_unique<TensorLiteralExpr>(tensor_shape, std::move(tensor_data))
    ));
    statements.push_back(std::make_unique<PrintStmt>(std::make_unique<VarExpr>(
        Token(TokenType::IDENTIFIER_TOKEN, "randomMatrix")
    )));

    DiagnosticEngine diagnostic_engine;
    VirtualMachine virtual_machine(diagnostic_engine);

    testing::internal::CaptureStdout();

    const auto vm_execution_result = virtual_machine.execute(std::move(compile(statements)));

    const std::string vm_output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(vm_execution_result, VirtualMachineResult::OK) << "VM executed with runtime-errors!";
}

