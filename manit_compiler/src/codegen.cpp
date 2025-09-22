#include "codegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

CodeGenerator::CodeGenerator() {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("ManiT_Module", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

llvm::Value* CodeGenerator::generate_expression(const Expression& expr) {
    if (auto const* int_lit = dynamic_cast<const IntegerLiteral*>(&expr)) {
        return builder->getInt32(int_lit->value);
    }
    else if (auto const* ident = dynamic_cast<const Identifier*>(&expr)) {
        if (named_values.count(ident->value)) {
            // Load the value from the variable's memory location
            return builder->CreateLoad(builder->getInt32Ty(), named_values[ident->value], ident->value.c_str());
        }
        // Handle error: unknown variable
        return nullptr;
    }
    else if (auto const* prefix_expr = dynamic_cast<const PrefixExpression*>(&expr)) {
        llvm::Value* right = generate_expression(*prefix_expr->right);
        if (!right) return nullptr;

        if (prefix_expr->op == "-") {
            return builder->CreateNeg(right, "negtmp");
        }
        // Handle other prefix operators here...
        return nullptr;
    }
    else if (auto const* infix_expr = dynamic_cast<const InfixExpression*>(&expr)) {
        llvm::Value* left = generate_expression(*infix_expr->left);
        llvm::Value* right = generate_expression(*infix_expr->right);
        if (!left || !right) return nullptr;

        if (infix_expr->op == "+") {
            return builder->CreateAdd(left, right, "addtmp");
        } else if (infix_expr->op == "-") {
            return builder->CreateSub(left, right, "subtmp");
        } else if (infix_expr->op == "*") {
            return builder->CreateMul(left, right, "multmp");
        } else if (infix_expr->op == "/") {
            return builder->CreateSDiv(left, right, "divtmp");
        }
        // Handle other infix operators here...
        return nullptr;
    }
    // *** NEW CODEGEN FOR IF EXPRESSION ***
    else if (auto const* if_expr = dynamic_cast<const IfExpression*>(&expr)) {
        // 1. Codegen for the condition
        llvm::Value* cond_v = generate_expression(*if_expr->condition);
        if (!cond_v) return nullptr;

        // 2. Convert condition to a bool (i1) by comparing with 0
        cond_v = builder->CreateICmpNE(cond_v, builder->getInt32(0), "ifcond");

        llvm::Function* the_function = builder->GetInsertBlock()->getParent();

        // 3. Create basic blocks
        llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(*context, "then", the_function);
        llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(*context, "else");
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*context, "ifcont");

        // 4. Create the conditional branch
        builder->CreateCondBr(cond_v, then_bb, else_bb);

        // 5. Emit 'then' block
        builder->SetInsertPoint(then_bb);
        for (const auto& stmt : if_expr->consequence->statements) {
            generate_statement(*stmt);
        }
        // Branch to merge block if the 'then' block isn't already terminated (e.g., by a return)
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(merge_bb);
        }

        // 6. Emit 'else' block
        the_function->insert(the_function->end(), else_bb);
        builder->SetInsertPoint(else_bb);
        if (if_expr->alternative) {
            for (const auto& stmt : if_expr->alternative->statements) {
                generate_statement(*stmt);
            }
        }
        // Branch to merge block if the 'else' block isn't already terminated
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(merge_bb);
        }
        
        // 7. Emit merge block
        the_function->insert(the_function->end(), merge_bb);
        builder->SetInsertPoint(merge_bb);

        // For now, if expressions return a null/zero value.
        // In the future, a PHINode will be used here to make if a true expression.
        return llvm::Constant::getNullValue(builder->getInt32Ty());
    }

    return nullptr;
}

void CodeGenerator::generate_statement(const Statement& stmt) {
    if (auto const* let_stmt = dynamic_cast<const LetStatement*>(&stmt)) {
        llvm::Value* val = generate_expression(*let_stmt->value);
        if (val) {
            // Allocate memory for the variable on the stack
            llvm::AllocaInst* alloca = builder->CreateAlloca(builder->getInt32Ty(), nullptr, let_stmt->name->value.c_str()); // *** THIS LINE IS FIXED ***
            // Store the initial value
            builder->CreateStore(val, alloca);
            // Add the variable to our symbol table
            named_values[let_stmt->name->value] = alloca;
        }
    }
    else if (auto const* return_stmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        if (return_stmt->return_value) {
            llvm::Value* return_val = generate_expression(*return_stmt->return_value);
            if (return_val) {
                builder->CreateRet(return_val);
            }
        }
    }
}

void CodeGenerator::generate(const Program& program) {
    llvm::FunctionType* func_type = llvm::FunctionType::get(builder->getInt32Ty(), false);
    llvm::Function* main_func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "main", module.get());

    llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(*context, "entry", main_func);
    builder->SetInsertPoint(entry_block);

    for (const auto& stmt : program.statements) {
        generate_statement(*stmt);
    }
    
    // If the main function is not terminated by a return, add a default one.
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateRet(builder->getInt32(0));
    }
    
    // Verify and print
    llvm::verifyFunction(*main_func);
    module->print(llvm::outs(), nullptr);
}