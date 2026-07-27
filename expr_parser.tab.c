/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         EPARSESTYPE
/* Substitute the variable and function names.  */
#define yyparse         eparseparse
#define yylex           eparselex
#define yyerror         eparseerror
#define yydebug         eparsedebug
#define yynerrs         eparsenerrs
#define yylval          eparselval
#define yychar          eparsechar


# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "expr_parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_NUM = 3,                        /* NUM  */
  YYSYMBOL_IDENT = 4,                      /* IDENT  */
  YYSYMBOL_TYPEWORD = 5,                   /* TYPEWORD  */
  YYSYMBOL_SIZEOF = 6,                     /* SIZEOF  */
  YYSYMBOL_ARROW = 7,                      /* ARROW  */
  YYSYMBOL_LSHIFT = 8,                     /* LSHIFT  */
  YYSYMBOL_RSHIFT = 9,                     /* RSHIFT  */
  YYSYMBOL_ANDAND = 10,                    /* ANDAND  */
  YYSYMBOL_OROR = 11,                      /* OROR  */
  YYSYMBOL_EQ = 12,                        /* EQ  */
  YYSYMBOL_NEQ = 13,                       /* NEQ  */
  YYSYMBOL_LE = 14,                        /* LE  */
  YYSYMBOL_GE = 15,                        /* GE  */
  YYSYMBOL_16_ = 16,                       /* '|'  */
  YYSYMBOL_17_ = 17,                       /* '^'  */
  YYSYMBOL_18_ = 18,                       /* '&'  */
  YYSYMBOL_19_ = 19,                       /* '<'  */
  YYSYMBOL_20_ = 20,                       /* '>'  */
  YYSYMBOL_21_ = 21,                       /* '+'  */
  YYSYMBOL_22_ = 22,                       /* '-'  */
  YYSYMBOL_23_ = 23,                       /* '*'  */
  YYSYMBOL_24_ = 24,                       /* '/'  */
  YYSYMBOL_25_ = 25,                       /* '%'  */
  YYSYMBOL_26_ = 26,                       /* '~'  */
  YYSYMBOL_27_ = 27,                       /* '!'  */
  YYSYMBOL_28_ = 28,                       /* '('  */
  YYSYMBOL_29_ = 29,                       /* ')'  */
  YYSYMBOL_30_ = 30,                       /* '['  */
  YYSYMBOL_31_ = 31,                       /* ']'  */
  YYSYMBOL_32_ = 32,                       /* '.'  */
  YYSYMBOL_YYACCEPT = 33,                  /* $accept  */
  YYSYMBOL_input = 34,                     /* input  */
  YYSYMBOL_expr = 35,                      /* expr  */
  YYSYMBOL_logand_expr = 36,               /* logand_expr  */
  YYSYMBOL_bitor_expr = 37,                /* bitor_expr  */
  YYSYMBOL_bitxor_expr = 38,               /* bitxor_expr  */
  YYSYMBOL_bitand_expr = 39,               /* bitand_expr  */
  YYSYMBOL_eq_expr = 40,                   /* eq_expr  */
  YYSYMBOL_rel_expr = 41,                  /* rel_expr  */
  YYSYMBOL_shift_expr = 42,                /* shift_expr  */
  YYSYMBOL_add_expr = 43,                  /* add_expr  */
  YYSYMBOL_mul_expr = 44,                  /* mul_expr  */
  YYSYMBOL_unary_expr = 45,                /* unary_expr  */
  YYSYMBOL_postfix_expr = 46,              /* postfix_expr  */
  YYSYMBOL_primary_expr = 47,              /* primary_expr  */
  YYSYMBOL_typename_words = 48,            /* typename_words  */
  YYSYMBOL_typename = 49,                  /* typename  */
  YYSYMBOL_stars_opt = 50                  /* stars_opt  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;



/* Unqualified %code blocks.  */
#line 19 "expr_parser.y"

#include <stdio.h>
#include <string.h>
#include "expr_internal.h"

int expr_parse_err = 0;
char expr_parse_errmsg[256] = {0};

