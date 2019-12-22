#ifndef BF_SPACE_HPP
#define BF_SPACE_HPP

#include <string>
#include <unordered_map>
#include <set>
#include <cassert>
#include <memory>


class Variable;

class Env {
public:
    Env() {}
    explicit Env(std::unique_ptr<Env> parent): parent_(std::move(parent)), next_free_(parent_->next_free_) {}
    Variable add(const std::string& name, int size = 1);
    Variable add_or_get(const std::string& name, int size = 1);
    Variable get(const std::string& name);
    Variable addTemp(int size = 1);
    void remove(int index);
    std::unique_ptr<Env> release_parent() { return std::move(parent_); }

    private:
    int next_free(int size);

    std::unique_ptr<Env> parent_;
    std::unordered_map<std::string, int> vars_;
    std::unordered_map<int, int> temp_sizes_;
    std::set<int> free_list_;
    int next_free_ = 0;
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
        ~Emitter() {
            if (parent_ != nullptr) {
                parent_->code_ += "\n";
            }
        }

        private:
        Emitter(BfSpace* parent): parent_(parent) {
            parent_->code_ += std::string(parent_->indent_ * 2, ' ');
        }

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

    BfSpace(): env_(std::make_unique<Env>()) {}
    const std::string& code() const { return code_; }
    Emitter operator<<(std::string_view s);
    Emitter operator<<(const Variable& v);
    Emitter operator<<(const Comment& c);

    Variable add(const std::string& name, int size = 1) { return env_->add(name, size); }
    Variable add_or_get(const std::string& name, int size = 1) { return env_->add_or_get(name, size); }
    Variable get(const std::string& name)  { return env_->get(name); }
    Variable addTemp(int size = 1) { return env_->addTemp(size); }
    Variable wrap_temp(Variable v);

    void copy(const Variable& src, const Variable& dst);
    void push_scope();
    void pop_scope();
    Indent indent() { return Indent(&indent_); }

    private:
        void moveTo(int pos);

        std::string code_;
        int current_tape_pos_ = 0;
        std::unique_ptr<Env> env_;
        int indent_ = 0;
};


#endif  // BF_SPACE_HPP