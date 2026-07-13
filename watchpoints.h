#ifndef WATCHPOINTS_H
#define WATCHPOINTS_H

#include "debugger.h"

/* Set a hardware watchpoint on addr.
 * type : WATCH_WRITE (1) or WATCH_RW (2)
 * size : 1, 2, 4, or 8
 * Returns 0 on success, -1 on error. */
int  set_watchpoint(debugger_t *dbg, void *addr, int type, int size);

/* Remove the watchpoint at addr.
 * Returns 0 on success, -1 if not found. */
int  remove_watchpoint(debugger_t *dbg, void *addr);

/* Remove all watchpoints. */
void remove_all_watchpoints(debugger_t *dbg);

/* Return the watchpoint slot that fired (checked from DR6), or -1. */
int  watchpoint_hit_slot(debugger_t *dbg);

/* Display watchpoint at addr that fired. */
void watchpoint_report(debugger_t *dbg, int slot, DWORD64 rip);

/* Print the current watchpoint list. */
void print_watchpoints(debugger_t *dbg);

#endif
