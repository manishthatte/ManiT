#ifndef MANIT_TOKEN_HPP
#define MANIT_TOKEN_HPP

#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    FN, LET, VAR, IF, ELSE, WHILE, FOR, RETURN, TRUE, FALSE, STRUCT,

    // Identifiers and Literals
    IDENTIFIER, INTEGER_LITERAL,

    // Operators
    PLUS, MINUS, STAR, SLASH,
    EQUAL, EQUAL_EQUAL, BANG, BANG_EQUAL,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,

    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, SEMICOLON, COLON,

    // Special
    END_OF_FILE, ILLEGAL
};

struct Token {
    TokenType type;
    std::string literal;
};

#endif //MANIT_TOKEN_HPP