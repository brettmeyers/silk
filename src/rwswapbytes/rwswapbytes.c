/*
** Copyright (C) 2003-2014 by Carnegie Mellon University.
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
 * rwswapbytes
 *
 * Read any rw file (rwpacked file, rwfilter output, etc) and output a
 * file in the specified byte order.
 *
 */

#include <silk/silk.h>

RCSIDENT("$SiLK: rwswapbytes.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* MACROS AND TYPEDEFS */

/* File handle for --help output */
#define USAGE_FH stdout

typedef enum rwswapOptions {
    RWSW_BIG = 128, RWSW_LITTLE, RWSW_NATIVE, RWSW_SWAP
} rwswapOptions_t;

#if SK_LITTLE_ENDIAN
#  define RWSW_NATIVE_FORMAT "little"
#else
#  define RWSW_NATIVE_FORMAT "big"
#endif


/* LOCAL VARIABLES */

static rwswapOptions_t out_endian;
static const char *in_path;
static const char *out_path;


/* OPTIONS */

static struct option appOptions[] = {
    {"big-endian",       NO_ARG,       0, RWSW_BIG},
    {"little-endian",    NO_ARG,       0, RWSW_LITTLE},
    {"native-endian",    NO_ARG,       0, RWSW_NATIVE},
    {"swap-endian",      NO_ARG,       0, RWSW_SWAP},
    {0,0,0,0}            /* sentinel entry */
};

static const char *appHelp[] = {
    /* add help strings here for the applications options */
    "Write output in big-endian format (network byte-order)",
    "Write output in little-endian format",
    ("Write output in this machine's native format [" RWSW_NATIVE_FORMAT "]"),
    "Unconditionally swap the byte-order of the input",
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
#define USAGE_MSG                                                       \
    ("[SWITCHES] <ENDIAN_SWITCH> <INPUT_FILE> <OUTPUT_FILE>\n"          \
     "\tChange the byte-order of <INPUT_FILE> as specified by "         \
     "<ENDIAN_SWITCH>\n"                                                \
     "\tand write result to <OUTPUT_FILE>.  You may use \"stdin\" for\n" \
     "\t<INPUT_FILE> and \"stdout\" for <OUTPUT_FILE>.\n")

    FILE *fh = USAGE_FH;
    int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    skOptionsNotesUsage(fh);
    fprintf(fh, "\nENDIAN_SWITCH:\n");
    for (i = 0; appOptions[i].name; i++ ) {
        fprintf(fh, "--%s: %s\n", appOptions[i].name, appHelp[i]);
    }
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
    int arg_index;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsNotesRegister(NULL))
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
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* Check that a swapping option was given */
    if (0 == out_endian) {
        skAppPrintErr("You must specify the output byte order.");
        skAppUsage();
    }

    /* Check that we have input */
    if (arg_index >= argc) {
        skAppPrintErr("Expecting input file name");
        skAppUsage();           /* never returns */
    }
    in_path = argv[arg_index];
    ++arg_index;

    /* check that we have an output location */
    if (arg_index >= argc) {
        skAppPrintErr("Expecting output file name");
        skAppUsage();           /* never returns */
    }
    out_path = argv[arg_index];
    ++arg_index;

    /* check for extra options */
    if (argc != arg_index) {
        skAppPrintErr("Got extra options");
        skAppUsage();           /* never returns */
    }

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
    char        UNUSED(*opt_arg))
{
    switch (opt_index) {
#if !SK_LITTLE_ENDIAN
      case RWSW_NATIVE:
#endif
      case RWSW_BIG:
        if ((0 != out_endian) && (RWSW_BIG != out_endian)) {
            skAppPrintErr("Conflicting endian options given");
            return 1;
        }
        out_endian = RWSW_BIG;
        break;

#if SK_LITTLE_ENDIAN
      case RWSW_NATIVE:
#endif
      case RWSW_LITTLE:
        if ((0 != out_endian) && (RWSW_LITTLE != out_endian)) {
            skAppPrintErr("Conflicting endian options given");
            return 1;
        }
        out_endian = RWSW_LITTLE;
        break;

      case RWSW_SWAP:
        if ((0 != out_endian) && (RWSW_SWAP != out_endian)) {
            skAppPrintErr("Conflicting endian options given");
            return 1;
        }
        out_endian = RWSW_SWAP;
        break;

      default:
        skAbortBadCase(opt_index);
    }

    return 0;                   /* OK */
}


