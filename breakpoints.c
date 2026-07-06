#include <stdio.h>
#include <stdlib.h>
#include "breakpoints.h"

breakpoint_t *find_breakpoint(debugger_t *dbg, void *addr)
{
    for (breakpoint_t *bp = dbg->breakpoints; bp != NULL; bp = bp->next)
    {
        if (bp->addr == addr)
            return bp;
    }
    return NULL;
}

int set_breakpoint(debugger_t *dbg, void *addr)
{
    SIZE_T n;

    if (find_breakpoint(dbg, addr) != NULL)
    {
        printf("breakpoint already set at %p\n", addr);
        return 0;
    }

    breakpoint_t *bp = (breakpoint_t*)malloc(sizeof(breakpoint_t));
    if (bp == NULL)
    {
        printf("out of memory\n");
        return -1;
    }

    ReadProcessMemory(
        dbg->process,
        addr,
        &bp->original_byte,
        1,
        &n
    );

    BYTE int3 = 0xCC;

    WriteProcessMemory(
        dbg->process,
        addr,
        &int3,
        1,
        &n
    );

    FlushInstructionCache(
        dbg->process,
        addr,
        1
    );

    bp->addr = addr;
    bp->next = dbg->breakpoints;
    dbg->breakpoints = bp;

    printf("breakpoint set at %p\n", addr);

    return 0;
}

int remove_breakpoint(debugger_t *dbg)
{
    (void)dbg;
    return 0;
}

int remove_breakpoint_at(debugger_t *dbg, void *addr)
{
    SIZE_T n;

    breakpoint_t *bp = find_breakpoint(dbg, addr);
    if (bp == NULL)
        return -1;

    WriteProcessMemory(
        dbg->process,
        addr,
        &bp->original_byte,
        1,
        &n
    );

    FlushInstructionCache(
        dbg->process,
        addr,
        1
    );

    return 0;
}

int set_temp_breakpoint(debugger_t *dbg, void *addr)
{
    SIZE_T n;

    ReadProcessMemory(
        dbg->process,
        addr,
        &dbg->temp_bp_byte,
        1,
        &n
    );

    BYTE int3 = 0xCC;

    WriteProcessMemory(
        dbg->process,
        addr,
        &int3,
        1,
        &n
    );

    FlushInstructionCache(
        dbg->process,
        addr,
        1
    );

    dbg->temp_bp_addr = addr;

    return 0;
}

int remove_temp_breakpoint(debugger_t *dbg)
{
    SIZE_T n;

    if (dbg->temp_bp_addr == NULL)
        return 0;

    WriteProcessMemory(
        dbg->process,
        dbg->temp_bp_addr,
        &dbg->temp_bp_byte,
        1,
        &n
    );

    FlushInstructionCache(
        dbg->process,
        dbg->temp_bp_addr,
        1
    );

    dbg->temp_bp_addr = NULL;

    return 0;
}

void print_breakpoints(debugger_t *dbg)
{
    if (dbg->breakpoints == NULL)
    {
        printf("no breakpoints set\n");
        return;
    }

    int i = 1;

    for (breakpoint_t *bp = dbg->breakpoints; bp != NULL; bp = bp->next, i++)
    {
        DWORD64 addr = (DWORD64)bp->addr;

        char sym_buffer[sizeof(SYMBOL_INFO) + 256] = {0};
        PSYMBOL_INFO sym = (PSYMBOL_INFO)sym_buffer;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;

        char symbol_name[300] = "???";
        DWORD64 disp = 0;

        if (SymFromAddr(dbg->sym_handle, addr, &disp, sym))
        {
            snprintf(symbol_name, sizeof(symbol_name),
                "%s+0x%llx", sym->Name, disp);
        }

        IMAGEHLP_LINE64 line = {0};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD line_disp = 0;

        char line_info[512] = "";

        if (SymGetLineFromAddr64(dbg->sym_handle, addr, &line_disp, &line))
        {
            snprintf(line_info, sizeof(line_info),
                " at %s:%lu", line.FileName, line.LineNumber);
        }

        printf("#%d  0x%llx  in %s%s\n", i, addr, symbol_name, line_info);
    }
}

/* A permanent breakpoint fires by removing its 0xCC so the debuggee can
 * execute past it; without rearming it, it would only ever fire once. This
 * restores the byte for exactly one instruction (via the trap flag) and
 * remembers to rewrite 0xCC once that single-step completes (see
 * finish_breakpoint_rearm, invoked from debugger.c's STATUS_SINGLE_STEP
 * handling), so it fires again on a later call -- e.g. a recursive function
 * or a breakpoint inside a loop. */
void arm_breakpoint_rearm(debugger_t *dbg, void *addr, BYTE orig_byte)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);
    ctx.EFlags |= 0x100;
    SetThreadContext(dbg->thread, &ctx);

    dbg->bp_rearm_addr = addr;
    dbg->bp_rearm_byte = orig_byte;
}

int breakpoint_rearm_pending(debugger_t *dbg)
{
    return dbg->bp_rearm_addr != NULL;
}

void finish_breakpoint_rearm(debugger_t *dbg)
{
    SIZE_T n;
    BYTE int3 = 0xCC;

    WriteProcessMemory(dbg->process, dbg->bp_rearm_addr, &int3, 1, &n);
    FlushInstructionCache(dbg->process, dbg->bp_rearm_addr, 1);
    dbg->bp_rearm_addr = NULL;
}