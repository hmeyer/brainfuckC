#include "expression.hpp"

namespace {

Variable lt(BfSpace* bf, Variable _x, Variable y) {
    Variable x = bf->wrap_temp(std::move(_x));
    Variable temp0 = bf->addTemp();
    Variable temp1 = bf->addTemp(3);
    *bf << Comment{"lt(" + x.DebugString() + ";" +  y.DebugString() + ")"};
    *bf << temp0 << "[-]";
    *bf << temp1 << "[-] >[-]+ >[-] <<"; 
    *bf << y << "[" << temp0 << "+" << temp1 << "+" << y << "-]"; 
    *bf << temp0 << "[" << y << "+" << temp0 << "-]"; 
    *bf << x << "[" << temp0 << "+" << x << "-]+"; 
    *bf << temp1 << "[>-]> [< " << x  << "-" << temp0 << "[-]" << temp1 << ">->]<+<"; 
    *bf << temp0 << "[" << temp1 << "- [>-]> [<" << x << "-" << temp0 << "[-]+" << temp1 << ">->]<+<" << temp0 << "-]"; 
    *bf << "\n";
    return x;
}

Variable le(BfSpace* bf, Variable _x, Variable y) {
    Variable x = bf->wrap_temp(std::move(_x));
    Variable temp0 = bf->addTemp();
    Variable temp1 = bf->addTemp(3);
    *bf << Comment{"le(" + x.DebugString() + ";" +  y.DebugString() + ")"};
    *bf << temp0 << "[-]"; 

    *bf << temp1 << "[-] >[-]+ >[-] <<"; 
    *bf << y << "[" << temp0 << "+ " << temp1 << "+ " << y << "-]"; 
    *bf << temp1 << "[" << y << "+ " << temp1 << "-]"; 
    *bf << x << "[" << temp1 << "+ " << x << "-]"; 
    *bf << temp1 << "[>-]> [< " << x << "+ " << temp0 << "[-] " << temp1 << ">->]<+<"; 
    *bf << temp0 << "[" << temp1 << "- [>-]> [< " << x << "+ " << temp0 << "[-]+ " << temp1 << ">->]<+< " << temp0 << "-]"; 
    return x;
}

}  // namespace

Variable Binary::evaluate_impl(BfSpace* bf) {
    Variable x = bf->wrap_temp(left_->evaluate(bf));
    Variable y = bf->wrap_temp(right_->evaluate(bf));
    switch (op_.type) {
        case PLUS:
            *bf << y << "[-" << x << "+" << y << "]";
            return x;
        case MINUS:
            *bf << y << "[-" << x << "-" << y << "]";
            return x;
        case STAR: {
            Variable t0 = bf->addTemp();
            Variable t1 = bf->addTemp();
            *bf << t0 << "[-]";
            *bf << t1 << "[-]";
            *bf << x << "[" << t1 << "+" << x << "-]";
            *bf << t1 << "[";
            *bf << y << "[" << x << "+" << t0 << "+" << y << "-]" << t0 << "[" << y << "+" << t0 << "-]";
            *bf << t1 << "-]";
            return x;
        }
        case SLASH: {
            Variable t0 = bf->addTemp();
            Variable t1 = bf->addTemp();
            Variable t2 = bf->addTemp();
            Variable t3 = bf->addTemp();

            *bf << t0 << "[-]";
            *bf << t1 << "[-]";
            *bf << t2 << "[-]";
            *bf << t3 << "[-]";
            *bf << x << "[" << t0 << "+" << x<< "-]";
            *bf << t0 << "[";
            *bf << y << "[" << t1 << "+" << t2 << "+" << y << "-]";
            *bf << t2 << "[" << y<< "+" << t2 << "-]";
            *bf << t1 << "[";
            *bf <<   t2 << "+";
            *bf <<   t0 << "-" << "[" << t2 << "[-]" << t3 << "+" << t0 << "-]";
            *bf <<   t3 << "[" << t0 << "+" << t3 << "-]";
            *bf <<   t2 << "[";
            *bf <<    t1 << "-";
            *bf <<    "[" << x << "-" << t1 << "[-]]+";
            *bf <<   t2 << "-]";
            *bf <<  t1 << "-]";
            *bf <<  x << "+";
            *bf << t0 << "]";            
            return x;
        }
        case GREATER: return lt(bf, std::move(y), std::move(x));
        case LESS: return lt(bf, std::move(x), std::move(y));
        case GREATER_EQUAL: return le(bf, std::move(y), std::move(x));
        case LESS_EQUAL: return le(bf, std::move(x), std::move(y));
        default: assert(false);  // Should never happen.
    }
}

