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
     ACCEPT_FROM_HOST_T = 258,
     COMMA = 259,
     END_GROUP_T = 260,
     END_PROBE_T = 261,
     END_SENSOR_T = 262,
     EOL = 263,
     GROUP_T = 264,
     INCLUDE_T = 265,
     INTERFACES_T = 266,
     INTERFACE_VALUES_T = 267,
     IPBLOCKS_T = 268,
     ISP_IP_T = 269,
     LISTEN_AS_HOST_T = 270,
     LISTEN_ON_PORT_T = 271,
     LISTEN_ON_USOCKET_T = 272,
     LOG_FLAGS_T = 273,
     POLL_DIRECTORY_T = 274,
     PRIORITY_T = 275,
     PROBE_T = 276,
     PROTOCOL_T = 277,
     QUIRKS_T = 278,
     READ_FROM_FILE_T = 279,
     REMAINDER_T = 280,
     SENSOR_T = 281,
     ID = 282,
     NET_NAME_INTERFACE = 283,
     NET_NAME_IPBLOCK = 284,
     PROBES = 285,
     QUOTED_STRING = 286,
     NET_DIRECTION = 287,
     FILTER = 288,
     ERR_STR_TOO_LONG = 289
   };
#endif
/* Tokens.  */
#define ACCEPT_FROM_HOST_T 258
#define COMMA 259
#define END_GROUP_T 260
#define END_PROBE_T 261
#define END_SENSOR_T 262
#define EOL 263
#define GROUP_T 264
#define INCLUDE_T 265
#define INTERFACES_T 266
#define INTERFACE_VALUES_T 267
#define IPBLOCKS_T 268
#define ISP_IP_T 269
#define LISTEN_AS_HOST_T 270
#define LISTEN_ON_PORT_T 271
#define LISTEN_ON_USOCKET_T 272
#define LOG_FLAGS_T 273
#define POLL_DIRECTORY_T 274
#define PRIORITY_T 275
#define PROBE_T 276
#define PROTOCOL_T 277
#define QUIRKS_T 278
#define READ_FROM_FILE_T 279
#define REMAINDER_T 280
#define SENSOR_T 281
#define ID 282
#define NET_NAME_INTERFACE 283
#define NET_NAME_IPBLOCK 284
#define PROBES 285
#define QUOTED_STRING 286
#define NET_DIRECTION 287
#define FILTER 288
#define ERR_STR_TOO_LONG 289




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 213 "probeconfparse.y"
{
    char               *string;
    sk_vector_t        *vector;
    uint32_t            u32;
    skpc_direction_t    net_dir;
    skpc_filter_t       filter;
}
/* Line 1529 of yacc.c.  */
#line 125 "probeconfparse.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

