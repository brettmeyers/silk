/*
** Copyright (C) 2009-2014 by Carnegie Mellon University.
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
**  uniq-distproto.c
**
**    A plug-in to be loaded by rwuniq to count the number of distinct
**    protocols per key.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: uniq-distproto.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skplugin.h>
#include <silk/rwrec.h>


/* DEFINES AND TYPEDEFS */

/* Plugin protocol version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0

/* Width of text field */
#define FIELD_WIDTH 3

/* Number of uint8_t's for a bitmaps of protocols */
#define BITMAP_SIZE  32         /* 256/8 */


/* LOCAL VARIABLES */

static const uint8_t BitsSetTable256[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};


/* FUNCTION DECLARATIONS */

static skplugin_err_t
recAddToMap(
    const rwRec        *rwrec,
    uint8_t            *bin_value,
    void               *idx,
    void              **extra);
static skplugin_err_t
mapToText(
    const uint8_t      *bin_value,
    char               *text_value,
    size_t              text_size,
    void               *idx);
static skplugin_err_t
mergeMaps(
    uint8_t            *dst_bin,
    const uint8_t      *src_bin,
    void               *idx);
static skplugin_err_t
compareMaps(
    int                *cmp,
    const uint8_t      *bin_a,
    const uint8_t      *bin_b,
    void               *idx);


/* FUNCTION DEFINITIONS */

/* the registration function called by skplugin.c */
skplugin_err_t
SKPLUGIN_SETUP_FN(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*pi_data))
{
    uint8_t init_value[BITMAP_SIZE];
    skplugin_field_t *field;
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

    memset(&init_value, 0, sizeof(init_value));

    /* register the new aggregate to use with rwuniq */
    memset(&regdata, 0, sizeof(regdata));
    regdata.column_width = FIELD_WIDTH;
    regdata.bin_bytes = sizeof(init_value);
    regdata.add_rec_to_bin = recAddToMap;
    regdata.bin_to_text = mapToText;
    regdata.bin_merge = mergeMaps;
    regdata.bin_compare = compareMaps;
    regdata.initial = init_value;
    return skpinRegField(&field, "proto-distinct", NULL, &regdata, NULL);
}


/*
 *  status = recAddToMap(rwrec, bin_val, &v_unused, NULL);
 *
 *    Given the SiLK Flow record 'rwrec' and an existing binary value
 *    'bin_val', add the protocol from 'rwrec' to 'bin_val'.
 */
static skplugin_err_t
recAddToMap(
    const rwRec            *rwrec,
    uint8_t                *map_value,
    void            UNUSED(*v_unused),
    void           UNUSED(**extra))
{
    uint8_t proto = rwRecGetProto(rwrec);

    map_value[proto >> 3] |= 1 << (proto & 0x7);
    return SKPLUGIN_OK;
}


/*
 *  status = mapToText(bin_value, text_val, text_len, &v_unused);
 *
 *    Given the binary value 'bin_value' created by calls to
 *    'recAddToMap()', fill 'text_val', a buffer of 'text_len'
 *    characters, with a textual representation of the binary
 *    value---in this case, the number of bits that are set in
 *    'bin_value'.
 */
static skplugin_err_t
mapToText(
    const uint8_t          *bin_value,
    char                   *text_value,
    size_t                  text_size,
    void            UNUSED(*v_unused))
{
    int i;
    int c = 0;

    for (i = 0; i < BITMAP_SIZE; ++i) {
        c += BitsSetTable256[bin_value[i]];
    }

    snprintf(text_value, text_size, "%3d", c);
    return SKPLUGIN_OK;
}


/*
 *  status = mergeMaps(dst_bin, src_bin, &v_unused);
 *
 *    Given two binary values 'dst_bin' and 'src_bin' created by calls
 *    to 'recAddToMap()', fill 'dst_bin', with the result of adding
 *    the two binary values---in this case, the result of performing a
 *    bitwise-OR on the values.
 */
static skplugin_err_t
mergeMaps(
    uint8_t                *dst_bin,
    const uint8_t          *src_bin,
    void            UNUSED(*v_unused))
{
    int i;

    for (i = 0; i < BITMAP_SIZE; ++i) {
        dst_bin[i] |= src_bin[i];
    }
    return SKPLUGIN_OK;
}


/*
 *  status = compareMaps(cmp, bin_a, bin_b, &v_unused);
 *
 *    Given two binary values 'bin_a' and 'bin_a' created by calls to
 *    'recAddToMap()', fill 'cmp', with the result of comparing the
 *    two binary values---in this case, the result of counting the
 *    number of bits set in each bitmap.
 */
static skplugin_err_t
compareMaps(
    int                    *cmp,
    const uint8_t          *bin_a,
    const uint8_t          *bin_b,
    void            UNUSED(*v_unused))
{
    int i;

    /* compute A-B to get correct comparison value. */
    *cmp = 0;
    for (i = 0; i < BITMAP_SIZE; ++i) {
        *cmp += ((int)(BitsSetTable256[bin_a[i]])
                 - (int)(BitsSetTable256[bin_b[i]]));
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
