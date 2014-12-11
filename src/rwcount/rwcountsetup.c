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
**  rwcountsetup.c
**
**    Routines for setting up rwcount
**
**    Michael P. Collins
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwcountsetup.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skstringmap.h>
#include "rwcount.h"


/* LOCAL DEFINES AND TYPEDEFS */

/* where to send --help output */
#define USAGE_FH stdout


/* LOCAL VARIABLES */

/* start and end time strings that user entered */
static const char *start_time;
static const char *end_time;

/* where to write output */
static sk_fileptr_t output;

static char *pager;

/* load-schemes */
static const sk_stringmap_entry_t load_schemes[] = {
    {"time-proportional",       LOAD_DURATION,  NULL,
     "split volume proportional to time active in bin"},
    {"bin-uniform",             LOAD_MEAN,      NULL,
     "split volume evenly across the bins"},
    {"start-spike",             LOAD_START,     NULL,
     "add complete volume to bin at start time"},
    {"middle-spike",            LOAD_MIDDLE,    NULL,
     "add complete volume to bin at midpoint (by time)"},
    {"end-spike",               LOAD_END,       NULL,
     "add complete volume to bin at end time"},
    {"maximum-volume",          LOAD_MAXIMUM,   NULL,
     "add complete volume to every bin"},
    {"minimum-volume",          LOAD_MINIMUM,   NULL,
     "add volume only when record in is single bin"},
    SK_STRINGMAP_SENTINEL
};

/* timestamp formats: the first of these will be the default */
static const sk_stringmap_entry_t timestamp_names[] = {
    {"default", 0,                    NULL, "yyyy/mm/ddThh:mm:ss.sss"},
    {"iso",     SKTIMESTAMP_ISO,      NULL, "yyyy-mm-dd hh:mm:ss.sss"},
    {"m/d/y",   SKTIMESTAMP_MMDDYYYY, NULL, "mm/dd/yyyy hh:mm:ss.sss"},
    {"epoch",   SKTIMESTAMP_EPOCH,    NULL,
     "seconds since UNIX epoch; ignores timezone"},
    SK_STRINGMAP_SENTINEL
};
static const sk_stringmap_entry_t timestamp_zones[] = {
    {"utc",     SKTIMESTAMP_UTC,      NULL, "use UTC"},
    {"local",   SKTIMESTAMP_LOCAL,    NULL,
     "use TZ environment variable or local timezone"},
    SK_STRINGMAP_SENTINEL
};
#if 0
static const sk_stringmap_entry_t timestamp_misc[] = {
    {"no-msec", SKTIMESTAMP_NOMSEC,   NULL, "truncate milliseconds"},
    SK_STRINGMAP_SENTINEL
};
#endif


/* OPTIONS SETUP */

typedef enum {
    OPT_BIN_SIZE, OPT_LOAD_SCHEME,
    OPT_START_TIME, OPT_END_TIME, OPT_SKIP_ZEROES,
    OPT_BIN_SLOTS, OPT_EPOCH_SLOTS,
    OPT_TIMESTAMP_FORMAT, OPT_NO_TITLES, OPT_NO_COLUMNS,
    OPT_COLUMN_SEPARATOR, OPT_NO_FINAL_DELIMITER, OPT_DELIMITED,
    OPT_OUTPUT_PATH, OPT_PAGER, OPT_LEGACY_TIMESTAMPS
} appOptionsEnum;

