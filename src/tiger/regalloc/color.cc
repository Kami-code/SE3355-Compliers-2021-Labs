#include <iostream>
#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {


/* TODO: Put your lab6 code here */
#define DEBUG 1
#ifdef DEBUG
    #define LOG(format, args...) do{            \
    FILE *debug_log = fopen("register.log", "a+"); \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__); \
    fprintf(debug_log, format, ##args);       \
    fclose(debug_log);\
  } while(0)
#else
#define LOG(format, args...) do{} while(0)
#endif

std::unique_ptr<col::Result> Color::TransferResult() {
    return std::move(result);
}

graph::NodeList<temp::Temp> *Color::Adjacent(graph::Node<temp::Temp> *n) {
    return n->Succ()->Diff(selectStack->Union(coalescedNodes));
}

live::MoveList *Color::NodeMoves(graph::Node<temp::Temp> *n) {
    return moveListTab->Look(n)->Intersect(activeMoves->Union(worklistMoves));
}

bool Color::MoveRelated(graph::Node<temp::Temp> *n) {
    return NodeMoves(n);
}

void Color::AddEdge(graph::Node<temp::Temp> *u, graph::Node<temp::Temp> *v) {
    if (!u->Succ()->Contain(v) && u != v) {
        if (!colorTab->Look(u)) {
            livegraph_.interf_graph->AddEdge(u, v);
            (*degreeTab->Look(u))++;
        }
        if (!colorTab->Look(v)) {
            livegraph_.interf_graph->AddEdge(v, u);
            (*degreeTab->Look(v))++;
        }
    }
}

Color::Color(frame::Frame* f, assem::InstrList* il):livegraph_(nullptr, nullptr) {
    bool done = true;
    int cnt = 0;
    noSpillTemps = new temp::TempList();
    while(done){
        cnt++;
        if (cnt == 3) break;
        done = false;
        std::cout << "il = " << il << std::endl;
        fg::FlowGraphFactory fgf = fg::FlowGraphFactory(il);
        fgf.AssemFlowGraph();
        live::LiveGraphFactory lf = live::LiveGraphFactory(fgf.GetFlowGraph());
        lf.Liveness();
        livegraph_ = lf.GetLiveGraph();
        Build();
        MakeWorklist();

        while(simplifyWorklist->GetList().size() != 0 || worklistMoves->GetList().size() != 0 || freezeWorklist->GetList().size() != 0 || spillWorklist->GetList().size() != 0){
            if(simplifyWorklist->GetList().size() != 0) {
                Simplify();
            }

            else if(worklistMoves->GetList().size() != 0) {
                Coalesce();
            }

            else if(freezeWorklist->GetList().size() != 0) {
                Freeze();
            }

            else if(spillWorklist->GetList().size() != 0){
                SelectSpill();
            }
        }
        AssignColors();
        if(spilledNodes->GetList().size() != 0){
            std::cout << "before rewrite" << std::endl;
            il = RewriteProgram(f, il);
            std::cout << "rewrite done size = " << il->GetList().size()<< std::endl;
            done = true;
        }
        std::cout << "coloring done!" << "il = " << il << std::endl;
    }
    std::cout << "coloring size = " << il->GetList().size()<< std::endl;
    result = std::make_unique<col::Result>(AssignRegisters(Color::livegraph_), il);
}



