#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdint>
#include <string>


using Word = uint32_t;

void print_usage(const std::string& program_name) {
    std::cerr << "Usage: " << program_name << " [--nocomments|-nc] <filename> [max_steps]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --nocomments, -nc    Don't ignore comments (parts of code after # character)" << std::endl;
    std::cerr << "  max_steps            Maximum number of steps to execute (default: 1000000)" << std::endl;
}

class BrainfuckInterpreter {
public:
    BrainfuckInterpreter(std::string code);
    // returns true if there are more step to run.
    bool run_step();
    // runs to the end.
    void run(size_t max_steps = 1000000);
private:
    char next();
    std::string code_;
    std::unordered_map<int, int> loop_start_to_end_;
    std::unordered_map<int, int> loop_end_to_start_;
    int ip_ = 0;
    std::unordered_map<int, Word> tape_;
    int tp_ = 0;
};

int main(int argc, const char * argv[]) {
    std::string code;
    size_t max_steps = 1000000;
    bool ignore_comments = true;
    std::string filename;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--nocomments" || arg == "-nc") {
            ignore_comments = false;
        } else if (filename.empty()) {
            // First non-option argument is the filename
            filename = arg;
        } else {
            // Second non-option argument is max_steps
            try {
                max_steps = std::stoull(arg);
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid max_steps value: " << arg << std::endl;
                print_usage(argv[0]);
                return 1;
            }
        }
    }
    
    if (filename.empty()) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string raw_code = buffer.str();
    
    // Process the code to handle comments if needed
    if (ignore_comments) {
        for (size_t i = 0; i < raw_code.length(); ++i) {
            if (raw_code[i] == '#') {
                // Skip until end of line or end of file
                while (i < raw_code.length() && raw_code[i] != '\n') {
                    ++i;
                }
                if (i >= raw_code.length()) break;
                if (raw_code[i] == '\n') {
                    code += raw_code[i];  // Keep the newline character
                }
            } else {
                code += raw_code[i];
            }
        }
    } else {
        code = raw_code;
    }
    
    BrainfuckInterpreter interpreter(code);
    interpreter.run(max_steps);

    return 0;
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
        case ',': {
            int ch = getchar();
            if (ch == EOF) {
                std::cerr << "Error: No input available from stdin" << std::endl;
                exit(1);
            }
            tape_[tp_] = ch;
            break;
        }
        case '+': tape_[tp_]++; break;
        case '-': tape_[tp_]--; break;
        case 'z': tape_[tp_] = 0; break;
        case '>': tp_++; break;
        case '<': tp_--; break;
        case '[': if (tape_[tp_] == 0) {
                ip_ = loop_start_to_end_.at(ip_);
            }
            break;
        case ']': ip_ = loop_end_to_start_.at(ip_);
            break;
        default:
            throw std::runtime_error("unexpected token" + code_.substr(ip_ -1, 1));
    }
    return true;
}

void BrainfuckInterpreter::run(size_t max_steps) {
    size_t steps = 0;
    while(run_step()) {
        steps++;
        if (steps >= max_steps) {
            std::cerr << "Error: Maximum steps (" << max_steps << ") reached" << std::endl;
            exit(1);
        }
    }
}

