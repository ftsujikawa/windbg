#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processes.h"
#include "leakcheck.h"

static void free_thread_list(thread_entry_t *t)
{
    while (t != NULL)
    {
        thread_entry_t *next = t->next;
        free(t);
        t = next;
    }
}

static void free_breakpoint_list(breakpoint_t *bp)
{
    while (bp != NULL)
    {
        breakpoint_t *next = bp->next;
        free(bp);
        bp = next;
    }
}

static void free_alloc_list(alloc_info_t *a)
{
    while (a != NULL)
    {
        alloc_info_t *next = a->next;
        free(a);
        a = next;
    }
}

/* Copy debugger_t's per-process fields into a process_entry_t (backgrounding
 * the currently active process). */
static void save_active_into(debugger_t *dbg, process_entry_t *dst)
{
    dst->pid = dbg->pid;
    dst->process = dbg->process;
    strncpy_s(dst->image_path, sizeof(dst->image_path), dbg->image_path, _TRUNCATE);

    dst->sym_handle = dbg->sym_handle;
    dst->symbols_initialized = dbg->symbols_initialized;

    dst->threads = dbg->threads;

    dst->breakpoints = dbg->breakpoints;
    dst->breakpoint_addr = dbg->breakpoint_addr;
    dst->original_byte = dbg->original_byte;

    dst->temp_bp_addr = dbg->temp_bp_addr;
    dst->temp_bp_byte = dbg->temp_bp_byte;
    dst->bp_rearm_addr = dbg->bp_rearm_addr;
    dst->bp_rearm_byte = dbg->bp_rearm_byte;
    dst->si_requested = dbg->si_requested;

    dst->next_mode = dbg->next_mode;
    dst->next_line = dbg->next_line;
    dst->next_rsp = dbg->next_rsp;
    dst->step_mode = dbg->step_mode;
    dst->step_line = dbg->step_line;

    strncpy_s(dst->list_file, sizeof(dst->list_file), dbg->list_file, _TRUNCATE);
    dst->list_next_line = dbg->list_next_line;

    dst->leak_tracking = dbg->leak_tracking;
    dst->malloc_addr = dbg->malloc_addr;
    dst->malloc_orig_byte = dbg->malloc_orig_byte;
    dst->calloc_addr = dbg->calloc_addr;
    dst->calloc_orig_byte = dbg->calloc_orig_byte;
    dst->realloc_addr = dbg->realloc_addr;
    dst->realloc_orig_byte = dbg->realloc_orig_byte;
    dst->free_addr = dbg->free_addr;
    dst->free_orig_byte = dbg->free_orig_byte;
    dst->malloc_ret_addr = dbg->malloc_ret_addr;
    dst->malloc_ret_byte = dbg->malloc_ret_byte;
    dst->pending_alloc_size = dbg->pending_alloc_size;
    strncpy_s(dst->current_alloc_func, sizeof(dst->current_alloc_func), dbg->current_alloc_func, _TRUNCATE);
    dst->leak_rearm_addr = dbg->leak_rearm_addr;
    dst->leak_rearm_byte = dbg->leak_rearm_byte;
    dst->allocations = dbg->allocations;

    memcpy(dst->watchpoints, dbg->watchpoints, sizeof(dst->watchpoints));
    dst->watch_count = dbg->watch_count;
}

/* Copy a process_entry_t's saved fields onto debugger_t (activating it).
 * dbg->tid/dbg->thread are left cleared -- the caller resolves focus within
 * the newly active process (EXCEPTION_DEBUG_EVENT resolves it right after
 * calling switch_active_process; 'process <pid>' sets it explicitly). */
static void load_active_from(debugger_t *dbg, process_entry_t *src)
{
    dbg->pid = src->pid;
    dbg->process = src->process;
    strncpy_s(dbg->image_path, sizeof(dbg->image_path), src->image_path, _TRUNCATE);

    dbg->sym_handle = src->sym_handle;
    dbg->symbols_initialized = src->symbols_initialized;

    dbg->threads = src->threads;

    dbg->breakpoints = src->breakpoints;
    dbg->breakpoint_addr = src->breakpoint_addr;
    dbg->original_byte = src->original_byte;

    dbg->temp_bp_addr = src->temp_bp_addr;
    dbg->temp_bp_byte = src->temp_bp_byte;
    dbg->bp_rearm_addr = src->bp_rearm_addr;
    dbg->bp_rearm_byte = src->bp_rearm_byte;
    dbg->si_requested = src->si_requested;

    dbg->next_mode = src->next_mode;
    dbg->next_line = src->next_line;
    dbg->next_rsp = src->next_rsp;
    dbg->step_mode = src->step_mode;
    dbg->step_line = src->step_line;

    strncpy_s(dbg->list_file, sizeof(dbg->list_file), src->list_file, _TRUNCATE);
    dbg->list_next_line = src->list_next_line;

    dbg->leak_tracking = src->leak_tracking;
    dbg->malloc_addr = src->malloc_addr;
    dbg->malloc_orig_byte = src->malloc_orig_byte;
    dbg->calloc_addr = src->calloc_addr;
    dbg->calloc_orig_byte = src->calloc_orig_byte;
    dbg->realloc_addr = src->realloc_addr;
    dbg->realloc_orig_byte = src->realloc_orig_byte;
    dbg->free_addr = src->free_addr;
    dbg->free_orig_byte = src->free_orig_byte;
    dbg->malloc_ret_addr = src->malloc_ret_addr;
    dbg->malloc_ret_byte = src->malloc_ret_byte;
    dbg->pending_alloc_size = src->pending_alloc_size;
    strncpy_s(dbg->current_alloc_func, sizeof(dbg->current_alloc_func), src->current_alloc_func, _TRUNCATE);
    dbg->leak_rearm_addr = src->leak_rearm_addr;
    dbg->leak_rearm_byte = src->leak_rearm_byte;
    dbg->allocations = src->allocations;

    memcpy(dbg->watchpoints, src->watchpoints, sizeof(dbg->watchpoints));
    dbg->watch_count = src->watch_count;

    dbg->tid = 0;
    dbg->thread = NULL;
}

