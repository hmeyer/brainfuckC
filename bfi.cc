#include "bfi.hpp"
#include <unordered_set>
#include <vector>
#include <iostream>
#include <cassert>

namespace bfi {

Word& Tape::operator[](int p) {
    std::vector<Word>* a = &positive_;
    if (p < 0) {
        p = 1 - p;
        a = &negative_;
    }
    if (p >= a->size()) {
        a->resize(p + 1);
    }
    return (*a)[p];
}

BrainfuckInterpreter::BrainfuckInterpreter(std::string code) {
    static const std::unordered_set<char>* kBfTokens = new std::unordered_set<char>{'.', ',', '+', '-', '<', '>', '[', ']'};
    std::vector<int> loop_starts;
    int ip = 0;
    while(ip < code.size()) {
        char t = code[ip++];
        if (kBfTokens->find(t) != kBfTokens->end()) {
            code_.push_back(t);
            if (t == '[') {
                loop_starts.push_back(code_.size());
            }
            if (t == ']') {
                if (loop_starts.empty()) {
                    throw std::invalid_argument("unmatched loop end at pos " + std::to_string(ip));
                }
                int start = loop_starts.back();
                loop_starts.pop_back();
                int end = code_.size();
                loop_start_to_end_[start] = end;
                loop_end_to_start_[end] = start;
            }
        }
    }
}

bool BrainfuckInterpreter::run_step() {
    if (ip_ >= code_.size()) {
        return false;
    }
    switch (code_[ip_++]) {
        case '.': std::cout << static_cast<char>(tape_[tp_]) << std::flush; break;
        case ',': tape_[tp_] = getchar(); break;
        case '+': tape_[tp_]++; break;
        case '-': tape_[tp_]--; break;
        case '>': tp_++; break;
        case '<': tp_--; break;
        case '[': if (tape_[tp_] == 0) {
                assert(loop_start_to_end_.find(ip_) != loop_start_to_end_.end());
                ip_ = loop_start_to_end_[ip_];
            }
            break;
        case ']': if (tape_[tp_] != 0) {
                assert(loop_end_to_start_.find(ip_) != loop_end_to_start_.end());
                ip_ = loop_end_to_start_[ip_];
            }
            break;
        default:
            throw std::runtime_error("unexpected token" + code_.substr(ip_ -1, 1));
    }
    return true;
}
void BrainfuckInterpreter::run() {
    while(run_step());
}


}  // namespace bfi