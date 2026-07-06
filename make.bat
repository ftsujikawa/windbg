cl /utf-8 /Fe:windbg.exe breakpoints.c commands.c debugger.c main.c registers.c symbols.c memory.c disasm.c expr.c leakcheck.c dbghelp.lib
cl /Zi testprog.c
cl /Zi testprog2.c
