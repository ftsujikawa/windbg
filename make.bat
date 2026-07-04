cl /Fe:windbg.exe breakpoints.c commands.c debugger.c main.c registers.c symbols.c memory.c disasm.c dbghelp.lib
cl /Zi testprog.c
