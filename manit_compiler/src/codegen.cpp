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
    else if (auto const* if_expr = dynamic_cast<const IfExpression*>(&expr)) {
        llvm::Value* cond_v = generate_expression(*if_expr->condition);
        if (!cond_v) return nullptr;

        cond_v = builder->CreateICmpNE(cond_v, builder->getInt32(0), "ifcond");

        llvm::Function* the_function = builder->GetInsertBlock()->getParent();

        llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(*context, "then", the_function);
        llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(*context, "else");
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*context, "ifcont");

        builder->CreateCondBr(cond_v, then_bb, else_bb);

        builder->SetInsertPoint(then_bb);
        llvm::Value* then_val = nullptr;
        if (!if_expr->consequence->statements.empty()) {
            if (auto* last_stmt_as_expr = dynamic_cast<ExpressionStatement*>(if_expr->consequence->statements.back().get())) {
                for (size_t i = 0; i < if_expr->consequence->statements.size() - 1; ++i) {
                    generate_statement(*if_expr->consequence->statements[i]);
                }
                then_val = generate_expression(*last_stmt_as_expr->expression);
            } else {
                for (const auto& stmt : if_expr->consequence->statements) {
                    generate_statement(*stmt);
                }
            }
        }
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(merge_bb);
        llvm::BasicBlock* then_end_bb = builder->GetInsertBlock();

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
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(merge_bb);
        llvm::BasicBlock* else_end_bb = builder->GetInsertBlock();
        
        the_function->insert(the_function->end(), merge_bb);
        builder->SetInsertPoint(merge_bb);
        llvm::PHINode* pn = builder->CreatePHI(builder->getInt32Ty(), 2, "iftmp");

        pn->addIncoming(then_val ? then_val : builder->getInt32(0), then_end_bb);
        pn->addIncoming(else_val ? else_val : builder->getInt32(0), else_end_bb);
        return pn;
    }
    else if (auto const* func_lit = dynamic_cast<const FunctionLiteral*>(&expr)) {
        llvm::BasicBlock* original_block = builder->GetInsertBlock();
        auto old_named_values = named_values;

        std::vector<llvm::Type*> param_types(func_lit->parameters.size(), builder->getInt32Ty());
        llvm::FunctionType* func_type = llvm::FunctionType::get(builder->getInt32Ty(), param_types, false);
        llvm::Function* the_function = llvm::Function::Create(func_type, llvm::Function::InternalLinkage, "user_fn", module.get());

        llvm::BasicBlock* func_entry_block = llvm::BasicBlock::Create(*context, "entry", the_function);
        builder->SetInsertPoint(func_entry_block);

        named_values.clear();
        size_t i = 0;
        for (auto& arg : the_function->args()) {
            const std::string& param_name = func_lit->parameters[i++]->value;
            arg.setName(param_name);
            llvm::AllocaInst* alloca = builder->CreateAlloca(builder->getInt32Ty(), nullptr, param_name);
            builder->CreateStore(&arg, alloca);
            named_values[param_name] = alloca;
        }

        for (const auto& stmt : func_lit->body->statements) {
            generate_statement(*stmt);
        }

        if (!builder->GetInsertBlock()->getTerminator()) {
             builder->CreateRet(builder->getInt32(0));
        }

        llvm::verifyFunction(*the_function);

        builder->SetInsertPoint(original_block);
        named_values = old_named_values;

        return the_function;
    }
    else if (auto const* call_expr = dynamic_cast<const CallExpression*>(&expr)) {
        auto const* ident = dynamic_cast<const Identifier*>(call_expr->function.get());
        if (!ident) return nullptr;

        // *** THIS IS THE FIX ***
        // Look up the function directly in the module by its name.
        llvm::Function* callee_func = module->getFunction(ident->value);
        if (!callee_func) {
            return nullptr; // Error: Function not found.
        }

        if (callee_func->arg_size() != call_expr->arguments.size()) {
            return nullptr; // Error: incorrect number of arguments
        }

        std::vector<llvm::Value*> args_v;
        for (const auto& arg : call_expr->arguments) {
            args_v.push_back(generate_expression(*arg));
            if (!args_v.back()) return nullptr;
        }
        
        return builder->CreateCall(callee_func, args_v, "calltmp");
    }

    return nullptr;
}

void CodeGenerator::generate_statement(const Statement& stmt) {
    if (auto const* let_stmt = dynamic_cast<const LetStatement*>(&stmt)) {
        llvm::Value* val = generate_expression(*let_stmt->value);
        if (val) {
            if (auto* func = llvm::dyn_cast<llvm::Function>(val)) {
                func->setName(let_stmt->name->value);
                return;
            }
            llvm::AllocaInst* alloca = builder->CreateAlloca(val->getType(), nullptr, let_stmt->name->value.c_str());
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
    else if (auto const* expr_stmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        generate_expression(*expr_stmt->expression);
    }
}

void CodeGenerator::generate(const Program& program) {
    for (const auto& stmt : program.statements) {
        generate_statement(*stmt);
    }

    llvm::Function* main_func = module->getFunction("main");
    if (!main_func) {
        llvm::FunctionType* func_type = llvm::FunctionType::get(builder->getInt32Ty(), false);
        main_func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "main", module.get());
        llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(*context, "entry", main_func);
        builder->SetInsertPoint(entry_block);
        builder->CreateRet(builder->getInt32(0));
    }
    
    // Set main function to have external linkage so it can be called
    main_func->setLinkage(llvm::Function::ExternalLinkage);
    
    for (auto& func : *module) {
        if (!func.isDeclaration()) {
            llvm::verifyFunction(func);
        }
    }

    module->print(llvm::outs(), nullptr);
}