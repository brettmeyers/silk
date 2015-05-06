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

/*
** rwcut.c
**
**      cut fields/records from the given input file(s) using field
**      specifications from here, record filter specifications from
**      module libfilter
**
** 1/15/2002
**
** Suresh L. Konda
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwcut.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwcut.h"


/* TYPEDEFS AND MACROS */

/* When --copy-input is active but the required 'num_recs' records
 * have been printed, skStreamSkipRecords() is used to read data from
 * all remaining input streams.  This specifies the record-count
 * parameter to pass to that function. */
#define CUT_SKIP_COUNT 65536

/* EXPORTED VARIABLES */

/* The object to convert the record to text; includes pointer to the
 * file handle where the records are written */
rwAsciiStream_t *ascii_str = NULL;

/* handle input streams */
sk_options_ctx_t *optctx = NULL;

/* number records to print */
uint64_t num_recs = 0;

/* number of records to skip before printing */
uint64_t skip_recs = 0;

/* number of records to "tail" */
uint64_t tail_recs = 0;

/* buffer used for storing 'tail_recs' records */
rwRec *tail_buf = NULL;

/* how to handle IPv6 flows */
sk_ipv6policy_t ipv6_policy = SK_IPV6POLICY_MIX;


/* LOCAL VARIABLES */

/* current position in the 'tail_buf' */
static rwRec *tail_buf_cur;

/* whether we read more than 'tail_recs' records. 1==yes */
static int tail_buf_full = 0;


/* FUNCTION DEFINITIONS */

/*
 *  status = tailFile(stream);
 *
 *    Read SiLK flow records from the file at 'stream' and store the
 *    most recent 'tail_recs' number of records in the 'tail_buf'
 *    buffer.
 *
 *    Return -1 on read error, or 0 otherwise.
 */
static int
tailFile(
    skstream_t         *rwios)
{
    int rv = SKSTREAM_OK;

    while ((rv = skStreamReadRecord(rwios, tail_buf_cur)) == SKSTREAM_OK) {
        ++tail_buf_cur;
        if (tail_buf_cur == &tail_buf[tail_recs]) {
            tail_buf_cur = tail_buf;
            tail_buf_full = 1;
        }
    }
    if (SKSTREAM_ERR_EOF != rv) {
        skStreamPrintLastErr(rwios, rv, &skAppPrintErr);
        return -1;
    }

    return 0;
}


/*
 *  printTailBuffer();
 *
 *    Print the SiLK Flow records that are in the global 'tail_buf'
 *    buffer.
 */
static void
printTailBuffer(
    void)
{
    uint64_t avail_recs;

    /* determine number of records available to print; if 'tail_buf'
     * is not full, move 'tail_buf_cur' to the first record.  When the
     * buffer is full; 'tail_buf_cur' is already sitting on the first
     * record to print. */
    if (tail_buf_full) {
        avail_recs = tail_recs;
    } else {
        avail_recs = tail_buf_cur - tail_buf;
        tail_buf_cur = tail_buf;
    }

    /* determine number of records to print */
    if (0 == num_recs) {
        num_recs = avail_recs;
    } else if (avail_recs < num_recs) {
        num_recs = avail_recs;
    }

    rwAsciiPrintTitles(ascii_str);

    while (num_recs) {
        rwAsciiPrintRec(ascii_str, tail_buf_cur);
        --num_recs;
        ++tail_buf_cur;
        if (tail_buf_cur == &tail_buf[tail_recs]) {
            tail_buf_cur = tail_buf;
        }
    }
}


/*
 *  status = cutFile(stream);
 *
 *    Read SiLK flow records from the file at 'stream' and maybe print
 *    them according the values in 'skip_recs' and 'num_recs'.
 *
 *    Return -1 on error.  Return 1 if all requested records have been
 *    printed and processing should stop.  Return 0 if processing
 *    should continue.
 */
