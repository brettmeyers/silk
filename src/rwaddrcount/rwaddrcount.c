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
 * addrcount.c
 *
 * this is the last major rwset tool I want to write, it takes in two streams
 * of data, an rwset and an rwfilter stream.  From this data, it then generates
 * a result - one of three outputs:
 *            totals (default) - outputs to screen a table containing the
 *                               ip address, bytes, packets, records
 *            print-ips        - outputs to screen the ip addresses
 *            set-file         - outputs to screen the set data.
 *
 * So the reason for the second two is because I'm including three thresholds
 * here - bytes, packets & records.
 *
 * 12/2 notes.  Often, the best is the enemy of the good, I am implementing
 * a simple version of this application for now with the long-term plan being
 * that I am going to write a better, faster, set-friendly version later.
 */

#include <silk/silk.h>

RCSIDENT("$SiLK: rwaddrcount.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/iptree.h>
#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/skstringmap.h>
#include <silk/skvector.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write output from --help */
#define USAGE_FH stdout

/* number of buckets in hash table */
#define RWAC_ARRAYSIZE   50865917

/* number of countRecord_t to allocate at one time */
#define RWAC_BLOCK_SIZE      4096

/*
 * Get the IP address from the record 'r' to use as the key.  Uses the
 * global variable 'use_dest'
 */
#define GETIP(r)                                                \
    ((use_dest) ? rwRecGetDIPv4(r) : rwRecGetSIPv4(r))

/*
 * One of the problems I've had recently is a tendency to focus on
 * doing the perfect hash.  The perfect hash is nice, but what we need
 * RIGHT HERE RIGHT NOW is a tool that'll actually do the job.  Enough
 * mathematical wanking.
 *
 * So, this is a hash whose collision compensation algorithm is linear
 * chaining.  I'm not happy about it, but it'll do for now.
 */
#define HASHFUNC(value)                                         \
    ((value ^ (value >> 7) ^ (value << 23)) % RWAC_ARRAYSIZE)

/*
 * CMPFNC: return TRUE if IP on the rwRec 'r' matches the IP stored in
 * the countRecord_t 'b'.
 */
#define CMPFNC(r, cr)                           \
    ((cr)->cr_key == GETIP(r))

/*
 * when generating output, this macro will evaluate to TRUE if the
 * record is within the limits given by the user and should be
 * printed/counted/used-to-genrate-output.  This macro uses the global
 * limit variables.
 */
#define IS_RECORD_WITHIN_LIMITS(count_rec)      \
    ((count_rec)->cr_bytes   >= min_bytes &&    \
     (count_rec)->cr_packets >= min_packets &&  \
     (count_rec)->cr_records >= min_records &&  \
     (count_rec)->cr_bytes   <= max_bytes &&    \
     (count_rec)->cr_packets <= max_packets &&  \
     (count_rec)->cr_records <= max_records)


/* formats for printing records */
#define FMT_REC_VALUE                                                   \
    "%*s%c%*" PRIu64 "%c%*" PRIu32 "%c%*" PRIu32 "%c%*s%c%*s%s\n"
#define FMT_REC_TITLE  "%*s%c%*s%c%*s%c%*s%c%*s%c%*s%s\n"
#define FMT_REC_WIDTH  {15, 20, 10, 10, 20, 20}

/* formats for printing statistics */
#define FMT_STAT_VALUE                                                  \
    "%*s%c%*" PRIu32 "%c%*" PRIu64 "%c%*" PRIu64 "%c%*" PRIu64 "%s\n"
#define FMT_STAT_TITLE "%*s%c%*s%c%*s%c%*s%c%*s%s\n"
#define FMT_STAT_WIDTH {10, 10, 20, 15, 15}

/* default time_flags */
#define TIME_FLAGS_DEFAULT  SKTIMESTAMP_NOMSEC

typedef struct countRecord_st countRecord_t;
struct countRecord_st {
    /* total number of bytes */
    uint64_t        cr_bytes;
    /* Pointer to the next record for collision */
    countRecord_t  *cr_next;
    /* IP address, or whatever we don't consider source or dest here*/
    uint32_t        cr_key;
    /* total number of packets */
    uint32_t        cr_packets;
    /* total number of records */
    uint32_t        cr_records;
    /* start time - epoch*/
    uint32_t        cr_start;
    /* total time lasted */
    uint32_t        cr_end;
};

typedef enum {
    RWAC_PMODE_NONE=0,
    RWAC_PMODE_IPS,
    RWAC_PMODE_RECORDS,
    RWAC_PMODE_STAT,
    RWAC_PMODE_IPSETFILE,
    RWAC_PMODE_SORTED_RECORDS,
    RWAC_PMODE_SORTED_IPS
} rwac_print_mode_t;


/* LOCAL VARIABLES */

/* for looping over files on the command line */
static sk_options_ctx_t *optctx;

/* output mode */
static rwac_print_mode_t print_mode = RWAC_PMODE_NONE;

/* user-specified limits of bins to print */
static uint64_t min_bytes = 0;
static uint64_t max_bytes = UINT64_MAX;
static uint32_t min_packets = 0;
static uint32_t max_packets = UINT32_MAX;
static uint32_t min_records = 0;
static uint32_t max_records = UINT32_MAX;

/* the hash table */
static countRecord_t *hash_bins[RWAC_ARRAYSIZE];

/* IPset file for output when --set-file is specified */
static const char *ipset_file = NULL;

/* entries are allocated in blocks; the blocks are stored in a
 * vector */
static sk_vector_t *mem_vector = NULL;

/* whether to use the source(==0) or destination(==1) IPs */
static uint8_t use_dest = 0;

/* output mode for IPs */
static uint32_t ip_format = SKIPADDR_CANONICAL;
static uint8_t sort_ips_flag = 0;

/* whether to suppress column titles; default no (i.e. print titles) */
static uint8_t no_titles = 0;

/* whether to suppress columnar output; default no (i.e. columnar) */
static uint8_t no_columns = 0;

/* whether to suppress the final delimiter; default no (i.e. end with '|') */
static uint8_t no_final_delimiter = 0;

/* column separator */
static char delimiter = '|';

/* what to print at the end of the line */
static char final_delim[] = {'\0', '\0'};

/* flags to pass to sktimestamp_r() */
static uint32_t time_flags = TIME_FLAGS_DEFAULT;

/* where to write output */
static sk_fileptr_t output;

/* name of program to run to page output */
static char *pager = NULL;

/* timestamp formats: the first of these will be the default */
static const sk_stringmap_entry_t timestamp_names[] = {
    {"default", 0,                    NULL, "yyyy/mm/ddThh:mm:ss"},
    {"iso",     SKTIMESTAMP_ISO,      NULL, "yyyy-mm-dd hh:mm:ss"},
    {"m/d/y",   SKTIMESTAMP_MMDDYYYY, NULL, "mm/dd/yyyy hh:mm:ss"},
    {"epoch",   SKTIMESTAMP_EPOCH,    NULL,
     "seconds since UNIX epoch; ignores timezone"},
    SK_STRINGMAP_SENTINEL
};
static const sk_stringmap_entry_t timestamp_zones[] = {
    {"utc",     SKTIMESTAMP_UTC,      NULL, "use UTC"},
    {"local",   SKTIMESTAMP_LOCAL,    NULL,
     "use TZ environment variable or local timezone"},
    SK_STRINGMAP_SENTINEL
};
#if 0
static const sk_stringmap_entry_t timestamp_misc[] = {
    {"no-msec", SKTIMESTAMP_NOMSEC,   NULL, "truncate milliseconds"},
    SK_STRINGMAP_SENTINEL
};
#endif


/* OPTIONS */

/* Names of options; keep the order in sync with appOptions[] */
typedef enum {
    OPT_PRINT_RECORDS,
    OPT_PRINT_STAT,
    OPT_PRINT_IPS,
    OPT_USE_DEST,
    OPT_MIN_BYTES,
    OPT_MIN_PACKETS,
    OPT_MIN_RECORDS,
    OPT_MAX_BYTES,
    OPT_MAX_PACKETS,
    OPT_MAX_RECORDS,
    OPT_SET_FILE,
    OPT_TIMESTAMP_FORMAT,
    OPT_SORT_IPS,
    OPT_NO_TITLES,
    OPT_NO_COLUMNS,
    OPT_COLUMN_SEPARATOR,
    OPT_NO_FINAL_DELIMITER,
    OPT_DELIMITED,
    OPT_OUTPUT_PATH,
    OPT_PAGER,
    OPT_LEGACY_TIMESTAMPS
} appOptionsEnum;

static struct option appOptions[] = {
    {"print-recs",          NO_ARG,       0, OPT_PRINT_RECORDS},
    {"print-stat",          NO_ARG,       0, OPT_PRINT_STAT},
    {"print-ips",           NO_ARG,       0, OPT_PRINT_IPS},
    {"use-dest",            NO_ARG,       0, OPT_USE_DEST},
    {"min-bytes",           REQUIRED_ARG, 0, OPT_MIN_BYTES},
    {"min-packets",         REQUIRED_ARG, 0, OPT_MIN_PACKETS},
    {"min-records",         REQUIRED_ARG, 0, OPT_MIN_RECORDS},
    {"max-bytes",           REQUIRED_ARG, 0, OPT_MAX_BYTES},
    {"max-packets",         REQUIRED_ARG, 0, OPT_MAX_PACKETS},
    {"max-records",         REQUIRED_ARG, 0, OPT_MAX_RECORDS},
    {"set-file",            REQUIRED_ARG, 0, OPT_SET_FILE},
    {"timestamp-format",    REQUIRED_ARG, 0, OPT_TIMESTAMP_FORMAT},
    {"sort-ips",            NO_ARG,       0, OPT_SORT_IPS},
    {"no-titles",           NO_ARG,       0, OPT_NO_TITLES},
    {"no-columns",          NO_ARG,       0, OPT_NO_COLUMNS},
    {"column-separator",    REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR},
    {"no-final-delimiter",  NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"delimited",           OPTIONAL_ARG, 0, OPT_DELIMITED},
    {"output-path",         REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {"pager",               REQUIRED_ARG, 0, OPT_PAGER},
    {"legacy-timestamps",   OPTIONAL_ARG, 0, OPT_LEGACY_TIMESTAMPS},
    {0,0,0,0}               /* sentinel entry */
};

static const char *appHelp[] = {
    "Print summary byte, packet, flow counts per IP bin",
    "Print statistics (total bytes, packets, flows, unique IPs)",
    "Print IP addresses only to stdout",
    "Use destination IP address as key. Def. Source address",
    ("Do not print IPs when sum has less than this many total\n"
     "\tbytes. Def. 1"),
    ("Do not print IPs when sum has less than this many total\n"
     "\tpackets. Def. 1"),
    ("Do not print IPs when sum has less than this many total\n"
     "\trecords. Def. 1"),
    ("Do not print IPs when sum has more than this many total\n"
     "\tbytes. Def. 18446744073709551615"),
    ("Do not print IPs when sum has more than this many total\n"
     "\tpackets. Def. 4294967295"),
    ("Do not print IPs when sum has more than this many total\n"
     "\trecords. Def. 4294967295"),
    "Write IPs to specified binary IPset file. Def. No",
    NULL, /* generated dynamically */
    "When printing results, sort by IP address. Def. No",
    "Do not print column titles. Def. Print titles",
    "Disable fixed-width columnar output. Def. Columnar",
    "Use specified character between columns. Def. '|'",
    "Suppress column delimiter at end of line. Def. No",
    "Shortcut for --no-columns --no-final-del --column-sep=CHAR",
    "Send output to given file path. Def. stdout",
    "Program to invoke to page output. Def. $SILK_PAGER or $PAGER",
    "DEPRECATED. Equivalent to --timestamp-format=m/d/y,no-msec",
    (char *)NULL
};

static struct option legacyOptions[] = {
    {"byte-min",            REQUIRED_ARG, 0, OPT_MIN_BYTES},
    {"packet-min",          REQUIRED_ARG, 0, OPT_MIN_PACKETS},
    {"rec-min",             REQUIRED_ARG, 0, OPT_MIN_RECORDS},
    {"byte-max",            REQUIRED_ARG, 0, OPT_MAX_BYTES},
    {"packet-max",          REQUIRED_ARG, 0, OPT_MAX_PACKETS},
    {"rec-max",             REQUIRED_ARG, 0, OPT_MAX_RECORDS},
    {0,0,0,0}               /* sentinel entry */
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  timestampFormatParse(const char *format, uint32_t *out_flags);
static void timestampFormatUsage(FILE *fh);


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
    ("{--print-recs|--print-stat|--print-ips} [SWITCHES] [FILES]\n"          \
     "\tSummarize SiLK Flow records by source or destination IP; with\n"     \
     "\tthe --print-recs option will produce textual output with counts of\n" \
     "\tbytes, packets, and flow records for each IP, and the time range\n"  \
     "\twhen the IP was active.  When no files are given on command line,\n" \
     "\tflows are read from STDIN.\n")

    FILE *fh = USAGE_FH;
    int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);

    for (i = 0; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch (appOptions[i].val) {
          case OPT_TIMESTAMP_FORMAT:
            timestampFormatUsage(fh);
            skOptionsIPFormatUsage(fh);
            break;
          default:
            /* Simple static help text from the appHelp array */
            fprintf(fh, "%s\n", appHelp[i]);
            break;
        }
    }
    skOptionsCtxOptionsUsage(optctx, fh);
    sksiteOptionsUsage(fh);

    fprintf(fh, "\nDEPRECATED SWITCHES:\n");
    for (i = 0; legacyOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. Deprecated alias for --%s\n",
                legacyOptions[i].name, SK_OPTION_HAS_ARG(legacyOptions[i]),
                appOptions[legacyOptions[i].val].name);
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
/* #define RWAC_MAX_CHAIN  64 */

    static int teardownFlag = 0;
    countRecord_t *bin;
    uint32_t i;
#ifdef RWAC_MAX_CHAIN
    uint32_t chained[RWAC_MAX_CHAIN];
    uint32_t chain_len;
#endif

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* close the output file or process */
    if (output.of_name) {
        skFileptrClose(&output, &skAppPrintErr);
    }

    /* close the copy-stream */
    skOptionsCtxCopyStreamClose(optctx, &skAppPrintErr);

#ifdef RWAC_MAX_CHAIN
    /* print chain length of each bucket */
    memset(chained, 0, sizeof(chained));
    chain_len = 0;

    for (i = 0; i < RWAC_ARRAYSIZE; ++i) {
        if (NULL != hash_bins[i]) {
            bin = hash_bins[i];
            chain_len = 0;
            do {
                ++chain_len;
                bin = bin->cr_next;
            } while (bin != hash_bins[i]);
            if (chain_len < RWAC_MAX_CHAIN) {
                ++chained[chain_len];
            } else {
                ++chained[RWAC_MAX_CHAIN-1];
            }
        }
    }

    if (chain_len) {
        fprintf(stderr, "Hash Chaining Information\n%10s|%10s|\n",
                "Length", "Count");
    }
    for (i = 0; i < RWAC_MAX_CHAIN; ++i) {
        if (chained[i]) {
            fprintf(stderr, ("%10" PRIu32 "|%10" PRIu32 "|\n"),
                    i, chained[i]);
        }
    }
#endif  /* RWAC_MAX_CHAIN */

    if (mem_vector) {
        for (i = 0; 0 == skVectorGetValue(&bin, mem_vector, i); ++i) {
            free(bin);
        }
        skVectorDestroy(mem_vector);
    }

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
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    use_dest = 0;
    memset(&output, 0, sizeof(output));
    output.of_fp = stdout;

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS | SK_OPTIONS_CTX_PRINT_FILENAMES
                    | SK_OPTIONS_CTX_COPY_INPUT);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsRegister(legacyOptions, &appOptionsHandler, NULL)
        || skOptionsIPFormatRegister(&ip_format)
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
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();/* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* handle the final delimiter */
    if (!no_final_delimiter) {
        final_delim[0] = delimiter;
    }

    if (print_mode == RWAC_PMODE_NONE) {
        skAppPrintErr("Must specify --%s, --%s, --%s, or --%s",
                      appOptions[OPT_PRINT_RECORDS].name,
                      appOptions[OPT_PRINT_STAT].name,
                      appOptions[OPT_PRINT_IPS].name,
                      appOptions[OPT_SET_FILE].name);
        skAppUsage();
    }

    /* verify that the bounds make sense */
    if (min_bytes > max_bytes) {
        skAppPrintErr(("The %s value is greater than %s:"
                       " %" PRIu64 " > %" PRIu64),
                      appOptions[OPT_MIN_BYTES].name,
                      appOptions[OPT_MAX_BYTES].name,
                      min_bytes, max_bytes);
        exit(EXIT_FAILURE);
    }
    if (min_packets > max_packets) {
        skAppPrintErr(("The %s value is greater than %s:"
                       " %" PRIu32 " > %" PRIu32),
                      appOptions[OPT_MIN_PACKETS].name,
                      appOptions[OPT_MAX_PACKETS].name,
                      min_packets, max_packets);
        exit(EXIT_FAILURE);
    }
    if (min_records > max_records) {
        skAppPrintErr(("The %s value is greater than %s:"
                       " %" PRIu32 " > %" PRIu32),
                      appOptions[OPT_MIN_RECORDS].name,
                      appOptions[OPT_MAX_RECORDS].name,
                      min_records, max_records);
        exit(EXIT_FAILURE);
    }

    /* Do they want the IPs in sorted order? */
    if (sort_ips_flag) {
        switch (print_mode) {
          case RWAC_PMODE_IPS:
            print_mode = RWAC_PMODE_SORTED_IPS;
            break;
          case RWAC_PMODE_RECORDS:
            print_mode = RWAC_PMODE_SORTED_RECORDS;
            break;
          default:
            /* skAppPrintErr("--sort-ips switch ignored"); */
            break;
        }
    }

    /* make certain stdout is not being used for multiple outputs */
    if (skOptionsCtxCopyStreamIsStdout(optctx)) {
        if ((NULL == output.of_name)
            || (0 == strcmp(output.of_name, "-"))
            || (0 == strcmp(output.of_name, "stdout")))
        {
            skAppPrintErr("May not use stdout for multiple output streams");
            exit(EXIT_FAILURE);
        }
    }

    /* create vector */
    mem_vector = skVectorNew(sizeof(countRecord_t*));
    if (NULL == mem_vector) {
        skAppPrintErr("Cannot create vector");
        exit(EXIT_FAILURE);
    }

    /* open the --output-path.  the 'of_name' member is NULL if user
     * didn't get an output-path. */
    if (output.of_name) {
        rv = skFileptrOpen(&output, SK_IO_WRITE);
        if (rv) {
            skAppPrintErr("Cannot open '%s': %s",
                          output.of_name, skFileptrStrerror(rv));
            exit(EXIT_FAILURE);
        }
    }

    /* looks good, open the --copy-input destination */
    if (skOptionsCtxOpenStreams(optctx, &skAppPrintErr)) {
        exit(EXIT_FAILURE);
    }

    return;                       /* OK */
}


