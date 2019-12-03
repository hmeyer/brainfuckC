#ifndef SCANNER_H
#define SCANNER_H


#include "token.hpp"
#include <vector>

namespace nbf {


class Scanner {
public:
    Scanner(std::string_view source):source_(std::move(source)) {}
    std::vector<Token> scanTokens();
protected:
    bool isAtEnd() const;
    void scanToken();
    char advance();
    bool match(char expected);
    char peek() const;
    void variable();
    int number();
    void addToken(Token token);
private:
    std::string_view source_;
    std::vector<Token> tokens_;
    int current_ = 0;
    int start_ = 0;
    int line_ = 1;
};

}  // namespace nbf

#endif  // SCANNER_H