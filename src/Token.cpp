#include "Token.h"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::T_IF: return "T_IF";
        case TokenType::T_ELSE: return "T_ELSE";
        case TokenType::T_WHILE: return "T_WHILE";
        case TokenType::T_FOR: return "T_FOR";
        case TokenType::T_PRINTLN: return "T_PRINTLN";
        case TokenType::T_ID: return "T_ID";
        case TokenType::T_NUM: return "T_NUM";
        case TokenType::T_FN: return "T_FN";
        case TokenType::T_LET: return "T_LET";
        case TokenType::T_MUT: return "T_MUT";
        case TokenType::T_MATCH: return "T_MATCH";
        case TokenType::T_UL: return "T_UL";
        case TokenType::T_BANG: return "T_BANG";
        case TokenType::T_REF: return "T_REF";
        case TokenType::T_COLON: return "T_COLON";
        case TokenType::T_ARROW: return "T_ARROW";
        case TokenType::T_FAT_ARROW: return "T_FAT_ARROW";
        case TokenType::T_TYPE: return "T_TYPE";
        case TokenType::T_STRING: return "T_STRING";
        case TokenType::T_COMMA: return "T_COMMA";
        case TokenType::T_ASSIGN: return "T_ASSIGN";
        case TokenType::T_EQ: return "T_EQ";
        case TokenType::T_PLUS: return "T_PLUS";
        case TokenType::T_MINUS: return "T_MINUS";
        case TokenType::T_MULT: return "T_MULT";
        case TokenType::T_DIV: return "T_DIV";
        case TokenType::T_LT: return "T_LT";
        case TokenType::T_GT: return "T_GT";
        case TokenType::T_LPAREN: return "T_LPAREN";
        case TokenType::T_RPAREN: return "T_RPAREN";
        case TokenType::T_LBRACE: return "T_LBRACE";
        case TokenType::T_RBRACE: return "T_RBRACE";
        case TokenType::T_SEMICOLON: return "T_SEMICOLON";
        case TokenType::T_EOF: return "T_EOF";
        case TokenType::T_LE: return "T_LE";
        case TokenType::T_GE: return "T_GE";
        case TokenType::T_NE: return "T_NE";
        case TokenType::T_AND: return "T_AND";
        case TokenType::T_OR: return "T_OR";
        default: return "UNKNOWN";
    }
}
