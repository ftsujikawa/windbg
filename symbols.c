#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <dbghelp.h>

#include "symbols.h"
#include "disasm.h"

#pragma comment(lib, "dbghelp.lib")

int init_symbols(debugger_t *dbg)
{
    printf("process handle = %p\n", dbg->process);

    BOOL ok = SymInitialize(
        dbg->sym_handle,
        NULL,
        FALSE
    );

    if (!ok)
    {
        DWORD err = GetLastError();

        printf("SymInitialize failed\n");
        printf("GetLastError = %lu (0x%lx)\n",
               err, err);

        return -1;
    }

    printf("symbol initialized\n");

    return 0;
}

void cleanup_symbols(debugger_t *dbg)
{
    if (dbg->symbols_initialized)
        SymCleanup(dbg->process);
}

int show_source_line(debugger_t *dbg, DWORD64 addr)
{
    IMAGEHLP_LINE64 line = {0};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;

    if (!SymGetLineFromAddr64(
            dbg->sym_handle,
            addr,
            &displacement,
            &line))
    {
        return 0;
    }

    printf("%s:%lu\n", line.FileName, line.LineNumber);

    FILE *f = fopen(line.FileName, "r");
    if (!f)
    {
        printf("(cannot open %s)\n", line.FileName);
        return 1;
    }

    char buf[512];
    DWORD cur = 1;
    DWORD start = (line.LineNumber > 3) ? line.LineNumber - 3 : 1;
    DWORD end = line.LineNumber + 3;

    while (fgets(buf, sizeof(buf), f))
    {
        if (cur >= start && cur <= end)
        {
            printf("%s %4lu: %s",
                (cur == line.LineNumber) ? ">" : " ",
                cur, buf);
        }
        if (cur > end)
            break;
        cur++;
    }

    fclose(f);
    return 1;
}

void print_disassembly(debugger_t *dbg, DWORD64 addr, int count)
{
    disassemble_at(dbg, addr, count);
}

void show_source_lines(debugger_t *dbg, const char *filename, DWORD from, DWORD count)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        printf("(cannot open %s)\n", filename);
        return;
    }

    char buf[512];
    DWORD cur = 1;
    DWORD end = from + count - 1;

    while (fgets(buf, sizeof(buf), f))
    {
        if (cur >= from && cur <= end)
            printf("  %4lu: %s", cur, buf);

        if (cur > end)
            break;

        cur++;
    }

    fclose(f);

    dbg->list_next_line = end + 1;
    strncpy(dbg->list_file, filename, sizeof(dbg->list_file) - 1);
}

DWORD get_source_line_number(debugger_t *dbg, DWORD64 addr)
{
    IMAGEHLP_LINE64 line = {0};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;

    if (!SymGetLineFromAddr64(
            dbg->sym_handle,
            addr,
            &displacement,
            &line))
    {
        return 0;
    }

    return line.LineNumber;
}

typedef struct {
    const char *target_name;
    SYMBOL_INFO *found_sym;
    int found;
} enum_var_ctx_t;

static BOOL CALLBACK enum_locals_cb(
    PSYMBOL_INFO sym_info,
    ULONG sym_size,
    PVOID user_ctx)
{
    enum_var_ctx_t *ectx = (enum_var_ctx_t*)user_ctx;

    if (_stricmp(sym_info->Name, ectx->target_name) == 0)
    {
        memcpy(ectx->found_sym, sym_info,
            sizeof(SYMBOL_INFO) + sym_info->NameLen);
        ectx->found = 1;
        return FALSE;
    }

    return TRUE;
}

static DWORD64 resolve_var_addr(
    debugger_t *dbg,
    CONTEXT *ctx,
    PSYMBOL_INFO sym)
{
    if (sym->Flags & SYMFLAG_REGREL)
    {
        long long offset = (long long)sym->Address;

        if (sym->Register == 335) /* CV_AMD64_RSP */
            return ctx->Rsp + offset;
        else if (sym->Register == 334) /* CV_AMD64_RBP */
            return ctx->Rbp + offset;
        else
            return ctx->Rsp + offset;
    }
    else if (sym->Flags & SYMFLAG_REGISTER)
    {
        return 0;
    }
    else
    {
        return sym->Address;
    }
}