/*
 *  status = appOptionsHandler(cData, opt_index, opt_arg);
 *
 *    Called by skOptionsParse(), this handles a user-specified switch
 *    that the application has registered, typically by setting global
 *    variables.  Returns 1 if the switch processing failed or 0 if it
 *    succeeded.  Returning a non-zero from from the handler causes
 *    skOptionsParse() to return a negative value.
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
      case OPT_USE_DEST:
        use_dest = 1;
        break;

      case OPT_MIN_BYTES:
        rv = skStringParseUint64(&min_bytes, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MAX_BYTES:
        rv = skStringParseUint64(&max_bytes, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MIN_PACKETS:
        rv = skStringParseUint32(&min_packets, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MAX_PACKETS:
        rv = skStringParseUint32(&max_packets, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MIN_RECORDS:
        rv = skStringParseUint32(&min_records, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_MAX_RECORDS:
        rv = skStringParseUint32(&max_records, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_PRINT_STAT:
        print_mode = RWAC_PMODE_STAT;
        break;

      case OPT_PRINT_IPS:
        print_mode = RWAC_PMODE_IPS;
        break;

      case OPT_PRINT_RECORDS:
        print_mode = RWAC_PMODE_RECORDS;
        break;

      case OPT_SET_FILE:
        print_mode = RWAC_PMODE_IPSETFILE;
        ipset_file = opt_arg;
        break;

      case OPT_TIMESTAMP_FORMAT:
        if (timestampFormatParse(opt_arg, &time_flags)) {
            return 1;
        }
        break;

      case OPT_SORT_IPS:
        sort_ips_flag = 1;
        break;

      case OPT_NO_TITLES:
        no_titles = 1;
        break;

      case OPT_NO_COLUMNS:
        no_columns = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        delimiter = opt_arg[0];
        break;

      case OPT_NO_FINAL_DELIMITER:
        no_final_delimiter = 1;
        break;

      case OPT_DELIMITED:
        no_columns = 1;
        no_final_delimiter = 1;
        if (opt_arg) {
            delimiter = opt_arg[0];
        }
        break;

      case OPT_OUTPUT_PATH:
        if (output.of_name) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        output.of_name = opt_arg;
        break;

      case OPT_PAGER:
        pager = opt_arg;
        break;

      case OPT_LEGACY_TIMESTAMPS:
        if ((opt_arg == NULL) || (opt_arg[0] == '\0') || (opt_arg[0] == '1')) {
            rv = timestampFormatParse("m/d/y", &time_flags);
            assert(0 == rv);
        } else {
            rv = timestampFormatParse(timestamp_names[0].name, &time_flags);
            assert(0 == rv);
        }
        break;
    }

    return 0;                     /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  appOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;
}


/*
 *  status = timestampFormatParse(format_string, out_flags);
 *
 *    Parse the comma-separated list of timestamp format strings
 *    contained in 'format_string' and set 'out_flags' to the result
 *    of parsing the string.  Return 0 on success, or -1 if parsing of
 *    the values fails or conflicting values are given.
 */
