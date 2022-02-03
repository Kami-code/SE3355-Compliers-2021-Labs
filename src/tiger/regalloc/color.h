#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

namespace col {
#define K 15

struct Result {
    Result() : coloring(nullptr), /*spills(nullptr),  */instrList(nullptr) {}
    Result(temp::Map *coloring,/* live::INodeListPtr spills, */ assem::InstrList *instrlist)
      : coloring(coloring),/* spills(spills), */ instrList(instrlist) {}
    temp::Map *coloring;
//    live::INodeListPtr spills;
    assem::InstrList *instrList;
};


class Color {
public:
    graph::NodeList<temp::Temp> *simplifyWorklist;
    graph::NodeList<temp::Temp> *freezeWorklist;
    graph::NodeList<temp::Temp> *spillWorklist;

    graph::NodeList<temp::Temp> *spilledNodes;
    graph::NodeList<temp::Temp> *coalescedNodes;
    graph::NodeList<temp::Temp> *coloredNodes;

    graph::NodeList<temp::Temp> *selectStack;


    live::MoveList *coalescedMoves;
    live::MoveList *constrainedMoves;
    live::MoveList *frozenMoves;
    live::MoveList *worklistMoves;
    live::MoveList *activeMoves;

    graph::Table<temp::Temp, int> *degreeTab;
    graph::Table<temp::Temp, live::MoveList> *moveListTab;
    graph::Table<temp::Temp, graph::Node<temp::Temp>> *aliasTab;
    graph::Table<temp::Temp, std::string> *colorTab;

    temp::TempList *noSpillTemps;

    live::LiveGraph livegraph_;
    std::unique_ptr<col::Result> result;

    std::unique_ptr<col::Result> TransferResult();
    Color(frame::Frame* f, assem::InstrList* il);
    bool MoveRelated(graph::Node<temp::Temp> *n);
    void MakeWorklist();
    void EnableMoves(graph::NodeList<temp::Temp> *nodes);
    void DecrementDegree(graph::Node<temp::Temp> *n);
    void Simplify();
    void AddWorkList(graph::Node<temp::Temp> *n);
    void AssignColors();
    temp::Map * AssignRegisters(live::LiveGraph g);
    assem::InstrList *RewriteProgram(frame::Frame *f, assem::InstrList *il);
    void SelectSpill();
    void Freeze();
    graph::NodeList<temp::Temp> *Adjacent(graph::Node<temp::Temp> *n);
    void AddEdge(graph::Node<temp::Temp> *u, graph::Node<temp::Temp> *v);
    void Build();

    void StringBoolTableShow(std::string *s, bool *b);
    void NodeStringTableShow(graph::Node<temp::Temp> *n, std::string *s);
    void NodeIntTableShow(graph::Node<temp::Temp> *n, int *s);
    void NodeNodeTableShow(graph::Node<temp::Temp> *n, graph::Node<temp::Temp> *s);
    void NodeMoveTableShow(graph::Node<temp::Temp> *n, live::MoveList *s);
    void TempGraphShow(FILE *out, temp::Temp *t);
    void ShowStatus();
    bool OK(graph::Node<temp::Temp> *v, graph::Node<temp::Temp> *u);
    bool Conservative(graph::NodeList<temp::Temp> *nodes);
    graph::Node<temp::Temp> *GetAlias(graph::Node<temp::Temp> *n);
    void Combine(graph::Node<temp::Temp> *u, graph::Node<temp::Temp> *v);
    void Coalesce();
    void FreezeMoves(graph::Node<temp::Temp> *u);
    live::MoveList *NodeMoves(graph::Node<temp::Temp> *n);
};



} // namespace col

#endif // TIGER_COMPILER_COLOR_H
