/*
** Copyright (C) 2003-2014 by Carnegie Mellon University.
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
**  Quick application to print the IPs in a set file
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwsetcat.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skipaddr.h>
#include <silk/skipset.h>
#include <silk/skprintnets.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* Where to print usage (--help) */
#define USAGE_FH stdout


/* LOCAL VARIABLES */

/* where to send output */
static skstream_t *stream_out;

/* index of first option that is not handled by the options handler.
 * If this is equal to argc, then input is from stdin. */
static int arg_index = 0;

/* output delimiter for network structure */
static char output_delimiter = '|';

/* type of network structure to print */
static const char *net_structure = NULL;

/* paging program to use for output */
static const char *pager = NULL;

/* how to print IPs from silk_types.h: enum skipaddr_flags_t */
static uint32_t ip_format = SKIPADDR_CANONICAL;

/* option flags */
static struct opt_flags_st {
    /* whether to output network breakdown of set contents */
    unsigned    network_structure   :1;
    /* whether to print IP ranges as LOW|HIGH| */
    unsigned    ip_ranges           :1;
    /* whether the user specified the --cidr-block switch */
    unsigned    user_cidr           :1;
    /* whether user wants to print IPs as CIDR blocks */
    unsigned    cidr_blocks         :1;
    /* whether to surpress fixed-width columnar network structure output */
    unsigned    no_columns          :1;
    /* whether to suppress the final delimiter */
    unsigned    no_final_delimiter  :1;
    /* whether user explicitly gave the print-ips option */
    unsigned    print_ips           :1;
    /* whether to count IPs: default no */
    unsigned    count_ips           :1;
    /* whether to print statistics; default no */
    unsigned    statistics          :1;
    /* whether to print file names; default is normally no; however,
     * the default becomes yes when --count-ips or --print-statistics
     * is specified and there are multiple input files. */
    unsigned    print_filenames     :1;
    /* whether user provided --print-filenames */
    unsigned    print_filenames_user:1;
} opt_flags = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* OPTIONS SETUP */

typedef enum {
    OPT_COUNT_IPS,
    OPT_PRINT_STATISTICS,
    OPT_PRINT_IPS,
    OPT_NETWORK_STRUCTURE,
    OPT_CIDR_BLOCKS,
    OPT_IP_RANGES,
    OPT_NO_COLUMNS,
    OPT_COLUMN_SEPARATOR,
    OPT_NO_FINAL_DELIMITER,
    OPT_DELIMITED,
    OPT_PRINT_FILENAMES,
    OPT_PAGER
} appOptionsEnum;

static struct option appOptions[] = {
    {"count-ips",           NO_ARG,       0, OPT_COUNT_IPS},
    {"print-statistics",    NO_ARG,       0, OPT_PRINT_STATISTICS},
    {"print-ips",           NO_ARG,       0, OPT_PRINT_IPS},
    {"network-structure",   OPTIONAL_ARG, 0, OPT_NETWORK_STRUCTURE},
    {"cidr-blocks",         OPTIONAL_ARG, 0, OPT_CIDR_BLOCKS},
    {"ip-ranges",           NO_ARG,       0, OPT_IP_RANGES},
    {"no-columns",          NO_ARG,       0, OPT_NO_COLUMNS},
    {"column-separator",    REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR},
    {"no-final-delimiter",  NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"delimited",           OPTIONAL_ARG, 0, OPT_DELIMITED},
    {"print-filenames",     OPTIONAL_ARG, 0, OPT_PRINT_FILENAMES},
    {"pager",               REQUIRED_ARG, 0, OPT_PAGER},
    {0,0,0,0}               /* sentinel entry */
};


