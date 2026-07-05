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

int set_register(debugger_t *dbg, const char *name, long long value)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    DWORD64 v = (DWORD64)value;

    if      (_stricmp(name, "rax") == 0) ctx.Rax = v;
    else if (_stricmp(name, "rbx") == 0) ctx.Rbx = v;
    else if (_stricmp(name, "rcx") == 0) ctx.Rcx = v;
    else if (_stricmp(name, "rdx") == 0) ctx.Rdx = v;
    else if (_stricmp(name, "rsi") == 0) ctx.Rsi = v;
    else if (_stricmp(name, "rdi") == 0) ctx.Rdi = v;
    else if (_stricmp(name, "rbp") == 0) ctx.Rbp = v;
    else if (_stricmp(name, "rsp") == 0) ctx.Rsp = v;
    else if (_stricmp(name, "rip") == 0) ctx.Rip = v;
    else if (_stricmp(name, "r8")  == 0) ctx.R8  = v;
    else if (_stricmp(name, "r9")  == 0) ctx.R9  = v;
    else if (_stricmp(name, "r10") == 0) ctx.R10 = v;
    else if (_stricmp(name, "r11") == 0) ctx.R11 = v;
    else if (_stricmp(name, "r12") == 0) ctx.R12 = v;
    else if (_stricmp(name, "r13") == 0) ctx.R13 = v;
    else if (_stricmp(name, "r14") == 0) ctx.R14 = v;
    else if (_stricmp(name, "r15") == 0) ctx.R15 = v;
    /* 32-bit aliases */
    else if (_stricmp(name, "eax") == 0) ctx.Rax = (ctx.Rax & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "ebx") == 0) ctx.Rbx = (ctx.Rbx & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "ecx") == 0) ctx.Rcx = (ctx.Rcx & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "edx") == 0) ctx.Rdx = (ctx.Rdx & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "esi") == 0) ctx.Rsi = (ctx.Rsi & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "edi") == 0) ctx.Rdi = (ctx.Rdi & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "ebp") == 0) ctx.Rbp = (ctx.Rbp & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "esp") == 0) ctx.Rsp = (ctx.Rsp & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "eip") == 0) ctx.Rip = (ctx.Rip & 0xffffffff00000000ULL) | (v & 0xffffffff);
    else if (_stricmp(name, "eflags") == 0) ctx.EFlags = (DWORD)v;
    else return 0; /* unknown register */

    SetThreadContext(dbg->thread, &ctx);
    printf("%s = 0x%llx\n", name, (unsigned long long)v);
    return 1;
}