void register_new_process(debugger_t *dbg, DWORD pid, HANDLE handle, const char *image_path)
{
    process_entry_t *p = (process_entry_t*)calloc(1, sizeof(process_entry_t));
    if (p == NULL)
        return;

    p->pid = pid;
    p->process = handle;
    if (image_path)
        strncpy_s(p->image_path, sizeof(p->image_path), image_path, _TRUNCATE);

    p->next = dbg->processes;
    dbg->processes = p;
}

int switch_active_process(debugger_t *dbg, DWORD pid)
{
    if (pid == dbg->pid)
        return 1;

    process_entry_t **pp = &dbg->processes;
    process_entry_t *target = NULL;
    while (*pp != NULL)
    {
        if ((*pp)->pid == pid)
        {
            target = *pp;
            *pp = target->next;
            break;
        }
        pp = &(*pp)->next;
    }
    if (target == NULL)
        return 0;

    process_entry_t *saved = (process_entry_t*)calloc(1, sizeof(process_entry_t));
    if (saved == NULL)
    {
        target->next = dbg->processes;
        dbg->processes = target;
        return 0;
    }

    save_active_into(dbg, saved);
    saved->next = dbg->processes;
    dbg->processes = saved;

    load_active_from(dbg, target);
    free(target);

    return 1;
}

int unregister_process(debugger_t *dbg, DWORD pid, DWORD exit_code)
{
    if (pid != dbg->pid)
    {
        if (!switch_active_process(dbg, pid))
        {
            /* unknown pid -- shouldn't happen; report and leave state alone */
            printf("process exited id=%lu exit code=%lu\n", pid, exit_code);
            return (dbg->pid != 0);
        }
    }

    printf("process exited id=%lu exit code=%lu\n", pid, exit_code);

    if (dbg->leak_tracking)
        print_leaks(dbg);

    if (dbg->locked_tid != 0)
    {
        for (thread_entry_t *t = dbg->threads; t != NULL; t = t->next)
        {
            if (t->tid == dbg->locked_tid)
            {
                dbg->locked_tid = 0;
                break;
            }
        }
    }

    free_breakpoint_list(dbg->breakpoints);
    dbg->breakpoints = NULL;
    free_alloc_list(dbg->allocations);
    dbg->allocations = NULL;
    free_thread_list(dbg->threads);
    dbg->threads = NULL;

    if (dbg->processes == NULL)
    {
        dbg->pid = 0;
        dbg->process = NULL;
        dbg->tid = 0;
        dbg->thread = NULL;
        return 0;
    }

    process_entry_t *next_active = dbg->processes;
    dbg->processes = next_active->next;
    load_active_from(dbg, next_active);
    free(next_active);
    return 1;
}

HANDLE find_process_handle(debugger_t *dbg, DWORD pid)
{
    if (pid == dbg->pid)
        return dbg->process;
    for (process_entry_t *p = dbg->processes; p != NULL; p = p->next)
        if (p->pid == pid)
            return p->process;
    return NULL;
}

process_entry_t *find_background_process(debugger_t *dbg, DWORD pid)
{
    for (process_entry_t *p = dbg->processes; p != NULL; p = p->next)
        if (p->pid == pid)
            return p;
    return NULL;
}

void print_processes(debugger_t *dbg)
{
    if (dbg->pid == 0)
    {
        printf("no processes\n");
        return;
    }

    printf("  pid=%lu%s%s  (current)\n", dbg->pid,
           dbg->image_path[0] ? "  " : "", dbg->image_path);

    for (process_entry_t *p = dbg->processes; p != NULL; p = p->next)
        printf("  pid=%lu%s%s\n", p->pid, p->image_path[0] ? "  " : "", p->image_path);
}

void free_all_processes(debugger_t *dbg)
{
    process_entry_t *p = dbg->processes;
    while (p != NULL)
    {
        process_entry_t *next = p->next;
        free_breakpoint_list(p->breakpoints);
        free_alloc_list(p->allocations);
        free_thread_list(p->threads);
        free(p);
        p = next;
    }
    dbg->processes = NULL;
}
