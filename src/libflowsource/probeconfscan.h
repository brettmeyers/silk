/*
** Copyright (C) 2005-2015 by Carnegie Mellon University.
**
** @OPENSOURCE_HEADER_START@
**
** Use of the SILK system and related source code is subject to the terms
** of the following licenses:
**
** GNU Public License (GPL) Rights pursuant to Version 2, June 1991
** Government Purpose License Rights (GPLR) pursuant to DFARS 252.227.7013
**
** NO WARRANTY
**
** ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER
** PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY
** PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN
** "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY
** KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT
** LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE,
** MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE
** OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT,
** SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY
** TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF
** WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES.
** LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF
** CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON
** CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE
** DELIVERABLES UNDER THIS LICENSE.
**
** Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie
** Mellon University, its trustees, officers, employees, and agents from
** all claims or demands made against them (and any related losses,
** expenses, or attorney's fees) arising out of, or relating to Licensee's
** and/or its sub licensees' negligent use or willful misuse of or
** negligent conduct or willful misconduct regarding the Software,
** facilities, or other rights or assistance granted by Carnegie Mellon
** University under this License, including, but not limited to, any
** claims of product liability, personal injury, death, damage to
** property, or violation of any laws or regulations.
**
** Carnegie Mellon University Software Engineering Institute authored
** documents are sponsored by the U.S. Department of Defense under
** Contract FA8721-05-C-0003. Carnegie Mellon University retains
** copyrights in all material produced under this contract. The U.S.
** Government retains a non-exclusive, royalty-free license to publish or
** reproduce these documents, or allow others to do so, for U.S.
** Government purposes only pursuant to the copyright license under the
** contract clause at 252.227.7013.
**
** @OPENSOURCE_HEADER_END@
*/
#ifndef _PROBECONFSCAN_H
#define _PROBECONFSCAN_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_PROBECONFSCAN_H, "$SiLK: probeconfscan.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/*
**  probeconfscan.h
**
**  Values needed for the lexer and parser to communicate.
**
*/

#include <silk/utils.h>
#include <silk/probeconf.h>
#include <silk/skvector.h>


/* Provide some grammar debugging info, if necessary */
#define YYDEBUG 1

#define PCSCAN_MAX_INCLUDE_DEPTH 8

/* error message strings */
#define PARSE_MSG_ERROR "Error while parsing file %s at line %d:\n"
#define PARSE_MSG_WARN  "Warning while parsing file %s at line %d:\n"


/* this list of definitions is from the automake info page */
#define yymaxdepth  probeconfscan_maxdepth
#define yyparse     probeconfscan_parse
#define yylex       probeconfscan_lex
#define yyerror     probeconfscan_error
#define yylval      probeconfscan_lval
#define yychar      probeconfscan_char
#define yydebug     probeconfscan_debug
#define yypact      probeconfscan_pact
#define yyr1        probeconfscan_r1
#define yyr2        probeconfscan_r2
#define yydef       probeconfscan_def
#define yychk       probeconfscan_chk
#define yypgo       probeconfscan_pgo
#define yyact       probeconfscan_act
#define yyexca      probeconfscan_exca
#define yyerrflag   probeconfscan_errflag
#define yynerrs     probeconfscan_nerrs
#define yyps        probeconfscan_ps
#define yypv        probeconfscan_pv
#define yys         probeconfscan_s
#define yy_yys      probeconfscan_yys
#define yystate     probeconfscan_state
#define yytmp       probeconfscan_tmp
#define yyv         probeconfscan_v
#define yy_yyv      probeconfscan_yyv
#define yyval       probeconfscan_val
#define yylloc      probeconfscan_lloc
#define yyreds      probeconfscan_reds
#define yytoks      probeconfscan_toks
#define yylhs       probeconfscan_yylhs
#define yylen       probeconfscan_yylen
#define yydefred    probeconfscan_yydefred
#define yydgoto     probeconfscan_yydgoto
#define yysindex    probeconfscan_yysindex
#define yyrindex    probeconfscan_yyrindex
#define yygindex    probeconfscan_yygindex
#define yytable     probeconfscan_yytable
#define yycheck     probeconfscan_yycheck
#define yyname      probeconfscan_yyname
#define yyrule      probeconfscan_yyrule


/* Last keyword */
extern char pcscan_clause[];

/* Global error count for return status of skpcParse */
extern int pcscan_errors;

extern int (*extra_sensor_verify_fn)(skpc_sensor_t *sensor);


int
yyparse(
    void);
int
yylex(
    void);
int
yyerror(
    char               *s);

typedef sk_vector_t number_list_t;

typedef sk_vector_t wildcard_list_t;


int
skpcParseSetup(
    void);

void
skpcParseTeardown(
    void);

#ifdef TEST_PRINTF_FORMATS
#define skpcParseErr printf
#else
int
skpcParseErr(
    const char         *fmt,
    ...)
    SK_CHECK_PRINTF(1, 2);
#endif

int
skpcParseIncludePop(
    void);

int
skpcParseIncludePush(
    char               *filename);


#if 0
/* Newer versions of flex define these functions.  Declare them here
 * to avoid gcc warnings, and just hope that their signatures don't
 * change. */
int
probeconfscan_get_leng(
    void);
char *
probeconfscan_get_text(
    void);
int
probeconfscan_get_debug(
    void);
void
probeconfscan_set_debug(
    int                 bdebug);
int
probeconfscan_get_lineno(
    void);
void
probeconfscan_set_lineno(
    int                 line_number);
FILE *
probeconfscan_get_in(
    void);
void
probeconfscan_set_in(
    FILE               *in_str);
FILE *
probeconfscan_get_out(
    void);
void
probeconfscan_set_out(
    FILE               *out_str);
int
probeconfscan_lex_destroy(
    void);
#endif  /* #if 0 */

#ifdef __cplusplus
}
#endif
#endif /* _PROBECONFSCAN_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