static int
timestampFormatParse(
    const char         *format,
    uint32_t           *out_flags)
{
    char buf[256];
    char *errmsg;
    sk_stringmap_t *str_map = NULL;
    sk_stringmap_iter_t *iter = NULL;
    sk_stringmap_entry_t *found_entry;
    const sk_stringmap_entry_t *entry;
    int name_seen = 0;
    int zone_seen = 0;
    int rv = -1;

    /* create a stringmap of the available timestamp formats */
    if (SKSTRINGMAP_OK != skStringMapCreate(&str_map)) {
        skAppPrintOutOfMemory(NULL);
        goto END;
    }
    if (skStringMapAddEntries(str_map, -1, timestamp_names) != SKSTRINGMAP_OK){
        skAppPrintOutOfMemory(NULL);
        goto END;
    }
    if (skStringMapAddEntries(str_map, -1, timestamp_zones) != SKSTRINGMAP_OK){
        skAppPrintOutOfMemory(NULL);
        goto END;
    }

    /* attempt to match */
    if (skStringMapParse(str_map, format, SKSTRINGMAP_DUPES_ERROR,
                         &iter, &errmsg))
    {
        skAppPrintErr("Invalid %s: %s",
                      appOptions[OPT_TIMESTAMP_FORMAT].name, errmsg);
        goto END;
    }

    *out_flags = TIME_FLAGS_DEFAULT;

    while (skStringMapIterNext(iter, &found_entry, NULL) == SK_ITERATOR_OK) {
        *out_flags |= found_entry->id;
        switch (found_entry->id) {
#if 0
          case SKTIMESTAMP_NOMSEC:
            break;
#endif
          case 0:
          case SKTIMESTAMP_EPOCH:
          case SKTIMESTAMP_ISO:
          case SKTIMESTAMP_MMDDYYYY:
            if (name_seen) {
                entry = timestamp_names;
                strncpy(buf, entry->name, sizeof(buf));
                for (++entry; entry->name; ++entry) {
                    strncat(buf, ",", sizeof(buf)-strlen(buf)-1);
                    strncat(buf, entry->name, sizeof(buf)-strlen(buf)-1);
                }
                skAppPrintErr("Invalid %s: May only specify one of %s",
                              appOptions[OPT_TIMESTAMP_FORMAT].name, buf);
                goto END;
            }
            name_seen = 1;
            break;

          case SKTIMESTAMP_UTC:
          case SKTIMESTAMP_LOCAL:
            if (zone_seen) {
                entry = timestamp_zones;
                strncpy(buf, entry->name, sizeof(buf));
                for (++entry; entry->name; ++entry) {
                    strncat(buf, ",", sizeof(buf)-strlen(buf)-1);
                    strncat(buf, entry->name, sizeof(buf)-strlen(buf)-1);
                }
                skAppPrintErr("Invalid %s: May only specify one of %s",
                              appOptions[OPT_TIMESTAMP_FORMAT].name, buf);
                goto END;
            }
            zone_seen = 1;
            break;

          default:
            skAbortBadCase(found_entry->id);
        }
    }

    rv = 0;

  END:
    if (str_map) {
        skStringMapDestroy(str_map);
    }
    if (iter) {
        skStringMapIterDestroy(iter);
    }
    return rv;
}


