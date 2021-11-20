# Lab3 实验报告

```
队长姓名:吴晨源
队长学号:PB19071467
队员1姓名:徐昊天
队员1学号:PB19071535
队员2姓名:尹家伟
队员2学号:PB19071522
```


## 实验难点

实验中遇到哪些挑战以及解决方案

1. 理解实验中填写函数需要实现的功能。解决方案：通过阅读cminusfc.cpp,cminusf_builder.hpp,cminusf.md,LigthtIR.md等相关文件，逐渐理解了本次实验的基本流程。
2. `scope.enter()` `scope.exit()`函数调用位置的问题。刚开始写的时候没看到CminusfBuilder构造函数里面写了scope.enter()，然后在第一个函数的首尾加上了enter和exit，导致后面的程序出错。删去多余的enter之后就解决了。还有就是push与enter的先后顺序错误，导致后面调用find的时候找不到相应的值，发生段错误。(调试的时候不知道是什么原因引起的，所以注释了可能出错的代码不断编译运行才找到错误的地方)。最后调整enter和exit的位置确保find函数能找到相应的值。
3. 从上往下写的时候遇到部分函数和变量意义不明。(ASTNum，node.num...)。解决方案：暂时跳过该函数编写。等到其他函数需要该函数再进行编写。
4. 实验中存在多种需对变量进行类型转换的特殊情况，其中包括int32类型与float类型之间的相互转换以及int1类型对int32类型的转换等。
5. 使用全局变量val时需要对其类型进行判断，若调用`accept()`函数返回的val是一个变量variable，则此时val是一个指针，应通过load指令取值。
6. 函数调用时对于形参类型需进行多种情况的区分和考虑。

## 实验设计

请写明为了顺利完成本次实验，加入了哪些亮点设计，并对这些设计进行解释。
可能的阐述方向有:

### 1. 如何设计全局变量

`Value *val`:用于在不同函数之间传递参数和返回值。

`Function *current_fun`:编写return statement的visit函数是需要知道函数签名中的返回类型，所以添加了current_fun来记录当前所在函数。

### 2. 遇到的难点以及解决方案

   见上一节

### 3. 如何降低生成 IR 中的冗余

在一个基本块内，对return后面的代码不翻译

修改`void CminusfBuilder::visit(ASTCompoundStmt &node)`,在statement->accept(*this)前加上条件判断当前块是否结束。这样return后面的statement不会翻译，降低了生成IR的冗余。

```
void CminusfBuilder::visit(ASTCompoundStmt &node)
{
    //every CompoundStmt means an action scope
    scope.enter();
    for (auto declaration : node.local_declarations)
    {
        declaration->accept(*this);
    }
    for (auto statement : node.statement_list)
    {
        if (builder->get_insert_block()->get_terminator() == nullptr)
        {
              statement->accept(*this);
        }
    }
    scope.exit();
}

```



### 4. 对同一作用域重复定义变量和函数报错

变量和函数定义好后需要调用scope.push函数，由scope.push函数返回值，判断当前作用域该变量（或函数）是否定义过，若该变量（或函数）已存在，返回false，则报错，否则返回true,表示push成功。



### 5. 对二元运算的参数进行类型转换

首先对两个参数的类型进行判断，若存在不同，则将整型数转换为浮点数，以`ASTAdditiveExpression`函数为例，具体实现代码如下：

```
if (lval->get_type()->is_integer_type() == true && rval->get_type()->is_float_type() == true)
{
    //需进行类型转换
    lval = builder->create_sitofp(lval, FloatType);
}
else if (lval->get_type()->is_float_type() == true && rval->get_type()->is_integer_type() == true)
{
    //需进行类型转换
    rval = builder->create_sitofp(rval, FloatType);
}
```

类型转换结束后根据参数类型和操作符类型选择二元运算的函数，以`ASTAdditiveExpression`函数为例，具体实现代码如下：

```
if (node.op == OP_PLUS)
{
    if (lval->get_type()->is_float_type() == true) //判断二元运算参数是否为浮点数
        val = builder->create_fadd(lval, rval);
    else
        val = builder->create_iadd(lval, rval);
}
else if (node.op == OP_MINUS)
{
    if (lval->get_type()->is_float_type() == true)
        val = builder->create_fsub(lval, rval);
    else
        val = builder->create_isub(lval, rval);
}
```



### 6.对最后一条指令不是终止指令的函数，在其最后主动加上一条返回语句

在测试complex4.cminus时发现报错是由于`void main()`函数的最后是一个while循环语句，而循环语句的设计是默认最后加入一个新的基本块，导致最后出现了一个空的基本块。

选择的解决方案是判断一个函数的最后一条指令是否是终止指令，如果不是，则按照该函数的返回类型主动加一条返回语句，即在函数`void CminusfBuilder::visit(ASTFunDeclaration &node)`的最后加上：

```
if(builder->get_insert_block()->get_terminator() == nullptr) //如果函数最后一条指令不是终止指令，则自己加上
    {
        if(node.type == TYPE_VOID)
            builder->create_void_ret();
        else if(node.type == TYPE_INT) 
            builder->create_ret(ConstantZero::get(int32Type, module.get()));
        else 
            builder->create_ret(ConstantZero::get(floatType, module.get()));
    }
```



### 实验总结

此次实验有什么收获

1. 熟悉并理解了C++面向对象编程的特性，掌握了访问者模式编程的具体流程。

2. 对于语法制导翻译生成中间代码的过程有了更深刻的理解。

3. 学习了利用git进行团队合作，其中包括了`分支管理`，`解决冲突`等方面，提升了对git的了解和使用熟练度，增强了队员们的实践能力。

4. 对于cminus-f的语法规则以及LightIR的核心类有了更加深入的了解。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
