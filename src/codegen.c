#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "token.h"
#include "parser.h"

FILE *output_file;

#define MAX_LOCAL_VARS 64

typedef struct
{
    char name[64];
    int offset;
} LocalVar;

static LocalVar local_var_table[MAX_LOCAL_VARS];
static int local_var_count = 0;
static char current_func[64] = "";

static const char *arg_regs[MAX_PARAMS] = {
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

static void clear_local_vars()
{
    memset(local_var_table, 0, sizeof local_var_table);
    local_var_count = 0;
    current_func[0] = '\0';
}

static void register_local(const char *name, int offset)
{
    if (local_var_count >= MAX_LOCAL_VARS)
        return;
    strncpy(local_var_table[local_var_count].name, name, 63);
    local_var_table[local_var_count].offset = offset;
    local_var_count++;
}

static const char *resolve(const char *name)
{
    for (int i = 0; i < local_var_count; i++)
        if (strcmp(local_var_table[i].name, name) == 0)
        {
            static char buf[32];
            snprintf(buf, sizeof buf, "%d(%%rbp)", local_var_table[i].offset);
            return buf;
        }
    return name;
}

// ============================================================================
// EXISTING HELPERS (unchanged)
// ============================================================================

static int is_number(const char *str)
{
    if (!str || !*str)
        return 0;
    for (int i = 0; str[i]; i++)
        if (!isdigit(str[i]))
            return 0;
    return 1;
}

// ============================================================================
// FILE MANAGEMENT
// ============================================================================

void init_assembler_file(char *name)
{
    output_file = fopen(name, "w");
    if (!output_file)
    {
        perror("Error opening output file");
        exit(1);
    }
}

void close_assembler_file()
{
    if (output_file)
        fclose(output_file);
}

// ============================================================================
// GLOBAL VARIABLE INIT  (called once for global vars before any functions)
// ============================================================================

void init_vars(SymbolEntry *symbol_table, int symbol_table_index)
{
    fputs(".section .data\n", output_file);
    fputs("_fmt_int: .asciz \"%d\\n\"\n", output_file); // ← must be here
    for (int i = 0; i < symbol_table_index; i++)
    {
        if (symbol_table[i].sym_type != SYM_VAR)
            continue;

        if (symbol_table[i].type == TYPE_INT)
            fprintf(output_file, "%s: .quad 0\n", symbol_table[i].name);
        else if (symbol_table[i].type == TYPE_STR)
            fprintf(output_file, "%s: .asciz \"\"\n", symbol_table[i].name);
    }
}

// ============================================================================
// MAIN PROGRAM START / END
// ============================================================================

void init_text()
{
    fputs("\n.section .text\n", output_file);
    fputs(".global _start\n", output_file);
}

void start_program()
{
    fputs("_start:\n", output_file);
}

void end_program()
{
    fputs("mov %rax, %rdi\n", output_file);
    fputs("mov $60, %rax\n", output_file);
    fputs("syscall\n", output_file);
}

// ============================================================================
// PROCEDURE DECLARATION
// ============================================================================

void code_proc_begin(char *name, char **param_names, int param_count)
{
    clear_local_vars();
    strncpy(current_func, name, 63);

    fprintf(output_file, "\n%s:\n", name);
    fputs("push %rbp\n", output_file);
    fputs("mov %rsp, %rbp\n", output_file);

    for (int i = 0; i < param_count && i < MAX_PARAMS; i++)
    {
        int offset = -(i + 1) * 8;
        fprintf(output_file, "mov %s, %d(%%rbp)\n", arg_regs[i], offset);
        register_local(param_names[i], offset);
    }
}

void code_local_vars(SymbolEntry *symbol_table, int symbol_table_index, int scope_level)
{
    int n_locals = 0;
    for (int i = 0; i < symbol_table_index; i++)
        if (symbol_table[i].scope_level == scope_level &&
            symbol_table[i].sym_type == SYM_VAR)
            n_locals++;

    if (n_locals == 0)
        return;

    int total_slots = local_var_count + n_locals;
    int stack_bytes = (total_slots * 8 + 15) & ~15;
    fprintf(output_file, "sub $%d, %%rsp\n", stack_bytes);

    int slot = local_var_count;
    for (int i = 0; i < symbol_table_index; i++)
    {
        if (symbol_table[i].scope_level == scope_level &&
            symbol_table[i].sym_type == SYM_VAR)
        {
            int offset = -(slot + 1) * 8;
            fprintf(output_file, "movq $0, %d(%%rbp)\n", offset);
            register_local(symbol_table[i].name, offset);
            slot++;
        }
    }
}

void code_proc_end()
{
    fputs("leave\n", output_file);
    fputs("ret\n", output_file);
    clear_local_vars();
}

// ============================================================================
// FUNCTION DECLARATION  (same frame layout; return value left in %rax)
// ============================================================================

void code_write_call(tokenStruct arg)
{
    fputs("lea _fmt_int(%rip), %rdi\n", output_file);

    if (is_number(arg.value))
        fprintf(output_file, "mov $%s, %%rsi\n", arg.value);
    else
        fprintf(output_file, "mov %s, %%rsi\n", resolve(arg.value));

    fputs("xor %eax, %eax\n", output_file);
    fputs("call printf\n", output_file);
}

void code_func_begin(char *name, char **param_names, int param_count)
{
    code_proc_begin(name, param_names, param_count);
}

void code_func_end()
{
    code_proc_end();
}

// ============================================================================
// PROCEDURE CALL  (used as a statement)
// ============================================================================

void code_procedure_call(tokenStruct proc, tokenStruct *args, int arg_count)
{
    // Load each argument into the matching register
    for (int i = 0; i < arg_count && i < MAX_PARAMS; i++)
    {
        if (is_number(args[i].value))
            fprintf(output_file, "mov $%s, %s\n", args[i].value, arg_regs[i]);
        else
            fprintf(output_file, "mov %s, %s\n", resolve(args[i].value), arg_regs[i]);
    }
    fprintf(output_file, "call %s\n", proc.value);
}

// ============================================================================
// FUNCTION CALL  (used as an operand — result is in %rax after the call)
// ============================================================================

void code_function_call(tokenStruct func, tokenStruct *args, int arg_count)
{
    code_procedure_call(func, args, arg_count);
}

// ============================================================================
// ASSIGNMENT  (updated to handle locals/params and function return values)
// ============================================================================

void code_simple_assignment(tokenStruct destination, tokenStruct source)
{
    if (is_number(source.value))
        fprintf(output_file, "mov $%s, %%rax\n", source.value);
    else
    {
        SymbolEntry *sym = get_symbol(source.value);
        if (!(sym && sym->sym_type == SYM_FUNC && strcmp(sym->name, source.value) == 0))
            fprintf(output_file, "mov %s, %%rax\n", resolve(source.value));
    }

    if (current_func[0] != '\0' && strcmp(destination.value, current_func) == 0)
        return;

    fprintf(output_file, "mov %%rax, %s\n", resolve(destination.value));
}

// ============================================================================
// SHUNTING-YARD EXPRESSION CODEGEN (value stack = hardware push/pop %rax)
// ============================================================================

static int expr_stack_depth = 0;

void code_expr_push_number(const char *value)
{
    fprintf(output_file, "mov $%s, %%rax\n", value);
    fputs("push %rax\n", output_file);
    expr_stack_depth++;
}

void code_expr_push_identifier(const char *name)
{
    fprintf(output_file, "mov %s, %%rax\n", resolve(name));
    fputs("push %rax\n", output_file);
    expr_stack_depth++;
}

void code_expr_push_rax(void)
{
    fputs("push %rax\n", output_file);
    expr_stack_depth++;
}

void code_expr_apply_operator(tokenType op)
{
    if (expr_stack_depth < 2)
    {
        fprintf(stderr, "Error: Expression stack underflow\n");
        exit(1);
    }

    fputs("pop %rcx\n", output_file);
    fputs("pop %rax\n", output_file);

    switch (op)
    {
    case TOKEN_PLUS:
        fputs("add %rcx, %rax\n", output_file);
        break;
    case TOKEN_MINUS:
        fputs("sub %rcx, %rax\n", output_file);
        break;
    case TOKEN_MULTIPLY:
        fputs("imul %rcx, %rax\n", output_file);
        break;
    case TOKEN_DIVIDE:
        fputs("push %rdx\n", output_file);
        fputs("xor %rdx, %rdx\n", output_file);
        fputs("idiv %rcx\n", output_file);
        fputs("pop %rdx\n", output_file);
        break;
    default:
        fprintf(stderr, "Error: Unsupported operator in expression\n");
        exit(1);
    }

    fputs("push %rax\n", output_file);
}

void code_expr_pop_to_rax(void)
{
    if (expr_stack_depth < 1)
    {
        fprintf(stderr, "Error: Expression stack empty\n");
        exit(1);
    }
    fputs("pop %rax\n", output_file);
    expr_stack_depth--;
}

void code_store_assignment(tokenStruct destination)
{
    if (current_func[0] != '\0' && strcmp(destination.value, current_func) == 0)
        return;

    fprintf(output_file, "mov %%rax, %s\n", resolve(destination.value));
}

void code_set_call_arg(int index)
{
    if (index >= 0 && index < MAX_PARAMS)
        fprintf(output_file, "mov %%rax, %s\n", arg_regs[index]);
}

void code_emit_call(const char *name)
{
    fprintf(output_file, "call %s\n", name);
}

void code_write_rax(void)
{
    fputs("lea _fmt_int(%rip), %rdi\n", output_file);
    fputs("mov %rax, %rsi\n", output_file);
    fputs("xor %eax, %eax\n", output_file);
    fputs("call printf\n", output_file);
}

void code_complex_assignment(tokenStruct destination, tokenStruct operand1,
                             tokenType operator, tokenStruct operand2)
{
    if (is_number(operand1.value))
        fprintf(output_file, "mov $%s, %%rax\n", operand1.value);
    else
    {
        SymbolEntry *s1 = get_symbol(operand1.value);
        if (!(s1 && s1->sym_type == SYM_FUNC && strcmp(s1->name, operand1.value) == 0))
            fprintf(output_file, "mov %s, %%rax\n", resolve(operand1.value));
    }

    if (is_number(operand2.value))
        fprintf(output_file, "mov $%s, %%rcx\n", operand2.value);
    else
    {
        SymbolEntry *s2 = get_symbol(operand2.value);
        if (s2 && s2->sym_type == SYM_FUNC && strcmp(s2->name, operand2.value) == 0)
            fputs("mov %rax, %rcx\n", output_file);
        else
            fprintf(output_file, "mov %s, %%rcx\n", resolve(operand2.value));
    }

    switch (operator)
    {
    case TOKEN_PLUS:
        fputs("add %rcx, %rax\n", output_file);
        break;
    case TOKEN_MINUS:
        fputs("sub %rcx, %rax\n", output_file);
        break;
    case TOKEN_MULTIPLY:
        fputs("imul %rcx, %rax\n", output_file);
        break;
    case TOKEN_DIVIDE:
        fputs("push %rdx\n", output_file);
        fputs("xor %rdx, %rdx\n", output_file);
        fputs("idiv %rcx\n", output_file);
        fputs("pop %rdx\n", output_file);
        break;
    default:
        fprintf(stderr, "Error: Unsupported operator\n");
    }

    if (current_func[0] != '\0' && strcmp(destination.value, current_func) == 0)
        return;

    fprintf(output_file, "mov %%rax, %s\n", resolve(destination.value));
}