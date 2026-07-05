#pragma execution_character_set("utf-8")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "expr.h"
#include "symbols.h"

/* ------------------------------------------------------------------ */
/* DbgHelp type tag constants                                          */
/* ------------------------------------------------------------------ */
#define SYM_TAG_UDT        11
#define SYM_TAG_POINTER    14
#define SYM_TAG_ARRAYTYPE  15
#define SYM_TAG_BASETYPE    2
#define SYM_TAG_TYPEDEF    17

/* ------------------------------------------------------------------ */
/* Token types                                                         */
/* ------------------------------------------------------------------ */
typedef enum {
    TOK_EOF,
    TOK_NUM,       /* integer literal */
    TOK_IDENT,     /* identifier */
    TOK_PLUS,      /* + */
    TOK_MINUS,     /* - */
    TOK_STAR,      /* * */
    TOK_SLASH,     /* / */
    TOK_PERCENT,   /* % */
    TOK_AMP,       /* & */
    TOK_PIPE,      /* | */
    TOK_CARET,     /* ^ */
    TOK_TILDE,     /* ~ */
    TOK_BANG,      /* ! */
    TOK_LSHIFT,    /* << */
    TOK_RSHIFT,    /* >> */
    TOK_AMAMP,     /* && */
    TOK_PIPEPIPE,  /* || */
    TOK_EQ,        /* == */
    TOK_NEQ,       /* != */
    TOK_LT,        /* < */
    TOK_GT,        /* > */
    TOK_LE,        /* <= */
    TOK_GE,        /* >= */
    TOK_LPAREN,    /* ( */
    TOK_RPAREN,    /* ) */
    TOK_LBRACKET,  /* [ */
    TOK_RBRACKET,  /* ] */
    TOK_DOT,       /* . */
    TOK_ARROW,     /* -> */
    TOK_COMMA,     /* , */
} token_kind_t;

typedef struct {
    token_kind_t kind;
    char         text[128];
    long long    ival;
} token_t;

/* ------------------------------------------------------------------ */
/* Lexer state                                                         */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *src;
    int         pos;
    token_t     cur;
    debugger_t *dbg;
    char        errmsg[256];
    int         err;
} lex_t;

static void lex_error(lex_t *l, const char *msg)
{
    if (!l->err)
    {
        strncpy(l->errmsg, msg, sizeof(l->errmsg) - 1);
        l->err = 1;
    }
}

static void lex_advance(lex_t *l)
{
    const char *s = l->src;
    int p = l->pos;

    while (s[p] && isspace((unsigned char)s[p])) p++;

    if (!s[p])
    {
        l->cur.kind = TOK_EOF;
        l->pos = p;
        return;
    }

    /* Number */
    if (isdigit((unsigned char)s[p]))
    {
        char *end;
        long long v = strtoll(s + p, &end, 0);
        l->cur.kind = TOK_NUM;
        l->cur.ival = v;
        int len = (int)(end - (s + p));
        if (len >= (int)sizeof(l->cur.text)) len = (int)sizeof(l->cur.text) - 1;
        strncpy(l->cur.text, s + p, len);
        l->cur.text[len] = '\0';
        l->pos = (int)(end - s);
        return;
    }

    /* Identifier */
    if (isalpha((unsigned char)s[p]) || s[p] == '_')
    {
        int start = p;
        while (s[p] && (isalnum((unsigned char)s[p]) || s[p] == '_')) p++;
        int len = p - start;
        if (len >= (int)sizeof(l->cur.text)) len = (int)sizeof(l->cur.text) - 1;
        strncpy(l->cur.text, s + start, len);
        l->cur.text[len] = '\0';
        l->cur.kind = TOK_IDENT;
        l->pos = p;
        return;
    }

    /* Two-char operators */
    if (s[p] == '-' && s[p+1] == '>')
    { l->cur.kind = TOK_ARROW;   strncpy(l->cur.text, "->", sizeof(l->cur.text)-1); l->pos = p+2; return; }
    if (s[p] == '<' && s[p+1] == '<')
    { l->cur.kind = TOK_LSHIFT;  strncpy(l->cur.text, "<<", sizeof(l->cur.text)-1); l->pos = p+2; return; }
    if (s[p] == '>' && s[p+1] == '>')
    { l->cur.kind = TOK_RSHIFT;  strncpy(l->cur.text, ">>", sizeof(l->cur.text)-1); l->pos = p+2; return; }
    if (s[p] == '&' && s[p+1] == '&')
    { l->cur.kind = TOK_AMAMP;   strncpy(l->cur.text, "&&", sizeof(l->cur.text)-1); l->pos = p+2; return; }
    if (s[p] == '|' && s[p+1] == '|')
    { l->cur.kind = TOK_PIPEPIPE;strncpy(l->cur.text, "||", sizeof(l->cur.text)-1); l->pos = p+2; return; }
    if (s[p] == '=' && s[p+1] == '=')
    { l->cur.kind = TOK_EQ;      strncpy(l->cur.text, "==", sizeof(l->cur.text)-1); l->pos = p+2; return; }
    if (s[p] == '!' && s[p+1] == '=')
    { l->cur.kind = TOK_NEQ;     strncpy(l->cur.text, "!=", sizeof(l->cur.text)-1); l->pos = p+2; return; }
    if (s[p] == '<' && s[p+1] == '=')
    { l->cur.kind = TOK_LE;      strncpy(l->cur.text, "<=", sizeof(l->cur.text)-1); l->pos = p+2; return; }
    if (s[p] == '>' && s[p+1] == '=')
    { l->cur.kind = TOK_GE;      strncpy(l->cur.text, ">=", sizeof(l->cur.text)-1); l->pos = p+2; return; }

    /* Single-char operators */
    switch (s[p])
    {
    case '+': l->cur.kind = TOK_PLUS;     break;
    case '-': l->cur.kind = TOK_MINUS;    break;
    case '*': l->cur.kind = TOK_STAR;     break;
    case '/': l->cur.kind = TOK_SLASH;    break;
    case '%': l->cur.kind = TOK_PERCENT;  break;
    case '&': l->cur.kind = TOK_AMP;      break;
    case '|': l->cur.kind = TOK_PIPE;     break;
    case '^': l->cur.kind = TOK_CARET;    break;
    case '~': l->cur.kind = TOK_TILDE;    break;
    case '!': l->cur.kind = TOK_BANG;     break;
    case '(': l->cur.kind = TOK_LPAREN;   break;
    case ')': l->cur.kind = TOK_RPAREN;   break;
    case '[': l->cur.kind = TOK_LBRACKET; break;
    case ']': l->cur.kind = TOK_RBRACKET; break;
    case '.': l->cur.kind = TOK_DOT;      break;
    case '<': l->cur.kind = TOK_LT;       break;
    case '>': l->cur.kind = TOK_GT;       break;
    case ',': l->cur.kind = TOK_COMMA;    break;
    default:
        {
            char msg[64];
            snprintf(msg, sizeof(msg), "unexpected char '%c'", s[p]);
            lex_error(l, msg);
            l->cur.kind = TOK_EOF;
            l->pos = p + 1;
            return;
        }
    }
    l->cur.text[0] = s[p]; l->cur.text[1] = '\0';
    l->pos = p + 1;
}

