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
**    rwbagcat reads a binary bag, converts it to text, and outputs it
**    to stdout.  It can also print various statistics and summary
**    information about the bag.  It attempts to read the bag(s) from
**    stdin or from any arguments.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwbagcat.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skbag.h>
#include <silk/skipaddr.h>
#include <silk/skipset.h>
#include <silk/skprintnets.h>
#include <silk/skstringmap.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* width of count fields in columnar output */
#define COUNT_WIDTH 20

/* the minimum counter allowed by the --mincounter switch */
#define BAGCAT_MIN_COUNTER  UINT64_C(1)

/* return a non-zero value if a record's 'key' and 'counter' values
 * are within the global limits and if the key is in the global
 * 'mask_set' if specified */
#define CHECK_LIMITS(k, c)                                      \
    (((c) >= limits.mincounter) && ((c) <= limits.maxcounter)   \
     && ((0 == limits.key_is_min)                               \
         || (skipaddrCompare(&limits.minkey_ip, (k)) <= 0))     \
     && ((0 == limits.key_is_max)                               \
         || (skipaddrCompare(&limits.maxkey_ip, (k)) >= 0))     \
     && ((NULL == limits.mask_set)                              \
         || skIPSetCheckAddress(limits.mask_set, (k))))

typedef enum bin_scheme_en {
    BINSCHEME_NONE=0,
    BINSCHEME_LINEAR=1,
    BINSCHEME_BINARY=2,
    BINSCHEME_DECIMAL=3
} bin_scheme_t;


/* LOCAL VARIABLES */

/* global I/O state */
sk_options_ctx_t *optctx = NULL;
static skstream_t *output = NULL;
static skstream_t *stats = NULL;
static int print_statistics = 0;
static int print_network = 0;
static bin_scheme_t bin_scheme = BINSCHEME_NONE;
static const char *net_structure = NULL;

/* delimiter between output columns for hosts/counts */
static char output_delimiter = '|';

/* whether key/counter output is in columns (0) or scrunched together (1) */
static int no_columns = 0;

/* whether to suppress the final delimiter; default no (i.e. end with '|') */
static int no_final_delimiter = 0;

/* how to format the keys.  Value is set by the --key-format switch,
 * and the value is an skipaddr_flags_t from silk_types.h.  If the
 * Bag's key is known not to be an IP address, SKIPADDR_DECIMAL is
 * used unless the user explicitly provides --key-format. */
static uint32_t key_format = SKIPADDR_CANONICAL;

/* whether the --key-format switch was explicitly given */
static int key_format_specified = 0;

/* print out keys whose counter is zero---requires a mask_set or that
 * both --minkey and --maxkey are specified */
static int print_zero_counts = 0;

/* the limits for determining which entries get printed. */
static struct limits_st {
    /* the {min,max}counter entered */
    uint64_t        mincounter;
    uint64_t        maxcounter;

    /* only print keys that appear in this set */
    skipset_t      *mask_set;

    /* the {min,max}key entered */
    skipaddr_t      minkey_ip;
    skipaddr_t      maxkey_ip;

    /* true when any limit switch or mask-set was specified */
    unsigned        active     :1;

    /* true when minkey or maxkey was given */
    unsigned        key_is_min :1;
    unsigned        key_is_max :1;

} limits;

/* name of program to run to page output */
static char *pager = NULL;

/* buffers for printing IPs */
static char ip_buf1[SK_NUM2DOT_STRLEN];
static char ip_buf2[SK_NUM2DOT_STRLEN];

/* printed IP address formats: the first of these will be the
 * default */
static const sk_stringmap_entry_t keyformat_names[] = {
    {"canonical",   SKIPADDR_CANONICAL,     NULL,
     "canonical IP format (127.0.0.0, ::1)"},
    {"zero-padded", SKIPADDR_ZEROPAD,       NULL,
     "fully expanded, zero-padded canonical IP format"},
    {"decimal",     SKIPADDR_DECIMAL,       NULL,
     "integer number in decimal format"},
    {"hexadecimal", SKIPADDR_HEXADECIMAL,   NULL,
     "integer number in hexadecimal format"},
    {"force-ipv6",  SKIPADDR_FORCE_IPV6,    NULL,
     "IPv6 hexadectet format with no IPv4 subpart"},
    SK_STRINGMAP_SENTINEL
};


/* OPTIONS SETUP */

typedef enum {
    OPT_NETWORK_STRUCTURE,
    OPT_BIN_IPS,
    OPT_PRINT_STATISTICS,
    OPT_MASK_SET,
    OPT_MINKEY,
    OPT_MAXKEY,
    OPT_MINCOUNTER,
    OPT_MAXCOUNTER,
    OPT_ZERO_COUNTS,
    OPT_OUTPUT_PATH,
    OPT_KEY_FORMAT,
    OPT_INTEGER_KEYS,
    OPT_ZERO_PAD_IPS,
    OPT_NO_COLUMNS,
    OPT_COLUMN_SEPARATOR,
    OPT_NO_FINAL_DELIMITER,
    OPT_DELIMITED,
    OPT_PAGER
} appOptionsEnum;


