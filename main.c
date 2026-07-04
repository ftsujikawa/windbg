#include <stdio.h>
#include "debugger.h"

int main(int argc, char *argv[])
{
    debugger_t dbg = {0};

    if (argc != 2)
    {
        printf("usage: windbg target.exe\n");
        return 1;
    }

    printf("target: [%s]\n", argv[1]);

    if (debugger_start(&dbg, argv[1]) < 0)
        return 1;

    debugger_loop(&dbg);

    return 0;
}