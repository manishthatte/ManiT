#!/bin/bash
# NOTE: set -e removed to allow lli to return our program's exit code.

# Create a build directory
mkdir -p build

# Compile the manitc compiler
clang++ -std=c++17 src/main.cpp src/lexer.cpp src/parser.cpp src/codegen.cpp src/ast.cpp \
    $(llvm-config --cxxflags --ldflags --system-libs --libs core) \
    -o build/manitc

# Run the compiler, pipe its output to the LLVM interpreter
echo "Running ManiT program..."
./build/manitc | lli

# Check the exit code of the executed program
result=$?
echo "--------------------------------"
echo "ManiT program exited with code: $result"

if [ "$result" -eq 45 ]; then
    echo "SUCCESS: The result is 45 as expected."
else
    echo "FAILURE: The result was not 45."
fi