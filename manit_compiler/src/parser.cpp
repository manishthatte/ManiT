#include "parser.hpp"
#include <iostream>

// Constructor
Parser::Parser(Lexer& l) : lexer(l) {
    // Initialize precedence map
    precedences = {
        {TokenType::EQUAL_EQUAL, Precedence::EQUALS},
        {TokenType::BANG_EQUAL, Precedence::EQUALS},
        {TokenType::LESS, Precedence::LESSGREATER},
        {TokenType::GREATER, Precedence::LESSGREATER},
        {TokenType::PLUS, Precedence::SUM},
        {TokenType::MINUS, Precedence::SUM},
        {TokenType::SLASH, Precedence::PRODUCT},
        {TokenType::STAR, Precedence::PRODUCT},
        // *** NEW PRECEDENCE FOR FUNCTION CALLS ***
        {TokenType::LPAREN, Precedence::CALL},
    };

    next_token();
    next_token();
}

// Helper methods for precedence
Precedence Parser::peek_precedence() {
    if (precedences.count(peek_token.type)) {
        return precedences[peek_token.type];
    }
    return Precedence::LOWEST;
}

Precedence Parser::current_precedence() {
    if (precedences.count(current_token.type)) {
        return precedences[current_token.type];
    }
    return Precedence::LOWEST;
}

void Parser::next_token() {
    current_token = peek_token;
    peek_token = lexer.next_token();
}

std::unique_ptr<Program> Parser::parse_program() {
    auto program = std::make_unique<Program>();
    while (current_token.type != TokenType::END_OF_FILE) {
        auto stmt = parse_statement();
        if (stmt) {
            program->statements.push_back(std::move(stmt));
        }
        next_token();
    }
    return program;
}

std::unique_ptr<Statement> Parser::parse_statement() {
    switch (current_token.type) {
        case TokenType::LET:
            return parse_let_statement();
        case TokenType::RETURN:
            return parse_return_statement();
        default:
            return parse_expression_statement();
    }
}

// Expression Parsing (Pratt Parser)
std::unique_ptr<Expression> Parser::parse_expression(Precedence precedence) {
    // Prefix parsing
    std::unique_ptr<Expression> left_exp;
    switch (current_token.type) {
        case TokenType::IDENTIFIER:
            left_exp = parse_identifier();
            break;
        case TokenType::INTEGER_LITERAL:
            left_exp = parse_integer_literal();
            break;
        case TokenType::BANG:
        case TokenType::MINUS:
            left_exp = parse_prefix_expression();
            break;
        case TokenType::IF:
            left_exp = parse_if_expression();
            break;
        // *** NEW PREFIX PARSER FOR FUNCTIONS ***
        case TokenType::FN:
            left_exp = parse_function_literal();
            break;
        default:
            return nullptr; // No prefix parse function found
    }

    // Infix parsing
    while (peek_token.type != TokenType::SEMICOLON && precedence < peek_precedence()) {
        // *** REFACTORED TO HANDLE DIFFERENT INFIX TYPES ***
        if (peek_token.type == TokenType::LPAREN) {
            next_token();
            left_exp = parse_call_expression(std::move(left_exp));
        } else {
            next_token();
            left_exp = parse_infix_expression(std::move(left_exp));
        }
    }

    return left_exp;
}

std::unique_ptr<Expression> Parser::parse_identifier() {
    auto ident = std::make_unique<Identifier>();
    ident->token = current_token;
    ident->value = current_token.literal;
    return ident;
}

std::unique_ptr<Expression> Parser::parse_integer_literal() {
    auto literal = std::make_unique<IntegerLiteral>();
    literal->token = current_token;
    try {
        literal->value = std::stoll(current_token.literal);
    } catch (const std::invalid_argument& ia) {
        return nullptr;
    }
    return literal;
}

std::unique_ptr<Expression> Parser::parse_prefix_expression() {
    auto expr = std::make_unique<PrefixExpression>();
    expr->token = current_token;
    expr->op = current_token.literal;
    next_token();
    expr->right = parse_expression(Precedence::PREFIX);
    return expr;
}

std::unique_ptr<Expression> Parser::parse_infix_expression(std::unique_ptr<Expression> left) {
    auto expr = std::make_unique<InfixExpression>();
    expr->token = current_token;
    expr->op = current_token.literal;
    expr->left = std::move(left);
    
    Precedence p = current_precedence();
    next_token();
    expr->right = parse_expression(p);
    
    return expr;
}

std::unique_ptr<ReturnStatement> Parser::parse_return_statement() {
    auto stmt = std::make_unique<ReturnStatement>();
    stmt->token = current_token;
    next_token();
    stmt->return_value = parse_expression(Precedence::LOWEST);
    if (peek_token.type == TokenType::SEMICOLON) {
        next_token();
    }
    return stmt;
}

