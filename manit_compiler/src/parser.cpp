#include "parser.hpp"
#include <charconv>
#include <system_error>

Parser::Parser(Lexer& l) : lexer(l) {
    precedences = {
        {TokenType::EQUAL, Precedence::ASSIGN},
        {TokenType::EQUAL_EQUAL, Precedence::EQUALS},
        {TokenType::BANG_EQUAL, Precedence::EQUALS},
        {TokenType::LESS, Precedence::LESSGREATER},
        {TokenType::GREATER, Precedence::LESSGREATER},
        {TokenType::LESS_EQUAL, Precedence::LESSGREATER},
        {TokenType::GREATER_EQUAL, Precedence::LESSGREATER},
        {TokenType::PLUS, Precedence::SUM},
        {TokenType::MINUS, Precedence::SUM},
        {TokenType::SLASH, Precedence::PRODUCT},
        {TokenType::STAR, Precedence::PRODUCT},
        {TokenType::LPAREN, Precedence::CALL},
        {TokenType::LBRACKET, Precedence::INDEX},
    };
    next_token();
    next_token();
}

void Parser::next_token() {
    current_token = peek_token;
    peek_token = lexer.next_token();
}

Precedence Parser::peek_precedence() {
    if (precedences.count(peek_token.type)) return precedences[peek_token.type];
    return Precedence::LOWEST;
}

Precedence Parser::current_precedence() {
    if (precedences.count(current_token.type)) return precedences[current_token.type];
    return Precedence::LOWEST;
}

std::unique_ptr<Program> Parser::parse_program() {
    auto program = std::make_unique<Program>();
    while (current_token.type != TokenType::END_OF_FILE) {
        auto stmt = parse_statement();
        if (stmt) program->statements.push_back(std::move(stmt));
        next_token();
    }
    return program;
}

std::unique_ptr<Statement> Parser::parse_statement() {
    switch (current_token.type) {
        case TokenType::LET:
            return parse_let_statement();
        case TokenType::VAR:
            return parse_var_statement();
        case TokenType::STRUCT:
            return parse_struct_definition_statement();
        case TokenType::RETURN:
            return parse_return_statement();
        default:
            return parse_expression_statement();
    }
}

std::unique_ptr<LetStatement> Parser::parse_let_statement() {
    auto stmt = std::make_unique<LetStatement>();
    stmt->token = current_token;

    if (peek_token.type != TokenType::IDENTIFIER) return nullptr;
    next_token();

    auto ident = std::make_unique<Identifier>();
    ident->token = current_token;
    ident->value = current_token.literal;
    stmt->name = std::move(ident);

    // Check for optional type annotation
    if (peek_token.type == TokenType::COLON) {
        next_token(); // Consume ':'
        next_token(); // Consume type identifier
        if (current_token.type != TokenType::IDENTIFIER) return nullptr;
        
        auto type_ident = std::make_unique<Identifier>();
        type_ident->token = current_token;
        type_ident->value = current_token.literal;
        stmt->type = std::move(type_ident);
    }

    if (peek_token.type != TokenType::EQUAL) return nullptr;
    next_token(); // Consume '='
    next_token(); // Move to the start of the expression

    stmt->value = parse_expression(Precedence::LOWEST);
    if (peek_token.type == TokenType::SEMICOLON) next_token();
    return stmt;
}

std::unique_ptr<VarStatement> Parser::parse_var_statement() {
    auto stmt = std::make_unique<VarStatement>();
    stmt->token = current_token;

    if (peek_token.type != TokenType::IDENTIFIER) return nullptr;
    next_token();

    auto ident = std::make_unique<Identifier>();
    ident->token = current_token;
    ident->value = current_token.literal;
    stmt->name = std::move(ident);

    // Check for optional type annotation
    if (peek_token.type == TokenType::COLON) {
        next_token(); // Consume ':'
        next_token(); // Consume type identifier
        if (current_token.type != TokenType::IDENTIFIER) return nullptr;

        auto type_ident = std::make_unique<Identifier>();
        type_ident->token = current_token;
        type_ident->value = current_token.literal;
        stmt->type = std::move(type_ident);
    }

    if (peek_token.type != TokenType::EQUAL) return nullptr;
    next_token(); // Consume '='
    next_token(); // Move to the start of the expression

    stmt->value = parse_expression(Precedence::LOWEST);
    if (peek_token.type == TokenType::SEMICOLON) next_token();
    return stmt;
}