static struct option appOptions[] = {
    {"network-structure",   OPTIONAL_ARG, 0, OPT_NETWORK_STRUCTURE},
    {"bin-ips",             OPTIONAL_ARG, 0, OPT_BIN_IPS},
    {"print-statistics",    OPTIONAL_ARG, 0, OPT_PRINT_STATISTICS},
    {"mask-set",            REQUIRED_ARG, 0, OPT_MASK_SET},
    {"minkey",              REQUIRED_ARG, 0, OPT_MINKEY},
    {"maxkey",              REQUIRED_ARG, 0, OPT_MAXKEY},
    {"mincounter",          REQUIRED_ARG, 0, OPT_MINCOUNTER},
    {"maxcounter",          REQUIRED_ARG, 0, OPT_MAXCOUNTER},
    {"zero-counts",         NO_ARG,       0, OPT_ZERO_COUNTS},
    {"output-path",         REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {"key-format",          REQUIRED_ARG, 0, OPT_KEY_FORMAT},
    {"integer-keys",        NO_ARG,       0, OPT_INTEGER_KEYS},
    {"zero-pad-ips",        NO_ARG,       0, OPT_ZERO_PAD_IPS},
    {"no-columns",          NO_ARG,       0, OPT_NO_COLUMNS},
    {"column-separator",    REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR},
    {"no-final-delimiter",  NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"delimited",           OPTIONAL_ARG, 0, OPT_DELIMITED},
    {"pager",               REQUIRED_ARG, 0, OPT_PAGER},
    {0,0,0,0 }              /* sentinel entry */
};


static const char *appHelp[] = {
    ("Print the sum of counters for each specified CIDR\n"
     "\tblock in the comma-separed list of CIDR block sizes (0--32) and/or\n"
     "\tletters (T=0,A=8,B=16,C=24,X=27,H=32). If argument contains 'S' or\n"
     "\t'/', for each CIDR block print host counts and number of occupied\n"
     "\tsmaller CIDR blocks.  Additional CIDR blocks to summarize can be\n"
     "\tspecified by listing them after the '/'. Def. v4:TS/8,16,24,27.\n"
     "\tA leading 'v6:' treats Bag's keys as IPv6, allows range 0--128,\n"
     "\tdisallows A,B,C,X, sets H to 128, and sets default to TS/48,64"),
    ("Invert the bag and count by distinct volume values.\n"
     "\tlinear   - volume => count(KEYS)\n"
     "\tbinary   - log2(volume) => count(KEYS)\n"
     "\tdecimal  - variation on log10(volume) => count(KEYS)"),
    ("Print statistics about the bag.  Def. no. Write\n"
     "\toutput to the standard output unless an argument is given.\n"
     "\tUse 'stderr' to send the output to the standard error"),
    "Output records that appear in this IPset. Def. All records",
    NULL,
    NULL,
    NULL,
    NULL,
    ("Print keys with a counter of zero. Def. No\n"
     "\t(requires --mask-set or both --minkey and --maxkey)"),
    "Write output to named stream. Def. stdout",
    NULL,
    "DEPRECATED. Equivalent to --key-format=decimal",
    "DEPRECATED. Equivalent to --key-format=zero-padded",
    "Disable fixed-width columnar output. Def. Columnar",
    "Use specified character between columns. Def. '|'",
    "Suppress column delimiter at end of line. Def. No",
    "Shortcut for --no-columns --no-final-del --column-sep=CHAR",
    "Program to invoke to page output. Def. $SILK_PAGER or $PAGER",
    (char *) NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  setOutput(const char* filename, skstream_t **stream_out);
static int  keyFormatParse(const char *format);
static void keyFormatUsage(FILE *fh);
static int
printStatistics(
    const skBag_t      *bag,
    skstream_t         *s_out);


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
#define USAGE_MSG                                                            \
    ("[SWITCHES] [BAG_FILES]\n"                                              \
     "\tPrint binary Bag files as text.  When multiple files are given,\n"   \
     "\tthe bags are processed sequentially---specifically, their entries\n" \
     "\tare not merged.\n")

    FILE *fh = USAGE_FH;
    int i;
#if SK_ENABLE_IPV6
    const char *v4_or_v6 = "v6";
#else
    const char *v4_or_v6 = "v4";
#endif

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    for (i = 0; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch (appOptions[i].val) {
          case OPT_MINKEY:
            fprintf(fh,
                    ("Output records whose key is at least VALUE,"
                     " an IP%s address\n\tor an integer between"
                     " %" PRIu64 " and %" PRIu64 ", inclusive."
                     " Def. Records with\n\tnon-zero counters\n"),
                    v4_or_v6,
                    (uint64_t)SKBAG_KEY_MIN, (uint64_t)SKBAG_KEY_MAX);
            break;
          case OPT_MAXKEY:
            fprintf(fh,
                    ("Output records whose key is not more than VALUE,"
                     " an IP%s\n\taddress or an integer."
                     " Def. Records with non-zero counters\n"),
                    v4_or_v6);
            break;
          case OPT_MINCOUNTER:
            fprintf(fh,
                    ("Output records whose counter is at least VALUE,"
                     " an integer\n\tbetween %" PRIu64 " and %" PRIu64
                     ", inclusive. Def. %" PRIu64 "\n"),
                    BAGCAT_MIN_COUNTER, SKBAG_COUNTER_MAX, BAGCAT_MIN_COUNTER);
            break;
          case OPT_MAXCOUNTER:
            fprintf(fh,
                    ("Output records whose counter is not more than VALUE,"
                     " an\n\tinteger.  Def. %" PRIu64 "\n"),
                    SKBAG_COUNTER_MAX);
            break;
          case OPT_KEY_FORMAT:
            keyFormatUsage(fh);
            break;
          default:
            fprintf(fh, "%s\n", appHelp[i]);
            break;
        }
    }

    skOptionsCtxOptionsUsage(optctx, fh);
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

    if (stats != output) {
        skStreamDestroy(&stats);
    }
    skStreamDestroy(&output);

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

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
            (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    memset(&limits, 0, sizeof(limits));
    limits.mincounter = SKBAG_COUNTER_MIN;
    limits.maxcounter = SKBAG_COUNTER_MAX;

    optctx_flags = (SK_OPTIONS_CTX_INPUT_BINARY | SK_OPTIONS_CTX_ALLOW_STDIN);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL))
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

