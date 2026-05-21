#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "token.h"

void generate_code();
void init_assembler_file(char *name);
void init_vars(SymbolEntry *symbol_table, int symbol_table_index);
void init_text();
void start_program();
void code_simple_assignment(tokenStruct destination, tokenStruct source);
void code_complex_assignment(tokenStruct destination, tokenStruct operand1, tokenType operator, tokenStruct operand2);
void code_expr_push_number(const char *value);
void code_expr_push_identifier(const char *name);
void code_expr_push_rax(void);
void code_expr_apply_operator(tokenType op);
void code_expr_pop_to_rax(void);
void code_store_assignment(tokenStruct destination);
void code_set_call_arg(int index);
void code_emit_call(const char *name);
void code_write_rax(void);
void code_procedure_call(tokenStruct proc, tokenStruct *args, int arg_count);
void code_function_call(tokenStruct func, tokenStruct *args, int arg_count);
void code_proc_end();
void code_func_end();
void code_proc_begin(char *name, char **param_names, int param_count);
void code_func_begin(char *name, char **param_names, int param_count);
void code_local_vars(SymbolEntry *symbol_table, int symbol_table_index, int scope_level);
void code_write_call(tokenStruct arg);
void end_program();
void close_assembler_file();

#endif
