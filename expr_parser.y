/*
 * Bison grammar for the debugger's C-like expression language (used by the
 * print / set commands). Mirrors the precedence and semantics of the
 * former hand-written recursive-descent parser in expr.c 1:1; the actions
 * below call the same dbghelp-backed helpers (expr_internal.h) that the
 * evaluator/printer in expr.c also uses.
 */

%code requires {
    #include "expr.h"
    #include "debugger.h"

    /* Wrapper structs so union members stay assignable via plain '='. */
    typedef struct { char s[128]; } etext_t;
    typedef struct { char first[32]; char full[96]; } etypewords_t;
    typedef struct { int size; int is_ptr; } etypeinfo_t;
}

%code {
#include <stdio.h>
#include <string.h>
#include "expr_internal.h"

int expr_parse_err = 0;
char expr_parse_errmsg[256] = {0};

static expr_val_t g_expr_result;

void eparseerror(debugger_t *dbg, const char *s);
int  eparselex(void);

static expr_val_t propagate_err(expr_val_t *a, expr_val_t *b)
{
    if (a->errmsg[0]) return *a;
    if (b->errmsg[0]) return *b;
    expr_val_t z = {0};
    return z;
}

static expr_val_t do_ident(debugger_t *dbg, const char *name)
{
    DWORD64 addr = 0, modbase = 0;
    DWORD   tid  = 0;
    ULONG   sz   = 0;

    if (!expr_lookup_symbol(dbg, name, &addr, &modbase, &tid, &sz))
    {
        char msg[160];
        snprintf(msg, sizeof(msg), "symbol '%s' not found", name);
        return make_err(msg);
    }

    expr_val_t v = {0};
    v.mod_base = modbase;
    v.type_id  = tid;
    if (sz == 0)
    {
        /* register variable: addr holds the value */
        v.is_lvalue = 0;
        v.value     = (long long)addr;
        v.byte_size = 4;
    }
    else
    {
        v.is_lvalue = 1;
        v.addr      = addr;
        v.byte_size = sz;
        load_lvalue(dbg, &v);
    }
    return v;
}

static expr_val_t do_cast(debugger_t *dbg, etypeinfo_t *ty, expr_val_t *inner)
{
    (void)dbg;
    if (inner->errmsg[0]) return *inner;

    if (ty->is_ptr)
    {
        long long ptr_val = inner->is_lvalue ? (long long)inner->addr : inner->value;
        expr_val_t r = {0};
        r.value     = ptr_val;
        r.byte_size = 8;
        return r;
    }

    expr_val_t r = *inner;
    int sz = ty->size;
    if (sz == 1)      r.value = (long long)(signed char)r.value;
    else if (sz == 2) r.value = (long long)(short)r.value;
    else if (sz == 4) r.value = (long long)(int)r.value;
    r.byte_size = (ULONG)sz;
    r.is_lvalue = 0;
    r.type_id   = 0;
    return r;
}

static expr_val_t do_index(debugger_t *dbg, expr_val_t *base, expr_val_t *idx)
{
    if (base->errmsg[0]) return *base;
    if (idx->errmsg[0])  return *idx;

    DWORD tag = 0;
    SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_SYMTAG, &tag);

    DWORD elem_type = 0;
    DWORD64 base_addr = base->addr;

    if (tag == SYM_TAG_POINTER)
    {
        SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_TYPEID, &elem_type);
        base_addr = (DWORD64)base->value;
    }
    else if (tag == SYM_TAG_ARRAYTYPE)
    {
        SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_TYPEID, &elem_type);
    }
    else
    {
        return make_err("subscript requires array or pointer");
    }

    ULONG64 elem_len = 0;
    SymGetTypeInfo(dbg->sym_handle, base->mod_base, elem_type, TI_GET_LENGTH, &elem_len);
    if (elem_len == 0) elem_len = 4;

    expr_val_t r = {0};
    r.is_lvalue = 1;
    r.addr      = base_addr + (DWORD64)idx->value * elem_len;
    r.mod_base  = base->mod_base;
    r.type_id   = elem_type;
    r.byte_size = (ULONG)elem_len;
    load_lvalue(dbg, &r);
    return r;
}