/*
 *  timestampFormatUsage(fh);
 *
 *    Print the description of the argument to the --timestamp-format
 *    switch to the 'fh' file handle.
 */
static void
timestampFormatUsage(
    FILE               *fh)
{
    const sk_stringmap_entry_t *e;
    const char *label;

    fprintf(fh, "Print times in specified format: Def. %s,%s\n",
            timestamp_names[0].name,
            timestamp_zones[(SK_ENABLE_LOCALTIME != 0)].name);

    label = "Format:";
    for (e = timestamp_names; e->name; ++e) {
        fprintf(fh, "\t%-10s%-8s - %s\n",
                label, e->name, (const char*)e->userdata);
        label = "";
    }
    label = "Timezone:";
    for (e = timestamp_zones; e->name; ++e) {
        fprintf(fh, "\t%-10s%-8s - %s\n",
                label, e->name, (const char*)e->userdata);
        label = "";
    }
#if 0
    label = "Misc:";
    for (e = timestamp_misc; e->name; ++e) {
        fprintf(fh, "\t%-10s%-8s - %s\n",
                label, e->name, (const char*)e->userdata);
        label = "";
    }
#endif
}


/*
 * void addToBin(countRecord_t *bin, rwRec *rwrec)
 *
 * Adds the contents of a record to the values stored in a bin.
 */
