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
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "external_y.y"

/*
 * Copyright (C) 2014-2015 Michael Petch <mpetch@gnubg.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * $Id: external_y.y,v 1.37 2023/12/14 07:02:38 plm Exp $
 */

#ifndef EXTERNAL_Y_H
#define EXTERNAL_Y_H

#define MERGE_(a,b)  a##b
#define LABEL_(a,b) MERGE_(a, b)

#define YY_PREFIX(variable) MERGE_(ext_,variable)

#define yymaxdepth YY_PREFIX(maxdepth)
#define yyparse YY_PREFIX(parse)
#define yylex   YY_PREFIX(lex)
#define yyerror YY_PREFIX(error)
#define yylval  YY_PREFIX(lval)
#define yychar  YY_PREFIX(char)
#define yydebug YY_PREFIX(debug)
#define yypact  YY_PREFIX(pact)
#define yyr1    YY_PREFIX(r1)
#define yyr2    YY_PREFIX(r2)
#define yydef   YY_PREFIX(def)
#define yychk   YY_PREFIX(chk)
#define yypgo   YY_PREFIX(pgo)
#define yyact   YY_PREFIX(act)
#define yyexca  YY_PREFIX(exca)
#define yyerrflag YY_PREFIX(errflag)
#define yynerrs YY_PREFIX(nerrs)
#define yyps    YY_PREFIX(ps)
#define yypv    YY_PREFIX(pv)
#define yys     YY_PREFIX(s)
#define yy_yys  YY_PREFIX(yys)
#define yystate YY_PREFIX(state)
#define yytmp   YY_PREFIX(tmp)
#define yyv     YY_PREFIX(v)
#define yy_yyv  YY_PREFIX(yyv)
#define yyval   YY_PREFIX(val)
#define yylloc  YY_PREFIX(lloc)
#define yyreds  YY_PREFIX(reds)
#define yytoks  YY_PREFIX(toks)
#define yylhs   YY_PREFIX(yylhs)
#define yylen   YY_PREFIX(yylen)
#define yydefred YY_PREFIX(yydefred)
#define yydgoto  YY_PREFIX(yydgoto)
#define yysindex YY_PREFIX(yysindex)
#define yyrindex YY_PREFIX(yyrindex)
#define yygindex YY_PREFIX(yygindex)
#define yytable  YY_PREFIX(yytable)
#define yycheck  YY_PREFIX(yycheck)
#define yyname   YY_PREFIX(yyname)
#define yyrule   YY_PREFIX(yyrule)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "glib-ext.h"
#include "external.h"
#include "backgammon.h"
#include "external_y.h"

/* Resolve a warning on older GLIBC/GNU systems that have stpcpy */
#if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE && !defined(__USE_XOPEN2K8)
extern char *stpcpy(char *s1, const char *s2);
#endif

#define extcmd ext_get_extra(scanner)  

int YY_PREFIX(get_column)  (void * yyscanner );
void YY_PREFIX(set_column) (int column_no, void * yyscanner );
extern int YY_PREFIX(lex) (YYSTYPE * yylval_param, scancontext *scanner);         
extern scancontext *YY_PREFIX(get_extra) (void *yyscanner );
extern void StartParse(void *scancontext);
extern void yyerror(scancontext *scanner, const char *str);

void yyerror(scancontext *scanner, const char *str)
{
    if (extcmd->ExtErrorHandler)
        extcmd->ExtErrorHandler(extcmd, str);
    else
        fprintf(stderr,"Error: %s\n",str);
}

#endif


