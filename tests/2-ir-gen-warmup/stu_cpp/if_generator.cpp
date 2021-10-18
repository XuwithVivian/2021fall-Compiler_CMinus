#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl;  // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main() {
    auto module = new Module("Cminus code");  // module name是什么无关紧要
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);
    Type *FloatType = Type::get_float_type(module);

    // main函数
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    // BasicBlock的名字在生成中无所谓,但是可以方便阅读
    builder->set_insert_point(bb);
  
    auto retAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0

    auto aAlloca = builder->create_alloca(FloatType);     // 在内存中分配参数a的位置
    builder->create_store(CONST_FP(5.555), aAlloca);  // 将参数a store下来
    auto aLoad = builder->create_load(aAlloca);  // 将参数a load上来
    auto fcmp = builder->create_fcmp_gt(aLoad, CONST_FP(1.0));  // 比较a和1
    auto trueBB = BasicBlock::create(module, "trueBB", mainFun);  // true分支
    auto falseBB = BasicBlock::create(module, "falseBB", mainFun); // false分支
    auto br = builder->create_cond_br(fcmp, trueBB, falseBB);  // 条件BR
    
    // if true
    builder->set_insert_point(trueBB);
    builder->create_ret(CONST_INT(233));  // return 233

    // if false
    builder->set_insert_point(falseBB);
    builder->create_ret(CONST_INT(0));  // return 0

    std::cout << module->print();
    delete module;
    return 0;

}