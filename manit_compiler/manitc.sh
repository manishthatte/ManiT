#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# 1. Create a test file with a user-defined 'add' and 'main' function.
echo 'let add = fn(a, b) {
    return a + b;
};

let main = fn() {
    let result = add(10, 20);
    return result;
};' > test.manit

# 2. Re-compile the manitc compiler
echo "--- Compiling manitc ---"
cd build
cmake .. > /dev/null
make
echo ""
echo "--- Compilation successful ---"
echo ""

# 3. Run the compiler on our test file
echo "--- Running manitc on test.manit ---"
./manitc < ../test.manit