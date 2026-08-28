#pragma once

#include "tame/ast/stmt.h"
#include "tame/backend/metal/msl_codegen.h"

namespace tame::backend {
    class LoweringEngine {
    public:
        void process(std::vector<std::unique_ptr<ast::Stmt>> &statements);
    private:
        void lower_statement(std::unique_ptr<ast::Stmt> &statement);
        std::unique_ptr<ast::Expr> lower_expression(std::unique_ptr<ast::Expr> &expression);
    };
}
