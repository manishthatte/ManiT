#!/bin/bash

# Do NOT exit on error, so we can see the log from the crash
# set -e

# 1. Create a test file with a while loop.
echo 'let main = fn() {
    while (1) {
        // empty body
    }
    return 0; // This part is unreachable
};' > ../test.manit

# 2. Re-compile the manitc compiler
echo "--- Compiling manitc ---"
cd build
cmake .. > /dev/null
make
echo ""
echo "--- Compilation successful ---"
echo ""

# 3. Run the compiler on our test file, redirecting stderr to a log file.
# The '|| true' is important to prevent the script from stopping on segfault.
echo "--- Running manitc on test.manit (logging to manitc.log) ---"
./manitc < ../test.manit 2> manitc.log || true

echo ""
echo "--- Compiler finished ---"
echo ""

# 4. Display the results.
echo "--- LLVM IR Output (stdout) ---"
# No stdout is expected if it segfaults before printing, but we check anyway.
# The IR will be in manitc.log as of the last change, but good practice to separate.

echo ""
echo "--- Diagnostic Log (manitc.log) ---"
cat manitc.log
echo "-----------------------------------"