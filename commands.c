#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "commands.h"
#include "cmdline_internal.h"
#include "breakpoints.h"
#include "registers.h"
#include "memory.h"
#include "symbols.h"
#include "expr.h"
#include "leakcheck.h"
#include "watchpoints.h"

void command_loop(debugger_t *dbg)
{
    char line[256];

    for (;;)
    {
        printf("(tdb) ");
        fflush(stdout);

        fgets(line, sizeof(line), stdin);

        cmd_action_t action = CMD_STAY;
        cmd_dispatch(dbg, line, &action);
        if (action == CMD_RETURN)
            return;
    }
}

/* Shared by break/del: "file:line" | "0xHEX" | symbol (optionally skipping
 * the function prologue for breakpoint-style callers). */
void *resolve_addr_spec(debugger_t *dbg, const char *arg_in, int skip_prologue)
{
    char arg[128];
    strncpy_s(arg, sizeof(arg), arg_in, _TRUNCATE);

    void *addr = NULL;
    char *colon = strchr(arg, ':');

    if (colon != NULL)
    {
        *colon = '\0';
        DWORD line_num = (DWORD)atoi(colon + 1);
        addr = lookup_source_line_addr(dbg, arg, line_num);
    }
    else if (strncmp(arg, "0x", 2) == 0)
    {
        unsigned long long x;
        sscanf_s(arg, "%llx", &x);
        addr = (void*)x;
    }
    else
    {
        addr = lookup_symbol(dbg, arg);
        if (addr && skip_prologue)
            addr = skip_function_prologue(dbg, addr);
    }
    return addr;
}

void do_break(debugger_t *dbg, const char *arg)
{
    void *addr = resolve_addr_spec(dbg, arg, 1);
    if (addr)
        set_breakpoint(dbg, addr);
}

void do_del(debugger_t *dbg, const char *arg)
{
    void *addr = resolve_addr_spec(dbg, arg, 1);
    if (addr)
    {
        if (remove_breakpoint_at(dbg, addr) == 0)
            printf("breakpoint removed at %p\n", addr);
        else
            printf("no breakpoint at %p\n", addr);
    }
    else
    {
        printf("could not resolve '%s'\n", arg);
    }
}

void do_wdel(debugger_t *dbg, const char *arg)
{
    void *waddr = NULL;

    if (strncmp(arg, "0x", 2) == 0 || strncmp(arg, "0X", 2) == 0)
    {
        unsigned long long x = 0;
        sscanf_s(arg, "%llx", &x);
        waddr = (void*)x;
    }
    else
    {
        waddr = lookup_symbol(dbg, arg);
    }

    if (!waddr)
        printf("wdel: cannot resolve address for '%s'\n", arg);
    else
        remove_watchpoint(dbg, waddr);
}

void do_syms(debugger_t *dbg, const char *arg)
{
    print_symbol_info(dbg, arg);
}

void do_regs(debugger_t *dbg)
{
    show_registers(dbg);
}

void do_si(debugger_t *dbg, cmd_action_t *action)
{
    dbg->si_requested = 1;
    single_step(dbg);
    *action = CMD_RETURN;
}

void do_continue(debugger_t *dbg, cmd_action_t *action)
{
    (void)dbg;
    *action = CMD_RETURN;
}

void do_step(debugger_t *dbg, cmd_action_t *action)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    DWORD cur_line = get_source_line_number(dbg, ctx.Rip);

    if (cur_line == 0)
    {
        printf("(no source info, using si)\n");
        single_step(dbg);
        *action = CMD_RETURN;
        return;
    }

    dbg->step_mode = 1;
    dbg->step_line = cur_line;

    single_step(dbg);
    *action = CMD_RETURN;
}

void do_next(debugger_t *dbg, cmd_action_t *action)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    DWORD cur_line = get_source_line_number(dbg, ctx.Rip);

    if (cur_line == 0)
    {
        printf("(no source info, using step)\n");
        single_step(dbg);
        *action = CMD_RETURN;
        return;
    }

    dbg->next_mode = 1;
    dbg->next_line = cur_line;
    dbg->next_rsp  = ctx.Rsp;

    single_step(dbg);
    *action = CMD_RETURN;
}

