#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP

#include "bf_space.hpp"
#include "token.hpp"
#include <memory>
#include <vector>

class Expression {
public:
    virtual ~Expression() {}
    Variable evaluate(BfSpace* bf) {
        auto i = bf->indent();
        *bf << Comment{DebugString()};
        return evaluate_impl(bf);
    }
    virtual Variable evaluate_impl(BfSpace* bf) = 0;
    virtual std::string DebugString() const = 0;
};

class Binary : public Expression {
public:
    Binary(std::unique_ptr<Expression> left, Token op, std::unique_ptr<Expression> right): left_(std::move(left)), op_(std::move(op)), right_(std::move(right)) {}
    Variable evaluate_impl(BfSpace* bf) override;
    std::string DebugString() const override;

private:
    std::unique_ptr<Expression> left_;
    Token op_;
    std::unique_ptr<Expression> right_;
};

class Unary : public Expression {
public:
    Unary(Token op, std::unique_ptr<Expression> right): op_(std::move(op)), right_(std::move(right)) {}
    Variable evaluate_impl(BfSpace* bf) override;
    std::string DebugString() const override;

private:
    Token op_;
    std::unique_ptr<Expression> right_;
};

class Literal : public Expression {
public:
    Literal(TokenValue value): value_(std::move(value)) {}
    Variable evaluate_impl(BfSpace* bf) override;
    std::string DebugString() const override;

private:
    TokenValue value_;
};

class VariableExpression : public Expression {
public:
    VariableExpression(Token name): name_(std::move(name)) {}
    Variable evaluate_impl(BfSpace* bf) override;
    std::string DebugString() const override;
    const Token& name_token() const { return name_; }

private:
    Token name_;
};


class Assignment : public Expression {
public:
    Assignment(Token left, std::unique_ptr<Expression> right) : left_(std::move(left)), right_(std::move(right)) {}
    Variable evaluate_impl(BfSpace* bf) override;
    std::string DebugString() const override;
 private:
    Token left_;
    std::unique_ptr<Expression> right_;
};

class Logical : public Expression {
public:
    Logical(std::unique_ptr<Expression> left, Token op, std::unique_ptr<Expression> right) : left_(std::move(left)), op_(std::move(op)), right_(std::move(right)) {}
    Variable evaluate_impl(BfSpace* bf) override;
    std::string DebugString() const override;
 private:
    std::unique_ptr<Expression> left_;
    Token op_;
    std::unique_ptr<Expression> right_;
};

class Call : public Expression {
public:
    Call(Token callee, std::vector<std::unique_ptr<Expression>> arguments) : callee_(std::move(callee)), arguments_(std::move(arguments)) {}
    Variable evaluate_impl(BfSpace* bf) override;
    std::string DebugString() const override;
    int arity() const { return arguments_.size(); }
 private:
    void print(BfSpace* bf) const;
    Token callee_;
    std::vector<std::unique_ptr<Expression>> arguments_;
};

#endif  // EXPRESSION_HPP