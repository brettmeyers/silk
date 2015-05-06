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
**  rwbag
**
**    Build binary Bag files from flow records.
**
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwbag.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwrec.h>
#include <silk/skbag.h>
#include <silk/skipaddr.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* number of key-types (e.g., sIP, dIP, ...) supported */
#define NUM_KEY_TYPES  9

/* number of potential output bags:
 *   NUM_KEY_TYPES * {flows,pkts,bytes} */
#define MAX_NUM_OUTPUTS (NUM_KEY_TYPES * 3)

typedef struct bagfile_st {
    /* the bag object */
    skBag_t            *bag;
    /* where to send the output of the bag */
    skstream_t         *stream;
    /* the type of bag (an appOptionsEnum value) */
    int                 key_value;
    /* whether this bag has had an overflow condition in one or more
     * of its counters */
    int                 overflow;
} bagfile_t;


/* LOCAL VARIABLES */

/* the potential bag files to create */
static bagfile_t bag_io[MAX_NUM_OUTPUTS];

/* number of bags requested---number of valid entries in bag_io */
static int num_outputs = 0;

/* bags are arranged in bag_io[] with the bags that have IP-keys at
 * the end.  this is the number of non-IP bags in bag_io[]---as well
 * as the index of the first IP bag in the array (if any) */
static int num_non_ip_outputs = 0;

/* the compression method to use when writing the files.
 * sksiteCompmethodOptionsRegister() will set this to the default or
 * to the value the user specifies. */
static sk_compmethod_t comp_method;

/* support for handling inputs */
static sk_options_ctx_t *optctx = NULL;

/* how to handle IPv6 flows */
static sk_ipv6policy_t ipv6_policy = SK_IPV6POLICY_MIX;

/*
 * stdout_used is set to 1 by prepareBagFile() when a bag file is to
 * be written to stdout.  ensures only one stream sets uses it.
 */
static int stdout_used = 0;

/* map from command line switches to types for key/counter.  The order
 * of the entries must be kept in sync with the appOptionsEnum. */
static const struct bag_types_st {
    skBagFieldType_t    key;
    skBagFieldType_t    counter;
} bag_types[] = {
    {SKBAG_FIELD_SIPv4, SKBAG_FIELD_RECORDS},       /* SIP_FLOWS */
    {SKBAG_FIELD_SIPv4, SKBAG_FIELD_SUM_PACKETS},   /* SIP_PKTS */
    {SKBAG_FIELD_SIPv4, SKBAG_FIELD_SUM_BYTES},     /* SIP_BYTES */
    {SKBAG_FIELD_DIPv4, SKBAG_FIELD_RECORDS},       /* DIP_FLOWS */
    {SKBAG_FIELD_DIPv4, SKBAG_FIELD_SUM_PACKETS},   /* DIP_PKTS */
    {SKBAG_FIELD_DIPv4, SKBAG_FIELD_SUM_BYTES},     /* DIP_BYTES */
    {SKBAG_FIELD_NHIPv4, SKBAG_FIELD_RECORDS},      /* NHIP_FLOWS */
    {SKBAG_FIELD_NHIPv4, SKBAG_FIELD_SUM_PACKETS},  /* NHIP_PKTS */
    {SKBAG_FIELD_NHIPv4, SKBAG_FIELD_SUM_BYTES},    /* NHIP_BYTES */
    {SKBAG_FIELD_SPORT, SKBAG_FIELD_RECORDS},       /* SPORT_FLOWS */
    {SKBAG_FIELD_SPORT, SKBAG_FIELD_SUM_PACKETS},   /* SPORT_PKTS */
    {SKBAG_FIELD_SPORT, SKBAG_FIELD_SUM_BYTES},     /* SPORT_BYTES */
    {SKBAG_FIELD_DPORT, SKBAG_FIELD_RECORDS},       /* DPORT_FLOWS */
    {SKBAG_FIELD_DPORT, SKBAG_FIELD_SUM_PACKETS},   /* DPORT_PKTS */
    {SKBAG_FIELD_DPORT, SKBAG_FIELD_SUM_BYTES},     /* DPORT_BYTES */
    {SKBAG_FIELD_PROTO, SKBAG_FIELD_RECORDS},       /* PROTO_FLOWS */
    {SKBAG_FIELD_PROTO, SKBAG_FIELD_SUM_PACKETS},   /* PROTO_PKTS */
    {SKBAG_FIELD_PROTO, SKBAG_FIELD_SUM_BYTES},     /* PROTO_BYTES */
    {SKBAG_FIELD_SID, SKBAG_FIELD_RECORDS},         /* SID_FLOWS */
    {SKBAG_FIELD_SID, SKBAG_FIELD_SUM_PACKETS},     /* SID_PKTS */
    {SKBAG_FIELD_SID, SKBAG_FIELD_SUM_BYTES},       /* SID_BYTES */
    {SKBAG_FIELD_INPUT, SKBAG_FIELD_RECORDS},       /* INPUT_FLOWS */
    {SKBAG_FIELD_INPUT, SKBAG_FIELD_SUM_PACKETS},   /* INPUT_PKTS */
    {SKBAG_FIELD_INPUT, SKBAG_FIELD_SUM_BYTES},     /* INPUT_BYTES */
    {SKBAG_FIELD_OUTPUT, SKBAG_FIELD_RECORDS},      /* OUTPUT_FLOWS */
    {SKBAG_FIELD_OUTPUT, SKBAG_FIELD_SUM_PACKETS},  /* OUTPUT_PKTS */
    {SKBAG_FIELD_OUTPUT, SKBAG_FIELD_SUM_BYTES}     /* OUTPUT_BYTES */
};