/* ------------------------------------------------------------------ */
/* Type name lookup helpers                                            */
/* ------------------------------------------------------------------ */

/* Known primitive type names → byte size */
static int builtin_type_size(const char *name)
{
    if (strcmp(name, "char")   == 0) return 1;
    if (strcmp(name, "short")  == 0) return 2;
    if (strcmp(name, "int")    == 0) return 4;
    if (strcmp(name, "long")   == 0) return 8;
    if (strcmp(name, "float")  == 0) return 4;
    if (strcmp(name, "double") == 0) return 8;
    if (strcmp(name, "void")   == 0) return 0;
    /* unsigned/signed variants */
    if (strncmp(name, "unsigned", 8) == 0) return 4;
    if (strncmp(name, "signed",   6) == 0) return 4;
    if (strcmp(name, "size_t")    == 0) return 8;
    if (strcmp(name, "ptrdiff_t") == 0) return 8;
    if (strcmp(name, "DWORD")     == 0) return 4;
    if (strcmp(name, "DWORD64")   == 0) return 8;
    if (strcmp(name, "BYTE")      == 0) return 1;
    if (strcmp(name, "WORD")      == 0) return 2;
    if (strcmp(name, "QWORD")     == 0) return 8;
    return -1;
}

static int expr_lookup_symbol(debugger_t *dbg, const char *name, DWORD64 *out_addr,
                              DWORD64 *out_modbase, DWORD *out_typeid, ULONG *out_size);

static int lookup_member(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                         const char *member, DWORD *out_typeid, DWORD *out_offset);

/* ------------------------------------------------------------------ */
/* Parser / evaluator                                                  */
/* ------------------------------------------------------------------ */
static expr_val_t parse_expr(lex_t *l);   /* forward */
static expr_val_t parse_assign_expr(lex_t *l); /* forward */

static expr_val_t make_err(const char *msg)
{
    expr_val_t v = {0};
    strncpy(v.errmsg, msg, sizeof(v.errmsg) - 1);
    return v;
}

static expr_val_t make_val(long long n)
{
    expr_val_t v = {0};
    v.value     = n;
    v.byte_size = 8;
    return v;
}

/* Read the current value of an lvalue into v.value */
static void load_lvalue(debugger_t *dbg, expr_val_t *v)
{
    if (!v->is_lvalue) return;
    if (!v->addr) return;
    ULONG sz = v->byte_size ? v->byte_size : 8;
    if (sz > 8) sz = 8;
    unsigned long long raw = 0;
    SIZE_T n = 0;
    if (!ReadProcessMemory(dbg->process, (void*)v->addr, &raw, sz, &n))
        return;
    if (sz <= 4)
        v->value = (long long)(int)(unsigned int)raw;
    else
        v->value = (long long)raw;
}

/* sizeof helper: given a type_id, return byte count */
static ULONG type_sizeof(debugger_t *dbg, DWORD64 modbase, DWORD type_id)
{
    ULONG64 len = 0;
    SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_LENGTH, &len);
    return (ULONG)len;
}