void Color::Build() {
    simplifyWorklist = new graph::NodeList<temp::Temp>();
    freezeWorklist = new graph::NodeList<temp::Temp>();
    spillWorklist = new graph::NodeList<temp::Temp>();

    spilledNodes = new graph::NodeList<temp::Temp>();
    coalescedNodes = new graph::NodeList<temp::Temp>();
    coloredNodes = new graph::NodeList<temp::Temp>();

    selectStack = new graph::NodeList<temp::Temp>();

    coalescedMoves = new live::MoveList();
    constrainedMoves = new live::MoveList();
    frozenMoves = new live::MoveList();
    worklistMoves = livegraph_.moves;
    activeMoves = new live::MoveList();

    degreeTab = new graph::Table<temp::Temp, int>();
    moveListTab = new graph::Table<temp::Temp, live::MoveList>();
    aliasTab = new graph::Table<temp::Temp, graph::Node<temp::Temp>>();
    colorTab = new graph::Table<temp::Temp, std::string>();

    for (graph::Node<temp::Temp> *node : Color::livegraph_.interf_graph->Nodes()->GetList()) {
        degreeTab->Enter(node, new int(node->OutDegree()));
        colorTab->Enter(node, reg_manager->RegMap()->Look(node->NodeInfo()));
        aliasTab->Enter(node, node);
        auto moveList = new live::MoveList();
        for (auto move : worklistMoves->GetList()) {
            if (move.first == node || move.second == node) {
                moveList->Append(move.first, move.second);
            }
        }
        moveListTab->Enter(node, moveList);
    }
}

void Color::MakeWorklist() {
    for (graph::Node<temp::Temp> *node : Color::livegraph_.interf_graph->Nodes()->GetList()) {
        std::cout << "node = " << node << std::endl;
        if (colorTab->Look(node)) continue;
        if (*degreeTab->Look(node) >= K) {
            spillWorklist->Append(node);
        }
        else if (MoveRelated(node)) {
            freezeWorklist->Append(node);
        }
        else {
            simplifyWorklist->Append(node);
        }
    }
    std::cout << "make worklist over" << std::endl;
}

void Color::EnableMoves(graph::NodeList<temp::Temp> *nodes) {
    for (graph::Node<temp::Temp> *node : nodes->GetList()) {
        live::MoveList *m = NodeMoves(node);
        for (std::pair<graph::Node<temp::Temp>*, graph::Node<temp::Temp>*> pair : m->GetList()) {
            graph::Node<temp::Temp> *src = pair.first, *dst = pair.second;
            if (activeMoves->Contain(src, dst)) {
                activeMoves->Delete(src, dst);
            }
        }
    }
}

void Color::DecrementDegree(graph::Node<temp::Temp> *n) {
    (*degreeTab->Look(n))--;
    if(*degreeTab->Look(n) == K-1 && !colorTab->Look(n)){
        auto tmp = new graph::NodeList<temp::Temp>();
        tmp->Append(n);
        for (graph::Node<temp::Temp> * node : n->Succ()->GetList()) {
            tmp->Append(node);
        }
        EnableMoves(tmp);
        spillWorklist->DeleteNode(n);
        if(MoveRelated(n))
            freezeWorklist->Append(n);
        else
            simplifyWorklist->Append(n);
    }
}


void Color::Simplify() {
    std::cout << "before simplify" << std::endl;
    graph::Node<temp::Temp> *n = simplifyWorklist->GetList().back();
    simplifyWorklist->DeleteNode(n);
    selectStack->Prepend(n);
    for (graph::Node<temp::Temp> *node : n->Succ()->GetList()) {
        DecrementDegree(node);
    }
    std::cout << "after simplify" << std::endl;
}

void Color::AddWorkList(graph::Node<temp::Temp> *n) {
    if(!colorTab->Look(n) && !MoveRelated(n) && *degreeTab->Look(n) < K){
        freezeWorklist->DeleteNode(n);
        simplifyWorklist->Prepend(n);
    }
}

// George
bool Color::OK(graph::Node<temp::Temp> *v, graph::Node<temp::Temp> *u) {
    bool ok = true;
    for (auto node : v->Succ()->GetList()) {
        if (!(*degreeTab->Look(node) < K || colorTab->Look(node) || u->Succ()->Contain(node))) {
            ok = false;
            break;
        }
    }
    return ok;
}

bool Color::Conservative(graph::NodeList<temp::Temp> *nodes) {
    int k = 0;
    for (auto node : nodes->GetList()) {
        if (*degreeTab->Look(node) >= K) {
            k++;
        }
    }
    return k < K;
}
graph::Node<temp::Temp> *Color::GetAlias(graph::Node<temp::Temp> *n) {
    if(coalescedNodes->Contain(n))
        return GetAlias(aliasTab->Look(n));
    else
        return n;
}

