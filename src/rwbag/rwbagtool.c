/*
** Copyright (C) 2004-2014 by Carnegie Mellon University.
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
**    rwbagtool performs various operations on bags.  It can add bags
**    together, subtract a subset of data from a bag, perform key
**    intersection with an IPset, extract the key list of a bag as an
**    IPset, or filter bag records based on their counter value.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwbagtool.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skbag.h>
#include <silk/skipaddr.h>
#include <silk/skipset.h>
#include <silk/skstream.h>
#include <silk/sksite.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* minimum counter that can be specified to --mincounter */
#define BAGTOOL_MIN_COUNTER  UINT64_C(1)

/* What to do when malloc() fails */
#define EXIT_NO_MEMORY                                               \
    do {                                                             \
        skAppPrintOutOfMemory(NULL);                                 \
        exit(EXIT_FAILURE);                                          \
    } while(0)

#define ERR_READ_BAG(erb_stream, erb_err)                               \
    if (SKBAG_ERR_READ == (erb_err)) {                                  \
        skStreamPrintLastErr((erb_stream),                              \
                             skStreamGetLastReturnValue(erb_stream),    \
                             &skAppPrintErr);                           \
    } else {                                                            \
        skAppPrintErr("Could not read Bag from '%s': %s",               \
                      skStreamGetPathname(erb_stream),                  \
                      skBagStrerror(erb_err));                          \
    }

#define ERR_GET_COUNT(egc_key, egc_err)                                 \
    skAppPrintErr("Error getting count for key (%s): %s",               \
                  skipaddrString(ipbuf1, &(egc_key).val.addr, 0),       \
                  skBagStrerror(egc_err))

#define ERR_SET_COUNT(esc_key, esc_val, esc_err)                        \
    skAppPrintErr(("Error setting key=>counter (%s=>%" PRIu64 ": %s"),  \
                  skipaddrString(ipbuf1, &(esc_key).val.addr, 0),       \
                  (esc_val).val.u64, skBagStrerror(esc_err))

#define ERR_REMOVE_KEY(erk_key, erk_err)                                \
    skAppPrintErr("Error removing key (%s): %s",                        \
                  skipaddrString(ipbuf1, &(erk_key).val.addr, 0),       \
                  skBagStrerror(erk_err))

#define ERR_ITERATOR(ei_description, ei_err)                    \
    skAppPrintErr("Error in %s bag iterator: %s",               \
                  ei_description, skBagStrerror(ei_err))

/* the IDs for options.  we need this early */
typedef enum {
    OPT_ADD,
    OPT_SUBTRACT,
    OPT_MINIMIZE,
    OPT_MAXIMIZE,
    OPT_DIVIDE,
    OPT_COMPARE,
    OPT_SCALAR_MULTIPLY,
    OPT_INTERSECT,
    OPT_COMPLEMENT,
    OPT_MINKEY,
    OPT_MAXKEY,
    OPT_MINCOUNTER,
    OPT_MAXCOUNTER,
    OPT_INVERT,
    OPT_COVERSET,
    OPT_OUTPUT_PATH
} appOptionsEnum;

#define NUM_BAG_COMARISONS 5

/* types of comparisons we support */
typedef enum {
    BAG_CMP_LT,
    BAG_CMP_LE,
    BAG_CMP_EQ,
    BAG_CMP_GE,
    BAG_CMP_GT
} bag_compare_t;


/* LOCAL VARIABLES */

/* the output bag that we create */
static skBag_t *out_bag = NULL;

/* where to write the resulting bag */
static skstream_t *out_stream = NULL;

/* What action the user wants to take (add, intersect, etc).  The
 * variable 'user_action' should be a value between OPT_ADD and
 * OPT_SCALAR_MULTIPLY.  Set 'user_action_none' to a value outside
 * that range; this denotes that the user did not choose a value.
 */
static const appOptionsEnum user_action_none = OPT_OUTPUT_PATH;
static appOptionsEnum user_action;

/* Type of comparison to do */
static bag_compare_t bag_cmp;

/* When writing the new bag, only output entries whose keys and/or
 * whose values lie between these limits. */
static uint64_t mincounter = SKBAG_COUNTER_MIN;
static uint64_t maxcounter = SKBAG_COUNTER_MAX;

static skipaddr_t minkey;
static skipaddr_t maxkey;

static int have_minkey = 0;
static int have_maxkey = 0;

/* Bag comparison map */
static const struct bag_compare_map_st {
    const char     *name;
    bag_compare_t   value;
} bag_compare_map[NUM_BAG_COMARISONS] = {
    {"lt",      BAG_CMP_LT},
    {"le",      BAG_CMP_LE},
    {"eq",      BAG_CMP_EQ},
    {"ge",      BAG_CMP_GE},
    {"gt",      BAG_CMP_GT}
};

/* Index of current file argument in argv */
static int arg_index = 0;

/* the compression method to use when writing the file.
 * sksiteCompmethodOptionsRegister() will set this to the default or
 * to the value the user specifies. */
static sk_compmethod_t comp_method;

/* buffers for printing IPs */
static char ipbuf1[SK_NUM2DOT_STRLEN];
static char ipbuf2[SK_NUM2DOT_STRLEN];

/* For scalar multiplication, the scalar */
static uint64_t scalar_multiply = 1;

/* Set to use for intersect and complement intersect */
static skipset_t *mask_set = NULL;

static struct app_flags_st {
    /* whether the mask_set is intersect(0) or complement(1) */
    unsigned  complement_set:1;

    /* whether to produce a coverset(1) instead of a bag(0) */
    unsigned  coverset      :1;

    /* whether to invert the bag(1 = yes) */
    unsigned  invert        :1;
} app_flags = {
    0, 0, 0
};

/* whether the --note-strip flag was specified */
static int note_strip = 0;


/* OPTIONS SETUP */

