#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debugger.h"
#include "breakpoints.h"
#include "commands.h"
#include "registers.h"
#include "symbols.h"
#include "leakcheck.h"
#include "watchpoints.h"
#include "threads.h"
#include "processes.h"

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
        DEBUG_PROCESS,
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

    strncpy_s(dbg->target_program, sizeof(dbg->target_program), program, sizeof(dbg->target_program) - 1);
    strncpy_s(dbg->image_path, sizeof(dbg->image_path), program, sizeof(dbg->image_path) - 1);

    printf("started pid=%lu\n", dbg->pid);

    return 0;
}

void debugger_restart(debugger_t *dbg)
{
    char program[512];
    strncpy_s(program, sizeof(program), dbg->target_program, sizeof(program) - 1);

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

    free_all_threads(dbg);
    free_all_processes(dbg);

    if (dbg->sym_handle)
        SymCleanup(dbg->sym_handle);

    memset(dbg, 0, sizeof(debugger_t));

    strncpy_s(dbg->target_program, sizeof(dbg->target_program), program, sizeof(dbg->target_program) - 1);

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

        switch (ev.dwDebugEventCode)
        {
            case CREATE_PROCESS_DEBUG_EVENT:

                if (ev.dwProcessId == dbg->pid)
                {
                    /* the root process, on its very first event --
                       debugger_start already set dbg->pid/process/tid/thread */
                    dbg->tid = ev.dwThreadId;
                    register_thread(&dbg->threads, dbg->tid, dbg->thread);

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

                    if (!base)
                    {
                        printf("SymLoadModuleEx err=%lu\n",
                            GetLastError());
                    }
                }
                else
                {
                    /* a child process the debuggee just spawned (DEBUG_PROCESS
                       follows the whole process tree). Register it as a
                       backgrounded process, then briefly activate it to reuse
                       the exact same symbol/thread init as the root case
                       above, and swap back to whichever process was actually
                       focused -- this doesn't disturb that process since no
                       user interaction happens between debug events. */
                    char image_path[512] = {0};
                    DWORD size = sizeof(image_path);
                    if (!QueryFullProcessImageNameA(ev.u.CreateProcessInfo.hProcess, 0, image_path, &size))
                        image_path[0] = '\0';

                    register_new_process(dbg, ev.dwProcessId, ev.u.CreateProcessInfo.hProcess, image_path);

                    DWORD prev_pid = dbg->pid;
                    switch_active_process(dbg, ev.dwProcessId);

                    dbg->sym_handle = dbg->process;
                    register_thread(&dbg->threads, ev.dwThreadId, ev.u.CreateProcessInfo.hThread);

                    init_symbols(dbg);

                    DWORD64 base =
                        SymLoadModuleEx(
                            dbg->sym_handle,
                            NULL,
                            image_path,
                            NULL,
                            (DWORD64)
                            ev.u.CreateProcessInfo.lpBaseOfImage,
                            0,
                            NULL,
                            0
                        );

                    if (!base)
                    {
                        printf("SymLoadModuleEx err=%lu\n",
                            GetLastError());
                    }

                    switch_active_process(dbg, prev_pid);

                    printf("process created id=%lu\n", ev.dwProcessId);
                }

                break;

            case CREATE_THREAD_DEBUG_EVENT:

                /* Doesn't touch dbg->tid/dbg->thread: this must not disturb
                   whatever thread is currently "current" for an in-flight
                   step/next/rearm sequence. */
                if (ev.dwProcessId == dbg->pid)
                {
                    register_thread(&dbg->threads, ev.dwThreadId, ev.u.CreateThread.hThread);
                    apply_watchpoints(dbg);
                }
                else
                {
                    process_entry_t *p = find_background_process(dbg, ev.dwProcessId);
                    if (p != NULL)
                    {
                        register_thread(&p->threads, ev.dwThreadId, ev.u.CreateThread.hThread);
                        apply_watchpoints_to_thread(p->watchpoints, p->watch_count, ev.u.CreateThread.hThread);
                    }
                }

                printf("thread created id=%lu\n", ev.dwThreadId);

                break;

            case EXIT_THREAD_DEBUG_EVENT:

                printf("thread exited id=%lu exit code=%lu\n",
                    ev.dwThreadId, ev.u.ExitThread.dwExitCode);

                /* release the pin so focus can move to another live thread
                   instead of auto-continuing everything forever */
                if (dbg->locked_tid == ev.dwThreadId)
                    dbg->locked_tid = 0;

                if (ev.dwProcessId == dbg->pid)
                {
                    unregister_thread(&dbg->threads, ev.dwThreadId);
                }
                else
                {
                    process_entry_t *p = find_background_process(dbg, ev.dwProcessId);
                    if (p != NULL)
                        unregister_thread(&p->threads, ev.dwThreadId);
                }

                break;

#ifndef DBG_CONTROL_C
#define DBG_CONTROL_C 0x40010005
#endif

            case EXCEPTION_DEBUG_EVENT:

                /* This event may be for a process other than whichever is
                   currently active (DEBUG_PROCESS delivers events for the
                   whole process tree through this same loop); swap that
                   process's state onto dbg first so every mechanic below
                   (breakpoints, watchpoints, stepping, auto_continue) reads
                   the correct one. */
                if (ev.dwProcessId != dbg->pid)
                    switch_active_process(dbg, ev.dwProcessId);

                dbg->tid = ev.dwThreadId;
                dbg->thread = find_thread_handle(dbg, dbg->tid);

                /* Focus auto-pins to whichever thread first produces a real,
                   deliberate stop (tracked breakpoint/watchpoint/step
                   landing -- see the "dbg->locked_tid = dbg->tid" lines
                   below); once pinned, any OTHER thread's such stop
                   auto-continues instead of switching focus / entering the
                   command loop. 'thread <tid>' re-pins explicitly, and
                   'thread unlock' clears the pin (see commands.c). */
                int auto_continue = (dbg->locked_tid != 0 && dbg->tid != dbg->locked_tid);

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

                        /* the four branches below (but not the return-catch)
                           rearm their entry breakpoint by briefly restoring
                           its original byte; freeze other threads for that
                           window so none of them slip past unnoticed */
                        if (hit_addr != dbg->malloc_ret_addr)
                            suspend_other_threads(dbg);

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

                    if (auto_continue)
                    {
                        printf("thread id=%lu returned to 0x%llx (not focused, continuing)\n",
                            dbg->tid, ctx.Rip);
                    }
                    else
                    {
                        dbg->locked_tid = dbg->tid;
                        printf("returned to 0x%llx\n", ctx.Rip);
                        show_pid_tid(dbg);
                        show_source_line(dbg, ctx.Rip);
                        source_shown = 1;
                    }
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
                        restore_breakpoint_byte(dbg, hit_addr, orig_byte);

                        ctx.Rip--;
                        SetThreadContext(dbg->thread, &ctx);

                        arm_breakpoint_rearm(dbg, hit_addr, orig_byte);
                        suspend_other_threads(dbg);

                        if (auto_continue)
                        {
                            printf("thread id=%lu hit breakpoint at %p (not focused, continuing)\n",
                                dbg->tid, hit_addr);
                        }
                        else
                        {
                            dbg->locked_tid = dbg->tid;
                            printf("hit breakpoint at %p\n", hit_addr);
                            show_pid_tid(dbg);
                            if (!show_source_line(dbg, ctx.Rip))
                                print_disassembly(dbg, ctx.Rip, 1);
                            source_shown = 1;
                        }
                    }
                    else
                    {
                        /* Do not decrement Rip here: untracked breakpoints
                         * such as the ntdll initial break are handled by the
                         * OS when we continue with DBG_CONTINUE. Deliberately
                         * does not auto-pin focus here (an untracked hit,
                         * e.g. the process-start ntdll break, isn't a
                         * user-chosen stop worth anchoring on). */
                        if (auto_continue)
                        {
                            printf("thread id=%lu breakpoint at %p (not tracked, not focused, continuing)\n",
                                dbg->tid, hit_addr);
                        }
                        else
                        {
                            printf("breakpoint at %p (not tracked)\n", hit_addr);
                            show_pid_tid(dbg);
                            if (!show_source_line(dbg, ctx.Rip))
                                print_disassembly(dbg, ctx.Rip, 1);
                            source_shown = 1;
                        }
                    }
                }

                if (ev.u.Exception.ExceptionRecord.ExceptionCode == 0x80000004)
                {
                    /* Check for hardware watchpoint hit (DR6 bits 0-3) before
                     * anything else so that a concurrent step/rearm still sees it. */
                    if (dbg->watch_count > 0)
                    {
                        int ws = watchpoint_hit_slot(dbg);
                        if (ws >= 0)
                        {
                            CONTEXT ctx = {0};
                            ctx.ContextFlags = CONTEXT_FULL;
                            GetThreadContext(dbg->thread, &ctx);
                            watchpoint_report(dbg, ws, ctx.Rip);
                            show_pid_tid(dbg);

                            if (auto_continue)
                            {
                                printf("(not focused, continuing)\n");
                                break;
                            }

                            dbg->locked_tid = dbg->tid;
                            if (!show_source_line(dbg, ctx.Rip))
                                print_disassembly(dbg, ctx.Rip, 1);
                            source_shown = 1;
                            dbg->list_next_line = 0;
                            dbg->list_file[0] = '\0';
                            command_loop(dbg);
                            break;
                        }
                    }

                    int si_requested = dbg->si_requested;
                    dbg->si_requested = 0;

                    if (breakpoint_rearm_pending(dbg))
                    {
                        finish_breakpoint_rearm(dbg);
                        resume_other_threads(dbg);

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
                        resume_other_threads(dbg);

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

                    if (auto_continue)
                        break;

                    dbg->locked_tid = dbg->tid;
                    printf("step -> RIP=0x%llx\n", ctx.Rip);
                    show_pid_tid(dbg);
                    if (!show_source_line(dbg, ctx.Rip))
                        print_disassembly(dbg, ctx.Rip, 1);
                    source_shown = 1;
                }

                if (auto_continue)
                {
                    dbg->list_next_line = 0;
                    dbg->list_file[0] = '\0';
                    break;
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

                if (!unregister_process(dbg, ev.dwProcessId, ev.u.ExitProcess.dwExitCode))
                {
                    /* that was the last tracked process -- end the session */
                    ContinueDebugEvent(
                        ev.dwProcessId,
                        ev.dwThreadId,
                        DBG_CONTINUE
                    );

                    printf("(tdb) process has exited. type 'quit' to exit.\n");
                    command_loop(dbg);
                    return;
                }

                /* other processes remain tracked; keep looping without
                   stopping interactively, same treatment as a thread exit */
                break;
        }

        ContinueDebugEvent(
            ev.dwProcessId,
            ev.dwThreadId,
            DBG_CONTINUE
        );
    }
}