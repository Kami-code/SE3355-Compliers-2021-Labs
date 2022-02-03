#include <iostream>
#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
assem::InstrList *RemoveMoveInstr(assem::InstrList *il, temp::Map *color) {
    //if the src and the dst of a moveinstr is marked with the same color, just remove the moveinstr
    auto res = new assem::InstrList();
    for (auto instr : il->GetList()) {
        auto moveInstr = dynamic_cast<assem::MoveInstr *>(instr);
        if (moveInstr) {
            if (color->Look(moveInstr->src_->GetList().front()) == color->Look(moveInstr->dst_->GetList().front())) {
                if (moveInstr->assem_ == "cqto") {}
                else {
                    moveInstr->assem_ = std::string("# remove ").append(moveInstr->assem_);
                }

            }
        }
        res->Append(instr);
    }
    return res;
}

void RegAllocator::RegAlloc() {
    col::Color color = col::Color(frame_, il_->GetInstrList());
    std::unique_ptr<col::Result> colResult = color.TransferResult();
    std::cout << "in regalloc.cc, instruction_list_size = " << colResult->instrList->GetList().size() << std::endl;
    result_ = std::make_unique<ra::Result>(ra::Result(colResult->coloring, RemoveMoveInstr(colResult->instrList, colResult->coloring)));
    std::cout << "in regalloc.cc, instruction_list_size = " << result_->il_->GetList().size() << std::endl;
}

std::unique_ptr<ra::Result> RegAllocator::TransferResult() {
    return std::move(result_);
}

} // namespace ra