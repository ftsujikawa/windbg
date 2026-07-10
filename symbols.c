#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <dbghelp.h>

#include "symbols.h"
#include "disasm.h"
#include "expr.h"

#pragma comment(lib, "dbghelp.lib")

int init_symbols(debugger_t *dbg)
{
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
        print_disassembly(dbg, addr, 1);
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
    strncpy_s(dbg->list_file, sizeof(dbg->list_file), filename, sizeof(dbg->list_file) - 1);
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

/* SymTagEnum values */
#define SYM_TAG_UDT        11
#define SYM_TAG_POINTER    14
#define SYM_TAG_ARRAYTYPE  15

static void print_struct(
    debugger_t *dbg,
    DWORD64 mod_base,
    DWORD type_id,
    DWORD64 base_addr,
    const char *prefix,
    int depth)
{
    if (depth > 4)
        return;

    DWORD child_count = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, type_id,
        TI_GET_CHILDRENCOUNT, &child_count);

    if (child_count == 0)
        return;

    DWORD buf_size = sizeof(TI_FINDCHILDREN_PARAMS) + child_count * sizeof(ULONG);
    TI_FINDCHILDREN_PARAMS *params = (TI_FINDCHILDREN_PARAMS*)malloc(buf_size);
    if (!params)
        return;

    memset(params, 0, buf_size);
    params->Count = child_count;
    params->Start = 0;

    if (!SymGetTypeInfo(dbg->sym_handle, (DWORD64)mod_base, type_id,
            TI_FINDCHILDREN, params))
    {
        free(params);
        return;
    }

    for (DWORD i = 0; i < child_count; i++)
    {
        ULONG child_id = params->ChildId[i];

        WCHAR *name_w = NULL;
        char member_name[256] = {0};
        if (SymGetTypeInfo(dbg->sym_handle, (DWORD64)mod_base, child_id,
                TI_GET_SYMNAME, &name_w) && name_w)
        {
            WideCharToMultiByte(CP_ACP, 0, name_w, -1,
                member_name, sizeof(member_name), NULL, NULL);
            LocalFree(name_w);
        }

        DWORD offset = 0;
        SymGetTypeInfo(dbg->sym_handle, (DWORD64)mod_base, child_id,
            TI_GET_OFFSET, &offset);

        ULONG64 length = 0;
        SymGetTypeInfo(dbg->sym_handle, (DWORD64)mod_base, child_id,
            TI_GET_LENGTH, &length);

        DWORD child_type_id = 0;
        SymGetTypeInfo(dbg->sym_handle, (DWORD64)mod_base, child_id,
            TI_GET_TYPEID, &child_type_id);

        DWORD child_tag = 0;
        SymGetTypeInfo(dbg->sym_handle, (DWORD64)mod_base, child_type_id,
            TI_GET_SYMTAG, &child_tag);

        char full_name[512];
        if (prefix[0])
            snprintf(full_name, sizeof(full_name), "%s.%s", prefix, member_name);
        else
            snprintf(full_name, sizeof(full_name), "%s", member_name);

        DWORD64 member_addr = base_addr + offset;

        if (child_tag == SYM_TAG_UDT)
        {
            printf("%s = {\n", full_name);
            print_struct(dbg, mod_base, child_type_id, member_addr,
                full_name, depth + 1);
            printf("}\n");
        }
        else
        {
            ULONG sz = (ULONG)(length > 0 && length <= 8 ? length : 4);
            unsigned long long val = 0;
            SIZE_T n;
            ReadProcessMemory(dbg->process, (void*)member_addr, &val, sz, &n);

            if (sz <= 4)
                printf("  %s = %d (0x%x)\n", full_name,
                    (int)val, (unsigned int)val);
            else
                printf("  %s = %lld (0x%llx)\n", full_name,
                    (long long)val, val);
        }
    }

    free(params);
}

