#include "tame/backend/metal/msl_codegen.h"

using namespace tame::ast;
using namespace tame::backend;

KernelResult MSLCodeGenerator::generate(std::unique_ptr<Expr> expr, std::string_view dtype) {
    values.clear();
    variables.clear();

    const auto kernel_body = process_node(expr);

    std::stringstream kernel_builder;
    kernel_builder << "#include <metal_stdlib>\n\n";
    kernel_builder << "using namespace metal;\n\n";
    kernel_builder << "kernel void tensor_op(\n";

    for (std::size_t i = 0; i < values.size(); ++i) {
        kernel_builder << "    device const " << dtype << "* buffer_" << i << " [[buffer(" << i << ")]],\n";
    }

    const auto output_index = values.size();
    kernel_builder << "    device float* resulting_tensor [[buffer(" << output_index << ")]],\n";
    kernel_builder << "    constant uint& total_elements [[buffer(" << output_index + 1 << ")]],\n";
    kernel_builder << "    uint id [[thread_position_in_grid]]\n";
    kernel_builder << ") {\n";
    kernel_builder << "    if (id >= total_elements) { return; }\n";
    kernel_builder << "    resulting_tensor[id] = " << kernel_body << ";\n";
    kernel_builder << "}\n";

    return KernelResult{
        .source = kernel_builder.str(),
        .values = std::move(values)
    };
}

std::string MSLCodeGenerator::process_node(std::unique_ptr<Expr> &expr) {
    if (const auto *variable_expression = dynamic_cast<VarExpr *>(expr.get())) {
        const auto &variable_name = variable_expression->name.lexeme;

        if (!variables.contains(variable_name)) {
            variables[variable_name] = values.size();
            values.push_back(std::move(expr));
        } else {
            expr.reset();
        }

        return "buffer_" + std::to_string(variables[variable_name]) + "[id]";
    }

    if (auto *binary_expression = dynamic_cast<BinaryExpr *>(expr.get())) {
        if (binary_expression->operator_token.type == frontend::TokenType::PLUS_TOKEN
            || binary_expression->operator_token.type == frontend::TokenType::MINUS_TOKEN) {
            const auto lhs = process_node(binary_expression->lhs);
            const auto rhs = process_node(binary_expression->rhs);
            const auto op = binary_expression->operator_token.type == frontend::TokenType::PLUS_TOKEN ? "+" : "-";

            return "(" + lhs + op + rhs + ")";
        }
    }

    if (auto *literal_expression = dynamic_cast<LiteralExpr *>(expr.get())) {
        if (literal_expression->value.is<float>()) {
            return std::to_string(literal_expression->value.get<float>()) + "f";
        }

        if (literal_expression->value.is<int>()) {
            return std::to_string(literal_expression->value.get<int>());
        }
    }

    const std::size_t buffer_index = values.size();
    values.push_back(std::move(expr));
    return "buffer_" + std::to_string(buffer_index) + "[id]";
}