void do_up(debugger_t *dbg, cmd_action_t *action)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    DWORD64 ret_addr = 0;
    SIZE_T n;
    ReadProcessMemory(dbg->process, (void*)ctx.Rsp, &ret_addr, sizeof(ret_addr), &n);

    printf("return addr = 0x%llx\n", ret_addr);

    set_temp_breakpoint(dbg, (void*)ret_addr);
    *action = CMD_RETURN;
}

void do_run(debugger_t *dbg, cmd_action_t *action)
{
    if (dbg->target_program[0] == '\0')
    {
        printf("no target program\n");
    }
    else
    {
        printf("restarting %s\n", dbg->target_program);
        debugger_restart(dbg);
        *action = CMD_RETURN;
    }
}

void do_kill(debugger_t *dbg, cmd_action_t *action)
{
    if (dbg->process)
    {
        TerminateProcess(dbg->process, 1);
        printf("killed process\n");
    }
    else
    {
        printf("no process to kill\n");
    }
    *action = CMD_RETURN;
}

void do_tb(debugger_t *dbg)
{
    print_backtrace(dbg);
}

typedef struct {
    const char *names[3];  /* primary name + aliases, NULL-terminated */
    const char *usage;
    const char *detail;
} help_entry_t;

static const help_entry_t help_table[] = {
    { {"break", "b", NULL}, "break <addr|symbol|file:line>",
      "Sets a software breakpoint (INT3) at the given location.\n"
      "  <addr>       hex address, e.g. 0x7ff6...\n"
      "  <symbol>     function or label name; its prologue is skipped\n"
      "               automatically so the breakpoint lands after the\n"
      "               stack frame has been set up\n"
      "  <file:line>  source file and line number, e.g. main.c:42\n"
      "Examples: break add        b main.c:23" },

    { {"del", NULL, NULL}, "del <addr|symbol|file:line>",
      "Removes the breakpoint previously set at <addr|symbol|file:line>.\n"
      "The location is resolved the same way as for 'break'." },

    { {"wdel", NULL, NULL}, "wdel <addr|symbol>",
      "Removes the hardware watchpoint previously set at <addr|symbol>." },

    { {"syms", NULL, NULL}, "syms <name>",
      "Looks up <name> in the debug symbols and prints its address,\n"
      "size and type information." },

    { {"regs", NULL, NULL}, "regs",
      "Prints the current values of the general-purpose registers\n"
      "(RAX, RBX, ... RIP, RFLAGS) for the stopped thread." },

    { {"si", NULL, NULL}, "si",
      "Executes exactly one machine instruction (single-step at the\n"
      "instruction level), then returns to the command prompt." },

    { {"continue", "c", NULL}, "continue",
      "Resumes the debuggee until the next breakpoint, watchpoint,\n"
      "or program exit." },

    { {"step", "s", NULL}, "step",
      "Executes until the next source line is reached, stepping into\n"
      "any function calls made along the way. Falls back to single\n"
      "instruction stepping ('si') if the current location has no\n"
      "source-line information." },

    { {"n", NULL, NULL}, "n",
      "Executes until the next source line is reached without stepping\n"
      "into called functions (step over). Falls back to 'si' if the\n"
      "current location has no source-line information." },

    { {"up", NULL, NULL}, "up",
      "Runs until the current function returns, by placing a temporary\n"
      "breakpoint at the return address found on the stack." },

    { {"run", NULL, NULL}, "run",
      "Terminates and restarts the target program from the beginning\n"
      "(only available when a target program was given on the command\n"
      "line)." },

    { {"kill", NULL, NULL}, "kill",
      "Forcibly terminates the debuggee process without restarting it." },

    { {"list", "l", NULL}, "list [line|file:line]",
      "Shows 10 lines of source code around <line> or <file:line>.\n"
      "With no argument, continues listing from where the previous\n"
      "'list' left off, or centers on the current instruction pointer\n"
      "if 'list' has not been used yet." },

    { {"dis", NULL, NULL}, "dis [addr|symbol|file:line]",
      "Disassembles 10 instructions starting at <addr|symbol|file:line>,\n"
      "or at the current instruction pointer if no argument is given." },

    { {"lines", NULL, NULL}, "lines [filter]",
      "Prints the line-number <-> address mapping for known source\n"
      "files. The optional [filter] restricts the listing to file\n"
      "names containing that substring." },

    { {"tb", NULL, NULL}, "tb",
      "Prints a stack backtrace (return addresses) for the current\n"
      "thread." },

    { {"print", "p", NULL}, "print [/fmt] <expr>",
      "Evaluates <expr> -- a C-like expression (literals, variables,\n"
      "'.'/'->' member access, '[]' indexing, '*'/'&', casts, sizeof,\n"
      "and the usual arithmetic/bitwise/logical/relational operators)\n"
      "-- and prints its value.\n"
      "Optional /fmt selects the display format:\n"
      "  /d /i  signed decimal     /x /X  hexadecimal\n"
      "  /o     octal              /t     binary\n"
      "  /c     character          /s     null-terminated string\n"
      "Examples: p x     p/x arr[2]     p (int)ptr->field     p sizeof(int)" },

    { {"set", NULL, NULL}, "set <lhs> = <expr>",
      "Two forms:\n"
      "  set <lhs> = <expr>          assigns <expr> to a register, or to\n"
      "                              a debuggee variable/lvalue if <lhs>\n"
      "                              is not a register name\n"
      "  set print pretty [on|off]   toggles multi-line struct/array\n"
      "                              formatting for 'print' (shows the\n"
      "                              current state if [on|off] is omitted)" },

    { {"show", NULL, NULL}, "show [locals|args|globals|bp|leaks|wp]",
      "Displays variables or internal state depending on the argument:\n"
      "  locals | args | globals   variables in the current scope\n"
      "  bp                        active breakpoints\n"
      "  wp                        active hardware watchpoints\n"
      "  leaks                     tracked, still-allocated blocks\n"
      "                            (see 'leak')" },

    { {"watch", NULL, NULL}, "watch [/r] [/1|/2|/4|/8] <addr|symbol|expr>",
      "Sets a hardware watchpoint (DR0-DR3) on <addr|symbol|expr>.\n"
      "  /r           break on read or write (default: write-only)\n"
      "  /1 /2 /4 /8  watch size in bytes (default: 4)\n"
      "Up to 4 watchpoints can be active at once." },

    { {"leak", NULL, NULL}, "leak [on|off]",
      "Toggles malloc/calloc/realloc/free instrumentation. While on,\n"
      "every allocation is recorded with its call site; 'show leaks'\n"
      "lists blocks that were never freed." },

    { {"help", "h", NULL}, "help [command]",
      "With no argument, shows the command summary. With a command\n"
      "name (or alias), shows a detailed explanation of that command." },

    { {"quit", NULL, NULL}, "quit",
      "Exits the debugger immediately." },
};

