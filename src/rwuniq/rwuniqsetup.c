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
**  rwuniqsetup.c
**
**  Application setup for rwuniq.  See rwuniq.c for a description.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwuniqsetup.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skcountry.h>
#include <silk/skplugin.h>
#include <silk/skprefixmap.h>
#include <silk/sksite.h>
#include <silk/skstringmap.h>
#include "rwuniq.h"


/* TYPEDEFS AND DEFINES */

/* file handle for --help usage message */
#define USAGE_FH stdout

/* where to write filenames if --print-file specified */
#define PRINT_FILENAMES_FH  stderr

/* suffix for distinct fields */
#define DISTINCT_SUFFIX  "-Distinct"

/* type of field being defined */
typedef enum field_type_en {
    FIELD_TYPE_KEY, FIELD_TYPE_VALUE, FIELD_TYPE_DISTINCT
} field_type_t;


/* LOCAL VARIABLES */

/* create aliases for exisitng value fields.  the struct contains the
 * name of the alias and an ID to match in the builtin_values[]
 * array */
static const struct builtin_value_aliases_st {
    const char     *ba_name;
    sk_fieldid_t    ba_id;
} builtin_value_aliases[] = {
    {"Flows",   SK_FIELD_RECORDS},
    {NULL,      (sk_fieldid_t)0}
};

/* key fields used when parsing the user's --fields switch */
static sk_stringmap_t *key_field_map = NULL;

/* available aggregate value fields */
static sk_stringmap_t *value_field_map = NULL;

/* the text the user entered for the --fields switch */
static const char *fields_arg = NULL;

/* the text the user entered for the --values switch */
static const char *values_arg = NULL;

/* the output stream */
static sk_fileptr_t output;

/* name of program to run to page output */
static char *pager;

/* where to copy the input to */
static skstream_t *copy_input = NULL;

/* temporary directory */
static const char *temp_directory = NULL;

/* how to print IP addresses */
static uint32_t ip_format = SKIPADDR_CANONICAL;

/* how to print timestamps */
static uint32_t time_flags = SKTIMESTAMP_NOMSEC;

/* the floor of the sTime and/or eTime */
static sktime_t time_bin_size = 0;

/* which of elapsed, sTime, and eTime were requested. uses the
 * PARSE_KEY_* values from rwuniq.h.  this value will be used to
 * initialize the 'time_fields_key' global. */
static unsigned int time_fields;

/* delimiter between output columns */
static char delimiter = '|';

/* input checker */
static sk_options_ctx_t *optctx = NULL;

/* fields that get defined just like plugins */
static const struct app_static_plugins_st {
    const char         *name;
    skplugin_setup_fn_t setup_fn;
} app_static_plugins[] = {
    {"addrtype",        skAddressTypesAddFields},
    {"ccfilter",        skCountryAddFields},
    {"pmapfilter",      skPrefixMapAddFields},
    {NULL, NULL}        /* sentinel */
};

/* plug-ins to attempt to load at startup */
static const char *app_plugin_names[] = {
    SK_PLUGIN_ADD_SUFFIX("silkpython"),
    NULL /* sentinel */
};

/* non-zero if we are shutting down due to a signal; controls whether
 * errors are printed in appTeardown(). */
static int caught_signal = 0;

