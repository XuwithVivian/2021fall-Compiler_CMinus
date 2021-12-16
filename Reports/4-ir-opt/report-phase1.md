# Lab4 实验报告-阶段一

#### 吴晨源 PB19071467

#### 徐昊天 PB19071535

#### 尹家伟 PB19071522

## 实验要求

阅读相关材料和代码，理解其具体实现过程，并回答思考题。

## 思考题
### LoopSearch

1. `LoopSearch`中直接用于描述一个循环的数据结构是什么？需要给出其具体类型。

   `unoredered_set<BasicBlock *>`

   `BBset_t*`

2. 循环入口是重要的信息，请指出`LoopSearch`中如何获取一个循环的入口？需要指出具体代码，并解释思路。

   ```cpp
   CFGNodePtr LoopSearch::find_loop_base(
       CFGNodePtrSet *set,
       CFGNodePtrSet &reserved)
   {
   
       CFGNodePtr base = nullptr;
       for (auto n : *set)
       {
           for (auto prev : n->prevs)
           {
               if (set->find(prev) == set->end())
               {
                   base = n;
               }
           }
       }
       if (base != nullptr)
           return base;
       for (auto res : reserved)
       {
           for (auto succ : res->succs)
           {
               if (set->find(succ) != set->end())
               {
                   base = succ;
               }
           }
       }
   	
       return base;
   }
   ```

   调用find_loop_base函数获取循环的入口。

   输入强连通片集合set和reserved集合。reserved集合包含的是前几次循环已找到的循环的入口结点指针。

   函数遍历set集合，如果一个结点的前驱不在强连通片内，则认为它是循环入口base。若base为空，则在reserved集合内寻找一个结点使得它的后继在set对应的强连通片内,则该后继作为base。

   

3. 仅仅找出强连通分量并不能表达嵌套循环的结构。为了处理嵌套循环，`LoopSearch`在Tarjan algorithm的基础之上做了什么特殊处理？

   Tarjan算法找到强连通片后，对各强连通片，寻找循环的入口结点base，将其插入reserved集合内。然后将base的前驱和后继指向base的边去掉，并将base从nodes集合去除掉。这样下次调用Tarjan算法外循环连通片不会被检测到，而可找到内层的连通片。

   对于嵌套的内循环，由于前驱被去掉，find_loop_search()函数第一个循环无法找到前驱在联通片外的结点。从而可在reserved集合结点的后继内寻找当前循环的base。

4. 某个基本块可以属于多层循环中，`LoopSearch`找出其所属的最内层循环的思路是什么？这里需要用到什么数据？这些数据在何时被维护？需要指出数据的引用与维护的代码，并简要分析。

   调用 get_inner_loop函数找到最内层循环。

   ```cpp
    // 得到bb所在最低层次的loop 
       BBset_t *get_inner_loop(BasicBlock* bb){
           if(bb2base.find(bb) == bb2base.end())
               return nullptr;
           return base2loop[bb2base[bb]];
       }
   ```

   bb2base可由基本块，找到该基本块所在循环的入口，默认最低层次的循环，即最内层循环。如果没找到循环入口，返回nullptr,否则返回base2loop[ bb2base[bb] ] 

   base2loop可由循环入口查找到对应的循环。

   需要用到base2loop,bb2base这两个数据。

   这些数据在run函数step4,step5维护

   ```cpp
    // step 4: store result
    auto bb_set = new BBset_t;
    std::string node_set_string = "";
   
    for (auto n : *scc)
    {
    	bb_set->insert(n->bb);
    	node_set_string = node_set_string + n->bb->get_name() + ',';
    }
    loop_set.insert(bb_set);
    func2loop[func].insert(bb_set);
    base2loop.insert({base->bb, bb_set});
    loop2base.insert({bb_set, base->bb});
    // step 5: map each node to loop base
    for (auto bb : *bb_set)
    {
    	if (bb2base.find(bb) == bb2base.end())
        	bb2base.insert({bb, base->bb});
    	else
        	bb2base[bb] = base->bb;
    }
   
   ```

   当找到一个强连通片对应循环的入口结点后，将该强连通片的所有结点作为一个循环集合bb_set。由base和bb_set可维护base2loop。

   对于bb_set里面的每个基本块，维护bb2base。循环的查找次序是由外向内的，所以默认bb2base是最低层次循环。

### Mem2reg
1. 请**简述**概念：支配性、严格支配性、直接支配性、支配边界。
   
    支配性：若n结点支配m结点，则到达m的每条代码路径都必然经由n，n∈DOM(m)。

    严格支配性：当且仅当a∈DOM(b)-{b}，a严格支配b。

    直接支配性：对于结点n，严格支配n的结点集中与n最接近的结点为n的直接支配结点。直接支配结点直接支配n。

    支配边界：对于结点n,同时满足n支配m的一个前趋且n不严格支配m两个条件的结点m的集合即为n的支配边界。


2. `phi`节点是SSA的关键特征，请**简述**`phi`节点的概念，以及引入`phi`节点的理由。

    phi节点概念：phi节点是一条指令, 用于根据当前块的前导选择一个值。例如对于当前块某条语句的右值含x，有不同的前驱块对x赋值，phi节点需要确定这个x是来自哪个前驱的值。
    
    引入phi节点理由：phi节点具有选择判断的作用，能够将不同情况到达的结果进行合并，确保得到正确的变量版本。


