#include "tame/frontend/lexer.h"

using namespace tame::frontend;

Lexer::Lexer(diagnostics::DiagnosticEngine &diagnostic_engine)
    : diagnostic_engine(diagnostic_engine) {}

std::vector<Token> Lexer::tokenize(std::string_view source) {
    source_ = source;

    while (!is_reached_end()) {
        start_ptr = current_ptr;
        start_line = line_counter;
        start_column = column_counter;
        scan_next_token();
    }

    tokens.emplace_back(
        TokenType::EOF_TOKEN, "", NIL{},
        SourceLocation{
            .line = line_counter,
            .column = column_counter,
            .offset = current_ptr,
            .length = 0
        }
    );
    return tokens;
}

void Lexer::scan_next_token() {
    switch (const auto token = advance(); token) {
        case '(' : add_token(TokenType::LEFT_PARENTHESIS_TOKEN); break;
        case ')' : add_token(TokenType::RIGHT_PARENTHESIS_TOKEN); break;
        case '[' : add_token(TokenType::LEFT_BRACKET_TOKEN); break;
        case ']' : add_token(TokenType::RIGHT_BRACKET_TOKEN); break;
        case '<': add_token(TokenType::LEFT_ANGLE_TOKEN); break;
        case '>': add_token(TokenType::RIGHT_ANGLE_TOKEN); break;
        case ',': add_token(TokenType::COMMA_TOKEN); break;
        case '.': add_token(TokenType::DOT_TOKEN); break;
        case ';': add_token(TokenType::SEMICOLON_TOKEN); break;
        case ':': add_token(TokenType::COLON_TOKEN); break;
        case '+': add_token(TokenType::PLUS_TOKEN); break;
        case '-': add_token(TokenType::MINUS_TOKEN); break;
        case '*': add_token(TokenType::STAR_TOKEN); break;
        case '@': add_token(TokenType::TENSOR_MUL_TOKEN); break;
        case '/': add_token(TokenType::SLASH_TOKEN); break;
        case '=': add_token(TokenType::EQUALS_TOKEN); break;
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            break;
        case '#': {
            while (peek() != '\n' && !is_reached_end()) {
                advance();
            }

            break;
        }
        case '\'':
        case '\"':
            tokenize_string();
            break;
        default: {
            if (is_digit(token)) {
                tokenize_number();
            } else if (is_alpha(token)) {
                tokenize_identifier();
            } else [[unlikely]] {
                report_error("unexpected character");
                break;
            }

            break;
        }
    }
}

bool Lexer::is_reached_end() const {
    return current_ptr >= source_.length();
}

char Lexer::advance() {
    const auto current_character = source_.at(current_ptr++);

    if (current_character == '\n') [[unlikely]] {
        line_counter++;
        column_counter = 1;
    } else [[likely]] {
        column_counter++;
    }

    return current_character;
}

char Lexer::peek() const {
    if (is_reached_end()) [[unlikely]] return '\0';
    return source_.at(current_ptr);
}

char Lexer::peek_next() const {
    if (current_ptr + 1 >= source_.length()) [[unlikely]] return '\0';
    return source_.at(current_ptr + 1);
}

bool Lexer::match(const char &expected_symbol) {
    if (is_reached_end() || source_.at(current_ptr) != expected_symbol) return false;
    current_ptr++;
    column_counter++;
    return true;
}

TokenType Lexer::check(std::size_t starting, std::size_t ending, const std::string &rest, TokenType kind) const {
    if (current_ptr - start_ptr == starting + ending) {
        if (const std::string_view text(source_.data() + start_ptr + starting, ending); text == rest) {
            return kind;
        }
    }

    return TokenType::IDENTIFIER_TOKEN;
}

void Lexer::add_token(const TokenType &token_type, const Value &token_literal) {
    tokens.emplace_back(
        token_type,
        source_.substr(start_ptr, current_ptr - start_ptr),
        token_literal,
        SourceLocation{
            .line = start_line,
            .column = start_column,
            .offset = start_ptr,
            .length = current_ptr - start_ptr
        }
    );
}

void Lexer::tokenize_string() {
    while (peek() != '\"' && !is_reached_end()) {
        advance();
    }

    if (is_reached_end()) [[unlikely]] {
        report_error("unterminated string");
        return;
    }

    advance();
    add_token(
        TokenType::STRING_TOKEN,
        std::make_shared<std::string>(source_.substr(start_ptr + 1, current_ptr - start_ptr - 2))
    );
}

void Lexer::tokenize_number() {
    while (is_digit(peek())) {
        advance();
    }

    bool is_floating_point = false;
    if (peek() == '.' && is_digit(peek_next())) {
        is_floating_point = true;

        advance();

        while (is_digit(peek())) {
            advance();
        }
    }

    const std::string_view numeric_str{
        source_.data() + start_ptr,
        current_ptr - start_ptr
    };

    if (is_floating_point) {
        float value{0};
        std::from_chars(numeric_str.data(), numeric_str.data() + numeric_str.size(), value);
        add_token(TokenType::FLOAT_TOKEN, value);
    } else {
        int value{0};
        std::from_chars(numeric_str.data(), numeric_str.data() + numeric_str.size(), value);
        add_token(TokenType::INT_TOKEN, value);
    }
}

void Lexer::tokenize_identifier() {
    while (is_alpha_numeric(peek())) {
        advance();
    }

    add_token(check_identifier());
}

TokenType Lexer::check_identifier() const {
    switch (source_.at(start_ptr)) {
        case 'e': return check(1, 3, "lse", TokenType::ELSE_TOKEN);
        case 'f': {
            if (current_ptr - start_ptr > 1) {
                switch (source_.at(start_ptr + 1)) {
                    case 'o': return check(2, 1, "r", TokenType::FOR_TOKEN);
                    case '3': return check(2, 1, "2", TokenType::FLOAT_TYPE_TOKEN);
                    default: return TokenType::IDENTIFIER_TOKEN;
                }
            }
        }
        case 'i': {
            if (current_ptr - start_ptr > 1) {
                switch (source_.at(start_ptr + 1)) {
                    case 'f': return check(2, 0, "", TokenType::IF_TOKEN);
                    case '3': return check(2, 1, "2", TokenType::INT_TYPE_TOKEN);
                    default: return TokenType::IDENTIFIER_TOKEN;
                }
            }
        }
        case 'p': return check(1, 4, "rint", TokenType::PRINT_TOKEN);
        case 't': return check(1, 5, "ensor", TokenType::TENSOR_TYPE_TOKEN);
        case 'v': return check(1, 2, "ar", TokenType::VAR_TOKEN);
        case 'w': return check(1, 4, "hile", TokenType::WHILE_TOKEN);
        default: return TokenType::IDENTIFIER_TOKEN;
    }
}

void Lexer::report_error(const std::string &message) const {
    diagnostic_engine.report(
        diagnostics::DiagnosticReport::DiagnosticReportType::ERROR,
        message,
        SourceLocation{
            .line = start_line,
            .column = start_column,
            .offset = start_ptr,
            .length = current_ptr - start_ptr
        }
    );
}
