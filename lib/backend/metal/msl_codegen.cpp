#include "tame/backend/metal/msl_codegen.h"

using namespace tame::ast;
using namespace tame::backend;

KernelResult MSLCodeGenerator::generate(std::unique_ptr<Expr> expr) {
    values.clear();
    variables.clear();

    const auto kernel_body = process_node(expr);

    std::stringstream kernel_builder;
    kernel_builder << "#include <metal_stdlib>\n\n";
    kernel_builder << "using namespace metal;\n\n";

    kernel_builder << "constant uint MAX_RANK = 8;\n\n";
    kernel_builder << "kernel void tensor_op(\n";

    for (std::size_t i = 0; i < values.size(); ++i) {
        kernel_builder << "    device const DTYPE* buffer_" << i << " [[buffer(" << i << ")]],\n";
    }

    kernel_builder << "    device DTYPE* resulting_tensor [[buffer(" << values.size() << ")]],\n";
    kernel_builder << "    constant const uint* tensor_shape [[buffer(" << values.size() + 1 << ")]],\n";
    kernel_builder << "    constant const uint& tensor_rank [[buffer(" << values.size() + 2 << ")]],\n";
    kernel_builder << "    constant const uint& total_elements [[buffer(" << values.size() + 3 << ")]],\n";

    uint current_buffer_index = values.size() + 4;
    for (std::size_t i = 0; i < values.size(); ++i) {
        kernel_builder << "    constant const uint64_t* tensor_strides" << i << " [[buffer(" << current_buffer_index++ << ")]],\n";
        kernel_builder << "    constant const size_t& tensor_offset" << i << " [[buffer(" << current_buffer_index++ << ")]],\n";
    }

    kernel_builder << "    uint id [[thread_position_in_grid]]\n";
    kernel_builder << ") {\n";
    kernel_builder << "    if (id >= total_elements) { return; }\n\n";
    kernel_builder << "    uint coordinates[MAX_RANK];\n";
    kernel_builder << "    uint temporary_id = id;\n\n";
    kernel_builder << "    for (int i = tensor_rank - 1; i >= 0; --i) {\n";
    kernel_builder << "        coordinates[i] = temporary_id % tensor_shape[i];\n";
    kernel_builder << "        temporary_id /= tensor_shape[i];\n";
    kernel_builder << "    }\n\n";

    for (std::size_t i = 0; i < values.size(); ++i) {
        kernel_builder << "    size_t idx_" << i << " = tensor_offset" << i << ";\n";
        kernel_builder << "    for (uint i = 0; i < tensor_rank; ++i) {\n";
        kernel_builder << "        idx_" << i << " += coordinates[i] * tensor_strides" << i << "[i];\n";
        kernel_builder << "    }\n";
    }

    kernel_builder << "\n    resulting_tensor[id] = " << kernel_body << ";\n";
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

        const auto variable_index = std::to_string(variables[variable_name]);
        return std::format("buffer_{}[idx_{}]", variable_index, variable_index);
    }

    if (auto *binary_expression = dynamic_cast<BinaryExpr *>(expr.get())) {
        if (binary_expression->operator_token.type == frontend::TokenType::PLUS_TOKEN
            || binary_expression->operator_token.type == frontend::TokenType::MINUS_TOKEN) {
            const auto lhs = process_node(binary_expression->lhs);
            const auto rhs = process_node(binary_expression->rhs);
            const auto op = binary_expression->operator_token.type == frontend::TokenType::PLUS_TOKEN ? "+" : "-";

            return std::format("({} {} {})", lhs, op, rhs);
        }
    }

    if (auto *literal_expression = dynamic_cast<LiteralExpr *>(expr.get())) {
        if (literal_expression->value.is<float>()) {
            return std::format("{}f", literal_expression->value.get<float>());
        }

        if (literal_expression->value.is<int>()) {
            return std::format("{}", literal_expression->value.get<int>());
        }
    }

    const std::size_t buffer_index = values.size();
    values.push_back(std::move(expr));
    return std::format("buffer_{}[idx_{}]", std::to_string(buffer_index), std::to_string(buffer_index));
}
