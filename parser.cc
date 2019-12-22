#include "parser.hpp"

std::vector<std::unique_ptr<Statement>> Parser::parse() {
    std::vector<std::unique_ptr<Statement>> result;
    while(!isAtEnd()) {
        result.push_back(declaration());
    }
    return result;
}

std::vector<std::unique_ptr<Statement>> Parser::block() {
    std::vector<std::unique_ptr<Statement>> statements;

    while (!check(RIGHT_BRACE) && !isAtEnd()) {     
      statements.push_back(declaration());                
    }                                               

    consume(RIGHT_BRACE, "Expect '}' after block.");
    return statements;    
}

std::unique_ptr<Statement> Parser::declaration() {
    if (match(VAR)) return var_declaration();
    return statement();                     
}

std::unique_ptr<Statement> Parser::var_declaration() {
    Token name = consume(IDENTIFIER, "Expect variable name.");

    std::unique_ptr<Expression> initializer;
    if (match(EQUAL)) {                                          
      initializer = expression();                                
    }                                                            

    consume(SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_unique<VarDeclaration>(name, std::move(initializer));
}


std::unique_ptr<Statement> Parser::statement() {
    if (match(IF)) {
        return if_statement();
    }
    if (match(PUTC)) {
        return putc();
    }
    if (match(PRINT)) {
        return print();
    }
    if (match(WHILE)) {
        return while_statement();
    }
    if (match(LEFT_BRACE)) {
        return std::make_unique<Block>(block());
    }
    return expression_statement();
}

std::unique_ptr<Statement> Parser::while_statement() {                                         
    consume(LEFT_PAREN, "Expect '(' after 'while'.");                     
    std::unique_ptr<Expression> condition = expression();                                     
    consume(RIGHT_PAREN, "Expect ')' after if condition.");

    std::unique_ptr<Statement> body = statement();                                     

    return std::make_unique<While>(std::move(condition), std::move(body));
}  

std::unique_ptr<Statement> Parser::if_statement() {                                         
    consume(LEFT_PAREN, "Expect '(' after 'if'.");                     
    std::unique_ptr<Expression> condition = expression();                                     
    consume(RIGHT_PAREN, "Expect ')' after if condition.");

    std::unique_ptr<Statement> then_branch = statement();                                     
    std::unique_ptr<Statement> else_branch = nullptr;                                            
    if (match(ELSE)) {                                                 
      else_branch = statement();                                        
    }                                                                  

    return std::make_unique<If>(std::move(condition), std::move(then_branch), std::move(else_branch));
}  

std::unique_ptr<Statement> Parser::expression_statement() {
    auto expr = expression();                          
    consume(SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<ExpressionStatement>(std::move(expr));
}

std::unique_ptr<Expression> Parser::assignment() {
    auto expr = logic_or();

    if (match(EQUAL)) {
        Token equals = previous();
        auto value = assignment();
        auto* var_expression = dynamic_cast<VariableExpression*>(expr.get());
        if (var_expression == nullptr) {
            throw std::invalid_argument("invalid assignment target [" + expr->DebugString() + "]");
        }
        return std::make_unique<Assignment>(var_expression->name_token(), std::move(value));
    }

    return expr;
}

std::unique_ptr<Expression> Parser::logic_or() {
    auto expr = logic_and();

    while (match(OR)) {                              
        Token op = previous();                   
        auto right = logic_and();                            
        expr = std::make_unique<Logical>(std::move(expr), op, std::move(right));
    }                                                

    return expr;  
}

std::unique_ptr<Expression> Parser::logic_and() {
    auto expr = equality();

    while (match(AND)) {                              
        Token op = previous();                   
        auto right = equality();                            
        expr = std::make_unique<Logical>(std::move(expr), op, std::move(right));
    }                                                

    return expr;  
}

std::unique_ptr<Statement> Parser::putc() {
    auto value = expression();
    consume(SEMICOLON, "Expect ';' after value.");
    return std::make_unique<Putc>(std::move(value));
}

std::unique_ptr<Statement> Parser::print() {
    auto value = expression();
    consume(SEMICOLON, "Expect ';' after value.");
    return std::make_unique<Print>(std::move(value));
}

bool Parser::match(std::vector<TokenType> types) {
    for (TokenType type : types) {           
        if (check(type)) {                     
            advance();
            return true;                         
        }                                      
    }
    return false;        
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;         
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();     
}

bool Parser::isAtEnd() {
    return peek().type == END_OF_FILE;
}

Token Parser::peek() {
    return tokens_[current_];
}

Token Parser::previous() {
    return tokens_[current_ - 1];
}

Token Parser::consume(TokenType type, const std::string& errorMessage) {
    if (check(type)) return advance();

    throw std::invalid_argument(errorMessage);                            
}


std::unique_ptr<Expression> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expression> Parser::equality() {
    auto expr = comparison();

    while (match({BANG_EQUAL, EQUAL_EQUAL})) {        
        Token op = previous();                  
        auto right = comparison();                    
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
    }                                               

    return expr;    
}
std::unique_ptr<Expression> Parser::comparison() {
    auto expr = addition();

    while (match({GREATER, GREATER_EQUAL, LESS, LESS_EQUAL})) {
        Token op = previous();                           
        auto right = addition();                               
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
    }                                                        

    return expr;   
}

std::unique_ptr<Expression> Parser::addition() {
    auto expr = multiplication();

    while (match({MINUS, PLUS})) {
        Token op = previous();                           
        auto right = multiplication();                               
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
    }                                                        

    return expr;   
}

std::unique_ptr<Expression> Parser::multiplication() {
    auto expr = unary();

    while (match({SLASH, STAR, PERCENT})) {
        Token op = previous();                           
        auto right = unary();                               
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
    }                                                        

    return expr;   
}

std::unique_ptr<Expression> Parser::unary() {
    if (match({BANG, MINUS})) {                
        Token op = previous();           
        auto right = unary();                  
        return std::make_unique<Unary>(std::move(op), std::move(right));
    }
    return primary();     
}

std::unique_ptr<Expression> Parser::primary() {
    if (match({NUMBER, STRING})) {                           
      return std::make_unique<Literal>(previous().value);         
    }


    if (match(IDENTIFIER)) {                      
      return std::make_unique<VariableExpression>(previous());       
    }

    if (match(LEFT_PAREN)) {                               
      auto expr = expression();                            
      consume(RIGHT_PAREN, "expected ')' after expression");
      return expr;                      
    }      

    Token token = peek();
    throw std::invalid_argument("expected expression, but got " + token.DebugString() + " in line " + std::to_string(token.line));
}
