#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"

void LoopInvHoist::run()
{
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();

    // 接下来由你来补充啦！
    auto func_list = m_->get_functions();
    for (auto func : func_list)
    {
        if (func->get_basic_blocks().size() == 0)   // 函数基本块列表
        {
            continue;
        }
        else
        {
            for (auto BBset : loop_searcher.get_loops_in_func(func))    // 得到函数内所有循环
            {
                auto get_base = loop_searcher.get_loop_base(BBset);     // 循环的条件块
                std::unordered_set<Value*> val;
                std::vector<Instruction *> ins;
                std::unordered_map<Instruction *, BasicBlock *> ins_bb;
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
            }
        }
    }
}