#line 176 "external_y.c"

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

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_EXTERNAL_Y_H_INCLUDED
# define YY_YY_EXTERNAL_Y_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    EOL = 258,                     /* EOL  */
    EXIT = 259,                    /* EXIT  */
    DISABLED = 260,                /* DISABLED  */
    INTERFACEVERSION = 261,        /* INTERFACEVERSION  */
    DEBUG = 262,                   /* DEBUG  */
    SET = 263,                     /* SET  */
    NEW = 264,                     /* NEW  */
    OLD = 265,                     /* OLD  */
    OUTPUT = 266,                  /* OUTPUT  */
    E_INTERFACE = 267,             /* E_INTERFACE  */
    HELP = 268,                    /* HELP  */
    PROMPT = 269,                  /* PROMPT  */
    E_STRING = 270,                /* E_STRING  */
    E_CHARACTER = 271,             /* E_CHARACTER  */
    E_INTEGER = 272,               /* E_INTEGER  */
    E_FLOAT = 273,                 /* E_FLOAT  */
    E_BOOLEAN = 274,               /* E_BOOLEAN  */
    FIBSBOARD = 275,               /* FIBSBOARD  */
    FIBSBOARDEND = 276,            /* FIBSBOARDEND  */
    EVALUATION = 277,              /* EVALUATION  */
    CRAWFORDRULE = 278,            /* CRAWFORDRULE  */
    JACOBYRULE = 279,              /* JACOBYRULE  */
    RESIGNATION = 280,             /* RESIGNATION  */
    BEAVERS = 281,                 /* BEAVERS  */
    CUBE = 282,                    /* CUBE  */
    CUBEFUL = 283,                 /* CUBEFUL  */
    CUBELESS = 284,                /* CUBELESS  */
    DETERMINISTIC = 285,           /* DETERMINISTIC  */
    NOISE = 286,                   /* NOISE  */
    PLIES = 287,                   /* PLIES  */
    PRUNE = 288                    /* PRUNE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define EOL 258
#define EXIT 259
#define DISABLED 260
#define INTERFACEVERSION 261
#define DEBUG 262
#define SET 263
#define NEW 264
#define OLD 265
#define OUTPUT 266
#define E_INTERFACE 267
#define HELP 268
#define PROMPT 269
#define E_STRING 270
#define E_CHARACTER 271
#define E_INTEGER 272
#define E_FLOAT 273
#define E_BOOLEAN 274
#define FIBSBOARD 275
#define FIBSBOARDEND 276
#define EVALUATION 277
#define CRAWFORDRULE 278
#define JACOBYRULE 279
#define RESIGNATION 280
#define BEAVERS 281
#define CUBE 282
#define CUBEFUL 283
#define CUBELESS 284
#define DETERMINISTIC 285
#define NOISE 286
#define PLIES 287
#define PRUNE 288

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 114 "external_y.y"

    gboolean boolean;
    gchar character;
    gfloat floatnum;
    gint intnum;
    GString *str;
    GValue *gv;
    GList *list;
    commandinfo *cmd;

#line 306 "external_y.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int yyparse (scancontext *scanner);


#endif /* !YY_YY_EXTERNAL_Y_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_EOL = 3,                        /* EOL  */
  YYSYMBOL_EXIT = 4,                       /* EXIT  */
  YYSYMBOL_DISABLED = 5,                   /* DISABLED  */
  YYSYMBOL_INTERFACEVERSION = 6,           /* INTERFACEVERSION  */
  YYSYMBOL_DEBUG = 7,                      /* DEBUG  */
  YYSYMBOL_SET = 8,                        /* SET  */
  YYSYMBOL_NEW = 9,                        /* NEW  */
  YYSYMBOL_OLD = 10,                       /* OLD  */
  YYSYMBOL_OUTPUT = 11,                    /* OUTPUT  */
  YYSYMBOL_E_INTERFACE = 12,               /* E_INTERFACE  */
  YYSYMBOL_HELP = 13,                      /* HELP  */
  YYSYMBOL_PROMPT = 14,                    /* PROMPT  */
  YYSYMBOL_E_STRING = 15,                  /* E_STRING  */
  YYSYMBOL_E_CHARACTER = 16,               /* E_CHARACTER  */
  YYSYMBOL_E_INTEGER = 17,                 /* E_INTEGER  */
  YYSYMBOL_E_FLOAT = 18,                   /* E_FLOAT  */
  YYSYMBOL_E_BOOLEAN = 19,                 /* E_BOOLEAN  */
  YYSYMBOL_FIBSBOARD = 20,                 /* FIBSBOARD  */
  YYSYMBOL_FIBSBOARDEND = 21,              /* FIBSBOARDEND  */
  YYSYMBOL_EVALUATION = 22,                /* EVALUATION  */
  YYSYMBOL_CRAWFORDRULE = 23,              /* CRAWFORDRULE  */
  YYSYMBOL_JACOBYRULE = 24,                /* JACOBYRULE  */
  YYSYMBOL_RESIGNATION = 25,               /* RESIGNATION  */
  YYSYMBOL_BEAVERS = 26,                   /* BEAVERS  */
  YYSYMBOL_CUBE = 27,                      /* CUBE  */
  YYSYMBOL_CUBEFUL = 28,                   /* CUBEFUL  */
  YYSYMBOL_CUBELESS = 29,                  /* CUBELESS  */
  YYSYMBOL_DETERMINISTIC = 30,             /* DETERMINISTIC  */
  YYSYMBOL_NOISE = 31,                     /* NOISE  */
  YYSYMBOL_PLIES = 32,                     /* PLIES  */
  YYSYMBOL_PRUNE = 33,                     /* PRUNE  */
  YYSYMBOL_34_ = 34,                       /* ':'  */
  YYSYMBOL_35_ = 35,                       /* '('  */
  YYSYMBOL_36_ = 36,                       /* ')'  */
  YYSYMBOL_37_ = 37,                       /* ','  */
  YYSYMBOL_YYACCEPT = 38,                  /* $accept  */
  YYSYMBOL_commands = 39,                  /* commands  */
  YYSYMBOL_setcommand = 40,                /* setcommand  */
  YYSYMBOL_command = 41,                   /* command  */
  YYSYMBOL_board_element = 42,             /* board_element  */
  YYSYMBOL_board_elements = 43,            /* board_elements  */
  YYSYMBOL_endboard = 44,                  /* endboard  */
  YYSYMBOL_sessionoption = 45,             /* sessionoption  */
  YYSYMBOL_evaloption = 46,                /* evaloption  */
  YYSYMBOL_sessionoptions = 47,            /* sessionoptions  */
  YYSYMBOL_evaloptions = 48,               /* evaloptions  */
  YYSYMBOL_boardcommand = 49,              /* boardcommand  */
  YYSYMBOL_evalcommand = 50,               /* evalcommand  */
  YYSYMBOL_board = 51,                     /* board  */
  YYSYMBOL_float_type = 52,                /* float_type  */
  YYSYMBOL_string_type = 53,               /* string_type  */
  YYSYMBOL_integer_type = 54,              /* integer_type  */
  YYSYMBOL_boolean_type = 55,              /* boolean_type  */
  YYSYMBOL_list_type = 56,                 /* list_type  */
  YYSYMBOL_basic_types = 57,               /* basic_types  */
  YYSYMBOL_list = 58,                      /* list  */
  YYSYMBOL_list_element = 59,              /* list_element  */
  YYSYMBOL_list_elements = 60              /* list_elements  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 125 "external_y.y"


#line 394 "external_y.c"


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
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

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
#define YYFINAL  25
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   74

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  23
/* YYNRULES -- Number of rules.  */
#define YYNRULES  55
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  85

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   288


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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      35,    36,     2,     2,    37,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    34,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   173,   173,   179,   186,   192,   198,   204,   258,   263,
     269,   275,   282,   290,   298,   310,   314,   319,   326,   330,
     335,   340,   345,   352,   357,   362,   370,   375,   380,   385,
     390,   395,   400,   408,   425,   433,   450,   455,   462,   476,
     491,   503,   511,   520,   528,   536,   546,   546,   546,   546,
     551,   558,   558,   563,   567,   572
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
  "\"end of file\"", "error", "\"invalid token\"", "EOL", "EXIT",
  "DISABLED", "INTERFACEVERSION", "DEBUG", "SET", "NEW", "OLD", "OUTPUT",
  "E_INTERFACE", "HELP", "PROMPT", "E_STRING", "E_CHARACTER", "E_INTEGER",
  "E_FLOAT", "E_BOOLEAN", "FIBSBOARD", "FIBSBOARDEND", "EVALUATION",
  "CRAWFORDRULE", "JACOBYRULE", "RESIGNATION", "BEAVERS", "CUBE",
  "CUBEFUL", "CUBELESS", "DETERMINISTIC", "NOISE", "PLIES", "PRUNE", "':'",
  "'('", "')'", "','", "$accept", "commands", "setcommand", "command",
  "board_element", "board_elements", "endboard", "sessionoption",
  "evaloption", "sessionoptions", "evaloptions", "boardcommand",
  "evalcommand", "board", "float_type", "string_type", "integer_type",
  "boolean_type", "list_type", "basic_types", "list", "list_element",
  "list_elements", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-49)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
       3,   -49,     7,   -23,    15,    10,    26,     0,    19,    32,
      41,   -49,   -49,   -49,   -49,   -14,   -49,   -49,    27,    18,
      34,    44,   -49,    16,    43,   -49,   -49,    12,   -49,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
       4,   -49,   -49,   -49,   -49,   -49,    49,   -49,    27,    27,
      48,    27,   -49,   -49,   -14,    33,    29,   -49,   -49,   -49,
     -49,   -49,    48,    27,   -49,   -49,    27,    25,    48,    27,
     -49,   -49,   -49,    -8,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,    48,   -49,   -49
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     2,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    12,    13,    33,     6,    53,    14,     4,     0,     0,
       0,     0,     5,     0,     0,     1,     7,    38,    42,    43,
      41,    44,    48,    47,    49,    46,    52,    51,    45,    54,
       0,     8,     9,    10,    11,     3,     0,    35,     0,     0,
       0,     0,    34,    50,     0,     0,    39,    20,    19,    21,
      22,    55,     0,     0,    31,    32,    28,     0,     0,    26,
      37,    36,    16,     0,    15,    30,    29,    24,    25,    23,
      27,    18,     0,    40,    17
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -49,   -49,   -49,   -49,   -16,   -49,   -49,    13,   -49,   -49,
     -49,   -49,   -49,    46,     1,    51,   -48,   -18,   -49,   -49,
      69,    20,   -49
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     9,    21,    10,    72,    73,    83,    52,    71,    27,
      56,    11,    12,    13,    32,    33,    34,    35,    36,    37,
      38,    39,    40
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      41,    28,    59,    29,    30,    31,     1,     2,     3,     4,
      14,     5,    15,    81,    74,    23,     6,    18,    17,    78,
      79,    15,    19,     7,    20,     8,    82,    42,    43,    22,
      57,    58,    25,    60,    74,    48,    49,    50,    51,    24,
      53,    54,    29,    30,    26,    75,    31,    45,    76,    28,
      46,    80,    48,    49,    50,    51,    63,    64,    65,    66,
      67,    68,    69,     7,    55,    29,    84,    62,    77,    70,
      47,    44,    16,     0,    61
};

