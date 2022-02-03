#include "tiger/liveness/flowgraph.h"
#include <iostream>

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
    std::cout << "start AssemFlowGraph" << std::endl;
    for (auto instr : instr_list_->GetList()) {
        graph::Node<assem::Instr> *prev = nullptr;
        if (flowgraph_->Nodes()->GetList().size() != 0) {
            prev = flowgraph_->Nodes()->GetList().back();
        }
        auto node = flowgraph_->NewNode(instr);

        if (prev && !IsJmp(prev)) {
            flowgraph_->AddEdge(prev, node);
        }

        auto label_instr = dynamic_cast<assem::LabelInstr *>(instr);
        if (label_instr) {
            label_map_->Enter(label_instr->label_, node);
        }
    }

    for (auto node : flowgraph_->Nodes()->GetList()) {
        if (dynamic_cast<assem::OperInstr *>(node->NodeInfo()) == nullptr || !dynamic_cast<assem::OperInstr *>(node->NodeInfo())->jumps_)
            continue;
        for (auto label : *dynamic_cast<assem::OperInstr *>(node->NodeInfo())->jumps_->labels_) {
            if (!label) break;  //some instruction will add a nullptr at last
            flowgraph_->AddEdge(node, label_map_->Look(label));
        }
    }
    std::cout << "out AssemFlowGraph" << std::endl;
}


bool IsMove(graph::Node<assem::Instr>* n) {
    return dynamic_cast<assem::MoveInstr *>(n->NodeInfo()) != nullptr;
}

bool IsJmp(graph::Node<assem::Instr>* n) {
    assem::Instr *instr = n->NodeInfo();
    return dynamic_cast<assem::OperInstr *>(instr) && dynamic_cast<assem::OperInstr *>(instr)->assem_.find("jmp") != std::string::npos;
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
    return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
    if (dst_) return dst_;
    else return new temp::TempList();
}

temp::TempList *OperInstr::Def() const {
    if (dst_) return dst_;
    else return new temp::TempList();
}

temp::TempList *LabelInstr::Use() const {
    return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
    if (src_) return src_;
    else return new temp::TempList();
}

temp::TempList *OperInstr::Use() const {
    if (src_) return src_;
    else return new temp::TempList();
}

} // namespace assem