    if (print_network == 1 && bin_scheme != BINSCHEME_NONE) {
        skAppPrintErr("Cannot have both --%s and --%s",
                      appOptions[OPT_NETWORK_STRUCTURE].name,
                      appOptions[OPT_BIN_IPS].name);
        skAppUsage();           /* never returns */
    }

    /* when printing of entries with counters of 0 is requested,
     * either --mask-set or both --minkey and --maxkey must be
     * given */
    if (print_zero_counts
        && (NULL == limits.mask_set)
        && (0 == limits.key_is_min || 0 == limits.key_is_max))
    {
        skAppPrintErr("To use --%s, either --%s or both --%s and --%s"
                      " must be specified",
                      appOptions[OPT_ZERO_COUNTS].name,
                      appOptions[OPT_MASK_SET].name,
                      appOptions[OPT_MINKEY].name,
                      appOptions[OPT_MAXKEY].name);
        skAppUsage();           /* never returns */
    }

    /* write an error message and exit when a minimum is greater than
     * a maximum */
    if (limits.mincounter > limits.maxcounter) {
        skAppPrintErr(("Minimum counter greater than maximum: "
                       "%" PRIu64 " > %" PRIu64),
                      limits.mincounter, limits.maxcounter);
        exit(EXIT_FAILURE);
    }
    if (limits.key_is_min && limits.key_is_max) {
        if (skipaddrCompare(&limits.maxkey_ip, &limits.minkey_ip) < 0) {
            skAppPrintErr("Minimum key greater than maximum: %s > %s",
                          skipaddrString(ip_buf1, &limits.minkey_ip, 0),
                          skipaddrString(ip_buf2, &limits.maxkey_ip, 0));
            exit(EXIT_FAILURE);
        }
    }

    /* Set the default output if none was set */
    if (output == NULL) {
        if (setOutput("stdout", &output)) {
            skAppPrintErr("Unable to print to standard output");
            exit(EXIT_FAILURE);
        }
    }

    /* If print-statistics was requested but its output stream hasn't
     * been set, set it to stdout. */
    if (print_statistics && stats == NULL) {
        if (setOutput("stdout", &stats)) {
            skAppPrintErr("Unable to print to standard output");
            exit(EXIT_FAILURE);
        }
    }

    rv = skStreamOpen(output);
    if (rv) {
        skStreamPrintLastErr(output, rv, &skAppPrintErr);
        exit(EXIT_FAILURE);
    }
    if (stats != NULL && stats != output) {
        rv = skStreamOpen(stats);
        if (rv) {
            skStreamPrintLastErr(stats, rv, &skAppPrintErr);
            exit(EXIT_FAILURE);
        }
    }

    /* Allow paging of the output */
    skStreamPageOutput(output, pager);

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
    skstream_t *stream = NULL;
    uint64_t val64;
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_NETWORK_STRUCTURE:
        net_structure = opt_arg;
        print_network = 1;
        break;

      case OPT_BIN_IPS:
        if (opt_arg == NULL) {
            bin_scheme = BINSCHEME_LINEAR;
        } else {
            size_t len = strlen(opt_arg);
            if (len == 0) {
                skAppPrintErr("Invalid %s: Switch requires an argument",
                              appOptions[opt_index].name);
                return 1;
            }
            if (strncmp(opt_arg, "linear", len) == 0) {
                bin_scheme = BINSCHEME_LINEAR;
            } else if (strncmp(opt_arg, "binary", len) == 0) {
                bin_scheme = BINSCHEME_BINARY;
            } else if (strncmp(opt_arg, "decimal", len) == 0) {
                bin_scheme = BINSCHEME_DECIMAL;
            } else {
                skAppPrintErr("Illegal bin scheme. "
                              "Should be one of: linear, binary, decimal.");
                return 1;
            }
        }
        break;

