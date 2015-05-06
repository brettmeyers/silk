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

/*
**  rwp2f_minbytes.c
**
**    An example of a simple plug-in that can be used with rwptoflow.
**
**  Mark Thomas
**  September 2006
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwp2f_minbytes.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/utils.h>
#include <silk/skplugin.h>
#include <silk/skstream.h>
#include "rwppacketheaders.h"

/* Plugin protocol version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0


/* LOCAL VARIABLE DEFINITIONS */

/*
 * rwptoflow hands the packet to the plugin as an "extra argument".
 * rwptoflow and its plugins must agree on the name of this argument.
 * The extra argument is specified in a NULL-terminated array of
 * argument names defined in rwppacketheaders.h.
 */
static const char *plugin_extra_args[] = RWP2F_EXTRA_ARGUMENTS;

/* the minimum number of bytes a packet must have to pass, as entered
 * by the user */
static uint32_t byte_limit = 0;


/* OPTIONS SETUP */

typedef enum {
    OPT_BYTE_LIMIT
} plugin_options_enum;

static struct option plugin_options[] = {
    {"byte-limit",      REQUIRED_ARG, 0, OPT_BYTE_LIMIT},
    {0,0,0,0}           /* sentinel */
};

static const char *plugin_help[] = {
    ("Reject the packet if its length (hdr+payload) is less\n"
     "\tthan this value"),
    (char*)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static skplugin_err_t optionsHandler(const char *opt_arg, void *cbdata);
static skplugin_err_t
p2f_minbytes(
    rwRec              *rwrec,
    void               *cbdata,
    void              **extra_args);


/* FUNCTION DEFINITIONS */

/* the registration function called by skplugin.c */
skplugin_err_t
SKPLUGIN_SETUP_FN(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*pi_data))
{
    int i;
    skplugin_err_t rv;

    /* Check API version */
    rv = skpinSimpleCheckVersion(major_version, minor_version,
                                 PLUGIN_API_VERSION_MAJOR,
                                 PLUGIN_API_VERSION_MINOR,
                                 skAppPrintErr);
    if (rv != SKPLUGIN_OK) {
        return rv;
    }

    assert((sizeof(plugin_options)/sizeof(struct option))
           == (sizeof(plugin_help)/sizeof(char*)));

    /* register the options to use for rwptoflow.  when the option is
     * given, we will call skpinRegTransform() to register the
     * transformation function. */
    for (i = 0; plugin_options[i].name; ++i) {
        rv = skpinRegOption2(plugin_options[i].name, plugin_options[i].has_arg,
                             plugin_help[i], NULL, &optionsHandler,
                             (void*)&plugin_options[i].val,
                             1, SKPLUGIN_APP_TRANSFORM);
        if (SKPLUGIN_OK != rv && SKPLUGIN_ERR_DID_NOT_REGISTER != rv) {
            return rv;
        }
    }

    return SKPLUGIN_OK;
}


/*
 *  status = optionsHandler(opt_arg, &index);
 *
 *    Handles options for the plugin.  'opt_arg' is the argument, or
 *    NULL if no argument was given.  'index' is the enum passed in
 *    when the option was registered.
 *
 *    Returns SKPLUGIN_OK on success, or SKPLUGIN_ERR if there was a
 *    problem.
 */
static skplugin_err_t
optionsHandler(
    const char         *opt_arg,
    void               *cbdata)
{
    plugin_options_enum opt_index = *((plugin_options_enum*)cbdata);
    skplugin_callbacks_t regdata;
    int rv;

    switch (opt_index) {
      case OPT_BYTE_LIMIT:
        rv = skStringParseUint32(&byte_limit, opt_arg, 0, 0);
        if (rv) {
            skAppPrintErr("Invalid %s '%s': %s",
                          plugin_options[opt_index].name, opt_arg,
                          skStringParseStrerror(rv));
            return SKPLUGIN_ERR;
        }
        break;
    }

    /* register the transform function */
    memset(&regdata, 0, sizeof(regdata));
    regdata.transform = p2f_minbytes;
    regdata.extra = plugin_extra_args;
    return skpinRegTransformer(NULL, &regdata, NULL);
}


skplugin_err_t
p2f_minbytes(
    rwRec       UNUSED(*rwrec),
    void        UNUSED(*cbdata),
    void              **extra_args)
{
    sk_pktsrc_t *pktsrc = (sk_pktsrc_t*)extra_args[0];
    ip_header_t *iph;

    iph = (ip_header_t*)(pktsrc->pcap_data + sizeof(eth_header_t));
    if (ntohs(iph->tlen) < byte_limit) {
        return SKPLUGIN_FILTER_FAIL;
    }

    return SKPLUGIN_FILTER_PASS;
}




/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
