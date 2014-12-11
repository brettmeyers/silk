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
 * rwrtd2split
 *
 *  Read an input file in RWROUTED format and write the records to a
 *  new file in RWSPLIT format.
 */

#include <silk/silk.h>

RCSIDENT("$SiLK: rwrtd2split.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skstream.h>
#include <silk/rwrec.h>
#include <silk/utils.h>
#include <silk/sksite.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout


/* LOCAL VARIABLES */

static char *in_fpath;
static char *out_fpath;
static skstream_t *in_ios;
static skstream_t *out_ios;

static int64_t hdr_len = 0;
static int64_t rec_len;


/* OPTIONS SETUP */

/*
** typedef enum {
** } appOptionsEnum;
*/

static struct option appOptions[] = {
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
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
#define USAGE_MSG                                                         \
    ("<INPUT_FILE> <OUTPUT_FILE>\n"                                       \
     "\tConvert INPUT_FILE, which should be in the FT_RWROUTED format,\n" \
     "\tto an FT_RWSPLIT file and write the result to OUTPUT_FILE.\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
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

    if (in_ios) {
        skStreamDestroy(&in_ios);
    }
    if (out_ios) {
        skStreamDestroy(&out_ios);
    }

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
    sk_file_header_t *in_hdr;
    sk_file_header_t *out_hdr;
    int arg_index;
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL)
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
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        skAppUsage();           /* never returns */
    }

    /* Ensure the site config is available */
    if (sksiteConfigure(1)) {
        exit(EXIT_FAILURE);
    }

    /* Get the input file name */
    if (arg_index == argc) {
        skAppPrintErr("Missing input file name");
        skAppUsage();         /* never returns */
    }
    in_fpath = argv[arg_index];
    ++arg_index;

    /* Get the output file name */
    if (arg_index == argc) {
        skAppPrintErr("Missing output file name");
        skAppUsage();         /* never returns */
    }
    out_fpath = argv[arg_index];
    ++arg_index;

    /* We don't expect additional files */
    if (arg_index != argc) {
        skAppPrintErr("Too many arguments or unrecognized switch '%s'",
                      argv[arg_index]);
        skAppUsage();
    }

    /* Open input file */
    rv = skStreamOpenSilkFlow(&in_ios, in_fpath, SK_IO_READ);
    if (rv) {
        skStreamPrintLastErr(in_ios, rv, &skAppPrintErr);
        skStreamDestroy(&in_ios);
        exit(EXIT_FAILURE);
    }

    /* Get input file's header */
    in_hdr = skStreamGetSilkHeader(in_ios);

    /* Check input version */
    if (skHeaderGetFileFormat(in_hdr) != FT_RWROUTED) {
        skAppPrintErr("Input file '%s' not in RWROUTED format",
                      in_fpath);
        skStreamDestroy(&in_ios);
        exit(EXIT_FAILURE);
    }

    /* Create and open output file */
    rv = skStreamCreate(&out_ios, SK_IO_WRITE, SK_CONTENT_SILK_FLOW);
    if (rv == SKSTREAM_OK) {
        rv = skStreamBind(out_ios, out_fpath);
    }
    if (rv == SKSTREAM_OK) {
        out_hdr = skStreamGetSilkHeader(out_ios);
        rv = skHeaderCopy(out_hdr, in_hdr,
                          (SKHDR_CP_ALL & ~SKHDR_CP_FORMAT));
    }
    if (rv == SKSTREAM_OK) {
        rv = skHeaderSetFileFormat(out_hdr, FT_RWSPLIT);
    }
    if (rv == SKSTREAM_OK) {
        rv = skStreamOpen(out_ios);
    }
    if (rv == SKSTREAM_OK) {
        rv = skStreamWriteSilkHeader(out_ios);
    }
    if (rv != SKSTREAM_OK) {
        skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
        skAppPrintErr("Unable to open output file '%s'.", out_fpath);
        skStreamDestroy(&out_ios);
        exit(EXIT_FAILURE);
    }

    hdr_len = skHeaderGetLength(out_hdr);
    rec_len = skHeaderGetRecordLength(out_hdr);
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
    int          UNUSED(opt_index),
    char        UNUSED(*opt_arg))
{
    return 0;
}


int main(int argc, char **argv)
{
    rwRec rwrec;
    int64_t rec_count = 0;
    int64_t file_size_real = 0;
    int64_t file_size_calc = 0;
    int in_rv;
    int rv;

    /* Setup app: open input and output files; will exit(1) on error */
    appSetup(argc, argv);

    /* Process body */
    while (SKSTREAM_OK == (in_rv = skStreamReadRecord(in_ios, &rwrec))) {
        rec_count++;
        rv = skStreamWriteRecord(out_ios, &rwrec);
        if (rv != SKSTREAM_OK) {
            skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
            if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                skAppPrintErr("Error writing to '%s'.  Stopping copy.",
                              out_fpath);
                break;
            }
        }
    }
    if ((SKSTREAM_OK != in_rv) && (SKSTREAM_ERR_EOF != in_rv)) {
        skStreamPrintLastErr(in_ios, in_rv, &skAppPrintErr);
    }

    skStreamDestroy(&in_ios);
    rv = skStreamClose(out_ios);
    if (rv) {
        skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
    }
    skStreamDestroy(&out_ios);

    file_size_real = skFileSize(out_fpath);
    file_size_calc = hdr_len + rec_len * rec_count;
    if (file_size_real != file_size_calc) {
        skAppPrintErr(("ERROR: output filesize mismatch."
                       " Calc. %" PRId64 " vs real %" PRId64),
                      file_size_calc, file_size_real);
        exit(EXIT_FAILURE);
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
