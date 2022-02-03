#include "tiger/codegen/assem.h"

#include <cassert>
#include <iostream>
#include <tiger/frame/frame.h>
#include <tiger/frame/x64frame.h>

namespace temp {

extern frame::RegManager *reg_manager;
Temp *TempList::NthTemp(int i) const {
  for (auto it : temp_list_)
    if (i-- == 0)
      return it;
  assert(0);
}
} // namespace temp

namespace assem {
/**
 * First param is string created by this function by reading 'assem' string
 * and replacing `d `s and `j stuff.
 * Last param is function to use to determine what to do with each temp.
 * @param assem assembly string
 * @param dst dst_ temp
 * @param src src temp
 * @param jumps jump labels_
 * @param m temp map
 * @return formatted assembly string
 */
static std::string Format(std::string_view assem, temp::TempList *dst,
                          temp::TempList *src, Targets *jumps, temp::Map *m) {
    std::cout << "dst = " << dst << " src = " << src << "assem = " << assem << std::endl;
    std::string result;
    for (std::string::size_type i = 0; i < assem.size(); i++) {
//        std::cout << "char = " << assem.at(i) << std::endl;
        char ch = assem.at(i);
        if (ch == '`') {
            i++;
            switch (assem.at(i)) {
                case 's': {
                    i++;
                    int n = assem.at(i) - '0';
                    std::cout << "src->NthTemp(n)->int = " << src->NthTemp(n)->Int() << std::endl;
                    std::string *s = m->Look(src->NthTemp(n));
                    if (reg_manager->check_temp(src->NthTemp(n)->Int()) == "") {
                        //不属于machine register的情况
                        if (s == nullptr) {
                            result += "t" + std::to_string(src->NthTemp(n)->Int());
                        }
                        else result += *s;
                    }
                    else {
                        result += reg_manager->check_temp(src->NthTemp(n)->Int());
                    }
                    break;
                }
                case 'd': {
                    i++;
                    int n = assem.at(i) - '0';
                    std::cout << "dst->NthTemp(n)->int = " << dst->NthTemp(n)->Int() << std::endl;
                    std::string *s = m->Look(dst->NthTemp(n));
                    if (reg_manager->check_temp(dst->NthTemp(n)->Int()) == "") {
                        if (s == nullptr) {
                            result += "t" + std::to_string(dst->NthTemp(n)->Int());
                        }
                        else result += *s;
                    }
                    else {
                        result += reg_manager->check_temp(dst->NthTemp(n)->Int());
                    }
                    break;
                }
                case 'j': {
                    i++;
                    assert(jumps);
                    std::string::size_type n = assem.at(i) - '0';
                    std::string s = temp::LabelFactory::LabelString(jumps->labels_->at(n));
                    result += s;
                    break;
                }
                case '`': {
                    result += '`';
                    break;
                }
                default: {
                    assert(0);
                }
            }
        }
        else {
            result += ch;
        }
    }
    return result;
}

void OperInstr::Print(FILE *out, temp::Map *m) const {
    std::string result = Format(assem_, dst_, src_, jumps_, m);
    fprintf(out, "%s\n", result.data());
}

void LabelInstr::Print(FILE *out, temp::Map *m) const {
    std::string result = Format(assem_, nullptr, nullptr, nullptr, m);
    fprintf(out, "%s:\n", result.data());
}

void MoveInstr::Print(FILE *out, temp::Map *m) const {
    if (!dst_ && !src_) {
        std::size_t srcpos = assem_.find_first_of('%');
        if (srcpos != std::string::npos) {
            std::size_t dstpos = assem_.find_first_of('%', srcpos + 1);
            if (dstpos != std::string::npos) {
                if ((assem_[srcpos + 1] == assem_[dstpos + 1]) &&
                (assem_[srcpos + 2] == assem_[dstpos + 2]) &&
                (assem_[srcpos + 3] == assem_[dstpos + 3]))
                return;
            }
        }
    }
    std::string result = Format(assem_, dst_, src_, nullptr, m);
    fprintf(out, "%s\n", result.data());
}

void InstrList::Print(FILE *out, temp::Map *m) const {
    for (auto instr : instr_list_)
        instr->Print(out, m);
    fprintf(out, "\n");
}

} // namespace assem
