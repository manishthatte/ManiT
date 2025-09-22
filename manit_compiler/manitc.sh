#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# 1. Create a test file that uses comparison operators
echo 'let main = fn() {
    let a = 10;
    let b = 20;
    if (a < b) {
        return 1;
    } else {
        return 0;
    }
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