      case OPT_PRINT_STATISTICS:
        if (opt_arg != NULL) {
            if (stats) {
                skAppPrintErr("Invalid %s: Switch used multiple times",
                              appOptions[opt_index].name);
                return 1;
            }
            if (setOutput(opt_arg, &stats)) {
                skAppPrintErr("Invalid %s '%s'",
                              appOptions[opt_index].name, opt_arg);
                return 1;
            }
        }
        print_statistics = 1;
        break;

      case OPT_MINCOUNTER:
        rv = skStringParseUint64(&val64, opt_arg,
                                 BAGCAT_MIN_COUNTER, SKBAG_COUNTER_MAX);
        if (rv == SKUTILS_ERR_MINIMUM) {
            skAppPrintErr(("Invalid %s: Smallest allowable value is %" PRIu64
                           ".\n"
                           "\tUse --%s to print records whose counters are 0"),
                          appOptions[opt_index].name, BAGCAT_MIN_COUNTER,
                          appOptions[OPT_ZERO_COUNTS].name);
            return 1;
        }
        if (rv) {
            goto PARSE_ERROR;
        }
        limits.mincounter = val64;
        limits.active = 1;
        break;

      case OPT_MAXCOUNTER:
        rv = skStringParseUint64(&val64, opt_arg,
                                 BAGCAT_MIN_COUNTER, SKBAG_COUNTER_MAX);
        if (rv) {
            goto PARSE_ERROR;
        }
        limits.maxcounter = val64;
        limits.active = 1;
        break;

      case OPT_MINKEY:
        rv = skStringParseIP(&limits.minkey_ip, opt_arg);
        if (rv) {
            goto PARSE_ERROR;
        }
        limits.key_is_min = 1;
        break;

      case OPT_MAXKEY:
        rv = skStringParseIP(&limits.maxkey_ip, opt_arg);
        if (rv) {
            goto PARSE_ERROR;
        }
        limits.key_is_max = 1;
        break;

      case OPT_MASK_SET:
        if (limits.mask_set) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
            || (rv = skStreamBind(stream, opt_arg))
            || (rv = skStreamOpen(stream)))
        {
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            skStreamDestroy(&stream);
            return 1;
        }
        rv = skIPSetRead(&limits.mask_set, stream);
        if (rv) {
            if (SKIPSET_ERR_FILEIO == rv) {
                skStreamPrintLastErr(stream, skStreamGetLastReturnValue(stream),
                                     &skAppPrintErr);
            } else {
                skAppPrintErr("Unable to read IPset from '%s': %s",
                              opt_arg, skIPSetStrerror(rv));
            }
            skStreamDestroy(&stream);
            return 1;
        }
        skStreamDestroy(&stream);
        break;

      case OPT_OUTPUT_PATH:
        if (output) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if (setOutput(opt_arg, &output)) {
            skAppPrintErr("Invalid %s '%s'",
                          appOptions[opt_index].name, opt_arg);
            return 1;
        }
        break;

      case OPT_NO_COLUMNS:
        no_columns = 1;
        break;

      case OPT_NO_FINAL_DELIMITER:
        no_final_delimiter = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        output_delimiter = opt_arg[0];
        break;

      case OPT_DELIMITED:
        no_columns = 1;
        no_final_delimiter = 1;
        if (opt_arg) {
            output_delimiter = opt_arg[0];
        }
        break;

      case OPT_KEY_FORMAT:
        if (keyFormatParse(opt_arg)) {
            return 1;
        }
        break;

      case OPT_INTEGER_KEYS:
        if (keyFormatParse("decimal")) {
            skAbort();
        }
        break;

      case OPT_ZERO_PAD_IPS:
        if (keyFormatParse("zero-padded")) {
            skAbort();
        }
        break;

      case OPT_ZERO_COUNTS:
        print_zero_counts = 1;
        break;

      case OPT_PAGER:
        pager = opt_arg;
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
 *  status = keyFormatParse(format_string);
 *
 *    Parse the key-format value contained in 'format_string'.  Return
 *    0 on success, or -1 if parsing of the value fails.
 */