/*
 *  is_ok = rwswap_file(input_file, output_file, endian);
 *
 *    Byte swap the file named 'input_file' and write it to
 *    'output_file' using the 'endian' to determine the byte-order of
 *    'output_file'.
 */
static int
rwswap_file(
    const char         *in_file,
    const char         *out_file,
    rwswapOptions_t     endian)
{
    silk_endian_t byte_order;
    skstream_t *in_ios;
    skstream_t *out_ios;
    sk_file_header_t *in_hdr;
    sk_file_header_t *out_hdr;
    rwRec rwrec;
    int in_rv = SKSTREAM_OK;
    int rv = 0;

    /* Open input file */
    in_rv = skStreamOpenSilkFlow(&in_ios, in_file, SK_IO_READ);
    if (in_rv) {
        skStreamPrintLastErr(in_ios, in_rv, &skAppPrintErr);
        skStreamDestroy(&in_ios);
        return 1;
    }

    in_hdr = skStreamGetSilkHeader(in_ios);

    switch (endian) {
      case RWSW_BIG:
        byte_order = SILK_ENDIAN_BIG;
        break;
      case RWSW_LITTLE:
        byte_order = SILK_ENDIAN_LITTLE;
        break;
      case RWSW_NATIVE:
        byte_order = SILK_ENDIAN_NATIVE;
        break;
      case RWSW_SWAP:
        switch (skHeaderGetByteOrder(in_hdr)) {
          case SILK_ENDIAN_BIG:
            byte_order = SILK_ENDIAN_LITTLE;
            break;
          case SILK_ENDIAN_LITTLE:
            byte_order = SILK_ENDIAN_BIG;
            break;
          default:
            skAbortBadCase(skHeaderGetByteOrder(in_hdr));
        }
        break;
      default:
        skAbortBadCase(endian);
    }

    /* Open the output file and copy the headers from the source file,
     * but modify the byte order */
    rv = skStreamCreate(&out_ios, SK_IO_WRITE, SK_CONTENT_SILK_FLOW);
    if (rv == SKSTREAM_OK) {
        rv = skStreamBind(out_ios, out_file);
        out_hdr = skStreamGetSilkHeader(out_ios);
    }
    if (rv == SKSTREAM_OK) {
        rv = skHeaderCopy(out_hdr, in_hdr,
                          (SKHDR_CP_ALL & ~SKHDR_CP_ENDIAN));
    }
    if (rv == SKSTREAM_OK) {
        rv = skHeaderSetByteOrder(out_hdr, byte_order);
    }
    if (rv == SKSTREAM_OK) {
        rv = skOptionsNotesAddToStream(out_ios);
    }
    if (rv == SKSTREAM_OK) {
        rv = skStreamOpen(out_ios);
    }
    if (rv == SKSTREAM_OK) {
        rv = skStreamWriteSilkHeader(out_ios);
    }
    if (rv != SKSTREAM_OK) {
        goto END;
    }

    /* read and write the data until we encounter an error */
    while ((in_rv = skStreamReadRecord(in_ios, &rwrec)) == SKSTREAM_OK) {
        rv = skStreamWriteRecord(out_ios, &rwrec);
        if (SKSTREAM_ERROR_IS_FATAL(rv)) {
            goto END;
        }
    }

    /* input error */
    if (in_rv != SKSTREAM_ERR_EOF) {
        skStreamPrintLastErr(in_ios, in_rv, &skAppPrintErr);
    }

    if (rv == SKSTREAM_OK) {
        rv = skStreamClose(out_ios);
    }

  END:
    if (rv) {
        skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
    }
    if (out_ios) {
        skStreamDestroy(&out_ios);
    }
    if (in_ios) {
        skStreamDestroy(&in_ios);
    }

    return rv;
}


int main(int argc, char **argv)
{
    int rv;

    appSetup(argc, argv);

    rv = rwswap_file(in_path, out_path, out_endian);

    /* done */
    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