static struct option appOptions[] = {
    {"add",                  NO_ARG,       0, OPT_ADD},
    {"subtract",             NO_ARG,       0, OPT_SUBTRACT},
    {"minimize",             NO_ARG,       0, OPT_MINIMIZE},
    {"maximize",             NO_ARG,       0, OPT_MAXIMIZE},
    {"divide",               NO_ARG,       0, OPT_DIVIDE},
    {"compare",              REQUIRED_ARG, 0, OPT_COMPARE},
    {"scalar-multiply",      REQUIRED_ARG, 0, OPT_SCALAR_MULTIPLY},
    {"intersect",            REQUIRED_ARG, 0, OPT_INTERSECT},
    {"complement-intersect", REQUIRED_ARG, 0, OPT_COMPLEMENT},
    {"minkey",               REQUIRED_ARG, 0, OPT_MINKEY},
    {"maxkey",               REQUIRED_ARG, 0, OPT_MAXKEY},
    {"mincounter",           REQUIRED_ARG, 0, OPT_MINCOUNTER},
    {"maxcounter",           REQUIRED_ARG, 0, OPT_MAXCOUNTER},
    {"invert",               NO_ARG,       0, OPT_INVERT},
    {"coverset",             NO_ARG,       0, OPT_COVERSET},
    {"output-path",          REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {0,0,0,0}                /* sentinel entry */
};

static const char *appHelp[] = {
    "Add the counters for each key across all Bag files",
    "Subtract from first Bag file all subsequent Bag files",
    ("Write to the output the minimum counter for each key across\n"
     "\tall input Bag files. Counter for a missing key is 0"),
    ("Write to the output the maximum counter for each key across\n"
     "\tall input Bag files"),
    "Divide the first Bag by the second Bag",
    ("Compare key/value pairs in exactly two Bag files.  Keep\n"
     "\tonly those keys in the first Bag that also appear in the second Bag\n"
     "\tand whose counter is OP those in the second Bag, where OP is one of:\n"
     "\t  'lt': less than; 'le': less than or equal to; 'eq': equal to;\n"
     "\t  'ge': greater than or equal to; 'gt': greater than.\n"
     "\tThe counter for each key that remains is set to 1."),
    ("Multiply each counter in the Bag by the specified\n"
     "\tvalue. Accepts a single Bag file as input."),
    "Masks keys in bag file using IPs in given IPset file",
    ("Masks keys in bag file using IPs NOT\n"
     "\tin given IPset file"),
    NULL,
    NULL,
    NULL,
    NULL,
    "Count keys for each unique counter value",
    "Extract the IPs from the bag file into an IPset file",
    "Redirect output to specified file.",
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  bagtoolDivide(skstream_t *stream);
static int  bagtoolSubtract(skstream_t *stream);
static int  writeOutput(void);


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
    ("[SWITCHES] BAG_FILE [BAG_FILES...]\n"                                \
     "\tPerform operations on bag files, creating a new bag file.\n"       \
     "\tRequires at least one bag file to be given on the command line\n"  \
     "\tor to be read from the standard input.  The resulting bag will\n"  \
     "\twill be written to the specified output file or to the standard\n" \
     "\toutput.\n")

    FILE *fh = USAGE_FH;
    int i;
#if SK_ENABLE_IPV6
    const char *v4_or_v6 = "v6";
#else
    const char *v4_or_v6 = "v4";
#endif

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    for (i = 0; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch (appOptions[i].val) {
          case OPT_MINKEY:
            fprintf(fh,
                    ("Output records whose key is at least VALUE,"
                     " an IP%s address\n\tor an integer between"
                     " %" PRIu64 " and %" PRIu64 ", inclusive."
                     " Def. Records with\n\tnon-zero counters"),
                    v4_or_v6,
                    (uint64_t)SKBAG_KEY_MIN, (uint64_t)SKBAG_KEY_MAX);
            break;
          case OPT_MAXKEY:
            fprintf(fh,
                    ("Output records whose key is not more than VALUE,"
                     " an IP%s\n\taddress or an integer."
                     " Def. Records with non-zero counters"),
                    v4_or_v6);
            break;
          case OPT_MINCOUNTER:
            fprintf(fh,
                    ("Output records whose counter is at least VALUE,"
                     " an integer\n\tbetween %" PRIu64 " and %" PRIu64
                     ", inclusive. Def. %" PRIu64),
                    BAGTOOL_MIN_COUNTER, SKBAG_COUNTER_MAX,
                    BAGTOOL_MIN_COUNTER);
            break;
          case OPT_MAXCOUNTER:
            fprintf(fh,
                    ("Output records whose counter is not more than VALUE,"
                     " an\n\tinteger.  Def. %" PRIu64),
                    SKBAG_COUNTER_MAX);
            break;
          default:
            fprintf(fh, "%s", appHelp[i]);
            break;
        }
        fprintf(fh, "\n");
    }
    skOptionsNotesUsage(fh);
    sksiteCompmethodOptionsUsage(fh);
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

    /* free the output bag, stream, and set */
    skBagDestroy(&out_bag);
    skStreamDestroy(&out_stream);
    skIPSetDestroy(&mask_set);

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
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    user_action = user_action_none;

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsNotesRegister(&note_strip)
        || sksiteCompmethodOptionsRegister(&comp_method))
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

    /* parse the options; returns the index into argv[] of the first
     * non-option or < 0 on error  May re-arrange argv[]. */
    arg_index = skOptionsParse(argc, argv);
    assert(arg_index <= argc);
    if (arg_index < 0) {
        /* options parsing should print error */
        skAppUsage();             /* never returns */
    }

    /* The default action is to add the bags together */
    if (user_action_none == user_action) {
        user_action = OPT_ADD;
    }

    /* error if a minimum is greater than a maximum */
    if (mincounter > maxcounter) {
        skAppPrintErr(("Minimum counter greater than maximum: "
                       "%" PRIu64 " > %" PRIu64),
                      mincounter, maxcounter);
        exit(EXIT_FAILURE);
    }
    if (have_minkey && have_maxkey &&
        skipaddrCompare(&minkey, &maxkey) > 0)
    {
        skAppPrintErr("Minimum key greater than maximum: %s > %s",
                      skipaddrString(ipbuf1, &minkey, 0),
                      skipaddrString(ipbuf2, &maxkey, 0));
        exit(EXIT_FAILURE);
    }

