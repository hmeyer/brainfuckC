#include "statement.hpp"
#include "scanner.hpp"
#include "parser.hpp"


void VarDeclaration::evaluate_impl(BfSpace* bf) const {
    auto v = bf->add(std::get<std::string>(name_.value));
    if (initializer_ != nullptr) {
        bf->copy(initializer_->evaluate(bf), v);
    }
}

std::string VarDeclaration::DebugString() const {
    std::string s = "var " + name_.DebugString();
    if (initializer_) {
        s += " = " + initializer_->DebugString();
    }
    return s + ";";
}

void Putc::evaluate_impl(BfSpace* bf) const {
    Variable w = value_->evaluate(bf);
    *bf << w << ".";
}

std::string Putc::DebugString() const {
    return "putc(" + value_->DebugString() + ");";
}


void Print::evaluate_impl(BfSpace* bf) const {
    Variable p = bf->add_or_get("__print_value");
    bf->copy(value_->evaluate(bf), p);
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

std::string Print::DebugString() const {
    return "print(" + value_->DebugString() + ");";
}

void ExpressionStatement::evaluate_impl(BfSpace* bf) const {
    value_->evaluate(bf);
}

std::string ExpressionStatement::DebugString() const {
    return value_->DebugString() + ";";
}

void Block::evaluate_impl(BfSpace* bf) const {
    bf->push_scope();
    for (const auto& s : statements_) {
        s->evaluate(bf);
    }
    bf->pop_scope();
}

std::string Block::DebugString() const {
    std::string result;
    for (const auto& s : statements_) {
        result += s->DebugString() + " ";
    }
    return result;
}

void If::evaluate_impl(BfSpace* bf) const {
    auto c = bf->wrap_temp(condition_->evaluate(bf));
    if (else_branch_ == nullptr) {
        *bf << c << "[";
        then_branch_->evaluate(bf);
        *bf << c << "[-]]";
    } else {
        Variable t0 = bf->addTemp();
        *bf << t0 << "[-]+";
        *bf << c << "[";
        then_branch_->evaluate(bf);
        *bf <<  t0 << "-";
        *bf <<  c << "[-]";
        *bf << "]";
        *bf << t0 << "[";
        else_branch_->evaluate(bf);
        *bf << t0 << "-]";        
    }
}

std::string If::DebugString() const {
    std::string result = "if (" + condition_->DebugString() + ") " + then_branch_->DebugString();
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

std::string While::DebugString() const {
    return "while (" + condition_->DebugString() + ") ";
}
