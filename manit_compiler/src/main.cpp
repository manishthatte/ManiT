#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

int main() {
    // Test case for the new boolean type and control flow.
    std::string source_code = R"(
        if (10 < 20) {
            return 55;
        } else {
            return 99;
        }
    )";

    Lexer l(source_code);
    Parser p(l);
    auto program = p.parse_program();

    if (!program) {
        std::cerr << "Parsing failed." << std::endl;
        return 1;
    }
    
    CodeGenerator codegen;
    codegen.generate(*program);

    return 0;
}