static expr_val_t g_expr_result;

void eparseerror(debugger_t *dbg, const char *s);
int  eparselex(void);

static expr_val_t propagate_err(expr_val_t *a, expr_val_t *b)
{
    if (a->errmsg[0]) return *a;
    if (b->errmsg[0]) return *b;
    expr_val_t z = {0};
    return z;
}

static expr_val_t do_ident(debugger_t *dbg, const char *name)
{
    DWORD64 addr = 0, modbase = 0;
    DWORD   tid  = 0;
    ULONG   sz   = 0;

    if (!expr_lookup_symbol(dbg, name, &addr, &modbase, &tid, &sz))
    {
        char msg[160];
        snprintf(msg, sizeof(msg), "symbol '%s' not found", name);
        return make_err(msg);
    }

    expr_val_t v = {0};
    v.mod_base = modbase;
    v.type_id  = tid;
    if (sz == 0)
    {
        /* register variable: addr holds the value */
        v.is_lvalue = 0;
        v.value     = (long long)addr;
        v.byte_size = 4;
    }
    else
    {
        v.is_lvalue = 1;
        v.addr      = addr;
        v.byte_size = sz;
        load_lvalue(dbg, &v);
    }
    return v;
}

static expr_val_t do_cast(debugger_t *dbg, etypeinfo_t *ty, expr_val_t *inner)
{
    (void)dbg;
    if (inner->errmsg[0]) return *inner;

    if (ty->is_ptr)
    {
        long long ptr_val = inner->is_lvalue ? (long long)inner->addr : inner->value;
        expr_val_t r = {0};
        r.value     = ptr_val;
        r.byte_size = 8;
        return r;
    }

    expr_val_t r = *inner;
    int sz = ty->size;
    if (sz == 1)      r.value = (long long)(signed char)r.value;
    else if (sz == 2) r.value = (long long)(short)r.value;
    else if (sz == 4) r.value = (long long)(int)r.value;
    r.byte_size = (ULONG)sz;
    r.is_lvalue = 0;
    r.type_id   = 0;
    return r;
}

static expr_val_t do_index(debugger_t *dbg, expr_val_t *base, expr_val_t *idx)
{
    if (base->errmsg[0]) return *base;
    if (idx->errmsg[0])  return *idx;

    DWORD tag = 0;
    SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_SYMTAG, &tag);

    DWORD elem_type = 0;
    DWORD64 base_addr = base->addr;

    if (tag == SYM_TAG_POINTER)
    {
        SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_TYPEID, &elem_type);
        base_addr = (DWORD64)base->value;
    }
    else if (tag == SYM_TAG_ARRAYTYPE)
    {
        SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_TYPEID, &elem_type);
    }
    else
    {
        return make_err("subscript requires array or pointer");
    }

    ULONG64 elem_len = 0;
    SymGetTypeInfo(dbg->sym_handle, base->mod_base, elem_type, TI_GET_LENGTH, &elem_len);
    if (elem_len == 0) elem_len = 4;

    expr_val_t r = {0};
    r.is_lvalue = 1;
    r.addr      = base_addr + (DWORD64)idx->value * elem_len;
    r.mod_base  = base->mod_base;
    r.type_id   = elem_type;
    r.byte_size = (ULONG)elem_len;
    load_lvalue(dbg, &r);
    return r;
}

static expr_val_t do_member(debugger_t *dbg, expr_val_t *base, const char *member, int is_arrow)
{
    if (base->errmsg[0]) return *base;

    DWORD64 base_addr = base->addr;
    DWORD   base_type = base->type_id;

    if (is_arrow)
    {
        DWORD tag = 0;
        SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_SYMTAG, &tag);
        if (tag == SYM_TAG_POINTER)
        {
            DWORD pt = 0;
            SymGetTypeInfo(dbg->sym_handle, base->mod_base, base->type_id, TI_GET_TYPEID, &pt);
            base_type = pt;
        }
        base_addr = (DWORD64)base->value;
    }

    DWORD mem_type = 0, mem_off = 0;
    if (!lookup_member(dbg, base->mod_base, base_type, member, &mem_type, &mem_off))
    {
        char msg[160];
        snprintf(msg, sizeof(msg), "member '%s' not found", member);
        return make_err(msg);
    }

    ULONG64 mem_len = 0;
    SymGetTypeInfo(dbg->sym_handle, base->mod_base, mem_type, TI_GET_LENGTH, &mem_len);
    if (mem_len == 0) mem_len = 4;

    expr_val_t r = {0};
    r.is_lvalue = 1;
    r.addr      = base_addr + mem_off;
    r.mod_base  = base->mod_base;
    r.type_id   = mem_type;
    r.byte_size = (ULONG)mem_len;
    load_lvalue(dbg, &r);
    return r;
}