static void print_array(
    debugger_t *dbg,
    DWORD64 mod_base,
    DWORD type_id,
    DWORD64 base_addr,
    const char *varname)
{
    DWORD elem_type_id = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, type_id,
        TI_GET_TYPEID, &elem_type_id);

    ULONG64 total_len = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, type_id,
        TI_GET_LENGTH, &total_len);

    ULONG64 elem_len = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, elem_type_id,
        TI_GET_LENGTH, &elem_len);

    if (elem_len == 0) elem_len = 4;

    DWORD count = (DWORD)(total_len / elem_len);

    DWORD elem_tag = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, elem_type_id,
        TI_GET_SYMTAG, &elem_tag);

    printf("%s = {\n", varname);
    for (DWORD i = 0; i < count; i++)
    {
        DWORD64 elem_addr = base_addr + i * elem_len;
        char idx_name[64];
        snprintf(idx_name, sizeof(idx_name), "[%u]", i);

        if (elem_tag == SYM_TAG_UDT)
        {
            printf("  %s = {\n", idx_name);
            print_struct(dbg, mod_base, elem_type_id, elem_addr,
                idx_name, 1);
            printf("  }\n");
        }
        else
        {
            unsigned long long val = 0;
            SIZE_T n;
            ReadProcessMemory(dbg->process, (void*)elem_addr, &val,
                (SIZE_T)elem_len, &n);

            if (elem_len <= 4)
                printf("  [%u] = %d (0x%x)\n", i,
                    (int)val, (unsigned int)val);
            else
                printf("  [%u] = %lld (0x%llx)\n", i,
                    (long long)val, val);
        }
    }
    printf("}\n");
}

static BOOL find_member(
    debugger_t *dbg,
    DWORD64 mod_base,
    DWORD type_id,
    const char *member_name,
    DWORD *out_type_id,
    DWORD *out_offset)
{
    DWORD child_count = 0;
    if (!SymGetTypeInfo(dbg->sym_handle, mod_base, type_id,
            TI_GET_CHILDRENCOUNT, &child_count) || child_count == 0)
        return FALSE;

    DWORD buf_size = sizeof(TI_FINDCHILDREN_PARAMS) + child_count * sizeof(ULONG);
    TI_FINDCHILDREN_PARAMS *params = (TI_FINDCHILDREN_PARAMS*)malloc(buf_size);
    if (!params)
        return FALSE;

    memset(params, 0, buf_size);
    params->Count = child_count;

    if (!SymGetTypeInfo(dbg->sym_handle, mod_base, type_id,
            TI_FINDCHILDREN, params))
    {
        free(params);
        return FALSE;
    }

    for (DWORD i = 0; i < child_count; i++)
    {
        ULONG child_id = params->ChildId[i];

        WCHAR *name_w = NULL;
        char mname[256] = {0};
        if (SymGetTypeInfo(dbg->sym_handle, mod_base, child_id,
                TI_GET_SYMNAME, &name_w) && name_w)
        {
            WideCharToMultiByte(CP_ACP, 0, name_w, -1,
                mname, sizeof(mname), NULL, NULL);
            LocalFree(name_w);
        }

        if (strcmp(mname, member_name) == 0)
        {
            DWORD offset = 0;
            SymGetTypeInfo(dbg->sym_handle, mod_base, child_id,
                TI_GET_OFFSET, &offset);
            DWORD child_type_id = 0;
            SymGetTypeInfo(dbg->sym_handle, mod_base, child_id,
                TI_GET_TYPEID, &child_type_id);
            *out_type_id = child_type_id;
            *out_offset  = offset;
            free(params);
            return TRUE;
        }
    }

    free(params);
    return FALSE;
}

