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
