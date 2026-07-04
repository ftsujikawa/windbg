#include <stdio.h>
#include "breakpoints.h"

breakpoint_t *find_breakpoint(debugger_t *dbg, void *addr)
{
    for (int i = 0; i < dbg->breakpoint_count; i++)
    {
        if (dbg->breakpoints[i].addr == addr)
            return &dbg->breakpoints[i];
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

    if (dbg->breakpoint_count >= MAX_BREAKPOINTS)
    {
        printf("too many breakpoints\n");
        return -1;
    }

    ReadProcessMemory(
        dbg->process,
        addr,
        &dbg->breakpoints[dbg->breakpoint_count].original_byte,
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

    dbg->breakpoints[dbg->breakpoint_count].addr = addr;
    dbg->breakpoint_count++;

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