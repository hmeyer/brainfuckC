#ifndef BF_SPACE_HPP
#define BF_SPACE_HPP

#include <string>
#include <unordered_map>
#include <set>
#include <cassert>
#include <memory>
#include <vector>
#include <functional>
#include <iostream>


class Variable;
class Function;

class Env {
public:
    explicit Env(int named_reservation_size = 0): named_reservation_size_(named_reservation_size), next_free_(named_reservation_size_) {}
    explicit Env(std::unique_ptr<Env> parent, int min_next_free = 0)
      : parent_(std::move(parent)),
        named_reservation_size_(parent_->named_reservation_size_),
        next_free_(std::max(std::max(min_next_free, parent_->next_free_), named_reservation_size_)) {
        }
    Variable add(const std::string& name, int size = 1);
    Variable add_alias(const std::string& original, const std::string& alias);
    Variable add_or_get(const std::string& name, int size = 1);
    Variable get(const std::string& name);
    Variable addTemp(int size = 1);
    void remove(int index);
    std::unique_ptr<Env> release_parent() { return std::move(parent_); }
    int top() const { return next_free_; }
    int num_named_cells() const;

    private:
    int next_free(int size);

    std::unique_ptr<Env> parent_;
    std::unordered_map<std::string, int> vars_;
    std::unordered_map<int, int> temp_sizes_;
    std::set<int> free_list_;
    int named_reservation_size_ = 0;
    int next_free_ = 0;
    int num_named_cells_ = 0;
};

class Variable {
    public:
    Variable(Env* parent, int index, std::string name): parent_(parent), index_(index), name_(std::move(name)) {
        assert(parent != nullptr);
    }
    Variable(Variable&& other): parent_(other.parent_), index_(other.index_), name_(std::move(other.name_)) {
        other.parent_ = nullptr;
    }
    Variable& operator=(const Variable& other) = delete;
    Variable get_predecessor(int num = 1) const { return Variable{parent_, index_ - num, DebugString() + "_" + std::to_string(num) + "predecessor"}; }

    ~Variable() {
        if (is_temp() && parent_ != nullptr) {
            parent_->remove(index_);
        }
    }
    int index() const { return index_; }
    bool is_temp() const { return name_.empty(); }
    std::string DebugString() const;

    private:
    Env* parent_;
    int index_;
    std::string name_;
};


struct Comment {
    std::string value;
};

struct Verbatim {
    std::string value;
};

class FunctionStorage {
    public:
    struct IndexedFunction {
        size_t index;
        int arity;
        const Function* function;
    };
    int lookup_function(std::string_view name, int arity);
    void define_function(const Function& function);
    const std::unordered_map<std::string, IndexedFunction>& functions() const { return functions_; }
    int max_arity() const { return max_arity_; }

    private:

    std::unordered_map<std::string, IndexedFunction> functions_;
    int max_arity_ = 1;
};

class BfSpace {
    public:
    class Emitter {
        public:
        Emitter(Emitter&& other): parent_(other.parent_) {
            other.parent_ = nullptr;
        }
        Emitter& operator=(const Emitter&) = delete;
        Emitter& operator<<(std::string_view s);
        Emitter& operator<<(const Variable& v);
        Emitter& operator<<(const Comment& c);
        Emitter& operator<<(const Verbatim& v);
        ~Emitter() {
            if (parent_ != nullptr) {
                parent_->append_code("\n");
            }
        }

        private:
        Emitter(BfSpace* parent): parent_(parent) {}

        BfSpace* parent_;
        friend class BfSpace;
    };

    class Indent {
        public:
        ~Indent() { *i_ -= 1; }
        private:
        Indent(int* i) : i_(i) { *i_ += 1; }
        Indent(const Indent&) = delete;
        Indent& operator=(const Indent&) = delete;
        int* i_;
        friend class BfSpace;
    };

    BfSpace();
    std::string code();
    Emitter operator<<(std::string_view s);
    Emitter operator<<(const Variable& v);
    Emitter operator<<(const Comment& c);
    Emitter operator<<(const Verbatim& v);

    Variable add(const std::string& name, int size = 1) { return env_->add(name, size); }
    Variable add_or_get(const std::string& name, int size = 1) { return env_->add_or_get(name, size); }
    void register_parameter(int num, const std::string& name);
    Variable get(const std::string& name) const  { return env_->get(name); }
    Variable get_return_position() const;
    Variable get_call_pending() const;
    Variable addTemp(int size = 1) { return env_->addTemp(size); }
    Variable addTempWithValue(int value);
    Variable wrap_temp(Variable v);

    Variable op_add(Variable x, Variable y);
    Variable op_sub(Variable x, Variable y);
    Variable op_mul(Variable x, Variable y);
    Variable op_div(Variable x, Variable y);
    Variable op_lt(Variable x, Variable y);
    Variable op_le(Variable x, Variable y);
    Variable op_eq(Variable x, Variable y);
    Variable op_neq(Variable x, Variable y);
    Variable op_neg(Variable x);
    Variable op_not(Variable x);
    Variable op_and(Variable x, std::function<Variable()> y);
    Variable op_or(Variable x, std::function<Variable()> y);
    void op_if_then_else(Variable condition, std::function<void()> then_branch, std::function<void()> else_branch = std::function<void()>());
    void op_call_function(const std::string& name, std::vector<Variable> arguments);

    void copy(const Variable& src, const Variable& dst);
    class [[nodiscard]] ScopePopper {
        public:
        explicit ScopePopper(BfSpace* bf) : bf_(bf) {}
        ScopePopper(const ScopePopper&) = delete;
        const ScopePopper& operator=(const ScopePopper&) = delete;
        ~ScopePopper() { bf_->pop_scope(); }
        private:
        BfSpace* bf_;

    };
    ScopePopper push_scope(int min_next_free = 0);
    void pop_scope();
    Indent indent() { return Indent(&indent_); }
    int lookup_function(std::string_view name, int arity) { return functions_->lookup_function(name, arity); }
    void register_functions(const std::vector<std::unique_ptr<Function>>& functions);
    int num_function_calls() const { return num_function_calls_; }

    private:
        void moveTo(int pos);
        void move_to_top() { moveTo(env_->top()); }
        // Whether of not this is the primary/ base space. The primary space needs to include the logic
        // for function dispatch.
        std::string generate_dispatch_wrapped_code();
        void reset_env_and_code();
        void finish_function_call(const std::string& name);
        void append_code(std::string_view t);

        std::string code_;
        int current_tape_pos_ = 0;
        std::unique_ptr<Env> env_;
        std::unique_ptr<FunctionStorage> functions_;
        int indent_;
        bool is_on_new_line_ = true;
        std::unordered_map<std::string, int> max_named_cells_per_function_call_;
        int num_function_calls_ = 0;
};


#endif  // BF_SPACE_HPP