#pragma once

#include "tame/structures/token.h"
#include "tame/support/diagnostic_engine.h"

namespace tame::frontend {
    class Lexer {
    public:
        explicit Lexer(diagnostics::DiagnosticEngine &diagnostic_engine);
        std::vector<Token> tokenize(std::string_view source);
    private:
        diagnostics::DiagnosticEngine &diagnostic_engine;

        std::string_view source_;

        std::size_t start_ptr{0};
        std::size_t current_ptr{0};
        std::size_t line_counter{1};
        std::size_t start_line{1};
        std::size_t column_counter{1};
        std::size_t start_column{1};

        std::vector<Token> tokens;

        void scan_next_token();
        [[nodiscard]] bool is_reached_end() const;
        char advance();
        [[nodiscard]] char peek() const;
        [[nodiscard]] char peek_next() const;
        [[nodiscard]] bool match(const char &expected_symbol);
        [[nodiscard]] TokenType check(std::size_t starting, std::size_t ending, const std::string &rest, TokenType kind) const;
        void add_token(const TokenType &token_type, const Value &token_literal = NIL{});
        void tokenize_string();
        void tokenize_number();
        void tokenize_identifier();
        [[nodiscard]] TokenType check_identifier() const;
        void report_error(const std::string &message) const;
    };
}
