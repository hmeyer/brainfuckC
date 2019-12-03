#ifndef TOKEN_H
#define TOKEN_H

#include <ostream>
#include <string>
#include <variant>
#include <optional>

namespace nbf {

using BfToken = char;  // . , + - < > [ ]

struct Variable {
    std::string name;
    int size;
};

using Token = std::variant<BfToken, Variable>;
std::ostream& operator<<(std::ostream& os, const Token& t);


}  // nbf

#endif  // TOKEN_H