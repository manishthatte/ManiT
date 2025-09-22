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

// *** NEW IMPLEMENTATION ***
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