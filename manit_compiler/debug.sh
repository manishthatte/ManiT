#!/bin/bash

echo "--- Overwriting all source files with definitive versions ---"

# --- token.hpp ---
cat <<'EOF' > src/token.hpp
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
EOF

# --- lexer.hpp ---
cat <<'EOF' > src/lexer.hpp
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
EOF

# --- lexer.cpp ---
cat <<'EOF' > src/lexer.cpp
#include "lexer.hpp"
#include <map>

bool is_letter(char ch) { return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_'; }
bool is_digit(char ch) { return '0' <= ch && ch <= '9'; }

std::map<std::string, TokenType> keywords = {
    {"fn", TokenType::FN}, {"let", TokenType::LET}, {"var", TokenType::VAR},
    {"if", TokenType::IF}, {"else", TokenType::ELSE}, {"while", TokenType::WHILE},
    {"for", TokenType::FOR}, {"return", TokenType::RETURN},
    {"true", TokenType::TRUE}, {"false", TokenType::FALSE},
};

Lexer::Lexer(std::string input) : input(input), position(0), read_position(0), ch(0) { read_char(); }

void Lexer::read_char() {
    if (read_position >= input.length()) { ch = 0; } else { ch = input[read_position]; }
    position = read_position;
    read_position += 1;
}

char Lexer::peek_char() {
    if (read_position >= input.length()) { return 0; }
    return input[read_position];
}

void Lexer::skip_whitespace() {
    while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') { read_char(); }
}

Token Lexer::read_identifier() {
    size_t start_pos = position;
    while (is_letter(ch)) { read_char(); }
    std::string literal = input.substr(start_pos, position - start_pos);
    if (keywords.count(literal)) { return {keywords[literal], literal}; }
    return {TokenType::IDENTIFIER, literal};
}

Token Lexer::read_number() {
    size_t start_pos = position;
    while (is_digit(ch)) { read_char(); }
    return {TokenType::INTEGER_LITERAL, input.substr(start_pos, position - start_pos)};
}

Token Lexer::next_token() {
    Token tok;
    skip_whitespace();
    switch (ch) {
        case '=': if (peek_char() == '=') { read_char(); tok = {TokenType::EQUAL_EQUAL, "=="}; } else { tok = {TokenType::EQUAL, "="}; } break;
        case '+': tok = {TokenType::PLUS, "+"}; break;
        case '-': tok = {TokenType::MINUS, "-"}; break;
        case '!': if (peek_char() == '=') { read_char(); tok = {TokenType::BANG_EQUAL, "!="}; } else { tok = {TokenType::BANG, "!"}; } break;
        case '*': tok = {TokenType::STAR, "*"}; break;
        case '/': if (peek_char() == '/') { while (ch != '\n' && ch != 0) read_char(); return next_token(); } else { tok = {TokenType::SLASH, "/"}; } break;
        case '<': if (peek_char() == '=') { read_char(); tok = {TokenType::LESS_EQUAL, "<="}; } else { tok = {TokenType::LESS, "<"}; } break;
        case '>': if (peek_char() == '=') { read_char(); tok = {TokenType::GREATER_EQUAL, ">="}; } else { tok = {TokenType::GREATER, ">"}; } break;
        case ';': tok = {TokenType::SEMICOLON, ";"}; break;
        case '(': tok = {TokenType::LPAREN, "("}; break;
        case ')': tok = {TokenType::RPAREN, ")"}; break;
        case '{': tok = {TokenType::LBRACE, "{"}; break;
        case '}': tok = {TokenType::RBRACE, "}"}; break;
        case '[': tok = {TokenType::LBRACKET, "["}; break;
        case ']': tok = {TokenType::RBRACKET, "]"}; break;
        case ',': tok = {TokenType::COMMA, ","}; break;
        case 0: tok = {TokenType::END_OF_FILE, ""}; break;
        default:
            if (is_letter(ch)) { return read_identifier(); }
            else if (is_digit(ch)) { return read_number(); }
            else { tok = {TokenType::ILLEGAL, std::string(1, ch)}; }
    }
    read_char();
    return tok;
}
EOF

# --- ast.hpp ---
cat <<'EOF' > src/ast.hpp
#ifndef MANIT_AST_HPP
#define MANIT_AST_HPP

#include "token.hpp"
#include <string>
#include <vector>
#include <memory>

struct Statement;
struct BlockStatement;
struct Identifier;

struct Node { virtual ~Node() = default; virtual std::string to_string() const = 0; };
struct Expression : public Node {};
struct Statement : public Node {};

struct Program : public Node { std::vector<std::unique_ptr<Statement>> statements; std::string to_string() const override; };
struct Identifier : public Expression { Token token; std::string value; std::string to_string() const override; };
struct IntegerLiteral : public Expression { Token token; long long value; std::string to_string() const override; };
struct BooleanLiteral : public Expression { Token token; bool value; std::string to_string() const override; };
struct ArrayLiteral : public Expression { Token token; std::vector<std::unique_ptr<Expression>> elements; std::string to_string() const override; };
struct PrefixExpression : public Expression { Token token; std::string op; std::unique_ptr<Expression> right; std::string to_string() const override; };
struct InfixExpression : public Expression { Token token; std::unique_ptr<Expression> left; std::string op; std::unique_ptr<Expression> right; std::string to_string() const override; };
struct AssignmentExpression : public Expression { Token token; std::unique_ptr<Identifier> name; std::unique_ptr<Expression> value; std::string to_string() const override; };
struct IndexExpression : public Expression { Token token; std::unique_ptr<Expression> left; std::unique_ptr<Expression> index; std::string to_string() const override; };
struct LetStatement : public Statement { Token token; std::unique_ptr<Identifier> name; std::unique_ptr<Expression> value; std::string to_string() const override; };
struct VarStatement : public Statement { Token token; std::unique_ptr<Identifier> name; std::unique_ptr<Expression> value; std::string to_string() const override; };
struct ReturnStatement : public Statement { Token token; std::unique_ptr<Expression> return_value; std::string to_string() const override; };
struct ExpressionStatement : public Statement { Token token; std::unique_ptr<Expression> expression; std::string to_string() const override; };
struct BlockStatement : public Statement { Token token; std::vector<std::unique_ptr<Statement>> statements; std::string to_string() const override; };
struct IfExpression : public Expression { Token token; std::unique_ptr<Expression> condition; std::unique_ptr<BlockStatement> consequence; std::unique_ptr<BlockStatement> alternative; std::string to_string() const override; };
struct FunctionLiteral : public Expression { Token token; std::vector<std::unique_ptr<Identifier>> parameters; std::unique_ptr<BlockStatement> body; std::string to_string() const override; };
struct CallExpression : public Expression { Token token; std::unique_ptr<Expression> function; std::vector<std::unique_ptr<Expression>> arguments; std::string to_string() const override; };
struct WhileExpression : public Expression { Token token; std::unique_ptr<Expression> condition; std::unique_ptr<BlockStatement> body; std::string to_string() const override; };
struct ForLoopExpression : public Expression { Token token; std::unique_ptr<Statement> initializer; std::unique_ptr<Expression> condition; std::unique_ptr<Expression> increment; std::unique_ptr<BlockStatement> body; std::string to_string() const override; };

#endif // MANIT_AST_HPP
EOF

# --- ast.cpp ---
cat <<'EOF' > src/ast.cpp
#include "ast.hpp"
#include <sstream>

std::string Program::to_string() const { std::stringstream ss; for (const auto& s : statements) { ss << s->to_string(); } return ss.str(); }
std::string Identifier::to_string() const { return value; }
std::string IntegerLiteral::to_string() const { return token.literal; }
std::string BooleanLiteral::to_string() const { return token.literal; }
std::string PrefixExpression::to_string() const { std::stringstream ss; ss << "(" << op << right->to_string() << ")"; return ss.str(); }
std::string InfixExpression::to_string() const { std::stringstream ss; ss << "(" << left->to_string() << " " << op << " " << right->to_string() << ")"; return ss.str(); }
std::string AssignmentExpression::to_string() const { std::stringstream ss; ss << "(" << name->to_string() << " = " << value->to_string() << ")"; return ss.str(); }
std::string IndexExpression::to_string() const { std::stringstream ss; ss << "(" << left->to_string() << "[" << index->to_string() << "])"; return ss.str(); }
std::string LetStatement::to_string() const { std::stringstream ss; ss << token.literal << " " << name->to_string() << " = "; if (value) { ss << value->to_string(); } ss << ";"; return ss.str(); }
std::string VarStatement::to_string() const { std::stringstream ss; ss << token.literal << " " << name->to_string() << " = "; if (value) { ss << value->to_string(); } ss << ";"; return ss.str(); }
std::string ReturnStatement::to_string() const { std::stringstream ss; ss << token.literal << " "; if (return_value) { ss << return_value->to_string(); } ss << ";"; return ss.str(); }
std::string ExpressionStatement::to_string() const { if (expression) { return expression->to_string() + ";"; } return ";"; }
std::string BlockStatement::to_string() const { std::stringstream ss; for (const auto& s : statements) { ss << s->to_string(); } return ss.str(); }
std::string IfExpression::to_string() const { std::stringstream ss; ss << "if" << condition->to_string() << " " << consequence->to_string(); if (alternative) { ss << "else " << alternative->to_string(); } return ss.str(); }
std::string FunctionLiteral::to_string() const { std::stringstream ss; ss << token.literal << "("; for (size_t i = 0; i < parameters.size(); ++i) { ss << parameters[i]->to_string() << (i < parameters.size() - 1 ? ", " : ""); } ss << ") " << body->to_string(); return ss.str(); }
std::string CallExpression::to_string() const { std::stringstream ss; ss << function->to_string() << "("; for (size_t i = 0; i < arguments.size(); ++i) { ss << arguments[i]->to_string() << (i < arguments.size() - 1 ? ", " : ""); } ss << ")"; return ss.str(); }
std::string WhileExpression::to_string() const { std::stringstream ss; ss << "while(" << condition->to_string() << ") {" << body->to_string() << "}"; return ss.str(); }
std::string ArrayLiteral::to_string() const { std::stringstream ss; ss << "["; for (size_t i = 0; i < elements.size(); ++i) { ss << elements[i]->to_string() << (i < elements.size() - 1 ? ", " : ""); } ss << "]"; return ss.str(); }
std::string ForLoopExpression::to_string() const { std::stringstream ss; ss << "for("; std::string init_str = initializer ? initializer->to_string() : ""; if (!init_str.empty() && init_str.back() == ';') { init_str.pop_back(); } ss << init_str << "; "; if (condition) { ss << condition->to_string(); } ss << "; "; if (increment) { ss << increment->to_string(); } ss << ") { " << body->to_string() << " }"; return ss.str(); }
EOF

# --- parser.hpp ---
cat <<'EOF' > src/parser.hpp
#ifndef MANIT_PARSER_HPP
#define MANIT_PARSER_HPP

#include "lexer.hpp"
#include "ast.hpp"
#include <memory>
#include <map>
#include <vector>

enum Precedence { LOWEST, ASSIGN, EQUALS, LESSGREATER, SUM, PRODUCT, PREFIX, CALL, INDEX };

class Parser {
public:
    Parser(Lexer& l);
    std::unique_ptr<Program> parse_program();
private:
    Lexer& lexer;
    Token current_token, peek_token;
    std::map<TokenType, Precedence> precedences;
    void next_token();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<LetStatement> parse_let_statement();
    std::unique_ptr<VarStatement> parse_var_statement();
    std::unique_ptr<ReturnStatement> parse_return_statement();
    std::unique_ptr<ExpressionStatement> parse_expression_statement();
    std::unique_ptr<Expression> parse_expression(Precedence precedence);
    std::unique_ptr<Expression> parse_identifier();
    std::unique_ptr<Expression> parse_integer_literal();
    std::unique_ptr<Expression> parse_boolean_literal();
    std::unique_ptr<Expression> parse_array_literal();
    std::unique_ptr<Expression> parse_index_expression(std::unique_ptr<Expression> left);
    std::unique_ptr<Expression> parse_prefix_expression();
    std::unique_ptr<Expression> parse_infix_expression(std::unique_ptr<Expression> left);
    std::unique_ptr<Expression> parse_assignment_expression(std::unique_ptr<Expression> left);
    std::unique_ptr<Expression> parse_if_expression();
    std::unique_ptr<BlockStatement> parse_block_statement();
    std::unique_ptr<Expression> parse_function_literal();
    std::unique_ptr<Expression> parse_call_expression(std::unique_ptr<Expression> function);
    std::vector<std::unique_ptr<Identifier>> parse_function_parameters();
    std::vector<std::unique_ptr<Expression>> parse_call_arguments();
    std::unique_ptr<Expression> parse_while_expression();
    std::unique_ptr<Expression> parse_for_loop_expression();
    std::vector<std::unique_ptr<Expression>> parse_expression_list(TokenType end_token);
    Precedence peek_precedence();
    Precedence current_precedence();
};

#endif // MANIT_PARSER_HPP
EOF

# --- parser.cpp ---
cat <<'EOF' > src/parser.cpp
#include "parser.hpp"
#include <charconv>
#include <system_error>

Parser::Parser(Lexer& l) : lexer(l) {
    precedences = { {TokenType::EQUAL, Precedence::ASSIGN}, {TokenType::EQUAL_EQUAL, Precedence::EQUALS}, {TokenType::BANG_EQUAL, Precedence::EQUALS}, {TokenType::LESS, Precedence::LESSGREATER}, {TokenType::GREATER, Precedence::LESSGREATER}, {TokenType::LESS_EQUAL, Precedence::LESSGREATER}, {TokenType::GREATER_EQUAL, Precedence::LESSGREATER}, {TokenType::PLUS, Precedence::SUM}, {TokenType::MINUS, Precedence::SUM}, {TokenType::SLASH, Precedence::PRODUCT}, {TokenType::STAR, Precedence::PRODUCT}, {TokenType::LPAREN, Precedence::CALL}, {TokenType::LBRACKET, Precedence::INDEX}, };
    next_token(); next_token();
}
void Parser::next_token() { current_token = peek_token; peek_token = lexer.next_token(); }
Precedence Parser::peek_precedence() { if (precedences.count(peek_token.type)) return precedences[peek_token.type]; return Precedence::LOWEST; }
Precedence Parser::current_precedence() { if (precedences.count(current_token.type)) return precedences[current_token.type]; return Precedence::LOWEST; }
std::unique_ptr<Program> Parser::parse_program() { auto program = std::make_unique<Program>(); while (current_token.type != TokenType::END_OF_FILE) { auto stmt = parse_statement(); if (stmt) program->statements.push_back(std::move(stmt)); next_token(); } return program; }
std::unique_ptr<Statement> Parser::parse_statement() { switch (current_token.type) { case TokenType::LET: return parse_let_statement(); case TokenType::VAR: return parse_var_statement(); case TokenType::RETURN: return parse_return_statement(); default: return parse_expression_statement(); } }
std::unique_ptr<Expression> Parser::parse_expression(Precedence precedence) {
    std::unique_ptr<Expression> left_exp;
    switch (current_token.type) {
        case TokenType::IDENTIFIER: left_exp = parse_identifier(); break; case TokenType::INTEGER_LITERAL: left_exp = parse_integer_literal(); break;
        case TokenType::TRUE: case TokenType::FALSE: left_exp = parse_boolean_literal(); break; case TokenType::LBRACKET: left_exp = parse_array_literal(); break;
        case TokenType::BANG: case TokenType::MINUS: left_exp = parse_prefix_expression(); break; case TokenType::IF: left_exp = parse_if_expression(); break;
        case TokenType::FN: left_exp = parse_function_literal(); break; case TokenType::WHILE: left_exp = parse_while_expression(); break; case TokenType::FOR: left_exp = parse_for_loop_expression(); break;
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
std::unique_ptr<LetStatement> Parser::parse_let_statement() { auto stmt = std::make_unique<LetStatement>(); stmt->token = current_token; if (peek_token.type != TokenType::IDENTIFIER) return nullptr; next_token(); auto ident = std::make_unique<Identifier>(); ident->token = current_token; ident->value = current_token.literal; stmt->name = std::move(ident); if (peek_token.type != TokenType::EQUAL) return nullptr; next_token(); next_token(); stmt->value = parse_expression(Precedence::LOWEST); if (peek_token.type == TokenType::SEMICOLON) next_token(); return stmt; }
std::unique_ptr<VarStatement> Parser::parse_var_statement() { auto stmt = std::make_unique<VarStatement>(); stmt->token = current_token; if (peek_token.type != TokenType::IDENTIFIER) return nullptr; next_token(); auto ident = std::make_unique<Identifier>(); ident->token = current_token; ident->value = current_token.literal; stmt->name = std::move(ident); if (peek_token.type != TokenType::EQUAL) return nullptr; next_token(); next_token(); stmt->value = parse_expression(Precedence::LOWEST); if (peek_token.type == TokenType::SEMICOLON) next_token(); return stmt; }
std::unique_ptr<ReturnStatement> Parser::parse_return_statement() { auto stmt = std::make_unique<ReturnStatement>(); stmt->token = current_token; next_token(); stmt->return_value = parse_expression(Precedence::LOWEST); if (peek_token.type == TokenType::SEMICOLON) next_token(); return stmt; }
std::unique_ptr<ExpressionStatement> Parser::parse_expression_statement() { auto stmt = std::make_unique<ExpressionStatement>(); stmt->token = current_token; stmt->expression = parse_expression(Precedence::LOWEST); if (peek_token.type == TokenType::SEMICOLON) next_token(); return stmt; }
std::unique_ptr<BlockStatement> Parser::parse_block_statement() { auto block = std::make_unique<BlockStatement>(); block->token = current_token; next_token(); while (current_token.type != TokenType::RBRACE && current_token.type != TokenType::END_OF_FILE) { auto stmt = parse_statement(); if (stmt) block->statements.push_back(std::move(stmt)); next_token(); } return block; }
std::vector<std::unique_ptr<Expression>> Parser::parse_expression_list(TokenType end_token) { std::vector<std::unique_ptr<Expression>> list; if (peek_token.type == end_token) { next_token(); return list; } next_token(); list.push_back(parse_expression(Precedence::LOWEST)); while (peek_token.type == TokenType::COMMA) { next_token(); next_token(); list.push_back(parse_expression(Precedence::LOWEST)); } if (peek_token.type != end_token) return {}; next_token(); return list; }
std::vector<std::unique_ptr<Expression>> Parser::parse_call_arguments() { return parse_expression_list(TokenType::RPAREN); }
std::unique_ptr<Expression> Parser::parse_call_expression(std::unique_ptr<Expression> function) { auto expr = std::make_unique<CallExpression>(); expr->token = current_token; expr->function = std::move(function); expr->arguments = parse_call_arguments(); return expr; }
std::unique_ptr<Expression> Parser::parse_if_expression() { auto expr = std::make_unique<IfExpression>(); expr->token = current_token; if (peek_token.type != TokenType::LPAREN) return nullptr; next_token(); next_token(); expr->condition = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::RPAREN) return nullptr; next_token(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); expr->consequence = parse_block_statement(); if (peek_token.type == TokenType::ELSE) { next_token(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); expr->alternative = parse_block_statement(); } return expr; }
std::unique_ptr<Expression> Parser::parse_while_expression() { auto expr = std::make_unique<WhileExpression>(); expr->token = current_token; if (peek_token.type != TokenType::LPAREN) return nullptr; next_token(); next_token(); expr->condition = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::RPAREN) return nullptr; next_token(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); expr->body = parse_block_statement(); return expr; }
std::unique_ptr<Expression> Parser::parse_for_loop_expression() { auto expr = std::make_unique<ForLoopExpression>(); expr->token = current_token; if (peek_token.type != TokenType::LPAREN) return nullptr; next_token(); next_token(); if (current_token.type != TokenType::SEMICOLON) expr->initializer = parse_statement(); if (current_token.type != TokenType::SEMICOLON) return nullptr; next_token(); if (current_token.type != TokenType::SEMICOLON) expr->condition = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::SEMICOLON) return nullptr; next_token(); next_token(); if (current_token.type != TokenType::RPAREN) expr->increment = parse_expression(Precedence::LOWEST); if (peek_token.type != TokenType::RPAREN) return nullptr; next_token(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); expr->body = parse_block_statement(); return expr; }
std::vector<std::unique_ptr<Identifier>> Parser::parse_function_parameters() { std::vector<std::unique_ptr<Identifier>> params; if (peek_token.type == TokenType::RPAREN) { next_token(); return params; } next_token(); auto ident = std::make_unique<Identifier>(); ident->token = current_token; ident->value = current_token.literal; params.push_back(std::move(ident)); while (peek_token.type == TokenType::COMMA) { next_token(); next_token(); auto next_ident = std::make_unique<Identifier>(); next_ident->token = current_token; next_ident->value = current_token.literal; params.push_back(std::move(next_ident)); } if (peek_token.type != TokenType::RPAREN) return {}; next_token(); return params; }
std::unique_ptr<Expression> Parser::parse_function_literal() { auto func = std::make_unique<FunctionLiteral>(); func->token = current_token; if (peek_token.type != TokenType::LPAREN) return nullptr; next_token(); func->parameters = parse_function_parameters(); if (peek_token.type != TokenType::LBRACE) return nullptr; next_token(); func->body = parse_block_statement(); return func; }
EOF