void print_variable(debugger_t *dbg, const char *name)
{
    int addr_of = 0;
    int deref   = 0;
    const char *varname = name;

    while (varname[0] == '*')
    {
        deref++;
        varname++;
    }

    if (!deref && name[0] == '&')
    {
        addr_of = 1;
        varname = name + 1;
    }

    char base_name[128] = {0};
    char member_path[256] = {0};

    const char *sep = strpbrk(varname, ".[- ");
    while (sep && (sep[0] == ' ' || (sep[0] == '-' && sep[1] != '>')))
        sep = strpbrk(sep + 1, ".[- ");

    if (sep)
    {
        size_t base_len = (size_t)(sep - varname);
        if (base_len >= sizeof(base_name))
            base_len = sizeof(base_name) - 1;
        strncpy_s(base_name, sizeof(base_name), varname, base_len);
        base_name[base_len] = '\0';

        if (sep[0] == '-' && sep[1] == '>')
            strncpy_s(member_path, sizeof(member_path), sep + 2, sizeof(member_path) - 1);
        else if (sep[0] == '[')
            strncpy_s(member_path, sizeof(member_path), sep, sizeof(member_path) - 1);
        else
            strncpy_s(member_path, sizeof(member_path), sep + 1, sizeof(member_path) - 1);

        varname = base_name;
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
    DWORD64 mod_base = sym->ModBase;
    DWORD   cur_type = sym->TypeIndex;

    if (member_path[0] != '\0')
    {
        char *path = member_path;
        char token[128];

        while (path && path[0] != '\0')
        {
            DWORD type_tag = 0;
            SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                TI_GET_SYMTAG, &type_tag);

            if (path[0] == '[')
            {
                int idx = 0;
                char *close = strchr(path, ']');
                if (!close) { printf("syntax error: missing ']'\n"); return; }
                sscanf_s(path + 1, "%d", &idx);

                if (type_tag == SYM_TAG_POINTER)
                {
                    DWORD pointed = 0;
                    SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                        TI_GET_TYPEID, &pointed);
                    cur_type = pointed;
                    unsigned long long ptr_val = 0;
                    SIZE_T n;
                    ReadProcessMemory(dbg->process, (void*)var_addr,
                        &ptr_val, sizeof(ptr_val), &n);
                    var_addr = (DWORD64)ptr_val;
                }
                else if (type_tag == SYM_TAG_ARRAYTYPE)
                {
                    DWORD elem_type = 0;
                    SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                        TI_GET_TYPEID, &elem_type);
                    cur_type = elem_type;
                }

                ULONG64 elem_len = 0;
                SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                    TI_GET_LENGTH, &elem_len);
                if (elem_len == 0) elem_len = 4;

                var_addr += (DWORD64)idx * elem_len;

                path = close + 1;
                if (path[0] == '.') path++;
                else if (path[0] == '-' && path[1] == '>') path += 2;
                continue;
            }

            char *next_dot  = strchr(path, '.');
            char *next_arr  = strstr(path, "->");
            char *next_idx  = strchr(path, '[');
            char *next = NULL;
            int   is_ptr = 0;

            if (next_dot && next_arr)
            {
                if (next_arr < next_dot) { next = next_arr; is_ptr = 1; }
                else                     { next = next_dot; is_ptr = 0; }
            }
            else if (next_arr) { next = next_arr; is_ptr = 1; }
            else if (next_dot) { next = next_dot; is_ptr = 0; }

            if (next_idx && (!next || next_idx < next))
            {
                next = next_idx; is_ptr = 0;
            }

            size_t tlen = next ? (size_t)(next - path) : strlen(path);
            if (tlen >= sizeof(token)) tlen = sizeof(token) - 1;
            strncpy_s(token, sizeof(token), path, tlen);
            token[tlen] = '\0';

            if (is_ptr)
            {
                unsigned long long ptr_val = 0;
                SIZE_T n;
                ReadProcessMemory(dbg->process, (void*)var_addr,
                    &ptr_val, sizeof(ptr_val), &n);
                var_addr = (DWORD64)ptr_val;
            }

            if (type_tag == SYM_TAG_POINTER)
            {
                DWORD pointed_type = 0;
                SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                    TI_GET_TYPEID, &pointed_type);
                cur_type = pointed_type;

                unsigned long long ptr_val = 0;
                SIZE_T n;
                ReadProcessMemory(dbg->process, (void*)var_addr,
                    &ptr_val, sizeof(ptr_val), &n);
                var_addr = (DWORD64)ptr_val;
            }

            DWORD member_type = 0, member_offset = 0;
            if (!find_member(dbg, mod_base, cur_type, token,
                    &member_type, &member_offset))
            {
                printf("member '%s' not found\n", token);
                return;
            }

            var_addr += member_offset;
            cur_type  = member_type;

            if (!next)
                path = NULL;
            else if (next == next_idx)
                path = next;
            else
                path = is_ptr ? next + 2 : next + 1;
        }

        if (addr_of)
        {
            printf("%s = 0x%llx\n", name, var_addr);
            return;
        }

        DWORD final_tag = 0;
        SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
            TI_GET_SYMTAG, &final_tag);

        ULONG64 length = 0;
        SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
            TI_GET_LENGTH, &length);

        if (final_tag == SYM_TAG_UDT)
        {
            printf("%s = {\n", name);
            print_struct(dbg, mod_base, cur_type, var_addr, "", 0);
            printf("}\n");
        }
        else if (final_tag == SYM_TAG_ARRAYTYPE)
        {
            print_array(dbg, mod_base, cur_type, var_addr, name);
        }
        else
        {
            ULONG sz = (ULONG)(length > 0 && length <= 8 ? length : 4);
            unsigned long long val = 0;
            SIZE_T n;
            ReadProcessMemory(dbg->process, (void*)var_addr, &val, sz, &n);
            if (sz <= 4)
                printf("%s = %d (0x%x)\n", name,
                    (int)val, (unsigned int)val);
            else
                printf("%s = %lld (0x%llx)\n", name,
                    (long long)val, val);
        }
        return;
    }

    if (addr_of)
    {
        printf("&%s = 0x%llx\n", varname, var_addr);
        return;
    }

    DWORD type_tag = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, sym->TypeIndex,
        TI_GET_SYMTAG, &type_tag);

    if (!deref && type_tag == SYM_TAG_UDT)
    {
        printf("%s = {\n", varname);
        print_struct(dbg, mod_base, sym->TypeIndex, var_addr, "", 0);
        printf("}\n");
        return;
    }

    if (!deref && type_tag == SYM_TAG_ARRAYTYPE)
    {
        print_array(dbg, mod_base, sym->TypeIndex, var_addr, varname);
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
        return;
    }

    DWORD64 deref_addr = var_addr;
    DWORD   deref_type = cur_type;

    for (int d = 0; d < deref; d++)
    {
        DWORD dtag = 0;
        SymGetTypeInfo(dbg->sym_handle, mod_base, deref_type,
            TI_GET_SYMTAG, &dtag);

        if (dtag != SYM_TAG_POINTER)
        {
            printf("'%s' is not a pointer\n", name);
            return;
        }

        DWORD pointed = 0;
        SymGetTypeInfo(dbg->sym_handle, mod_base, deref_type,
            TI_GET_TYPEID, &pointed);
        deref_type = pointed;

        unsigned long long ptr_val = 0;
        SIZE_T rn;
        ReadProcessMemory(dbg->process, (void*)deref_addr,
            &ptr_val, sizeof(ptr_val), &rn);
        deref_addr = (DWORD64)ptr_val;
    }

    DWORD final_tag = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, deref_type,
        TI_GET_SYMTAG, &final_tag);

    ULONG64 final_len = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, deref_type,
        TI_GET_LENGTH, &final_len);

    if (final_tag == SYM_TAG_UDT)
    {
        printf("%s = {\n", name);
        print_struct(dbg, mod_base, deref_type, deref_addr, "", 0);
        printf("}\n");
    }
    else if (final_tag == SYM_TAG_ARRAYTYPE)
    {
        print_array(dbg, mod_base, deref_type, deref_addr, name);
    }
    else
    {
        ULONG dsz = (ULONG)(final_len > 0 && final_len <= 8 ? final_len : 4);
        unsigned long long dval = 0;
        SIZE_T rn;
        ReadProcessMemory(dbg->process, (void*)deref_addr, &dval, dsz, &rn);
        if (dsz <= 4)
            printf("%s = %d (0x%x)\n", name,
                (int)dval, (unsigned int)dval);
        else
            printf("%s = %lld (0x%llx)\n", name,
                (long long)dval, dval);
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

/* StackWalk64's hProcess argument is also handed to the SymFunctionTableAccess64/
 * SymGetModuleBase64 callbacks, which look up loaded-module info keyed by that
 * handle. Everything was loaded via SymInitialize/SymLoadModuleEx under
 * dbg->sym_handle (GetCurrentProcess(), not the debuggee), so hProcess must be
 * dbg->sym_handle for unwinding to find the module -- but actual stack memory
 * still has to be read from the real debuggee. This callback redirects the
 * default memory reads there. */
static HANDLE g_backtrace_debuggee = NULL;

static BOOL CALLBACK backtrace_read_memory(
    HANDLE hProcess,
    DWORD64 base_addr,
    PVOID buffer,
    DWORD size,
    LPDWORD bytes_read)
{
    (void)hProcess;
    SIZE_T n = 0;
    BOOL ok = ReadProcessMemory(g_backtrace_debuggee, (void*)base_addr, buffer, size, &n);
    if (bytes_read)
        *bytes_read = (DWORD)n;
    return ok;
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

    g_backtrace_debuggee = dbg->process;

    for (int i = 0; i < 32; i++)
    {
        BOOL ok = StackWalk64(
            machine,
            dbg->sym_handle,
            dbg->thread,
            &frame,
            &ctx,
            backtrace_read_memory,
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

        int is_main = 0;
        if (SymFromAddr(dbg->sym_handle, addr, &disp, sym))
        {
            snprintf(symbol_name, sizeof(symbol_name),
                "%s+0x%llx", sym->Name, disp);
            if (strcmp(sym->Name, "main") == 0)
                is_main = 1;
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

        if (is_main)
            break;
    }
}

int set_variable(debugger_t *dbg, const char *name, long long value)
{
    const char *varname = name;
    int deref = 0;

    if (name[0] == '&')
    {
        printf("cannot assign to address of '%s'\n", name + 1);
        return -1;
    }

    while (varname[0] == '*')
    {
        deref++;
        varname++;
    }

    char base_name[128] = {0};
    char member_path[256] = {0};

    const char *sep = strpbrk(varname, ".[- ");
    while (sep && (sep[0] == ' ' || (sep[0] == '-' && sep[1] != '>')))
        sep = strpbrk(sep + 1, ".[- ");

    if (sep)
    {
        size_t base_len = (size_t)(sep - varname);
        if (base_len >= sizeof(base_name))
            base_len = sizeof(base_name) - 1;
        strncpy_s(base_name, sizeof(base_name), varname, base_len);
        base_name[base_len] = '\0';

        if (sep[0] == '-' && sep[1] == '>')
            strncpy_s(member_path, sizeof(member_path), sep + 2, sizeof(member_path) - 1);
        else if (sep[0] == '[')
            strncpy_s(member_path, sizeof(member_path), sep, sizeof(member_path) - 1);
        else
            strncpy_s(member_path, sizeof(member_path), sep + 1, sizeof(member_path) - 1);

        varname = base_name;
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
        return -1;
    }

    if (sym->Flags & SYMFLAG_REGISTER)
    {
        printf("'%s' is in a register (cannot set)\n", varname);
        return -1;
    }

    DWORD64 var_addr = resolve_var_addr(dbg, &ctx, sym);
    DWORD64 mod_base = sym->ModBase;
    DWORD   cur_type = sym->TypeIndex;
    ULONG   size     = (sym->Size > 0 && sym->Size <= 8) ? (ULONG)sym->Size : 4;

    if (member_path[0] != '\0')
    {
        char *path = member_path;
        char token[128];

        while (path && path[0] != '\0')
        {
            DWORD type_tag_s = 0;
            SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                TI_GET_SYMTAG, &type_tag_s);

            if (path[0] == '[')
            {
                int idx = 0;
                char *close = strchr(path, ']');
                if (!close) { printf("syntax error: missing ']'\n"); return -1; }
                sscanf_s(path + 1, "%d", &idx);

                if (type_tag_s == SYM_TAG_POINTER)
                {
                    DWORD pointed = 0;
                    SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                        TI_GET_TYPEID, &pointed);
                    cur_type = pointed;
                    unsigned long long ptr_val = 0;
                    SIZE_T rn;
                    ReadProcessMemory(dbg->process, (void*)var_addr,
                        &ptr_val, sizeof(ptr_val), &rn);
                    var_addr = (DWORD64)ptr_val;
                }
                else if (type_tag_s == SYM_TAG_ARRAYTYPE)
                {
                    DWORD elem_type = 0;
                    SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                        TI_GET_TYPEID, &elem_type);
                    cur_type = elem_type;
                }

                ULONG64 elem_len = 0;
                SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                    TI_GET_LENGTH, &elem_len);
                if (elem_len == 0) elem_len = 4;
                size = (ULONG)elem_len;

                var_addr += (DWORD64)idx * elem_len;

                path = close + 1;
                if (path[0] == '.') path++;
                else if (path[0] == '-' && path[1] == '>') path += 2;
                continue;
            }

            char *next_dot = strchr(path, '.');
            char *next_arr = strstr(path, "->");
            char *next_idx = strchr(path, '[');
            char *next = NULL;
            int   is_ptr = 0;

            if (next_dot && next_arr)
            {
                if (next_arr < next_dot) { next = next_arr; is_ptr = 1; }
                else                     { next = next_dot; is_ptr = 0; }
            }
            else if (next_arr) { next = next_arr; is_ptr = 1; }
            else if (next_dot) { next = next_dot; is_ptr = 0; }

            if (next_idx && (!next || next_idx < next))
            {
                next = next_idx; is_ptr = 0;
            }

            size_t tlen = next ? (size_t)(next - path) : strlen(path);
            if (tlen >= sizeof(token)) tlen = sizeof(token) - 1;
            strncpy_s(token, sizeof(token), path, tlen);
            token[tlen] = '\0';

            DWORD type_tag = 0;
            SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                TI_GET_SYMTAG, &type_tag);

            if (is_ptr || type_tag == SYM_TAG_POINTER)
            {
                if (type_tag == SYM_TAG_POINTER)
                {
                    DWORD pointed = 0;
                    SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                        TI_GET_TYPEID, &pointed);
                    cur_type = pointed;
                }
                unsigned long long ptr_val = 0;
                SIZE_T n;
                ReadProcessMemory(dbg->process, (void*)var_addr,
                    &ptr_val, sizeof(ptr_val), &n);
                var_addr = (DWORD64)ptr_val;
            }

            DWORD member_type = 0, member_offset = 0;
            if (!find_member(dbg, mod_base, cur_type, token,
                    &member_type, &member_offset))
            {
                printf("member '%s' not found\n", token);
                return -1;
            }

            var_addr += member_offset;
            cur_type  = member_type;

            ULONG64 mlen = 0;
            SymGetTypeInfo(dbg->sym_handle, mod_base, cur_type,
                TI_GET_LENGTH, &mlen);
            size = (ULONG)(mlen > 0 && mlen <= 8 ? mlen : 4);

            if (!next)
                path = NULL;
            else if (next == next_idx)
                path = next;
            else
                path = is_ptr ? next + 2 : next + 1;
        }
    }

    if (deref > 0)
    {
        DWORD deref_type = cur_type;
        DWORD64 deref_addr = var_addr;

        for (int d = 0; d < deref; d++)
        {
            DWORD dtag = 0;
            SymGetTypeInfo(dbg->sym_handle, mod_base, deref_type,
                TI_GET_SYMTAG, &dtag);

            if (dtag != SYM_TAG_POINTER)
            {
                printf("'%s' is not a pointer\n", name);
                return -1;
            }

            DWORD pointed = 0;
            SymGetTypeInfo(dbg->sym_handle, mod_base, deref_type,
                TI_GET_TYPEID, &pointed);
            deref_type = pointed;

            if (d < deref - 1)
            {
                unsigned long long ptr_val = 0;
                SIZE_T rn;
                ReadProcessMemory(dbg->process, (void*)deref_addr,
                    &ptr_val, sizeof(ptr_val), &rn);
                deref_addr = (DWORD64)ptr_val;
            }
        }

        unsigned long long ptr_target = 0;
        SIZE_T rn;
        ReadProcessMemory(dbg->process, (void*)deref_addr,
            &ptr_target, sizeof(ptr_target), &rn);
        var_addr = (DWORD64)ptr_target;

        ULONG64 dlen = 0;
        SymGetTypeInfo(dbg->sym_handle, mod_base, deref_type,
            TI_GET_LENGTH, &dlen);
        size = (ULONG)(dlen > 0 && dlen <= 8 ? dlen : 4);
    }

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
        printf("WriteProcessMemory failed for '%s'\n", name);
        return -1;
    }

    printf("%s = %lld\n", name, value);
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