static const yytype_int8 yycheck[] =
{
      18,    15,    50,    17,    18,    19,     3,     4,     5,     6,
       3,     8,    35,    21,    62,    15,    13,     7,     3,    67,
      68,    35,    12,    20,    14,    22,    34,     9,    10,     3,
      48,    49,     0,    51,    82,    23,    24,    25,    26,    20,
      36,    37,    17,    18,     3,    63,    19,     3,    66,    15,
      34,    69,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    20,    15,    17,    82,    34,    67,    56,
      24,    20,     3,    -1,    54
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     8,    13,    20,    22,    39,
      41,    49,    50,    51,     3,    35,    58,     3,     7,    12,
      14,    40,     3,    15,    20,     0,     3,    47,    15,    17,
      18,    19,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    55,     9,    10,    53,     3,    34,    51,    23,    24,
      25,    26,    45,    36,    37,    15,    48,    55,    55,    54,
      55,    59,    34,    27,    28,    29,    30,    31,    32,    33,
      45,    46,    42,    43,    54,    55,    55,    52,    54,    54,
      55,    21,    34,    44,    42
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    38,    39,    39,    39,    39,    39,    39,    40,    40,
      40,    40,    41,    41,    41,    42,    43,    43,    44,    45,
      45,    45,    45,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    47,    47,    48,    48,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    57,    57,    57,
      58,    59,    59,    60,    60,    60
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     3,     2,     2,     2,     2,     2,     2,
       2,     2,     1,     1,     2,     1,     1,     3,     1,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     1,     2,
       2,     1,     1,     0,     2,     0,     2,     2,     2,     4,
       7,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     1,     0,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

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
                  Kind, Value, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, scancontext *scanner)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (scanner);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, scancontext *scanner)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, scanner);
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
                 int yyrule, scancontext *scanner)
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
                       &yyvsp[(yyi + 1) - (yynrhs)], scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


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
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, scancontext *scanner)
{
  YY_USE (yyvaluep);
  YY_USE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_E_STRING: /* E_STRING  */
#line 166 "external_y.y"
            { if (((*yyvaluep).str)) g_string_free(((*yyvaluep).str), TRUE); }
#line 1414 "external_y.c"
        break;

    case YYSYMBOL_setcommand: /* setcommand  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1420 "external_y.c"
        break;

    case YYSYMBOL_command: /* command  */
#line 169 "external_y.y"
            { if (((*yyvaluep).cmd)) { g_free(((*yyvaluep).cmd)); }}
#line 1426 "external_y.c"
        break;

    case YYSYMBOL_board_element: /* board_element  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1432 "external_y.c"
        break;

    case YYSYMBOL_board_elements: /* board_elements  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1438 "external_y.c"
        break;

    case YYSYMBOL_sessionoption: /* sessionoption  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1444 "external_y.c"
        break;

    case YYSYMBOL_evaloption: /* evaloption  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1450 "external_y.c"
        break;

    case YYSYMBOL_sessionoptions: /* sessionoptions  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1456 "external_y.c"
        break;

    case YYSYMBOL_evaloptions: /* evaloptions  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1462 "external_y.c"
        break;

    case YYSYMBOL_boardcommand: /* boardcommand  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1468 "external_y.c"
        break;

    case YYSYMBOL_evalcommand: /* evalcommand  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1474 "external_y.c"
        break;

    case YYSYMBOL_board: /* board  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1480 "external_y.c"
        break;

    case YYSYMBOL_float_type: /* float_type  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1486 "external_y.c"
        break;

    case YYSYMBOL_string_type: /* string_type  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1492 "external_y.c"
        break;

    case YYSYMBOL_integer_type: /* integer_type  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1498 "external_y.c"
        break;

    case YYSYMBOL_boolean_type: /* boolean_type  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1504 "external_y.c"
        break;

    case YYSYMBOL_list_type: /* list_type  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1510 "external_y.c"
        break;

    case YYSYMBOL_basic_types: /* basic_types  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1516 "external_y.c"
        break;

    case YYSYMBOL_list: /* list  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1522 "external_y.c"
        break;

    case YYSYMBOL_list_element: /* list_element  */
#line 168 "external_y.y"
            { if (((*yyvaluep).gv)) { g_value_unsetfree(((*yyvaluep).gv)); }}
#line 1528 "external_y.c"
        break;

    case YYSYMBOL_list_elements: /* list_elements  */
#line 167 "external_y.y"
            { if (((*yyvaluep).list)) g_list_free(((*yyvaluep).list)); }
#line 1534 "external_y.c"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (scancontext *scanner)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

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

  yychar = YYEMPTY; /* Cause a token to be read.  */

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
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
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
  yychar = YYEMPTY;
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
  case 2: /* commands: EOL  */
#line 174 "external_y.y"
        {
            extcmd->ct = COMMAND_NONE;
            YYACCEPT;
        }
#line 1816 "external_y.c"
    break;

  case 3: /* commands: SET setcommand EOL  */
#line 180 "external_y.y"
        {
            extcmd->pCmdData = (yyvsp[-1].list);
            extcmd->ct = COMMAND_SET;
            YYACCEPT;
        }
#line 1826 "external_y.c"
    break;

  case 4: /* commands: INTERFACEVERSION EOL  */
#line 187 "external_y.y"
        {
            extcmd->ct = COMMAND_VERSION;
            YYACCEPT;
        }
#line 1835 "external_y.c"
    break;

  case 5: /* commands: HELP EOL  */
#line 193 "external_y.y"
        {
            extcmd->ct = COMMAND_HELP;
            YYACCEPT;
        }
#line 1844 "external_y.c"
    break;

  case 6: /* commands: EXIT EOL  */
#line 199 "external_y.y"
        {
            extcmd->ct = COMMAND_EXIT;
            YYACCEPT;
        }
#line 1853 "external_y.c"
    break;

  case 7: /* commands: command EOL  */
#line 205 "external_y.y"
        {
            if ((yyvsp[-1].cmd)->cmdType == COMMAND_LIST) {
                g_value_unsetfree((yyvsp[-1].cmd)->pvData);
                extcmd->ct = (yyvsp[-1].cmd)->cmdType;
                YYACCEPT;
            } else {
                GMap *optionsmap = (GMap *)g_value_get_boxed((GValue *)g_list_nth_data(g_value_get_boxed((yyvsp[-1].cmd)->pvData), 1));
                GList *boarddata = (GList *)g_value_get_boxed((GValue *)g_list_nth_data(g_value_get_boxed((yyvsp[-1].cmd)->pvData), 0));
                extcmd->ct = (yyvsp[-1].cmd)->cmdType;
                extcmd->pCmdData = (yyvsp[-1].cmd)->pvData;

                if (g_list_length(boarddata) < MAX_RFBF_ELEMENTS) {
                    GVALUE_CREATE(G_TYPE_INT, int, 0, gvfalse); 
                    GVALUE_CREATE(G_TYPE_INT, int, 1, gvtrue); 
                    GVALUE_CREATE(G_TYPE_FLOAT, float, 0.0, gvfloatzero); 

                    extcmd->bi.gsName = g_string_new(g_value_get_gstring_gchar(boarddata->data));
                    extcmd->bi.gsOpp = g_string_new(g_value_get_gstring_gchar(g_list_nth_data(boarddata, 1)));

                    GList *curbrdpos = g_list_nth(boarddata, 2);
                    int *curarraypos = extcmd->anList;
                    while (curbrdpos != NULL) {
                        *curarraypos++ = g_value_get_int(curbrdpos->data);                
                        curbrdpos = g_list_next(curbrdpos);
                    }

                    extcmd->nPlies = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_PLIES, gvfalse));
                    extcmd->fCrawfordRule = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_CRAWFORDRULE, gvfalse));
                    extcmd->fJacobyRule = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_JACOBYRULE, gvfalse));
                    extcmd->fUsePrune = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_PRUNE, gvfalse));
                    extcmd->fCubeful =  g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_CUBEFUL, gvfalse));
                    extcmd->rNoise = g_value_get_float(str2gv_map_get_key_value(optionsmap, KEY_STR_NOISE, gvfloatzero));
                    extcmd->fDeterministic = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_DETERMINISTIC, gvtrue));
                    extcmd->nResignation = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_RESIGNATION, gvfalse));
                    extcmd->fBeavers = g_value_get_int(str2gv_map_get_key_value(optionsmap, KEY_STR_BEAVERS, gvtrue));

                    g_value_unsetfree(gvtrue);
                    g_value_unsetfree(gvfalse);
                    g_value_unsetfree(gvfloatzero);
                    g_free((yyvsp[-1].cmd));
                    
                    YYACCEPT;
                } else {
                    yyerror(scanner, "Invalid board. Maximum number of elements is 52");
                    g_value_unsetfree((yyvsp[-1].cmd)->pvData);                
                    g_free((yyvsp[-1].cmd));
                    YYERROR;
                }
            }
        }
