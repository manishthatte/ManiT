#!/bin/bash

# Restore the correct, non-debugging test case in main.cpp
cat <<'EOF' > src/main.cpp
#include <iostream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

int main() {
    // Test case for the new for loop. Expected result: 45.
    std::string source_code = R"(
        var total = 0;
        for (var i = 0; i < 10; i = i + 1) {
            total = total + i;
        }
        return total;
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
EOF

echo "src/main.cpp restored. Now compiling and running for the final test..."
echo "------------------------------------------------------------------"

# Run the build script
./manitc.sh