static const char *sym_tag_name(DWORD tag)
{
    /* SymTagEnum values (dbghelp.h): note SymTagBaseType is 16, not 2
       (2 is SymTagCompiland) -- this table previously had them swapped. */
    switch (tag)
    {
    case  5: return "Function";
    case  7: return "Data";
    case 11: return "UDT (struct/union)";
    case 12: return "Enum";
    case 13: return "FunctionType";
    case 14: return "Pointer";
    case 15: return "Array";
    case 16: return "BaseType";
    case 17: return "Typedef";
    default: return "Unknown";
    }
}

typedef struct {
    char         target[256];
    int          found;
    char         sym_buf[sizeof(SYMBOL_INFO) + 256];
} syms_enum_ctx_t;

static BOOL CALLBACK syms_enum_cb(PSYMBOL_INFO si, ULONG size, PVOID ctx)
{
    syms_enum_ctx_t *c = (syms_enum_ctx_t *)ctx;
    if (strcmp(si->Name, c->target) == 0)
    {
        ULONG copy_size = si->SizeOfStruct + si->MaxNameLen;
        if (copy_size > sizeof(c->sym_buf)) copy_size = sizeof(c->sym_buf);
        memcpy(c->sym_buf, si, copy_size);
        c->found = 1;
        return FALSE;
    }
    return TRUE;
}

