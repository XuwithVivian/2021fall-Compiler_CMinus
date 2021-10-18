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

    // main函数
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    // BasicBlock的名字在生成中无所谓,但是可以方便阅读
    builder->set_insert_point(bb);
  
    auto retAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(0), retAlloca);             // 默认 ret 0

    auto aAlloca = builder->create_alloca(Int32Type);     // 在内存中分配参数a的位置
    auto iAlloca = builder->create_alloca(Int32Type);     // 在内存中分配参数i的位置 
    builder->create_store(CONST_INT(10), aAlloca);              // 将参数a store下来
    builder->create_store(CONST_INT(0), iAlloca);              // 将参数i store下来
    
    // while(i < 10)
    auto whileBB = BasicBlock::create(module, "whileBB", mainFun);
    builder->create_br(whileBB);
    builder->set_insert_point(whileBB);
    
    auto aLoad = builder->create_load(aAlloca);  // 将参数a load上来
    auto iLoad = builder->create_load(iAlloca);  // 将参数i load上来

    auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));  // i和10的比较

    auto trueBB = BasicBlock::create(module, "trueBB", mainFun);    // true分支
    auto falseBB = BasicBlock::create(module, "falseBB", mainFun);  // false分支
    auto br = builder->create_cond_br(icmp, trueBB, falseBB);  // 条件BR
    
    // if true
    builder->set_insert_point(trueBB);
    iLoad = builder->create_load(iAlloca);
    auto iret = builder->create_iadd(iLoad, CONST_INT(1));  // i = i + 1
    builder->create_store(iret, iAlloca);
    iLoad = builder->create_load(iAlloca);
    aLoad = builder->create_load(aAlloca);
    auto aret = builder->create_iadd(aLoad, iLoad);  // a = a + i
    builder->create_store(aret, aAlloca);
    aLoad = builder->create_load(aAlloca);
    builder->create_br(whileBB);

    // if false
    builder->set_insert_point(falseBB);
    aLoad = builder->create_load(aAlloca);
    builder->create_ret(aLoad);  // return a

    std::cout << module->print();
    delete module;
    return 0;


}