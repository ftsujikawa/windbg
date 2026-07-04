#include <stdio.h>
#include "registers.h"

void show_registers(debugger_t *dbg)
{
    CONTEXT ctx = {0};

    ctx.ContextFlags = CONTEXT_FULL;

    GetThreadContext(dbg->thread, &ctx);

#ifdef _WIN64

    printf("RIP=%llx\n", ctx.Rip);
    printf("RAX=%llx\n", ctx.Rax);
    printf("RBX=%llx\n", ctx.Rbx);
    printf("RCX=%llx\n", ctx.Rcx);
    printf("RDX=%llx\n", ctx.Rdx);
    printf("RSP=%llx\n", ctx.Rsp);

#else

    printf("EIP=%lx\n", ctx.Eip);
    printf("EAX=%lx\n", ctx.Eax);
    printf("EBX=%lx\n", ctx.Ebx);
    printf("ECX=%lx\n", ctx.Ecx);
    printf("EDX=%lx\n", ctx.Edx);
    printf("ESP=%lx\n", ctx.Esp);

#endif
}

void single_step(debugger_t *dbg)
{
    CONTEXT ctx = {0};

    ctx.ContextFlags = CONTEXT_FULL;

    GetThreadContext(dbg->thread, &ctx);

    ctx.EFlags |= 0x100;

    SetThreadContext(dbg->thread, &ctx);
}