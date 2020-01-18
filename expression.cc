#include "expression.hpp"


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
    return bf->addTempWithValue(value_);
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::string Literal::DebugString() const { 
    return std::to_string(value_);
}

Variable VariableExpression::evaluate_impl(BfSpace* bf) {
    auto var = bf->get(std::get<std::string>(name_.value));
    if (index_ != nullptr) {
        auto index = index_->evaluate(bf);
        return bf->op_array_read(std::move(var), std::move(index));
    }
    return var;
}

std::string VariableExpression::DebugString() const {
    return "variable(" + name_.DebugString() + ")";
}

Variable Assignment::evaluate_impl(BfSpace* bf) {
    Variable left = bf->get(std::get<std::string>(left_.value));
    Variable right = right_->evaluate(bf);
    if (left_index_ != nullptr) {
        Variable index = left_index_->evaluate(bf);
        auto right_copy = bf->addTempAsCopy(right);
        bf->op_array_write(std::move(left), std::move(index), std::move(right_copy));
    }
    bf->copy(right, left);
    return right;
}

std::string Assignment::DebugString() const {
    return left_.DebugString() + " = " + right_->DebugString();
}

Variable Logical::evaluate_impl(BfSpace* bf) {
    Variable x = left_->evaluate(bf);
    auto y = [this, bf](){ return right_->evaluate(bf); };
    switch (op_.type) {
        case AND: return bf->op_and(std::move(x), std::move(y));
        case OR: return bf->op_or(std::move(x), std::move(y));
        default: throw std::runtime_error("unknown logical operator: " + op_.DebugString());
    }
}

std::string Logical::DebugString() const {
    return left_->DebugString() + op_.DebugString() + right_->DebugString();
}
