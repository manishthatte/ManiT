#ifndef MANIT_TOKEN_HPP
#define MANIT_TOKEN_HPP

#include <string>
#include <vector>

// Enum defines every possible type of token in ManiT.
enum class TokenType {
    // Keywords
    FN, LET, VAR, STRUCT, ENUM,
    IF, ELSE, WHILE, FOR, RETURN,
    TRY, CATCH, COMPTIME,
    TRUE, FALSE,

    // Identifiers & Literals
    IDENTIFIER,
    INTEGER_LITERAL,
    STRING_LITERAL,

    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQUAL, EQUAL_EQUAL, BANG, BANG_EQUAL,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    AMPERSAND_AMPERSAND, PIPE_PIPE,

    // Punctuation
    LPAREN, RPAREN, // ( )
    LBRACE, RBRACE, // { }
    LBRACKET, RBRACKET, // [ ]
    COMMA, COLON, SEMICOLON, PERIOD,

    // Miscellaneous
    END_OF_FILE,
    ILLEGAL
};

// Represents a single token: its type and its literal value (e.g., the variable name).
struct Token {
    TokenType type;
    std::string literal;
};

#endif //MANIT_TOKEN_HPP