    /* arg_index is looking at first file name to process.  We
     * require at least one bag file, either from stdin or from the
     * command line. */
    switch (user_action) {
      case OPT_COMPARE:
      case OPT_DIVIDE:
        if ((argc - arg_index) != 2) {
            skAppPrintErr("The --%s switch requires exactly two Bag files",
                          appOptions[user_action].name);
            skAppUsage();
        }
        break;

      case OPT_SCALAR_MULTIPLY:
        if ((argc - arg_index) > 1) {
            skAppPrintErr("The --%s switch operates on a single Bag file",
                          appOptions[user_action].name);
            skAppUsage();
        }
        if ((arg_index == argc) && (FILEIsATty(stdin))) {
            skAppPrintErr("No input files on command line and"
                          " stdin is connected to a terminal");
            skAppUsage();
        }
        break;

      default:
        if ((arg_index == argc) && (FILEIsATty(stdin))) {
            skAppPrintErr("No input files on command line and"
                          " stdin is connected to a terminal");
            skAppUsage();
        }
        break;
    }

    /* Set the default output location */
    if (out_stream == NULL) {
        if ((rv = skStreamCreate(&out_stream, SK_IO_WRITE, SK_CONTENT_SILK))
            || (rv = skStreamBind(out_stream, "stdout")))
        {
            skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
            skStreamDestroy(&out_stream);
            exit(EXIT_FAILURE);
        }
    }

    /* Open the output file */
    if ((rv = skStreamSetCompressionMethod(out_stream, comp_method))
        || (rv = skStreamOpen(out_stream)))
    {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        skStreamDestroy(&out_stream);
        exit(EXIT_FAILURE);
    }

    /* add any notes (annotations) to the output */
    rv = skOptionsNotesAddToStream(out_stream);
    if (rv) {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        exit(EXIT_FAILURE);
    }
    skOptionsNotesTeardown();

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
    skstream_t *stream = NULL;
    uint64_t val64;
    int i;
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_ADD:
      case OPT_SUBTRACT:
      case OPT_DIVIDE:
      case OPT_MINIMIZE:
      case OPT_MAXIMIZE:
        if (user_action != user_action_none) {
            if (user_action == (appOptionsEnum)opt_index) {
                skAppPrintErr("Invalid %s: Switch used multiple times",
                              appOptions[opt_index].name);
            } else {
                skAppPrintErr("Switches --%s and --%s are incompatible",
                              appOptions[opt_index].name,
                              appOptions[user_action].name);
            }
            return 1;
        }
        user_action = (appOptionsEnum)opt_index;
        break;

      case OPT_COMPARE:
        if (user_action != user_action_none) {
            if (user_action == (appOptionsEnum)opt_index) {
                skAppPrintErr("Invalid %s: Switch used multiple times",
                              appOptions[opt_index].name);
            } else {
                skAppPrintErr("Switches --%s and --%s are incompatible",
                              appOptions[opt_index].name,
                              appOptions[user_action].name);
            }
            return 1;
        }
        user_action = (appOptionsEnum)opt_index;
        for (i = 0; i < NUM_BAG_COMARISONS; ++i) {
            if (0 == strcasecmp(opt_arg, bag_compare_map[i].name)) {
                bag_cmp = bag_compare_map[i].value;
                break;
            }
        }
        if (i == NUM_BAG_COMARISONS) {
            skAppPrintErr("Invalid %s: Unknown comparator '%s'",
                          appOptions[opt_index].name, opt_arg);
            return 1;
        }
        break;

      case OPT_SCALAR_MULTIPLY:
        if (user_action != user_action_none) {
            if (user_action == (appOptionsEnum)opt_index) {
                skAppPrintErr("Invalid %s: Switch used multiple times",
                              appOptions[opt_index].name);
            } else {
                skAppPrintErr("Switches --%s and --%s are incompatible",
                              appOptions[opt_index].name,
                              appOptions[user_action].name);
            }
            return 1;
        }
        user_action = (appOptionsEnum)opt_index;
        rv = skStringParseUint64(&val64, opt_arg, 1, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        scalar_multiply = val64;
        break;

      case OPT_INVERT:
        app_flags.invert = 1;
        break;

      case OPT_COVERSET:
        app_flags.coverset = 1;
        break;

      case OPT_COMPLEMENT:
        app_flags.complement_set = 1;
        /* FALLTHROUGH */

      case OPT_INTERSECT:
        if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
            || (rv = skStreamBind(stream, opt_arg))
            || (rv = skStreamOpen(stream)))
        {
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            skStreamDestroy(&stream);
            exit(EXIT_FAILURE);
        }
        rv = skIPSetRead(&mask_set, stream);
        if (rv) {
            if (SKIPSET_ERR_FILEIO == rv) {
                skStreamPrintLastErr(stream, skStreamGetLastReturnValue(stream),
                                     &skAppPrintErr);
            } else {
                skAppPrintErr("Unable to read %s IPset from '%s': %s",
                              appOptions[user_action].name, opt_arg,
                              skIPSetStrerror(rv));
            }
            skStreamDestroy(&stream);
            exit(EXIT_FAILURE);
        }
        skStreamDestroy(&stream);
        break;

      case OPT_MINCOUNTER:
        rv = skStringParseUint64(&mincounter, opt_arg,
                                 BAGTOOL_MIN_COUNTER, SKBAG_COUNTER_MAX);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MAXCOUNTER:
        rv = skStringParseUint64(&maxcounter, opt_arg,
                                 BAGTOOL_MIN_COUNTER, SKBAG_COUNTER_MAX);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MINKEY:
        rv = skStringParseIP(&minkey, opt_arg);
        if (rv) {
            goto PARSE_ERROR;
        }
        have_minkey = 1;
        break;