std::unique_ptr<StructDefinitionStatement> Parser::parse_struct_definition_statement() {
    auto stmt = std::make_unique<StructDefinitionStatement>();
    stmt->token = current_token;

    if (peek_token.type != TokenType::IDENTIFIER) return nullptr;
    next_token();
    
    auto name = std::make_unique<Identifier>();
    name->token = current_token;
    name->value = current_token.literal;
    stmt->name = std::move(name);

    if (peek_token.type != TokenType::LBRACE) return nullptr;
    next_token();

    if (peek_token.type != TokenType::RBRACE) {
        next_token(); 

        if (current_token.type != TokenType::IDENTIFIER) return nullptr;
        auto first_field_name = std::make_unique<Identifier>();
        first_field_name->token = current_token;
        first_field_name->value = current_token.literal;

        if (peek_token.type != TokenType::COLON) return nullptr;
        next_token();

        if (peek_token.type != TokenType::IDENTIFIER) return nullptr;
        next_token();
        auto first_field_type = std::make_unique<Identifier>();
        first_field_type->token = current_token;
        first_field_type->value = current_token.literal;
        stmt->fields.push_back({std::move(first_field_name), std::move(first_field_type)});

        while (peek_token.type == TokenType::COMMA) {
            next_token();
            next_token();

            if (current_token.type != TokenType::IDENTIFIER) return nullptr;
            auto next_field_name = std::make_unique<Identifier>();
            next_field_name->token = current_token;
            next_field_name->value = current_token.literal;

            if (peek_token.type != TokenType::COLON) return nullptr;
            next_token();

            if (peek_token.type != TokenType::IDENTIFIER) return nullptr;
            next_token();
            auto next_field_type = std::make_unique<Identifier>();
            next_field_type->token = current_token;
            next_field_type->value = current_token.literal;
            stmt->fields.push_back({std::move(next_field_name), std::move(next_field_type)});
        }
    }

    if (peek_token.type != TokenType::RBRACE) return nullptr;
    next_token();

    if (peek_token.type == TokenType::SEMICOLON) next_token();

    return stmt;
}

std::unique_ptr<ReturnStatement> Parser::parse_return_statement() {
    auto stmt = std::make_unique<ReturnStatement>();
    stmt->token = current_token;
    next_token();
    stmt->return_value = parse_expression(Precedence::LOWEST);
    if (peek_token.type == TokenType::SEMICOLON) next_token();
    return stmt;
}

std::unique_ptr<ExpressionStatement> Parser::parse_expression_statement() {
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->token = current_token;
    stmt->expression = parse_expression(Precedence::LOWEST);
    if (peek_token.type == TokenType::SEMICOLON) next_token();
    return stmt;
}

std::unique_ptr<Expression> Parser::parse_expression(Precedence precedence) {
    std::unique_ptr<Expression> left_exp;
    switch (current_token.type) {
        case TokenType::IDENTIFIER: left_exp = parse_identifier(); break;
        case TokenType::INTEGER_LITERAL: left_exp = parse_integer_literal(); break;
        case TokenType::TRUE: case TokenType::FALSE: left_exp = parse_boolean_literal(); break;
        case TokenType::LBRACKET: left_exp = parse_array_literal(); break;
        case TokenType::BANG: case TokenType::MINUS: left_exp = parse_prefix_expression(); break;
        case TokenType::IF: left_exp = parse_if_expression(); break;
        case TokenType::FN: left_exp = parse_function_literal(); break;
        case TokenType::WHILE: left_exp = parse_while_expression(); break;
        case TokenType::FOR: left_exp = parse_for_loop_expression(); break;
        default: return nullptr;
    }

    while (peek_token.type != TokenType::SEMICOLON && precedence < peek_precedence()) {
        TokenType peek_type = peek_token.type;
        if (peek_type == TokenType::LPAREN) { next_token(); left_exp = parse_call_expression(std::move(left_exp)); }
        else if (peek_type == TokenType::LBRACKET) { next_token(); left_exp = parse_index_expression(std::move(left_exp)); }
        else if (peek_type == TokenType::EQUAL) { next_token(); left_exp = parse_assignment_expression(std::move(left_exp)); }
        else if (peek_type == TokenType::PLUS || peek_type == TokenType::MINUS || peek_type == TokenType::SLASH || peek_type == TokenType::STAR || peek_type == TokenType::EQUAL_EQUAL || peek_type == TokenType::BANG_EQUAL || peek_type == TokenType::LESS || peek_type == TokenType::GREATER || peek_type == TokenType::LESS_EQUAL || peek_type == TokenType::GREATER_EQUAL) { next_token(); left_exp = parse_infix_expression(std::move(left_exp)); }
        else { return left_exp; }
    }
    return left_exp;
}