static void
addToBin(
    countRecord_t      *bin,
    const rwRec        *rwrec)
{
    assert(bin->cr_key == GETIP(rwrec));
    bin->cr_bytes += rwRecGetBytes(rwrec);
    bin->cr_packets += rwRecGetPkts(rwrec);
    ++bin->cr_records;
    if (rwRecGetStartSeconds(rwrec) < bin->cr_start) {
        bin->cr_start = rwRecGetStartSeconds(rwrec);
    }
    if (bin->cr_end < rwRecGetEndSeconds(rwrec)) {
        bin->cr_end = rwRecGetEndSeconds(rwrec);
    }
}


/*
 * int newBin(rwRec *rwrec)
 *
 * Creates a new countRecord and initializes it with the values from
 * the record.
 */
static countRecord_t *
newBin(
    const rwRec        *rwrec)
{
    static countRecord_t *current_block = NULL;
    static uint32_t available = 0;
    countRecord_t *bin;

    if (available) {
        --available;
        bin = current_block;
        ++current_block;
    } else {
        current_block
            = (countRecord_t*)malloc(RWAC_BLOCK_SIZE * sizeof(countRecord_t));
        if (NULL == current_block) {
            skAppPrintErr("Error allocating memory for bin");
            exit(EXIT_FAILURE);
        }
        if (skVectorAppendValue(mem_vector, &current_block)) {
            skAppPrintErr("Error allocating memory for bin");
            exit(EXIT_FAILURE);
        }
        bin = current_block;
        ++current_block;
        available = RWAC_BLOCK_SIZE - 1;
    }

    bin->cr_bytes = rwRecGetBytes(rwrec);
    bin->cr_packets = rwRecGetPkts(rwrec);
    bin->cr_records = 1;
    bin->cr_key = GETIP(rwrec);
    bin->cr_start = rwRecGetStartSeconds(rwrec);
    bin->cr_end = rwRecGetEndSeconds(rwrec);
    bin->cr_next = NULL;

    return bin;
}


