#include "bf_space.hpp"
#include "statement.hpp"

namespace {
int find_consecutive(const std::set<int>& s, int size, int next_free) {
    for(int start_pos : s) {
        bool matches = true;
        for(int i = 1; i < size; i++) {
            int test_index = start_pos + i;
            if (s.count(test_index) == 0 && test_index < next_free) {
                matches = false;
                break;
            }
        }
        if (matches) {
            return start_pos;
        }
    }
    return -1;
}
}  // namespace

std::string Variable::DebugString() const { 
    if (name_.empty()) {
        return "~t" + std::to_string(index_);
    } else {
        return name_ + std::to_string(index_);
    }
}


int Env::next_free(int size) {
    int start = find_consecutive(free_list_, size, next_free_);
    if (start == -1) {
        start = next_free_;
    }
    for(int i = 0; i < size; i++) {
        free_list_.erase(start + i);
    }
    next_free_ = std::max(next_free_, start + size);
    return start;
}

Variable Env::add(const std::string& name, int size) {
    auto [it, inserted] = vars_.insert(std::make_pair(std::string(name), 0));
    if (!inserted) {
        throw std::runtime_error("tried to insert already existing " + name);
    }
    it->second = next_free(size);
    return Variable(this, it->second, name);
}

Variable Env::add_or_get(const std::string& name, int size) {
    auto [it, inserted] = vars_.insert(std::make_pair(std::string(name), 0));
    if (!inserted) {
        return get(name);
    }
    it->second = next_free(size);
    return Variable(this, it->second, name);
}

Variable Env::get(const std::string& name) {
    auto it = vars_.find(name);
    if (it == vars_.end()) {
        if (parent_ != nullptr) {
            return parent_->get(name);
        }
        throw std::runtime_error("could not find " + name);
    }
    return Variable(this, it->second, name);
}

Variable Env::addTemp(int size) {
    int index = next_free(size);
    if (size != 1) { 
        temp_sizes_[index] = size;
    }
    return Variable(this, index, "");
}

Variable Env::addTempOnTop(int size) {
    int index = next_free_;
    next_free_ += size;
    if (size != 1) { 
        temp_sizes_[index] = size;
    }
    return Variable(this, index, "");
}

void Env::remove(int index) {
    if (index >= next_free_ || free_list_.count(index) > 0) {
        throw std::runtime_error("tried to remove non-existent index " + std::to_string(index));
    }

    int size = 1;
    auto it = temp_sizes_.find(index);
    if (it != temp_sizes_.end()) {
        size = it->second;
    }
    temp_sizes_.erase(index);
    for(; size > 0; size--) {
        free_list_.insert(index++);
    }
}

namespace {
    constexpr char kMyFunctionIndex[] = "__MyFunctionIndex";
    constexpr char kCalledFunctionIndex[] = "__CalledFunctionIndex";
    constexpr char kReturnPosition[] = "__ReturnPosition";
    constexpr char kReturnValue[] = "__ReturnValue";
}

BfSpace::BfSpace(BfSpace* parent): parent_(parent),
                                   env_(std::make_unique<Env>()),
                                   functions_(parent_!=nullptr?parent_->functions_:nullptr),
                                   function_vars_(functions_, env_.get()) {}


BfSpace::Emitter BfSpace::operator<<(std::string_view s) {
    Emitter e{this};
    e << s;
    return e;
}

BfSpace::Emitter BfSpace::operator<<(const Variable& v) {
    Emitter e{this};
    e << v;
    return e;
}

BfSpace::Emitter BfSpace::operator<<(const Comment& c) {
    Emitter e{this};
    e << c;
    return e;
}

BfSpace::Emitter& BfSpace::Emitter::operator<<(std::string_view s) {
    if (s.find_first_not_of(",.+-<>[] \n") != std::string::npos) {
        throw std::invalid_argument("Code contains non-brainfuck character: " + std::string(s));
    }
    parent_->code_ += s;
    return *this;
}