/* ------------------------------------------------------------------ */
/* Parse a type-name in parentheses for cast / sizeof.                */
/* Returns byte_size; sets *out_is_pointer if it ends with '*'.       */
/* Returns -1 if not a type name.                                     */
/* ------------------------------------------------------------------ */
static int try_parse_typename(lex_t *l, int *out_is_pointer)
{
    /* Save position so we can backtrack */
    int saved_pos = l->pos;
    token_t saved_tok = l->cur;

    *out_is_pointer = 0;

    if (l->cur.kind != TOK_IDENT)
        return -1;

    /* Accept optional: unsigned, signed, long, short */
    char base[128] = {0};
    strncpy(base, l->cur.text, sizeof(base) - 1);

    /* multi-word types: "unsigned long long", etc. */
    int sz = -1;

    /* Try to consume type words */
    char full[256] = {0};
    strncpy(full, l->cur.text, sizeof(full) - 1);
    lex_advance(l);

    /* Allow one more word (e.g. "unsigned int", "long long") */
    if (l->cur.kind == TOK_IDENT &&
        (strcmp(l->cur.text,"int")==0   || strcmp(l->cur.text,"char")==0  ||
         strcmp(l->cur.text,"short")==0 || strcmp(l->cur.text,"long")==0  ||
         strcmp(l->cur.text,"double")==0))
    {
        strncat(full, " ", sizeof(full)-strlen(full)-1);
        strncat(full, l->cur.text, sizeof(full)-strlen(full)-1);
        lex_advance(l);
        /* yet another "long" for "long long" */
        if (l->cur.kind == TOK_IDENT && strcmp(l->cur.text,"long")==0)
        {
            strncat(full, " ", sizeof(full)-strlen(full)-1);
            strncat(full, l->cur.text, sizeof(full)-strlen(full)-1);
            lex_advance(l);
        }
    }

    sz = builtin_type_size(full);
    if (sz < 0) sz = builtin_type_size(base);

    /* If neither matched, not a known type → backtrack */
    if (sz < 0)
    {
        l->pos = saved_pos;
        l->cur = saved_tok;
        return -1;
    }

    /* Optional '*' for pointer */
    if (l->cur.kind == TOK_STAR)
    {
        *out_is_pointer = 1;
        lex_advance(l);
        /* multiple stars */
        while (l->cur.kind == TOK_STAR)
            lex_advance(l);
        sz = 8; /* pointer is always 8 bytes on x64 */
    }

    return sz;
}

/* ------------------------------------------------------------------ */
/* Recursive descent parser                                            */
/* ------------------------------------------------------------------ */

/* primary: literal | identifier | '(' expr ')' | sizeof */
static expr_val_t parse_primary(lex_t *l)
{
    if (l->err) return make_err(l->errmsg);

    /* Number literal */
    if (l->cur.kind == TOK_NUM)
    {
        expr_val_t v = make_val(l->cur.ival);
        lex_advance(l);
        return v;
    }

    /* sizeof */
    if (l->cur.kind == TOK_IDENT && strcmp(l->cur.text, "sizeof") == 0)
    {
        lex_advance(l);
        if (l->cur.kind != TOK_LPAREN)
            return make_err("sizeof: expected '('");
        lex_advance(l);

        int is_ptr = 0;
        int sz = try_parse_typename(l, &is_ptr);
        if (sz >= 0)
        {
            /* sizeof(type) */
            if (l->cur.kind != TOK_RPAREN)
                return make_err("sizeof: expected ')'");
            lex_advance(l);
            return make_val(sz);
        }
        else
        {
            /* sizeof(expr): evaluate to get type info */
            expr_val_t inner = parse_expr(l);
            if (l->cur.kind != TOK_RPAREN)
                return make_err("sizeof: expected ')'");
            lex_advance(l);
            return make_val(inner.byte_size > 0 ? inner.byte_size : 4);
        }
    }

    /* Identifier */
    if (l->cur.kind == TOK_IDENT)
    {
        char name[128];
        strncpy(name, l->cur.text, sizeof(name)-1);
        lex_advance(l);

        DWORD64 addr = 0, modbase = 0;
        DWORD   tid  = 0;
        ULONG   sz   = 0;

        if (!expr_lookup_symbol(l->dbg, name, &addr, &modbase, &tid, &sz))
        {
            char msg[160];
            snprintf(msg, sizeof(msg), "symbol '%s' not found", name);
            return make_err(msg);
        }

        expr_val_t v = {0};
        v.mod_base  = modbase;
        v.type_id   = tid;
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
            load_lvalue(l->dbg, &v);
        }
        return v;
    }

    /* Parenthesised expression or cast */
    if (l->cur.kind == TOK_LPAREN)
    {
        lex_advance(l);

        int is_ptr = 0;
        int sz = try_parse_typename(l, &is_ptr);
        if (sz >= 0)
        {
            /* Cast expression: (type) expr */
            if (l->cur.kind != TOK_RPAREN)
                return make_err("cast: expected ')'");
            lex_advance(l);
            expr_val_t inner = parse_assign_expr(l);
            if (*inner.errmsg) return inner;

            if (is_ptr)
            {
                /* pointer cast: result is the address value, type info cleared */
                long long ptr_val = inner.is_lvalue ? (long long)inner.addr
                                                    : inner.value;
                expr_val_t r = {0};
                r.is_lvalue = 0;
                r.value     = ptr_val;
                r.byte_size = 8;
                /* type_id left 0 so expr_print treats it as plain integer */
                return r;
            }
            else
            {
                /* Truncate to target size */
                if (sz == 1) inner.value = (long long)(signed char)inner.value;
                else if (sz == 2) inner.value = (long long)(short)inner.value;
                else if (sz == 4) inner.value = (long long)(int)inner.value;
                inner.byte_size = (ULONG)sz;
                inner.is_lvalue = 0;
                inner.type_id   = 0;
            }
            return inner;
        }

        /* Regular parenthesised expression */
        expr_val_t v = parse_expr(l);
        if (l->cur.kind != TOK_RPAREN)
            return make_err("expected ')'");
        lex_advance(l);
        return v;
    }

    return make_err("unexpected token in expression");
}

