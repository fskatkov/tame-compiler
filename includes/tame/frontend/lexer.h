#pragma once

#include "tame/structures/token.h"

namespace tame::frontend {
    class Lexer {
    public:
        explicit Lexer(std::string source);
        std::vector<Token> run();
    private:
        std::string source_str;

        std::size_t start_ptr{0};
        std::size_t current_ptr{0};

        std::vector<Token> tokens;

        void scan_next_token();
        [[nodiscard]] bool is_reached_end() const;
        char advance();
        [[nodiscard]] char peek() const;
        [[nodiscard]] char peek_next() const;
        bool match(const char &expected_symbol) const;
        [[nodiscard]] TokenType check(std::size_t starting, std::size_t ending, const std::string &rest, TokenType kind) const;
        void add_token(const TokenType &token_type, const Value &token_literal = NIL{});
        void tokenize_string();
        void tokenize_number();
        void tokenize_identifier();
        [[nodiscard]] TokenType check_identifier() const;
    };
}
