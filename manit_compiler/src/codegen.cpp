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
            return builder->CreateLoad(builder->getInt32Ty(), named_values[ident->value], ident->value.c_str());
        }
        return nullptr;
    }
    else if (auto const* prefix_expr = dynamic_cast<const PrefixExpression*>(&expr)) {
        llvm::Value* right = generate_expression(*prefix_expr->right);
        if (!right) return nullptr;
        if (prefix_expr->op == "-") {
            return builder->CreateNeg(right, "negtmp");
        }
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
        return nullptr;
    }
    // *** MAJOR REFACTOR FOR IF EXPRESSION WITH PHINODE ***
    else if (auto const* if_expr = dynamic_cast<const IfExpression*>(&expr)) {
        llvm::Value* cond_v = generate_expression(*if_expr->condition);
        if (!cond_v) return nullptr;

        cond_v = builder->CreateICmpNE(cond_v, builder->getInt32(0), "ifcond");

        llvm::Function* the_function = builder->GetInsertBlock()->getParent();

        llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(*context, "then", the_function);
        llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(*context, "else");
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*context, "ifcont");

        builder->CreateCondBr(cond_v, then_bb, else_bb);

        // --- Emit 'then' block and get its value ---
        builder->SetInsertPoint(then_bb);
        llvm::Value* then_val = nullptr;
        if (!if_expr->consequence->statements.empty()) {
            // The value of the block is the value of its last expression statement.
            if (auto* last_stmt_as_expr = dynamic_cast<ExpressionStatement*>(if_expr->consequence->statements.back().get())) {
                 // Generate all statements *before* the last one.
                for (size_t i = 0; i < if_expr->consequence->statements.size() - 1; ++i) {
                    generate_statement(*if_expr->consequence->statements[i]);
                }
                // The value of the block is the generated value of the final expression.
                then_val = generate_expression(*last_stmt_as_expr->expression);
            } else {
                // The block ends in a non-expression (e.g. `let`), so it has no value.
                for (const auto& stmt : if_expr->consequence->statements) {
                    generate_statement(*stmt);
                }
            }
        }
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(merge_bb);
        }
        llvm::BasicBlock* then_end_bb = builder->GetInsertBlock();

        // --- Emit 'else' block and get its value ---
        the_function->insert(the_function->end(), else_bb);
        builder->SetInsertPoint(else_bb);
        llvm::Value* else_val = nullptr;
        if (if_expr->alternative) {
             if (!if_expr->alternative->statements.empty()) {
                if (auto* last_stmt_as_expr = dynamic_cast<ExpressionStatement*>(if_expr->alternative->statements.back().get())) {
                    for (size_t i = 0; i < if_expr->alternative->statements.size() - 1; ++i) {
                        generate_statement(*if_expr->alternative->statements[i]);
                    }
                    else_val = generate_expression(*last_stmt_as_expr->expression);
                } else {
                    for (const auto& stmt : if_expr->alternative->statements) {
                        generate_statement(*stmt);
                    }
                }
            }
        }
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(merge_bb);
        }
        llvm::BasicBlock* else_end_bb = builder->GetInsertBlock();
        
        // --- Emit merge block and create PHI node ---
        the_function->insert(the_function->end(), merge_bb);
        builder->SetInsertPoint(merge_bb);
        llvm::PHINode* pn = builder->CreatePHI(builder->getInt32Ty(), 2, "iftmp");

        // Use a default value of 0 if a block doesn't yield a value.
        pn->addIncoming(then_val ? then_val : builder->getInt32(0), then_end_bb);
        pn->addIncoming(else_val ? else_val : builder->getInt32(0), else_end_bb);
        return pn;
    }

    return nullptr;
}

void CodeGenerator::generate_statement(const Statement& stmt) {
    if (auto const* let_stmt = dynamic_cast<const LetStatement*>(&stmt)) {
        llvm::Value* val = generate_expression(*let_stmt->value);
        if (val) {
            llvm::AllocaInst* alloca = builder->CreateAlloca(builder->getInt32Ty(), nullptr, let_stmt->name->value.c_str());
            builder->CreateStore(val, alloca);
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
    // *** NEW: HANDLE EXPRESSION STATEMENTS ***
    else if (auto const* expr_stmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        // Generate the expression, but discard its value.
        generate_expression(*expr_stmt->expression);
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
    
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateRet(builder->getInt32(0));
    }
    
    llvm::verifyFunction(*main_func);
    module->print(llvm::outs(), nullptr);
}