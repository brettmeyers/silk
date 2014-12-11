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
 *
 *  rwsettool.c
 *
 *    Manipulate IPset files to produce a new IPset.  Supports
 *    operations such as union, intersection, difference, and
 *    sampling.
 */

#include <silk/silk.h>

RCSIDENT("$SiLK: rwsettool.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skipaddr.h>
#include <silk/skipset.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write output from --help */
#define USAGE_FH stdout

/* structure used when walking over the elements in an IPset to
 * generate a sampled IPset */
typedef struct sample_state_st {
    uint64_t   sample_remain;
    uint64_t   set_remain;
    skipset_t *ipset;
    long       frac;
} sample_state_t;

/*
 * How to handle command line history in the output file.  If <0, do
 * not write any invocation to the output file.  If 0, record this
 * invocation, but do not copy the invocation from the input file(s).
 * If >0, copy the command line histories from the input file and
 * record this invocation.  WARNING: A value >0 can quickly cause the
 * output files to become HUGE.
 */
#ifndef RWSETTOOL_INVOCATION_HISTORY
#define RWSETTOOL_INVOCATION_HISTORY 0
#endif


/* LOCAL VARIABLES */

/* index of first option that is not handled by the options handler.
 * If this is equal to argc, then input is from stdin. */
static int arg_index = 0;

/* where to write the resulting set */
static skstream_t *out_stream = NULL;

/* the appOptionsEnum value for the operation being executed. */
static int operation = -1;

/* the appOptionsEnum value indicating whether --sample should select
 * based on a target sample size, or a ratio of the set size */
static int sample_type = 0;

/* the random sample size from the --size switch */
static uint64_t sample_size;

/* the random sample ratio from the --ratio switch */
static double sample_ratio;

/* the seed for the pseudo-random number generator */
static uint32_t sample_seed;

/* group IPs into CIDR blocks of this size */
static uint32_t mask = 0;

/* when masking, whether the CIDR blocks should be full */
static int fill_blocks = 0;

/* options for writing the IPset */
static skipset_options_t set_options;


/* OPTIONS SETUP */

typedef enum {
    OPT_UNION,
    OPT_INTERSECT,
    OPT_DIFFERENCE,
    OPT_MASK,
    OPT_FILL_BLOCKS,
    OPT_SAMPLE,
    OPT_SAMPLE_SIZE,
    OPT_SAMPLE_RATIO,
    OPT_SAMPLE_SEED,
    OPT_OUTPUT_PATH
} appOptionsEnum;

static struct option appOptions[] = {
    {"union",           NO_ARG,       0, OPT_UNION},
    {"intersect",       NO_ARG,       0, OPT_INTERSECT},
    {"difference",      NO_ARG,       0, OPT_DIFFERENCE},
    {"mask",            REQUIRED_ARG, 0, OPT_MASK},
    {"fill-blocks",     REQUIRED_ARG, 0, OPT_FILL_BLOCKS},
    {"sample",          NO_ARG,       0, OPT_SAMPLE},
    {"size",            REQUIRED_ARG, 0, OPT_SAMPLE_SIZE},
    {"ratio",           REQUIRED_ARG, 0, OPT_SAMPLE_RATIO},
    {"seed",            REQUIRED_ARG, 0, OPT_SAMPLE_SEED},
    {"output-path",     REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {0, 0, 0, 0}        /* sentinel entry */
};

static const char *appHelp[] = {
    ("Create an IPset containing the IPs that exist in ANY of\n"
     "\tthe input IPsets"),
    ("Create an IPset containing the IPs that exist in ALL of\n"
     "\tthe input IPsets"),
    ("Create an IPset containing the IPs from the first IPset\n"
     "\tthat do not exist any subsequent IPset"),
    ("Create an IPset containing a single IP in each block of the\n"
     "\tspecified bitmask length when ANY of the input IPsets have an\n"
     "\tIP in that block"),
    ("Create an IPset containing a completely full block of\n"
     "\tthe specified bitmask length when ANY of the input IPsets have\n"
     "\tan IP in that block"),
    ("Create an IPset containing a random sample of IPs from\n"
     "\tall input IPsets.  Requires the --size or --ratio switch."),
    ("Specify the sample size (number of IPs sampled from each\n"
     "\tinput IPset) for the --sample operation"),
    ("Specify the probability, as a floating point value between\n"
     "\t0.0 and 1.0, that an individual IP will be sampled"),
    "Specify the random number seed for the --sample operation",
    "Write the resulting IPset to this location. Def. stdout",
    (char *) NULL
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
#define USAGE_MSG                                                          \
    ("<OPERATION> [SWITCHES] IPSET..\n"                                    \
     "\tPerforms the specified OPERATION, one of --union, --intersect,\n"  \
     "\t--difference, --mask, --fill-blocks, or --sample, on the input\n"  \
     "\tIPset file(s) and generates a new IPset file.\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
    skIPSetOptionsUsage(fh);
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

    if (out_stream) {
        skStreamDestroy(&out_stream);
    }

    skIPSetOptionsTeardown();
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
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp) / sizeof(char *)) ==
           (sizeof(appOptions) / sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    memset(&set_options, 0, sizeof(skipset_options_t));
    set_options.existing_silk_files = 1;
#if RWSETTOOL_INVOCATION_HISTORY >= 0
    set_options.argc = argc;
    set_options.argv = argv;
#endif

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skIPSetOptionsRegister(&set_options))
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
        skAppUsage();                            /* never returns */
    }

    /* verify that we have something to do */
    if (operation == -1) {
        skAppPrintErr(("One of --%s, --%s, --%s,"
                       " --%s, --%s, or --%s is required"),
                      appOptions[OPT_UNION].name,
                      appOptions[OPT_INTERSECT].name,
                      appOptions[OPT_DIFFERENCE].name,
                      appOptions[OPT_MASK].name,
                      appOptions[OPT_FILL_BLOCKS].name,
                      appOptions[OPT_SAMPLE].name);
        skAppUsage();
    }

    /* either need name of set file(s) after options or a set file on stdin */
    if ((arg_index == argc) && (FILEIsATty(stdin))) {
        skAppPrintErr("No files on the command line and"
                      " stdin is connected to a terminal");
        skAppUsage();
    }

    /* verify that we have a sample size or ratio and seed the random
     * number generator */
    if (operation == OPT_SAMPLE) {
        if (!sample_type) {
            skAppPrintErr(("The --%s switch requires a valid --%s or --%s "
                           "argument"),
                          appOptions[OPT_SAMPLE].name,
                          appOptions[OPT_SAMPLE_SIZE].name,
                          appOptions[OPT_SAMPLE_RATIO].name);
            skAppUsage();
        }
        if (!sample_seed) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            sample_seed = (uint32_t)tv.tv_sec + (uint32_t)tv.tv_usec;
        }
        srandom((unsigned long)sample_seed);
    }

    /* bind the output stream to the default location */
    if (out_stream == NULL) {
        if ((rv = skStreamCreate(&out_stream, SK_IO_WRITE, SK_CONTENT_SILK))
            || (rv = skStreamBind(out_stream, "stdout")))
        {
            skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
            skStreamDestroy(&out_stream);
            exit(EXIT_FAILURE);
        }
    }

    /* open the output stream */
    rv = skStreamOpen(out_stream);
    if (rv) {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        skStreamDestroy(&out_stream);
        exit(EXIT_FAILURE);
    }

    return;                                      /* OK */
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

    switch ((appOptionsEnum) opt_index) {
      case OPT_INTERSECT:
      case OPT_DIFFERENCE:
      case OPT_SAMPLE:
        if ((operation >= 0) && (operation != opt_index)) {
            skAppPrintErr("Switches --%s and --%s are incompatible",
                          appOptions[operation].name,
                          appOptions[opt_index].name);
            return 1;
        }
        operation = opt_index;
        break;

      case OPT_UNION:
        if ((operation >= 0) && (operation != opt_index)
            && (operation != OPT_MASK) && (operation != OPT_FILL_BLOCKS))
        {
            skAppPrintErr("Switches --%s and --%s are incompatible",
                          appOptions[operation].name,
                          appOptions[opt_index].name);
            return 1;
        }
        operation = opt_index;
        break;

      case OPT_FILL_BLOCKS:
        fill_blocks = 1;
        /* FALLTHROUGH */
      case OPT_MASK:
        if ((operation >= 0) && (operation != opt_index)
            && (operation != OPT_UNION))
        {
            skAppPrintErr("Switches --%s and --%s are incompatible",
                          appOptions[operation].name,
                          appOptions[opt_index].name);
            return 1;
        }
        operation = opt_index;
#if SK_ENABLE_IPV6
        rv = skStringParseUint32(&mask, opt_arg, 1, 128);
#else
        rv = skStringParseUint32(&mask, opt_arg, 1, 32);
#endif
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_SAMPLE_SIZE:
        if (sample_type == opt_index) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if (sample_type) {
            skAppPrintErr("Switches --%s and --%s are incompatible",
                          appOptions[sample_type].name,
                          appOptions[opt_index].name);
            return 1;
        }
        sample_type = opt_index;
        rv = skStringParseUint64(&sample_size, opt_arg, 1, 0);
        if (rv) {
            goto PARSE_ERROR;
        }

        break;

      case OPT_SAMPLE_RATIO:
        if (sample_type == opt_index) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if (sample_type) {
            skAppPrintErr("Switches %s and %s are incompatible",
                          appOptions[sample_type].name,
                          appOptions[opt_index].name);
            return 1;
        }
        sample_type = opt_index;
        rv = skStringParseDouble(&sample_ratio, opt_arg, 0.0, 1.0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_SAMPLE_SEED:
        rv = skStringParseUint32(&sample_seed, opt_arg, 1, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_OUTPUT_PATH:
        if (out_stream) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if ((rv = skStreamCreate(&out_stream, SK_IO_WRITE, SK_CONTENT_SILK))
            || (rv = skStreamBind(out_stream, opt_arg)))
        {
            skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
            skStreamDestroy(&out_stream);
            return 1;
        }
        break;
    }

    return 0;                   /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  appOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;
}


/*
 *  ok = appNextInput(argc, argv, &stream);
 *
 *    Return the name of the next input file from the command line or
 *    the standard input if no files were given on the command line.
 */
static int
appNextInput(
    int                 argc,
    char              **argv,
    skstream_t        **stream)
{
    static int initialized = 0;
    const char *fname = NULL;
    sk_file_header_t *hdr = NULL;
    int rv;

    if (arg_index < argc) {
        /* get current file and prepare to get next */
        fname = argv[arg_index];
        ++arg_index;
    } else {
        if (initialized) {
            /* no more input */
            return 0;
        }
        /* input is from stdin */
        fname = "stdin";
    }

    initialized = 1;

    /* open the input stream */
    if ((rv = skStreamCreate(stream, SK_IO_READ, SK_CONTENT_SILK))
        || (rv = skStreamBind((*stream), fname))
        || (rv = skStreamOpen((*stream)))
        || (rv = skStreamReadSilkHeader((*stream), &hdr)))
    {
        skStreamPrintLastErr((*stream), rv, &skAppPrintErr);
        skStreamDestroy(stream);
        return -1;
    }

    /* copy invocation and notes (annotations) from the input files to
     * the output file; these headers will not be written to the
     * output if --invocation-strip or --notes-strip was specified. */
    rv = skHeaderCopyEntries(skStreamGetSilkHeader(out_stream), hdr,
                             SK_HENTRY_ANNOTATION_ID);
#if RWSETTOOL_INVOCATION_HISTORY > 0
    if (0 == rv) {
        rv = skHeaderCopyEntries(skStreamGetSilkHeader(out_stream), hdr,
                                 SK_HENTRY_INVOCATION_ID);
    }
#endif
    if (rv) {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        skStreamDestroy(&(*stream));
        return -1;
    }

    return 1;
}


/*
 *  status = sampleRatioCallback(ipaddr, prefix, &state);
 *
 *    Callback used by sampleSets() when the --ratio option was
 *    specified.
 *
 *    Visit each IP in the CIDR block and determine whether it should
 *    be added to the output IPset contained in 'state'.
 */
static int
sampleRatioCallback(
    skipaddr_t         *ipaddr,
    uint32_t            prefix,
    void               *v_state)
{
    sample_state_t *state = (sample_state_t*)v_state;
    skipaddr_t end_ip;
    int rv = 0;

    skCIDR2IPRange(ipaddr, prefix, ipaddr, &end_ip);

    for (;;) {
        if (random() < state->frac) {
            rv = skIPSetInsertAddress(state->ipset, ipaddr, 0);
            if (rv) {
                break;
            }
        }

        if (skipaddrCompare(ipaddr, &end_ip) == 0) {
            return 0;
        }

        skipaddrIncrement(ipaddr);
    }

    skAppPrintErr("Error inserting into IPset: %s",
                  skIPSetStrerror(rv));
    return -1;
}


/*
 *  status = sampleSizeCallback(ipaddr, prefix, &state);
 *
 *    Callback used by sampleSets() when the --size option was
 *    specified.
 *
 *    Visit each IP in the CIDR block and determine whether it should
 *    be added to the output IPset contained in 'state'.
 */
static int
sampleSizeCallback(
    skipaddr_t         *ipaddr,
    uint32_t            prefix,
    void               *v_state)
{
    sample_state_t *state = (sample_state_t*)v_state;
    skipaddr_t end_ip;
    double frac;
    int rv;

    if (state->set_remain == state->sample_remain) {
        /* must add this entire block to the result set */
        rv = skIPSetInsertAddress(state->ipset, ipaddr, prefix);
        if (rv) {
            goto ERROR;
        }
        return 0;
    }

    /* process the remaining IPs in this CIDR block */
    skCIDR2IPRange(ipaddr, prefix, ipaddr, &end_ip);

    while (state->set_remain > state->sample_remain) {
        /* chance of selecting an IP is number of IPs we still require
         * divided by number of IPs that are still available */
        frac = ((double)RAND_MAX * state->sample_remain / state->set_remain);
        --state->set_remain;

        if ((double)random() < frac) {
            rv = skIPSetInsertAddress(state->ipset, ipaddr, 0);
            if (rv) {
                goto ERROR;
            }
            --state->sample_remain;
            if (0 == state->sample_remain) {
                /* we've met the quota and can stop */
                return 1;
            }
        }

        if (skipaddrCompare(ipaddr, &end_ip) == 0) {
            /* no more IPs in this CIDR block */
            return 0;
        }

        skipaddrIncrement(ipaddr);
    }

    /* must add all remaining IPs to the result set */
    while ((rv = skIPSetInsertAddress(state->ipset, ipaddr, 0))
           == 0)
    {
        if (skipaddrCompare(ipaddr, &end_ip) == 0) {
            return 0;
        }
        skipaddrIncrement(ipaddr);
    }

  ERROR:
    skAppPrintErr("Error inserting into IPset: %s",
                  skIPSetStrerror(rv));
    return -1;
}


/*
 *  out_set = sampleSets(argc, argv);
 *
 *    Create a new IPset.  For each IPset specified on the command
 *    line---given by 'argc' and 'argv'---walk over the CIDR blocks in
 *    the set and add a sample of the IPs to the new IPset.  Return
 *    the new IPset, or return NULL on error.
 */
static skipset_t *
sampleSets(
    int                 argc,
    char              **argv)
{
    skipset_t *out_set;
    sample_state_t state;
    skipset_t *in_set = NULL;
    skstream_t *in_stream = NULL;
    sk_ipv6policy_t policy;
    int have_input;
    int rv;

    /* create the output set */
    rv = skIPSetCreate(&out_set, 0);
    if (rv) {
        skAppPrintErr("Cannot create IPset: %s", skIPSetStrerror(rv));
        return NULL;
    }
    skIPSetOptionsBind(out_set, &set_options);

    /* initialize the state structure */
    memset(&state, 0, sizeof(state));
    state.ipset = out_set;

    /* set the policy to be the type of IPs that out_set contains */
    policy = SK_IPV6POLICY_ASV4;

    if (OPT_SAMPLE_RATIO == sample_type) {
        /* the probability of choosing an IP is fixed. */
        state.frac = (long)((double)RAND_MAX * sample_ratio);
    }

    while (1 == (have_input = appNextInput(argc, argv, &in_stream))) {
        /* Read IPset */
        rv = skIPSetRead(&in_set, in_stream);
        if (rv) {
            if (SKIPSET_ERR_FILEIO == rv) {
                skStreamPrintLastErr(in_stream,
                                     skStreamGetLastReturnValue(in_stream),
                                     &skAppPrintErr);
            } else {
                skAppPrintErr("Unable to read IPset from '%s': %s",
                              skStreamGetPathname(in_stream),
                              skIPSetStrerror(rv));
            }
            goto ERROR;
        }
        /* no longer need the input stream */
        skStreamDestroy(&in_stream);

        /* convert output set to IPv6 if required */
        if (skIPSetContainsV6(in_set) && !skIPSetIsV6(out_set)) {
            policy = SK_IPV6POLICY_FORCE;
            rv = skIPSetConvert(out_set, 6);
            if (rv) {
                goto ERROR;
            }
        }

        /* TODO: Try list-based sampling algorithm  */

        switch (sample_type) {
          case OPT_SAMPLE_RATIO:
            rv = skIPSetWalk(
                in_set, 1, policy, &sampleRatioCallback, (void*)&state);
            break;

          case OPT_SAMPLE_SIZE:
            /* update state counts for this IPset */
            state.set_remain = skIPSetCountIPs(in_set, NULL);
            if (state.set_remain <= sample_size) {
                rv = skIPSetUnion(out_set, in_set);
            } else {
                state.sample_remain = sample_size;
                rv = skIPSetWalk(
                    in_set, 1, policy, &sampleSizeCallback, (void*)&state);
            }
            break;

          default:
            skAbortBadCase(sample_type);
        }
        if (rv < 0) {
            goto ERROR;
        }

        skIPSetDestroy(&in_set);
    }
    if (0 != have_input) {
        goto ERROR;
    }

    skIPSetClean(out_set);

    return out_set;

  ERROR:
    skIPSetDestroy(&in_set);
    skStreamDestroy(&in_stream);
    skIPSetDestroy(&out_set);
    return NULL;
}


int main(int argc, char **argv)
{
    skstream_t *in_stream;
    skipset_t *out_set = NULL;
    skipset_t *in_set = NULL;
    int have_input;
    int rv;

    appSetup(argc, argv);       /* never returns on error */

    if (OPT_SAMPLE == operation) {
        out_set = sampleSets(argc, argv);
        if (NULL == out_set) {
            return EXIT_FAILURE;
        }
    } else {
        /* load the first set; it will become the basis for the output set */
        have_input = appNextInput(argc, argv, &in_stream);
        if (have_input != 1) {
            return EXIT_FAILURE;
        }
        rv = skIPSetRead(&out_set, in_stream);
        if (rv) {
            if (rv == SKIPSET_ERR_FILEIO) {
                skStreamPrintLastErr(in_stream,
                                     skStreamGetLastReturnValue(in_stream),
                                     &skAppPrintErr);
            } else {
                skAppPrintErr("Unable to read IPset from '%s': %s",
                              skStreamGetPathname(in_stream),
                              skIPSetStrerror(rv));
            }
            skStreamDestroy(&in_stream);
            skIPSetDestroy(&out_set);
            return EXIT_FAILURE;
        }
        skStreamDestroy(&in_stream);
        skIPSetOptionsBind(out_set, &set_options);

        /* read remaining sets */
        while (1 == (have_input = appNextInput(argc, argv, &in_stream))) {
            rv = skIPSetRead(&in_set, in_stream);
            if (rv) {
                if (rv == SKIPSET_ERR_FILEIO) {
                    skStreamPrintLastErr(in_stream,
                                         skStreamGetLastReturnValue(in_stream),
                                         &skAppPrintErr);
                } else {
                    skAppPrintErr("Unable to read IPset from '%s': %s",
                                  skStreamGetPathname(in_stream),
                                  skIPSetStrerror(rv));
                }
                skStreamDestroy(&in_stream);
                return EXIT_FAILURE;
            }
            skStreamDestroy(&in_stream);

            switch (operation) {
              case OPT_UNION:
              case OPT_MASK:
              case OPT_FILL_BLOCKS:
                /* convert output set to IPv6 if required */
                if (skIPSetContainsV6(in_set) && !skIPSetIsV6(out_set)) {
                    rv = skIPSetConvert(out_set, 6);
                    if (rv) {
                        break;
                    }
                }
                rv = skIPSetUnion(out_set, in_set);
                break;

              case OPT_INTERSECT:
                rv = skIPSetIntersect(out_set, in_set);
                break;

              case OPT_DIFFERENCE:
                skIPSetSubtract(out_set, in_set);
                break;

              default:
                skAbortBadCase(operation);
            }

            skIPSetDestroy(&in_set);
            in_set = NULL;

            if (rv) {
                skAppPrintErr("Error in %s operation: %s",
                              appOptions[operation].name, skIPSetStrerror(rv));
                return EXIT_FAILURE;
            }
        }
        if (0 != have_input) {
            return EXIT_FAILURE;
        }
    }

    /* mask the IPs in the resulting set */
    if (mask != 0) {
        rv = 0;
        if (skIPSetIsV6(out_set)) {
            if (mask < 128) {
                if (fill_blocks) {
                    rv = skIPSetMaskAndFill(out_set, mask);
                } else {
                    rv = skIPSetMask(out_set, mask);
                }
            }
        } else if (mask > 32) {
            skAppPrintErr("Ignoring mask of %" PRIu32 " for an IPv4 IPset",
                          mask);
        } else if (mask != 32) {
            if (fill_blocks) {
                rv = skIPSetMaskAndFill(out_set, mask);
            } else {
                rv = skIPSetMask(out_set, mask);
            }
        }
        if (rv) {
            skAppPrintErr(("Error applying mask of '%" PRIu32 "' to IPset"),
                          mask);
        }
    }

#if SK_ENABLE_IPV6
    /* convert the set to IPv4 if possible */
    if (skIPSetIsV6(out_set) && !skIPSetContainsV6(out_set)) {
        skIPSetConvert(out_set, 4);
    }
#endif  /* SK_ENABLE_IPV6 */

    skIPSetClean(out_set);

    /* write the set */
    rv = skIPSetWrite(out_set, out_stream);
    if (rv) {
        if (rv == SKIPSET_ERR_FILEIO) {
            skStreamPrintLastErr(out_stream,
                                 skStreamGetLastReturnValue(out_stream),
                                 &skAppPrintErr);
        } else {
            skAppPrintErr("Error writing IPset to '%s': %s",
                          skStreamGetPathname(out_stream),skIPSetStrerror(rv));
        }
        skStreamDestroy(&out_stream);
        skIPSetDestroy(&out_set);
        return EXIT_FAILURE;
    }

    /* close the output stream */
    if ((rv = skStreamClose(out_stream))
        || (rv = skStreamDestroy(&out_stream)))
    {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        skStreamDestroy(&out_stream);
        skIPSetDestroy(&out_set);
        return EXIT_FAILURE;
    }

    skStreamDestroy(&out_stream);
    skIPSetDestroy(&out_set);

    /* done */
    return (0);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