/* timestamp formats: the first of these will be the default */
static const sk_stringmap_entry_t timestamp_names[] = {
    {"default", 0,                    NULL, "yyyy/mm/ddThh:mm:ss"},
    {"iso",     SKTIMESTAMP_ISO,      NULL, "yyyy-mm-dd hh:mm:ss"},
    {"m/d/y",   SKTIMESTAMP_MMDDYYYY, NULL, "mm/dd/yyyy hh:mm:ss"},
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


/* OPTIONS */

typedef enum {
    OPT_HELP_FIELDS,
    OPT_FIELDS,
    OPT_VALUES,
    OPT_PLUGIN,
    OPT_ALL_COUNTS,
    /* OPT_BYTES...OPT_DIP_DISTINCT must be contiguous and appear in
     * same order as in builtin_values[] */
    OPT_BYTES,
    OPT_PACKETS,
    OPT_FLOWS,
    OPT_STIME,
    OPT_ETIME,
    OPT_SIP_DISTINCT,
    OPT_DIP_DISTINCT,
    OPT_PRESORTED_INPUT,
    OPT_SORT_OUTPUT,
    OPT_BIN_TIME,
    OPT_TIMESTAMP_FORMAT,
    OPT_EPOCH_TIME,
    OPT_INTEGER_SENSORS,
    OPT_INTEGER_TCP_FLAGS,
    OPT_NO_TITLES,
    OPT_NO_COLUMNS,
    OPT_COLUMN_SEPARATOR,
    OPT_NO_FINAL_DELIMITER,
    OPT_DELIMITED,
    OPT_PRINT_FILENAMES,
    OPT_COPY_INPUT,
    OPT_OUTPUT_PATH,
    OPT_PAGER,
    OPT_LEGACY_TIMESTAMPS
} appOptionsEnum;


static struct option appOptions[] = {
    {"help-fields",         NO_ARG,       0, OPT_HELP_FIELDS},
    {"fields",              REQUIRED_ARG, 0, OPT_FIELDS},
    {"values",              REQUIRED_ARG, 0, OPT_VALUES},
    {"plugin",              REQUIRED_ARG, 0, OPT_PLUGIN},
    {"all-counts",          NO_ARG,       0, OPT_ALL_COUNTS},
    {"bytes",               OPTIONAL_ARG, 0, OPT_BYTES},
    {"packets",             OPTIONAL_ARG, 0, OPT_PACKETS},
    {"flows",               OPTIONAL_ARG, 0, OPT_FLOWS},
    {"stime",               NO_ARG,       0, OPT_STIME},
    {"etime",               NO_ARG,       0, OPT_ETIME},
    {"sip-distinct",        OPTIONAL_ARG, 0, OPT_SIP_DISTINCT},
    {"dip-distinct",        OPTIONAL_ARG, 0, OPT_DIP_DISTINCT},
    {"presorted-input",     NO_ARG,       0, OPT_PRESORTED_INPUT},
    {"sort-output",         NO_ARG,       0, OPT_SORT_OUTPUT},
    {"bin-time",            OPTIONAL_ARG, 0, OPT_BIN_TIME},
    {"timestamp-format",    REQUIRED_ARG, 0, OPT_TIMESTAMP_FORMAT},
    {"epoch-time",          NO_ARG,       0, OPT_EPOCH_TIME},
    {"integer-sensors",     NO_ARG,       0, OPT_INTEGER_SENSORS},
    {"integer-tcp-flags",   NO_ARG,       0, OPT_INTEGER_TCP_FLAGS},
    {"no-titles",           NO_ARG,       0, OPT_NO_TITLES},
    {"no-columns",          NO_ARG,       0, OPT_NO_COLUMNS},
    {"column-separator",    REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR},
    {"no-final-delimiter",  NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"delimited",           OPTIONAL_ARG, 0, OPT_DELIMITED},
    {"print-filenames",     NO_ARG,       0, OPT_PRINT_FILENAMES},
    {"copy-input",          REQUIRED_ARG, 0, OPT_COPY_INPUT},
    {"output-path",         REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {"pager",               REQUIRED_ARG, 0, OPT_PAGER},
    {"legacy-timestamps",   OPTIONAL_ARG, 0, OPT_LEGACY_TIMESTAMPS},
    {0,0,0,0}               /* sentinel entry */
};

static const char *appHelp[] = {
    "Describe each possible field and value and exit. Def. no",
    ("Use these fields as the grouping key. Specify fields as a\n"
     "\tcomma-separated list of names, IDs, and/or ID-ranges"),
    ("Compute these values for each group. Def. records\n"
     "\tSpecify values as a comma-separated list of names"),
    ("Load given plug-in to add fields and/or values. Switch may\n"
     "\tbe repeated to load multiple plug-ins. Def. None"),
    ("Enable the next five switches--count everything.  If no\n"
     "\tcount is specified, flows are counted.  Def. No"),
    ("Sum bytes in each bin; optionally choose to print\n"
     "\tbins whose total is in given range; range is MIN or MIN-MAX. Def. No"),
    ("Sum packets in each bin; optionally choose to print\n"
     "\tbins whose total is in given range; range is MIN or MIN-MAX. Def. No"),
    ("Count flow records in each bin; optionally choose to print\n"
     "\tbins whose count is in given range; range is MIN or MIN-MAX. Def. No"),
    "Print earliest time flow was seen in each bin. Def. No",
    "Print latest time flow was seen  in each bin. Def. No",
    ("Count distinct sIPs in each bin; optionally choose to\n"
     "\tprint bins whose count is in range; range is MIN or MIN-MAX. Def. No"),
    ("Count distinct dIPs in each bin; optionally choose to\n"
     "\tprint bins whose count is in range; range is MIN or MIN-MAX. Def. No"),
    ("Assume input has been presorted using\n"
     "\trwsort invoked with the exact same --fields value. Def. No"),
    ("Present the output in sorted order. Def. No"),
    ("When using 'sTime' or 'eTime' as a key, adjust time(s) to\n"
     "\tto appear in N-second bins (floor of time is used). Def. No, "),
    NULL, /* generated dynamically */
    "DEPRECATED. Equivalent to --timestamp-format=epoch",
    "Print sensor as an integer. Def. Sensor name",
    "Print TCP Flags as an integer. Def. No",
    "Do not print column titles. Def. Print titles",
    "Disable fixed-width columnar output. Def. Columnar",
    "Use specified character between columns. Def. '|'",
    "Suppress column delimiter at end of line. Def. No",
    "Shortcut for --no-columns --no-final-del --column-sep=CHAR",
    "Print names of input files as they are opened. Def. No",
    "Copy all input SiLK Flows to given pipe or file. Def. No",
    "Send output to given file path. Def. stdout",
    "Program to invoke to page output. Def. $SILK_PAGER or $PAGER",
    "DEPRECATED. Equivalent to --timestamp-format=m/d/y",
    (char *)NULL
};



/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static void appHandleSignal(int sig);

static int  timestampFormatParse(const char* opt_arg, uint32_t *out_flags);
static void timestampFormatUsage(FILE *fh);
static void helpFields(FILE *fh);

static int  createStringmaps(void);
static int  parseKeyFields(const char *field_string);
static int  parseValueFields(const char *field_string);
static int
appAddPlugin(
    skplugin_field_t   *pi_field,
    field_type_t        field_type);
static int
isFieldDuplicate(
    const sk_fieldlist_t   *flist,
    sk_fieldid_t            fid,
    const void             *fcontext);
static int  prepareFileForRead(skstream_t *rwios);


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
    FILE *fh = USAGE_FH;
    int i;

#define USAGE_MSG                                                             \
    ("--fields=N [SWITCHES] [FILES]\n"                                        \
     "\tSummarize SiLK Flow records into user-defined keyed bins specified\n" \
     "\twith the --fields switch.  For each keyed bin, print byte, packet,\n" \
     "\tand/or flow counts and/or the time window when key was active.\n"     \
     "\tWhen no files are given on command line, flows are read from STDIN.\n")

    /* Create the string maps for --fields and --values */
    createStringmaps();

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);

    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    for (i = 0; appOptions[i].name; i++) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch ((appOptionsEnum)i) {
          case OPT_FIELDS:
            /* Dynamically build the help */
            fprintf(fh, "%s\n", appHelp[i]);
            skStringMapPrintUsage(key_field_map, fh, 4);
            break;
          case OPT_VALUES:
            fprintf(fh, "%s\n", appHelp[i]);
            skStringMapPrintUsage(value_field_map, fh, 4);
            break;
          case OPT_BIN_TIME:
            fprintf(fh, "%s%d\n", appHelp[i], DEFAULT_TIME_BIN);
            break;
          case OPT_TIMESTAMP_FORMAT:
            timestampFormatUsage(fh);
            skOptionsIPFormatUsage(fh);
            break;
          default:
            /* Simple help text from the appHelp array */
            fprintf(fh, "%s\n", appHelp[i]);
            break;
        }
    }

    skOptionsCtxOptionsUsage(optctx, fh);
    skIPv6PolicyUsage(fh);
    skOptionsTempDirUsage(fh);
    sksiteOptionsUsage(fh);
    skPluginOptionsUsage(fh);
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
    char *path;
    int rv;
    int j;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    memset(&app_flags, 0, sizeof(app_flags));
    memset(&output, 0, sizeof(output));
    output.of_fp = stdout;

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS);

    /* initialize plugin library */
    skPluginSetup(2, SKPLUGIN_APP_UNIQ_FIELD, SKPLUGIN_APP_UNIQ_VALUE);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsTempDirRegister(&temp_directory)
        || skOptionsIPFormatRegister(&ip_format)
        || skIPv6PolicyOptionsRegister(&ipv6_policy)
        || sksiteOptionsRegister(SK_SITE_FLAG_CONFIG_FILE))
    {
        skAppPrintErr("Unable to register options");
        appExit(EXIT_FAILURE);
    }

    /* register the teardown handler */
    if (atexit(appTeardown) < 0) {
        skAppPrintErr("Unable to register appTeardown() with atexit()");
        appExit(EXIT_FAILURE);
    }

    /* try to load hard-coded plugins */
    for (j = 0; app_static_plugins[j].name; ++j) {
        skPluginAddAsPlugin(app_static_plugins[j].name,
                            app_static_plugins[j].setup_fn);
    }
    for (j = 0; app_plugin_names[j]; ++j) {
        skPluginLoadPlugin(app_plugin_names[j], 0);
    }

    /* parse options */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names, but we
     * should not consider it a complete failure */
    sksiteConfigure(0);

    /* create the ascii stream and set its properties */
    if (rwAsciiStreamCreate(&ascii_str)) {
        skAppPrintErr("Unable to create ascii stream");
        appExit(EXIT_FAILURE);
    }
    rwAsciiSetDelimiter(ascii_str, delimiter);
    rwAsciiSetIPv6Policy(ascii_str, ipv6_policy);
    rwAsciiSetIPFormatFlags(ascii_str, ip_format);
    rwAsciiSetTimestampFlags(ascii_str, time_flags);
    if (app_flags.no_final_delimiter) {
        rwAsciiSetNoFinalDelimiter(ascii_str);
    }
    if (app_flags.no_titles) {
        rwAsciiSetNoTitles(ascii_str);
    }
    if (app_flags.no_columns) {
        rwAsciiSetNoColumns(ascii_str);
    }
    if (app_flags.integer_sensors) {
        rwAsciiSetIntegerSensors(ascii_str);
    }
    if (app_flags.integer_tcp_flags) {
        rwAsciiSetIntegerTcpFlags(ascii_str);
    }

    /* set up the key_field_map and value_field_map */
    if (createStringmaps()) {
        appExit(EXIT_FAILURE);
    }

    /* make sure the user specified at least one key field */
    if (fields_arg == NULL || fields_arg[0] == '\0') {
        skAppPrintErr("The --%s switch is required",
                      appOptions[OPT_FIELDS].name);
        skAppUsage();         /* never returns */
    }

    /* parse the --fields and --values switches */
    if (parseKeyFields(fields_arg)) {
        appExit(EXIT_FAILURE);
    }
    if (parseValueFields(values_arg)) {
        appExit(EXIT_FAILURE);
    }

    /* make certain stdout is not being used for multiple outputs */
    if (copy_input
        && ((0 == strcmp(skStreamGetPathname(copy_input), "-"))
            || (0 == strcmp(skStreamGetPathname(copy_input), "stdout"))))
    {
        if ((NULL == output.of_name)
            || (0 == strcmp(output.of_name, "-"))
            || (0 == strcmp(output.of_name, "stdout")))
        {
            skAppPrintErr("May not use stdout for multiple output streams");
            exit(EXIT_FAILURE);
        }
    }

    /* create and initialize the uniq object */
    if (app_flags.presorted_input) {
        if (skPresortedUniqueCreate(&ps_uniq)) {
            appExit(EXIT_FAILURE);
        }

        skPresortedUniqueSetTempDirectory(ps_uniq, temp_directory);
        skPresortedUniqueSetErrorFunction(ps_uniq, skAppPrintErr);

        if (skPresortedUniqueSetFields(ps_uniq, key_fields, distinct_fields,
                                       value_fields))
        {
            skAppPrintErr("Unable to set fields");
            appExit(EXIT_FAILURE);
        }

        while ((rv = skOptionsCtxNextArgument(optctx, &path)) == 0) {
            skPresortedUniqueAddInputFile(ps_uniq, path);
        }
        if (rv < 0) {
            appExit(EXIT_FAILURE);
        }

        skPresortedUniqueSetPostOpenFn(ps_uniq, prepareFileForRead);
        if (time_bin_size > 1) {
            skPresortedUniqueSetReadFn(ps_uniq, readRecord);
        }

    } else {
        if (skUniqueCreate(&uniq)) {
            appExit(EXIT_FAILURE);
        }
        if (app_flags.sort_output) {
            skUniqueSetSortedOutput(uniq);
        }

        skUniqueSetTempDirectory(uniq, temp_directory);
        skUniqueSetErrorFunction(uniq, skAppPrintErr);

        if (skUniqueSetFields(uniq, key_fields, distinct_fields, value_fields)
            || skUniquePrepareForInput(uniq))
        {
            skAppPrintErr("Unable to set fields");
            appExit(EXIT_FAILURE);
        }
    }

    /* open the --output-path.  the 'of_name' member is NULL if user
     * didn't get an output-path. */
    if (output.of_name) {
        rv = skFileptrOpen(&output, SK_IO_WRITE);
        if (rv) {
            skAppPrintErr("Unable to open %s '%s': %s",
                          appOptions[OPT_OUTPUT_PATH].name,
                          output.of_name, skFileptrStrerror(rv));
            appExit(EXIT_FAILURE);
        }
    }

    /* open the --copy-input destination */
    if (copy_input) {
        rv = skStreamOpen(copy_input);
        if (rv) {
            skStreamPrintLastErr(copy_input, rv, &skAppPrintErr);
            appExit(EXIT_FAILURE);
        }
    }

    /* set signal handler to clean up temp files on SIGINT, SIGTERM, etc */
    if (skAppSetSignalHandler(&appHandleSignal)) {
        appExit(EXIT_FAILURE);
    }

    return;                       /* OK */
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
    int rv;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    skUniqueDestroy(&uniq);
    skPresortedUniqueDestroy(&ps_uniq);

    /* destroy field lists */
    skFieldListDestroy(&key_fields);
    skFieldListDestroy(&distinct_fields);
    skFieldListDestroy(&value_fields);

    /* plugin teardown */
    skPluginRunCleanup(SKPLUGIN_FN_ANY);
    skPluginTeardown();

    /* destroy output */
    rwAsciiStreamDestroy(&ascii_str);

    /* close output */
    if (output.of_name) {
        skFileptrClose(&output, &skAppPrintErr);
    }
    /* close the --copy-input */
    if (copy_input) {
        rv = skStreamClose(copy_input);
        if (rv && rv != SKSTREAM_ERR_NOT_OPEN) {
            skStreamPrintLastErr(copy_input, rv, &skAppPrintErr);
        }
        skStreamDestroy(&copy_input);
    }

    /* destroy string maps for keys and values */
    if (key_field_map) {
        skStringMapDestroy(key_field_map);
        key_field_map = NULL;
    }
    if (value_field_map) {
        skStringMapDestroy(value_field_map);
        value_field_map = NULL;
    }

    skOptionsCtxDestroy(&optctx);
    skAppUnregister();
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
    uint32_t val32;
    size_t i;
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_HELP_FIELDS:
        helpFields(USAGE_FH);
        exit(EXIT_SUCCESS);

      case OPT_FIELDS:
        if (fields_arg) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        fields_arg = opt_arg;
        break;

      case OPT_VALUES:
        if (values_arg) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        values_arg = opt_arg;
        break;

      case OPT_ALL_COUNTS:
        for (i = 0; i < num_builtin_values; ++i) {
            if (builtin_values[i].bf_all_counts) {
                builtin_values[i].bf_switched_on = 1;
            }
        }
        break;

      case OPT_BYTES:
      case OPT_PACKETS:
      case OPT_FLOWS:
      case OPT_STIME:
      case OPT_ETIME:
      case OPT_SIP_DISTINCT:
      case OPT_DIP_DISTINCT:
        i = opt_index - OPT_BYTES;
        builtin_values[i].bf_switched_on = 1;
        if (opt_arg) {
            rv = skStringParseRange64(&builtin_values[i].bf_min,
                                      &builtin_values[i].bf_max,
                                      opt_arg, 0, 0,
                                      SKUTILS_RANGE_SINGLE_OPEN);
            if (rv) {
                goto PARSE_ERROR;
            }
            /* treat a single value as having no max, not as a range
             * of a single value */
            if ((builtin_values[i].bf_min == builtin_values[i].bf_max)
                && !strchr(opt_arg, '-'))
            {
                builtin_values[i].bf_max = UINT64_MAX;
            }
            app_flags.check_limits = 1;
        }
        break;

      case OPT_PLUGIN:
        if (skPluginLoadPlugin(opt_arg, 1) != 0) {
            skAppPrintErr("Unable to load %s as a plugin", opt_arg);
            return 1;
        }
        break;

      case OPT_BIN_TIME:
        if (opt_arg == NULL || opt_arg[0] == '\0') {
            /* no time given; use default */
            time_bin_size = sktimeCreate(DEFAULT_TIME_BIN, 0);
        } else {
            /* parse user's time */
            rv = skStringParseUint32(&val32, opt_arg, 1, 0);
            if (rv) {
                goto PARSE_ERROR;
            }
            time_bin_size = sktimeCreate(val32, 0);
        }
        break;

      case OPT_PRESORTED_INPUT:
        app_flags.presorted_input = 1;
        break;

      case OPT_SORT_OUTPUT:
        app_flags.sort_output = 1;
        break;

      case OPT_TIMESTAMP_FORMAT:
        if (timestampFormatParse(opt_arg, &time_flags)) {
            return 1;
        }
        break;

      case OPT_EPOCH_TIME:
        rv = timestampFormatParse("epoch", &time_flags);
        assert(0 == rv);
        break;

      case OPT_INTEGER_SENSORS:
        app_flags.integer_sensors = 1;
        break;

      case OPT_INTEGER_TCP_FLAGS:
        app_flags.integer_tcp_flags = 1;
        break;

      case OPT_NO_TITLES:
        app_flags.no_titles = 1;
        break;

      case OPT_NO_COLUMNS:
        app_flags.no_columns = 1;
        break;

      case OPT_NO_FINAL_DELIMITER:
        app_flags.no_final_delimiter = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        delimiter = opt_arg[0];
        break;

      case OPT_DELIMITED:
        app_flags.no_columns = 1;
        app_flags.no_final_delimiter = 1;
        if (opt_arg) {
            delimiter = opt_arg[0];
        }
        break;

      case OPT_PRINT_FILENAMES:
        app_flags.print_filenames = 1;
        break;

      case OPT_COPY_INPUT:
        if (copy_input) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if ((rv=skStreamCreate(&copy_input, SK_IO_WRITE, SK_CONTENT_SILK_FLOW))
            || (rv = skStreamBind(copy_input, opt_arg)))
        {
            skStreamPrintLastErr(copy_input, rv, &skAppPrintErr);
            return 1;
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

      case OPT_LEGACY_TIMESTAMPS:
        if ((opt_arg == NULL) || (opt_arg[0] == '\0') || (opt_arg[0] == '1')) {
            rv = timestampFormatParse("m/d/y", &time_flags);
            assert(0 == rv);
        } else {
            rv = timestampFormatParse(timestamp_names[0].name, &time_flags);
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
 *  appExit(status)
 *
 *  Exit the application with the given status.
 */
void
appExit(
    int                 status)
{
    appTeardown();
    exit(status);
}


/*
 *  appHandleSignal(signal_value)
 *
 *    Call appExit() to exit the program.  If signal_value is SIGPIPE,
 *    close cleanly; otherwise print a message that we've caught the
 *    signal and exit with EXIT_FAILURE.
 */
static void
appHandleSignal(
    int                 sig)
{
    caught_signal = 1;

    if (sig == SIGPIPE) {
        /* we get SIGPIPE if something downstream, like rwcut, exits
         * early, so don't bother to print a warning, and exit
         * successfully */
        appExit(EXIT_SUCCESS);
    } else {
        skAppPrintErr("Caught signal..cleaning up and exiting");
        appExit(EXIT_FAILURE);
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

    *out_flags = SKTIMESTAMP_NOMSEC;

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
 *  helpFields(fh);
 *
 *    Print a description of each field to the 'fh' file pointer
 */
static void
helpFields(
    FILE               *fh)
{
    if (createStringmaps()) {
        exit(EXIT_FAILURE);
    }

    fprintf(fh,
            ("The following names may be used in the --%s switch. Names are\n"
             "case-insensitive and may be abbreviated to the shortest"
             " unique prefix.\n"),
            appOptions[OPT_FIELDS].name);
    skStringMapPrintDetailedUsage(key_field_map, fh);

    fprintf(fh,
            ("\n"
             "The following names may be used in the --%s switch. Names are\n"
             "case-insensitive and may be abbreviated to the shortest"
             " unique prefix.\n"),
            appOptions[OPT_VALUES].name);
    skStringMapPrintDetailedUsage(value_field_map, fh);
}


/*
 *  builtin_value_get_title(buf, bufsize, field_entry);
 *
 *    Invoked by rwAsciiPrintTitles() to get the title for an
 *    aggregate value field represented by an sk_fieldid_t.
 *
 *    Fill 'buf' with the title for the column represented by the
 *    field list entry 'field_entry'.  This function should write no
 *    more than 'bufsize' characters to 'buf'.
 */
static void
builtin_value_get_title(
    char               *text_buf,
    size_t              text_buf_size,
    void               *v_fl_entry)
{
    sk_fieldentry_t *fl_entry = (sk_fieldentry_t*)v_fl_entry;
    builtin_field_t *bf;

    bf = (builtin_field_t*)skFieldListEntryGetContext(fl_entry);
    strncpy(text_buf, bf->bf_title, text_buf_size);
}

/*
 *  value_to_ascii(rwrec, buf, bufsize, field_entry, extra);
 *
 *    Invoked by rwAsciiPrintRecExtra() to get the value for an
 *    aggregate value field.  This function is called for built-in
 *    aggregate values as well as plug-in defined values.
 *
 *    Fill 'buf' with the value for the column represented by the
 *    aggregate value field list entry 'field_entry'.  'rwrec' is
 *    ignored; 'extra' is an array[3] that contains the buffers for
 *    the key, aggregate value, and distinct field-lists.  This
 *    function should write no more than 'bufsize' characters to
 *    'buf'.
 */
static int
value_to_ascii(
    const rwRec UNUSED(*rwrec),
    char               *text_buf,
    size_t              text_buf_size,
    void               *v_fl_entry,
    void               *v_outbuf)
{
    sk_fieldentry_t *fl_entry = (sk_fieldentry_t*)v_fl_entry;
    uint64_t val64;
    uint32_t val32;
    uint8_t bin_buf[HASHLIB_MAX_VALUE_WIDTH];

    switch (skFieldListEntryGetId(fl_entry)) {
      case SK_FIELD_SUM_BYTES:
      case SK_FIELD_SUM_PACKETS:
        skFieldListExtractFromBuffer(value_fields, ((uint8_t**)v_outbuf)[1],
                                     fl_entry, (uint8_t*)&val64);
        snprintf(text_buf, text_buf_size, ("%" PRIu64), val64);
        break;

      case SK_FIELD_RECORDS:
      case SK_FIELD_SUM_ELAPSED:
        skFieldListExtractFromBuffer(value_fields, ((uint8_t**)v_outbuf)[1],
                                     fl_entry, (uint8_t*)&val32);
        snprintf(text_buf, text_buf_size, ("%" PRIu32), val32);
        break;

      case SK_FIELD_MIN_STARTTIME:
      case SK_FIELD_MAX_ENDTIME:
        skFieldListExtractFromBuffer(value_fields, ((uint8_t**)v_outbuf)[1],
                                     fl_entry, (uint8_t*)&val32);
        assert(text_buf_size > SKTIMESTAMP_STRLEN);
        sktimestamp_r(text_buf, sktimeCreate(val32, 0), time_flags);
        break;

      case SK_FIELD_CALLER:
        /* get the binary value from the field-list */
        skFieldListExtractFromBuffer(value_fields, ((uint8_t**)v_outbuf)[1],
                                     fl_entry, bin_buf);
        /* call the plug-in to convert from binary to text */
        skPluginFieldRunBinToTextFn(
            (skplugin_field_t*)skFieldListEntryGetContext(fl_entry),
            text_buf, text_buf_size, bin_buf);
        break;

      default:
        skAbortBadCase(skFieldListEntryGetId(fl_entry));
    }

    return 0;
}

/*
 *  builtin_distinct_get_title(buf, bufsize, field_entry);
 *
 *    Invoked by rwAsciiPrintTitles() to get the title for a distinct
 *    field represented by an sk_fieldid_t.
 *
 *    Fill 'buf' with the title for the column represented by the
 *    field list entry 'field_entry'.  This function should write no
 *    more than 'bufsize' characters to 'buf'.
 */
static void
builtin_distinct_get_title(
    char               *text_buf,
    size_t              text_buf_size,
    void               *v_fl_entry)
{
    sk_fieldentry_t *fl_entry = (sk_fieldentry_t*)v_fl_entry;
    int id;
    size_t sz;

    id = skFieldListEntryGetId(fl_entry);
    switch (id) {
      case SK_FIELD_SIPv4:
      case SK_FIELD_SIPv6:
        rwAsciiGetFieldName(text_buf, text_buf_size, RWREC_FIELD_SIP);
        break;
      case SK_FIELD_DIPv4:
      case SK_FIELD_DIPv6:
        rwAsciiGetFieldName(text_buf, text_buf_size, RWREC_FIELD_DIP);
        break;
      case SK_FIELD_NHIPv4:
      case SK_FIELD_NHIPv6:
        rwAsciiGetFieldName(text_buf, text_buf_size, RWREC_FIELD_NHIP);
        break;
      default:
        rwAsciiGetFieldName(text_buf, text_buf_size,
                            (rwrec_printable_fields_t)id);
        break;
    }
    sz = strlen(text_buf);
    strncpy(text_buf+sz, DISTINCT_SUFFIX, text_buf_size-sz);
    text_buf[text_buf_size-1] = '\0';
}

/*
 *  distinct_to_ascii(rwrec, buf, bufsize, field_entry, extra);
 *
 *    Invoked by rwAsciiPrintRecExtra() to get the value for a
 *    distinct field.  This function is called for built-in distinct
 *    fields as well as those from a plug-in.
 *
 *    Fill 'buf' with the value for the column represented by the
 *    distinct field list entry 'field_entry'.  'rwrec' is ignored;
 *    'extra' is an array[3] that contains the buffers for the key,
 *    aggregate value, and distinct field-lists.  This function should
 *    write no more than 'bufsize' characters to 'buf'.
 */
static int
distinct_to_ascii(
    const rwRec UNUSED(*rwrec),
    char               *text_buf,
    size_t              text_buf_size,
    void               *v_fl_entry,
    void               *v_outbuf)
{
    sk_fieldentry_t *fl_entry = (sk_fieldentry_t*)v_fl_entry;
    size_t len;
    union value_un {
        uint8_t   ar[HASHLIB_MAX_VALUE_WIDTH];
        uint64_t  u64;
        uint32_t  u32;
        uint16_t  u16;
        uint8_t   u8;
    } value;

    len = skFieldListEntryGetBinOctets(fl_entry);
    switch (len) {
      case 1:
        skFieldListExtractFromBuffer(distinct_fields, ((uint8_t**)v_outbuf)[2],
                                     fl_entry, &value.u8);
        snprintf(text_buf, text_buf_size, ("%" PRIu8), value.u8);
        break;
      case 2:
        skFieldListExtractFromBuffer(distinct_fields, ((uint8_t**)v_outbuf)[2],
                                     fl_entry, (uint8_t*)&value.u16);
        snprintf(text_buf, text_buf_size, ("%" PRIu16), value.u16);
        break;
      case 4:
        skFieldListExtractFromBuffer(distinct_fields, ((uint8_t**)v_outbuf)[2],
                                     fl_entry, (uint8_t*)&value.u32);
        snprintf(text_buf, text_buf_size, ("%" PRIu32), value.u32);
        break;
      case 8:
        skFieldListExtractFromBuffer(distinct_fields, ((uint8_t**)v_outbuf)[2],
                                     fl_entry, (uint8_t*)&value.u64);
        snprintf(text_buf, text_buf_size, ("%" PRIu64), value.u64);
        break;

      case 3:
      case 5:
      case 6:
      case 7:
        value.u64 = 0;
#if SK_BIG_ENDIAN
        skFieldListExtractFromBuffer(distinct_fields, ((uint8_t**)v_outbuf)[2],
                                     fl_entry, &value.ar[8 - len]);
#else
        skFieldListExtractFromBuffer(distinct_fields, ((uint8_t**)v_outbuf)[2],
                                     fl_entry, &value.ar[0]);
#endif  /* #else of #if SK_BIG_ENDIAN */
        snprintf(text_buf, text_buf_size, ("%" PRIu64), value.u64);
        break;

      default:
        skFieldListExtractFromBuffer(distinct_fields, ((uint8_t**)v_outbuf)[2],
                                     fl_entry, value.ar);
        snprintf(text_buf, text_buf_size, ("%" PRIu64), value.u64);
        break;
    }

    return 0;
}

/*
 *  plugin_get_title(buf, buf_size, field_entry);
 *
 *    Invoked by rwAsciiPrintTitles() to get the title for a key or
 *    aggregate value field defined by a plug-in.
 *
 *    Fill 'buf' with the title for the column represented by the
 *    plug-in associated with 'field_entry'.  This function should
 *    write no more than 'bufsize' characters to 'buf'.
 */
static void
plugin_get_title(
    char               *text_buf,
    size_t              text_buf_size,
    void               *v_fl_entry)
{
    sk_fieldentry_t *fl_entry = (sk_fieldentry_t*)v_fl_entry;
    const char *title;

    skPluginFieldTitle(
        (skplugin_field_t*)skFieldListEntryGetContext(fl_entry), &title);
    strncpy(text_buf, title, text_buf_size);
    text_buf[text_buf_size-1] = '\0';
}

/*
 *  plugin_distinct_get_title(buf, bufsize, field_entry);
 *
 *    Invoked by rwAsciiPrintTitles() to get the title for a distinct
 *    field.
 *
 *    Fill 'buf' with the title for the column represented by a
 *    distinct count over the plug-in associated with 'field_entry'.
 *    This function should write no more than 'bufsize' characters to
 *    'buf'.
 */
static void
plugin_distinct_get_title(
    char               *text_buf,
    size_t              text_buf_size,
    void               *v_fl_entry)
{
    sk_fieldentry_t *fl_entry = (sk_fieldentry_t*)v_fl_entry;
    const char *title;

    skPluginFieldTitle(
        (skplugin_field_t*)skFieldListEntryGetContext(fl_entry), &title);
    snprintf(text_buf, text_buf_size, ("%s" DISTINCT_SUFFIX), title);
}

/*
 *  plugin_key_to_ascii(rwrec, buf, bufsize, keyfield, extra);
 *
 *    Invoked by rwAsciiPrintRecExtra() to get the value for a key
 *    field that is defined by a plug-in.
 *
 *    Fill 'buf' with a textual representation of the key for the
 *    column represented by the plug-in associated with 'field_entry'.
 *    'rwrec' is ignored; 'extra' is an array[3] that contains the
 *    buffers for the key, aggregate value, and distinct field-lists.
 *    This function should write no more than 'bufsize' characters to
 *    'buf'.
 */
static int
plugin_key_to_ascii(
    const rwRec UNUSED(*rwrec),
    char               *text_buf,
    size_t              text_buf_size,
    void               *v_fl_entry,
    void               *v_outbuf)
{
    sk_fieldentry_t *fl_entry = (sk_fieldentry_t*)v_fl_entry;
    uint8_t bin_buf[HASHLIB_MAX_KEY_WIDTH];

    /* get the binary value from the field-list */
    skFieldListExtractFromBuffer(key_fields, ((uint8_t**)v_outbuf)[0],
                                 fl_entry, bin_buf);

    /* call the plug-in to convert from binary to text */
    skPluginFieldRunBinToTextFn(
        (skplugin_field_t*)skFieldListEntryGetContext(fl_entry),
        text_buf, text_buf_size, bin_buf);

    return 0;
}

/*
 *  plugin_rec_to_bin(rwrec, out_buf, plugin_field);
 *
 *    Invoked by skFieldListRecToBinary() to get the binary value,
 *    based on the given 'rwrec', for a key field that is defined by a
 *    plug-in.
 *
 *    The size of 'out_buf' was specified when the field was added to
 *    the field-list.
 */
static void
plugin_rec_to_bin(
    const rwRec        *rwrec,
    uint8_t            *out_buf,
    void               *v_pi_field)
{
    skPluginFieldRunRecToBinFn((skplugin_field_t*)v_pi_field,
                               out_buf, rwrec, NULL);
}

/*
 *  plugin_add_rec_to_bin(rwrec, in_out_buf, plugin_field);
 *
 *    Invoked by skFieldListAddRecToBinary() to get the binary value,
 *    based on the given 'rwrec' and merge that with the current
 *    binary value for a key field that is defined by a plug-in.
 *
 *    The size of 'out_buf' was specified when the field was added to
 *    the field-list.
 */
static void
plugin_add_rec_to_bin(
    const rwRec        *rwrec,
    uint8_t            *in_out_buf,
    void               *v_pi_field)
{
    skPluginFieldRunAddRecToBinFn((skplugin_field_t*)v_pi_field,
                                  in_out_buf, rwrec, NULL);
}

/*
 *  plugin_bin_compare(buf1, buf2, plugin_field);
 *
 *    Invoked by skFieldListCompareBuffers() to compare current value
 *    of the key or aggregate value fields specified by 'buf1' and
 *    'buf2'.
 *
 *    The size of 'buf1' and 'buf2' were specified when the field was
 *    added to the field-list.
 */
static int
plugin_bin_compare(
    const uint8_t      *buf1,
    const uint8_t      *buf2,
    void               *v_pi_field)
{
    int val = 0;
    skPluginFieldRunBinCompareFn((skplugin_field_t*)v_pi_field,
                                 &val, buf1, buf2);
    return val;
}

/*
 *  plugin_bin_merge(in_out_buf, in_buf, plugin_field);
 *
 *    Invoked by skFieldListMergeBuffers() to merge the current values
 *    of the key or aggregate value fields specified by 'in_out_buf'
 *    and 'in_buf'.  The merged value should be placed into
 *    'in_out_buf'.
 *
 *    The size of 'in_out_buf' and 'in_buf' were specified when the
 *    field was added to the field-list.
 */
static void
plugin_bin_merge(
    uint8_t            *in_out_buf,
    const uint8_t      *in_buf,
    void               *v_pi_field)
{
    skPluginFieldRunBinMergeFn((skplugin_field_t*)v_pi_field,
                               in_out_buf, in_buf);
}


/*
 *  ok = createStringmaps();
 *
 *    Create the string-maps to assist in parsing the --fields and
 *    --values switches.
 */
static int
createStringmaps(
    void)
{
    skplugin_field_iter_t  pi_iter;
    skplugin_err_t         pi_err;
    skplugin_field_t      *pi_field;
    sk_stringmap_status_t  sm_err;
    sk_stringmap_entry_t   sm_entry;
    const char           **field_names;
    const char           **name;
    uint32_t               max_id;
    size_t                 i;
    size_t                 j;

    /* initialize string-map of field identifiers: add default fields,
     * then remove millisec fields, since unique-ing over them makes
     * little sense.
     *
     * Note that although we remove the MSEC fields from the available
     * fields here, the remainder of the code still supports MSEC
     * fields---which are mapped onto the non-MSEC versions of the
     * fields. */
    if (rwAsciiFieldMapAddDefaultFields(&key_field_map)) {
        skAppPrintErr("Unable to setup fields stringmap");
        return -1;
    }
    (void)skStringMapRemoveByID(key_field_map, RWREC_FIELD_STIME_MSEC);
    (void)skStringMapRemoveByID(key_field_map, RWREC_FIELD_ETIME_MSEC);
    (void)skStringMapRemoveByID(key_field_map, RWREC_FIELD_ELAPSED_MSEC);
    max_id = RWREC_PRINTABLE_FIELD_COUNT - 1;

    /* add "icmpTypeCode" field */
    ++max_id;
    if (rwAsciiFieldMapAddIcmpTypeCode(key_field_map, max_id)) {
        skAppPrintErr("Unable to add icmpTypeCode");
        return -1;
    }

    /* add --fields from the plug-ins */
    pi_err = skPluginFieldIteratorBind(&pi_iter, SKPLUGIN_APP_UNIQ_FIELD, 1);
    if (pi_err != SKPLUGIN_OK) {
        assert(pi_err == SKPLUGIN_OK);
        skAppPrintErr("Unable to bind plugin field iterator");
        return -1;
    }

    while (skPluginFieldIteratorNext(&pi_iter, &pi_field)) {
        skPluginFieldName(pi_field, &field_names);
        ++max_id;

        /* Add keys to the key_field_map */
        for (name = field_names; *name; name++) {
            memset(&sm_entry, 0, sizeof(sm_entry));
            sm_entry.name = *name;
            sm_entry.id = max_id;
            sm_entry.userdata = pi_field;
            skPluginFieldDescription(pi_field, &sm_entry.description);
            sm_err = skStringMapAddEntries(key_field_map, 1, &sm_entry);
            if (sm_err != SKSTRINGMAP_OK) {
                const char *plugin_name;
                skPluginFieldGetPluginName(pi_field, &plugin_name);
                skAppPrintErr(("Plug-in cannot add field named '%s': %s."
                               " Plug-in file: %s"),
                              *name, skStringMapStrerror(sm_err),plugin_name);
                return -1;
            }
        }
    }


    max_id = 0;

    /* create the string-map for value field identifiers */
    if (skStringMapCreate(&value_field_map)) {
        skAppPrintErr("Unable to create map for values");
        return -1;
    }

    /* add the built-in names */
    for (i = 0; i < num_builtin_values; ++i) {
        memset(&sm_entry, 0, sizeof(sk_stringmap_entry_t));
        sm_entry.name = builtin_values[i].bf_title;
        sm_entry.id = i;
        sm_entry.description = builtin_values[i].bf_description;
        sm_err = skStringMapAddEntries(value_field_map, 1, &sm_entry);
        if (sm_err) {
            skAppPrintErr("Unable to add value field named '%s': %s",
                          sm_entry.name, skStringMapStrerror(sm_err));
            return -1;
        }
        if (sm_entry.id > max_id) {
            max_id = sm_entry.id;
        }
    }

    /* add aliases for built-in fields */
    for (j = 0; builtin_value_aliases[j].ba_name; ++j) {
        for (i = 0; i < num_builtin_values; ++i) {
            if (builtin_value_aliases[j].ba_id == builtin_values[i].bf_id) {
                memset(&sm_entry, 0, sizeof(sk_stringmap_entry_t));
                sm_entry.name = builtin_value_aliases[j].ba_name;
                sm_entry.id = i;
                sm_err = skStringMapAddEntries(value_field_map, 1, &sm_entry);
                if (sm_err) {
                    skAppPrintErr("Unable to add value field named '%s': %s",
                                  sm_entry.name, skStringMapStrerror(sm_err));
                    return -1;
                }
                break;
            }
        }
        if (i == num_builtin_values) {
            skAppPrintErr("No field found with id %d",
                          builtin_value_aliases[j].ba_id);
            return -1;
        }
    }

    /* add the value fields from the plugins */
    pi_err = skPluginFieldIteratorBind(&pi_iter, SKPLUGIN_APP_UNIQ_VALUE, 1);
    assert(pi_err == SKPLUGIN_OK);

    while (skPluginFieldIteratorNext(&pi_iter, &pi_field)) {
        skPluginFieldName(pi_field, &field_names);
        ++max_id;

        /* Add value names to the field_map */
        for (name = field_names; *name; ++name) {
            memset(&sm_entry, 0, sizeof(sm_entry));
            sm_entry.name = *name;
            sm_entry.id = max_id;
            sm_entry.userdata = pi_field;
            skPluginFieldDescription(pi_field, &sm_entry.description);
            sm_err = skStringMapAddEntries(value_field_map, 1, &sm_entry);
            if (sm_err != SKSTRINGMAP_OK) {
                const char *plugin_name;
                skPluginFieldGetPluginName(pi_field, &plugin_name);
                skAppPrintErr(("Plug-in cannot add value named '%s': %s."
                               " Plug-in file: %s"),
                              *name, skStringMapStrerror(sm_err),plugin_name);
                return -1;
            }
        }
    }

    return 0;
}


/*
 *  status = parseKeyFields(field_string);
 *
 *    Parse the string that represents the key fields the user wishes
 *    to bin by, create and fill in the global sk_fieldlist_t
 *    'key_fields', and add columns to the rwAsciiStream.  Return 0 on
 *    success or non-zero on error.
 */
static int
parseKeyFields(
    const char         *field_string)
{
    sk_stringmap_iter_t *sm_iter = NULL;
    sk_stringmap_entry_t *sm_entry = NULL;
    sk_fieldentry_t *fl_entry;

    /* keep track of which time field we see last; uses the
     * RWREC_FIELD_* values from rwascii.h */
    rwrec_printable_fields_t final_time_field = (rwrec_printable_fields_t)0;

    /* keep track of which ICMP field(s) we see */
    int have_icmp_type_code = 0;

    /* return value; assume failure */
    int rv = -1;

    /* error message generated when parsing fields */
    char *errmsg;

    /* the field IDs for SIP, DIP, and NHIP depend on the ipv6 policy */
    sk_fieldid_t ip_fields[3] = {
        SK_FIELD_SIPv4, SK_FIELD_DIPv4, SK_FIELD_NHIPv4
    };

#if SK_ENABLE_IPV6
    if (ipv6_policy >= SK_IPV6POLICY_MIX) {
        ip_fields[0] = SK_FIELD_SIPv6;
        ip_fields[1] = SK_FIELD_DIPv6;
        ip_fields[2] = SK_FIELD_NHIPv6;
    }
#endif  /* SK_ENABLE_IPV6 */

    /* parse the --fields argument */
    if (skStringMapParse(key_field_map, field_string, SKSTRINGMAP_DUPES_ERROR,
                         &sm_iter, &errmsg))
    {
        skAppPrintErr("Invalid %s: %s",
                      appOptions[OPT_FIELDS].name, errmsg);
        goto END;
    }

    /* create the field-list */
    if (skFieldListCreate(&key_fields)) {
        skAppPrintErr("Unable to create key field list");
        goto END;
    }

    /* see which time fields and ICMP fields are requested */
    while (skStringMapIterNext(sm_iter, &sm_entry, NULL) == SK_ITERATOR_OK) {
        switch (sm_entry->id) {
          case RWREC_FIELD_DPORT:
            dport_key = 1;
            break;
          case RWREC_FIELD_STIME:
          case RWREC_FIELD_STIME_MSEC:
            time_fields |= PARSE_KEY_STIME;
            final_time_field = (rwrec_printable_fields_t)sm_entry->id;
            break;
          case RWREC_FIELD_ELAPSED:
          case RWREC_FIELD_ELAPSED_MSEC:
            time_fields |= PARSE_KEY_ELAPSED;
            final_time_field = (rwrec_printable_fields_t)sm_entry->id;
            break;
          case RWREC_FIELD_ETIME:
          case RWREC_FIELD_ETIME_MSEC:
            time_fields |= PARSE_KEY_ETIME;
            final_time_field = (rwrec_printable_fields_t)sm_entry->id;
            break;
          case RWREC_FIELD_ICMP_TYPE:
          case RWREC_FIELD_ICMP_CODE:
            have_icmp_type_code |= 1;
            break;
          case RWREC_PRINTABLE_FIELD_COUNT:
            have_icmp_type_code |= 2;
            break;
          default:
            break;
        }
    }

    /* set 'time_fields_key' to the time fields that will be in the
     * key.  since only two of the three time fields are independent,
     * when all three are requested only the first two fields are put
     * into the key. */
    time_fields_key = time_fields;
    if (PARSE_KEY_ALL_TIMES == time_fields_key) {
        switch (final_time_field) {
          case RWREC_FIELD_STIME:
          case RWREC_FIELD_STIME_MSEC:
            time_fields_key &= ~PARSE_KEY_STIME;
            break;
          case RWREC_FIELD_ELAPSED:
          case RWREC_FIELD_ELAPSED_MSEC:
            time_fields_key &= ~PARSE_KEY_ELAPSED;
            break;
          case RWREC_FIELD_ETIME:
          case RWREC_FIELD_ETIME_MSEC:
            time_fields_key &= ~PARSE_KEY_ETIME;
            break;
          default:
            skAbortBadCase(final_time_field);
        }
    }

    /* when binning by time was requested, see if time fields make sense */
    if (time_bin_size != 0) {
        switch (time_fields) {
          case 0:
          case PARSE_KEY_ELAPSED:
            if (FILEIsATty(stderr)) {
                skAppPrintErr("Warning: Neither sTime nor eTime appear in"
                              " --%s; %s switch ignored",
                              appOptions[OPT_FIELDS].name,
                              appOptions[OPT_BIN_TIME].name);
            }
            time_bin_size = 0;
            break;
          case PARSE_KEY_ALL_TIMES:
            /* must adjust elapsed to be eTime-sTime */
            if (FILEIsATty(stderr)) {
                skAppPrintErr("Warning: Modifying duration field "
                              "to be difference of eTime and sTime");
            }
            break;
        }
    }

    /* warn when using --presorted-input and multiple time fields are
     * present or when the time field is not the final field */
    if (app_flags.presorted_input && FILEIsATty(stderr)) {
        switch (time_fields) {
          case 0:
            /* no time fields present */
            break;
          case PARSE_KEY_ELAPSED:
          case PARSE_KEY_STIME:
          case PARSE_KEY_ETIME:
            /* one field is present.  see if it is last.  note that
             * 'sm_entry' is still pointed at the final entry */
            switch (sm_entry->id) {
              case RWREC_FIELD_STIME:
              case RWREC_FIELD_STIME_MSEC:
              case RWREC_FIELD_ELAPSED:
              case RWREC_FIELD_ELAPSED_MSEC:
              case RWREC_FIELD_ETIME:
              case RWREC_FIELD_ETIME_MSEC:
                /* one field is present and it is last */
                break;
              default:
                /* one field is present but it is not last */
                skAppPrintErr(("Warning: Suggest putting '%s' last in --%s"
                               " when using --%s due to millisecond"
                               " truncation"),
                              ((PARSE_KEY_ELAPSED == time_fields)
                               ? "elapsed"
                               : ((PARSE_KEY_STIME == time_fields)
                                  ? "sTime"
                                  : "eTime")),
                              appOptions[OPT_FIELDS].name,
                              appOptions[OPT_PRESORTED_INPUT].name);
                break;
            }
            break;
          default:
            /* multiple time fields present */
            skAppPrintErr(("Warning: Using multiple time-related key"
                           " fields with\n\t--%s may lead to unexpected"
                           " results due to millisecond truncation"),
                          appOptions[OPT_PRESORTED_INPUT].name);
            break;
        }
    }

    /* handle legacy icmpTypeCode field */
    if (3 == have_icmp_type_code) {
        skAppPrintErr("Invalid %s: May not mix field %s with %s or %s",
                      appOptions[OPT_FIELDS].name,
                      skStringMapGetFirstName(
                          key_field_map, RWREC_PRINTABLE_FIELD_COUNT),
                      skStringMapGetFirstName(
                          key_field_map, RWREC_FIELD_ICMP_TYPE),
                      skStringMapGetFirstName(
                          key_field_map, RWREC_FIELD_ICMP_CODE));
        goto END;
    }

    skStringMapIterReset(sm_iter);

    /* add the key fields to the field-list and to the ascii stream. */
    while (skStringMapIterNext(sm_iter, &sm_entry, NULL) == SK_ITERATOR_OK) {
        if (sm_entry->userdata) {
            assert(sm_entry->id > RWREC_PRINTABLE_FIELD_COUNT);
            if (appAddPlugin((skplugin_field_t*)(sm_entry->userdata),
                             FIELD_TYPE_KEY))
            {
                skAppPrintErr("Cannot add key field '%s' from plugin",
                              sm_entry->name);
                goto END;
            }
            continue;
        }
        if (sm_entry->id == RWREC_PRINTABLE_FIELD_COUNT) {
            /* handle the icmpTypeCode field */
            rwrec_printable_fields_t icmp_fields[] = {
                RWREC_FIELD_ICMP_TYPE, RWREC_FIELD_ICMP_CODE
            };
            char name_buf[128];
            size_t k;

            for (k = 0; k < sizeof(icmp_fields)/sizeof(icmp_fields[0]); ++k) {
                if (rwAsciiAppendOneField(ascii_str, icmp_fields[k])
                    || !skFieldListAddKnownField(key_fields, icmp_fields[k],
                                                 NULL))
                {
                    rwAsciiGetFieldName(name_buf, sizeof(name_buf),
                                        icmp_fields[k]);
                    skAppPrintErr("Cannot add key field '%s' to stream",
                                  name_buf);
                    goto END;
                }
            }
            continue;
        }
        assert(sm_entry->id < RWREC_PRINTABLE_FIELD_COUNT);
        if (rwAsciiAppendOneField(ascii_str, sm_entry->id)) {
            skAppPrintErr("Cannot add key field '%s' to stream",
                          sm_entry->name);
            goto END;
        }
        if (PARSE_KEY_ALL_TIMES == time_fields
            && (rwrec_printable_fields_t)sm_entry->id == final_time_field)
        {
            /* when all time fields were requested, do not include the
             * final one that was seen as part of the key */
            continue;
        }
        switch ((rwrec_printable_fields_t)sm_entry->id) {
          case RWREC_FIELD_SIP:
            fl_entry = skFieldListAddKnownField(key_fields, ip_fields[0],NULL);
            break;
          case RWREC_FIELD_DIP:
            fl_entry = skFieldListAddKnownField(key_fields, ip_fields[1],NULL);
            break;
          case RWREC_FIELD_NHIP:
            fl_entry = skFieldListAddKnownField(key_fields, ip_fields[2],NULL);
            break;
          default:
            fl_entry = skFieldListAddKnownField(key_fields, sm_entry->id,NULL);
            break;
        }
        if (NULL == fl_entry) {
            skAppPrintErr("Cannot add key field '%s' to field list",
                          sm_entry->name);
            goto END;
        }
    }

    /* successful */
    rv = 0;

  END:
    if (rv != 0) {
        /* something went wrong.  clean up */
        if (key_fields) {
            skFieldListDestroy(&key_fields);
            key_fields = NULL;
        }
    }
    /* do standard clean-up */
    if (sm_iter != NULL) {
        skStringMapIterDestroy(sm_iter);
    }

    return rv;
}


/*
 *  ok = parseValueFields(field_string);
 *
 *    Parse the string that represents the aggregate value and
 *    distinct fields the user wishes to compute, create and fill in
 *    the global sk_fieldlist_t 'value_fields' and 'distinct_fields',
 *    and add columns to the rwAsciiStream.  Return 0 on success or
 *    non-zero on error.
 *
 *    Returns 0 on success, or non-zero on error.
 */
static int
parseValueFields(
    const char         *value_string)
{
    sk_stringmap_iter_t *sm_iter = NULL;
    sk_stringmap_entry_t *sm_entry;
    sk_stringmap_status_t sm_err;
    sk_stringmap_id_t sm_entry_id;
    const char *sm_attr;
    size_t i;

    /* to create a new --values switch */
    char *buf = NULL;
    size_t buf_size;

    /* return value; assume failure */
    int rv = -1;

    /* error message generated when parsing fields */
    char *errmsg;

    builtin_field_t *bf;
    sk_fieldentry_t *fl_entry;

#if SK_ENABLE_IPV6
    if (ipv6_policy >= SK_IPV6POLICY_MIX) {
        /* change the field id of the distinct fields */
        for (i = 0, bf = builtin_values; i < num_builtin_values; ++i, ++bf) {
            switch (bf->bf_id) {
              case SK_FIELD_SIPv4:
                bf->bf_id = SK_FIELD_SIPv6;
                break;
              case SK_FIELD_DIPv4:
                bf->bf_id = SK_FIELD_DIPv6;
                break;
              default:
                break;
            }
        }
    }
#endif  /* SK_ENABLE_IPV6 */

    if (time_flags & SKTIMESTAMP_EPOCH) {
        /* Reduce width of the textual columns for the MIN_STARTTIME
         * and MAX_ENDTIME fields. */
        for (i = 0, bf = builtin_values; i < num_builtin_values; ++i, ++bf) {
            if ((bf->bf_id == SK_FIELD_MIN_STARTTIME)
                || (bf->bf_id == SK_FIELD_MAX_ENDTIME))
            {
                bf->bf_text_len = 10;
            }
        }
    }

    /*
     *    Handling the old style --bytes,--packets,etc switches and
     *    the new --values switch is a bit of a pain.
     *
     *    First, parse --values if it is provided.  If any --values
     *    fields are also specified as stand-alone switches (e.g.,
     *    --bytes), turn off the stand-alone switch.
     *
     *    If any stand-alone switch is still on, create a new --values
     *    switch that includes the names of the stand-alone switches.
     *    Or, if no --values and no stand-alone switches are given,
     *    fall-back to the default and count flow records.
     */

    /* parse the --values field list if given */
    if (value_string) {
        if (skStringMapParseWithAttributes(value_field_map, value_string,
                                           SKSTRINGMAP_DUPES_KEEP, &sm_iter,
                                           &errmsg))
        {
            skAppPrintErr("Invalid %s: %s",
                          appOptions[OPT_VALUES].name, errmsg);
            goto END;
        }

        /* turn off the --bytes,--packets,etc switches if they also appear
         * in the --values switch */
        while (skStringMapIterNext(sm_iter, &sm_entry, NULL)==SK_ITERATOR_OK) {
            if (sm_entry->id < num_builtin_values) {
                builtin_values[sm_entry->id].bf_switched_on = 0;
            }
        }

        skStringMapIterDestroy(sm_iter);
        sm_iter = NULL;
    }

    /* determine whether there are any active --bytes,--packets,etc
     * switches */
    buf_size = 0;
    for (i = 0, bf = builtin_values; i < num_builtin_values; ++i, ++bf) {
        if (bf->bf_switched_on) {
            buf_size += 2 + strlen(bf->bf_title);
        }
    }

    if (buf_size) {
        /* switches are active; create new --values switch */
        if (NULL == value_string) {
            buf = (char*)malloc(buf_size);
            if (!buf) {
                skAppPrintOutOfMemory(NULL);
                goto END;
            }
            buf[0] = '\0';
        } else {
            buf_size += 1 + strlen(value_string);
            buf = (char*)malloc(buf_size);
            if (!buf) {
                skAppPrintOutOfMemory(NULL);
                goto END;
            }
            strncpy(buf, value_string, buf_size);
        }
        for (i = 0, bf = builtin_values; i < num_builtin_values; ++i, ++bf) {
            if (bf->bf_switched_on) {
                strncat(buf, ",", 2);
                strncat(buf, bf->bf_title, buf_size - 1 - strlen(buf));
            }
        }
        value_string = buf;

    } else if (!value_string) {
        /* no --values switch and no --bytes,--packets,etc switches,
         * so count flow records */
        for (i = 0, bf = builtin_values; i < num_builtin_values; ++i, ++bf) {
            if (SK_FIELD_RECORDS == bf->bf_id) {
                value_string = bf->bf_title;
                break;
            }
        }
    }

    /* parse the --values field list */
    if (skStringMapParseWithAttributes(value_field_map, value_string,
                                       SKSTRINGMAP_DUPES_KEEP, &sm_iter,
                                       &errmsg))
    {
        skAppPrintErr("Invalid %s: %s",
                      appOptions[OPT_VALUES].name, errmsg);
        goto END;
    }

    /* create the field-lists */
    if (skFieldListCreate(&value_fields)) {
        skAppPrintErr("Unable to create value field list");
        goto END;
    }
    if (skFieldListCreate(&distinct_fields)) {
        skAppPrintErr("Unable to create distinct field list");
        goto END;
    }

    /* loop over the selected values */
    while (skStringMapIterNext(sm_iter, &sm_entry, &sm_attr)==SK_ITERATOR_OK) {
        if (sm_entry->userdata) {
            /* this is a values field that comes from a plug-in */
            if (sm_attr[0]) {
                skAppPrintErr("Invalid %s: Extra text after field name ':%s'",
                              appOptions[OPT_VALUES].name, sm_attr);
                goto END;
            }
            if (isFieldDuplicate(value_fields, SK_FIELD_CALLER,
                                 sm_entry->userdata))
            {
                skAppPrintErr("Invalid %s: Duplicate name '%s'",
                              appOptions[OPT_VALUES].name, sm_entry->name);
                goto END;
            }
            if (appAddPlugin((skplugin_field_t*)(sm_entry->userdata),
                             FIELD_TYPE_VALUE))
            {
                skAppPrintErr("Cannot add value field '%s' from plugin",
                              sm_entry->name);
                goto END;
            }
            continue;
        }
        /* else, field is built-in */
        assert(sm_entry->id < num_builtin_values);
        bf = &builtin_values[sm_entry->id];
        if (0 == bf->bf_is_distinct) {
            /* this is a built-in values field; must have no attribute */
            if (sm_attr[0]) {
                skAppPrintErr("Invalid %s: Unrecognized field '%s:%s'",
                              appOptions[OPT_VALUES].name,
                              bf->bf_title, sm_attr);
                goto END;
            }
            if (isFieldDuplicate(value_fields, bf->bf_id, NULL)) {
                skAppPrintErr("Invalid %s: Duplicate name '%s'",
                              appOptions[OPT_VALUES].name, bf->bf_title);
                goto END;
            }
            fl_entry = skFieldListAddKnownField(value_fields, bf->bf_id, bf);
            if (NULL == fl_entry) {
                skAppPrintErr("Cannot add value field '%s' to field list",
                              sm_entry->name);
                goto END;
            }
            if (rwAsciiAppendCallbackFieldExtra(ascii_str,
                                                &builtin_value_get_title,
                                                &value_to_ascii,
                                                fl_entry, bf->bf_text_len))
            {
                skAppPrintErr("Cannot add value field '%s' to stream",
                              sm_entry->name);
                goto END;
            }
        } else if (SK_FIELD_CALLER != bf->bf_id) {
            /* one of the old sip-distinct,dip-distinct fields; must
             * have no attribute */
            if (sm_attr[0]) {
                skAppPrintErr("Invalid %s: Unrecognized field '%s:%s'",
                              appOptions[OPT_VALUES].name,
                              bf->bf_title, sm_attr);
                goto END;
            }
            /* is this a duplicate field? */
            if (isFieldDuplicate(distinct_fields, bf->bf_id, NULL)) {
                skAppPrintErr("Invalid %s: Duplicate name '%s'",
                              appOptions[OPT_VALUES].name, bf->bf_title);
                goto END;
            }
            fl_entry = skFieldListAddKnownField(distinct_fields, bf->bf_id,bf);
            if (NULL == fl_entry) {
                skAppPrintErr("Cannot add distinct field '%s' to field list",
                              sm_entry->name);
                goto END;
            }
            if (rwAsciiAppendCallbackFieldExtra(ascii_str,
                                                &builtin_distinct_get_title,
                                                &distinct_to_ascii,
                                                fl_entry, bf->bf_text_len))
            {
                skAppPrintErr("Cannot add distinct field '%s' to stream",
                              sm_entry->name);
                goto END;
            }
        } else {
            /* got a distinct:KEY field */
            if (!sm_attr[0]) {
                skAppPrintErr(("Invalid %s:"
                               " The distinct value requires a field"),
                              appOptions[OPT_VALUES].name);
                goto END;
            }
            /* need to parse KEY as a key field */
            sm_err = skStringMapGetByName(key_field_map, sm_attr, &sm_entry);
            if (sm_err) {
                if (strchr(sm_attr, ',')) {
                    skAppPrintErr(("Invalid %s:"
                                   " May only distinct over a single field"),
                                  appOptions[OPT_VALUES].name);
                } else {
                    skAppPrintErr("Invalid %s: Bad distinct field '%s': %s",
                                  appOptions[OPT_VALUES].name, sm_attr,
                                  skStringMapStrerror(sm_err));
                }
                goto END;
            }
            if (sm_entry->userdata) {
                /* distinct:KEY where KEY is from a plug-in */
                if (isFieldDuplicate(distinct_fields, SK_FIELD_CALLER,
                                     sm_entry->userdata))
                {
                    skAppPrintErr("Invalid %s: Duplicate distinct '%s'",
                                  appOptions[OPT_VALUES].name, sm_entry->name);
                    goto END;
                }
                if (appAddPlugin((skplugin_field_t*)(sm_entry->userdata),
                                 FIELD_TYPE_DISTINCT))
                {
                    skAppPrintErr("Cannot add distinct field '%s' from plugin",
                                  sm_entry->name);
                    goto END;
                }
                continue;
            }
            if (isFieldDuplicate(distinct_fields, (sk_fieldid_t)sm_entry->id,
                                 NULL))
            {
                skAppPrintErr("Invalid %s: Duplicate distinct '%s'",
                              appOptions[OPT_VALUES].name, sm_entry->name);
                goto END;
            }
            if (sm_entry->id == RWREC_PRINTABLE_FIELD_COUNT) {
                skAppPrintErr("Invalid %s: May not count distinct '%s' entries",
                              appOptions[OPT_VALUES].name, sm_entry->name);
                goto END;
            }
            sm_entry_id = sm_entry->id;
#if SK_ENABLE_IPV6
            if (ipv6_policy >= SK_IPV6POLICY_MIX) {
                /* make certain field can hold an IPv6 address */
                switch (sm_entry_id) {
                  case SK_FIELD_SIPv4:
                    sm_entry_id = SK_FIELD_SIPv6;
                    break;
                  case SK_FIELD_DIPv4:
                    sm_entry_id = SK_FIELD_DIPv6;
                    break;
                  case SK_FIELD_NHIPv4:
                    sm_entry_id = SK_FIELD_NHIPv6;
                    break;
                }
            }
#endif  /* #if SK_ENABLE_IPV6 */
            fl_entry = skFieldListAddKnownField(distinct_fields,
                                                sm_entry_id, NULL);
            if (NULL == fl_entry) {
                skAppPrintErr("Cannot add distinct field '%s' to field list",
                              sm_entry->name);
                goto END;
            }
            if (rwAsciiAppendCallbackFieldExtra(ascii_str,
                                                &builtin_distinct_get_title,
                                                &distinct_to_ascii,
                                                fl_entry, bf->bf_text_len))
            {
                skAppPrintErr("Cannot add distinct field '%s' to stream",
                              sm_entry->name);
                goto END;
            }
        }
    }

    rv = 0;

  END:
    /* do standard clean-up */
    if (sm_iter) {
        skStringMapIterDestroy(sm_iter);
    }
    if (buf) {
        free(buf);
    }
    if (rv != 0) {
        /* something went wrong. do additional clean-up */
        if (value_fields) {
            skFieldListDestroy(&value_fields);
            value_fields = NULL;
        }
        if (distinct_fields) {
            skFieldListDestroy(&distinct_fields);
            distinct_fields = NULL;
        }
    }

    return rv;
}


/*
 *  status = appAddPlugin(plugin_field, field_type);
 *
 *    Given a key, an aggregate value, or distinct(key) field defined
 *    in a plug-in, activate that field and get the information from
 *    the field that the application requires.  field_type indicates
 *    whether the field represents a key, an aggregate value, or a
 *    distinct field.
 *
 *    The function adds the field to the approprirate sk_fieldlist_t
 *    ('key_fields', 'value_fields', 'distinct_fields') and to the
 *    rwAsciiStream.
 */
static int
appAddPlugin(
    skplugin_field_t   *pi_field,
    field_type_t        field_type)
{
    uint8_t bin_buf[HASHLIB_MAX_VALUE_WIDTH];
    sk_fieldlist_entrydata_t regdata;
    sk_fieldentry_t *fl_entry;
    size_t text_width;
    skplugin_err_t pi_err;

    /* set the regdata for the sk_fieldlist_t */
    memset(&regdata, 0, sizeof(regdata));
    regdata.bin_compare = plugin_bin_compare;
    regdata.add_rec_to_bin = plugin_add_rec_to_bin;
    regdata.bin_merge = plugin_bin_merge;
    /* regdata.bin_output; */

    /* activate the field (so cleanup knows about it) */
    pi_err = skPluginFieldActivate(pi_field);
    if (pi_err != SKPLUGIN_OK) {
        return -1;
    }

    /* initialize this field */
    pi_err = skPluginFieldRunInitialize(pi_field);
    if (pi_err != SKPLUGIN_OK) {
        return -1;
    }

    /* get the required textual width of the column */
    pi_err = skPluginFieldGetLenText(pi_field, &text_width);
    if (pi_err != SKPLUGIN_OK) {
        return -1;
    }
    if (0 == text_width) {
        const char *title;
        skPluginFieldTitle(pi_field, &title);
        skAppPrintErr("Plug-in field '%s' has a textual width of 0",
                      title);
        return -1;
    }

    /* get the bin width for this field */
    pi_err = skPluginFieldGetLenBin(pi_field, &regdata.bin_octets);
    if (pi_err != SKPLUGIN_OK) {
        return -1;
    }
    if (0 == regdata.bin_octets) {
        const char *title;
        skPluginFieldTitle(pi_field, &title);
        skAppPrintErr("Plug-in field '%s' has a binary width of 0",
                      title);
        return -1;
    }
    if (regdata.bin_octets > HASHLIB_MAX_VALUE_WIDTH) {
        return -1;
    }

    memset(bin_buf, 0, sizeof(bin_buf));
    pi_err = skPluginFieldGetInitialValue(pi_field, bin_buf);
    if (pi_err != SKPLUGIN_OK) {
        return -1;
    }
    regdata.initial_value = bin_buf;

    switch (field_type) {
      case FIELD_TYPE_KEY:
        regdata.rec_to_bin = plugin_rec_to_bin;
        fl_entry = skFieldListAddField(key_fields, &regdata, (void*)pi_field);
        break;
      case FIELD_TYPE_VALUE:
        fl_entry = skFieldListAddField(value_fields, &regdata,(void*)pi_field);
        break;
      case FIELD_TYPE_DISTINCT:
        regdata.rec_to_bin = plugin_rec_to_bin;
        fl_entry = skFieldListAddField(distinct_fields, &regdata,
                                       (void*)pi_field);
        break;
      default:
        skAbortBadCase(field_type);
    }
    if (NULL == fl_entry) {
        skAppPrintErr("Unable to add field to field list");
        return -1;
    }

    switch (field_type) {
      case FIELD_TYPE_KEY:
        return rwAsciiAppendCallbackFieldExtra(ascii_str, &plugin_get_title,
                                               &plugin_key_to_ascii, fl_entry,
                                               text_width);
      case FIELD_TYPE_VALUE:
        return rwAsciiAppendCallbackFieldExtra(ascii_str, &plugin_get_title,
                                               &value_to_ascii, fl_entry,
                                               text_width);
      case FIELD_TYPE_DISTINCT:
        return rwAsciiAppendCallbackFieldExtra(ascii_str,
                                               &plugin_distinct_get_title,
                                               &distinct_to_ascii,
                                               fl_entry, text_width);
      default:
        skAbortBadCase(field_type);
    }

    return -1;                  /* NOTREACHED */
}


/*
 *  is_duplicate = isFieldDuplicate(flist, fid, fcontext);
 *
 *    Return 1 if the field-id 'fid' appears in the field-list
 *    'flist'.  If 'fid' is SK_FIELD_CALLER, return 1 when a field in
 *    'flist' has the id SK_FIELD_CALLER and its context object points
 *    to 'fcontext'.  Return 0 otherwise.
 *
 *    In this function, IPv4 and IPv6 fields are considered
 *    equivalent; that is, you cannot have both SK_FIELD_SIPv4 and
 *    SK_FIELD_SIPv6, and multiple SK_FIELD_CALLER fields are allowed.
 */
static int
isFieldDuplicate(
    const sk_fieldlist_t   *flist,
    sk_fieldid_t            fid,
    const void             *fcontext)
{
    sk_fieldlist_iterator_t fl_iter;
    sk_fieldentry_t *fl_entry;

    skFieldListIteratorBind(flist, &fl_iter);
    switch (fid) {
      case SK_FIELD_SIPv4:
      case SK_FIELD_SIPv6:
        while ((fl_entry = skFieldListIteratorNext(&fl_iter)) != NULL) {
            switch (skFieldListEntryGetId(fl_entry)) {
              case SK_FIELD_SIPv4:
              case SK_FIELD_SIPv6:
                return 1;
              default:
                break;
            }
        }
        break;

      case SK_FIELD_DIPv4:
      case SK_FIELD_DIPv6:
        while ((fl_entry = skFieldListIteratorNext(&fl_iter)) != NULL) {
            switch (skFieldListEntryGetId(fl_entry)) {
              case SK_FIELD_DIPv4:
              case SK_FIELD_DIPv6:
                return 1;
              default:
                break;
            }
        }
        break;

      case SK_FIELD_NHIPv4:
      case SK_FIELD_NHIPv6:
        while ((fl_entry = skFieldListIteratorNext(&fl_iter)) != NULL) {
            switch (skFieldListEntryGetId(fl_entry)) {
              case SK_FIELD_NHIPv4:
              case SK_FIELD_NHIPv6:
                return 1;
              default:
                break;
            }
        }
        break;

      case SK_FIELD_CALLER:
        while ((fl_entry = skFieldListIteratorNext(&fl_iter)) != NULL) {
            if ((skFieldListEntryGetId(fl_entry) == (uint32_t)fid)
                && (skFieldListEntryGetContext(fl_entry) == fcontext))
            {
                return 1;
            }
        }
        break;

      default:
        while ((fl_entry = skFieldListIteratorNext(&fl_iter)) != NULL) {
            if (skFieldListEntryGetId(fl_entry) == (uint32_t)fid) {
                return 1;
            }
        }
        break;
    }
    return 0;
}


static int
prepareFileForRead(
    skstream_t         *rwios)
{
    if (app_flags.print_filenames) {
        fprintf(PRINT_FILENAMES_FH, "%s\n", skStreamGetPathname(rwios));
    }
    if (copy_input) {
        skStreamSetCopyInput(rwios, copy_input);
    }
    skStreamSetIPv6Policy(rwios, ipv6_policy);

    return 0;
}


/*
 *  status = readRecord(stream, rwrec);
 *
 *    Fill 'rwrec' with a SiLK Flow record read from 'stream'.  Modify
 *    the times on the record if the user has requested time binning.
 *
 *    Return the status of reading the record.
 */
int
readRecord(
    skstream_t         *rwios,
    rwRec              *rwrec)
{
    sktime_t sTime;
    sktime_t sTime_mod;
    uint32_t elapsed;
    int rv;

    rv = skStreamReadRecord(rwios, rwrec);
    if (SKSTREAM_OK == rv
        && time_bin_size > 1)
    {
        switch (time_fields) {
          case PARSE_KEY_STIME:
          case (PARSE_KEY_STIME | PARSE_KEY_ELAPSED):
            /* adjust start time */
            sTime = rwRecGetStartTime(rwrec);
            sTime_mod = sTime % time_bin_size;
            rwRecSetStartTime(rwrec, (sTime - sTime_mod));
            break;
          case PARSE_KEY_ALL_TIMES:
          case (PARSE_KEY_STIME | PARSE_KEY_ETIME):
            /* adjust sTime and elapsed/duration */
            sTime = rwRecGetStartTime(rwrec);
            sTime_mod = sTime % time_bin_size;
            rwRecSetStartTime(rwrec, (sTime - sTime_mod));
            /*
             * the following sets elapsed to:
             * ((eTime - (eTime % bin_size)) - (sTime - (sTime % bin_size)))
             */
            elapsed = rwRecGetElapsed(rwrec);
            elapsed = (elapsed + sTime_mod
                       - ((sTime + elapsed) % time_bin_size));
            rwRecSetElapsed(rwrec, elapsed);
            break;
          case PARSE_KEY_ETIME:
          case (PARSE_KEY_ETIME | PARSE_KEY_ELAPSED):
            /* want to set eTime to (eTime - (eTime % bin_size)), but
             * eTime is computed as (sTime + elapsed) */
            sTime = rwRecGetStartTime(rwrec);
            rwRecSetStartTime(rwrec, (sTime - ((sTime + rwRecGetElapsed(rwrec))
                                               % time_bin_size)));
            break;
          case 0:
          case PARSE_KEY_ELAPSED:
          default:
            skAbortBadCase(time_fields);
        }
    }

    return rv;
}


/*
 *  int = appNextInput(&rwios);
 *
 *    Fill 'rwios' with the next input file to read.  Return 0 if
 *    'rwios' was successfully opened, 1 if there are no more input
 *    files, or -1 if an error was encountered.
 */
int
appNextInput(
    skstream_t        **rwios)
{
    char *path = NULL;
    int rv;

    rv = skOptionsCtxNextArgument(optctx, &path);
    if (0 == rv) {
        rv = skStreamOpenSilkFlow(rwios, path, SK_IO_READ);
        if (rv) {
            skStreamPrintLastErr(*rwios, rv, &skAppPrintErr);
            skStreamDestroy(rwios);
            return -1;
        }

        (void)prepareFileForRead(*rwios);
    }

    return rv;
}


/*
 *  setOutputHandle();
 *
 *    If using the pager, enable it and bind it to the Ascii stream.
 */
void
setOutputHandle(
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

    /* bind the Ascii Stream to the output */
    rwAsciiSetOutputHandle(ascii_str, output.of_fp);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
