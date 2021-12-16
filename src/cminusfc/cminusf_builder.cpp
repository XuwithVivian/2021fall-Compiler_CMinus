/*
 * 声明：本代码为 2021 秋 中国科大编译原理（李诚）课程实验参考实现。
 * 请不要以任何方式，将本代码上传到可以公开访问的站点或仓库
*/

#include "cminusf_builder.hpp"

#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_INT(num) \
    ConstantInt::get(num, module.get())

// You can define global variables here
// to store state

// store temporary value
Value *tmp_val = nullptr;
// whether require lvalue
bool require_lvalue = false;
// function that is being built
Function *cur_fun = nullptr;
// detect scope pre-enter (for elegance only)
bool pre_enter_scope = false;

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

bool promote(IRBuilder *builder, Value **l_val_p, Value **r_val_p) {
    bool is_int;
    auto &l_val = *l_val_p;
    auto &r_val = *r_val_p;
    if (l_val->get_type() == r_val->get_type()) {
        is_int = l_val->get_type()->is_integer_type();
    } else {
        is_int = false;
        if (l_val->get_type()->is_integer_type())
            l_val = builder->create_sitofp(l_val, FLOAT_T);
        else
            r_val = builder->create_sitofp(r_val, FLOAT_T);
    }
    return is_int;
}


/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) {
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());

    for (auto decl: node.declarations) {
        decl->accept(*this);
    }
}

void CminusfBuilder::visit(ASTNum &node) {
    if (node.type == TYPE_INT)
        tmp_val = CONST_INT(node.i_val);
    else
        tmp_val = CONST_FP(node.f_val);
}

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    Type *var_type;
    if (node.type == TYPE_INT)
        var_type = Type::get_int32_type(module.get());
    else
        var_type = Type::get_float_type(module.get());
    if (node.num == nullptr) {
        if (scope.in_global()) {
            auto initializer = ConstantZero::get(var_type, module.get());
            auto var = GlobalVariable::create
                    (
                    node.id,
                    module.get(),
                    var_type,
                    false,
                    initializer);
            scope.push(node.id, var);
        } else {
            auto var = builder->create_alloca(var_type);
            scope.push(node.id, var);
        }
    } else {
        auto *array_type = ArrayType::get(var_type, node.num->i_val);
        if (scope.in_global()) {
            auto initializer = ConstantZero::get(array_type, module.get());
            auto var = GlobalVariable::create
                    (
                    node.id,
                    module.get(),
                    array_type,
                    false,
                    initializer);
            scope.push(node.id, var);
        } else {
            auto var = builder->create_alloca(array_type);
            scope.push(node.id, var);
        }
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto& param: node.params) {
        if (param->type == TYPE_INT) {
            if (param->isarray) {
                param_types.push_back(INT32PTR_T);
            } else {
                param_types.push_back(INT32_T);
            }
        } else {
            if (param->isarray) {
                param_types.push_back(FLOATPTR_T);
            } else {
                param_types.push_back(FLOAT_T);
            }
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto fun =
        Function::create(
                fun_type,
                node.id,
                module.get());
    scope.push(node.id, fun);
    cur_fun = fun;
    auto funBB = BasicBlock::create(module.get(), "entry", fun);
    builder->set_insert_point(funBB);
    scope.enter();
    pre_enter_scope = true;
    std::vector<Value*> args;
    for (auto arg = fun->arg_begin();arg != fun->arg_end();arg++) {
        args.push_back(*arg);
    }
    for (int i = 0;i < node.params.size();++i) {
        if (node.params[i]->isarray) {
            Value *array_alloc;
            if (node.params[i]->type == TYPE_INT)
                array_alloc = builder->create_alloca(INT32PTR_T);
            else
                array_alloc = builder->create_alloca(FLOATPTR_T);
            builder->create_store(args[i], array_alloc);
            scope.push(node.params[i]->id, array_alloc);
        } else {
            Value *alloc;
            if (node.params[i]->type == TYPE_INT)
                alloc = builder->create_alloca(INT32_T);
            else
                alloc = builder->create_alloca(FLOAT_T);
            builder->create_store(args[i], alloc);
            scope.push(node.params[i]->id, alloc);
        }
    }
    node.compound_stmt->accept(*this);
    if (builder->get_insert_block()->get_terminator() == nullptr){
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) { }

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    bool need_exit_scope = !pre_enter_scope;
    if (pre_enter_scope) {
        pre_enter_scope = false;
    } else {
        scope.enter();
    }

    for (auto& decl: node.local_declarations) {
        decl->accept(*this);
    }

    for (auto& stmt: node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->get_terminator() != nullptr)
            break;
    }

    if (need_exit_scope) {
        scope.exit();
    }
}

