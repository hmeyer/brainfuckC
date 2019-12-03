#include "transformer.hpp"
#include "scanner.hpp"
#include <variant>
#include <cassert>

namespace nbf {

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Transformer::Transformer(std::string_view code):tokens_(Scanner(code).scanTokens()) {}

std::string Transformer::transform() {
    std::string result;
    for(const auto& t : tokens_) {
        std::visit(overloaded {
            [&result](BfToken arg) { result += arg; },
            [&result, this](Variable arg) { result += moveToVar(arg); },
        }, t);
    }
    return result;
}

std::string Transformer::moveToVar(const Variable& var) {
    int var_tape;
    auto [it, inserted] = var2tape_.insert(std::make_pair(var.name, max_tape_));
    auto [size_it, size_inserted] = var2size_.insert(std::make_pair(var.name, var.size));
    if (inserted) {
        assert(size_inserted);
        var_tape = max_tape_;
        max_tape_ += var.size;
    } else {
        if (size_it->second != var.size) {
            throw std::invalid_argument(var.name + " was previously registered with size " + std::to_string(size_it->second));
        }
        var_tape = it->second;

    }
    return moveTo(var_tape);
}

std::string Transformer::moveTo(int tape) {
    int delta = tape - current_tape_;
    current_tape_ = tape;
    char m = '>';
    if (delta < 0) {
        m = '<';
        delta = -delta;
    }
    return std::string(delta, m);
}


}  // namespace nbf