/* OPTIONS SETUP */

typedef enum {
    /* These MUST be kept in order with the options */
    SIP_FLOWS=0, SIP_PKTS,  SIP_BYTES,
    DIP_FLOWS, DIP_PKTS, DIP_BYTES,
    NHIP_FLOWS, NHIP_PKTS, NHIP_BYTES,
    SPORT_FLOWS, SPORT_PKTS, SPORT_BYTES,
    DPORT_FLOWS, DPORT_PKTS, DPORT_BYTES,
    PROTO_FLOWS, PROTO_PKTS, PROTO_BYTES,
    SID_FLOWS, SID_PKTS, SID_BYTES,
    INPUT_FLOWS, INPUT_PKTS, INPUT_BYTES,
    OUTPUT_FLOWS, OUTPUT_PKTS, OUTPUT_BYTES
} appOptionsEnum;

/* This #define is used to denote first bag that does not have an IP
 * value as its key. */
#define FIRST_NON_IP_BAG  SPORT_FLOWS

/* this is the final value related to bag creatation */
#define FINAL_BAG_KEY     OUTPUT_BYTES

static struct option appOptions[] = {
    {"sip-flows",       REQUIRED_ARG, 0, SIP_FLOWS},
    {"sip-packets",     REQUIRED_ARG, 0, SIP_PKTS},
    {"sip-bytes",       REQUIRED_ARG, 0, SIP_BYTES},
    {"dip-flows",       REQUIRED_ARG, 0, DIP_FLOWS},
    {"dip-packets",     REQUIRED_ARG, 0, DIP_PKTS},
    {"dip-bytes",       REQUIRED_ARG, 0, DIP_BYTES},
    {"nhip-flows",      REQUIRED_ARG, 0, NHIP_FLOWS},
    {"nhip-packets",    REQUIRED_ARG, 0, NHIP_PKTS},
    {"nhip-bytes",      REQUIRED_ARG, 0, NHIP_BYTES},
    {"sport-flows",     REQUIRED_ARG, 0, SPORT_FLOWS},
    {"sport-packets",   REQUIRED_ARG, 0, SPORT_PKTS},
    {"sport-bytes",     REQUIRED_ARG, 0, SPORT_BYTES},
    {"dport-flows",     REQUIRED_ARG, 0, DPORT_FLOWS},
    {"dport-packets",   REQUIRED_ARG, 0, DPORT_PKTS},
    {"dport-bytes",     REQUIRED_ARG, 0, DPORT_BYTES},
    {"proto-flows",     REQUIRED_ARG, 0, PROTO_FLOWS},
    {"proto-packets",   REQUIRED_ARG, 0, PROTO_PKTS},
    {"proto-bytes",     REQUIRED_ARG, 0, PROTO_BYTES},
    {"sensor-flows",    REQUIRED_ARG, 0, SID_FLOWS},
    {"sensor-packets",  REQUIRED_ARG, 0, SID_PKTS},
    {"sensor-bytes",    REQUIRED_ARG, 0, SID_BYTES},
    {"input-flows",     REQUIRED_ARG, 0, INPUT_FLOWS},
    {"input-packets",   REQUIRED_ARG, 0, INPUT_PKTS},
    {"input-bytes",     REQUIRED_ARG, 0, INPUT_BYTES},
    {"output-flows",    REQUIRED_ARG, 0, OUTPUT_FLOWS},
    {"output-packets",  REQUIRED_ARG, 0, OUTPUT_PKTS},
    {"output-bytes",    REQUIRED_ARG, 0, OUTPUT_BYTES},
    {0,0,0,0}           /* sentinel entry */
};


