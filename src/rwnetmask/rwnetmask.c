/*
** Copyright (C) 2004-2014 by Carnegie Mellon University.
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
**  rwnetmask
**
**  Read in SiLK Flow records and write out SiLK Flow records, masking
**  the Source IP and Destination IP by the prefix-lengths given on
**  the command line.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwnetmask.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* TYPEDEFS AND DEFINES */

/* File handle for --help output */
#define USAGE_FH stdout

/* Number of prefixes supported: sip, dip, nhip */
#define PREFIX_COUNT 3

enum {
    SIP_MASK, DIP_MASK, NHIP_MASK,
    /* next must be last */
    _FINAL_MASK_
};


/* LOCAL VARIABLES */

/* The masks of source/dest/next-hop IP for IPv4 and IPv6, and the
 * number of bits in each mask. */
static struct net_mask_st {
    uint8_t     mask6[16];
    uint32_t    mask4;
    uint8_t     bits6;
    uint8_t     bits4;
} net_mask[PREFIX_COUNT];

/* support for looping over input files */
static sk_options_ctx_t *optctx = NULL;

/* Where to write the output */
static const char *output_path = NULL;

/* the compression method to use when writing the file.
 * sksiteCompmethodOptionsRegister() will set this to the default or
 * to the value the user specifies. */
static sk_compmethod_t comp_method;

/* how to handle IPv6 flows */
static sk_ipv6policy_t ipv6_policy = SK_IPV6POLICY_MIX;


/* OPTIONS */

typedef enum {
    OPT_4SIP_PREFIX_LENGTH, OPT_4DIP_PREFIX_LENGTH, OPT_4NHIP_PREFIX_LENGTH,
#if SK_ENABLE_IPV6
    OPT_6SIP_PREFIX_LENGTH, OPT_6DIP_PREFIX_LENGTH, OPT_6NHIP_PREFIX_LENGTH,
#endif
    OPT_OUTPUT_PATH
}  appOptionsEnum;

static struct option appOptions[] = {
    {"4sip-prefix-length",  REQUIRED_ARG, 0, OPT_4SIP_PREFIX_LENGTH},
    {"4dip-prefix-length",  REQUIRED_ARG, 0, OPT_4DIP_PREFIX_LENGTH},
    {"4nhip-prefix-length", REQUIRED_ARG, 0, OPT_4NHIP_PREFIX_LENGTH},
#if SK_ENABLE_IPV6
    {"6sip-prefix-length",  REQUIRED_ARG, 0, OPT_6SIP_PREFIX_LENGTH},
    {"6dip-prefix-length",  REQUIRED_ARG, 0, OPT_6DIP_PREFIX_LENGTH},
    {"6nhip-prefix-length", REQUIRED_ARG, 0, OPT_6NHIP_PREFIX_LENGTH},
#endif
    {"output-path",         REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {0,0,0,0}               /* sentinel entry */
};

static const char *appHelp[] = {
    /* add help strings here for the applications options */
    "High bits of source IPv4 to keep. Def 32",
    "High bits of destination IPv4 to keep. Def 32",
    "High bits of next-hop IPv4 to keep. Def 32",
#if SK_ENABLE_IPV6
    "High bits of source IPv6 to keep. Def 128",
    "High bits of destination IPv6 to keep. Def 128",
    "High bits of next-hop IPv6 to keep. Def 128",
#endif
    "Write output to given file path. Def. stdout",
    (char *)NULL
};

/* for compatibility */
static struct option legacyOptions[] = {
    {"sip-prefix-length",           REQUIRED_ARG, 0, OPT_4SIP_PREFIX_LENGTH},
    {"source-prefix-length",        REQUIRED_ARG, 0, OPT_4SIP_PREFIX_LENGTH},
    {"dip-prefix-length",           REQUIRED_ARG, 0, OPT_4DIP_PREFIX_LENGTH},
    {"destination-prefix-length",   REQUIRED_ARG, 0, OPT_4DIP_PREFIX_LENGTH},
    {"d",                           REQUIRED_ARG, 0, OPT_4DIP_PREFIX_LENGTH},
    {"nhip-prefix-length",          REQUIRED_ARG, 0, OPT_4NHIP_PREFIX_LENGTH},
    {"next-hop-prefix-length",      REQUIRED_ARG, 0, OPT_4NHIP_PREFIX_LENGTH},
    {0,0,0,0}                       /* sentinel entry */
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);


/* FUNCTION DEFINITIONS */

/*
 *  appUsageLong();
 *
 *    Print complete usage information to USAGE_FH.  Pass this
 *    function to skOptionsSetUsageCallback(); skOptionsParse() will
 *    call this funciton and then exit the program when the --help
 *    option is given.
 */
static void
appUsageLong(
    void)
{
#define USAGE_MSG                                                          \
    ("<PREFIX_SWITCH> [<PREFIX_SWITCH>...] [SWITCHES] [FILES]\n"           \
     "\tRead SiLK Flow records from FILES named on the command line or\n"  \
     "\tfrom the standard input, keep the specified number of most\n"      \
     "\tsignificant bits for each IP address, and write the modified\n"    \
     "\trecords to the specified output file or to the standard output.\n")

    FILE *fh = USAGE_FH;
    int i, j;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);

