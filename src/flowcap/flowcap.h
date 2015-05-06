/*
** Copyright (C) 2004-2015 by Carnegie Mellon University.
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
#ifndef _FLOWCAP_H
#define _FLOWCAP_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_FLOWCAP_H, "$SiLK: flowcap.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/*
**  flowcap.h
**
**  Common information between flowcap objects
**
**/

#include <silk/probeconf.h>
#include <silk/skdaemon.h>
#include <silk/sklog.h>
#include <silk/skstream.h>
#include <silk/skvector.h>
#include <silk/utils.h>


/* Max timestamp length (YYYYMMDDhhmmss) */
#define FC_TIMESTAMP_MAX 15
/* Maximum sensor size (including either trailing zero or preceeding hyphen) */
#define FC_SENSOR_MAX (SK_MAX_STRLEN_SENSOR + 1)
/* Maximum probe size (including either trailing zero or preceeding hyphen) */
#define FC_PROBE_MAX (SK_MAX_STRLEN_SENSOR + 1)
/* Size of uniquness extension */
#define FC_UNIQUE_MAX 7
/* Previous two, plus hyphen */
#define FC_NAME_MAX                                     \
    (FC_TIMESTAMP_MAX + FC_SENSOR_MAX +                 \
     FC_PROBE_MAX + FC_UNIQUE_MAX)


/* Minimum flowcap version */
/* We no longer support flowcap version 1 */
#define FC_VERSION_MIN 2

/* Maximum flowcap version */
#define FC_VERSION_MAX 5

/* Default version of flowcap to produce */
#define FC_VERSION_DEFAULT 5

/* minimum number of bytes to leave free on the data disk.  File
 * distribution will stop when the freespace on the disk reaches or
 * falls below this mark.  This value is parsed by
 * skStringParseHumanUint64(). */
#define DEFAULT_FREESPACE_MINIMUM   "1g"

/* maximum percentage of disk space to take */
#define DEFAULT_SPACE_MAXIMUM_PERCENT  ((double)98.00)


/* Where to write files */
extern const char *destination_dir;

/* Compression method for output files */
extern sk_compmethod_t comp_method;

/* The version of flowcap to produce */
extern uint8_t flowcap_version;

/* To ensure records are sent along in a timely manner, the files are
 * closed when a timer fires or once they get to a certain size.
 * These variables define those values. */
extern uint32_t write_timeout;
extern uint32_t max_file_size;

/* Timer base (0 if none) from which we calculate timeouts */
extern sktime_t clock_time;

/* Amount of disk space to allow for a new file when determining
 * whether there is disk space available. */
extern uint64_t alloc_file_size;

/* Probes the user wants to flowcap process */
extern sk_vector_t *probe_vec;

#ifdef SK_HAVE_STATVFS
/* leave at least this much free space on the disk; specified by
 * --freespace-minimum */
extern int64_t freespace_minimum;

/* take no more that this amount of the disk; as a percentage.
 * specified by --space-maximum-percent */
extern double space_maximum_percent;
#endif /* SK_HAVE_STATVFS */


void
appSetup(
    int                 argc,
    char              **argv);
void
appTeardown(
    void);

int
createReaders(
    void);

#ifdef __cplusplus
}
#endif
#endif /* _FLOWCAP_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