# --- codegen.hpp ---
cat <<'EOF' > src/codegen.hpp
#ifndef MANIT_CODEGEN_HPP
#define MANIT_CODEGEN_HPP

#include "ast.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <map>
#include <string>

namespace llvm { class AllocaInst; class Function; class Type; }

class CodeGenerator {
public:
    CodeGenerator();
    void generate(const Program& program);
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::map<std::string, llvm::AllocaInst*> named_values;
    llvm::Value* generate_expression(const Expression& expr);
    void generate_statement(const Statement& stmt);
    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* the_function, const std::string& var_name, llvm::Type* type);
};

#endif // MANIT_CODEGEN_HPP
EOF

# --- codegen.cpp ---
cat <<'EOF' > src/codegen.cpp
#include "codegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

CodeGenerator::CodeGenerator() { context = std::make_unique<llvm::LLVMContext>(); module = std::make_unique<llvm::Module>("ManiT_Module", *context); builder = std::make_unique<llvm::IRBuilder<>>(*context); }
llvm::AllocaInst* CodeGenerator::create_entry_block_alloca(llvm::Function* the_function, const std::string& var_name, llvm::Type* type) { llvm::IRBuilder<> tmp_builder(&the_function->getEntryBlock(), the_function->getEntryBlock().begin()); return tmp_builder.CreateAlloca(type, nullptr, var_name); }
llvm::Value* CodeGenerator::generate_expression(const Expression& expr) {
    if (auto const* int_lit = dynamic_cast<const IntegerLiteral*>(&expr)) { return builder->getInt32(int_lit->value); }
    else if (auto const* bool_lit = dynamic_cast<const BooleanLiteral*>(&expr)) { return builder->getInt1(bool_lit->value); }
    else if (auto const* array_lit = dynamic_cast<const ArrayLiteral*>(&expr)) {
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::Type* element_type = builder->getInt32Ty();
        uint64_t array_size = array_lit->elements.size();
        llvm::ArrayType* array_type = llvm::ArrayType::get(element_type, array_size);
        llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, "array_lit", array_type);
        for (uint64_t i = 0; i < array_size; ++i) {
            llvm::Value* element_val = generate_expression(*array_lit->elements[i]);
            if (!element_val) return nullptr;
            std::vector<llvm::Value*> indices = { builder->getInt32(0), builder->getInt32(i) };
            llvm::Value* element_ptr = builder->CreateGEP(array_type, alloca, indices, "element_ptr");
            builder->CreateStore(element_val, element_ptr);
        }
        return alloca;
    }
    else if (auto const* index_expr = dynamic_cast<const IndexExpression*>(&expr)) {
        llvm::Value* array_val = generate_expression(*index_expr->left);
        if (!array_val) return nullptr;
        llvm::Value* index_val = generate_expression(*index_expr->index);
        if (!index_val) return nullptr;
        auto* array_alloca = llvm::cast<llvm::AllocaInst>(array_val);
        llvm::Type* array_type = array_alloca->getAllocatedType();
        std::vector<llvm::Value*> indices = { builder->getInt32(0), index_val };
        llvm::Value* element_ptr = builder->CreateGEP(array_type, array_val, indices, "element_ptr");
        llvm::Type* element_type = llvm::cast<llvm::ArrayType>(array_type)->getElementType();
        return builder->CreateLoad(element_type, element_ptr, "array_idx_val");
    }
    else if (auto const* ident = dynamic_cast<const Identifier*>(&expr)) {
        if (named_values.count(ident->value)) {
            llvm::AllocaInst* alloca = named_values[ident->value];
            llvm::Type* var_type = alloca->getAllocatedType();
            if (var_type->isArrayTy()) { return alloca; }
            else { return builder->CreateLoad(var_type, alloca, ident->value.c_str()); }
        }
        return nullptr;
    }
    else if (auto const* assign_expr = dynamic_cast<const AssignmentExpression*>(&expr)) {
        if (named_values.find(assign_expr->name->value) == named_values.end()) return nullptr;
        llvm::AllocaInst* variable_alloca = named_values[assign_expr->name->value];
        llvm::Value* new_val = generate_expression(*assign_expr->value);
        if (!new_val) return nullptr;
        builder->CreateStore(new_val, variable_alloca);
        return new_val;
    }
    else if (auto const* prefix_expr = dynamic_cast<const PrefixExpression*>(&expr)) {
        llvm::Value* right = generate_expression(*prefix_expr->right);
        if (!right) return nullptr;
        if (prefix_expr->op == "-") { return builder->CreateNeg(right, "negtmp"); }
        return nullptr;
    }
    else if (auto const* infix_expr = dynamic_cast<const InfixExpression*>(&expr)) {
        llvm::Value* left = generate_expression(*infix_expr->left);
        llvm::Value* right = generate_expression(*infix_expr->right);
        if (!left || !right) return nullptr;
        if (infix_expr->op == "+") { return builder->CreateAdd(left, right, "addtmp"); }
        else if (infix_expr->op == "-") { return builder->CreateSub(left, right, "subtmp"); }
        else if (infix_expr->op == "*") { return builder->CreateMul(left, right, "multmp"); }
        else if (infix_expr->op == "/") { return builder->CreateSDiv(left, right, "divtmp"); }
        else if (infix_expr->op == "==") { return builder->CreateICmpEQ(left, right, "eqtmp"); }
        else if (infix_expr->op == "!=") { return builder->CreateICmpNE(left, right, "neqtmp"); }
        else if (infix_expr->op == "<") { return builder->CreateICmpSLT(left, right, "lttmp"); }
        else if (infix_expr->op == "<=") { return builder->CreateICmpSLE(left, right, "letmp"); }
        else if (infix_expr->op == ">") { return builder->CreateICmpSGT(left, right, "gttmp"); }
        else if (infix_expr->op == ">=") { return builder->CreateICmpSGE(left, right, "getmp"); }
        return nullptr;
    }
    else if (auto const* if_expr = dynamic_cast<const IfExpression*>(&expr)) {
        llvm::Value* cond_v = generate_expression(*if_expr->condition); if (!cond_v) return nullptr;
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(*context, "then", the_function);
        llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(*context, "else");
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*context, "ifcont");
        if (if_expr->alternative) { builder->CreateCondBr(cond_v, then_bb, else_bb); } else { builder->CreateCondBr(cond_v, then_bb, merge_bb); }
        builder->SetInsertPoint(then_bb);
        llvm::Value* then_val = nullptr;
        if (!if_expr->consequence->statements.empty()) { if (auto* last_stmt_as_expr = dynamic_cast<ExpressionStatement*>(if_expr->consequence->statements.back().get())) { for (size_t i = 0; i < if_expr->consequence->statements.size() - 1; ++i) generate_statement(*if_expr->consequence->statements[i]); then_val = generate_expression(*last_stmt_as_expr->expression); } else { for (const auto& stmt : if_expr->consequence->statements) generate_statement(*stmt); } }
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(merge_bb);
        llvm::BasicBlock* then_end_bb = builder->GetInsertBlock();
        llvm::Value* else_val = nullptr;
        llvm::BasicBlock* else_end_bb = else_bb;
        if (if_expr->alternative) {
            the_function->insert(the_function->end(), else_bb);
            builder->SetInsertPoint(else_bb);
            if (!if_expr->alternative->statements.empty()) { if (auto* last_stmt_as_expr = dynamic_cast<ExpressionStatement*>(if_expr->alternative->statements.back().get())) { for (size_t i = 0; i < if_expr->alternative->statements.size() - 1; ++i) generate_statement(*if_expr->alternative->statements[i]); else_val = generate_expression(*last_stmt_as_expr->expression); } else { for (const auto& stmt : if_expr->alternative->statements) generate_statement(*stmt); } }
            if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(merge_bb);
            else_end_bb = builder->GetInsertBlock();
        }
        the_function->insert(the_function->end(), merge_bb);
        builder->SetInsertPoint(merge_bb);
        if (then_val || else_val) { llvm::PHINode* pn = builder->CreatePHI(builder->getInt32Ty(), 2, "iftmp"); pn->addIncoming(then_val ? then_val : builder->getInt32(0), then_end_bb); pn->addIncoming(else_val ? else_val : builder->getInt32(0), else_end_bb); return pn; }
        return builder->getInt32(0);
    }
    else if (auto const* func_lit = dynamic_cast<const FunctionLiteral*>(&expr)) {
        llvm::BasicBlock* original_block = builder->GetInsertBlock(); auto old_named_values = named_values;
        std::vector<llvm::Type*> param_types(func_lit->parameters.size(), builder->getInt32Ty());
        llvm::FunctionType* func_type = llvm::FunctionType::get(builder->getInt32Ty(), param_types, false);
        llvm::Function* the_function = llvm::Function::Create(func_type, llvm::Function::InternalLinkage, "user_fn", module.get());
        llvm::BasicBlock* func_entry_block = llvm::BasicBlock::Create(*context, "entry", the_function); builder->SetInsertPoint(func_entry_block);
        named_values.clear(); size_t i = 0;
        for (auto& arg : the_function->args()) { const std::string& param_name = func_lit->parameters[i++]->value; arg.setName(param_name); llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, param_name, builder->getInt32Ty()); builder->CreateStore(&arg, alloca); named_values[param_name] = alloca; }
        for (const auto& stmt : func_lit->body->statements) generate_statement(*stmt);
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateRet(builder->getInt32(0));
        llvm::verifyFunction(*the_function); builder->SetInsertPoint(original_block); named_values = old_named_values; return the_function;
    }
    else if (auto const* call_expr = dynamic_cast<const CallExpression*>(&expr)) {
        auto const* ident = dynamic_cast<const Identifier*>(call_expr->function.get()); if (!ident) return nullptr;
        llvm::Function* callee_func = module->getFunction(ident->value); if (!callee_func) return nullptr; if (callee_func->arg_size() != call_expr->arguments.size()) return nullptr;
        std::vector<llvm::Value*> args_v;
        for (const auto& arg : call_expr->arguments) { args_v.push_back(generate_expression(*arg)); if (!args_v.back()) return nullptr; }
        return builder->CreateCall(callee_func, args_v, "calltmp");
    }
    else if (auto const* while_expr = dynamic_cast<const WhileExpression*>(&expr)) {
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* loop_header_bb = llvm::BasicBlock::Create(*context, "loop_header", the_function);
        llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(*context, "loop_body", the_function);
        llvm::BasicBlock* loop_exit_bb = llvm::BasicBlock::Create(*context, "loop_exit", the_function);
        builder->CreateBr(loop_header_bb); builder->SetInsertPoint(loop_header_bb);
        llvm::Value* cond_v = generate_expression(*while_expr->condition); if (!cond_v) return nullptr;
        builder->CreateCondBr(cond_v, loop_body_bb, loop_exit_bb);
        builder->SetInsertPoint(loop_body_bb); for (const auto& stmt : while_expr->body->statements) generate_statement(*stmt);
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(loop_header_bb);
        builder->SetInsertPoint(loop_exit_bb); return llvm::Constant::getNullValue(builder->getInt32Ty());
    }
    else if (auto const* for_expr = dynamic_cast<const ForLoopExpression*>(&expr)) {
        auto old_named_values = named_values; if (for_expr->initializer) generate_statement(*for_expr->initializer);
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* loop_header_bb = llvm::BasicBlock::Create(*context, "loop_header", the_function);
        llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(*context, "loop_body", the_function);
        llvm::BasicBlock* loop_inc_bb = llvm::BasicBlock::Create(*context, "loop_inc", the_function);
        llvm::BasicBlock* loop_exit_bb = llvm::BasicBlock::Create(*context, "loop_exit", the_function);
        builder->CreateBr(loop_header_bb); builder->SetInsertPoint(loop_header_bb);
        llvm::Value* cond_v; if (for_expr->condition) { cond_v = generate_expression(*for_expr->condition); if (!cond_v) return nullptr; } else { cond_v = builder->getInt1(true); }
        builder->CreateCondBr(cond_v, loop_body_bb, loop_exit_bb);
        builder->SetInsertPoint(loop_body_bb); if (for_expr->body) { for (const auto& stmt : for_expr->body->statements) generate_statement(*stmt); }
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(loop_inc_bb);
        builder->SetInsertPoint(loop_inc_bb); if (for_expr->increment) generate_expression(*for_expr->increment);
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(loop_header_bb);
        builder->SetInsertPoint(loop_exit_bb); named_values = old_named_values; return llvm::Constant::getNullValue(builder->getInt32Ty());
    }
    return nullptr;
}
void CodeGenerator::generate_statement(const Statement& stmt) {
    if (auto const* let_stmt = dynamic_cast<const LetStatement*>(&stmt)) {
        llvm::Value* val = generate_expression(*let_stmt->value);
        if (val) {
            if (auto* func = llvm::dyn_cast<llvm::Function>(val)) { func->setName(let_stmt->name->value); return; }
            if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(val)) { alloca->setName(let_stmt->name->value); named_values[let_stmt->name->value] = alloca; }
            else { llvm::Function* the_function = builder->GetInsertBlock()->getParent(); llvm::AllocaInst* scalar_alloca = create_entry_block_alloca(the_function, let_stmt->name->value, val->getType()); builder->CreateStore(val, scalar_alloca); named_values[let_stmt->name->value] = scalar_alloca; }
        }
    }
    else if (auto const* var_stmt = dynamic_cast<const VarStatement*>(&stmt)) {
        llvm::Value* val = generate_expression(*var_stmt->value);
        if (val) { llvm::Function* the_function = builder->GetInsertBlock()->getParent(); llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, var_stmt->name->value, val->getType()); builder->CreateStore(val, alloca); named_values[var_stmt->name->value] = alloca; }
    }
    else if (auto const* return_stmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        if (return_stmt->return_value) { llvm::Value* return_val = generate_expression(*return_stmt->return_value); if (return_val) builder->CreateRet(return_val); }
        else { builder->CreateRetVoid(); }
    }
    else if (auto const* expr_stmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        generate_expression(*expr_stmt->expression);
    }
}
void CodeGenerator::generate(const Program& program) {
    llvm::FunctionType* func_type = llvm::FunctionType::get(builder->getInt32Ty(), false);
    llvm::Function* main_func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "main", module.get());
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", main_func); builder->SetInsertPoint(entry);
    for (const auto& stmt : program.statements) { generate_statement(*stmt); }
    if (!builder->GetInsertBlock()->getTerminator()) { builder->CreateRet(builder->getInt32(0)); }
    bool user_defined_main = false;
    for (const auto& stmt : program.statements) { if (auto const* let_stmt = dynamic_cast<const LetStatement*>(stmt.get())) { if (let_stmt->name->value == "main") { user_defined_main = true; break; } } }
    if (user_defined_main) { main_func->eraseFromParent(); }
    llvm::verifyModule(*module, &llvm::errs()); module->print(llvm::outs(), nullptr);
}
EOF

# --- main.cpp ---
cat <<'EOF' > src/main.cpp
#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

int main() {
    std::string source_code = R"(
        let data = [11, 22, 33];
        return data[1];
    )";
    Lexer l(source_code);
    Parser p(l);
    auto program = p.parse_program();
    if (!program) { std::cerr << "Parsing failed." << std::endl; return 1; }
    CodeGenerator codegen;
    codegen.generate(*program);
    return 0;
}
EOF

# --- manitc.sh ---
cat <<EOF > manitc.sh
#!/bin/bash
mkdir -p build
clang++ -std=c++17 src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp src/ast.cpp \$(llvm-config --cxxflags --ldflags --system-libs --libs core) -o build/manitc
echo "Running ManiT program..."
./build/manitc | lli
result=\$?
echo "--------------------------------"
echo "ManiT program exited with code: \$result"
if [ "\$result" -eq 22 ]; then
    echo "SUCCESS: The result is 22 as expected."
else
    echo "FAILURE: The result was not 22."
fi
EOF

# --- Make script executable and run ---
chmod +x manitc.sh
echo "All files have been reset to a known-good state. Running final test..."
./manitc.sh