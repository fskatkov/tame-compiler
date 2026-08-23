#include "tame/backend/compiler.h"

using namespace tame::frontend;
using namespace tame::ast;
using namespace tame::backend;

void Compiler::run(const std::vector<std::unique_ptr<Stmt>> &statements) {
    for (const auto &statement : statements) {
        statement->accept(*this);
    }

    emit(std::to_underlying(Instruction::OP_RETURN));
}

void Compiler::visit_var_stmt(VarStmt *stmt) {

}

void Compiler::visit_expr_stmt(ExprStmt *stmt) {

}

void Compiler::visit_print_stmt(PrintStmt *stmt) {

}

void Compiler::visit_var_expr(VarExpr *expr) {

}

void Compiler::visit_assign_expr(AssignExpr *expr) {

}

void Compiler::visit_binary_expr(BinaryExpr *expr) {

}

void Compiler::visit_literal_expr(LiteralExpr *expr) {

}

void Compiler::visit_tensor_literal_expr(TensorLiteralExpr *expr) {

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
