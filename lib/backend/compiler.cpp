#include "tame/backend/compiler.h"

using namespace tame::frontend;
using namespace tame::ast;
using namespace tame::backend;

Compiler::Compiler() : code_buffer(std::make_unique<CodeBuffer>()) {  }

std::unique_ptr<CodeBuffer> Compiler::run(const std::vector<std::unique_ptr<Stmt>> &statements) {
    for (const auto &statement : statements) {
        statement->accept(*this);
    }

    emit(std::to_underlying(Instruction::OP_RETURN));
    return std::move(code_buffer);
}

void Compiler::visit_var_stmt(VarStmt *stmt) {
    if (stmt->initializer) {
        stmt->initializer->accept(*this);
    } else {
        emit(std::to_underlying(Instruction::OP_NULL));
    }

    if (scope_depth > 0) {
        local_variables.emplace_back(stmt->name, scope_depth);
        return;
    }

    code_buffer->add(std::make_shared<std::string>(stmt->name.lexeme));

    emit(std::to_underlying(Instruction::OP_DEFINE_VARIABLE));
    emit(static_cast<std::uint8_t>(code_buffer->values.size() - 1));
}

void Compiler::visit_expr_stmt(ExprStmt *stmt) {
    stmt->expression->accept(*this);
    emit(std::to_underlying(Instruction::OP_POP));
}

void Compiler::visit_print_stmt(PrintStmt *stmt) {
    stmt->expression->accept(*this);
    emit(std::to_underlying(Instruction::OP_PRINT));
}

void Compiler::visit_var_expr(VarExpr *expr) {
    if (const auto arg = resolve_local_variable(expr->name); arg != -1) {
        emit(std::to_underlying(Instruction::OP_GET_LOCAL));
        emit(static_cast<std::uint8_t>(arg));
    } else {
        code_buffer->add(std::make_shared<std::string>(expr->name.lexeme));
        emit(std::to_underlying(Instruction::OP_GET_GLOBAL));
        emit(static_cast<std::uint8_t>(code_buffer->values.size() - 1));
    }
}

void Compiler::visit_assign_expr(AssignExpr *expr) {
    if (auto *variable_expression = dynamic_cast<VarExpr *>(expr->lhs.get())) {
        expr->rhs->accept(*this);

        if (const auto arg = resolve_local_variable(variable_expression->name); arg != -1) {
            emit(std::to_underlying(Instruction::OP_SET_LOCAL));
            emit(static_cast<std::uint8_t>(arg));
        } else {
            code_buffer->add(std::make_shared<std::string>(variable_expression->name.lexeme));
            emit(std::to_underlying(Instruction::OP_SET_GLOBAL));
            emit(static_cast<std::uint8_t>(code_buffer->values.size() - 1));
        }
    }
}

void Compiler::visit_binary_expr(BinaryExpr *expr) {
    expr->lhs->accept(*this);
    expr->rhs->accept(*this);

    switch (expr->operator_token.type) {
        case TokenType::PLUS_TOKEN:  emit(std::to_underlying(Instruction::OP_ADD)); break;
        case TokenType::MINUS_TOKEN: emit(std::to_underlying(Instruction::OP_SUB)); break;
        case TokenType::STAR_TOKEN:  emit(std::to_underlying(Instruction::OP_MUL)); break;
        case TokenType::SLASH_TOKEN: emit(std::to_underlying(Instruction::OP_DIV)); break;
        default: break;
    }
}

void Compiler::visit_literal_expr(LiteralExpr *expr) {
    code_buffer->insert_value(expr->value);
}

void Compiler::visit_tensor_literal_expr(TensorLiteralExpr *expr) {
    for (const auto &element : expr->data) {
        element->accept(*this);
    }

    emit(std::to_underlying(Instruction::OP_BUILD_TENSOR));
    emit(static_cast<std::uint8_t>(expr->shape.size()));

    for (const auto &dimension : expr->shape) {
        emit(static_cast<std::uint8_t>(dimension));
    }
}

void Compiler::emit(const std::uint8_t &byte) const {
    code_buffer->update(byte);
}

int Compiler::resolve_local_variable(const Token &name) const {
    for (int i = static_cast<int>(local_variables.size()) - 1; i >= 0; --i) {
        if (local_variables.at(i).name.lexeme == name.lexeme) {
            return i;
        }
    }

    return -1;
}
