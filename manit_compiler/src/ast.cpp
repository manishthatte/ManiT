#include "ast.hpp"
#include <sstream>

std::string Program::to_string() const {
    std::stringstream ss;
    for (const auto& stmt : statements) {
        ss << stmt->to_string();
    }
    return ss.str();
}

std::string Identifier::to_string() const {
    return value;
}

std::string IntegerLiteral::to_string() const {
    return token.literal;
}

std::string PrefixExpression::to_string() const {
    std::stringstream ss;
    ss << "(" << op << right->to_string() << ")";
    return ss.str();
}

std::string InfixExpression::to_string() const {
    std::stringstream ss;
    ss << "(" << left->to_string() << " " << op << " " << right->to_string() << ")";
    return ss.str();
}

std::string LetStatement::to_string() const {
    std::stringstream ss;
    ss << token.literal << " " << name->to_string() << " = ";
    if (value) {
        ss << value->to_string();
    }
    ss << ";";
    return ss.str();
}

std::string ReturnStatement::to_string() const {
    std::stringstream ss;
    ss << token.literal << " ";
    if (return_value) {
        ss << return_value->to_string();
    }
    ss << ";";
    return ss.str();
}

std::string ExpressionStatement::to_string() const {
    std::stringstream ss;
    if (expression) {
        ss << expression->to_string();
    }
    ss << ";";
    return ss.str();
}

std::string BlockStatement::to_string() const {
    std::stringstream ss;
    for (const auto& stmt : statements) {
        ss << stmt->to_string();
    }
    return ss.str();
}

std::string IfExpression::to_string() const {
    std::stringstream ss;
    ss << "if" << condition->to_string() << " ";
    ss << "{ " << consequence->to_string() << " }";
    if (alternative) {
        ss << " else { " << alternative->to_string() << " }";
    }
    return ss.str();
}

std::string FunctionLiteral::to_string() const {
    std::stringstream ss;
    ss << token.literal; // "fn"
    ss << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        ss << parameters[i]->to_string();
        if (i < parameters.size() - 1) {
            ss << ", ";
        }
    }
    ss << ") ";
    ss << body->to_string();
    return ss.str();
}

std::string CallExpression::to_string() const {
    std::stringstream ss;
    ss << function->to_string();
    ss << "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        ss << arguments[i]->to_string();
        if (i < arguments.size() - 1) {
            ss << ", ";
        }
    }
    ss << ")";
    return ss.str();
}

// *** NEW IMPLEMENTATION ***
std::string WhileExpression::to_string() const {
    std::stringstream ss;
    ss << "while" << condition->to_string() << " ";
    ss << body->to_string();
    return ss.str();
}