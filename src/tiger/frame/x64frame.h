//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

extern frame::RegManager *reg_manager;
namespace frame {
class X64RegManager : public RegManager {
public :
    X64RegManager() {
        rax = temp::TempFactory::NewTemp();
        rdi = temp::TempFactory::NewTemp();
        rsi = temp::TempFactory::NewTemp();
        rdx = temp::TempFactory::NewTemp();
        rcx = temp::TempFactory::NewTemp();
        r8 = temp::TempFactory::NewTemp();
        r9 = temp::TempFactory::NewTemp();
        r10 = temp::TempFactory::NewTemp();
        r11 = temp::TempFactory::NewTemp();
        rbx = temp::TempFactory::NewTemp();
        rbp = temp::TempFactory::NewTemp();
        r12 = temp::TempFactory::NewTemp();
        r13 = temp::TempFactory::NewTemp();
        r14 = temp::TempFactory::NewTemp();
        r15 = temp::TempFactory::NewTemp();
        fp = temp::TempFactory::NewTemp();
        rsp = temp::TempFactory::NewTemp();
        caller_saved_regiters = new temp::TempList({rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11});
        callee_saved_registers = new temp::TempList({rbx, rbp, r12, r13, r14, r15});
        registers = new temp::TempList({rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11, rbx, rbp, r12, r13, r14, r15, fp, rsp});
        args_registers = new temp::TempList({rdi, rsi, rdx, rcx, r8, r9});
        ret_sink_registers = new temp::TempList({rsp, rax, rbx, rbp, r12, r13, r14, r15});
        allregs_noRSP = new temp::TempList({rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11, rbx, rbp, r12, r13, r14, r15, fp});
    }
    temp::TempList *Registers() override {
        return registers;
    }

    temp::TempList *ArgRegs() override {
        return args_registers;
    }

    temp::TempList *Allregs_noRSP() override {
        return allregs_noRSP;
    }

    temp::TempList *CallerSaves() override {
        return caller_saved_regiters;
    }

    temp::TempList *CalleeSaves() override {
        return callee_saved_registers;
    }

    temp::TempList *ReturnSink() override {
        //procEntryExit2在函数体的末尾添加了一条sink指令，告诉寄存器分配器在过程的出口某些寄存器是活跃的，在x86-64上就是rax、rsp和callee-saved register
        return ret_sink_registers;
    }

    int WordSize() override {
        return 8;
    }

    temp::Temp *FramePointer() override {
        return fp;
    }

    temp::Temp *StackPointer() override {
        return rsp;
    }

    temp::Temp *ReturnValue() override {
        return rax;
    }

    temp::Temp *ARG_nth(int num) override {
        switch (num) {
            case 1: return rdi;
            case 2: return rsi;
            case 3: return rdx;
            case 4: return rcx;
            case 5: return r8;
            case 6: return r9;
            default: return nullptr;
        }
    }

    temp::Map *RegMap() override {
        if(regmap)
            return regmap;
        else {
            regmap = temp::Map::Empty();
            regmap->Enter(rax, new std::string("%rax"));
            regmap->Enter(rdi, new std::string("%rdi"));
            regmap->Enter(rsi, new std::string("%rsi"));
            regmap->Enter(rdx, new std::string("%rdx"));
            regmap->Enter(rcx, new std::string("%rcx"));
            regmap->Enter(r8, new std::string("%r8"));
            regmap->Enter(r9, new std::string("%r9"));
            regmap->Enter(r10, new std::string("%r10"));
            regmap->Enter(r11, new std::string("%r11"));
            regmap->Enter(rbx, new std::string("%rbx"));
            regmap->Enter(rbp, new std::string("%rbp"));
            regmap->Enter(r12, new std::string("%r12"));
            regmap->Enter(r13, new std::string("%r13"));
            regmap->Enter(r14, new std::string("%r14"));
            regmap->Enter(r15, new std::string("%r15"));
            regmap->Enter(rsp, new std::string("%rsp"));
            return regmap;
        }
    }


    std::string check_temp(int num) override {
        if (num == rax->Int()) return "%rax";
        if (num == rdi->Int()) return "%rdi";
        if (num == rsi->Int()) return "%rsi";
        if (num == rdx->Int()) return "%rdx";
        if (num == rcx->Int()) return "%rcx";
        if (num == r8->Int()) return "%r8";
        if (num == r9->Int()) return "%r9";
        if (num == r10->Int()) return "%r10";
        if (num == r11->Int()) return "%r11";
        if (num == rbx->Int()) return "%rbx";
        if (num == rbp->Int()) return "%rbp";
        if (num == r12->Int()) return "%r12";
        if (num == r13->Int()) return "%r13";
        if (num == r14->Int()) return "%r14";
        if (num == r15->Int()) return "%r15";
        if (num == rsp->Int()) return "%rsp";
        return "";
    }

    //caller-saved registers
    temp::Temp *rax, *rdi, *rsi, *rdx, *rcx, *r8, *r9, *r10, *r11;

    //callee-saved registers
    temp::Temp *rbx, *rbp, *r12, *r13, *r14, *r15;

    temp::Temp *fp;
    temp::Temp *rsp;

    temp::TempList *caller_saved_regiters, *callee_saved_registers, *registers, *args_registers, *ret_sink_registers, *allregs_noRSP;

    temp::Map *regmap = nullptr;
};

class InFrameAccess : public Access {
public:
    int offset;

    explicit InFrameAccess(int offset) : offset(offset) {}
    tree::Exp *ToExp(tree::Exp *framePtr) const {
        return tree::NewMemPlus_Const(framePtr, offset);
    }
};


class InRegAccess : public Access {
public:
    temp::Temp *reg;

    explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
    tree::Exp *ToExp(tree::Exp *framePtr) const {
        return new tree::TempExp(reg);
    }
};

class X64Frame : public Frame {
public:
    X64Frame(temp::Label *name, std::list<bool> *escapes) {
        label = name;
        s_offset = -reg_manager->WordSize();
        formals = std::list<frame::Access*>();
        if (escapes) {
            for (bool escape: *escapes) {
                formals.push_back(allocLocal(escape));
            }
        }
    }
    Access *allocLocal(bool escape) override {
        if (escape) {   // the variable need to be saved in memory(stack)
            auto inFrameAccess = new frame::InFrameAccess(s_offset);
            s_offset -= reg_manager->WordSize();
            return inFrameAccess;
        }
        else {  // the variable can be saved in register(when the register number is infinite)
            auto inRegAccess = new InRegAccess(temp::TempFactory::NewTemp());
            return inRegAccess;
        }
    }
};

tree::Stm *procEntryExit1(frame::Frame *frame, tree::Stm *stm);
assem::InstrList *procEntryExit2(assem::InstrList *body);
assem::Proc *procEntryExit3(frame::Frame *frame, assem::InstrList * body);
} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
