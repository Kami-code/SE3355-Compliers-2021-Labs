#include <iostream>
#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
      return std::any_of(move_list_->cbegin(), move_list_->cend(),
                         [src, dst](std::pair<INodePtr, INodePtr> move) {
                           return move.first == src && move.second == dst;
                         });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_->begin();
  for (; move_it != move_list_->end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_->erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
    assert(list);
    auto res = new MoveList();
    for (auto move : GetList()) {
        res->move_list_->push_back(move);
    }
    for (auto move : list->GetList()) {
        if (!res->Contain(move.first, move.second))
            res->move_list_->push_back(move);
    }
    return res;
}

    MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_->push_back(move);
  }
  return res;
}

temp::TempList *Union(temp::TempList *left, temp::TempList *right) {
    auto unio = new temp::TempList();
    for (temp::Temp *reg : left->GetList()) {
        unio->Append(reg);
    }
    for (temp::Temp *reg : right->GetList()) {
        bool hasTheSame = false;
        for (temp::Temp *reg2 : left->GetList()) {
            if (reg == reg2) {
                hasTheSame = true;
                break;
            }
        }
        if (!hasTheSame) {
            unio->Append(reg);
        }
    }
    return unio;
}

temp::TempList *Difference(temp::TempList *left, temp::TempList *right) {
    auto diff = new temp::TempList();
    for (temp::Temp *reg : left->GetList()) {
        if(!Contain(right, reg))
            diff->Append(reg);
    }
    return diff;
}

bool Contain(temp::TempList *container, temp::Temp *temp) {
//    std::cout << "in Contain" << std::endl;
    if (!container) {
//        std::cout << "out Contain" << std::endl;
        return false;
    }
//    std::cout << "in middle contain " << container <<  " " << container->GetList().size() << std::endl;
//    std::cout << "first element = " << container->GetList().front() << std::endl;
    for (auto reg : container->GetList()) {
//        std::cout << "loop Contain reg = " << reg << std::endl;
        if (reg == temp) {
//            std::cout << "out Contain" << std::endl;
            return true;
        }
    }
//    std::cout << "out Contain" << std::endl;
    return false;
}

bool Equal(temp::TempList *left, temp::TempList *right) {
    for (temp::Temp *reg : left->GetList()) {
        if(!Contain(right, reg))
            return false;
    }
    for (temp::Temp *reg : right->GetList()) {
        if(!Contain(left, reg))
            return false;
    }
    return true;
}

void LiveGraphFactory::LiveMap() {
    for (graph::Node<assem::Instr> *node : flowgraph_->Nodes()->GetList()) {
        in_->Enter(node, new temp::TempList());
        out_->Enter(node, new temp::TempList());
    }

    /*
     * repeat:
     *    for each node:
     *        in[node] <- use[node] U (out[node] - def[node])
     *        for n in succ[node]:
     *            out[node] <- out[node] U in[n]
     * until fixedpoint
     */
//    LOG("Liveness: Find Fixed Point\n");
    bool fixedpoint = false;
    int cnt = 0;
    while(!fixedpoint){
        cnt ++;
        std::cout << "fixed point in " << cnt << std::endl;
        fixedpoint = true;
        std::list<graph::Node<assem::Instr> *> reversedList = flowgraph_->Nodes()->GetList();
        reversedList.reverse();
        std::cout << "reversedList.size = " << reversedList.size() << std::endl;
        for (graph::Node<assem::Instr> *node : reversedList) {
            temp::TempList *oldin_templist = in_->Look(node);
            temp::TempList *oldout_templist = out_->Look(node);
            std::cout << "before out_.size = " << in_->Look(node)->GetList().size() << std::endl;
            std::cout << "before in_.size = " << out_->Look(node)->GetList().size() << std::endl;
            for (graph::Node<assem::Instr> *succ : node->Succ()->GetList()) {
                out_->Set(node, Union(out_->Look(node), in_->Look(succ)));
            }

            std::cout << "middle out_.size = " << out_->Look(node)->GetList().size() << std::endl;
            in_->Set(node, Union(node->NodeInfo()->Use(), Difference(out_->Look(node), node->NodeInfo()->Def())));
            std::cout << "middle in_.size = " << in_->Look(node)->GetList().size() << std::endl;
            if (!Equal(oldin_templist, in_->Look(node)) || !Equal(oldout_templist, out_->Look(node))) {
                fixedpoint = false;
            }
        }
    }
    std::cout << "LiveMap out " << cnt << std::endl;
}

