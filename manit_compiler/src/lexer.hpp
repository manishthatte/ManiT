#ifndef MANIT_LEXER_HPP
#define MANIT_LEXER_HPP

#include "token.hpp"
#include <string>

class Lexer {
public:
    Lexer(std::string input);
    Token next_token();
private:
    std::string input;
    size_t position;
    size_t read_position;
    char ch;
    void read_char();
    void skip_whitespace();
    Token read_identifier();
    Token read_number();
    char peek_char();
};

#endif //MANIT_LEXER_HPP
