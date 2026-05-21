
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "scanner.h"
#include "token.h"

struct token *buffer_tokens;
int token_count = 0;
int buffer_tokens_size = BUFFER_SIZE;
char *token_value;
int token_value_size = 0;
int column = 1;
int line = 1;

void new_buffer()
{
    token_value = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if (token_value == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }
}

void init_buffer_tokens()
{
    buffer_tokens = (struct token *)malloc(sizeof(struct token) * BUFFER_SIZE);
    if (buffer_tokens == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }
}

void resize_tokens()
{
    struct token *new_tokens = (struct token *)realloc(buffer_tokens, sizeof(struct token) * buffer_tokens_size * 2);
    if (new_tokens == NULL)
    {
        printf("Memory allocation failed\n");
        free(buffer_tokens);
        exit(1);
    }
    buffer_tokens_size *= 2;
    buffer_tokens = new_tokens;
}

void shrink_token_value()
{
    char *new_buffer = (char *)realloc(token_value, sizeof(char) * token_value_size + 1);
    if (new_buffer == NULL)
    {
        printf("Memory allocation failed\n");
        free(token_value);
        exit(1);
    }
    token_value = new_buffer;
    token_value[token_value_size] = '\0';
}

void add_token(struct token t)
{
    if (token_count >= buffer_tokens_size)
    {
        resize_tokens();
    }
    char *value_copy = strdup(t.value);
    if (value_copy == NULL)
    {
        perror("Memory allocation failed");
        exit(1);
    }
    t.value = value_copy;
    buffer_tokens[token_count++] = t;
}

void free_token_value()
{
    free(token_value);
}
void free_buffer_tokens()
{
    for (int i = 0; i < token_count; i++)
    {
        free(buffer_tokens[i].value);
    }
    free(buffer_tokens);
}

void read_keyword_or_identifier(FILE *file, char c)
{
    struct token t;
    tokenType kwd;
    token_value[token_value_size++] = c;
    while ((c = getc(file)) != EOF && (isalpha(c) || isdigit(c) || c == '_'))
    {
        token_value[token_value_size++] = c;
    }
    ungetc(c, file);
    shrink_token_value();
    t.value = token_value;
    if (is_keyword(token_value, &kwd) != false)
    {
        t.type = kwd;
    }
    else
    {
        t.type = TOKEN_IDENTIFIER;
    }
    t.line = line;
    t.column = column;
    t.is_valid = 1;
    add_token(t);
}

void read_number(FILE *file, char c)
{
    struct token t;

    token_value[token_value_size++] = c;
    t.is_valid = 1;
    while ((c = getc(file)) != EOF && (isdigit(c) || isalpha(c)))
    {
        if (isalpha(c))
        {
            t.is_valid = 0;
        }
        token_value[token_value_size++] = c;
    }

    ungetc(c, file);
    shrink_token_value();
    t.type = TOKEN_NUMBER;
    t.value = token_value;
    t.line = line;
    t.column = column;
    if (t.is_valid == 0)
    {
        printf("Invalid literal at line %d, column %d\n", line, column);
    }
    add_token(t);
}

