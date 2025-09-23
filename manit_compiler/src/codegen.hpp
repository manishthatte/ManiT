#ifndef MANIT_CODEGEN_HPP
#define MANIT_CODEGEN_HPP

#include "ast.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <map>
#include <string>

// Forward declarations for LLVM classes
namespace llvm {
    class AllocaInst;
    class Function;
    class Type;
    class StructType;
}

class CodeGenerator {
public:
    CodeGenerator();
    void generate(const Program& program);

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // Symbol table for variable allocations
    std::map<std::string, llvm::AllocaInst*> named_values;
    // Type table for struct definitions
    std::map<std::string, llvm::StructType*> struct_types;

    // Visitor methods
    llvm::Value* generate_expression(const Expression& expr);
    void generate_statement(const Statement& stmt);

    // Helper methods
    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* the_function, const std::string& var_name, llvm::Type* type);
};

#endif // MANIT_CODEGEN_HPP