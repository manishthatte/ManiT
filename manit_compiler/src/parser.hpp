#ifndef MANIT_PARSER_HPP
#define MANIT_PARSER_HPP

#include "lexer.hpp"
#include "ast.hpp"
#include <memory>
#include <map>
#include <vector>

// Precedence levels for operators
enum Precedence {
    LOWEST,
    EQUALS,      // ==
    LESSGREATER, // > or <
    SUM,         // +
    PRODUCT,     // *
    PREFIX,      // -X or !X
    CALL         // myFunction(X)
};

class Parser {
public:
    Parser(Lexer& l);

    std::unique_ptr<Program> parse_program();

private:
    Lexer& lexer;
    Token current_token;
    Token peek_token;
    
    // Maps token types to their precedence
    std::map<TokenType, Precedence> precedences;

    void next_token();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<LetStatement> parse_let_statement();
    std::unique_ptr<ReturnStatement> parse_return_statement();
    std::unique_ptr<ExpressionStatement> parse_expression_statement();
    
    std::unique_ptr<Expression> parse_expression(Precedence precedence);
    
    // Parsing functions for different expression types
    std::unique_ptr<Expression> parse_identifier();
    std::unique_ptr<Expression> parse_integer_literal();
    std::unique_ptr<Expression> parse_prefix_expression();
    std::unique_ptr<Expression> parse_infix_expression(std::unique_ptr<Expression> left);
    std::unique_ptr<Expression> parse_if_expression();
    std::unique_ptr<BlockStatement> parse_block_statement();

    // *** NEW DECLARATIONS ***
    std::unique_ptr<Expression> parse_function_literal();
    std::unique_ptr<Expression> parse_call_expression(std::unique_ptr<Expression> function);
    std::vector<std::unique_ptr<Identifier>> parse_function_parameters();
    std::vector<std::unique_ptr<Expression>> parse_call_arguments();


    Precedence peek_precedence();
    Precedence current_precedence();
};

#endif // MANIT_PARSER_HPP