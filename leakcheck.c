#include <stdio.h>
#include <stdlib.h>
#include "leakcheck.h"
#include "symbols.h"

int leak_tracking_enable(debugger_t *dbg)
{
    if (dbg->leak_tracking)
    {
        printf("leak tracking is already on\n");
        return 0;
    }

    void *malloc_addr = lookup_symbol(dbg, "malloc");
    void *calloc_addr = lookup_symbol(dbg, "calloc");
    void *realloc_addr = lookup_symbol(dbg, "realloc");
    void *free_addr = lookup_symbol(dbg, "free");

    if (!malloc_addr || !free_addr)
    {
        printf("cannot enable leak tracking: 'malloc'/'free' symbols not "
               "found (target must statically link the CRT, e.g. /MT or /MTd)\n");
        return -1;
    }

    SIZE_T n;
    BYTE int3 = 0xCC;

    ReadProcessMemory(dbg->process, malloc_addr, &dbg->malloc_orig_byte, 1, &n);
    WriteProcessMemory(dbg->process, malloc_addr, &int3, 1, &n);
    FlushInstructionCache(dbg->process, malloc_addr, 1);

    if (calloc_addr)
    {
        ReadProcessMemory(dbg->process, calloc_addr, &dbg->calloc_orig_byte, 1, &n);
        WriteProcessMemory(dbg->process, calloc_addr, &int3, 1, &n);
        FlushInstructionCache(dbg->process, calloc_addr, 1);
    }

    if (realloc_addr)
    {
        ReadProcessMemory(dbg->process, realloc_addr, &dbg->realloc_orig_byte, 1, &n);
        WriteProcessMemory(dbg->process, realloc_addr, &int3, 1, &n);
        FlushInstructionCache(dbg->process, realloc_addr, 1);
    }

    ReadProcessMemory(dbg->process, free_addr, &dbg->free_orig_byte, 1, &n);
    WriteProcessMemory(dbg->process, free_addr, &int3, 1, &n);
    FlushInstructionCache(dbg->process, free_addr, 1);

    dbg->malloc_addr = malloc_addr;
    dbg->calloc_addr = calloc_addr;
    dbg->realloc_addr = realloc_addr;
    dbg->free_addr = free_addr;
    dbg->leak_tracking = 1;

    printf("leak tracking enabled (malloc=%p", malloc_addr);
    if (calloc_addr) printf(" calloc=%p", calloc_addr);
    if (realloc_addr) printf(" realloc=%p", realloc_addr);
    printf(" free=%p)\n", free_addr);

    return 0;
}

void leak_tracking_disable(debugger_t *dbg)
{
    if (!dbg->leak_tracking)
    {
        printf("leak tracking is already off\n");
        return;
    }

    SIZE_T n;

    WriteProcessMemory(dbg->process, dbg->malloc_addr, &dbg->malloc_orig_byte, 1, &n);
    FlushInstructionCache(dbg->process, dbg->malloc_addr, 1);

    if (dbg->calloc_addr)
    {
        WriteProcessMemory(dbg->process, dbg->calloc_addr, &dbg->calloc_orig_byte, 1, &n);
        FlushInstructionCache(dbg->process, dbg->calloc_addr, 1);
    }

    if (dbg->realloc_addr)
    {
        WriteProcessMemory(dbg->process, dbg->realloc_addr, &dbg->realloc_orig_byte, 1, &n);
        FlushInstructionCache(dbg->process, dbg->realloc_addr, 1);
    }

    WriteProcessMemory(dbg->process, dbg->free_addr, &dbg->free_orig_byte, 1, &n);
    FlushInstructionCache(dbg->process, dbg->free_addr, 1);

    if (dbg->malloc_ret_addr != NULL)
    {
        WriteProcessMemory(dbg->process, dbg->malloc_ret_addr,
            &dbg->malloc_ret_byte, 1, &n);
        FlushInstructionCache(dbg->process, dbg->malloc_ret_addr, 1);
        dbg->malloc_ret_addr = NULL;
    }

    dbg->leak_rearm_addr = NULL;

    alloc_info_t *a = dbg->allocations;
    while (a != NULL)
    {
        alloc_info_t *next = a->next;
        free(a);
        a = next;
    }
    dbg->allocations = NULL;

    dbg->malloc_addr = NULL;
    dbg->calloc_addr = NULL;
    dbg->realloc_addr = NULL;
    dbg->free_addr = NULL;
    dbg->leak_tracking = 0;

    printf("leak tracking disabled\n");
}

/* Restore the real first byte of a malloc/free-entry breakpoint and arm the
 * trap flag so exactly that one original instruction executes; debugger.c's
 * STATUS_SINGLE_STEP handling rewrites the 0xCC once that step completes.
 * This is what lets the same entry point fire on every call instead of once. */
static void rearm_after_one_step(debugger_t *dbg, void *addr, BYTE orig_byte)
{
    SIZE_T n;
    WriteProcessMemory(dbg->process, addr, &orig_byte, 1, &n);
    FlushInstructionCache(dbg->process, addr, 1);

    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);
    ctx.EFlags |= 0x100;
    SetThreadContext(dbg->thread, &ctx);

    dbg->leak_rearm_addr = addr;
    dbg->leak_rearm_byte = orig_byte;
}

