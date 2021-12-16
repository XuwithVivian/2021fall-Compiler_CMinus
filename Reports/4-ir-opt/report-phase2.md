# Lab4 实验报告

#### 吴晨源 PB19071467

#### 徐昊天 PB19071535

#### 尹家伟 PB19071522

## 实验要求

完成**常量传播与死代码删除**，**循环不变式外提**，**活跃变量分析**三个基本优化pass。

#### 1.常量传播与死代码删除

a. 只需要考虑过程内的常量传播，可以不用考虑数组，**全局变量只需要考虑块内的常量传播**

b. 整形浮点型都需要考虑。

c. 对于`a = 1 / 0`的情形，可以不考虑，即可以做处理也可以不处理。


#### 2.循环不变式外提

a. 思考如何判断语句与循环无关，且外提没有副作用。

b. 循环的条件块（就是在 LoopSearch 中找到的 Base 块）最多只有两个前驱，思考下，不变式应该外提到哪一个前驱。


#### 3.活跃变量分析

a. 分析bb块的入口和出口的活跃变量，并输出到json文件。

b. 需要考虑phi指令的特殊性，例如%0 = phi [%op1, %bb1], [%op2, %bb2]，只有控制流从%bb1传过来phi才产生%op1的活跃性，从%bb2传过来phi才产生%op2的活跃性。


## 实验难点

#### 常量传播与死代码删除

1. 判断一个表达式只含常量
2. 实现对全局变量的块内常量传播
3. 处理明确跳转方向的branch指令
3. 优化是基于mem2reg之上的

#### 循环不变式外提

1. 设计判断语句与循环无关的方式并储存该语句，并考虑外提的最佳方法。
2. 判断不变式应该外提到循环条件块的哪一个前驱以及如何找到该前驱。

#### 活跃变量分析

1. 由于phi指令的特殊性，需要对它进行特殊处理。
2. 在迭代求解live_in、live_out时如何判断两次迭代结果相同。

## 实验设计

