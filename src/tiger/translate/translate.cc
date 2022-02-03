#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>
#include <tiger/semant/semant.h>
#include <iostream>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {


tr::Access *tr::Access::AllocLocal(Level *level, bool escape) {
    return new Access(level, level->frame_->allocLocal(escape));
}

Level *Level::NewLevel(Level *parent, temp::Label *name, std::list<bool> *formals) {
    formals->push_back(true);  //static link
    int count = 1;
    for (bool &formal: *formals) {
        count++;
        if (count > 6) {
            formal = true;
        }
    }
    return new Level(new frame::X64Frame(name, formals), parent);
}

void do_patch(temp::Label **tList, temp::Label *label) {
    for (int i = 0; ; i++) {
        if (tList[i] != nullptr) {
            tList[i] = label;
        }
        else break;
    }
}

temp::Label **join_patch(temp::Label **first, temp::Label **second) {
    if (first[0] == nullptr) return second;
    else {
        int first_length = 0, second_length = 0;
        for (; ; first_length++) {
            if (first[first_length] != nullptr) continue;
            else break;
        }
        for (; ; second_length++) {
            if (second[second_length] != nullptr) continue;
            else break;
        }
        temp::Label **result = new temp::Label*[first_length + second_length + 1];
        for (int i = 0; i < first_length; i++) result[i] = first[i];
        for (int i = 0; i < second_length; i++) result[i + first_length] = second[i];
        result[first_length + second_length] = nullptr;
        return result;
    }
}

class Cx {
public:
  temp::Label **trues_;
  temp::Label **falses_;
  tree::Stm *stm_;

  Cx(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() const = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() const = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) const = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
      return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
      return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
      auto stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
      auto trues = new temp::Label*[2];
      auto falses = new temp::Label*[2];
      trues[0] = stm->true_label_; trues[1] = nullptr;
      falses[0] = stm->false_label_; falses[1] = nullptr;
      return Cx(trues, falses, stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
      return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
      return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
//      errormsg->Error(0, "Error: nx can't be a test exp.");
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(temp::Label** trues, temp::Label** falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  CxExp(Cx cx_):cx_(cx_) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
      temp::Temp *r = temp::TempFactory::NewTemp();
      temp::Label *t = temp::LabelFactory::NewLabel(), *f = temp::LabelFactory::NewLabel();
      do_patch(cx_.trues_, t);
      do_patch(cx_.falses_, f);

      auto cjumpStm = dynamic_cast<tree::CjumpStm *>(cx_.stm_);
      cjumpStm->true_label_ = t;
      cjumpStm->false_label_ = f;

      return new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
                            new tree::EseqExp(cx_.stm_,
                                           new tree::EseqExp(new tree::LabelStm(f),
                                                          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(0)),
                                                                         new tree::EseqExp(new tree::LabelStm(t), new tree::TempExp(r))))));

  }
  [[nodiscard]] tree::Stm *UnNx() const override {
      temp::Label *label = temp::LabelFactory::NewLabel();
      do_patch(cx_.trues_, label);
      do_patch(cx_.falses_, label);

      auto cjumpStm = dynamic_cast<tree::CjumpStm *>(cx_.stm_);
      cjumpStm->true_label_ = label;
      cjumpStm->false_label_ = label;

      return new tree::SeqStm(cx_.stm_, new tree::LabelStm(label));
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
      return cx_;
  }
};

Level *Outermost() {
    return new Level(new frame::X64Frame(temp::LabelFactory::NamedLabel("tigermain"), nullptr), nullptr);
}

