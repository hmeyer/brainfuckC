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
        return "~t{" + std::to_string(index_) + "}";
    } else {
        return name_ + "{" + std::to_string(index_) + "}";
    }
}


int Env::next_free(int size, bool on_top) {
    int start = on_top?-1:find_consecutive(free_list_, size, next_free_);
    if (start == -1) {
        start = next_free_;
    }
    for(int i = 0; i < size; i++) {
        free_list_.erase(start + i);
    }
    next_free_ = std::max(next_free_, start + size);
    return start;
}

Variable Env::add(const std::string& name, int size, bool on_top) {
    auto [it, inserted] = vars_.insert(std::make_pair(std::string(name), 0));
    if (!inserted) {
        throw std::runtime_error("tried to insert already existing " + name);
    }
    it->second = next_free(size, on_top);
    return Variable(this, it->second, name);
}

Variable Env::add_alias(const std::string& original, const std::string& alias) {
    Variable orig = get(original);
    auto [unused_it, inserted] = vars_.insert(std::make_pair(std::string(alias), orig.index()));
    if (!inserted) {
        throw std::runtime_error("tried to insert already existing " + alias);
    }
    return Variable(this, orig.index(), alias);
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

Variable Env::addTemp(int size, bool on_top) {
    int index = next_free(size, on_top);
    if (size != 1) { 
        temp_sizes_[index] = size;
    }
    return Variable(this, index, "");
}

void Env::remove(int index) {
    if (index >= next_free_ || free_list_.count(index) > 0) {
        throw std::runtime_error("tried to remove non-existent index " + std::to_string(index) + " from env: " + std::to_string(long(this)));
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

constexpr char kCalledFunctionIndex[] = "__CalledFunctionIndex";
constexpr char kReturnPosition[] = "__ReturnPosition";
constexpr char kCallPending[] = "__CallPending";
constexpr char kParameterPrefix[] = "__Parameter";

std::string parameter_name(int num) {
    return kParameterPrefix + std::to_string(num);
}

void add_function_vars(int max_arity, Env* env) {
    for (const char* var_name : {kCalledFunctionIndex, kReturnPosition, kCallPending}) {
        env->add(std::string(var_name), 1, true);
    }
    for (int i = 0; i < max_arity; i++) {
        env->add(parameter_name(i));
    }
}
}  // namespace

BfSpace::BfSpace(): env_{std::make_unique<Env>()},
                    functions_{std::make_unique<FunctionStorage>()},
                    indent_(0) {
    add_function_vars(functions_->max_arity(), env_.get());
}

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

BfSpace::Emitter BfSpace::operator<<(const Verbatim& v) {
    Emitter e{this};
    e << v;
    return e;
}

BfSpace::Emitter& BfSpace::Emitter::operator<<(std::string_view s) {
    if (s.find_first_not_of(",.+-<>[] \n") != std::string::npos) {
        throw std::invalid_argument("Code contains non-brainfuck character: " + std::string(s));
    }
    parent_->append_code(s);
    return *this;
}

BfSpace::Emitter& BfSpace::Emitter::operator<<(const Variable& v) {
    parent_->append_code(v.DebugString());
    parent_->moveTo(v.index());
    return *this;
}

BfSpace::Emitter& BfSpace::Emitter::operator<<(const Comment& c) {
    if (c.value.find_first_of(",.+-<>[]") != std::string::npos) {
        throw std::invalid_argument(std::string("Comment contains brainfuck character: ") + c.value);
    }
    parent_->append_code(c.value);
    return *this;
}

BfSpace::Emitter& BfSpace::Emitter::operator<<(const Verbatim& v) {
    parent_->append_code(v.value);
    return *this;
}

void BfSpace::append_code(std::string_view t) {
    for (char c : t) {
        if (is_on_new_line_) {
            code_ += std::string(indent_ * 2, ' ');
            is_on_new_line_ = false;
        }
        code_ += c;
        if (c == '\n') {
            is_on_new_line_ = true;
        }
    }
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

Variable BfSpace::get_return_position() const {
    return get(kReturnPosition);
}

void BfSpace::register_parameter(int num, const std::string& name) {
    env_->add_alias(parameter_name(num), name);
}


Variable BfSpace::get_call_pending() const {
    return get(kCallPending);
}

Variable BfSpace::op_add(Variable _x, Variable _y) {
    *this << Comment{"add(" + _x.DebugString() + "; " + _y.DebugString() + ")"};
    Variable x = wrap_temp(std::move(_x));
    Variable y = wrap_temp(std::move(_y));
    *this << y << "[-" << x << "+" << y << "]";
    return x;
}

Variable BfSpace::op_sub(Variable _x, Variable _y) {
    *this << Comment{"sub(" + _x.DebugString() + "; " + _y.DebugString() + ")"};
    Variable x = wrap_temp(std::move(_x));
    Variable y = wrap_temp(std::move(_y));
    *this << y << "[-" << x << "-" << y << "]";
    return x;
}

Variable BfSpace::op_mul(Variable _x, Variable y) {
    *this << Comment{"mul(" + _x.DebugString() + "; " + y.DebugString() + ")"};
    Variable x = wrap_temp(std::move(_x));
    Variable t0 = addTemp();
    Variable t1 = addTemp();
    *this << t0 << "[-]"
          << t1 << "[-]"
          << x << "[" << t1 << "+" << x << "-]"
          << t1 << "["
          << y << "[" << x << "+" << t0 << "+" << y << "-]" << t0 << "[" << y << "+" << t0 << "-]"
          << t1 << "-]";
    return x;
}

Variable BfSpace::op_div(Variable _x, Variable y) {
    *this << Comment{"div(" + _x.DebugString() + "; " + y.DebugString() + ")"};
    Variable x = wrap_temp(std::move(_x));
    Variable t0 = addTemp();
    Variable t1 = addTemp();
    Variable t2 = addTemp();
    Variable t3 = addTemp();

    *this << t0 << "[-]"
          << t1 << "[-]"
          << t2 << "[-]"
          << t3 << "[-]"
          << x << "[" << t0 << "+" << x<< "-]"
          << t0 << "[";
    {
        auto i = indent();
        *this << y << "[" << t1 << "+" << t2 << "+" << y << "-]"
              << t2 << "[" << y<< "+" << t2 << "-]"
              << t1 << "[";
        {
            auto i = indent();
            *this <<   t2 << "+"
                  <<   t0 << "-" << "[" << t2 << "[-]" << t3 << "+" << t0 << "-]"
                  <<   t3 << "[" << t0 << "+" << t3 << "-]"
                  <<   t2 << "[";
            {
                auto i = indent();
                *this <<    t1 << "-"
                      <<    "[" << x << "-" << t1 << "[-]]+"
                      <<   t2 << "-]";
            }
            *this <<  t1 << "-]";
        }
        *this <<  x << "+";
    }
    *this << t0 << "]";   
    return x;
}

Variable BfSpace::op_lt(Variable _x, Variable y) {
    *this << Comment{"lt(" + _x.DebugString() + "; " + y.DebugString() + ")"};
    Variable x = wrap_temp(std::move(_x));
    Variable temp0 = addTemp();
    Variable temp1 = addTemp(3);
    *this << Comment{"lt(" + x.DebugString() + ";" +  y.DebugString() + ")"};
    *this << temp0 << "[-]"
          << temp1 << "[-] >[-]+ >[-] <<"
          << y << "[" << temp0 << "+" << temp1 << "+" << y << "-]"
          << temp0 << "[" << y << "+" << temp0 << "-]"
          << x << "[" << temp0 << "+" << x << "-]+"
          << temp1 << "[>-]> [< " << x  << "-" << temp0 << "[-]" << temp1 << ">->]<+<"
          << temp0 << "[" << temp1 << "- [>-]> [<" << x << "-" << temp0 << "[-]+" << temp1 << ">->]<+<" << temp0 << "-]"; 
    return x;
}

Variable BfSpace::op_le(Variable _x, Variable y) {
    *this << Comment{"le(" + _x.DebugString() + "; " + y.DebugString() + ")"};
    Variable x = wrap_temp(std::move(_x));
    Variable temp0 = addTemp();
    Variable temp1 = addTemp(3);
    *this << Comment{"le(" + x.DebugString() + ";" +  y.DebugString() + ")"};
    *this << temp0 << "[-]"
          << temp1 << "[-] >[-]+ >[-] <<"
          << y << "[" << temp0 << "+ " << temp1 << "+ " << y << "-]"
          << temp1 << "[" << y << "+ " << temp1 << "-]"
          << x << "[" << temp1 << "+ " << x << "-]"
          << temp1 << "[>-]> [< " << x << "+ " << temp0 << "[-] " << temp1 << ">->]<+<"
          << temp0 << "[" << temp1 << "- [>-]> [< " << x << "+ " << temp0 << "[-]+ " << temp1 << ">->]<+< " << temp0 << "-]"; 
    return x;
}

Variable BfSpace::op_eq(Variable x, Variable y) {
    *this << Comment{"eq(" + x.DebugString() + "; " + y.DebugString() + ")"};
    auto i = indent();
    return op_not(op_neq(std::move(x), std::move(y)));
}

Variable BfSpace::op_neq(Variable x, Variable y) {
    *this << Comment{"neq(" + x.DebugString() + "; " + y.DebugString() + ")"};
    return op_sub(std::move(x), std::move(y));
}

Variable BfSpace::op_neg(Variable _x) {
    *this << Comment{"neg(" + _x.DebugString() + ")"};
    Variable x = wrap_temp(std::move(_x));
    Variable t = addTemp();
    *this << t << "[-]"
          << x << "[" << t << "-" << x << "-]"
          << t << "[" << x << "-" << t << "+]";
    return x;
}

Variable BfSpace::op_not(Variable _x) {
    *this << Comment{"not(" + _x.DebugString() + ")"};
    Variable x = wrap_temp(std::move(_x));
    Variable t = addTemp();
    *this << t << "[-]"
          << x << "[" << t << "+" << x << "[-]]+"
          << t << "[" << x << "-" << t << "-]";
    return x;
}

Variable BfSpace::op_and(Variable x, std::function<Variable()> y) {
    Variable result = addTempWithValue(0);
    Variable t = wrap_temp(std::move(x));
    *this << t << "[";
    auto i = indent();
    copy(y(), t);
    *this << t << "["
          << result << "+"
          << t << "[-]]"
          << t << "]";
    return result;
}

Variable BfSpace::op_or(Variable x, std::function<Variable()> y) {
    Variable result = addTempWithValue(0);
    Variable t = wrap_temp(std::move(x));
    Variable flag = addTemp();
    *this << flag << "[-]+"  // Set t = 1
          << t << "[" << result << "+" << flag << "-" << t << "[-]]"
          << flag << "[" << flag << "-";
    auto i = indent();
    copy(y(), t);
    *this << t << "[" << result << "+" << t << "[-]]"
          << flag << "]";
    return result;
}


void BfSpace::op_if_then_else(Variable condition, std::function<void()> then_branch, std::function<void()> else_branch) {
    *this << Comment{"if (" + condition.DebugString() + ")"};
    auto c = wrap_temp(std::move(condition));
    if (!else_branch) {
        *this << c << "[";
        {
            auto i = indent();
            then_branch();
        }
        *this << c << "[-]]";
    } else {
        Variable t0 = addTemp();
        *this << t0 << "[-]+"
              << c << "[";
        {
            auto i = indent();
            then_branch();
        }
        *this <<  t0 << "-"
              <<  c << "[-]"
              << "]"
              << t0 << "[";
        {
            auto i = indent();
            else_branch();
        }
        *this << t0 << "-]";        
    }
}



void BfSpace::moveTo(int pos) {
    int delta = pos - current_tape_pos_;
    current_tape_pos_ = pos;
    char mover = '>';
    if (delta < 0) {
        mover = '<';
        delta = -delta;
    }
    append_code(std::string(delta, mover));
}

void BfSpace::copy(const Variable& src, const Variable& dst) {
    auto i = indent();
    *this << Comment{"copy(" + src.DebugString() + "; " + dst.DebugString() + ")"};
    Variable t = addTemp();
    *this << t << "[-]" << dst << "[-]" 
          << src << "[" << dst << "+" << t << "+" << src << "-]"
          << t << "[" << src << "+" << t << "-" << "]";
}

Variable BfSpace::wrap_temp(Variable v) {
    if (v.is_temp()) return v;
    Variable result = addTemp();
    copy(v, result);
    return result;
}

BfSpace::ScopePopper BfSpace::push_scope(int min_next_free) {
    env_ = std::make_unique<Env>(std::move(env_), min_next_free);
    return ScopePopper(this);
}

void BfSpace::pop_scope() {
    env_ = env_->release_parent();
}

int FunctionStorage::lookup_function(std::string_view name, int arity) {
    auto it = functions_.find(std::string(name));
    if (it == functions_.end()) {
        throw std::invalid_argument("undefined function: " + std::string(name));
    }
    if (it->second.arity != arity) {
        throw std::invalid_argument("tried to function: " + std::string(name) + " with " + std::to_string(arity) +
                                    " parameters, but it has arity of " + std::to_string(it->second.arity));
    }
    return it->second.index;
}

void FunctionStorage::define_function(const Function& function) {
    max_arity_ = std::max(max_arity_, function.arity());
    auto [_ignored_it, inserted] = functions_.insert(std::make_pair(function.name(),
                                            IndexedFunction{functions_.size() + 1, function.arity(), &function}));
    if (!inserted) {
        throw std::invalid_argument("function already exists '" + function.name() + "'");
    }
}

void BfSpace::reset_env_and_code() {
    env_ = std::make_unique<Env>();
    add_function_vars(functions_->max_arity(), env_.get());
    code_.clear();
    is_on_new_line_ = true;
    current_tape_pos_ = 0;
}

std::string BfSpace::code() {
    // First run generator as analyser.
    reset_env_and_code();
    generate_dispatch_wrapped_code();
    reset_env_and_code();
    // Then run generator for realz.
    auto result = generate_dispatch_wrapped_code();
    return result;
}

void BfSpace::finish_function_call() {
    *this << Comment{"\nfinish the function call by jumping down the stack and clear call pending: "}
          << std::string(max_stack_size_, '<') << get(kCallPending) << "[-]";
}

std::string BfSpace::generate_dispatch_wrapped_code() {
    op_call_function("main", {});
    *this << Comment{"\nfunction loop"};
    *this << get(kCalledFunctionIndex) << "[";
    *this << get(kCallPending) << "[-]";

    {
        auto i = indent();
        for(const auto& name_and_f : functions_->functions()) {
            const auto& f = name_and_f.second;
            auto cond = op_eq(get(kCalledFunctionIndex), addTempWithValue(f.index));
            op_if_then_else(std::move(cond), [&f, this](){ 
                move_to_top();
                auto scope_popper = push_scope();
                auto i = indent();
                *this << Comment{"\ndefine " + f.function->Description()};
                num_function_calls_ = 0;
                f.function->evaluate(this);
                op_if_then_else(op_not(get(kCallPending)), [this]() {finish_function_call();});
            });
        }
    }
    *this << get(kCalledFunctionIndex) << "]";
    return code_;
}

void BfSpace::register_functions(const std::vector<std::unique_ptr<Function>>& functions) {
    for (const auto &f : functions) {
        functions_->define_function(*f);
    }
}

void BfSpace::op_call_function(std::string_view name, std::vector<Variable> arguments) {
    *this << Comment{"calling " + std::string(name)};
    auto i = indent();
    copy(addTempWithValue(++num_function_calls_), get(kReturnPosition));
    max_stack_size_ = std::max(max_stack_size_, env_->top());
    int function_index = functions_->lookup_function(name, arguments.size());
    auto scope_popper = push_scope(max_stack_size_);
    add_function_vars(functions_->max_arity(), env_.get());
    for (int i = 0; i < arguments.size(); i++) {
        copy(arguments[i], get(parameter_name(i)));
    }
    copy(addTempWithValue(function_index), get(kCalledFunctionIndex));
    *this << get(kReturnPosition) << "[-]";
    *this << get(kCallPending) << "[-]+";
    *this << Comment{"jump up the stackframe: "} << std::string(max_stack_size_, '>');
}
