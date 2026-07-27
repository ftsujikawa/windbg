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

#ifndef YY_EPARSE_EXPR_PARSER_TAB_H_INCLUDED
# define YY_EPARSE_EXPR_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef EPARSEDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define EPARSEDEBUG 1
#  else
#   define EPARSEDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define EPARSEDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined EPARSEDEBUG */
#if EPARSEDEBUG
extern int eparsedebug;
#endif
/* "%code requires" blocks.  */
#line 9 "expr_parser.y"

    #include "expr.h"
    #include "debugger.h"

    /* Wrapper structs so union members stay assignable via plain '='. */
    typedef struct { char s[128]; } etext_t;
    typedef struct { char first[32]; char full[96]; } etypewords_t;
    typedef struct { int size; int is_ptr; } etypeinfo_t;

#line 67 "expr_parser.tab.h"

/* Token kinds.  */
#ifndef EPARSETOKENTYPE
# define EPARSETOKENTYPE
  enum eparsetokentype
  {
    EPARSEEMPTY = -2,
    EPARSEEOF = 0,                 /* "end of file"  */
    EPARSEerror = 256,             /* error  */
    EPARSEUNDEF = 257,             /* "invalid token"  */
    NUM = 258,                     /* NUM  */
    IDENT = 259,                   /* IDENT  */
    TYPEWORD = 260,                /* TYPEWORD  */
    SIZEOF = 261,                  /* SIZEOF  */
    ARROW = 262,                   /* ARROW  */
    LSHIFT = 263,                  /* LSHIFT  */
    RSHIFT = 264,                  /* RSHIFT  */
    ANDAND = 265,                  /* ANDAND  */
    OROR = 266,                    /* OROR  */
    EQ = 267,                      /* EQ  */
    NEQ = 268,                     /* NEQ  */
    LE = 269,                      /* LE  */
    GE = 270                       /* GE  */
  };
  typedef enum eparsetokentype eparsetoken_kind_t;
#endif

/* Value type.  */
#if ! defined EPARSESTYPE && ! defined EPARSESTYPE_IS_DECLARED
union EPARSESTYPE
{
#line 222 "expr_parser.y"

    expr_val_t   val;
    etext_t      text;
    etypewords_t words;
    etypeinfo_t  ty;

#line 106 "expr_parser.tab.h"

};
typedef union EPARSESTYPE EPARSESTYPE;
# define EPARSESTYPE_IS_TRIVIAL 1
# define EPARSESTYPE_IS_DECLARED 1
#endif


extern EPARSESTYPE eparselval;


int eparseparse (debugger_t *dbg);


#endif /* !YY_EPARSE_EXPR_PARSER_TAB_H_INCLUDED  */