      case OPT_MAXKEY:
        rv = skStringParseIP(&maxkey, opt_arg);
        if (rv) {
            goto PARSE_ERROR;
        }
        have_maxkey = 1;
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
 *  ok = bagtoolDivide(pathname);
 *
 *    Divide the values in the global 'out_bag' by the values
 *    found in the bag stored in 'pathname'.  The keys of the two bags
 *    must match exactly.  Return 0 on success; non-zero otherwise.
 */
static int
bagtoolDivide(
    skstream_t         *stream)
{
    skBag_t *in_bag;
    skBagIterator_t *iter_dividend = NULL;
    skBagIterator_t *iter_divisor = NULL;
    skBagErr_t rv_dividend;
    skBagErr_t rv_divisor;
    skBagTypedKey_t key_dividend;
    skBagTypedKey_t key_divisor;
    skBagTypedCounter_t counter_dividend;
    skBagTypedCounter_t counter_divisor;
    int cmp;
    int rv = -1;

    /* set the types for the keys and counters */
    key_divisor.type = SKBAG_KEY_IPADDR;
    counter_divisor.type = SKBAG_COUNTER_U64;
    key_dividend.type = SKBAG_KEY_IPADDR;
    counter_dividend.type = SKBAG_COUNTER_U64;

    /* read the bag containing the divisors */
    rv_divisor = skBagRead(&in_bag, stream);
    if (SKBAG_OK != rv_divisor) {
        ERR_READ_BAG(stream, rv_divisor);
        return rv;
    }

    /* Create an iterator to loop over the divisor bag */
    rv_divisor = skBagIteratorCreate(in_bag, &iter_divisor);
    if (SKBAG_OK != rv_divisor) {
        iter_divisor = NULL;
        goto END;
    }
    rv_divisor = skBagIteratorNextTyped(iter_divisor, &key_divisor,
                                        &counter_divisor);

    /* Create an iterator to loop over the dividend bag */
    rv_dividend = skBagIteratorCreate(out_bag, &iter_dividend);
    if (SKBAG_OK != rv_dividend) {
        iter_dividend = NULL;
        goto END;
    }
    rv_dividend = skBagIteratorNextTyped(iter_dividend, &key_dividend,
                                         &counter_dividend);

    while (SKBAG_OK == rv_dividend && SKBAG_OK == rv_divisor) {
        cmp = skipaddrCompare(&key_divisor.val.addr, &key_dividend.val.addr);
        if (0 == cmp) {
            /* key is in both bags.  do the division and round up for
             * any remainder that is more than half the divisor. */
            counter_dividend.val.u64 =
                ((counter_dividend.val.u64 / counter_divisor.val.u64)
                 + ((counter_dividend.val.u64 % counter_divisor.val.u64)
                    >= ((counter_divisor.val.u64 >> 1)
                        + (counter_divisor.val.u64 & 1))));
            rv_dividend = skBagCounterSet(out_bag, &key_dividend,
                                          &counter_dividend);
            if (SKBAG_OK != rv_dividend) {
                ERR_SET_COUNT(key_dividend, counter_dividend, rv_dividend);
                goto END;
            }

            /* get next keys */
            rv_dividend = skBagIteratorNextTyped(iter_dividend, &key_dividend,
                                                 &counter_dividend);
            rv_divisor = skBagIteratorNextTyped(iter_divisor, &key_divisor,
                                                &counter_divisor);

        } else if (cmp < 0) {
            /* key is in divisor bag only; ignore--that is, treat as
             * if dividend value is 0 */
            rv_divisor = skBagIteratorNextTyped(iter_divisor, &key_divisor,
                                                &counter_divisor);
        } else {
            /* key is only in dividend bag.  complain---that is, treat
             * as division by 0. */
            skAppPrintErr("Error dividing bags; key %s not in divisor bag",
                          skipaddrString(ipbuf1, &key_dividend.val.addr, 0));
            goto END;
        }
    }

    /* Verify no errors from the divisor bag */
    if (SKBAG_OK != rv_divisor) {
        if (SKBAG_ERR_KEY_NOT_FOUND == rv_divisor) {
            rv_divisor = SKBAG_OK;
        } else {
            ERR_ITERATOR("divisor", rv_divisor);
            goto END;
        }
    }

    /* If keys remain in the dividend bag, we have a division by zero
     * problem. */
    if (SKBAG_OK == rv_dividend) {
        skAppPrintErr(("Error dividing bags; key %" PRIu32
                       " not in divisor bag"),
                      key_dividend.val.u32);
        goto END;
    } else if (SKBAG_ERR_KEY_NOT_FOUND != rv_dividend) {
        ERR_ITERATOR("dividend", rv_divisor);
        goto END;
    }

    /* Success */
    rv = 0;

    /* Modify key-type and counter-type */
    skBagModify(out_bag, skBagFieldTypeMerge(skBagKeyFieldType(out_bag),
                                             skBagKeyFieldType(in_bag)),
                skBagFieldTypeMerge(skBagCounterFieldType(out_bag),
                                    skBagCounterFieldType(in_bag)),
                SKBAG_OCTETS_NO_CHANGE, SKBAG_OCTETS_NO_CHANGE);

  END:
    if (iter_dividend) {
        skBagIteratorDestroy(iter_dividend);
    }
    if (iter_divisor) {
        skBagIteratorDestroy(iter_divisor);
    }

    /* Blow the bag away. We're done with it */
    skBagDestroy(&in_bag);

    return rv;
}


/*
 *  status = bagtoolSubtractInit(stream_bag, bag);
 *
 *    Callback to support bagtoolSubtract().  Set the key and counter
 *    types on the output bag.
 */
static skBagErr_t
bagtoolSubtractInit(
    const skBag_t      *bag2,
    void               *v_bag1)
{
    skBag_t *bag1 = (skBag_t*)v_bag1;

    /* Modify key-type and counter-type */
    return skBagModify(bag1, skBagFieldTypeMerge(skBagKeyFieldType(bag1),
                                                 skBagKeyFieldType(bag2)),
                       skBagFieldTypeMerge(skBagCounterFieldType(bag1),
                                           skBagCounterFieldType(bag2)),
                       SKBAG_OCTETS_NO_CHANGE, SKBAG_OCTETS_NO_CHANGE);
}


/*
 *  status = bagtoolSubtractEntry(stream_bag, key, counter, bag);
 *
 *    Callback to support bagtoolSubtract().  Subtract from the value
 *    of 'key' in 'bag' the value 'counter'.  If the resulting value
 *    would be negative, remove 'key' from 'bag'.  Return SKBAG_OK on
 *    success.
 */
static skBagErr_t
bagtoolSubtractEntry(
    const skBag_t               UNUSED(*bag2),
    const skBagTypedKey_t              *key_bag2,
    const skBagTypedCounter_t          *counter_bag2,
    void                               *v_bag1)
{
    skBag_t *bag1 = (skBag_t*)v_bag1;
    skBagErr_t rv_bag;

    rv_bag = skBagCounterSubtract(bag1, key_bag2, counter_bag2, NULL);
    switch (rv_bag) {
      case SKBAG_OK:
        break;

      case SKBAG_ERR_OP_BOUNDS:
        /* value is negative or not in bag. remove it */
        rv_bag = skBagKeyRemove(bag1, key_bag2);
        if (SKBAG_OK != rv_bag) {
            ERR_REMOVE_KEY(*key_bag2, rv_bag);
        }
        break;

      default:
        skAppPrintErr(("Error when subtracting from bag for key %" PRIu32
                       ": %s"),
                      key_bag2->val.u32, skBagStrerror(rv_bag));
        break;
    }

    return rv_bag;
}



/*
 *  ok = bagtoolSubtract(stream);
 *
 *    Subtract the values found in the bag read from 'stream' from
 *    the values in the global 'out_bag'.  Missing keys are
 *    treated as if they had the value 0.  A negative value is treated
 *    as 0.
 */
static int
bagtoolSubtract(
    skstream_t         *stream)
{
    skBagErr_t rv_bag;

    rv_bag = skBagProcessStreamTyped(stream, out_bag, &bagtoolSubtractInit,
                                     &bagtoolSubtractEntry);

    return ((SKBAG_OK == rv_bag) ? 0 : -1);
}


/*
 *  ok = bagtoolMinimize(stream);
 *
 *    Perform multi-set intersection.
 *
 *    Update the counters in the global 'out_bag' to the minimum of
 *    the 'out_bag' counter or the counter from the bag read from
 *    'stream'.  Missing keys are treated as 0.
 */
static int
bagtoolMinimize(
    skstream_t         *stream)
{
    skBag_t *in_bag;
    skBagIterator_t *iter = NULL;
    skBagTypedKey_t key;
    skBagTypedCounter_t out_counter;
    skBagTypedCounter_t in_counter;
    skBagErr_t rv_bag;

    /* Read Bag from the 'stream' */
    rv_bag = skBagRead(&in_bag, stream);
    if (SKBAG_OK != rv_bag) {
        ERR_READ_BAG(stream, rv_bag);
        return 1;
    }

    /* Create an iterator to loop over the global output bag. */
    rv_bag = skBagIteratorCreate(out_bag, &iter);
    if (SKBAG_OK != rv_bag) {
        goto END;
    }

    /* set the types for the key and counters */
    key.type = SKBAG_KEY_IPADDR;
    in_counter.type = SKBAG_COUNTER_U64;
    out_counter.type = SKBAG_COUNTER_U64;

    /* If out_bag's counter is larger than that from in_bag, set
     * out_bag's counter to the input value.  Since missing keys have
     * a value of 0, this will remove keys from out_bag that do not
     * appear in in_bag. */
    while (skBagIteratorNextTyped(iter, &key, &out_counter) == SKBAG_OK) {
        skBagCounterGet(in_bag, &key, &in_counter);
        if (in_counter.val.u64 < out_counter.val.u64) {
            /* must set value to the minimum */
            rv_bag = skBagCounterSet(out_bag, &key, &in_counter);
            if (SKBAG_OK != rv_bag) {
                ERR_SET_COUNT(key, in_counter, rv_bag);
                goto END;
            }
        }
    }

  END:
    if (iter) {
        skBagIteratorDestroy(iter);
    }
    /* Blow the input bag away. We're done with it */
    skBagDestroy(&in_bag);

    return ((SKBAG_OK == rv_bag) ? 0 : -1);
}


/*
 *  status = bagtoolMaximizeCallback(key, counter, bag);
 *
 *    Callback to support bagtoolMaximize().  Look up the value of
 *    'key' in 'bag'.  If 'counter' is greater than the value, set the
 *    value of 'key' to 'counter'.  Since missing keys have the value
 *    0, keys not already in 'bag' will be added.  Return SKBAG_OK on
 *    success.
 */
static skBagErr_t
bagtoolMaximizeCallback(
    const skBag_t               UNUSED(*bag2),
    const skBagTypedKey_t              *key_bag2,
    const skBagTypedCounter_t          *counter_bag2,
    void                               *v_bag1)
{
    skBag_t *bag1 = (skBag_t*)v_bag1;
    skBagTypedCounter_t counter_bag1;
    skBagErr_t rv_bag;

    counter_bag1.type = SKBAG_COUNTER_U64;

    /* Use the counter value from bag2 if that value is larger than
     * the value from bag1. */
    rv_bag = skBagCounterGet(bag1, key_bag2, &counter_bag1);
    if (counter_bag2->val.u64 > counter_bag1.val.u64) {
        /* set counter to the maximum */
        rv_bag = skBagCounterSet(bag1, key_bag2, counter_bag2);
        if (SKBAG_OK != rv_bag) {
            ERR_SET_COUNT(*key_bag2, *counter_bag2, rv_bag);
        }
    }

    return rv_bag;
}


/*
 *  ok = bagtoolMaximize(stream);
 *
 *    Perform multi-set union.
 *
 *    Update the counters in the global 'out_bag' to the maximum of
 *    the 'out_bag' counter or the counter from the bag read from
 *    'stream'.  Missing keys are treated as 0.
 */
static int
bagtoolMaximize(
    skstream_t         *stream)
{
    skBagErr_t rv_bag;

    rv_bag = skBagProcessStreamTyped(stream, out_bag, NULL,
                                     &bagtoolMaximizeCallback);
    return ((SKBAG_OK == rv_bag) ? 0 : -1);
}


/*
 *  ok = bagtoolCompare(stream);
 *
 *    Subtract the values found in the bag read from 'stream' from
 *    the values in the global 'out_bag'.  Missing keys are
 *    .  A negative value is treated
 *    as 0.
 */
static int
bagtoolCompare(
    skstream_t         *stream)
{
    skBag_t *bag2;

    skBagIterator_t *iter_bag1 = NULL;
    skBagIterator_t *iter_bag2 = NULL;
    skBagErr_t rv_bag1 = SKBAG_OK;
    skBagErr_t rv_bag2;
    int cmp;

    skBagTypedKey_t key_bag1;
    skBagTypedKey_t key_bag2;

    skBagTypedCounter_t counter_bag1;
    skBagTypedCounter_t counter_bag2;
    skBagTypedCounter_t new_counter;

    rv_bag2 = skBagRead(&bag2, stream);
    if (SKBAG_OK != rv_bag2) {
        ERR_READ_BAG(stream, rv_bag2);
        return 1;
    }

    /* set the types for the keys and counters */
    key_bag1.type = SKBAG_KEY_IPADDR;
    key_bag2.type = SKBAG_KEY_IPADDR;
    counter_bag1.type = SKBAG_COUNTER_U64;
    counter_bag2.type = SKBAG_COUNTER_U64;
    new_counter.type = SKBAG_COUNTER_U64;

    /* Create an iterator to loop over the input bag */
    rv_bag2 = skBagIteratorCreate(bag2, &iter_bag2);
    if (SKBAG_OK != rv_bag2) {
        goto END;
    }
    rv_bag2 = skBagIteratorNextTyped(iter_bag2, &key_bag2, &counter_bag2);

    /* Create an iterator to loop over the output bag */
    rv_bag1 = skBagIteratorCreate(out_bag, &iter_bag1);
    if (SKBAG_OK != rv_bag1) {
        goto END;
    }
    rv_bag1 = skBagIteratorNextTyped(iter_bag1, &key_bag1, &counter_bag1);

    while (SKBAG_OK == rv_bag1 && SKBAG_OK == rv_bag2) {
        cmp = skipaddrCompare(&key_bag1.val.addr, &key_bag2.val.addr);
        if (0 == cmp) {
            /* key is in both bags */
            new_counter.val.u64 = 0;
            switch (bag_cmp) {
              case BAG_CMP_LT:
                if (counter_bag1.val.u64 < counter_bag2.val.u64) {
                    new_counter.val.u64 = 1;
                }
                break;
              case BAG_CMP_LE:
                if (counter_bag1.val.u64 <= counter_bag2.val.u64) {
                    new_counter.val.u64 = 1;
                }
                break;
              case BAG_CMP_EQ:
                if (counter_bag1.val.u64 == counter_bag2.val.u64) {
                    new_counter.val.u64 = 1;
                }
                break;
              case BAG_CMP_GE:
                if (counter_bag1.val.u64 >= counter_bag2.val.u64) {
                    new_counter.val.u64 = 1;
                }
                break;
              case BAG_CMP_GT:
                if (counter_bag1.val.u64 > counter_bag2.val.u64) {
                    new_counter.val.u64 = 1;
                }
                break;
            }
            rv_bag1 = skBagCounterSet(out_bag, &key_bag1, &new_counter);
            if (rv_bag1) {
                ERR_SET_COUNT(key_bag1, new_counter, rv_bag1);
                goto END;
            }
            rv_bag1 = skBagIteratorNextTyped(iter_bag1, &key_bag1,
                                             &counter_bag1);
            rv_bag2 = skBagIteratorNextTyped(iter_bag2, &key_bag2,
                                             &counter_bag2);

        } else if (cmp < 0) {
            /* key is in bag1 only.  remove it */
            rv_bag1 = skBagKeyRemove(out_bag, &key_bag1);
            if (rv_bag1) {
                ERR_REMOVE_KEY(key_bag1, rv_bag1);
                goto END;
            }
            rv_bag1 = skBagIteratorNextTyped(iter_bag1, &key_bag1,
                                             &counter_bag1);

        } else {
            assert(cmp > 0);

            /* key is in bag2 only.  it will not appear in the output. */
            rv_bag2 = skBagIteratorNextTyped(iter_bag2, &key_bag2,
                                             &counter_bag2);
        }
    }

    /* Verify no errors from bag2 */
    if (SKBAG_OK != rv_bag2) {
        if (SKBAG_ERR_KEY_NOT_FOUND == rv_bag2) {
            rv_bag2 = SKBAG_OK;
        } else {
            ERR_ITERATOR(skStreamGetPathname(stream), rv_bag2);
            goto END;
        }
    }

    /* Check to see if keys remain in bag1; if so, we must remove
     * them. */
    while (SKBAG_OK == rv_bag1) {
        rv_bag1 = skBagKeyRemove(out_bag, &key_bag1);
        if (rv_bag1) {
            ERR_REMOVE_KEY(key_bag1, rv_bag1);
            goto END;
        }
        rv_bag1 = skBagIteratorNextTyped(iter_bag1, &key_bag1, &counter_bag1);
    }

    /* Verify that we reached the end of bag1 */
    if (SKBAG_ERR_KEY_NOT_FOUND == rv_bag1) {
        rv_bag1 = SKBAG_OK;
    } else {
        ERR_ITERATOR(skStreamGetPathname(out_stream), rv_bag1);
        goto END;
    }

  END:
    if (iter_bag1) {
        skBagIteratorDestroy(iter_bag1);
    }
    if (iter_bag2) {
        skBagIteratorDestroy(iter_bag2);
    }
    /* Blow the bag away. We're done with it */
    skBagDestroy(&bag2);

    return (((SKBAG_OK == rv_bag1) && (SKBAG_OK == rv_bag2)) ? 0 : -1);
}


/*
 *  status = bagtoolInvert(bag);
 *
 *    Invert the Bag 'bag' in place.  Return 0 on success, or -1 on
 *    error.
 */
static int
bagtoolInvert(
    skBag_t            *bag)
{
    skBag_t *inv_bag = NULL;
    skBagIterator_t *iter = NULL;
    skBagTypedKey_t bin;
    skBagTypedKey_t key;
    skBagTypedCounter_t counter;
    skBagErr_t rv_bag;
    int rv = -1;

    /* We cannot invert the bag in place.  We create a new bag and
     * make a pass through the existing bag, removing its entries and
     * inserting the inverted values into the new bag.  Once that is
     * complete, we make a pass over the inverted bag and set the
     * values in the original bag. */
    if (skBagCreate(&inv_bag) != SKBAG_OK) {
        goto END;
    }

    /* set the types for the keys and counter */
    bin.type = SKBAG_KEY_U32;
    key.type = SKBAG_KEY_IPADDR;
    counter.type = SKBAG_COUNTER_U64;

    /* Run through the original bag */
    rv_bag = skBagIteratorCreate(bag, &iter);
    if (SKBAG_OK != rv_bag) {
        goto END;
    }
    while (skBagIteratorNextTyped(iter, &key, &counter) == SKBAG_OK) {
        /* remove this key from the bag--set its count to 0 */
        rv_bag = skBagKeyRemove(bag, &key);
        if (SKBAG_OK != rv_bag) {
            ERR_REMOVE_KEY(key, rv_bag);
            goto END;
        }
        /* determine key to use in inverted bag, handing overflow */
        bin.val.u32 = ((counter.val.u64 < (uint64_t)SKBAG_KEY_MAX)
                       ? (uint32_t)counter.val.u64
                       : SKBAG_KEY_MAX);

        /* increment value in the inverted bag */
        rv_bag = skBagCounterIncrement(inv_bag, &bin);
        if (SKBAG_OK != rv_bag) {
            if (SKBAG_ERR_OP_BOUNDS == rv_bag) {
                skAppPrintErr(("Overflow when inverting bag (key %" PRIu32 ")"),
                              bin.val.u32);
                goto END;
            }
            skAppPrintErr("Error when inverting bag: %s",
                          skBagStrerror(rv_bag));
            goto END;
        }
    }
    skBagIteratorDestroy(iter);
    iter = NULL;

    /* The output bag is empty; modify it to hold integer keys and
     * counters */
    rv_bag = skBagModify(bag, SKBAG_FIELD_CUSTOM, SKBAG_FIELD_CUSTOM,
                         sizeof(uint32_t), SKBAG_OCTETS_NO_CHANGE);
    if (SKBAG_OK != rv_bag) {
        skAppPrintErr("Error when modifying bag: %s",
                      skBagStrerror(rv_bag));
        goto END;
    }

    /* Run through the inverted bag and set the values in the output bag. */
    rv_bag = skBagIteratorCreate(inv_bag, &iter);
    if (SKBAG_OK != rv_bag) {
        goto END;
    }
    while (skBagIteratorNextTyped(iter, &bin, &counter) == SKBAG_OK) {
        rv_bag = skBagCounterSet(bag, &bin, &counter);
        if (SKBAG_OK != rv_bag) {
            ERR_SET_COUNT(bin, counter, rv_bag);
            goto END;
        }
    }

    /* done */
    rv = 0;

  END:
    if (inv_bag) {
        skBagDestroy(&inv_bag);
    }
    if (iter) {
        skBagIteratorDestroy(iter);
    }
    return rv;
}


/*
 *  status = bagtoolCoverSet(bag, stream);
 *
 *    Create an IPset and fill it with the keys in the Bag 'bag'.
 *    Write the IPset to 'stream'.  Return 0 on success, or -1 on
 *    failure.
 */
static int
bagtoolCoverSet(
    skBag_t            *bag,
    skstream_t         *stream)
{
    skBagIterator_t *iter = NULL;
    skBagTypedKey_t key;
    skBagTypedCounter_t counter;
    int rv;
    skipset_t *set = NULL;

    /* Create the cover set */
    rv = skIPSetCreate(&set, 0);
    if (rv) {
        skAppPrintErr("Error creating cover IPset: %s", skIPSetStrerror(rv));
        goto END;
    }

    /* set the types for the keys and counter */
    key.type = SKBAG_KEY_IPADDR;
    counter.type = SKBAG_COUNTER_U64;

    /* Run through the bag, adding items to the set */
    if (skBagIteratorCreate(bag, &iter)) {
        goto END;
    }

    while (skBagIteratorNextTyped(iter, &key, &counter) == SKBAG_OK) {
        rv = skIPSetInsertAddress(set, &key.val.addr, 0);
        if (rv) {
            skAppPrintErr("Error inserting into IPset: %s",
                          skIPSetStrerror(rv));
            goto END;
        }
    }

    skIPSetClean(set);

    /* Write the set */
    rv = skIPSetWrite(set, stream);
    if (rv) {
        if (SKIPSET_ERR_FILEIO == rv) {
            char errbuf[2 * PATH_MAX];
            skStreamLastErrMessage(stream, skStreamGetLastReturnValue(stream),
                                   errbuf, sizeof(errbuf));
            skAppPrintErr("Error writing cover IPset: %s", errbuf);
        } else {
            skAppPrintErr("Error writing cover IPset to '%s': %s",
                          skStreamGetPathname(stream), skIPSetStrerror(rv));
        }
        goto END;
    }

    /* done */
    rv = 0;

  END:
    if (iter) {
        skBagIteratorDestroy(iter);
    }
    if (set) {
        skIPSetDestroy(&set);
    }

    return ((rv == 0) ? 0 : -1);
}


/*
 *  ok = applyCutoffs(bag);
 *
 *    Run through the bag and zero out any entries not within range or
 *    which aren't in the masking set.  Return 0 on success, non-zero
 *    otherwise.
 *
 *    FIXME: We could do some of this during the insertion stage
 *    instead output to save ourselves some storage. This is not the
 *    most efficient implementation.
 */
static int
applyCutoffs(
    skBag_t            *bag)
{
    skBagIterator_t *iter = NULL;
    skBagTypedKey_t key;
    skBagTypedCounter_t counter;
    skBagErr_t rv_bag;
    int fail = 0;

    /* determine whether there are any cut-offs to apply. If no sets
     * are given and the limits are all at their defaults, return. */
    if ((NULL == mask_set)
        && (0 == have_minkey)
        && (0 == have_maxkey)
        && (SKBAG_COUNTER_MIN == mincounter)
        && (SKBAG_COUNTER_MAX == maxcounter))
    {
        return 0;
    }

    /* set the types for the keys and counter */
    key.type = SKBAG_KEY_IPADDR;
    counter.type = SKBAG_COUNTER_U64;

    /* Create an iterator to loop over the bag */
    rv_bag = skBagIteratorCreate(bag, &iter);
    if (SKBAG_OK != rv_bag) {
        iter = NULL;
        goto END;
    }

    while (skBagIteratorNextTyped(iter, &key, &counter) == SKBAG_OK) {
        if (mask_set) {
            if (skIPSetCheckAddress(mask_set, &key.val.addr)) {
                /* address is in set; if the --complement was
                 * requested, we should fail this key. */
                fail = app_flags.complement_set;
            } else {
                fail = !app_flags.complement_set;
            }
        }

        if (fail
            || (have_minkey && skipaddrCompare(&key.val.addr, &minkey) < 0)
            || (have_maxkey && skipaddrCompare(&key.val.addr, &maxkey) > 0)
            || (counter.val.u64 < mincounter)
            || (counter.val.u64 > maxcounter))
        {
            /* if we're here, we DO NOT want the record */
            fail = 0;
            rv_bag = skBagKeyRemove(bag, &key);
            if (SKBAG_OK != rv_bag) {
                ERR_REMOVE_KEY(key, rv_bag);
                goto END;
            }
        }
    }

  END:
    if (iter) {
        skBagIteratorDestroy(iter);
    }
    return ((SKBAG_OK == rv_bag) ? 0 : -1);
}


/*
 *  ok = bagtoolScalarMultiply();
 *
 *    Apply the scalar_multiply multiplier to every counter
 */
static int
bagtoolScalarMultiply(
    void)
{
    skBagIterator_t *iter = NULL;
    skBagTypedKey_t key;
    skBagTypedCounter_t counter;
    skBagTypedCounter_t overflow;
    skBagErr_t rv_bag;

    /* Determine the counter value that will cause an overflow */
    overflow.type = SKBAG_COUNTER_U64;
    overflow.val.u64 = SKBAG_COUNTER_MAX / scalar_multiply;

    /* Create an iterator to loop over the bag */
    rv_bag = skBagIteratorCreate(out_bag, &iter);
    if (SKBAG_OK != rv_bag) {
        iter = NULL;
        goto END;
    }

    /* set types for key and counter */
    key.type = SKBAG_KEY_ANY;
    counter.type = SKBAG_COUNTER_U64;

    /* modify the bag while we iterate over it.  should be safe since
     * we are not adding any new keys. */
    while (skBagIteratorNextTyped(iter, &key, &counter) == SKBAG_OK) {
        if (counter.val.u64 > overflow.val.u64) {
            skAppPrintErr("Overflow when applying scalar multiplier");
        }
        counter.val.u64 *= scalar_multiply;
        rv_bag = skBagCounterSet(out_bag, &key, &counter);
        if (rv_bag) {
            ERR_SET_COUNT(key, counter, rv_bag);
            goto END;
        }
    }

  END:
    if (iter) {
        skBagIteratorDestroy(iter);
    }
    return ((SKBAG_OK == rv_bag) ? 0 : -1);
}


/*
 *  ok = writeOutput();
 *
 *    Generate the output.
 */
static int
writeOutput(
    void)
{
    skBagErr_t rv;

    /* Remove anything that's not in range or not in the intersecting
     * set (or complement) as appropriate */
    applyCutoffs(out_bag);

    if (app_flags.invert) {
        bagtoolInvert(out_bag);
    }

    if (app_flags.coverset) {
        return bagtoolCoverSet(out_bag, out_stream);
    }

    rv = skBagWrite(out_bag, out_stream);
    if (SKBAG_OK != rv) {
        if (SKBAG_ERR_OUTPUT == rv) {
            skStreamPrintLastErr(out_stream,
                                 skStreamGetLastReturnValue(out_stream),
                                 &skAppPrintErr);
        } else {
            skAppPrintErr("Error writing bag to output file '%s': %s",
                          skStreamGetPathname(out_stream), skBagStrerror(rv));
        }
        return -1;
    }

    return 0;
}


/*
 *  ok = appNextInput(argc, argv, &stream);
 *
 *    Fill 'stream' with the opened skStream object to next input
 *    file from the command line or the standard input if no files
 *    were given on the command line.
 *
 *    Return 1 if input is available, 0 if all input files have been
 *    processed, and -1 to indicate an error opening a file.
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

    /* copy notes (annotations) from the input files to the output file */
    if (!note_strip) {
        rv = skHeaderCopyEntries(skStreamGetSilkHeader(out_stream), hdr,
                                 SK_HENTRY_ANNOTATION_ID);
        if (rv) {
            skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
            skStreamDestroy(&(*stream));
            return -1;
        }
    }

