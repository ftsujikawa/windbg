win_bison -d -o expr_parser.tab.c expr_parser.y
win_flex -o expr_lexer.yy.c expr_lexer.l
win_bison -d -o cmdline_parser.tab.c cmdline_parser.y
win_flex -o cmdline_lexer.yy.c cmdline_lexer.l
cl /utf-8 /Fe:tdb.exe breakpoints.c commands.c debugger.c main.c registers.c symbols.c memory.c disasm.c expr.c leakcheck.c watchpoints.c threads.c processes.c expr_parser.tab.c expr_lexer.yy.c cmdline_parser.tab.c cmdline_lexer.yy.c dbghelp.lib
cl /Zi testprog.c
cl /Zi testprog2.c
