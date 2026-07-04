#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H

#include "debugger.h"

int set_breakpoint(debugger_t *dbg, void *addr);
int remove_breakpoint(debugger_t *dbg);

int set_temp_breakpoint(debugger_t *dbg, void *addr);
int remove_temp_breakpoint(debugger_t *dbg);

#endif