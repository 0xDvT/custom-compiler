// scanner header file
#ifndef SCANNER_H
#define SCANNER_H

#define BUFFER_SIZE 1024

extern struct token *buffer_tokens;
extern int token_count;
void scan_tokens(FILE *file);
void free_buffer_tokens();
void print_tokens();

#endif