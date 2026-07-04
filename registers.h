#ifndef REGISTERS_H
#define REGISTERS_H

#include "debugger.h"

void show_registers(debugger_t *dbg);
void single_step(debugger_t *dbg);

#endif