#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "scanner.h"
#include "token.h"
#include "parser.h"
#include "codegen.h"

// ============================================================================
// GRAMMAR RULES
// ============================================================================
// prg              = prgHeader varDefs "begin" statementSeq "end" "."
// prgHeader        = "program" identifier ";"
// varDefs          = [ "var" varSeq { varSeq } ]
// varSeq           = identifier { "," identifier } : type ";"
// type             = "integer" | "string"
// statementSeq     = { statement ";" }
// assignment       = identifier ":=" expression
// expression       = parsed via Shunting Yard (codegen during yard pass)

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
int position = 0;
int current_scope_level = 0;
char *current_program = NULL;
SymbolEntry *symbol_table;
int symbol_table_index = 0;
int symbol_table_size = BUFFER_SIZE;

// ============================================================================
// SCOPE MANAGEMENT
// ============================================================================

void enter_scope()
{
    current_scope_level++;
}

void free_identifier_entry(int index)
{
    if (symbol_table[index].name != NULL)
    {
        free(symbol_table[index].name);
    }

    if (symbol_table[index].param_names != NULL)
    {
        for (int i = 0; i < symbol_table[index].param_count; i++)
        {
            if (symbol_table[index].param_names[i] != NULL)
            {
                free(symbol_table[index].param_names[i]);
            }
        }
        free(symbol_table[index].param_names);
    }

    if (symbol_table[index].param_types != NULL)
    {
        free(symbol_table[index].param_types);
    }
}

void exit_scope()
{
    int write = 0;

    for (int i = 0; i < symbol_table_index; i++)
    {
        if (symbol_table[i].scope_level == current_scope_level)
        {
            free_identifier_entry(i);
        }
        else
        {
            if (write != i)
                symbol_table[write] = symbol_table[i];
            write++;
        }
    }
    symbol_table_index = write;
    current_scope_level--;
}

// ============================================================================
// SYMBOL TABLE MANAGEMENT
// ============================================================================

void init_symbol_table()
{
    symbol_table = (SymbolEntry *)malloc(sizeof(SymbolEntry) * symbol_table_size);
    if (symbol_table == NULL)
    {
        perror("Memory allocation failed\n");
        exit(1);
    }
}

void resize_symbol_table()
{
    SymbolEntry *new_table = (SymbolEntry *)realloc(symbol_table, sizeof(SymbolEntry) * symbol_table_size * 2);

    if (new_table == NULL)
    {
        perror("Memory allocation failed\n");
        free(symbol_table);
        exit(1);
    }
    symbol_table = new_table;
    symbol_table_size *= 2;
}

void add_function_to_symbol_table(char *name, DataType return_type, char **param_names, DataType *param_types, int param_count)
{
    SymbolEntry new_entry;
    if (symbol_table_index >= symbol_table_size)
    {
        resize_symbol_table();
    }

    new_entry = (SymbolEntry){
        .name = strdup(name),
        .type = return_type,
        .sym_type = SYM_FUNC,
        .scope_level = current_scope_level,
        .param_names = param_names,
        .param_types = param_types,
        .param_count = param_count,
        .local_var_count = 0};
    symbol_table[symbol_table_index] = new_entry;
    symbol_table_index++;
}

void add_procedure_to_symbol_table(char *name, char **param_names, DataType *param_types, int param_count)
{
    SymbolEntry new_entry;

    if (symbol_table_index >= symbol_table_size)
    {
        resize_symbol_table();
    }

    new_entry = (SymbolEntry){
        .name = strdup(name),
        .type = TYPE_VOID,
        .sym_type = SYM_PROC,
        .scope_level = current_scope_level,
        .param_names = param_names,
        .param_types = param_types,
        .param_count = param_count,
        .local_var_count = 0};
    symbol_table[symbol_table_index] = new_entry;
    symbol_table_index++;
}

void add_to_symbol_table(char *name, DataType type)
{
    SymbolEntry new_entry;

    if (symbol_table_index >= symbol_table_size)
    {
        resize_symbol_table();
    }
    new_entry = (SymbolEntry){
        .name = strdup(name),
        .type = type,
        .sym_type = SYM_VAR,
        .scope_level = current_scope_level,
        .param_names = NULL,
        .param_types = NULL,
        .param_count = 0,
        .local_var_count = 0};
    symbol_table[symbol_table_index] = new_entry;
    symbol_table_index++;
}

