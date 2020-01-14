#include "token.hpp"
#include <iostream>
#include <cassert>

std::string Token::DebugString() const {
    switch (type) {
        case LEFT_PAREN: return "(";
        case RIGHT_PAREN: return ")";
        case LEFT_BRACE: return "{";
        case RIGHT_BRACE: return "}";
        case LEFT_SQUARE_BRACKET: return "[";
        case RIGHT_SQUARE_BRACKET: return "]";
        case COMMA: return ",";
        case MINUS: return " MINUS ";
        case PLUS: return " PLUS ";
        case SEMICOLON: return ";";
        case SLASH: return "/";
        case STAR: return "*";
        case PERCENT: return "%";

        case BANG: return "!";
        case BANG_EQUAL: return "!=";
        case EQUAL: return "=";
        case EQUAL_EQUAL: return "==";
        case GREATER: return " gt ";
        case GREATER_EQUAL: return " ge ";
        case LESS: return " lt ";
        case LESS_EQUAL: return " le ";

        case IDENTIFIER: return "IDENTIFIER(" + std::get<std::string>(value) + ")";
        case STRING: return "STRING(" + std::get<std::string>(value) + ")";
        case NUMBER: return "NUMBER(" + std::to_string(std::get<int>(value)) + ")";

        // Keywords.                                     
        case AND: return "AND";
        case ELSE: return "ELSE";
        case FUN: return "FUN";
        case IF: return "IF";
        case OR: return "OR";
        case PUTC: return "PUTC";
        case RETURN: return "RETURN";
        case VAR: return "VAR";
        case WHILE: return "WHILE";
        case END: return "END";

        case END_OF_FILE: return "END_OF_FILE";

        default: assert(false);  // should never happen.
    }
}