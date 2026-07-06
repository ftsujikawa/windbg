#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "debugger.h"

int init_symbols(debugger_t *dbg);
void cleanup_symbols(debugger_t *dbg);

void* lookup_symbol(
    debugger_t *dbg,
    const char *name
);

int show_source_line(
    debugger_t *dbg,
    DWORD64 addr
);

void print_disassembly(
    debugger_t *dbg,
    DWORD64 addr,
    int count
);

void show_source_lines(
    debugger_t *dbg,
    const char *filename,
    DWORD from,
    DWORD count
);

DWORD get_source_line_number(
    debugger_t *dbg,
    DWORD64 addr
);

void* lookup_source_line_addr(
    debugger_t *dbg,
    const char *filename,
    DWORD line_number
);

void* skip_function_prologue(
    debugger_t *dbg,
    void *addr
);

void print_backtrace(
    debugger_t *dbg
);

void print_variable(
    debugger_t *dbg,
    const char *name
);

int set_variable(
    debugger_t *dbg,
    const char *name,
    long long value
);

void print_symbol_info(
    debugger_t *dbg,
    const char *name
);

void print_line_info(
    debugger_t *dbg,
    const char *filename_filter
);

void show_variables(
    debugger_t *dbg,
    const char *what
);

#endif