static expr_val_t do_deref(debugger_t *dbg, expr_val_t *inner)
{
    if (inner->errmsg[0]) return *inner;

    DWORD tag = 0;
    SymGetTypeInfo(dbg->sym_handle, inner->mod_base, inner->type_id, TI_GET_SYMTAG, &tag);
    if (tag != SYM_TAG_POINTER)
        return make_err("dereference of non-pointer");

    DWORD pt = 0;
    SymGetTypeInfo(dbg->sym_handle, inner->mod_base, inner->type_id, TI_GET_TYPEID, &pt);

    ULONG64 len = 0;
    SymGetTypeInfo(dbg->sym_handle, inner->mod_base, pt, TI_GET_LENGTH, &len);
    if (len == 0) len = 4;

    expr_val_t r = {0};
    r.is_lvalue = 1;
    r.addr      = (DWORD64)inner->value;
    r.mod_base  = inner->mod_base;
    r.type_id   = pt;
    r.byte_size = (ULONG)len;
    load_lvalue(dbg, &r);
    return r;
}

static expr_val_t do_addrof(expr_val_t *inner)
{
    if (inner->errmsg[0]) return *inner;
    if (!inner->is_lvalue) return make_err("& requires lvalue");
    expr_val_t r = make_val((long long)inner->addr);
    r.byte_size  = 8;
    r.is_address = 1;
    r.mod_base   = inner->mod_base;
    r.type_id    = inner->type_id;
    return r;
}

#line 361 "expr_parser.tab.c"

#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined EPARSESTYPE_IS_TRIVIAL && EPARSESTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  36
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   88

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  33
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  18
/* YYNRULES -- Number of rules.  */
#define YYNRULES  53
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  91

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   270


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    27,     2,     2,     2,    25,    18,     2,
      28,    29,    23,    21,     2,    22,    32,    24,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      19,     2,    20,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    30,     2,    31,    17,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    16,     2,    26,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15
};

