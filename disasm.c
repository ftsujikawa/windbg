#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "disasm.h"

struct decode_state
{
    unsigned char *buf;
    size_t size;
    size_t pos;
    int rex_w;
    int rex_r;
    int rex_x;
    int rex_b;
    int opsize_66;
    int addrsize_67;
};

static int read_byte(struct decode_state *s, unsigned char *b)
{
    if (s->pos >= s->size)
        return 0;
    *b = s->buf[s->pos++];
    return 1;
}

static int read_u32(struct decode_state *s, unsigned int *out)
{
    if (s->pos + 4 > s->size)
        return 0;
    *out = (unsigned int)s->buf[s->pos]
        | ((unsigned int)s->buf[s->pos + 1] << 8)
        | ((unsigned int)s->buf[s->pos + 2] << 16)
        | ((unsigned int)s->buf[s->pos + 3] << 24);
    s->pos += 4;
    return 1;
}

static int read_s32(struct decode_state *s, int *out)
{
    unsigned int u;
    if (!read_u32(s, &u))
        return 0;
    *out = (int)u;
    return 1;
}

static int read_u64(struct decode_state *s, unsigned long long *out)
{
    if (s->pos + 8 > s->size)
        return 0;
    *out = 0;
    for (int i = 0; i < 8; i++)
        *out |= ((unsigned long long)s->buf[s->pos + i]) << (i * 8);
    s->pos += 8;
    return 1;
}

static int read_s8(struct decode_state *s, signed char *out)
{
    unsigned char b;
    if (!read_byte(s, &b))
        return 0;
    *out = (signed char)b;
    return 1;
}

static int read_u8(struct decode_state *s, unsigned char *out)
{
    return read_byte(s, out);
}

static const char *reg_names[16] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

static const char *reg32_names[16] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
};

static const char *reg_name(int reg, int rex_b, int rex_w)
{
    int idx = reg | (rex_b ? 8 : 0);
    return rex_w ? reg_names[idx] : reg32_names[idx];
}

static const char *jcc_names[16] = {
    "jo", "jno", "jb", "jae", "je", "jne", "jbe", "ja",
    "js", "jns", "jp", "jnp", "jl", "jge", "jle", "jg"
};

static int has_modrm_1byte(unsigned char op)
{
    switch (op)
    {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x18: case 0x19: case 0x1A: case 0x1B:
        case 0x20: case 0x21: case 0x22: case 0x23:
        case 0x28: case 0x29: case 0x2A: case 0x2B:
        case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x62: case 0x63: case 0x69: case 0x6B:
        case 0x80: case 0x81: case 0x82: case 0x83:
        case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8A: case 0x8B:
        case 0x8C: case 0x8D: case 0x8E: case 0x8F:
        case 0xC0: case 0xC1: case 0xC4: case 0xC5:
        case 0xC6: case 0xC7: case 0xD0: case 0xD1:
        case 0xD2: case 0xD3: case 0xF6: case 0xF7:
        case 0xFE: case 0xFF:
            return 1;
        default:
            return 0;
    }
}

static int has_modrm_2byte(unsigned char op)
{
    return 1;
}

static int modrm_len(struct decode_state *s, unsigned char modrm)
{
    int mod = (modrm >> 6) & 3;
    int rm = modrm & 7;

    if (mod == 3)
        return 0;

    if (rm == 4)
    {
        unsigned char sib;
        if (!read_byte(s, &sib))
            return -1;

        int base = sib & 7;
        int sib_mod = (sib >> 6) & 3;

        if (mod == 0 && base == 5)
            return 4;
        return mod == 1 ? 1 : (mod == 2 ? 4 : 0);
    }

    if (mod == 0 && rm == 5)
        return 4;
    if (mod == 1)
        return 1;
    if (mod == 2)
        return 4;
    return 0;
}