/*
 * SECTION: Dumping
 *
 * All the output routines are in this section of the text.
 */


/*
 *  hashToIPTree(&iptree);
 *
 *    Fills in the 'iptree' with all the IPs in global hash_bins array.
 */
static void
hashToIPTree(
    skIPTree_t        **ip_tree)
{
    countRecord_t *bin;
    uint32_t i;

    skIPTreeCreate(ip_tree);

    for (i = 0; i < RWAC_ARRAYSIZE; i++) {
        if (NULL != hash_bins[i]) {
            bin = hash_bins[i];
            do {
                if (IS_RECORD_WITHIN_LIMITS(bin)) {
                    skIPTreeAddAddress((*ip_tree), (bin->cr_key));
                }
                bin = bin->cr_next;
            } while (bin != hash_bins[i]);
        }
    }
}


/*
 *  int dumpRecords(outfp)
 *
 *    Dumps the addrcount contents as a record of bytes, packets,
 *    times &c to 'outfp'
 *
 *    This is the typical text output from addrcount.
 *
 */
static int
dumpRecords(
    FILE               *outfp)
{
    int w[] = FMT_REC_WIDTH;
    uint32_t i;
    countRecord_t *bin;
    char ip_st[SK_NUM2DOT_STRLEN];
    char start_st[SKTIMESTAMP_STRLEN];
    char end_st[SKTIMESTAMP_STRLEN];
    skipaddr_t ipaddr;

    if (no_columns) {
        memset(w, 0, sizeof(w));
    }

    if ( !no_titles) {
        fprintf(outfp, FMT_REC_TITLE,
                w[0], (use_dest ? "dIP" : "sIP"), delimiter,
                w[1], "Bytes",      delimiter,
                w[2], "Packets",    delimiter,
                w[3], "Records",    delimiter,
                w[4], "Start_Time", delimiter,
                w[5], "End_Time",   final_delim);
    }

    for (i = 0; i < RWAC_ARRAYSIZE; i++) {
        if (NULL != hash_bins[i]) {
            bin = hash_bins[i];
            do {
                if (IS_RECORD_WITHIN_LIMITS(bin)) {
                    skipaddrSetV4(&ipaddr, &bin->cr_key);
                    fprintf(outfp, FMT_REC_VALUE,
                            w[0], skipaddrString(ip_st, &ipaddr, ip_format),
                            delimiter,
                            w[1], bin->cr_bytes,   delimiter,
                            w[2], bin->cr_packets, delimiter,
                            w[3], bin->cr_records, delimiter,
                            w[4], sktimestamp_r(start_st,
                                                sktimeCreate(bin->cr_start, 0),
                                                time_flags),
                            delimiter,
                            w[5], sktimestamp_r(end_st,
                                                sktimeCreate(bin->cr_end, 0),
                                                time_flags),
                            final_delim);
                }
                bin = bin->cr_next;
            } while (bin != hash_bins[i]);
        }
    }
    return 0;
}


/*
 *  int dumpRecordsSorted(outfp)
 *
 *    Dumps the addrcount contents as a record of bytes, packets,
 *    times &c to 'outfp', sorted by the IP address.
 *
 */