void ProgTr::Translate() {

    temp::Label *mainlabel = temp::LabelFactory::NamedLabel("tigermain");
    main_level_= std::make_unique<tr::Level>(*Outermost());
    Level *mainframe = main_level_.get();
    FillBaseVEnv();
    FillBaseTEnv();
    ExpAndTy *mainexp = absyn_tree_->Translate(venv_.get(), tenv_.get(), mainframe, mainlabel, errormsg_.get());
//    mainexp->exp_->
//    frags->PushBack(new frame::ProcFrag());
    errormsg_->any_errors_ = false;
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg)  {
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tree::Exp *StaticLink(tr::Level *target, tr::Level *level) {
    tree::Exp *staticlink = new tree::TempExp(reg_manager->FramePointer());
    while(level != target){
        frame::Access * sl = level->frame_->formals.back();
        staticlink = sl->ToExp(staticlink);
        level = level->parent_;
    }
    return staticlink;
}

tr::Exp *TranslateSimpleVar(tr::Access *access, tr::Level *level) {
    tree::Exp *real_fp = StaticLink(access->level_, level);
    return new tr::ExExp(access->access_->ToExp(real_fp));
}

tr::Exp *TranslateAssignExp(tr::Exp *var, tr::Exp *exp) {
    return new tr::NxExp(new tree::MoveStm(var->UnEx(), exp->UnEx()));
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg)  {
    tr::Exp *exp = nullptr;
    env::EnvEntry *entry = venv->Look(sym_);
    assert(entry);
    if (entry && (typeid(*entry) == typeid(env::VarEntry))) {
        auto varEntry = dynamic_cast<env::VarEntry *>(entry);
//        errormsg->Error(pos_, "variable = " + sym_->Name());
        return new tr::ExpAndTy(TranslateSimpleVar(varEntry->access_, level), type);
    }
    else {
        //errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
        return new tr::ExpAndTy(nullptr, type);
    }

}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg)  {
    tr::ExpAndTy *check_var = var_->Translate(venv, tenv, level, label, errormsg);
    if (check_var->ty_ == nullptr) {
        return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type);
    }
    type::Ty *actual_ty = check_var->ty_->ActualTy();

    type::FieldList *fields = ((type::RecordTy *)actual_ty)->fields_;
    int order = 0;
    for (type::Field *field:fields->GetList()) {
        if(field->name_ == sym_){
            tr::Exp *exp = new tr::ExExp(tree::NewMemPlus_Const(check_var->exp_->UnEx(), order * reg_manager->WordSize()));
            return new tr::ExpAndTy(exp, type);
        }
        order++;
    }
    return new tr::ExpAndTy(nullptr, type);
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg)  {
    tr::ExpAndTy *check_var = var_->Translate(venv, tenv, level, label, errormsg);
    tr::ExpAndTy *check_subscript = subscript_->Translate(venv, tenv, level, label, errormsg);
    tr::Exp *exp = new tr::ExExp(new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, check_var->exp_->UnEx(), new tree::BinopExp(tree::BinOp::MUL_OP, check_subscript->exp_->UnEx(), new tree::ConstExp(reg_manager->WordSize())))));
    return new tr::ExpAndTy(exp, type);
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg)  {
    return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg)  {
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type);
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg)  {
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)), type);
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg)  {
    temp::Label *string_label = temp::LabelFactory::NewLabel();
    auto *stringFrag = new frame::StringFrag(string_label, str_);
    frags->PushBack(stringFrag);
    return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(string_label)), type);
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg)  {
    tr::Exp *exp = nullptr;
    env::EnvEntry *entry = nullptr;
    if (func_) entry = venv->Look(func_);
    auto fun_entry = dynamic_cast<env::FunEntry *>(entry);
    if (!fun_entry) {
        return new tr::ExpAndTy(nullptr, type);
    }
    auto *list = new tree::ExpList();
    for (Exp* args_p:args_->GetList()) {
        tr::ExpAndTy *check_arg = args_p->Translate(venv, tenv, level, label, errormsg);
        list->Append(check_arg->exp_->UnEx());
    }
    if(!fun_entry->level_->parent_) {
        exp = new tr::ExExp(frame::externalCall(func_->Name(), list));
    }
    else {
        list->Append(StaticLink(fun_entry->level_->parent_, level));
        exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(func_), list));
    }

    return new tr::ExpAndTy(exp, type);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg)  {
    tr::ExpAndTy *leftExpAndTy = left_->Translate(venv, tenv, level, label, errormsg);
    tr::ExpAndTy *rightExpAndTy = right_->Translate(venv, tenv, level, label, errormsg);
    tree::Exp *leftExp = leftExpAndTy->exp_->UnEx();
    tree::Exp *rightExp = rightExpAndTy->exp_->UnEx();
    tr::Exp *exp = nullptr;
    switch(oper_) {
        case PLUS_OP:
        case MINUS_OP:
        case TIMES_OP:
        case DIVIDE_OP:
        {
            switch (oper_) {
                case PLUS_OP:
                    return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp((tree::PLUS_OP), leftExp, rightExp)), type);
                case MINUS_OP:
                    return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp((tree::MINUS_OP), leftExp, rightExp)), type);
                case TIMES_OP:
                    return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp((tree::MUL_OP), leftExp, rightExp)), type);
                case DIVIDE_OP:
                    return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp((tree::DIV_OP), leftExp, rightExp)), type);
            }
            break;
        }
        case LT_OP:
        case LE_OP:
        case GT_OP:
        case GE_OP:
        {
            tree::CjumpStm *stm;
            switch (oper_) {
                case Oper::LT_OP:
                    stm = new tree::CjumpStm(tree::RelOp::LT_OP, leftExp, rightExp, nullptr, nullptr);
                    break;
                case Oper::LE_OP:
                    stm = new tree::CjumpStm(tree::RelOp::LE_OP, leftExp, rightExp, nullptr, nullptr);
                    break;
                case Oper::GT_OP:
                    stm = new tree::CjumpStm(tree::RelOp::GT_OP, leftExp, rightExp, nullptr, nullptr);
                    break;
                case Oper::GE_OP:
                    stm = new tree::CjumpStm(tree::RelOp::GE_OP, leftExp, rightExp, nullptr, nullptr);
                    break;
            }

            auto trues = new temp::Label*[2];
            auto falses = new temp::Label*[2]();
            trues[0] = stm->true_label_; trues[1] = nullptr;
            falses[0] = stm->false_label_; falses[1] = nullptr;
            exp = new tr::CxExp(trues, falses, stm);
            break;
        }

        case EQ_OP:
        case NEQ_OP:
        {
            tree::CjumpStm *stm;
            switch (oper_) {
                case EQ_OP:
                    if (dynamic_cast<type::StringTy *>(leftExpAndTy->ty_) != nullptr) {
                        auto expList = new tree::ExpList({leftExp, rightExp});
                        stm = new tree::CjumpStm(tree::RelOp::EQ_OP, frame::externalCall("string_equal", expList), new tree::ConstExp(1), nullptr, nullptr);
                    }
                    else
                        stm = new tree::CjumpStm(tree::RelOp::EQ_OP, leftExp, rightExp, nullptr, nullptr);
                    break;
                case NEQ_OP:
                    stm = new tree::CjumpStm(tree::RelOp::NE_OP, leftExp, rightExp, nullptr, nullptr);
                    break;
            }
            auto trues = new temp::Label*[2];
            auto falses = new temp::Label*[2];
            trues[0] = stm->true_label_; trues[1] = nullptr;
            falses[0] = stm->false_label_; falses[1] = nullptr;
            exp = new tr::CxExp(trues, falses, stm);
            break;
        }
    }
    return new tr::ExpAndTy(exp, type);
}


tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg)  {
    tr::ExExp *exp = nullptr;
    auto list = new tree::ExpList();


    auto records = dynamic_cast<type::RecordTy *>(type)->fields_;
    int count = 0;
    for (EField *eField : fields_->GetList()) {
        count++;
        tr::ExpAndTy *check_exp = eField->exp_->Translate(venv, tenv, level, label, errormsg);
        list->Append(check_exp->exp_->UnEx());
    }
    temp::Temp *reg = temp::TempFactory::NewTemp();
    auto expList = new tree::ExpList({new tree::ConstExp(count * reg_manager->WordSize())});
    tree::Stm *stm = new tree::MoveStm(new tree::TempExp(reg), frame::externalCall("alloc_record", expList));
    count = 0;

    for (tree::Exp *exp1 : list->GetList()) {
        stm = new tree::SeqStm(stm, new tree::MoveStm(tree::NewMemPlus_Const(new tree::TempExp(reg), count * reg_manager->WordSize()), exp1));
        count++;
    }

    exp = new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(reg)));
    return new tr::ExpAndTy(exp, type);
}

tr::Exp *TranslateSeqExp(tr::Exp *left, tr::Exp *right) {
    if (right)
        return new tr::ExExp(new tree::EseqExp(left->UnNx(), right->UnEx()));
    else
        return new tr::ExExp(new tree::EseqExp(left->UnNx(), new tree::ConstExp(0)));
}

