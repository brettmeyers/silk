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
**  rwtotalsetup.c
**
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwtotalsetup.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwtotal.h"


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout


/* LOCAL VARIABLES */

/* where to write output */
static sk_fileptr_t output;

/* name of program to run to page output */
static char *pager;


/* OPTIONS SETUP */

static struct option appOptions[] = {
    {"sip-first-8",         NO_ARG,       0, OPT_SIP_FIRST_8},
    {"sip-first-16",        NO_ARG,       0, OPT_SIP_FIRST_16},
    {"sip-first-24",        NO_ARG,       0, OPT_SIP_FIRST_24},
    {"sip-last-8",          NO_ARG,       0, OPT_SIP_LAST_8},
    {"sip-last-16",         NO_ARG,       0, OPT_SIP_LAST_16},

    {"dip-first-8",         NO_ARG,       0, OPT_DIP_FIRST_8},
    {"dip-first-16",        NO_ARG,       0, OPT_DIP_FIRST_16},
    {"dip-first-24",        NO_ARG,       0, OPT_DIP_FIRST_24},
    {"dip-last-8",          NO_ARG,       0, OPT_DIP_LAST_8},
    {"dip-last-16",         NO_ARG,       0, OPT_DIP_LAST_16},

    {"sport",               NO_ARG,       0, OPT_SPORT},
    {"dport",               NO_ARG,       0, OPT_DPORT},
    {"proto",               NO_ARG,       0, OPT_PROTO},
    {"packets",             NO_ARG,       0, OPT_PACKETS},
    {"bytes",               NO_ARG,       0, OPT_BYTES},
    {"duration",            NO_ARG,       0, OPT_DURATION},
    {"icmp-code",           NO_ARG,       0, OPT_ICMP_CODE},

    {"summation",           NO_ARG,       0, OPT_SUMMATION},
    {"min-bytes",           REQUIRED_ARG, 0, OPT_MIN_BYTES},
    {"min-packets",         REQUIRED_ARG, 0, OPT_MIN_PACKETS},
    {"min-records",         REQUIRED_ARG, 0, OPT_MIN_RECORDS},
    {"max-bytes",           REQUIRED_ARG, 0, OPT_MAX_BYTES},
    {"max-packets",         REQUIRED_ARG, 0, OPT_MAX_PACKETS},
    {"max-records",         REQUIRED_ARG, 0, OPT_MAX_RECORDS},
    {"skip-zeroes",         NO_ARG,       0, OPT_SKIP_ZEROES},
    {"no-titles",           NO_ARG,       0, OPT_NO_TITLES},
    {"no-columns",          NO_ARG,       0, OPT_NO_COLUMNS},
    {"column-separator",    REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR},
    {"no-final-delimiter",  NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"delimited",           OPTIONAL_ARG, 0, OPT_DELIMITED},
    {"output-path",         REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {"pager",               REQUIRED_ARG, 0, OPT_PAGER},
    {0,0,0,0}               /* sentinel entry */
};

static const char *appHelp[] = {
    "Key on the first 8 bits of the source IP address",
    "Key on the first 16 bits of the source IP address",
    "Key on the first 24 bits of the source IP address",
    "Key on the last 8 bits of the source IP address",
    "Key on the last 16 bits of the source IP address",

    "Key on the first 8 bits of the destination IP address",
    "Key on the first 16 bits of the destination IP address",
    "Key on the first 24 bits of the destination IP address",
    "Key on the last 8 bits of the destination IP address",
    "Key on the last 16 bits of the  destination  IP address",

    "Key on the source port",
    "Key on the destination port",
    "Key on the protocol",
    "Key on the number of packets",
    "Key on the number of bytes",
    "Key on duration",
    ("Key on icmp type and code (DOES NOT check to see\n"
     "\t if the record is ICMP)"),

    "Print a summation row that totals all columns. Def. No",
    ("Do not print bins having fewer than this many bytes.\n"
     "\tDef. 0"),
    ("Do not print bins having fewer than this many packets.\n"
     "\tDef. 0"),
    ("Do not print bins having fewer than this many records.\n"
     "\tDef. 0"),
    ("Do not print bins having more than this many bytes.\n"
     "\tDef. 18446744073709551615"),
    ("Do not print bins having more than this many packets.\n"
     "\tDef. 18446744073709551615"),
    ("Do not print bins having more than this many records.\n"
     "\tDef. 18446744073709551615"),
    "Do not print bins having zero records. Def. Print all",
    "Do not print column titles. Def. Print titles",
    "Disable fixed-width columnar output. Def. Columnar",
    "Use specified character between columns. Def. '|'",
    "Suppress column delimiter at end of line. Def. No",
    "Shortcut for --no-columns --no-final-del --column-sep=CHAR",
    "Send output to given file path. Def. stdout",
    "Program to invoke to page output. Def. $SILK_PAGER or $PAGER",
    (char *)NULL
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
#define USAGE_MSG                                                             \
    ("<KEY> [SWITCHES] [FILES]\n"                                             \
     "\tSummarize SiLK Flow records by a specified key and print the byte,\n" \
     "\tpacket, and flow counts for flows matching the key.  When no files\n" \
     "\tare given on the command line, flows are read from STDIN.\n")

    FILE *fh = USAGE_FH;
    unsigned int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nKEY:\n");
    for (i = 0; appOptions[i].name && i <= COUNT_MODE_MAX_OPTION; ++i) {
        fprintf(fh, "--%s %s. %s\n", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]), appHelp[i]);
    }

    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    for ( ; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. %s\n", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]), appHelp[i]);
    }
    skOptionsCtxOptionsUsage(optctx, fh);
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
void
appTeardown(
    void)
{
    static uint8_t teardownFlag = 0;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* free our RAM */
    if (count_array) {
        free(count_array);
    }

    /* close output file or process */
    if (output.of_name) {
        skFileptrClose(&output, skAppPrintErr);
    }

    /* close copy-input stream */
    skOptionsCtxCopyStreamClose(optctx, &skAppPrintErr);

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
void
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

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize variables */
    count_mode = COUNT_MODE_UNSET;
    for (i = 0; i < NUM_TOTALS; ++i) {
        bounds[i] = 0;
        bounds[NUM_TOTALS + i] = UINT64_MAX;
    }

    memset(&output, 0, sizeof(output));
    output.of_fp = stdout;

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS | SK_OPTIONS_CTX_PRINT_FILENAMES
                    | SK_OPTIONS_CTX_COPY_INPUT);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
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
        skAppUsage();         /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* make certain we have something to count */
    if (count_mode == COUNT_MODE_UNSET) {
        skAppPrintErr("No key specified,\n"
                      "\t Please choose a summarization key.\n");
        skAppUsage();
    }

    /* verify our bounds */
    for (i = 0; i < NUM_TOTALS; ++i) {
        if (bounds[i] > bounds[NUM_TOTALS + i]) {
            const char *field = ((i == C_BYTES)
                                 ? "bytes"
                                 : ((i == C_PKTS)
                                    ? "packets"
                                    : "records"));
            skAppPrintErr(("The min-%s value is greater than max-%s: "
                           "%" PRIu64 " > %" PRIu64),
                          field, field, bounds[i], bounds[NUM_TOTALS + i]);
            exit(EXIT_FAILURE);
        }
    }

    /* make certain stdout is not being used for multiple outputs */
    if (skOptionsCtxCopyStreamIsStdout(optctx)) {
        if ((NULL == output.of_name)
            || (0 == strcmp(output.of_name, "-"))
            || (0 == strcmp(output.of_name, "stdout")))
        {
            skAppPrintErr("May not use stdout for multiple output streams");
            exit(EXIT_FAILURE);
        }
    }

    /* open the --ouptut-path: the 'of_name' member is non-NULL when
     * the switch is given */
    if (output.of_name) {
        rv = skFileptrOpen(&output, SK_IO_WRITE);
        if (rv) {
            skAppPrintErr("Cannot open '%s': %s",
                          output.of_name, skFileptrStrerror(rv));
            exit(EXIT_FAILURE);
        }
    }

    /* looks good, open the --copy-input destination */
    if (skOptionsCtxOpenStreams(optctx, &skAppPrintErr)) {
        exit(EXIT_FAILURE);
    }

    return; /* OK */
}