SymbolEntry *get_symbol(char *name)
{
    for (int scope = current_scope_level; scope >= 0; scope--)
    {
        for (int i = 0; i < symbol_table_index; i++)
        {
            if (symbol_table[i].scope_level == scope && strcmp(symbol_table[i].name, name) == 0)
            {
                return &symbol_table[i];
            }
            if (current_program != NULL && strcmp(current_program, symbol_table[i].name) == 0 && (symbol_table[i].sym_type == SYM_FUNC || symbol_table[i].sym_type == SYM_PROC))
            {
                for (int j = 0; j < symbol_table[i].param_count; j++)
                {
                    if (strcmp(symbol_table[i].param_names[j], name) == 0)
                    {
                        return &symbol_table[i];
                    }
                }
            }
        }
    }
    return NULL;
}

void free_symbol_table()
{
    for (int i = 0; i < symbol_table_index; i++)
    {
        free_identifier_entry(i);
        symbol_table[i].name = NULL;
        symbol_table[i].param_names = NULL;
        symbol_table[i].param_types = NULL;
    }
    free(symbol_table);
    symbol_table = NULL;
    symbol_table_index = 0;
}

// ============================================================================
// TOKEN MANAGEMENT
// ============================================================================

struct token current()
{
    return buffer_tokens[position];
}

void consume()
{
    if (position < token_count)
    {
        position++;
        return;
    }
    fprintf(stderr, "Unexpected end of input at line %d, column %d\n", current().line, current().column);
    exit(1);
}

void error(struct token t, tokenType expected)
{
    fprintf(stderr, "Error: Expected %s, got %s at line %d, column %d\n",
            token_strings[expected], token_strings[t.type], t.line, t.column);
    exit(1);
}

void validate_token(tokenType expected)
{
    if (current().type != expected)
    {
        error(current(), expected);
    }
}

void match_token(tokenType expected)
{
    validate_token(expected);
    consume();
}

// ============================================================================
// VALIDATION HELPERS
// ============================================================================

bool is_param_of_current(char *name)
{
    if (current_program == NULL)
        return false;
    SymbolEntry *fn = get_symbol(current_program);
    if (fn == NULL)
        return false;
    for (int i = 0; i < fn->param_count; i++)
        if (strcmp(fn->param_names[i], name) == 0)
            return true;
    return false;
}

void validate_new_identifier(tokenStruct identifier)
{
    // Only check symbols in the current scope level
    for (int i = 0; i < symbol_table_index; i++)
    {
        if (symbol_table[i].scope_level == current_scope_level &&
            strcmp(symbol_table[i].name, identifier.value) == 0)
        {
            fprintf(stderr, "Error: Identifier '%s' at line %d, column %d is already declared \n",
                    identifier.value, identifier.line, identifier.column);
            exit(1);
        }
    }
}

void validate_identifier_declared(tokenStruct identifier)
{
    if (!get_symbol(identifier.value))
    {
        fprintf(stderr, "Error: Undeclared identifier '%s' at line %d, column %d\n",
                identifier.value, identifier.line, identifier.column);
        exit(1);
    }
}

// ============================================================================
// PARSING PRIMITIVES
// ============================================================================

DataType type()
{
    if (strcmp(current().value, "integer") == 0)
    {
        return TYPE_INT;
    }
    if (strcmp(current().value, "string") == 0)
    {
        return TYPE_STR;
    }

    fprintf(stderr, "Error: Expected type 'integer' or 'string', got '%s' at line %d, column %d\n",
            current().value, current().line, current().column);
    exit(1);
    return TYPE_VOID;
}

// ============================================================================
// EXPRESSION PARSING (Shunting Yard — codegen on operator pop)
// ============================================================================

#define MAX_EXPR_OPS 64

static int is_arith_operator(tokenType t)
{
    return t == TOKEN_PLUS || t == TOKEN_MINUS ||
           t == TOKEN_MULTIPLY || t == TOKEN_DIVIDE;
}

