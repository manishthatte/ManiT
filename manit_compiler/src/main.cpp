#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename.manit>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << argv[1] << "'" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source_code = buffer.str();
    
    if (source_code.empty()) {
        std::cerr << "Warning: Input file '" << argv[1] << "' is empty." << std::endl;
    }

    Lexer l(source_code);
    Parser p(l);
    auto program = p.parse_program();

    if (!program) {
        std::cerr << "Error: Parsing failed. Please check the source code for syntax errors." << std::endl;
        return 1;
    }

    CodeGenerator codegen;
    codegen.generate(*program);

    return 0;
}