/*
 *  status = appOptionsHandler(cData, opt_index, opt_arg);
 *
 *    Called by skOptionsParse(), this handles a user-specified switch
 *    that the application has registered, typically by setting global
 *    variables.  Returns 1 if the switch processing failed or 0 if it
 *    succeeded.  Returning a non-zero from from the handler causes
 *    skOptionsParse() to return a negative value.
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
    int i;
    int rv;

    /* handle the switches that set the key/count-mode */
    if (opt_index <= COUNT_MODE_MAX_OPTION) {
        if (count_mode != COUNT_MODE_UNSET) {
            skAppPrintErr("Only one summarization key may be specified");
            return 1;
        }
        count_mode = opt_index;
        return 0;
    }

    switch (opt_index) {
      case OPT_SUMMATION:
        summation = 1;
        break;

      case OPT_MIN_BYTES:
        rv = skStringParseUint64(&(bounds[C_BYTES]), opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MIN_PACKETS:
        rv = skStringParseUint64(&(bounds[C_PKTS]), opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MIN_RECORDS:
        rv = skStringParseUint64(&(bounds[C_RECS]), opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MAX_BYTES:
        rv = skStringParseUint64(&(bounds[NUM_TOTALS + C_BYTES]), opt_arg,0,0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MAX_PACKETS:
        rv = skStringParseUint64(&(bounds[NUM_TOTALS + C_PKTS]), opt_arg, 0,0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MAX_RECORDS:
        rv = skStringParseUint64(&(bounds[NUM_TOTALS + C_RECS]), opt_arg, 0,0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_SKIP_ZEROES:
        /* set lower bound to 1 unless it is already set. */
        for (i = 0; i < NUM_TOTALS; ++i) {
            if (bounds[i] == 0) {
                bounds[i] = 1;
            }
        }
        break;

      case OPT_NO_TITLES:
        no_titles = 1;
        break;

      case OPT_NO_COLUMNS:
        no_columns = 1;
        break;

      case OPT_NO_FINAL_DELIMITER:
        no_final_delimiter = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        delimiter = opt_arg[0];
        break;

      case OPT_DELIMITED:
        no_columns = 1;
        no_final_delimiter = 1;
        if (opt_arg) {
            delimiter = opt_arg[0];
        }
        break;

      case OPT_OUTPUT_PATH:
        if (output.of_name) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        output.of_name = opt_arg;
        break;

      case OPT_PAGER:
        pager = opt_arg;
        break;

      default:
        skAbortBadCase(opt_index);
    } /* switch */

    return 0;                     /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  appOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;
}


FILE *
getOutputHandle(
    void)
{
    int rv;

    /* only invoke the pager when the user has not specified the
     * output-path, even if output-path is stdout */
    if (NULL == output.of_name) {
        /* invoke the pager */
        rv = skFileptrOpenPager(&output, pager);
        if (rv && rv != SK_FILEPTR_PAGER_IGNORED) {
            skAppPrintErr("Unable to invoke pager");
        }
    }

    return output.of_fp;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