static int op_precedence(tokenType t)
{
    switch (t)
    {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return 1;
    case TOKEN_MULTIPLY:
    case TOKEN_DIVIDE:
        return 2;
    default:
        return -1;
    }
}

static int is_expression_end(tokenType t, tokenType stop1, tokenType stop2)
{
    if (t == stop1)
        return 1;
    if (stop2 != TOKEN_EOF && t == stop2)
        return 1;
    return 0;
}

static void validate_integer_operand(tokenStruct tok, SymbolEntry *sym)
{
    if (tok.type == TOKEN_NUMBER)
        return;

    if (sym == NULL)
        return;

    if (sym->type != TYPE_INT)
    {
        fprintf(stderr, "Error: Integer expression expected, got non-integer '%s' at line %d, column %d\n",
                tok.value, tok.line, tok.column);
        exit(1);
    }

    if (sym->sym_type == SYM_FUNC && sym->type != TYPE_INT)
    {
        fprintf(stderr, "Error: Function '%s' does not return integer at line %d, column %d\n",
                tok.value, tok.line, tok.column);
        exit(1);
    }
}

static void apply_pending_ops(tokenType *op_stack, int *op_top, int min_prec)
{
    while (*op_top > 0 &&
           op_stack[*op_top - 1] != TOKEN_LEFT_PARENTH &&
           is_arith_operator(op_stack[*op_top - 1]) &&
           op_precedence(op_stack[*op_top - 1]) >= min_prec)
    {
        code_expr_apply_operator(op_stack[--(*op_top)]);
    }
}

static void push_expression_value(tokenStruct tok, SymbolEntry *sym)
{
    validate_integer_operand(tok, sym);

    if (tok.type == TOKEN_NUMBER)
        code_expr_push_number(tok.value);
    else
        code_expr_push_identifier(tok.value);
}

static void parse_factor_value(void);
static void parse_expression_until(tokenType stop1, tokenType stop2, int leave_on_stack);
static void emit_call_arguments(tokenType end_token);

static void parse_factor_value(void)
{
    SymbolEntry *symbol;

    if (current().type == TOKEN_NUMBER)
    {
        tokenStruct num = current();
        consume();
        code_expr_push_number(num.value);
        return;
    }

    if (current().type == TOKEN_IDENTIFIER)
    {
        validate_identifier_declared(current());
        symbol = get_symbol(current().value);

        if (symbol->sym_type == SYM_FUNC &&
            (current_program == NULL || strcmp(current_program, symbol->name) != 0))
        {
            tokenStruct fn = current();
            functionCall(fn);
            code_expr_push_rax();
            return;
        }

        tokenStruct id = current();
        consume();
        push_expression_value(id, symbol);
        return;
    }

    if (current().type == TOKEN_LEFT_PARENTH)
    {
        consume();
        parse_expression_until(TOKEN_RIGHT_PARENTH, TOKEN_EOF, 1);
        match_token(TOKEN_RIGHT_PARENTH);
        return;
    }

    error(current(), TOKEN_IDENTIFIER);
}

static void parse_expression_until(tokenType stop1, tokenType stop2, int leave_on_stack)
{
    tokenType op_stack[MAX_EXPR_OPS];
    int op_top = 0;

    if (is_expression_end(current().type, stop1, stop2))
        return;

    parse_factor_value();

    while (!is_expression_end(current().type, stop1, stop2))
    {
        if (is_arith_operator(current().type))
        {
            tokenType op = current().type;
            apply_pending_ops(op_stack, &op_top, op_precedence(op));
            if (op_top >= MAX_EXPR_OPS)
            {
                fprintf(stderr, "Error: Expression too complex at line %d, column %d\n",
                        current().line, current().column);
                exit(1);
            }
            op_stack[op_top++] = op;
            consume();
            parse_factor_value();
        }
        else
        {
            error(current(), TOKEN_PLUS);
        }
    }

    apply_pending_ops(op_stack, &op_top, 0);

    while (op_top > 0)
    {
        if (op_stack[op_top - 1] == TOKEN_LEFT_PARENTH)
        {
            fprintf(stderr, "Error: Mismatched parentheses at line %d, column %d\n",
                    current().line, current().column);
            exit(1);
        }
        code_expr_apply_operator(op_stack[--op_top]);
    }

    if (!leave_on_stack)
        code_expr_pop_to_rax();
}

