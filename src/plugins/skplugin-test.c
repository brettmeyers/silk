/*
** Copyright (C) 2010-2015 by Carnegie Mellon University.
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
**  Tests for the simplified skplugin registration functions.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skplugin-test.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skplugin.h>
#include <silk/utils.h>
#include <silk/rwrec.h>


/* DEFINES AND TYPEDEFS */

/*
 *    These variables specify the version of the SiLK plug-in API.
 *    They are used in the call to skpinSimpleCheckVersion() below.
 *    See the description of that function for their meaning.
 */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0

/* LOCAL VARIABLES */

static const char *test_labels[] =
    {
        "Low",
        "Medium",
        "High",
        NULL
    };

/* LOCAL FUNCTION PROTOTYPES */

static uint64_t
test_bytes(
    const rwRec        *rec);
static uint32_t
test_sipv4(
    const rwRec        *rec);
static void
test_sip(
    skipaddr_t         *dest,
    const rwRec        *rec);
static void
test_text(
    char               *dest,
    size_t              width,
    uint64_t            val);
static uint64_t
test_list(
    const rwRec        *rec);
static uint64_t
test_weird(
    uint64_t            current,
    uint64_t            operand);

/* FUNCTION DEFINITIONS */

/*
 *    This is the registration function.
 *
 *    When you provide "--plugin=my-plugin.so" on the command line to
 *    an application, the application calls this function to determine
 *    the new switches and/or fields that "my-plugin" provides.
 *
 *    This function is called with three arguments: the first two
 *    describe the version of the plug-in API, and the third is a
 *    pointer that is currently unused.
 */
skplugin_err_t
SKPLUGIN_SETUP_FN(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*plug_in_data))
{
    skplugin_err_t rv;

    /* Check the plug-in API version */
    rv = skpinSimpleCheckVersion(major_version, minor_version,
                                 PLUGIN_API_VERSION_MAJOR,
                                 PLUGIN_API_VERSION_MINOR,
                                 skAppPrintErr);
    if (rv != SKPLUGIN_OK) {
        return rv;
    }

    rv = skpinRegIntField("copy-bytes", 0, UINT32_MAX, test_bytes, 0);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    rv = skpinRegIPv4Field("copy-sipv4", test_sipv4, 0);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    rv = skpinRegIPAddressField("copy-sip", test_sip, 0);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    rv = skpinRegTextField("text-bytes", 0, UINT32_MAX, test_bytes,
                           test_text, 20);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    rv = skpinRegStringListField("quant-bytes", test_labels, 0,
                                 "Too many", test_list, 0);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    rv = skpinRegIntSumAggregator("sum-bytes", 0, test_bytes, 0);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    rv = skpinRegIntMinAggregator("min-bytes", 0, test_bytes, 0);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    rv = skpinRegIntMaxAggregator("max-bytes", 0, test_bytes, 0);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }
    rv = skpinRegIntAggregator("weird-bytes", UINT32_MAX, test_bytes,
                               test_weird, 0, 0);
    if (SKPLUGIN_OK != rv) {
        return rv;
    }

    return SKPLUGIN_OK;
}

static uint64_t
test_bytes(
    const rwRec        *rec)
{
    return rwRecGetBytes(rec);
}

static uint32_t
test_sipv4(
    const rwRec        *rec)
{
    return rwRecGetSIPv4(rec);
}

static void
test_sip(
    skipaddr_t         *dest,
    const rwRec        *rec)
{
    rwRecMemGetSIP(rec, dest);
}

static void
test_text(
    char               *dest,
    size_t              width,
    uint64_t            val)
{
    snprintf(dest, width, "Byte count %" PRIu64, val);
    dest[width - 1] = '\0';
}

static uint64_t
test_list(
    const rwRec        *rec)
{
    uint32_t bytes = rwRecGetBytes(rec);

    if (bytes < 100) {
        return 0;
    } else if (bytes < 150) {
        return 1;
    } else if (bytes < 200) {
        return 2;
    }
    return 3;
}

static uint64_t
test_weird(
    uint64_t            current,
    uint64_t            operand)
{
    if (current > operand) {
        return (current - operand) / 2;
    }
    return (operand - current) / 2;
}

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
