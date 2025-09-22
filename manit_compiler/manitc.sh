#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# 1. Create a simple, terminating test file.
echo 'let main = fn() {
    let x = 10;
    if (x == 10) {
        return 42;
    } else {
        return 0;
    }
};' > ../test.manit

# 2. Re-compile the manitc compiler
echo "--- Compiling manitc ---"
mkdir -p build
cd build
cmake .. > /dev/null
make
echo ""
echo "--- Compilation successful ---"
echo ""

# 3. Run the compiler on our test file
echo "--- Running manitc on test.manit ---"
./manitc < ../test.manit

echo ""
echo "--- Compiler finished ---"