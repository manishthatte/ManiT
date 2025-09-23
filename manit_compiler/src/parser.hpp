#ifndef MANIT_PARSER_HPP
#define MANIT_PARSER_HPP

#include "lexer.hpp"
#include "ast.hpp"
#include <memory>
#include <map>
#include <vector>

enum Precedence {
    LOWEST,
    ASSIGN,      // =
    EQUALS,      // ==
    LESSGREATER, // > or <
    SUM,         // +
    PRODUCT,     // *
    PREFIX,      // -X or !X
    CALL,        // myFunction(X)
    INDEX        // array[index]
};

class Parser {
public:
    Parser(Lexer& l);
    std::unique_ptr<Program> parse_program();

private:
    Lexer& lexer;
    Token current_token, peek_token;
    std::map<TokenType, Precedence> precedences;

    void next_token();

    // Statement Parsers
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<LetStatement> parse_let_statement();
    std::unique_ptr<VarStatement> parse_var_statement();
    std::unique_ptr<StructDefinitionStatement> parse_struct_definition_statement();
    std::unique_ptr<ReturnStatement> parse_return_statement();
    std::unique_ptr<ExpressionStatement> parse_expression_statement();
    std::unique_ptr<BlockStatement> parse_block_statement();
    
    // Expression Parsers
    std::unique_ptr<Expression> parse_expression(Precedence precedence);
    std::unique_ptr<Expression> parse_identifier();
    std::unique_ptr<Expression> parse_integer_literal();
    std::unique_ptr<Expression> parse_boolean_literal();
    std::unique_ptr<Expression> parse_array_literal();
    std::unique_ptr<Expression> parse_prefix_expression();
    std::unique_ptr<Expression> parse_infix_expression(std::unique_ptr<Expression> left);
    std::unique_ptr<Expression> parse_assignment_expression(std::unique_ptr<Expression> left);
    std::unique_ptr<Expression> parse_index_expression(std::unique_ptr<Expression> left);
    std::unique_ptr<Expression> parse_if_expression();
    std::unique_ptr<Expression> parse_function_literal();
    std::unique_ptr<Expression> parse_call_expression(std::unique_ptr<Expression> function);
    std::unique_ptr<Expression> parse_while_expression();
    std::unique_ptr<Expression> parse_for_loop_expression();

    // Parser Helpers
    std::vector<std::unique_ptr<Identifier>> parse_function_parameters();
    std::vector<std::unique_ptr<Expression>> parse_call_arguments();
    std::vector<std::unique_ptr<Expression>> parse_expression_list(TokenType end_token);
    Precedence peek_precedence();
    Precedence current_precedence();
};

#endif // MANIT_PARSER_HPP