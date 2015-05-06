/*
** Copyright (C) 2009-2015 by Carnegie Mellon University.
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
**  rwcompare.c
**
**    Compare SiLK files to determine if they contain the same data.
**
**    Mark Thomas
**    April 2009
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwcompare.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout


/* LOCAL VARIABLE DEFINITIONS */

/* index into argv */
static int arg_index;

/* whether to print the record that differs or just exit quietly */
static int quiet = 0;


/* OPTIONS SETUP */

typedef enum {
    OPT_QUIET
} appOptionsEnum;

static struct option appOptions[] = {
    {"quiet",           NO_ARG,     0, OPT_QUIET},
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
    "Do not print any output",
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
    ("[SWITCHES] FILE1 FILE2\n"                                               \
     "\tCompare the SiLK Flow records in FILE1 and FILE2.  Print nothing\n"   \
     "\tand exit with status 0 if the SiLK Flow records in the two files\n"   \
     "\tare identical.  Else, print the record where files differ and exit\n" \
     "\twith status 1.\n")

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
        /* options parsing should print error */
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* arg_index is looking at first file name to process */
    if (arg_index + 2 != argc) {
        skAppPrintErr("Expected two file names on the command line");
        skAppUsage();       /* never returns */
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
      case OPT_QUIET:
        quiet = 1;
        break;
    }

    return 0;  /* OK */
}


static int
compareFiles(
    char              **file)
{
    skstream_t *ios[2] = {NULL, NULL};
    rwRec rec[2];
    int i;
    int rv;
    int status = 2;
    uint64_t rec_count = 0;
    int eof = -1;

    memset(ios, 0, sizeof(ios));
    memset(rec, 0, sizeof(rec));

    for (i = 0; i < 2; ++i) {
        if ((rv = skStreamCreate(&ios[i], SK_IO_READ, SK_CONTENT_SILK_FLOW))
            || (rv = skStreamBind(ios[i], file[i]))
            || (rv = skStreamOpen(ios[i]))
            || (rv = skStreamReadSilkHeader(ios[i], NULL)))
        {
            /* Give up if we can't read the beginning of the silk header */
            if (rv != SKSTREAM_OK) {
                if (!quiet) {
                    skStreamPrintLastErr(ios[i], rv, &skAppPrintErr);
                }
                goto END;
            }
        }
    }

    while ((rv = skStreamReadRecord(ios[0], &rec[0])) == SKSTREAM_OK) {
        rv = skStreamReadRecord(ios[1], &rec[1]);
        if (rv != SKSTREAM_OK) {
            if (rv == SKSTREAM_ERR_EOF) {
                /* file 0 longer than file 1 */
                status = 1;
                eof = 1;
            } else {
                if (!quiet) {
                    skStreamPrintLastErr(ios[1], rv, &skAppPrintErr);
                }
                status = -1;
            }
            goto END;
        }

        ++rec_count;
        if (0 != memcmp(&rec[0], &rec[1], sizeof(rwRec))) {
            status = 1;
            goto END;
        }
    }

    if (rv != SKSTREAM_ERR_EOF) {
        if (!quiet) {
            skStreamPrintLastErr(ios[0], rv, &skAppPrintErr);
        }
    } else {
        rv = skStreamReadRecord(ios[1], &rec[1]);
        switch (rv) {
          case SKSTREAM_OK:
            /* file 1 longer than file 0 */
            status = 1;
            eof = 0;
            break;

          case SKSTREAM_ERR_EOF:
            /* files identical */
            status = 0;
            break;

          default:
            if (!quiet) {
                skStreamPrintLastErr(ios[1], rv, &skAppPrintErr);
            }
            break;
        }
    }

  END:
    for (i = 0; i < 2; ++i) {
        skStreamDestroy(&ios[i]);
    }
    if (1 == status && !quiet) {
        if (eof != -1) {
            printf("%s %s differ: EOF %s\n",
                   file[0], file[1], file[eof]);
        } else {
            printf(("%s %s differ: record %" PRIu64 "\n"),
                   file[0], file[1], rec_count);
        }
    }

    return status;
}


int main(int argc, char **argv)
{
    appSetup(argc, argv);                       /* never returns on error */

    return compareFiles(&argv[arg_index]);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
