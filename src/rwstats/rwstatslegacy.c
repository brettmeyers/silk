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
**  rwstatsutils.c
**
**  utility functions for the rwstats application.  See rwstats.c for
**  a full explanation.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwstatslegacy.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "rwstats.h"


/* OPTIONS SETUP */

typedef enum legacyOptions_en {
    LEGOPT_SIP, LEGOPT_DIP, LEGOPT_SPORT, LEGOPT_DPORT, LEGOPT_PROTOCOL,
    LEGOPT_ICMP,

    LEGOPT_FLOWS, LEGOPT_PACKETS, LEGOPT_BYTES
} legacyOptionsEnum;

static struct option legacyOptions[] = {
    {"sip",                    OPTIONAL_ARG, 0, LEGOPT_SIP},
    {"dip",                    OPTIONAL_ARG, 0, LEGOPT_DIP},
    {"sport",                  NO_ARG,       0, LEGOPT_SPORT},
    {"dport",                  NO_ARG,       0, LEGOPT_DPORT},
    {"protocol",               NO_ARG,       0, LEGOPT_PROTOCOL},
    {"icmp",                   NO_ARG,       0, LEGOPT_ICMP},

    {"flows",                  NO_ARG,       0, LEGOPT_FLOWS},
    {"packets",                NO_ARG,       0, LEGOPT_PACKETS},
    {"bytes",                  NO_ARG,       0, LEGOPT_BYTES},

    {0,0,0,0}                  /* sentinel entry */
};

static const char *legacyHelp[] = {
    ("Use: --fields=sip\n"
     "\tUse the source address as (part of) the key"),
    ("Use: --fields=dip\n"
     "\tUse the destination address as (part of) the key"),
    ("Use: --fields=sport\n"
     "\tUse the source port as (part of) the key"),
    ("Use: --fields=dport\n"
     "\tUse the destination port as (part of) the key"),
    ("Use: --fields=proto\n"
     "\tUse the protocol as the key"),
    ("Use: --fields=icmp\n"
     "\tUse the ICMP type and code as the key"),

    ("Use: --values=flows\n"
     "\tUse the flow count as the value"),
    ("Use: --values=packets\n"
     "\tUse the packet count as the value"),
    ("Use: --values=bytes\n"
     "\tUse the byte count as the value"),

    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int
legacyOptionsHandler(
    clientData          cData,
    int                 opt_index,
    char               *opt_arg);


/* FUNCTION DEFINITIONS */

/*
 *  legacyOptionsSetup(void);
 *
 *    Register the legacy options.
 */
int
legacyOptionsSetup(
    clientData          cData)
{
    assert((sizeof(legacyHelp)/sizeof(char *)) ==
           (sizeof(legacyOptions)/sizeof(struct option)));

    /* register the options */
    if (skOptionsRegister(legacyOptions, &legacyOptionsHandler, cData))
    {
        skAppPrintErr("Unable to register legacy options");
        return 1;
    }

    return 0;
}


/*
 *  legacyOptionsUsage(fh);
 *
 *    Print the usage information for the legacy options to the named
 *    file handle.
 */
void
legacyOptionsUsage(
    FILE               *fh)
{
    int i;

    fprintf(fh, "\nLEGACY SWITCHES:\n");
    for (i = 0; legacyOptions[i].name; i++ ) {
        fprintf(fh, "--%s %s. %s\n", legacyOptions[i].name,
                SK_OPTION_HAS_ARG(legacyOptions[i]), legacyHelp[i]);
    }
}


/*
 *  status = legacyOptionsHandler(cData, opt_index, opt_arg);
 *
 *    Process the legacy versions of the switches by calling the real
 *    appOptionsHandler().
 */
static int
legacyOptionsHandler(
    clientData          cData,
    int                 opt_index,
    char               *opt_arg)
{
#define KEY_COMBO_ERR(a, b)                                             \

    static int old_id = -1;
    rwstats_legacy_t *leg = (rwstats_legacy_t*)cData;
    const char *val_type = NULL;
    uint32_t val;
    int rv;

    switch ((legacyOptionsEnum)opt_index) {
      case LEGOPT_SIP:
        if (opt_arg) {
            rv = skStringParseUint32(&val, opt_arg, 1, 31);
            if (rv) {
                goto PARSE_ERROR;
            }
            cidr_sip = ~0 << (32 - val);
        }
        break;

      case LEGOPT_DIP:
        if (opt_arg) {
            rv = skStringParseUint32(&val, opt_arg, 1, 31);
            if (rv) {
                goto PARSE_ERROR;
            }
            cidr_dip = ~0 << (32 - val);
        }
        break;

      case LEGOPT_SPORT:
      case LEGOPT_DPORT:
      case LEGOPT_PROTOCOL:
      case LEGOPT_ICMP:
        break;

      case LEGOPT_FLOWS:
        val_type = "Records";
        break;

      case LEGOPT_PACKETS:
      case LEGOPT_BYTES:
        val_type = legacyOptions[opt_index].name;
        break;
    }

    if (opt_index <= LEGOPT_ICMP) {
        if (NULL == leg->fields) {
            old_id = opt_index;
            leg->fields = legacyOptions[opt_index].name;
        } else if (((1 << LEGOPT_SIP) | (1 << LEGOPT_DIP))
                   == ((1 << old_id) | (1 << opt_index)))
        {
            leg->fields = "sip,dip";
        } else if (((1 << LEGOPT_SPORT) | (1 << LEGOPT_DPORT))
                   == ((1 << old_id) | (1 << opt_index)))
        {
            leg->fields = "sport,dport";
        } else {
            skAppPrintErr(("Key combination --%s and --%s is not supported.\n"
                           "\tUse the --fields switch for this combination"),
                          legacyOptions[opt_index].name,
                          legacyOptions[old_id].name);
            return 1;
        }
    } else {
        assert(val_type != NULL);
        if (leg->values) {
            skAppPrintErr(("May only specify one of --%s, --%s or --%s.\n"
                           "Use the --values switch for multiple values"),
                          legacyOptions[LEGOPT_FLOWS].name,
                          legacyOptions[LEGOPT_PACKETS].name,
                          legacyOptions[LEGOPT_BYTES].name);
            return 1;
        }
        leg->values = val_type;
    }

    return 0;                   /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  legacyOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
