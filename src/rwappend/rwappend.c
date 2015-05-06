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
**
**  rwappend.c
**
**  Suresh L Konda
**  8/10/2002
**      Append f2..fn to f1.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwappend.c fb9df4655544 2015-02-03 20:13:17Z mthomas $");

#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write output from --help */
#define USAGE_FH stdout

/* where to write output from --print-stat */
#define STATISTICS_FH  stderr


/* LOCAL VARIABLES */

/* file to append to */
static skstream_t *out_ios = NULL;

/* whether to create the out_ios file if it does not exist */
static int allow_create = 0;

/* if creating the out_ios file, this is the name of the file to
 * use as the template for the new file. */
static const char *create_format = NULL;

/* whether to print the statistics */
static int print_statistics = 0;

/* index into argv[]; used to loop over filenames */
static int arg_index;


/* OPTIONS SETUP */

typedef enum {
    OPT_CREATE, OPT_PRINT_STATISTICS
} appOptionsEnum;

static struct option appOptions[] = {
    {"create",                  OPTIONAL_ARG, 0, OPT_CREATE},
    {"print-statistics",        NO_ARG,       0, OPT_PRINT_STATISTICS},
    {0,0,0,0}                   /* sentinel entry */
};

static const char *appHelp[] = {
    ("Create the TARGET-FILE if it does not exist.  Uses the\n"
     "\toptional SiLK file argument to determine the format of TARGET-FILE.\n"
     "\tDef. Exit when TARGET-FILE nonexistent; use default format"),
    ("Print to stderr the count of records read from each\n"
     "\tSOURCE-FILE and the total records added to the TARGET-FILE. Def. No"),
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  createFromTemplate(const char *new_path, const char *templ_file);


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
    ("[SWITCHES] TARGET-FILE SOURCE-FILE1 [SOURCE-FILE2...]\n"          \
     "\tAppend the SiLK Flow records contained in the second through\n" \
     "\tfinal filename arguments to the records contained in the\n"     \
     "\tfirst filename argument.  All files must be SiLK flow files;\n" \
     "\tthe TARGET-FILE must not be compressed.\n")

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
    int rv;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* close appended-to file */
    if (out_ios) {
        rv = skStreamDestroy(&out_ios);
        if (rv) {
            skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
        }
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
    const char *output_path;
    int did_create = 0;
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

    /* parse options */
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        skAppUsage(); /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* get the output file */
    if (argc == arg_index) {
        skAppPrintErr("Missing name of TARGET-FILE");
        skAppUsage();           /* never returns */
    }

    /* get the target file */
    output_path = argv[arg_index];
    ++arg_index;

    /* If the target does not exist, complain or create it. */
    errno = 0;
    if (skFileExists(output_path)) {
        /* file exists and is a regular file */

    } else if (0 == errno) {
        /* file is not a regular file */
        skAppPrintErr("Target file '%s' is invalid: Not a regular file",
                      output_path);
        exit(EXIT_FAILURE);

    } else if (ENOENT != errno) {
        /* Some error other than "does not exist" */
        skAppPrintSyserror("Target file '%s' is invalid",
                           output_path);
        exit(EXIT_FAILURE);

    } else if (0 == allow_create) {
        /* file does not exist but --create not given */
        skAppPrintSyserror(
            "Target file '%s' is invalid and --%s not specified",
            output_path, appOptions[OPT_CREATE].name);
        exit(EXIT_FAILURE);

    } else {
        /* create the file */
        did_create = 1;
        if (createFromTemplate(output_path, create_format)) {
            exit(EXIT_FAILURE);
        }
    }

    /* open the target file for append */
    rv = skStreamOpenSilkFlow(&out_ios, output_path, SK_IO_APPEND);
    if (rv) {
        if (did_create) {
            skAppPrintErr("Unable to open newly created target file '%s'",
                          output_path);
        }
        skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
        skStreamDestroy(&out_ios);
        exit(EXIT_FAILURE);
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
    switch ((appOptionsEnum)opt_index) {
      case OPT_CREATE:
        allow_create = 1;
        if (opt_arg) {
            errno = 0;
            if ( !skFileExists(opt_arg)) {
                skAppPrintSyserror("Invalid %s '%s'",
                                   appOptions[opt_index].name, opt_arg);
                return 1;
            }
            create_format = opt_arg;
        }
        break;

      case OPT_PRINT_STATISTICS:
        print_statistics = 1;
        break;
    }

    return 0;
}


/*
 *  status = createFromTemplate(new_path, templ_file);
 *
 *    Create a SiLK flow file at 'new_path'.  It should have the same
 *    format, version, and byte order as 'templ_file'.  If 'templ_file' is
 *    NULL, create 'new_file' in the default format.  Return 0 on
 *    success; non-zero otherwise.
 */
static int
createFromTemplate(
    const char         *new_path,
    const char         *templ_file)
{
    skstream_t *new_ios = NULL;
    skstream_t *ios = NULL;
    int rv, rv_temp;

    /* open the target file for write---this will create the file */
    if ((rv = skStreamCreate(&new_ios, SK_IO_WRITE, SK_CONTENT_SILK_FLOW))
        || (rv = skStreamBind(new_ios, new_path)))
    {
        goto END;
    }

    /* set file attributes based on the template if given */
    if (templ_file) {
        /* open the template file */
        rv_temp = skStreamOpenSilkFlow(&ios, templ_file, SK_IO_READ);
        if (rv_temp) {
            skStreamPrintLastErr(ios, rv_temp, &skAppPrintErr);
            skAppPrintErr("Cannot open template file '%s'",
                          templ_file);
            skStreamDestroy(&ios);
            skStreamDestroy(&new_ios);
            return 1;
        }

        rv = skHeaderCopy(skStreamGetSilkHeader(new_ios),
                          skStreamGetSilkHeader(ios),
                          SKHDR_CP_ALL);
    }

    /* open the target file, write the header, then close it */
    if (rv == SKSTREAM_OK) {
        rv = skStreamOpen(new_ios);
    }
    if (rv == SKSTREAM_OK) {
        rv = skStreamWriteSilkHeader(new_ios);
    }
    if (rv == SKSTREAM_OK) {
        rv = skStreamClose(new_ios);
    }

  END:
    if (rv != SKSTREAM_OK) {
        skStreamPrintLastErr(new_ios, rv, &skAppPrintErr);
        skAppPrintErr("Cannot create output file '%s'", new_path);
    }
    skStreamDestroy(&ios);
    skStreamDestroy(&new_ios);
    return rv;
}


int main(int argc, char **argv)
{
    const char *input_path;
    skstream_t *in_ios;
    rwRec rwrec;
    int file_count = 0;
    int rv;

    appSetup(argc, argv);                 /* never returns on error */

    /* loop over the source files */
    for ( ; arg_index < argc; ++arg_index) {
        input_path = argv[arg_index];

        /* skip files that are identical to the target or that we
         * cannot open */
        if (0 == strcmp(input_path, skStreamGetPathname(out_ios))) {
            skAppPrintErr(("Warning: skipping source-file%d:"
                           " identical to target file '%s'"),
                          file_count, input_path);
            continue;
        }
        rv = skStreamOpenSilkFlow(&in_ios, input_path, SK_IO_READ);
        if (rv) {
            skStreamPrintLastErr(in_ios, rv, &skAppPrintErr);
            skStreamDestroy(&in_ios);
            continue;
        }

        /* determine whether target file supports IPv6; if not, ignore
         * IPv6 flows */
        if (skStreamGetSupportsIPv6(out_ios) == 0) {
            skStreamSetIPv6Policy(in_ios, SK_IPV6POLICY_ASV4);
        }

        while ((rv = skStreamReadRecord(in_ios, &rwrec)) == SKSTREAM_OK) {
            rv = skStreamWriteRecord(out_ios, &rwrec);
            if (rv) {
                skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
                if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                    skStreamDestroy(&in_ios);
                    goto END;
                }
            }
        }
        if (rv != SKSTREAM_ERR_EOF) {
            skStreamPrintLastErr(in_ios, rv, &skAppPrintErr);
        }

        if (print_statistics) {
            ++file_count;
            fprintf(STATISTICS_FH,
                    ("%s: appended %" PRIu64 " records from %s to %s\n"),
                    skAppName(), skStreamGetRecordCount(in_ios),
                    skStreamGetPathname(in_ios),
                    skStreamGetPathname(out_ios));
        }
        skStreamDestroy(&in_ios);
    }

    /* close target */
    rv = skStreamClose(out_ios);
    if (rv) {
        skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
    }

    if (print_statistics) {
        fprintf(STATISTICS_FH,
                ("%s: appended %" PRIu64 " records from %d file%s to %s\n"),
                skAppName(), skStreamGetRecordCount(out_ios),
                file_count, ((file_count == 1) ? "" : "s"),
                skStreamGetPathname(out_ios));
    }

  END:
    skStreamDestroy(&out_ios);
    appTeardown();

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
