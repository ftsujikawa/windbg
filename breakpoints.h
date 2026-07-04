#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H

#include "debugger.h"

breakpoint_t *find_breakpoint(debugger_t *dbg, void *addr);

int set_breakpoint(debugger_t *dbg, void *addr);
int remove_breakpoint(debugger_t *dbg);
int remove_breakpoint_at(debugger_t *dbg, void *addr);

int set_temp_breakpoint(debugger_t *dbg, void *addr);
int remove_temp_breakpoint(debugger_t *dbg);

#endif