static int
keyFormatParse(
    const char         *format)
{
    char buf[256];
    char *errmsg;
    sk_stringmap_t *str_map = NULL;
    sk_stringmap_iter_t *iter = NULL;
    sk_stringmap_entry_t *found_entry;
    const sk_stringmap_entry_t *entry;
    int name_seen = 0;
    int rv = -1;

    /* create a stringmap of the available ip formats */
    if (SKSTRINGMAP_OK != skStringMapCreate(&str_map)) {
        skAppPrintOutOfMemory(NULL);
        goto END;
    }
    if (skStringMapAddEntries(str_map, -1, keyformat_names) != SKSTRINGMAP_OK){
        skAppPrintOutOfMemory(NULL);
        goto END;
    }

    /* attempt to match */
    if (skStringMapParse(str_map, format, SKSTRINGMAP_DUPES_ERROR,
                         &iter, &errmsg))
    {
        skAppPrintErr("Invalid %s: %s",
                      appOptions[OPT_KEY_FORMAT].name, errmsg);
        goto END;
    }

    while (skStringMapIterNext(iter, &found_entry, NULL) == SK_ITERATOR_OK) {
        switch (found_entry->id) {
          case SKIPADDR_CANONICAL:
          case SKIPADDR_ZEROPAD:
          case SKIPADDR_DECIMAL:
          case SKIPADDR_HEXADECIMAL:
          case SKIPADDR_FORCE_IPV6:
            if (name_seen) {
                entry = keyformat_names;
                strncpy(buf, entry->name, sizeof(buf));
                for (++entry; entry->name; ++entry) {
                    strncat(buf, ",", sizeof(buf)-strlen(buf)-1);
                    strncat(buf, entry->name, sizeof(buf)-strlen(buf)-1);
                }
                skAppPrintErr("Invalid %s: May only specify one of %s",
                              appOptions[OPT_KEY_FORMAT].name, buf);
                goto END;
            }
            name_seen = 1;
            key_format = found_entry->id;
            break;

          default:
            skAbortBadCase(found_entry->id);
        }
    }

    key_format_specified = 1;
    rv = 0;

  END:
    if (str_map) {
        skStringMapDestroy(str_map);
    }
    if (iter) {
        skStringMapIterDestroy(iter);
    }
    return rv;
}


/*
 *  keyFormatUsage(fh);
 *
 *    Print the description of the argument to the --key-format
 *    switch to the 'fh' file handle.
 */
static void
keyFormatUsage(
    FILE               *fh)
{
    const sk_stringmap_entry_t *e;
    unsigned int decimal;

    for (e = keyformat_names, decimal = 0; e->name; ++e, ++decimal) {
        if (SKIPADDR_DECIMAL == e->id) {
            break;
        }
    }
    if (NULL == e->name) {
        skAbort();
    }

    fprintf(fh,
            ("Print keys in specified format. Def. '%s' unless\n"
             "\tBag's key is known not to be an IP, then '%s'. Choices:\n"),
            keyformat_names[0].name, keyformat_names[decimal].name);
    for (e = keyformat_names; e->name; ++e) {
        fprintf(fh, "\t%-12s - %s\n",
                e->name, (const char*)e->userdata);
    }
}


/*
 *  status = setOutput(name, &stream);
 *
 *    Set stream's output to 'name'.  If 'name' is the standard output
 *    and an existing stream is already open to the standard output,
 *    set 'stream' to that existing stream.  Return 0 on success, -1
 *    otherwise.
 */
static int
setOutput(
    const char         *filename,
    skstream_t        **stream)
{
    int rv;

    assert(stream);

    if (filename == NULL || filename[0] == '\0') {
        skAppPrintErr("Empty filename");
        return -1;
    }

    /* compare 'filename' with known streams */
    if (output) {
        if ((0 == strcmp(skStreamGetPathname(output), filename))
            || (0 == strcmp(filename, "stdout")
                && 0 == strcmp(skStreamGetPathname(output), "-"))
            || (0 == strcmp(filename, "-")
                && 0 == strcmp(skStreamGetPathname(output), "stdout")))
        {
            *stream = output;
            return 0;
        }
    }
    if (stats) {
        if ((0 == strcmp(skStreamGetPathname(stats), filename))
            || (0 == strcmp(filename, "stdout")
                && 0 == strcmp(skStreamGetPathname(stats), "-"))
            || (0 == strcmp(filename, "-")
                && 0 == strcmp(skStreamGetPathname(stats), "stdout")))
        {
            *stream = stats;
            return 0;
        }
    }

    if ((rv = skStreamCreate(stream, SK_IO_WRITE, SK_CONTENT_TEXT))
        || (rv = skStreamBind(*stream, filename)))
    {
        skStreamPrintLastErr(*stream, rv, &skAppPrintErr);
        skStreamDestroy(stream);
        return -1;
    }

    return 0;
}


