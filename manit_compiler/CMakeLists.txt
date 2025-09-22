cmake_minimum_required(VERSION 3.10)
project(manit_compiler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(manitc 
    src/main.cpp 
    src/lexer.cpp
    src/parser.cpp
    src/ast.cpp
    src/codegen.cpp
)

# Find and link LLVM libraries
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# This is the corrected, minimal set of components needed for IR generation.
llvm_map_components_to_libnames(LLVM_LIBS
    Support
    Core
)

target_link_libraries(manitc PRIVATE ${LLVM_LIBS})