#line 1908 "external_y.c"
    break;

  case 8: /* setcommand: DEBUG boolean_type  */
#line 259 "external_y.y"
        {
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_DEBUG, (yyvsp[0].gv));
        }
#line 1916 "external_y.c"
    break;

  case 9: /* setcommand: E_INTERFACE NEW  */
#line 264 "external_y.y"
        {
            GVALUE_CREATE(G_TYPE_INT, int, 1, gvint); 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_NEWINTERFACE, gvint);
        }
#line 1925 "external_y.c"
    break;

  case 10: /* setcommand: E_INTERFACE OLD  */
#line 270 "external_y.y"
        {
            GVALUE_CREATE(G_TYPE_INT, int, 0, gvint); 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_NEWINTERFACE, gvint);
        }
#line 1934 "external_y.c"
    break;

  case 11: /* setcommand: PROMPT string_type  */
#line 276 "external_y.y"
        {
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_PROMPT, (yyvsp[0].gv));
        }
#line 1942 "external_y.c"
    break;

  case 12: /* command: boardcommand  */
#line 283 "external_y.y"
        {
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = (yyvsp[0].gv);
            cmdInfo->cmdType = COMMAND_FIBSBOARD;
            (yyval.cmd) = cmdInfo;
        }
#line 1953 "external_y.c"
    break;

  case 13: /* command: evalcommand  */