static int
bagcatInvertBag(
    const skBag_t      *bag)
{
    skBag_t *inverted_bag = NULL;
    skBagIterator_t *iter = NULL;
    skBagTypedKey_t key;
    skBagTypedCounter_t counter;
    skBagTypedKey_t bin;
    char s_label[64];
    char final_delim[] = {'\0', '\0'};
    int rv = 1;

    if (!no_final_delimiter) {
        final_delim[0] = output_delimiter;
    }

    /* Create an inverted bag */
    if (skBagCreate(&inverted_bag) != SKBAG_OK) {
        goto END;
    }
    if (skBagIteratorCreate(bag, &iter) != SKBAG_OK) {
        goto END;
    }

    /* get key from bag as an ip address */
    key.type = SKBAG_KEY_IPADDR;
    counter.type = SKBAG_COUNTER_U64;

    bin.type = SKBAG_KEY_U32;

    /* loop over the entries, check whether they are in limits, and if
     * so, add the inverted entry to the bag */
    while (skBagIteratorNextTyped(iter, &key, &counter) == SKBAG_OK) {
        if (!CHECK_LIMITS(&key.val.addr, counter.val.u64)) {
            continue;
        }
        switch (bin_scheme) {
          case BINSCHEME_LINEAR:
            bin.val.u32 = (uint32_t)((counter.val.u64 < UINT32_MAX)
                                     ? counter.val.u64
                                     : UINT32_MAX);
            break;
          case BINSCHEME_BINARY:
            bin.val.u32 = skIntegerLog2(counter.val.u64);
            break;
          case BINSCHEME_DECIMAL:
            if (counter.val.u64 < 100) {
                bin.val.u32 = (uint32_t)counter.val.u64;
            } else {
                bin.val.u32
                    = (uint32_t)floor((log10((double)counter.val.u64) - 1.0)
                                      * 100.0);
            }
            break;
          case BINSCHEME_NONE:
            skAbortBadCase(bin_scheme);
        }
        if (skBagCounterIncrement(inverted_bag, &bin) != SKBAG_OK) {
            goto END;
        }
    }

    skBagIteratorDestroy(iter);
    iter = NULL;

    /* iterate over inverted bag to print entries */
    if (skBagIteratorCreate(inverted_bag, &iter) != SKBAG_OK) {
        goto END;
    }

    while (skBagIteratorNextTyped(iter, &bin, &counter) == SKBAG_OK) {
        switch (bin_scheme) {
          case BINSCHEME_LINEAR:
            /* label is just bin number */
            snprintf(s_label, sizeof(s_label), ("%" PRIu32), bin.val.u32);
            break;

          case BINSCHEME_BINARY:
            /* label is range of values "2^03 to 2^04-1" */
            snprintf(s_label, sizeof(s_label),
                     ("2^%02" PRIu32 " to 2^%02" PRIu32 "-1"),
                     bin.val.u32, (bin.val.u32 + 1));
            break;

          case BINSCHEME_DECIMAL:
            /* label is the median value of possible keys in that bin */
            if (bin.val.u32 < 100) {
                snprintf(s_label, sizeof(s_label), ("%" PRIu32),bin.val.u32);
            } else {
                double min, max, mid;
                min = ceil(pow(10, (((double)bin.val.u32 / 100.0) + 1.0)));
                max = floor(pow(10, ((((double)bin.val.u32 + 1.0)/100.0)+1.0)));
                mid = floor((min + max) / 2.0);
                snprintf(s_label, sizeof(s_label), "%.0f", mid);
            }
            break;

          case BINSCHEME_NONE:
            skAbortBadCase(bin_scheme);
        }

        if (!no_columns) {
            skStreamPrint(output, ("%*s%c%*" PRIu64 "%s\n"),
                          COUNT_WIDTH, s_label, output_delimiter,
                          COUNT_WIDTH, counter.val.u64, final_delim);
        } else {
            skStreamPrint(output, ("%s%c%" PRIu64 "%s\n"),
                          s_label, output_delimiter,
                          counter.val.u64, final_delim);
        }
    }

    rv = 0;

  END:
    skBagDestroy(&inverted_bag);
    if (iter) {
        skBagIteratorDestroy(iter);
    }

    return rv;
}


