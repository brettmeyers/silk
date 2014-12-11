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
**  rwfglob
**
**    A utility to print to stdout the list of files that fglob would
**    normally return.
**
**    If BLOCK_CHECK_DEFAULT is 1, this application will stat() each
**    file and print the string defined by BLOCK_CHECK_ZERO_MSG if its
**    size is non-zero but the number of blocks allocated to the file
**    is 0.  This is an indication that the file is stored offline.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwfglobapp.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "rwfilter.h"


/* LOCAL DEFINES AND TYPEDEFS */

/* whether to check for files that have a block count of 0; 1=yes */
#ifndef BLOCK_CHECK_DEFAULT
#define   BLOCK_CHECK_DEFAULT  1
#endif

/* what to print before the file name when file has 0 blocks */
#define BLOCK_CHECK_ZERO_MSG "  \t*** ON_TAPE ***"

/* where to write --help output */
#define USAGE_FH stdout

/* where to write the output */
#define OUTPUT_FH stdout


/* EXPORTED VARIABLES */


/* index into argv of first option that does not start with a '--'.
 * This assumes getopt rearranges the options, which gnu getopt will
 * do.  (Must be non-static since available in rwfilter.h) */
int arg_index;


/* LOCAL VARIABLES */

/* when this variable is nonzero, stat() to get the file's size and
 * block count and print BLOCK_CHECK_ZERO_MSG if size is non-zero but
 * block count is 0 */
static int no_block_check = !BLOCK_CHECK_DEFAULT;

/* whether to skip the printing of the summary of the number of files:
 * 0==print summary, 1==skip summary */
static int no_summary = 0;

/* whether to only to print the summary---do not print file names:
 * 0==print file names; 1==no file names */
static int no_file_names = 0;


/* OPTIONS SETUP */

typedef enum {
    OPT_NO_BLOCK_CHECK,
    OPT_NO_FILE_NAMES,
    OPT_NO_SUMMARY
} appOptionsEnum;

static struct option appOptions[] = {
    {"no-block-check",  NO_ARG,       0, OPT_NO_BLOCK_CHECK},
    {"no-file-names",   NO_ARG,       0, OPT_NO_FILE_NAMES},
    {"no-summary",      NO_ARG,       0, OPT_NO_SUMMARY},
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
    ("Do not check whether the block count of the\n"
     "\tfound files is 0. Def. "
#if BLOCK_CHECK_DEFAULT
     "Check the block count"
#else
     "Do not check the block count"
#endif
     ),
    ("Do not print the names of files that were\n"
     "\tsuccessfully found.  Def. Print file names"),
    ("Do not print the summary line listing the number of\n"
     "\tfiles that were found. Def. Print summary"),
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
    ("<SWITCHES>\n"                                                     \
     "\tA utility to simply print to stdout the list of files\n"        \
     "\tthat rwfilter would normally process for a given set of\n"      \
     "\tfile selection switches.\n")

    FILE *fh = USAGE_FH;
    int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nAPPLICATION SWITCHES\n");
    skOptionsDefaultUsage(fh);
    for (i = 0; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. %s\n", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]), appHelp[i]);
    }
    fglobUsage(fh);
}


/*
 *  appTeardown()
 *
 *    Teardown all modules, close all files, and tidy up all
 *    application state.
 *
 *    This function is idempotent.
 *
 *    NOTE: Make this extern due to declaration in rwfilter.h.
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

    /* cleanup */
    fglobTeardown();

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
 *
 *    NOTE: Make this extern due to declaration in rwfilter.h.
 */
void
appSetup(
    int                 argc,
    char              **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);
    int rv;

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

    /* Set up the fglob module */
    if (fglobSetup()){
        skAppPrintErr("Unable to setup fglob module");
        exit(EXIT_FAILURE);
    }

    /* register the teardown handler */
    if (atexit(appTeardown) < 0) {
        skAppPrintErr("Unable to register appTeardown() with atexit()");
        appTeardown();
        exit(EXIT_FAILURE);
    }

    /* Parse the options */
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        skAppUsage();
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* check for extraneous arguments */
    if (arg_index != argc) {
        skAppPrintErr("Too many arguments or unrecognized switch '%s'",
                      argv[arg_index]);
        skAppUsage();
    }

    /* verify that an fglob option was provided and that everything is
     * ok with the setup. */
    rv = fglobValid();
    if (rv == -1) {
        exit(EXIT_FAILURE);
    }
    if (rv == 0) {
        skAppPrintErr("Must specify at least one file selection switch");
        exit(EXIT_FAILURE);
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
    char        UNUSED(*opt_arg))
{
    switch ((appOptionsEnum)opt_index) {
      case OPT_NO_BLOCK_CHECK:
        no_block_check = 1;
        break;

      case OPT_NO_FILE_NAMES:
        no_file_names = 1;
        break;

      case OPT_NO_SUMMARY:
        no_summary = 1;
        break;
    }

    return 0;  /* OK */
}


int main(int argc, char **argv)
{
    char pathname[PATH_MAX];
    struct stat statbuf;
    unsigned int numFiles = 0;
    unsigned int numOnTape = 0;


    appSetup(argc, argv);

    switch ((no_block_check << 1) | (no_file_names)) {
      case 3:
        /* do not stat() the files; do not print file names */
        while (fglobNext(pathname, sizeof(pathname)) != NULL) {
            ++numFiles;
        }
        break;

      case 2:
        /* do not stat() the files; print file names */
        while (fglobNext(pathname, sizeof(pathname)) != NULL) {
            fprintf(OUTPUT_FH, "%s\n", pathname);
            ++numFiles;
        }
        break;

      case 1:
        /* stat the files; do not print the file names */
        while (fglobNext(pathname, sizeof(pathname)) != NULL) {
            if (-1 == stat(pathname, &statbuf)) {
                /* should never happen; fglob wouldn't have returned it */
                skAppPrintSyserror("Cannot stat '%s'", pathname);
                exit(EXIT_FAILURE);
            }
            if (0 == statbuf.st_blocks && statbuf.st_size > 0) {
                ++numOnTape;
            }
            ++numFiles;
        }
        break;

      case 0:
        /* stat the files; print the file names */
        while (fglobNext(pathname, sizeof(pathname)) != NULL) {
            if (-1 == stat(pathname, &statbuf)) {
                /* should never happen; fglob wouldn't have returned it */
                skAppPrintSyserror("Cannot stat '%s'", pathname);
                exit(EXIT_FAILURE);
            }
            if (0 == statbuf.st_blocks && statbuf.st_size > 0) {
                fprintf(OUTPUT_FH, "%s%s\n", BLOCK_CHECK_ZERO_MSG, pathname);
                ++numOnTape;
            } else {
                fprintf(OUTPUT_FH, "%s\n", pathname);
            }
            ++numFiles;
        }
        break;

      default:
        skAbortBadCase((no_block_check << 1) | (no_file_names));
    }

    if (!no_summary) {
        if (no_block_check) {
            fprintf(OUTPUT_FH, "globbed %u files\n",
                    numFiles);
        } else {
            fprintf(OUTPUT_FH, "globbed %u files; %u on tape\n",
                    numFiles, numOnTape);
        }
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
