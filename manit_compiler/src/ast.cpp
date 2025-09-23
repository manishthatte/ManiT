#include "ast.hpp"
#include <sstream>

std::string Program::to_string() const {
    std::stringstream ss;
    for (const auto& s : statements) {
        ss << s->to_string();
    }
    return ss.str();
}

std::string Identifier::to_string() const { return value; }
std::string IntegerLiteral::to_string() const { return token.literal; }
std::string BooleanLiteral::to_string() const { return token.literal; }

std::string ArrayLiteral::to_string() const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        ss << elements[i]->to_string() << (i < elements.size() - 1 ? ", " : "");
    }
    ss << "]";
    return ss.str();
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

std::string AssignmentExpression::to_string() const {
    std::stringstream ss;
    ss << "(" << name->to_string() << " = " << value->to_string() << ")";
    return ss.str();
}

std::string IndexExpression::to_string() const {
    std::stringstream ss;
    ss << "(" << left->to_string() << "[" << index->to_string() << "])";
    return ss.str();
}

std::string LetStatement::to_string() const {
    std::stringstream ss;
    ss << token.literal << " " << name->to_string();
    if (type) {
        ss << ": " << type->to_string();
    }
    ss << " = ";
    if (value) {
        ss << value->to_string();
    }
    ss << ";";
    return ss.str();
}

std::string VarStatement::to_string() const {
    std::stringstream ss;
    ss << token.literal << " " << name->to_string();
    if (type) {
        ss << ": " << type->to_string();
    }
    ss << " = ";
    if (value) {
        ss << value->to_string();
    }
    ss << ";";
    return ss.str();
}

std::string StructDefinitionStatement::to_string() const {
    std::stringstream ss;
    ss << token.literal << " " << name->to_string() << " {";
    for (size_t i = 0; i < fields.size(); ++i) {
        ss << " " << fields[i].name->to_string() << ": " << fields[i].type->to_string();
        if (i < fields.size() - 1) {
            ss << ",";
        }
    }
    ss << " };";
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
    if (expression) {
        return expression->to_string() + ";";
    }
    return ";";
}

std::string BlockStatement::to_string() const {
    std::stringstream ss;
    for (const auto& s : statements) {
        ss << s->to_string();
    }
    return ss.str();
}

std::string IfExpression::to_string() const {
    std::stringstream ss;
    ss << "if" << condition->to_string() << " " << consequence->to_string();
    if (alternative) {
        ss << "else " << alternative->to_string();
    }
    return ss.str();
}

std::string FunctionLiteral::to_string() const {
    std::stringstream ss;
    ss << token.literal << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        ss << parameters[i]->to_string() << (i < parameters.size() - 1 ? ", " : "");
    }
    ss << ") " << body->to_string();
    return ss.str();
}

std::string CallExpression::to_string() const {
    std::stringstream ss;
    ss << function->to_string() << "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        ss << arguments[i]->to_string() << (i < arguments.size() - 1 ? ", " : "");
    }
    ss << ")";
    return ss.str();
}

std::string WhileExpression::to_string() const {
    std::stringstream ss;
    ss << "while(" << condition->to_string() << ") {" << body->to_string() << "}";
    return ss.str();
}

std::string ForLoopExpression::to_string() const {
    std::stringstream ss;
    ss << "for(";
    std::string init_str = initializer ? initializer->to_string() : "";
    if (!init_str.empty() && init_str.back() == ';') {
        init_str.pop_back();
    }
    ss << init_str << "; ";
    if (condition) {
        ss << condition->to_string();
    }
    ss << "; ";
    if (increment) {
        ss << increment->to_string();
    }
    ss << ") { " << body->to_string() << " }";
    return ss.str();
}