#line 291 "external_y.y"
        {
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = (yyvsp[0].gv);
            cmdInfo->cmdType = COMMAND_EVALUATION;
            (yyval.cmd) = cmdInfo;
        }
#line 1964 "external_y.c"
    break;

  case 14: /* command: DISABLED list  */
#line 299 "external_y.y"
        { 
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, (yyvsp[0].list), gvptr);
            g_list_free((yyvsp[0].list));
            commandinfo *cmdInfo = g_malloc0(sizeof(commandinfo));
            cmdInfo->pvData = gvptr;
            cmdInfo->cmdType = COMMAND_LIST;
            (yyval.cmd) = cmdInfo;
        }
#line 1977 "external_y.c"
    break;

  case 16: /* board_elements: board_element  */
#line 315 "external_y.y"
        { 
            (yyval.list) = g_list_prepend(NULL, (yyvsp[0].gv)); 
        }
#line 1985 "external_y.c"
    break;

  case 17: /* board_elements: board_elements ':' board_element  */
#line 320 "external_y.y"
        { 
            (yyval.list) = g_list_prepend((yyvsp[-2].list), (yyvsp[0].gv)); 
        }
#line 1993 "external_y.c"
    break;

  case 19: /* sessionoption: JACOBYRULE boolean_type  */