static int decode_modrm(struct decode_state *s, char *dst, size_t dst_size,
    int reg, int rex_r, int rex_w)
{
    unsigned char modrm;
    if (!read_byte(s, &modrm))
        return -1;

    int mod = (modrm >> 6) & 3;
    int rm = modrm & 7;
    int reg_field = (modrm >> 3) & 7;

    (void)reg;

    const char *reg_str = reg_name(reg_field, rex_r, rex_w);

    if (mod == 3)
    {
        snprintf(dst, dst_size, "%s", reg_name(rm, s->rex_b, rex_w));
        return 0;
    }

    if (rm == 4)
    {
        unsigned char sib;
        if (!read_byte(s, &sib))
            return -1;

        int base = sib & 7;
        int index = (sib >> 3) & 7;
        int scale = 1 << ((sib >> 6) & 3);

        int disp_len = 0;
        int disp = 0;

        if (mod == 0 && base == 5)
        {
            disp_len = 4;
        }
        else
        {
            disp_len = mod == 1 ? 1 : (mod == 2 ? 4 : 0);
        }

        if (disp_len == 1)
        {
            signed char d;
            if (!read_s8(s, &d))
                return -1;
            disp = d;
        }
        else if (disp_len == 4)
        {
            unsigned int d;
            if (!read_u32(s, &d))
                return -1;
            disp = (int)d;
        }

        (void)index;
        (void)scale;
        (void)disp;
        (void)base;

        snprintf(dst, dst_size, "[rsp%+d]", disp);
        return 0;
    }

    int disp_len = 0;
    int disp = 0;

    if (mod == 0 && rm == 5)
    {
        disp_len = 4;
    }
    else
    {
        disp_len = mod == 1 ? 1 : (mod == 2 ? 4 : 0);
    }

    if (disp_len == 1)
    {
        signed char d;
        if (!read_s8(s, &d))
            return -1;
        disp = d;
    }
    else if (disp_len == 4)
    {
        unsigned int d;
        if (!read_u32(s, &d))
            return -1;
        disp = (int)d;
    }

    (void)reg_str;

    if (mod == 0 && rm == 5)
    {
        snprintf(dst, dst_size, "[rip%+d]", disp);
    }
    else
    {
        snprintf(dst, dst_size, "[%s%+d]", reg_name(rm, s->rex_b, 1), disp);
    }

    return 0;
}

static int imm_len_for_op(unsigned char op, unsigned char ext)
{
    (void)ext;
    switch (op)
    {
        case 0x80: case 0xC0: case 0xC1:
        case 0xC6: case 0xEB: case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x74: case 0x75: case 0x76: case 0x77: case 0x78: case 0x79:
        case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            return 1;
        case 0x81: case 0xC7: case 0xE8: case 0xE9:
            return 4;
        case 0x83: case 0x6B:
            return 1;
        case 0x69:
            return 4;
        default:
            return 0;
    }
}