void read_operator_or_comment(FILE *file, char c)
{
    struct token t;

    token_value[token_value_size++] = c;
    if (c == '/')
    {
        char next_char = getc(file);
        if (next_char == '/')
        {
            token_value[token_value_size++] = next_char;
            while ((c = getc(file)) != EOF && c != '\n')
            {
                token_value[token_value_size++] = c;
            }
            shrink_token_value();
            t.type = TOKEN_COMMENT;
            t.value = token_value;
            t.line = line;
            t.column = column;
            t.is_valid = 1;
            line++;
            column = 1;
            token_value_size = 0;
            // add_token(t);
        }
        else
        {
            ungetc(next_char, file);
            shrink_token_value();
            t.type = TOKEN_DIVIDE;
            t.value = token_value;
            t.line = line;
            t.column = column;
            t.is_valid = 1;
            add_token(t);
        }
    }
    else if (c == '{')
    {
        t.line = line;
        t.column = column;
        while ((c = getc(file)) != EOF && c != '}')
        {
            token_value[token_value_size++] = c;
            if (c == '\n')
            {
                line++;
                column = 1;
            }
            else
            {
                column++;
            }
        }
        if (c == '}')
        {
            token_value[token_value_size++] = c;
            column++;
        }
        else
        {
            printf("Unterminated comment at line %d, column %d\n", line, column);
        }
        shrink_token_value();
        t.type = TOKEN_COMMENT;
        t.value = token_value;
        t.is_valid = 1;
        token_value_size = 0;
    }
    else if (c == ':')
    {
        char next_char = getc(file);
        if (next_char == '=')
        {
            token_value[token_value_size++] = next_char;
            shrink_token_value();
            t.type = TOKEN_ASSIGN;
            t.value = token_value;
            t.line = line;
            t.column = column;
            t.is_valid = 1;
            add_token(t);
        }
        else
        {
            ungetc(next_char, file);
            shrink_token_value();
            t.type = TOKEN_COLON;
            t.value = token_value;
            t.line = line;
            t.column = column;
            t.is_valid = 1;
            add_token(t);
        }
    }
    else
    {

        if (c == '+')
        {
            t.type = TOKEN_PLUS;
        }
        else if (c == '-')
        {
            t.type = TOKEN_MINUS;
        }
        else if (c == '*')
        {
            t.type = TOKEN_MULTIPLY;
        }
        else if (c == ';')
        {
            t.type = TOKEN_SEMICOLON;
        }
        else if (c == ',')
        {
            t.type = TOKEN_COMMA;
        }
        else if (c == '.')
        {
            t.type = TOKEN_DOT;
        }
        else if (c == '(')
        {
            t.type = TOKEN_LEFT_PARENTH;
        }
        else if (c == ')')
        {
            t.type = TOKEN_RIGHT_PARENTH;
        }
        shrink_token_value();
        t.value = token_value;
        t.line = line;
        t.column = column;
        t.is_valid = 1;
        add_token(t);
    }
}

void read_string_literal(FILE *file)
{
    struct token t;
    char c;

    while ((c = getc(file)) != EOF && c != '"' && c != '\n')
    {
        token_value[token_value_size++] = c;
    }
    ungetc(c, file);
    shrink_token_value();
    t.type = TOKEN_LITERAL;
    t.value = token_value;
    t.line = line;
    t.column = column;
    if (c == '"')
    {
        t.is_valid = 1;
    }
    else
    {
        t.is_valid = 0;
        printf("Unterminated string literal at line %d, column %d\n", line, column);
    }
    add_token(t);
}

void scan_tokens(FILE *file)
{
    init_buffer_tokens();
    char c;
    while ((c = getc(file)) != EOF)
    {
        if (c == '\n')
        {
            line++;
            column = 1;
            token_value_size = 0;
        }
        else if (c == ' ' || c == '\t' || c == '\r')
        {
            column++;
        }
        else
        {
            new_buffer();
            token_value_size = 0;

            if (isalpha(c))
            {
                read_keyword_or_identifier(file, c);
            }
            else if (isdigit(c))
            {
                read_number(file, c);
            }
            else if (is_symbol(c))
            {
                read_operator_or_comment(file, c);
            }
            else if (c == '_')
            {
                read_keyword_or_identifier(file, c);
            }
            else if (c == '"')
            {
                read_string_literal(file);
                column += 2;
            }
            else
            {
                printf("Invalid character: %c at line %d, column %d\n", c, line, column);
                continue;
            }
            column += token_value_size;
            token_value_size = 0;
        }
    }
}

void print_tokens()
{
    for (int i = 0; i < token_count; i++)
    {
        printf("%s  %s  (%d,%d)\n", token_strings[buffer_tokens[i].type], buffer_tokens[i].value, buffer_tokens[i].line, buffer_tokens[i].column);
    }
}