    return 1;
}


int main(int argc, char **argv)
{
    skstream_t *in_stream;
    skBagErr_t rv;

    appSetup(argc, argv);                       /* never returns on error */

    /* Read the first bag; this will be the basis of the output bag. */
    if (appNextInput(argc, argv, &in_stream) != 1) {
        return 1;
    }
    rv = skBagRead(&out_bag, in_stream);
    if (SKBAG_OK != rv) {
        ERR_READ_BAG(in_stream, rv);
        skStreamDestroy(&in_stream);
        return 1;
    }
    skStreamDestroy(&in_stream);

    /* Open up each remaining bag and process it appropriately */
    while (1 == appNextInput(argc, argv, &in_stream)) {
        switch (user_action) {
          case OPT_ADD:
            rv = skBagAddFromStream(out_bag, in_stream);
            if (SKBAG_OK != rv) {
                skAppPrintErr("Error when adding bags: %s",
                              skBagStrerror(rv));
                skStreamDestroy(&in_stream);
                return 1;
            }
            break;

          case OPT_SUBTRACT:
            if (bagtoolSubtract(in_stream)) {
                skStreamDestroy(&in_stream);
                return 1;
            }
            break;

          case OPT_MINIMIZE:
            if (bagtoolMinimize(in_stream)) {
                skStreamDestroy(&in_stream);
                return 1;
            }
            break;

          case OPT_MAXIMIZE:
            if (bagtoolMaximize(in_stream)) {
                skStreamDestroy(&in_stream);
                return 1;
            }
            break;

          case OPT_DIVIDE:
            if (bagtoolDivide(in_stream)) {
                skStreamDestroy(&in_stream);
                return 1;
            }
            break;

          case OPT_COMPARE:
            if (bagtoolCompare(in_stream)) {
                skStreamDestroy(&in_stream);
                return 1;
            }
            break;

          case OPT_SCALAR_MULTIPLY:
            /* should never get here, since we only take one file and
             * we tested for this above */
            skAppPrintErr("Only one file allowed for scalar multiply");
            skAbortBadCase(user_action);

          default:
            /* Processing options not handled in this switch require a
             * single bag file */
            skAbortBadCase(user_action);
        }

        skStreamDestroy(&in_stream);
    }

    if (OPT_SCALAR_MULTIPLY == user_action) {
        if (bagtoolScalarMultiply()) {
            return 1;
        }
    }

    /* Write the output */
    if (writeOutput()) {
        return 1;
    }

    /* done */
    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
