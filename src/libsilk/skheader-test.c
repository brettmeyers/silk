/*
** Copyright (C) 2006-2014 by Carnegie Mellon University.
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
**  skheaders-test
**
**    Simple program to test the skheader.c code.
**
**    Mark Thomas
**    November 2006
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skheader-test.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout


/* LOCAL VARIABLE DEFINITIONS */

/* name of file to read or write */
static const char *fname = NULL;

/* whether to read or write */
static int read_or_write = -1;


/* OPTIONS SETUP */

typedef enum {
    OPT_READ,
    OPT_WRITE
} appOptionsEnum;

static struct option appOptions[] = {
    {"read",            REQUIRED_ARG, 0, OPT_READ},
    {"write",           REQUIRED_ARG, 0, OPT_WRITE},
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
    "File to read.",
    "File to write",
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
    ("[SWITCHES] {--read=FILE | --write=FILE}\n"                        \
     "\tTest program for skheader.c.  With --write=FILE, writes a simple\n" \
     "\tfile to FILE.  With --read=FILE; reads that file.  Only use\n"  \
     "\t--read for files created with skheader-test --write=FILE\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
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
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL))
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
        /* options parsing should print error */
        skAppUsage();           /* never returns */
    }

    /* Try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names. */
    sksiteConfigure(0);

    if (fname == NULL) {
        skAppPrintErr("Must specify --%s or --%s",
                      appOptions[OPT_READ].name, appOptions[OPT_WRITE].name);
        skAppUsage();
    }

    /* check for extraneous arguments */
    if (arg_index != argc) {
        skAppPrintErr("Too many arguments or unrecognized switch '%s'",
                      argv[arg_index]);
        skAppUsage();           /* never returns */
    }

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
      case OPT_READ:
      case OPT_WRITE:
        if (fname) {
            skAppPrintErr("May only specify one of --%s or --%s and only once",
                          appOptions[OPT_READ].name,
                          appOptions[OPT_WRITE].name);
            return 1;
        }
        fname = opt_arg;
        read_or_write = opt_index;
        break;
    }

    return 0;  /* OK */
}




static int
doread(
    const char         *file)
{
    skstream_t *stream;
    sk_file_header_t *hdr;
    sk_header_entry_t *he;
    sk_hentry_iterator_t iter;
    int rv;

    if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
        || (rv = skStreamBind(stream, file))
        || (rv = skStreamOpen(stream)))
    {
        skAppPrintErr("Unable to open %s", file);
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        skStreamDestroy(&stream);
        return -1;
    }

    if (skStreamReadSilkHeader(stream, &hdr)) {
        skAppPrintErr("Unable to read header start");
        return -1;
    }

    skHeaderIteratorBind(&iter, hdr);
    while ((he = skHeaderIteratorNext(&iter)) != NULL) {
        skHeaderEntryPrint(he, stdout);
        printf("\n");
    }

    return 0;
}


static int
dowrite(
    const char         *file,
    int                 argc,
    char              **argv)
{
    skstream_t *stream;
    sk_file_header_t *hdr;
    sk_header_entry_t *hentry;
    int rv;

    if ((rv = skStreamCreate(&stream, SK_IO_WRITE, SK_CONTENT_SILK))
        || (rv = skStreamBind(stream, file)))
    {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        skStreamDestroy(&stream);
        return -1;
    }

    hdr = skStreamGetSilkHeader(stream);

    hentry = skHentryPackedfileCreate(1164215667, 1, 5);
    if (NULL == hentry) {
        skAppPrintErr("Unable to create packedfile header");
        return -1;
    }
    if (skHeaderAddEntry(hdr, hentry)) {
        skAppPrintErr("Unable to add packedfile hentry");
        return -1;
    }

    hentry = skHentryInvocationCreate(1, argc, argv);
    if (NULL == hentry) {
        skAppPrintErr("Unable to create invocation header");
        return -1;
    }
    if (skHeaderAddEntry(hdr, hentry)) {
        skAppPrintErr("Unable to add invocation hentry");
        return -1;
    }

    hentry = skHentryAnnotationCreate("blah blah blah");
    if (NULL == hentry) {
        skAppPrintErr("Unable to create annotation header");
        return -1;
    }
    if (skHeaderAddEntry(hdr, hentry)) {
        skAppPrintErr("Unable to add annotation hentry");
        return -1;
    }

    hentry = skHentryProbenameCreate("S1_yaf");
    if (NULL == hentry) {
        skAppPrintErr("Unable to create probename header");
        return -1;
    }
    if (skHeaderAddEntry(hdr, hentry)) {
        skAppPrintErr("Unable to add probename hentry");
        return -1;
    }

    rv = skStreamOpen(stream);
    if (rv) {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        skStreamDestroy(&stream);
        return -1;
    }

    if (skStreamWriteSilkHeader(stream)) {
        skAppPrintErr("Unable to write header");
        return -1;
    }

    rv = skStreamClose(stream);
    if (rv) {
        skAppPrintErr("Error closing %s: %s", file, strerror(errno));
        return -1;
    }
    skStreamDestroy(&stream);

    return 0;
}


int main(int argc, char **argv)
{
    appSetup(argc, argv);                       /* never returns on error */

    skHeaderInitialize();

    switch ((appOptionsEnum)read_or_write) {
      case OPT_READ:
        doread(fname);
        break;
      case OPT_WRITE:
        dowrite(fname, argc, argv);
        break;
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
