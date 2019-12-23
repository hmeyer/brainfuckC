#ifndef PARSER_HPP
#define PARSER_HPP

#include "expression.hpp"
#include "statement.hpp"
#include "token.hpp"
#include <vector>
#include <memory>

class Parser {
public:
    Parser(std::vector<Token> tokens): tokens_(std::move(tokens)) {}
    std::vector<std::unique_ptr<Statement>> parse();

private:
    bool match(std::vector<TokenType> types);
    bool match(TokenType type) { return match(std::vector<TokenType>{type}); }
    bool check(TokenType type);
    Token advance();
    bool isAtEnd();
    Token peek();
    Token previous();
    Token consume(TokenType type, const std::string& errorMessage);

    std::vector<std::unique_ptr<Statement>> block();

    std::unique_ptr<Statement> declaration();
    std::unique_ptr<Statement> var_declaration();
    std::unique_ptr<Statement> statement();
    std::unique_ptr<Statement> if_statement();
    std::unique_ptr<Statement> while_statement();
    std::unique_ptr<Statement> expression_statement();
    std::unique_ptr<Statement> putc();

    std::unique_ptr<Expression> assignment();
    std::unique_ptr<Expression> logic_and();
    std::unique_ptr<Expression> logic_or();
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> addition();
    std::unique_ptr<Expression> multiplication();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> call(Token function);
    std::unique_ptr<Expression> primary();


    std::vector<Token> tokens_;
    int current_ = 0;
};

#endif  // PARSER_HPP