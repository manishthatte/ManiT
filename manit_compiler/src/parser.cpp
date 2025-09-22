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
            // *** THIS IS THE KEY CHANGE ***
            // If it's not a known statement, parse it as an expression statement.
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
        default:
            return nullptr; // No prefix parse function found
    }

    // Infix parsing
    while (peek_token.type != TokenType::SEMICOLON && precedence < peek_precedence()) {
        next_token(); // Move to the infix operator
        left_exp = parse_infix_expression(std::move(left_exp));
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

// *** NEW IMPLEMENTATION ***
std::unique_ptr<ExpressionStatement> Parser::parse_expression_statement() {
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->token = current_token;
    stmt->expression = parse_expression(Precedence::LOWEST);

    // If a semicolon is next, consume it. This makes them optional.
    if (peek_token.type == TokenType::SEMICOLON) {
        next_token();
    }
    return stmt;
}


std::unique_ptr<BlockStatement> Parser::parse_block_statement() {
    auto block = std::make_unique<BlockStatement>();
    block->token = current_token; // The '{' token

    next_token(); // Move past '{'

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
    expr->token = current_token; // The 'if' token

    next_token(); // Expect '('
    if (current_token.type != TokenType::LPAREN) {
        return nullptr;
    }
    
    next_token(); // Move to the start of the condition expression
    expr->condition = parse_expression(Precedence::LOWEST);

    next_token(); // Expect ')'
    if (current_token.type != TokenType::RPAREN) {
        return nullptr;
    }

    next_token(); // Expect '{'
    if (current_token.type != TokenType::LBRACE) {
        return nullptr;
    }
    
    expr->consequence = parse_block_statement();

    // Check for optional 'else' block
    if (peek_token.type == TokenType::ELSE) {
        next_token(); // Consume 'else'
        
        next_token(); // Expect '{'
        if (current_token.type != TokenType::LBRACE) {
            return nullptr;
        }
        
        expr->alternative = parse_block_statement();
    }

    return expr;
}