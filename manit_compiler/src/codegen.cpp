#include "codegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

CodeGenerator::CodeGenerator() {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("ManiT_Module", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

// This is a helper function to create an Alloca instruction in the entry block.
llvm::AllocaInst* CodeGenerator::create_entry_block_alloca(llvm::Function* the_function, const std::string& var_name, llvm::Type* type) {
    llvm::IRBuilder<> tmp_builder(&the_function->getEntryBlock(), the_function->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(type, nullptr, var_name);
}

llvm::Value* CodeGenerator::generate_expression(const Expression& expr) {
    if (auto const* int_lit = dynamic_cast<const IntegerLiteral*>(&expr)) {
        return builder->getInt32(int_lit->value);
    }
    else if (auto const* bool_lit = dynamic_cast<const BooleanLiteral*>(&expr)) {
        // **NEW**: Generate a 1-bit integer (i1) for boolean values.
        return builder->getInt1(bool_lit->value);
    }
    else if (auto const* ident = dynamic_cast<const Identifier*>(&expr)) {
        if (named_values.count(ident->value)) {
            // A variable is a pointer (AllocaInst). To get its value, we must load it.
            // **FIX**: We need to know the type to load. This assumes i32 for now.
            // A proper type system will be needed to handle loading booleans vs integers.
            return builder->CreateLoad(builder->getInt32Ty(), named_values[ident->value], ident->value.c_str());
        }
        return nullptr; // Error: unknown variable
    }
    else if (auto const* assign_expr = dynamic_cast<const AssignmentExpression*>(&expr)) {
        if (named_values.find(assign_expr->name->value) == named_values.end()) {
            return nullptr; // Error: undeclared variable
        }
        llvm::AllocaInst* variable_alloca = named_values[assign_expr->name->value];

        llvm::Value* new_val = generate_expression(*assign_expr->value);
        if (!new_val) return nullptr;

        builder->CreateStore(new_val, variable_alloca);
        return new_val;
    }
    else if (auto const* prefix_expr = dynamic_cast<const PrefixExpression*>(&expr)) {
        llvm::Value* right = generate_expression(*prefix_expr->right);
        if (!right) return nullptr;
        if (prefix_expr->op == "-") {
            return builder->CreateNeg(right, "negtmp");
        }
        // Future: handle '!' operator for booleans here.
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
        // **UPDATED**: Comparison operators now return i1 directly.
        else if (infix_expr->op == "==") {
            return builder->CreateICmpEQ(left, right, "eqtmp");
        } else if (infix_expr->op == "!=") {
            return builder->CreateICmpNE(left, right, "neqtmp");
        } else if (infix_expr->op == "<") {
            return builder->CreateICmpSLT(left, right, "lttmp");
        } else if (infix_expr->op == "<=") {
            return builder->CreateICmpSLE(left, right, "letmp");
        } else if (infix_expr->op == ">") {
            return builder->CreateICmpSGT(left, right, "gttmp");
        } else if (infix_expr->op == ">=") {
            return builder->CreateICmpSGE(left, right, "getmp");
        }
        return nullptr;
    }
    else if (auto const* if_expr = dynamic_cast<const IfExpression*>(&expr)) {
        llvm::Value* cond_v = generate_expression(*if_expr->condition);
        if (!cond_v) return nullptr;

        // **UPDATED**: No longer need to compare to zero. The condition is already an i1.
        // cond_v = builder->CreateICmpNE(cond_v, builder->getInt32(0), "ifcond");

        llvm::Function* the_function = builder->GetInsertBlock()->getParent();

        llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(*context, "then", the_function);
        llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(*context, "else");
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*context, "ifcont");
        
        if (if_expr->alternative) {
            builder->CreateCondBr(cond_v, then_bb, else_bb);
        } else {
            builder->CreateCondBr(cond_v, then_bb, merge_bb);
        }

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
        
        llvm::Value* else_val = nullptr;
        llvm::BasicBlock* else_end_bb = else_bb;
        if (if_expr->alternative) {
            the_function->insert(the_function->end(), else_bb);
            builder->SetInsertPoint(else_bb);
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
            if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(merge_bb);
            else_end_bb = builder->GetInsertBlock();
        }
        
        the_function->insert(the_function->end(), merge_bb);
        builder->SetInsertPoint(merge_bb);

        if (then_val || else_val) {
            llvm::PHINode* pn = builder->CreatePHI(builder->getInt32Ty(), 2, "iftmp");
            pn->addIncoming(then_val ? then_val : builder->getInt32(0), then_end_bb);
            pn->addIncoming(else_val ? else_val : builder->getInt32(0), else_end_bb);
            return pn;
        }

        return builder->getInt32(0);
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
            llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, param_name, builder->getInt32Ty());
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

        llvm::Function* callee_func = module->getFunction(ident->value);
        if (!callee_func) return nullptr;
        if (callee_func->arg_size() != call_expr->arguments.size()) return nullptr;

        std::vector<llvm::Value*> args_v;
        for (const auto& arg : call_expr->arguments) {
            args_v.push_back(generate_expression(*arg));
            if (!args_v.back()) return nullptr;
        }
        
        return builder->CreateCall(callee_func, args_v, "calltmp");
    }
    else if (auto const* while_expr = dynamic_cast<const WhileExpression*>(&expr)) {
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();

        llvm::BasicBlock* loop_header_bb = llvm::BasicBlock::Create(*context, "loop_header", the_function);
        llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(*context, "loop_body", the_function);
        llvm::BasicBlock* loop_exit_bb = llvm::BasicBlock::Create(*context, "loop_exit", the_function);

        builder->CreateBr(loop_header_bb);

        builder->SetInsertPoint(loop_header_bb);
        llvm::Value* cond_v = generate_expression(*while_expr->condition);
        if (!cond_v) return nullptr;
        
        // **UPDATED**: No longer need to compare to zero. The condition is already an i1.
        // cond_v = builder->CreateICmpNE(cond_v, builder->getInt32(0), "loopcond");
        
        builder->CreateCondBr(cond_v, loop_body_bb, loop_exit_bb);

        builder->SetInsertPoint(loop_body_bb);
        for (const auto& stmt : while_expr->body->statements) {
            generate_statement(*stmt);
        }
        
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(loop_header_bb);
        }

        builder->SetInsertPoint(loop_exit_bb);
        return llvm::Constant::getNullValue(builder->getInt32Ty());
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
            llvm::Function* the_function = builder->GetInsertBlock()->getParent();
            llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, let_stmt->name->value, val->getType());
            builder->CreateStore(val, alloca);
            named_values[let_stmt->name->value] = alloca;
        }
    }
    else if (auto const* var_stmt = dynamic_cast<const VarStatement*>(&stmt)) {
        llvm::Value* val = generate_expression(*var_stmt->value);
        if (val) {
            llvm::Function* the_function = builder->GetInsertBlock()->getParent();
            llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, var_stmt->name->value, val->getType());
            builder->CreateStore(val, alloca);
            named_values[var_stmt->name->value] = alloca;
        }
    }
    else if (auto const* return_stmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        if (return_stmt->return_value) {
            llvm::Value* return_val = generate_expression(*return_stmt->return_value);
            if (return_val) {
                builder->CreateRet(return_val);
            }
        } else {
            builder->CreateRetVoid();
        }
    }
    else if (auto const* expr_stmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        generate_expression(*expr_stmt->expression);
    }
}

void CodeGenerator::generate(const Program& program) {
    llvm::FunctionType* func_type = llvm::FunctionType::get(builder->getInt32Ty(), false);
    llvm::Function* main_func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "main", module.get());
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", main_func);
    builder->SetInsertPoint(entry);

    for (const auto& stmt : program.statements) {
        generate_statement(*stmt);
    }

    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateRet(builder->getInt32(0));
    }
    
    bool user_defined_main = false;
    for (const auto& stmt : program.statements) {
        if (auto const* let_stmt = dynamic_cast<const LetStatement*>(stmt.get())) {
            if (let_stmt->name->value == "main") {
                user_defined_main = true;
                break;
            }
        }
    }

    if (user_defined_main) {
        main_func->eraseFromParent();
    }

    llvm::verifyModule(*module, &llvm::errs());
    module->print(llvm::outs(), nullptr);
}