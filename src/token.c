#include <stdio.h>
#include <string.h>
#include "token.h"

const char *operators[] = {
    "+",
    "-",
    "*",
    "/",
    "=",
    ".",
    ":",
    ";",
    "(",
    ")",
    "{",
    "}",
    ","};

const char *token_strings[] = {
    "KWD_PROGRAM",
    "KWD_PROCEDURE",
    "KWD_FUNCTION",
    "KWD_VAR",
    "KWD_IF",
    "KWD_THEN",
    "KWD_BEGIN",
    "KWD_END",
    "KWD_TYPE_INTEGER",
    "KWD_TYPE_STRING",
    "KWD_WRITE",
    "IDENT",
    "LITERAL",
    "NUMBER",
    "ASSIGN",
    "SEMICOLON",
    "COLON",
    "DOT",
    "COMMA",
    "LEFT_PARENTH",
    "RIGHT_PARENTH",
    "PLUS",
    "MINUS",
    "MULTIPLY",
    "DIVIDE",
    "COMMENT",
    "EOF"};

bool is_keyword(char *value, tokenType *kwd)
{
    if (strcmp(value, "program") == 0)
    {
        *kwd = TOKEN_KEYWORD_PROGRAM;
    }
    else if (strcmp(value, "procedure") == 0)
    {
        *kwd = TOKEN_KEYWORD_PROCEDURE;
    }
    else if (strcmp(value, "function") == 0)
    {
        *kwd = TOKEN_KEYWORD_FUNCTION;
    }
    else if (strcmp(value, "var") == 0)
    {
        *kwd = TOKEN_KEYWORD_VAR;
    }
    else if (strcmp(value, "if") == 0)
    {
        *kwd = TOKEN_KEYWORD_IF;
    }
    else if (strcmp(value, "then") == 0)
    {
        *kwd = TOKEN_KEYWORD_THEN;
    }
    else if (strcmp(value, "begin") == 0)
    {
        *kwd = TOKEN_KEYWORD_BEGIN;
    }
    else if (strcmp(value, "end") == 0)
    {
        *kwd = TOKEN_KEYWORD_END;
    }
    else if (strcmp(value, ".") == 0)
    {
        *kwd = TOKEN_DOT;
    }
    else if (strcmp(value, "integer") == 0)
    {
        *kwd = TOKEN_KEYWORD_TYPE_INTEGER;
    }
    else if (strcmp(value, "string") == 0)
    {
        *kwd = TOKEN_KEYWORD_TYPE_STRING;
    }
    else if (strcmp(value, "write") == 0)
    {
        *kwd = TOKEN_KEYWORD_WRITE;
    }
    else
    {
        return false;
    }
    return true;
}

int is_symbol(char c)
{
    for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++)
    {
        if (strchr(operators[i], c) != NULL)
        {
            return 1;
        }
    }
    return 0;
}
