#include "cminusf_builder.hpp"

// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_INT(num) \
    ConstantInt::get(num, module.get())


// You can define global variables here
// to store state
Value *val;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) { }

void CminusfBuilder::visit(ASTNum &node) { }

void CminusfBuilder::visit(ASTVarDeclaration &node) { }

void CminusfBuilder::visit(ASTFunDeclaration &node) { }

void CminusfBuilder::visit(ASTParam &node) { }

void CminusfBuilder::visit(ASTCompoundStmt &node) { }

void CminusfBuilder::visit(ASTExpressionStmt &node) { }

void CminusfBuilder::visit(ASTSelectionStmt &node) { }

void CminusfBuilder::visit(ASTIterationStmt &node) { }

void CminusfBuilder::visit(ASTReturnStmt &node) { }

void CminusfBuilder::visit(ASTVar &node) { }

void CminusfBuilder::visit(ASTAssignExpression &node) {
    Type *FloatType = Type::get_float_type(module.get());
    Type *Int32Type = Type::get_int32_type(module.get());
    node.var->accept(*this);
    Value *var = val;
    node.expression->accept(*this);
    if(val->get_type()->is_pointer_type() == true)
        val = builder->create_load(val);
    if(val->get_type() == var->get_type()->get_pointer_element_type())
        builder->create_store(val, var);
    else {
        if(var->get_type()->get_pointer_element_type()->is_integer_type() == true) {
            val = builder->create_fptosi(val, Int32Type);
            builder->create_store(val, var);
        }
        else if(var->get_type()->get_pointer_element_type()->is_float_type() == true) {
            val = builder->create_sitofp(val, FloatType);
            builder->create_store(val, var);
        }
    }
 }

void CminusfBuilder::visit(ASTSimpleExpression &node) {
    if(node.additive_expression_r == nullptr) {
        node.additive_expression_l->accept(*this);
    }
    else {
        Type *FloatType = Type::get_float_type(module.get());
        node.additive_expression_l->accept(*this);
        Value *lval = val;
        node.additive_expression_r->accept(*this);
        Value *rval = val;

        // 判断操作数类型并根据情况转换数据类型
        bool itype;

        if(lval->get_type()->is_integer_type() == true && rval->get_type()->is_integer_type() == true)
            itype = true;
        else if(lval->get_type()->is_integer_type() == true && rval->get_type()->is_float_type() == true) {
            itype = false;
            lval = builder->create_sitofp(lval, FloatType);
        }
        else if(lval->get_type()->is_float_type() == true && rval->get_type()->is_integer_type() == true) {
            itype = false;
            rval = builder->create_sitofp(rval, FloatType);
        }
        else if(lval->get_type()->is_float_type() == true && rval->get_type()->is_float_type() == true)
            itype = false;

        // 判断操作符类型
        if (node.op == OP_LT) {
            if (itype == true)
               val = builder->create_icmp_lt(lval, rval);
            else
               val = builder->create_fcmp_lt(lval, rval);
        }
        else if (node.op == OP_LE) {
            if (itype == true)
               val = builder->create_icmp_le(lval, rval);
            else
               val = builder->create_fcmp_le(lval, rval);
        }
        else if (node.op == OP_GE) {
            if (itype == true)
               val = builder->create_icmp_ge(lval, rval);
            else
               val = builder->create_fcmp_ge(lval, rval);
        }
        else if (node.op == OP_GT) {
            if (itype == true)
               val = builder->create_icmp_gt(lval, rval);
            else
               val = builder->create_fcmp_gt(lval, rval);
        }
        else if (node.op == OP_EQ) {
            if (itype == true)
               val = builder->create_icmp_eq(lval, rval);
            else
               val = builder->create_fcmp_eq(lval, rval);
        }
        else if (node.op == OP_NEQ) {
            if (itype == true)
               val = builder->create_icmp_ne(lval, rval);
            else
               val = builder->create_fcmp_ne(lval, rval);
        }
    }
 }

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    if(node.additive_expression == nullptr) {
        node.term->accept(*this);
    }
    else {
        Type *FloatType = Type::get_float_type(module.get());
        node.additive_expression->accept(*this);
        Value *lval = val;
        node.term->accept(*this);
        Value *rval = val;

        // 判断操作数类型并根据情况转换数据类型
        bool itype;

        if(lval->get_type()->is_integer_type() == true && rval->get_type()->is_integer_type() == true)
            itype = true;
        else if(lval->get_type()->is_integer_type() == true && rval->get_type()->is_float_type() == true) {
            itype = false;
            lval = builder->create_sitofp(lval, FloatType);
        }
        else if(lval->get_type()->is_float_type() == true && rval->get_type()->is_integer_type() == true) {
            itype = false;
            rval = builder->create_sitofp(rval, FloatType);
        }
        else if(lval->get_type()->is_float_type() == true && rval->get_type()->is_float_type() == true)
            itype = false;

        // 判断操作符类型
        if (node.op == OP_PLUS) {
            if (itype == true)
               val = builder->create_iadd(lval, rval);
            else
               val = builder->create_fadd(lval, rval);
        }
        else if (node.op == OP_MINUS) {
            if (itype == true)
               val = builder->create_isub(lval, rval);
            else
               val = builder->create_fsub(lval, rval);
        }
    }
 }

