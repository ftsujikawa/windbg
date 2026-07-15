#pragma execution_character_set("utf-8")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "expr_internal.h"

/* ------------------------------------------------------------------ */
/* Type name lookup helpers                                            */
/* ------------------------------------------------------------------ */

/* Known primitive type names -> byte size (also used by expr_parser.y
 * for cast / sizeof(type) resolution) */
int builtin_type_size(const char *name)
{
    if (strcmp(name, "char")   == 0) return 1;
    if (strcmp(name, "short")  == 0) return 2;
    if (strcmp(name, "int")    == 0) return 4;
    if (strcmp(name, "long")   == 0) return 8;
    if (strcmp(name, "float")  == 0) return 4;
    if (strcmp(name, "double") == 0) return 8;
    if (strcmp(name, "void")   == 0) return 0;
    /* unsigned/signed variants */
    if (strncmp(name, "unsigned", 8) == 0) return 4;
    if (strncmp(name, "signed",   6) == 0) return 4;
    if (strcmp(name, "size_t")    == 0) return 8;
    if (strcmp(name, "ptrdiff_t") == 0) return 8;
    if (strcmp(name, "DWORD")     == 0) return 4;
    if (strcmp(name, "DWORD64")   == 0) return 8;
    if (strcmp(name, "BYTE")      == 0) return 1;
    if (strcmp(name, "WORD")      == 0) return 2;
    if (strcmp(name, "QWORD")     == 0) return 8;
    return -1;
}

expr_val_t make_err(const char *msg)
{
    expr_val_t v = {0};
    strncpy_s(v.errmsg, sizeof(v.errmsg), msg, _TRUNCATE);
    return v;
}

expr_val_t make_val(long long n)
{
    expr_val_t v = {0};
    v.value     = n;
    v.byte_size = 8;
    return v;
}

/* Read the current value of an lvalue into v.value */
void load_lvalue(debugger_t *dbg, expr_val_t *v)
{
    if (!v->is_lvalue) return;
    if (!v->addr) return;
    ULONG sz = v->byte_size ? v->byte_size : 8;
    if (sz > 8) sz = 8;
    unsigned long long raw = 0;
    SIZE_T n = 0;
    if (!ReadProcessMemory(dbg->process, (void*)v->addr, &raw, sz, &n))
        return;
    if (sz <= 4)
        v->value = (long long)(int)(unsigned int)raw;
    else
        v->value = (long long)raw;

    /* If the lvalue is a floating-point type, also decode its real value. */
    if (v->mod_base && v->type_id)
    {
        DWORD tag = 0, bt = 0;
        SymGetTypeInfo(dbg->sym_handle, v->mod_base, v->type_id,
                       TI_GET_SYMTAG, &tag);
        if (tag == SYM_TAG_BASETYPE)
        {
            SymGetTypeInfo(dbg->sym_handle, v->mod_base, v->type_id,
                           TI_GET_BASETYPE, &bt);
            if (bt == 8) /* btFloat */
            {
                v->is_float = 1;
                if (sz == 4)
                {
                    float f;
                    unsigned int u = (unsigned int)(raw & 0xffffffff);
                    memcpy(&f, &u, sizeof(f));
                    v->fvalue = (double)f;
                }
                else if (sz == 8)
                {
                    double d;
                    memcpy(&d, &raw, sizeof(d));
                    v->fvalue = d;
                }
                else
                {
                    v->fvalue = 0.0;
                }
            }
        }
    }
}

/* sizeof helper: given a type_id, return byte count */
ULONG type_sizeof(debugger_t *dbg, DWORD64 modbase, DWORD type_id)
{
    ULONG64 len = 0;
    SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_LENGTH, &len);
    return (ULONG)len;
}

typedef struct {
    char target[256];
    int found;
    char sym_buf[sizeof(SYMBOL_INFO) + 256];
} enum_ctx_t;

static BOOL CALLBACK enum_sym_cb(PSYMBOL_INFO si, ULONG size, PVOID ctx)
{
    enum_ctx_t *c = (enum_ctx_t*)ctx;
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

int expr_lookup_symbol(debugger_t *dbg, const char *name,
                        DWORD64 *out_addr, DWORD64 *out_modbase,
                        DWORD *out_typeid, ULONG *out_size)
{
    enum_ctx_t ctx = {0};
    strncpy_s(ctx.target, sizeof(ctx.target), name, _TRUNCATE);
    SymEnumSymbols(dbg->sym_handle, 0, "*", enum_sym_cb, &ctx);

    PSYMBOL_INFO si;
    char global_buf[sizeof(SYMBOL_INFO) + 256];

    if (ctx.found)
    {
        si = (PSYMBOL_INFO)ctx.sym_buf;
    }
    else
    {
        /* SymEnumSymbols with BaseOfDll=0 only walks the current scope's
           locals/params -- it never reaches true global variables. Fall
           back to SymFromName, which resolves module-level symbols
           (globals, functions) directly by name. */
        PSYMBOL_INFO gsi = (PSYMBOL_INFO)global_buf;
        gsi->SizeOfStruct = sizeof(SYMBOL_INFO);
        gsi->MaxNameLen = 255;
        if (!SymFromName(dbg->sym_handle, name, gsi))
            return 0;
        si = gsi;
    }

    /* Resolve address */
    CONTEXT tctx = {0};
    tctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &tctx);

    DWORD64 addr = 0;
    if (si->Flags & SYMFLAG_REGREL)
    {
        long long offset = (long long)(LONG64)si->Address;
        if (si->Register == 335) /* CV_AMD64_RSP */
            addr = tctx.Rsp + (DWORD64)offset;
        else if (si->Register == 334) /* CV_AMD64_RBP */
            addr = tctx.Rbp + (DWORD64)offset;
        else
            addr = tctx.Rsp + (DWORD64)offset;
    }
    else if (si->Flags & SYMFLAG_REGISTER)
    {
        /* value lives in a register: read it directly */
        DWORD64 reg_val = 0;
        switch (si->Register)
        {
        case 328: reg_val = tctx.Rax; break; /* CV_AMD64_RAX */
        case 329: reg_val = tctx.Rcx; break; /* CV_AMD64_RCX */
        case 330: reg_val = tctx.Rdx; break; /* CV_AMD64_RDX */
        case 331: reg_val = tctx.Rbx; break; /* CV_AMD64_RBX */
        case 335: reg_val = tctx.Rsp; break; /* CV_AMD64_RSP */
        case 334: reg_val = tctx.Rbp; break; /* CV_AMD64_RBP */
        case 336: reg_val = tctx.Rsi; break; /* CV_AMD64_RSI */
        case 337: reg_val = tctx.Rdi; break; /* CV_AMD64_RDI */
        case 344: reg_val = tctx.R8;  break; /* CV_AMD64_R8  */
        case 345: reg_val = tctx.R9;  break; /* CV_AMD64_R9  */
        case 346: reg_val = tctx.R10; break;
        case 347: reg_val = tctx.R11; break;
        case 348: reg_val = tctx.R12; break;
        case 349: reg_val = tctx.R13; break;
        case 350: reg_val = tctx.R14; break;
        case 351: reg_val = tctx.R15; break;
        /* 32-bit sub-registers */
        case  17: reg_val = tctx.Rax & 0xffffffff; break; /* CV_AMD64_EAX */
        case  18: reg_val = tctx.Rcx & 0xffffffff; break;
        case  19: reg_val = tctx.Rdx & 0xffffffff; break;
        case  20: reg_val = tctx.Rbx & 0xffffffff; break;
        case  21: reg_val = tctx.Rsp & 0xffffffff; break;
        case  22: reg_val = tctx.Rbp & 0xffffffff; break;
        case  23: reg_val = tctx.Rsi & 0xffffffff; break;
        case  24: reg_val = tctx.Rdi & 0xffffffff; break;
        default:  reg_val = 0; break;
        }
        /* out_size=0 signals "register variable, value in out_addr" */
        *out_addr    = reg_val;
        *out_modbase = si->ModBase;
        *out_typeid  = si->TypeIndex;
        *out_size    = 0;  /* sentinel: register variable */
        return 1;
    }
    else
    {
        addr = si->Address;
    }

    *out_addr    = addr;
    *out_modbase = si->ModBase;
    *out_typeid  = si->TypeIndex;
    *out_size    = si->Size;
    return 1;
}

