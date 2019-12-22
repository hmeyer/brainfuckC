#include "bf_space.hpp"

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