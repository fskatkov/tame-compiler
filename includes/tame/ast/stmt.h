#pragma once

#include "expr.h"
#include "types.h"

namespace tame::ast {
    struct VarStmt;
    struct ExprStmt;
    struct PrintStmt;

    struct StmtVisitor {
        virtual ~StmtVisitor() = default;

        virtual void visit_var_stmt(VarStmt *stmt) = 0;
        virtual void visit_expr_stmt(ExprStmt *stmt) = 0;
        virtual void visit_print_stmt(PrintStmt *stmt) = 0;
    };

    struct Stmt {
        virtual ~Stmt() = default;
        virtual void accept(StmtVisitor &visitor) = 0;
    };

    struct VarStmt : public Stmt {
        frontend::Token name;
        std::unique_ptr<TypeAnnotation> type;
        std::unique_ptr<Expr> initializer;

        explicit VarStmt(frontend::Token name, std::unique_ptr<TypeAnnotation> type, std::unique_ptr<Expr> initializer)
            : name(std::move(name)), type(std::move(type)), initializer(std::move(initializer)) {  }

        void accept(StmtVisitor &visitor) override {
            visitor.visit_var_stmt(this);
        }
    };

    struct ExprStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        explicit ExprStmt(std::unique_ptr<Expr> expression)
            : expression(std::move(expression)) {  }

        void accept(StmtVisitor &visitor) override {
            visitor.visit_expr_stmt(this);
        }
    };

    struct PrintStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        explicit PrintStmt(std::unique_ptr<Expr> expression)
            : expression(std::move(expression)) {  }

        void accept(StmtVisitor &visitor) override {
            visitor.visit_print_stmt(this);
        }
    };
}