int lookup_member(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                   const char *member, DWORD *out_typeid, DWORD *out_offset)
{
    DWORD child_count = 0;
    if (!SymGetTypeInfo(dbg->sym_handle, modbase, type_id,
                        TI_GET_CHILDRENCOUNT, &child_count))
        return 0;

    TI_FINDCHILDREN_PARAMS *p =
        (TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS)
                                        + child_count * sizeof(ULONG));
    if (!p) return 0;
    p->Count = child_count;
    p->Start = 0;

    if (!SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_FINDCHILDREN, p))
    { free(p); return 0; }

    for (DWORD i = 0; i < child_count; i++)
    {
        DWORD child_id = p->ChildId[i];
        WCHAR *wname = NULL;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_id,
                       TI_GET_SYMNAME, &wname);
        if (!wname) continue;

        char aname[128] = {0};
        WideCharToMultiByte(CP_ACP, 0, wname, -1, aname, sizeof(aname)-1, NULL, NULL);
        LocalFree(wname);

        if (strcmp(aname, member) == 0)
        {
            DWORD offset = 0;
            SymGetTypeInfo(dbg->sym_handle, modbase, child_id,
                           TI_GET_OFFSET, &offset);
            DWORD child_type = 0;
            SymGetTypeInfo(dbg->sym_handle, modbase, child_id,
                           TI_GET_TYPEID, &child_type);
            *out_typeid = child_type;
            *out_offset = offset;
            free(p);
            return 1;
        }
    }
    free(p);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

eval_status_t expr_eval(debugger_t *dbg, const char *expr, expr_val_t *out)
{
    if (!dbg->thread || !dbg->process)
    {
        strncpy_s(out->errmsg, sizeof(out->errmsg), "no active process", _TRUNCATE);
        return EVAL_ERR;
    }

    /* Set symbol context */
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(dbg->thread, &ctx);
    IMAGEHLP_STACK_FRAME frm = {0};
    frm.InstructionOffset = ctx.Rip;
    frm.FrameOffset       = ctx.Rsp;
    frm.StackOffset       = ctx.Rsp;
    SymSetContext(dbg->sym_handle, &frm, NULL);

    if (expr_parse(dbg, expr, out) != 0)
        return EVAL_ERR;
    return EVAL_OK;
}

/* ------------------------------------------------------------------ */
/* Print helpers (same style as original print_variable)              */
/* ------------------------------------------------------------------ */
static void print_int_fmt(const char *label, long long value,
                          ULONG byte_size, print_fmt_t fmt, int newline,
                          const char *type_name);

/* Map a BaseType (dbghelp.h BasicType enum) + byte size to a C type name */
static const char *basetype_name(DWORD basetype, ULONG len)
{
    switch (basetype)
    {
    case 1: return "void";           /* btVoid  */
    case 2: return "char";           /* btChar  */
    case 3: return "wchar_t";        /* btWChar */
    case 6:                          /* btInt   */
        switch (len)
        {
        case 1:  return "signed char";
        case 2:  return "short";
        case 8:  return "__int64";
        default: return "int";
        }
    case 7:                          /* btUInt  */
        switch (len)
        {
        case 1:  return "unsigned char";
        case 2:  return "unsigned short";
        case 8:  return "unsigned __int64";
        default: return "unsigned int";
        }
    case 8:  return (len == 8) ? "double" : "float";  /* btFloat */
    case 10: return "bool";          /* btBool */
    case 13: return "long";          /* btLong */
    case 14: return "unsigned long"; /* btULong */
    case 31: return "HRESULT";       /* btHresult */
    case 32: return "char16_t";      /* btChar16 */
    case 33: return "char32_t";      /* btChar32 */
    case 34: return "char8_t";       /* btChar8 */
    default: return "";
    }
}

/* Forward declaration: defined later in this file */
static const char *udt_kind_prefix(debugger_t *dbg, DWORD64 modbase, DWORD type_id);

/* Resolve a printable C-style type name for any type_id: base types are
 * named from their BasicType+size, pointers/arrays are built up
 * recursively from their pointed-to/element type, everything else
 * (UDT/enum/typedef) uses its own DbgHelp symbol name. */
