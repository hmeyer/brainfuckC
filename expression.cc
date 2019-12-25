#include "expression.hpp"
#include "statement.hpp"
#include "scanner.hpp"
#include "parser.hpp"


Variable Binary::evaluate_impl(BfSpace* bf) {
    Variable x = left_->evaluate(bf);
    Variable y = right_->evaluate(bf);
    switch (op_.type) {
        case PLUS: return bf->op_add(std::move(x), std::move(y));
        case MINUS: return bf->op_sub(std::move(x), std::move(y));
        case STAR: return bf->op_mul(std::move(x), std::move(y));
        case SLASH: return bf->op_div(std::move(x), std::move(y));
        case GREATER: return bf->op_lt(std::move(y), std::move(x));
        case LESS: return bf->op_lt(std::move(x), std::move(y));
        case GREATER_EQUAL: return bf->op_le(std::move(y), std::move(x));
        case LESS_EQUAL: return bf->op_le(std::move(x), std::move(y));
        case EQUAL_EQUAL: return bf->op_eq(std::move(x), std::move(y));
        case BANG_EQUAL: return bf->op_neq(std::move(x), std::move(y));
        default: assert(false);  // Should never happen.
    }
}

std::string Binary::DebugString() const { 
    return "(" + left_->DebugString() + op_.DebugString() + right_->DebugString() + ")";
}


Variable Unary::evaluate_impl(BfSpace* bf) {
    Variable x = right_->evaluate(bf);
    switch (op_.type) {
        case BANG:
            return bf->op_not(std::move(x));
        case MINUS:
            return bf->op_neg(std::move(x));
        default: assert(false);  // Should never happen.
    }
}

std::string Unary::DebugString() const { 
    return "(" + op_.DebugString() + right_->DebugString() + ")";
}

Variable Literal::evaluate_impl(BfSpace* bf) {
    assert(!std::holds_alternative<std::nullopt_t>(value_));
    if (std::holds_alternative<int>(value_)) {
        return bf->addTempWithValue(std::get<int>(value_));
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

void Call::print(BfSpace* bf) const {
    if (arguments_.size() != 1) { 
        throw std::invalid_argument("print should have exactly one argument at line " + std::to_string(callee_.line));
    }

    Variable p = bf->add_or_get("__print_value");
    bf->copy(arguments_[0]->evaluate(bf), p);
    static const Statement* const kPrintStatement = [](){
        constexpr char printer[] = R"(
        {
            var old_power = 1;
            while(__print_value or old_power) {
                var digit = __print_value;
                var power = 1;

                while(digit > 9) {
                    digit = digit / 10;
                    power = power * 10;
                }

                if (power < old_power) {
                    putc('0');
                    old_power = old_power / 10;
                } else {
                    putc(digit + '0');
                    __print_value = __print_value - digit * power;
                    old_power = power / 10;
                }
            }
        }
        )";
        auto statements = Parser(Scanner(printer).scanTokens()).parse();
        assert(statements.size() == 1);
        return statements[0].release();
    }();
    kPrintStatement->evaluate_impl(bf);
}

Variable Call::evaluate_impl(BfSpace* bf) {
    auto callee = std::get<std::string>(callee_.value);
    if (callee == "print") {
        print(bf);
        return bf->addTemp();
    }
    int index = bf->lookup_function(callee, arguments_.size());
    return bf->addTemp();
}

std::string Call::DebugString() const {
    std::string args;
    for (const auto& a : arguments_) {
        if (!args.empty()) args += "; ";
        args += a->DebugString();
    }
    return std::get<std::string>(callee_.value) + "(" + args + ")";
}