static int
dumpRecordsSorted(
    FILE               *outfp)
{
    int w[] = FMT_REC_WIDTH;
    uint32_t ip, key;
    countRecord_t *bin;
    skIPTree_t *ipset;
    skIPTreeIterator_t iter;
    char ip_st[SK_NUM2DOT_STRLEN];
    char start_st[SKTIMESTAMP_STRLEN];
    char end_st[SKTIMESTAMP_STRLEN];
    skipaddr_t ipaddr;

    if (no_columns) {
        memset(w, 0, sizeof(w));
    }

    hashToIPTree(&ipset);
    if (skIPTreeIteratorBind(&iter, ipset)) {
        skAppPrintErr("Unable to bind IPTree iterator");
        skIPTreeDelete(&ipset);
        return 1;
    }

    if ( !no_titles) {
        fprintf(outfp, FMT_REC_TITLE,
                w[0], (use_dest ? "dIP" : "sIP"), delimiter,
                w[1], "Bytes",      delimiter,
                w[2], "Packets",    delimiter,
                w[3], "Records",    delimiter,
                w[4], "Start_Time", delimiter,
                w[5], "End_Time",   final_delim);
    }

    while (skIPTreeIteratorNext(&ip, &iter) == SK_ITERATOR_OK) {

        /* find the ip's entry in the hash table */
        key = HASHFUNC(ip);

        /* loop through the list of records at this hash table entry
         * until we find the one that has the IP we want */
        bin = hash_bins[key];
        while (ip != bin->cr_key) {
            /* not the ip we wanted; goto next */
            bin = bin->cr_next;
            /* if we've looped all the around, we missed the
             * IP we wanted, and things are horked */
            assert(bin != hash_bins[key]);
        }

        skipaddrSetV4(&ipaddr, &bin->cr_key);
        fprintf(outfp, FMT_REC_VALUE,
                w[0], skipaddrString(ip_st, &ipaddr, ip_format), delimiter,
                w[1], bin->cr_bytes,   delimiter,
                w[2], bin->cr_packets, delimiter,
                w[3], bin->cr_records, delimiter,
                w[4], sktimestamp_r(start_st,
                                    sktimeCreate(bin->cr_start, 0),
                                    time_flags),
                delimiter,
                w[5], sktimestamp_r(end_st,
                                    sktimeCreate(bin->cr_end, 0),
                                    time_flags),
                final_delim);
    }

    skIPTreeDelete(&ipset);
    return 0;
}


/*
 *  int dumpIPs(outfp)
 *
 *    writes IP addresses to the stream 'outfp' in hash-table order
 *    (unsorted).
 *
 *    Returns 0 on success.
 */
static int
dumpIPs(
    FILE               *outfp)
{
    uint32_t i;
    countRecord_t *bin;
    char ip_st[SK_NUM2DOT_STRLEN];
    skipaddr_t ipaddr;

    if ( !no_titles) {
        fprintf(outfp, "%15s\n", (use_dest ? "dIP" : "sIP"));
    }

    for (i = 0; i < RWAC_ARRAYSIZE; i++) {
        if (NULL != hash_bins[i]) {
            bin = hash_bins[i];
            do {
                if (IS_RECORD_WITHIN_LIMITS(bin)) {
                    skipaddrSetV4(&ipaddr, &bin->cr_key);
                    fprintf(outfp, "%15s\n",
                            skipaddrString(ip_st, &ipaddr, ip_format));
                }
                bin = bin->cr_next;
            } while (bin != hash_bins[i]);
        }
    }
    return 0;
}


/*
 *  int dumpIPsSorted(outfp)
 *
 *    Writes the IPs to the stream 'outfp' in sorted order.
 *
 */
static int
dumpIPsSorted(
    FILE               *outfp)
{
    uint32_t ip;
    skIPTree_t *ipset;
    skIPTreeIterator_t iter;
    char ip_st[SK_NUM2DOT_STRLEN];
    skipaddr_t ipaddr;

    hashToIPTree(&ipset);
    if (skIPTreeIteratorBind(&iter, ipset)) {
        skAppPrintErr("Unable to create IPTree iterator");
        skIPTreeDelete(&ipset);
        return 1;
    }

    if ( !no_titles) {
        fprintf(outfp, "%15s\n", (use_dest ? "dIP" : "sIP"));
    }
    while (skIPTreeIteratorNext(&ip, &iter) == SK_ITERATOR_OK) {
        skipaddrSetV4(&ipaddr, &ip);
        fprintf(outfp, "%15s\n", skipaddrString(ip_st, &ipaddr, ip_format));
    }

    skIPTreeDelete(&ipset);
    return 0;
}


/*
 *  int dumpStats(outfp)
 *
 *    dumps a text string describing rwaddrcont results to the stream
 *    'outfp'.
 */
static int
dumpStats(
    FILE               *outfp)
{
    int fmt_width[] = FMT_STAT_WIDTH;
    uint32_t i;
    uint32_t qual_ips;
    uint32_t tot_ips;
    uint64_t qual_bytes, qual_packets, qual_records;
    uint64_t tot_bytes,  tot_packets,  tot_records;
    countRecord_t *bin;

    qual_ips = 0;
    tot_ips = 0;
    qual_bytes = qual_packets = qual_records = 0;
    tot_bytes  = tot_packets  = tot_records  = 0;

    if (no_columns) {
        memset(fmt_width, 0, sizeof(fmt_width));
    }
    for (i = 0; i < RWAC_ARRAYSIZE; i++) {
        if (NULL != hash_bins[i]) {
            bin = hash_bins[i];
            do {
                ++tot_ips;
                tot_bytes   += bin->cr_bytes;
                tot_packets += bin->cr_packets;
                tot_records += bin->cr_records;

                if (IS_RECORD_WITHIN_LIMITS(bin)) {
                    ++qual_ips;
                    qual_bytes   += bin->cr_bytes;
                    qual_packets += bin->cr_packets;
                    qual_records += bin->cr_records;
                }
                bin = bin->cr_next;
            } while (bin != hash_bins[i]);
        }
    }

    /* title */
    if ( !no_titles) {
        fprintf(outfp, FMT_STAT_TITLE,
                fmt_width[0], "", delimiter,
                fmt_width[1], (use_dest ? "dIP_Uniq" : "sIP_Uniq"), delimiter,
                fmt_width[2], "Bytes",   delimiter,
                fmt_width[3], "Packets", delimiter,
                fmt_width[4], "Records", final_delim);
    }

    fprintf(outfp, FMT_STAT_VALUE,
            fmt_width[0], "Total",          delimiter,
            fmt_width[1], tot_ips,          delimiter,
            fmt_width[2], tot_bytes,        delimiter,
            fmt_width[3], tot_packets,      delimiter,
            fmt_width[4], tot_records,      final_delim);

    /* print qualifying records if limits were given */
    if (0 < min_bytes || max_bytes < UINT64_MAX
        || 0 < min_packets || max_packets < UINT32_MAX
        || 0 < min_records || max_records < UINT32_MAX)
    {
        fprintf(outfp, FMT_STAT_VALUE,
                fmt_width[0], "Qualifying", delimiter,
                fmt_width[1], qual_ips,     delimiter,
                fmt_width[2], qual_bytes,   delimiter,
                fmt_width[3], qual_packets, delimiter,
                fmt_width[4], qual_records, final_delim);
    }

    return 0;
}


