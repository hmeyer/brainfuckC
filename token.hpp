#ifndef TOKEN_H
#define TOKEN_H

#include <ostream>
#include <string>
#include <variant>
#include <optional>

enum TokenType {
  // Single-character tokens.                      
  LEFT_PAREN, RIGHT_PAREN,
  LEFT_BRACE, RIGHT_BRACE,
  LEFT_SQUARE_BRACKET, RIGHT_SQUARE_BRACKET,
  COMMA, MINUS, PLUS, SEMICOLON, SLASH, STAR, PERCENT,

  // One or two character tokens.                  
  BANG, BANG_EQUAL,             
  EQUAL, EQUAL_EQUAL,                              
  GREATER, GREATER_EQUAL,                          
  LESS, LESS_EQUAL,                                

  // Literals.                                     
  IDENTIFIER, STRING, NUMBER,                  

  // Keywords.                                     
  AND, ELSE, FUN, IF, OR,  
  PUTC, RETURN, VAR, WHILE, END,

  END_OF_FILE,
};

using TokenValue = std::variant<std::nullopt_t, std::string, int>;


struct Token {
    TokenType type;
    std::string lexeme;
    TokenValue value;
    int line;
    std::string DebugString() const;
};


#endif  // TOKEN_H