static int
printNetwork(
    const skBag_t      *bag)
{
    skBagTypedKey_t key;
    skBagTypedCounter_t counter;
    skNetStruct_t *ns;

    /* Set up the skNetStruct */
    if (skNetStructureCreate(&ns, 1)) {
        skAppPrintErr("Error creating network-structure");
        return 1;
    }
    skNetStructureSetCountWidth(ns, COUNT_WIDTH);
    if (skNetStructureParse(ns, net_structure)) {
        return 1;
    }
    skNetStructureSetOutputStream(ns, output);
    skNetStructureSetDelimiter(ns, output_delimiter);
    if (no_columns) {
        skNetStructureSetNoColumns(ns);
    }
    if (no_final_delimiter) {
        skNetStructureSetNoFinalDelimiter(ns);
    }
    skNetStructureSetIpFormat(ns, key_format);

    /* set type for key and counter */
    key.type = SKBAG_KEY_IPADDR;
    counter.type = SKBAG_COUNTER_U64;

    if (0 == print_zero_counts) {
        /* print contents of the bag, subject to limits */
        skBagIterator_t *b_iter;

        if (skBagIteratorCreate(bag, &b_iter) != SKBAG_OK) {
            return 1;
        }
        while (skBagIteratorNextTyped(b_iter, &key, &counter) == SKBAG_OK) {
            if (CHECK_LIMITS(&key.val.addr, counter.val.u64)) {
                skNetStructureAddKeyCounter(ns, &key.val.addr,
                                            &counter.val.u64);
            }
        }
        skBagIteratorDestroy(b_iter);

    } else if (NULL == limits.mask_set) {
        /* print keys between two key values, subject to maximum
         * counter limit */

        /* handle first key */
        skipaddrCopy(&key.val.addr, &limits.minkey_ip);
        skBagCounterGet(bag, &key, &counter);
        if (counter.val.u64 <= limits.maxcounter) {
            skNetStructureAddKeyCounter(ns, &key.val.addr, &counter.val.u64);
        }

        /* handle remaining keys */
        while (skipaddrCompare(&key.val.addr, &limits.maxkey_ip) < 0) {
            skipaddrIncrement(&key.val.addr);
            skBagCounterGet(bag, &key, &counter);
            if (counter.val.u64 <= limits.maxcounter) {
                skNetStructureAddKeyCounter(ns, &key.val.addr,
                                            &counter.val.u64);
            }
        }

    } else if (0 == limits.key_is_min && 0 == limits.key_is_max) {
        /* print keys that appear in the IPset, subject to the maximum
         * counter limit */
        skipset_iterator_t s_iter;
        uint32_t cidr;

        skIPSetIteratorBind(&s_iter, limits.mask_set, 0, SK_IPV6POLICY_MIX);
        while (skIPSetIteratorNext(&s_iter, &key.val.addr, &cidr)
               == SK_ITERATOR_OK)
        {
            skBagCounterGet(bag, &key, &counter);
            if (counter.val.u64 <= limits.maxcounter) {
                skNetStructureAddKeyCounter(ns, &key.val.addr,
                                            &counter.val.u64);
            }
        }

    } else {
        /* print keys that appear in the IPset, subject to limits */
        skipset_iterator_t s_iter;
        uint32_t cidr;

        /* ensure minimum counter is 0*/
        limits.mincounter = SKBAG_COUNTER_MIN;

        skIPSetIteratorBind(&s_iter, limits.mask_set, 0, SK_IPV6POLICY_MIX);
        while (skIPSetIteratorNext(&s_iter, &key.val.addr, &cidr)
               == SK_ITERATOR_OK)
        {
            skBagCounterGet(bag, &key, &counter);
            if (CHECK_LIMITS(&key.val.addr, counter.val.u64)) {
                skNetStructureAddKeyCounter(ns, &key.val.addr,
                                            &counter.val.u64);
            }
        }
    }

    skNetStructurePrintFinalize(ns);
    skNetStructureDestroy(&ns);

    return 0;
}


static int
printStatistics(
    const skBag_t      *bag,
    skstream_t         *stream_out)
{
    double counter_temp =  0.0;
    double counter_mult =  0.0;
    double sum =  0.0; /* straight sum */
    double sum2 = 0.0; /* sum of squares */
    double sum3 = 0.0; /* sum of cubes */

    double key_count = 0.0;
    double mean = 0.0;
    double stddev = 0.0;
    double temp = 0.0;
    double variance = 0.0;
    double skew = 0.0;
    double kurtosis = 0.0;

    skBagIterator_t *iter;
    skBagTypedKey_t key;
    skBagTypedCounter_t counter;

    skipaddr_t min_seen_key, max_seen_key;
    uint64_t min_seen_counter, max_seen_counter;
    skBagErr_t rv;

#define SUMS_OF_COUNTERS(soc_count)                     \
    {                                                   \
        /* straight sum */                              \
        counter_temp = (double)(soc_count);             \
        sum += counter_temp;                            \
        /* sum of squares */                            \
        counter_mult = counter_temp * counter_temp;     \
        sum2 += counter_mult;                           \
        /* sum of cubes */                              \
        counter_mult *= counter_temp;                   \
        sum3 += counter_mult;                           \
    }

    assert(bag != NULL);
    assert(stream_out != NULL);

    if (skBagIteratorCreateUnsorted(bag, &iter) != SKBAG_OK) {
        return 1;
    }

    key.type = SKBAG_KEY_IPADDR;
    counter.type = SKBAG_COUNTER_U64;

    /* find first entry within limits */
    while ((rv = skBagIteratorNextTyped(iter, &key, &counter))
           == SKBAG_OK)
    {
        if (CHECK_LIMITS(&key.val.addr, counter.val.u64)) {
            break;
        }
        ++key_count;
    }

    if (SKBAG_ERR_KEY_NOT_FOUND == rv) {
        /* reached end of bag */
        skStreamPrint(stream_out, "\nStatistics\n");
        if (key_count < 1.0) {
            skStreamPrint(stream_out, "  No entries in bag.\n");
        } else {
            skStreamPrint(stream_out,
                          "  No entries in bag within limits.\n");
        }
        return 0;
    }
    if (SKBAG_OK != rv) {
        /* some other unexpected error */
        skAppPrintErr("Error iterating over bag: %s",
                      skBagStrerror(rv));
        return 1;
    }

    key_count = 1;
    skipaddrCopy(&min_seen_key, &key.val.addr);
    skipaddrCopy(&max_seen_key, &key.val.addr);
    min_seen_counter = max_seen_counter = counter.val.u64;
    SUMS_OF_COUNTERS(counter.val.u64);

    while (skBagIteratorNextTyped(iter, &key, &counter) == SKBAG_OK) {
        if (!CHECK_LIMITS(&key.val.addr, counter.val.u64)) {
            continue;
        }

        ++key_count;
        SUMS_OF_COUNTERS(counter.val.u64);

        if (counter.val.u64 < min_seen_counter) {
            min_seen_counter = counter.val.u64;
        } else if (counter.val.u64 > max_seen_counter) {
            max_seen_counter = counter.val.u64;
        }
        if (skipaddrCompare(&key.val.addr, &min_seen_key) < 0) {
            skipaddrCopy(&min_seen_key, &key.val.addr);
        } else if (skipaddrCompare(&key.val.addr, &max_seen_key) > 0) {
            skipaddrCopy(&max_seen_key, &key.val.addr);
        }
    }

    if (skBagIteratorDestroy(iter) != SKBAG_OK) {
        return 1;
    }

    skStreamPrint(stream_out, "\nStatistics\n");

    skipaddrString(ip_buf1, &min_seen_key, key_format);
    skipaddrString(ip_buf2, &max_seen_key, key_format);

    /* formulae derived from HyperStat Online - David M. Lane */

    /* http://davidmlane.com/hyperstat/A15885.html (mean) */
    mean = sum / key_count;

    /* http://davidmlane.com/hyperstat/A16252.html (variance) */

    temp = (sum2
            - (2.0 * mean * sum)
            + (key_count * mean * mean));

    variance = temp / (key_count - 1.0);

    /* http://davidmlane.com/hyperstat/A16252.html (standard deviation) */
    stddev = sqrt(variance);

    /* http://davidmlane.com/hyperstat/A11284.html (skew) */
    skew = ((sum3
             - (3.0 * mean * sum2)
             + (3.0 * mean * mean * sum)
             - (key_count * mean * mean * mean))
            / (key_count * variance * stddev));

    /* http://davidmlane.com/hyperstat/A53638.html (kurtosis) */
    kurtosis = (temp * temp) / (key_count * variance * variance);

    skStreamPrint(stream_out, ("%18s:  %" PRIu64 "\n%18s:  %" PRIu64 "\n"
                               "%18s:  %s\n%18s:  %s\n"
                               "%18s:  %" PRIu64 "\n%18s:  %" PRIu64 "\n"
                               "%18s:  %.4g\n%18s:  %.4g\n%18s:  %.4g\n"
                               "%18s:  %.4g\n%18s:  %.4g\n"),
                  "keys",               (uint64_t)key_count,
                  "sum of counters",    (uint64_t)sum,
                  "minimum key",        ip_buf1,
                  "maximum key",        ip_buf2,
                  "minimum counter",    (uint64_t)min_seen_counter,
                  "maximum counter",    (uint64_t)max_seen_counter,
                  "mean",               mean,
                  "variance",           variance,
                  "standard deviation", stddev,
                  "skew",               skew,
                  "kurtosis",           kurtosis);
    skBagPrintTreeStats(bag, stream_out);

    return 0;
}


