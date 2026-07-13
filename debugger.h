#ifndef DEBUGGER_H
#define DEBUGGER_H

/* Disable MSVC deprecation warnings for standard C functions */
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <dbghelp.h>

typedef struct breakpoint
{
    void *addr;
    BYTE original_byte;
    struct breakpoint *next;
} breakpoint_t;

/* Watch type: 1=write, 2=read/write */
#define WATCH_WRITE  1
#define WATCH_RW     2

/* Hardware watchpoint (uses DR0-DR3 / DR7) */
typedef struct watchpoint
{
    void    *addr;
    int      type;   /* WATCH_WRITE or WATCH_RW */
    int      size;   /* 1, 2, 4, or 8 bytes */
    int      slot;   /* DR index 0-3 */
} watchpoint_t;

typedef struct alloc_info
{
    void *addr;
    size_t size;
    DWORD64 caller;
    char alloc_func[32];  /* "malloc", "calloc", "realloc", etc. */
    struct alloc_info *next;
} alloc_info_t;

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

    void *bp_rearm_addr;        /* a permanent breakpoint's byte was restored for one
                                   real instruction (via single-step) and must be
                                   rewritten back to 0xCC once that single-step
                                   completes, so it can fire again on a later call */
    BYTE bp_rearm_byte;

    int si_requested;          /* the user's `si` set the trap flag itself (as
                                   opposed to next_mode/step_mode/a rearm doing
                                   so internally); if a breakpoint rearm is also
                                   pending on this same single-step, the step
                                   must still be shown instead of swallowed */

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

    int print_pretty;

    /* malloc/free leak tracking (leakcheck.c) */
    int leak_tracking;
    void *malloc_addr;
    BYTE malloc_orig_byte;
    void *calloc_addr;
    BYTE calloc_orig_byte;
    void *realloc_addr;
    BYTE realloc_orig_byte;
    void *free_addr;
    BYTE free_orig_byte;
    void *malloc_ret_addr;      /* one-shot temp bp: return address of the in-flight malloc call */
    BYTE malloc_ret_byte;
    size_t pending_alloc_size;
    char current_alloc_func[32]; /* "malloc", "calloc", "realloc" */
    void *leak_rearm_addr;      /* malloc_addr/free_addr byte was restored for one real
                                   instruction (via single-step) and must be rewritten
                                   back to 0xCC once that single-step completes */
    BYTE leak_rearm_byte;
    alloc_info_t *allocations;

    /* hardware watchpoints (up to 4: DR0-DR3) */
    watchpoint_t watchpoints[4];
    int          watch_count;

} debugger_t;
int debugger_start(debugger_t *dbg, const char *program);
void debugger_loop(debugger_t *dbg);
void debugger_restart(debugger_t *dbg);

#endif