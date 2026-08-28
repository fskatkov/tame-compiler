#pragma once

#include "tame/ast/expr.h"

namespace tame::backend {
    struct KernelResult {
        std::string source;
        std::vector<std::unique_ptr<ast::Expr>> values;
    };

    class MSLCodeGenerator {
    public:
        KernelResult generate(std::unique_ptr<ast::Expr> expr);
    private:
        std::vector<std::unique_ptr<ast::Expr>> values;
        std::unordered_map<std::string_view, std::size_t> variables;

        std::string process_node(std::unique_ptr<ast::Expr> &expr);
    };
}
