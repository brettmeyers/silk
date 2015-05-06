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
 * rwguess.c
 *
 * rwguess reads a PDU file and determines which SNMP interfaces it
 * uses.  It presents those as a sorted list based on the number of
 * records each interface sees.
 *
 */

#include <silk/silk.h>

RCSIDENT("$SiLK: rwguess.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/utils.h>
#include <silk/rwrec.h>
#include <silk/libflowsource.h>
#include <silk/skheap.h>
#include <silk/sklog.h>


/* TYPEDEFS AND DEFINES */

#define RWGUESS_DEFAULT_TOP 10

/* file handle for --help usage message */
#define USAGE_FH stdout


/* LOCAL VARIABLES */

/* count of records seen at each interface */
static uint32_t rec_count[2][SK_SNMP_INDEX_LIMIT];

/* title for each interface */
static const char *snmp_title[] = {"Input", "Output"};

/* number of top interfaces to print */
static int top = -1;

/* whether to print all interfaces */
static int print_all = 0;

/* global used during the printing stage; needed by heap-compare function */
static int sort_idx;

/* index into the argv[] array */
static int arg_index;


/* OPTIONS SETUP */

typedef enum {
    OPT_TOP,
    OPT_PRINT_ALL
} appOptionsEnum;

static struct option appOptions[] = {
    {"top",             REQUIRED_ARG, 0, OPT_TOP},
    {"print-all",       NO_ARG,       0, OPT_PRINT_ALL},
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
    "Specify the number of top-N entries to print. Def. 10",
    "Print all indices sorted by interface number. Def. No",
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  analyze(const char *file_name);
static void printAll(void);
static void printTop(void);
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
#define USAGE_MSG                                                       \
    ("[SWITCHES] <PDU_FILENAME>\n"                                           \
     "\tPrints to stdout the SNMP interfaces in <PDU_FILENAME> that saw\n"   \
     "\ttraffic.  Output is either the top-N input and output interfaces,\n" \
     "\tor all interfaces that saw traffic sorted by the index.\n"           \
     "\tAs of SiLK 3.8.3, rwguess is deprecated; details in manual page.\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
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
    uint32_t u32;
    int rv;

    switch (opt_index) {
      case OPT_TOP:
        rv = skStringParseUint32(&u32, opt_arg, 1, SK_SNMP_INDEX_LIMIT);
        if (rv) {
            skAppPrintErr("Invalid %s '%s': %s",
                          appOptions[opt_index].name, opt_arg,
                          skStringParseStrerror(rv));
            return 1;
        }
        top = (int)u32;
        break;

      case OPT_PRINT_ALL:
        print_all = 1;
        break;
    }

    return 0;                   /* OK */
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
    static uint8_t teardownFlag = 0;
    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* set level to "emerg" to avoid the "Stopped logging" message */
    sklogSetLevel("emerg");
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

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    memset(rec_count, 0, sizeof(rec_count));

    /* set up logging so sklogSetLevel() in appTeardown works */
    sklogSetup(0);

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

    /* parse options. If OK, arg_index is the first file */
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        skAppUsage();           /* never returns */
    }

    if (top == -1) {
        top = RWGUESS_DEFAULT_TOP;
    } else if (print_all) {
        skAppPrintErr("May only specify one of --%s or --%s",
                      appOptions[OPT_TOP].name,appOptions[OPT_PRINT_ALL].name);
        skAppUsage();       /* never returns */
    }

    if (arg_index == argc) {
        skAppPrintErr("No PDU input files specied on the command line");
        skAppUsage();           /* never returns */
    }

    /* Must enable the logger */
    sklogSetDestination("stderr");
    sklogSetStampFunction(&logprefix);
    sklogOpen();

    return;                     /* OK */
}


/*
 *    Prefix any error messages from libflowsource with the program
 *    name instead of the standard logging tag.
 */
static size_t
logprefix(
    char               *buffer,
    size_t              bufsize)
{
    return (size_t)snprintf(buffer, bufsize, "%s: ", skAppName());
}


/*
 *  ok = analyze(filename);
 *
 *    Read PDUs from 'filename' and increment the index counts in the
 *    global 'rec_count' array.
 */
static int
analyze(
    const char         *file_name)
{
    skpc_probe_t *probe;
    skPDUSource_t *pdu_src;
    skFlowSourceParams_t params;
    rwRec rwrec;

    if (skpcSetup()) {
        return -1;
    }
    if (skpcProbeCreate(&probe)) {
        return -1;
    }
    skpcProbeSetName(probe, "rwguess");
    skpcProbeSetType(probe, PROBE_ENUM_NETFLOW_V5);
    skpcProbeSetFileSource(probe, file_name);
    skpcProbeClearLogFlags(probe);
    if (skpcProbeVerify(probe, 0)) {
        return -1;
    }

    params.path_name = file_name;

    pdu_src = skPDUSourceCreate(probe, &params);
    if (pdu_src == NULL) {
        return -1;
    }

    while (-1 != skPDUSourceGetGeneric(pdu_src, &rwrec)) {
        ++rec_count[0][rwRecGetInput(&rwrec)];
        ++rec_count[1][rwRecGetOutput(&rwrec)];
    }

    skPDUSourceDestroy(pdu_src);
    skpcTeardown();

    return 0;
}


static void
printAll(
    void)
{
    int i;

    /* title */
    fprintf(stdout, "%5s|%7s_Recs|%7s_Recs|\n",
            "Index", snmp_title[0], snmp_title[1]);

    /* record counts */
    for (i = 0; i < SK_SNMP_INDEX_LIMIT; i++) {
        if (rec_count[0][i] || rec_count[1][i]) {
            fprintf(stdout, "%5d|%12" PRIu32 "|%12" PRIu32 "|\n",
                    i, rec_count[0][i], rec_count[1][i]);
        }
    }
}


static int
compareHeapNodes(
    skheapnode_t        node1,
    skheapnode_t        node2)
{
    if (rec_count[sort_idx][*(int32_t*)node1]
        < rec_count[sort_idx][*(int32_t*)node2])
    {
        return -1;
    }
    if (rec_count[sort_idx][*(int32_t*)node1]
        > rec_count[sort_idx][*(int32_t*)node2])
    {
        return 1;
    }
    return 0;
}


static void
printTop(
    void)
{
    int entries;
    skheap_t *heap;
    int32_t top_heap;
    int32_t i;

    /* create the heap, which will index the SNMP interfaces */
    heap = skHeapCreate(&compareHeapNodes, SK_SNMP_INDEX_LIMIT,
                        sizeof(int32_t), NULL);
    if (NULL == heap) {
        skAppPrintErr("Unable to create heap for sorting");
        return;
    }

    /* run loop twice, once for input and once for output */
    for (sort_idx = 0; sort_idx < 2; ++sort_idx) {
        entries = 0;

        /* add each interface that has data to the heap. */
        for (i = 0; i < SK_SNMP_INDEX_LIMIT; ++i) {
            if (rec_count[sort_idx][i]) {
                skHeapInsert(heap, &i);
                ++entries;
            }
        }

        /* title */
        fprintf(stdout, "Top %d (of %d) SNMP %s Interfaces\n",
                top, entries, snmp_title[sort_idx]);
        fprintf(stdout, "%5s|%7s_Recs|\n", "Index", snmp_title[sort_idx]);

        /* print the top entries */
        for (entries = 0;
             (skHeapExtractTop(heap,(skheapnode_t)&top_heap) !=SKHEAP_ERR_EMPTY
              && entries < top);
            ++entries)
        {
            fprintf(stdout, ("%5" PRId32 "|%12" PRIu32 "|\n"),
                    top_heap, rec_count[sort_idx][top_heap]);
        }
        fprintf(stdout, "\n");

        skHeapEmpty(heap);
    }

    skHeapFree(heap);
}


int main(int argc, char **argv)
{
    int count = 0;

    appSetup(argc, argv);       /* never returns on failure */

    /* analyze each file on the command line */
    for ( ; arg_index < argc; ++arg_index) {
        if (0 == analyze(argv[arg_index])) {
            ++count;
        }
    }

    /* print the results */
    if (count > 0) {
        if (print_all) {
            printAll();
        } else {
            printTop();
        }
    }

    return !count;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