void print_symbol_info(debugger_t *dbg, const char *name)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    IMAGEHLP_STACK_FRAME frm = {0};
    frm.InstructionOffset = ctx.Rip;
    frm.FrameOffset       = ctx.Rsp;
    frm.StackOffset       = ctx.Rsp;
    SymSetContext(dbg->sym_handle, &frm, NULL);

    syms_enum_ctx_t ec = {0};
    strncpy_s(ec.target, sizeof(ec.target), name, sizeof(ec.target) - 1);
    SymEnumSymbols(dbg->sym_handle, 0, "*", syms_enum_cb, &ec);

    PSYMBOL_INFO sym;
    char global_buf[sizeof(SYMBOL_INFO) + 256];

    if (ec.found)
    {
        sym = (PSYMBOL_INFO)ec.sym_buf;
    }
    else
    {
        /* SymEnumSymbols with BaseOfDll=0 only walks the current scope's
           locals/params -- it never reaches functions or true global
           variables. Fall back to SymFromName, which resolves module-level
           symbols directly by name (same technique as lookup_symbol() and
           expr_lookup_symbol()). */
        PSYMBOL_INFO gsym = (PSYMBOL_INFO)global_buf;
        gsym->SizeOfStruct = sizeof(SYMBOL_INFO);
        gsym->MaxNameLen = 255;
        if (!SymFromName(dbg->sym_handle, name, gsym))
        {
            printf("symbol '%s' not found\n", name);
            return;
        }
        sym = gsym;
    }

    /* Resolve address */
    DWORD64 addr = resolve_var_addr(dbg, &ctx, sym);

    /* Type info */
    DWORD64 mod_base = sym->ModBase;
    DWORD   type_id  = sym->TypeIndex;

    DWORD tag = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, type_id, TI_GET_SYMTAG, &tag);

    ULONG64 byte_len = 0;
    SymGetTypeInfo(dbg->sym_handle, mod_base, type_id, TI_GET_LENGTH, &byte_len);

    /* For pointer: get pointed-to type tag */
    DWORD pointed_tag = 0;
    if (tag == SYM_TAG_POINTER)
    {
        DWORD pt = 0;
        SymGetTypeInfo(dbg->sym_handle, mod_base, type_id, TI_GET_TYPEID, &pt);
        SymGetTypeInfo(dbg->sym_handle, mod_base, pt, TI_GET_SYMTAG, &pointed_tag);
    }

    /* UDT name */
    char udt_name[128] = {0};
    if (tag == SYM_TAG_UDT)
    {
        WCHAR *wn = NULL;
        SymGetTypeInfo(dbg->sym_handle, mod_base, type_id, TI_GET_SYMNAME, &wn);
        if (wn)
        {
            WideCharToMultiByte(CP_ACP, 0, wn, -1,
                udt_name, sizeof(udt_name)-1, NULL, NULL);
            LocalFree(wn);
        }
    }

    printf("name    : %s\n", sym->Name);
    printf("address : 0x%llx\n", (unsigned long long)addr);
    printf("size    : %llu bytes\n", (unsigned long long)(byte_len ? byte_len : sym->Size));
    printf("type    : %s (tag=%u)", sym_tag_name(tag), tag);
    if (udt_name[0]) printf(" [%s]", udt_name);
    if (tag == SYM_TAG_POINTER)
        printf(" -> %s (tag=%u)", sym_tag_name(pointed_tag), pointed_tag);
    printf("\n");
    printf("flags   : 0x%x", sym->Flags);
    if (sym->Flags & SYMFLAG_REGREL)  printf(" REGREL(reg=%u,off=%lld)",
        sym->Register, (long long)(LONG)sym->Address);
    if (sym->Flags & SYMFLAG_REGISTER) printf(" REGISTER(%u)", sym->Register);
    printf("\n");
    printf("type_id : %u  mod_base: 0x%llx\n",
        type_id, (unsigned long long)mod_base);
}

