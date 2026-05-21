// token header file

#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>

typedef enum token_type
{
    TOKEN_KEYWORD_PROGRAM,
    TOKEN_KEYWORD_PROCEDURE,
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_THEN,
    TOKEN_KEYWORD_BEGIN,
    TOKEN_KEYWORD_END,
    TOKEN_KEYWORD_TYPE_INTEGER,
    TOKEN_KEYWORD_TYPE_STRING,
    TOKEN_KEYWORD_WRITE,
    TOKEN_IDENTIFIER,
    TOKEN_LITERAL,
    TOKEN_NUMBER,
    TOKEN_ASSIGN,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_LEFT_PARENTH,
    TOKEN_RIGHT_PARENTH,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_COMMENT,
    TOKEN_EOF
} tokenType;

typedef struct token
{
    enum token_type type;
    char *value;
    int line;
    int column;
    int is_valid;
} tokenStruct;

extern const char *token_strings[];

bool is_keyword(char *value, tokenType *kwd);
int is_symbol(char c);

#endif
