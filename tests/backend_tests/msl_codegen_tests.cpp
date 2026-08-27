#include <gtest/gtest.h>

#include "tame/backend/metal/msl_codegen.h"

using namespace tame::ast;
using namespace tame::frontend;
using namespace tame::backend;

namespace {
    class MSLCodeGeneratorTest : public ::testing::Test {
    protected:
        static KernelResult generate(std::unique_ptr<Expr> expr) {
            MSLCodeGenerator code_generator;
            return code_generator.generate(std::move(expr));
        }
    };
}

TEST_F(MSLCodeGeneratorTest, GenerateTensorAdditionMSLKernel) {
    testing::internal::CaptureStdout();

    std::vector<int> tensor_shape{2, 3};

    std::vector<std::unique_ptr<Expr>> tensor_data;
    tensor_data.push_back(std::make_unique<LiteralExpr>(1, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(2, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(3, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(4, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(5, Token{}));
    tensor_data.push_back(std::make_unique<LiteralExpr>(6, Token{}));

    auto generated_msl_kernel = generate(std::make_unique<BinaryExpr>(
        std::make_unique<TensorLiteralExpr>(tensor_shape, std::move(tensor_data)),
        Token(TokenType::PLUS_TOKEN),
        std::make_unique<TensorLiteralExpr>(tensor_shape, std::move(tensor_data))
    ));

    const std::string msl_codegen_output = testing::internal::GetCapturedStdout();
}