#if EPARSEDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   245,   245,   249,   250,   257,   258,   265,   266,   273,
     274,   281,   282,   289,   290,   294,   301,   302,   306,   310,
     314,   321,   322,   326,   333,   334,   338,   345,   346,   350,
     356,   365,   366,   367,   368,   369,   370,   371,   372,   376,
     377,   378,   379,   383,   384,   385,   386,   387,   391,   397,
     403,   412,   423,   424
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "NUM", "IDENT",
  "TYPEWORD", "SIZEOF", "ARROW", "LSHIFT", "RSHIFT", "ANDAND", "OROR",
  "EQ", "NEQ", "LE", "GE", "'|'", "'^'", "'&'", "'<'", "'>'", "'+'", "'-'",
  "'*'", "'/'", "'%'", "'~'", "'!'", "'('", "')'", "'['", "']'", "'.'",
  "$accept", "input", "expr", "logand_expr", "bitor_expr", "bitxor_expr",
  "bitand_expr", "eq_expr", "rel_expr", "shift_expr", "add_expr",
  "mul_expr", "unary_expr", "postfix_expr", "primary_expr",
  "typename_words", "typename", "stars_opt", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-28)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      34,   -28,   -28,   -15,    34,    34,    34,    34,    34,    34,
       5,    15,    10,    41,    13,     8,    29,    23,    49,    37,
      44,    19,   -28,     9,   -28,     5,   -28,   -28,   -28,   -28,
     -28,   -28,    53,    -5,   -28,    38,   -28,    34,    34,    34,
      34,    34,    34,    34,    34,    34,    34,    34,    34,    34,
      34,    34,    34,    34,    34,    50,    34,    70,     1,    48,
      73,   -28,    56,    34,    41,    13,     8,    29,    23,    49,
      49,    37,    37,    37,    37,    44,    44,    19,    19,   -28,
     -28,   -28,   -28,     3,   -28,   -28,   -28,   -28,   -28,   -28,
     -28
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    43,    44,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     2,     3,     5,     7,     9,    11,    13,    16,
      21,    24,    27,    31,    39,     0,    33,    35,    34,    32,
      36,    37,    48,     0,    52,     0,     1,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    47,    51,     0,     4,     6,     8,    10,    12,    14,
      15,    19,    20,    17,    18,    22,    23,    25,    26,    28,
      29,    30,    42,     0,    41,    46,    45,    50,    53,    38,
      40
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -28,   -28,    -3,    43,    45,    42,    46,    47,    28,   -27,
      24,    25,    -4,   -28,   -28,   -28,    57,   -28
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    34,    35,    62
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      26,    27,    28,    29,    30,    31,    37,    33,     1,     2,
      32,     3,    37,    25,    37,    36,    55,    71,    72,    73,
      74,    37,    58,     4,    61,    40,     5,     6,     7,    39,
      85,     8,     9,    10,    90,    42,    43,     1,     2,    56,
       3,    57,    52,    53,    54,    48,    49,    41,    79,    80,
      81,    38,     4,    83,    82,     5,     6,     7,    60,    89,
       8,     9,    10,    44,    45,    50,    51,    63,    46,    47,
      69,    70,    75,    76,    84,    77,    78,    86,    87,    88,
      64,    66,    59,    65,     0,     0,    67,     0,    68
};

static const yytype_int8 yycheck[] =
{
       4,     5,     6,     7,     8,     9,    11,    10,     3,     4,
       5,     6,    11,    28,    11,     0,     7,    44,    45,    46,
      47,    11,    25,    18,    29,    17,    21,    22,    23,    16,
      29,    26,    27,    28,    31,    12,    13,     3,     4,    30,
       6,    32,    23,    24,    25,     8,     9,    18,    52,    53,
      54,    10,    18,    56,     4,    21,    22,    23,     5,    63,
      26,    27,    28,    14,    15,    21,    22,    29,    19,    20,
      42,    43,    48,    49,     4,    50,    51,    29,     5,    23,
      37,    39,    25,    38,    -1,    -1,    40,    -1,    41
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     6,    18,    21,    22,    23,    26,    27,
      28,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    28,    45,    45,    45,    45,
      45,    45,     5,    35,    48,    49,     0,    11,    10,    16,
      17,    18,    12,    13,    14,    15,    19,    20,     8,     9,
      21,    22,    23,    24,    25,     7,    30,    32,    35,    49,
       5,    29,    50,    29,    36,    37,    38,    39,    40,    41,
      41,    42,    42,    42,    42,    43,    43,    44,    44,    45,
      45,    45,     4,    35,     4,    29,    29,     5,    23,    45,
      31
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    33,    34,    35,    35,    36,    36,    37,    37,    38,
      38,    39,    39,    40,    40,    40,    41,    41,    41,    41,
      41,    42,    42,    42,    43,    43,    43,    44,    44,    44,
      44,    45,    45,    45,    45,    45,    45,    45,    45,    46,
      46,    46,    46,    47,    47,    47,    47,    47,    48,    48,
      48,    49,    50,    50
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     3,     1,     3,     3,     3,
       3,     1,     3,     3,     1,     3,     3,     1,     3,     3,
       3,     1,     2,     2,     2,     2,     2,     2,     4,     1,
       4,     3,     3,     1,     1,     4,     4,     3,     1,     2,
       3,     2,     0,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = EPARSEEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == EPARSEEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (dbg, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use EPARSEerror or EPARSEUNDEF. */
#define YYERRCODE EPARSEUNDEF


/* Enable debugging if requested.  */
#if EPARSEDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, dbg); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, debugger_t *dbg)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (dbg);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, debugger_t *dbg)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, dbg);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, debugger_t *dbg)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], dbg);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, dbg); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !EPARSEDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !EPARSEDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, debugger_t *dbg)
{
  YY_USE (yyvaluep);
  YY_USE (dbg);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (debugger_t *dbg)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = EPARSEEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == EPARSEEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= EPARSEEOF)
    {
      yychar = EPARSEEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == EPARSEerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = EPARSEUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = EPARSEEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* input: expr  */
#line 245 "expr_parser.y"
           { g_expr_result = (yyvsp[0].val); (yyval.val) = (yyvsp[0].val); }
#line 1641 "expr_parser.tab.c"
    break;

  case 3: /* expr: logand_expr  */
#line 249 "expr_parser.y"
                  { (yyval.val) = (yyvsp[0].val); }
#line 1647 "expr_parser.tab.c"
    break;

  case 4: /* expr: expr OROR logand_expr  */
#line 250 "expr_parser.y"
                            {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value || (yyvsp[0].val).value);
      }
#line 1656 "expr_parser.tab.c"
    break;

  case 5: /* logand_expr: bitor_expr  */
#line 257 "expr_parser.y"
                 { (yyval.val) = (yyvsp[0].val); }
#line 1662 "expr_parser.tab.c"
    break;

  case 6: /* logand_expr: logand_expr ANDAND bitor_expr  */
#line 258 "expr_parser.y"
                                    {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value && (yyvsp[0].val).value);
      }
#line 1671 "expr_parser.tab.c"
    break;

  case 7: /* bitor_expr: bitxor_expr  */
#line 265 "expr_parser.y"
                  { (yyval.val) = (yyvsp[0].val); }
#line 1677 "expr_parser.tab.c"
    break;

  case 8: /* bitor_expr: bitor_expr '|' bitxor_expr  */
#line 266 "expr_parser.y"
                                 {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value | (yyvsp[0].val).value);
      }