BfSpace::Emitter& BfSpace::Emitter::operator<<(const Variable& v) {
    parent_->code_ += v.DebugString();
    parent_->moveTo(v.index());
    return *this;
}

BfSpace::Emitter& BfSpace::Emitter::operator<<(const Comment& c) {
    if (c.value.find_first_of(",.+-<>[]") != std::string::npos) {
        throw std::invalid_argument(std::string("Comment contains brainfuck character: ") + c.value);
    }
    parent_->code_ += c.value;
    return *this;
}

Variable BfSpace::addTempWithValue(int value) {
    Variable t = addTemp();
    *this << Comment{t.DebugString() + "=" + std::to_string(value)}; 
    char op = '+';
    if (value < 0) {
        op = '-';
        value = -value;
    }
    *this << t << "[-]" << std::string(value, op);
    return t;
}

Variable BfSpace::op_add(Variable _x, Variable y) {
    Variable x = wrap_temp(std::move(_x));
    *this << y << "[-" << x << "+" << y << "]";
    return x;
}

Variable BfSpace::op_sub(Variable _x, Variable _y) {
    Variable x = wrap_temp(std::move(_x));
    Variable y = wrap_temp(std::move(_y));
    *this << y << "[-" << x << "-" << y << "]";
    return x;
}

Variable BfSpace::op_mul(Variable _x, Variable y) {
    Variable x = wrap_temp(std::move(_x));
    Variable t0 = addTemp();
    Variable t1 = addTemp();
    *this << t0 << "[-]";
    *this << t1 << "[-]";
    *this << x << "[" << t1 << "+" << x << "-]";
    *this << t1 << "[";
    *this << y << "[" << x << "+" << t0 << "+" << y << "-]" << t0 << "[" << y << "+" << t0 << "-]";
    *this << t1 << "-]";
    return x;
}

Variable BfSpace::op_div(Variable _x, Variable y) {
    Variable x = wrap_temp(std::move(_x));
    Variable t0 = addTemp();
    Variable t1 = addTemp();
    Variable t2 = addTemp();
    Variable t3 = addTemp();

    *this << t0 << "[-]";
    *this << t1 << "[-]";
    *this << t2 << "[-]";
    *this << t3 << "[-]";
    *this << x << "[" << t0 << "+" << x<< "-]";
    *this << t0 << "[";
    {
        auto i = indent();
        *this << y << "[" << t1 << "+" << t2 << "+" << y << "-]";
        *this << t2 << "[" << y<< "+" << t2 << "-]";
        *this << t1 << "[";
        {
            auto i = indent();
            *this <<   t2 << "+";
            *this <<   t0 << "-" << "[" << t2 << "[-]" << t3 << "+" << t0 << "-]";
            *this <<   t3 << "[" << t0 << "+" << t3 << "-]";
            *this <<   t2 << "[";
            {
                auto i = indent();
                *this <<    t1 << "-";
                *this <<    "[" << x << "-" << t1 << "[-]]+";
                *this <<   t2 << "-]";
            }
            *this <<  t1 << "-]";
        }
        *this <<  x << "+";
    }
    *this << t0 << "]";   
    return x;
}

Variable BfSpace::op_lt(Variable _x, Variable y) {
    Variable x = wrap_temp(std::move(_x));
    Variable temp0 = addTemp();
    Variable temp1 = addTemp(3);
    *this << Comment{"lt(" + x.DebugString() + ";" +  y.DebugString() + ")"};
    *this << temp0 << "[-]";
    *this << temp1 << "[-] >[-]+ >[-] <<"; 
    *this << y << "[" << temp0 << "+" << temp1 << "+" << y << "-]"; 
    *this << temp0 << "[" << y << "+" << temp0 << "-]"; 
    *this << x << "[" << temp0 << "+" << x << "-]+"; 
    *this << temp1 << "[>-]> [< " << x  << "-" << temp0 << "[-]" << temp1 << ">->]<+<"; 
    *this << temp0 << "[" << temp1 << "- [>-]> [<" << x << "-" << temp0 << "[-]+" << temp1 << ">->]<+<" << temp0 << "-]"; 
    *this << "\n";
    return x;
}