#line 331 "external_y.y"
        { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_JACOBYRULE, (yyvsp[0].gv)); 
        }
#line 2001 "external_y.c"
    break;

  case 20: /* sessionoption: CRAWFORDRULE boolean_type  */
#line 336 "external_y.y"
        { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_CRAWFORDRULE, (yyvsp[0].gv));
        }
#line 2009 "external_y.c"
    break;

  case 21: /* sessionoption: RESIGNATION integer_type  */
#line 341 "external_y.y"
        { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_RESIGNATION, (yyvsp[0].gv));
        }
#line 2017 "external_y.c"
    break;

  case 22: /* sessionoption: BEAVERS boolean_type  */
#line 346 "external_y.y"
        { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_BEAVERS, (yyvsp[0].gv));
        }
#line 2025 "external_y.c"
    break;

  case 23: /* evaloption: PLIES integer_type  */
#line 353 "external_y.y"
        { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_PLIES, (yyvsp[0].gv)); 
        }
#line 2033 "external_y.c"
    break;

  case 24: /* evaloption: NOISE float_type  */
#line 358 "external_y.y"
        {
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_NOISE, (yyvsp[0].gv)); 
        }
#line 2041 "external_y.c"
    break;

  case 25: /* evaloption: NOISE integer_type  */
#line 363 "external_y.y"
        {
            float floatval = (float) g_value_get_int((yyvsp[0].gv)) / 10000.0f;
            GVALUE_CREATE(G_TYPE_FLOAT, float, floatval, gvfloat); 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_NOISE, gvfloat); 
            g_value_unsetfree((yyvsp[0].gv));
        }
#line 2052 "external_y.c"
    break;

  case 26: /* evaloption: PRUNE  */
#line 371 "external_y.y"
        { 
            (yyval.list) = create_str2int_tuple (KEY_STR_PRUNE, TRUE);
        }
#line 2060 "external_y.c"
    break;

  case 27: /* evaloption: PRUNE boolean_type  */
#line 376 "external_y.y"
        { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_PRUNE, (yyvsp[0].gv));
        }
#line 2068 "external_y.c"
    break;

  case 28: /* evaloption: DETERMINISTIC  */
#line 381 "external_y.y"
        { 
            (yyval.list) = create_str2int_tuple (KEY_STR_DETERMINISTIC, TRUE);
        }
#line 2076 "external_y.c"
    break;

  case 29: /* evaloption: DETERMINISTIC boolean_type  */