tr::Exp *TranslateNilExp() {
    return new tr::ExExp(new tree::ConstExp(0));
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg)  {
    tr::Exp *exp = TranslateNilExp();
    if(seq_->GetList().size() == 0)
        return new tr::ExpAndTy(nullptr, type);

    auto *check_exp = new tr::ExpAndTy(nullptr, nullptr);
    for (absyn::Exp *exp_:seq_->GetList()) {
        check_exp = exp_->Translate(venv, tenv, level, label, errormsg);
        exp = TranslateSeqExp(exp, check_exp->exp_);
    }

    return new tr::ExpAndTy(exp, check_exp->ty_);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg)  {
    tr::ExpAndTy *check_var = var_->Translate(venv, tenv, level, label, errormsg);
    tr::ExpAndTy *check_exp  = exp_->Translate(venv, tenv, level, label, errormsg);
    tr::Exp *exp = new tr::NxExp(new tree::MoveStm(check_var->exp_->UnEx(), check_exp->exp_->UnEx()));
    return new tr::ExpAndTy(exp, type);
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg)  {
    tr::ExpAndTy *check_test = test_->Translate(venv, tenv, level, label, errormsg);
    tr::ExpAndTy *check_then = then_->Translate(venv, tenv, level, label, errormsg);

    tr::Exp *exp = nullptr;
    if(elsee_){
        tr::ExpAndTy *check_elsee = elsee_->Translate(venv, tenv, level, label, errormsg);
        tr::Cx testc = check_test->exp_->UnCx(errormsg);
        temp::Temp *r = temp::TempFactory::NewTemp();
        temp::Label *true_label = temp::LabelFactory::NewLabel();
        temp::Label *false_label = temp::LabelFactory::NewLabel();
        temp::Label *meeting = temp::LabelFactory::NewLabel();
        tr::do_patch(testc.trues_, true_label);
        tr::do_patch(testc.falses_, false_label);

        auto cjumpStm = dynamic_cast<tree::CjumpStm *>(testc.stm_);
        cjumpStm->true_label_ = true_label;
        cjumpStm->false_label_ = false_label;

        auto labelList = new std::vector<temp::Label *>();
        labelList->push_back(meeting); labelList->push_back(nullptr);
        exp = new tr::ExExp(new tree::EseqExp(testc.stm_,
                                           new tree::EseqExp(new tree::LabelStm(true_label),
                                                          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), check_then->exp_->UnEx()),
                                                                         new tree::EseqExp(new tree::JumpStm(new tree::NameExp(meeting), labelList),
                                                                                        new tree::EseqExp(new tree::LabelStm(false_label),
                                                                                                       new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), check_elsee->exp_->UnEx()),
                                                                                                                      new tree::EseqExp(new tree::JumpStm(new tree::NameExp(meeting), labelList),
                                                                                                                                     new tree::EseqExp(new tree::LabelStm(meeting), new tree::TempExp(r))))))))));
    } else {
        tr::Cx testc = check_test->exp_->UnCx(errormsg);
        temp::Temp *r = temp::TempFactory::NewTemp();
        temp::Label *true_label = temp::LabelFactory::NewLabel();
        temp::Label *false_label = temp::LabelFactory::NewLabel();
        temp::Label *meeting = temp::LabelFactory::NewLabel();
        tr::do_patch(testc.trues_, true_label);
        tr::do_patch(testc.falses_, false_label);

        auto cjumpStm = dynamic_cast<tree::CjumpStm *>(testc.stm_);
        cjumpStm->true_label_ = true_label;
        cjumpStm->false_label_ = false_label;

        exp = new tr::NxExp(new tree::SeqStm(testc.stm_, new tree::SeqStm(new tree::LabelStm(true_label), new tree::SeqStm(check_then->exp_->UnNx(), new tree::LabelStm(false_label)))));
    }

    return new tr::ExpAndTy(exp, check_then->ty_);
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg)  {
    tr::Exp *exp = nullptr;

    temp::Label *done_label = temp::LabelFactory::NewLabel();
    tr::ExpAndTy *check_test = test_->Translate(venv, tenv, level, label, errormsg);
    tr::ExpAndTy *check_body = body_->Translate(venv, tenv, level, done_label, errormsg);

    temp::Label *test_label = temp::LabelFactory::NewLabel();
    temp::Label *body_label = temp::LabelFactory::NewLabel();
    tr::Cx condition = check_test->exp_->UnCx(errormsg);
    tr::do_patch(condition.trues_, body_label);
    tr::do_patch(condition.falses_, done_label);

    auto cjumpStm = dynamic_cast<tree::CjumpStm *>(condition.stm_);
    cjumpStm->true_label_ = body_label;
    cjumpStm->false_label_ = done_label;

    auto *labelList = new std::vector<temp::Label *>();
    labelList->push_back(test_label); labelList->push_back(nullptr);
    exp = new tr::NxExp(
            new tree::SeqStm(new tree::LabelStm(test_label),
                          new tree::SeqStm(condition.stm_,
                                        new tree::SeqStm(new tree::LabelStm(body_label),
                                                      new tree::SeqStm(check_body->exp_->UnNx(),
                                                                    new tree::SeqStm(new tree::JumpStm(new tree::NameExp(test_label), labelList),
                                                                                  new tree::LabelStm(done_label)))))));

    return new tr::ExpAndTy(exp, type);
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg)  {
    tr::Exp *exp = nullptr;

    tr::ExpAndTy *check_lo = lo_->Translate(venv,tenv,level,label, errormsg);
    tr::ExpAndTy *check_hi = hi_->Translate(venv,tenv,level,label, errormsg);

    venv->BeginScope();
    venv->Enter(var_, new env::VarEntry(tr::Access::AllocLocal(level, escape_), check_lo->ty_));
    tr::ExpAndTy *check_body = body_->Translate(venv, tenv, level, label, errormsg);
    venv->EndScope();


    auto decList = new absyn::DecList();
    decList->Prepend(new absyn::VarDec(0, var_, sym::Symbol::UniqueSymbol("int"), lo_));
    decList->Prepend(new absyn::VarDec(0, sym::Symbol::UniqueSymbol("__limit_var__"), sym::Symbol::UniqueSymbol("int"), hi_));

    auto expList = new absyn::ExpList();

//    expList->Prepend(new absyn::IfExp(0, new absyn::OpExp(0, absyn::Oper::EQ_OP, new absyn::VarExp(0, new absyn::SimpleVar(0, var_)), new absyn::VarExp(0, new absyn::SimpleVar(0, sym::Symbol::UniqueSymbol("__limit_var__")))),
//                                      new absyn::BreakExp(0),
//                                      nullptr));
    expList->Prepend(new absyn::AssignExp(0, new absyn::SimpleVar(0,var_), new absyn::OpExp(0, absyn::PLUS_OP, new absyn::VarExp(0, new absyn::SimpleVar(0,var_)), new absyn::IntExp(0, 1))));
    expList->Prepend(body_);
    absyn::Exp *forexp_to_letexp = new absyn::LetExp(0, decList,
                                             new absyn::WhileExp(0, new absyn::OpExp(0, absyn::Oper::LE_OP, new absyn::VarExp(0, new absyn::SimpleVar(0, var_)), new absyn::VarExp(0, new absyn::SimpleVar(0, sym::Symbol::UniqueSymbol("__limit_var__")))),
                                                             new absyn::SeqExp(0, expList)));

    return forexp_to_letexp->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg)  {
//    auto *labelList = new std::vector<temp::Label *>({label});
//    labelList->push_back(label); labelList->push_back(nullptr);
    tree::Stm *stm = new tree::JumpStm(new tree::NameExp(label), new std::vector<temp::Label *>({label}));
    return new tr::ExpAndTy(new tr::NxExp(stm), type);
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg)  {
    tr::Exp *exp = nullptr;

    static bool first = true;
    bool isMain = false;
    if(first){
        isMain = true;
        first = false;
    }

    tree::Exp *res = nullptr;

    venv->BeginScope();
    tenv->BeginScope();
    tree::Stm *stm = nullptr;

    int count = 0;
    for (Dec* dec : decs_->GetList()) {
        if (count == 0) {
            stm = dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
        }
        else {
            stm = new tree::SeqStm(stm, dec->Translate(venv,tenv,level,label, errormsg)->UnNx());
        }
        count++;
    }

    tr::ExpAndTy *check_body = body_->Translate(venv, tenv, level, label, errormsg);
    venv->EndScope();
    tenv->EndScope();
    if(stm)
        res = new tree::EseqExp(stm, check_body->exp_->UnEx());
    else
        res = check_body->exp_->UnEx();
    stm = new tree::ExpStm(res);

    if(isMain){
        frags->PushBack(new frame::ProcFrag(stm, level->frame_));
        isMain = false;
    }
    return new tr::ExpAndTy(new tr::ExExp(res), type);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg)  {
    tr::ExpAndTy *check_size = size_->Translate(venv, tenv, level, label, errormsg);
    tr::ExpAndTy *check_init = init_->Translate(venv, tenv, level, label, errormsg);
    auto *expList = new tree::ExpList();
    expList->Append(check_size->exp_->UnEx());
    expList->Append(check_init->exp_->UnEx());
    tr::Exp *exp = new tr::ExExp(frame::externalCall("init_array", expList));
    return new tr::ExpAndTy(exp, type);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg)  {
    return new tr::ExpAndTy(nullptr, type);
}