3. 下面给出的`cminus`代码显然不是SSA的，后面是使用lab3的功能将其生成的LLVM IR（**未加任何Pass**），说明对一个变量的多次赋值变成了什么形式？
   
    给出的`cminus`代码对变量a的多次赋值在生成的LLVM IR中对应内容如下：

    ```
    %op0 = alloca i32
    store i32 0, i32* %op0
    %op1 = add i32 1, 2
    store i32 %op1, i32* %op0
    %op2 = load i32, i32* %op0
    %op3 = mul i32 %op2, 4
    store i32 %op3, i32* %op0
    ```

    故对一个变量的多次赋值形式为：每一次分别用不同的目的操作数计算需赋给变量的数，计算结束后将得到的目的操作数`store`进同一个目的地址。


4. 对下面给出的`cminus`程序，使用lab3的功能，分别关闭/开启`Mem2Reg`生成LLVM IR。对比生成的两段LLVM IR，开启`Mem2Reg`后，每条`load`, `store`指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

    * func函数中，返回结果的方式发生了变化。分支语句变化。

        ```
        label7:                                                ; preds = %label_entry, %label6
        %op8 = load i32, i32* %op1
        ret i32 %op8
        ```
        关闭`Mem2Reg`时直接用`%op1`储存返回值并返回。
        ```
        label7:                                                ; preds = %label_entry, %label6
        %op9 = phi i32 [ %arg0, %label_entry ], [ 0, %label6 ]
        ret i32 %op9
        ```
        打开`Mem2Reg`时，phi指令可直接用于从不同基本块中选择正确的变量，通过phi指令在不同分支之间进行了选择并返回。同时label6 中的store %op1也不需要了,由常量0替换掉了。

        ```
        label6:                                                ; preds = %label_entry
          store i32 0, i32* %op1
        ```
    
    * 对于函数参数的调用方式发生了变化。
    
        ```
        %op1 = alloca i32
        store i32 %arg0, i32* %op1
        %op2 = load i32, i32* %op1
        %op3 = icmp sgt i32 %op2, 0
        ```
        关闭`Mem2Reg`时将函数参数`store`后再`load`出来。
        ```
        %op3 = icmp sgt i32 %arg0, 0
        ```
        打开`Mem2Reg`时直接从函数参数寄存器中调用参数。rename和remove_alloca将多余的alloca,store,load去除了。
    
    * 调用函数时传入参数的方式发生了变化。
    
        ```
        store i32 2333, i32* %op1
        %op6 = load i32, i32* %op1
        %op7 = call i32 @func(i32 %op6)
        ```
        关闭`Mem2Reg`时将参数`store`后再`load`出来并传入函数中。
        ```
        %op7 = call i32 @func(i32 2333)
        ```
        打开`Mem2Reg`时直接将常量参数传入函数中。rename将冗余的load，store删除了。
    
    * 对于全局变量的赋值与调用未发生变化
    
        ```
        store i32 1, i32* @globVar
        %op8 = load i32, i32* @globVar
        %op9 = call i32 @func(i32 %op8)
        ```
        全局变量的store会对其他基本块造成影响，不能删除。rename函数中的判断语句`  if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val))` 说明不会将全局变量的store,load语句加入将要删除的指令组内。
    
    * 对于GEP_instr未发生变化
    
        ```
        %op5 = getelementptr [10 x i32], [10 x i32]* %op0, i32 0, i32 5
        store i32 999, i32* %op5
        ```
        对于`cminus`代码中`arr[5] = 999`语句，打开`Mem2Reg`与否不会改变生成的IR部分。因为rename函数中的判断语句`  if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val))` 说明不会将GEP_instr语句加入将要删除的指令组内。


5. 指出放置phi节点的代码，并解释是如何使用支配树的信息的。需要给出代码中的成员变量或成员函数名称。

    `Mem2Reg::generate_phi()`函数中部分代码如下：

    ```cpp
    std::map<std::pair<BasicBlock *,Value *>, bool> bb_has_var_phi; // bb has phi for var
    for (auto var : global_live_var_name )
    {
        std::vector<BasicBlock *> work_list;
        work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
        for (int i =0 ; i < work_list.size() ; i++ )
        {   
            auto bb = work_list[i];
            for ( auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb))
            {
                if ( bb_has_var_phi.find({bb_dominance_frontier_bb, var}) == bb_has_var_phi.end() )
                { 
                    // generate phi for bb_dominance_frontier_bb & add bb_dominance_frontier_bb to work list
                    auto phi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb);
                    phi->set_lval(var);
                    bb_dominance_frontier_bb->add_instr_begin( phi );
                    work_list.push_back( bb_dominance_frontier_bb );
                    bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
                }
            }
        }
    }
    ```

    确定活动于多个基本块的全局变量，利用支配树产生的支配边界信息，即`bb_dominance_frontier_bb`,确定属于该支配边界的所有节点，由于支配边界上的节点中变量的值可能存在多种情况，故需插入phi指令，即`phi`。

### 代码阅读总结

通过对代码的阅读，我们了解到：

LoopSearch类构建了CFG图，用Tarjan算法求得强连通片，找到所有的循环和循环入口块，记录相应信息，为后续优化做准备。

Dominators类实现了数据流分析中的逆后序树构造，支配者树构造，对一个函数内基本块结点的支配者结点集合，直接支配者结点集合，支配边界的构造等操作。

Mem2Reg类用放置phi函数，重命名，删除分配语句操作构造了LLVM IR 的SSA格式，删除了部分冗余的变量和load,store,alloca指令。

阅读相关材料的过程，我们结合代码，一步步学会了如何计算支配边界，插入phi函数，重命名，最终构造静态单赋值形式。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息