static void get_type_name(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                          char *buf, size_t buflen)
{
    buf[0] = '\0';
    if (!modbase || !type_id) return;

    DWORD tag = 0;
    SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_SYMTAG, &tag);

    if (tag == SYM_TAG_POINTER)
    {
        DWORD inner_id = 0;
        char inner[112] = {0};
        if (SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_TYPEID, &inner_id))
            get_type_name(dbg, modbase, inner_id, inner, sizeof(inner));
        snprintf(buf, buflen, "%s *", inner[0] ? inner : "void");
        return;
    }

    if (tag == SYM_TAG_ARRAYTYPE)
    {
        DWORD elem_id = 0;
        ULONG64 total_len = 0, elem_len = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_TYPEID, &elem_id);
        SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_LENGTH, &total_len);
        SymGetTypeInfo(dbg->sym_handle, modbase, elem_id, TI_GET_LENGTH, &elem_len);
        if (elem_len == 0) elem_len = 1;

        char inner[112] = {0};
        get_type_name(dbg, modbase, elem_id, inner, sizeof(inner));
        snprintf(buf, buflen, "%s [%llu]", inner[0] ? inner : "?",
            (unsigned long long)(total_len / elem_len));
        return;
    }

    if (tag == SYM_TAG_BASETYPE)
    {
        DWORD basetype = 0;
        ULONG64 len = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_BASETYPE, &basetype);
        SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_LENGTH, &len);
        strncpy_s(buf, buflen, basetype_name(basetype, (ULONG)len), _TRUNCATE);
        return;
    }

    /* UDT: include struct/union prefix */
    if (tag == SYM_TAG_UDT)
    {
        const char *prefix = udt_kind_prefix(dbg, modbase, type_id);
        WCHAR *wname = NULL;
        if (SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_SYMNAME, &wname)
            && wname)
        {
            char name[112] = {0};
            WideCharToMultiByte(CP_ACP, 0, wname, -1, name, sizeof(name), NULL, NULL);
            snprintf(buf, buflen, "%s%s", prefix, name);
            LocalFree(wname);
        }
        return;
    }

    /* Enum / Typedef / others: use the symbol's own name */
    WCHAR *wname = NULL;
    if (SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_SYMNAME, &wname)
        && wname)
    {
        WideCharToMultiByte(CP_ACP, 0, wname, -1, buf, (int)buflen, NULL, NULL);
        LocalFree(wname);
    }
}

/* UdtKind values (cvconst.h): UdtStruct=0, UdtClass=1, UdtUnion=2, UdtInterface=3 */
#define UDT_KIND_UNION 2

/* "struct " / "union " prefix for a UDT type name, gdb-style */
static const char *udt_kind_prefix(debugger_t *dbg, DWORD64 modbase, DWORD type_id)
{
    DWORD kind = 0;
    if (SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_UDTKIND, &kind)
        && kind == UDT_KIND_UNION)
        return "union ";
    return "struct ";
}

static void print_struct_ex(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                             DWORD64 base_addr, int depth, print_fmt_t fmt);
static void print_char_value(int c);