void CminusfBuilder::visit(ASTTerm &node) {
    if(node.term == nullptr) {
        node.factor->accept(*this);
    }
    else {
        Type *FloatType = Type::get_float_type(module.get());
        node.term->accept(*this);
        Value *lval = val;
        node.factor->accept(*this);
        Value *rval = val;

        // 判断操作数类型并根据情况转换数据类型
        bool itype;

        if(lval->get_type()->is_integer_type() == true && rval->get_type()->is_integer_type() == true)
            itype = true;
        else if(lval->get_type()->is_integer_type() == true && rval->get_type()->is_float_type() == true) {
            itype = false;
            lval = builder->create_sitofp(lval, FloatType);
        }
        else if(lval->get_type()->is_float_type() == true && rval->get_type()->is_integer_type() == true) {
            itype = false;
            rval = builder->create_sitofp(rval, FloatType);
        }
        else if(lval->get_type()->is_float_type() == true && rval->get_type()->is_float_type() == true)
            itype = false;

        // 判断操作符类型
        if (node.op == OP_MUL) {
            if (itype == true)
               val = builder->create_imul(lval, rval);
            else
               val = builder->create_fmul(lval, rval);
        }
        else if (node.op == OP_DIV) {
            if (itype == true)
               val = builder->create_isdiv(lval, rval);
            else
               val = builder->create_fdiv(lval, rval);
        }
    }
 }

void CminusfBuilder::visit(ASTCall &node) {
    Type *FloatType = Type::get_float_type(module.get());
    Type *Int32Type = Type::get_int32_type(module.get());
    Value *func = scope.find(node.id);
    std::vector<Value *> callargs;
    Type *functype = func->get_type();
    FunctionType *calltype = (FunctionType*)functype;
    auto param = calltype->param_begin();
    for (auto arg: node.args) {
        arg->accept(*this);
        if(val->get_type()->is_pointer_type() == false) {
            if(val->get_type()->is_float_type() == true && (*param)->is_integer_type() == true)
               val = builder->create_fptosi(val, Int32Type);
            else if(val->get_type()->is_integer_type() == true && (*param)->is_float_type() == true)
               val = builder->create_sitofp(val, FloatType);
        }
        else {
            if(val->get_type()->get_pointer_element_type()->is_float_type() == true && (*param)->is_integer_type() == true) {
               val = builder->create_load(val);
               val = builder->create_fptosi(val, Int32Type);
            }
               
            else if(val->get_type()->get_pointer_element_type()->is_integer_type() == true && (*param)->is_float_type() == true) {
               val = builder->create_load(val);
               val = builder->create_sitofp(val, FloatType);
            }
        }
        callargs.push_back(val);
        param++;
    }
    val = builder->create_call(func, callargs);
 }