static const char *appHelp[] = {
    "Print the number of IP in each IPset listed on the command\n"
    "\tline; disables default printing of IPs. Def. No",
    "Print statistics about the IPset (min-/max-ip, etc);\n"
    "\tdisable default printing of IPs. Def. No",
    "Also print IPs when count or statistics switch is given",
    ("Print the number of hosts for each specified CIDR\n"
     "\tblock in the comma-separed list of CIDR block sizes (0--32) and/or\n"
     "\tletters (T=0,A=8,B=16,C=24,X=27,H=32). If argument contains 'S' or\n"
     "\t'/', for each CIDR block print host counts and number of occupied\n"
     "\tsmaller CIDR blocks. Additional CIDR blocks to summarize can be\n"
     "\tspecified by listing them after the '/'. Def. v4:TS/8,16,24,27.\n"
     "\tA leading 'v6:' treats IPset as being IPv6, allows range 0--128,\n"
     "\tdisallows A,B,C,X, sets H to 128, and sets default to TS/48,64"),
    ("Print IPs in CIDR block notation when no argument given\n"
     "\tor argument is 1; otherwise, print individual IPs.\n"
     "\tDef. Individual IPs for IPv4 IPsets, CIDR blocks for IPv6 IPsets"),
    "Print IPs as ranges of count|low|high|. Def. No",
    ("When printing network-structure or ip-ranges, disable\n"
     "\tfixed-width columnar output. Def. Columnar"),
    ("When printing network-structure or ip-ranges, use\n"
     "\tspecified character between columns. Def. '|'"),
    "Suppress column delimiter at end of line. Def. No",
    "Shortcut for --no-columns --no-final-del --column-sep=CHAR",
    ("Print the name of each filename. 0 = no; 1 = yes.\n"
     "\tDefault is no unless multiple input files are provided and output\n"
     "\tis --count-ips or --print-statistics"),
    "Program to invoke to page output. Def. $SILK_PAGER or $PAGER",
    (char *)NULL /* sentinel entry */
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);

static void setcatPrintNetwork(skstream_t *outstream,const skipset_t *ipset);
static int setcatProcessFile(skstream_t *outstream, const char *fileName);



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
    ("[SWITCHES] [IPSET_FILES]\n"                                             \
     "\tBy default, prints the IPs in the specified IPSET_FILES.  Use\n"      \
     "\tswitches to control format of the outout and to optionally or\n"      \
     "\tadditionally print the number of IPs in the file, the network\n"      \
     "\tstructure, or other statistics.  If no IPSET_FILEs are given on\n"    \
     "\tthe command line, the IPset will be read from the standard input.\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
    skOptionsIPFormatUsage(fh);
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

    skStreamDestroy(&stream_out);
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
    stream_out = NULL;

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsIPFormatRegister(&ip_format))
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
        skAppUsage();             /* never returns */
    }

    /* either need name of set file(s) after options or a set file on stdin */
    if ((arg_index == argc) && (FILEIsATty(stdin))) {
        skAppPrintErr("No files on the command line and"
                      " stdin is connected to a terminal");
        skAppUsage();
    }

    /* determine whether to print file names */
    if (!opt_flags.print_filenames_user
        && (argc - arg_index > 1)
        && (opt_flags.count_ips || opt_flags.statistics))
    {
        opt_flags.print_filenames = 1;
    }

    /* network structure output conflicts with most other output */
    if (opt_flags.network_structure) {
        if (opt_flags.user_cidr) {
            skAppPrintErr("Cannot combine the --%s and --%s switches.",
                          appOptions[OPT_NETWORK_STRUCTURE].name,
                          appOptions[OPT_CIDR_BLOCKS].name);
            skAppUsage();
        }
        if (opt_flags.print_ips) {
            skAppPrintErr("Cannot combine the --%s and --%s switches.",
                          appOptions[OPT_NETWORK_STRUCTURE].name,
                          appOptions[OPT_PRINT_IPS].name);
            skAppUsage();
        }
        if (opt_flags.count_ips) {
            skAppPrintErr("Cannot combine the --%s and --%s switches.",
                          appOptions[OPT_NETWORK_STRUCTURE].name,
                          appOptions[OPT_COUNT_IPS].name);
            skAppUsage();
        }
        if (opt_flags.ip_ranges) {
            skAppPrintErr("Cannot combine the --%s and --%s switches.",
                          appOptions[OPT_NETWORK_STRUCTURE].name,
                          appOptions[OPT_IP_RANGES].name);
            skAppUsage();
        }
    }

    if (opt_flags.ip_ranges) {
        if (opt_flags.user_cidr) {
            skAppPrintErr("Cannot combine the --%s and --%s switches.",
                          appOptions[OPT_IP_RANGES].name,
                          appOptions[OPT_CIDR_BLOCKS].name);
            skAppUsage();
        }
        opt_flags.print_ips = 0;
    }

    /* If no output was specified, print the ips */
    if (!opt_flags.statistics && !opt_flags.count_ips
        && !opt_flags.network_structure && !opt_flags.print_ips
        && !opt_flags.ip_ranges)
    {
        opt_flags.print_ips = 1;
    }

    /* Create the output stream */
    if ((rv = skStreamCreate(&stream_out, SK_IO_WRITE, SK_CONTENT_TEXT))
        || (rv = skStreamBind(stream_out, "stdout"))
        || (rv = skStreamPageOutput(stream_out, pager))
        || (rv = skStreamOpen(stream_out)))
    {
        skStreamPrintLastErr(stream_out, rv, &skAppPrintErr);
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
      case OPT_PRINT_STATISTICS:
        opt_flags.statistics = 1;
        break;

      case OPT_COUNT_IPS:
        opt_flags.count_ips = 1;
        break;

      case OPT_NETWORK_STRUCTURE:
        net_structure = opt_arg;
        opt_flags.network_structure = 1;
        break;

      case OPT_PRINT_IPS:
        opt_flags.print_ips = 1;
        break;

      case OPT_CIDR_BLOCKS:
        opt_flags.print_ips = 1;
        opt_flags.user_cidr = 1;
        if (NULL == opt_arg) {
            opt_flags.cidr_blocks = 1;
        } else if (0 == strcmp(opt_arg, "1")) {
            opt_flags.cidr_blocks = 1;
        } else if (0 != strcmp(opt_arg, "0")) {
            skAppPrintErr("Invalid %s: Value must be 0 or 1",
                          appOptions[opt_index].name);
            return -1;
        }
        break;

      case OPT_IP_RANGES:
        opt_flags.ip_ranges = 1;
        break;

      case OPT_NO_COLUMNS:
        opt_flags.no_columns = 1;
        break;

      case OPT_NO_FINAL_DELIMITER:
        opt_flags.no_final_delimiter = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        output_delimiter = opt_arg[0];
        break;

      case OPT_DELIMITED:
        opt_flags.no_columns = 1;
        opt_flags.no_final_delimiter = 1;
        if (opt_arg) {
            output_delimiter = opt_arg[0];
        }
        break;

      case OPT_PRINT_FILENAMES:
        opt_flags.print_filenames_user = 1;
        if (NULL == opt_arg) {
            opt_flags.print_filenames = 1;
        } else if (0 == strcmp(opt_arg, "1")) {
            opt_flags.print_filenames = 1;
        } else if (0 != strcmp(opt_arg, "0")) {
            skAppPrintErr("Invalid %s: Value must be 0 or 1",
                          appOptions[opt_index].name);
            return -1;
        }
        break;

      case OPT_PAGER:
        pager = opt_arg;
        break;
    }

    return 0;                   /* OK */
}