static const char *appHelp[] = {
    "Write bag of flow counts by unique source IP",
    "Write bag of packet counts by unique source IP",
    "Write bag of byte counts by unique source IP",
    "Write bag of flow counts by unique destination IP",
    "Write bag of packet counts by unique destination IP",
    "Write bag of byte counts by unique destination IP",
    "Write bag of flow counts by unique next hop IP",
    "Write bag of packet counts by unique next hop IP",
    "Write bag of byte counts by unique next hop IP",
    "Write bag of flow counts by unique source port",
    "Write bag of packet counts by unique source port",
    "Write bag of byte counts by unique source port",
    "Write bag of flow counts by unique destination port",
    "Write bag of packet counts by unique destination port",
    "Write bag of byte counts by unique destination port",
    "Write bag of flow counts by unique protocol",
    "Write bag of packet counts by unique protocol",
    "Write bag of byte counts by unique protocol",
    "Write bag of flow counts by unique sensor ID",
    "Write bag of packet counts by unique sensor ID",
    "Write bag of byte counts by unique sensor ID",
    "Write bag of flow counts by unique input interface",
    "Write bag of packet counts by unique input interface",
    "Write bag of byte counts by unique input interface",
    "Write bag of flow counts by unique output interface",
    "Write bag of packet counts by unique output interface",
    "Write bag of byte counts by unique output interface",
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int
prepareBagFile(
    const char         *path,
    int                 opt_index);


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
#define USAGE_MSG                                                           \
    ("<BAG-CREATION-SWITCHES> [SWITCHES] [FILES]\n"                         \
     "\tRead SiLK Flow records and builds binary Bag(s) containing\n"       \
     "\tkey-count pairs.  Key is one of source or destination address or\n" \
     "\tport, protocol, sensor, input or output interface, or next hop IP.\n" \
     "\tCounter is sum of flows, packets, or bytes.  Reads SiLK Flows\n"    \
     "\tfrom named files or from the standard input.\n")

    FILE *fh = USAGE_FH;
    int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);

    /* we want to print options that are NOT part of bag creation */
    for (i = 1+FINAL_BAG_KEY; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. %s\n", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]), appHelp[i]);
    }

    skOptionsNotesUsage(fh);
    skOptionsCtxOptionsUsage(optctx, fh);
    skIPv6PolicyUsage(fh);
    sksiteCompmethodOptionsUsage(fh);
    sksiteOptionsUsage(fh);

    fprintf(fh, "\nBAG CREATION SWITCHES:\n");
    for (i = 0; i <= FINAL_BAG_KEY && appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. %s\n", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]), appHelp[i]);
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
    int i;
    int rv;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* close all bag files */
    for (i = 0; i < num_outputs; ++i) {
        skBagDestroy(&bag_io[i].bag);
        if (bag_io[i].stream) {
            rv = skStreamClose(bag_io[i].stream);
            if (rv) {
                skStreamPrintLastErr(bag_io[i].stream, rv, &skAppPrintErr);
            }
            skStreamDestroy(&bag_io[i].stream);
        }
        memset(&bag_io[i], 0, sizeof(bagfile_t));
    }

    /* close the copy stream */
    skOptionsCtxCopyStreamClose(optctx, &skAppPrintErr);

    skOptionsCtxDestroy(&optctx);
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
    int optctx_flags;
    int i;
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize variables */
    memset(bag_io, 0, sizeof(bag_io));

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS | SK_OPTIONS_CTX_PRINT_FILENAMES
                    | SK_OPTIONS_CTX_COPY_INPUT);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsNotesRegister(NULL)
        || sksiteCompmethodOptionsRegister(&comp_method)
        || sksiteOptionsRegister(SK_SITE_FLAG_CONFIG_FILE)
        || skIPv6PolicyOptionsRegister(&ipv6_policy))
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
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* verify that the user requested output */
    if (num_outputs == 0) {
        skAppPrintErr("Must specify type of output(s) to generate.");
        skAppUsage();
    }

    /* make certain stdout is not being used for multiple outputs */
    if (stdout_used && skOptionsCtxCopyStreamIsStdout(optctx)) {
        skAppPrintErr("May not use stdout for multiple output streams");
        exit(EXIT_FAILURE);
    }

    /* For each output file, set the compression method, add the
     * notes (if given), and open the file */
    for (i = 0; i < num_outputs; ++i) {
        if (bag_io[i].stream) {
            rv = skStreamSetCompressionMethod(bag_io[i].stream, comp_method);
            if (rv) {
                skStreamPrintLastErr(bag_io[i].stream, rv, &skAppPrintErr);
                exit(EXIT_FAILURE);
            }
            rv = skOptionsNotesAddToStream(bag_io[i].stream);
            if (rv) {
                skStreamPrintLastErr(bag_io[i].stream, rv, &skAppPrintErr);
                exit(EXIT_FAILURE);
            }
            rv = skStreamOpen(bag_io[i].stream);
            if (rv) {
                skStreamPrintLastErr(bag_io[i].stream, rv, &skAppPrintErr);
                exit(EXIT_FAILURE);
            }
        }
    }

    /* No longer need the notes. */
    skOptionsNotesTeardown();

    /* open the --copy-input stream */
    if (skOptionsCtxOpenStreams(optctx, &skAppPrintErr)) {
        exit(EXIT_FAILURE);
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
    char               *opt_arg)
{
    switch ((appOptionsEnum)opt_index) {
      case SIP_FLOWS:    case SIP_PKTS:    case SIP_BYTES:
      case DIP_FLOWS:    case DIP_PKTS:    case DIP_BYTES:
      case NHIP_FLOWS:   case NHIP_PKTS:   case NHIP_BYTES:
      case SPORT_FLOWS:  case SPORT_PKTS:  case SPORT_BYTES:
      case DPORT_FLOWS:  case DPORT_PKTS:  case DPORT_BYTES:
      case PROTO_FLOWS:  case PROTO_PKTS:  case PROTO_BYTES:
      case SID_FLOWS:    case SID_PKTS:    case SID_BYTES:
      case INPUT_FLOWS:  case INPUT_PKTS:  case INPUT_BYTES:
      case OUTPUT_FLOWS: case OUTPUT_PKTS: case OUTPUT_BYTES:
        if (prepareBagFile(opt_arg, opt_index)) {
            return 1;
        }
        break;
    }

    return 0;                   /* OK */
}


