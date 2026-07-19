/*
 * Bison grammar for the debugger's interactive command line. Recognizes
 * command syntax (keyword + optional single-word or rest-of-line
 * argument) and dispatches to the do_xxx() handlers in commands.c, which
 * carry the original argument-interpretation logic (colon/0x/symbol
 * resolution, flag parsing, etc.) unchanged.
 */

%code requires {
    #include "debugger.h"
    #include "cmdline_internal.h"

    typedef struct { char s[512]; } ctext_t;
}

%code {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdline_internal.h"

void cparseerror(debugger_t *dbg, cmd_action_t *action, const char *s);
int  cparselex(void);
}

%parse-param { debugger_t *dbg }
%parse-param { cmd_action_t *action }
%define api.prefix {cparse}
%define parse.error verbose

%union {
    ctext_t text;
}

%token <text> ARGWORD
%token <text> STRING
%token KW_BREAK KW_DEL KW_WDEL KW_SYMS
%token KW_REGS KW_SI KW_CONTINUE KW_STEP KW_N KW_UP KW_TB KW_RUN KW_KILL
%token KW_HELP KW_QUIT
%token KW_X KW_DIS KW_LIST KW_LINES KW_LEAK KW_SHOW KW_WATCH KW_PRINT KW_SET
%token KW_SETPRINTPRETTY KW_THREAD

%%

line:
      /* empty */                { }
    | KW_BREAK ARGWORD              { do_break(dbg, $2.s); }
    | KW_BREAK                   { printf("usage: break <addr|symbol|file:line>\n"); }
    | KW_DEL ARGWORD                { do_del(dbg, $2.s); }
    | KW_DEL                     { printf("usage: del <addr|symbol|file:line>\n"); }
    | KW_WDEL ARGWORD               { do_wdel(dbg, $2.s); }
    | KW_WDEL                    { printf("usage: wdel <addr|symbol>\n"); }
    | KW_SYMS ARGWORD               { do_syms(dbg, $2.s); }
    | KW_SYMS                    { printf("usage: syms <name>\n"); }
    | KW_REGS                    { do_regs(dbg); }
    | KW_SI                      { do_si(dbg, action); }
    | KW_CONTINUE                { do_continue(dbg, action); }
    | KW_STEP                    { do_step(dbg, action); }
    | KW_N                       { do_next(dbg, action); }
    | KW_UP                      { do_up(dbg, action); }
    | KW_RUN                     { do_run(dbg, action); }
    | KW_KILL                    { do_kill(dbg, action); }
    | KW_TB                      { do_tb(dbg); }
    | KW_HELP STRING             { do_help($2.s); }
    | KW_QUIT                    { exit(0); }
    | KW_X STRING                { do_x(dbg, $2.s); }
    | KW_DIS STRING              { do_dis(dbg, $2.s); }
    | KW_LIST STRING             { do_list(dbg, $2.s); }
    | KW_LINES STRING            { do_lines(dbg, $2.s); }
    | KW_LEAK STRING             { do_leak(dbg, $2.s); }
    | KW_SHOW STRING             { do_show(dbg, $2.s); }
    | KW_WATCH STRING            { do_watch(dbg, $2.s); }
    | KW_PRINT STRING            { do_print(dbg, $2.s); }
    | KW_SET STRING              { do_set(dbg, $2.s); }
    | KW_SETPRINTPRETTY STRING   { do_set_print_pretty(dbg, $2.s); }
    | KW_THREAD ARGWORD          { do_thread(dbg, $2.s); }
    | KW_THREAD                  { printf("usage: thread <id>\n"); }
    | ARGWORD                       { do_unknown($1.s); }
    ;

%%

void cparseerror(debugger_t *dbg, cmd_action_t *action, const char *s)
{
    (void)dbg;
    (void)action;
    (void)s; /* malformed lines are reported by the individual do_xxx()
                usage messages / the ARGWORD fallback; a raw bison syntax
                error is not shown to the user */
}

#ifndef YY_NO_UNISTD_H
#define YY_NO_UNISTD_H 1
#endif
#include "cmdline_lexer.h"

void cparse_reset_state(void);

void cmd_dispatch(debugger_t *dbg, const char *line, cmd_action_t *action)
{
    *action = CMD_STAY;

    /* Guarantee a trailing newline so BOL/EOL lexer rules always see a
     * terminator, even if the caller's buffer doesn't have one. */
    char buf[600];
    size_t len = strlen(line);
    if (len >= sizeof(buf) - 2) len = sizeof(buf) - 2;
    memcpy(buf, line, len);
    if (len == 0 || buf[len - 1] != '\n') buf[len++] = '\n';
    buf[len] = '\0';

    cparse_reset_state();
    YY_BUFFER_STATE b = cparse_scan_string(buf);
    cparseparse(dbg, action);
    cparse_delete_buffer(b);
}
