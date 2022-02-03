#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>
#include <iostream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
    assem_instr_ = std::make_unique<cg::AssemInstr>(cg::AssemInstr(new assem::InstrList()));
    saveCalleeRegs(*assem_instr_->GetInstrList());
    fs_ = frame_->label->Name() + "_framesize";
    std::cout << "fs = " << fs_ << std::endl;
    for (tree::Stm *stm : traces_->GetStmList()->GetList()) {
        stm->Munch(*assem_instr_->GetInstrList(), fs_);
    }
    restoreCalleeRegs(*assem_instr_->GetInstrList());

    frame::procEntryExit2(assem_instr_->GetInstrList());
}

static temp::Temp *saved_rbx = nullptr;
static temp::Temp *saved_rbp = nullptr;
static temp::Temp *saved_r12 = nullptr;
static temp::Temp *saved_r13 = nullptr;
static temp::Temp *saved_r14 = nullptr;
static temp::Temp *saved_r15 = nullptr;

static void saveCalleeRegs(assem::InstrList &instr_list) {
    saved_rbx = temp::TempFactory::NewTemp();
    saved_rbp = temp::TempFactory::NewTemp();
    saved_r12 = temp::TempFactory::NewTemp();
    saved_r13 = temp::TempFactory::NewTemp();
    saved_r14 = temp::TempFactory::NewTemp();
    saved_r15 = temp::TempFactory::NewTemp();
    auto x64RM= dynamic_cast<frame::X64RegManager *>(reg_manager);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(saved_rbx), new temp::TempList(x64RM->rbx)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(saved_rbp), new temp::TempList(x64RM->rbp)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(saved_r12), new temp::TempList(x64RM->r12)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(saved_r13), new temp::TempList(x64RM->r13)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(saved_r14), new temp::TempList(x64RM->r14)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(saved_r15), new temp::TempList(x64RM->r15)));
}

static void restoreCalleeRegs(assem::InstrList &instr_list) {
    auto x64RM= dynamic_cast<frame::X64RegManager *>(reg_manager);

    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(x64RM->rbx), new temp::TempList(saved_rbx)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(x64RM->rbp), new temp::TempList(saved_rbp)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(x64RM->r12), new temp::TempList(saved_r12)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(x64RM->r13), new temp::TempList(saved_r13)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(x64RM->r14), new temp::TempList(saved_r14)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(x64RM->r15), new temp::TempList(saved_r15)));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
    for (auto instr : instr_list_->GetList())
        instr->Print(out, map);
    fprintf(out, "\n");
}
} // namespace cg

namespace tree {

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
    left_->Munch(instr_list, fs);
    right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
    instr_list.Append(new assem::LabelInstr(label_->Name(), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
    temp::Label *label = exp_->name_;
    auto targets = new assem::Targets(jumps_);
    instr_list.Append(new assem::OperInstr("jmp " + label->Name(), nullptr, nullptr, targets));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in CjumpStm" << std::endl;
    temp::Temp *left = left_->Munch(instr_list, fs);
    temp::Temp *right = right_->Munch(instr_list, fs);
    if (left->Int() < 0) assert(0);
    if (right->Int() < 0) assert(0);
    instr_list.Append(new assem::OperInstr("cmp `s0, `s1", nullptr, new temp::TempList({right, left}), nullptr));

    std::string str;
    switch(op_){
        case tree::RelOp::EQ_OP: str = std::string("je ");  break;
        case tree::RelOp::NE_OP: str = std::string("jne "); break;
        case tree::RelOp::LT_OP: str = std::string("jl ");  break;
        case tree::RelOp::GT_OP: str = std::string("jg ");  break;
        case tree::RelOp::LE_OP: str = std::string("jle "); break;
        case tree::RelOp::GE_OP: str = std::string("jge "); break;
    }
    std::cout << "Cjump str = " << str << std::endl;
    auto targets = new assem::Targets(new std::vector<temp::Label *>({true_label_}));
    instr_list.Append(new assem::OperInstr(str + temp::LabelFactory::LabelString(true_label_), nullptr, nullptr, targets));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in MoveStm" << std::endl;
    if (dynamic_cast<tree::TempExp *>(dst_) != nullptr) {
        std::cout << "choose road 1!" << std::endl;
        temp::Temp *left = src_->Munch(instr_list, fs);
        if (dynamic_cast<tree::TempExp *>(dst_)->temp_->Int() < 0) exit(3);
        instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({dynamic_cast<tree::TempExp *>(dst_)->temp_}),  new temp::TempList(left)));
    }
    else if (dynamic_cast<tree::MemExp *>(dst_) != nullptr) {
        std::cout << "choose road 2!" << std::endl;
        temp::Temp *left = src_->Munch(instr_list, fs);
        temp::Temp *right = dynamic_cast<tree::MemExp *>(dst_)->exp_->Munch(instr_list, fs);
        if (left->Int() < 0) assert(0);
        if (right->Int() < 0) assert(0);
        instr_list.Append(new assem::OperInstr("movq `s0, (`s1)", nullptr, new temp::TempList({left, right}), nullptr));
    }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
    exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in BinopExp" << std::endl;
    temp::Temp *reg = temp::TempFactory::NewTemp();
    temp::Temp *left = left_->Munch(instr_list, fs);
    temp::Temp *right = right_->Munch(instr_list, fs);

