#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# 1. Create a test file with our new if/else syntax.
# Note: The 'if' expression must be part of another statement (like 'let')
# because our top-level parser only understands 'let' and 'return' for now.
echo 'let result = if (1) {
    return 55;
} else {
    return 99;
};' > test.manit

# 2. Re-compile the manitc compiler
echo "--- Compiling manitc ---"
cd build
cmake .. > /dev/null
make
echo ""
echo "--- Compilation successful ---"
echo ""

# 3. Create a main.cpp to drive the compiler
# This assumes the compiler reads from standard input
cat << 'EOF' > ../src/main.cpp
#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

int main() {
    std::string line;
    std::string source_code;
    while (std::getline(std::cin, line)) {
        source_code += line + '\n';
    }

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
EOF

# This step is needed because I am not sure of the contents of your main.cpp
# We will recompile one last time with this standard main.cpp
make

# 4. Run the compiler on our test file
echo "--- Running manitc on test.manit ---"
./manitc < ../test.manit