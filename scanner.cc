#include "scanner.hpp"
#include <cctype>
#include <unordered_map>


std::vector<Token> Scanner::scanTokens() {
    std::vector<Token> result;
    while (!isAtEnd()) {
        // We are at the beginning of the next lexeme.
        start_ = current_;                              
        scanToken();                                  
    }
    addToken(END_OF_FILE);
    return tokens_;       
}

void Scanner::scanToken() {                     
    char c = advance();
    switch (c) {       
        case '(': addToken(LEFT_PAREN); break;     
        case ')': addToken(RIGHT_PAREN); break;    
        case '{': addToken(LEFT_BRACE); break;     
        case '}': addToken(RIGHT_BRACE); break;    
        case '[': addToken(LEFT_SQUARE_BRACKET); break;     
        case ']': addToken(RIGHT_SQUARE_BRACKET); break;    
        case ',': addToken(COMMA); break;          
        case '-': addToken(MINUS); break;          
        case '+': addToken(PLUS); break;           
        case ';': addToken(SEMICOLON); break;      
        case '*': addToken(STAR); break; 

        case '!': addToken(match('=') ? BANG_EQUAL : BANG); break;      
        case '=': addToken(match('=') ? EQUAL_EQUAL : EQUAL); break;    
        case '<': addToken(match('=') ? LESS_EQUAL : LESS); break;      
        case '>': addToken(match('=') ? GREATER_EQUAL : GREATER); break;        

        case '/':                                                       
            if (match('/')) {                                             
                // A comment goes until the end of the line.                
                while (peek() != '\n' && !isAtEnd()) advance();             
            } else {                                                      
                addToken(SLASH);                                            
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

        case '\'': char_literal(); break;
        case '"': string_literal(); break;

        default:
            if (isdigit(c)) {                   
                number_literal();
            } else if (isalpha(c) || c == '_') {                   
                identifier();
            } else {                                   
                throw std::invalid_argument(std::string("unexpected character: ") + c + " at line " + std::to_string(line_));
            }         
    }                                            
}

void Scanner::identifier() {                
    while (isalnum(peek()) || peek() == '_') advance();

    static const std::unordered_map<std::string, TokenType>* const kKeywords = new std::unordered_map<std::string, TokenType>{
        {"and",    AND},                       
        {"else",   ELSE},                      
        {"fun",    FUN},                       
        {"if",     IF},                        
        {"or",     OR},                        
        {"putc",   PUTC},                     
        {"return", RETURN},                    
        {"var",    VAR},                       
        {"while",  WHILE},
    };

    std::string text = source_.substr(start_, current_ - start_);
    auto it = kKeywords->find(text);
    TokenType t;
    TokenValue v {std::nullopt};
    if (it != kKeywords->end()) {
        t = it->second;
    } else {
        t = IDENTIFIER;
        v = text;
    }
    addToken(t, v);                    
} 

void Scanner::number_literal() {                           
    while (isdigit(peek())) advance();
    auto text = std::string_view(source_).substr(start_, current_ - start_);
    int value = std::stoi(std::string(text));
    addToken(NUMBER, value);
}   

void Scanner::char_literal() {
    char value = source_[start_ + 1];
    if (peek() == '\\') {
        advance();
        switch (peek()) {
            case '\'': value = '\''; break;
            case 'n': value = '\n'; break;
            default: std::invalid_argument("invalid escape in char literal at line " + std::to_string(line_));              
        }
    }
    advance();
    if (peek() != '\'') {
        throw std::invalid_argument("Unterminated char literal in line " + std::to_string(line_));              
    }
    // The closing '.                                       
    advance();

    addToken(NUMBER, value);
}   

void Scanner::string_literal() {                           
    while (peek() != '"' && !isAtEnd()) {                   
        if (peek() == '\n') line_++;                           
        advance();                                            
    }

    // Unterminated string.                                 
    if (isAtEnd()) {
        throw std::invalid_argument("Unterminated string in line " + std::to_string(line_));              
    }                                                       

    // The closing ".                                       
    advance();                                              

    // Trim the surrounding quotes.                         
    std::string value = source_.substr(start_ + 1, current_ - start_ - 2);
    addToken(STRING, value);
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

void Scanner::addToken(TokenType type, TokenValue value) {
    std::string text = source_.substr(start_, current_ - start_);      
    tokens_.push_back(Token{type, text, value, line_});    
}  

bool Scanner::isAtEnd() const {
    return current_ >= source_.size();
}