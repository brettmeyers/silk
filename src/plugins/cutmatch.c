/*
** Copyright (C) 2008-2014 by Carnegie Mellon University.
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
**  cutmatch.c
**
**    A plug-in to be loaded by rwcut to display the values that
**    rwmatch encodes in the NextHopIP field.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: cutmatch.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skplugin.h>
#include <silk/rwrec.h>


/* DEFINES AND TYPEDEFS */

#define FIELD_WIDTH 10

/* Plugin protocol version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0


/* FUNCTION DECLARATIONS */

static skplugin_err_t
recToText(
    const rwRec            *rwrec,
    char                   *text_value,
    size_t                  text_size,
    void            UNUSED(*idx),
    void           UNUSED(**extra));


/* FUNCTION DEFINITIONS */

/* the registration function called by skplugin.c */
skplugin_err_t
SKPLUGIN_SETUP_FN(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*pi_data))
{
    skplugin_field_t *field = NULL;
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

    /* register the field to use for rwcut */
    memset(&regdata, 0, sizeof(regdata));
    regdata.column_width = FIELD_WIDTH;
    regdata.rec_to_text  = recToText;
    rv = skpinRegField(&field, "match", NULL, &regdata, NULL);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    if (field) {
        rv = skpinSetFieldTitle(field, "<->Match#");
    }

    return rv;
}


/*
 *  status = recToText(rwrec, text_val, text_len, &index);
 *
 *    Given the SiLK Flow record 'rwrec', fill 'text_val', a buffer of
 *    'text_len' characters, with the appropriate direction and match
 *    number.
 */
static skplugin_err_t
recToText(
    const rwRec            *rwrec,
    char                   *text_value,
    size_t                  text_size,
    void            UNUSED(*idx),
    void           UNUSED(**extra))
{
    static const char *match_dir[] = {"->", "<-"};
    uint32_t nhip;
    uint32_t match_count;

    nhip = rwRecGetNhIPv4(rwrec);
    match_count = 0x00FFFFFF & nhip;
    if (match_count) {
        snprintf(text_value, text_size, ("%s%8" PRIu32),
                 match_dir[(nhip & 0xFF000000) ? 1 : 0],
                 match_count);
    } else {
        snprintf(text_value, text_size, "%-10s",
                 match_dir[(nhip & 0xFF000000) ? 1 : 0]);
    }

    return SKPLUGIN_OK;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