std::unique_ptr<Expression> Parser::parse_identifier() { auto ident = std::make_unique<Identifier>(); ident->token = current_token; ident->value = current_token.literal; return ident; }
std::unique_ptr<Expression> Parser::parse_integer_literal() { auto literal = std::make_unique<IntegerLiteral>(); literal->token = current_token; const std::string& s = current_token.literal; auto result = std::from_chars(s.data(), s.data() + s.size(), literal->value); if (result.ec != std::errc() || result.ptr != s.data() + s.size()) return nullptr; return literal; }
std::unique_ptr<Expression> Parser::parse_boolean_literal() { auto literal = std::make_unique<BooleanLiteral>(); literal->token = current_token; literal->value = (current_token.type == TokenType::TRUE); return literal; }
std::unique_ptr<Expression> Parser::parse_array_literal() { auto array_lit = std::make_unique<ArrayLiteral>(); array_lit->token = current_token; array_lit->elements = parse_expression_list(TokenType::RBRACKET); return array_lit; }
std::unique_ptr<Expression> Parser::parse_index_expression(std::unique_ptr<Expression> left) { auto expr = std::make_unique<IndexExpression>(); expr->token = current_token; expr->left = std::move(left); next_token(); expr->index = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::RBRACKET) return nullptr; next_token(); return expr; }
std::unique_ptr<Expression> Parser::parse_prefix_expression() { auto expr = std::make_unique<PrefixExpression>(); expr->token = current_token; expr->op = current_token.literal; next_token(); expr->right = parse_expression(Precedence::PREFIX); return expr; }
std::unique_ptr<Expression> Parser::parse_infix_expression(std::unique_ptr<Expression> left) { auto expr = std::make_unique<InfixExpression>(); expr->token = current_token; expr->op = current_token.literal; expr->left = std::move(left); Precedence p = current_precedence(); next_token(); expr->right = parse_expression(p); return expr; }
std::unique_ptr<Expression> Parser::parse_assignment_expression(std::unique_ptr<Expression> left) { auto ident_node = dynamic_cast<Identifier*>(left.get()); if (!ident_node) return nullptr; auto expr = std::make_unique<AssignmentExpression>(); expr->token = current_token; left.release(); expr->name = std::unique_ptr<Identifier>(ident_node); Precedence p = current_precedence(); next_token(); expr->value = parse_expression(p); return expr; }
std::unique_ptr<BlockStatement> Parser::parse_block_statement() { auto block = std::make_unique<BlockStatement>(); block->token = current_token; next_token(); while (current_token.type != TokenType::RBRACE && current_token.type != TokenType::END_OF_FILE) { auto stmt = parse_statement(); if (stmt) block->statements.push_back(std::move(stmt)); next_token(); } return block; }
std::vector<std::unique_ptr<Expression>> Parser::parse_expression_list(TokenType end_token) { std::vector<std::unique_ptr<Expression>> list; if (peek_token.type == end_token) { next_token(); return list; } next_token(); list.push_back(parse_expression(Precedence::LOWEST)); while (peek_token.type == TokenType::COMMA) { next_token(); next_token(); list.push_back(parse_expression(Precedence::LOWEST)); } if (peek_token.type != end_token) return {}; next_token(); return list; }
std::vector<std::unique_ptr<Expression>> Parser::parse_call_arguments() { return parse_expression_list(TokenType::RPAREN); }
std::unique_ptr<Expression> Parser::parse_call_expression(std::unique_ptr<Expression> function) { auto expr = std::make_unique<CallExpression>(); expr->token = current_token; expr->function = std::move(function); expr->arguments = parse_call_arguments(); return expr; }
std::unique_ptr<Expression> Parser::parse_if_expression() { auto expr = std::make_unique<IfExpression>(); expr->token = current_token; if (peek_token.type != TokenType::LPAREN) return nullptr; next_token(); next_token(); expr->condition = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::RPAREN) return nullptr; next_token(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); expr->consequence = parse_block_statement(); if (peek_token.type == TokenType::ELSE) { next_token(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); expr->alternative = parse_block_statement(); } return expr; }
std::unique_ptr<Expression> Parser::parse_while_expression() { auto expr = std::make_unique<WhileExpression>(); expr->token = current_token; if (peek_token.type != TokenType::LPAREN) return nullptr; next_token(); next_token(); expr->condition = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::RPAREN) return nullptr; next_token(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); expr->body = parse_block_statement(); return expr; }
std::unique_ptr<Expression> Parser::parse_for_loop_expression() { auto expr = std::make_unique<ForLoopExpression>(); expr->token = current_token; if (peek_token.type != TokenType::LPAREN) return nullptr; next_token(); next_token(); if (current_token.type != TokenType::SEMICOLON) expr->initializer = parse_statement(); if (current_token.type != TokenType::SEMICOLON) return nullptr; next_token(); if (current_token.type != TokenType::SEMICOLON) expr->condition = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::SEMICOLON) return nullptr; next_token(); next_token(); if (current_token.type != TokenType::RPAREN) expr->increment = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::RPAREN) return nullptr; next_token(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); expr->body = parse_block_statement(); return expr; }
std::vector<std::unique_ptr<Identifier>> Parser::parse_function_parameters() { std::vector<std::unique_ptr<Identifier>> params; if (peek_token.type == TokenType::RPAREN) { next_token(); return params; } next_token(); auto ident = std::make_unique<Identifier>(); ident->token = current_token; ident->value = current_token.literal; params.push_back(std::move(ident)); while (peek_token.type == TokenType::COMMA) { next_token(); next_token(); auto next_ident = std::make_unique<Identifier>(); next_ident->token = current_token; next_ident->value = current_token.literal; params.push_back(std::move(next_ident)); } if (peek_token.type != TokenType::RPAREN) return {}; next_token(); return params; }
std::unique_ptr<Expression> Parser::parse_function_literal() { auto func = std::make_unique<FunctionLiteral>(); func->token = current_token; if (peek_token.type != TokenType::LPAREN) return nullptr; next_token(); func->parameters = parse_function_parameters(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); func->body = parse_block_statement(); return func; }