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
**  rwdedupesetup.c
**
**    Options processing and additional set-up for rwdedupe.  See
**    rwdedupe.c for implementation details.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwdedupesetup.c dc9ecb5e3731 2014-09-24 20:02:10Z mthomas $");

#include <silk/sksite.h>
#include <silk/skstringmap.h>
#include "rwdedupe.h"


/* LOCAL DEFINES AND TYPEDEFS */

/* where to send --help output */
#define USAGE_FH stdout


/* LOCAL VARIABLES */

/* available key fields; rwAsciiFieldMapAddDefaultFields() fills this */
static sk_stringmap_t *field_map = NULL;

/* input checker */
static sk_options_ctx_t *optctx = NULL;

/* non-zero if we are shutting down due to a signal; controls whether
 * errors are printed in appTeardown(). */
static int caught_signal = 0;

/* the compression method to use when writing the file.
 * sksiteCompmethodOptionsRegister() will set this to the default or
 * to the value the user specifies. */
static sk_compmethod_t comp_method;

/* the string containing the list of fields to ignore */
static const char *ignore_fields = NULL;

/* temporary directory */
static const char *temp_directory = NULL;

/* read-only cache of argc and argv used for writing header to output
 * file */
static int pargc;
static char **pargv;


/* OPTIONS */

typedef enum {
    OPT_HELP_FIELDS,
    OPT_IGNORE_FIELDS,
    OPT_PACKETS_DELTA,
    OPT_BYTES_DELTA,
    OPT_STIME_DELTA,
    OPT_DURATION_DELTA,
    OPT_OUTPUT_PATH,
    OPT_BUFFER_SIZE
} appOptionsEnum;