static void print_help_summary(void)
{
    printf("Available commands:\n");
    printf("  break <addr|symbol|file:line>  -- set a breakpoint\n");
    printf("  b <addr|symbol|file:line>      -- alias for break\n");
    printf("  continue / c                   -- continue execution\n");
    printf("  del <addr|symbol|file:line>    -- delete a breakpoint\n");
    printf("  dis [addr|symbol|file:line]    -- disassemble\n");
    printf("  help [command] / h            -- show this help, or details for one command\n");
    printf("  kill                           -- forcibly terminate the debuggee process\n");
    printf("  leak [on|off]                  -- toggle malloc/calloc/realloc/free leak tracking\n");
    printf("  list / l [line|file:line]      -- show source code around a line\n");
    printf("  lines [filter]                 -- show line number <-> address mapping\n");
    printf("  n                              -- step over (next source line)\n");
    printf("  print [/fmt] <expr>            -- print expression value\n");
    printf("  p [/fmt] <expr>                -- alias for print\n");
    printf("  quit                           -- exit debugger\n");
    printf("  regs                           -- show registers\n");
    printf("  run                            -- restart target program\n");
    printf("  s / step                       -- step into (one source line)\n");
    printf("  set print pretty [on|off]      -- toggle pretty printing\n");
    printf("  set <lhs> = <expr>            -- assign value to variable or register\n");
    printf("  show [locals|args|globals|bp|leaks|wp] -- show variables / breakpoints / leaks / watchpoints\n");
    printf("  si                             -- single step (one instruction)\n");
    printf("  watch [/r] [/1|/2|/4|/8] <addr|symbol> -- set hardware watchpoint (default: write, 4 bytes)\n");
    printf("  wdel <addr|symbol>             -- remove a hardware watchpoint\n");
    printf("  syms <name>                    -- show symbol details (address/size/type)\n");
    printf("  tb                             -- print backtrace\n");
    printf("  up                             -- run until the current function returns\n");
    printf("  x <addr|expr>                  -- examine memory\n");
    printf("Type 'help <command>' for a detailed explanation of one command.\n");
}

