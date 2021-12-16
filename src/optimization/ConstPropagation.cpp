#include "ConstPropagation.hpp"
#include "logging.hpp"
#include <algorithm>
#define IS_GLOBAL_VARIABLE(l_val) dynamic_cast<GlobalVariable *>(l_val)

// 给出了返回整形值的常数折叠实现，大家可以参考，在此基础上拓展
// 当然如果同学们有更好的方式，不强求使用下面这种方式
ConstantInt *ConstFolder::compute(
    Instruction::OpID op,
    ConstantInt *value1,
    ConstantInt *value2)
{
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);
        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// fold float point number
ConstantFP *ConstFolder::computeFP(
    Instruction::OpID op,
    ConstantFP *value1,
    ConstantFP *value2)
{
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);
        break;
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
        break;
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
        break;
    case Instruction::fdiv:
        return ConstantFP::get((c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// convert type or extern
Constant *ConstFolder::computeConvert(
    Instruction::OpID op,
    Constant *value)
{
    auto contant_int = dynamic_cast<ConstantInt *>(value);
    auto contant_float = dynamic_cast<ConstantFP *>(value);
    switch (op)
    {
    case Instruction::fptosi:
        return ConstantInt::get((int)(contant_float->get_value()), module_);
        break;
    case Instruction::sitofp:
        return ConstantFP::get((float)(contant_int->get_value()), module_);
        break;
    case Instruction::zext:
        return ConstantInt::get((int)(contant_int->get_value()), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// icmp with const
Constant *ConstFolder::computeIcmp(
    CmpInst::CmpOp op,
    ConstantInt *value1,
    ConstantInt *value2)
{
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case CmpInst::EQ:
        return ConstantInt::get(c_value1 == c_value2, module_);
        break;
    case CmpInst::NE:
        return ConstantInt::get(c_value1 != c_value2, module_);
        break;
    case CmpInst::GT:
        return ConstantInt::get(c_value1 > c_value2, module_);
        break;
    case CmpInst::GE:
        return ConstantInt::get(c_value1 >= c_value2, module_);
        break;
    case CmpInst::LT:
        return ConstantInt::get(c_value1 < c_value2, module_);
        break;
    case CmpInst::LE:
        return ConstantInt::get(c_value1 <= c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// fcmp with const
Constant *ConstFolder::computeFcmp(
    FCmpInst::CmpOp op,
    ConstantFP *value1,
    ConstantFP *value2)
{
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op)
    {
    case CmpInst::EQ:
        return ConstantInt::get(c_value1 == c_value2, module_);
        break;
    case CmpInst::NE:
        return ConstantInt::get(c_value1 != c_value2, module_);
        break;
    case CmpInst::GT:
        return ConstantInt::get(c_value1 > c_value2, module_);
        break;
    case CmpInst::GE:
        return ConstantInt::get(c_value1 >= c_value2, module_);
        break;
    case CmpInst::LT:
        return ConstantInt::get(c_value1 < c_value2, module_);
        break;
    case CmpInst::LE:
        return ConstantInt::get(c_value1 <= c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}
// 用来判断value是否为ConstantFP，如果不是则会返回nullptr
ConstantFP *cast_constantfp(Value *value)
{
    auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
    if (constant_fp_ptr)
    {
        return constant_fp_ptr;
    }
    else
    {
        return nullptr;
    }
}

// 用来判断value是否为ConstantInt，如果不是则会返回nullptr
ConstantInt *cast_constantint(Value *value)
{
    auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (constant_int_ptr)
    {
        return constant_int_ptr;
    }
    else
    {
        return nullptr;
    }
}

// a const propagation to compute const and delete useless code
void ConstPropagation::CPpass(BasicBlock *bb)
{
    std::vector<Instruction *> instr_set; // useless instr which need to delete
    visited_bb.push_back(bb);             // insert bb to show it has been visited

    for (auto instr : bb->get_instructions())
    {
        if (instr->get_num_operand() == 0)
        {
            continue;
        }
        int i = 0;
        bool AllConst = true; // record whether operands are all const val
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
        // deal with store
        if (instr->is_store())
        {
            // store i32 a, i32 *b
            // a is r_val, b is l_val
            auto r_val = static_cast<StoreInst *>(instr)->get_rval();
            auto l_val = static_cast<StoreInst *>(instr)->get_lval();
            // constant has been in it, erase because it has been changed
            // because of SSA, it changes again only by store instr
            if (constant_map.find(l_val->get_name()) != constant_map.end())
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
            // operation with one operand
            if (instr->is_fp2si() || instr->is_si2fp() || instr->is_zext())
            {
                auto const1 = dynamic_cast<Constant *>(instr->get_operand(0));
                if (const1)
                {
                    const_value = ConstFolder(m_).computeConvert(instr->get_instr_type(), const1);
                    change = false;
                }
            }
            // operation with two operand
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
                if (const1 && const2) // make sure get all const
                {
                    const_value = ConstFolder(m_).compute(instr->get_instr_type(), const1, const2);
                    change = false;
                }
            }
            else if (instr->is_fadd() || instr->is_fsub() || instr->is_fmul() || instr->is_fdiv())
            {
                auto const1 = cast_constantfp(instr->get_operand(0));
                auto const2 = cast_constantfp(instr->get_operand(1));
                if (const1 && const2) // make sure get all const
                {
                    const_value = ConstFolder(m_).computeFP(instr->get_instr_type(), const1, const2);
                    change = false;
                }
            }
            if (!change)
            {
                constant_map.insert(std::pair<std::string, Constant *>(instr->get_name(), const_value));
                instr->replace_all_use_with(const_value);
                instr_set.push_back(instr);
            }
        }
    }
    // delete instr after the propagation of whole block
    for (auto instr : instr_set)
    {
        bb->delete_instr(instr);
    }
    // clear map because we don't conpagation global var between different block
    constant_map.clear();
    // process with succ block
    for (auto succ : bb->get_succ_basic_blocks())
    {
        // succ not visit
        if (!std::count(visited_bb.begin(), visited_bb.end(), succ))
        {
            CPpass(succ);
        }
    }
}

void ConstPropagation::run()
{
    // 从这里开始吧！
    for (auto func : m_->get_functions())
    {
        if (func->get_num_basic_blocks() == 0)
            continue;
        auto entry_bb = func->get_entry_block();
        CPpass(entry_bb);
    }
}