    fprintf(fh, "\nPREFIX SWITCHES:\n");
    /* print everything before --output-path */
    for (i=0; appOptions[i].name && appOptions[i].val < OPT_OUTPUT_PATH; ++i) {
        fprintf(fh, "--%s %s. %s\n", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]), appHelp[i]);
    }

    /* print the first three switches again as aliases---use a
     * different variable!! */
    for (j = 0;
         appOptions[j].name && appOptions[j].val <= OPT_4NHIP_PREFIX_LENGTH;
         ++j)
    {
        fprintf(fh, "--%s %s. Alias for --%s\n", appOptions[j].name + 1,
                SK_OPTION_HAS_ARG(appOptions[j]), appOptions[j].name);
    }

    /* print remaining options */
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    for ( ; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. %s\n", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]), appHelp[i]);
    }

    skOptionsCtxOptionsUsage(optctx, fh);
    skIPv6PolicyUsage(fh);
    sksiteCompmethodOptionsUsage(fh);
    skOptionsNotesUsage(fh);
    sksiteOptionsUsage(fh);
}


/*
 *  appTeardown()
 *
 *    Teardown all modules, close all files, and tidy up all
 *    application state.
 *
 *    This function is idempotent.
 */
static void
appTeardown(
    void)
{
    static int teardownFlag = 0;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    skOptionsNotesTeardown();
    skOptionsCtxDestroy(&optctx);
    skAppUnregister();
}


/*
 *  appSetup(argc, argv);
 *
 *    Perform all the setup for this application include setting up
 *    required modules, parsing options, etc.  This function should be
 *    passed the same arguments that were passed into main().
 *
 *    Returns to the caller if all setup succeeds.  If anything fails,
 *    this function will cause the application to exit with a FAILURE
 *    exit status.
 */
static void
appSetup(
    int                 argc,
    char              **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);
    int optctx_flags;
    int rv;
    int i;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    assert(PREFIX_COUNT == _FINAL_MASK_);

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize variables */
    memset(net_mask, 0, sizeof(net_mask));

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS | SK_OPTIONS_CTX_PRINT_FILENAMES);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsRegister(legacyOptions, &appOptionsHandler, NULL)
        || skIPv6PolicyOptionsRegister(&ipv6_policy)
        || skOptionsNotesRegister(NULL)
        || sksiteCompmethodOptionsRegister(&comp_method)
        || sksiteOptionsRegister(SK_SITE_FLAG_CONFIG_FILE))
    {
        skAppPrintErr("Unable to register options");
        exit(EXIT_FAILURE);
    }

    /* register the teardown handler */
    if (atexit(appTeardown) < 0) {
        skAppPrintErr("Unable to register appTeardown() with atexit()");
        appTeardown();
        exit(EXIT_FAILURE);
    }

    /* parse options */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();           /* never returns */
    }

    /* make certain at least one mask was specified */
    for (i = 0; i < PREFIX_COUNT; ++i) {
        if (net_mask[i].bits6 || net_mask[i].bits4) {
            break;
        }
    }
    if (i == PREFIX_COUNT) {
        skAppPrintErr("Must specify at least one prefix length option");
        skAppUsage();
    }

    /* check the output */
    if (output_path == NULL) {
        output_path = "-";
    }
}


/*
 *  status = appOptionsHandler(cData, opt_index, opt_arg);
 *
 *    This function is passed to skOptionsRegister(); it will be called
 *    by skOptionsParse() for each user-specified switch that the
 *    application has registered; it should handle the switch as
 *    required---typically by setting global variables---and return 1
 *    if the switch processing failed or 0 if it succeeded.  Returning
 *    a non-zero from from the handler causes skOptionsParse() to return
 *    a negative value.
 *
 *    The clientData in 'cData' is typically ignored; 'opt_index' is
 *    the index number that was specified as the last value for each
 *    struct option in appOptions[]; 'opt_arg' is the user's argument
 *    to the switch for options that have a REQUIRED_ARG or an
 *    OPTIONAL_ARG.
 */