/*
 *  ok = prepareBagFile(pathname, opt_index);
 *
 *    Prepare the global 'bag_io' to write a bag to 'pathname'.
 *
 *    This function creates an skstream_t to 'pathname' and allocates
 *    a bag that will be writing to that file.  The function makes
 *    sure a file with that name does not currently exist.  If
 *    pathname is "stdout" and no other bag files are writing to
 *    stdout, then stdout will be used.
 *
 *    Returns 0 on success.  Returns non-zero if allocation files, if
 *    we attempt to open an existing file, or if more than one bag use
 *    stdout.
 */
static int
prepareBagFile(
    const char         *pathname,
    int                 opt_index)
{
    bagfile_t *bagf;
    int rv;
    int i;

    assert(num_outputs < MAX_NUM_OUTPUTS);

    if (!pathname || !pathname[0]) {
        skAppPrintErr("Invalid %s: Missing file name",
                      appOptions[opt_index].name);
        return 1;
    }
    for (i = 0; i < num_outputs; ++i) {
        if (bag_io[i].key_value == opt_index) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
    }
    if (0 == strcmp("stdout", pathname) || 0 == strcmp("-", pathname)) {
        if (stdout_used) {
            skAppPrintErr("Invalid %s: Only one output may use stdout",
                          appOptions[opt_index].name);
            return 1;
        }
        stdout_used = 1;
    }

    /* determine where in bag_io[] to put this bag */
    if (opt_index < FIRST_NON_IP_BAG) {
        /* this is an IP bag, it goes at the end */
        bagf = &bag_io[num_outputs];
        ++num_outputs;
    } else if (num_non_ip_outputs == num_outputs) {
        /* no IP bags, so this bag can go at the end */
        bagf = &bag_io[num_outputs];
        ++num_outputs;
        ++num_non_ip_outputs;
    } else {
        /* move the first IP bag to the end of the array, and put this
         * non-IP bag before the IP bags */
        bagf = &bag_io[num_non_ip_outputs];
        memcpy(&bag_io[num_outputs], bagf, sizeof(bagfile_t));
        memset(bagf, 0, sizeof(bagfile_t));
        ++num_outputs;
        ++num_non_ip_outputs;
    }

    bagf->key_value = opt_index;
    if (SKBAG_OK != skBagCreateTyped(&bagf->bag, bag_types[opt_index].key,
                                     bag_types[opt_index].counter, 0, 0))
    {
        skAppPrintErr("Error allocating Bag for %s",
                      appOptions[opt_index].name);
        return 1;
    }

    if ((rv = skStreamCreate(&(bagf->stream), SK_IO_WRITE, SK_CONTENT_SILK))
        || (rv = skStreamBind(bagf->stream, pathname)))
    {
        skStreamPrintLastErr(bagf->stream, rv, &skAppPrintErr);
        skStreamDestroy(&bagf->stream);
        return 1;
    }

    return 0;                   /* OK */
}


