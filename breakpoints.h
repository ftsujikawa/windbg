#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H

#include "debugger.h"

breakpoint_t *find_breakpoint(debugger_t *dbg, void *addr);

/* Looks up a breakpoint by its 1-based display index, i.e. the same
 * numbering `print_breakpoints` (the `show bp` command) prints as "#N".
 * Returns NULL if index is out of range. */
breakpoint_t *find_breakpoint_by_index(debugger_t *dbg, int index);

int set_breakpoint(debugger_t *dbg, void *addr);
int remove_breakpoint(debugger_t *dbg);
int remove_breakpoint_at(debugger_t *dbg, void *addr);

/* Temporarily restores the original byte at addr WITHOUT touching the
 * dbg->breakpoints list (unlike remove_breakpoint_at, which is a permanent
 * removal used by the `del` command). Used when a breakpoint hits and is
 * about to be re-armed via arm_breakpoint_rearm(), so the breakpoint_t node
 * -- and thus find_breakpoint()'s ability to recognize the next hit -- stays
 * intact. */
void restore_breakpoint_byte(debugger_t *dbg, void *addr, BYTE orig_byte);

int set_temp_breakpoint(debugger_t *dbg, void *addr);
int remove_temp_breakpoint(debugger_t *dbg);

void print_breakpoints(debugger_t *dbg);

void arm_breakpoint_rearm(debugger_t *dbg, void *addr, BYTE orig_byte);
int breakpoint_rearm_pending(debugger_t *dbg);
void finish_breakpoint_rearm(debugger_t *dbg);

#endif