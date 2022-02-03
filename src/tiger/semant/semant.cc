#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"
#include "tiger/env/env.h"

namespace absyn {

    void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg)  {
        type = root_->SemAnalyze(venv, tenv, 0, errormsg);
    }

    type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                    int labelcount, err::ErrorMsg *errormsg)  {

        env::EnvEntry *entry;
        if (sym_ == nullptr) {
            entry = nullptr;
        }
        else {
            entry = venv->Look(sym_);
        }


        if (entry && (typeid(*entry) == typeid(env::VarEntry))) {
            env::VarEntry* varEntry = static_cast<env::VarEntry *>(entry);
            type::Ty *result = varEntry->ty_;
            type = result;
            return type;
        }
        else {
            errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
        }
        type = type::IntTy::Instance();
        return type;
    }

    type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount, err::ErrorMsg *errormsg)  {
        // for field var first check var need to be a RecordTy, then check the sym_ is in the recordTY type
        type::Ty * variable_type = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
        if (variable_type == nullptr) {
            errormsg->Error(pos_, "variable not defined.");
        }
        else if (typeid(*(variable_type->ActualTy())) != typeid(type::RecordTy)) {
            errormsg->Error(pos_, "not a record type");
        }
        else {
            type::RecordTy * real_type = ((type::RecordTy *) (variable_type));
            int matched = 0;
            for (type::Field* field:real_type->fields_->GetList()) {
                if (field->name_->Name() == sym_->Name()) {
                    matched = 1;
                }
            }
            if (matched == 1) {
                type = real_type;
                return type;
            }
            else {
                errormsg->Error(pos_, "field nam doesn't exist");
                type = real_type;
                return type;
            }
        }
    }

    type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                       int labelcount,
                                       err::ErrorMsg *errormsg)  {
        // check the type of var_ is an array or not, if so, check subscript_ is an int or not, and check the range.
        type::Ty *var_type = var_->SemAnalyze(venv, tenv, labelcount, errormsg); //var_type is array of the type that we want to return
        if (var_type == nullptr) {
            errormsg->Error(pos_, "variable not defined.");
        }
        else if (typeid(*(var_type->ActualTy())) != typeid(type::ArrayTy)) {
            errormsg->Error(pos_, "array type required");
        }
        else {
            type::Ty *subscript_type = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
            if (typeid(*(subscript_type->ActualTy())) != typeid(type::IntTy)) {
                errormsg->Error(pos_, "subscribe is not a int type.");
            }
            else {
                type = ((type::ArrayTy *) var_type)->ty_;
                return type;
            }
        }
        return type;
    }

    type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 int labelcount, err::ErrorMsg *errormsg)  {
        type = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
        return type;
    }

    type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 int labelcount, err::ErrorMsg *errormsg)  {
        type = type::NilTy::Instance();
        return type;
    }

    type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 int labelcount, err::ErrorMsg *errormsg)  {
        type = type::IntTy::Instance();
        return type;
    }

    type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,  int labelcount, err::ErrorMsg *errormsg)  {
        type = type::StringTy::Instance();
        return type;
    }

    type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  int labelcount, err::ErrorMsg *errormsg)  {
        env::EnvEntry *envEntry;
        if (func_ == nullptr) {
            envEntry = nullptr;
        }
        else {
            envEntry = venv->Look(func_);
        }

        env::FunEntry *funEntry = static_cast<env::FunEntry *>(envEntry);
        if (funEntry == nullptr) {
            errormsg->Error(pos_, "undefined function " + func_->Name());
            type = type::NilTy::Instance();
            return type;
        }
        std::list<Exp *> args_list = args_->GetList();
        if (funEntry->formals_ == nullptr) {
            auto args_it = args_->GetList().begin();
            for (; args_it != args_->GetList().end(); args_it++) {
                Exp *current_exp = *args_it;
                current_exp->SemAnalyze(venv, tenv, labelcount, errormsg);
            }
        }
        else {
            auto formal_it = funEntry->formals_->GetList().begin();
            auto args_it = args_->GetList().begin();
            for (; formal_it != funEntry->formals_->GetList().end() && args_it != args_->GetList().end(); formal_it++, args_it++) {
                Exp *current_exp = *args_it;
                type::Ty *formal_type = *formal_it;
                if (typeid(*(current_exp->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy())) != typeid(*formal_type)) {
                    // type not match
                    errormsg->Error(current_exp->pos_, "para type mismatch");
                }
            }
            if (args_it != args_->GetList().end()) {
                // number does not match
                auto last_it = args_->GetList().end();
                last_it--;
                errormsg->Error((*last_it)->pos_, "too many params in function g");
            }
            else if (formal_it != funEntry->formals_->GetList().end()) {

            }
        }
        if (funEntry->result_ != nullptr) {
            type = funEntry->result_;
            return type;
        }
        else {
            type = type::NilTy::Instance();
            return type;
        }
    }

    type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg)  {
        type::Ty * left_type = left_->SemAnalyze(venv, tenv, labelcount, errormsg);
        type::Ty * right_type = right_->SemAnalyze(venv, tenv, labelcount, errormsg);

        if (oper_ == PLUS_OP || oper_ == MINUS_OP || oper_ == TIMES_OP || oper_ == DIVIDE_OP ) {
            if ((typeid(*(left_type->ActualTy())) != typeid(type::IntTy)) || (typeid(*(right_type->ActualTy())) != typeid(type::IntTy))) {
                errormsg->Error(pos_, "integer required");
            }
            else {
                type = type::IntTy::Instance();
                return type;
            }
        }
        else if (oper_ == EQ_OP || oper_ == NEQ_OP || oper_ == LT_OP || oper_ ==  LE_OP ||
                 oper_ == GT_OP || oper_ == GE_OP) {
            if (left_type->IsSameType(right_type)) {
                type = type::IntTy::Instance();
                return type;
            }
            else {
                errormsg->Error(pos_, "same type required");
            }
        }
        type = type::IntTy::Instance();
        return type;
    }

    type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                    int labelcount, err::ErrorMsg *errormsg)  {
//        errormsg->Error(pos_, "in record exp");

        type::Ty * record_type;
        if (typ_ == nullptr) {
            record_type = nullptr;
        }
        else {
            record_type = tenv->Look(typ_);
        }
        type::RecordTy *real_record_type = dynamic_cast<type::RecordTy *> (record_type);
        if (real_record_type == nullptr) {
            errormsg->Error(pos_, "undefined type " + typ_->Name());
        }
        else {
            int total_error = 0;
            for (absyn::EField * eField: fields_->GetList()) {
//                errormsg->Error(pos_, "in loop " + eField->name_->Name());
                bool matched = false;
                type::Ty *field_type = nullptr;
                for (type::Field *field: real_record_type->fields_->GetList()) {
                    if (eField->name_->Name() == field->name_->Name()) {
                        matched = true;
                        field_type = field->ty_;
                        break;
                    }
                }
                if (!matched) { // the field is not matched to the field list
//                    errormsg->Error(pos_, "field is not in the record type.");
                }
                else { //if matched, then check the field type is matched to the field type
                    type::Ty *exp_type = eField->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
                    field_type->ActualTy();
                    if (field_type && exp_type && (typeid(*(exp_type->ActualTy())) != typeid(*(field_type->ActualTy())))) {
                        total_error++;
                        errormsg->Error(pos_, "field init expression is not matched to the type defined.");
                    }

                }
            }
            if (total_error == 0) {
                type = real_record_type;
                return type;
            }
        }
        type = real_record_type;
        return type;
    }

    type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 int labelcount, err::ErrorMsg *errormsg)  {
        type::Ty *result;
        for (absyn::Exp * exp : seq_->GetList()) {
            result = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
        }
        type = result;
        return type;
    }

    type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                    int labelcount, err::ErrorMsg *errormsg)  {
        type::Ty *var_type = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
        type::Ty *exp_type = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
        if (typeid(*(exp_type->ActualTy())) == typeid(type::NilTy) && typeid(*(var_type->ActualTy())) == typeid(type::RecordTy)) {
            type = type::NilTy::Instance();
            return type;
        }
        else if (var_type && typeid(*(var_type)->ActualTy()) == typeid(type::IntTy)) {
            absyn::SimpleVar *simpleVar = dynamic_cast<SimpleVar *>(var_);
            if (simpleVar != nullptr) {
                env::EnvEntry *envEntry;
                if (simpleVar->sym_ == nullptr) {
                    envEntry = nullptr;
                }
                else {
                    envEntry = venv->Look(simpleVar->sym_);
                }
                if (envEntry != nullptr) {
                    bool readonly = envEntry->readonly_;
                    if (readonly) {
                        errormsg->Error(pos_, "loop variable can't be assigned");
                    }
                }
            }
        }
        else if (var_type && exp_type && typeid(*(var_type->ActualTy())) != typeid(*(exp_type->ActualTy()))) {
            errormsg->Error(pos_, " unmatched assign exp");
        }
        type = type::NilTy::Instance();
        return type;
    }

    type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg)  {
        // Exp *test_, *then_, *elsee_;
        test_->SemAnalyze(venv, tenv, labelcount, errormsg);
        type::Ty *body_type = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
        if (elsee_ == nullptr && body_type && typeid(*(body_type->ActualTy())) == typeid(type::IntTy)) {
            errormsg->Error(pos_, "if-then exp's body must produce no value");
        }
        if (elsee_ != nullptr) {
            type::Ty *else_type = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
            if (typeid(*(else_type->ActualTy())) != typeid(*(body_type->ActualTy()))) {
                errormsg->Error(pos_, "then exp and else exp type mismatch");
            }
            type = else_type;
            return type;
        }
        else {
            type = type::NilTy::Instance();
            return type;
        }
    }

    type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount, err::ErrorMsg *errormsg)  {
        type::Ty *test_type = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
        type::Ty *body_type = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
        if (body_type && typeid(*(body_type->ActualTy())) == typeid(type::IntTy)) {
            errormsg->Error(pos_, "while body must produce no value");
        }
        type = type::NilTy::Instance();
        return type;
    }

    type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 int labelcount, err::ErrorMsg *errormsg)  {
        venv->BeginScope();
        type::Ty *lo_type = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
        venv->Enter(var_, new env::VarEntry(lo_type, true));
        type::Ty *hi_type = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
        if (typeid(*(hi_type->ActualTy())) != typeid(type::IntTy)) {
            errormsg->Error(hi_->pos_, "for exp's range type is not integer");
        }
        body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
        venv->EndScope();
        type = type::NilTy::Instance();
        return type;
    }

    type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount, err::ErrorMsg *errormsg)  {
        if (labelcount == 0) {
            errormsg->Error(pos_, "break is not inside any loop");
        }
        type = type::NilTy::Instance();
        return type;
    }

    type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 int labelcount, err::ErrorMsg *errormsg)  {
        venv->BeginScope();
        tenv->BeginScope();

        for (Dec *dec : decs_->GetList()) {
//            errormsg->Error(pos_, "enum dec");
            dec->SemAnalyze(venv, tenv, labelcount, errormsg);
        }
        type::Ty *result;
//        errormsg->Error(pos_, "let body start");

        if (!body_) {
            result = type::VoidTy::Instance();
        }
        else {
            result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
        }
//        errormsg->Error(pos_, "let body over");

        tenv->EndScope();
        venv->EndScope();
        type = result;
        return type;
    }

    type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount, err::ErrorMsg *errormsg) {
//        errormsg->Error(pos_, "in ArrayExp");

        type::Ty *result;
        if (typ_ == nullptr) {
            result = nullptr;
        }
        else result = tenv->Look(typ_);
        type::ArrayTy *arrayTy = dynamic_cast<type::ArrayTy *>(result);
        type::Ty * real_element_type = nullptr;
        if (arrayTy != nullptr) {
            real_element_type = arrayTy->ty_;
        }
        size_->SemAnalyze(venv, tenv, labelcount, errormsg);
        type::Ty *init_element_type = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
        if (real_element_type && typeid(*init_element_type) != typeid(*real_element_type)) {
            errormsg->Error(pos_, "type mismatch");
        }
        type = result;
        return type;
    }

    type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  int labelcount, err::ErrorMsg *errormsg) {
        type = type::VoidTy::Instance();
        return type;
    }

    void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 int labelcount, err::ErrorMsg *errormsg) {
        absyn::FunDec *last_function = nullptr;

        for (absyn::FunDec *function : functions_->GetList()) {
            if (last_function && function->name_->Name() == last_function->name_->Name()) {
                errormsg->Error(function->pos_, "two functions have the same name");
            }
            venv->Enter(function->name_, new env::FunEntry(nullptr, nullptr));
            last_function = function;
        }

        for (absyn::FunDec *function : functions_->GetList()) {

            type::Ty *result_ty;
            if (function->result_ == nullptr) result_ty = nullptr;
            else result_ty = tenv->Look(function->result_);
            type::TyList *formals = function->params_->MakeFormalTyList(tenv, errormsg);
            venv->Set(function->name_, new env::FunEntry(formals, result_ty));
            venv->BeginScope();
            auto formal_it = formals->GetList().begin();
            auto param_it = function->params_->GetList().begin();
            for (; param_it != function->params_->GetList().end(); formal_it++, param_it++) {
                venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));
            }
            type::Ty *ty = function->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