typedef struct {
    const char *filter;   /* NULL or filename substring to filter */
    char        cur_file_buf[MAX_PATH];
    int         count;
} lines_enum_ctx_t;

static BOOL CALLBACK lines_enum_cb(PSRCCODEINFO linfo, PVOID ctx)
{
    lines_enum_ctx_t *c = (lines_enum_ctx_t *)ctx;

    /* Apply filename filter */
    if (c->filter && c->filter[0])
    {
        if (!strstr(linfo->FileName, c->filter))
            return TRUE;
    }

    /* Print filename header when file changes */
    if (strcmp(linfo->FileName, c->cur_file_buf) != 0)
    {
        strncpy_s(c->cur_file_buf, sizeof(c->cur_file_buf), linfo->FileName, sizeof(c->cur_file_buf) - 1);
        printf("\n%s:\n", c->cur_file_buf);
    }

    printf("  line %-5u  addr 0x%llx\n",
           linfo->LineNumber,
           (unsigned long long)linfo->Address);
    c->count++;
    return TRUE;
}

void print_line_info(debugger_t *dbg, const char *filename_filter)
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    /* Enumerate all loaded modules to get their base addresses */
    DWORD64 mod_base = SymGetModuleBase64(dbg->sym_handle, ctx.Rip);

    lines_enum_ctx_t lctx = {0};
    lctx.filter = (filename_filter && filename_filter[0]) ? filename_filter : NULL;

    BOOL ok = SymEnumLines(dbg->sym_handle, mod_base, NULL, NULL,
                           lines_enum_cb, &lctx);
    if (!ok || lctx.count == 0)
    {
        printf("no line info found");
        if (lctx.filter) printf(" for '%s'", lctx.filter);
        printf("\n");
    }
    else
    {
        printf("\n%d line(s) found\n", lctx.count);
    }
}

