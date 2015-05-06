/*
** Copyright (C) 2001-2015 by Carnegie Mellon University.
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
#ifndef _RWTOTAL_H
#define _RWTOTAL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWTOTAL_H, "$SiLK: rwtotal.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/*
 * rwtotal.h
 *
 * Various handy header information for rwtotal.
 */

/* number of things to compute (used to compute size of count_array[]) */
#define NUM_TOTALS 3

/* offsets into the count_array[] array */
#define C_RECS  0
#define C_BYTES 1
#define C_PKTS  2



/* define the options; also these determine how to compute the key for
 * each bin */
typedef enum {
    OPT_SIP_FIRST_8=0, OPT_SIP_FIRST_16,  OPT_SIP_FIRST_24,
    OPT_SIP_LAST_8,    OPT_SIP_LAST_16,
    OPT_DIP_FIRST_8,   OPT_DIP_FIRST_16,  OPT_DIP_FIRST_24,
    OPT_DIP_LAST_8,    OPT_DIP_LAST_16,
    OPT_SPORT,         OPT_DPORT,
    OPT_PROTO,
    OPT_PACKETS,
    OPT_BYTES,
    OPT_DURATION,
    OPT_ICMP_CODE,

    /* above map to count-modes; below control output */

    OPT_SUMMATION,
    OPT_MIN_BYTES, OPT_MIN_PACKETS, OPT_MIN_RECORDS,
    OPT_MAX_BYTES, OPT_MAX_PACKETS, OPT_MAX_RECORDS,
    OPT_SKIP_ZEROES,
    OPT_NO_TITLES,
    OPT_NO_COLUMNS,
    OPT_COLUMN_SEPARATOR,
    OPT_NO_FINAL_DELIMITER,
    OPT_DELIMITED,
    OPT_OUTPUT_PATH,
    OPT_PAGER
} appOptionsEnum;


#define COUNT_MODE_UNSET     -1

/* which of the above is the maximum possible count_mode */
#define COUNT_MODE_MAX_OPTION OPT_ICMP_CODE

/* which of the above is final value to handle IP addresses.  used for
 * ignoring IPv6 addresses */
#define COUNT_MODE_FINAL_ADDR  OPT_DIP_LAST_16

/* which count mode to use */
extern int count_mode;

extern sk_options_ctx_t *optctx;

extern int  summation;
extern int  no_titles;
extern int  no_columns;
extern int  no_final_delimiter;
extern char delimiter;

/* array that holds the counts */
extern uint64_t *count_array;

extern uint64_t bounds[2 * NUM_TOTALS];

void
appTeardown(
    void);
void
appSetup(
    int                 argc,
    char              **argv);
FILE *
getOutputHandle(
    void);


#ifdef __cplusplus
}
#endif
#endif /* _RWTOTAL_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
