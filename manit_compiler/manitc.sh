#!/bin/bash
mkdir -p build
clang++ -std=c++17 src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp src/ast.cpp $(llvm-config --cxxflags --ldflags --system-libs --libs core) -o build/manitc
echo "Running ManiT program..."
./build/manitc | lli
result=$?
echo "--------------------------------"
echo "ManiT program exited with code: $result"
if [ "$result" -eq 22 ]; then
    echo "SUCCESS: The result is 22 as expected."
else
    echo "FAILURE: The result was not 22."
fi
