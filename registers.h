#ifndef REGISTERS_H
#define REGISTERS_H

#include "debugger.h"

void show_registers(debugger_t *dbg);
void single_step(debugger_t *dbg);
int  set_register(debugger_t *dbg, const char *name, long long value, double fvalue, int is_float);

#endif