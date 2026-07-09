#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "commands.h"
#include "breakpoints.h"
#include "registers.h"
#include "memory.h"
#include "symbols.h"
#include "expr.h"
#include "leakcheck.h"

void command_loop(debugger_t *dbg)
{
    char line[256];

    while (1)
    {
        printf("(tdb) ");
        fflush(stdout);

        fgets(line, sizeof(line), stdin);

        if (strncmp(line, "break", 5) == 0
            || (strncmp(line, "b ", 2) == 0))
        {
            char arg[128];

            /* use secure variant to avoid deprecated sscanf */
            if (sscanf_s(line, "%*s %127s", arg, (unsigned)sizeof(arg)) != 1)
            {
                printf("usage: break <addr|symbol|file:line>\n");
                continue;
            }

            void *addr=NULL;

            char *colon = strchr(arg, ':');

            if (colon != NULL)
            {
                *colon = '\0';
                DWORD line_num = (DWORD)atoi(colon + 1);

                addr = lookup_source_line_addr(
                    dbg, arg, line_num);
            }
            else if (strncmp(arg,"0x",2)==0)
            {
                unsigned long long x;

                sscanf_s(arg, "%llx", &x);

                addr=(void*)x;
            }
            else
            {
                addr=lookup_symbol(
                    dbg,arg);

                if (addr)
                    addr = skip_function_prologue(dbg, addr);
            }

            if(addr)
                set_breakpoint(
                    dbg,addr);
        }

        else if (strncmp(line, "del", 3) == 0)
        {
            char arg[128];
            /* use secure variant to avoid deprecated sscanf */
            if (sscanf_s(line, "%*s %127s", arg, (unsigned)sizeof(arg)) != 1)
            {
                printf("usage: del <addr|symbol|file:line>\n");
                continue;
            }

            void *addr=NULL;

            char *colon = strchr(arg, ':');

            if (colon != NULL)
            {
                *colon = '\0';
                DWORD line_num = (DWORD)atoi(colon + 1);

                addr = lookup_source_line_addr(
                    dbg, arg, line_num);
            }
            else if (strncmp(arg,"0x",2)==0)
            {
                unsigned long long x;

                sscanf_s(arg, "%llx", &x);

                addr=(void*)x;
            }
            else
            {
                addr=lookup_symbol(
                    dbg,arg);

                if (addr)
                    addr = skip_function_prologue(dbg, addr);
            }

            if(addr)
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

        else if (strncmp(line, "regs", 4) == 0)
        {
            show_registers(dbg);
        }

        else if (strncmp(line, "si", 2) == 0)
        {
            dbg->si_requested = 1;
            single_step(dbg);

            return;
        }

        else if (strncmp(line, "x", 1) == 0)
        {
            char arg[256] = {0};
            sscanf_s(line, "%*s %255[^\n]", arg, (unsigned)sizeof(arg));

            unsigned long long addr = 0;
            if (strncmp(arg, "rsp+", 4) == 0 ||
                strncmp(arg, "rsp-", 4) == 0)
            {
                int offset = (int)atoi(arg + 3);
                CONTEXT ctx = {0};
                ctx.ContextFlags = CONTEXT_FULL;
                GetThreadContext(dbg->thread, &ctx);
                addr = ctx.Rsp + offset;
            }
            else if (strncmp(arg, "rbp+", 4) == 0 ||
                     strncmp(arg, "rbp-", 4) == 0)
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

        else if (strncmp(line, "continue", 8) == 0
                 || strncmp(line, "c\n", 2) == 0
                 || strncmp(line, "c\r", 2) == 0)
        {
            return;
        }

        else if (strncmp(line,"syms ",5)==0)
        {
            char name[128] = {0};
            sscanf_s(line, "syms %127s", name, (unsigned)sizeof(name));
            if (name[0])
                print_symbol_info(dbg, name);
            else
                printf("usage: syms <name>\n");
        }

        else if ((line[0] == 's' && (line[1] == '\n' || line[1] == '\0'))
                 || strncmp(line, "step", 4) == 0)
        {
            CONTEXT ctx = {0};
            ctx.ContextFlags = CONTEXT_FULL;
            GetThreadContext(dbg->thread, &ctx);

            DWORD cur_line = get_source_line_number(
                dbg, ctx.Rip);

            if (cur_line == 0)
            {
                printf("(no source info, using si)\n");
                single_step(dbg);
                return;
            }

            dbg->step_mode = 1;
            dbg->step_line = cur_line;

            single_step(dbg);

            return;
        }

        else if (strncmp(line, "n", 1) == 0
                 && (line[1] == '\n' || line[1] == '\0'))
        {
            CONTEXT ctx = {0};
            ctx.ContextFlags = CONTEXT_FULL;
            GetThreadContext(dbg->thread, &ctx);

            DWORD cur_line = get_source_line_number(
                dbg, ctx.Rip);

            if (cur_line == 0)
            {
                printf("(no source info, using step)\n");
                single_step(dbg);
                return;
            }

            dbg->next_mode = 1;
            dbg->next_line = cur_line;
            dbg->next_rsp = ctx.Rsp;

            single_step(dbg);

            return;
        }

        else if (strncmp(line, "list", 4) == 0
                 || (line[0] == 'l' && (line[1] == '\n' || line[1] == '\0'
                     || line[1] == ' ')))
        {
            char arg[256] = {0};
            sscanf_s(line, "%*s %255[^\n]", arg, (unsigned)sizeof(arg));

            if (arg[0] != '\0')
            {
                char *colon = strchr(arg, ':');

                if (colon != NULL)
                {
                    *colon = '\0';
                    DWORD lineno = (DWORD)atoi(colon + 1);
                    DWORD start = (lineno > 5) ? lineno - 4 : 1;

                    show_source_lines(dbg, arg, start, 10);
                }
                else
                {
                    DWORD lineno = (DWORD)atoi(arg);

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

                        if (SymGetLineFromAddr64(
                                dbg->sym_handle,
                                ctx2.Rip,
                                &disp2,
                                &src2))
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
                    show_source_lines(dbg,
                        dbg->list_file,
                        dbg->list_next_line,
                        10);
                }
                else if (SymGetLineFromAddr64(
                        dbg->sym_handle,
                        ctx.Rip,
                        &disp,
                        &src))
                {
                    DWORD start = (src.LineNumber > 5)
                        ? src.LineNumber - 4 : 1;

                    show_source_lines(dbg, src.FileName, start, 10);
                }
                else
                {
                    printf("(no source info)\n");
                }
            }
        }

        else if (strncmp(line, "set print pretty", 16) == 0)
        {
            const char *p = line + 16;
            while (*p == ' ') p++;
            char *nl = strchr((char*)p, '\n'); if (nl) *nl = '\0';
            char *cr = strchr((char*)p, '\r'); if (cr) *cr = '\0';
            if (p[0] == '\0')
            {
                printf("print pretty is %s\n", dbg->print_pretty ? "on" : "off");
            }
            else if (_stricmp(p, "on") == 0 || strcmp(p, "1") == 0)
            {
                dbg->print_pretty = 1;
                printf("print pretty is now on\n");
            }
            else if (_stricmp(p, "off") == 0 || strcmp(p, "0") == 0)
            {
                dbg->print_pretty = 0;
                printf("print pretty is now off\n");
            }
            else
            {
                printf("usage: set print pretty [on|off]\n");
            }
        }

        else if (strncmp(line, "set ", 4) == 0)
        {
            char lhs[256] = {0};
            char rhs[256] = {0};

            const char *p = line + 4;
            while (*p == ' ') p++;

            const char *eq = strchr(p, '=');
            if (!eq || (eq > p && eq[-1] == '!') ||
                (eq > p && eq[-1] == '<') ||
                (eq > p && eq[-1] == '>') ||
                eq[1] == '=')
            {
                printf("usage: set <lhs> = <expr>\n");
            }
            else
            {
                const char *vend = eq - 1;
                while (vend > p && *vend == ' ') vend--;
                size_t vlen = (size_t)(vend - p + 1);
                if (vlen >= sizeof(lhs)) vlen = sizeof(lhs) - 1;
                strncpy(lhs, p, vlen);

                const char *vstart = eq + 1;
                while (*vstart == ' ') vstart++;
                strncpy(rhs, vstart, sizeof(rhs) - 1);
                char *nl = strchr(rhs, '\n'); if (nl) *nl = '\0';
                char *cr = strchr(rhs, '\r'); if (cr) *cr = '\0';

                if (lhs[0] == '\0' || rhs[0] == '\0')
                {
                    printf("usage: set <lhs> = <expr>\n");
                }
                else
                {
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
            }
        }

        else if (strncmp(line, "print", 5) == 0
                 || line[0] == 'p')
        {
            /* Determine start of "p" or "print" token, then look for /fmt */
            const char *cmd_end = (line[0] == 'p' && line[1] != 'r')
                                  ? line + 1 : line + 5;

            print_fmt_t fmt = FMT_DEFAULT;
            const char *arg = cmd_end;

            /* Skip spaces, then check for /fmt specifier */
            while (*arg == ' ') arg++;

            if (*arg == '/')
            {
                arg++; /* skip '/' */
                switch (*arg)
                {
                case 'd': case 'i': fmt = FMT_DEC;  arg++; break;
                case 'x': case 'X': fmt = FMT_HEX;  arg++; break;
                case 'o':           fmt = FMT_OCT;  arg++; break;
                case 't':           fmt = FMT_BIN;  arg++; break;
                case 'c':           fmt = FMT_CHAR; arg++; break;
                case 's':           fmt = FMT_STR;  arg++; break;
                default: break;
                }
                while (*arg == ' ') arg++;
            }

            {
                char expr_buf[512] = {0};
                strncpy(expr_buf, arg, sizeof(expr_buf) - 1);
                char *nl = strchr(expr_buf, '\n'); if (nl) *nl = '\0';
                char *cr = strchr(expr_buf, '\r'); if (cr) *cr = '\0';
                if (expr_buf[0])
                    expr_print_fmt(dbg, expr_buf, fmt);
                else
                    printf("usage: p[/fmt] <expr>\n");
            }
        }

        else if (strncmp(line, "up", 2) == 0)
        {
            CONTEXT ctx = {0};
            ctx.ContextFlags = CONTEXT_FULL;
            GetThreadContext(dbg->thread, &ctx);

            DWORD64 ret_addr = 0;
            SIZE_T n;
            ReadProcessMemory(
                dbg->process,
                (void*)ctx.Rsp,
                &ret_addr,
                sizeof(ret_addr),
                &n
            );

            printf("return addr = 0x%llx\n", ret_addr);

            set_temp_breakpoint(dbg, (void*)ret_addr);

            return;
        }

        else if (strncmp(line, "dis", 3) == 0)
        {
            char arg[256] = {0};
            sscanf_s(line, "%*s %255[^\n]", arg, (unsigned)sizeof(arg));

            if (arg[0] == '\0')
            {
                CONTEXT ctx = {0};
                ctx.ContextFlags = CONTEXT_FULL;
                GetThreadContext(dbg->thread, &ctx);

                print_disassembly(dbg, ctx.Rip, 10);
            }
            else
            {
                char *colon = strchr(arg, ':');
                void *addr = NULL;

                if (colon != NULL)
                {
                    *colon = '\0';
                    DWORD lineno = (DWORD)atoi(colon + 1);
                    addr = lookup_source_line_addr(dbg, arg, lineno);
                }
                else if (strncmp(arg, "0x", 2) == 0)
                {
                    unsigned long long x;
                    sscanf_s(arg, "%llx", &x);
                    addr = (void*)x;
                }
                else
                {
                    DWORD lineno = (DWORD)atoi(arg);

                    if (lineno > 0 && dbg->list_file[0] != '\0')
                    {
                        addr = lookup_source_line_addr(
                            dbg, dbg->list_file, lineno);
                    }
                    else if (lineno > 0)
                    {
                        CONTEXT ctx = {0};
                        ctx.ContextFlags = CONTEXT_FULL;
                        GetThreadContext(dbg->thread, &ctx);

                        IMAGEHLP_LINE64 src = {0};
                        src.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                        DWORD disp = 0;

                        if (SymGetLineFromAddr64(
                                dbg->sym_handle,
                                ctx.Rip,
                                &disp,
                                &src))
                        {
                            addr = lookup_source_line_addr(
                                dbg, src.FileName, lineno);
                        }
                    }

                    if (addr == NULL)
                        addr = lookup_symbol(dbg, arg);
                }

                if (addr)
                    print_disassembly(dbg, (DWORD64)addr, 10);
                else
                    printf("could not resolve '%s'\n", arg);
            }
        }

        else if (strncmp(line, "tb", 2) == 0)
        {
            print_backtrace(dbg);
        }

        else if (strncmp(line, "lines", 5) == 0)
        {
            char filter[256] = {0};
            /* optional filename filter after "lines " */
            if (line[5] == ' ')
            {
                strncpy(filter, line + 6, sizeof(filter) - 1);
                char *nl = strchr(filter, '\n'); if (nl) *nl = '\0';
                char *cr = strchr(filter, '\r'); if (cr) *cr = '\0';
            }
            print_line_info(dbg, filter);
        }

        else if (strncmp(line, "leak", 4) == 0)
        {
            const char *p = line + 4;
            while (*p == ' ') p++;
            char *nl = strchr((char*)p, '\n'); if (nl) *nl = '\0';
            char *cr = strchr((char*)p, '\r'); if (cr) *cr = '\0';

            if (_stricmp(p, "on") == 0)
                leak_tracking_enable(dbg);
            else if (_stricmp(p, "off") == 0)
                leak_tracking_disable(dbg);
            else
                printf("usage: leak [on|off]\n");
        }

        else if (strncmp(line, "show ", 5) == 0
                 || strcmp(line, "show\n") == 0
                 || strcmp(line, "show\r\n") == 0
                 || strcmp(line, "show") == 0)
        {
            char what[128] = {0};
            if (line[4] == ' ')
            {
                strncpy(what, line + 5, sizeof(what) - 1);
                char *nl = strchr(what, '\n'); if (nl) *nl = '\0';
                char *cr = strchr(what, '\r'); if (cr) *cr = '\0';
            }

            if (strcmp(what, "bp") == 0)
                print_breakpoints(dbg);
            else if (strcmp(what, "leaks") == 0)
                print_leaks(dbg);
            else
                show_variables(dbg, what);
        }

        else if (strncmp(line, "run", 3) == 0)
        {
            if (dbg->target_program[0] == '\0')
            {
                printf("no target program\n");
            }
            else
            {
                printf("restarting %s\n", dbg->target_program);
                debugger_restart(dbg);
                return;
            }
        }

        else if (strncmp(line, "help", 4) == 0
                 || strcmp(line, "h\n") == 0
                 || strcmp(line, "h\r\n") == 0)
        {
            printf("Available commands:\n");
            printf("  break <addr|symbol|file:line>  -- set a breakpoint\n");
            printf("  b <addr|symbol|file:line>      -- alias for break\n");
            printf("  continue / c                   -- continue execution\n");
            printf("  del <addr|symbol|file:line>    -- delete a breakpoint\n");
            printf("  dis [addr|symbol|file:line]    -- disassemble\n");
            printf("  help / h                       -- show this help\n");
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
            printf("  show [locals|args|globals|bp|leaks] -- show variables / breakpoints / leaks\n");
            printf("  si                             -- single step (one instruction)\n");
            printf("  syms <name>                    -- show symbol details (address/size/type)\n");
            printf("  tb                             -- print backtrace\n");
            printf("  up                             -- run until the current function returns\n");
            printf("  x <addr|expr>                  -- examine memory\n");
        }

        else if (strncmp(line, "quit", 4) == 0)
        {
            exit(0);
        }

        else
        {
            /* strip trailing newline for display */
            char cmd[64] = {0};
            strncpy(cmd, line, sizeof(cmd) - 1);
            char *nl = strchr(cmd, '\n'); if (nl) *nl = '\0';
            char *cr = strchr(cmd, '\r'); if (cr) *cr = '\0';
            if (cmd[0] != '\0')
                printf("unknown command: '%s'\n", cmd);
        }
    }
}