void do_help(const char *arg)
{
    if (!arg || arg[0] == '\0')
    {
        print_help_summary();
        return;
    }

    for (size_t i = 0; i < sizeof(help_table) / sizeof(help_table[0]); i++)
    {
        const help_entry_t *e = &help_table[i];
        for (int j = 0; j < 3 && e->names[j]; j++)
        {
            if (_stricmp(arg, e->names[j]) == 0)
            {
                printf("usage: %s\n", e->usage);
                printf("%s\n", e->detail);
                return;
            }
        }
    }

    printf("no help available for '%s'\n", arg);
    printf("valid topics:");
    for (size_t i = 0; i < sizeof(help_table) / sizeof(help_table[0]); i++)
        printf(" %s", help_table[i].names[0]);
    printf("\n");
}

void do_x(debugger_t *dbg, const char *arg)
{
    unsigned long long addr = 0;

    if (strncmp(arg, "rsp+", 4) == 0 || strncmp(arg, "rsp-", 4) == 0)
    {
        int offset = (int)atoi(arg + 3);
        CONTEXT ctx = {0};
        ctx.ContextFlags = CONTEXT_FULL;
        GetThreadContext(dbg->thread, &ctx);
        addr = ctx.Rsp + offset;
    }
    else if (strncmp(arg, "rbp+", 4) == 0 || strncmp(arg, "rbp-", 4) == 0)
    {
        int offset = (int)atoi(arg + 3);
        CONTEXT ctx = {0};
        ctx.ContextFlags = CONTEXT_FULL;
        GetThreadContext(dbg->thread, &ctx);
        addr = ctx.Rbp + offset;
    }
    else
    {
        sscanf_s(arg, "%llx", &addr);
    }

    examine_memory(dbg, (void*)addr);
}

void do_dis(debugger_t *dbg, const char *arg)
{
    if (arg[0] == '\0')
    {
        CONTEXT ctx = {0};
        ctx.ContextFlags = CONTEXT_FULL;
        GetThreadContext(dbg->thread, &ctx);

        print_disassembly(dbg, ctx.Rip, 10);
        return;
    }

    char argbuf[256];
    strncpy_s(argbuf, sizeof(argbuf), arg, _TRUNCATE);

    char *colon = strchr(argbuf, ':');
    void *addr = NULL;

    if (colon != NULL)
    {
        *colon = '\0';
        DWORD lineno = (DWORD)atoi(colon + 1);
        addr = lookup_source_line_addr(dbg, argbuf, lineno);
    }
    else if (strncmp(argbuf, "0x", 2) == 0)
    {
        unsigned long long x;
        sscanf_s(argbuf, "%llx", &x);
        addr = (void*)x;
    }
    else
    {
        DWORD lineno = (DWORD)atoi(argbuf);

        if (lineno > 0 && dbg->list_file[0] != '\0')
        {
            addr = lookup_source_line_addr(dbg, dbg->list_file, lineno);
        }
        else if (lineno > 0)
        {
            CONTEXT ctx = {0};
            ctx.ContextFlags = CONTEXT_FULL;
            GetThreadContext(dbg->thread, &ctx);

            IMAGEHLP_LINE64 src = {0};
            src.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD disp = 0;

            if (SymGetLineFromAddr64(dbg->sym_handle, ctx.Rip, &disp, &src))
                addr = lookup_source_line_addr(dbg, src.FileName, lineno);
        }

        if (addr == NULL)
            addr = lookup_symbol(dbg, argbuf);
    }

    if (addr)
        print_disassembly(dbg, (DWORD64)addr, 10);
    else
        printf("could not resolve '%s'\n", argbuf);
}