static void emit_call_arguments(tokenType end_token)
{
    int arg_count = 0;

    if (current().type == end_token)
        return;

    parse_expression_until(TOKEN_COMMA, end_token, 0);
    code_set_call_arg(arg_count++);

    while (current().type == TOKEN_COMMA)
    {
        consume();
        if (arg_count >= MAX_PARAMS)
        {
            fprintf(stderr, "Error: Too many arguments at line %d, column %d\n",
                    current().line, current().column);
            exit(1);
        }
        parse_expression_until(TOKEN_COMMA, end_token, 0);
        code_set_call_arg(arg_count++);
    }
}

void functionCall(tokenStruct function_token)
{
    SymbolEntry *sym = get_symbol(function_token.value);

    validate_identifier_declared(function_token);
    if (sym && sym->type != TYPE_INT)
    {
        fprintf(stderr, "Error: Function '%s' used in expression but does not return integer at line %d, column %d\n",
                function_token.value, function_token.line, function_token.column);
        exit(1);
    }

    consume();
    match_token(TOKEN_LEFT_PARENTH);
    emit_call_arguments(TOKEN_RIGHT_PARENTH);
    match_token(TOKEN_RIGHT_PARENTH);
    code_emit_call(function_token.value);
}

static void procedureCall_local(tokenStruct proc_token)
{
    validate_identifier_declared(proc_token);
    consume();
    match_token(TOKEN_LEFT_PARENTH);
    emit_call_arguments(TOKEN_RIGHT_PARENTH);
    match_token(TOKEN_RIGHT_PARENTH);
    code_emit_call(proc_token.value);
}

// ============================================================================
// ASSIGNMENT PARSING
// ============================================================================

static void simpleAssignment(tokenStruct destination)
{
    match_token(TOKEN_ASSIGN);

    if (current().type == TOKEN_SEMICOLON)
    {
        fprintf(stderr, "Error: Expected expression after ':=' at line %d, column %d\n",
                current().line, current().column);
        exit(1);
    }

    SymbolEntry *dest_sym = get_symbol(destination.value);

    if (dest_sym && dest_sym->type == TYPE_STR)
    {
        if (current().type == TOKEN_LITERAL)
        {
            tokenStruct src = current();
            consume();
            code_simple_assignment(destination, src);
            return;
        }
        if (current().type == TOKEN_IDENTIFIER &&
            position + 1 < token_count &&
            buffer_tokens[position + 1].type == TOKEN_SEMICOLON)
        {
            tokenStruct src = current();
            SymbolEntry *src_sym = get_symbol(src.value);
            if (src_sym && src_sym->type == TYPE_STR)
            {
                consume();
                code_simple_assignment(destination, src);
                return;
            }
        }
    }

    parse_expression_until(TOKEN_SEMICOLON, TOKEN_EOF, 0);
    code_store_assignment(destination);
}

// ============================================================================
// STATEMENT PARSING
// ============================================================================

void statement()
{
    struct token identifier_token = current();
    validate_token(TOKEN_IDENTIFIER);

    SymbolEntry *symbol = get_symbol(identifier_token.value);

    if (symbol == NULL)
    {
        fprintf(stderr, "Error: Undeclared identifier '%s' at line %d, column %d\n",
                identifier_token.value, identifier_token.line, identifier_token.column);
        exit(1);
    }

    bool is_var = (symbol->sym_type == SYM_VAR);
    bool is_param = is_param_of_current(identifier_token.value);
    bool is_return = (current_program != NULL &&
                      strcmp(identifier_token.value, current_program) == 0);
    bool is_proc_call = (symbol->sym_type == SYM_PROC);
    bool is_func_call = (symbol->sym_type == SYM_FUNC);

    if (!is_var && !is_param && !is_return && !is_proc_call && !is_func_call)
    {
        fprintf(stderr, "Error: Cannot assign to '%s' at line %d, column %d\n",
                identifier_token.value, identifier_token.line, identifier_token.column);
        exit(1);
    }
    if (is_var || is_param || is_return)
    {
        consume();
        simpleAssignment(identifier_token);
    }
    else if (is_func_call)
    {
        functionCall(identifier_token);
    }
    else if (is_proc_call)
    {
        procedureCall_local(identifier_token);
    }
    else
    {
        fprintf(stderr, "Error: Expected ':=' or '(' after '%s' at line %d, column %d\n",
                identifier_token.value, identifier_token.line, identifier_token.column);
        exit(1);
    }
}