static struct option appOptions[] = {
    {"help-fields",         NO_ARG,       0, OPT_HELP_FIELDS},
    {"ignore-fields",       REQUIRED_ARG, 0, OPT_IGNORE_FIELDS},
    {"packets-delta",       REQUIRED_ARG, 0, OPT_PACKETS_DELTA},
    {"bytes-delta",         REQUIRED_ARG, 0, OPT_BYTES_DELTA},
    {"stime-delta",         REQUIRED_ARG, 0, OPT_STIME_DELTA},
    {"duration-delta",      REQUIRED_ARG, 0, OPT_DURATION_DELTA},
    {"output-path",         REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {"buffer-size",         REQUIRED_ARG, 0, OPT_BUFFER_SIZE},
    {0,0,0,0}               /* sentinel entry */
};

static const char *appHelp[] = {
    "Describe each possible field and exit. Def. no",
    ("Ignore these field(s) (ie, treat them as being\n"
     "\tidentical) when comparing records:"),
    ("Treat the packets field on two flows as identical if\n"
     "\ttheir values differ by this number of packets or less. Def. 0 "),
    ("Treat the bytes field on two flows as identical if\n"
     "\ttheir values differ by this number of bytes or less. Def. 0 "),
    ("Treat the start time field on two flows as identical if\n"
     "\ttheir values differ by this number of milliseconds or less. Def. 0 "),
    ("Treat the duration field on two flows as identical if\n"
     "\ttheir values differ by this number of milliseconds or less. Def. 0 "),
    ("Destination for output (stdout|pipe).\n"
     "\tDefault is stdout if stdout is not a terminal"),
    NULL, /* generated dynamically */
    (char *)NULL
};



/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static void appHandleSignal(int sig);
static int  parseFields(const char *fields_string);
static void helpFields(FILE *fh);
static int
setSortFields(
    uint32_t            ignore_count,
    uint32_t           *ignore_field_ids);



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
     "\tRead SiLK Flow records from FILES given on command line or from\n"    \
     "\tthe standard input and write the records to the named output path\n"  \
     "\tor to the standard output, removing any duplicate flow records.\n"    \
     "\tTwo records are duplications when ALL fields are identical.  Note\n"  \
     "\tthat the order of records is not maintained.\n")

    FILE *fh = USAGE_FH;
    int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);

    for (i = 0; appOptions[i].name; i++ ) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch (i) {
          case OPT_IGNORE_FIELDS:
            /* Dynamically build the help */
            fprintf(fh, "%s\n", appHelp[i]);
            skStringMapPrintUsage(field_map, fh, 4);
            break;
          case OPT_BUFFER_SIZE:
            fprintf(fh,
                    ("Attempt to allocate this much memory for the in-core\n"
                     "\tbuffer, in bytes."
                     "  Append k, m, g, for kilo-, mega-, giga-bytes,\n"
                     "\trespectively. Def. %" PRIu32 "\n"),
                    (uint32_t)DEFAULT_BUFFER_SIZE);
            break;
          default:
            /* Simple help text from the appHelp array */
            assert(appHelp[i]);
            fprintf(fh, "%s\n", appHelp[i]);
            break;
        }
    }

    skOptionsCtxOptionsUsage(optctx, fh);
    skOptionsTempDirUsage(fh);
    skOptionsNotesUsage(fh);
    sksiteCompmethodOptionsUsage(fh);
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
    int rv;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* close and destroy output */
    if (out_rwios) {
        rv = skStreamDestroy(&out_rwios);
        if (rv && !caught_signal) {
            /* only print error when not in signal handler */
            skStreamPrintLastErr(out_rwios, rv, &skAppPrintErr);
        }
        out_rwios = NULL;
    }

    /* remove any temporary files */
    skTempFileTeardown(&tmpctx);

    /* free variables */
    if (field_map) {
        skStringMapDestroy(field_map);
    }

    skOptionsNotesTeardown();
    skOptionsCtxDestroy(&optctx);
    skAppUnregister();
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

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    memset(&delta, 0, sizeof(flow_delta_t));

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS | SK_OPTIONS_CTX_PRINT_FILENAMES);

    /* store a copy of the arguments */
    pargc = argc;
    pargv = argv;

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsTempDirRegister(&temp_directory)
        || skOptionsNotesRegister(NULL)
        || sksiteCompmethodOptionsRegister(&comp_method)
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

    /* initialize string-map of field identifiers.  Remove any fields
     * that do not correspond to a field on the actual rwRec */
    if (rwAsciiFieldMapAddDefaultFields(&field_map)) {
        skAppPrintErr("Unable to setup fields stringmap");
        appExit(EXIT_FAILURE);
    }
    (void)skStringMapRemoveByID(field_map, RWREC_FIELD_STIME_MSEC);
    (void)skStringMapRemoveByID(field_map, RWREC_FIELD_ETIME_MSEC);
    (void)skStringMapRemoveByID(field_map, RWREC_FIELD_ELAPSED_MSEC);
    (void)skStringMapRemoveByID(field_map, RWREC_FIELD_ETIME);
    (void)skStringMapRemoveByID(field_map, RWREC_FIELD_ICMP_TYPE);
    (void)skStringMapRemoveByID(field_map, RWREC_FIELD_ICMP_CODE);

    /* parse options */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* parse the ignore fields list and set the sort-fields */
    if (ignore_fields) {
        if (parseFields(ignore_fields)) {
            appExit(EXIT_FAILURE);
        }
    } else if (delta.d_packets||delta.d_bytes||delta.d_stime||delta.d_elapsed){
        /* when a delta field was set but no --ignore-fields were
         * provided, set the fields to all fields */
        if (setSortFields(0, NULL)) {
            appExit(EXIT_FAILURE);
        }
    }

    /* verify that the temp directory is valid */
    if (skTempFileInitialize(&tmpctx, temp_directory, NULL, &skAppPrintErr)) {
        appExit(EXIT_FAILURE);
    }

    /* Check for an output stream; or default to stdout  */
    if (out_rwios == NULL) {
        if ((rv = skStreamCreate(&out_rwios, SK_IO_WRITE,SK_CONTENT_SILK_FLOW))
            || (rv = skStreamBind(out_rwios, "-")))
        {
            skStreamPrintLastErr(out_rwios, rv, NULL);
            skStreamDestroy(&out_rwios);
            appExit(EXIT_FAILURE);
        }
    }

    /* set the compmethod on the header */
    rv = skHeaderSetCompressionMethod(skStreamGetSilkHeader(out_rwios),
                                      comp_method);
    if (rv) {
        skAppPrintErr("Error setting header on %s: %s",
                      skStreamGetPathname(out_rwios), skHeaderStrerror(rv));
        appExit(EXIT_FAILURE);
    }

    /* open output */
    rv = skStreamOpen(out_rwios);
    if (rv) {
        skStreamPrintLastErr(out_rwios, rv, NULL);
        skAppPrintErr("Could not open output file.  Exiting.");
        appExit(EXIT_FAILURE);
    }

    /* set signal handler to clean up temp files on SIGINT, SIGTERM, etc */
    if (skAppSetSignalHandler(&appHandleSignal)) {
        appExit(EXIT_FAILURE);
    }

    return;                       /* OK */
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
    uint32_t tmp32;
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_HELP_FIELDS:
        helpFields(USAGE_FH);
        exit(EXIT_SUCCESS);

      case OPT_IGNORE_FIELDS:
        if (ignore_fields) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        ignore_fields = opt_arg;
        break;

      case OPT_PACKETS_DELTA:
        rv = skStringParseUint32(&tmp32, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        delta.d_packets = tmp32;
        break;

      case OPT_BYTES_DELTA:
        rv = skStringParseUint32(&tmp32, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        delta.d_bytes = tmp32;
        break;

      case OPT_STIME_DELTA:
        rv = skStringParseUint32(&tmp32, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        delta.d_stime = tmp32;
        break;

      case OPT_DURATION_DELTA:
        rv = skStringParseUint32(&tmp32, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        delta.d_elapsed = tmp32;
        break;

      case OPT_OUTPUT_PATH:
        /* check for switch given multiple times */
        if (out_rwios) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            skStreamDestroy(&out_rwios);
            return 1;
        }
        if ((rv = skStreamCreate(&out_rwios, SK_IO_WRITE,SK_CONTENT_SILK_FLOW))
            || (rv = skStreamBind(out_rwios, opt_arg)))
        {
            skStreamPrintLastErr(out_rwios, rv, NULL);
            return 1;
        }
        break;

      case OPT_BUFFER_SIZE:
        rv = skStringParseHumanUint64(&buffer_size, opt_arg,
                                      SK_HUMAN_NORMAL);
        if (rv) {
            goto PARSE_ERROR;
        }
        if ((buffer_size < MIN_IN_CORE_RECORDS * NODE_SIZE)
            || (buffer_size >= UINT32_MAX))
        {
            skAppPrintErr(("The --%s value must be between %" PRIu32
                           " and %" PRIu32),
                          appOptions[opt_index].name,
                          (MIN_IN_CORE_RECORDS * NODE_SIZE), UINT32_MAX);
            return 1;
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
 *  status = parseFields(fields_string);
 *
 *    Parse the user's option for the --ignore-fields switch and then
 *    fill in the globals sort_fields[] and num_fields.  Return 0 on
 *    success; -1 on failure.
 */
static int
parseFields(
    const char         *field_string)
{
#define DEDUPE_MAX_FIELDS  256
    uint32_t ignore_field_ids[DEDUPE_MAX_FIELDS];
    sk_stringmap_iter_t *iter = NULL;
    sk_stringmap_entry_t *entry;
    uint32_t ignore_count = 0;
    char *errmsg;
    int rv = -1;

    /* only visit this function once */
    assert(0 == num_fields);

    /* parse the input */
    if (skStringMapParse(field_map, field_string, SKSTRINGMAP_DUPES_ERROR,
                         &iter, &errmsg))
    {
        skAppPrintErr("Invalid %s: %s",
                      appOptions[OPT_IGNORE_FIELDS].name, errmsg);
        goto END;
    }

    /* fill the array of IDs */
    while (skStringMapIterNext(iter, &entry, NULL) == SK_ITERATOR_OK) {
        if (ignore_count >= DEDUPE_MAX_FIELDS) {
            skAppPrintErr("Only %d ignore-fields are supported",
                          DEDUPE_MAX_FIELDS);
            goto END;
        }
        ignore_field_ids[ignore_count] = entry->id;
        ++ignore_count;
    }

    /* set the sort fields ignoring some fields */
    if (setSortFields(ignore_count, ignore_field_ids)) {
        goto END;
    }

    /* success */
    rv = 0;

  END:
    if (iter) {
        skStringMapIterDestroy(iter);
    }
    return rv;
}


/*
 *  status = setSortFields(count, list);
 *
 *    Add the fields defined in 'field_map' that are NOT listed in
 *    'list' to the global 'sort_fields'.  Update 'num_fields' with
 *    the number of fields to sort over.  The parameter 'count' lists
 *    the number of fields in 'list'.  'count' may be zero.  Any
 *    fields related to delta.* fields should be last in the sort
 *    list.
 */
static int
setSortFields(
    uint32_t            ignore_count,
    uint32_t           *ignore_field_ids)
{
    uint32_t delta_fields[RWDEDUP_DELTA_FIELD_COUNT];
    uint32_t num_deltas = 0;
    uint32_t i, j;

    for (i = 0; i < RWREC_PRINTABLE_FIELD_COUNT; ++i) {

        /* are we concerned with this id? */
        if (NULL == skStringMapGetFirstName(field_map, i)) {
            /* no entries with this id.  ignore it */
            continue;
        }

        /* did the user request we ignore it */
        for (j = 0; j < ignore_count; ++j) {
            if (i == ignore_field_ids[j]) {
                /* ignore; jump to bottom of the outer for() */
                goto NEXT_FIELD;
            }
            /* handle the fields that are "linked" */
            if ((i == RWREC_FIELD_FTYPE_CLASS
                 && ignore_field_ids[j] == RWREC_FIELD_FTYPE_TYPE)
                || (i == RWREC_FIELD_FTYPE_TYPE
                    && ignore_field_ids[j] == RWREC_FIELD_FTYPE_CLASS))
            {
                /* ignore; jump to bottom of the outer for() */
                goto NEXT_FIELD;
            }
        }

        /* if a delta value is set for the field, add the field to a
         * temporary list */
        switch (i) {
          case RWREC_FIELD_STIME:
          case RWREC_FIELD_STIME_MSEC:
            if (delta.d_stime) {
                delta_fields[num_deltas] = i;
                ++num_deltas;
                goto NEXT_FIELD;
            }
            break;

          case RWREC_FIELD_ELAPSED:
          case RWREC_FIELD_ELAPSED_MSEC:
            if (delta.d_elapsed) {
                delta_fields[num_deltas] = i;
                ++num_deltas;
                goto NEXT_FIELD;
            }
            break;

          case RWREC_FIELD_PKTS:
            if (delta.d_packets) {
                delta_fields[num_deltas] = i;
                ++num_deltas;
                goto NEXT_FIELD;
            }
            break;

          case RWREC_FIELD_BYTES:
            if (delta.d_bytes) {
                delta_fields[num_deltas] = i;
                ++num_deltas;
                goto NEXT_FIELD;
            }
            break;
        }

        /* field was not ignored and no delta for it; add it */
        sort_fields[num_fields] = i;
        ++num_fields;

      NEXT_FIELD:
        ; /* empty */
    }

    /* add the delta fields to the sort fields */
    for (i = 0; i < num_deltas; ++i) {
        sort_fields[num_fields] = delta_fields[i];
        ++num_fields;
    }

    return 0;
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
    fprintf(fh,
            ("The following names may be used in the --%s switch. Names are\n"
             "case-insensitive and may be abbreviated to the shortest"
             " unique prefix.\n"),
            appOptions[OPT_IGNORE_FIELDS].name);

    skStringMapPrintDetailedUsage(field_map, fh);
}


/*
 *  int = appNextInput(&rwios);
 *
 *    Fill 'rwios' with the next input file to read.  Return 0 if
 *    'rwios' was successfully opened or 1 if there are no more input
 *    files.  Return -1 if a file cannot be opened.
 */
int
appNextInput(
    skstream_t        **rwios)
{
    int retval;
    int rv;

    retval = skOptionsCtxNextSilkFile(optctx, rwios, &skAppPrintErr);
    if (0 == retval) {
        /* copy annotations and command line entries from the input to
         * the output */
        if ((rv = skHeaderCopyEntries(skStreamGetSilkHeader(out_rwios),
                                      skStreamGetSilkHeader(*rwios),
                                      SK_HENTRY_INVOCATION_ID))
            || (rv = skHeaderCopyEntries(skStreamGetSilkHeader(out_rwios),
                                         skStreamGetSilkHeader(*rwios),
                                         SK_HENTRY_ANNOTATION_ID)))
        {
            skStreamPrintLastErr(out_rwios, rv, &skAppPrintErr);
            retval = -1;
        }
    } else if (1 == retval) {
        /* no more input.  add final information to header */
        if ((rv = skHeaderAddInvocation(skStreamGetSilkHeader(out_rwios),
                                        1, pargc, pargv))
            || (rv = skOptionsNotesAddToStream(out_rwios)))
        {
            skStreamPrintLastErr(out_rwios, rv, &skAppPrintErr);
            retval = -1;
        }
    }

    return retval;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
