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
**  rwsynackfin.c
**
**    Pass web traffic and fail all other traffic.  For web traffic,
**    keep a count of the number/types of flags seen, and print
**    summary to stderr once processing is complete.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwsynackfin.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skplugin.h>
#include <silk/rwrec.h>


/* DEFINES AND TYPEDEFS */

/* Plugin protocol version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0


/* LOCAL VARIABLES */

static uint32_t fin_count, ack_count, rst_count, syn_count;


/* FUNCTION DECLARATIONS */

static skplugin_err_t check(const rwRec *rwrec, void *cbdata, void **extra);
static skplugin_err_t summary(void *cbdata);


/* FUNCTION DEFINITIONS */

/* the registration function called by skplugin.c */
skplugin_err_t
SKPLUGIN_SETUP_FN(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*pi_data))
{
    skplugin_err_t rv;
    skplugin_callbacks_t regdata;

    /* Check API version */
    rv = skpinSimpleCheckVersion(major_version, minor_version,
                                 PLUGIN_API_VERSION_MAJOR,
                                 PLUGIN_API_VERSION_MINOR,
                                 skAppPrintErr);
    if (rv != SKPLUGIN_OK) {
        return rv;
    }

    syn_count = fin_count = ack_count = rst_count = 0;

    /* Register the function to use for filtering */
    memset(&regdata, 0, sizeof(regdata));
    regdata.cleanup = summary;
    regdata.filter = check;
    return skpinRegFilter(NULL, &regdata, NULL);
}


/*
 *  status = check(rwrec, data, NULL);
 *
 *    Check whether 'rwrec' passes the filter.  Return
 *    SKPLUGIN_FILTER_PASS if it does; SKPLUGIN_FILTER_FAIL otherwise.
 *
 *    Pass the filter if 80/tcp or 443/tcp
 */
static skplugin_err_t
check(
    const rwRec            *rwrec,
    void            UNUSED(*cbdata),
    void           UNUSED(**extra))
{
    if (rwRecGetProto(rwrec) != 6) {
        return SKPLUGIN_FILTER_FAIL;
    }
    if (rwRecGetDPort(rwrec) != 80 && rwRecGetDPort(rwrec) != 443) {
        return SKPLUGIN_FILTER_FAIL;
    }

    if (rwRecGetFlags(rwrec) == ACK_FLAG
        && rwRecGetPkts(rwrec) == 1
        && rwRecGetBytes(rwrec) == 40)
    {
        ack_count++;
        return SKPLUGIN_FILTER_PASS;
    }

    if (rwRecGetFlags(rwrec) & FIN_FLAG) {
        fin_count++;
    }
    if (rwRecGetFlags(rwrec) & RST_FLAG) {
        rst_count++;
    }
    if (rwRecGetFlags(rwrec) & SYN_FLAG) {
        syn_count++;
    }
    return SKPLUGIN_FILTER_PASS;
}


/*
 *  status = summary(cbdata);
 *
 *    Print a summary of the flows we've seen to stderr.
 */
static skplugin_err_t
summary(
    void        UNUSED(*cbdata))
{
    fprintf(stderr, ("WEB SYN %" PRIu32 "  FIN %" PRIu32
                     "  RST %" PRIu32 "  ACK %" PRIu32 "\n"),
            syn_count, fin_count, rst_count, ack_count);

    return SKPLUGIN_OK;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