void LiveGraphFactory::InterfGraph() {
    std::cout << "InterfGraph in " << std::endl;
    live_graph_.moves = new MoveList();
    IGraphPtr g = live_graph_.interf_graph;

    // step 1
    for(temp::Temp *reg : reg_manager->Allregs_noRSP()->GetList()){ //all the machine register has a node (except rsp)
        graph::Node<temp::Temp> *node = g->NewNode(reg);
        temp_node_map_->Enter(reg, node);
    }
    int count1 = 0, count2 = 0;
    for(temp::Temp *reg1 : reg_manager->Allregs_noRSP()->GetList()) {  //fully-linked machine register
        count2 = 0;
        for (temp::Temp *reg2 : reg_manager->Allregs_noRSP()->GetList()) {
            if (count2 > count1) {
                g->AddEdge(temp_node_map_->Look(reg1), temp_node_map_->Look(reg2));
                g->AddEdge(temp_node_map_->Look(reg2), temp_node_map_->Look(reg1));
            }
            count2++;
        }
        count1++;
    }
    std::cout << "InterfGraph step1 over " << std::endl;
    // step 2: put all the temporary register in to temp_node_map
    for (graph::Node<assem::Instr> *node : flowgraph_->Nodes()->GetList()) {
        temp::TempList *list = Union(out_->Look(node), node->NodeInfo()->Def());
        for (temp::Temp *reg : list->GetList()) {
            if (reg == reg_manager->StackPointer()) continue;
            if (temp_node_map_->Look(reg) == nullptr) {
                graph::Node<temp::Temp> *tmp_node = g->NewNode(reg);
                temp_node_map_->Enter(reg, tmp_node);
            }
        }
    }
    std::cout << "InterfGraph step1.5 over " << std::endl;
    for (graph::Node<assem::Instr> *node : flowgraph_->Nodes()->GetList()) {
        if (!fg::IsMove(node)) { // non-move instr
            for (temp::Temp *def : node->NodeInfo()->Def()->GetList()) {
                for (temp::Temp *out : out_->Look(node)->GetList()) {
                    if (def == reg_manager->StackPointer() || out == reg_manager->StackPointer()) continue;
                    g->AddEdge(temp_node_map_->Look(def), temp_node_map_->Look(out));
                    g->AddEdge(temp_node_map_->Look(out), temp_node_map_->Look(def));
                }
            }
        }
        else { // move instr
            for (temp::Temp *def : node->NodeInfo()->Def()->GetList()) {
                for (temp::Temp *out : Difference(out_->Look(node), node->NodeInfo()->Use())->GetList()) {
                    if (def == reg_manager->StackPointer() || out == reg_manager->StackPointer()) continue;
                    g->AddEdge(temp_node_map_->Look(def), temp_node_map_->Look(out));
                    g->AddEdge(temp_node_map_->Look(out), temp_node_map_->Look(def));
                }
                for (temp::Temp *use : node->NodeInfo()->Use()->GetList()) {
                    if (def == reg_manager->StackPointer() || use == reg_manager->StackPointer()) continue;
                    if (!live_graph_.moves->Contain(temp_node_map_->Look(def), temp_node_map_->Look(use))) {
                        live_graph_.moves->Append(temp_node_map_->Look(def), temp_node_map_->Look(use));
                    }
                }
            }
        }
    }
    std::cout << "InterfGraph over " << std::endl;
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live