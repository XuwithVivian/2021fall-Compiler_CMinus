#include "ActiveVars.hpp"
#include <algorithm>
void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions())
    {
        if (func->get_basic_blocks().empty())
        {
            continue;
        }
        else
        {
            func_ = func;

            func_->set_instr_name();
            live_in.clear();
            live_out.clear();

            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内

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

            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return;
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars += "{\n";
    active_vars += "\"function\": \"";
    active_vars += func_->get_name();
    active_vars += "\",\n";

    active_vars += "\"live_in\": {\n";
    for (auto &p : live_in)
    {
        if (p.second.size() == 0)
        {
            continue;
        }
        else
        {
            active_vars += "  \"";
            active_vars += p.first->get_name();
            active_vars += "\": [";
            for (auto &v : p.second)
            {
                active_vars += "\"%";
                active_vars += v->get_name();
                active_vars += "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    },\n";

    active_vars += "\"live_out\": {\n";
    for (auto &p : live_out)
    {
        if (p.second.size() == 0)
        {
            continue;
        }
        else
        {
            active_vars += "  \"";
            active_vars += p.first->get_name();
            active_vars += "\": [";
            for (auto &v : p.second)
            {
                active_vars += "\"%";
                active_vars += v->get_name();
                active_vars += "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}