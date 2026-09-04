#pragma once

#include "value.h"

namespace tame::frontend {
    enum class TokenType {
        LEFT_PARENTHESIS_TOKEN, RIGHT_PARENTHESIS_TOKEN, LEFT_BRACKET_TOKEN, RIGHT_BRACKET_TOKEN,
        LEFT_ANGLE_TOKEN, RIGHT_ANGLE_TOKEN, COMMA_TOKEN, DOT_TOKEN,
        SEMICOLON_TOKEN, COLON_TOKEN, TENSOR_MUL_TOKEN,

        PLUS_TOKEN, MINUS_TOKEN, STAR_TOKEN, SLASH_TOKEN,
        INT_TOKEN, FLOAT_TOKEN, NIL_TOKEN, STRING_TOKEN,
        INT_TYPE_TOKEN, FLOAT_TYPE_TOKEN, TENSOR_TYPE_TOKEN,

        EQUALS_TOKEN,

        VAR_TOKEN, IF_TOKEN, ELSE_TOKEN, WHILE_TOKEN,
        FOR_TOKEN, IDENTIFIER_TOKEN,

        EOF_TOKEN, ERROR_TOKEN,

        PRINT_TOKEN,
    };

    struct Token {
        TokenType type;
        std::string_view lexeme;
        Value literal;
        SourceLocation source_location;
    };
}