void do_list(debugger_t *dbg, const char *arg)
{
    if (arg[0] != '\0')
    {
        char argbuf[256];
        strncpy_s(argbuf, sizeof(argbuf), arg, _TRUNCATE);

        char *colon = strchr(argbuf, ':');

        if (colon != NULL)
        {
            *colon = '\0';
            DWORD lineno = (DWORD)atoi(colon + 1);
            DWORD start = (lineno > 5) ? lineno - 4 : 1;

            show_source_lines(dbg, argbuf, start, 10);
        }
        else
        {
            DWORD lineno = (DWORD)atoi(argbuf);

            if (lineno > 0 && dbg->list_file[0] != '\0')
            {
                DWORD start = (lineno > 5) ? lineno - 4 : 1;

                show_source_lines(dbg, dbg->list_file, start, 10);
            }
            else if (lineno > 0)
            {
                CONTEXT ctx2 = {0};
                ctx2.ContextFlags = CONTEXT_FULL;
                GetThreadContext(dbg->thread, &ctx2);

                IMAGEHLP_LINE64 src2 = {0};
                src2.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                DWORD disp2 = 0;

                if (SymGetLineFromAddr64(dbg->sym_handle, ctx2.Rip, &disp2, &src2))
                {
                    DWORD start = (lineno > 5) ? lineno - 4 : 1;

                    show_source_lines(dbg, src2.FileName, start, 10);
                }
                else
                {
                    printf("(no source file known)\n");
                }
            }
            else
            {
                printf("(invalid argument)\n");
            }
        }
    }
    else
    {
        CONTEXT ctx = {0};
        ctx.ContextFlags = CONTEXT_FULL;
        GetThreadContext(dbg->thread, &ctx);

        IMAGEHLP_LINE64 src = {0};
        src.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD disp = 0;

        if (dbg->list_next_line > 0 && dbg->list_file[0] != '\0')
        {
            show_source_lines(dbg, dbg->list_file, dbg->list_next_line, 10);
        }
        else if (SymGetLineFromAddr64(dbg->sym_handle, ctx.Rip, &disp, &src))
        {
            DWORD start = (src.LineNumber > 5) ? src.LineNumber - 4 : 1;

            show_source_lines(dbg, src.FileName, start, 10);
        }
        else
        {
            printf("(no source info)\n");
        }
    }
}

void do_lines(debugger_t *dbg, const char *arg)
{
    print_line_info(dbg, arg);
}

void do_leak(debugger_t *dbg, const char *arg)
{
    if (_stricmp(arg, "on") == 0)
        leak_tracking_enable(dbg);
    else if (_stricmp(arg, "off") == 0)
        leak_tracking_disable(dbg);
    else
        printf("usage: leak [on|off]\n");
}

void do_show(debugger_t *dbg, const char *arg)
{
    if (strcmp(arg, "bp") == 0)
        print_breakpoints(dbg);
    else if (strcmp(arg, "leaks") == 0)
        print_leaks(dbg);
    else if (strcmp(arg, "wp") == 0)
        print_watchpoints(dbg);
    else
        show_variables(dbg, arg);
}