/* postfix: primary ( '[' expr ']' | '.' ident | '->' ident )* */
static expr_val_t parse_postfix(lex_t *l)
{
    expr_val_t v = parse_primary(l);
    if (*v.errmsg) return v;

    for (;;)
    {
        if (l->cur.kind == TOK_LBRACKET)
        {
            /* array index */
            lex_advance(l);
            expr_val_t idx = parse_expr(l);
            if (*idx.errmsg) return idx;
            if (l->cur.kind != TOK_RBRACKET)
                return make_err("expected ']'");
            lex_advance(l);

            /* Determine element type & size */
            DWORD tag = 0;
            SymGetTypeInfo(l->dbg->sym_handle, v.mod_base, v.type_id,
                TI_GET_SYMTAG, &tag);

            DWORD elem_type = 0;
            DWORD64 base_addr = v.addr;

            if (tag == SYM_TAG_POINTER)
            {
                /* pointer[i]: dereference pointer first */
                SymGetTypeInfo(l->dbg->sym_handle, v.mod_base, v.type_id,
                    TI_GET_TYPEID, &elem_type);
                base_addr = (DWORD64)v.value;
            }
            else if (tag == SYM_TAG_ARRAYTYPE)
            {
                SymGetTypeInfo(l->dbg->sym_handle, v.mod_base, v.type_id,
                    TI_GET_TYPEID, &elem_type);
            }
            else
            {
                return make_err("subscript requires array or pointer");
            }

            ULONG64 elem_len = 0;
            SymGetTypeInfo(l->dbg->sym_handle, v.mod_base, elem_type,
                TI_GET_LENGTH, &elem_len);
            if (elem_len == 0) elem_len = 4;

            expr_val_t r = {0};
            r.is_lvalue = 1;
            r.addr      = base_addr + (DWORD64)idx.value * elem_len;
            r.mod_base  = v.mod_base;
            r.type_id   = elem_type;
            r.byte_size = (ULONG)elem_len;
            load_lvalue(l->dbg, &r);
            v = r;
        }
        else if (l->cur.kind == TOK_DOT || l->cur.kind == TOK_ARROW)
        {
            int is_arrow = (l->cur.kind == TOK_ARROW);
            lex_advance(l);
            if (l->cur.kind != TOK_IDENT)
                return make_err("expected member name");
            char member[128];
            strncpy(member, l->cur.text, sizeof(member)-1);
            lex_advance(l);

            DWORD64 base_addr = v.addr;
            DWORD   base_type = v.type_id;

            if (is_arrow)
            {
                /* dereference pointer */
                DWORD tag = 0;
                SymGetTypeInfo(l->dbg->sym_handle, v.mod_base, v.type_id,
                    TI_GET_SYMTAG, &tag);
                if (tag == SYM_TAG_POINTER)
                {
                    DWORD pt = 0;
                    SymGetTypeInfo(l->dbg->sym_handle, v.mod_base, v.type_id,
                        TI_GET_TYPEID, &pt);
                    base_type = pt;
                }
                base_addr = (DWORD64)v.value;
            }

            DWORD mem_type = 0, mem_off = 0;
            if (!lookup_member(l->dbg, v.mod_base, base_type,
                               member, &mem_type, &mem_off))
            {
                char msg[160];
                snprintf(msg, sizeof(msg), "member '%s' not found", member);
                return make_err(msg);
            }

            ULONG64 mem_len = 0;
            SymGetTypeInfo(l->dbg->sym_handle, v.mod_base, mem_type,
                TI_GET_LENGTH, &mem_len);
            if (mem_len == 0) mem_len = 4;

            expr_val_t r = {0};
            r.is_lvalue = 1;
            r.addr      = base_addr + mem_off;
            r.mod_base  = v.mod_base;
            r.type_id   = mem_type;
            r.byte_size = (ULONG)mem_len;
            load_lvalue(l->dbg, &r);
            v = r;
        }
        else
        {
            break;
        }
    }
    return v;
}

/* unary: ('*'|'&'|'-'|'+'|'~'|'!') unary | cast | postfix */
static expr_val_t parse_unary(lex_t *l)
{
    if (l->cur.kind == TOK_STAR)
    {
        lex_advance(l);
        expr_val_t inner = parse_unary(l);
        if (*inner.errmsg) return inner;

        DWORD tag = 0;
        SymGetTypeInfo(l->dbg->sym_handle, inner.mod_base, inner.type_id,
            TI_GET_SYMTAG, &tag);
        if (tag != SYM_TAG_POINTER)
            return make_err("dereference of non-pointer");

        DWORD pt = 0;
        SymGetTypeInfo(l->dbg->sym_handle, inner.mod_base, inner.type_id,
            TI_GET_TYPEID, &pt);

        ULONG64 len = 0;
        SymGetTypeInfo(l->dbg->sym_handle, inner.mod_base, pt,
            TI_GET_LENGTH, &len);
        if (len == 0) len = 4;

        expr_val_t r = {0};
        r.is_lvalue = 1;
        r.addr      = (DWORD64)inner.value;
        r.mod_base  = inner.mod_base;
        r.type_id   = pt;
        r.byte_size = (ULONG)len;
        load_lvalue(l->dbg, &r);
        return r;
    }

    if (l->cur.kind == TOK_AMP)
    {
        lex_advance(l);
        expr_val_t inner = parse_unary(l);
        if (*inner.errmsg) return inner;
        if (!inner.is_lvalue)
            return make_err("& requires lvalue");
        expr_val_t r = make_val((long long)inner.addr);
        r.byte_size = 8;
        return r;
    }

    if (l->cur.kind == TOK_MINUS)
    {
        lex_advance(l);
        expr_val_t inner = parse_unary(l);
        if (*inner.errmsg) return inner;
        return make_val(-inner.value);
    }

    if (l->cur.kind == TOK_PLUS)
    {
        lex_advance(l);
        return parse_unary(l);
    }

    if (l->cur.kind == TOK_TILDE)
    {
        lex_advance(l);
        expr_val_t inner = parse_unary(l);
        if (*inner.errmsg) return inner;
        return make_val(~inner.value);
    }

    if (l->cur.kind == TOK_BANG)
    {
        lex_advance(l);
        expr_val_t inner = parse_unary(l);
        if (*inner.errmsg) return inner;
        return make_val(!inner.value);
    }

    return parse_postfix(l);
}

