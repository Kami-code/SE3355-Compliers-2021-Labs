#include "straightline/slp.h"

#include <iostream>
#include <cmath>


//1. Write a function int maxargs(A_stm) that tells the maximum number
//        of arguments of any print statement within any subexpression of a given
//        statement. For example, maxargs(prog) is 2.
//2. Write a function void interp(A_stm) that “interprets” a program in this
//      language. To write in a “functional programming” style – in which you never
//      use an assignment statement – initialize each local variable as you declare it.
namespace A {
int A::CompoundStm::MaxArgs() const {
  return std::max(stm1->MaxArgs(), stm2->MaxArgs());
}
int A::AssignStm::MaxArgs() const {
    return exp->MaxArgs();
}
int A::PrintStm::MaxArgs() const {
    // Not correct here, if no containing printf result is exps->numExps, but if containing printf
    // result will be std::max(exps->NumExps, containing print)
    return std::max(exps->NumExps(), exps->MaxArgs());
}
int A::NumExp::MaxArgs() const{
    return 0;
}
int A::IdExp::MaxArgs() const {
    return 0;
}
int A::EseqExp::MaxArgs() const {
    //the statement or expression will containing printf
    return std::max(stm->MaxArgs(), exp->MaxArgs());
}
int A::OpExp::MaxArgs() const {
    return std::max(left->MaxArgs(), right->MaxArgs());
}
int A::PairExpList::MaxArgs() const{
    return std::max(exp->MaxArgs(), tail->MaxArgs());
}
int A::LastExpList::MaxArgs() const{
    return exp->MaxArgs();
}
int A::LastExpList::NumExps() const {
    return 1;
}
int A::PairExpList::NumExps() const {
    return 1 + tail->NumExps();
}


Table *A::CompoundStm::Interp(Table *t) const {
    Table *first_table = stm1->Interp(t);
    Table *second_table = stm2->Interp(first_table);
    return second_table;
}



Table *A::AssignStm::Interp(Table *t) const {
  IntAndTable *int_and_table = exp->Interp(t);
  Table *table = int_and_table->t;
  int expression_result = int_and_table->i;
  if (table == nullptr) {
      table = new Table(id, expression_result, nullptr);
  }
  else {
      table = new Table(id, expression_result, table);
      //table->Update(id, expression_result);
  }

  return table;
}



Table *A::PrintStm::Interp(Table *t) const {
    IntAndTable *int_and_table = exps->Interp(t);
    return int_and_table->t;
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
    IntAndTable *int_and_table = exp->Interp(t);
    printf("%d\n", int_and_table->i);
    return int_and_table;
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
    IntAndTable *first_int_and_table = exp->Interp(t);
    printf("%d ", first_int_and_table->i);
    IntAndTable *last_int_and_table = tail->Interp(first_int_and_table->t);
    return last_int_and_table;
}

IntAndTable *A::OpExp::Interp(Table *t) const {
    IntAndTable *left_int_and_table = left->Interp(t);
    IntAndTable *right_int_and_table = right->Interp(left_int_and_table->t);
    enum BinOp { PLUS = 0, MINUS, TIMES, DIV };
    int result = 0;
    switch (oper) {
        case PLUS:
            result = left_int_and_table->i + right_int_and_table->i;
            break;
        case MINUS:
            result = left_int_and_table->i - right_int_and_table->i;
            break;
        case TIMES:
            result = left_int_and_table->i * right_int_and_table->i;
            break;
        case DIV:
            result = left_int_and_table->i / right_int_and_table->i;
    }
    return new IntAndTable(result, right_int_and_table->t);
}
IntAndTable *A::IdExp::Interp(Table * t) const {
    return new IntAndTable(t->Lookup(id), t);
}
IntAndTable *A::EseqExp::Interp(Table * t) const {
    Table *first_table = stm->Interp(t);
    IntAndTable *second_int_and_table = exp->Interp(first_table);
    return second_int_and_table;
}
IntAndTable *A::NumExp::Interp(Table *t) const {
    return new IntAndTable(num, t);
}



int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A