std::unique_ptr<LetStatement> Parser::parse_let_statement() {
    auto stmt = std::make_unique<LetStatement>();
    stmt->token = current_token;

    next_token(); // Expect IDENTIFIER
    if (current_token.type != TokenType::IDENTIFIER) return nullptr;
    
    auto ident = std::make_unique<Identifier>();
    ident->token = current_token;
    ident->value = current_token.literal;
    stmt->name = std::move(ident);

    next_token(); // Expect EQUAL
    if (current_token.type != TokenType::EQUAL) return nullptr;
    
    next_token();
    stmt->value = parse_expression(Precedence::LOWEST);

    if (peek_token.type == TokenType::SEMICOLON) {
        next_token();
    }

    return stmt;
}

std::unique_ptr<ExpressionStatement> Parser::parse_expression_statement() {
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->token = current_token;
    stmt->expression = parse_expression(Precedence::LOWEST);

    if (peek_token.type == TokenType::SEMICOLON) {
        next_token();
    }
    return stmt;
}


std::unique_ptr<BlockStatement> Parser::parse_block_statement() {
    auto block = std::make_unique<BlockStatement>();
    block->token = current_token;

    next_token();

    while (current_token.type != TokenType::RBRACE && current_token.type != TokenType::END_OF_FILE) {
        auto stmt = parse_statement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
        next_token();
    }

    return block;
}

std::unique_ptr<Expression> Parser::parse_if_expression() {
    auto expr = std::make_unique<IfExpression>();
    expr->token = current_token;

    next_token(); 
    if (current_token.type != TokenType::LPAREN) return nullptr;
    
    next_token();
    expr->condition = parse_expression(Precedence::LOWEST);

    next_token(); 
    if (current_token.type != TokenType::RPAREN) return nullptr;

    next_token(); 
    if (current_token.type != TokenType::LBRACE) return nullptr;
    
    expr->consequence = parse_block_statement();

    if (peek_token.type == TokenType::ELSE) {
        next_token();
        
        next_token();
        if (current_token.type != TokenType::LBRACE) return nullptr;
        
        expr->alternative = parse_block_statement();
    }

    return expr;
}

// *** NEW IMPLEMENTATIONS FOR FUNCTIONS ***

std::vector<std::unique_ptr<Identifier>> Parser::parse_function_parameters() {
    std::vector<std::unique_ptr<Identifier>> params;

    if (peek_token.type == TokenType::RPAREN) {
        next_token(); // Consume ')'
        return params;
    }

    next_token(); // Consume '(' to get to first parameter

    auto ident = std::make_unique<Identifier>();
    ident->token = current_token;
    ident->value = current_token.literal;
    params.push_back(std::move(ident));

    while (peek_token.type == TokenType::COMMA) {
        next_token(); // consume ','
        next_token(); // move to next parameter
        auto next_ident = std::make_unique<Identifier>();
        next_ident->token = current_token;
        next_ident->value = current_token.literal;
        params.push_back(std::move(next_ident));
    }

    if (peek_token.type != TokenType::RPAREN) return {}; // Return empty on error
    next_token(); // Consume ')'
    
    return params;
}

std::unique_ptr<Expression> Parser::parse_function_literal() {
    auto func = std::make_unique<FunctionLiteral>();
    func->token = current_token; // 'fn' token

    if (peek_token.type != TokenType::LPAREN) return nullptr;
    next_token(); // consume '('

    func->parameters = parse_function_parameters();

    if (peek_token.type != TokenType::LBRACE) return nullptr;
    next_token(); // consume '{'

    func->body = parse_block_statement();

    return func;
}

std::vector<std::unique_ptr<Expression>> Parser::parse_call_arguments() {
    std::vector<std::unique_ptr<Expression>> args;

    if (peek_token.type == TokenType::RPAREN) {
        next_token(); // Consume ')'
        return args;
    }

    next_token(); // Consume '(' to get to first argument
    args.push_back(parse_expression(Precedence::LOWEST));

    while (peek_token.type == TokenType::COMMA) {
        next_token(); // Consume ','
        next_token(); // Move to next argument's first token
        args.push_back(parse_expression(Precedence::LOWEST));
    }

    if (peek_token.type != TokenType::RPAREN) return {}; // Return empty on error
    next_token(); // Consume ')'

    return args;
}

std::unique_ptr<Expression> Parser::parse_call_expression(std::unique_ptr<Expression> function) {
    auto expr = std::make_unique<CallExpression>();
    expr->token = current_token; // '(' token
    expr->function = std::move(function);
    expr->arguments = parse_call_arguments();
    return expr;
}