#line 1686 "expr_parser.tab.c"
    break;

  case 9: /* bitxor_expr: bitand_expr  */
#line 273 "expr_parser.y"
                  { (yyval.val) = (yyvsp[0].val); }
#line 1692 "expr_parser.tab.c"
    break;

  case 10: /* bitxor_expr: bitxor_expr '^' bitand_expr  */
#line 274 "expr_parser.y"
                                  {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value ^ (yyvsp[0].val).value);
      }
#line 1701 "expr_parser.tab.c"
    break;

  case 11: /* bitand_expr: eq_expr  */
#line 281 "expr_parser.y"
              { (yyval.val) = (yyvsp[0].val); }
#line 1707 "expr_parser.tab.c"
    break;

  case 12: /* bitand_expr: bitand_expr '&' eq_expr  */
#line 282 "expr_parser.y"
                              {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value & (yyvsp[0].val).value);
      }
#line 1716 "expr_parser.tab.c"
    break;

  case 13: /* eq_expr: rel_expr  */
#line 289 "expr_parser.y"
               { (yyval.val) = (yyvsp[0].val); }
#line 1722 "expr_parser.tab.c"
    break;

  case 14: /* eq_expr: eq_expr EQ rel_expr  */
#line 290 "expr_parser.y"
                          {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value == (yyvsp[0].val).value);
      }
#line 1731 "expr_parser.tab.c"
    break;

  case 15: /* eq_expr: eq_expr NEQ rel_expr  */
#line 294 "expr_parser.y"
                           {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value != (yyvsp[0].val).value);
      }
#line 1740 "expr_parser.tab.c"
    break;

  case 16: /* rel_expr: shift_expr  */
#line 301 "expr_parser.y"
                 { (yyval.val) = (yyvsp[0].val); }
#line 1746 "expr_parser.tab.c"
    break;

  case 17: /* rel_expr: rel_expr '<' shift_expr  */
#line 302 "expr_parser.y"
                              {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value < (yyvsp[0].val).value);
      }