* 常量传播

    ##### 1.判断一个表达式只含常量。
    
    实现思路：遍历operand,若operand->get_name()若为空字符串，表示该操作数为常量。然而具体测试的时候发现某些非常量操作数operand->get_name()也为空，导致程序出错。所以后面又加上了必要的条件判断。
    
    `auto const1 = dynamic_cast<Constant *>(instr->get_operand(0)); `
    
    先将操作数转换为常量，如果非空`if (const1)`，再执行对应的算符操作。
    
    相应代码：
    
    ```cpp
    	for (auto operand : instr->get_operands())
            {
                // const value dosen't have name but not all true because some mistakes maybe. so Allconst doen't matter
                if (!operand->get_name().empty())
                {
                    // const propagation
                    if (constant_map.find(operand->get_name()) != constant_map.end())
                    {
                        instr->set_operand(i, constant_map[operand->get_name()]);
                    }
                    else
                    {
                        AllConst = false;
                    }
                }
                i++;
            }
    	if (AllConst)
            {
                Constant *const_value;
                bool change = true;
                //operation with one operand
                if (instr->is_fp2si() || instr->is_si2fp() || instr->is_zext())
                {
                    auto const1 = dynamic_cast<Constant *>(instr->get_operand(0));
                    if (const1)
                    {
                        const_value = ConstFolder(m_).computeConvert(instr->get_instr_type(), const1);
                        change = false;
                    }
                }
    		...
    ```
    
    ##### 2.计算常量表达式的值：
    
    实现思路：根据指令的类型，提取一定数量的操作数，并判断是否全为常量，然后使用特定的compute函数计算表达式的值。compute函数根据助教给的样式拓展即可。
    
    相应代码:
    
    ```cpp
    	if (AllConst)
            {
                Constant *const_value;
                bool change = true;
                //operation with one operand
                if (instr->is_fp2si() || instr->is_si2fp() || instr->is_zext())
                {
                    auto const1 = dynamic_cast<Constant *>(instr->get_operand(0));
                    if (const1)
                    {
                        const_value = ConstFolder(m_).computeConvert(instr->get_instr_type(), const1);
                        change = false;
                    }
                }
                //operation with two operand
                else if (instr->is_cmp())
                {
                    auto const1 = cast_constantint(instr->get_operand(0));
                    auto const2 = cast_constantint(instr->get_operand(1));
                    auto cmpInstr = dynamic_cast<CmpInst *>(instr);
                    if (const1 && const2)
                    {
                        const_value = ConstFolder(m_).computeIcmp(cmpInstr->get_cmp_op(), const1, const2);
                        change = false;
                    }
                }
                else if (instr->is_fcmp())
                {
                    auto const1 = cast_constantfp(instr->get_operand(0));
                    auto const2 = cast_constantfp(instr->get_operand(1));
                    auto fcmpInstr = dynamic_cast<FCmpInst *>(instr);
                    if (const1 && const2)
                    {
                        const_value = ConstFolder(m_).computeFcmp(fcmpInstr->get_cmp_op(), const1, const2);
                        change = false;
                    }
                }
                else if (instr->is_add() || instr->is_sub() || instr->is_mul() || instr->is_div())
                {
                    auto const1 = cast_constantint(instr->get_operand(0));
                    auto const2 = cast_constantint(instr->get_operand(1));
                    if (const1 && const2) //make sure get all const
                    {
                        const_value = ConstFolder(m_).compute(instr->get_instr_type(), const1, const2);
                        change = false;
                    }
                }
                else if (instr->is_fadd() || instr->is_fsub() || instr->is_fmul() || instr->is_fdiv())
                {
                    auto const1 = cast_constantfp(instr->get_operand(0));
                    auto const2 = cast_constantfp(instr->get_operand(1));
                    if (const1 && const2) //make sure get all const
                    {
                        const_value = ConstFolder(m_).computeFP(instr->get_instr_type(), const1, const2);
                        change = false;
                    }
                }
    
    ```
    
    ##### 3.计算完表达式的值，需要用其进行值替换，并删除该条死代码
    
    实现思路：将产生的常量值，和原先的变量名的pair插入constant_map内，为之后的instr常量传播做准备，然后调用IR提供的指令，删除这条指令的操作数，并在所有地方将该值用新的值替换，并维护好use_def和def_use链表。最后将待删除的指令插入指令列表内，在遍历完一个基本块后对其删除。(若直接在每条指令遍历过程中删除，程序会出错)。
    
    相应代码：
    
    ```cpp
    		if (!change)
                {
                    constant_map.insert(std::pair<std::string, Constant *>(instr->get_name(), const_value));
                    instr->replace_all_use_with(const_value);
                    instr_set.push_back(instr);
                }
         	}
        }
        //delete instr after the propagation of whole block
        for (auto instr : instr_set)
        {
            bb->delete_instr(instr);
        }
    
    ```
    
    ##### 4.对全局变量进行块内常量传播
    
    实现思路：因为采用的是静态单赋值，在块内，全局变量的值被修改需要用到store指令，所以对store指令进行以下操作:如果它在constant_map内则将其从constant_map去除，因为该变量的值已经改变了。然后判断左值是否是全局变量，右值是否是常量。若满足条件，将其插入constant_map内。对于load指令，若右值出现在constant_map内，则对其进行值替换和死代码删除。在一个块分析完后将constant_map清空因为只考虑块内的常量传播。
    
    相应代码：
    
    ```cpp
    #define IS_GLOBAL_VARIABLE(l_val) dynamic_cast<GlobalVariable *>(l_val)
    //deal with store
            if (instr->is_store())
            {
                    // store i32 a, i32 *b
                    // a is r_val, b is l_val
                    auto r_val = static_cast<StoreInst *>(instr)->get_rval();
                    auto l_val = static_cast<StoreInst *>(instr)->get_lval();
                    // constant has been in it, erase because it has been changed
                    // because of SSA, it changes again only by store instr
                    if (constant_youzmap.find(l_val->get_name()) != constant_map.end())
                    {
                        constant_map.erase(l_val->get_name());
                    }
                    if (IS_GLOBAL_VARIABLE(l_val)) // store const in global var
                    {
    
                        auto const_int = cast_constantint(instr->get_operand(0));
                        auto const_fp = cast_constantfp(instr->get_operand(0));
                        // insert into map
                        if (const_int)
                        {
                            constant_map.insert(std::pair<std::string, Constant *>(l_val->get_name(), const_int));
                        }
                        else if (const_fp)
                        {
                            constant_map.insert(std::pair<std::string, Constant *>(l_val->get_name(), const_fp));
                        }
                    }
                continue;
            }
            // const propagation
            if (instr->is_load())
            {
                auto operand = instr->get_operand(0);
                if (constant_map.find(operand->get_name()) != constant_map.end())
                {
                    instr->replace_all_use_with(constant_map[operand->get_name()]);
                    instr_set.push_back(instr);
                }
                continue;
            }
    ....
     //clear map because we don't conpagation global var between different block
        constant_map.clear();
    
    ```
    
    ##### 5.处理明确跳转方向的branch指令
    
    实现思路：branch 指令的第一个操作数表示条件，若它是一个常量则根据true or false修改branch指令，删除操作符，并添加其将跳转的基本块作为操作符，然后删除基本块之间的前驱后继关系。这样可能产生冗余的基本块(本次实验不要求对此删除)。
    
    相关代码:
    
    ```cpp
    // deal with branch
            if (instr->is_br())
            {
                auto operand = instr->get_operand(0);
                auto val1 = cast_constantint(operand);
                bool conditon = true;
                bool need2change = false;
                if (val1)
                {
                    conditon = val1->get_value();
                    need2change = true;
                }
                else if (constant_map.find(operand->get_name()) != constant_map.end())
                {
                    conditon = constant_map[operand->get_name()];
                    need2change = true;
                }
                if (need2change)
                {
                    auto BB2br = instr->get_operand(conditon ? 1 : 2);
                    auto BB2del = dynamic_cast<BasicBlock *>(instr->get_operand(conditon ? 2 : 1));
                    instr->remove_operands(0, 2);
                    instr->add_operand(BB2br);
                    instr->add_use(BB2br);
                    bb->remove_succ_basic_block(BB2del);
                    BB2del->remove_pre_basic_block(bb);
                }
            }
    ```
    
    ##### 6.删除只含两个操作数的phi指令
    
    实现思路：若phi指令只有两个操作数则可以删除它，并用它的第一个操作数替换之后语句中的该值。这里首先要判断是否含undef操作数，undef会导致`get_num_operand()`得到的操作数个数小于实际的操作数个数。
    
    相应代码：
    
    ```cpp
    // deal with phi
            if (instr->is_phi() && instr->get_num_operand() == 2)
            {
                if (instr->get_num_operand() / 2 < instr->get_parent()->get_pre_basic_blocks().size()) // undef situation
                {
                    continue;
                }
                // phi instr which has only two operand and not because of undef
                auto operand1 = instr->get_operand(0);
                instr->replace_all_use_with(operand1);
                instr_set.push_back(instr);
                continue;
            }
    ```
    
    
    
    优化前后的IR对比（举一个例子）并辅以简单说明：
    
    优化前
    
    ```
    ; ModuleID = 'cminus'
    source_filename = "testcase-3.cminus"
    
    @opa = global i32 zeroinitializer
    @opb = global i32 zeroinitializer
    @opc = global i32 zeroinitializer
    @opd = global i32 zeroinitializer
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define i32 @max() {
    label_entry:
      %op0 = mul i32 0, 1
      %op1 = mul i32 %op0, 2
      %op2 = mul i32 %op1, 3
      %op3 = mul i32 %op2, 4
      %op4 = mul i32 %op3, 5
      %op5 = mul i32 %op4, 6
      %op6 = mul i32 %op5, 7
      store i32 %op6, i32* @opa
      %op7 = mul i32 1, 2
      %op8 = mul i32 %op7, 3
      %op9 = mul i32 %op8, 4
      %op10 = mul i32 %op9, 5
      %op11 = mul i32 %op10, 6
      %op12 = mul i32 %op11, 7
      %op13 = mul i32 %op12, 8
      store i32 %op13, i32* @opb
      %op14 = mul i32 2, 3
      %op15 = mul i32 %op14, 4
      %op16 = mul i32 %op15, 5
      %op17 = mul i32 %op16, 6
      %op18 = mul i32 %op17, 7
      %op19 = mul i32 %op18, 8
      %op20 = mul i32 %op19, 9
      store i32 %op20, i32* @opc
      %op21 = mul i32 3, 4
      %op22 = mul i32 %op21, 5
      %op23 = mul i32 %op22, 6
      %op24 = mul i32 %op23, 7
      %op25 = mul i32 %op24, 8
      %op26 = mul i32 %op25, 9
      %op27 = mul i32 %op26, 10
      store i32 %op27, i32* @opd
      %op28 = load i32, i32* @opa
      %op29 = load i32, i32* @opb
      %op30 = icmp slt i32 %op28, %op29
      %op31 = zext i1 %op30 to i32
      %op32 = icmp ne i32 %op31, 0
      br i1 %op32, label %label33, label %label39
    label33:                                                ; preds = %label_entry
      %op34 = load i32, i32* @opb
      %op35 = load i32, i32* @opc
      %op36 = icmp slt i32 %op34, %op35
      %op37 = zext i1 %op36 to i32
      %op38 = icmp ne i32 %op37, 0
      br i1 %op38, label %label40, label %label46
    label39:                                                ; preds = %label_entry, %label46
      ret i32 0
    label40:                                                ; preds = %label33
      %op41 = load i32, i32* @opc
      %op42 = load i32, i32* @opd
      %op43 = icmp slt i32 %op41, %op42
      %op44 = zext i1 %op43 to i32
      %op45 = icmp ne i32 %op44, 0
      br i1 %op45, label %label47, label %label49
    label46:                                                ; preds = %label33, %label49
      br label %label39
    label47:                                                ; preds = %label40
      %op48 = load i32, i32* @opd
      ret i32 %op48
    label49:                                                ; preds = %label40
      br label %label46
    }
    define void @main() {
    label_entry:
	  br label %label1
	label1:                                                ; preds = %label_entry, %label6
      %op15 = phi i32 [ 0, %label_entry ], [ %op9, %label6 ]
      %op3 = icmp slt i32 %op15, 200000000
      %op4 = zext i1 %op3 to i32
      %op5 = icmp ne i32 %op4, 0
      br i1 %op5, label %label6, label %label10
    label6:                                                ; preds = %label1
      %op7 = call i32 @max()
      %op9 = add i32 %op15, 1
      br label %label1
    label10:                                                ; preds = %label1
      %op11 = load i32, i32* @opa
      call void @output(i32 %op11)
      %op12 = load i32, i32* @opb
      call void @output(i32 %op12)
      %op13 = load i32, i32* @opc
      call void @output(i32 %op13)
      %op14 = load i32, i32* @opd
      call void @output(i32 %op14)
      ret void
    }
    ```
    优化后
    
    ```
    ; ModuleID = 'cminus'
    source_filename = "testcase-3.cminus"
    @opa = global i32 zeroinitializer
    @opb = global i32 zeroinitializer
    @opc = global i32 zeroinitializer
    @opd = global i32 zeroinitializer
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define i32 @max() {
    label_entry:
      store i32 0, i32* @opa
      store i32 40320, i32* @opb
      store i32 362880, i32* @opc
      store i32 1814400, i32* @opd
      br label %label33
    label33:                                                ; preds = %label_entry
      %op34 = load i32, i32* @opb
      %op35 = load i32, i32* @opc
      %op36 = icmp slt i32 %op34, %op35
      %op37 = zext i1 %op36 to i32
      %op38 = icmp ne i32 %op37, 0
      br i1 %op38, label %label40, label %label46
    label39:                                                ; preds = %label46
      ret i32 0
    label40:                                                ; preds = %label33
      %op41 = load i32, i32* @opc
      %op42 = load i32, i32* @opd
      %op43 = icmp slt i32 %op41, %op42
      %op44 = zext i1 %op43 to i32
      %op45 = icmp ne i32 %op44, 0
      br i1 %op45, label %label47, label %label49
    label46:                                                ; preds = %label33, %label49
      br label %label39
    label47:                                                ; preds = %label40
      %op48 = load i32, i32* @opd
      ret i32 %op48
    label49:                                                ; preds = %label40
      br label %label46
    }
    define void @main() {
    label_entry:
      br label %label1
    label1:                                                ; preds = %label_entry, %label6
      %op15 = phi i32 [ 0, %label_entry ], [ %op9, %label6 ]
      %op3 = icmp slt i32 %op15, 200000000
      %op4 = zext i1 %op3 to i32
      %op5 = icmp ne i32 %op4, 0
      br i1 %op5, label %label6, label %label10
    label6:                                                ; preds = %label1
      %op7 = call i32 @max()
      %op9 = add i32 %op15, 1
      br label %label1
    label10:                                                ; preds = %label1
      %op11 = load i32, i32* @opa
      call void @output(i32 %op11)
      %op12 = load i32, i32* @opb
      call void @output(i32 %op12)
      %op13 = load i32, i32* @opc
      call void @output(i32 %op13)
      %op14 = load i32, i32* @opd
      call void @output(i32 %op14)
      ret void
    }
    
    ```

    举例：可以看到
    
    ```
      %op0 = mul i32 0, 1
      %op1 = mul i32 %op0, 2
      %op2 = mul i32 %op1, 3
      %op3 = mul i32 %op2, 4
      %op4 = mul i32 %op3, 5
      %op5 = mul i32 %op4, 6
      %op6 = mul i32 %op5, 7
      store i32 %op6, i32* @opa
    ```

    被改为
    
    ```
    store i32 0, i32* @opa
    ```

    完成了常量传播和死代码删除。
    
    ```
      %op28 = load i32, i32* @opa
      %op29 = load i32, i32* @opb
      %op30 = icmp slt i32 %op28, %op29
      %op31 = zext i1 %op30 to i32
      %op32 = icmp ne i32 %op31, 0
      br i1 %op32, label %label33, label %label39
    ```

    被改为
    
    ```
    br label %label33
    ```

    完成了全局变量的块内常量传播以及branch指令的修改。
    
    (其余类似的优化不再展示)


