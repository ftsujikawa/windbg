#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debugger.h"
#include "breakpoints.h"
#include "commands.h"
#include "registers.h"
#include "symbols.h"
#include "leakcheck.h"

/* Print PID and TID at a stop point before source/disassembly output. */
static void show_pid_tid(debugger_t *dbg)
{
    printf("process id=%lu thread id=%lu\n", dbg->pid, dbg->tid);
}

/* Console control handler: swallow Ctrl+C in the debugger itself.
 * The debuggee, being in the same console group, receives the same event
 * and reports it as DBG_CONTROL_C; debugger_loop treats that as a break-in. */
static BOOL WINAPI ctrl_event_handler(DWORD ctrl_type)
{
    if (ctrl_type == CTRL_C_EVENT)
        return TRUE;
    return FALSE;
}

int debugger_start(debugger_t *dbg, const char *program)
{
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};

    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
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
    dbg->print_pretty = 1;

    if (!SetConsoleCtrlHandler(ctrl_event_handler, TRUE))
        printf("SetConsoleCtrlHandler failed: %lu\n", GetLastError());

    /* Must be the real debuggee handle, not GetCurrentProcess(): DbgHelp's
       .pdata (unwind info) lookups for StackWalk64 read the module image
       from whatever process handle Sym* calls were made with, and that
       image is only mapped in the debuggee. Symbol/line lookups work with
       either handle (they read the .pdb file, not live memory), but
       SymFunctionTableAccess64 needs the real one. */
    dbg->sym_handle = dbg->process;

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

    alloc_info_t *a = dbg->allocations;
    while (a != NULL)
    {
        alloc_info_t *next = a->next;
        free(a);
        a = next;
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
                        dbg->target_program,
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

#ifndef DBG_CONTROL_C
#define DBG_CONTROL_C 0x40010005
#endif

            case EXCEPTION_DEBUG_EVENT:

                printf(
                    "exception=0x%lx\n",
                    ev.u.Exception
                        .ExceptionRecord
                        .ExceptionCode
                    );

                int source_shown = 0;

                /* Ctrl+C sent to the debuggee: treat it as a break-in. */
                if (ev.u.Exception.ExceptionRecord.ExceptionCode == DBG_CONTROL_C)
                {
                    CONTEXT ctx = {0};
                    ctx.ContextFlags = CONTEXT_FULL;
                    GetThreadContext(dbg->thread, &ctx);
                    printf("Interrupted by Ctrl+C\n");
                    printf("stopped exception=0x%lx RIP=0x%llx\n",
                           ev.u.Exception.ExceptionRecord.ExceptionCode,
                           ctx.Rip);
                    show_pid_tid(dbg);
                    if (!show_source_line(dbg, ctx.Rip))
                        print_disassembly(dbg, ctx.Rip, 1);
                    source_shown = 1;
                    command_loop(dbg);
                    break;
                }

                /* Access violations in system CRT: ignore to allow continuation */
                if (ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
                {
                    CONTEXT ctx = {0};
                    ctx.ContextFlags = CONTEXT_FULL;
                    GetThreadContext(dbg->thread, &ctx);
                    printf("stopped exception=0x%lx RIP=0x%llx\n",
                           ev.u.Exception.ExceptionRecord.ExceptionCode,
                           ctx.Rip);
                    fflush(stdout);
                    if (!show_source_line(dbg, ctx.Rip))
                        print_disassembly(dbg, ctx.Rip, 1);
                    /* Do NOT enter command loop; continue execution */
                    break;
                }

                if (dbg->leak_tracking &&
                    ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                {
                    CONTEXT lctx = {0};
                    lctx.ContextFlags = CONTEXT_FULL;
                    GetThreadContext(dbg->thread, &lctx);
                    void *hit_addr = (void*)(lctx.Rip - 1);

                    if (hit_addr == dbg->malloc_addr || 
                        (dbg->calloc_addr && hit_addr == dbg->calloc_addr) ||
                        (dbg->realloc_addr && hit_addr == dbg->realloc_addr) ||
                        hit_addr == dbg->free_addr ||
                        (dbg->malloc_ret_addr != NULL && hit_addr == dbg->malloc_ret_addr))
                    {
                        lctx.Rip--;
                        SetThreadContext(dbg->thread, &lctx);

                        if (hit_addr == dbg->malloc_addr)
                            leak_on_malloc_enter(dbg);
                        else if (dbg->calloc_addr && hit_addr == dbg->calloc_addr)
                            leak_on_calloc_enter(dbg);
                        else if (dbg->realloc_addr && hit_addr == dbg->realloc_addr)
                            leak_on_realloc_enter(dbg);
                        else if (hit_addr == dbg->free_addr)
                            leak_on_free_enter(dbg);
                        else
                            leak_on_malloc_return(dbg);

                        break;
                    }
                }

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
                        BYTE orig_byte = bp->original_byte;
                        remove_breakpoint_at(dbg, hit_addr);

                        ctx.Rip--;
                        SetThreadContext(dbg->thread, &ctx);

                        arm_breakpoint_rearm(dbg, hit_addr, orig_byte);

                        printf("hit breakpoint at %p\n", hit_addr);
                        show_pid_tid(dbg);
                        if (!show_source_line(dbg, ctx.Rip))
                            print_disassembly(dbg, ctx.Rip, 1);
                        source_shown = 1;
                    }
                    else
                    {
                        /* Do not decrement Rip here: untracked breakpoints
                         * such as the ntdll initial break are handled by the
                         * OS when we continue with DBG_CONTINUE. */
                        printf("breakpoint at %p (not tracked)\n", hit_addr);
                        show_pid_tid(dbg);
                        if (!show_source_line(dbg, ctx.Rip))
                            print_disassembly(dbg, ctx.Rip, 1);
                        source_shown = 1;
                    }
                }

                if (ev.u.Exception.ExceptionRecord.ExceptionCode == 0x80000004)
                {
                    int si_requested = dbg->si_requested;
                    dbg->si_requested = 0;

                    if (breakpoint_rearm_pending(dbg))
                    {
                        finish_breakpoint_rearm(dbg);

                        /* a user step/next was also in flight: keep it
                           moving, this tick was consumed internally */
                        if (dbg->next_mode || dbg->step_mode)
                        {
                            single_step(dbg);
                            break;
                        }

                        /* a plain `si` landed on exactly this rearm step:
                           still show it, don't silently swallow it */
                        if (!si_requested)
                            break;
                    }

                    if (dbg->leak_tracking && leak_rearm_pending(dbg))
                    {
                        leak_finish_rearm(dbg);

                        /* a user step/next was also in flight: keep it
                           moving, this tick was consumed internally */
                        if (dbg->next_mode || dbg->step_mode)
                            single_step(dbg);

                        break;
                    }

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
                    show_pid_tid(dbg);
                    if (!show_source_line(dbg, ctx.Rip))
                        print_disassembly(dbg, ctx.Rip, 1);
                    source_shown = 1;
                }

                {
                    CONTEXT ctx2 = {0};
                    ctx2.ContextFlags = CONTEXT_FULL;
                    GetThreadContext(dbg->thread, &ctx2);

                    printf("stopped exception=0x%lx RIP=0x%llx\n",
                        ev.u.Exception.ExceptionRecord.ExceptionCode,
                        ctx2.Rip);
                    fflush(stdout);

                    if (!source_shown)
                    {
                        if (!show_source_line(dbg, ctx2.Rip))
                            print_disassembly(dbg, ctx2.Rip, 1);
                    }
                }

                dbg->list_next_line = 0;
                dbg->list_file[0] = '\0';

                command_loop(dbg);

                break;

            case EXIT_PROCESS_DEBUG_EVENT:

                printf("process exited with code %lu\n",
                    ev.u.ExitProcess.dwExitCode);

                if (dbg->leak_tracking)
                    print_leaks(dbg);

                ContinueDebugEvent(
                    ev.dwProcessId,
                    ev.dwThreadId,
                    DBG_CONTINUE
                );

                printf("(tdb) process has exited. type 'quit' to exit.\n");
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