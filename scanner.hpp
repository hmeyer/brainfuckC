#ifndef SCANNER_H
#define SCANNER_H


#include "token.hpp"
#include <vector>


class Scanner {
public:
    Scanner(std::string source):source_(std::move(source)) {}
    std::vector<Token> scanTokens();
protected:
    bool isAtEnd() const;
    void scanToken();
    char advance();
    bool match(char expected);
    char peek() const;
    void number_literal();
    void char_literal();
    void string_literal();
    void identifier();
    void addToken(TokenType type, TokenValue value = std::nullopt);
private:
    std::string source_;
    std::vector<Token> tokens_;
    int current_ = 0;
    int start_ = 0;
    int line_ = 1;
};


#endif  // SCANNER_H