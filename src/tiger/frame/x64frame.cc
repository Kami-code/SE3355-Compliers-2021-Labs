#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
tree::Exp *externalCall(std::string s, tree::ExpList *args) {
    return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
}

tree::Stm *procEntryExit1(frame::Frame *frame, tree::Stm *stm) {
    int num = 1;
    tree::Stm *viewshift = new tree::ExpStm(new tree::ConstExp(0));
    frame->formals.reverse();
    num = frame->formals.size();
    for (frame::Access *formal : frame->formals) {
        if (num <= 6) {
            viewshift = new tree::SeqStm(viewshift, new tree::MoveStm(formal->ToExp(new tree::TempExp(reg_manager->FramePointer())), new tree::TempExp(reg_manager->ARG_nth(num))));
        }
        else {
            viewshift = new tree::SeqStm(viewshift, new tree::MoveStm(formal->ToExp(new tree::TempExp(reg_manager->FramePointer())), tree::NewMemPlus_Const(new tree::TempExp(reg_manager->FramePointer()), -1 * (num - 6) * reg_manager->WordSize())));
        }
        num--;
    }
    return new tree::SeqStm(viewshift, stm);
//    int num = 1;
//    tree::Stm *viewshift = new tree::ExpStm(new tree::ConstExp(0));
//    int total_size = frame->formals.size();
//    std::vector<temp::Temp *> tempList;
//    for (frame::Access *formal : frame->formals) {
//        temp::Temp *reg = temp::TempFactory::NewTemp();
//        if (num <= 6) {
//            viewshift = new tree::SeqStm(viewshift, new tree::MoveStm(new tree::TempExp(reg), new tree::TempExp(reg_manager->ARG_nth(num))));
//        }
//        else {
//            viewshift = new tree::SeqStm(viewshift, new tree::MoveStm(new tree::TempExp(reg), tree::NewMemPlus_Const(new tree::TempExp(reg_manager->FramePointer()), -1 * (num - 6) * reg_manager->WordSize())));
//        }
//        tempList.push_back(reg);
//        num++;
//    }
//    num = 1;
//    for (frame::Access *formal : frame->formals) {
//        viewshift = new tree::SeqStm(viewshift, new tree::MoveStm(formal->ToExp(new tree::TempExp(reg_manager->FramePointer())), new tree::TempExp(tempList[num - 1])));
//        num++;
//    }
//    return new tree::SeqStm(viewshift, stm);
}

assem::InstrList *procEntryExit2(assem::InstrList *body) {
    temp::TempList *returnSink = reg_manager->ReturnSink();
    body->Append(new assem::OperInstr("", nullptr, returnSink, nullptr));
    return body;
}

assem::Proc *procEntryExit3(frame::Frame *frame, assem::InstrList * body) {
    static char instr[256];

    std::string prolog;
    int size = -frame->s_offset - 8;
    sprintf(instr, ".set %s_framesize, %d\n", frame->label->Name().c_str(), size);
    prolog = std::string(instr);
    sprintf(instr, "%s:\n", frame->label->Name().c_str());
    prolog.append(std::string(instr));
    sprintf(instr, "subq $%d, %%rsp\n", size);
    prolog.append(std::string(instr));

    sprintf(instr, "addq $%d, %%rsp\n", size);
    std::string epilog = std::string(instr);
    epilog.append(std::string("retq\n"));
    return new assem::Proc(prolog, body, epilog);
}
} // namespace frame