#line 1755 "expr_parser.tab.c"
    break;

  case 18: /* rel_expr: rel_expr '>' shift_expr  */
#line 306 "expr_parser.y"
                              {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value > (yyvsp[0].val).value);
      }
#line 1764 "expr_parser.tab.c"
    break;

  case 19: /* rel_expr: rel_expr LE shift_expr  */
#line 310 "expr_parser.y"
                             {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value <= (yyvsp[0].val).value);
      }
#line 1773 "expr_parser.tab.c"
    break;

  case 20: /* rel_expr: rel_expr GE shift_expr  */
#line 314 "expr_parser.y"
                             {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value >= (yyvsp[0].val).value);
      }
#line 1782 "expr_parser.tab.c"
    break;

  case 21: /* shift_expr: add_expr  */
#line 321 "expr_parser.y"
               { (yyval.val) = (yyvsp[0].val); }
#line 1788 "expr_parser.tab.c"
    break;

  case 22: /* shift_expr: shift_expr LSHIFT add_expr  */
#line 322 "expr_parser.y"
                                 {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value << ((yyvsp[0].val).value & 63));
      }
#line 1797 "expr_parser.tab.c"
    break;

  case 23: /* shift_expr: shift_expr RSHIFT add_expr  */
#line 326 "expr_parser.y"
                                 {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((long long)((unsigned long long)(yyvsp[-2].val).value >> ((yyvsp[0].val).value & 63)));
      }
#line 1806 "expr_parser.tab.c"
    break;

  case 24: /* add_expr: mul_expr  */
#line 333 "expr_parser.y"
               { (yyval.val) = (yyvsp[0].val); }
#line 1812 "expr_parser.tab.c"
    break;

  case 25: /* add_expr: add_expr '+' mul_expr  */
#line 334 "expr_parser.y"
                            {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value + (yyvsp[0].val).value);
      }
#line 1821 "expr_parser.tab.c"
    break;

  case 26: /* add_expr: add_expr '-' mul_expr  */
#line 338 "expr_parser.y"
                            {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value - (yyvsp[0].val).value);
      }
#line 1830 "expr_parser.tab.c"
    break;

  case 27: /* mul_expr: unary_expr  */
#line 345 "expr_parser.y"
                 { (yyval.val) = (yyvsp[0].val); }
#line 1836 "expr_parser.tab.c"
    break;

  case 28: /* mul_expr: mul_expr '*' unary_expr  */
#line 346 "expr_parser.y"
                              {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          (yyval.val) = e.errmsg[0] ? e : make_val((yyvsp[-2].val).value * (yyvsp[0].val).value);
      }
#line 1845 "expr_parser.tab.c"
    break;

  case 29: /* mul_expr: mul_expr '/' unary_expr  */
#line 350 "expr_parser.y"
                              {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          if (e.errmsg[0]) (yyval.val) = e;
          else if ((yyvsp[0].val).value == 0) (yyval.val) = make_err("division by zero");
          else (yyval.val) = make_val((yyvsp[-2].val).value / (yyvsp[0].val).value);
      }
#line 1856 "expr_parser.tab.c"
    break;

  case 30: /* mul_expr: mul_expr '%' unary_expr  */
#line 356 "expr_parser.y"
                              {
          expr_val_t e = propagate_err(&(yyvsp[-2].val), &(yyvsp[0].val));
          if (e.errmsg[0]) (yyval.val) = e;
          else if ((yyvsp[0].val).value == 0) (yyval.val) = make_err("division by zero");
          else (yyval.val) = make_val((yyvsp[-2].val).value % (yyvsp[0].val).value);
      }
#line 1867 "expr_parser.tab.c"
    break;

  case 31: /* unary_expr: postfix_expr  */
#line 365 "expr_parser.y"
                               { (yyval.val) = (yyvsp[0].val); }
#line 1873 "expr_parser.tab.c"
    break;

  case 32: /* unary_expr: '*' unary_expr  */
#line 366 "expr_parser.y"
                               { (yyval.val) = do_deref(dbg, &(yyvsp[0].val)); }