void do_watch(debugger_t *dbg, const char *arg)
{
    /* Syntax: [/r] [/1|/2|/4|/8] <addr|symbol|expr>
     *   /r  = read/write (default: write-only)
     *   /1 /2 /4 /8 = size in bytes (default: 4) */
    char rest[256];
    strncpy_s(rest, sizeof(rest), arg, _TRUNCATE);

    int  wtype = WATCH_WRITE;
    int  wsize = 4;
    char *p = rest;

    while (*p == ' ') p++;
    while (*p == '/')
    {
        p++;
        if (*p == 'r' || *p == 'R') { wtype = WATCH_RW; p++; }
        else if (*p == '1') { wsize = 1; p++; }
        else if (*p == '2') { wsize = 2; p++; }
        else if (*p == '4') { wsize = 4; p++; }
        else if (*p == '8') { wsize = 8; p++; }
        else { printf("unknown watch flag: /%c\n", *p); p++; }
        while (*p == ' ') p++;
    }
    while (*p == ' ') p++;

    if (*p == '\0')
    {
        printf("usage: watch [/r] [/1|/2|/4|/8] <addr|symbol|expr>\n");
        return;
    }

    void *waddr = NULL;

    if (strncmp(p, "0x", 2) == 0 || strncmp(p, "0X", 2) == 0)
    {
        unsigned long long x = 0;
        sscanf_s(p, "%llx", &x);
        waddr = (void*)x;
    }
    else
    {
        waddr = lookup_symbol(dbg, p);
        if (!waddr)
        {
            expr_val_t v = {0};
            if (expr_eval(dbg, p, &v) == EVAL_OK && !v.is_float && v.value != 0)
                waddr = (void*)(uintptr_t)v.value;
        }
    }

    if (!waddr)
        printf("watch: cannot resolve address for '%s'\n", p);
    else
        set_watchpoint(dbg, waddr, wtype, wsize);
}

void do_print(debugger_t *dbg, const char *arg)
{
    print_fmt_t fmt = FMT_DEFAULT;
    const char *p = arg;

    while (*p == ' ') p++;

    if (*p == '/')
    {
        p++;
        switch (*p)
        {
        case 'd': case 'i': fmt = FMT_DEC;  p++; break;
        case 'x': case 'X': fmt = FMT_HEX;  p++; break;
        case 'o':           fmt = FMT_OCT;  p++; break;
        case 't':           fmt = FMT_BIN;  p++; break;
        case 'c':           fmt = FMT_CHAR; p++; break;
        case 's':           fmt = FMT_STR;  p++; break;
        default: break;
        }
        while (*p == ' ') p++;
    }

    if (*p)
        expr_print_fmt(dbg, p, fmt);
    else
        printf("usage: p[/fmt] <expr>\n");
}

void do_set(debugger_t *dbg, const char *arg)
{
    char lhs[256] = {0};
    char rhs[256] = {0};

    const char *p = arg;
    while (*p == ' ') p++;

    const char *eq = strchr(p, '=');
    if (!eq || (eq > p && eq[-1] == '!') ||
        (eq > p && eq[-1] == '<') ||
        (eq > p && eq[-1] == '>') ||
        eq[1] == '=')
    {
        printf("usage: set <lhs> = <expr>\n");
        return;
    }

    const char *vend = eq - 1;
    while (vend > p && *vend == ' ') vend--;
    size_t vlen = (size_t)(vend - p + 1);
    if (vlen >= sizeof(lhs)) vlen = sizeof(lhs) - 1;
    strncpy_s(lhs, sizeof(lhs), p, vlen);

    const char *vstart = eq + 1;
    while (*vstart == ' ') vstart++;
    strncpy_s(rhs, sizeof(rhs), vstart, sizeof(rhs) - 1);

    if (lhs[0] == '\0' || rhs[0] == '\0')
    {
        printf("usage: set <lhs> = <expr>\n");
        return;
    }

    expr_val_t rv = {0};
    if (expr_eval(dbg, rhs, &rv) != EVAL_OK)
    {
        printf("error: %s\n", rv.errmsg);
    }
    else
    {
        /* Try register first, fall back to variable */
        if (!set_register(dbg, lhs, rv.value, rv.fvalue, rv.is_float))
            expr_assign(dbg, lhs, &rv);
    }
}

void do_set_print_pretty(debugger_t *dbg, const char *arg)
{
    if (arg[0] == '\0')
    {
        printf("print pretty is %s\n", dbg->print_pretty ? "on" : "off");
    }
    else if (_stricmp(arg, "on") == 0 || strcmp(arg, "1") == 0)
    {
        dbg->print_pretty = 1;
        printf("print pretty is now on\n");
    }
    else if (_stricmp(arg, "off") == 0 || strcmp(arg, "0") == 0)
    {
        dbg->print_pretty = 0;
        printf("print pretty is now off\n");
    }
    else
    {
        printf("usage: set print pretty [on|off]\n");
    }
}

void do_unknown(const char *word)
{
    printf("unknown command: '%.63s'\n", word);
}
