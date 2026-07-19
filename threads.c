#include <stdio.h>
#include <stdlib.h>
#include "threads.h"

void register_thread(debugger_t *dbg, DWORD tid, HANDLE handle)
{
    thread_entry_t *t = (thread_entry_t*)malloc(sizeof(thread_entry_t));
    if (t == NULL)
        return;

    t->tid = tid;
    t->handle = handle;
    t->next = dbg->threads;
    dbg->threads = t;
}

void unregister_thread(debugger_t *dbg, DWORD tid)
{
    thread_entry_t **pp = &dbg->threads;
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

        printf("  tid=%lu%s\n", t->tid, tag);
    }
}