static void print_val(debugger_t *dbg, const char *label,
                      DWORD64 modbase, DWORD type_id, ULONG byte_size,
                      DWORD64 addr, long long value, int depth, print_fmt_t fmt,
                      int newline)
{
    const char *nl = newline ? "\n" : "";
    DWORD tag = 0;
    SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_SYMTAG, &tag);

    char indent[64] = {0};
    if (dbg->print_pretty)
        for (int i = 0; i < depth && i < 30; i++) { indent[i*2] = ' '; indent[i*2+1] = ' '; }

    char tname[128] = {0};
    if (dbg->print_pretty)
        get_type_name(dbg, modbase, type_id, tname, sizeof(tname));

    char ilabel[512];
    snprintf(ilabel, sizeof(ilabel), "%s%s", indent, label);

    if (tag == SYM_TAG_UDT)
    {
        if (tname[0])
            printf("%s = (%s) {%s", ilabel, tname, nl);
        else
            printf("%s = {%s", ilabel, nl);
        print_struct_ex(dbg, modbase, type_id, addr, depth + 1, fmt);
        printf("%s}%s", indent, nl);
        return;
    }
    if (tag == SYM_TAG_ARRAYTYPE)
    {
        DWORD elem_type = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_TYPEID, &elem_type);
        ULONG64 total = 0, elem_len = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_LENGTH, &total);
        SymGetTypeInfo(dbg->sym_handle, modbase, elem_type, TI_GET_LENGTH, &elem_len);
        if (elem_len == 0) elem_len = 1;
        DWORD count = (DWORD)(total / elem_len);

        DWORD elem_tag = 0, elem_bt = 0;
        ULONG64 elem_len2 = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, elem_type, TI_GET_SYMTAG, &elem_tag);
        SymGetTypeInfo(dbg->sym_handle, modbase, elem_type, TI_GET_BASETYPE, &elem_bt);
        SymGetTypeInfo(dbg->sym_handle, modbase, elem_type, TI_GET_LENGTH, &elem_len2);
        int is_char_elem = (elem_tag == SYM_TAG_BASETYPE &&
                            (elem_bt == 3 ||
                             (elem_len2 == 1 && (elem_bt == 6 || elem_bt == 7))));
        if (!is_char_elem && elem_len == 1) is_char_elem = 1;

        char elem_tname[128] = {0};
        get_type_name(dbg, modbase, elem_type, elem_tname, sizeof(elem_tname));

        if (dbg->print_pretty)
        {
            printf("%s = {%s", ilabel, nl);
            for (DWORD i = 0; i < count; i++)
            {
                DWORD64 ea = addr + i * elem_len;
                unsigned long long val = 0; SIZE_T n;
                SIZE_T read_len = (elem_len > sizeof(val)) ? sizeof(val) : (SIZE_T)elem_len;
                ReadProcessMemory(dbg->process, (void*)ea, &val, read_len, &n);
                char idx[64]; snprintf(idx, sizeof(idx), "%s  [%u]", indent, i);
                if (is_char_elem && fmt == FMT_DEFAULT)
                {
                    if (elem_tname[0])
                        printf("%s = (%s) ", idx, elem_tname);
                    else
                        printf("%s = ", idx);
                    print_char_value((int)(val & 0xff));
                    printf(" (0x%x)%s", (unsigned int)(val & 0xff), nl);
                }
                else
                    print_int_fmt(idx, (long long)val, (ULONG)elem_len, fmt, 1, elem_tname);
            }
            printf("%s}%s", indent, nl);
        }
        else
        {
            printf("%s = { ", ilabel);
            for (DWORD i = 0; i < count; i++)
            {
                DWORD64 ea = addr + i * elem_len;
                unsigned long long val = 0; SIZE_T n;
                SIZE_T read_len = (elem_len > sizeof(val)) ? sizeof(val) : (SIZE_T)elem_len;
                ReadProcessMemory(dbg->process, (void*)ea, &val, read_len, &n);
                if (i > 0) printf(", ");
                if (is_char_elem && fmt == FMT_DEFAULT)
                {
                    if (elem_tname[0])
                        printf("[%u] = (%s) ", i, elem_tname);
                    else
                        printf("[%u] = ", i);
                    print_char_value((int)(val & 0xff));
                    printf(" (0x%x)", (unsigned int)(val & 0xff));
                }
                else if (fmt == FMT_HEX)
                    printf("[%u] = 0x%llx", i, (unsigned long long)val);
                else if (fmt == FMT_OCT)
                    printf("[%u] = 0%llo", i, (unsigned long long)val);
                else if (fmt == FMT_BIN)
                {
                    int bits = (elem_len > 0 && elem_len <= 8) ? (int)elem_len * 8 : 64;
                    printf("[%u] = 0b", i);
                    for (int b = bits - 1; b >= 0; b--)
                        putchar(((val >> b) & 1) ? '1' : '0');
                }
                else if (fmt == FMT_DEC)
                {
                    if (elem_len <= 4)
                        printf("[%u] = %d", i, (int)(long long)val);
                    else
                        printf("[%u] = %lld", i, (long long)val);
                }
                else if (fmt == FMT_CHAR)
                    printf("[%u] = '%c' (%lld)", i, (int)(val & 0xff), (long long)val);
                else /* FMT_DEFAULT */
                {
                    if (elem_len <= 4)
                        printf("[%u] = %d (0x%x)", i, (int)val, (unsigned int)val);
                    else
                        printf("[%u] = %lld (0x%llx)", i, (long long)val, val);
                }
            }
            printf(" }%s", nl);
        }
        return;
    }
    if (tag == SYM_TAG_POINTER)
    {
        /* Check if it's a char* and show string after address */
        DWORD pt_type = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_TYPEID, &pt_type);
        DWORD resolved = pt_type;
        for (int di = 0; di < 8; di++)
        {
            DWORD rtag = 0;
            SymGetTypeInfo(dbg->sym_handle, modbase, resolved, TI_GET_SYMTAG, &rtag);
            if (rtag == SYM_TAG_BASETYPE) break;
            DWORD inner = 0;
            if (!SymGetTypeInfo(dbg->sym_handle, modbase, resolved, TI_GET_TYPEID, &inner)
                || inner == 0 || inner == resolved) break;
            resolved = inner;
        }
        DWORD pt_tag = 0;
        ULONG64 pt_len = 0;
        DWORD pt_basetype = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, resolved, TI_GET_SYMTAG,   &pt_tag);
        SymGetTypeInfo(dbg->sym_handle, modbase, resolved, TI_GET_LENGTH,   &pt_len);
        SymGetTypeInfo(dbg->sym_handle, modbase, resolved, TI_GET_BASETYPE, &pt_basetype);
        ULONG64 orig_len2 = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, pt_type, TI_GET_LENGTH, &orig_len2);
        int is_char_ptr = ((pt_tag == SYM_TAG_BASETYPE &&
                            (pt_basetype == 3 || pt_basetype == 5)) ||
                           (pt_len == 1) ||
                           (orig_len2 == 1));
        if (is_char_ptr && fmt != FMT_HEX && fmt != FMT_DEC &&
            fmt != FMT_OCT && fmt != FMT_BIN)
        {
            /* Show address + string content */
            char strbuf[256] = {0};
            SIZE_T nr = 0;
            if (value)
                ReadProcessMemory(dbg->process, (void*)(DWORD64)value,
                                  strbuf, sizeof(strbuf)-1, &nr);
            strbuf[sizeof(strbuf)-1] = '\0';
            if (tname[0])
                printf("%s = (%s) 0x%llx \"%s\"%s", ilabel, tname,
                       (unsigned long long)value, value ? strbuf : "", nl);
            else
                printf("%s = 0x%llx \"%s\"%s", ilabel,
                       (unsigned long long)value, value ? strbuf : "", nl);
        }
        else if (fmt == FMT_DEFAULT || fmt == FMT_HEX)
        {
            if (tname[0])
                printf("%s = (%s) 0x%llx%s", ilabel, tname, (unsigned long long)value, nl);
            else
                printf("%s = 0x%llx%s", ilabel, (unsigned long long)value, nl);
        }
        else
            print_int_fmt(ilabel, value, byte_size, fmt, newline, tname);
        return;
    }
    {
        /* Check if this is a char type (btChar=3 or size-1 basetype) */
        DWORD scalar_tag = 0, scalar_basetype = 0;
        ULONG64 scalar_len = 0;
        DWORD resolved_id = type_id;
        for (int di = 0; di < 8; di++)
        {
            SymGetTypeInfo(dbg->sym_handle, modbase, resolved_id, TI_GET_SYMTAG, &scalar_tag);
            if (scalar_tag == SYM_TAG_BASETYPE) break;
            DWORD inner = 0;
            if (!SymGetTypeInfo(dbg->sym_handle, modbase, resolved_id, TI_GET_TYPEID, &inner)
                || inner == 0 || inner == resolved_id) break;
            resolved_id = inner;
        }
        SymGetTypeInfo(dbg->sym_handle, modbase, resolved_id, TI_GET_BASETYPE, &scalar_basetype);
        SymGetTypeInfo(dbg->sym_handle, modbase, resolved_id, TI_GET_LENGTH,   &scalar_len);
        int is_char = (scalar_tag == SYM_TAG_BASETYPE &&
                       (scalar_basetype == 3 ||
                        (scalar_len == 1 && (scalar_basetype == 6 || scalar_basetype == 7))));
        if (!is_char)
        {
            ULONG64 orig_len3 = 0;
            SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_GET_LENGTH, &orig_len3);
            if (orig_len3 == 1) is_char = 1;
        }

        int is_float = (scalar_tag == SYM_TAG_BASETYPE && scalar_basetype == 8); /* btFloat */
        double fvalue = 0.0;
        if (is_float)
        {
            if (scalar_len == 4)
            {
                float f;
                unsigned int u = (unsigned int)(value & 0xffffffff);
                memcpy(&f, &u, sizeof(f));
                fvalue = (double)f;
            }
            else if (scalar_len == 8)
            {
                memcpy(&fvalue, &value, sizeof(double));
            }
        }

        if (is_float && fmt == FMT_DEFAULT)
        {
            if (tname[0])
                printf("%s = (%s) %f%s", ilabel, tname, fvalue, nl);
            else
                printf("%s = %f%s", ilabel, fvalue, nl);
        }
        else if (fmt == FMT_DEFAULT)
        {
            if (is_char)
            {
                printf("%s = ", ilabel);
                print_char_value((int)(value & 0xff));
                printf(" (0x%x)%s", (unsigned int)(value & 0xff), nl);
            }
            else if (byte_size <= 4)
            {
                if (tname[0])
                    printf("%s = (%s) %d (0x%x)%s", ilabel, tname,
                           (int)value, (unsigned int)(unsigned long long)value, nl);
                else
                    printf("%s = %d (0x%x)%s", ilabel,
                           (int)value, (unsigned int)(unsigned long long)value, nl);
            }
            else
            {
                if (tname[0])
                    printf("%s = (%s) %lld (0x%llx)%s", ilabel, tname,
                           value, (unsigned long long)value, nl);
                else
                    printf("%s = %lld (0x%llx)%s", ilabel,
                           value, (unsigned long long)value, nl);
            }
        }
        else
            print_int_fmt(ilabel, value, byte_size, fmt, newline, tname);
    }
}