#line 1879 "expr_parser.tab.c"
    break;

  case 33: /* unary_expr: '&' unary_expr  */
#line 367 "expr_parser.y"
                               { (yyval.val) = do_addrof(&(yyvsp[0].val)); }
#line 1885 "expr_parser.tab.c"
    break;

  case 34: /* unary_expr: '-' unary_expr  */
#line 368 "expr_parser.y"
                               { (yyval.val) = (yyvsp[0].val).errmsg[0] ? (yyvsp[0].val) : make_val(-(yyvsp[0].val).value); }
#line 1891 "expr_parser.tab.c"
    break;

  case 35: /* unary_expr: '+' unary_expr  */
#line 369 "expr_parser.y"
                               { (yyval.val) = (yyvsp[0].val); }
#line 1897 "expr_parser.tab.c"
    break;

  case 36: /* unary_expr: '~' unary_expr  */
#line 370 "expr_parser.y"
                               { (yyval.val) = (yyvsp[0].val).errmsg[0] ? (yyvsp[0].val) : make_val(~(yyvsp[0].val).value); }
#line 1903 "expr_parser.tab.c"
    break;

  case 37: /* unary_expr: '!' unary_expr  */
#line 371 "expr_parser.y"
                               { (yyval.val) = (yyvsp[0].val).errmsg[0] ? (yyvsp[0].val) : make_val(!(yyvsp[0].val).value); }
#line 1909 "expr_parser.tab.c"
    break;

  case 38: /* unary_expr: '(' typename ')' unary_expr  */
#line 372 "expr_parser.y"
                                  { (yyval.val) = do_cast(dbg, &(yyvsp[-2].ty), &(yyvsp[0].val)); }
#line 1915 "expr_parser.tab.c"
    break;

  case 39: /* postfix_expr: primary_expr  */
#line 376 "expr_parser.y"
                                         { (yyval.val) = (yyvsp[0].val); }
#line 1921 "expr_parser.tab.c"
    break;

  case 40: /* postfix_expr: postfix_expr '[' expr ']'  */
#line 377 "expr_parser.y"
                                         { (yyval.val) = do_index(dbg, &(yyvsp[-3].val), &(yyvsp[-1].val)); }
#line 1927 "expr_parser.tab.c"
    break;

  case 41: /* postfix_expr: postfix_expr '.' IDENT  */
#line 378 "expr_parser.y"
                                         { (yyval.val) = do_member(dbg, &(yyvsp[-2].val), (yyvsp[0].text).s, 0); }
#line 1933 "expr_parser.tab.c"
    break;

  case 42: /* postfix_expr: postfix_expr ARROW IDENT  */
#line 379 "expr_parser.y"
                                         { (yyval.val) = do_member(dbg, &(yyvsp[-2].val), (yyvsp[0].text).s, 1); }
#line 1939 "expr_parser.tab.c"
    break;

  case 43: /* primary_expr: NUM  */
#line 383 "expr_parser.y"
                                { (yyval.val) = (yyvsp[0].val); }
#line 1945 "expr_parser.tab.c"
    break;

  case 44: /* primary_expr: IDENT  */
#line 384 "expr_parser.y"
                                { (yyval.val) = do_ident(dbg, (yyvsp[0].text).s); }
#line 1951 "expr_parser.tab.c"
    break;

  case 45: /* primary_expr: SIZEOF '(' typename ')'  */
#line 385 "expr_parser.y"
                                { (yyval.val) = make_val((yyvsp[-1].ty).size); }
#line 1957 "expr_parser.tab.c"
    break;

  case 46: /* primary_expr: SIZEOF '(' expr ')'  */
#line 386 "expr_parser.y"
                                { (yyval.val) = make_val((yyvsp[-1].val).byte_size > 0 ? (long long)(yyvsp[-1].val).byte_size : 4); }
#line 1963 "expr_parser.tab.c"
    break;

  case 47: /* primary_expr: '(' expr ')'  */
#line 387 "expr_parser.y"
                                { (yyval.val) = (yyvsp[-1].val); }