static int decode_one(struct decode_state *s, DWORD64 base_addr,
    char *mnemonic, size_t mnem_size)
{
    size_t start = s->pos;
    s->rex_w = 0;
    s->rex_r = 0;
    s->rex_x = 0;
    s->rex_b = 0;
    s->opsize_66 = 0;
    s->addrsize_67 = 0;

    unsigned char b;
    while (read_byte(s, &b))
    {
        if (b >= 0x40 && b <= 0x4F)
        {
            s->rex_w = (b >> 3) & 1;
            s->rex_r = (b >> 2) & 1;
            s->rex_x = (b >> 1) & 1;
            s->rex_b = b & 1;
        }
        else if (b == 0x66)
        {
            s->opsize_66 = 1;
        }
        else if (b == 0x67)
        {
            s->addrsize_67 = 1;
        }
        else
        {
            break;
        }
    }

    unsigned char op = b;

    if (op == 0x0F)
    {
        unsigned char op2;
        if (!read_byte(s, &op2))
            return 0;

        if (op2 >= 0x80 && op2 <= 0x8F)
        {
            int cond = op2 - 0x80;
            int rel;
            if (!read_s32(s, (signed int*)&rel))
                return 0;
            snprintf(mnemonic, mnem_size, "%s 0x%llx",
                jcc_names[cond], base_addr + s->pos + (int)rel);
            return (int)(s->pos - start);
        }

        if (op2 == 0x05)
        {
            snprintf(mnemonic, mnem_size, "syscall");
            return (int)(s->pos - start);
        }
        if (op2 == 0xA2)
        {
            snprintf(mnemonic, mnem_size, "cpuid");
            return (int)(s->pos - start);
        }
        if (op2 == 0xB6 || op2 == 0xB7)
        {
            char rm[64];
            if (decode_modrm(s, rm, sizeof(rm), 0, s->rex_r, 0) < 0)
                return 0;
            snprintf(mnemonic, mnem_size, "movzx %s, %s",
                reg_name(0, s->rex_b, s->rex_w), rm);
            return (int)(s->pos - start);
        }

        snprintf(mnemonic, mnem_size, "0f_%02x", op2);
        return (int)(s->pos - start);
    }

    if (op >= 0x50 && op <= 0x57)
    {
        snprintf(mnemonic, mnem_size, "push %s", reg_name(op - 0x50, s->rex_b, 1));
        return (int)(s->pos - start);
    }
    if (op >= 0x58 && op <= 0x5F)
    {
        snprintf(mnemonic, mnem_size, "pop %s", reg_name(op - 0x58, s->rex_b, 1));
        return (int)(s->pos - start);
    }
    if (op >= 0xB8 && op <= 0xBF)
    {
        int reg = op - 0xB8;
        if (s->rex_w)
        {
            unsigned long long imm;
            if (!read_u64(s, &imm))
                return 0;
            snprintf(mnemonic, mnem_size, "mov %s, 0x%llx",
                reg_name(reg, s->rex_b, 1), imm);
        }
        else
        {
            unsigned int imm;
            if (!read_u32(s, &imm))
                return 0;
            snprintf(mnemonic, mnem_size, "mov %s, 0x%x",
                reg_name(reg, s->rex_b, 0), imm);
        }
        return (int)(s->pos - start);
    }
    if (op >= 0x70 && op <= 0x7F)
    {
        signed char rel;
        if (!read_s8(s, &rel))
            return 0;
        snprintf(mnemonic, mnem_size, "%s 0x%llx",
            jcc_names[op - 0x70], base_addr + s->pos + rel);
        return (int)(s->pos - start);
    }

    switch (op)
    {
        case 0x90:
            snprintf(mnemonic, mnem_size, "nop");
            return (int)(s->pos - start);
        case 0xC3:
            snprintf(mnemonic, mnem_size, "ret");
            return (int)(s->pos - start);
        case 0xC2:
        {
            unsigned int imm;
            if (!read_u32(s, &imm))
                return 0;
            snprintf(mnemonic, mnem_size, "ret 0x%x", imm & 0xFFFF);
            return (int)(s->pos - start);
        }
        case 0xCC:
            snprintf(mnemonic, mnem_size, "int3");
            return (int)(s->pos - start);
        case 0xC9:
            snprintf(mnemonic, mnem_size, "leave");
            return (int)(s->pos - start);
        case 0xE8:
        {
            int rel;
            if (!read_s32(s, &rel))
                return 0;
            snprintf(mnemonic, mnem_size, "call 0x%llx",
                base_addr + s->pos + rel);
            return (int)(s->pos - start);
        }
        case 0xE9:
        {
            int rel;
            if (!read_s32(s, &rel))
                return 0;
            snprintf(mnemonic, mnem_size, "jmp 0x%llx",
                base_addr + s->pos + rel);
            return (int)(s->pos - start);
        }
        case 0xEB:
        {
            signed char rel;
            if (!read_s8(s, &rel))
                return 0;
            snprintf(mnemonic, mnem_size, "jmp 0x%llx",
                base_addr + s->pos + rel);
            return (int)(s->pos - start);
        }
    }

    if (has_modrm_1byte(op))
    {
        char rm[64];
        int reg_field = 0;
        unsigned char modrm;

        size_t modrm_pos = s->pos;
        if (!read_byte(s, &modrm))
            return 0;
        s->pos = modrm_pos;

        reg_field = (modrm >> 3) & 7;
        (void)reg_field;

        if (decode_modrm(s, rm, sizeof(rm), 0, s->rex_r, s->rex_w) < 0)
            return 0;

        int extra_imm = 0;
        if (op == 0x80 || op == 0x81 || op == 0x83)
        {
            int ext = (modrm >> 3) & 7;
            const char *ops[] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
            int imm = 0;
            if (op == 0x80 || op == 0x81)
            {
                unsigned int u;
                if (!read_u32(s, &u))
                    return 0;
                imm = (int)u;
            }
            else
            {
                signed char c;
                if (!read_s8(s, &c))
                    return 0;
                imm = c;
            }
            snprintf(mnemonic, mnem_size, "%s %s, 0x%x",
                ops[ext], rm, imm);
            return (int)(s->pos - start);
        }

        if (op == 0x89)
        {
            snprintf(mnemonic, mnem_size, "mov %s, %s",
                rm, reg_name(reg_field, s->rex_r, s->rex_w));
            return (int)(s->pos - start);
        }
        if (op == 0x8B)
        {
            snprintf(mnemonic, mnem_size, "mov %s, %s",
                reg_name(reg_field, s->rex_r, s->rex_w), rm);
            return (int)(s->pos - start);
        }
        if (op == 0x8D)
        {
            snprintf(mnemonic, mnem_size, "lea %s, %s",
                reg_name(reg_field, s->rex_r, s->rex_w), rm);
            return (int)(s->pos - start);
        }
        if (op == 0xC6)
        {
            unsigned char imm;
            if (!read_u8(s, &imm))
                return 0;
            snprintf(mnemonic, mnem_size, "mov %s, 0x%x", rm, imm);
            return (int)(s->pos - start);
        }
        if (op == 0xC7)
        {
            unsigned int imm;
            if (!read_u32(s, &imm))
                return 0;
            snprintf(mnemonic, mnem_size, "mov %s, 0x%x", rm, imm);
            return (int)(s->pos - start);
        }
        if (op == 0xFF)
        {
            int ext = (modrm >> 3) & 7;
            const char *ops[] = { "inc", "dec", "call", "call", "jmp", "jmp", "push", "??" };
            snprintf(mnemonic, mnem_size, "%s %s", ops[ext], rm);
            return (int)(s->pos - start);
        }

        extra_imm = imm_len_for_op(op, 0);
        if (extra_imm > 0)
        {
            if (extra_imm == 1)
            {
                unsigned char imm;
                if (!read_u8(s, &imm))
                    return 0;
                s->pos += extra_imm - 1;
            }
            else
            {
                unsigned int imm;
                if (!read_u32(s, &imm))
                    return 0;
                s->pos += extra_imm - 4;
            }
        }

        snprintf(mnemonic, mnem_size, "op_%02x %s", op, rm);
        return (int)(s->pos - start);
    }

    snprintf(mnemonic, mnem_size, ".byte 0x%02x", op);
    return (int)(s->pos - start);
}

