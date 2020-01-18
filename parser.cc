#include "parser.hpp"
#include "scanner.hpp"

namespace {
std::unique_ptr<Function> fake_main() {
    Token main_token{IDENTIFIER, "main", "main", -1};
    return std::make_unique<Function>(std::move(main_token), std::vector<Token>(), nullptr);
}
void set_fake_main_body(std::vector<std::unique_ptr<Statement>> body, Function* main) {
    main->set_body(std::make_unique<Block>(std::move(body)));
}
}  // namespace

std::vector<std::unique_ptr<Function>> Parser::parse(AddMain add_main) {
    std::vector<std::unique_ptr<Function>> result;
    if (add_main == kAddMain) {
        result.push_back(fake_main());
    }
    std::vector<std::unique_ptr<Statement>> main;
    while(!isAtEnd()) {
        if (match(FUN)) {
            result.push_back(function());
        } else {
            assert(add_main == kAddMain);
            main.push_back(declaration());
        }
    }
    if (add_main == kAddMain) {
        set_fake_main_body(std::move(main), result[0].get());
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

std::unique_ptr<Function> Parser::function() {
    Token name = consume(IDENTIFIER, "Expect function name.");
    consume(LEFT_PAREN, "Expect '(' after function name.");       
    std::vector<Token> parameters;                       
    if (!check(RIGHT_PAREN)) {                                        
      do {                                                            
        parameters.push_back(consume(IDENTIFIER, "Expect parameter name."));
      } while (match(COMMA));                                         
    }                                                                 
    consume(RIGHT_PAREN, "Expect ')' after parameters.");
    consume(LEFT_BRACE, "Expect '{' before function body.");
    auto body = std::make_unique<Block>(block());                    
    return std::make_unique<Function>(std::move(name), std::move(parameters), std::move(body));
}

std::unique_ptr<Statement> Parser::var_declaration() {
    Token name = consume(IDENTIFIER, "Expect variable name.");
    int size = 1;
    if (match(LEFT_SQUARE_BRACKET)) {
        Token size_token = consume(NUMBER, "Expect variable size as number.");
        consume(RIGHT_SQUARE_BRACKET, "Expect closing bracket after size.");
        size = std::get<int>(size_token.value);
    }

    std::vector<std::unique_ptr<Expression>> initializer;
    if (match(EQUAL)) {
        if (match(STRING)) {
            auto s = std::get<std::string>(previous().value);
            for(char c : s) {
                initializer.push_back(std::make_unique<Literal>(c));
            }
            initializer.push_back(std::make_unique<Literal>(0));
        } else {
            do {                                       
                initializer.push_back(expression());
            } while(match(COMMA));
        }
    }

    consume(SEMICOLON, "Expect ';' after variable declaration.");
    
    if (!initializer.empty() && initializer.size() != size) {
        throw std::invalid_argument("Variable size (" + std::to_string(size) + ") does not match initializer list size (" + std::to_string(initializer.size()) + ").");
    }

    return std::make_unique<VarDeclaration>(name, size, std::move(initializer));
}


std::unique_ptr<Statement> Parser::statement() {
    if (match(IF)) {
        return if_statement();
    }
    if (match(PUTC)) {
        return putc();
    }
    if (check(IDENTIFIER) && check_next(LEFT_PAREN)) {
            return call();
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
        return std::make_unique<Assignment>(var_expression->name_token(), var_expression->release_index(), std::move(value));
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

bool Parser::check_next(TokenType type) {
    if (current_ + 1 == tokens_.size()) return false;         
    return tokens_[current_ + 1].type == type;
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
        // bool invert = false;
        // if (op.type == EQUAL_EQUAL) {
        //     invert = true;
        // }
        // // Rewrite equality as MINUS.
        // op.type = MINUS;
        expr = std::make_unique<Binary>(std::move(expr), std::move(op), std::move(right));
        // if (invert) {
        //     Token op = previous();
        //     op.type = BANG;
        //     expr = std::make_unique<Unary>(std::move(op), std::move(expr));
        // }
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

std::unique_ptr<Statement> Parser::call() {
    Token callee = consume(IDENTIFIER, "expected identifier for call statement");
    Token lparen = consume(LEFT_PAREN, "expected '(' for call statement");
    std::vector<std::unique_ptr<Expression>> arguments;
    if (!check(RIGHT_PAREN)) {                                        
      do {                                                            
        arguments.push_back(expression());                
      } while (match(COMMA));                                         
    }

    Token rparen = consume(RIGHT_PAREN, "expected ')' after arguments");
    Token semicolon = consume(SEMICOLON, "expected ';' after call statement");

    return std::make_unique<Call>(std::move(callee), std::move(arguments));         
}

std::unique_ptr<Expression> Parser::primary() {
    if (match(NUMBER)) {                           
        return std::make_unique<Literal>(std::get<int>(previous().value));
    }

    if (match(IDENTIFIER)) {
        auto name = previous();
        std::unique_ptr<Expression> index;
        if (match(LEFT_SQUARE_BRACKET)) {
            index = expression();
            consume(RIGHT_SQUARE_BRACKET, "Expect ']' after index.");
        }
        return std::make_unique<VariableExpression>(name, std::move(index));
    }

    if (match(LEFT_PAREN)) {                               
      auto expr = expression();                            
      consume(RIGHT_PAREN, "expected ')' after expression");
      return expr;                      
    }      

    Token token = peek();
    throw std::invalid_argument("expected expression, but got " + token.DebugString() + " in line " + std::to_string(token.line));
}