static void specialFunction(tokenType func_type)
{
    consume();
    match_token(TOKEN_LEFT_PARENTH);

    if (func_type == TOKEN_KEYWORD_WRITE)
    {
        if (current().type == TOKEN_RIGHT_PARENTH)
        {
            fprintf(stderr, "Error: 'write' expects exactly 1 argument at line %d, column %d\n",
                    current().line, current().column);
            exit(1);
        }
        parse_expression_until(TOKEN_RIGHT_PARENTH, TOKEN_EOF, 0);
        match_token(TOKEN_RIGHT_PARENTH);
        code_write_rax();
    }
}

void statementSeq()
{
    while (current().type != TOKEN_KEYWORD_END)
    {
        if (current().type == TOKEN_KEYWORD_WRITE)
        {
            specialFunction(TOKEN_KEYWORD_WRITE);
            match_token(TOKEN_SEMICOLON);
        }
        else
        {
            statement();
            match_token(TOKEN_SEMICOLON);
        }
    }
}

// ============================================================================
// SYM_VAR DECLARATION PARSING
// ============================================================================

void varSeq()
{
    int vars = 0;
    DataType var_type = TYPE_VOID;

    validate_token(TOKEN_IDENTIFIER);
    validate_new_identifier(current());
    add_to_symbol_table(current().value, TYPE_VOID);
    vars++;
    consume();

    while (current().type == TOKEN_COMMA)
    {
        consume();
        validate_token(TOKEN_IDENTIFIER);
        validate_new_identifier(current());
        add_to_symbol_table(current().value, TYPE_VOID);
        vars++;
        consume();
    }

    match_token(TOKEN_COLON);
    var_type = type();
    consume();

    for (int i = symbol_table_index - vars; i < symbol_table_index; i++)
    {
        symbol_table[i].type = var_type;
    }

    match_token(TOKEN_SEMICOLON);
}

void varDefs()
{
    match_token(TOKEN_KEYWORD_VAR);
    varSeq();

    while (current().type == TOKEN_IDENTIFIER)
    {
        varSeq();
    }
}

// ============================================================================
// SYM_PROC AND SYM_FUNC PARSING
// ============================================================================

void paramSeq(char **param_names, DataType *param_types, int *param_count)
{
    DataType return_type;
    int initial_param_count = *param_count;

    validate_token(TOKEN_IDENTIFIER);
    validate_new_identifier(current());
    param_names[*param_count] = strdup(current().value);
    param_types[*param_count] = TYPE_VOID;
    *param_count += 1;
    consume();

    while (current().type == TOKEN_COMMA)
    {
        if (*param_count >= MAX_PARAMS - 1)
        {
            fprintf(stderr, "Error: Too many parameters at line %d, column %d\n", current().line, current().column);
            exit(1);
        }

        consume();
        validate_token(TOKEN_IDENTIFIER);
        validate_new_identifier(current());

        param_names[*param_count] = strdup(current().value);
        param_types[*param_count] = TYPE_VOID;
        (*param_count)++;
        consume();
    }
    match_token(TOKEN_COLON);
    return_type = type();
    for (int i = initial_param_count; i < *param_count; i++)
    {
        param_types[i] = return_type;
    }
    consume();
}

void formalParams(char **param_name, DataType *param_type, int *param_count)
{
    match_token(TOKEN_LEFT_PARENTH);
    paramSeq(param_name, param_type, param_count);

    while (current().type == TOKEN_SEMICOLON)
    {
        consume();
        paramSeq(param_name, param_type, param_count);
    }

    match_token(TOKEN_RIGHT_PARENTH);
}

