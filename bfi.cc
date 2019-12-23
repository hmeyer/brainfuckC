#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cassert>


using Word = uint32_t;

class Tape {
public:
    Word& operator[](int p);
private:
    std::vector<Word> negative_;
    std::vector<Word> positive_;
};

class BrainfuckInterpreter {
public:
    BrainfuckInterpreter(std::string code);
    // returns true if there are more step to run.
    bool run_step();
    // runs to the end.
    void run();
private:
    char next();
    std::string code_;
    std::unordered_map<int, int> loop_start_to_end_;
    std::unordered_map<int, int> loop_end_to_start_;
    int ip_ = 0;
    Tape tape_;
    int tp_ = 0;
};

int main(int argc, const char * argv[]) {
    std::string code;
    for (std::string line; std::getline(std::cin, line);) {
        code += line;
    }

    BrainfuckInterpreter interpreter(code);
    interpreter.run();

    return 0;
}

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
        if (code.substr(ip, 3) == "[-]") {
            code_.push_back('z');
            ip+=3;
            continue;
        }
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
        case 'z': tape_[tp_] = 0; break;
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