std::list<bool> *make_formal_esclist(absyn::FieldList *params) {
    auto result = new std::list<bool>();
    if(params) {
        for (Field * param : params->GetList()) {
            result->push_back(param->escape_);
        }
    }
    return result;
}

static type::TyList *make_formal_tylist(env::TEnvPtr tenv, FieldList *params) {
    auto result = new type::TyList();
    if (params){
        for (Field * param : params->GetList()) {
            type::Ty *ty = nullptr;
            if (param->typ_) ty = tenv->Look(param->typ_);
            result->Append(ty);
        }

    }
    return result;
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg)  {
    for (FunDec *fundec : functions_->GetList()) {
        type::TyList *formaltys = make_formal_tylist(tenv, fundec->params_);
        tr::Level *new_level = tr::Level::NewLevel(level, fundec->name_, make_formal_esclist(fundec->params_));

//        errormsg->Error(fundec->pos_, "new_level = " + std::to_string((long long) (new_level)));
//        errormsg->Error(fundec->pos_, "function_name = " + fundec->name_->Name());

        if(!fundec->result_){
            venv->Enter(fundec->name_, new env::FunEntry(new_level, fundec->name_, formaltys, type::VoidTy::Instance()));
        }
        else {
            type::Ty *result = tenv->Look(fundec->result_);
            venv->Enter(fundec->name_, new env::FunEntry(new_level, fundec->name_, formaltys, result));
        }
    }

    for (FunDec *fundec : functions_->GetList()) {
        venv->BeginScope();
        auto funentry = dynamic_cast<env::FunEntry *>(venv->Look(fundec->name_));

        int count = 0;
        for (Field *field : fundec->params_->GetList()) {
            type::Ty *ty = nullptr;
            if (field->typ_) ty = tenv->Look(field->typ_);

            frame::Access *resultAccess;
            int count2 = 0;
            for (frame::Access *access : funentry->level_->frame_->formals) {
                if (count2 == count) {
                    resultAccess = access;
                }
                count2++;
            }
            venv->Enter(field->name_, new env::VarEntry(new tr::Access(funentry->level_, resultAccess), ty));
            count++;
        }
//        auto accessList = std::list<frame::Access*>();
//
//        for (Field *field : fundec->params_->GetList()) {
//            bool escape = field->escape_;
//            type::Ty *ty = nullptr;
//            if (field->typ_) ty = tenv->Look(field->typ_);
//            frame::Access *access = funentry->level_->frame_->allocLocal(escape);
//            auto trAccess = new tr::Access(funentry->level_, access);
//            accessList.push_back(access);
//            venv->Enter(field->name_, new env::VarEntry(trAccess, ty));
//        }
//
//        funentry->level_->frame_->formals = accessList;
//        venv->Set(fundec->name_, funentry);

        tr::ExpAndTy *entry = fundec->body_->Translate(venv, tenv, funentry->level_, funentry->label_, errormsg);
        venv->EndScope();
        frags->PushBack(new frame::ProcFrag(procEntryExit1(funentry->level_->frame_, new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), entry->exp_->UnEx())), funentry->level_->frame_));
    }
    return TranslateNilExp();
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg)  {
    tr::ExpAndTy *check_init = init_->Translate(venv,tenv,level,label, errormsg);
    tr::Access *access;

    access = tr::Access::AllocLocal(level, true);
    venv->Enter(var_, new env::VarEntry(access, check_init->ty_));

    return TranslateAssignExp(TranslateSimpleVar(access, level), check_init->exp_);
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) {
    for (NameAndTy* current: types_->GetList()) {   // put all the name into the env first
        sym::Symbol *symbol = current->name_;
//        errormsg->Error(pos_, "define symbol " + symbol->Name());
        tenv->Enter(symbol, new type::NameTy(symbol, current->ty_->Translate(tenv, errormsg)));
    }
    return TranslateNilExp();
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) {
//    errormsg->Error(pos_, "type = NameTy ty ");
    return type;
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) {
//    errormsg->Error(pos_, "type = record ty ");
    return type;
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) {
//    errormsg->Error(pos_, "type = ArrayTy ty ");
    return type;
}

} // namespace absyn