static void print_struct_ex(debugger_t *dbg, DWORD64 modbase, DWORD type_id,
                             DWORD64 base_addr, int depth, print_fmt_t fmt)
{
    if (depth > 5) return;

    DWORD child_count = 0;
    if (!SymGetTypeInfo(dbg->sym_handle, modbase, type_id,
                        TI_GET_CHILDRENCOUNT, &child_count)) return;

    TI_FINDCHILDREN_PARAMS *p =
        (TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS)
                                        + child_count * sizeof(ULONG));
    if (!p) return;
    p->Count = child_count;
    p->Start = 0;
    if (!SymGetTypeInfo(dbg->sym_handle, modbase, type_id, TI_FINDCHILDREN, p))
    { free(p); return; }

    for (DWORD i = 0; i < child_count; i++)
    {
        DWORD child_id = p->ChildId[i];
        WCHAR *wname = NULL;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_id, TI_GET_SYMNAME, &wname);
        if (!wname) continue;
        char aname[128] = {0};
        WideCharToMultiByte(CP_ACP, 0, wname, -1, aname, sizeof(aname)-1, NULL, NULL);
        LocalFree(wname);

        DWORD offset = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_id, TI_GET_OFFSET, &offset);
        DWORD child_type = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_id, TI_GET_TYPEID, &child_type);
        ULONG64 child_len = 0;
        SymGetTypeInfo(dbg->sym_handle, modbase, child_type, TI_GET_LENGTH, &child_len);
        if (child_len == 0) child_len = 4;

        DWORD64 member_addr = base_addr + offset;
        unsigned long long val = 0;
        SIZE_T n;
        SIZE_T read_len = (child_len > sizeof(val)) ? sizeof(val) : (SIZE_T)child_len;
        ReadProcessMemory(dbg->process, (void*)member_addr, &val, read_len, &n);

        int pretty = dbg->print_pretty;
        int member_nl = pretty ? 1 : 0;
        print_val(dbg, aname, modbase, child_type, (ULONG)child_len,
                  member_addr, (long long)val, depth, fmt, member_nl);
        if (!pretty && i + 1 < child_count)
            printf(", ");
    }
    free(p);
}

/* Print a single byte value as a C character literal */
static void print_char_value(int c)
{
    c = c & 0xff;
    switch (c)
    {
    case 0:       printf("'\\0'"); break;
    case '\n':    printf("'\\n'"); break;
    case '\t':    printf("'\\t'"); break;
    case '\r':    printf("'\\r'"); break;
    case '\\':    printf("'\\\\'"); break;
    case '\'':    printf("'\\''"); break;
    case '\a':    printf("'\\a'"); break;
    case '\b':    printf("'\\b'"); break;
    case '\f':    printf("'\\f'"); break;
    case '\v':    printf("'\\v'"); break;
    default:
        if (c >= 32 && c <= 126)
            printf("'%c'", c);
        else
            printf("'\\x%02x'", c);
        break;
    }
}

/* Format a single integer value according to fmt */
static void print_int_fmt(const char *label, long long value,
                          ULONG byte_size, print_fmt_t fmt, int newline,
                          const char *type_name)
{
    const char *nl = newline ? "\n" : "";
    unsigned long long uv = (unsigned long long)value;
    char lbl[512];
    if (type_name && type_name[0])
        snprintf(lbl, sizeof(lbl), "%s = (%s)", label, type_name);
    else
        snprintf(lbl, sizeof(lbl), "%s =", label);
    switch (fmt)
    {
    case FMT_HEX:
        printf("%s 0x%llx%s", lbl, uv, nl);
        break;
    case FMT_OCT:
        printf("%s 0%llo%s", lbl, uv, nl);
        break;
    case FMT_BIN:
    {
        int bits = (byte_size > 0 && byte_size <= 8) ? (int)byte_size * 8 : 64;
        printf("%s 0b", lbl);
        for (int b = bits - 1; b >= 0; b--)
            putchar((uv >> b) & 1 ? '1' : '0');
        printf("%s", nl);
        break;
    }
    case FMT_CHAR:
        printf("%s '%c' (%lld)%s", lbl, (int)(value & 0xff), value, nl);
        break;
    case FMT_DEC:
        if (byte_size <= 4)
            printf("%s %d%s", lbl, (int)value, nl);
        else
            printf("%s %lld%s", lbl, value, nl);
        break;
    default: /* FMT_DEFAULT */
        if (byte_size <= 4)
            printf("%s %d (0x%x)%s", lbl,
                   (int)value, (unsigned int)uv, nl);
        else
            printf("%s %lld (0x%llx)%s", lbl, value, uv, nl);
        break;
    }
}

