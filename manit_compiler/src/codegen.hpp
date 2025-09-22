#ifndef MANIT_CODEGEN_HPP
#define MANIT_CODEGEN_HPP

#include "ast.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <map>

class CodeGenerator {
public:
    CodeGenerator();
    void generate(const Program& program);

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    
    // Corrected line: Changed std.unique_ptr to std::unique_ptr
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    std::map<std::string, llvm::Value*> named_values;

    llvm::Value* generate_expression(const Expression& expr);
    void generate_statement(const Statement& stmt);
};

#endif // MANIT_CODEGEN_HPP