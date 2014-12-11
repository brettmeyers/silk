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
 *  rwpdu2silk.c
 *
 *    rwpdu2silk reads PDU file(s) and converts the data to SiLK Flow
 *    records.
 *
 */

#include <silk/silk.h>

RCSIDENT("$SiLK: rwpdu2silk.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/libflowsource.h>
#include <silk/rwrec.h>
#include <silk/sklog.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* TYPEDEFS AND DEFINES */

/* file handle for --help usage message */
#define USAGE_FH stdout

/* where to write --print-stat output */
#define STATS_FH stderr

/* default value for --log-destination */
#define LOG_DESTINATION_DEFAULT  "none"


/* LOCAL VARIABLES */

/* for looping over input */
static sk_options_ctx_t *optctx = NULL;

/* the SiLK flow file to write (--silk-output) */
static skstream_t *silk_output = NULL;

/* where to write log messages (--log-destination) */
static char log_destination[PATH_MAX];

/* whether to print statistics (--print-statistics) */
static int print_statistics = 0;

/* the compression method to use when writing the file.
 * sksiteCompmethodOptionsRegister() will set this to the default or
 * to the value the user specifies. */
static sk_compmethod_t comp_method;

/* required to process the PDUs */
static skpc_probe_t *probe;


/* OPTIONS SETUP */

typedef enum {
    OPT_SILK_OUTPUT,
    OPT_PRINT_STATISTICS,
    OPT_LOG_DESTINATION
} appOptionsEnum;

static struct option appOptions[] = {
    {"silk-output",             REQUIRED_ARG, 0, OPT_SILK_OUTPUT},
    {"print-statistics",        NO_ARG,       0, OPT_PRINT_STATISTICS},
    {"log-destination",         REQUIRED_ARG, 0, OPT_LOG_DESTINATION},
    {0,0,0,0}                   /* sentinel entry */
};

static const char *appHelp[] = {
    ("Write the SiLK Flow records to the specified path.\n\tDef. stdout"),
    "Print the number of records written. Def. No.",
    ("Write messages about number of records read from\n"
     "\teach input and messages about invalid records to the specified\n"
     "\tlocation. Def. none. Choices: none, stdout, stderr, or a filename"),
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static size_t logprefix(char *buffer, size_t bufsize);


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
#define USAGE_MSG                                                         \
    ("[SWITCHES] <PDU_FILENAMES>\n"                                       \
     "\tReads NetFlowV5 records (PDUs) from files named on the command\n" \
     "\tline, converts them to the SiLK format, and writes the SiLK\n"    \
     "\trecords to the named file or to the standard output.\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
    skOptionsCtxOptionsUsage(optctx, fh);
    skOptionsNotesUsage(fh);
    sksiteCompmethodOptionsUsage(fh);
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

    /* close SiLK flow output file */
    if (silk_output) {
        rv = skStreamClose(silk_output);
        if (rv && rv != SKSTREAM_ERR_NOT_OPEN) {
            skStreamPrintLastErr(silk_output, rv, &skAppPrintErr);
        }
        skStreamDestroy(&silk_output);
    }

    skpcTeardown();

    /* set level to "emerg" to avoid the "Stopped logging" message */
    sklogSetLevel("emerg");
    sklogTeardown();

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
    sk_file_header_t *silk_hdr;
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    optctx_flags = (SK_OPTIONS_CTX_INPUT_BINARY | SK_OPTIONS_CTX_XARGS);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsNotesRegister(NULL)
        || sksiteCompmethodOptionsRegister(&comp_method))
    {
        skAppPrintErr("Unable to register options");
        exit(EXIT_FAILURE);
    }

    /* enable the logger */
    sklogSetup(0);
    sklogSetDestination("stderr");
    sklogSetStampFunction(&logprefix);

    /* register the teardown handler */
    if (atexit(appTeardown) < 0) {
        skAppPrintErr("Unable to register appTeardown() with atexit()");
        appTeardown();
        exit(EXIT_FAILURE);
    }

    /* parse the options */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();           /* never returns */
    }

    if ('\0' == log_destination[0]) {
        strncpy(log_destination, LOG_DESTINATION_DEFAULT,
                sizeof(log_destination));
    }
    sklogSetDestination(log_destination);

    /* default output is "stdout" */
    if (!silk_output) {
        if ((rv =skStreamCreate(&silk_output,SK_IO_WRITE,SK_CONTENT_SILK_FLOW))
            || (rv = skStreamBind(silk_output, "-")))
        {
            skStreamPrintLastErr(silk_output, rv, &skAppPrintErr);
            exit(EXIT_FAILURE);
        }
    }

    /* get the header */
    silk_hdr = skStreamGetSilkHeader(silk_output);

    /* open the output */
    if ((rv = skHeaderSetCompressionMethod(silk_hdr, comp_method))
        || (rv = skOptionsNotesAddToStream(silk_output))
        || (rv = skHeaderAddInvocation(silk_hdr, 1, argc, argv))
        || (rv = skStreamOpen(silk_output))
        || (rv = skStreamWriteSilkHeader(silk_output)))
    {
        skStreamPrintLastErr(silk_output, rv, &skAppPrintErr);
        exit(EXIT_FAILURE);
    }

    if (skpcSetup()) {
        exit(EXIT_FAILURE);
    }
    if (skpcProbeCreate(&probe)) {
        exit(EXIT_FAILURE);
    }
    skpcProbeSetName(probe, skAppName());
    skpcProbeSetType(probe, PROBE_ENUM_NETFLOW_V5);
    skpcProbeSetFileSource(probe, "/dev/null");
    skpcProbeSetLogFlags(probe, SOURCE_LOG_NONE);
    if (skpcProbeVerify(probe, 0)) {
        exit(EXIT_FAILURE);
    }

    sklogOpen();

    return;                     /* OK */
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
    size_t sz;
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_SILK_OUTPUT:
        if (silk_output) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if ((rv =skStreamCreate(&silk_output,SK_IO_WRITE,SK_CONTENT_SILK_FLOW))
            || (rv = skStreamBind(silk_output, opt_arg)))
        {
            skStreamPrintLastErr(silk_output, rv, &skAppPrintErr);
            exit(EXIT_FAILURE);
        }
        break;

      case OPT_PRINT_STATISTICS:
        print_statistics = 1;
        break;

      case OPT_LOG_DESTINATION:
        if ('\0' != log_destination[0]) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
        }
        if ('\0' == opt_arg[0]) {
            skAppPrintErr("Invalid %s: Path name is required",
                          appOptions[opt_index].name);
            return 1;
        }
        if (0 == strcmp("stdout", opt_arg)
            || 0 == strcmp("stderr", opt_arg)
            || 0 == strcmp("none", opt_arg))
        {
            strncpy(log_destination, opt_arg, sizeof(log_destination));
            break;
        }
        if ('/' == opt_arg[0]) {
            if (strlen(opt_arg) >= sizeof(log_destination)) {
                skAppPrintErr("Invalid %s: Name is too long",
                              appOptions[opt_index].name);
                return 1;
            }
            strncpy(log_destination, opt_arg, sizeof(log_destination));
            break;
        }
        if (NULL == getcwd(log_destination, sizeof(log_destination))) {
            skAppPrintSyserror("Unable to get current directory");
            return 1;
        }
        sz = strlen(log_destination);
        if (sz + strlen(opt_arg) + 1 >= sizeof(log_destination)) {
            skAppPrintErr("Invalid %s: Name is too long",
                          appOptions[opt_index].name);
            return 1;
        }
        snprintf(log_destination + sz, sizeof(log_destination) - sz, "/%s",
                 opt_arg);
        break;
    }

    return 0;                   /* OK */
}


