/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_NL = 258,
     TOK_ATOM = 259,
     TOK_INTEGER = 260,
     TOK_STRING = 261,
     TOK_CLASS = 262,
     TOK_DEF_CLASS = 263,
     TOK_DEF_TYPES = 264,
     TOK_END_CLASS = 265,
     TOK_END_GROUP = 266,
     TOK_GROUP = 267,
     TOK_INCLUDE = 268,
     TOK_PATH_FORMAT = 269,
     TOK_PACKING_LOGIC = 270,
     TOK_SENSOR = 271,
     TOK_SENSORS = 272,
     TOK_TYPE = 273,
     TOK_VERSION = 274,
     ERR_UNK_CMD = 275,
     ERR_UNREC = 276,
     ERR_UNTERM_STRING = 277,
     ERR_STR_TOO_LONG = 278,
     ERR_INVALID_OCTAL_ESCAPE = 279
   };
#endif
/* Tokens.  */
#define TOK_NL 258
#define TOK_ATOM 259
#define TOK_INTEGER 260
#define TOK_STRING 261
#define TOK_CLASS 262
#define TOK_DEF_CLASS 263
#define TOK_DEF_TYPES 264
#define TOK_END_CLASS 265
#define TOK_END_GROUP 266
#define TOK_GROUP 267
#define TOK_INCLUDE 268
#define TOK_PATH_FORMAT 269
#define TOK_PACKING_LOGIC 270
#define TOK_SENSOR 271
#define TOK_SENSORS 272
#define TOK_TYPE 273
#define TOK_VERSION 274
#define ERR_UNK_CMD 275
#define ERR_UNREC 276
#define ERR_UNTERM_STRING 277
#define ERR_STR_TOO_LONG 278
#define ERR_INVALID_OCTAL_ESCAPE 279




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 159 "sksiteconfig_parse.y"
{
    int integer;
    char *str;
    sk_vector_t *str_list;
}
/* Line 1529 of yacc.c.  */
#line 103 "sksiteconfig_parse.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

