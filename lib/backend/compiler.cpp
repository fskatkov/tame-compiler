#include "tame/backend/compiler.h"

using namespace tame::frontend;
using namespace tame::ast;
using namespace tame::backend;

Compiler::Compiler(MetalEngine &metal_engine)
    : metal_engine(metal_engine), code_buffer(std::make_unique<CodeBuffer>()) {  }

std::unique_ptr<CodeBuffer> Compiler::run(const std::vector<std::unique_ptr<Stmt>> &statements) {
    for (const auto &statement : statements) {
        statement->accept(*this);
    }

    emit(std::to_underlying(Instruction::OP_RETURN), SourceLocation{});
    return std::move(code_buffer);
}

void Compiler::visit_var_stmt(VarStmt *stmt) {
    if (stmt->initializer) {
        stmt->initializer->accept(*this);
    } else {
        emit(std::to_underlying(Instruction::OP_NULL), stmt->name.source_location);
    }

    if (scope_depth > 0) {
        local_variables.emplace_back(stmt->name, scope_depth);
        return;
    }

    code_buffer->add(std::make_shared<std::string>(stmt->name.lexeme));

    emit(std::to_underlying(Instruction::OP_DEFINE_VARIABLE), SourceLocation{});
    emit(static_cast<std::uint8_t>(code_buffer->values.size() - 1), SourceLocation{});
}

void Compiler::visit_expr_stmt(ExprStmt *stmt) {
    stmt->expression->accept(*this);
    emit(std::to_underlying(Instruction::OP_POP), SourceLocation{});
}

void Compiler::visit_print_stmt(PrintStmt *stmt) {
    stmt->expression->accept(*this);
    emit(std::to_underlying(Instruction::OP_PRINT), SourceLocation{});
}

void Compiler::visit_var_expr(VarExpr *expr) {
    if (const auto arg = resolve_local_variable(expr->name); arg != -1) {
        emit(std::to_underlying(Instruction::OP_GET_LOCAL), SourceLocation{});
        emit(static_cast<std::uint8_t>(arg), SourceLocation{});
    } else {
        code_buffer->add(std::make_shared<std::string>(expr->name.lexeme));
        emit(std::to_underlying(Instruction::OP_GET_GLOBAL), SourceLocation{});
        emit(static_cast<std::uint8_t>(code_buffer->values.size() - 1), SourceLocation{});
    }
}

void Compiler::visit_assign_expr(AssignExpr *expr) {
    if (auto *variable_expression = dynamic_cast<VarExpr *>(expr->lhs.get())) {
        expr->rhs->accept(*this);

        if (const auto arg = resolve_local_variable(variable_expression->name); arg != -1) {
            emit(std::to_underlying(Instruction::OP_SET_LOCAL), SourceLocation{});
            emit(static_cast<std::uint8_t>(arg), SourceLocation{});
        } else {
            code_buffer->add(std::make_shared<std::string>(variable_expression->name.lexeme));
            emit(std::to_underlying(Instruction::OP_SET_GLOBAL), SourceLocation{});
            emit(static_cast<std::uint8_t>(code_buffer->values.size() - 1), SourceLocation{});
        }
    }
}

void Compiler::visit_binary_expr(BinaryExpr *expr) {
    expr->lhs->accept(*this);
    expr->rhs->accept(*this);

    switch (expr->operator_token.type) {
        case TokenType::PLUS_TOKEN:  emit(std::to_underlying(Instruction::OP_ADD), SourceLocation{}); break;
        case TokenType::MINUS_TOKEN: emit(std::to_underlying(Instruction::OP_SUB), SourceLocation{}); break;
        case TokenType::STAR_TOKEN:  emit(std::to_underlying(Instruction::OP_MUL), SourceLocation{}); break;
        case TokenType::SLASH_TOKEN: emit(std::to_underlying(Instruction::OP_DIV), SourceLocation{}); break;
        default: break;
    }
}

void Compiler::visit_literal_expr(LiteralExpr *expr) {
    code_buffer->insert_value(expr->value, expr->starting_position.source_location);
}

void Compiler::visit_tensor_literal_expr(TensorLiteralExpr *expr) {
    bool is_floating_point = true;
    if (!expr->data.empty()) {
        if (const auto *first_literal_expr = dynamic_cast<LiteralExpr *>(expr->data.front().get())) {
            is_floating_point = first_literal_expr->value.is<float>();
        }
    }

    const std::size_t element_size = is_floating_point ? sizeof(float) : sizeof(int);
    const std::size_t bytes_quantity = expr->data.size() * element_size;
    auto binary_blob_ptr = std::make_shared<std::vector<std::byte>>(bytes_quantity);

    auto *destination_ptr = binary_blob_ptr->data();
    for (const auto &element : expr->data) {
        auto *literal_expr = dynamic_cast<LiteralExpr *>(element.get());
        if (!literal_expr) {
            continue;
        }

        if (is_floating_point) {
            const float value = literal_expr->value.get<float>();
            std::memcpy(destination_ptr, &value, sizeof(float));
        } else {
            const int value = literal_expr->value.get<int>();
            std::memcpy(destination_ptr, &value, sizeof(int));
        }

        destination_ptr += element_size;
    }

    const auto blob_ptr_index = code_buffer->add(binary_blob_ptr);

    emit(std::to_underlying(Instruction::OP_BUILD_TENSOR), SourceLocation{});
    emit(static_cast<std::uint8_t>(is_floating_point ? 0 : 1), SourceLocation{});
    emit(static_cast<std::uint8_t>(expr->shape.size()), SourceLocation{});

    for (const auto &dimension : expr->shape) {
        emit(static_cast<std::uint8_t>(dimension), SourceLocation{});
    }

    emit(static_cast<std::uint8_t>(blob_ptr_index), SourceLocation{});
}

void Compiler::visit_gpu_launch_expr(GPULaunchExpr *expr) {
    const auto pipeline_id = metal_engine.compile_kernel(expr->source);
    code_buffer->add(static_cast<int>(pipeline_id));

    emit(std::to_underlying(Instruction::OP_CONSTANT), SourceLocation{});
    emit(static_cast<std::uint8_t>(code_buffer->values.size() - 1), SourceLocation{});

    for (const auto &value : expr->values) {
        value->accept(*this);
    }

    emit(std::to_underlying(Instruction::OP_EXECUTE_GPU), SourceLocation{});
    emit(static_cast<std::uint8_t>(expr->values.size()), SourceLocation{});
}

void Compiler::emit(const std::uint8_t &byte, const SourceLocation &location) const {
    code_buffer->update(byte, location);
}

int Compiler::resolve_local_variable(const Token &name) const {
    for (int i = static_cast<int>(local_variables.size()) - 1; i >= 0; --i) {
        if (local_variables.at(i).name.lexeme == name.lexeme) {
            return i;
        }
    }

    return -1;
}