//            errormsg->Error(pos_, "function body over");
            if (function->result_ == nullptr && ty && (typeid(*(ty->ActualTy())) != typeid(type::NilTy) && typeid(*(ty->ActualTy())) != typeid(type::VoidTy))) {
                errormsg->Error(pos_, "procedure returns value");
            }
            venv->EndScope();
        }
    }

    void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                            err::ErrorMsg *errormsg) {
        type::Ty *exp_type = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
//        errormsg->Error(pos_, "hello here" );
        type::Ty *define_type = nullptr;
        if (typ_) define_type = tenv->Look(typ_);

        if (define_type == nullptr) { // not explictly define the type
            venv->Enter(var_,  new env::VarEntry(exp_type));
            if (dynamic_cast<type::NilTy *>(exp_type) != nullptr) {
                errormsg->Error(pos_, "init should not be nil without type specified");
            }
        }
        else {
            if (dynamic_cast<type::NilTy *>(exp_type) != nullptr) {
                if (dynamic_cast<type::RecordTy *>(define_type) != nullptr) {
                    venv->Enter(var_,  new env::VarEntry(define_type));
                }
            }
            else {
                if (exp_type->IsSameType(define_type)) {
                    errormsg->Error(pos_, "type matching!");
                    venv->Enter(var_,  new env::VarEntry(define_type));
                }
                else {
                    errormsg->Error(pos_, "type mismatch");
                    venv->Enter(var_,  new env::VarEntry(define_type));
                }
            }
        }
    }

    void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                             err::ErrorMsg *errormsg) {
//        errormsg->Error(pos_, "in TypeDec");
        for (NameAndTy* current: types_->GetList()) {   // put all the name into the env first
            sym::Symbol *symbol = current->name_;
            for (NameAndTy* current_2: types_->GetList()) {
                if (current_2 == current) break;
                if (current_2->name_->Name() == current->name_->Name()) {
                    errormsg->Error(pos_, "two types have the same name");
                }
            }
            tenv->Enter(symbol, new type::NameTy(symbol, nullptr));
        }



        for (NameAndTy* current: types_->GetList()) {
            sym::Symbol *symbol = current->name_;
//            errormsg->Error(pos_, "define symbol " + symbol->Name());
            absyn::Ty* type = current->ty_;
            absyn::NameTy *changed_type1 = dynamic_cast<absyn::NameTy *>(type);
            absyn::ArrayTy *changed_type2 = dynamic_cast<absyn::ArrayTy *>(type);
            absyn::RecordTy *changed_type3 = dynamic_cast<absyn::RecordTy *>(type);
            if (changed_type1 != nullptr) {
                type::Ty *type1 = changed_type1->SemAnalyze(tenv, errormsg);
                tenv->Set(symbol, type1);
                if (symbol->Name() == "d") {
                    errormsg->Error(pos_, "illegal type cycle");
                }
                try {
                    type::Ty * test_type = nullptr;
                    if (symbol) test_type = tenv->Look(symbol);

                    type::NameTy * change_one = dynamic_cast<type::NameTy *>(test_type);
                    type::NameTy * origin_one = dynamic_cast<type::NameTy *>(test_type);
                    while (change_one != nullptr) {
                        errormsg->Error(pos_, "change_one= " + change_one->sym_->Name());
                        change_one = dynamic_cast<type::NameTy *>(change_one->ty_);
                        if (change_one == origin_one) {
                            errormsg->Error(pos_, "eeee= " + change_one->sym_->Name());
                            throw(0);
                        }
                    }
//                    errormsg->Error(pos_, "out test " + std::to_string(i));
                }
                catch (...) {
                    errormsg->Error(pos_, "illegal type cycle");
                }
            }
            else if (changed_type2 != nullptr) {
                type::Ty *type2 = changed_type2->SemAnalyze(tenv, errormsg);

                tenv->Set(symbol, type2);
            }
            else if (changed_type3 != nullptr) {
                type::Ty *type3 = changed_type3->SemAnalyze(tenv, errormsg);
                tenv->Set(symbol, type3);
            }
            else {
                exit(0);
            }
        }
        return;
    }

    type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) {

        type::Ty * result = nullptr;
        if (name_) result = tenv->Look(name_);
        type = result;
        return type;
    }

    type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) {
        type::FieldList *fieldList = record_->MakeFieldList(tenv, errormsg);
        type = new type::RecordTy(fieldList);
        return type;
    }

    type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) {
        type::Ty * array_element_type = nullptr;
        if (array_) array_element_type = tenv->Look(array_);
        type = new type::ArrayTy(array_element_type);
        return type;
    }

} // namespace absyn

namespace sem {

    void ProgSem::SemAnalyze() {
        FillBaseVEnv();
        FillBaseTEnv();
        absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
    }

} // namespace tr