/*
 *  int dumpIPSet (char *path)
 *
 *    Dumps the ip addresses counted during normal operation to disk
 *    in IPSet format.
 */
static int
dumpIPSet(
    const char         *path)
{
    skIPTree_t *ipset;

    /* Create the ip tree */
    hashToIPTree(&ipset);

    /*
     * Okay, now we write to disk.
     */
    if (skIPTreeSave(ipset, path)) {
        exit(EXIT_FAILURE);
    }

    skIPTreeDelete(&ipset);
    return 0;
}


/*
 *  void countFile(stream)
 *
 *    Read the flow records from stream and fill the hash table with
 *    countRecord_t's.
 */
static void
countFile(
    skstream_t         *rwIOS)
{
    uint32_t hash_idx;
    countRecord_t *bin;
    rwRec rwrec;
    int rv;

    /* Read records */
    while ((rv = skStreamReadRecord(rwIOS, &rwrec)) == SKSTREAM_OK) {
        hash_idx = HASHFUNC(GETIP(&rwrec));

        /*
         * standard hash foo - check to see if we've got a value.
         * If not, create and stuff.  If so, check to see if they
         * match - if so, stuff.  If not, move down until you do -
         * if you find nothing, stuff.
         */
        if (hash_bins[hash_idx] == NULL) {
            /* new bin */
            hash_bins[hash_idx] = newBin(&rwrec);
            hash_bins[hash_idx]->cr_next = hash_bins[hash_idx];
        } else {
            /* hash collision */
            bin = hash_bins[hash_idx];
            while ((bin->cr_next != hash_bins[hash_idx]) &&
                   !CMPFNC(&rwrec, bin))
            {
                bin = bin->cr_next;
            }
            /*
             * Alright, we've either hit the end of the linked
             * list or we've found the value (or both).  Check if
             * we found the value. */
            if (CMPFNC(&rwrec, bin)) {
                addToBin(bin, &rwrec);
            } else {
                assert(bin->cr_next == hash_bins[hash_idx]);
                bin->cr_next = newBin(&rwrec);
                bin = bin->cr_next;
                bin->cr_next = hash_bins[hash_idx]; /* Restore the loop */
            }
            hash_bins[hash_idx] = bin;
        }
    }
    if (rv != SKSTREAM_ERR_EOF) {
        skStreamPrintLastErr(rwIOS, rv, &skAppPrintErr);
    }
}


int main(int argc, char **argv)
{
    skstream_t *rwios;
    int rv;

    appSetup(argc, argv);                 /* never returns on error */

    /* Read in records from all input files */
    while ((rv = skOptionsCtxNextSilkFile(optctx, &rwios, &skAppPrintErr))
           == 0)
    {
        skStreamSetIPv6Policy(rwios, SK_IPV6POLICY_ASV4);
        countFile(rwios);
        skStreamDestroy(&rwios);
    }
    if (rv < 0) {
        exit(EXIT_FAILURE);
    }

    /* Invoke the pager when appropriate. */
    switch (print_mode) {
      case RWAC_PMODE_STAT:
      case RWAC_PMODE_IPSETFILE:
        break;
      case RWAC_PMODE_RECORDS:
      case RWAC_PMODE_IPS:
      case RWAC_PMODE_SORTED_RECORDS:
      case RWAC_PMODE_SORTED_IPS:
        if (NULL == output.of_name) {
            rv = skFileptrOpenPager(&output, pager);
            if (rv && rv != SK_FILEPTR_PAGER_IGNORED) {
                skAppPrintErr("Unable to invoke pager");
            }
        }
        break;
      case RWAC_PMODE_NONE:
        skAbortBadCase(print_mode);
    }

    /* Produce the output */
    switch (print_mode) {
      case RWAC_PMODE_STAT:
        dumpStats(output.of_fp);
        break;
      case RWAC_PMODE_IPSETFILE:
        dumpIPSet(ipset_file);
        break;
      case RWAC_PMODE_RECORDS:
        dumpRecords(output.of_fp);
        break;
      case RWAC_PMODE_IPS:
        dumpIPs(output.of_fp);
        break;
      case RWAC_PMODE_SORTED_RECORDS:
        dumpRecordsSorted(output.of_fp);
        break;
      case RWAC_PMODE_SORTED_IPS:
        dumpIPsSorted(output.of_fp);
        break;
      case RWAC_PMODE_NONE:
        skAbortBadCase(print_mode);
    }

    /* output */
    return (0);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
