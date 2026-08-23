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
        DiagnosticEngine diagnostic_engine;

        std::unique_ptr<CodeBuffer> compile(const std::string &source_str) {
            diagnostic_engine.init(source_str);

            Parser parser(source_str, diagnostic_engine);
            const auto statements = parser.run();

            Compiler compiler;
            auto code_buffer = compiler.run(statements);
            return std::move(code_buffer);
        }
    };
}

TEST_F(CompilerTest, EmitsPrint) {
    const auto code_buffer = compile("print(15)");

    ASSERT_NE(code_buffer, nullptr) << "code buffer should be initialized";
    EXPECT_EQ(code_buffer->data.front(), std::to_underlying(Instruction::OP_CONSTANT));
    EXPECT_EQ(code_buffer->data.at(2), std::to_underlying(Instruction::OP_PRINT));
}

TEST_F(CompilerTest, EmitsVariableDeclaration) {
    const auto code_buffer = compile("var kernelSize: i32 = 42\nprint(kernelSize)");
    ASSERT_NE(code_buffer, nullptr) << "code buffer should be initialized";

    EXPECT_EQ(code_buffer->values.at(0).get<int>(), 42) << "expected kernelSize value in the constant pool";

    EXPECT_EQ(code_buffer->data.front(), std::to_underlying(Instruction::OP_CONSTANT));
    EXPECT_EQ(code_buffer->data.at(1), 0);
}

TEST_F(CompilerTest, EmitsTensorDeclaration) {
    const auto code_buffer = compile("var randomMatrix: tensor<2, 3> = [[1, 2, 3], [4, 5, 6]]");
    ASSERT_NE(code_buffer, nullptr) << "code buffer should be initialized";

    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(code_buffer->values.at(i).get<int>(), i + 1);
    }
}