/* multiplicative: unary (('*'|'/'|'%') unary)* */
static expr_val_t parse_mul(lex_t *l)
{
    expr_val_t v = parse_unary(l);
    while (!*v.errmsg && (l->cur.kind == TOK_STAR ||
                          l->cur.kind == TOK_SLASH ||
                          l->cur.kind == TOK_PERCENT))
    {
        token_kind_t op = l->cur.kind;
        lex_advance(l);
        expr_val_t r = parse_unary(l);
        if (*r.errmsg) return r;
        if (op == TOK_STAR)        v = make_val(v.value * r.value);
        else if (op == TOK_SLASH)  { if (r.value == 0) return make_err("division by zero");
                                     v = make_val(v.value / r.value); }
        else                       { if (r.value == 0) return make_err("division by zero");
                                     v = make_val(v.value % r.value); }
    }
    return v;
}

/* additive: mul (('+' | '-') mul)* */
static expr_val_t parse_add(lex_t *l)
{
    expr_val_t v = parse_mul(l);
    while (!*v.errmsg && (l->cur.kind == TOK_PLUS || l->cur.kind == TOK_MINUS))
    {
        token_kind_t op = l->cur.kind;
        lex_advance(l);
        expr_val_t r = parse_mul(l);
        if (*r.errmsg) return r;
        v = make_val(op == TOK_PLUS ? v.value + r.value : v.value - r.value);
    }
    return v;
}

/* shift: add (('<<' | '>>') add)* */
static expr_val_t parse_shift(lex_t *l)
{
    expr_val_t v = parse_add(l);
    while (!*v.errmsg && (l->cur.kind == TOK_LSHIFT || l->cur.kind == TOK_RSHIFT))
    {
        token_kind_t op = l->cur.kind;
        lex_advance(l);
        expr_val_t r = parse_add(l);
        if (*r.errmsg) return r;
        v = make_val(op == TOK_LSHIFT
            ? (v.value << (r.value & 63))
            : ((unsigned long long)v.value >> (r.value & 63)));
    }
    return v;
}

/* relational: shift (('<'|'>'|'<='|'>=') shift)* */
static expr_val_t parse_rel(lex_t *l)
{
    expr_val_t v = parse_shift(l);
    while (!*v.errmsg && (l->cur.kind == TOK_LT || l->cur.kind == TOK_GT ||
                          l->cur.kind == TOK_LE || l->cur.kind == TOK_GE))
    {
        token_kind_t op = l->cur.kind;
        lex_advance(l);
        expr_val_t r = parse_shift(l);
        if (*r.errmsg) return r;
        long long res = 0;
        if      (op == TOK_LT) res = v.value <  r.value;
        else if (op == TOK_GT) res = v.value >  r.value;
        else if (op == TOK_LE) res = v.value <= r.value;
        else                   res = v.value >= r.value;
        v = make_val(res);
    }
    return v;
}

/* equality: rel (('=='|'!=') rel)* */
static expr_val_t parse_eq(lex_t *l)
{
    expr_val_t v = parse_rel(l);
    while (!*v.errmsg && (l->cur.kind == TOK_EQ || l->cur.kind == TOK_NEQ))
    {
        token_kind_t op = l->cur.kind;
        lex_advance(l);
        expr_val_t r = parse_rel(l);
        if (*r.errmsg) return r;
        v = make_val(op == TOK_EQ ? v.value == r.value : v.value != r.value);
    }
    return v;
}

/* bitand: eq ('&' eq)* */
static expr_val_t parse_bitand(lex_t *l)
{
    expr_val_t v = parse_eq(l);
    while (!*v.errmsg && l->cur.kind == TOK_AMP)
    {
        lex_advance(l);
        expr_val_t r = parse_eq(l);
        if (*r.errmsg) return r;
        v = make_val(v.value & r.value);
    }
    return v;
}

/* bitxor: bitand ('^' bitand)* */
static expr_val_t parse_bitxor(lex_t *l)
{
    expr_val_t v = parse_bitand(l);
    while (!*v.errmsg && l->cur.kind == TOK_CARET)
    {
        lex_advance(l);
        expr_val_t r = parse_bitand(l);
        if (*r.errmsg) return r;
        v = make_val(v.value ^ r.value);
    }
    return v;
}

/* bitor: bitxor ('|' bitxor)* */
static expr_val_t parse_bitor(lex_t *l)
{
    expr_val_t v = parse_bitxor(l);
    while (!*v.errmsg && l->cur.kind == TOK_PIPE)
    {
        lex_advance(l);
        expr_val_t r = parse_bitxor(l);
        if (*r.errmsg) return r;
        v = make_val(v.value | r.value);
    }
    return v;
}

/* logand: bitor ('&&' bitor)* */
static expr_val_t parse_logand(lex_t *l)
{
    expr_val_t v = parse_bitor(l);
    while (!*v.errmsg && l->cur.kind == TOK_AMAMP)
    {
        lex_advance(l);
        expr_val_t r = parse_bitor(l);
        if (*r.errmsg) return r;
        v = make_val(v.value && r.value);
    }
    return v;
}

