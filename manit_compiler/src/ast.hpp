#ifndef MANIT_AST_HPP
#define MANIT_AST_HPP

#include "token.hpp"
#include <string>
#include <vector>
#include <memory>

// Forward declarations
struct Statement;
struct BlockStatement;
struct Identifier;

// Base interface for all nodes in the AST
struct Node {
    virtual ~Node() = default;
    virtual std::string to_string() const = 0;
};

// Base interface for all expression nodes
struct Expression : public Node {};

// Base interface for all statement nodes
struct Statement : public Node {};

// The root of every ManiT program is a sequence of statements
struct Program : public Node {
    std::vector<std::unique_ptr<Statement>> statements;
    std::string to_string() const override;
};

// Represents an identifier, like a variable name `x`
struct Identifier : public Expression {
    Token token; // The IDENTIFIER token
    std::string value;
    std::string to_string() const override;
};

// Represents an integer literal, e.g., `5`
struct IntegerLiteral : public Expression {
    Token token;
    long long value;
    std::string to_string() const override;
};

// Represents a prefix expression, e.g., `-5` or `!true`
struct PrefixExpression : public Expression {
    Token token; // The prefix token, e.g., ! or -
    std::string op;
    std::unique_ptr<Expression> right;
    std::string to_string() const override;
};

// Represents an infix expression, e.g., `5 + 5`
struct InfixExpression : public Expression {
    Token token; // The operator token, e.g., +
    std::unique_ptr<Expression> left;
    std::string op;
    std::unique_ptr<Expression> right;
    std::string to_string() const override;
};

// Represents a `let` statement, e.g., `let x = 5;`
struct LetStatement : public Statement {
    Token token; // The LET token
    std::unique_ptr<Identifier> name;
    std::unique_ptr<Expression> value;
    std::string to_string() const override;
};

// Represents a `return` statement, e.g., `return 5;`
struct ReturnStatement : public Statement {
    Token token; // The RETURN token
    std::unique_ptr<Expression> return_value;
    std::string to_string() const override;
};

// Represents a statement consisting of a single expression, e.g., `x + 5;`
struct ExpressionStatement : public Statement {
    Token token; // The first token of the expression
    std::unique_ptr<Expression> expression;
    std::string to_string() const override;
};

// Represents a block of statements, e.g., `{ let x = 5; return x; }`
struct BlockStatement : public Statement {
    Token token; // The { token
    std::vector<std::unique_ptr<Statement>> statements;
    std::string to_string() const override;
};

// Represents an if-else expression, e.g., `if (x > y) { return x; } else { return y; }`
struct IfExpression : public Expression {
    Token token; // The IF token
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> consequence;
    std::unique_ptr<BlockStatement> alternative; // Can be nullptr for if without else
    std::string to_string() const override;
};

// *** NEW AST NODE ***
// Represents a function definition, e.g., `fn(x, y) { x + y; }`
struct FunctionLiteral : public Expression {
    Token token; // The 'fn' token
    std::vector<std::unique_ptr<Identifier>> parameters;
    std::unique_ptr<BlockStatement> body;
    std::string to_string() const override;
};

// *** NEW AST NODE ***
// Represents a function call, e.g., `add(2, 3)`
struct CallExpression : public Expression {
    Token token; // The '(' token
    std::unique_ptr<Expression> function; // Identifier or FunctionLiteral
    std::vector<std::unique_ptr<Expression>> arguments;
    std::string to_string() const override;
};


#endif // MANIT_AST_HPP