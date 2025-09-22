#!/bin/bash
# This script creates the initial project structure for the ManiT compiler.

set -e

echo "--- Creating the ManiT compiler project structure ---"

# Create the main project directory
if [ ! -d "manit_compiler" ]; then
    echo "[1/3] Creating project directory 'manit_compiler'..."
    mkdir manit_compiler
else
    echo "[1/3] Directory 'manit_compiler' already exists. Skipping creation."
fi
cd manit_compiler

# Create the source directory
if [ ! -d "src" ]; then
    echo "[2/3] Creating source directory 'src'..."
    mkdir src
else
    echo "[2/3] Directory 'src' already exists. Skipping creation."
fi

# Create initial empty files
echo "[3/3] Creating initial source and build files..."
touch src/main.cpp
touch src/token.hpp
touch CMakeLists.txt

echo "--- Project structure created successfully! ---"
echo "Layout is:"
echo "manit_compiler/"
echo "├── src/"
echo "│   ├── main.cpp"
echo "│   └── token.hpp"
echo "└── CMakeLists.txt"