#ifndef DISASM_H
#define DISASM_H

#include "debugger.h"

void disassemble_at(
    debugger_t *dbg,
    DWORD64 addr,
    int count
);

#endif
