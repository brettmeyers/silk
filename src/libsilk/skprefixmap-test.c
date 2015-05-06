/*
** Copyright (C) 2004-2015 by Carnegie Mellon University.
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
** prefixmap_test.c
**
** John McClary Prevost
** December 3rd, 2004
**
** Small application to test the prefixmap library by taking a
** prefixmap file and an IP address and do a lookup on that file to
** print the result.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skprefixmap-test.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skprefixmap.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* max expected length of a pmap dictionary entry */
#define DICTIONARY_ENTRY_BUFLEN 2048


/* EXPORTED FUNCTIONS */


/* LOCAL VARIABLES */



/* OPTIONS SETUP */

typedef enum {
    OPT_MAP_FILE, OPT_ADDRESS, OPT_STRING
} appOptionsEnum;

static struct option appOptions[] = {
    {"map-file", REQUIRED_ARG, 0, OPT_MAP_FILE},
    {"address",  REQUIRED_ARG, 0, OPT_ADDRESS},
    {"string",   NO_ARG,       0, OPT_STRING},
    {0,0,0,0}                   /* sentinel entry */
};

static const char *appHelp[] = {
    /* add help strings here for the applications options */
    "path name of the map file.",
    "IP address to look up",
    "output dictionary string instead of integer value",
    (char *)NULL
};

static struct {
    const char *map_file;       /* filename of map file */
    skipaddr_t  address;        /* IP address to look up */
    uint32_t    have_address;   /* whether an address was given */
    uint32_t    string;         /* look up string */
} prefixmap_test_opt;


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
    ("[SWITCHES]\n"                                                     \
     "\tSmall application to test the prefixmap library by taking a\n"  \
     "\tprefixmap file and an IP address and searching the file to\n"   \
     "\tprint the result.\n")

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

    memset(&prefixmap_test_opt, 0, sizeof(prefixmap_test_opt));

    /* register the teardown handler */
    if (atexit(appTeardown) < 0) {
        skAppPrintErr("Unable to register appTeardown() with atexit()");
        appTeardown();
        exit(EXIT_FAILURE);
    }

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL))
    {
        skAppPrintErr("Unable to register options");
        exit(EXIT_FAILURE);
    }

    /* parse the options */
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        /* options parsing should print error */
        skAppUsage();           /* never returns */
    }

    if ( (NULL == prefixmap_test_opt.map_file) ||
         (0 == prefixmap_test_opt.have_address) ) {
        if ( NULL == prefixmap_test_opt.map_file ) {
            skAppPrintErr("Required argument map-file not provided.");
        }
        if ( 0 == prefixmap_test_opt.have_address ) {
            skAppPrintErr("Required argument address not provided.");
        }
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
    char               *opt_arg)
{
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_MAP_FILE:
        prefixmap_test_opt.map_file = opt_arg;
        break;

      case OPT_ADDRESS:
        rv = skStringParseIP(&prefixmap_test_opt.address, opt_arg);
        if (rv) {
            skAppPrintErr("Invalid %s '%s': %s",
                          appOptions[opt_index].name, opt_arg,
                          skStringParseStrerror(rv));
            exit(EXIT_FAILURE);
        }
        prefixmap_test_opt.have_address = 1;
        break;

    case OPT_STRING:
        prefixmap_test_opt.string = 1;
        break;
    }

    return 0;  /* OK */
}


int main(int argc, char **argv)
{
    skstream_t *inputFile;
    skPrefixMap_t *prefixMap;
    skPrefixMapErr_t map_error = SKPREFIXMAP_OK;
    char buf[DICTIONARY_ENTRY_BUFLEN];
    int rv;

    appSetup(argc, argv);                       /* never returns on error */

    /* Okay.  Now we should open the prefixmap file, read it in, and */
    /* then look up our address! */

    if ((rv = skStreamCreate(&inputFile, SK_IO_READ, SK_CONTENT_SILK))
        || (rv = skStreamBind(inputFile, prefixmap_test_opt.map_file))
        || (rv = skStreamOpen(inputFile)))
    {
        skStreamPrintLastErr(inputFile, rv, &skAppPrintErr);
        skStreamDestroy(&inputFile);
        exit(EXIT_FAILURE);
    }

    map_error = skPrefixMapRead(&prefixMap, inputFile);
    skStreamDestroy(&inputFile);

    if ( SKPREFIXMAP_OK != map_error ) {
        skAppPrintErr("Failed to read map file: %s",
                      skPrefixMapStrerror(map_error));
        exit(EXIT_FAILURE);
    }

    if ( prefixmap_test_opt.string ) {
        int v = skPrefixMapFindString(prefixMap, &prefixmap_test_opt.address,
                                      buf, sizeof(buf));
        if ( v < 0 ) {
            strncpy(buf, "(null)", sizeof(buf));
        }
        printf("%s\n", buf);
    } else {
        printf("%d\n",
               skPrefixMapFindValue(prefixMap, &prefixmap_test_opt.address));
    }

    skPrefixMapDelete(prefixMap);

    /* done */
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