void Color::Combine(graph::Node<temp::Temp> *u, graph::Node<temp::Temp> *v) {
    if (freezeWorklist->Contain(v))
        freezeWorklist->DeleteNode(v);
    else
        spillWorklist->DeleteNode(v);
    coalescedNodes->Prepend(v);
    aliasTab->Set(v, u);

    moveListTab->Set(u, moveListTab->Look(u)->Union(moveListTab->Look(v)));
    auto tmp = new graph::NodeList<temp::Temp>();
    tmp->Append(v);
    EnableMoves(tmp);

    for (auto node : v->Succ()->GetList()) {
        AddEdge(node, u);
        DecrementDegree(node);
    }
    if (*degreeTab->Look(u) >= K && freezeWorklist->Contain(u)) {
        freezeWorklist->DeleteNode(u);
        spillWorklist->Prepend(u);
    }
}

void Color::Coalesce() {
    std::cout << "start coalesce " << std::endl;

    graph::Node<temp::Temp> *x, *y, *u, *v;
    x = worklistMoves->GetList().front().first;
    y = worklistMoves->GetList().front().second;
    if(colorTab->Look(GetAlias(y))){
        u = GetAlias(y);
        v = GetAlias(x);
    } else {
        u = GetAlias(x);
        v = GetAlias(y);
    }
    worklistMoves->Delete(x, y);

    if(u == v){
        coalescedMoves->Prepend(x, y);
        AddWorkList(u);
    }
    else if(colorTab->Look(v) || v->Succ()->Contain(u)){
        constrainedMoves->Prepend(x, y);
        AddWorkList(u);
        AddWorkList(v);
    }
    else if((colorTab->Look(u) && OK(v, u)) || (!colorTab->Look(u) && Conservative(u->Succ()->Union(v->Succ())))){
        coalescedMoves->Prepend(x, y);
        Combine(u, v);
        AddWorkList(u);
    }
    else
        activeMoves->Prepend(x, y);
    std::cout << "end coalesce" << std::endl;
}

void Color::FreezeMoves(graph::Node<temp::Temp> *u) {
    for (auto m : NodeMoves(u)->GetList()) {
        auto x = m.first;
        auto y = m.second;
        graph::Node<temp::Temp> *v;
        if (GetAlias(y) == GetAlias(u)) {
            v = GetAlias(x);
        }
        else {
            v = GetAlias(y);
        }
        activeMoves->Delete(x, y);
        frozenMoves->Prepend(x, y);
        if (!NodeMoves((v))&& *degreeTab->Look(v) < K) {
            freezeWorklist->DeleteNode(v);
            simplifyWorklist->Prepend(v);
        }
    }
}

void Color::Freeze() {
    std::cout << "start freeze" << std::endl;
    graph::Node<temp::Temp> *u = freezeWorklist->GetList().front();
    freezeWorklist->DeleteNode(u);
    simplifyWorklist->Append(u);
    FreezeMoves(u);
    std::cout << "end freeze" << std::endl;
}

void Color::SelectSpill() {
    std::cout << "start select spill" << std::endl;

    graph::Node<temp::Temp> *m = nullptr;
    int maxweight = 0;
    for (graph::Node<temp::Temp> *node : spillWorklist->GetList()) {
        std::cout << "in select spill loop" << std::endl;
        if (!node) continue;
        std::cout << "noSpillTemps = " << noSpillTemps << "node->infor = " << node->NodeInfo()->Int() << std::endl;
        if(noSpillTemps && live::Contain(noSpillTemps, node->NodeInfo()))
            continue;
        std::cout << "in select spill middle" << std::endl;
        if (*degreeTab->Look(node) > maxweight) {
            m = node;
            maxweight = *degreeTab->Look(node);
        }
        std::cout << "in select spill loop end" << std::endl;
    }
    std::cout << "out select spill loop" << std::endl;
    assert(m);
    std::cout << "try spilling the node with int = " << m->NodeInfo()->Int() << std::endl;
    spillWorklist->DeleteNode(m);
    simplifyWorklist->Append(m);
    FreezeMoves(m);
    std::cout << "end select spill" << std::endl;
}

