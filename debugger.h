#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <windows.h>
#include <dbghelp.h>

typedef struct breakpoint
{
    void *addr;
    BYTE original_byte;
    struct breakpoint *next;
} breakpoint_t;

typedef struct
{
    HANDLE process;
    HANDLE thread;

    HANDLE sym_handle;
    
    DWORD pid;
    DWORD tid;

    breakpoint_t *breakpoints;

    void *breakpoint_addr;
    BYTE original_byte;

    void *temp_bp_addr;
    BYTE temp_bp_byte;

    int running;

    int next_mode;
    DWORD next_line;
    DWORD64 next_rsp;

    int step_mode;
    DWORD step_line;

    int symbols_initialized;

    char list_file[512];
    DWORD list_next_line;

    char target_program[512];

} debugger_t;
int debugger_start(debugger_t *dbg, const char *program);
void debugger_loop(debugger_t *dbg);
void debugger_restart(debugger_t *dbg);

#endif