/* logor: logand ('||' logand)* */
static expr_val_t parse_logor(lex_t *l)
{
    expr_val_t v = parse_logand(l);
    while (!*v.errmsg && l->cur.kind == TOK_PIPEPIPE)
    {
        lex_advance(l);
        expr_val_t r = parse_logand(l);
        if (*r.errmsg) return r;
        v = make_val(v.value || r.value);
    }
    return v;
}

static expr_val_t parse_assign_expr(lex_t *l)
{
    return parse_logor(l);
}

static expr_val_t parse_expr(lex_t *l)
{
    return parse_assign_expr(l);
}

typedef struct {
    char target[256];
    int found;
    char sym_buf[sizeof(SYMBOL_INFO) + 256];
} enum_ctx_t;

static BOOL CALLBACK enum_sym_cb(PSYMBOL_INFO si, ULONG size, PVOID ctx)
{
    enum_ctx_t *c = (enum_ctx_t*)ctx;
    if (strcmp(si->Name, c->target) == 0)
    {
        ULONG copy_size = si->SizeOfStruct + si->MaxNameLen;
        if (copy_size > sizeof(c->sym_buf)) copy_size = sizeof(c->sym_buf);
        memcpy(c->sym_buf, si, copy_size);
        c->found = 1;
        return FALSE;
    }
    return TRUE;
}

static int expr_lookup_symbol(debugger_t *dbg, const char *name,
                              DWORD64 *out_addr, DWORD64 *out_modbase,
                              DWORD *out_typeid, ULONG *out_size)
{
    enum_ctx_t ctx = {0};
    strncpy(ctx.target, name, sizeof(ctx.target) - 1);
    SymEnumSymbols(dbg->sym_handle, 0, "*", enum_sym_cb, &ctx);

    if (!ctx.found)
        return 0;

    PSYMBOL_INFO si = (PSYMBOL_INFO)ctx.sym_buf;

    /* Resolve address */
    CONTEXT tctx = {0};
    tctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &tctx);

    DWORD64 addr = 0;
    if (si->Flags & SYMFLAG_REGREL)
    {
        long long offset = (long long)(LONG64)si->Address;
        if (si->Register == 335) /* CV_AMD64_RSP */
            addr = tctx.Rsp + (DWORD64)offset;
        else if (si->Register == 334) /* CV_AMD64_RBP */
            addr = tctx.Rbp + (DWORD64)offset;
        else
            addr = tctx.Rsp + (DWORD64)offset;
    }
    else if (si->Flags & SYMFLAG_REGISTER)
    {
        /* value lives in a register: read it directly */
        DWORD64 reg_val = 0;
        switch (si->Register)
        {
        case 328: reg_val = tctx.Rax; break; /* CV_AMD64_RAX */
        case 329: reg_val = tctx.Rcx; break; /* CV_AMD64_RCX */
        case 330: reg_val = tctx.Rdx; break; /* CV_AMD64_RDX */
        case 331: reg_val = tctx.Rbx; break; /* CV_AMD64_RBX */
        case 335: reg_val = tctx.Rsp; break; /* CV_AMD64_RSP */
        case 334: reg_val = tctx.Rbp; break; /* CV_AMD64_RBP */
        case 336: reg_val = tctx.Rsi; break; /* CV_AMD64_RSI */
        case 337: reg_val = tctx.Rdi; break; /* CV_AMD64_RDI */
        case 344: reg_val = tctx.R8;  break; /* CV_AMD64_R8  */
        case 345: reg_val = tctx.R9;  break; /* CV_AMD64_R9  */
        case 346: reg_val = tctx.R10; break;
        case 347: reg_val = tctx.R11; break;
        case 348: reg_val = tctx.R12; break;
        case 349: reg_val = tctx.R13; break;
        case 350: reg_val = tctx.R14; break;
        case 351: reg_val = tctx.R15; break;
        /* 32-bit sub-registers */
        case  17: reg_val = tctx.Rax & 0xffffffff; break; /* CV_AMD64_EAX */
        case  18: reg_val = tctx.Rcx & 0xffffffff; break;
        case  19: reg_val = tctx.Rdx & 0xffffffff; break;
        case  20: reg_val = tctx.Rbx & 0xffffffff; break;
        case  21: reg_val = tctx.Rsp & 0xffffffff; break;
        case  22: reg_val = tctx.Rbp & 0xffffffff; break;
        case  23: reg_val = tctx.Rsi & 0xffffffff; break;
        case  24: reg_val = tctx.Rdi & 0xffffffff; break;
        default:  reg_val = 0; break;
        }
        /* out_size=0 signals "register variable, value in out_addr" */
        *out_addr    = reg_val;
        *out_modbase = si->ModBase;
        *out_typeid  = si->TypeIndex;
        *out_size    = 0;  /* sentinel: register variable */
        return 1;
    }
    else
    {
        addr = si->Address;
    }

    *out_addr    = addr;
    *out_modbase = si->ModBase;
    *out_typeid  = si->TypeIndex;
    *out_size    = si->Size;
    return 1;
}

