#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "commands.h"
#include "breakpoints.h"
#include "registers.h"
#include "memory.h"
#include "symbols.h"

void command_loop(debugger_t *dbg)
{
    char line[256];

    while (1)
    {
        printf("(mini-gdb) ");

        fgets(line, sizeof(line), stdin);

        if (strncmp(line, "break", 5) == 0
            || (strncmp(line, "b ", 2) == 0))
        {
            char arg[128];

            sscanf(line,"%*s %s",arg);

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

                sscanf(arg,"%llx",&x);

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

        else if (strncmp(line, "regs", 4) == 0)
        {
            show_registers(dbg);
        }

        else if (strncmp(line, "si", 2) == 0)
        {
            single_step(dbg);

            return;
        }

        else if (strncmp(line, "x", 1) == 0)
        {
            char arg[256] = {0};
            sscanf(line, "%*s %255[^\n]", arg);

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
                sscanf(arg, "%llx", &addr);
            }

            examine_memory(dbg, (void*)addr);
        }

        else if (strncmp(line, "continue", 8) == 0)
        {
            return;
        }

        else if (strncmp(line,"sym",3)==0)
        {
            char name[128];

            sscanf(line,"sym %s",name);

            printf("name = %s\n",name);

            lookup_symbol(dbg,name);
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
            sscanf(line, "%*s %255[^\n]", arg);

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

        else if (strncmp(line, "set ", 4) == 0)
        {
            char varname[128];
            char op;
            char value_str[128];

            int n = sscanf(line, "%*s %s %c %127s", varname, &op, value_str);

            if (n != 3 || op != '=')
            {
                printf("usage: set <var> = <value>\n");
            }
            else
            {
                long long value = strtoll(value_str, NULL, 0);
                set_variable(dbg, varname, value);
            }
        }

        else if (strncmp(line, "print ", 6) == 0
                 || strncmp(line, "p ", 2) == 0)
        {
            char varname[256] = {0};

            sscanf(line, "%*s %255[^\n]", varname);

            char *nl = strchr(varname, '\n');
            if (nl) *nl = '\0';
            char *cr = strchr(varname, '\r');
            if (cr) *cr = '\0';

            print_variable(dbg, varname);
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
            sscanf(line, "%*s %255[^\n]", arg);

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
                    sscanf(arg, "%llx", &x);
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

        else if (strncmp(line, "quit", 4) == 0)
        {
            exit(0);
        }
    }
}