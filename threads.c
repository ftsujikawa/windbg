#include <stdio.h>
#include <stdlib.h>
#include "threads.h"

void register_thread(thread_entry_t **list, DWORD tid, HANDLE handle)
{
    thread_entry_t *t = (thread_entry_t*)malloc(sizeof(thread_entry_t));
    if (t == NULL)
        return;

    t->tid = tid;
    t->handle = handle;
    t->next = *list;
    *list = t;
}

void unregister_thread(thread_entry_t **list, DWORD tid)
{
    thread_entry_t **pp = list;
    while (*pp != NULL)
    {
        if ((*pp)->tid == tid)
        {
            thread_entry_t *dead = *pp;
            *pp = dead->next;
            CloseHandle(dead->handle);
            free(dead);
            return;
        }
        pp = &(*pp)->next;
    }
}

HANDLE find_thread_handle(debugger_t *dbg, DWORD tid)
{
    for (thread_entry_t *t = dbg->threads; t != NULL; t = t->next)
        if (t->tid == tid)
            return t->handle;
    return NULL;
}

void free_all_threads(debugger_t *dbg)
{
    thread_entry_t *t = dbg->threads;
    while (t != NULL)
    {
        thread_entry_t *next = t->next;
        free(t);
        t = next;
    }
    dbg->threads = NULL;
}

void suspend_other_threads(debugger_t *dbg)
{
    for (thread_entry_t *t = dbg->threads; t != NULL; t = t->next)
        if (t->tid != dbg->tid)
            SuspendThread(t->handle);
}

void resume_other_threads(debugger_t *dbg)
{
    for (thread_entry_t *t = dbg->threads; t != NULL; t = t->next)
        if (t->tid != dbg->tid)
            ResumeThread(t->handle);
}

void print_threads(debugger_t *dbg)
{
    if (dbg->threads == NULL)
    {
        printf("no threads\n");
        return;
    }

    for (thread_entry_t *t = dbg->threads; t != NULL; t = t->next)
    {
        const char *tag = "";
        if (t->tid == dbg->tid)
            tag = (dbg->locked_tid == t->tid) ? "  (current, locked)" : "  (current)";

        /* GetThreadContext only gives a reliable snapshot for a suspended
           thread; the current thread is already stopped (that's why we're
           in command_loop), but any other thread may still be running
           freely, so briefly suspend just that one to read it safely. */
        int suspended = (t->tid != dbg->tid);
        if (suspended)
            SuspendThread(t->handle);

        CONTEXT ctx = {0};
        ctx.ContextFlags = CONTEXT_FULL;
        BOOL ok = GetThreadContext(t->handle, &ctx);

        if (suspended)
            ResumeThread(t->handle);

        char location[512] = "";

        if (ok)
        {
            char sym_buffer[sizeof(SYMBOL_INFO) + 256] = {0};
            PSYMBOL_INFO sym = (PSYMBOL_INFO)sym_buffer;
            sym->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym->MaxNameLen = 255;

            char symbol_name[300] = "???";
            DWORD64 disp = 0;

            if (SymFromAddr(dbg->sym_handle, ctx.Rip, &disp, sym))
            {
                snprintf(symbol_name, sizeof(symbol_name),
                    "%s+0x%llx", sym->Name, disp);
            }

            IMAGEHLP_LINE64 line = {0};
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD line_disp = 0;

            char line_info[512] = "";

            if (SymGetLineFromAddr64(dbg->sym_handle, ctx.Rip, &line_disp, &line))
            {
                snprintf(line_info, sizeof(line_info),
                    " at %s:%lu", line.FileName, line.LineNumber);
            }

            snprintf(location, sizeof(location),
                "  0x%llx in %s%s", ctx.Rip, symbol_name, line_info);
        }

        printf("  tid=%lu%s%s\n", t->tid, location, tag);
    }
}