static expr_val_t do_member(debugger_t *dbg, expr_val_t *base, const char *member, int is_arrow)
{
    if (base->errmsg[0]) return *base;

    DWORD64 base_addr = base->addr;
    DWORD   base_type = base->type_id;

    if (is_arrow)
    {
        DWORD tag = 0;
        SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_SYMTAG, &tag);
        if (tag == SYM_TAG_POINTER)
        {
            DWORD pt = 0;
            SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_TYPEID, &pt);
            base_type = pt;
        }
        base_addr = (DWORD64)base->value;
    }

    DWORD mem_type = 0, mem_off = 0;
    if (!lookup_member(dbg, base->mod_base, base_type, member, &mem_type, &mem_off))
    {
        char msg[160];
        snprintf(msg, sizeof(msg), "member '%s' not found", member);
        return make_err(msg);
    }

    ULONG64 mem_len = 0;
    SymGetTypeInfo(dbg->sym_handle, base->mod_base, mem_type, TI_GET_LENGTH, &mem_len);
    if (mem_len == 0) mem_len = 4;

    expr_val_t r = {0};
    r.is_lvalue = 1;
    r.addr      = base_addr + mem_off;
    r.mod_base  = base->mod_base;
    r.type_id   = mem_type;
    r.byte_size = (ULONG)mem_len;
    load_lvalue(dbg, &r);
    return r;
}

static expr_val_t do_deref(debugger_t *dbg, expr_val_t *inner)
{
    if (inner->errmsg[0]) return *inner;

    DWORD tag = 0;
    SymGetTypeInfo(dbg->sym_handle, inner->mod_base, inner->type_id, TI_GET_SYMTAG, &tag);
    if (tag != SYM_TAG_POINTER)
        return make_err("dereference of non-pointer");

    DWORD pt = 0;
    SymGetTypeInfo(dbg->sym_handle, inner->mod_base, inner->type_id, TI_GET_TYPEID, &pt);

    ULONG64 len = 0;
    SymGetTypeInfo(dbg->sym_handle, inner->mod_base, pt, TI_GET_LENGTH, &len);
    if (len == 0) len = 4;

    expr_val_t r = {0};
    r.is_lvalue = 1;
    r.addr      = (DWORD64)inner->value;
    r.mod_base  = inner->mod_base;
    r.type_id   = pt;
    r.byte_size = (ULONG)len;
    load_lvalue(dbg, &r);
    return r;
}

static expr_val_t do_addrof(expr_val_t *inner)
{
    if (inner->errmsg[0]) return *inner;
    if (!inner->is_lvalue) return make_err("& requires lvalue");
    expr_val_t r = make_val((long long)inner->addr);
    r.byte_size  = 8;
    r.is_address = 1;
    r.mod_base   = inner->mod_base;
    r.type_id    = inner->type_id;
    return r;
}
}

%parse-param { debugger_t *dbg }
%define api.prefix {eparse}
%define parse.error verbose

%union {
    expr_val_t   val;
    etext_t      text;
    etypewords_t words;
    etypeinfo_t  ty;
}

%token <val>  NUM
%token <text> IDENT
%token <text> TYPEWORD
%token SIZEOF
%token ARROW LSHIFT RSHIFT ANDAND OROR EQ NEQ LE GE

%type <val>   input expr logand_expr bitor_expr bitxor_expr
%type <val>   bitand_expr eq_expr rel_expr shift_expr add_expr mul_expr
%type <val>   unary_expr postfix_expr primary_expr
%type <words> typename_words
%type <ty>    typename
%type <ty>    stars_opt

%%

input:
      expr { g_expr_result = $1; $$ = $1; }
    ;

expr:
      logand_expr { $$ = $1; }
    | expr OROR logand_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value || $3.value);
      }
    ;

logand_expr:
      bitor_expr { $$ = $1; }
    | logand_expr ANDAND bitor_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value && $3.value);
      }
    ;

bitor_expr:
      bitxor_expr { $$ = $1; }
    | bitor_expr '|' bitxor_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value | $3.value);
      }
    ;

bitxor_expr:
      bitand_expr { $$ = $1; }
    | bitxor_expr '^' bitand_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value ^ $3.value);
      }
    ;

bitand_expr:
      eq_expr { $$ = $1; }
    | bitand_expr '&' eq_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value & $3.value);
      }
    ;

eq_expr:
      rel_expr { $$ = $1; }
    | eq_expr EQ rel_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value == $3.value);
      }
    | eq_expr NEQ rel_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value != $3.value);
      }
    ;

rel_expr:
      shift_expr { $$ = $1; }
    | rel_expr '<' shift_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value < $3.value);
      }
    | rel_expr '>' shift_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value > $3.value);
      }
    | rel_expr LE shift_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value <= $3.value);
      }
    | rel_expr GE shift_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value >= $3.value);
      }
    ;

shift_expr:
      add_expr { $$ = $1; }
    | shift_expr LSHIFT add_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value << ($3.value & 63));
      }
    | shift_expr RSHIFT add_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val((long long)((unsigned long long)$1.value >> ($3.value & 63)));
      }
    ;

add_expr:
      mul_expr { $$ = $1; }
    | add_expr '+' mul_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value + $3.value);
      }
    | add_expr '-' mul_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value - $3.value);
      }
    ;