static int
appOptionsHandler(
    clientData   UNUSED(cData),
    int                 opt_index,
    char               *opt_arg)
{
#if SK_ENABLE_IPV6
    uint32_t j;
#endif
    uint32_t n;
    uint32_t i;
    int rv;

    /* Store the address of the prefix to set in the switch(), then
     * set the referenced value below the switch() */
    switch ((appOptionsEnum)opt_index) {
      case OPT_4SIP_PREFIX_LENGTH:
      case OPT_4DIP_PREFIX_LENGTH:
      case OPT_4NHIP_PREFIX_LENGTH:
        /* Which mask to change */
        i = (opt_index - OPT_4SIP_PREFIX_LENGTH);
        /* Parse value */
        rv = skStringParseUint32(&n, opt_arg, 1, 32);
        if (rv) {
            goto PARSE_ERROR;
        }
        if (net_mask[i].bits4) {
            skAppPrintErr(("The %s value was given multiple times;\n"
                           "\tusing final value %lu"),
                          appOptions[opt_index].name, (unsigned long)n);
        }
        net_mask[i].bits4 = (uint8_t)n;
        net_mask[i].mask4 = ~((n == 32) ? 0 : (UINT32_MAX >> n));
        break;

#if SK_ENABLE_IPV6
      case OPT_6SIP_PREFIX_LENGTH:
      case OPT_6DIP_PREFIX_LENGTH:
      case OPT_6NHIP_PREFIX_LENGTH:
        /* Which mask to change */
        i = (opt_index - OPT_6SIP_PREFIX_LENGTH);
        /* Parse value */
        rv = skStringParseUint32(&n, opt_arg, 1, 128);
        if (rv) {
            goto PARSE_ERROR;
        }
        if (net_mask[i].bits6) {
            skAppPrintErr(("The %s value was given multiple times;\n"
                           "\tusing final value %lu"),
                          appOptions[opt_index].name, (unsigned long)n);
        }
        net_mask[i].bits6 = (uint8_t)n;
        /* byte in the uint8_t[16] where the change occurs */
        j = n >> 3;
        memset(&net_mask[i].mask6[0], 0xFF, j);
        if (n < 128) {
            net_mask[i].mask6[j] = ~(0xFF >> (n & 0x07));
            memset(&net_mask[i].mask6[j+1], 0, (15 - j));
        }
        break;
#endif  /* SK_ENABLE_IPV6 */

      case OPT_OUTPUT_PATH:
        if (output_path) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        output_path = opt_arg;
        break;
    }

    return 0;                   /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  appOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;
}


/*
 *  maskInput(in_ios, out_ios);
 *
 *    Read SiLK Flow records from the 'in_ios' stream, mask off the
 *    source, destination, and/or next-hop IP addresses, and print the
 *    records to the 'out_ios' stream.
 */
static int
maskInput(
    skstream_t         *in_ios,
    skstream_t         *out_ios)
{
    rwRec rwrec;
    int rv;

    /* read the records and mask the IP addresses */
    while ((rv = skStreamReadRecord(in_ios, &rwrec)) == SKSTREAM_OK) {
#if SK_ENABLE_IPV6
        if (rwRecIsIPv6(&rwrec)) {
            if (net_mask[SIP_MASK].bits6) {
                rwRecApplyMaskSIPv6(&rwrec, net_mask[SIP_MASK].mask6);
            }
            if (net_mask[DIP_MASK].bits6) {
                rwRecApplyMaskDIPv6(&rwrec, net_mask[DIP_MASK].mask6);
            }
            if (net_mask[NHIP_MASK].bits6) {
                rwRecApplyMaskNhIPv6(&rwrec, net_mask[NHIP_MASK].mask6);
            }
        } else
#endif  /* SK_ENABLE_IPV6 */
        {
            if (net_mask[SIP_MASK].bits4) {
                rwRecApplyMaskSIPv4(&rwrec, net_mask[SIP_MASK].mask4);
            }
            if (net_mask[DIP_MASK].bits4) {
                rwRecApplyMaskDIPv4(&rwrec, net_mask[DIP_MASK].mask4);
            }
            if (net_mask[NHIP_MASK].bits4) {
                rwRecApplyMaskNhIPv4(&rwrec, net_mask[NHIP_MASK].mask4);
            }
        }

        rv = skStreamWriteRecord(out_ios, &rwrec);
        if (SKSTREAM_ERROR_IS_FATAL(rv)) {
            skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
            return rv;
        }
    }

    if (SKSTREAM_ERR_EOF != rv) {
        skStreamPrintLastErr(in_ios, rv, &skAppPrintErr);
    }
    return SKSTREAM_OK;
}


int main(int argc, char **argv)
{
    skstream_t *rwios_in;
    skstream_t *rwios_out;
    int rv;

    appSetup(argc, argv);       /* never returns on error */

    /* Open the output file */
    if ((rv = skStreamCreate(&rwios_out, SK_IO_WRITE, SK_CONTENT_SILK_FLOW))
        || (rv = skStreamBind(rwios_out, output_path))
        || (rv = skStreamSetCompressionMethod(rwios_out, comp_method))
        || (rv = skOptionsNotesAddToStream(rwios_out))
        || (rv = skStreamOpen(rwios_out))
        || (rv = skStreamWriteSilkHeader(rwios_out)))
    {
        skStreamPrintLastErr(rwios_out, rv, &skAppPrintErr);
        skStreamDestroy(&rwios_out);
        exit(EXIT_FAILURE);
    }

    /* Process each input file */
    while ((rv = skOptionsCtxNextSilkFile(optctx, &rwios_in, skAppPrintErr))
           == 0)
    {
        skStreamSetIPv6Policy(rwios_in, ipv6_policy);
        maskInput(rwios_in, rwios_out);
        skStreamDestroy(&rwios_in);
    }
    if (rv < 0) {
        exit(EXIT_FAILURE);
    }

    /* Close output */
    rv = skStreamClose(rwios_out);
    if (rv) {
        skStreamPrintLastErr(rwios_out, rv, &skAppPrintErr);
    }
    skStreamDestroy(&rwios_out);

    /* done */
    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
