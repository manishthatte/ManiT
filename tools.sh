#!/bin/bash

# This script sets up the development environment for the ManiT language compiler.
# It installs Clang, LLVM development libraries, and CMake.

set -e # Exit immediately if a command exits with a non-zero status.

echo "--- Starting ManiT Compiler Development Environment Setup ---"

# Step 1: Install prerequisite Debian packages
echo "[1/1] Installing C++, Clang, LLVM, and CMake..."
sudo apt-get update
sudo apt-get install -y build-essential clang cmake lld llvm-dev

echo "--- Setup Complete! ---"
echo "Your environment is now configured to build the ManiT compiler."