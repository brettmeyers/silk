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

/*
**  rwcombine.h
**
**    Common declarations needed by rwcombine.  See rwcombine.c for
**    implementation details.
*/
#ifndef _RWCOMBINE_H
#define _RWCOMBINE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWCOMBINE_H, "$SiLK: rwcombine.h 6dc464c27dd9 2014-09-23 13:15:24Z mthomas $");

#include <silk/rwascii.h>
#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/skstream.h>
#include <silk/sktempfile.h>
#include <silk/utils.h>

/* use TRACEMSG_LEVEL as our tracing variable */
#define TRACEMSG(msg) TRACEMSG_TO_TRACEMSGLVL(1, msg)
#include <silk/sktracemsg.h>


/* LOCAL DEFINES AND TYPEDEFS */

/*
 *    The largest buffer we attempt to use, unless the user selects a
 *    different value with the --buffer-size switch.
 *
 *    Support of a buffer of almost 2GB.
 */
#define DEFAULT_BUFFER_SIZE   0x78000000

/*
 *    We do not allocate the buffer at once, but use realloc() to grow
 *    the buffer linearly to the maximum size.  The following is the
 *    number of steps to take to reach the maximum size.  The number
 *    of realloc() calls will be once less than this value.
 *
 *    If the initial allocation fails, the number of chunks is
 *    incremented---making the size of the initial malloc()
 *    smaller---and allocation is attempted again.
 */
#define NUM_CHUNKS  6

/*
 *    If we can't allocate space for at least this many records, give
 *    up.
 */
#define MIN_IN_CORE_RECORDS     1000

/*
 *    Maximum number of files to attempt to merge-sort at once.
 */
#define MAX_MERGE_FILES 1024

/*
 *    Size of a node is constant: the size of a complete rwRec.
 */
#define NODE_SIZE       ((uint32_t)sizeof(rwRec))



/* VARIABLES */

/* number of fields to sort over */
extern uint32_t num_fields;

/* IDs of the fields to sort over; values are from the
 * rwrec_printable_fields_t enum. */
extern uint32_t sort_fields[RWREC_PRINTABLE_FIELD_COUNT];

/* output stream */
extern skstream_t *out_stream;

/* statistics stream */
extern skstream_t *print_statistics;

/* temp file context */
extern sk_tempfilectx_t *tmpctx;

/* maximum amount of RAM to attempt to allocate */
extern uint64_t buffer_size;

/* maximum amount of idle time to allow between flows */
extern int64_t max_idle_time;


/* FUNCTIONS */

void
appExit(
    int                 status);
void
appSetup(
    int                 argc,
    char              **argv);
int
appNextInput(
    skstream_t        **stream);


#ifdef __cplusplus
}
#endif
#endif /* _RWCOMBINE_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
