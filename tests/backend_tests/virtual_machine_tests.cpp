#include <gtest/gtest.h>

#include "tame/backend/virtual_machine.h"
#include "tame/backend/compiler.h"
#include "tame/ast/stmt.h"
#include "tame/structures/token.h"

using namespace tame::ast;
using namespace tame::frontend;
using namespace tame::backend;

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
            std::make_unique<LiteralExpr>(42),
            Token(TokenType::PLUS_TOKEN, "+"),
            std::make_unique<LiteralExpr>(15)
        )
    ));

    VirtualMachine virtual_machine;

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
        std::make_unique<LiteralExpr>(42)
    ));

    statements.push_back(std::make_unique<VarStmt>(
        Token(TokenType::IDENTIFIER_TOKEN, "secondKernelSize"),
        std::make_unique<IntTypeAnnotation>(),
        std::make_unique<LiteralExpr>(15)
    ));

    statements.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "firstKernelSize")),
            Token(TokenType::PLUS_TOKEN, "+"),
            std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "secondKernelSize"))
        )
    ));

    VirtualMachine virtual_machine;

    testing::internal::CaptureStdout();

    const auto vm_execution_result = virtual_machine.execute(std::move(compile(statements)));

    const std::string vm_output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(vm_execution_result, VirtualMachineResult::OK) << "VM executed with runtime-errors!";
    EXPECT_EQ(vm_output, "57\n") << "seems like 57 is not exactly what we wanted to calculate";
}