std::string Binary::DebugString() const { 
    return "(" + left_->DebugString() + op_.DebugString() + right_->DebugString() + ")";
}


Variable Unary::evaluate_impl(BfSpace* bf) {
    Variable x = bf->wrap_temp(right_->evaluate(bf));
    Variable t = bf->addTemp();
    switch (op_.type) {
        case BANG:
            *bf << t << "[-]";
            *bf << x << "[" << t << "+" << x << "[-]]+";
            *bf << t << "[" << x << "-" << t << "-]";
            return x;
            break;
        case MINUS:
            *bf << t << "[-]";
            *bf << x << "[" << t << "-" << x << "-]";
            *bf << t << "[" << x << "-" << t << "+]";
            return x;
        default: assert(false);  // Should never happen.
    }
    return x;
}

std::string Unary::DebugString() const { 
    return "(" + op_.DebugString() + right_->DebugString() + ")";
}

Variable Literal::evaluate_impl(BfSpace* bf) {
    assert(!std::holds_alternative<std::nullopt_t>(value_));
    if (std::holds_alternative<int>(value_)) {
        int v = std::get<int>(value_);
        Variable t = bf->addTemp();
        *bf << Comment{t.DebugString() + "=" + std::to_string(v)}; 
        *bf << t << "[-]";
        char op = '+';
        if (v < 0) {
            op = '-';
            v = -v;
        }
        *bf << t; 
        *bf << std::string(v, op);
        *bf << "\n\n";
        return t;
    }
    if (std::holds_alternative<std::string>(value_)) {
        throw std::runtime_error("cannot handle string literals yet");
    }
    assert(false);  // should never reach here.
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::string Literal::DebugString() const { 
    std::string result;
    std::visit(overloaded {
        [&result](std::nullopt_t) { result = "nullopt_t"; },
        [&result](int v) { result = std::to_string(v); },
        [&result](std::string v) { result = v; },
    }, value_);
    return result;
}

Variable VariableExpression::evaluate_impl(BfSpace* bf) {
    return bf->get(std::get<std::string>(name_.value));
}

std::string VariableExpression::DebugString() const {
    return "variable(" + name_.DebugString() + ")";
}

Variable Assignment::evaluate_impl(BfSpace* bf) {
    Variable x = bf->get(std::get<std::string>(left_.value));
    Variable y = right_->evaluate(bf);
    bf->copy(y, x);
    return x;
}

std::string Assignment::DebugString() const {
    return left_.DebugString() + " = " + right_->DebugString();
}

Variable Logical::evaluate_impl(BfSpace* bf) {
    Variable t = bf->wrap_temp(left_->evaluate(bf));
    Variable result = bf->addTemp();
    *bf << result << "[-]";
    switch (op_.type) {
        case AND: {
            *bf << t << "[";
            Variable y = right_->evaluate(bf);
            bf->copy(y, t);
            *bf <<   t << "[";
            *bf <<   result << "+";
            *bf <<   t << "[-]]";
            *bf << t << "]";
            return result;
        }
        case OR: {
            Variable flag = bf->addTemp();
            *bf << flag << "[-]+";  // Set t = 1
            *bf << t << "[" << result << "+" << flag << "-" << t << "[-]]";
            *bf << flag << "[" << flag << "-";
            bf->copy(right_->evaluate(bf), t);
            *bf << t << "[" << result << "+" << t << "[-]]";
            *bf << flag << "]";
            return result;
        }
        default: throw std::runtime_error("unknown logical operator: " + op_.DebugString());
    }
}

std::string Logical::DebugString() const {
    return left_->DebugString() + op_.DebugString() + right_->DebugString();
}