void leak_on_alloc_enter(debugger_t *dbg, void *addr, const char *func_name)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    /* x64 Microsoft ABI: size_t size arrives in RCX */
    /* For calloc: RCX=count, RDX=size */
    /* For realloc: RCX=ptr, RDX=size */
    dbg->pending_alloc_size = (size_t)ctx.Rcx;
    if (strcmp(func_name, "calloc") == 0)
    {
        dbg->pending_alloc_size *= (size_t)ctx.Rdx;  /* count * size */
    }
    
    strcpy(dbg->current_alloc_func, func_name);

    DWORD64 ret_addr = 0;
    SIZE_T n;
    ReadProcessMemory(dbg->process, (void*)ctx.Rsp, &ret_addr, sizeof(ret_addr), &n);

    if (dbg->malloc_ret_addr != NULL)
    {
        /* stale return breakpoint from a call that never returned; restore
           it before reusing the single tracking slot */
        WriteProcessMemory(dbg->process, dbg->malloc_ret_addr,
            &dbg->malloc_ret_byte, 1, &n);
        FlushInstructionCache(dbg->process, dbg->malloc_ret_addr, 1);
    }

    BYTE orig;
    ReadProcessMemory(dbg->process, (void*)ret_addr, &orig, 1, &n);

    BYTE int3 = 0xCC;
    WriteProcessMemory(dbg->process, (void*)ret_addr, &int3, 1, &n);
    FlushInstructionCache(dbg->process, (void*)ret_addr, 1);

    dbg->malloc_ret_addr = (void*)ret_addr;
    dbg->malloc_ret_byte = orig;

    BYTE orig_byte;
    if (strcmp(func_name, "malloc") == 0)
        orig_byte = dbg->malloc_orig_byte;
    else if (strcmp(func_name, "calloc") == 0)
        orig_byte = dbg->calloc_orig_byte;
    else if (strcmp(func_name, "realloc") == 0)
        orig_byte = dbg->realloc_orig_byte;
    else
        orig_byte = dbg->malloc_orig_byte;  /* fallback */

    rearm_after_one_step(dbg, addr, orig_byte);
}

void leak_on_malloc_enter(debugger_t *dbg)
{
    leak_on_alloc_enter(dbg, dbg->malloc_addr, "malloc");
}

void leak_on_calloc_enter(debugger_t *dbg)
{
    if (dbg->calloc_addr)
        leak_on_alloc_enter(dbg, dbg->calloc_addr, "calloc");
}

void leak_on_realloc_enter(debugger_t *dbg)
{
    if (dbg->realloc_addr)
        leak_on_alloc_enter(dbg, dbg->realloc_addr, "realloc");
}

void leak_on_malloc_return(debugger_t *dbg)
{
    SIZE_T n;

    WriteProcessMemory(dbg->process, dbg->malloc_ret_addr,
        &dbg->malloc_ret_byte, 1, &n);
    FlushInstructionCache(dbg->process, dbg->malloc_ret_addr, 1);
    dbg->malloc_ret_addr = NULL;

    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    void *ptr = (void*)ctx.Rax;
    if (ptr == NULL)
        return;

    alloc_info_t *a = (alloc_info_t*)malloc(sizeof(alloc_info_t));
    if (!a)
        return;

    a->addr = ptr;
    a->size = dbg->pending_alloc_size;
    a->caller = ctx.Rip;
    strcpy(a->alloc_func, dbg->current_alloc_func);
    a->next = dbg->allocations;
    dbg->allocations = a;
}

void leak_on_free_enter(debugger_t *dbg)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    /* x64 Microsoft ABI: void *ptr arrives in RCX */
    void *ptr = (void*)ctx.Rcx;
    if (ptr != NULL)
    {
        alloc_info_t **pp = &dbg->allocations;
        while (*pp != NULL)
        {
            if ((*pp)->addr == ptr)
            {
                alloc_info_t *dead = *pp;
                *pp = dead->next;
                free(dead);
                break;
            }
            pp = &(*pp)->next;
        }
    }

    rearm_after_one_step(dbg, dbg->free_addr, dbg->free_orig_byte);
}

int leak_rearm_pending(debugger_t *dbg)
{
    return dbg->leak_rearm_addr != NULL;
}

void leak_finish_rearm(debugger_t *dbg)
{
    SIZE_T n;
    BYTE int3 = 0xCC;

    WriteProcessMemory(dbg->process, dbg->leak_rearm_addr, &int3, 1, &n);
    FlushInstructionCache(dbg->process, dbg->leak_rearm_addr, 1);
    dbg->leak_rearm_addr = NULL;
}

void print_leaks(debugger_t *dbg)
{
    if (!dbg->leak_tracking)
    {
        printf("leak tracking is off (use 'leak on' first)\n");
        return;
    }

    if (dbg->allocations == NULL)
    {
        printf("no leaks detected\n");
        return;
    }

    int count = 0;
    size_t total = 0;
    int i = 1;

    for (alloc_info_t *a = dbg->allocations; a != NULL; a = a->next, i++)
    {
        char sym_buffer[sizeof(SYMBOL_INFO) + 256] = {0};
        PSYMBOL_INFO sym = (PSYMBOL_INFO)sym_buffer;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;

        char symbol_name[300] = "???";
        DWORD64 disp = 0;

        if (SymFromAddr(dbg->sym_handle, a->caller, &disp, sym))
        {
            snprintf(symbol_name, sizeof(symbol_name),
                "%s+0x%llx", sym->Name, disp);
        }

        IMAGEHLP_LINE64 line = {0};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD line_disp = 0;

        char line_info[512] = "";

        if (SymGetLineFromAddr64(dbg->sym_handle, a->caller, &line_disp, &line))
        {
            snprintf(line_info, sizeof(line_info),
                " at %s:%lu", line.FileName, line.LineNumber);
        }

        printf("#%d  %p  %zu bytes  allocated by %s() %s%s\n",
            i, a->addr, a->size, a->alloc_func, symbol_name, line_info);

        count++;
        total += a->size;
    }

    printf("%d leak(s), %zu byte(s) total\n", count, total);
}