void procedureDecl()
{
    char *name;
    DataType *param_type;
    char **param_name;
    int param_count = 0;

    consume();
    validate_token(TOKEN_IDENTIFIER);
    name = strdup(current().value);
    current_program = name;
    consume();

    param_type = (DataType *)malloc(sizeof(DataType) * MAX_PARAMS);
    param_name = (char **)malloc(sizeof(char *) * MAX_PARAMS);

    if (param_type == NULL || param_name == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    enter_scope();
    formalParams(param_name, param_type, &param_count);
    add_procedure_to_symbol_table(name, param_name, param_type, param_count);
    symbol_table[symbol_table_index - 1].scope_level = current_scope_level - 1;

    code_proc_begin(name, param_name, param_count);

    match_token(TOKEN_SEMICOLON);

    if (current().type == TOKEN_KEYWORD_VAR)
    {
        varDefs();
        code_local_vars(symbol_table, symbol_table_index, current_scope_level);
    }

    match_token(TOKEN_KEYWORD_BEGIN);
    statementSeq();
    match_token(TOKEN_KEYWORD_END);
    if (strcmp(name, current().value) != 0)
    {
        fprintf(stderr, "Error: Procedure '%s' must end with its own name at line %d, column %d\n",
                name, current().line, current().column);
        exit(1);
    }
    match_token(TOKEN_IDENTIFIER);

    code_proc_end();

    exit_scope();
    match_token(TOKEN_SEMICOLON);
    free(name);
    current_program = NULL;
}

void functionDecl()
{
    DataType return_type;
    DataType *param_type;
    char **param_name;
    int param_count = 0;
    char *name;

    consume();
    validate_token(TOKEN_IDENTIFIER);
    name = strdup(current().value);
    current_program = name;
    consume();

    param_type = (DataType *)malloc(sizeof(DataType) * MAX_PARAMS);
    param_name = (char **)malloc(sizeof(char *) * MAX_PARAMS);

    if (param_type == NULL || param_name == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    enter_scope();
    formalParams(param_name, param_type, &param_count);
    match_token(TOKEN_COLON);
    return_type = type();
    consume();

    add_function_to_symbol_table(name, return_type, param_name, param_type, param_count);
    symbol_table[symbol_table_index - 1].scope_level = current_scope_level - 1;

    code_func_begin(name, param_name, param_count);

    match_token(TOKEN_SEMICOLON);

    if (current().type == TOKEN_KEYWORD_VAR)
    {
        varDefs();
        code_local_vars(symbol_table, symbol_table_index, current_scope_level);
    }

    match_token(TOKEN_KEYWORD_BEGIN);
    statementSeq();
    match_token(TOKEN_KEYWORD_END);
    if (strcmp(name, current().value) != 0)
    {
        fprintf(stderr, "Error: Function '%s' must end with its own name at line %d, column %d\n",
                name, current().line, current().column);
        exit(1);
    }
    match_token(TOKEN_IDENTIFIER);

    code_func_end();

    exit_scope();
    match_token(TOKEN_SEMICOLON);
    free(name);
    current_program = NULL;
}

// ============================================================================
// PROGRAM PARSING
// ============================================================================

void prgHeader()
{
    match_token(TOKEN_KEYWORD_PROGRAM);
    validate_token(TOKEN_IDENTIFIER);
    add_to_symbol_table(current().value, TYPE_VOID);
    consume();
    match_token(TOKEN_SEMICOLON);
}

void prg()
{
    prgHeader();
    varDefs();
    init_vars(symbol_table, symbol_table_index);
    init_text();

    while (current().type != TOKEN_KEYWORD_BEGIN)
    {
        if (current().type == TOKEN_KEYWORD_PROCEDURE)
        {
            procedureDecl();
        }
        else if (current().type == TOKEN_KEYWORD_FUNCTION)
        {
            functionDecl();
        }
        else
        {
            break;
        }
    }
    match_token(TOKEN_KEYWORD_BEGIN);
    start_program();
    statementSeq();
    match_token(TOKEN_KEYWORD_END);
    end_program();
    consume();
}

void run_parser()
{
    init_symbol_table();
    prg();
    free_symbol_table();
}