#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>

#ifndef BFI_HPP
#define BFI_HPP

// Brainfuck interpreter.

namespace bfi {

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

}  // namespace bfi

#endif  // #ifndef BFI_HPP