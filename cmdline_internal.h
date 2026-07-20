#pragma once
#include "debugger.h"

/*
 * Shared between commands.c (command_loop + do_xxx handlers) and the
 * generated bison/flex command-line parser (cmdline_parser.y /
 * cmdline_lexer.l). The grammar only recognizes command syntax (keyword +
 * flags + a raw argument string); the actual argument interpretation
 * (colon/0x/symbol resolution, flag parsing, etc.) lives in these
 * handlers, unchanged from the original command_loop implementation.
 */

typedef enum {
    CMD_STAY = 0,   /* keep reading commands */
    CMD_RETURN      /* return from command_loop (resume the debuggee) */
} cmd_action_t;

/* Parses and dispatches one command line. `action` receives CMD_RETURN if
 * command_loop should return to let the debuggee run. Implemented by the
 * generated parser (cmdline_parser.y). */
void cmd_dispatch(debugger_t *dbg, const char *line, cmd_action_t *action);

/* Shared address-spec resolver: "file:line" | "0xHEX" | symbol (optionally
 * skipping the function prologue for breakpoint-style callers). */
void *resolve_addr_spec(debugger_t *dbg, const char *arg, int skip_prologue);

/* Command handlers (bodies ported unchanged from the previous if/else
 * command_loop; see commands.c). */
void do_break(debugger_t *dbg, const char *arg);
void do_del(debugger_t *dbg, const char *arg);
void do_wdel(debugger_t *dbg, const char *arg);
void do_syms(debugger_t *dbg, const char *arg);
void do_regs(debugger_t *dbg);
void do_si(debugger_t *dbg, cmd_action_t *action);
void do_continue(debugger_t *dbg, cmd_action_t *action);
void do_step(debugger_t *dbg, cmd_action_t *action);
void do_next(debugger_t *dbg, cmd_action_t *action);
void do_up(debugger_t *dbg, cmd_action_t *action);
void do_run(debugger_t *dbg, cmd_action_t *action);
void do_kill(debugger_t *dbg, cmd_action_t *action);
void do_tb(debugger_t *dbg);
void do_help(const char *arg);
void do_x(debugger_t *dbg, const char *arg);
void do_dis(debugger_t *dbg, const char *arg);
void do_list(debugger_t *dbg, const char *arg);
void do_lines(debugger_t *dbg, const char *arg);
void do_leak(debugger_t *dbg, const char *arg);
void do_show(debugger_t *dbg, const char *arg);
void do_thread(debugger_t *dbg, const char *arg);
void do_process(debugger_t *dbg, const char *arg);
void do_watch(debugger_t *dbg, const char *arg);
void do_print(debugger_t *dbg, const char *arg);
void do_set(debugger_t *dbg, const char *arg);
void do_set_print_pretty(debugger_t *dbg, const char *arg);
void do_unknown(const char *word);