Variable BfSpace::op_le(Variable _x, Variable y) {
    Variable x = wrap_temp(std::move(_x));
    Variable temp0 = addTemp();
    Variable temp1 = addTemp(3);
    *this << Comment{"le(" + x.DebugString() + ";" +  y.DebugString() + ")"};
    *this << temp0 << "[-]"; 

    *this << temp1 << "[-] >[-]+ >[-] <<"; 
    *this << y << "[" << temp0 << "+ " << temp1 << "+ " << y << "-]"; 
    *this << temp1 << "[" << y << "+ " << temp1 << "-]"; 
    *this << x << "[" << temp1 << "+ " << x << "-]"; 
    *this << temp1 << "[>-]> [< " << x << "+ " << temp0 << "[-] " << temp1 << ">->]<+<"; 
    *this << temp0 << "[" << temp1 << "- [>-]> [< " << x << "+ " << temp0 << "[-]+ " << temp1 << ">->]<+< " << temp0 << "-]"; 
    return x;
}

Variable BfSpace::op_eq(Variable x, Variable y) {
    return op_not(op_neq(std::move(x), std::move(y)));
}

Variable BfSpace::op_neq(Variable x, Variable y) {
    return op_sub(std::move(x), std::move(y));
}

Variable BfSpace::op_neg(Variable _x) {
    Variable x = wrap_temp(std::move(_x));
    Variable t = addTemp();
    *this << t << "[-]";
    *this << x << "[" << t << "-" << x << "-]";
    *this << t << "[" << x << "-" << t << "+]";
    return x;
}

Variable BfSpace::op_not(Variable _x) {
    Variable x = wrap_temp(std::move(_x));
    Variable t = addTemp();
    *this << t << "[-]";
    *this << x << "[" << t << "+" << x << "[-]]+";
    *this << t << "[" << x << "-" << t << "-]";
    return x;
}

void BfSpace::moveTo(int pos) {
    int delta = pos - current_tape_pos_;
    current_tape_pos_ = pos;
    char mover = '>';
    if (delta < 0) {
        mover = '<';
        delta = -delta;
    }
    code_ += std::string(delta, mover);
}

void BfSpace::copy(const Variable& src, const Variable& dst) {
    Variable t = addTemp();
    *this << t << "[-]";
    *this << dst << "[-]";
    *this << src << "[" << dst << "+" << t << "+" << src << "-]";
    *this << t << "[" << src << "+" << t << "-" << "]";
}

Variable BfSpace::wrap_temp(Variable v) {
    if (v.is_temp()) return v;
    Variable result = addTemp();
    copy(v, result);
    return result;
}

void BfSpace::push_scope() {
    env_ = std::make_unique<Env>(std::move(env_));
}

void BfSpace::pop_scope() {
    env_ = env_->release_parent();
}

int FunctionStorage::lookup_function(std::string_view name, int arity) {
    auto [it, inserted] = functions_.insert(std::make_pair(std::string(name),
                                            IndexedFunction{functions_.size(), arity, nullptr}));
    return it->second.index;
}

void FunctionStorage::define_function(const Function& function) {
    auto [it, inserted] = functions_.insert(std::make_pair(function.name(),
                                            IndexedFunction{functions_.size(), function.arity(), nullptr}));
    if (!inserted) {
        if (it->second.function != nullptr) {
            throw std::invalid_argument("function already exists '" + function.name() + "'");
        } else {
            if (it->second.arity != function.arity()) {
                throw std::invalid_argument("function '" + function.name() + "' has wrong arity (" +
                                            std::to_string(it->second.arity) + " vs " + std::to_string(function.arity()) + ")");
            }
        }
    }
    max_arity_ = std::max(max_arity_, it->second.arity);
    it->second.function = &function;
}

void FunctionStorage::validate_functions() const { 
    for (const auto& [name, fun] : functions_) {
        if (fun.function == nullptr) {
            throw std::invalid_argument("undefined function '" + name + "' of arity " + std::to_string(fun.arity));
        }
    }
}
