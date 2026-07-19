#ifndef WATCHPOINTS_H
#define WATCHPOINTS_H

#include "debugger.h"

/* (Re-)apply dbg->watchpoints' DR0-DR3/DR7 encoding to every registered
 * thread. DR registers are per-thread CPU state, so this must run on each
 * one for a watchpoint to fire regardless of which thread touches the
 * watched memory; also called when a new thread is created so it inherits
 * the current watchpoint set. */
void apply_watchpoints(debugger_t *dbg);

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
