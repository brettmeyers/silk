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
**  Code to test the sklog module.  Note that this C files #includes
**  the sklog.c source file.
**
*/

/* set a testing flag */
#define SKLOG_TESTING_LOG 1

/* NOTE: pull in the sklog source file */
#include "sklog.c"

RCSIDENTVAR(rcsID_sklog_c, "$SiLK: sklog-test.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout


/* LOCAL VARIABLE DEFINITIONS */

/* whether to print a message using EMERGMSG(); usually it is skipped */
static int test_emerg = 0;

/* whether to test threaded logging; default is no */
static int test_threaded = 0;

/* number of times to write a log message */
static uint32_t repeat_count = 5;

/* number of seconds to wait between writing to the log */
static uint32_t repeat_delay = 10;

/* number of times the logToAllLevels() function has been called */
static int global_count = 0;

/* mutex to protect the count */
static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;


/* OPTIONS SETUP */

typedef enum {
    OPT_TEST_FEATURES,
    OPT_TEST_EMERG,
    OPT_TEST_THREADED,
    OPT_REPEAT_COUNT,
    OPT_REPEAT_DELAY
} appOptionsEnum;

static struct option appOptions[] = {
    {"test-features",   REQUIRED_ARG, 0, OPT_TEST_FEATURES},
    {"test-emerg",      NO_ARG,       0, OPT_TEST_EMERG},
    {"test-threaded",   NO_ARG,       0, OPT_TEST_THREADED},
    {"repeat-count",    REQUIRED_ARG, 0, OPT_REPEAT_COUNT},
    {"repeat-delay",    REQUIRED_ARG, 0, OPT_REPEAT_DELAY},
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
    ("The features to test.  Passed to sklogSetup(). Sum of:\n"
     "\t1  Enable options for use of syslog\n"
     "\t2  Enable options that mimic SiLK legacy logging"),
    "Test EMERGMSG() as well (usually it is skipped)",
    "Test threaded logging. Def. no",
    "Number of times to write messages to the log. Def. 5",
    "Number of seconds between writes to the log. Def. 10",
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
    ("--test-features=FEATURES [SWITCHES]\n"                            \
     "\tSimple code to test the sklog module.\n"                        \
     "\tUse ``--test-features=FEATURES --help'' to see the options\n"   \
     "\tthat sklog will provide for various feature levels.\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);

    fprintf(fh, "\nLog switches:\n");
    sklogOptionsUsage(fh);
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

    sklogTeardown();
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

    /* parse the options */
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        /* options parsing should print error */
        skAppUsage();           /* never returns */
    }

    /* check for extraneous arguments */
    if (arg_index != argc) {
        skAppPrintErr("Too many arguments or unrecognized switch '%s'",
                      argv[arg_index]);
        skAppUsage();           /* never returns */
    }

    /* verify logging */
    if (sklogOptionsVerify()) {
        exit(EXIT_FAILURE);
    }

    /* write command line */
    sklogCommandLine(argc, argv);

    /* register the teardown handler */
    if (atexit(appTeardown) < 0) {
        skAppPrintErr("Unable to register appTeardown() with atexit()");
        appTeardown();
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
    uint32_t val;
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_TEST_FEATURES:
        rv = skStringParseUint32(&val, opt_arg, 0,
                                 (SKLOG_FEATURE_LEGACY | SKLOG_FEATURE_SYSLOG));
        if (rv) {
            goto PARSE_ERROR;
        }
        if (sklogSetup((int)val)) {
            skAppPrintErr("Unable to setup log");
            exit(EXIT_FAILURE);
        }
        break;

      case OPT_TEST_EMERG:
        test_emerg = 1;
        break;

      case OPT_TEST_THREADED:
        test_threaded = 1;
        sklogEnableThreadedLogging();
        break;

      case OPT_REPEAT_COUNT:
        rv = skStringParseUint32(&val, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        repeat_count = val;
        break;

      case OPT_REPEAT_DELAY:
        rv = skStringParseUint32(&val, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        repeat_delay = val;
        break;
    }

    return 0;  /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  appOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;
}


static void
logToAllLevels(
    void               *v_name)
{
#define ST_ND_TH(c) \
    ((((c) % 10) == 1) ? "st" : ((((c) % 10) == 2) ? "nd" : "th"))

    int c;
    char *name = (char *)v_name;

    pthread_mutex_lock(&count_mutex);
    c = ++global_count;
    pthread_mutex_unlock(&count_mutex);

    if (test_emerg) {
        EMERGMSG("Writing a EMERGMSG for the %d%s time [%s]",
                 c, ST_ND_TH(c), name);
    }
    ALERTMSG("Writing a ALERTMSG for the %d%s time [%s]",
             c, ST_ND_TH(c), name);
    CRITMSG("Writing a CRITMSG for the %d%s time [%s]",
            c, ST_ND_TH(c), name);
    ERRMSG("Writing a ERRMSG for the %d%s time [%s]",
           c, ST_ND_TH(c), name);
    WARNINGMSG("Writing a WARNINGMSG for the %d%s time [%s]",
               c, ST_ND_TH(c), name);
    NOTICEMSG("Writing a NOTICEMSG for the %d%s time [%s]",
            c, ST_ND_TH(c), name);
    INFOMSG("Writing a INFOMSG for the %d%s time [%s]",
            c, ST_ND_TH(c), name);
    DEBUGMSG("Writing a DEBUGMSG for the %d%s time [%s]",
             c, ST_ND_TH(c), name);
}


/*
 *  THREAD ENTRY POINT
 */
static void *
write_msg_thread(
    void               *v_name)
{
    uint32_t i;

    for (i = 0; i < repeat_count; ++i) {
        if (i > 0) {
            sleep(repeat_delay);
        }
        logToAllLevels(v_name);
    }

    return v_name;
}


int main(int argc, char **argv)
{
    appSetup(argc, argv);                       /* never returns on error */

    if (sklogOpen()) {
        skAppPrintErr("Unable to open log");
    }

    if (!test_threaded) {
        write_msg_thread((void*)"main");
    } else {
        pthread_t p1;
        pthread_t p2;

        pthread_create(&p1, NULL, &write_msg_thread, (void*)"p1");
        pthread_create(&p2, NULL, &write_msg_thread, (void*)"p2");
        logToAllLevels((void*)"main");
        pthread_join(p2, NULL);
        pthread_join(p1, NULL);
    }

    sklogClose();

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
