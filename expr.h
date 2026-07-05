#pragma once
#include "debugger.h"

/*
 * Expression evaluator for the debugger.
 * Supports a subset of C expressions:
 *   - Literals: decimal, hex (0x), octal (0)
 *   - Variables / struct members: var, var.mem, var->mem, var[i]
 *   - Unary: *, &, -, +, ~, !
 *   - Binary: +, -, *, /, %, <<, >>, &, |, ^, ==, !=, <, >, <=, >=, &&, ||
 *   - Cast: (type*)expr, (int)expr, etc.
 *   - sizeof: sizeof(type), sizeof(expr)
 */

typedef enum {
    EVAL_OK,
    EVAL_ERR
} eval_status_t;

/* Result of evaluating an expression */
typedef struct {
    /* If is_lvalue: address in debuggee memory + type info */
    int      is_lvalue;
    DWORD64  addr;       /* valid when is_lvalue */

    /* Always filled: the numeric value (or address when is_lvalue) */
    long long value;

    /* Type info (may be 0 if unknown) */
    DWORD64  mod_base;
    DWORD    type_id;
    ULONG    byte_size;  /* size in bytes of the type */

    char     errmsg[256];
} expr_val_t;

/* Evaluate expression string, return EVAL_OK on success */
eval_status_t expr_eval(debugger_t *dbg, const char *expr, expr_val_t *out);

/* Print result (like p command) */
void expr_print(debugger_t *dbg, const char *expr);

/* Write value to lvalue result (like set command) */
int expr_assign(debugger_t *dbg, const char *lhs, long long value);
