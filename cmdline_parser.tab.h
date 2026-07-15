/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

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

#line 64 "cmdline_parser.tab.h"

/* Token kinds.  */
#ifndef CPARSETOKENTYPE
# define CPARSETOKENTYPE
  enum cparsetokentype
  {
    CPARSEEMPTY = -2,
    CPARSEEOF = 0,                 /* "end of file"  */
    CPARSEerror = 256,             /* error  */
    CPARSEUNDEF = 257,             /* "invalid token"  */
    ARGWORD = 258,                 /* ARGWORD  */
    STRING = 259,                  /* STRING  */
    KW_BREAK = 260,                /* KW_BREAK  */
    KW_DEL = 261,                  /* KW_DEL  */
    KW_WDEL = 262,                 /* KW_WDEL  */
    KW_SYMS = 263,                 /* KW_SYMS  */
    KW_REGS = 264,                 /* KW_REGS  */
    KW_SI = 265,                   /* KW_SI  */
    KW_CONTINUE = 266,             /* KW_CONTINUE  */
    KW_STEP = 267,                 /* KW_STEP  */
    KW_N = 268,                    /* KW_N  */
    KW_UP = 269,                   /* KW_UP  */
    KW_TB = 270,                   /* KW_TB  */
    KW_RUN = 271,                  /* KW_RUN  */
    KW_KILL = 272,                 /* KW_KILL  */
    KW_HELP = 273,                 /* KW_HELP  */
    KW_QUIT = 274,                 /* KW_QUIT  */
    KW_X = 275,                    /* KW_X  */
    KW_DIS = 276,                  /* KW_DIS  */
    KW_LIST = 277,                 /* KW_LIST  */
    KW_LINES = 278,                /* KW_LINES  */
    KW_LEAK = 279,                 /* KW_LEAK  */
    KW_SHOW = 280,                 /* KW_SHOW  */
    KW_WATCH = 281,                /* KW_WATCH  */
    KW_PRINT = 282,                /* KW_PRINT  */
    KW_SET = 283,                  /* KW_SET  */
    KW_SETPRINTPRETTY = 284        /* KW_SETPRINTPRETTY  */
  };
  typedef enum cparsetokentype cparsetoken_kind_t;
#endif

/* Value type.  */
#if ! defined CPARSESTYPE && ! defined CPARSESTYPE_IS_DECLARED
union CPARSESTYPE
{
#line 31 "cmdline_parser.y"

    ctext_t text;

#line 114 "cmdline_parser.tab.h"

};
typedef union CPARSESTYPE CPARSESTYPE;
# define CPARSESTYPE_IS_TRIVIAL 1
# define CPARSESTYPE_IS_DECLARED 1
#endif


extern CPARSESTYPE cparselval;


int cparseparse (debugger_t *dbg, cmd_action_t *action);


#endif /* !YY_CPARSE_CMDLINE_PARSER_TAB_H_INCLUDED  */
