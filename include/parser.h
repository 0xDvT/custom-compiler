#ifndef PARSER_H
#define PARSER_H

#define MAX_PARAMS 6

typedef enum data_type
{
    TYPE_INT,
    TYPE_STR,
    TYPE_VOID
} DataType;

typedef enum sym_type
{
    SYM_VAR,
    SYM_FUNC,
    SYM_PROC
} SymType;

typedef struct param_list
{
    char **names;
    DataType *types;
    int count;
} paramList;

typedef struct symbol_entry
{
    char *name;
    SymType sym_type;
    DataType type;
    int scope_level;

    /* Function/Procedure specific */
    int param_count;
    DataType *param_types;
    char **param_names;
    int local_var_count;
} SymbolEntry;

void run_parser();
void functionDecl();
void procedureDecl();
void functionCall(struct token function_token);
bool is_param_of_current(char *name);
SymbolEntry *get_symbol(char *name);

#endif