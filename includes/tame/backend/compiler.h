#pragma once

#include "tame/structures/code_buffer.h"
#include "tame/ast/stmt.h"

namespace tame::backend {
    struct LocalVariable {
        frontend::Token name;
        int depth{0};
    };

    class Compiler : public ast::ExprVisitor, public ast::StmtVisitor {
    public:
        explicit Compiler();

        std::unique_ptr<CodeBuffer> code_buffer;

        void run(const std::vector<std::unique_ptr<ast::Stmt>> &statements);

        void visit_var_stmt(ast::VarStmt *stmt) override;
        void visit_expr_stmt(ast::ExprStmt *stmt) override;
        void visit_print_stmt(ast::PrintStmt *stmt) override;

        void visit_var_expr(ast::VarExpr *expr) override;
        void visit_assign_expr(ast::AssignExpr *expr) override;
        void visit_binary_expr(ast::BinaryExpr *expr) override;
        void visit_literal_expr(ast::LiteralExpr *expr) override;
        void visit_tensor_literal_expr(ast::TensorLiteralExpr *expr) override;
    private:
        std::vector<LocalVariable> local_variables;

        int scope_depth{0};

        void emit(const std::uint8_t &byte) const;
        [[nodiscard]] int resolve_local_variable(const frontend::Token &name) const;
    };
}
