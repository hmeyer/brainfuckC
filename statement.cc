#include "statement.hpp"
#include "scanner.hpp"
#include "parser.hpp"

void Statement::evaluate(BfSpace* bf) const {
    auto i = bf->indent();
    *bf << Comment{Description()};
    auto not_pending = bf->op_not(bf->get_call_pending());
    auto correct_return_position = [this, bf]() { return bf->op_le(bf->get_return_position(), bf->addTempWithValue(bf->num_function_calls())); };
    bf->op_if_then_else(bf->op_and(std::move(not_pending), std::move(correct_return_position)),
                        [this, bf](){evaluate_impl(bf);});
}

void VarDeclaration::evaluate_impl(BfSpace* bf) const {
    auto v = bf->add(std::get<std::string>(name_.value));
    if (initializer_ != nullptr) {
        bf->copy(initializer_->evaluate(bf), v);
    }
}

std::string VarDeclaration::Description() const {
    std::string s = "var " + name_.DebugString();
    if (initializer_) {
        s += " = " + initializer_->DebugString();
    }
    return s + ";";
}

std::string VarDeclaration::DebugString() const {
    return Description();
}

void Putc::evaluate_impl(BfSpace* bf) const {
    Variable w = value_->evaluate(bf);
    *bf << w << ".";
}

std::string Putc::Description() const {
    return "putc(" + value_->DebugString() + ");";
}

std::string Putc::DebugString() const {
    return Description();
}

void ExpressionStatement::evaluate_impl(BfSpace* bf) const {
    value_->evaluate(bf);
}

std::string ExpressionStatement::Description() const {
    return value_->DebugString() + ";";
}

std::string ExpressionStatement::DebugString() const {
    return Description();
}

void Block::evaluate_impl(BfSpace* bf) const {
    auto popper = bf->push_scope();
    for (const auto& s : statements_) {
        s->evaluate(bf);
    }
}

std::string Block::Description() const {
    return "";
}

std::string Block::DebugString() const {
    std::string result;
    for (const auto& s : statements_) {
        if (!result.empty()) result += "\n";
        result += s->DebugString();
    }
    return result;
}

void If::evaluate_impl(BfSpace* bf) const {
    bf->op_if_then_else(condition_->evaluate(bf), [&](){then_branch_->evaluate(bf);}, [&](){else_branch_->evaluate(bf);});
}

std::string If::Description() const {
    return "if (" + condition_->DebugString() + ")";
}

std::string If::DebugString() const {
    std::string result = Description() + " " + then_branch_->DebugString();
    if (else_branch_ != nullptr) {
        result += " else " + else_branch_->DebugString();
    }
    return result;
}

void While::evaluate_impl(BfSpace* bf) const {
    auto c = bf->wrap_temp(condition_->evaluate(bf));
    *bf << c << "[";
    body_->evaluate(bf);
    auto t = condition_->evaluate(bf);
    bf->copy(t, c);
    *bf << c << "]";
}

std::string While::Description() const {
    return "while (" + condition_->DebugString() + ")";
}

std::string While::DebugString() const {
    return Description() + " " + body_->DebugString();
}

void Function::evaluate_impl(BfSpace* bf) const {
    auto popper = bf->push_scope();
    for(const auto& p : parameters_) {
        bf->add(std::get<std::string>(p.value));
    }
    body_->evaluate(bf);
}

std::string Function::Description() const {
    std::string parameters;
    for (const auto& p : parameters_) {
        if (!parameters.empty()) parameters += "; ";
        parameters += std::get<std::string>(p.value);
    }
    return "fun " + std::get<std::string>(name_.value) + "(" + parameters + ")";
}

std::string Function::DebugString() const {
    return Description() + " {\n" + body_->DebugString() + "\n}";
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

void Call::evaluate_impl(BfSpace* bf) const {
    auto callee = std::get<std::string>(callee_.value);
    if (callee == "print") {
        print(bf);
        return;
    }
    std::vector<Variable> argument_vars;
    for(const auto& a : arguments_) {
        argument_vars.push_back(a->evaluate(bf));
    }
    bf->op_call_function(callee, std::move(argument_vars));
    return;
}

std::string Call::Description() const {
    std::string args;
    for (const auto& a : arguments_) {
        if (!args.empty()) args += "; ";
        args += a->DebugString();
    }
    return std::get<std::string>(callee_.value) + "(" + args + ")";
}

std::string Call::DebugString() const {
    return Description();
}