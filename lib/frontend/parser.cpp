#include "tame/frontend/parser.h"

using namespace tame::frontend;
using namespace tame::diagnostics;
using namespace tame::ast;

Parser::Parser(DiagnosticEngine &diagnostic_engine)
    : diagnostic_engine(diagnostic_engine) {}

std::vector<std::unique_ptr<Stmt>> Parser::run(std::span<const Token> tokens) {
    tokens_ = tokens;

    std::vector<std::unique_ptr<Stmt>> statements;
    while (!is_reached_end()) {
        statements.push_back(parse_declaration());
    }
    return statements;
}

std::unique_ptr<Stmt> Parser::parse_declaration() {
    if (match({ TokenType::VAR_TOKEN })) {
        return parse_var_declaration();
    }

    return parse_statement();
}

std::unique_ptr<Stmt> Parser::parse_var_declaration() {
    auto variable_name = consume(TokenType::IDENTIFIER_TOKEN, "expected variable name");

    consume(TokenType::COLON_TOKEN, "expected variable type");

    std::unique_ptr<TypeAnnotation> type;
    if (match({ TokenType::TENSOR_TYPE_TOKEN })) {
        consume(TokenType::LEFT_ANGLE_TOKEN, "expected `<` after `tensor` type annotation");

        std::vector<int> shape;
        if (!check(TokenType::RIGHT_ANGLE_TOKEN)) {
            do {
                auto dimension = consume(TokenType::INT_TOKEN, "expected `tensor` dimension value");
                shape.push_back(dimension.literal.get<int>());
            } while (match({ TokenType::COMMA_TOKEN }));
        }

        consume(TokenType::RIGHT_ANGLE_TOKEN, "expected `>` at end of `tensor` type annotation");
        type = std::make_unique<TensorTypeAnnotation>(std::move(shape));
    } else if (match({ TokenType::FLOAT_TYPE_TOKEN })) {
        type = std::make_unique<FloatTypeAnnotation>();
    } else if (match({ TokenType::INT_TYPE_TOKEN })) {
        type = std::make_unique<IntTypeAnnotation>();
    }

    consume(TokenType::EQUALS_TOKEN, "expected `=` to bind variable with value");
    return std::make_unique<VarStmt>(std::move(variable_name), std::move(type), std::move(parse_expression()));
}

std::unique_ptr<Stmt> Parser::parse_statement() {
    if (match({ TokenType::PRINT_TOKEN })) {
        return parse_print_statement();
    }

    return std::make_unique<ExprStmt>(std::move(parse_expression()));
}

std::unique_ptr<Stmt> Parser::parse_print_statement() {
    consume(TokenType::LEFT_PARENTHESIS_TOKEN, "expected `(` before `print`");
    auto inner_expression = parse_expression();
    consume(TokenType::RIGHT_PARENTHESIS_TOKEN, "expected `)` at end of `print`");
    return std::make_unique<PrintStmt>(std::move(inner_expression));
}

std::unique_ptr<Expr> Parser::parse_expression() {
    return parse_assignment_expression();
}

std::unique_ptr<Expr> Parser::parse_assignment_expression() {
    auto left_expression = parse_term_expression();

    if (match({ TokenType::EQUALS_TOKEN })) {
        const auto operator_token = previous();
        auto right_expression = parse_term_expression();
        return std::make_unique<AssignExpr>(std::move(left_expression), operator_token, std::move(right_expression));
    }

    return left_expression;
}

std::unique_ptr<Expr> Parser::parse_term_expression() {
    auto left_expression = parse_factor_expression();

    while (match({ TokenType::PLUS_TOKEN, TokenType::MINUS_TOKEN })) {
        const auto operator_token = previous();
        auto right_expression = parse_factor_expression();
        left_expression = std::make_unique<BinaryExpr>(
            std::move(left_expression),
            operator_token,
            std::move(right_expression)
        );
    }

    return left_expression;
}

std::unique_ptr<Expr> Parser::parse_factor_expression() {
    auto left_expression = parse_primary_expression();

    while (match({ TokenType::STAR_TOKEN, TokenType::TENSOR_MUL_TOKEN, TokenType::SLASH_TOKEN })) {
        const auto operator_token = previous();
        auto right_expression = parse_primary_expression();
        left_expression = std::make_unique<BinaryExpr>(
            std::move(left_expression),
            operator_token,
            std::move(right_expression)
        );
    }

    return left_expression;
}

std::unique_ptr<Expr> Parser::parse_primary_expression() {
    if (match({ TokenType::NIL_TOKEN })) {
        return std::make_unique<LiteralExpr>(NIL{}, previous());
    }

    if (match({ TokenType::IDENTIFIER_TOKEN })) {
        return std::make_unique<VarExpr>(previous());
    }

    if (match({ TokenType::INT_TOKEN, TokenType::FLOAT_TOKEN, TokenType::STRING_TOKEN })) {
        return std::make_unique<LiteralExpr>(previous().literal, previous());
    }

    if (match({ TokenType::LEFT_BRACKET_TOKEN })) {
        std::vector<int> tensor_shape;
        std::vector<std::unique_ptr<Expr>> tensor_data;

        auto parse_flatten_level = [&](this auto &self, std::size_t depth) -> void {
            int counter{0};

            if (depth >= tensor_shape.size()) {
                tensor_shape.push_back(0);
            }

            do {
                if (check(TokenType::LEFT_BRACKET_TOKEN)) {
                    advance();
                    self(depth + 1);
                    consume(TokenType::RIGHT_BRACKET_TOKEN, "expected `]` at end of flatten tensor");
                } else {
                    tensor_data.push_back(parse_expression());
                }

                counter++;
            } while (match({ TokenType::COMMA_TOKEN }));

            if (tensor_shape[depth] == 0) {
                tensor_shape[depth] = counter;
            }
        };

        parse_flatten_level(0);
        consume(TokenType::RIGHT_BRACKET_TOKEN, "expected `]` at end of `tensor` declaration");
        return std::make_unique<TensorLiteralExpr>(std::move(tensor_shape), std::move(tensor_data));
    }

    report_error("expected expression");
    advance();
    return nullptr;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    if (std::ranges::any_of(types, [this](const auto &type) { return check(type); })) {
        advance();
        return true;
    }

    return false;
}

bool Parser::check(const TokenType &type) {
    if (is_reached_end()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!is_reached_end()) current_ptr++;
    return previous();
}

bool Parser::is_reached_end() {
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::peek() {
    return tokens_[current_ptr];
}

Token Parser::previous() {
    return tokens_[current_ptr - 1];
}

void Parser::report_error(const std::string &message) {
    diagnostic_engine.report(
        DiagnosticReport::DiagnosticReportType::ERROR,
        message,
        peek().source_location
    );
}

Token Parser::consume(const TokenType &type, const std::string &message) {
    if (check(type)) [[likely]] {
        return advance();
    }

    report_error(message);

    return Token{
        .type = TokenType::ERROR_TOKEN,
        .lexeme = "error",
        .literal = NIL{},
        .source_location = SourceLocation{}
    };
}
