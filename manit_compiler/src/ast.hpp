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