void disassemble_at(debugger_t *dbg, DWORD64 addr, int count)
{
    unsigned char buf[256];
    SIZE_T n = 0;

    int had_bp[MAX_BREAKPOINTS] = {0};
    BYTE saved_bp_byte[MAX_BREAKPOINTS] = {0};
    DWORD64 bp_addr[MAX_BREAKPOINTS] = {0};
    int had_bp_count = 0;

    int had_temp_bp = 0;
    BYTE saved_temp_bp_byte = 0;
    DWORD64 temp_bp_addr = 0;

    for (int i = 0; i < dbg->breakpoint_count; i++)
    {
        DWORD64 bp = (DWORD64)dbg->breakpoints[i].addr;
        if (bp >= addr && bp < addr + sizeof(buf))
        {
            ReadProcessMemory(dbg->process, (void*)bp, &saved_bp_byte[had_bp_count], 1, &n);
            WriteProcessMemory(dbg->process, (void*)bp, &dbg->breakpoints[i].original_byte, 1, &n);
            FlushInstructionCache(dbg->process, (void*)bp, 1);
            bp_addr[had_bp_count] = bp;
            had_bp[had_bp_count] = 1;
            had_bp_count++;
        }
    }

    if (dbg->temp_bp_addr != NULL)
    {
        temp_bp_addr = (DWORD64)dbg->temp_bp_addr;
        if (temp_bp_addr >= addr && temp_bp_addr < addr + sizeof(buf))
        {
            ReadProcessMemory(dbg->process, (void*)temp_bp_addr, &saved_temp_bp_byte, 1, &n);
            WriteProcessMemory(dbg->process, (void*)temp_bp_addr, &dbg->temp_bp_byte, 1, &n);
            FlushInstructionCache(dbg->process, (void*)temp_bp_addr, 1);
            had_temp_bp = 1;
        }
    }

    ReadProcessMemory(
        dbg->process,
        (void*)addr,
        buf,
        sizeof(buf),
        &n
    );

    for (int i = 0; i < had_bp_count; i++)
    {
        if (had_bp[i])
        {
            BYTE cc = 0xCC;
            WriteProcessMemory(dbg->process, (void*)bp_addr[i], &cc, 1, &n);
            FlushInstructionCache(dbg->process, (void*)bp_addr[i], 1);
        }
    }

    if (had_temp_bp)
    {
        BYTE cc = 0xCC;
        WriteProcessMemory(dbg->process, (void*)temp_bp_addr, &cc, 1, &n);
        FlushInstructionCache(dbg->process, (void*)temp_bp_addr, 1);
    }

    if (n == 0)
    {
        printf("cannot read memory at 0x%llx\n", addr);
        return;
    }

    printf("disassembly at 0x%llx:\n", addr);

    struct decode_state s = {0};
    s.buf = buf;
    s.size = n;
    s.pos = 0;

    DWORD64 cur_addr = addr;
    int printed = 0;

    while (s.pos < s.size && printed < count)
    {
        char mnemonic[256];
        size_t start_pos = s.pos;
        int len = decode_one(&s, cur_addr, mnemonic, sizeof(mnemonic));

        if (len <= 0)
            break;

        printf("  0x%llx: ", cur_addr);

        for (size_t i = start_pos; i < start_pos + len && i < s.size; i++)
            printf("%02x ", buf[i]);

        for (size_t i = 0; i < 15 - len; i++)
            printf("   ");

        printf(" %s\n", mnemonic);

        cur_addr += len;
        printed++;
    }
}