void Color::AssignColors() {
    std::cout << "start assign color" << std::endl;
    tab::Table<std::string, bool> okColors;
    for (temp::Temp *t : reg_manager->Allregs_noRSP()->GetList()) {
        okColors.Enter(reg_manager->RegMap()->Look(t), new bool(true));
    }


    for (graph::Node<temp::Temp> *n : selectStack->GetList()) {
        //pop the nodes in the stack
        for (temp::Temp *t : reg_manager->Allregs_noRSP()->GetList()) {
            okColors.Set(reg_manager->RegMap()->Look(t), new bool(true));
        }
        //initialize the okColor to true
        for (graph::Node<temp::Temp> *node : n->Succ()->GetList()) {
            if (colorTab->Look(GetAlias(node))) {
                okColors.Set(colorTab->Look(GetAlias(node)), new bool(false));
            }
        }

        bool realspill = true;
        temp::Temp *t = nullptr;
        for (temp::Temp *real_t : reg_manager->Allregs_noRSP()->GetList()) {
            t = real_t;
            if (reg_manager->RegMap()->Look(real_t) == 0) {
                break;
            }
            if (*okColors.Look(reg_manager->RegMap()->Look(real_t))) {
                realspill = false;

                std::cout << "find color" << reg_manager->RegMap()->Look(real_t) << "not really spill the node with int = " << n->NodeInfo()->Int() << std::endl;
                break;
            }
        }
        if (realspill) {
            spilledNodes->Append(n);
        }
        else {
            coloredNodes->Append(n);
            std::cout << "node = " << n->NodeInfo()->Int() << " color = " << reg_manager->RegMap()->Look(t) << std::endl;
            if (reg_manager->RegMap()->Look(t) == nullptr) continue;
            colorTab->Set(n, reg_manager->RegMap()->Look(t));
        }
    }

    for (graph::Node<temp::Temp> *node : coalescedNodes->GetList()) {
        colorTab->Set(node, colorTab->Look(GetAlias(node)));
    }
    for (graph::Node<temp::Temp> *node : spilledNodes->GetList()) {
        std::cout << "spilling node  = " << node->NodeInfo()->Int() << std::endl;
    }
    std::cout << "after assign colors" << std::endl;
}

temp::TempList *replaceTempList(temp::TempList *list, temp::Temp *oldd, temp::Temp *neww) {
    auto result = new temp::TempList();
    for (temp::Temp *reg : list->GetList()) {
        if (reg == oldd) {
            result->Append(neww);
        }
        else {
            result->Append(reg);
        }
    }
    return result;
}

