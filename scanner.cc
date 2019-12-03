#include "scanner.hpp"
#include <iostream>
#include <cctype>

namespace nbf {


std::vector<Token> Scanner::scanTokens() {
    std::vector<Token> result;
    while (!isAtEnd()) {
        // We are at the beginning of the next lexeme.
        start_ = current_;                              
        scanToken();                                  
    }
    return tokens_;       
}

void Scanner::scanToken() {                     
    char c = advance();
    switch (c) {                                 
        case '+': 
        case '-': 
        case '[': 
        case ']': 
        case ',': 
        case '.': 
        case '>': 
        case '<': addToken(BfToken(c)); break;
        case '/':                                                       
            if (match('/')) {                                             
                // A comment goes until the end of the line.                
                while (peek() != '\n' && !isAtEnd()) advance();             
            }                                                             
            break;
        case ' ':                                    
        case '\r':                                   
        case '\t':                                   
            // Ignore whitespace.                      
            break;
        case '\n':                                   
            line_++;                                    
            break;  
        default:
            if (isalpha(c)) {                   
                variable();                                              
            } else {                                   
                throw std::invalid_argument(std::string("unexpected character: ") + c + " at line " + std::to_string(line_));
            }         
    }                                            
}

void Scanner::variable() {                
    while (isalnum(peek())) advance();
    std::string_view name = source_.substr(start_, current_ - start_);
    int size = 1;
    if (match(':')) {
        size = number();
    }
    addToken(Variable{.name = std::string(name), .size = size});                    
}  

int Scanner::number() {                           
    int start = current_;
    while (isdigit(peek())) advance();
    std::string_view text = source_.substr(start, current_ - start);
    return std::stoi(std::string(text));
}   

char Scanner::peek() const {           
    if (isAtEnd()) return '\0';   
    return source_[current_];
}  

bool Scanner::match(char expected) {                 
    if (isAtEnd()) return false;                         
    if (source_[current_] != expected) return false;
    current_++;                                           
    return true;                                         
} 

char Scanner::advance() {
    current_++;
    return source_[current_-1];
}

void Scanner::addToken(Token token) {
    tokens_.push_back(token);        
}

bool Scanner::isAtEnd() const {
    return current_ >= source_.size();
}

}  // namespace nbf