/*
 *    Prefix any log messages from libflowsource with the program name
 *    instead of the standard logging tag.
 */
static size_t
logprefix(
    char               *buffer,
    size_t              bufsize)
{
    return (size_t)snprintf(buffer, bufsize, "%s: ", skAppName());
}


/*
 *  count = pdu2silk(filename);
 *
 *    Read PDUs from 'filename' and write records to the global
 *    'silk_output' file.  Return number of records processed, or -1
 *    on error.
 */
static int64_t
pdu2silk(
    const char         *filename)
{
    static unsigned int file_count = 0;
    char probe_name[128];
    skPDUSource_t *pdu_src;
    skFlowSourceParams_t params;
    int64_t count;
    rwRec rwrec;
    int rv;

    ++file_count;
    snprintf(probe_name, sizeof(probe_name), "input%04u", file_count);

    params.path_name = filename;
    skpcProbeSetName(probe, probe_name);
    pdu_src = skPDUSourceCreate(probe, &params);
    if (pdu_src == NULL) {
        return -1;
    }

    count = 0;
    while (-1 != skPDUSourceGetGeneric(pdu_src, &rwrec)) {
        ++count;
        rv = skStreamWriteRecord(silk_output, &rwrec);
        if (rv) {
            skStreamPrintLastErr(silk_output, rv, &skAppPrintErr);
            if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                exit(EXIT_FAILURE);
            }
        }
    }

    skPDUSourceLogStatsAndClear(pdu_src);
    skPDUSourceDestroy(pdu_src);

    return count;
}


int main(int argc, char **argv)
{
    char *path;
    int64_t total_count;
    int64_t count;
    int rv;

    appSetup(argc, argv);       /* never returns on failure */

    total_count = 0;

    /* process each file on the command line */
    while ((rv = skOptionsCtxNextArgument(optctx, &path)) == 0) {
        count = pdu2silk(path);
        if (count < 0) {
            exit(EXIT_FAILURE);
        }
        total_count += count;
    }
    if (rv < 0) {
        exit(EXIT_FAILURE);
    }

    if (print_statistics) {
        fprintf(STATS_FH, ("%s: Wrote %" PRIu64 " records to '%s'\n"),
                skAppName(), total_count, skStreamGetPathname(silk_output));
    }

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
