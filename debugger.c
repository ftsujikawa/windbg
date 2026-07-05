#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debugger.h"
#include "breakpoints.h"
#include "commands.h"
#include "symbols.h"

int debugger_start(debugger_t *dbg, const char *program)
{
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};

    si.cb = sizeof(si);

    BOOL ok = CreateProcess(
        program,
        NULL,
        NULL,
        NULL,
        FALSE,
        DEBUG_ONLY_THIS_PROCESS,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!ok)
    {
        printf("CreateProcess failed: %lu\n", GetLastError());
        return -1;
    }

    dbg->process = pi.hProcess;
    dbg->thread = pi.hThread;
    dbg->pid = pi.dwProcessId;
    dbg->tid = pi.dwThreadId;
    dbg->sym_handle = GetCurrentProcess();

    strncpy(dbg->target_program, program, sizeof(dbg->target_program) - 1);

    printf("started pid=%lu\n", dbg->pid);

    return 0;
}

void debugger_restart(debugger_t *dbg)
{
    char program[512];
    strncpy(program, dbg->target_program, sizeof(program) - 1);

    breakpoint_t *bp = dbg->breakpoints;
    while (bp != NULL)
    {
        breakpoint_t *next = bp->next;
        free(bp);
        bp = next;
    }

    if (dbg->sym_handle)
        SymCleanup(dbg->sym_handle);

    memset(dbg, 0, sizeof(debugger_t));

    strncpy(dbg->target_program, program, sizeof(dbg->target_program) - 1);

    if (debugger_start(dbg, program) < 0)
    {
        printf("failed to restart\n");
        return;
    }

    debugger_loop(dbg);
}

void debugger_loop(debugger_t *dbg)
{
    DEBUG_EVENT ev;

    while (1)
    {
        BOOL ok =
            WaitForDebugEvent(
                &ev,
                INFINITE
            );

        if (!ok)
        {
            printf(
                "WaitForDebugEvent failed\n"
            );

            return;
        }

        dbg->tid = ev.dwThreadId;

        switch (ev.dwDebugEventCode)
        {
            case CREATE_PROCESS_DEBUG_EVENT:

                printf(
                    "CREATE_PROCESS\n"
                );

                printf(
                    "base=%p\n",
                    ev.u
                      .CreateProcessInfo
                      .lpBaseOfImage
                );

                init_symbols(dbg);

                DWORD64 base =
                    SymLoadModuleEx(
                        dbg->sym_handle,
                        NULL,
                        "testprog.exe",
                        NULL,
                        (DWORD64)
                        ev.u.CreateProcessInfo.lpBaseOfImage,
                        0,
                        NULL,
                        0
                    );

                printf("module loaded = %llx\n", base);

                if (!base)
                {
                    printf("SymLoadModuleEx err=%lu\n",
                        GetLastError());
                }
                
                break;

            case EXCEPTION_DEBUG_EVENT:

                printf(
                    "exception=0x%lx\n",
                    ev.u.Exception
                        .ExceptionRecord
                        .ExceptionCode
                    );

                int source_shown = 0;

                if (ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT
                    && dbg->temp_bp_addr != NULL)
                {
                    remove_temp_breakpoint(dbg);

                    CONTEXT ctx = {0};
                    ctx.ContextFlags = CONTEXT_FULL;
                    GetThreadContext(dbg->thread, &ctx);
                    ctx.Rip--;
                    SetThreadContext(dbg->thread, &ctx);

                    if (dbg->next_mode)
                    {
                        DWORD cur_line = get_source_line_number(
                            dbg, ctx.Rip);

                        if (cur_line != 0 && cur_line == dbg->next_line)
                        {
                            single_step(dbg);
                            break;
                        }

                        dbg->next_mode = 0;
                        dbg->next_line = 0;
                    }

                    printf("returned to 0x%llx\n", ctx.Rip);
                    show_source_line(dbg, ctx.Rip);
                    source_shown = 1;
                }

                else if (ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                {
                    CONTEXT ctx = {0};
                    ctx.ContextFlags = CONTEXT_FULL;
                    GetThreadContext(dbg->thread, &ctx);

                    void *hit_addr = (void*)(ctx.Rip - 1);
                    breakpoint_t *bp = find_breakpoint(dbg, hit_addr);

                    if (bp != NULL)
                    {
                        remove_breakpoint_at(dbg, hit_addr);

                        ctx.Rip--;
                        SetThreadContext(dbg->thread, &ctx);

                        printf("hit breakpoint at %p\n", hit_addr);
                        if (!show_source_line(dbg, ctx.Rip))
                            print_disassembly(dbg, ctx.Rip, 10);
                        source_shown = 1;
                    }
                    else
                    {
                        printf("breakpoint at %p (not tracked)\n", hit_addr);
                    }
                }

                if (ev.u.Exception.ExceptionRecord.ExceptionCode == 0x80000004)
                {
                    CONTEXT ctx = {0};
                    ctx.ContextFlags = CONTEXT_FULL;
                    GetThreadContext(dbg->thread, &ctx);

                    if (dbg->next_mode)
                    {
                        DWORD cur_line = get_source_line_number(
                            dbg, ctx.Rip);

                        if (cur_line == 0)
                        {
                            DWORD64 ret_addr = 0;
                            SIZE_T n;
                            ReadProcessMemory(
                                dbg->process,
                                (void*)ctx.Rsp,
                                &ret_addr,
                                sizeof(ret_addr),
                                &n
                            );
                            set_temp_breakpoint(dbg, (void*)ret_addr);
                            break;
                        }

                        if (cur_line == dbg->next_line)
                        {
                            single_step(dbg);
                            break;
                        }

                        dbg->next_mode = 0;
                        dbg->next_line = 0;
                    }

                    if (dbg->step_mode)
                    {
                        DWORD cur_line = get_source_line_number(
                            dbg, ctx.Rip);

                        printf("[debug step] cur_line=%lu step_line=%lu\n",
                            cur_line, dbg->step_line);

                        if (cur_line == 0)
                        {
                            single_step(dbg);
                            break;
                        }

                        if (cur_line == dbg->step_line)
                        {
                            single_step(dbg);
                            break;
                        }

                        dbg->step_mode = 0;
                        dbg->step_line = 0;
                    }

                    printf("step -> RIP=0x%llx\n", ctx.Rip);
                    show_source_line(dbg, ctx.Rip);
                    source_shown = 1;
                }

                {
                    CONTEXT ctx2 = {0};
                    ctx2.ContextFlags = CONTEXT_FULL;
                    GetThreadContext(dbg->thread, &ctx2);

                    printf("stopped exception=0x%lx RIP=0x%llx\n",
                        ev.u.Exception.ExceptionRecord.ExceptionCode,
                        ctx2.Rip);

                    if (!source_shown)
                        show_source_line(dbg, ctx2.Rip);
                }

                dbg->list_next_line = 0;
                dbg->list_file[0] = '\0';

                command_loop(dbg);

                break;

            case EXIT_PROCESS_DEBUG_EVENT:

                printf("process exited with code %lu\n",
                    ev.u.ExitProcess.dwExitCode);

                ContinueDebugEvent(
                    ev.dwProcessId,
                    ev.dwThreadId,
                    DBG_CONTINUE
                );

                printf("(mini-gdb) process has exited. type 'quit' to exit.\n");
                command_loop(dbg);
                return;
        }

        ContinueDebugEvent(
            ev.dwProcessId,
            ev.dwThreadId,
            DBG_CONTINUE
        );
    }
}