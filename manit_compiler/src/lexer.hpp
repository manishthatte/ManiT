#ifndef MANIT_LEXER_HPP
#define MANIT_LEXER_HPP

#include "token.hpp"
#include <string>

class Lexer {
public:
    // Constructor to initialize the lexer with source code
    Lexer(std::string input);

    // Main function to get the next token from the input
    Token next_token();

private:
    std::string input;
    size_t position;      // current position in input (points to current char)
    size_t read_position; // current reading position in input (after current char)
    char ch;              // current char under examination

    void read_char();
    void skip_whitespace();
    Token read_identifier();
    Token read_number();
    char peek_char();
};

#endif //MANIT_LEXER_HPP