static int lookup_member(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                         const char *member, DWORD *out_typeid, DWORD *out_offset)
{
    DWORD child_count = 0;
    if (!SymGetTypeInfo(dbg->sym_handle, modbase, type_id,
                        TI_GET_CHILDRENCOUNT, &child_count))
        return 0;

    TI_FINDCHILDREN_PARAMS *p =
        (TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS)
                                        + child_count * sizeof(ULONG));
    if (!p) return 0;
    p->Count = child_count;
    p->Start = 0;

    if (!SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_FINDCHILDREN, p))
    { free(p); return 0; }

    for (DWORD i = 0; i < child_count; i++)
    {
        DWORD child_id = p->ChildId[i];
        WCHAR *wname = NULL;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_id,
                       TI_GET_SYMNAME, &wname);
        if (!wname) continue;

        char aname[128] = {0};
        WideCharToMultiByte(CP_ACP, 0, wname, -1, aname, sizeof(aname)-1, NULL, NULL);
        LocalFree(wname);

        if (strcmp(aname, member) == 0)
        {
            DWORD offset = 0;
            SymGetTypeInfo(dbg->sym_handle, modbase, child_id,
                           TI_GET_OFFSET, &offset);
            DWORD child_type = 0;
            SymGetTypeInfo(dbg->sym_handle, modbase, child_id,
                           TI_GET_TYPEID, &child_type);
            *out_typeid = child_type;
            *out_offset = offset;
            free(p);
            return 1;
        }
    }
    free(p);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

eval_status_t expr_eval(debugger_t *dbg, const char *expr, expr_val_t *out)
{
    if (!dbg->thread || !dbg->process)
    {
        strncpy(out->errmsg, "no active process", sizeof(out->errmsg)-1);
        return EVAL_ERR;
    }

    /* Set symbol context */
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);
    IMAGEHLP_STACK_FRAME frm = {0};
    frm.InstructionOffset = ctx.Rip;
    frm.FrameOffset       = ctx.Rsp;
    frm.StackOffset       = ctx.Rsp;
    SymSetContext(dbg->sym_handle, &frm, NULL);

    lex_t l = {0};
    l.src = expr;
    l.dbg = dbg;
    lex_advance(&l);

    *out = parse_expr(&l);

    if (l.err || *out->errmsg)
    {
        if (!*out->errmsg)
            strncpy(out->errmsg, l.errmsg, sizeof(out->errmsg)-1);
        return EVAL_ERR;
    }
    return EVAL_OK;
}

/* ------------------------------------------------------------------ */
/* Print helpers (same style as original print_variable)              */
/* ------------------------------------------------------------------ */
static void print_struct_ex(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                             DWORD64 base_addr, int depth);

static void print_val(debugger_t *dbg, const char *label,
                      DWORD64 modbase, DWORD type_id, ULONG byte_size,
                      DWORD64 addr, long long value, int depth)
{
    DWORD tag = 0;
    SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_SYMTAG, &tag);

    char indent[32] = {0};
    for (int i = 0; i < depth && i < 15; i++) indent[i] = ' ', indent[i+1] = ' ';

    if (tag == SYM_TAG_UDT)
    {
        printf("%s%s = {\n", indent, label);
        print_struct_ex(dbg, modbase, type_id, addr, depth + 1);
        printf("%s}\n", indent);
        return;
    }
    if (tag == SYM_TAG_POINTER)
    {
        printf("%s%s = 0x%llx\n", indent, label, (unsigned long long)value);
        return;
    }
    if (byte_size <= 4)
        printf("%s%s = %d (0x%x)\n", indent, label,
               (int)value, (unsigned int)(unsigned long long)value);
    else
        printf("%s%s = %lld (0x%llx)\n", indent, label,
               value, (unsigned long long)value);
}

static void print_struct_ex(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                             DWORD64 base_addr, int depth)
{
    if (depth > 5) return;

    DWORD child_count = 0;
    if (!SymGetTypeInfo(dbg->sym_handle, modbase, type_id,
                        TI_GET_CHILDRENCOUNT, &child_count)) return;

    TI_FINDCHILDREN_PARAMS *p =
        (TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS)
                                        + child_count * sizeof(ULONG));
    if (!p) return;
    p->Count = child_count;
    p->Start = 0;
    if (!SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_FINDCHILDREN, p))
    { free(p); return; }

    for (DWORD i = 0; i < child_count; i++)
    {
        DWORD child_id = p->ChildId[i];
        WCHAR *wname = NULL;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_id, TI_GET_SYMNAME, &wname);
        if (!wname) continue;
        char aname[128] = {0};
        WideCharToMultiByte(CP_ACP, 0, wname, -1, aname, sizeof(aname)-1, NULL, NULL);
        LocalFree(wname);

        DWORD offset = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_id, TI_GET_OFFSET, &offset);
        DWORD child_type = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_id, TI_GET_TYPEID, &child_type);
        ULONG64 child_len = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_type, TI_GET_LENGTH, &child_len);
        if (child_len == 0) child_len = 4;

        DWORD64 member_addr = base_addr + offset;
        unsigned long long val = 0;
        SIZE_T n;
        ReadProcessMemory(dbg->process, (void*)member_addr, &val, (SIZE_T)child_len, &n);

        print_val(dbg, aname, modbase, child_type, (ULONG)child_len,
                  member_addr, (long long)val, depth);
    }
    free(p);
}

/* Format a single integer value according to fmt */
static void print_int_fmt(const char *label, long long value,
                          ULONG byte_size, print_fmt_t fmt)
{
    unsigned long long uv = (unsigned long long)value;
    switch (fmt)
    {
    case FMT_HEX:
        printf("%s = 0x%llx\n", label, uv);
        break;
    case FMT_OCT:
        printf("%s = 0%llo\n", label, uv);
        break;
    case FMT_BIN:
    {
        int bits = (byte_size > 0 && byte_size <= 8) ? (int)byte_size * 8 : 64;
        printf("%s = 0b", label);
        for (int b = bits - 1; b >= 0; b--)
            putchar((uv >> b) & 1 ? '1' : '0');
        printf("\n");
        break;
    }
    case FMT_CHAR:
        printf("%s = '%c' (%lld)\n", label, (int)(value & 0xff), value);
        break;
    case FMT_DEC:
        if (byte_size <= 4)
            printf("%s = %d\n", label, (int)value);
        else
            printf("%s = %lld\n", label, value);
        break;
    default: /* FMT_DEFAULT */
        if (byte_size <= 4)
            printf("%s = %d (0x%x)\n", label,
                   (int)value, (unsigned int)uv);
        else
            printf("%s = %lld (0x%llx)\n", label, value, uv);
        break;
    }
}

