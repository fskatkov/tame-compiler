#pragma once

#include "tame/structures/token.h"

namespace tame::ast {
    struct VarExpr;
    struct AssignExpr;
    struct BinaryExpr;
    struct LiteralExpr;
    struct TensorLiteralExpr;
    struct GPULaunchExpr;

    struct ExprVisitor {
        virtual ~ExprVisitor() = default;

        virtual void visit_var_expr(VarExpr *expr) = 0;
        virtual void visit_assign_expr(AssignExpr *expr) = 0;
        virtual void visit_binary_expr(BinaryExpr *expr) = 0;
        virtual void visit_literal_expr(LiteralExpr *expr) = 0;
        virtual void visit_tensor_literal_expr(TensorLiteralExpr *expr) = 0;
        virtual void visit_gpu_launch_expr(GPULaunchExpr *expr) = 0;
    };

    struct Expr {
        virtual ~Expr() = default;
        virtual void accept(ExprVisitor &visitor) = 0;
    };

    struct VarExpr : public Expr {
        frontend::Token name;

        explicit VarExpr(frontend::Token name)
            : name(std::move(name)) {  }

        void accept(ExprVisitor &visitor) override {
            visitor.visit_var_expr(this);
        }
    };

    struct AssignExpr : public Expr {
        std::unique_ptr<Expr> lhs;
        frontend::Token operator_token;
        std::unique_ptr<Expr> rhs;

        explicit AssignExpr(std::unique_ptr<Expr> lhs, frontend::Token operator_token, std::unique_ptr<Expr> rhs)
            : lhs(std::move(lhs)), operator_token(std::move(operator_token)), rhs(std::move(rhs)) {  }

        void accept(ExprVisitor &visitor) override {
            visitor.visit_assign_expr(this);
        }
    };

    struct BinaryExpr : public Expr {
        std::unique_ptr<Expr> lhs;
        frontend::Token operator_token;
        std::unique_ptr<Expr> rhs;

        explicit BinaryExpr(std::unique_ptr<Expr> lhs, frontend::Token operator_token, std::unique_ptr<Expr> rhs)
            : lhs(std::move(lhs)), operator_token(std::move(operator_token)), rhs(std::move(rhs)) {  }

        void accept(ExprVisitor &visitor) override {
            visitor.visit_binary_expr(this);
        }
    };

    struct LiteralExpr : public Expr {
        frontend::Value value;
        frontend::Token starting_position;

        explicit LiteralExpr(frontend::Value value, frontend::Token starting_position)
            : value(std::move(value)), starting_position(std::move(starting_position)) {  }

        void accept(ExprVisitor &visitor) override {
            visitor.visit_literal_expr(this);
        }
    };

    struct TensorLiteralExpr : public Expr {
        std::vector<int> shape;
        std::vector<std::unique_ptr<Expr>> data;

        explicit TensorLiteralExpr(std::vector<int> shape, std::vector<std::unique_ptr<Expr>> data)
            : shape(std::move(shape)), data(std::move(data)) {  }

        void accept(ExprVisitor &visitor) override {
            visitor.visit_tensor_literal_expr(this);
        }
    };

    struct GPULaunchExpr : public Expr {
        std::string source;
        std::vector<std::unique_ptr<Expr>> values;

        explicit GPULaunchExpr(std::string source, std::vector<std::unique_ptr<Expr>> values)
            : source(std::move(source)), values(std::move(values)) {  }

        void accept(ExprVisitor &visitor) override {
            visitor.visit_gpu_launch_expr(this);
        }
    };
}
