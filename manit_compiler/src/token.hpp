#ifndef MANIT_TOKEN_HPP
#define MANIT_TOKEN_HPP

#include <string>
#include <vector>

enum class TokenType {
    FN, LET, VAR, IF, ELSE, WHILE, FOR, RETURN, TRUE, FALSE,
    IDENTIFIER, INTEGER_LITERAL,
    PLUS, MINUS, STAR, SLASH,
    EQUAL, EQUAL_EQUAL, BANG, BANG_EQUAL,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, SEMICOLON,
    END_OF_FILE, ILLEGAL
};

struct Token {
    TokenType type;
    std::string literal;
};

#endif //MANIT_TOKEN_HPP
