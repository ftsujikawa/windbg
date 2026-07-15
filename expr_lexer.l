%option prefix="eparse"
%option noyywrap nounput noinput nounistd
%option header-file="expr_lexer.h"

%{
#define YY_NO_UNISTD_H 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expr_internal.h"
#include "expr_parser.tab.h"

static void set_lex_error(const char *msg)
{
    if (!expr_parse_err)
    {
        expr_parse_err = 1;
        strncpy_s(expr_parse_errmsg, sizeof(expr_parse_errmsg), msg, _TRUNCATE);
    }
}

/* Reserved builtin type-name spellings recognized for cast / sizeof(type);
 * matches builtin_type_size()'s vocabulary in expr.c. */
static int is_typeword(const char *s)
{
    static const char *words[] = {
        "char", "short", "int", "long", "float", "double", "void",
        "unsigned", "signed", "size_t", "ptrdiff_t",
        "DWORD", "DWORD64", "BYTE", "WORD", "QWORD", NULL
    };
    for (int i = 0; words[i]; i++)
        if (strcmp(s, words[i]) == 0) return 1;
    return 0;
}
%}

DIGIT   [0-9]
HEXD    [0-9a-fA-F]
IDSTART [A-Za-z_]
IDCONT  [A-Za-z0-9_]

%%

[ \t]+          { /* skip whitespace */ }

{DIGIT}+"."{DIGIT}+ {
                    eparselval.val = make_val((long long)strtod(eparsetext, NULL));
                    eparselval.val.fvalue   = strtod(eparsetext, NULL);
                    eparselval.val.is_float = 1;
                    return NUM;
                }

0[xX]{HEXD}+   {
                    eparselval.val = make_val(strtoll(eparsetext, NULL, 16));
                    return NUM;
                }

0[0-7]*        {
                    eparselval.val = make_val(strtoll(eparsetext, NULL, 8));
                    return NUM;
                }

[1-9]{DIGIT}*  {
                    eparselval.val = make_val(strtoll(eparsetext, NULL, 10));
                    return NUM;
                }

'(\\.|[^'\\])' {
                    char ch;
                    if (eparsetext[1] == '\\')
                    {
                        switch (eparsetext[2])
                        {
                        case 'n':  ch = '\n'; break;
                        case 't':  ch = '\t'; break;
                        case 'r':  ch = '\r'; break;
                        case '0':  ch = '\0'; break;
                        case '\'': ch = '\''; break;
                        case '\\': ch = '\\'; break;
                        case 'a':  ch = '\a'; break;
                        case 'b':  ch = '\b'; break;
                        case 'f':  ch = '\f'; break;
                        case 'v':  ch = '\v'; break;
                        default:   ch = eparsetext[2]; break;
                        }
                    }
                    else
                    {
                        ch = eparsetext[1];
                    }
                    eparselval.val = make_val((unsigned char)ch);
                    return NUM;
                }

"sizeof"        { return SIZEOF; }

{IDSTART}{IDCONT}* {
                    if (is_typeword(eparsetext))
                    {
                        strncpy_s(eparselval.text.s, sizeof(eparselval.text.s), eparsetext, _TRUNCATE);
                        return TYPEWORD;
                    }
                    strncpy_s(eparselval.text.s, sizeof(eparselval.text.s), eparsetext, _TRUNCATE);
                    return IDENT;
                }

"->"            { return ARROW; }
"<<"            { return LSHIFT; }
">>"            { return RSHIFT; }
"&&"            { return ANDAND; }
"||"            { return OROR; }
"=="            { return EQ; }
"!="            { return NEQ; }
"<="            { return LE; }
">="            { return GE; }

[-+*/%&|^~!()\[\].<>,] { return eparsetext[0]; }

.               {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "unexpected char '%c'", eparsetext[0]);
                    set_lex_error(msg);
                    return eparsetext[0];
                }

%%
