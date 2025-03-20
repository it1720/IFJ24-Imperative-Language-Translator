#ifndef SCANNER_H
#define SCANNER_H
#include <stdio.h>
#include <string.h>
#include "error.h"

enum TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_SQ_BRACKET, RIGHT_SQ_BRACKET, LEFT_BRACE, RIGHT_BRACE, PIPE,
    COMMA, COLON, SEMICOLON, DOT, MINUS, PLUS, SLASH, STAR, QUESTION_MARK, AT, UNDERSCORE, 

    // One or two character tokens
    BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,

    // Literals
    IDENTIFIER, STRING, NUMBER, FLOAT_NUMBER, ANY_PARAM_TYPE,

    // Keywords
    CONST, ELSE, FN, IF, I32, F64, K_NULL, PUB, RETURN, U8, VAR, VOID, WHILE, IMPORT,

    EOF_TOKEN
    };

struct Token {
    enum TokenType type;
    char* lexeme;
    int length;
    int line;
};

struct TokenDLL {
    struct Token* token;
    struct TokenDLL* next;
    struct TokenDLL* prev;
    struct TokenDLL* active;
};

struct Token* createToken(enum TokenType type, char* lexeme, int length, int line);
struct TokenDLL* scanTokens(FILE*);
void printTokens();
void ungetToken();
struct Token* getToken();
struct Token * getNextToken();
#endif // SCANNER_H