#if SK_ENABLE_IPV6
/*
 *  setcatPrintRangesV6(outstream, ipset);
 *
 *    Print IPset in three output columns; where the first is the
 *    number of IPs in the range, the second is the starting IP, the
 *    third is the ending IP.
 *
 *     COUNT| LOW| HIGH|
 */
static void
setcatPrintRangesV6(
    skstream_t         *outstream,
    const skipset_t    *ipset)
{
    skipset_iterator_t iter;
    skipaddr_t ipaddr;
    skipaddr_t contig;
    skipaddr_t start;
    skipaddr_t end;
    uint32_t prefix;
    char ip1[SK_NUM2DOT_STRLEN+1];
    char ip2[SK_NUM2DOT_STRLEN+1];
    int widths[3];
    char final_delim[] = {'\0', '\0'};
    double d_count;
    uint64_t big_count;
    uint64_t small_count;

    if (!opt_flags.no_final_delimiter) {
        final_delim[0] = output_delimiter;
    }

    if (opt_flags.no_columns) {
        memset(widths, 0, sizeof(widths));
    } else {
        if (skIPSetContainsV6(ipset)) {
            widths[0] = 39;
        } else {
            widths[0] = 10;
        }
        switch (ip_format) {
          case SKIPADDR_HEXADECIMAL:
            widths[1] = widths[2] = 32;
            break;
          case SKIPADDR_DECIMAL:
          case SKIPADDR_ZEROPAD:
          case SKIPADDR_CANONICAL:
          case SKIPADDR_FORCE_IPV6:
          default:
            widths[1] = widths[2] = 39;
            break;
        }
    }

    skIPSetIteratorBind(&iter, ipset, 1, SK_IPV6POLICY_FORCE);
    if (skIPSetIteratorNext(&iter, &ipaddr, &prefix) != SK_ITERATOR_OK) {
        /* empty set */
        return;
    }

    skCIDR2IPRange(&ipaddr, prefix, &start, &end);
    if (prefix <= 64) {
        big_count = (UINT64_C(1) << (64 - prefix));
        small_count = 0;
    } else {
        big_count = 0;
        small_count = (UINT64_C(1) << (128 - prefix));
    }

    while (skIPSetIteratorNext(&iter, &ipaddr, &prefix) == SK_ITERATOR_OK) {
        /* check whether this range is continuous with previous */
        skipaddrCopy(&contig, &end);
        skipaddrIncrement(&contig);
        if (0 == skipaddrCompare(&contig, &ipaddr)) {
            skCIDR2IPRange(&ipaddr, prefix, &ipaddr, &end);
            if (prefix <= 64) {
                big_count += (UINT64_C(1) << (64 - prefix));
            } else {
                uint64_t tmp = (UINT64_C(1) << (128 - prefix));
                if ((UINT64_MAX - tmp) > small_count) {
                    small_count += tmp;
                } else {
                    ++big_count;
                    small_count -= ((UINT64_MAX - tmp) + 1);
                }
            }
            continue;
        }

        /* print current range */
        if (0 == big_count) {
            skStreamPrint(outstream, ("%*" PRIu64 "%c%*s%c%*s%s\n"),
                          widths[0], small_count, output_delimiter,
                          widths[1], skipaddrString(ip1, &start, ip_format),
                          output_delimiter,
                          widths[2], skipaddrString(ip2, &end, ip_format),
                          final_delim);
        } else {
            d_count = ((double)big_count * ((double)UINT64_MAX + 1.0)
                       + (double)small_count);
            skStreamPrint(outstream, ("%*.0f%c%*s%c%*s%s\n"),
                          widths[0], d_count, output_delimiter,
                          widths[1], skipaddrString(ip1, &start, ip_format),
                          output_delimiter,
                          widths[2], skipaddrString(ip2, &end, ip_format),
                          final_delim);
        }

        /* begin a new range */
        skCIDR2IPRange(&ipaddr, prefix, &start, &end);
        if (prefix <= 64) {
            big_count = (UINT64_C(1) << (64 - prefix));
            small_count = 0;
        } else {
            big_count = 0;
            small_count = (UINT64_C(1) << (128 - prefix));
        }
    }

    /* print final range */
    if (0 == big_count) {
        skStreamPrint(outstream, ("%*" PRIu64 "%c%*s%c%*s%s\n"),
                      widths[0], small_count, output_delimiter,
                      widths[1], skipaddrString(ip1, &start, ip_format),
                      output_delimiter,
                      widths[2], skipaddrString(ip2, &end, ip_format),
                      final_delim);
    } else {
        d_count = ((double)big_count * ((double)UINT64_MAX + 1.0)
                   + (double)small_count);
        skStreamPrint(outstream, ("%*.0f%c%*s%c%*s%s\n"),
                      widths[0], d_count, output_delimiter,
                      widths[1], skipaddrString(ip1, &start, ip_format),
                      output_delimiter,
                      widths[2], skipaddrString(ip2, &end, ip_format),
                      final_delim);
        }
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  setcatPrintRangesV4(outstream, ipset);
 *
 *    Print IPset in three output columns; where the first is the
 *    number of IPs in the range, the second is the starting IP, the
 *    third is the ending IP.
 *
 *     COUNT| LOW| HIGH|
 */
static void
setcatPrintRangesV4(
    skstream_t         *outstream,
    const skipset_t    *ipset)
{
    skipset_iterator_t iter;
    skipaddr_t ipaddr;
    skipaddr_t contig;
    skipaddr_t start;
    skipaddr_t end;
    uint32_t prefix;
    char ip1[SK_NUM2DOT_STRLEN+1];
    char ip2[SK_NUM2DOT_STRLEN+1];
    int widths[3];
    char final_delim[] = {'\0', '\0'};
    uint64_t count;

    if (!opt_flags.no_final_delimiter) {
        final_delim[0] = output_delimiter;
    }

    if (opt_flags.no_columns) {
        memset(widths, 0, sizeof(widths));
    } else {
        widths[0] = 10;
        switch (ip_format) {
          case SKIPADDR_HEXADECIMAL:
            widths[1] = widths[2] = 8;
            break;
          case SKIPADDR_DECIMAL:
            widths[1] = widths[2] = 10;
            break;
          case SKIPADDR_ZEROPAD:
          case SKIPADDR_CANONICAL:
          case SKIPADDR_FORCE_IPV6:
          default:
            widths[1] = widths[2] = 15;
            break;
        }
    }

    skIPSetIteratorBind(&iter, ipset, 1, SK_IPV6POLICY_ASV4);
    if (skIPSetIteratorNext(&iter, &ipaddr, &prefix) != SK_ITERATOR_OK) {
        /* empty set */
        return;
    }

    skCIDR2IPRange(&ipaddr, prefix, &start, &end);
    count = (1 << (32 - prefix));

    while (skIPSetIteratorNext(&iter, &ipaddr, &prefix) == SK_ITERATOR_OK) {
        /* check whether this range is continuous with previous */
        skipaddrCopy(&contig, &end);
        skipaddrIncrement(&contig);
        if (0 == skipaddrCompare(&contig, &ipaddr)) {
            skCIDR2IPRange(&ipaddr, prefix, &ipaddr, &end);
            count += (1 << (32 - prefix));
            continue;
        }

        /* print current range */
        skStreamPrint(outstream, ("%*" PRIu64 "%c%*s%c%*s%s\n"),
                      widths[0], count, output_delimiter,
                      widths[1], skipaddrString(ip1, &start, ip_format),
                      output_delimiter,
                      widths[2], skipaddrString(ip2, &end, ip_format),
                      final_delim);

        skCIDR2IPRange(&ipaddr, prefix, &start, &end);
        count = (1 << (32 - prefix));
    }

    /* print final range */
    skStreamPrint(outstream, ("%*" PRIu64 "%c%*s%c%*s%s\n"),
                  widths[0], count, output_delimiter,
                  widths[1], skipaddrString(ip1, &start, ip_format),
                  output_delimiter,
                  widths[2], skipaddrString(ip2, &end, ip_format),
                  final_delim);
}


/*
 *  setcatPrintNetwork(outstream, ipset);
 *
 *    Print information about the strucuture of the IPset.
 */
static void
setcatPrintNetwork(
    skstream_t         *outstream,
    const skipset_t    *ipset)
{
    skipset_iterator_t iter;
    skipaddr_t ipaddr;
    uint32_t prefix;
    skNetStruct_t *ns = NULL;

    /* Set up the skNetStruct */
    if (skNetStructureCreate(&ns, 0 /*no counts*/)) {
        skAppPrintErr("Error creating network-structure");
        return;
    }
    if (skNetStructureParse(ns, net_structure)) {
        goto END;
    }
    skNetStructureSetOutputStream(ns, outstream);
    skNetStructureSetDelimiter(ns, output_delimiter);
    if (opt_flags.no_columns) {
        skNetStructureSetNoColumns(ns);
    }
    skNetStructureSetIpFormat(ns, ip_format);

    skIPSetIteratorBind(&iter, ipset, 1, SK_IPV6POLICY_MIX);
    while (skIPSetIteratorNext(&iter, &ipaddr, &prefix) == SK_ITERATOR_OK) {
        skNetStructureAddCIDR(ns, &ipaddr, prefix);
    }

    /* set the last key flag and call it once more, for good measure.
     * (that way, it closes out blocks after the last key.) */
    skNetStructurePrintFinalize(ns);

  END:
    skNetStructureDestroy(&ns);
}


#if SK_ENABLE_IPV6
/*
 *  setcatPrintStatisticsV6
 *
 *    Prints, to outF, statistics of the IPSet ipset.  Statistics
 *    printed are the minimum IP, the maximum IP, a count of the
 *    number of every /N from where N is an integer mulitple of 8
 *    between 8 and 120 inclusive.
 */
static void
setcatPrintStatisticsV6(
    skstream_t         *outstream,
    const skipset_t    *ipset)
{
#define NUM_LEVELS_V6 15

    const unsigned int cidr[NUM_LEVELS_V6] = {
        8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120
    };
    struct count_st {
        uint64_t upper;
        uint64_t lower;
    } count[NUM_LEVELS_V6];
    skipset_iterator_t iter;
    skipaddr_t ipaddr;
    skipaddr_t min_ip;
    skipaddr_t max_ip;
    uint8_t old_ip[16];
    uint8_t ipv6[16];
    uint32_t prefix;
    char ip_str1[SK_NUM2DOT_STRLEN+1];
    char ip_str2[SK_NUM2DOT_STRLEN+1];
    uint64_t tmp;
    double d_count;
    int i;

    memset(count, 0, sizeof(count));

    if (skIPSetIteratorBind(&iter, ipset, 1, SK_IPV6POLICY_FORCE)) {
        return;
    }

    /* Get first IP */
    if (skIPSetIteratorNext(&iter, &ipaddr, &prefix) != SK_ITERATOR_OK) {
        /* empty ipset */
        skStreamPrint(outstream,
                      ("Network Summary\n"
                       "\tminimumIP = %s\n"
                       "\tmaximumIP = %s\n"),
                      "-", "-");
        for (i = 0; i < NUM_LEVELS_V6; ++i) {
            skStreamPrint(outstream, ("\t%39" PRIu64 " occupied /%u%s\n"),
                          count[i].lower, cidr[i],
                          ((count[i].lower > 1) ? "s" : ""));
        }
        tmp = 0;
        skStreamPrint(outstream, ("\t%39" PRIu64 " host%s\n"),
                      tmp, ((tmp > 1) ? "s (/128s)" : " (/128)"));
        return;
    }

    /* first IP */
    skipaddrCopy(&min_ip, &ipaddr);
    skipaddrGetV6(&ipaddr, old_ip);
    old_ip[0] = ~old_ip[0];

    do {
        skipaddrGetV6(&ipaddr, ipv6);

        for (i = 0; i < 16; ++i) {
            if (ipv6[i] != old_ip[i]) {
                for ( ; i < NUM_LEVELS_V6 && prefix >= cidr[i]; ++i) {
                    if (count[i].lower < UINT64_MAX) {
                        ++count[i].lower;
                    } else {
                        ++count[i].upper;
                        count[i].lower = 0;
                    }
                }
                for ( ; i < NUM_LEVELS_V6 && (cidr[i] - prefix < 64); ++i) {
                    tmp = UINT64_C(1) << (cidr[i] - prefix);
                    if (UINT64_MAX - tmp > count[i].lower) {
                        count[i].lower += tmp;
                    } else {
                        ++count[i].upper;
                        count[i].lower -= (UINT64_MAX - tmp) + 1;
                    }
                }
                for ( ; i < NUM_LEVELS_V6; ++i) {
                    count[i].upper += UINT64_C(1) << (cidr[i] - prefix - 64);
                }
                break;
            }
        }

        memcpy(old_ip, ipv6, sizeof(ipv6));
    } while (skIPSetIteratorNext(&iter, &ipaddr, &prefix) == SK_ITERATOR_OK);

    /* get max IP */
    skCIDR2IPRange(&ipaddr, prefix, &ipaddr, &max_ip);

    skStreamPrint(outstream,
                  ("Network Summary\n"
                   "\tminimumIP = %s\n"
                   "\tmaximumIP = %s\n"),
                  skipaddrString(ip_str1, &min_ip, ip_format),
                  skipaddrString(ip_str2, &max_ip, ip_format));

    for (i = 0; i < NUM_LEVELS_V6; ++i) {
        if (0 == count[i].upper) {
            skStreamPrint(outstream, ("\t%39" PRIu64 " occupied /%u%s\n"),
                          count[i].lower, cidr[i],
                          ((count[i].lower > 1) ? "s" : ""));
        } else {
            d_count = ((double)count[i].upper * ((double)UINT64_MAX + 1.0)
                       + (double)count[i].lower);
            skStreamPrint(outstream, ("\t%39.f occupied /%us\n"),
                          d_count, cidr[i]);
        }
    }
    tmp = skIPSetCountIPs(ipset, &d_count);
    if (tmp < UINT64_MAX) {
        skStreamPrint(outstream, ("\t%39" PRIu64 " host%s\n"),
                      tmp, ((tmp > 1) ? "s (/128s)" : " (/128)"));
    } else {
        skStreamPrint(outstream, ("\t%39.f hosts (/128s)\n"),
                      d_count);
    }
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  setcatPrintStatisticsV4
 *
 *    Prints, to outF, statistics of the IPSet ipset.  Statistics
 *    printed are the minimum IP, the maximum IP, a count of the class
 *    A blocks, class B blocks (number of nodes), class C blocks, and
 *    a count of the addressBlocks (/27's) used.  If integerIP is 0,
 *    the min and max IPs are printed in dotted-quad form; otherwise
 *    they are printed as integers.
 */
static void
setcatPrintStatisticsV4(
    skstream_t         *outstream,
    const skipset_t    *ipset)
{
#define PLURAL_COMMA(count, prefix)             \
    ((count == 1)                               \
     ? ((prefix < 10) ? ",  " : ", ")           \
     : ((prefix < 10) ? "s, " : "s,"))
#define NUM_LEVELS_V4 5

    const unsigned int cidr[NUM_LEVELS_V4] = {32, 8, 16, 24, 27};
    const uint32_t mask[NUM_LEVELS_V4] = {0x0000001F, 0xFF000000, 0x00FF0000,
                                          0x0000FF00, 0x000000E0};
    uint64_t count[NUM_LEVELS_V4];
    skipset_iterator_t iter;
    skipaddr_t ipaddr;
    skipaddr_t min_ip;
    skipaddr_t max_ip;
    uint32_t prefix;
    uint32_t old_addr;
    uint32_t xor_ips;
    char ip_str1[SK_NUM2DOT_STRLEN+1];
    char ip_str2[SK_NUM2DOT_STRLEN+1];
    int i;

    memset(count, 0, sizeof(count));

    if (skIPSetIteratorBind(&iter, ipset, 1, SK_IPV6POLICY_MIX)) {
        return;
    }

    /* Get first IP */
    if (skIPSetIteratorNext(&iter, &ipaddr, &prefix) != SK_ITERATOR_OK) {
        /* empty ipset */
        skStreamPrint(outstream,
                      ("Network Summary\n"
                       "\tminimumIP = %15s\n"
                       "\tmaximumIP = %15s\n"),
                      "-", "-");
        skStreamPrint(outstream,
                      ("\t%10" PRIu64 " host%s  %10.6f%% of 2^32\n"),
                      count[0], ((count[0] == 1) ? " (/32),  " : "s (/32s),"),
                      100.0 *((double)count[0]) / pow(2.0, cidr[0]));

        for (i = 1; i < NUM_LEVELS_V4; ++i) {
            skStreamPrint(outstream,
                          ("\t%10" PRIu64 " occupied /%u%s %10.6f%% of 2^%u\n"),
                          count[i], cidr[i], PLURAL_COMMA(count[i], cidr[i]),
                          100.0 * (double)count[i]/pow(2.0, cidr[i]), cidr[i]);
        }
        return;
    }

    /* first IP */
    skipaddrCopy(&min_ip, &ipaddr);
    old_addr = ~(skipaddrGetV4(&ipaddr));

    do {
        /* find most significant bit where they differ */
        xor_ips = old_addr ^ skipaddrGetV4(&ipaddr);

        count[0] += (1 << (32 - prefix));
        for (i = 1; i < NUM_LEVELS_V4; ++i) {
            if (xor_ips & mask[i]) {
                for ( ; i < NUM_LEVELS_V4 && prefix >= cidr[i]; ++i) {
                    ++count[i];
                }
                for ( ; i < NUM_LEVELS_V4; ++i) {
                    count[i] += (1 << (cidr[i] - prefix));
                }
                break;
            }
        }

        old_addr = skipaddrGetV4(&ipaddr);
    } while (skIPSetIteratorNext(&iter, &ipaddr, &prefix) == SK_ITERATOR_OK);

    /* get max IP */
    skCIDR2IPRange(&ipaddr, prefix, &ipaddr, &max_ip);

    skStreamPrint(outstream,
                  ("Network Summary\n"
                   "\tminimumIP = %15s\n"
                   "\tmaximumIP = %15s\n"),
                  skipaddrString(ip_str1, &min_ip, ip_format),
                  skipaddrString(ip_str2, &max_ip, ip_format));

    skStreamPrint(outstream,
                  ("\t%10" PRIu64 " host%s  %10.6f%% of 2^32\n"),
                  count[0], ((count[0] == 1) ? " (/32),  " : "s (/32s),"),
                  100.0 *((double)count[0]) / pow(2.0, cidr[0]));

    for (i = 1; i < NUM_LEVELS_V4; ++i) {
        skStreamPrint(outstream,
                      ("\t%10" PRIu64 " occupied /%u%s %10.6f%% of 2^%u\n"),
                      count[i], cidr[i], PLURAL_COMMA(count[i], cidr[i]),
                      100.0 * (double)count[i] / pow(2.0, cidr[i]), cidr[i]);
    }
}


/*
 *  setcatProcessFile(fh, fileName);
 *
 *    Open the binary set-file given in 'fileName', read the IPset
 *    from it; then print the output requested by the user to the
 *    stream 'fh'.  Return 0 on success, or 1 if the IPset cannot be
 *    read from 'fileName'.
 */
static int
setcatProcessFile(
    skstream_t         *outstream,
    const char         *fileName)
{
    char errbuf[2 * PATH_MAX];
    skstream_t *stream = NULL;
    char count[64];
    skipset_t *ipset = NULL;
    int rv;

    /* Read IPset from file */
    if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
        || (rv = skStreamBind(stream, fileName))
        || (rv = skStreamOpen(stream)))
    {
        skStreamLastErrMessage(stream, rv, errbuf, sizeof(errbuf));
        skAppPrintErr("Unable to read IPset from '%s': %s",
                      fileName, errbuf);
        skStreamDestroy(&stream);
        return 1;
    }
    rv = skIPSetRead(&ipset, stream);
    if (rv) {
        if (SKIPSET_ERR_FILEIO == rv) {
            skStreamLastErrMessage(stream, skStreamGetLastReturnValue(stream),
                                   errbuf, sizeof(errbuf));
        } else {
            strncpy(errbuf, skIPSetStrerror(rv), sizeof(errbuf));
        }
        skAppPrintErr("Unable to read IPset from '%s': %s",
                      fileName, errbuf);
        skStreamDestroy(&stream);
        return 1;
    }
    skStreamDestroy(&stream);

    if (opt_flags.count_ips) {
        if (opt_flags.print_filenames) {
            skStreamPrint(outstream, "%s:", fileName);
        }
        skStreamPrint(outstream, "%s\n",
                      skIPSetCountIPsString(ipset, count, sizeof(count)));
    } else if (opt_flags.print_filenames) {
        skStreamPrint(outstream, "%s:\n", fileName);
    }

    if (opt_flags.print_ips) {
        skIPSetPrint(ipset, outstream, (skipaddr_flags_t)ip_format,
                     (opt_flags.user_cidr
                      ? opt_flags.cidr_blocks : skIPSetIsV6(ipset)));
    }

    if (opt_flags.network_structure) {
        setcatPrintNetwork(outstream, ipset);
    }

    if (opt_flags.ip_ranges) {
#if SK_ENABLE_IPV6
        if (skIPSetIsV6(ipset)) {
            setcatPrintRangesV6(outstream, ipset);
        } else
#endif  /* SK_ENABLE_IPV6 */
        {
            setcatPrintRangesV4(outstream, ipset);
        }
    }

    if (opt_flags.statistics) {
#if SK_ENABLE_IPV6
        if (skIPSetIsV6(ipset)) {
            setcatPrintStatisticsV6(outstream, ipset);
        } else
#endif  /* SK_ENABLE_IPV6 */
        {
            setcatPrintStatisticsV4(outstream, ipset);
        }
    }

    skIPSetDestroy(&ipset);

    return 0;
}


int main(int argc, char **argv)
{
    int i;

    appSetup(argc, argv);                       /* never returns on error */

    if (arg_index == argc) {
        (void)setcatProcessFile(stream_out, "stdin");
    } else {
        for (i = arg_index; i < argc; ++i) {
            (void)setcatProcessFile(stream_out, argv[i]);
        }
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
