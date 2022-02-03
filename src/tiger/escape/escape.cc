#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  root_->Traverse(env, 1);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  esc::EscapeEntry *entry = env->Look(sym_);
  if (entry == nullptr) return;
  if (entry->depth_ < depth) *(entry->escape_) = true;
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
    var_->Traverse(env, depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
    var_->Traverse(env, depth);
    subscript_->Traverse(env, depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
    var_->Traverse(env, depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
    return;
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
    return;
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
    return;
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
    return;
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
    return;
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
    left_->Traverse(env, depth);
    right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  for (EField *eField:fields_->GetList()) {
      eField->exp_->Traverse(env, depth);
  }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
    for(Exp *exp: seq_->GetList()) {
        exp->Traverse(env, depth);
    }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
    var_->Traverse(env, depth);
    exp_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
    test_->Traverse(env, depth);
    then_->Traverse(env, depth);
    if (elsee_) elsee_->Traverse(env, depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
    test_->Traverse(env, depth);
    body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
    escape_ = false;
    env->Enter(var_, new esc::EscapeEntry(depth, &escape_));
    lo_->Traverse(env, depth);
    hi_->Traverse(env, depth);
    body_->Traverse(env, depth);
}


void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  for (Dec *dec:decs_->GetList()) {
      dec->Traverse(env, depth);
  }
  body_->Traverse(env, depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
    size_->Traverse(env, depth);
    init_->Traverse(env, depth);
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
    for (Exp *exp:args_->GetList()) {
        exp->Traverse(env, depth);
    }
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
    for (FunDec *funDec:functions_->GetList()) {
        for(Field *field:funDec->params_->GetList()) {
            field->escape_ = false;
            env->Enter(field->name_, new esc::EscapeEntry(depth + 1, &field->escape_));
        }
        funDec->body_->Traverse(env, depth + 1);
    }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
    escape_ = false;
    env->Enter(var_, new esc::EscapeEntry(depth, &escape_));
    init_->Traverse(env, depth);
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
    return;
}


} // namespace absyn
