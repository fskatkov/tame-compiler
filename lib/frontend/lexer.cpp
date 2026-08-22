#include "tame/frontend/lexer.h"

using namespace tame::frontend;

Lexer::Lexer(std::string source) : source_str(std::move(source)) {
}

std::vector<Token> Lexer::run() {

}

void Lexer::scan_next_token() {

}

bool Lexer::is_reached_end() const {

}

char Lexer::advance() {

}

char Lexer::peek() const {

}

char Lexer::peek_next() const {

}

bool Lexer::match(const char &expected_symbol) {

}

TokenType Lexer::check(std::size_t starting, std::size_t ending, const std::string &rest, TokenType kind) const {

}

void Lexer::add_token(const TokenType &token_type, const Value &token_literal) {

}

void Lexer::add_string_token() {

}

void Lexer::add_numeric_token() {

}

void Lexer::add_identifier_token() {

}

TokenType Lexer::check_identifier() const {

}