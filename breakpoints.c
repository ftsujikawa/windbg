#include <stdio.h>
#include "breakpoints.h"

int set_breakpoint(debugger_t *dbg, void *addr)
{
    SIZE_T n;

    ReadProcessMemory(
        dbg->process,
        addr,
        &dbg->original_byte,
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

    dbg->breakpoint_addr = addr;

    printf("breakpoint set at %p\n", addr);

    return 0;
}

int remove_breakpoint(debugger_t *dbg)
{
    SIZE_T n;

    WriteProcessMemory(
        dbg->process,
        dbg->breakpoint_addr,
        &dbg->original_byte,
        1,
        &n
    );

    FlushInstructionCache(
        dbg->process,
        dbg->breakpoint_addr,
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