* 循环不变式外提

    ##### 1.判断是否存在需要外提的不变式

    实现思路：首先遍历循环中所有基本块中的`instruction`并储存所有被赋值的变量，之后再次遍历并判断每一个操作数是否被赋值，若未被赋值即可将对应`instruction`外提。
    相应代码：

    ```cpp
          for (auto BB : *BBset)
                    {
                        for (auto get_ins : BB->get_instructions())     // 得到基本块所有指令
                        {
                            val.insert(get_ins);
                        }
                    }
                    bool flag = true;     // 判断是否存在需要外提的循环不变式
                    while (flag == true)
                    {
                        flag = false;
                        for (auto BB : *BBset)
                        {
                            for (auto get_ins : BB->get_instructions())
                            {
                                bool outins = true;         // 判断指令是否已存在于ins中
                                bool outloop = true;        // 判断是否需要外提
                                for (auto exist_ins : ins)
                                {
                                    if (exist_ins == get_ins)
                                    {
                                        outins = false;
                                        break;
                                    }
                                }
                                // 指令不可外提或已存于ins
                                if (outins == false || get_ins->is_br() == true || get_ins->is_call() == true || get_ins->is_load() ==true || get_ins->is_store() == true || get_ins->is_ret() == true || get_ins->is_phi() == true)
                                    continue;
                                
                                for (auto get_ope : get_ins->get_operands())    // 取出操作数
                                {
                                    if (val.find(get_ope) != val.end())
                                    {
                                        outloop = false;    // 式子不可外提
                                        break;
                                    }
                                }
                                if(outloop == true)
                                {
                                    flag = true;
                                    ins.push_back(get_ins);
                                    val.erase(get_ins);
                                    ins_bb.insert({get_ins, BB});
                                }
                            }
                        }
                    }
    ```

    ##### 2.将得到的循环不变式外提

    实现思路：循环条件块最多存在两个前驱块前驱块，应选择将不变式外提至在该循环之外的前驱块，并从该条件块中将原不变式删除。

    相应代码：

    ```cpp
          for (auto get_ins : ins)
                    {
                        ins_bb[get_ins]->delete_instr(get_ins);     // 从所在基本块中删除
                    }
                    
                    
                    for (auto pre_block : get_base->get_pre_basic_blocks())     // 遍历前驱块
                    {
                        if (BBset->find(pre_block) == BBset->end())     // 前驱块应为循环以外的基本块
                        {
                            auto get_term = pre_block->get_terminator();
                            pre_block->delete_instr(get_term);
                            for (auto get_ins : ins)    // 将循环不变式外提
                            {
                                pre_block->add_instruction(get_ins);
                                get_ins->set_parent(pre_block);
                            }
                            pre_block->add_instruction(get_term);
                        }
                    }
    ```

    优化前后的IR对比（举一个例子）并辅以简单说明：

    以`testcase-2.cminus`为例，优化前后生成的`.ll`分别如下：

    优化前：

    ```
    ; ModuleID = 'cminus'
    source_filename = "testcase-2.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      br label %label4
    label4:                                                ; preds = %label_entry, %label59
      %op62 = phi i32 [ %op65, %label59 ], [ undef, %label_entry ]
      %op63 = phi i32 [ 0, %label_entry ], [ %op61, %label59 ]
      %op64 = phi i32 [ %op66, %label59 ], [ undef, %label_entry ]
      %op6 = icmp slt i32 %op63, 10000000
      %op7 = zext i1 %op6 to i32
      %op8 = icmp ne i32 %op7, 0
      br i1 %op8, label %label9, label %label10
    label9:                                                ; preds = %label4
      br label %label12
    label10:                                                ; preds = %label4
      call void @output(i32 %op62)
      ret void
    label12:                                                ; preds = %label9, %label17
      %op65 = phi i32 [ %op62, %label9 ], [ %op56, %label17 ]
      %op66 = phi i32 [ 0, %label9 ], [ %op58, %label17 ]
      %op14 = icmp slt i32 %op66, 2
      %op15 = zext i1 %op14 to i32
      %op16 = icmp ne i32 %op15, 0
      br i1 %op16, label %label17, label %label59
    label17:                                                ; preds = %label12
      %op20 = mul i32 2, 2
      %op22 = mul i32 %op20, 2
      %op24 = mul i32 %op22, 2
      %op26 = mul i32 %op24, 2
      %op28 = mul i32 %op26, 2
      %op30 = mul i32 %op28, 2
      %op32 = mul i32 %op30, 2
      %op34 = mul i32 %op32, 2
      %op36 = mul i32 %op34, 2
      %op38 = sdiv i32 %op36, 2
      %op40 = sdiv i32 %op38, 2
      %op42 = sdiv i32 %op40, 2
      %op44 = sdiv i32 %op42, 2
      %op46 = sdiv i32 %op44, 2
      %op48 = sdiv i32 %op46, 2
      %op50 = sdiv i32 %op48, 2
      %op52 = sdiv i32 %op50, 2
      %op54 = sdiv i32 %op52, 2
      %op56 = sdiv i32 %op54, 2
      %op58 = add i32 %op66, 1
      br label %label12
    label59:                                                ; preds = %label12
      %op61 = add i32 %op63, 1
      br label %label4
    }
    
    ```

    优化后：

    ```
    ; ModuleID = 'cminus'
    source_filename = "testcase-2.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      %op20 = mul i32 2, 2
      %op22 = mul i32 %op20, 2
      %op24 = mul i32 %op22, 2
      %op26 = mul i32 %op24, 2
      %op28 = mul i32 %op26, 2
      %op30 = mul i32 %op28, 2
      %op32 = mul i32 %op30, 2
      %op34 = mul i32 %op32, 2
      %op36 = mul i32 %op34, 2
      %op38 = sdiv i32 %op36, 2
      %op40 = sdiv i32 %op38, 2
      %op42 = sdiv i32 %op40, 2
      %op44 = sdiv i32 %op42, 2
      %op46 = sdiv i32 %op44, 2
      %op48 = sdiv i32 %op46, 2
      %op50 = sdiv i32 %op48, 2
      %op52 = sdiv i32 %op50, 2
      %op54 = sdiv i32 %op52, 2
      %op56 = sdiv i32 %op54, 2
      br label %label4
    label4:                                                ; preds = %label_entry, %label59
      %op62 = phi i32 [ %op65, %label59 ], [ undef, %label_entry ]
      %op63 = phi i32 [ 0, %label_entry ], [ %op61, %label59 ]
      %op64 = phi i32 [ %op66, %label59 ], [ undef, %label_entry ]
      %op6 = icmp slt i32 %op63, 10000000
      %op7 = zext i1 %op6 to i32
      %op8 = icmp ne i32 %op7, 0
      br i1 %op8, label %label9, label %label10
    label9:                                                ; preds = %label4
      br label %label12
    label10:                                                ; preds = %label4
      call void @output(i32 %op62)
      ret void
    label12:                                                ; preds = %label9, %label17
      %op65 = phi i32 [ %op62, %label9 ], [ %op56, %label17 ]
      %op66 = phi i32 [ 0, %label9 ], [ %op58, %label17 ]
      %op14 = icmp slt i32 %op66, 2
      %op15 = zext i1 %op14 to i32
      %op16 = icmp ne i32 %op15, 0
      br i1 %op16, label %label17, label %label59
    label17:                                                ; preds = %label12
      %op58 = add i32 %op66, 1
      br label %label12
    label59:                                                ; preds = %label12
      %op61 = add i32 %op63, 1
      br label %label4
    }
    
    ```

    对比优化前后的代码，可知如下代码：

    ```
      %op20 = mul i32 2, 2
      %op22 = mul i32 %op20, 2
      %op24 = mul i32 %op22, 2
      %op26 = mul i32 %op24, 2
      %op28 = mul i32 %op26, 2
      %op30 = mul i32 %op28, 2
      %op32 = mul i32 %op30, 2
      %op34 = mul i32 %op32, 2
      %op36 = mul i32 %op34, 2
      %op38 = sdiv i32 %op36, 2
      %op40 = sdiv i32 %op38, 2
      %op42 = sdiv i32 %op40, 2
      %op44 = sdiv i32 %op42, 2
      %op46 = sdiv i32 %op44, 2
      %op48 = sdiv i32 %op46, 2
      %op50 = sdiv i32 %op48, 2
      %op52 = sdiv i32 %op50, 2
      %op54 = sdiv i32 %op52, 2
      %op56 = sdiv i32 %op54, 2
    ```

    从较为内部的`label17`基本块被移动至外部的`label_entry`基本块。