#line 386 "external_y.y"
        { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_DETERMINISTIC, (yyvsp[0].gv));
        }
#line 2084 "external_y.c"
    break;

  case 30: /* evaloption: CUBE boolean_type  */
#line 391 "external_y.y"
        { 
            (yyval.list) = create_str2gvalue_tuple (KEY_STR_CUBEFUL, (yyvsp[0].gv));
        }
#line 2092 "external_y.c"
    break;

  case 31: /* evaloption: CUBEFUL  */
#line 396 "external_y.y"
        { 
            (yyval.list) = create_str2int_tuple (KEY_STR_CUBEFUL, TRUE); 
        }
#line 2100 "external_y.c"
    break;

  case 32: /* evaloption: CUBELESS  */
#line 401 "external_y.y"
        { 
            (yyval.list) = create_str2int_tuple (KEY_STR_CUBEFUL, FALSE); 
        }
#line 2108 "external_y.c"
    break;

  case 33: /* sessionoptions: %empty  */
#line 408 "external_y.y"
        { 
            /* Setup the defaults */
            STR2GV_MAPENTRY_CREATE(KEY_STR_JACOBYRULE, fJacoby, G_TYPE_INT, 
                                    int, jacobyentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_CRAWFORDRULE, TRUE, G_TYPE_INT, 
                                    int, crawfordentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_RESIGNATION, FALSE, G_TYPE_INT, 
                                    int, resignentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_BEAVERS, TRUE, G_TYPE_INT, 
                                    int, beaversentry);

            GList *defaults = 
                g_list_prepend(g_list_prepend(g_list_prepend(g_list_prepend(NULL, jacobyentry), crawfordentry), \
                               resignentry), beaversentry);
            (yyval.list) = defaults;
        }
#line 2129 "external_y.c"
    break;

  case 34: /* sessionoptions: sessionoptions sessionoption  */
#line 426 "external_y.y"
        { 
            STR2GV_MAP_ADD_ENTRY((yyvsp[-1].list), (yyvsp[0].list), (yyval.list)); 
        }
#line 2137 "external_y.c"
    break;

  case 35: /* evaloptions: %empty  */
#line 433 "external_y.y"
        { 
            /* Setup the defaults */
            STR2GV_MAPENTRY_CREATE(KEY_STR_JACOBYRULE, fJacoby, G_TYPE_INT, 
                                    int, jacobyentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_CRAWFORDRULE, TRUE, G_TYPE_INT, 
                                    int, crawfordentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_RESIGNATION, FALSE, G_TYPE_INT, 
                                    int, resignentry);
            STR2GV_MAPENTRY_CREATE(KEY_STR_BEAVERS, TRUE, G_TYPE_INT, 
                                    int, beaversentry);

            GList *defaults = 
                g_list_prepend(g_list_prepend(g_list_prepend(g_list_prepend(NULL, jacobyentry), crawfordentry), \
                               resignentry), beaversentry);
            (yyval.list) = defaults;
        }
#line 2158 "external_y.c"
    break;

  case 36: /* evaloptions: evaloptions evaloption  */
#line 451 "external_y.y"
        { 
            STR2GV_MAP_ADD_ENTRY((yyvsp[-1].list), (yyvsp[0].list), (yyval.list)); 
        }
#line 2166 "external_y.c"
    break;

  case 37: /* evaloptions: evaloptions sessionoption  */
#line 456 "external_y.y"
        { 
            STR2GV_MAP_ADD_ENTRY((yyvsp[-1].list), (yyvsp[0].list), (yyval.list)); 
        }
#line 2174 "external_y.c"
    break;

  case 38: /* boardcommand: board sessionoptions  */
#line 463 "external_y.y"
        {
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, (yyvsp[-1].list), gvptr1);
            GVALUE_CREATE(G_TYPE_BOXED_MAP_GV, boxed, (yyvsp[0].list), gvptr2);
            GList *newList = g_list_prepend(g_list_prepend(NULL, gvptr2), gvptr1);  
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, newList, gvnewlist);
            (yyval.gv) = gvnewlist;
            g_list_free(newList);
            g_list_free((yyvsp[-1].list));
            g_list_free((yyvsp[0].list));
        }
#line 2189 "external_y.c"
    break;

  case 39: /* evalcommand: EVALUATION FIBSBOARD board evaloptions  */
#line 477 "external_y.y"
        {
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, (yyvsp[-1].list), gvptr1);
            GVALUE_CREATE(G_TYPE_BOXED_MAP_GV, boxed, (yyvsp[0].list), gvptr2);
            
            GList *newList = g_list_prepend(g_list_prepend(NULL, gvptr2), gvptr1);  
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, newList, gvnewlist);
            (yyval.gv) = gvnewlist;
            g_list_free(newList);
            g_list_free((yyvsp[-1].list));
            g_list_free((yyvsp[0].list));
        }