void print_variable(debugger_t *dbg, const char *name)
{
    int addr_of = 0;
    int deref   = 0;
    const char *varname = name;

    if (name[0] == '&')
    {
        addr_of = 1;
        varname = name + 1;
    }
    else if (name[0] == '*')
    {
        deref = 1;
        varname = name + 1;
    }

    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    IMAGEHLP_STACK_FRAME img_frame = {0};
    img_frame.InstructionOffset = ctx.Rip;
    img_frame.FrameOffset = ctx.Rsp;
    img_frame.StackOffset = ctx.Rsp;

    SymSetContext(dbg->sym_handle, &img_frame, NULL);

    char buffer[sizeof(SYMBOL_INFO) + 256];
    PSYMBOL_INFO sym = (PSYMBOL_INFO)buffer;
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 255;

    enum_var_ctx_t ectx = {0};
    ectx.target_name = varname;
    ectx.found_sym = sym;
    ectx.found = 0;

    SymEnumSymbols(dbg->sym_handle, 0, "*", enum_locals_cb, &ectx);

    if (!ectx.found)
    {
        printf("variable '%s' not found\n", varname);
        return;
    }

    if (sym->Flags & SYMFLAG_REGISTER)
    {
        printf("'%s' is in a register (not memory)\n", varname);
        return;
    }

    DWORD64 var_addr = resolve_var_addr(dbg, &ctx, sym);

    if (addr_of)
    {
        printf("&%s = 0x%llx\n", varname, var_addr);
        return;
    }

    ULONG size = (sym->Size > 0 && sym->Size <= 8) ? (ULONG)sym->Size : 4;

    unsigned long long value = 0;
    SIZE_T n;
    ReadProcessMemory(
        dbg->process,
        (void*)var_addr,
        &value,
        size,
        &n
    );

    if (!deref)
    {
        if (size <= 4)
            printf("%s = %d (0x%x)\n", varname,
                (int)value, (unsigned int)value);
        else
            printf("%s = %lld (0x%llx)\n", varname,
                (long long)value, value);
    }
    else
    {
        DWORD64 ptr_addr = value;
        unsigned long long deref_val = 0;

        if (!ReadProcessMemory(
                dbg->process,
                (void*)ptr_addr,
                &deref_val,
                sizeof(deref_val),
                &n))
        {
            printf("*%s = ??? (cannot read 0x%llx)\n", varname, ptr_addr);
            return;
        }

        printf("*%s = %lld (0x%llx)  [ptr=0x%llx]\n",
            varname,
            (long long)deref_val,
            deref_val,
            ptr_addr);
    }
}

void* skip_function_prologue(debugger_t *dbg, void *addr)
{
    IMAGEHLP_LINE64 line = {0};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD disp = 0;

    if (!SymGetLineFromAddr64(
            dbg->sym_handle,
            (DWORD64)addr,
            &disp,
            &line))
    {
        return addr;
    }

    for (DWORD i = 1; i <= 20; i++)
    {
        IMAGEHLP_LINE64 next = {0};
        next.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        LONG next_disp = 0;

        if (SymGetLineFromName64(
                dbg->sym_handle,
                NULL,
                line.FileName,
                line.LineNumber + i,
                &next_disp,
                &next))
        {
            if (next.Address > (DWORD64)addr)
            {
                printf("prologue skipped -> %s:%lu\n",
                    next.FileName, next.LineNumber);
                return (void*)next.Address;
            }
        }
    }

    return addr;
}

