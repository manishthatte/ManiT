#include "codegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

CodeGenerator::CodeGenerator() {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("ManiT_Module", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

llvm::AllocaInst* CodeGenerator::create_entry_block_alloca(llvm::Function* the_function, const std::string& var_name, llvm::Type* type) {
    llvm::IRBuilder<> tmp_builder(&the_function->getEntryBlock(), the_function->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(type, nullptr, var_name);
}

void CodeGenerator::generate_statement(const Statement& stmt) {
    if (auto const* let_stmt = dynamic_cast<const LetStatement*>(&stmt)) {
        llvm::Value* val = generate_expression(*let_stmt->value);
        if (!val) return;
        
        // --- BUG FIX: Corrected control flow from if/if/else to if/else if/else ---
        if (auto* func = llvm::dyn_cast<llvm::Function>(val)) {
            func->setName(let_stmt->name->value);
        }
        else if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(val)) {
            alloca->setName(let_stmt->name->value);
            named_values[let_stmt->name->value] = alloca;
        } else {
            llvm::Function* the_function = builder->GetInsertBlock()->getParent();
            llvm::AllocaInst* scalar_alloca = create_entry_block_alloca(the_function, let_stmt->name->value, val->getType());
            builder->CreateStore(val, scalar_alloca);
            named_values[let_stmt->name->value] = scalar_alloca;
        }
    }
    else if (auto const* var_stmt = dynamic_cast<const VarStatement*>(&stmt)) {
        llvm::Value* val = generate_expression(*var_stmt->value);
        if (!val) return;
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, var_stmt->name->value, val->getType());
        builder->CreateStore(val, alloca);
        named_values[var_stmt->name->value] = alloca;
    }
    else if (auto const* struct_def_stmt = dynamic_cast<const StructDefinitionStatement*>(&stmt)) {
        const std::string& struct_name = struct_def_stmt->name->value;
        if (struct_types.count(struct_name)) { return; }
        llvm::StructType* struct_type = llvm::StructType::create(*context, struct_name);
        struct_types[struct_name] = struct_type;
        std::vector<llvm::Type*> field_types;
        for (const auto& field : struct_def_stmt->fields) {
            if (field.type->value == "i32") {
                field_types.push_back(builder->getInt32Ty());
            }
        }
        struct_type->setBody(field_types);
    }
    else if (auto const* return_stmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        if (return_stmt->return_value) {
            llvm::Value* return_val = generate_expression(*return_stmt->return_value);
            if (return_val) builder->CreateRet(return_val);
        } else {
            builder->CreateRetVoid();
        }
    }
    else if (auto const* expr_stmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        generate_expression(*expr_stmt->expression);
    }
}

llvm::Value* CodeGenerator::generate_expression(const Expression& expr) {
    if (auto const* int_lit = dynamic_cast<const IntegerLiteral*>(&expr)) { return builder->getInt32(int_lit->value); }
    else if (auto const* bool_lit = dynamic_cast<const BooleanLiteral*>(&expr)) { return builder->getInt1(bool_lit->value); }
    else if (auto const* array_lit = dynamic_cast<const ArrayLiteral*>(&expr)) {
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::Type* element_type = builder->getInt32Ty();
        uint64_t array_size = array_lit->elements.size();
        llvm::ArrayType* array_type = llvm::ArrayType::get(element_type, array_size);
        llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, "array_lit", array_type);
        
        std::vector<llvm::Value*> element_values;
        element_values.reserve(array_size);
        for (const auto& elem_expr : array_lit->elements) {
            llvm::Value* val = generate_expression(*elem_expr);
            if (!val) return nullptr;
            element_values.push_back(val);
        }

        for (uint64_t i = 0; i < array_size; ++i) {
            std::vector<llvm::Value*> indices = { builder->getInt32(0), builder->getInt32(i) };
            llvm::Value* element_ptr = builder->CreateGEP(array_type, alloca, indices, "element_ptr");
            builder->CreateStore(element_values[i], element_ptr);
        }
        return alloca;
    }
    else if (auto const* index_expr = dynamic_cast<const IndexExpression*>(&expr)) {
        llvm::Value* array_ptr = generate_expression(*index_expr->left);
        if (!array_ptr) return nullptr;
        llvm::Value* index_val = generate_expression(*index_expr->index);
        if (!index_val) return nullptr;
        
        llvm::Type* array_type = llvm::cast<llvm::AllocaInst>(array_ptr)->getAllocatedType();
        std::vector<llvm::Value*> indices = { builder->getInt32(0), index_val };
        llvm::Value* element_ptr = builder->CreateGEP(array_type, array_ptr, indices, "element_ptr");
        llvm::Type* element_type = llvm::cast<llvm::ArrayType>(array_type)->getElementType();
        return builder->CreateLoad(element_type, element_ptr, "array_idx_val");
    }
    else if (auto const* ident = dynamic_cast<const Identifier*>(&expr)) {
        if (named_values.count(ident->value)) {
            llvm::AllocaInst* alloca = named_values[ident->value];
            llvm::Type* var_type = alloca->getAllocatedType();
            if (var_type->isArrayTy()) { return alloca; }
            else { return builder->CreateLoad(var_type, alloca, ident->value.c_str()); }
        }
        return nullptr;
    }
    else if (auto const* assign_expr = dynamic_cast<const AssignmentExpression*>(&expr)) {
        llvm::Value* new_val = generate_expression(*assign_expr->value);
        if (!new_val) return nullptr;
        if (named_values.find(assign_expr->name->value) == named_values.end()) return nullptr;
        llvm::AllocaInst* variable_alloca = named_values[assign_expr->name->value];
        builder->CreateStore(new_val, variable_alloca);
        return new_val;
    }
    else if (auto const* prefix_expr = dynamic_cast<const PrefixExpression*>(&expr)) {
        llvm::Value* right = generate_expression(*prefix_expr->right);
        if (!right) return nullptr;
        if (prefix_expr->op == "-") { return builder->CreateNeg(right, "negtmp"); }
        return nullptr;
    }
    else if (auto const* infix_expr = dynamic_cast<const InfixExpression*>(&expr)) {
        llvm::Value* left = generate_expression(*infix_expr->left);
        llvm::Value* right = generate_expression(*infix_expr->right);
        if (!left || !right) return nullptr;
        if (infix_expr->op == "+") { return builder->CreateAdd(left, right, "addtmp"); }
        else if (infix_expr->op == "-") { return builder->CreateSub(left, right, "subtmp"); }
        else if (infix_expr->op == "*") { return builder->CreateMul(left, right, "multmp"); }
        else if (infix_expr->op == "/") { return builder->CreateSDiv(left, right, "divtmp"); }
        else if (infix_expr->op == "==") { return builder->CreateICmpEQ(left, right, "eqtmp"); }
        else if (infix_expr->op == "!=") { return builder->CreateICmpNE(left, right, "neqtmp"); }
        else if (infix_expr->op == "<") { return builder->CreateICmpSLT(left, right, "lttmp"); }
        else if (infix_expr->op == "<=") { return builder->CreateICmpSLE(left, right, "letmp"); }
        else if (infix_expr->op == ">") { return builder->CreateICmpSGT(left, right, "gttmp"); }
        else if (infix_expr->op == ">=") { return builder->CreateICmpSGE(left, right, "getmp"); }
        return nullptr;
    }
    else if (auto const* if_expr = dynamic_cast<const IfExpression*>(&expr)) {
        llvm::Value* cond_v = generate_expression(*if_expr->condition); if (!cond_v) return nullptr;
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(*context, "then", the_function);
        llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(*context, "else");
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*context, "ifcont");
        if (if_expr->alternative) { builder->CreateCondBr(cond_v, then_bb, else_bb); } else { builder->CreateCondBr(cond_v, then_bb, merge_bb); }
        builder->SetInsertPoint(then_bb);
        llvm::Value* then_val = nullptr;
        if (!if_expr->consequence->statements.empty()) { if (auto* last_stmt_as_expr = dynamic_cast<ExpressionStatement*>(if_expr->consequence->statements.back().get())) { for (size_t i = 0; i < if_expr->consequence->statements.size() - 1; ++i) generate_statement(*if_expr->consequence->statements[i]); then_val = generate_expression(*last_stmt_as_expr->expression); } else { for (const auto& stmt : if_expr->consequence->statements) generate_statement(*stmt); } }
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(merge_bb);
        llvm::BasicBlock* then_end_bb = builder->GetInsertBlock();
        llvm::Value* else_val = nullptr;
        llvm::BasicBlock* else_end_bb = else_bb;
        if (if_expr->alternative) {
            the_function->insert(the_function->end(), else_bb);
            builder->SetInsertPoint(else_bb);
            if (!if_expr->alternative->statements.empty()) { if (auto* last_stmt_as_expr = dynamic_cast<ExpressionStatement*>(if_expr->alternative->statements.back().get())) { for (size_t i = 0; i < if_expr->alternative->statements.size() - 1; ++i) generate_statement(*if_expr->alternative->statements[i]); else_val = generate_expression(*last_stmt_as_expr->expression); } else { for (const auto& stmt : if_expr->alternative->statements) generate_statement(*stmt); } }
            if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(merge_bb);
            else_end_bb = builder->GetInsertBlock();
        }
        the_function->insert(the_function->end(), merge_bb);
        builder->SetInsertPoint(merge_bb);
        if (then_val || else_val) { llvm::PHINode* pn = builder->CreatePHI(builder->getInt32Ty(), 2, "iftmp"); pn->addIncoming(then_val ? then_val : builder->getInt32(0), then_end_bb); pn->addIncoming(else_val ? else_val : builder->getInt32(0), else_end_bb); return pn; }
        return builder->getInt32(0);
    }
    else if (auto const* func_lit = dynamic_cast<const FunctionLiteral*>(&expr)) {
        llvm::BasicBlock* original_block = builder->GetInsertBlock(); auto old_named_values = named_values;
        std::vector<llvm::Type*> param_types(func_lit->parameters.size(), builder->getInt32Ty());
        llvm::FunctionType* func_type = llvm::FunctionType::get(builder->getInt32Ty(), param_types, false);
        llvm::Function* the_function = llvm::Function::Create(func_type, llvm::Function::InternalLinkage, "user_fn", module.get());
        llvm::BasicBlock* func_entry_block = llvm::BasicBlock::Create(*context, "entry", the_function); builder->SetInsertPoint(func_entry_block);
        named_values.clear(); size_t i = 0;
        for (auto& arg : the_function->args()) { const std::string& param_name = func_lit->parameters[i++]->value; arg.setName(param_name); llvm::AllocaInst* alloca = create_entry_block_alloca(the_function, param_name, builder->getInt32Ty()); builder->CreateStore(&arg, alloca); named_values[param_name] = alloca; }
        for (const auto& stmt : func_lit->body->statements) generate_statement(*stmt);
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateRet(builder->getInt32(0));
        llvm::verifyFunction(*the_function); builder->SetInsertPoint(original_block); named_values = old_named_values; return the_function;
    }
    else if (auto const* call_expr = dynamic_cast<const CallExpression*>(&expr)) {
        auto const* ident = dynamic_cast<const Identifier*>(call_expr->function.get()); if (!ident) return nullptr;
        llvm::Function* callee_func = module->getFunction(ident->value); if (!callee_func) return nullptr; if (callee_func->arg_size() != call_expr->arguments.size()) return nullptr;
        std::vector<llvm::Value*> args_v;
        for (const auto& arg : call_expr->arguments) { args_v.push_back(generate_expression(*arg)); if (!args_v.back()) return nullptr; }
        return builder->CreateCall(callee_func, args_v, "calltmp");
    }
    else if (auto const* while_expr = dynamic_cast<const WhileExpression*>(&expr)) {
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* loop_header_bb = llvm::BasicBlock::Create(*context, "loop_header", the_function);
        llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(*context, "loop_body", the_function);
        llvm::BasicBlock* loop_exit_bb = llvm::BasicBlock::Create(*context, "loop_exit", the_function);
        builder->CreateBr(loop_header_bb); builder->SetInsertPoint(loop_header_bb);
        llvm::Value* cond_v = generate_expression(*while_expr->condition); if (!cond_v) return nullptr;
        builder->CreateCondBr(cond_v, loop_body_bb, loop_exit_bb);
        builder->SetInsertPoint(loop_body_bb); for (const auto& stmt : while_expr->body->statements) generate_statement(*stmt);
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(loop_header_bb);
        builder->SetInsertPoint(loop_exit_bb); return llvm::Constant::getNullValue(builder->getInt32Ty());
    }
    else if (auto const* for_expr = dynamic_cast<const ForLoopExpression*>(&expr)) {
        auto old_named_values = named_values;
        if (for_expr->initializer) generate_statement(*for_expr->initializer);
        llvm::Function* the_function = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* loop_header_bb = llvm::BasicBlock::Create(*context, "loop_header", the_function);
        llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(*context, "loop_body", the_function);
        llvm::BasicBlock* loop_inc_bb = llvm::BasicBlock::Create(*context, "loop_inc", the_function);
        llvm::BasicBlock* loop_exit_bb = llvm::BasicBlock::Create(*context, "loop_exit", the_function);
        builder->CreateBr(loop_header_bb);
        builder->SetInsertPoint(loop_header_bb);
        llvm::Value* cond_v; if (for_expr->condition) { cond_v = generate_expression(*for_expr->condition); } else { cond_v = builder->getInt1(true); }
        if (!cond_v) return nullptr;
        builder->CreateCondBr(cond_v, loop_body_bb, loop_exit_bb);
        builder->SetInsertPoint(loop_body_bb);
        if (for_expr->body) { 
            for (const auto& stmt : for_expr->body->statements) {
                generate_statement(*stmt);
            }
        }
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(loop_inc_bb);
        builder->SetInsertPoint(loop_inc_bb);
        if (for_expr->increment) generate_expression(*for_expr->increment);
        if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(loop_header_bb);
        builder->SetInsertPoint(loop_exit_bb);
        named_values = old_named_values;
        return llvm::Constant::getNullValue(builder->getInt32Ty());
    }
    return nullptr;
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