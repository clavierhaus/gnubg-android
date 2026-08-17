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

#line 144 "external_y.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int yyparse (scancontext *scanner);


#endif /* !YY_YY_EXTERNAL_Y_H_INCLUDED  */
