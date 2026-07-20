%option prefix="cparse"
%option noyywrap nounput noinput nounistd
%option header-file="cmdline_lexer.h"

%x SKIPREST
%x RAWLINE

BOUND [^A-Za-z0-9_]

%{
#define YY_NO_UNISTD_H 1
#include <stdio.h>
#include <string.h>
#include "cmdline_internal.h"
#include "cmdline_parser.tab.h"

static void capture(char *dst, size_t dstsz, const char *src)
{
    while (*src == ' ' || *src == '\t') src++;
    char tmp[512];
    strncpy_s(tmp, sizeof(tmp), src, _TRUNCATE);
    size_t n = strlen(tmp);
    while (n > 0 && (tmp[n - 1] == ' ' || tmp[n - 1] == '\t' ||
                     tmp[n - 1] == '\r' || tmp[n - 1] == '\n')) tmp[--n] = '\0';
    strncpy_s(dst, dstsz, tmp, _TRUNCATE);
}
%}

%%

<INITIAL>{
^"set"[ \t]+"print"[ \t]+"pretty"/{BOUND}  { BEGIN(RAWLINE); return KW_SETPRINTPRETTY; }
^"break"/{BOUND}     { return KW_BREAK; }
^"b"[ \t]            { return KW_BREAK; }
^"del"/{BOUND}       { return KW_DEL; }
^"wdel"/{BOUND}      { return KW_WDEL; }
^"syms"/{BOUND}      { return KW_SYMS; }
^"regs"/{BOUND}      { BEGIN(SKIPREST); return KW_REGS; }
^"si"/{BOUND}        { BEGIN(SKIPREST); return KW_SI; }
^"continue"/{BOUND}  { BEGIN(SKIPREST); return KW_CONTINUE; }
^"c"/\r?\n           { BEGIN(SKIPREST); return KW_CONTINUE; }
^"step"/{BOUND}      { BEGIN(SKIPREST); return KW_STEP; }
^"s"/\r?\n           { BEGIN(SKIPREST); return KW_STEP; }
^"n"/\r?\n           { BEGIN(SKIPREST); return KW_N; }
^"up"/{BOUND}        { BEGIN(SKIPREST); return KW_UP; }
^"tb"/{BOUND}        { BEGIN(SKIPREST); return KW_TB; }
^"run"/{BOUND}       { BEGIN(SKIPREST); return KW_RUN; }
^"kill"/{BOUND}      { BEGIN(SKIPREST); return KW_KILL; }
^"help"/{BOUND}      { BEGIN(RAWLINE); return KW_HELP; }
^"h"/{BOUND}         { BEGIN(RAWLINE); return KW_HELP; }
^"quit"/{BOUND}      { BEGIN(SKIPREST); return KW_QUIT; }
^"x"/{BOUND}         { BEGIN(RAWLINE); return KW_X; }
^"dis"/{BOUND}       { BEGIN(RAWLINE); return KW_DIS; }
^"list"/{BOUND}      { BEGIN(RAWLINE); return KW_LIST; }
^"l"/{BOUND}         { BEGIN(RAWLINE); return KW_LIST; }
^"lines"/{BOUND}     { BEGIN(RAWLINE); return KW_LINES; }
^"leak"/{BOUND}      { BEGIN(RAWLINE); return KW_LEAK; }
^"show"/{BOUND}      { BEGIN(RAWLINE); return KW_SHOW; }
^"watch""/"[^ \t\r\n]* { yyless(5); BEGIN(RAWLINE); return KW_WATCH; }
^"watch"/{BOUND}     { BEGIN(RAWLINE); return KW_WATCH; }
^"print""/"[^ \t\r\n]* { yyless(5); BEGIN(RAWLINE); return KW_PRINT; }
^"print"/{BOUND}     { BEGIN(RAWLINE); return KW_PRINT; }
^"p""/"[^ \t\r\n]*   { yyless(1); BEGIN(RAWLINE); return KW_PRINT; }
^"p"/{BOUND}         { BEGIN(RAWLINE); return KW_PRINT; }
^"set"/{BOUND}       { BEGIN(RAWLINE); return KW_SET; }
^"thread"/{BOUND}    { return KW_THREAD; }
^"process"/{BOUND}   { return KW_PROCESS; }
}

<INITIAL>[^ \t\r\n]+ {
    strncpy_s(cparselval.text.s, sizeof(cparselval.text.s), cparsetext, _TRUNCATE);
    BEGIN(SKIPREST);
    return ARGWORD;
}

<INITIAL>[ \t]+   { /* skip */ }
<INITIAL>\r?\n    { /* blank line */ }

<RAWLINE>[^\n]*\n? {
    capture(cparselval.text.s, sizeof(cparselval.text.s), cparsetext);
    BEGIN(SKIPREST);
    return STRING;
}

<SKIPREST>[^\n]*\n? { BEGIN(INITIAL); }

%%

/* cmd_dispatch() calls this before scanning each line: flex's start
 * condition is a global that outlives yy_scan_string() buffer switches,
 * so a line that ends mid-state (e.g. on a parse error) would otherwise
 * corrupt the lexing of every subsequent command. */
void cparse_reset_state(void)
{
    BEGIN(INITIAL);
}
