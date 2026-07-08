#ifndef LEAKCHECK_H
#define LEAKCHECK_H

#include "debugger.h"

int leak_tracking_enable(debugger_t *dbg);
void leak_tracking_disable(debugger_t *dbg);

void leak_on_malloc_enter(debugger_t *dbg);
void leak_on_calloc_enter(debugger_t *dbg);
void leak_on_realloc_enter(debugger_t *dbg);
void leak_on_malloc_return(debugger_t *dbg);
void leak_on_free_enter(debugger_t *dbg);

int leak_rearm_pending(debugger_t *dbg);
void leak_finish_rearm(debugger_t *dbg);

void print_leaks(debugger_t *dbg);

#endif