/* Read a null-terminated string from debuggee memory */
static void print_string_fmt(debugger_t *dbg, const char *label, DWORD64 addr)
{
    if (!addr) { printf("%s = (null)\n", label); return; }
    char buf[256] = {0};
    SIZE_T n = 0;
    ReadProcessMemory(dbg->process, (void*)addr, buf, sizeof(buf)-1, &n);
    buf[sizeof(buf)-1] = '\0';
    printf("%s = \"%s\"\n", label, buf);
}

void expr_print_fmt(debugger_t *dbg, const char *expr_str, print_fmt_t fmt)
{
    expr_val_t v = {0};
    if (expr_eval(dbg, expr_str, &v) != EVAL_OK)
    {
        printf("error: %s\n", v.errmsg);
        return;
    }

    /* /s: treat value as pointer to string */
    if (fmt == FMT_STR)
    {
        print_string_fmt(dbg, expr_str, (DWORD64)v.value);
        return;
    }

    DWORD tag = 0;
    if (v.mod_base && v.type_id)
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id, TI_GET_SYMTAG, &tag);

    /* Array: only default format shows struct layout; others flatten to elements */
    if (tag == SYM_TAG_ARRAYTYPE && fmt == FMT_DEFAULT)
    {
        DWORD elem_type = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id,
            TI_GET_TYPEID, &elem_type);
        ULONG64 total = 0, elem_len = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id,
            TI_GET_LENGTH, &total);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type,
            TI_GET_LENGTH, &elem_len);
        if (elem_len == 0) elem_len = 4;
        DWORD count = (DWORD)(total / elem_len);

        DWORD elem_tag = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type,
            TI_GET_SYMTAG, &elem_tag);

        printf("%s = {\n", expr_str);
        for (DWORD i = 0; i < count; i++)
        {
            DWORD64 ea = v.addr + i * elem_len;
            char idx[32];
            snprintf(idx, sizeof(idx), "[%u]", i);
            if (elem_tag == SYM_TAG_UDT)
            {
                printf("  %s = {\n", idx);
                print_struct_ex(dbg, v.mod_base, elem_type, ea, 2);
                printf("  }\n");
            }
            else
            {
                unsigned long long val = 0; SIZE_T n;
                ReadProcessMemory(dbg->process, (void*)ea, &val, (SIZE_T)elem_len, &n);
                if (elem_len <= 4)
                    printf("  [%u] = %d (0x%x)\n", i, (int)val, (unsigned int)val);
                else
                    printf("  [%u] = %lld (0x%llx)\n", i, (long long)val, val);
            }
        }
        printf("}\n");
        return;
    }

    /* Array with explicit format: print each element */
    if (tag == SYM_TAG_ARRAYTYPE)
    {
        DWORD elem_type = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id, TI_GET_TYPEID, &elem_type);
        ULONG64 total = 0, elem_len = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id, TI_GET_LENGTH, &total);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type, TI_GET_LENGTH, &elem_len);
        if (elem_len == 0) elem_len = 4;
        DWORD count = (DWORD)(total / elem_len);
        printf("%s = {\n", expr_str);
        for (DWORD i = 0; i < count; i++)
        {
            DWORD64 ea = v.addr + i * elem_len;
            unsigned long long val = 0; SIZE_T n;
            ReadProcessMemory(dbg->process, (void*)ea, &val, (SIZE_T)elem_len, &n);
            char idx[32]; snprintf(idx, sizeof(idx), "  [%u]", i);
            print_int_fmt(idx, (long long)val, (ULONG)elem_len, fmt);
        }
        printf("}\n");
        return;
    }

    /* UDT: only default shows struct; explicit fmt prints address */
    if (tag == SYM_TAG_UDT && fmt == FMT_DEFAULT)
    {
        printf("%s = {\n", expr_str);
        print_struct_ex(dbg, v.mod_base, v.type_id, v.addr, 1);
        printf("}\n");
        return;
    }

    print_int_fmt(expr_str, v.value, v.byte_size, fmt);
}

void expr_print(debugger_t *dbg, const char *expr_str)
{
    expr_print_fmt(dbg, expr_str, FMT_DEFAULT);
}

int expr_assign(debugger_t *dbg, const char *lhs, long long value)
{
    expr_val_t v = {0};
    if (expr_eval(dbg, lhs, &v) != EVAL_OK)
    {
        printf("error: %s\n", v.errmsg);
        return -1;
    }
    if (!v.is_lvalue)
    {
        printf("error: '%s' is not assignable\n", lhs);
        return -1;
    }

    ULONG sz = v.byte_size ? v.byte_size : 4;
    if (sz > 8) sz = 8;

    unsigned long long write_val =
        (sz <= 4) ? (unsigned long long)(long)value
                  : (unsigned long long)value;

    SIZE_T n;
    if (!WriteProcessMemory(dbg->process, (void*)v.addr, &write_val, sz, &n))
    {
        printf("WriteProcessMemory failed for '%s'\n", lhs);
        return -1;
    }
    printf("%s = %lld\n", lhs, value);
    return 0;
}
