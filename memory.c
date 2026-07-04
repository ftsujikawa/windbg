#include <stdio.h>
#include "memory.h"

void examine_memory(debugger_t *dbg, void *addr)
{
    BYTE buf[16];
    SIZE_T n;

    ReadProcessMemory(
        dbg->process,
        addr,
        buf,
        16,
        &n
    );

    for(int i=0;i<16;i++)
        printf("%02x ", buf[i]);

    printf("\n");
}