static int
cutFile(
    skstream_t         *rwios)
{
    static int copy_input_only = 0;
    rwRec rwrec;
    int rv = SKSTREAM_OK;
    size_t num_skipped;
    int ret_val = 0;

    /* handle case where all requested records have been printed, but
     * we need to write all records to the --copy-input stream. */
    if (copy_input_only) {
        while ((rv = skStreamSkipRecords(rwios, CUT_SKIP_COUNT, NULL))
               == SKSTREAM_OK)
            ;  /* empty */

        if (SKSTREAM_ERR_EOF != rv) {
            ret_val = -1;
        }
        goto END;
    }

    /* skip any leading records */
    if (skip_recs) {
        rv = skStreamSkipRecords(rwios, skip_recs, &num_skipped);
        switch (rv) {
          case SKSTREAM_OK:
            skip_recs -= num_skipped;
            break;
          case SKSTREAM_ERR_EOF:
            skip_recs -= num_skipped;
            goto END;
          default:
            ret_val = -1;
            goto END;
        }
    }

    if (0 == num_recs) {
        /* print all records */
        while ((rv = skStreamReadRecord(rwios, &rwrec)) == SKSTREAM_OK) {
            rwAsciiPrintRec(ascii_str, &rwrec);
        }
        if (SKSTREAM_ERR_EOF != rv) {
            ret_val = -1;
        }
    } else {
        while (num_recs
               && ((rv = skStreamReadRecord(rwios, &rwrec)) == SKSTREAM_OK))
        {
            rwAsciiPrintRec(ascii_str, &rwrec);
            --num_recs;
        }
        switch (rv) {
          case SKSTREAM_OK:
          case SKSTREAM_ERR_EOF:
            break;
          default:
            ret_val = -1;
            goto END;
        }
        if (0 == num_recs) {
            if (0 == skOptionsCtxCopyStreamIsActive(optctx)) {
                /* we're done */
                ret_val = 1;
            } else {
                /* send all remaining records to copy-input */
                copy_input_only = 1;
                while ((rv = skStreamSkipRecords(rwios, CUT_SKIP_COUNT, NULL))
                       == SKSTREAM_OK)
                    ;  /* empty */
                if (SKSTREAM_ERR_EOF != rv) {
                    ret_val = -1;
                }
            }
        }
    }

  END:
    if (-1 == ret_val) {
        skStreamPrintLastErr(rwios, rv, &skAppPrintErr);
    }
    return ret_val;
}


int main(int argc, char **argv)
{
    skstream_t *rwios;
    int rv = 0;

    appSetup(argc, argv);                 /* never returns on error */

    if (tail_buf) {
        assert(tail_recs);
        tail_buf_cur = tail_buf;

        /* Process the files from command line or stdin */
        while ((rv = skOptionsCtxNextSilkFile(optctx, &rwios, &skAppPrintErr))
               == 0)
        {
            skStreamSetIPv6Policy(rwios, ipv6_policy);
            rv = tailFile(rwios);
            skStreamDestroy(&rwios);
            if (-1 == rv) {
                exit(EXIT_FAILURE);
            }
        }
        if (rv < 0) {
            exit(EXIT_FAILURE);
        }
        printTailBuffer();
    } else {
        /* Process the files on command line or records from stdin */

        /* get first file */
        rv = skOptionsCtxNextSilkFile(optctx, &rwios, &skAppPrintErr);
        if (rv < 0) {
            exit(EXIT_FAILURE);
        }

        /* print title line */
        rwAsciiPrintTitles(ascii_str);

        if (1 == rv) {
            /* xargs with no input; we are done */
            appTeardown();
            return 0;
        }

        do {
            skStreamSetIPv6Policy(rwios, ipv6_policy);
            rv = cutFile(rwios);
            skStreamDestroy(&rwios);
            if (-1 == rv) {
                exit(EXIT_FAILURE);
            }
            if (1 == rv) {
                break;
            }
        } while ((rv = skOptionsCtxNextSilkFile(optctx, &rwios,&skAppPrintErr))
                 == 0);
        if (rv < 0) {
            exit(EXIT_FAILURE);
        }
    }

    /* done */
    appTeardown();

    return 0;
}

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
