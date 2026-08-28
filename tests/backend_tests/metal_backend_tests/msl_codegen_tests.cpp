#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tame/ast/stmt.h"
#include "tame/backend/metal/msl_codegen.h"

using namespace tame::ast;
using namespace tame::frontend;
using namespace tame::backend;

TEST(MSLCodeGeneratorTest, GeneratesAdditionExpression) {
    MSLCodeGenerator code_generator;

    const auto generated_kernel = code_generator.generate(std::make_unique<BinaryExpr>(
        std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "firstMatrix")),
        Token(TokenType::PLUS_TOKEN, "+"),
        std::make_unique<VarExpr>(Token(TokenType::IDENTIFIER_TOKEN, "secondMatrix"))
    ));

    EXPECT_THAT(generated_kernel.source, testing::HasSubstr("resulting_tensor[id] = (buffer_0[id]+buffer_1[id]);"));
    EXPECT_EQ(generated_kernel.values.size(), 2);
}