/* ------------------------------------------------------------------ */
/* show variables (locals / args / globals)                           */
/* ------------------------------------------------------------------ */
#define SHOW_ENUM_MAX_NAMES 512

typedef struct {
    const char *what;      /* "locals", "args", "globals" */
    debugger_t *dbg;
    int count;
    char names[SHOW_ENUM_MAX_NAMES][128];  /* matched names, printed after
                                               enumeration finishes (see below) */
    int name_count;
} show_enum_ctx_t;

static BOOL CALLBACK show_enum_cb(PSYMBOL_INFO sym, ULONG size, PVOID ctx)
{
    show_enum_ctx_t *c = (show_enum_ctx_t*)ctx;

    /* Filter by scope */
    if (strcmp(c->what, "locals") == 0)
    {
        if (!(sym->Flags & SYMFLAG_REGREL) && !(sym->Flags & SYMFLAG_LOCAL))
            return TRUE;
        /* exclude parameters */
        if (sym->Flags & SYMFLAG_PARAMETER)
            return TRUE;
    }
    else if (strcmp(c->what, "args") == 0)
    {
        if (!(sym->Flags & SYMFLAG_PARAMETER))
            return TRUE;
    }
    else if (strcmp(c->what, "globals") == 0)
    {
        /* globals have no parameter/local/regrel flags and are not registers */
        if ((sym->Flags & SYMFLAG_PARAMETER) || (sym->Flags & SYMFLAG_REGREL)
            || (sym->Flags & SYMFLAG_LOCAL) || (sym->Flags & SYMFLAG_REGISTER))
            return TRUE;
        if (sym->Tag != 7 /* SymTagData: exclude functions/types picked up
                              by the module-wide walk below */)
            return TRUE;
    }
    else
    {
        return FALSE;
    }

    /* Defer to after SymEnumSymbols returns: expr_print() triggers its own
       SymEnumSymbols call (via expr_lookup_symbol), and calling that
       reentrantly on the same handle from inside this callback corrupts/
       truncates the enumeration in progress. Just collect the name here. */
    if (c->name_count < SHOW_ENUM_MAX_NAMES)
        strncpy_s(c->names[c->name_count], sizeof(c->names[0]), sym->Name, _TRUNCATE);
    c->name_count++;
    return TRUE;
}

