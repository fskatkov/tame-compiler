#pragma once

#include "lexer.h"
#include "tame/ast/stmt.h"
#include "tame/support/diagnostic_engine.h"

namespace tame::frontend {
    class Parser {
    public:
        explicit Parser(std::string source, diagnostics::DiagnosticEngine &diagnostic_engine);
        std::vector<std::unique_ptr<ast::Stmt>> run();
    private:
        diagnostics::DiagnosticEngine &diagnostic_engine;

        std::vector<Token> tokens;
        std::size_t current_ptr{0};

        std::unique_ptr<ast::Stmt> parse_declaration();
        std::unique_ptr<ast::Stmt> parse_var_declaration();
        std::unique_ptr<ast::Stmt> parse_statement();
        std::unique_ptr<ast::Stmt> parse_print_statement();

        std::unique_ptr<ast::Expr> parse_expression();
        std::unique_ptr<ast::Expr> parse_assignment_expression();
        std::unique_ptr<ast::Expr> parse_term_expression();
        std::unique_ptr<ast::Expr> parse_factor_expression();
        std::unique_ptr<ast::Expr> parse_primary_expression();

        bool match(std::initializer_list<TokenType> types);
        bool check(const TokenType &type);
        Token advance();
        bool is_reached_end();
        Token peek();
        Token previous();
        Token consume(const TokenType &type, const std::string &message);
    };
}
