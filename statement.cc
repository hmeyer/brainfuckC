#include "statement.hpp"
#include "scanner.hpp"
#include "parser.hpp"
#include <numeric>

namespace {

Variable return_position_condition(BfSpace* bf) {
    return bf->op_le(bf->get_return_position(),
                        bf->addTempWithValue(bf->num_function_calls()));
}

Variable statement_condition(BfSpace* bf) {
    return bf->op_and(bf->get_call_not_pending(), [=](){return return_position_condition(bf);});
}

}  // namespace

void Statement::evaluate(BfSpace* bf) const {
    auto i = bf->indent();
    *bf << Comment{Description()};
    bf->op_if_then(statement_condition(bf), [this, bf](){evaluate_impl(bf);});
}

void VarDeclaration::evaluate_impl(BfSpace* bf) const {
    int array_head_size = (size_ == 1) ? 0 : 4;
    auto v = bf->add(std::get<std::string>(name_.value), size_ + array_head_size);
    for(int i = 0; i < initializer_.size(); i++) {
        bf->copy(initializer_[i]->evaluate(bf), v.get_successor(i + array_head_size));
    }
}

std::string VarDeclaration::Description() const {
    std::string s = "var " + name_.DebugString();
    if (!initializer_.empty()) {
        std::string seperator = " = ";
        for(const auto& i : initializer_) {
         s += seperator + i->DebugString();
         seperator = "; ";
        }
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

int Block::num_calls() const {
    return std::accumulate(statements_.begin(), statements_.end(), 0,
                           [](int a, const std::unique_ptr<Statement>& b) {
                               return a + b->num_calls();
                           });    
}

namespace {

Variable condition_add_return_pos_check(BfSpace* bf, Variable cond, int num_calls) {
    int current_calls = bf->num_function_calls();
    assert(num_calls >= 0);
    switch(num_calls) {
        case 0: return std::move(cond);
        case 1: return bf->op_or(std::move(cond), [=](){return bf->op_eq(
            bf->get_return_position(), bf->addTempWithValue(current_calls + 1));}
        );
        default: return bf->op_or(std::move(cond), [=](){return bf->op_and(
            bf->op_lt(bf->addTempWithValue(current_calls), bf->get_return_position()),
            [=](){return bf->op_le(bf->get_return_position(), bf->addTempWithValue(current_calls + num_calls));}
        );});
    }
}

}  // namespace

void If::evaluate_impl(BfSpace* bf) const {
    auto cond = condition_->evaluate(bf);
    std::optional<Variable> else_cond;
    if (else_branch_ != nullptr) {
        auto not_cond = bf->op_not(bf->addTempAsCopy(cond));
        else_cond = bf->op_and(std::move(not_cond), [=](){return return_position_condition(bf);});
    }

    auto then_cond = bf->op_and(std::move(cond), [=](){return return_position_condition(bf);});
    auto then_or_return_cond = condition_add_return_pos_check(bf, std::move(then_cond), then_branch_->num_calls());
    auto not_pending_and_then_or_return_cond = bf->op_and(std::move(then_or_return_cond), [=](){return bf->get_call_not_pending();});
    bf->op_if_then(std::move(not_pending_and_then_or_return_cond), [&](){then_branch_->evaluate(bf);});

    if (else_cond.has_value()) {
        auto else_or_return_cond = condition_add_return_pos_check(bf, std::move(*else_cond), then_branch_->num_calls());
        auto not_pending_and_else_or_return_cond = bf->op_and(std::move(else_or_return_cond), [=](){return bf->get_call_not_pending();});
        bf->op_if_then(std::move(not_pending_and_else_or_return_cond), [&](){else_branch_->evaluate(bf);});
    }
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

void While::evaluate(BfSpace* bf) const {
    auto i = bf->indent();
    *bf << Comment{Description()};
    evaluate_impl(bf);
}

void While::evaluate_impl(BfSpace* bf) const {
    auto while_cond = bf->op_and(condition_->evaluate(bf), [=](){return return_position_condition(bf);});
    auto while_or_return_cond = condition_add_return_pos_check(bf, std::move(while_cond), body_->num_calls());
    auto final_cond = bf->op_and(std::move(while_or_return_cond), [=](){return bf->get_call_not_pending();});

    *bf << final_cond << "[";
    body_->evaluate(bf);

    auto repeating_while_cond = bf->op_and(bf->get_call_not_pending(), [=](){return condition_->evaluate(bf);});

    bf->copy(repeating_while_cond, final_cond);
    *bf << final_cond << "]";
}

std::string While::Description() const {
    return "while (" + condition_->DebugString() + ")";
}

std::string While::DebugString() const {
    return Description() + " " + body_->DebugString();
}

void Function::evaluate_impl(BfSpace* bf) const {
    auto popper = bf->push_scope();
    for(int i = 0; i < parameters_.size(); i++) {
        bf->register_parameter(i, std::get<std::string>(parameters_[i].value));
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

void Call::evaluate(BfSpace* bf) const {
    auto i = bf->indent();
    *bf << Comment{Description()};
    bf->op_if_then(statement_condition(bf), [this, bf](){evaluate_impl(bf);});

    auto my_return_cond = bf->op_eq(bf->get_return_position(), bf->addTempWithValue(bf->num_function_calls()));
    auto my_return_and_not_pending = bf->op_and(std::move(my_return_cond), [=](){return bf->get_call_not_pending();});
    bf->op_if_then(std::move(my_return_and_not_pending), [=](){
        *bf << Comment{"Decrease return position since we might be in a while loop"} << bf->get_return_position() << "-";
    });
}

void Call::evaluate_impl(BfSpace* bf) const {
    auto callee = std::get<std::string>(callee_.value);
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