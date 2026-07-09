#include <stdio.h>
#include <string.h>
#include "registers.h"

#ifdef _WIN64
static void print_m128a(const char *name, const M128A *v)
{
    printf("%s=%016llx%016llx\n", name, v->High, v->Low);
}

static void show_fp_registers(debugger_t *dbg)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FLOATING_POINT;
    if (!GetThreadContext(dbg->thread, &ctx))
        return;

    printf("\n[x87 FPU Registers]\n");
    for (int i = 0; i < 8; i++) {
        printf("  ST%d=%04llx%016llx\n", i,
               ctx.FltSave.FloatRegisters[i].High & 0xffff,
               ctx.FltSave.FloatRegisters[i].Low);
    }

    printf("\n[XMM Registers]\n");
    print_m128a("  XMM0", &ctx.FltSave.XmmRegisters[0]);
    print_m128a("  XMM1", &ctx.FltSave.XmmRegisters[1]);
    print_m128a("  XMM2", &ctx.FltSave.XmmRegisters[2]);
    print_m128a("  XMM3", &ctx.FltSave.XmmRegisters[3]);
    print_m128a("  XMM4", &ctx.FltSave.XmmRegisters[4]);
    print_m128a("  XMM5", &ctx.FltSave.XmmRegisters[5]);
    print_m128a("  XMM6", &ctx.FltSave.XmmRegisters[6]);
    print_m128a("  XMM7", &ctx.FltSave.XmmRegisters[7]);
    print_m128a("  XMM8", &ctx.FltSave.XmmRegisters[8]);
    print_m128a("  XMM9", &ctx.FltSave.XmmRegisters[9]);
    print_m128a("  XMM10", &ctx.FltSave.XmmRegisters[10]);
    print_m128a("  XMM11", &ctx.FltSave.XmmRegisters[11]);
    print_m128a("  XMM12", &ctx.FltSave.XmmRegisters[12]);
    print_m128a("  XMM13", &ctx.FltSave.XmmRegisters[13]);
    print_m128a("  XMM14", &ctx.FltSave.XmmRegisters[14]);
    print_m128a("  XMM15", &ctx.FltSave.XmmRegisters[15]);
}
#endif

