/*
** Copyright (C) 2001-2014 by Carnegie Mellon University.
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
#ifndef _RWCOUNT_H
#define _RWCOUNT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWCOUNT_H, "$SiLK: rwcount.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>

/*
 *  rwcount.h
 *
 *    Header file for the rwcount utility.
 */


/* DEFINES AND TYPEDEFS */

/* bin loading schemata */
typedef enum {
    LOAD_MEAN=0, LOAD_START, LOAD_END, LOAD_MIDDLE,
    LOAD_DURATION, LOAD_MAXIMUM, LOAD_MINIMUM
} bin_load_scheme_enum_t;

#define MAX_LOAD_SCHEME LOAD_MINIMUM

#define DEFAULT_LOAD_SCHEME LOAD_DURATION


/* default size of bins, in milliseconds */
#define DEFAULT_BINSIZE 30000

/* Values to use for the start_time and end_time to denote that they
 * are not set */
#define RWCO_UNINIT_START 0
#define RWCO_UNINIT_END   INT64_MAX


/* counting data structure */
typedef struct count_bin_st {
    double bytes;
    double pkts;
    double flows;
} count_bin_t;


typedef struct count_data_st {
    /* size of each bin, in milliseconds */
    int64_t     size;
    /* total number of bins that are allocated */
    int64_t     count;
    /* time on the first bin, in UNIX epoch milliseconds */
    sktime_t    window_min;
    /* one millisecond after the final bin, in UNIX epoch milliseconds */
    sktime_t    window_max;
    /* range of dates for printing of data in UNIX epoch milliseconds */
    sktime_t    start_time;
    sktime_t    end_time;

    /* the data */
    count_bin_t *data;
} count_data_t;


typedef struct count_flags_st {
    /* how to label timestamps */
    uint32_t    timeflags;

    /* bin loading scheme */
    bin_load_scheme_enum_t  load_scheme;

    /* delimiter between columns */
    char        delimiter;

    /* when non-zero, print row label with bin's index value */
    unsigned    label_index         :1;

    /* when non-zero, do not print column titles */
    unsigned    no_titles           :1;

    /* when non-zero, suppress the final delimiter */
    unsigned    no_final_delimiter  :1;

    /* when non-zero, do not print bins with zero counts */
    unsigned    skip_zeroes         :1;

    /* when non-zero, do not print column titles */
    unsigned    no_columns          :1;
} count_flags_t;


/* FUNCTIONS */

void
appSetup(
    int                 argc,
    char              **argv);
void
appTeardown(
    void);
FILE *
getOutputHandle(
    void);


/* VARIABLES */

extern sk_options_ctx_t *optctx;

/* the data */
extern count_data_t bins;

/* flags */
extern count_flags_t flags;

#ifdef __cplusplus
}
#endif
#endif /* _RWCOUNT_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
