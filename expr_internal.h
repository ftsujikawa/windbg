#pragma once
#include "expr.h"
#include "symbols.h"

/*
 * Shared between expr.c (helpers + evaluation/printing) and the generated
 * bison/flex expression parser (expr_parser.y / expr_lexer.l). The grammar
 * only builds expr_val_t results; symbol/type lookups and memory access
 * live here so both sides can call the same code.
 */

/* DbgHelp type tag constants */
#define SYM_TAG_UDT        11
#define SYM_TAG_POINTER    14
#define SYM_TAG_ARRAYTYPE  15
#define SYM_TAG_BASETYPE   16
#define SYM_TAG_TYPEDEF    17

/* Known primitive type names -> byte size (used by cast / sizeof(type)) */
int builtin_type_size(const char *name);

/* Build a plain error / integer result */
expr_val_t make_err(const char *msg);
expr_val_t make_val(long long n);

/* Read the current value of an lvalue into v->value (and v->fvalue if float) */
void load_lvalue(debugger_t *dbg, expr_val_t *v);

/* sizeof helper: given a type_id, return byte count */
ULONG type_sizeof(debugger_t *dbg, DWORD64 modbase, DWORD type_id);

/* Symbol / member lookups (dbghelp-backed) */
int expr_lookup_symbol(debugger_t *dbg, const char *name, DWORD64 *out_addr,
                        DWORD64 *out_modbase, DWORD *out_typeid, ULONG *out_size);

int lookup_member(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                   const char *member, DWORD *out_typeid, DWORD *out_offset);

/* Parses and evaluates `src`, filling *out. Returns 0 on success, -1 on
 * error (out->errmsg set either way). Implemented by the generated
 * bison/flex parser (expr_parser.y). */
int expr_parse(debugger_t *dbg, const char *src, expr_val_t *out);

/* Shared error-reporting state between expr_parser.y and expr_lexer.l */
extern int  expr_parse_err;
extern char expr_parse_errmsg[256];