#line 2205 "external_y.c"
    break;

  case 40: /* board: FIBSBOARD E_STRING ':' E_STRING ':' board_elements endboard  */
#line 492 "external_y.y"
        {
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, (yyvsp[-3].str), gvstr1); 
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, (yyvsp[-5].str), gvstr2); 
            (yyvsp[-1].list) = g_list_reverse((yyvsp[-1].list));
            (yyval.list) = g_list_prepend(g_list_prepend((yyvsp[-1].list), gvstr1), gvstr2); 
            g_string_free((yyvsp[-3].str), TRUE);
            g_string_free((yyvsp[-5].str), TRUE);
        }
#line 2218 "external_y.c"
    break;

  case 41: /* float_type: E_FLOAT  */
#line 504 "external_y.y"
        { 
            GVALUE_CREATE(G_TYPE_FLOAT, float, (yyvsp[0].floatnum), gvfloat); 
            (yyval.gv) = gvfloat; 
        }
#line 2227 "external_y.c"
    break;

  case 42: /* string_type: E_STRING  */
#line 512 "external_y.y"
        { 
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, (yyvsp[0].str), gvstr); 
            g_string_free ((yyvsp[0].str), TRUE); 
            (yyval.gv) = gvstr; 
        }
#line 2237 "external_y.c"
    break;

  case 43: /* integer_type: E_INTEGER  */
#line 521 "external_y.y"
        { 
            GVALUE_CREATE(G_TYPE_INT, int, (yyvsp[0].intnum), gvint); 
            (yyval.gv) = gvint; 
        }
#line 2246 "external_y.c"
    break;

  case 44: /* boolean_type: E_BOOLEAN  */
#line 529 "external_y.y"
        { 
            GVALUE_CREATE(G_TYPE_INT, int, (yyvsp[0].boolean), gvint); 
            (yyval.gv) = gvint; 
        }
#line 2255 "external_y.c"
    break;

  case 45: /* list_type: list  */
#line 537 "external_y.y"
        { 
            GVALUE_CREATE(G_TYPE_BOXED_GLIST_GV, boxed, (yyvsp[0].list), gvptr);
            g_list_free((yyvsp[0].list));
            (yyval.gv) = gvptr;
        }
#line 2265 "external_y.c"
    break;

  case 50: /* list: '(' list_elements ')'  */
#line 552 "external_y.y"
        { 
            (yyval.list) = g_list_reverse((yyvsp[-1].list));
        }
#line 2273 "external_y.c"
    break;

  case 53: /* list_elements: %empty  */
#line 563 "external_y.y"
        { 
            (yyval.list) = NULL; 
        }
#line 2281 "external_y.c"
    break;

  case 54: /* list_elements: list_element  */
#line 568 "external_y.y"
        { 
            (yyval.list) = g_list_prepend(NULL, (yyvsp[0].gv));
        }
#line 2289 "external_y.c"
    break;

  case 55: /* list_elements: list_elements ',' list_element  */
#line 573 "external_y.y"
        { 
            (yyval.list) = g_list_prepend((yyvsp[-2].list), (yyvsp[0].gv)); 
        }
#line 2297 "external_y.c"
    break;


#line 2301 "external_y.c"

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
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
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
        yyerror (scanner, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, scanner);
          yychar = YYEMPTY;
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, scanner);
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
  yyerror (scanner, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, scanner);
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

#line 577 "external_y.y"


#ifdef EXTERNAL_TEST

/* 
 * Test code can be built by configuring GNUbg with --without-gtk option and doing the following:
 *
 * ./ylwrap external_l.l lex.yy.c external_l.c -- flex 
 * ./ylwrap external_y.y y.tab.c external_y.c y.tab.h test1_y.h -- bison 
 * gcc -Ilib -I. -Wall `pkg-config  gobject-2.0 --cflags --libs` external_l.c external_y.c  glib-ext.c -DEXTERNAL_TEST -o exttest
 *
 */
 
#define BUFFERSIZE 1024

int fJacoby = TRUE;

int main()
{
    char buffer[BUFFERSIZE];
    scancontext scanctx;

    memset(&scanctx, 0, sizeof(scanctx));
    g_type_init ();
    ExtInitParse((void **)&scanctx);

    while(fgets(buffer, BUFFERSIZE, stdin) != NULL) {
        ExtStartParse(scanctx.scanner, buffer);
        if(scanctx.ct == COMMAND_EXIT)
            return 0;
        
        if (scanctx.bi.gsName)
            g_string_free(scanctx.bi.gsName, TRUE);
        if (scanctx.bi.gsOpp)
            g_string_free(scanctx.bi.gsOpp, TRUE);

        scanctx.bi.gsName = NULL;
        scanctx.bi.gsOpp = NULL;
    }

    ExtDestroyParse(scanctx.scanner);
    return 0;
}

#endif