/*
 *  ok = processFile(rwIOS);
 *
 *    Read the SiLK Flow records from the 'rwIOS' stream---and
 *    potentially create bag files for {sIP,dIP,sPort,dPort,proto} x
 *    {flows,pkts,bytes}.
 *
 *    Return 0 if successful; non-zero otherwise.
 */
static int
processFile(
    skstream_t         *rwIOS)
{
    skBagTypedKey_t key;
    skBagTypedKey_t ipkey;
    skBagTypedCounter_t counter;
    skBagErr_t err;
    rwRec rwrec;
    int i;
    int rv;

    /* set the types for the key and counter once */
    key.type = SKBAG_KEY_U32;
    ipkey.type = SKBAG_KEY_IPADDR;
    counter.type = SKBAG_COUNTER_U64;

    while ((rv = skStreamReadRecord(rwIOS, &rwrec)) == SKSTREAM_OK) {
        /* handle non-IP-bags (if any) */
        for (i = 0; i < num_non_ip_outputs; ++i) {
            switch (bag_io[i].key_value) {
              case SPORT_FLOWS:
                key.val.u32 = rwRecGetSPort(&rwrec);
                counter.val.u64 = 1;
                break;
              case DPORT_FLOWS:
                key.val.u32 = rwRecGetDPort(&rwrec);
                counter.val.u64 = 1;
                break;
              case PROTO_FLOWS:
                key.val.u32 = rwRecGetProto(&rwrec);
                counter.val.u64 = 1;
                break;
              case SID_FLOWS:
                key.val.u32 = rwRecGetSensor(&rwrec);
                counter.val.u64 = 1;
                break;
              case INPUT_FLOWS:
                key.val.u32 = rwRecGetInput(&rwrec);
                counter.val.u64 = 1;
                break;
              case OUTPUT_FLOWS:
                key.val.u32 = rwRecGetOutput(&rwrec);
                counter.val.u64 = 1;
                break;

              case SPORT_PKTS:
                key.val.u32 = rwRecGetSPort(&rwrec);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;
              case DPORT_PKTS:
                key.val.u32 = rwRecGetDPort(&rwrec);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;
              case PROTO_PKTS:
                key.val.u32 = rwRecGetProto(&rwrec);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;
              case SID_PKTS:
                key.val.u32 = rwRecGetSensor(&rwrec);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;
              case INPUT_PKTS:
                key.val.u32 = rwRecGetInput(&rwrec);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;
              case OUTPUT_PKTS:
                key.val.u32 = rwRecGetOutput(&rwrec);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;

              case SPORT_BYTES:
                key.val.u32 = rwRecGetSPort(&rwrec);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;
              case DPORT_BYTES:
                key.val.u32 = rwRecGetDPort(&rwrec);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;
              case PROTO_BYTES:
                key.val.u32 = rwRecGetProto(&rwrec);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;
              case SID_BYTES:
                key.val.u32 = rwRecGetSensor(&rwrec);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;
              case INPUT_BYTES:
                key.val.u32 = rwRecGetInput(&rwrec);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;
              case OUTPUT_BYTES:
                key.val.u32 = rwRecGetOutput(&rwrec);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;

              default:
                skAbortBadCase(bag_io[i].key_value);
            }

            err = skBagCounterAdd(bag_io[i].bag, &key, &counter, NULL);
            if (err) {
                if (err == SKBAG_ERR_OP_BOUNDS) {
                    counter.val.u64 = SKBAG_COUNTER_MAX;
                    skBagCounterSet(bag_io[i].bag, &ipkey, &counter);
                    if (!bag_io[i].overflow) {
                        bag_io[i].overflow = 1;
                        skAppPrintErr("**WARNING** Overflow for %s bag %s",
                                      appOptions[bag_io[i].key_value].name,
                                      skStreamGetPathname(bag_io[i].stream));
                    }
                } else if (err == SKBAG_ERR_MEMORY) {
                    skAppPrintErr(("Out of memory for %s bag %s\n"
                                   "\tCleaning up and exiting"),
                                  appOptions[bag_io[i].key_value].name,
                                  skStreamGetPathname(bag_io[i].stream));
                    rv = -1;
                    goto END;
                } else {
                    skAppPrintErr("Error setting value for %s bag %s: %s",
                                  appOptions[bag_io[i].key_value].name,
                                  skStreamGetPathname(bag_io[i].stream),
                                  skBagStrerror(err));
                    rv = -1;
                    goto END;
                }
            }
        }

        /* handle IP-bags.  do not reset 'i' */
        for ( ; i < num_outputs; ++i) {
            switch (bag_io[i].key_value) {
              case SIP_FLOWS:
                rwRecMemGetSIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = 1;
                break;
              case DIP_FLOWS:
                rwRecMemGetDIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = 1;
                break;
              case NHIP_FLOWS:
                rwRecMemGetNhIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = 1;
                break;

              case SIP_PKTS:
                rwRecMemGetSIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;
              case DIP_PKTS:
                rwRecMemGetDIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;
              case NHIP_PKTS:
                rwRecMemGetNhIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = rwRecGetPkts(&rwrec);
                break;

              case SIP_BYTES:
                rwRecMemGetSIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;
              case DIP_BYTES:
                rwRecMemGetDIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;
              case NHIP_BYTES:
                rwRecMemGetNhIP(&rwrec, &ipkey.val.addr);
                counter.val.u64 = rwRecGetBytes(&rwrec);
                break;

              default:
                skAbortBadCase(bag_io[i].key_value);
            }

            err = skBagCounterAdd(bag_io[i].bag, &ipkey, &counter, NULL);
            if (err) {
                if (err == SKBAG_ERR_OP_BOUNDS) {
                    counter.val.u64 = SKBAG_COUNTER_MAX;
                    skBagCounterSet(bag_io[i].bag, &ipkey, &counter);
                    if (!bag_io[i].overflow) {
                        bag_io[i].overflow = 1;
                        skAppPrintErr("**WARNING** Overflow for %s bag %s",
                                      appOptions[bag_io[i].key_value].name,
                                      skStreamGetPathname(bag_io[i].stream));
                    }
                } else if (err == SKBAG_ERR_MEMORY) {
                    skAppPrintErr(("Out of memory for %s bag %s\n"
                                   "\tCleaning up and exiting"),
                                  appOptions[bag_io[i].key_value].name,
                                  skStreamGetPathname(bag_io[i].stream));
                    rv = -1;
                    goto END;
                } else {
                    skAppPrintErr("Error setting value for %s bag %s: %s",
                                  appOptions[bag_io[i].key_value].name,
                                  skStreamGetPathname(bag_io[i].stream),
                                  skBagStrerror(err));
                    rv = -1;
                    goto END;
                }
            }
        }
    }
    if (rv == SKSTREAM_ERR_EOF) {
        /* Successful if we make it here */
        rv = 0;
    } else {
        skStreamPrintLastErr(rwIOS, rv, &skAppPrintErr);
        rv = -1;
    }

  END:
    return rv;
}


