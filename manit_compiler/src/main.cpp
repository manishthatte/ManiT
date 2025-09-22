#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

int main() {
    // Test case for mutable variables and while loops.
    std::string source_code = R"(
        var i = 0;
        while (i < 5) {
            i = i + 1;
        }
        return i;
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