/* Read a null-terminated string from debuggee memory */
static void print_string_fmt(debugger_t *dbg, const char *label, DWORD64 addr,
                              const char *type_name)
{
    if (!addr) {
        if (type_name && type_name[0])
            printf("%s = (%s) (null)\n", label, type_name);
        else
            printf("%s = (null)\n", label);
        return;
    }
    char buf[256] = {0};
    SIZE_T n = 0;
    ReadProcessMemory(dbg->process, (void*)addr, buf, sizeof(buf)-1, &n);
    buf[sizeof(buf)-1] = '\0';
    if (type_name && type_name[0])
        printf("%s = (%s) \"%s\"\n", label, type_name, buf);
    else
        printf("%s = \"%s\"\n", label, buf);
}

void expr_print_fmt(debugger_t *dbg, const char *expr_str, print_fmt_t fmt)
{
    expr_val_t v = {0};
    if (expr_eval(dbg, expr_str, &v) != EVAL_OK)
    {
        printf("error: %s\n", v.errmsg);
        return;
    }

    char top_tname[128] = {0};
    if (dbg->print_pretty && v.mod_base && v.type_id)
        get_type_name(dbg, v.mod_base, v.type_id, top_tname, sizeof(top_tname));

    char top_label[512];
    snprintf(top_label, sizeof(top_label), "%s", expr_str);

    /* Address-of: always show as a hex pointer, regardless of underlying type */
    if (v.is_address && (fmt == FMT_DEFAULT || fmt == FMT_HEX))
    {
        char base_name[128] = {0};
        if (v.mod_base && v.type_id)
            get_type_name(dbg, v.mod_base, v.type_id, base_name, sizeof(base_name));
        if (base_name[0])
            printf("%s = (%s *) 0x%llx\n", top_label, base_name, (unsigned long long)v.value);
        else
            printf("%s = 0x%llx\n", top_label, (unsigned long long)v.value);
        return;
    }

    /* /s: treat value as pointer to string */
    if (fmt == FMT_STR)
    {
        print_string_fmt(dbg, top_label, (DWORD64)v.value, top_tname);
        return;
    }

    DWORD tag = 0;
    if (v.mod_base && v.type_id)
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id, TI_GET_SYMTAG, &tag);

    /* Array: only default format shows struct layout; others flatten to elements */
    if (tag == SYM_TAG_ARRAYTYPE && fmt == FMT_DEFAULT)
    {
        DWORD elem_type = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id,
            TI_GET_TYPEID, &elem_type);
        ULONG64 total = 0, elem_len = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id,
            TI_GET_LENGTH, &total);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type,
            TI_GET_LENGTH, &elem_len);
        if (elem_len == 0) elem_len = 1;
        DWORD count = (DWORD)(total / elem_len);

        DWORD elem_tag = 0, elem_bt = 0;
        ULONG64 elem_len2 = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type,
            TI_GET_SYMTAG, &elem_tag);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type,
            TI_GET_BASETYPE, &elem_bt);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type,
            TI_GET_LENGTH, &elem_len2);
        int is_char_elem = (elem_tag == SYM_TAG_BASETYPE &&
                            (elem_bt == 3 ||
                             (elem_len2 == 1 && (elem_bt == 6 || elem_bt == 7))));
        if (!is_char_elem && elem_len == 1) is_char_elem = 1;

        char elem_tname[128] = {0};
        get_type_name(dbg, v.mod_base, elem_type, elem_tname, sizeof(elem_tname));

        if (dbg->print_pretty)
        {
            if (top_tname[0] && !is_char_elem)
                printf("%s = (%s) {\n", top_label, top_tname);
            else
                printf("%s = {\n", top_label);
            for (DWORD i = 0; i < count; i++)
            {
                DWORD64 ea = v.addr + i * elem_len;
                char idx[32];
                snprintf(idx, sizeof(idx), "[%u]", i);
                if (elem_tag == SYM_TAG_UDT)
                {
                    printf("  %s = {\n", idx);
                    print_struct_ex(dbg, v.mod_base, elem_type, ea, 2, FMT_DEFAULT);
                    printf("  }\n");
                }
                else if (is_char_elem)
                {
                    unsigned long long val = 0; SIZE_T n;
                    SIZE_T read_len = (elem_len > sizeof(val)) ? sizeof(val) : (SIZE_T)elem_len;
                    ReadProcessMemory(dbg->process, (void*)ea, &val, read_len, &n);
                    if (elem_tname[0])
                        printf("  %s = (%s) ", idx, elem_tname);
                    else
                        printf("  %s = ", idx);
                    print_char_value((int)(val & 0xff));
                    printf(" (0x%x)\n", (unsigned int)(val & 0xff));
                }
                else
                {
                    unsigned long long val = 0; SIZE_T n;
                    SIZE_T read_len = (elem_len > sizeof(val)) ? sizeof(val) : (SIZE_T)elem_len;
                    ReadProcessMemory(dbg->process, (void*)ea, &val, read_len, &n);
                    if (elem_len <= 4)
                        printf("  [%u] = %d (0x%x)\n", i, (int)val, (unsigned int)val);
                    else
                        printf("  [%u] = %lld (0x%llx)\n", i, (long long)val, val);
                }
            }
            printf("}\n");
        }
        else
        {
            printf("%s = { ", top_label);
            for (DWORD i = 0; i < count; i++)
            {
                DWORD64 ea = v.addr + i * elem_len;
                char idx[32];
                snprintf(idx, sizeof(idx), "[%u]", i);
                if (elem_tag == SYM_TAG_UDT)
                {
                    printf("%s = { ", idx);
                    print_struct_ex(dbg, v.mod_base, elem_type, ea, 2, FMT_DEFAULT);
                    printf(" }");
                }
                else if (is_char_elem)
                {
                    unsigned long long val = 0; SIZE_T n;
                    SIZE_T read_len = (elem_len > sizeof(val)) ? sizeof(val) : (SIZE_T)elem_len;
                    ReadProcessMemory(dbg->process, (void*)ea, &val, read_len, &n);
                    if (i > 0) printf(", ");
                    if (elem_tname[0])
                        printf("[%u] = (%s) ", i, elem_tname);
                    else
                        printf("[%u] = ", i);
                    print_char_value((int)(val & 0xff));
                    printf(" (0x%x)", (unsigned int)(val & 0xff));
                }
                else
                {
                    unsigned long long val = 0; SIZE_T n;
                    SIZE_T read_len = (elem_len > sizeof(val)) ? sizeof(val) : (SIZE_T)elem_len;
                    ReadProcessMemory(dbg->process, (void*)ea, &val, read_len, &n);
                    if (i > 0) printf(", ");
                    if (elem_len <= 4)
                        printf("[%u] = %d (0x%x)", i, (int)val, (unsigned int)val);
                    else
                        printf("[%u] = %lld (0x%llx)", i, (long long)val, val);
                }
            }
            printf(" }\n");
        }
        return;
    }

    /* Array with explicit format: print each element */
    if (tag == SYM_TAG_ARRAYTYPE)
    {
        DWORD elem_type = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id, TI_GET_TYPEID, &elem_type);
        ULONG64 total = 0, elem_len = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id, TI_GET_LENGTH, &total);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type, TI_GET_LENGTH, &elem_len);
        if (elem_len == 0) elem_len = 1;
        DWORD count = (DWORD)(total / elem_len);

        /* Determine whether the element type is a char type. */
        DWORD elem_tag = 0, elem_bt = 0;
        ULONG64 elem_len2 = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type, TI_GET_SYMTAG, &elem_tag);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type, TI_GET_BASETYPE, &elem_bt);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, elem_type, TI_GET_LENGTH, &elem_len2);
        int is_char_elem = (elem_tag == SYM_TAG_BASETYPE &&
                            (elem_bt == 3 ||
                             (elem_len2 == 1 && (elem_bt == 6 || elem_bt == 7))));
        if (!is_char_elem && elem_len == 1) is_char_elem = 1;

        char elem_tname[128] = {0};
        get_type_name(dbg, v.mod_base, elem_type, elem_tname, sizeof(elem_tname));

        if (dbg->print_pretty)
        {
            /* char arrays are shown element-by-element without the type name */
            if (top_tname[0] && !is_char_elem)
                printf("%s = (%s) {\n", top_label, top_tname);
            else
                printf("%s = {\n", top_label);
            for (DWORD i = 0; i < count; i++)
            {
                DWORD64 ea = v.addr + i * elem_len;
                unsigned long long val = 0; SIZE_T n;
                SIZE_T read_len = (elem_len > sizeof(val)) ? sizeof(val) : (SIZE_T)elem_len;
                ReadProcessMemory(dbg->process, (void*)ea, &val, read_len, &n);
                char idx[32]; snprintf(idx, sizeof(idx), "  [%u]", i);
                if (is_char_elem && fmt == FMT_DEFAULT)
                {
                    if (elem_tname[0])
                        printf("%s = (%s) ", idx, elem_tname);
                    else
                        printf("%s = ", idx);
                    print_char_value((int)(val & 0xff));
                    printf(" (0x%x)\n", (unsigned int)(val & 0xff));
                }
                else
                    print_int_fmt(idx, (long long)val, (ULONG)elem_len, fmt, 1, elem_tname);
            }
            printf("}\n");
        }
        else
        {
            printf("%s = { ", top_label);
            for (DWORD i = 0; i < count; i++)
            {
                DWORD64 ea = v.addr + i * elem_len;
                unsigned long long val = 0; SIZE_T n;
                SIZE_T read_len = (elem_len > sizeof(val)) ? sizeof(val) : (SIZE_T)elem_len;
                ReadProcessMemory(dbg->process, (void*)ea, &val, read_len, &n);
                if (i > 0) printf(", ");
                if (is_char_elem && fmt == FMT_DEFAULT)
                {
                    if (elem_tname[0])
                        printf("[%u] = (%s) ", i, elem_tname);
                    else
                        printf("[%u] = ", i);
                    print_char_value((int)(val & 0xff));
                    printf(" (0x%x)", (unsigned int)(val & 0xff));
                }
                else if (fmt == FMT_HEX)
                    printf("[%u] = 0x%llx", i, (unsigned long long)val);
                else if (fmt == FMT_OCT)
                    printf("[%u] = 0%llo", i, (unsigned long long)val);
                else if (fmt == FMT_BIN)
                {
                    int bits = (elem_len > 0 && elem_len <= 8) ? (int)elem_len * 8 : 64;
                    printf("[%u] = 0b", i);
                    for (int b = bits - 1; b >= 0; b--)
                        putchar(((val >> b) & 1) ? '1' : '0');
                }
                else if (fmt == FMT_DEC)
                {
                    if (elem_len <= 4)
                        printf("[%u] = %d", i, (int)(long long)val);
                    else
                        printf("[%u] = %lld", i, (long long)val);
                }
                else if (fmt == FMT_CHAR)
                    printf("[%u] = '%c' (%lld)", i, (int)(val & 0xff), (long long)val);
                else /* FMT_DEFAULT */
                {
                    if (elem_len <= 4)
                        printf("[%u] = %d (0x%x)", i, (int)val, (unsigned int)val);
                    else
                        printf("[%u] = %lld (0x%llx)", i, (long long)val, val);
                }
            }
            printf(" }\n");
        }
        return;
    }

    /* UDT: always show struct members, apply fmt to each member value */
    if (tag == SYM_TAG_UDT)
    {
        if (dbg->print_pretty)
        {
            if (top_tname[0])
                printf("%s = (%s) {\n", top_label, top_tname);
            else
                printf("%s = {\n", top_label);
            print_struct_ex(dbg, v.mod_base, v.type_id, v.addr, 1, fmt);
            printf("}\n");
        }
        else
        {
            printf("%s = { ", top_label);
            print_struct_ex(dbg, v.mod_base, v.type_id, v.addr, 1, fmt);
            printf(" }\n");
        }
        return;
    }

    /* Pointer: check for char* and show string */
    if (tag == SYM_TAG_POINTER)
    {
        DWORD pt_type = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id, TI_GET_TYPEID, &pt_type);

        /* Resolve typedef/alias chain: follow TI_GET_TYPEID until BaseType or dead end */
        DWORD resolved = pt_type;
        for (int depth2 = 0; depth2 < 8; depth2++)
        {
            DWORD rtag = 0;
            SymGetTypeInfo(dbg->sym_handle, v.mod_base, resolved, TI_GET_SYMTAG, &rtag);
            if (rtag == SYM_TAG_BASETYPE) break;
            DWORD inner = 0;
            if (!SymGetTypeInfo(dbg->sym_handle, v.mod_base, resolved, TI_GET_TYPEID, &inner)
                || inner == 0 || inner == resolved) break;
            resolved = inner;
        }

        DWORD pt_tag = 0;
        ULONG64 pt_len = 0;
        DWORD pt_basetype = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, resolved, TI_GET_SYMTAG,   &pt_tag);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, resolved, TI_GET_LENGTH,   &pt_len);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, resolved, TI_GET_BASETYPE, &pt_basetype);

        /* Also check the original pt_type length in case chain did not resolve fully */
        ULONG64 orig_len = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, pt_type, TI_GET_LENGTH, &orig_len);

        /* char* if resolved BaseType with btChar/btWChar, or if pointed-to size is 1 */
        int is_char_ptr = ((pt_tag == SYM_TAG_BASETYPE &&
                            (pt_basetype == 3 || pt_basetype == 5)) ||
                           (pt_len == 1) ||
                           (orig_len == 1));

        if (is_char_ptr && fmt != FMT_HEX && fmt != FMT_DEC &&
            fmt != FMT_OCT && fmt != FMT_BIN)
        {
            char strbuf[256] = {0};
            SIZE_T nr = 0;
            if (v.value)
                ReadProcessMemory(dbg->process, (void*)(DWORD64)v.value,
                                  strbuf, sizeof(strbuf)-1, &nr);
            strbuf[sizeof(strbuf)-1] = '\0';
            if (top_tname[0])
                printf("%s = (%s) 0x%llx \"%s\"\n", top_label, top_tname,
                       (unsigned long long)v.value, v.value ? strbuf : "");
            else
                printf("%s = 0x%llx \"%s\"\n", top_label,
                       (unsigned long long)v.value, v.value ? strbuf : "");
        }
        else if (fmt == FMT_DEFAULT || fmt == FMT_HEX)
        {
            if (top_tname[0])
                printf("%s = (%s) 0x%llx\n", top_label, top_tname, (unsigned long long)v.value);
            else
                printf("%s = 0x%llx\n", top_label, (unsigned long long)v.value);
        }
        else
            print_int_fmt(top_label, v.value, v.byte_size, fmt, 1, top_tname);
        return;
    }

    /* Scalar: check if char type and append character */
    {
        DWORD sc_tag = 0, sc_basetype = 0;
        ULONG64 sc_len = 0;
        DWORD sc_id = v.type_id;
        for (int di = 0; di < 8 && v.mod_base && sc_id; di++)
        {
            SymGetTypeInfo(dbg->sym_handle, v.mod_base, sc_id, TI_GET_SYMTAG, &sc_tag);
            if (sc_tag == SYM_TAG_BASETYPE) break;
            DWORD inner = 0;
            if (!SymGetTypeInfo(dbg->sym_handle, v.mod_base, sc_id, TI_GET_TYPEID, &inner)
                || inner == 0 || inner == sc_id) break;
            sc_id = inner;
        }
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, sc_id, TI_GET_BASETYPE, &sc_basetype);
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, sc_id, TI_GET_LENGTH,   &sc_len);
        int is_char = (sc_tag == SYM_TAG_BASETYPE &&
                       (sc_basetype == 3 ||
                        (sc_len == 1 && (sc_basetype == 6 || sc_basetype == 7))));
        if (!is_char && v.byte_size == 1) is_char = 1;

        if (is_char && fmt == FMT_DEFAULT)
        {
            printf("%s = ", top_label);
            print_char_value((int)(v.value & 0xff));
            printf(" (0x%x)\n", (unsigned int)(v.value & 0xff));
        }
        else if (v.is_float && fmt == FMT_DEFAULT)
        {
            if (top_tname[0])
                printf("%s = (%s) %f\n", top_label, top_tname, v.fvalue);
            else
                printf("%s = %f\n", top_label, v.fvalue);
        }
        else
            print_int_fmt(top_label, v.value, v.byte_size, fmt, 1, top_tname);
    }
}

