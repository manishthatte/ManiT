#include "lexer.hpp"
#include <map>

// Helper functions
bool is_letter(char ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

bool is_digit(char ch) {
    return '0' <= ch && ch <= '9';
}

// Map of keywords to their corresponding token types
std::map<std::string, TokenType> keywords = {
    {"fn", TokenType::FN},       {"let", TokenType::LET},   {"var", TokenType::VAR},
    {"if", TokenType::IF},       {"else", TokenType::ELSE}, {"while", TokenType::WHILE},
    {"for", TokenType::FOR},     {"return", TokenType::RETURN}, {"true", TokenType::TRUE},
    {"false", TokenType::FALSE}, {"struct", TokenType::STRUCT},
};

Lexer::Lexer(std::string input) : input(input), position(0), read_position(0), ch(0) {
    read_char();
}

void Lexer::read_char() {
    if (read_position >= input.length()) {
        ch = 0; // Null character for EOF
    } else {
        ch = input[read_position];
    }
    position = read_position;
    read_position += 1;
}

char Lexer::peek_char() {
    if (read_position >= input.length()) {
        return 0;
    }
    return input[read_position];
}

void Lexer::skip_whitespace() {
    while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
        read_char();
    }
}

Token Lexer::read_identifier() {
    size_t start_pos = position;
    while (is_letter(ch)) {
        read_char();
    }
    std::string literal = input.substr(start_pos, position - start_pos);

    if (keywords.count(literal)) {
        return {keywords[literal], literal};
    }
    return {TokenType::IDENTIFIER, literal};
}

Token Lexer::read_number() {
    size_t start_pos = position;
    while (is_digit(ch)) {
        read_char();
    }
    return {TokenType::INTEGER_LITERAL, input.substr(start_pos, position - start_pos)};
}

Token Lexer::next_token() {
    Token tok;

    skip_whitespace();

    switch (ch) {
        case '=':
            if (peek_char() == '=') {
                read_char();
                tok = {TokenType::EQUAL_EQUAL, "=="};
            } else {
                tok = {TokenType::EQUAL, "="};
            }
            break;
        case '+':
            tok = {TokenType::PLUS, "+"};
            break;
        case '-':
            tok = {TokenType::MINUS, "-"};
            break;
        case '!':
            if (peek_char() == '=') {
                read_char();
                tok = {TokenType::BANG_EQUAL, "!="};
            } else {
                tok = {TokenType::BANG, "!"};
            }
            break;
        case '*':
            tok = {TokenType::STAR, "*"};
            break;
        case '/':
            if (peek_char() == '/') { // Handle comments
                while (ch != '\n' && ch != 0) {
                    read_char();
                }
                return next_token(); // Recursively get the next token after the comment
            } else {
                tok = {TokenType::SLASH, "/"};
            }
            break;
        case '<':
            if (peek_char() == '=') {
                read_char();
                tok = {TokenType::LESS_EQUAL, "<="};
            } else {
                tok = {TokenType::LESS, "<"};
            }
            break;
        case '>':
            if (peek_char() == '=') {
                read_char();
                tok = {TokenType::GREATER_EQUAL, ">="};
            } else {
                tok = {TokenType::GREATER, ">"};
            }
            break;
        case ';':
            tok = {TokenType::SEMICOLON, ";"};
            break;
        case ':': // New case for colon
            tok = {TokenType::COLON, ":"};
            break;
        case '(':
            tok = {TokenType::LPAREN, "("};
            break;
        case ')':
            tok = {TokenType::RPAREN, ")"};
            break;
        case '{':
            tok = {TokenType::LBRACE, "{"};
            break;
        case '}':
            tok = {TokenType::RBRACE, "}"};
            break;
        case '[':
            tok = {TokenType::LBRACKET, "["};
            break;
        case ']':
            tok = {TokenType::RBRACKET, "]"};
            break;
        case ',':
            tok = {TokenType::COMMA, ","};
            break;
        case 0:
            tok = {TokenType::END_OF_FILE, ""};
            break;
        default:
            if (is_letter(ch)) {
                return read_identifier();
            } else if (is_digit(ch)) {
                return read_number();
            } else {
                tok = {TokenType::ILLEGAL, std::string(1, ch)};
            }
    }

    read_char();
    return tok;
}