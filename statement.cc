#include "statement.hpp"


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
        result += s->DebugString() + "\n";
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
    return "while (" + condition_->DebugString() + ") " + body_->DebugString();
}

void Function::evaluate_impl(BfSpace* bf) const {
    bf->define_function(*this);
}

std::string Function::DebugString() const {
    std::string parameters;
    for (const auto& p : parameters_) {
        if (!parameters.empty()) parameters += "; ";
        parameters += std::get<std::string>(p.value);
    }

    return "fun " + std::get<std::string>(name_.value) + "(" + parameters + ") {\n" + body_->DebugString() + "\n}";
}