#line 1969 "expr_parser.tab.c"
    break;

  case 48: /* typename_words: TYPEWORD  */
#line 391 "expr_parser.y"
               {
          etypewords_t w = {0};
          strncpy_s(w.first, sizeof(w.first), (yyvsp[0].text).s, _TRUNCATE);
          strncpy_s(w.full,  sizeof(w.full),  (yyvsp[0].text).s, _TRUNCATE);
          (yyval.words) = w;
      }
#line 1980 "expr_parser.tab.c"
    break;

  case 49: /* typename_words: TYPEWORD TYPEWORD  */
#line 397 "expr_parser.y"
                        {
          etypewords_t w = {0};
          strncpy_s(w.first, sizeof(w.first), (yyvsp[-1].text).s, _TRUNCATE);
          snprintf(w.full, sizeof(w.full), "%s %s", (yyvsp[-1].text).s, (yyvsp[0].text).s);
          (yyval.words) = w;
      }
#line 1991 "expr_parser.tab.c"
    break;

  case 50: /* typename_words: TYPEWORD TYPEWORD TYPEWORD  */
#line 403 "expr_parser.y"
                                 {
          etypewords_t w = {0};
          strncpy_s(w.first, sizeof(w.first), (yyvsp[-2].text).s, _TRUNCATE);
          snprintf(w.full, sizeof(w.full), "%s %s %s", (yyvsp[-2].text).s, (yyvsp[-1].text).s, (yyvsp[0].text).s);
          (yyval.words) = w;
      }
#line 2002 "expr_parser.tab.c"
    break;

  case 51: /* typename: typename_words stars_opt  */
#line 412 "expr_parser.y"
                               {
          etypeinfo_t ti = {0};
          int sz = builtin_type_size((yyvsp[-1].words).full);
          if (sz < 0) sz = builtin_type_size((yyvsp[-1].words).first);
          ti.is_ptr = (yyvsp[0].ty).is_ptr;
          ti.size   = ti.is_ptr ? 8 : sz;
          (yyval.ty) = ti;
      }
#line 2015 "expr_parser.tab.c"
    break;

  case 52: /* stars_opt: %empty  */
#line 423 "expr_parser.y"
                     { etypeinfo_t z = {0}; (yyval.ty) = z; }
#line 2021 "expr_parser.tab.c"
    break;

  case 53: /* stars_opt: stars_opt '*'  */
#line 424 "expr_parser.y"
                     { etypeinfo_t z = {0}; z.is_ptr = 1; (yyval.ty) = z; }
#line 2027 "expr_parser.tab.c"
    break;


#line 2031 "expr_parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == EPARSEEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (dbg, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= EPARSEEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == EPARSEEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, dbg);
          yychar = EPARSEEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, dbg);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (dbg, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != EPARSEEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, dbg);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, dbg);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 427 "expr_parser.y"


void eparseerror(debugger_t *dbg, const char *s)
{
    (void)dbg;
    if (!expr_parse_err)
    {
        expr_parse_err = 1;
        strncpy_s(expr_parse_errmsg, sizeof(expr_parse_errmsg), s, _TRUNCATE);
    }
}

#ifndef YY_NO_UNISTD_H
#define YY_NO_UNISTD_H 1
#endif
#include "expr_lexer.h"

int expr_parse(debugger_t *dbg, const char *src, expr_val_t *out)
{
    expr_parse_err = 0;
    expr_parse_errmsg[0] = '\0';
    memset(&g_expr_result, 0, sizeof(g_expr_result));

    YY_BUFFER_STATE buf = eparse_scan_string(src);
    int rc = eparseparse(dbg);
    eparse_delete_buffer(buf);

    if (out) *out = g_expr_result;

    if (rc != 0 || expr_parse_err || g_expr_result.errmsg[0])
    {
        if (out && !out->errmsg[0])
            strncpy_s(out->errmsg, sizeof(out->errmsg),
                      expr_parse_errmsg[0] ? expr_parse_errmsg : "parse error", _TRUNCATE);
        return -1;
    }
    return 0;
}