void print_backtrace(debugger_t *dbg)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    STACKFRAME64 frame = {0};
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    DWORD machine = IMAGE_FILE_MACHINE_AMD64;

    for (int i = 0; i < 32; i++)
    {
        BOOL ok = StackWalk64(
            machine,
            dbg->process,
            dbg->thread,
            &frame,
            &ctx,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );

        if (!ok || frame.AddrPC.Offset == 0)
            break;

        DWORD64 addr = frame.AddrPC.Offset;

        char sym_buffer[sizeof(SYMBOL_INFO) + 256] = {0};
        PSYMBOL_INFO sym = (PSYMBOL_INFO)sym_buffer;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;

        char symbol_name[256] = "???";
        DWORD64 disp = 0;

        if (SymFromAddr(dbg->sym_handle, addr, &disp, sym))
        {
            snprintf(symbol_name, sizeof(symbol_name),
                "%s+0x%llx", sym->Name, disp);
        }

        IMAGEHLP_LINE64 line = {0};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD line_disp = 0;

        const char *file_info = "";
        char line_info[512] = {0};

        if (SymGetLineFromAddr64(
                dbg->sym_handle,
                addr,
                &line_disp,
                &line))
        {
            snprintf(line_info, sizeof(line_info),
                " at %s:%lu", line.FileName, line.LineNumber);
            file_info = line_info;
        }

        printf("#%2d 0x%llx %s%s\n",
            i, addr, symbol_name, file_info);
    }
}

int set_variable(debugger_t *dbg, const char *name, long long value)
{
    const char *varname = name;

    if (name[0] == '&')
    {
        printf("cannot assign to address of '%s'\n", name + 1);
        return -1;
    }

    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    IMAGEHLP_STACK_FRAME img_frame = {0};
    img_frame.InstructionOffset = ctx.Rip;

    SymSetContext(dbg->sym_handle, &img_frame, NULL);

    char buffer[sizeof(SYMBOL_INFO) + 256];
    PSYMBOL_INFO sym = (PSYMBOL_INFO)buffer;
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 255;

    enum_var_ctx_t ectx = {0};
    ectx.target_name = varname;
    ectx.found_sym = sym;
    ectx.found = 0;

    SymEnumSymbols(dbg->sym_handle, 0, "*", enum_locals_cb, &ectx);

    if (!ectx.found)
    {
        printf("variable '%s' not found\n", varname);
        return -1;
    }

    if (sym->Flags & SYMFLAG_REGISTER)
    {
        printf("'%s' is in a register (cannot set)\n", varname);
        return -1;
    }

    DWORD64 var_addr = resolve_var_addr(dbg, &ctx, sym);
    ULONG size = (sym->Size > 0 && sym->Size <= 8) ? (ULONG)sym->Size : 4;

    unsigned long long write_val = 0;

    if (size <= 4)
        write_val = (unsigned long long)(long)value;
    else
        write_val = (unsigned long long)value;

    SIZE_T n;
    BOOL ok = WriteProcessMemory(
        dbg->process,
        (void*)var_addr,
        &write_val,
        size,
        &n
    );

    if (!ok)
    {
        printf("WriteProcessMemory failed for '%s'\n", varname);
        return -1;
    }

    printf("%s = %lld\n", varname, value);
    return 0;
}

void* lookup_source_line_addr(
    debugger_t *dbg,
    const char *filename,
    DWORD line_number)
{
    IMAGEHLP_LINE64 line = {0};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    LONG displacement = 0;

    if (!SymGetLineFromName64(
            dbg->sym_handle,
            NULL,
            filename,
            line_number,
            &displacement,
            &line))
    {
        printf("line not found: %s:%lu\n",
            filename, line_number);
        return NULL;
    }

    printf("%s:%lu -> 0x%llx\n",
        filename, line.LineNumber,
        line.Address);

    return (void*)line.Address;
}

void* lookup_symbol(
    debugger_t *dbg,
    const char *name)
{
    char buffer[
        sizeof(SYMBOL_INFO)+256];

    PSYMBOL_INFO sym =
        (PSYMBOL_INFO)buffer;

    sym->SizeOfStruct =
        sizeof(SYMBOL_INFO);

    sym->MaxNameLen = 255;

    if (!SymFromName(
            dbg->sym_handle,
            name,
            sym))
    {
        printf("symbol not found\n");
        return NULL;
    }

    printf("%s -> 0x%llx\n",
        name,
        sym->Address);

    return (void*)sym->Address;
}