    if (left->Int() < 0) assert(0);
    if (right->Int() < 0) assert(0);
    if (reg->Int() < 0) assert(0);
    auto x64RM= dynamic_cast<frame::X64RegManager *>(reg_manager);
    switch (op_) {
        case tree::PLUS_OP: {
            instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg), new temp::TempList(left)));
            instr_list.Append(new assem::MoveInstr("addq `s0, `d0", new temp::TempList(reg), new temp::TempList({right, reg})));
            break;
        }
        case tree::MINUS_OP: {
            instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg), new temp::TempList(left)));
            instr_list.Append(new assem::MoveInstr("subq `s0, `d0", new temp::TempList(reg), new temp::TempList({right, reg})));
            break;
        }
        case tree::MUL_OP: {
//            instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg), new temp::TempList(left)));
//            instr_list.Append(new assem::MoveInstr("imulq `s0, `d0", new temp::TempList(reg), new temp::TempList({right, reg})));
            instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(x64RM->rax), new temp::TempList(left)));
            instr_list.Append(new assem::MoveInstr("imulq `s0", new temp::TempList(x64RM->rax), new temp::TempList({right})));
            instr_list.Append(new assem::MoveInstr("cqto", new temp::TempList({x64RM->rax, x64RM->rdx}), new temp::TempList(x64RM->rax)));
            instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg), new temp::TempList(x64RM->rax)));
            break;
        }
        case tree::DIV_OP: {

            instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(x64RM->rax), new temp::TempList(left)));
            instr_list.Append(new assem::MoveInstr("cqto", new temp::TempList({x64RM->rax, x64RM->rdx}), new temp::TempList(x64RM->rax)));
            instr_list.Append(new assem::MoveInstr("idivq `s0", new temp::TempList({x64RM->rax, x64RM->rdx}), new temp::TempList({right, x64RM->rax, x64RM->rdx})));
            instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg), new temp::TempList(x64RM->rax)));
            break;
        }
    }
    return reg;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in MemExp" << std::endl;
    temp::Temp *reg = temp::TempFactory::NewTemp();
    temp::Temp *r = exp_->Munch(instr_list, fs);
    if (reg->Int() < 0) assert(0);
    if (r->Int() < 0) assert(0);
    instr_list.Append(new assem::OperInstr("movq (`s0), `d0", new temp::TempList(reg), new temp::TempList(r), nullptr));
    return reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in TempExp" << std::endl;
    if (temp_ != reg_manager->FramePointer()) {
        std::cout << "return temp = " << temp_->Int() << std::endl;
        return temp_;
    }
    else {
        // if we want to use FP, we change it to reg = fs + stack_pointer
        temp::Temp *reg = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::OperInstr("leaq " + std::string(fs) + "(`s0), `d0", new temp::TempList(reg), new temp::TempList(reg_manager->StackPointer()), nullptr));
        std::cout << "return temp = " << reg->Int() << std::endl;
        return reg;
    }
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in EseqExp" << std::endl;
    assert(stm_ && exp_);
    stm_->Munch(instr_list, fs);
    return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in NameExp" << std::endl;
    std::string assem = "leaq " + name_->Name() + "(%rip), `d0";
    temp::Temp *reg = temp::TempFactory::NewTemp();
    instr_list.Append(new assem::OperInstr(assem, new temp::TempList(reg), nullptr, nullptr));
    return reg;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in ConstExp" << std::endl;
    temp::Temp *reg = temp::TempFactory::NewTemp();
    std::string assem = "movq $" + std::to_string(consti_) + ", `d0";
    instr_list.Append(new assem::OperInstr(assem, new temp::TempList(reg), nullptr, nullptr));
    return reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
    std::cout << "in CallExp" << std::endl;
    temp::Temp *reg = temp::TempFactory::NewTemp();
    auto x64RM= dynamic_cast<frame::X64RegManager *>(reg_manager);
    args_->MunchArgs(instr_list, fs);
    std::string assem = "call " + dynamic_cast<tree::NameExp *>(fun_)->name_->Name();
    instr_list.Append(new assem::OperInstr(assem, reg_manager->CallerSaves(), reg_manager->ArgRegs(), nullptr));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg), new temp::TempList(x64RM->rax)));
    return reg;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
    auto tempList = new temp::TempList();
    int num = 1;
    int out_formal = exp_list_.size() - 6;

    if (out_formal > 0)
        instr_list.Append(new assem::OperInstr("subq $" + std::to_string(reg_manager->WordSize()) + ",%rsp",
                                               nullptr, nullptr, nullptr));

    for (tree::Exp *exp : exp_list_) {
        temp::Temp *arg = exp->Munch(instr_list, fs);
        tempList->Append(arg);
        if (reg_manager->ARG_nth(num)) {
            instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg_manager->ARG_nth(num)), new temp::TempList(arg)));
        }
        else {
            instr_list.Append(new assem::OperInstr("subq $" + std::to_string(reg_manager->WordSize()) + ",%rsp",
                                       nullptr, nullptr, nullptr));
            instr_list.Append(new assem::MoveInstr("movq `s0, (`d0)", new temp::TempList(reg_manager->StackPointer()), new temp::TempList(arg)));
        }
        num++;
    }

    if (out_formal > 0)
        instr_list.Append(new assem::OperInstr("addq $" + std::to_string(reg_manager->WordSize() * (out_formal + 1)) + ",%rsp",
                                               nullptr, nullptr, nullptr));
    return tempList;
}

} // namespace tree
