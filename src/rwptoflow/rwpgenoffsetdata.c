/*
** Copyright (C) 2005-2015 by Carnegie Mellon University.
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
** Take a tcpdump capture file as input, and output the same file, but
** with timestamps skewed by 0-4 milliseconds.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwpgenoffsetdata.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwppacketheaders.h"


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write output from --help */
#define USAGE_FH stdout


/* LOCAL VARIABLES */

static pcap_t *g_pkt_input = NULL;
static pcap_t *g_output;
static pcap_dumper_t *g_output_dumper = NULL;


/* OPTION SETUP */

typedef enum {
    OPT_HELP /* remove this when real options added */
} appOptionsEnum;


static struct option appOptions[] = {
    {0, 0, 0, 0}         /* sentinel entry */
};


static const char *appHelp[] = {
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData  cData, int opt_index, char *opt_arg);


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
#define USAGE_MSG                                                            \
    ("<TCPDUMP_FILE>\n"                                                      \
     "\tOpens TCPDUMP_FILE, offsets the times in the packets, and writes\n"  \
     "\tthe resulting packets in TCPDUMP format to the standard output,\n"   \
     "\twhich must not be connected to a terminal.\n")

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
    static uint8_t teardownFlag = 0;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* Close all files */
    if (g_pkt_input) {
        pcap_close(g_pkt_input);
    }
    if (g_output_dumper) {
        pcap_dump_close(g_output_dumper);
    }
    if (g_output) {
        pcap_close(g_output);
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
    char errbuf[PCAP_ERRBUF_SIZE];
    int arg_index;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char*)) ==
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

    /* parse options */
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        skAppUsage(); /* never returns */
    }

    /* verify input file */
    if (arg_index >= argc) {
        skAppPrintErr("No input packet file found");
        skAppUsage();             /* never returns */
    }

    /* verify output is not a terminal */
    if (FILEIsATty(stdout)) {
        skAppPrintErr("stdout is connected to a terminal");
        skAppUsage();             /* never returns */
    }

    /* open input file */
    g_pkt_input = pcap_open_offline(argv[arg_index], errbuf);
    if (g_pkt_input == NULL) {
        skAppPrintErr("Unable to open input file %s: %s",
                      argv[arg_index], errbuf);
        exit(EXIT_FAILURE);
    }

    /* open output file (tcpdump format packet file) */
    g_output = pcap_open_dead(pcap_datalink(g_pkt_input),
                              pcap_snapshot(g_pkt_input));
    if (g_output == NULL) {
        skAppPrintErr("Error opening stdout: %s", errbuf);
        exit(EXIT_FAILURE);
    }

    g_output_dumper = pcap_dump_open(g_output, "-");
    if (g_output_dumper == NULL) {
        skAppPrintErr("Error opening stdout: %s", pcap_geterr(g_output));
        exit(EXIT_FAILURE);
    }

    return; /* OK */
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
      case OPT_HELP: /* remove this when options are added */
        assert(0);
        skAbort();
        break;
    }

    return 0; /* OK */
}



int main(int argc, char **argv)
{
    int offset = 0;
    struct pcap_pkthdr next_pkt_hdr;
    const u_char *next_pkt_data = NULL;

    appSetup(argc, argv);

    while ((next_pkt_data = pcap_next(g_pkt_input, &next_pkt_hdr)) != NULL) {
        next_pkt_hdr.ts.tv_usec += offset % 5000;
        pcap_dump((u_char *) g_output_dumper, &next_pkt_hdr,
                  next_pkt_data);
        offset += 1000;
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