int main(int argc, char **argv)
{
    skstream_t *rwios;
    char errbuf[2 * PATH_MAX];
    int had_err = 0;
    skBagErr_t err;
    ssize_t rv;
    int i;

    appSetup(argc, argv);                       /* never returns on error */

    /* process input */
    while ((rv = skOptionsCtxNextSilkFile(optctx, &rwios, &skAppPrintErr))
           == 0)
    {
        skStreamSetIPv6Policy(rwios, ipv6_policy);
        if (0 != processFile(rwios)) {
            skAppPrintErr("Error processing input from %s",
                          skStreamGetPathname(rwios));
            skStreamDestroy(&rwios);
            return EXIT_FAILURE;
        }
        skStreamDestroy(&rwios);
    }
    if (rv < 0) {
        exit(EXIT_FAILURE);
    }

    /* write the bags */
    for (i = 0; i < num_outputs; ++i) {
        if (bag_io[i].bag) {
            err = skBagWrite(bag_io[i].bag, bag_io[i].stream);
            if (SKBAG_OK == err) {
                rv = skStreamClose(bag_io[i].stream);
                if (rv) {
                    had_err = 1;
                    skStreamLastErrMessage(bag_io[i].stream, rv,
                                           errbuf, sizeof(errbuf));
                    skAppPrintErr("Error writing %s bag: %s",
                                  appOptions[bag_io[i].key_value].name,errbuf);
                }
            } else if (SKBAG_ERR_OUTPUT == err) {
                had_err = 1;
                rv = skStreamGetLastReturnValue(bag_io[i].stream);
                skStreamLastErrMessage(bag_io[i].stream, rv,
                                       errbuf, sizeof(errbuf));
                skAppPrintErr("Error writing %s bag: %s",
                              appOptions[bag_io[i].key_value].name, errbuf);
            } else {
                had_err = 1;
                skAppPrintErr("Error writing %s bag to '%s': %s",
                              appOptions[bag_io[i].key_value].name,
                              skStreamGetPathname(bag_io[i].stream),
                              skBagStrerror(err));
            }
            skStreamDestroy(&bag_io[i].stream);
        }
    }

    /* done */
    return ((had_err) ? EXIT_FAILURE : EXIT_SUCCESS);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