* 活跃变量分析
  
    ##### 1.找出每个基本块bb的use[bb]、def[bb]以及phi_uses[bb]（其中phi_uses[bb]是基本块bb的所有后继基本块中的phi指令用到的对应bb的变量）
    
    实现思路：遍历所有基本块，对于每个基本块，遍历其指令，对每条指令进行类型判断，并将对应的变量插入 **use[bb]** 、 **def[bb]** 及 **phi_uses[pre_bb]** 中（pre_bb指phi指令中该变量对应的当前基本块的前驱）。

    相应的代码 ：

    ```cpp
          // 分析 func_ 的每个bb块的use、def、phi_uses（注：这里phi_uses[bb]中的bb是phi指令中变量对应的前驱基本块）
          std::map<BasicBlock *, std::set<Value *>> use, def, phi_uses;
          for (auto bb : func_->get_basic_blocks())
          {
              for (auto instr : bb->get_instructions())
              {
                  // phi指令
                  if (instr->is_phi())
                  {
                      def[bb].insert(instr);
                      for (int i = 0; i < instr->get_num_operand() / 2; i++)
                      {
                          if (std::find(bb->get_pre_basic_blocks().begin(), bb->get_pre_basic_blocks().end(), instr->get_operand(2 * i + 1)) != bb->get_pre_basic_blocks().end())  //变量不是undef
                          {
                              if (!dynamic_cast<ConstantInt *>(instr->get_operand(2 * i)) && !dynamic_cast<ConstantFP *>(instr->get_operand(2 * i)))
                              {
                                  // 将变量插入其对应的前驱基本块的phi_uses中
                                  phi_uses[static_cast<BasicBlock *>(instr->get_operand(2 * i + 1))].insert(instr->get_operand(2 * i));
                              }
                          }
                      }
                  }
                  // alloc指令
                  else if (instr->is_alloca())
                  {
                      def[bb].insert(instr);
                  }
                  // br指令
                  else if (instr->is_br())
                  {
                      if (instr->get_num_operand() == 3) //有条件跳转
                      {
                          if (def[bb].find(instr->get_operand(0)) == def[bb].end() && !dynamic_cast<ConstantInt *>(instr->get_operand(0)) && !dynamic_cast<ConstantFP *>(instr->get_operand(0)))
                          {
                              use[bb].insert(instr->get_operand(0));
                          }
                      }
                  }
                  // call指令
                  else if (instr->is_call())
                  {
                      if (!instr->is_void())
                      {
                          def[bb].insert(instr);
                      }
                      for (auto operand : instr->get_operands())
                      {
                          if (operand != instr->get_operand(0) && def[bb].find(operand) == def[bb].end() && !dynamic_cast<ConstantInt *>(operand) && !dynamic_cast<ConstantFP *>(operand))
                          {
                              use[bb].insert(operand);
                          }
                      }
                  }
                  // load、zext、fp2si、si2fp
                  else if (instr->is_load() || instr->is_zext() || instr->is_fp2si() || instr->is_si2fp())
                  {
                      def[bb].insert(instr);
                      if (def[bb].find(instr->get_operand(0)) == def[bb].end() && !dynamic_cast<ConstantInt *>(instr->get_operand(0)) && !dynamic_cast<ConstantFP *>(instr->get_operand(0)))
                      {
                          use[bb].insert(instr->get_operand(0));
                      }
                  }
                  // ret指令
                  else if (instr->is_ret())
                  {
                      if (instr->get_num_operand())
                      {
                          if (def[bb].find(instr->get_operand(0)) == def[bb].end() && !dynamic_cast<ConstantInt *>(instr->get_operand(0)) && !dynamic_cast<ConstantFP *>(instr->get_operand(0)))
                          {
                              use[bb].insert(instr->get_operand(0));
                          }
                      }
                  }
                  // store指令
                  else if (instr->is_store())
                  {
                      for (auto operand : instr->get_operands())
                      {
                          if (def[bb].find(operand) == def[bb].end() && !dynamic_cast<ConstantInt *>(instr->get_operand(0)) && !dynamic_cast<ConstantFP *>(instr->get_operand(0)))
                          {
                              use[bb].insert(operand);
                          }
                      }
                  }
                  // gep指令
                  else if (instr->is_gep())
                  {
                      def[bb].insert(instr);
                      for (auto operand : instr->get_operands())
                      {
                          if (def[bb].find(operand) == def[bb].end() && !dynamic_cast<ConstantInt *>(operand) && !dynamic_cast<ConstantFP *>(operand))
                          {
                              use[bb].insert(operand);
                          }
                      }
                  }
                  // cmp、fcmp、add、sub、mul、sdiv、fadd、fsub、fmul、fdiv
                  else if (instr->is_cmp() || instr->is_fcmp() || instr->isBinary())
                  {
                      def[bb].insert(instr);
                      if (def[bb].find(instr->get_operand(0)) == def[bb].end() && !dynamic_cast<ConstantInt *>(instr->get_operand(0)) && !dynamic_cast<ConstantFP *>(instr->get_operand(0)))
                      {
                          use[bb].insert(instr->get_operand(0));
                      }
                      if (def[bb].find(instr->get_operand(1)) == def[bb].end() && !dynamic_cast<ConstantInt *>(instr->get_operand(1)) && !dynamic_cast<ConstantFP *>(instr->get_operand(1)))
                      {
                          use[bb].insert(instr->get_operand(1));
                      }
                  }
              }
          }
    ```

    ##### 2.每次迭代求出所有基本块的live_in、live_out直到前后两次得到的结果相同，即live_in、live_out已为最终结果。

    实现思路：每次迭代都遍历每个基本块，并利用 **OUT[bb]=∪<sub>S是bb的后继</sub>​IN[S]∪​phi_uses[bb]** 、 **IN[bb] = use[bb] ∪ (OUT[bb] - def[bb])** 两个公式求该基本块的live_in、live_out；且定义一个初始值为false的布尔值changed来控制循环的终止，每次更新当前基本块bb的live_in[bb]时，先把原先的live_in[bb]内容保存，待求得新live_in[bb]后，与原先的内容进行对比，只要有一个基本块的live_in发生了改变，则changed置为true，否则changed为初始值false，继续迭代。

    相应的代码 ：

    ```cpp
      // live_in、live_out迭代求解
      bool changed;
      do
      {
          changed = false;
          for (auto bb : func->get_basic_blocks())
          {
              // OUT[bb] = ∪IN[succ_bb] ∪ phi_uses[bb]
              live_out[bb].clear();
              std::set_union(live_out[bb].begin(), live_out[bb].end(), phi_uses[bb].begin(), phi_uses[bb].end(), inserter(live_out[bb], live_out[bb].begin()));
              for (auto succ_bb : bb->get_succ_basic_blocks())
              {
                  std::set_union(live_out[bb].begin(), live_out[bb].end(), live_in[succ_bb].begin(), live_in[succ_bb].end(), inserter(live_out[bb], live_out[bb].begin()));
              }
    
              auto pre_live_in_bb = live_in[bb];
    
              // IN[bb] = use[bb] ∪ (OUT[bb] - def[bb])
              live_in[bb].clear();
              std::set_difference(live_out[bb].begin(), live_out[bb].end(), def[bb].begin(), def[bb].end(), inserter(live_in[bb], live_in[bb].begin()));
              std::set_union(use[bb].begin(), use[bb].end(), live_in[bb].begin(), live_in[bb].end(), inserter(live_in[bb], live_in[bb].begin()));
    
              // 只要有一个IN[bb]变化，changed置为true，进行下一次迭代
              std::set<Value *> is_changed;
              std::set_difference(live_in[bb].begin(), live_in[bb].end(), pre_live_in_bb.begin(), pre_live_in_bb.end(), inserter(is_changed, is_changed.begin()));
              if (is_changed.begin() != is_changed.end())
              {
                  changed = true;
              }
          }
      } while (changed);
    ```


### 实验总结

​	   通过本次实验的动手实践，我再次熟悉了lightir的结构和接口，学会了如何使用`pass`遍历`module`内的结构，分析出信息(例如对活跃变量的分析 `pass`)，或者是对`module`内的指令和bb做一些变换(例如本次实验中的常量传播与死代码删除和循环不变式外提 `pass`)。同时掌握了一些c++ STL的使用方法。

​	   此次实验促进我深入理解了课堂上学习的理论知识，对循环不变式部分中循环及其基本块之间的关系有了更加深刻的理解，并在此基础上实现了循环不变式的外提，通过调用`cminusfc`生成`.ll`文件并与`baseline`目录中的文件进行对比从而对代码进行调试和修改，并且学习了更加灵活地使用`git`。

此次实验利用ligitir实现了活跃变量分析，对该部分知识有了更深刻的理解，并且掌握了用 `set_union` 和 `set_difference` 求两个集合的并集和差集的方法。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