/*
 * Output bag using current state of options
 */
static int
processBag(
    const skBag_t      *bag)
{
    /* determine output format based on type of key in the bag unless
     * the user provided the --key-format switch. */
    if (0 == key_format_specified) {
        switch (skBagKeyFieldType(bag)) {
          case SKBAG_FIELD_CUSTOM:
          case SKBAG_FIELD_SIPv4:
          case SKBAG_FIELD_DIPv4:
          case SKBAG_FIELD_NHIPv4:
          case SKBAG_FIELD_ANY_IPv4:
          case SKBAG_FIELD_SIPv6:
          case SKBAG_FIELD_DIPv6:
          case SKBAG_FIELD_NHIPv6:
          case SKBAG_FIELD_ANY_IPv6:
            key_format = SKIPADDR_CANONICAL;
            break;
          default:
            key_format = SKIPADDR_DECIMAL;
            break;
        }
    }

    /* default to printing network hosts */
    if (!print_statistics && !print_network
        && (bin_scheme == BINSCHEME_NONE))
    {
        print_network = 1;
        if (16 == skBagKeyFieldLength(bag)) {
            net_structure = "v6:H";
        } else {
            net_structure = "v4:H";
        }
    }

    if (print_network != 0) {
        if (printNetwork(bag) != 0) {
            skAppPrintErr("Cannot print network structure");
            exit(EXIT_FAILURE);
        }
    }

    if (bin_scheme != BINSCHEME_NONE) {
        bagcatInvertBag(bag);
    }
    if (print_statistics) {
        printStatistics(bag, stats);
    }

    return 0;
}


int main(int argc, char **argv)
{
    skBagErr_t err;
    skBag_t *bag = NULL;
    char *filename;
    int rv;

    appSetup(argc, argv);       /* never returns on error */

    while ((rv = skOptionsCtxNextArgument(optctx, &filename)) == 0) {
        err = skBagLoad(&bag, filename);
        if (err != SKBAG_OK) {
            skAppPrintErr("Error reading bag from input stream '%s': %s",
                          filename, skBagStrerror(err));
            exit(EXIT_FAILURE);
        }
        if (processBag(bag)) {
            skAppPrintErr("Error processing bag '%s'", filename);
            skBagDestroy(&bag);
            exit(EXIT_FAILURE);
        }
        skBagDestroy(&bag);
    }

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
