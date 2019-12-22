#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include "expression.hpp"
#include "bf_space.hpp"
#include <memory>
#include <vector>

class Statement {
public:
    virtual ~Statement() {}
    void evaluate(BfSpace* bf) {
        auto i = bf->indent();
        *bf << Comment{DebugString()};
        evaluate_impl(bf);
    }
    virtual void evaluate_impl(BfSpace* bf) const = 0;
    virtual std::string DebugString() const = 0;
};

class VarDeclaration : public Statement {
public:
    VarDeclaration(Token name, std::unique_ptr<Expression> initializer) : name_(std::move(name)), initializer_(std::move(initializer)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string DebugString() const override;
 private:
    Token name_;
    std::unique_ptr<Expression> initializer_;
};

class Putc : public Statement {
public:
    Putc(std::unique_ptr<Expression> value) : value_(std::move(value)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string DebugString() const override;
 private:
    std::unique_ptr<Expression> value_;
};

class Print : public Statement {
public:
    Print(std::unique_ptr<Expression> value) : value_(std::move(value)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string DebugString() const override;
 private:
    std::unique_ptr<Expression> value_;
};

class ExpressionStatement : public Statement {
public:
    ExpressionStatement(std::unique_ptr<Expression> value) : value_(std::move(value)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string DebugString() const override;
 private:
    std::unique_ptr<Expression> value_;
};

class Block : public Statement {
public:
    Block(std::vector<std::unique_ptr<Statement>> statements) : statements_(std::move(statements)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string DebugString() const override;
 private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

class If : public Statement {
public:
    If(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> then_branch, std::unique_ptr<Statement> else_branch)
      : condition_(std::move(condition)), then_branch_(std::move(then_branch)), else_branch_(std::move(else_branch)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string DebugString() const override;
 private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> then_branch_;
    std::unique_ptr<Statement> else_branch_;
};

class While : public Statement {
public:
    While(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
      : condition_(std::move(condition)), body_(std::move(body)) {}
    void evaluate_impl(BfSpace* bf) const override;
    std::string DebugString() const override;
 private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> body_;
};

#endif  // STATEMENT_HPP