void show_variables(debugger_t *dbg, const char *what)
{
    if (!what || !what[0])
    {
        printf("usage: show [locals|args|globals]\n");
        return;
    }

    if (strcmp(what, "locals") != 0 &&
        strcmp(what, "args") != 0 &&
        strcmp(what, "globals") != 0)
    {
        printf("usage: show [locals|args|globals]\n");
        return;
    }

    if (!dbg->thread || !dbg->process)
    {
        printf("no active process\n");
        return;
    }

    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);

    show_enum_ctx_t sctx = {0};
    sctx.what = what;
    sctx.dbg  = dbg;

    if (strcmp(what, "globals") == 0)
    {
        /* SymEnumSymbols with BaseOfDll=0 only walks the current context's
           scope (locals/params); it never reaches true global variables.
           Enumerate the whole module by its real base address instead. */
        DWORD64 mod_base = SymGetModuleBase64(dbg->sym_handle, ctx.Rip);
        SymEnumSymbols(dbg->sym_handle, mod_base, "*", show_enum_cb, &sctx);
    }
    else
    {
        IMAGEHLP_STACK_FRAME img_frame = {0};
        img_frame.InstructionOffset = ctx.Rip;
        img_frame.FrameOffset = ctx.Rsp;
        img_frame.StackOffset = ctx.Rsp;

        SymSetContext(dbg->sym_handle, &img_frame, NULL);

        SymEnumSymbols(dbg->sym_handle, 0, "*", show_enum_cb, &sctx);
    }

    int limit = sctx.name_count < SHOW_ENUM_MAX_NAMES ? sctx.name_count : SHOW_ENUM_MAX_NAMES;
    for (int i = 0; i < limit; i++)
    {
        expr_print(dbg, sctx.names[i]);
        sctx.count++;
    }

    if (sctx.count == 0)
        printf("no %s found\n", what);
    else
        printf("%d %s(s) found\n", sctx.count, what);
}