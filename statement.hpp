#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include "expression.hpp"
#include "bf_space.hpp"
#include <memory>
#include <optional>
#include <vector>

class Statement {
public:
    virtual ~Statement() {}
    virtual void evaluate(BfSpace* bf) const;
    virtual void evaluate_impl(BfSpace* bf) const = 0;
    virtual std::string Description() const = 0;
    virtual std::string DebugString() const = 0;
    virtual int num_calls() const { return 0; }
};

class VarDeclaration : public Statement {
public:
    VarDeclaration(Token name, int size, std::vector<std::unique_ptr<Expression>> initializer) : name_(std::move(name)), size_(size), initializer_(std::move(initializer)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string Description() const override;
    std::string DebugString() const override;
 private:
    Token name_;
    int size_;
    std::vector<std::unique_ptr<Expression>> initializer_;
};

class Putc : public Statement {
public:
    Putc(std::unique_ptr<Expression> value) : value_(std::move(value)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string Description() const override;
    std::string DebugString() const override;
 private:
    std::unique_ptr<Expression> value_;
};

class ExpressionStatement : public Statement {
public:
    ExpressionStatement(std::unique_ptr<Expression> value) : value_(std::move(value)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string Description() const override;
    std::string DebugString() const override;
 private:
    std::unique_ptr<Expression> value_;
};

class Block : public Statement {
public:
    Block(std::vector<std::unique_ptr<Statement>> statements) : statements_(std::move(statements)) {}
    void evaluate(BfSpace* bf) const override { evaluate_impl(bf); }
    void evaluate_impl(BfSpace* bf) const override;
    std::string Description() const override;
    std::string DebugString() const override;
    int num_calls() const override;
 private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

class If : public Statement {
public:
    If(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> then_branch, std::unique_ptr<Statement> else_branch)
      : condition_(std::move(condition)), then_branch_(std::move(then_branch)), else_branch_(std::move(else_branch)) {}
    void evaluate(BfSpace* bf) const override { evaluate_impl(bf); }
    void evaluate_impl(BfSpace* bf) const override;
    std::string Description() const override;
    std::string DebugString() const override;
    int num_calls() const override { return then_branch_->num_calls() + else_branch_->num_calls(); }
 private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> then_branch_;
    std::unique_ptr<Statement> else_branch_;
};

class While : public Statement {
public:
    While(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
      : condition_(std::move(condition)), body_(std::move(body)) {}
    void evaluate(BfSpace* bf) const override;
    void evaluate_impl(BfSpace* bf) const override;
    std::string Description() const override;
    std::string DebugString() const override;
    int num_calls() const override { return body_->num_calls(); }
 private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> body_;
};

class Function : public Statement {
public:
    Function(Token name, std::vector<Token> parameters, std::unique_ptr<Statement> body)
      : name_(std::move(name)), parameters_(std::move(parameters)), body_(std::move(body)){}
    void set_body(std::unique_ptr<Statement> body) { body_ = std::move(body); }
    void evaluate(BfSpace* bf) const override { evaluate_impl(bf); }
    void evaluate_impl(BfSpace* bf) const override;
    std::string Description() const override;
    std::string DebugString() const override;
    const std::string& name() const { return std::get<std::string>(name_.value); }
    int arity() const { return parameters_.size(); }
 private:
    Token name_;
    std::vector<Token> parameters_;
    std::unique_ptr<Statement> body_;
};


class Call : public Statement {
public:
    Call(Token callee, std::vector<std::unique_ptr<Expression>> arguments) : callee_(std::move(callee)), arguments_(std::move(arguments)) {}
    void evaluate(BfSpace* bf) const override;
    void evaluate_impl(BfSpace* bf) const override;
    std::string Description() const override;
    std::string DebugString() const override;
    int arity() const { return arguments_.size(); }
    int num_calls() const override { return 1; }
 private:
    Token callee_;
    std::vector<std::unique_ptr<Expression>> arguments_;
};

#endif  // STATEMENT_HPP