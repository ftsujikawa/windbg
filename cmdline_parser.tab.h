/* A Bison parser, made by GNU Bison 3.4.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_CPARSE_CMDLINE_PARSER_TAB_H_INCLUDED
# define YY_CPARSE_CMDLINE_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef CPARSEDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define CPARSEDEBUG 1
#  else
#   define CPARSEDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define CPARSEDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined CPARSEDEBUG */
#if CPARSEDEBUG
extern int cparsedebug;
#endif
/* "%code requires" blocks.  */
#line 9 "cmdline_parser.y"

    #include "debugger.h"
    #include "cmdline_internal.h"

    typedef struct { char s[512]; } ctext_t;

#line 63 "cmdline_parser.tab.h"

/* Token type.  */
#ifndef CPARSETOKENTYPE
# define CPARSETOKENTYPE
  enum cparsetokentype
  {
    ARGWORD = 258,
    STRING = 259,
    KW_BREAK = 260,
    KW_DEL = 261,
    KW_WDEL = 262,
    KW_SYMS = 263,
    KW_REGS = 264,
    KW_SI = 265,
    KW_CONTINUE = 266,
    KW_STEP = 267,
    KW_N = 268,
    KW_UP = 269,
    KW_TB = 270,
    KW_RUN = 271,
    KW_KILL = 272,
    KW_HELP = 273,
    KW_QUIT = 274,
    KW_X = 275,
    KW_DIS = 276,
    KW_LIST = 277,
    KW_LINES = 278,
    KW_LEAK = 279,
    KW_SHOW = 280,
    KW_WATCH = 281,
    KW_PRINT = 282,
    KW_SET = 283,
    KW_SETPRINTPRETTY = 284,
    KW_THREAD = 285,
    KW_PROCESS = 286
  };
#endif

/* Value type.  */
#if ! defined CPARSESTYPE && ! defined CPARSESTYPE_IS_DECLARED
union CPARSESTYPE
{
#line 31 "cmdline_parser.y"

    ctext_t text;

#line 110 "cmdline_parser.tab.h"

};
typedef union CPARSESTYPE CPARSESTYPE;
# define CPARSESTYPE_IS_TRIVIAL 1
# define CPARSESTYPE_IS_DECLARED 1
#endif


extern CPARSESTYPE cparselval;

int cparseparse (debugger_t *dbg, cmd_action_t *action);

#endif /* !YY_CPARSE_CMDLINE_PARSER_TAB_H_INCLUDED  */