void show_registers(debugger_t *dbg)
{
    CONTEXT ctx = {0};

    ctx.ContextFlags = CONTEXT_FULL;

    GetThreadContext(dbg->thread, &ctx);

#ifdef _WIN64

    printf("[General Purpose Registers]\n");
    printf("  RIP=%016llx  RAX=%016llx\n", ctx.Rip, ctx.Rax);
    printf("  RBX=%016llx  RCX=%016llx\n", ctx.Rbx, ctx.Rcx);
    printf("  RDX=%016llx  RSI=%016llx\n", ctx.Rdx, ctx.Rsi);
    printf("  RDI=%016llx  RBP=%016llx\n", ctx.Rdi, ctx.Rbp);
    printf("  RSP=%016llx  R8 =%016llx\n", ctx.Rsp, ctx.R8);
    printf("  R9 =%016llx  R10=%016llx\n", ctx.R9, ctx.R10);
    printf("  R11=%016llx  R12=%016llx\n", ctx.R11, ctx.R12);
    printf("  R13=%016llx  R14=%016llx\n", ctx.R13, ctx.R14);
    printf("  R15=%016llx  RFL=%016llx\n", ctx.R15, (unsigned long long)ctx.EFlags);

    show_fp_registers(dbg);

#else

    printf("[General Purpose Registers]\n");
    printf("  EIP=%08lx  EAX=%08lx\n", ctx.Eip, ctx.Eax);
    printf("  EBX=%08lx  ECX=%08lx\n", ctx.Ebx, ctx.Ecx);
    printf("  EDX=%08lx  ESP=%08lx\n", ctx.Edx, ctx.Esp);

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

#ifdef _WIN64
static DWORD64 double_bits(double d)
{
    DWORD64 bits;
    memcpy(&bits, &d, sizeof(bits));
    return bits;
}

static void double_to_x87(double d, M128A *out)
{
    DWORD64 bits = double_bits(d);
    int sign = (int)((bits >> 63) & 1);
    int exponent = (int)((bits >> 52) & 0x7FF);
    DWORD64 mantissa = bits & 0xFFFFFFFFFFFFFLL;

    if (exponent == 0) {
        if (mantissa == 0) {
            /* zero */
            out->Low = 0;
            out->High = sign ? 0x8000ULL : 0ULL;
        } else {
            /* subnormal: effective exponent is -1022 */
            out->Low = mantissa << 11;
            out->High = (sign ? 0x8000ULL : 0ULL) | (DWORD64)(1 - 1023 + 16383);
        }
    } else if (exponent == 0x7FF) {
        /* infinity or NaN */
        out->Low = mantissa ? 0xC000000000000000ULL : 0x8000000000000000ULL;
        out->High = (sign ? 0x8000ULL : 0ULL) | 0x7FFFULL;
    } else {
        /* normalized */
        DWORD64 x87_exp = (DWORD64)(exponent - 1023 + 16383);
        DWORD64 x87_mant = (1ULL << 63) | (mantissa << 11);
        out->Low = x87_mant;
        out->High = ((DWORD64)sign << 15) | x87_exp;
    }
}
#endif

int set_register(debugger_t *dbg, const char *name, long long value, double fvalue, int is_float)
{
    CONTEXT ctx = {0};
#ifdef _WIN64
    ctx.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
#else
    ctx.ContextFlags = CONTEXT_FULL;
#endif
    if (!GetThreadContext(dbg->thread, &ctx))
        return 0;

    DWORD64 v = (DWORD64)value;
    int handled = 1;

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
#ifdef _WIN64
    else if (_stricmp(name, "xmm0") == 0) { ctx.FltSave.XmmRegisters[0].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[0].High  = 0; }
    else if (_stricmp(name, "xmm1") == 0) { ctx.FltSave.XmmRegisters[1].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[1].High  = 0; }
    else if (_stricmp(name, "xmm2") == 0) { ctx.FltSave.XmmRegisters[2].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[2].High  = 0; }
    else if (_stricmp(name, "xmm3") == 0) { ctx.FltSave.XmmRegisters[3].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[3].High  = 0; }
    else if (_stricmp(name, "xmm4") == 0) { ctx.FltSave.XmmRegisters[4].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[4].High  = 0; }
    else if (_stricmp(name, "xmm5") == 0) { ctx.FltSave.XmmRegisters[5].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[5].High  = 0; }
    else if (_stricmp(name, "xmm6") == 0) { ctx.FltSave.XmmRegisters[6].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[6].High  = 0; }
    else if (_stricmp(name, "xmm7") == 0) { ctx.FltSave.XmmRegisters[7].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[7].High  = 0; }
    else if (_stricmp(name, "xmm8") == 0) { ctx.FltSave.XmmRegisters[8].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[8].High  = 0; }
    else if (_stricmp(name, "xmm9") == 0) { ctx.FltSave.XmmRegisters[9].Low  = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[9].High  = 0; }
    else if (_stricmp(name, "xmm10") == 0) { ctx.FltSave.XmmRegisters[10].Low = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[10].High = 0; }
    else if (_stricmp(name, "xmm11") == 0) { ctx.FltSave.XmmRegisters[11].Low = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[11].High = 0; }
    else if (_stricmp(name, "xmm12") == 0) { ctx.FltSave.XmmRegisters[12].Low = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[12].High = 0; }
    else if (_stricmp(name, "xmm13") == 0) { ctx.FltSave.XmmRegisters[13].Low = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[13].High = 0; }
    else if (_stricmp(name, "xmm14") == 0) { ctx.FltSave.XmmRegisters[14].Low = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[14].High = 0; }
    else if (_stricmp(name, "xmm15") == 0) { ctx.FltSave.XmmRegisters[15].Low = is_float ? double_bits(fvalue) : v; ctx.FltSave.XmmRegisters[15].High = 0; }
    else if (_stricmp(name, "st0") == 0) double_to_x87(fvalue, &ctx.FltSave.FloatRegisters[0]);
    else if (_stricmp(name, "st1") == 0) double_to_x87(fvalue, &ctx.FltSave.FloatRegisters[1]);
    else if (_stricmp(name, "st2") == 0) double_to_x87(fvalue, &ctx.FltSave.FloatRegisters[2]);
    else if (_stricmp(name, "st3") == 0) double_to_x87(fvalue, &ctx.FltSave.FloatRegisters[3]);
    else if (_stricmp(name, "st4") == 0) double_to_x87(fvalue, &ctx.FltSave.FloatRegisters[4]);
    else if (_stricmp(name, "st5") == 0) double_to_x87(fvalue, &ctx.FltSave.FloatRegisters[5]);
    else if (_stricmp(name, "st6") == 0) double_to_x87(fvalue, &ctx.FltSave.FloatRegisters[6]);
    else if (_stricmp(name, "st7") == 0) double_to_x87(fvalue, &ctx.FltSave.FloatRegisters[7]);
#endif
    else handled = 0; /* unknown register */

    if (!handled)
        return 0;

    if (!SetThreadContext(dbg->thread, &ctx))
        return 0;

    if (is_float)
        printf("%s = %g\n", name, fvalue);
    else
        printf("%s = 0x%llx\n", name, (unsigned long long)v);
    return 1;
}