void expr_print(debugger_t *dbg, const char *expr_str)
{
    expr_print_fmt(dbg, expr_str, FMT_DEFAULT);
}

int expr_assign(debugger_t *dbg, const char *lhs, expr_val_t *val)
{
    expr_val_t v = {0};
    if (expr_eval(dbg, lhs, &v) != EVAL_OK)
    {
        printf("error: %s\n", v.errmsg);
        return -1;
    }
    if (!v.is_lvalue)
    {
        printf("error: '%s' is not assignable\n", lhs);
        return -1;
    }

    ULONG sz = v.byte_size ? v.byte_size : 4;
    if (sz > 8) sz = 8;

    /* Determine if target is a floating-point type (btFloat == 8) */
    int target_is_fp = 0;
    if (v.mod_base && v.type_id)
    {
        DWORD tag = 0, bt = 0;
        SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id,
                       TI_GET_SYMTAG, &tag);
        if (tag == SYM_TAG_BASETYPE)
        {
            SymGetTypeInfo(dbg->sym_handle, v.mod_base, v.type_id,
                           TI_GET_BASETYPE, &bt);
            if (bt == 8) target_is_fp = 1;
        }
    }

    SIZE_T n;
    if (target_is_fp)
    {
        double src = val->is_float ? val->fvalue : (double)val->value;
        if (sz == 4)
        {
            float f = (float)src;
            if (!WriteProcessMemory(dbg->process, (void*)v.addr, &f, sizeof(f), &n))
            {
                printf("WriteProcessMemory failed for '%s'\n", lhs);
                return -1;
            }
            printf("%s = %f\n", lhs, f);
        }
        else
        {
            if (!WriteProcessMemory(dbg->process, (void*)v.addr, &src, sizeof(src), &n))
            {
                printf("WriteProcessMemory failed for '%s'\n", lhs);
                return -1;
            }
            printf("%s = %f\n", lhs, src);
        }
    }
    else
    {
        unsigned long long write_val =
            (sz <= 4) ? (unsigned long long)(long)val->value
                      : (unsigned long long)val->value;
        if (!WriteProcessMemory(dbg->process, (void*)v.addr, &write_val, sz, &n))
        {
            printf("WriteProcessMemory failed for '%s'\n", lhs);
            return -1;
        }
        printf("%s = %lld\n", lhs, val->value);
    }
    return 0;
}