void CminusfBuilder::visit(ASTExpressionStmt &node) {
    if (node.expression != nullptr)
        node.expression->accept(*this);
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    node.expression->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    Value *cond_val;
    if (ret_val->get_type()->is_integer_type())
        cond_val = builder->create_icmp_ne(ret_val, CONST_INT(0));
    else
        cond_val = builder->create_fcmp_ne(ret_val, CONST_FP(0.));

    if (node.else_statement == nullptr) {
        builder->create_cond_br(cond_val, trueBB, contBB);
    } else {
        builder->create_cond_br(cond_val, trueBB, falseBB);
    }
    builder->set_insert_point(trueBB);
    node.if_statement->accept(*this);

    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(contBB);

    if (node.else_statement == nullptr) {
        falseBB->erase_from_parent();
    } else {
        builder->set_insert_point(falseBB);
        node.else_statement->accept(*this);
        if (builder->get_insert_block()->get_terminator() == nullptr)
            builder->create_br(contBB);
    }

    builder->set_insert_point(contBB);
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    auto exprBB = BasicBlock::create(module.get(), "", cur_fun);
    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(exprBB);
    builder->set_insert_point(exprBB);
    node.expression->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    Value *cond_val;
    if (ret_val->get_type()->is_integer_type())
        cond_val = builder->create_icmp_ne(ret_val, CONST_INT(0));
    else
        cond_val = builder->create_fcmp_ne(ret_val, CONST_FP(0.));

    builder->create_cond_br(cond_val, trueBB, contBB);
    builder->set_insert_point(trueBB);
    node.statement->accept(*this);
    if (builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(exprBB);
    builder->set_insert_point(contBB);
}

void CminusfBuilder::visit(ASTReturnStmt &node) {
    if (node.expression == nullptr) {
        builder->create_void_ret();
    } else {
        auto fun_ret_type = cur_fun->get_function_type()->get_return_type();
        node.expression->accept(*this);
        if (fun_ret_type != tmp_val->get_type()) {
            if (fun_ret_type->is_integer_type())
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            else
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
        }

        builder->create_ret(tmp_val);
    }
}

void CminusfBuilder::visit(ASTVar &node) {
    auto var = scope.find(node.id);
    assert(var != nullptr);
    auto is_int = var->get_type()->get_pointer_element_type()->is_integer_type();
    auto is_float = var->get_type()->get_pointer_element_type()->is_float_type();
    auto is_ptr = var->get_type()->get_pointer_element_type()->is_pointer_type();
    bool should_return_lvalue = require_lvalue;
    require_lvalue = false;
    if (node.expression == nullptr) {
        if (should_return_lvalue) {
            tmp_val = var;
            require_lvalue = false;
        } else {
            if (is_int || is_float || is_ptr) {
                tmp_val = builder->create_load(var);
            } else {
                tmp_val = builder->create_gep(var, {CONST_INT(0), CONST_INT(0)});
            }
        }
    } else {
        node.expression->accept(*this);
        auto val = tmp_val;
        Value *is_neg;
        auto exceptBB = BasicBlock::create(module.get(), "", cur_fun);
        auto contBB = BasicBlock::create(module.get(), "", cur_fun);
        if (val->get_type()->is_float_type())
            val = builder->create_fptosi(val, INT32_T);

        is_neg = builder->create_icmp_lt(val, CONST_INT(0));

        builder->create_cond_br(is_neg, exceptBB, contBB);
        builder->set_insert_point(exceptBB);
        auto neg_idx_except_fun = scope.find("neg_idx_except");
        builder->create_call( static_cast<Function *>(neg_idx_except_fun), {});
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));

        builder->set_insert_point(contBB);
        Value *tmp_ptr;
        if (is_int || is_float)
            tmp_ptr = builder->create_gep(var, {val} );
        else if (is_ptr) {
            auto array_load = builder->create_load(var);
            tmp_ptr = builder->create_gep(array_load, {val} );
        } else
            tmp_ptr = builder->create_gep(var, {CONST_INT(0), val});
        if (should_return_lvalue) {
            tmp_val = tmp_ptr;
            require_lvalue = false;
        } else {
            tmp_val = builder->create_load(tmp_ptr);
        }
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) {
    node.expression->accept(*this);
    auto expr_result = tmp_val;
    require_lvalue = true;
    node.var->accept(*this);
    auto var_addr = tmp_val;
    if (var_addr->get_type()->get_pointer_element_type() != expr_result->get_type()) {
        if (expr_result->get_type() == INT32_T)
            expr_result = builder->create_sitofp(expr_result, FLOAT_T);
        else
            expr_result = builder->create_fptosi(expr_result, INT32_T);
    }
    builder->create_store(expr_result, var_addr);
    tmp_val = expr_result;
}