static const struct option appOptions[] = {
    {"bin-size",            REQUIRED_ARG, 0, OPT_BIN_SIZE},
    {"load-scheme",         REQUIRED_ARG, 0, OPT_LOAD_SCHEME},
    {"start-time",          REQUIRED_ARG, 0, OPT_START_TIME},
    {"end-time",            REQUIRED_ARG, 0, OPT_END_TIME},
    {"skip-zeroes",         NO_ARG,       0, OPT_SKIP_ZEROES},
    {"bin-slots",           NO_ARG,       0, OPT_BIN_SLOTS},
    {"epoch-slots",         NO_ARG,       0, OPT_EPOCH_SLOTS},
    {"timestamp-format",    REQUIRED_ARG, 0, OPT_TIMESTAMP_FORMAT},
    {"no-titles",           NO_ARG,       0, OPT_NO_TITLES},
    {"no-columns",          NO_ARG,       0, OPT_NO_COLUMNS},
    {"column-separator",    REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR},
    {"no-final-delimiter",  NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"delimited",           OPTIONAL_ARG, 0, OPT_DELIMITED},
    {"output-path",         REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {"pager",               REQUIRED_ARG, 0, OPT_PAGER},
    {"legacy-timestamps",   OPTIONAL_ARG, 0, OPT_LEGACY_TIMESTAMPS},
    {0,0,0,0}               /* sentinel entry */
};

static const char *appHelp[] = {
    "Set size of bins in seconds; may be fractional. Def. 30.000",
    NULL, /* generated dynamically */
    "Print bins from this time forward. Def. First nonzero bin",
    "Print bins until this time. Def. Last nonzero bin",
    "Do not print bins that have no flows. Def. Print all",
    "Print bin labels using the internal bin index. Def. No",
    "Print bin labels using epoch time. Def. Human readable",
    NULL, /* generated dynamically */
    "Do not print column titles. Def. Print titles",
    "Disable fixed-width columnar output. Def. Columnar",
    "Use specified character between columns. Def. '|'",
    "Suppress column delimiter at end of line. Def. No",
    "Shortcut for --no-columns --no-final-del --column-sep=CHAR",
    "Send output to given file path. Def. stdout",
    "Program to invoke to page output. Def. $SILK_PAGER or $PAGER",
    "DEPRECATED. Equivalent to --timestamp-format=m/d/y",
    (char *)NULL
};

static const struct option deprecatedOptions[] = {
    {"start-epoch",         REQUIRED_ARG, 0, OPT_START_TIME},
    {"end-epoch",           REQUIRED_ARG, 0, OPT_END_TIME},
    {0,0,0,0}               /* sentinel entry */
};

static const char *deprecatedHelp[] = {
    "DEPRECATED. Alias for --start-time",
    "DEPRECATED. Alias for --end-time",
    (char *)NULL
};

/* Allow any abbreviation of "--start-" and "--end-" to work */
static const struct option deprecatedOptionsShort[] = {
    {"start-",              REQUIRED_ARG, 0, OPT_START_TIME},
    {"start",               REQUIRED_ARG, 0, OPT_START_TIME},
    {"star",                REQUIRED_ARG, 0, OPT_START_TIME},
    {"sta",                 REQUIRED_ARG, 0, OPT_START_TIME},
    {"st",                  REQUIRED_ARG, 0, OPT_START_TIME},
    /* "--s" can be --start-time or --skip-zeroes */
    {"end-",                REQUIRED_ARG, 0, OPT_END_TIME},
    {"end",                 REQUIRED_ARG, 0, OPT_END_TIME},
    {"en",                  REQUIRED_ARG, 0, OPT_END_TIME},
    /* "--e" can be --end-time or --epoch-slots */
    {0,0,0,0}               /* sentinel entry */
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  loadschemeParse(const char *format, bin_load_scheme_enum_t *ls);
static void loadschemeUsage(FILE *fh);
static int  timestampFormatParse(const char *format, uint32_t *out_flags);
static void timestampFormatUsage(FILE *fh);


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
    ("[SWITCHES] [FILES]\n"                                                   \
     "\tSummarize SiLK Flow records across time, producing textual output\n"  \
     "\twith counts of bytes, packets, and flow records for each time bin.\n" \
     "\tWhen no files given on command line, flows are read from STDIN.\n")

    FILE *fh = USAGE_FH;
    int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);

    for (i = 0; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch (appOptions[i].val) {
          case OPT_LOAD_SCHEME:
            loadschemeUsage(fh);
            break;
          case OPT_TIMESTAMP_FORMAT:
            timestampFormatUsage(fh);
            break;
          default:
            fprintf(fh, "%s\n", appHelp[i]);
            break;
        }
    }
    for (i = 0; deprecatedOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. %s\n", deprecatedOptions[i].name,
                SK_OPTION_HAS_ARG(deprecatedOptions[i]), deprecatedHelp[i]);
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
    static int teardownFlag = 0;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* free our memory */
    if (bins.data) {
        free(bins.data);
    }

    /* close the output file or process */
    if (output.of_name) {
        skFileptrClose(&output, &skAppPrintErr);
    }

    /* close the copy-stream */
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
    sktime_t t;
    int start_precision = -1;
    int end_precision = -1;
    int64_t bin_count;
    int rv;

    /* make sure count of option's declarations and help-strings match */
    assert((sizeof(appOptions)/sizeof(struct option)) ==
           (sizeof(appHelp)/sizeof(char *)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    memset(&flags, 0, sizeof(flags));
    flags.delimiter = '|';
    flags.load_scheme = DEFAULT_LOAD_SCHEME;

    memset(&output, 0, sizeof(output));
    output.of_fp = stdout;

    memset(&bins, 0, sizeof(bins));
    bins.start_time = RWCO_UNINIT_START;
    bins.end_time = RWCO_UNINIT_END;
    bins.size = DEFAULT_BINSIZE;

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS | SK_OPTIONS_CTX_PRINT_FILENAMES
                    | SK_OPTIONS_CTX_COPY_INPUT);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsRegister(deprecatedOptions, &appOptionsHandler, NULL)
        || skOptionsRegister(deprecatedOptionsShort, &appOptionsHandler, NULL)
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

    /* parse options; print usage if error */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* parse the epoch times */
    if (start_time) {
        rv = skStringParseDatetime(&(bins.start_time), start_time,
                                   &start_precision);
        if (rv) {
            skAppPrintErr("Invalid %s '%s': %s",
                          appOptions[OPT_START_TIME].name, start_time,
                          skStringParseStrerror(rv));
            skAppUsage();
        }
    }

    if (end_time) {
        rv = skStringParseDatetime(&t, end_time, &end_precision);
        if (rv) {
            skAppPrintErr("Invalid %s '%s': %s",
                          appOptions[OPT_END_TIME].name, end_time,
                          skStringParseStrerror(rv));
            skAppUsage();
        }

        if (start_time) {
            /* move end-time to its ceiling */
            if (start_precision > end_precision) {
                end_precision = start_precision;
            }
            skDatetimeCeiling(&t, &t, end_precision);
            ++t;

            /* verify times */
            if (t <= bins.start_time) {
                char buf_s[SKTIMESTAMP_STRLEN];
                char buf_e[SKTIMESTAMP_STRLEN];
                skAppPrintErr("The %s is less than %s: %s < %s",
                              appOptions[OPT_END_TIME].name,
                              appOptions[OPT_START_TIME].name,
                              sktimestamp_r(buf_e, t, SKTIMESTAMP_NOMSEC),
                              sktimestamp_r(buf_s, bins.start_time,
                                            SKTIMESTAMP_NOMSEC));
                exit(EXIT_FAILURE);
            }

            /* make certain end_time fails on a boundary */
            bin_count = ((t - bins.start_time) / bins.size);
            if (t > (bins.start_time + bins.size * bin_count)) {
                /* make end_time fail on a bin boundary */
                ++bin_count;
                t = bins.start_time + bins.size * bin_count;
            }
            bins.end_time = t;
        } else {
            /* when only end_time is given, create bins up to its
             * ceiling value */
            bins.end_time = t;
            skDatetimeCeiling(&t, &t, end_precision);
            ++t;
            bin_count = ((t - bins.end_time) / bins.size);
            bins.end_time += bin_count * bins.size;
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

    /* open the --output-path: the 'of_name' member is non-NULL when
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

    return;                     /* OK */
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
    double opt_double;
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_LOAD_SCHEME:
        if (loadschemeParse(opt_arg, &flags.load_scheme)) {
            return 1;
        }
        break;

      case OPT_BIN_SIZE:
        rv = skStringParseDouble(&opt_double, opt_arg, 0.001, INT32_MAX);
        if (rv) {
            goto PARSE_ERROR;
        }
        bins.size = (sktime_t)(1000.0 * opt_double);
        break;

      case OPT_EPOCH_SLOTS:
        rv = timestampFormatParse("epoch", &flags.timeflags);
        assert(0 == rv);
        break;

      case OPT_BIN_SLOTS:
        flags.label_index = 1;
        break;

      case OPT_START_TIME:
        if (start_time != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
        }
        start_time = opt_arg;
        break;

      case OPT_END_TIME:
        if (end_time != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
        }
        end_time = opt_arg;
        break;

      case OPT_SKIP_ZEROES:
        flags.skip_zeroes = 1;
        break;

      case OPT_NO_TITLES:
        flags.no_titles = 1;
        break;

      case OPT_NO_COLUMNS:
        flags.no_columns = 1;
        break;

      case OPT_NO_FINAL_DELIMITER:
        flags.no_final_delimiter = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        flags.delimiter = opt_arg[0];
        break;

      case OPT_DELIMITED:
        flags.no_columns = 1;
        flags.no_final_delimiter = 1;
        if (opt_arg) {
            flags.delimiter = opt_arg[0];
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

      case OPT_TIMESTAMP_FORMAT:
        if (timestampFormatParse(opt_arg, &flags.timeflags)) {
            return 1;
        }
        break;

      case OPT_LEGACY_TIMESTAMPS:
        if ((opt_arg == NULL) || (opt_arg[0] == '\0') || (opt_arg[0] == '1')) {
            rv = timestampFormatParse("m/d/y", &flags.timeflags);
            assert(0 == rv);
        } else {
            rv = timestampFormatParse(timestamp_names[0].name,
                                      &flags.timeflags);
            assert(0 == rv);
        }
        break;
    }

    return 0;                     /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  appOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;
}


/*
 *  status = loadschemeParse(scheme_name, load_scheme);
 *
 *    Parse the load-scheme name in 'scheme_name' and set
 *    'load_scheme' to the result of parsing the string.  Return 0 on
 *    success, or -1 if parsing of the value fails.
 */
static int
loadschemeParse(
    const char             *scheme_name,
    bin_load_scheme_enum_t *load_scheme)
{
    char buf[128];
    sk_stringmap_entry_t new_entry;
    sk_stringmap_t *str_map = NULL;
    sk_stringmap_status_t sm_err;
    sk_stringmap_entry_t *sm_entry;
    const sk_stringmap_entry_t *e;
    int rv = -1;

    memset(&new_entry, 0, sizeof(new_entry));
    new_entry.name = buf;

    /* create a stringmap of the available load-scheme names */
    if (SKSTRINGMAP_OK != skStringMapCreate(&str_map)) {
        skAppPrintOutOfMemory(NULL);
        goto END;
    }
    if (skStringMapAddEntries(str_map, -1, load_schemes) != SKSTRINGMAP_OK){
        skAppPrintOutOfMemory(NULL);
        goto END;
    }

    /* allow the integer ID of each load-scheme to work */
    for (e = load_schemes; e->name; ++e) {
        new_entry.id = e->id;
        snprintf(buf, sizeof(buf), "%u", e->id);
        if (skStringMapAddEntries(str_map, 1, &new_entry) != SKSTRINGMAP_OK) {
            skAppPrintOutOfMemory(NULL);
            goto END;
        }
    }

    /* attempt to match */
    sm_err = skStringMapGetByName(str_map, scheme_name, &sm_entry);
    switch (sm_err) {
      case SKSTRINGMAP_OK:
        *load_scheme = (bin_load_scheme_enum_t)sm_entry->id;
        rv = 0;
        break;

      case SKSTRINGMAP_PARSE_AMBIGUOUS:
        skAppPrintErr("Invalid %s: '%s' is ambiguous",
                      appOptions[OPT_LOAD_SCHEME].name, scheme_name);
        break;

      case SKSTRINGMAP_PARSE_NO_MATCH:
        skAppPrintErr("Invalid %s: '%s' is not recognized",
                      appOptions[OPT_LOAD_SCHEME].name, scheme_name);
        break;

      default:
        skAppPrintErr("Unexpected return value from string-map parser (%d)",
                      sm_err);
        break;
    }

  END:
    if (str_map) {
        skStringMapDestroy(str_map);
    }
    return rv;
}


/*
 *  loadschemeUsage(fh);
 *
 *    Print the description of the argument to the --load-scheme
 *    switch to the 'fh' file handle.
 */
static void
loadschemeUsage(
    FILE               *fh)
{
    const sk_stringmap_entry_t *e;
    char buf[128];

    /* Find name of the default load-scheme */
    for (e = load_schemes; e->name; ++e) {
        if (DEFAULT_LOAD_SCHEME == e->id) {
            break;
        }
    }
    if (NULL == e->name) {
        skAbort();
    }

    fprintf(fh, "Split a record's volume (bytes & packets) among the\n"
            "\tbins it spans using this scheme. Def. %s. Choices:\n",
            e->name);
    for (e = load_schemes; e->name; ++e) {
        if (e->userdata) {
            snprintf(buf, sizeof(buf), "%s,%u", e->name, e->id);
            fprintf(fh, "\t  %-19s - %s\n",
                    buf, (const char*)e->userdata);
        }
    }
}


/*
 *  status = timestampFormatParse(format_string, out_flags);
 *
 *    Parse the comma-separated list of timestamp format strings
 *    contained in 'format_string' and set 'out_flags' to the result
 *    of parsing the string.  Return 0 on success, or -1 if parsing of
 *    the values fails or conflicting values are given.
 */
static int
timestampFormatParse(
    const char         *format,
    uint32_t           *out_flags)
{
    char buf[256];
    char *errmsg;
    sk_stringmap_t *str_map = NULL;
    sk_stringmap_iter_t *iter = NULL;
    sk_stringmap_entry_t *found_entry;
    const sk_stringmap_entry_t *entry;
    int name_seen = 0;
    int zone_seen = 0;
    int rv = -1;

    /* create a stringmap of the available timestamp formats */
    if (SKSTRINGMAP_OK != skStringMapCreate(&str_map)) {
        skAppPrintOutOfMemory(NULL);
        goto END;
    }
    if (skStringMapAddEntries(str_map, -1, timestamp_names) != SKSTRINGMAP_OK){
        skAppPrintOutOfMemory(NULL);
        goto END;
    }
    if (skStringMapAddEntries(str_map, -1, timestamp_zones) != SKSTRINGMAP_OK){
        skAppPrintOutOfMemory(NULL);
        goto END;
    }

    /* attempt to match */
    if (skStringMapParse(str_map, format, SKSTRINGMAP_DUPES_ERROR,
                         &iter, &errmsg))
    {
        skAppPrintErr("Invalid %s: %s",
                      appOptions[OPT_TIMESTAMP_FORMAT].name, errmsg);
        goto END;
    }

    *out_flags = 0;

    while (skStringMapIterNext(iter, &found_entry, NULL) == SK_ITERATOR_OK) {
        *out_flags |= found_entry->id;
        switch (found_entry->id) {
#if 0
          case SKTIMESTAMP_NOMSEC:
            break;
#endif
          case 0:
          case SKTIMESTAMP_EPOCH:
          case SKTIMESTAMP_ISO:
          case SKTIMESTAMP_MMDDYYYY:
            if (name_seen) {
                entry = timestamp_names;
                strncpy(buf, entry->name, sizeof(buf));
                for (++entry; entry->name; ++entry) {
                    strncat(buf, ",", sizeof(buf)-strlen(buf)-1);
                    strncat(buf, entry->name, sizeof(buf)-strlen(buf)-1);
                }
                skAppPrintErr("Invalid %s: May only specify one of %s",
                              appOptions[OPT_TIMESTAMP_FORMAT].name, buf);
                goto END;
            }
            name_seen = 1;
            break;

          case SKTIMESTAMP_UTC:
          case SKTIMESTAMP_LOCAL:
            if (zone_seen) {
                entry = timestamp_zones;
                strncpy(buf, entry->name, sizeof(buf));
                for (++entry; entry->name; ++entry) {
                    strncat(buf, ",", sizeof(buf)-strlen(buf)-1);
                    strncat(buf, entry->name, sizeof(buf)-strlen(buf)-1);
                }
                skAppPrintErr("Invalid %s: May only specify one of %s",
                              appOptions[OPT_TIMESTAMP_FORMAT].name, buf);
                goto END;
            }
            zone_seen = 1;
            break;

          default:
            skAbortBadCase(found_entry->id);
        }
    }

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
 *  timestampFormatUsage(fh);
 *
 *    Print the description of the argument to the --timestamp-format
 *    switch to the 'fh' file handle.
 */
static void
timestampFormatUsage(
    FILE               *fh)
{
    const sk_stringmap_entry_t *e;
    const char *label;

    fprintf(fh, "Print times in specified format: Def. %s,%s\n",
            timestamp_names[0].name,
            timestamp_zones[(SK_ENABLE_LOCALTIME != 0)].name);

    label = "Format:";
    for (e = timestamp_names; e->name; ++e) {
        fprintf(fh, "\t%-10s%-8s - %s\n",
                label, e->name, (const char*)e->userdata);
        label = "";
    }
    label = "Timezone:";
    for (e = timestamp_zones; e->name; ++e) {
        fprintf(fh, "\t%-10s%-8s - %s\n",
                label, e->name, (const char*)e->userdata);
        label = "";
    }
#if 0
    label = "Misc:";
    for (e = timestamp_misc; e->name; ++e) {
        fprintf(fh, "\t%-10s%-8s - %s\n",
                label, e->name, (const char*)e->userdata);
        label = "";
    }
#endif  /* 0 */
}


/*
 *  fp = getOutputHandle();
 *
 *    Return the file handle to use for output.
 */
FILE *
getOutputHandle(
    void)
{
    int rv;

    /* only invoke the pager when the user has not specified the
     * output-path, even if output-path is stdout */
    if (NULL == output.of_name) {
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