assem::InstrList *Color::RewriteProgram(frame::Frame *f, assem::InstrList *il) {

    noSpillTemps = new temp::TempList();
    temp::Map * coloring = AssignRegisters(livegraph_);
    for (graph::Node<temp::Temp> *node : spilledNodes->GetList()) {
        temp::Temp *spilledtemp = node->NodeInfo();

        f->s_offset -= 8;
        auto after_change = new assem::InstrList();
        for (assem::Instr *instr : il->GetList()) {
            std::string str = "";
            temp::TempList *tempList_src = nullptr, *tempList_dst = nullptr;
            auto operInstr = dynamic_cast<assem::OperInstr *>(instr);
            auto moveInstr = dynamic_cast<assem::MoveInstr *>(instr);
            if (operInstr) {
                std::cout << "oper _assem = " << operInstr->assem_ << std::endl;
                tempList_src = operInstr->src_;
                tempList_dst = operInstr->dst_;
                if (tempList_src && tempList_dst && (tempList_src->GetList().size() != 0) && (tempList_dst->GetList().size() != 0))
                if (coloring->Look(tempList_src->GetList().front()) == coloring->Look(tempList_dst->GetList().front())) {
                    after_change->Append(instr);
                    continue;
                }
            }
            else if (moveInstr) {
                std::cout << "move _assem = " << moveInstr->assem_ << std::endl;
                tempList_src = moveInstr->src_;
                tempList_dst = moveInstr->dst_;
                if (tempList_src && tempList_dst && (tempList_src->GetList().size() != 0) && (tempList_dst->GetList().size() != 0))
                if (coloring->Look(tempList_src->GetList().front()) == coloring->Look(tempList_dst->GetList().front())) {
                    after_change->Append(instr);
                    continue;
                }
            }
            else {
                after_change->Append(instr);
                continue;
            }

            temp::Temp *neww = nullptr;
            if (tempList_src && live::Contain(tempList_src, spilledtemp)) {
                //src_templist中包含spilledtemp的情况
                neww = temp::TempFactory::NewTemp();
                noSpillTemps->Append(neww);
                std::cout << "add " << neww->Int() << " into nospilltemp" << std::endl;
                if (moveInstr) {
                    moveInstr->src_ = replaceTempList(tempList_src, spilledtemp, neww);
                }
                else if (operInstr) {
                    operInstr->src_ = replaceTempList(tempList_src, spilledtemp, neww);
                }
                //move spilledtemp, %machine_register - > move new_temp, %machine_register
                std::cout << "尝试把 " << spilledtemp->Int() << " 替换为 "<< neww->Int() << std::endl;
                str += "# Ops! Spilled before\n";
                str += "movq (" + f->label->Name() + "_framesize-" + std::to_string(-f->s_offset) + ")(`s0), `d0";
                auto newInstr = new assem::OperInstr(str, new temp::TempList(neww), new temp::TempList(reg_manager->StackPointer()),
                                                      nullptr);
                //move (memory on stack), new_temp
                after_change->Append(newInstr);
                after_change->Append(instr);
            }


            else if(tempList_dst && live::Contain(tempList_dst, spilledtemp)){
                neww = temp::TempFactory::NewTemp();
                noSpillTemps->Append(neww);
                std::cout << "add " << neww->Int() << " into nospilltemp" << std::endl;
                if (moveInstr) {
                    moveInstr->dst_ = replaceTempList(tempList_dst, spilledtemp, neww);
                }
                else if (operInstr) {
                    operInstr->dst_ = replaceTempList(tempList_dst, spilledtemp, neww);
                }
                //move %machine_register,spilledtemp - > move %machine_register,new_temp
                std::cout << "尝试把 " << spilledtemp->Int() << " 替换为 "<< neww->Int() << std::endl;
                str += "# Ops! Spill after\n";
                str += "movq `s0, (" + f->label->Name() + "_framesize-" + std::to_string(-f->s_offset) + ")(`d0)";
                auto newinstr = new assem::OperInstr(str, new temp::TempList(reg_manager->StackPointer()), new temp::TempList(neww),
                                                     nullptr);
                after_change->Append(instr);
                after_change->Append(newinstr);
            }
            else {
                after_change->Append(instr);
            }
        }
        std::cout << "il = " << il->GetList().size() << " after_change = " << after_change->GetList().size() << std::endl;
        il = after_change;
    }
    spilledNodes = new graph::NodeList<temp::Temp>();
    return il;
}


temp::Map *Color::AssignRegisters(live::LiveGraph g) {
    LOG("AssignRegisters\n");
    std::cout << "assign registers" << std::endl;
    temp::Map *res = temp::Map::Empty();
    res->Enter(reg_manager->StackPointer(), new std::string("%rsp"));
    for (graph::Node<temp::Temp> *node : g.interf_graph->Nodes()->GetList()) {
        res->Enter(node->NodeInfo(), colorTab->Look(node));
        std::cout << "node info = " << node->NodeInfo()->Int() << " color = " << colorTab->Look(node) << std::endl;
    }
    return temp::Map::LayerMap(reg_manager->RegMap(), res);
}

} // namespace col