void CminusfBuilder::visit(ASTSimpleExpression &node) {
    if (node.additive_expression_r == nullptr) {
        node.additive_expression_l->accept(*this);
    } else {
        node.additive_expression_l->accept(*this);
        auto l_val = tmp_val;
        node.additive_expression_r->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        Value *cmp;
        switch (node.op) {
        case OP_LT:
            if (is_int)
                cmp = builder->create_icmp_lt(l_val, r_val);
            else
                cmp = builder->create_fcmp_lt(l_val, r_val);
            break;
        case OP_LE:
            if (is_int)
                cmp = builder->create_icmp_le(l_val, r_val);
            else
                cmp = builder->create_fcmp_le(l_val, r_val);
            break;
        case OP_GE:
            if (is_int)
                cmp = builder->create_icmp_ge(l_val, r_val);
            else
                cmp = builder->create_fcmp_ge(l_val, r_val);
            break;
        case OP_GT:
            if (is_int)
                cmp = builder->create_icmp_gt(l_val, r_val);
            else
                cmp = builder->create_fcmp_gt(l_val, r_val);
            break;
        case OP_EQ:
            if (is_int)
                cmp = builder->create_icmp_eq(l_val, r_val);
            else
                cmp = builder->create_fcmp_eq(l_val, r_val);
            break;
        case OP_NEQ:
            if (is_int)
                cmp = builder->create_icmp_ne(l_val, r_val);
            else
                cmp = builder->create_fcmp_ne(l_val, r_val);
            break;
        }

        tmp_val = builder->create_zext(cmp, INT32_T);
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    if (node.additive_expression == nullptr) {
        node.term->accept(*this);
    } else {
        node.additive_expression->accept(*this);
        auto l_val = tmp_val;
        node.term->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        switch (node.op) {
        case OP_PLUS:
            if (is_int)
                tmp_val = builder->create_iadd(l_val, r_val);
            else
                tmp_val = builder->create_fadd(l_val, r_val);
            break;
        case OP_MINUS:
            if (is_int)
                tmp_val = builder->create_isub(l_val, r_val);
            else
                tmp_val = builder->create_fsub(l_val, r_val);
            break;
        }
    }
}

void CminusfBuilder::visit(ASTTerm &node) {
    if (node.term == nullptr) {
        node.factor->accept(*this);
    } else {
        node.term->accept(*this);
        auto l_val = tmp_val;
        node.factor->accept(*this);
        auto r_val = tmp_val;
        bool is_int = promote(builder, &l_val, &r_val);
        switch (node.op) {
        case OP_MUL:
            if (is_int)
                tmp_val = builder->create_imul(l_val, r_val);
            else
                tmp_val = builder->create_fmul(l_val, r_val);
            break;
        case OP_DIV:
            if (is_int)
                tmp_val = builder->create_isdiv(l_val, r_val);
            else
                tmp_val = builder->create_fdiv(l_val, r_val);
            break;
        }
    }
}

void CminusfBuilder::visit(ASTCall &node) {
    auto fun = static_cast<Function *>(scope.find(node.id));
    std::vector<Value *> args;
    auto param_type = fun->get_function_type()->param_begin();
    for (auto &arg: node.args) {
        arg->accept(*this);
        if (!tmp_val->get_type()->is_pointer_type() &&
            *param_type != tmp_val->get_type()) {
            if (tmp_val->get_type()->is_integer_type())
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            else
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
        }
        args.push_back(tmp_val);
        param_type++;
    }

    tmp_val = builder->create_call(static_cast<Function *>(fun), args);
}

// #include "cminusf_builder.hpp"

// // use these macros to get constant value
// #define CONST_FP(num) \
//     ConstantFP::get((float)num, module.get())
// #define CONST_INT(num) \
//     ConstantInt::get(num, module.get())

// // You can define global variables here
// // to store state
// Value *val;
// Function *current_fun;
// /*
//  * use CMinusfBuilder::Scope to construct scopes
//  * scope.enter: enter a new scope
//  * scope.exit: exit current scope
//  * scope.push: add a new binding to current scope
//  * scope.find: find and return the value bound to the name
//  */

// void CminusfBuilder::visit(ASTProgram &node)
// {
//     // scope.enter() has operated in CminusfBuilder's constructed function
//     // traverse vector
//     for (auto declaration : node.declarations)
//     {
//         declaration->accept(*this);
//     }
//     scope.exit();
// }

// void CminusfBuilder::visit(ASTNum &node)
// { // store num

//     if (node.type == TYPE_INT)
//     {
//         val = CONST_INT(node.i_val);
//     }
//     else if (node.type == TYPE_FLOAT)
//     {
//         val = CONST_FP(node.f_val);
//     }
// }

// void CminusfBuilder::visit(ASTVarDeclaration &node)
// {
//     //num: array's element number
//     auto TyInt32 = Type::get_int32_type(module.get());
//     auto TyFloat = Type::get_float_type(module.get());
//     if (scope.in_global())
//     {
//         // define global variable
//         if (node.num != nullptr)
//         { //array
//             if (node.type == TYPE_INT)
//             {
//                 auto *arrayType = ArrayType::get(TyInt32, node.num->i_val);
//                 auto initializer = ConstantZero::get(TyInt32, module.get());
//                 auto var = GlobalVariable::create(node.id, module.get(), arrayType, false, initializer);
//                 if (!scope.push(node.id, var))
//                     std::cout << node.id << " already exists\n";
//             }
//             else
//             {
//                 auto *arrayType = ArrayType::get(TyFloat, node.num->i_val);
//                 auto initializer = ConstantZero::get(TyFloat, module.get());
//                 auto var = GlobalVariable::create(node.id, module.get(), arrayType, false, initializer);
//                 if (!scope.push(node.id, var))
//                     std::cout << node.id << " already exists\n";
//             }
//         }
//         else
//         { //num
//             if (node.type == TYPE_INT)
//             {
//                 auto initializer = ConstantZero::get(TyInt32, module.get());
//                 auto var = GlobalVariable::create(node.id, module.get(), TyInt32, false, initializer);
//                 if (!scope.push(node.id, var))
//                     std::cout << node.id << " already exists\n";
//             }
//             else
//             {
//                 auto initializer = ConstantZero::get(TyFloat, module.get());
//                 auto var = GlobalVariable::create(node.id, module.get(), TyFloat, false, initializer);
//                 if (!scope.push(node.id, var))
//                     std::cout << node.id << " already exists\n";
//             }
//         }
//     }
//     else
//     {
//         //define local variable
//         if (node.num != nullptr)
//         { //array
//             node.num->accept(*this);
//             if (node.type == TYPE_INT)
//             {
//                 auto arrayType = Type::get_array_type(TyInt32, node.num->i_val);
//                 auto var = builder->create_alloca(arrayType);
//                 if (!scope.push(node.id, var))
//                     std::cout << node.id << " already exists\n";
//             }
//             else
//             {
//                 auto arrayType = Type::get_array_type(TyFloat, node.num->i_val);
//                 auto var = builder->create_alloca(arrayType);
//                 if (!scope.push(node.id, var))
//                     std::cout << node.id << " already exists\n";
//             }
//         }
//         else
//         { //num
//             if (node.type == TYPE_INT)
//             {
//                 auto var = builder->create_alloca(TyInt32);
//                 if (!scope.push(node.id, var))
//                     std::cout << node.id << " already exists\n";
//             }
//             else
//             {
//                 auto var = builder->create_alloca(TyFloat);
//                 if (!scope.push(node.id, var))
//                     std::cout << node.id << " already exists\n";
//             }
//         }
//     }
// }

// void CminusfBuilder::visit(ASTFunDeclaration &node)
// {

//     std::vector<Type *> paramList;

//     auto intptrType = Type::get_int32_ptr_type(module.get());
//     auto floatptrType = Type::get_float_ptr_type(module.get());
//     auto int32Type = Type::get_int32_type(module.get());
//     auto floatType = Type::get_float_type(module.get());
//     for (auto param : node.params)
//     {
//         param->accept(*this);
//         // get ptr type for array
//         if (param->isarray)
//         {
//             if (param->type == TYPE_INT)
//             {
//                 paramList.push_back(intptrType);
//             }
//             else
//             {
//                 paramList.push_back(floatptrType);
//             }
//         }
//         else
//         {
//             if (param->type == TYPE_INT)
//             {
//                 paramList.push_back(int32Type);
//             }
//             else
//             {
//                 paramList.push_back(floatType);
//             }
//         }
//     }
//     // define function's return value's type
//     Type *retType = Type::get_int32_type(module.get());
//     switch (node.type)
//     {
//     case TYPE_INT:
//         retType = Type::get_int32_type(module.get());
//         break;
//     case TYPE_FLOAT:
//         retType = Type::get_float_type(module.get());
//         break;
//     case TYPE_VOID:
//         retType = Type::get_void_type(module.get());
//         break;
//     default:
//         std::cout << "wrong function type:" << node.id << std::endl;
//         exit(1);
//         break;
//     }

//     auto funType = FunctionType::get(retType, paramList);
//     auto fun = Function::create(funType, node.id, module.get());
//     if (!scope.push(node.id, fun))
//                     std::cout << node.id << " already exists\n";
//     scope.enter();
//     current_fun = fun;

//     auto bb = BasicBlock::create(module.get(), node.id, fun);
//     builder->set_insert_point(bb);
//     // get formal parameters
//     std::vector<Value *> args;
//     for (auto arg = fun->arg_begin();arg!=fun->arg_end();arg++){
//         args.push_back(*arg);
//     }

//     int i = 0;
//     // load args
//     for(auto param: node.params){
//         //array
//         if(param->isarray){
//             if(param->type == TYPE_INT){
//                 auto varAlloca = builder->create_alloca(intptrType);
//                 builder->create_store(args[i],varAlloca);
//                 scope.push(param->id,varAlloca);
//             }
//             else{
//                 auto varAlloca = builder->create_alloca(floatptrType);
//                 builder->create_store(args[i],varAlloca);
//                 scope.push(param->id,varAlloca);
//             }
//         }
//         //num
//         else{
//             if(param->type == TYPE_INT){
//                 auto varAlloca = builder->create_alloca(int32Type);
//                 builder->create_store(args[i],varAlloca);
//                 scope.push(param->id,varAlloca);
//             }
//             else{
//                 auto varAlloca = builder->create_alloca(floatType);
//                 builder->create_store(args[i],varAlloca);
//                 scope.push(param->id,varAlloca);
//             }
//         }
//         i++;
//     }

//     node.compound_stmt->accept(*this);
//     if(builder->get_insert_block()->get_terminator() == nullptr) //如果函数最后一条指令不是终止指令，则自己加上
//     {
//         if(node.type == TYPE_VOID)
//             builder->create_void_ret();
//         else if(node.type == TYPE_INT) 
//             builder->create_ret(ConstantZero::get(int32Type, module.get()));
//         else 
//             builder->create_ret(ConstantZero::get(floatType, module.get()));
//     }
//     scope.exit();
// }

// void CminusfBuilder::visit(ASTParam &node)
// {
//     if (node.isarray)
//     {
//         //array type
//     }
//     else
//     {
//         //not array type
//     }
// }

// void CminusfBuilder::visit(ASTCompoundStmt &node)
// {
//     //every CompoundStmt means an action scope
//     scope.enter();
//     for (auto declaration : node.local_declarations)
//     {
//         declaration->accept(*this);
//     }
//     for (auto statement : node.statement_list)
//     {
//         if (builder->get_insert_block()->get_terminator() == nullptr)
//         {
//               statement->accept(*this);
//         }
//     }
//     scope.exit();
// }
// // ****************************
// void CminusfBuilder::visit(ASTExpressionStmt &node)
// {
//     if (node.expression != nullptr)
//     {
//         node.expression->accept(*this);
//     }
// }

// void CminusfBuilder::visit(ASTSelectionStmt &node)
// // SelectionStmt -> ​if(expression) statement∣ if(expression) statement else statement​
// {
//     node.expression->accept(*this);
//     Value *cmp;
//     bool trueBB_returnd = true, falseBB_returnd = true;  //记录这两个基本块是否有返回语句
//     auto trueBB = BasicBlock::create(module.get(), "", current_fun);
//     auto falseBB = BasicBlock::create(module.get(), "", current_fun);
//     auto nextBB = BasicBlock::create(module.get(), "", current_fun);
//     nextBB->erase_from_parent();
//     if(val->get_type()->is_pointer_type())  //若是变量，则需要取出其值
//     {
//         val = builder->create_load(val);
//     }
//     if (val->get_type()->is_integer_type())
//     {
//         cmp = builder->create_icmp_ne(val, CONST_INT(0));
//     }
//     else if (val->get_type()->is_float_type())
//     {
//         cmp = builder->create_fcmp_ne(val, CONST_FP(0.0));
//     }
//     //没有else语句
//     if (node.else_statement == nullptr)
//     {
//         falseBB->erase_from_parent();
//         builder->create_cond_br(cmp, trueBB, nextBB);
//         builder->set_insert_point(trueBB);
//         node.if_statement->accept(*this);
//         //如果if语句最后一条指令不是终止指令，则需要插入一个新的基本块
//         if (builder->get_insert_block()->get_terminator() == nullptr) 
//         {
//             current_fun->add_basic_block(nextBB);
//             builder->create_br(nextBB);
//             builder->set_insert_point(nextBB);
//         }
//     }
//     //有else语句
//     else
//     {
//         builder->create_cond_br(cmp, trueBB, falseBB);
//         builder->set_insert_point(trueBB);
//         node.if_statement->accept(*this);
//         if (builder->get_insert_block()->get_terminator() == nullptr)
//         {
//             trueBB_returnd = false;
//             builder->create_br(nextBB);
//         }
//         builder->set_insert_point(falseBB);
//         node.else_statement->accept(*this);
//         if (builder->get_insert_block()->get_terminator() == nullptr)
//         {
//             falseBB_returnd = false;
//             builder->create_br(nextBB);
//         }
//         //只要if语句和else语句两者之间存在一个的最后一条指令不是终止指令，就必须插入一个新的基本块
//         if(trueBB_returnd == false || falseBB_returnd == false)
//         {
//             current_fun->add_basic_block(nextBB);
//             builder->set_insert_point(nextBB);
//         }
//     }
// }

// void CminusfBuilder::visit(ASTIterationStmt &node)
// //IterationStmt -> while(expression) statement
// {
//     auto exprBB = BasicBlock::create(module.get(), "", current_fun);
//     auto trueBB = BasicBlock::create(module.get(), "", current_fun);
//     auto nextBB = BasicBlock::create(module.get(), "", current_fun);
//     Value *cmp;
//     builder->create_br(exprBB);
//     builder->set_insert_point(exprBB);
//     node.expression->accept(*this);
//     if(val->get_type()->is_pointer_type())   
//     {
//         val = builder->create_load(val);
//     }
//     if (val->get_type()->is_integer_type())
//     {
//         cmp = builder->create_icmp_gt(val, CONST_INT(0));
//     }
//     else if (val->get_type()->is_float_type())
//     {
//         cmp = builder->create_fcmp_gt(val, CONST_FP(0.0));
//     }
//     builder->create_cond_br(cmp, trueBB, nextBB);
//     builder->set_insert_point(trueBB);
//     node.statement->accept(*this);
//     if (builder->get_insert_block()->get_terminator() == nullptr)
//     {
//         builder->create_br(exprBB);
//     }   
//     builder->set_insert_point(nextBB);
// }

// void CminusfBuilder::visit(ASTReturnStmt &node)
// // ReturnStmt -> Return | Return expression
// {
//     Type *FloatType = Type::get_float_type(module.get());
//     Type *Int32Type = Type::get_int32_type(module.get());
//     if (node.expression == nullptr)
//     {
//         builder->create_void_ret();
//     }
//     else
//     {
//         node.expression->accept(*this);
//         if(val->get_type()->is_pointer_type())
//         {
//             val = builder->create_load(val);
//         }
//         if (current_fun->get_return_type()->is_float_type() == true && val->get_type()->is_integer_type() == true)
//         {
//             val = builder->create_sitofp(val, FloatType);
//         }
//         else if (current_fun->get_return_type()->is_integer_type() == true && val->get_type()->is_float_type() == true)
//         {
//             val = builder->create_fptosi(val, Int32Type);
//         }
//         builder->create_ret(val);
//     }
// }

// void CminusfBuilder::visit(ASTVar &node)
// {
//     auto int32Type = Type::get_int32_type(module.get());
//     auto floatType = Type::get_float_type(module.get());
//     auto var = scope.find(node.id);
//     if (node.expression == nullptr)
//     {
//         val = var;
//     }
//     else
//     {
//         node.expression->accept(*this);
//         auto idx = val;
//         if (idx->get_type()->is_pointer_type() == false)    // not pointer type
//         {
//             if (idx->get_type()->is_float_type() == true)
//                 idx = builder->create_fptosi(idx, int32Type);
//         }
//         else    // is pointer type
//         {
//             if (idx->get_type()->get_pointer_element_type()->is_array_type() == true)   // array type
//             {
//                 idx = builder->create_gep(idx, {CONST_INT(0), CONST_INT(0)});
//                 if (idx->get_type()->is_float_type())
//                     idx = builder->create_fptosi(idx, int32Type);
//             }
//             else    // not array type
//             {
//                 idx = builder->create_load(idx);
//                 if (idx->get_type()->is_float_type())
//                     idx = builder->create_fptosi(idx, int32Type);
//             }
//         }
//         auto is_neg_idx = builder->create_icmp_lt(idx, CONST_INT(0));
//         auto errorBB = BasicBlock::create(module.get(), "", current_fun);
//         auto nextBB = BasicBlock::create(module.get(), "", current_fun);
//         builder->create_cond_br(is_neg_idx, errorBB, nextBB);
//         builder->set_insert_point(errorBB);
//         auto neg_idx_except_func = scope.find("neg_idx_except");
//         builder->create_call(neg_idx_except_func, {});
//         if (current_fun->get_return_type()->is_integer_type() == true)
//         {
//             builder->create_ret(CONST_INT(0));
//         }
//         else if (current_fun->get_return_type()->is_float_type() == true)
//         {
//             builder->create_ret(CONST_FP(0));
//         }
//         else
//             builder->create_void_ret();
//         builder->set_insert_point(nextBB);
//         if (builder->create_load(var)->get_type()->is_array_type() == true)
//             val = builder->create_gep(var, {CONST_INT(0), idx});
//         else
//             val = builder->create_gep(builder->create_load(var), {idx});
//     }
// }
// // ****************************
// void CminusfBuilder::visit(ASTAssignExpression &node)
// {
//     Type *FloatType = Type::get_float_type(module.get());
//     Type *Int32Type = Type::get_int32_type(module.get());
//     node.var->accept(*this);
//     Value *var = val;
//     node.expression->accept(*this);
//     if (val->get_type()->is_pointer_type() == true)
//     {
//         val = builder->create_load(val);
//     }
//     // 若存在不同类型则进行转换
//     if (val->get_type()->is_float_type() == true && var->get_type()->get_pointer_element_type()->is_integer_type() == true)
//         val = builder->create_fptosi(val, Int32Type);
//     else if (val->get_type()->is_integer_type() == true && var->get_type()->get_pointer_element_type()->is_float_type() == true)
//         val = builder->create_sitofp(val, FloatType);
//     builder->create_store(val, var);
// }

// void CminusfBuilder::visit(ASTSimpleExpression &node)
// // SimpleExpression -> AdditiveExpression relop AdditiveExpression | AdditiveExpression
// {
//     if (node.additive_expression_r == nullptr)
//     {
//         node.additive_expression_l->accept(*this);
//     }
//     else
//     {
//         Type *FloatType = Type::get_float_type(module.get());
//         node.additive_expression_l->accept(*this);
//         Value *lval = val;
//         node.additive_expression_r->accept(*this);
//         Value *rval = val;
//         if (lval->get_type()->is_pointer_type())
//         {
//             lval = builder->create_load(lval);
//         }
//         if (rval->get_type()->is_pointer_type())
//         {
//             rval = builder->create_load(rval);
//         }
//         if (lval->get_type()->is_integer_type() == true && rval->get_type()->is_float_type() == true)
//         {
//             //需进行类型转换
//             lval = builder->create_sitofp(lval, FloatType);
//         }
//         else if (lval->get_type()->is_float_type() == true && rval->get_type()->is_integer_type() == true)
//         {
//             //需进行类型转换
//             rval = builder->create_sitofp(rval, FloatType);
//         }

//         // 判断操作符类型
//         if (node.op == OP_LT)
//         {
//             if (lval->get_type()->is_float_type() == true) //判断二元运算参数是否为浮点数
//                 val = builder->create_fcmp_lt(lval, rval);
//             else
//                 val = builder->create_icmp_lt(lval, rval);
//         }
//         else if (node.op == OP_LE)
//         {
//             if (lval->get_type()->is_float_type() == true)
//                 val = builder->create_fcmp_le(lval, rval);
//             else
//                 val = builder->create_icmp_le(lval, rval);
//         }
//         else if (node.op == OP_GE)
//         {
//             if (lval->get_type()->is_float_type() == true)
//                 val = builder->create_fcmp_ge(lval, rval);
//             else
//                 val = builder->create_icmp_ge(lval, rval);
//         }
//         else if (node.op == OP_GT)
//         {
//             if (lval->get_type()->is_float_type() == true)
//                 val = builder->create_fcmp_gt(lval, rval);
//             else
//                 val = builder->create_icmp_gt(lval, rval);
//         }
//         else if (node.op == OP_EQ)
//         {
//             if (lval->get_type()->is_float_type() == true)
//                 val = builder->create_fcmp_eq(lval, rval);
//             else
//                 val = builder->create_icmp_eq(lval, rval);
//         }
//         else if (node.op == OP_NEQ)
//         {
//             if (lval->get_type()->is_float_type() == true)
//                 val = builder->create_fcmp_ne(lval, rval);
//             else
//                 val = builder->create_icmp_ne(lval, rval);
//         }
//         val = builder->create_zext(val, Type::get_int32_type(module.get())); //转化为32位整型数
//     }
// }

// void CminusfBuilder::visit(ASTAdditiveExpression &node)
// // AdditiveExpression -> AdditiveExpression addop term | term
// {
//     if (node.additive_expression == nullptr)
//     {
//         node.term->accept(*this);
//     }
//     else
//     {
//         Type *FloatType = Type::get_float_type(module.get());
//         node.additive_expression->accept(*this);
//         Value *lval = val;
//         node.term->accept(*this);
//         Value *rval = val;
//         if (lval->get_type()->is_pointer_type())
//         {
//             lval = builder->create_load(lval);
//         }
//         if (rval->get_type()->is_pointer_type())
//         {
//             rval = builder->create_load(rval);
//         }
//         if (lval->get_type()->is_integer_type() == true && rval->get_type()->is_float_type() == true)
//         {
//             //需进行类型转换
//             lval = builder->create_sitofp(lval, FloatType);
//         }
//         else if (lval->get_type()->is_float_type() == true && rval->get_type()->is_integer_type() == true)
//         {
//             //需进行类型转换
//             rval = builder->create_sitofp(rval, FloatType);
//         }

//         // 判断操作符类型
//         if (node.op == OP_PLUS)
//         {
//             if (lval->get_type()->is_float_type() == true) //判断二元运算参数是否为浮点数
//                 val = builder->create_fadd(lval, rval);
//             else
//                 val = builder->create_iadd(lval, rval);
//         }
//         else if (node.op == OP_MINUS)
//         {
//             if (lval->get_type()->is_float_type() == true)
//                 val = builder->create_fsub(lval, rval);
//             else
//                 val = builder->create_isub(lval, rval);
//         }
//     }
// }

// void CminusfBuilder::visit(ASTTerm &node)
// // term -> term mulop factor | factor
// {
//     if (node.term == nullptr)
//     {
//         node.factor->accept(*this);
//     }
//     else
//     {
//         Type *FloatType = Type::get_float_type(module.get());
//         node.term->accept(*this);
//         Value *lval = val;
//         node.factor->accept(*this);
//         Value *rval = val;

//         if (lval->get_type()->is_pointer_type())
//         {
//             lval = builder->create_load(lval);
//         }
//         if (rval->get_type()->is_pointer_type())
//         {
//             rval = builder->create_load(rval);
//         }

//         if (lval->get_type()->is_integer_type() == true && rval->get_type()->is_float_type() == true)
//         {
//             //需进行类型转换
//             lval = builder->create_sitofp(lval, FloatType);
//         }
//         else if (lval->get_type()->is_float_type() == true && rval->get_type()->is_integer_type() == true)
//         {
//             //需进行类型转换
//             rval = builder->create_sitofp(rval, FloatType);
//         }

//         // 判断操作符类型
//         if (node.op == OP_MUL)
//         {
//             if (lval->get_type()->is_float_type() == true) //判断二元运算参数是否为浮点数
//                 val = builder->create_fmul(lval, rval);
//             else
//                 val = builder->create_imul(lval, rval);
//         }
//         else if (node.op == OP_DIV)
//         {
//             if (lval->get_type()->is_float_type() == true)
//                 val = builder->create_fdiv(lval, rval);
//             else
//                 val = builder->create_isdiv(lval, rval);
//         }
//     }
// }

// void CminusfBuilder::visit(ASTCall &node)
// {

//     Type *FloatType = Type::get_float_type(module.get());
//     Type *Int32Type = Type::get_int32_type(module.get());
//     Type *Int32ptrType = Type::get_int32_ptr_type(module.get());
//     Type *FloatptrType = Type::get_float_ptr_type(module.get());
//     Value *func = scope.find(node.id);
//     std::vector<Value *> callargs;
//     Type *gettype = func->get_type();
//     FunctionType *functype = static_cast<FunctionType *>(gettype);
//     auto param = functype->param_begin();
//     for (auto arg : node.args)
//     {
//         arg->accept(*this);
//         if (val->get_type()->is_pointer_type() == false)
//         { //参数不是指针类型
//             if (val->get_type()->is_float_type() == true && (*param)->is_integer_type() == true)
//                 val = builder->create_fptosi(val, Int32Type);
//             else if (val->get_type()->is_integer_type() == true && (*param)->is_float_type() == true)
//                 val = builder->create_sitofp(val, FloatType);
//         }
//         else
//         { //参数是指针类型
//             if (val->get_type() == Int32ptrType || val->get_type() == FloatptrType)
//             {
//                 val = builder->create_load(val); //取值
//                 if (val->get_type()->is_float_type() == true && (*param)->is_integer_type() == true)
//                     val = builder->create_fptosi(val, Int32Type);
//                 else if (val->get_type()->is_integer_type() == true && (*param)->is_float_type() == true)
//                     val = builder->create_sitofp(val, FloatType);
//             }
//             else if (val->get_type()->get_pointer_element_type() == Int32ptrType || val->get_type()->get_pointer_element_type() == FloatptrType)
//             {                                    // 二维指针
//                 val = builder->create_load(val); //取值
//             }
//             else if (val->get_type()->get_pointer_element_type()->is_array_type() == true)
//             { // 数组类型
//                 val = builder->create_gep(val, {CONST_INT(0), CONST_INT(0)});
//             }
//         }
//         callargs.push_back(val);
//         param++;
//     }
//     val = builder->create_call(func, callargs);
// }