mul_expr:
      unary_expr { $$ = $1; }
    | mul_expr '*' unary_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          $$ = e.errmsg[0] ? e : make_val($1.value * $3.value);
      }
    | mul_expr '/' unary_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          if (e.errmsg[0]) $$ = e;
          else if ($3.value == 0) $$ = make_err("division by zero");
          else $$ = make_val($1.value / $3.value);
      }
    | mul_expr '%' unary_expr {
          expr_val_t e = propagate_err(&$1, &$3);
          if (e.errmsg[0]) $$ = e;
          else if ($3.value == 0) $$ = make_err("division by zero");
          else $$ = make_val($1.value % $3.value);
      }
    ;

unary_expr:
      postfix_expr             { $$ = $1; }
    | '*' unary_expr           { $$ = do_deref(dbg, &$2); }
    | '&' unary_expr           { $$ = do_addrof(&$2); }
    | '-' unary_expr           { $$ = $2.errmsg[0] ? $2 : make_val(-$2.value); }
    | '+' unary_expr           { $$ = $2; }
    | '~' unary_expr           { $$ = $2.errmsg[0] ? $2 : make_val(~$2.value); }
    | '!' unary_expr           { $$ = $2.errmsg[0] ? $2 : make_val(!$2.value); }
    | '(' typename ')' unary_expr { $$ = do_cast(dbg, &$2, &$4); }
    ;

postfix_expr:
      primary_expr                       { $$ = $1; }
    | postfix_expr '[' expr ']'          { $$ = do_index(dbg, &$1, &$3); }
    | postfix_expr '.' IDENT             { $$ = do_member(dbg, &$1, $3.s, 0); }
    | postfix_expr ARROW IDENT           { $$ = do_member(dbg, &$1, $3.s, 1); }
    ;

primary_expr:
      NUM                       { $$ = $1; }
    | IDENT                     { $$ = do_ident(dbg, $1.s); }
    | SIZEOF '(' typename ')'   { $$ = make_val($3.size); }
    | SIZEOF '(' expr ')'       { $$ = make_val($3.byte_size > 0 ? (long long)$3.byte_size : 4); }
    | '(' expr ')'              { $$ = $2; }
    ;

typename_words:
      TYPEWORD {
          etypewords_t w = {0};
          strncpy_s(w.first, sizeof(w.first), $1.s, _TRUNCATE);
          strncpy_s(w.full,  sizeof(w.full),  $1.s, _TRUNCATE);
          $$ = w;
      }
    | TYPEWORD TYPEWORD {
          etypewords_t w = {0};
          strncpy_s(w.first, sizeof(w.first), $1.s, _TRUNCATE);
          snprintf(w.full, sizeof(w.full), "%s %s", $1.s, $2.s);
          $$ = w;
      }
    | TYPEWORD TYPEWORD TYPEWORD {
          etypewords_t w = {0};
          strncpy_s(w.first, sizeof(w.first), $1.s, _TRUNCATE);
          snprintf(w.full, sizeof(w.full), "%s %s %s", $1.s, $2.s, $3.s);
          $$ = w;
      }
    ;

typename:
      typename_words stars_opt {
          etypeinfo_t ti = {0};
          int sz = builtin_type_size($1.full);
          if (sz < 0) sz = builtin_type_size($1.first);
          ti.is_ptr = $2.is_ptr;
          ti.size   = ti.is_ptr ? 8 : sz;
          $$ = ti;
      }
    ;

stars_opt:
      /* empty */    { etypeinfo_t z = {0}; $$ = z; }
    | stars_opt '*'  { etypeinfo_t z = {0}; z.is_ptr = 1; $$ = z; }
    ;

%%

void eparseerror(debugger_t *dbg, const char *s)
{
    (void)dbg;
    if (!expr_parse_err)
    {
        expr_parse_err = 1;
        strncpy_s(expr_parse_errmsg, sizeof(expr_parse_errmsg), s, _TRUNCATE);
    }
}

#ifndef YY_NO_UNISTD_H
#define YY_NO_UNISTD_H 1
#endif
#include "expr_lexer.h"

int expr_parse(debugger_t *dbg, const char *src, expr_val_t *out)
{
    expr_parse_err = 0;
    expr_parse_errmsg[0] = '\0';
    memset(&g_expr_result, 0, sizeof(g_expr_result));

    YY_BUFFER_STATE buf = eparse_scan_string(src);
    int rc = eparseparse(dbg);
    eparse_delete_buffer(buf);

    if (out) *out = g_expr_result;

    if (rc != 0 || expr_parse_err || g_expr_result.errmsg[0])
    {
        if (out && !out->errmsg[0])
            strncpy_s(out->errmsg, sizeof(out->errmsg),
                      expr_parse_errmsg[0] ? expr_parse_errmsg : "parse error", _TRUNCATE);
        return -1;
    }
    return 0;
}
