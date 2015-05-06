/*
** Copyright (C) 2007-2015 by Carnegie Mellon University.
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
**  rwsilk2ipfix.c
**
**    SiLK to IPFIX translation application
**
**    Brian Trammell
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwsilk2ipfix.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwrec.h>
#include <silk/skipfix.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* where to write --print-stat output */
#define STATS_FH stderr


/* LOCAL VARIABLE DEFINITIONS */

/* for looping over input */
static sk_options_ctx_t *optctx = NULL;

/* the IPFIX output file; use stdout if no name provided */
static sk_fileptr_t ipfix_output;

/* whether to print statistics */
static int print_statistics = 0;


/* OPTIONS SETUP */

typedef enum {
    OPT_IPFIX_OUTPUT,
    OPT_PRINT_STATISTICS
} appOptionsEnum;

static struct option appOptions[] = {
    {"ipfix-output",            REQUIRED_ARG, 0, OPT_IPFIX_OUTPUT},
    {"print-statistics",        NO_ARG,       0, OPT_PRINT_STATISTICS},
    {0,0,0,0}                   /* sentinel entry */
};

static const char *appHelp[] = {
    ("Write IPFIX records to the specified path. Def. stdout"),
    ("Print the count of processed records. Def. No"),
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
    ("[SWITCHES] [SILK_FILES]\n"                                              \
     "\tReads SiLK Flow records from files named on the command line or\n"    \
     "\tfrom the standard input, converts them to an IPFIX format, and\n"     \
     "\twrites the IPFIX records to the named file or the standard output.\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
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
static void
appTeardown(
    void)
{
    static int teardownFlag = 0;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    if (ipfix_output.of_fp) {
        skFileptrClose(&ipfix_output, &skAppPrintErr);
    }

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

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS);

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

    /* parse the options */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    return;  /* OK */
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
    switch ((appOptionsEnum)opt_index) {
      case OPT_IPFIX_OUTPUT:
        if (ipfix_output.of_name) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        ipfix_output.of_name = opt_arg;
        break;

      case OPT_PRINT_STATISTICS:
        print_statistics = 1;
        break;
    }

    return 0;  /* OK */
}


int main(int argc, char **argv)
{
    fBuf_t *fbuf = NULL;
    GError *err = NULL;
    skstream_t *rwios = NULL;
    rwRec rwrec;
    uint64_t rec_count = 0;
    int rv;

    appSetup(argc, argv);                       /* never returns on error */

    /* open output file if provided */
    if (NULL == ipfix_output.of_name) {
        ipfix_output.of_name = "-";
        ipfix_output.of_fp = stdout;
    } else {
        rv = skFileptrOpen(&ipfix_output, SK_IO_WRITE);
        if (rv) {
            skAppPrintErr("Could not open IPFIX output file '%s': %s",
                          ipfix_output.of_name, skFileptrStrerror(rv));
            exit(EXIT_FAILURE);
        }

        if (SK_FILEPTR_IS_PROCESS == ipfix_output.of_type) {
            skAppPrintErr("Writing to gzipped files is not supported");
            exit(EXIT_FAILURE);
        }
    }

    if (FILEIsATty(ipfix_output.of_fp)) {
        skAppPrintErr("Will not write binary data to the terminal");
        exit(EXIT_FAILURE);
    }

    /* Create IPFIX buffer */
    fbuf = skiCreateWriteBufferForFP(ipfix_output.of_fp, 0, &err);
    if (!fbuf) {
        skAppPrintErr("Could not open '%s' for IPFIX: %s",
                      ipfix_output.of_name, err->message);
        exit(EXIT_FAILURE);
    }

    /* For each input, process each record */
    while ((rv = skOptionsCtxNextSilkFile(optctx, &rwios, &skAppPrintErr))
           == 0)
    {
        while ((rv = skStreamReadRecord(rwios, &rwrec)) == SKSTREAM_OK) {
            /* process record */
            ++rec_count;
            if (!skiRwAppendRecord(fbuf, &rwrec, &err)) {
                skAppPrintErr("Could not write IPFIX record: %s",
                              err->message);
                exit(EXIT_FAILURE);
            }
        }
        if (rv != SKSTREAM_ERR_EOF) {
            skStreamPrintLastErr(rwios, rv, &skAppPrintErr);
        }
        skStreamDestroy(&rwios);
    }

    /* finalize the output */
    if (!fBufEmit(fbuf, &err)) {
        skAppPrintErr("Could not write final IPFIX message: %s",
                      err->message);
        exit(EXIT_FAILURE);
    }
    fbExporterClose(fBufGetExporter(fbuf));

    if (fbuf) {
        fBufFree(fbuf);
        skiTeardown();
    }

    /* print record count */
    if (print_statistics) {
        fprintf(STATS_FH, ("%s: Wrote %" PRIu64 " IPFIX records to '%s'\n"),
                skAppName(), rec_count, ipfix_output.of_name);
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
