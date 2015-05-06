/*
** Copyright (C) 2006-2015 by Carnegie Mellon University.
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
#ifndef _SKSITECONFIG_H
#define _SKSITECONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcs_SKSITECONFIG_H, "$SiLK: sksiteconfig.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skvector.h>

#define SKSITECONFIG_MAX_INCLUDE_DEPTH 16

/* from sksite.c */

extern const char path_format_conversions[];


/* from sksiteconfig_parse.y */

extern int
sksiteconfig_error(
    char               *s);
extern int
sksiteconfig_parse(
    void);

extern int sksiteconfig_testing;


/* from sksiteconfig_lex.l */

extern int
sksiteconfig_lex(
    void);

int
sksiteconfigParse(
    const char         *filename,
    int                 verbose);

#ifdef TEST_PRINTF_FORMATS
#define sksiteconfigErr printf
#else
void
sksiteconfigErr(
    const char         *fmt,
    ...)
    SK_CHECK_PRINTF(1, 2);
#endif

int
sksiteconfigIncludePop(
    void);
void
sksiteconfigIncludePush(
    char               *filename);


/* this list of definitions is from the automake info page */
#define yymaxdepth  sksiteconfig_maxdepth
#define yyparse     sksiteconfig_parse
#define yylex       sksiteconfig_lex
#define yyerror     sksiteconfig_error
#define yylval      sksiteconfig_lval
#define yychar      sksiteconfig_char
#define yydebug     sksiteconfig_debug
#define yypact      sksiteconfig_pact
#define yyr1        sksiteconfig_r1
#define yyr2        sksiteconfig_r2
#define yydef       sksiteconfig_def
#define yychk       sksiteconfig_chk
#define yypgo       sksiteconfig_pgo
#define yyact       sksiteconfig_act
#define yyexca      sksiteconfig_exca
#define yyerrflag   sksiteconfig_errflag
#define yynerrs     sksiteconfig_nerrs
#define yyps        sksiteconfig_ps
#define yypv        sksiteconfig_pv
#define yys         sksiteconfig_s
#define yy_yys      sksiteconfig_yys
#define yystate     sksiteconfig_state
#define yytmp       sksiteconfig_tmp
#define yyv         sksiteconfig_v
#define yy_yyv      sksiteconfig_yyv
#define yyval       sksiteconfig_val
#define yylloc      sksiteconfig_lloc
#define yyreds      sksiteconfig_reds
#define yytoks      sksiteconfig_toks
#define yylhs       sksiteconfig_yylhs
#define yylen       sksiteconfig_yylen
#define yydefred    sksiteconfig_yydefred
#define yydgoto     sksiteconfig_yydgoto
#define yysindex    sksiteconfig_yysindex
#define yyrindex    sksiteconfig_yyrindex
#define yygindex    sksiteconfig_yygindex
#define yytable     sksiteconfig_yytable
#define yycheck     sksiteconfig_yycheck
#define yyname      sksiteconfig_yyname
#define yyrule      sksiteconfig_yyrule

#if 0
/* Newer versions of flex define these functions.  Declare them here
 * to avoid gcc warnings, and just hope that their signatures don't
 * change. */
int
sksiteconfig_get_leng(
    void);
char *
sksiteconfig_get_text(
    void);
int
sksiteconfig_get_debug(
    void);
void
sksiteconfig_set_debug(
    int                 bdebug);
int
sksiteconfig_get_lineno(
    void);
void
sksiteconfig_set_lineno(
    int                 line_number);
FILE *
sksiteconfig_get_in(
    void);
void
sksiteconfig_set_in(
    FILE               *in_str);
FILE *
sksiteconfig_get_out(
    void);
void
sksiteconfig_set_out(
    FILE               *out_str);
int
sksiteconfig_lex_destroy(
    void);
#endif  /* #if 0 */

#ifdef __cplusplus
}
#endif
#endif /* _SKSITECONFIG_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
