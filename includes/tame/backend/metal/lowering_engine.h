#pragma once

#include "tame/ast/stmt.h"
#include "tame/backend/metal/msl_codegen.h"

namespace tame::backend {
    class LoweringEngine {
    public:
        void process(std::vector<std::unique_ptr<ast::Stmt>> &statements);
    private:
        static void lower_statement(const std::unique_ptr<ast::Stmt> &statement);
        static std::unique_ptr<ast::Expr> lower_expression(std::unique_ptr<ast::Expr> expression);
    };
}
