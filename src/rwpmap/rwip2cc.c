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
**  rwip2cc.c
**
**  Mark Thomas
**
**  Small application to read in IP addresses and print out the
**  corresponding country code.  Can read addresses from the command
**  line, from a file, or from stdin.  One can specify the map file on
**  the command line, or it will use the default.  Probably knows more
**  about the country code map than it should.
**
**  Based on
**
**      skprefixmap-test.c by John McClary Prevost, December 3rd, 2004
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwip2cc.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skcountry.h>
#include <silk/skipaddr.h>
#include <silk/skstream.h>
#include <silk/utils.h>

/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* value indicating an unset IP address */
#define ADDRESS_UNSET 0xFFFFFFFF


/* LOCAL VARIABLES */

/* the output stream */
static skstream_t *out = NULL;

/* flags used to print IPs */
static uint32_t ip_flags = SKIPADDR_CANONICAL;


/* OPTIONS SETUP */

typedef enum {
    OPT_MAP_FILE, OPT_ADDRESS, OPT_INPUT_FILE,
    OPT_PRINT_IPS, OPT_INTEGER_IPS, OPT_ZERO_PAD_IPS,
    OPT_NO_COLUMNS, OPT_COLUMN_SEPARATOR, OPT_NO_FINAL_DELIMITER,
    OPT_DELIMITED,
    OPT_OUTPUT_PATH, OPT_PAGER
} appOptionsEnum;

static struct option appOptions[] = {
    {"map-file",            REQUIRED_ARG, 0, OPT_MAP_FILE},
    {"address",             REQUIRED_ARG, 0, OPT_ADDRESS},
    {"input-file",          REQUIRED_ARG, 0, OPT_INPUT_FILE},
    {"print-ips",           REQUIRED_ARG, 0, OPT_PRINT_IPS},
    {"integer-ips",         NO_ARG,       0, OPT_INTEGER_IPS},
    {"zero-pad-ips",        NO_ARG,       0, OPT_ZERO_PAD_IPS},
    {"no-columns",          NO_ARG,       0, OPT_NO_COLUMNS},
    {"column-separator",    REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR},
    {"no-final-delimiter",  NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"delimited",           OPTIONAL_ARG, 0, OPT_DELIMITED},
    {"output-path",         REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {"pager",               REQUIRED_ARG, 0, OPT_PAGER},
    {0,0,0,0}               /* sentinel entry */
};

static const char *appHelp[] = {
    "Path name of the Country Code mapping file.",
    "IP address to look up",
    ("Path from which to read IP addresses, one per line.\n"
     "\tUse \"stdin\" or \"-\" to read from the standard input"),
    ("When argument 1, write two columns: IP|country_code|.\n"
     "\tWhen argument 0, write country code only. Def. 0 when --address is\n"
     "\tspecified; 1 when --input-file is specified"),
    "Print IP numbers as integers. Def. Dotted decimal",
    "Print IP numbers as zero-padded dotted decimal. Def. No",
    "Disable fixed-width columnar output. Def. Columnar",
    "Use specified character between columns. Def. '|'",
    "Suppress column delimiter at end of line. Def. No",
    "Shortcut for --no-columns --no-final-del --column-sep=CHAR",
    "Send output to given file path. Def. stdout",
    "Program to invoke to page output. Def. $SILK_PAGER or $PAGER",
    (char *)NULL
};

static struct app_opt_st {
    /* filename of map file */
    const char *map_file;
    /* IP address to look up */
    const char *address;
    /* filename to read IPs from */
    const char *input_file;
    /* where to send output */
    const char *output_path;
    /* paging program */
    const char *pager;
    /* delimiter */
    char        column_separator;
    /* ip printing, -1 == not set, 0 == no, 1 == yes */
    int8_t      print_ips;
    /* how to format output */
    unsigned    no_columns          :1;
    unsigned    no_final_delimiter  :1;
} app_opt = {
    NULL, NULL, NULL, "stdout", NULL, '|', -1, 0, 0
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
#define USAGE_MSG                                                            \
    ("{--address=IP_ADDRESS | --input-file=FILE} [SWITCHES]\n"               \
     "\tMaps from textual IP address(es) to country code(s) using the\n"     \
     "\tspecified country code map file or the default map.  Must specify\n" \
     "\ta single address or a file or stream containing textual IPs.\n")

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

    skStreamDestroy(&out);

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
    int rv;

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

    /* register the teardown hanlder */
    if (atexit(appTeardown) < 0) {
        skAppPrintErr("Unable to register appTeardown() with atexit()");
        appTeardown();
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
        skAppUsage();             /* never returns */
    }

    /* check for one and only one of --input-file and --address */
    if (NULL != app_opt.input_file) {
        if (NULL != app_opt.address) {
            skAppPrintErr("Only one of --%s or --%s may be specified.",
                          appOptions[OPT_ADDRESS].name,
                          appOptions[OPT_INPUT_FILE].name);
            skAppUsage();
        }
    } else if (NULL == app_opt.address) {
        skAppPrintErr("Either the --%s or --%s option is required.",
                      appOptions[OPT_ADDRESS].name,
                      appOptions[OPT_INPUT_FILE].name);
        skAppUsage();
    }

    /* find and load the map file */
    if (skCountrySetup(app_opt.map_file, &skAppPrintErr)) {
        exit(EXIT_FAILURE);
    }

    /* use default for print-ips if unset by user */
    if (-1 == app_opt.print_ips) {
        if (app_opt.input_file) {
            app_opt.print_ips = 1;
        } else {
            app_opt.print_ips = 0;
        }
    }

    /* create the output stream. Only page output when processing
     * input file */
    if ((rv = skStreamCreate(&out, SK_IO_WRITE, SK_CONTENT_TEXT))
        || (rv = skStreamBind(out, app_opt.output_path)))
    {
        skStreamPrintLastErr(out, rv, &skAppPrintErr);
        exit(EXIT_FAILURE);
    }
    if (app_opt.input_file) {
        rv = skStreamPageOutput(out, app_opt.pager);
        if (rv) {
            skStreamPrintLastErr(out, rv, &skAppPrintErr);
            exit(EXIT_FAILURE);
        }
    }
    rv = skStreamOpen(out);
    if (rv) {
        skStreamPrintLastErr(out, rv, &skAppPrintErr);
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
    switch ((appOptionsEnum)opt_index) {
      case OPT_MAP_FILE:
        app_opt.map_file = opt_arg;
        break;

      case OPT_ADDRESS:
        app_opt.address = opt_arg;
        break;

      case OPT_INPUT_FILE:
        app_opt.input_file = opt_arg;
        break;

      case OPT_PRINT_IPS:
        switch (*opt_arg) {
          case '0':
            app_opt.print_ips = 0;
            break;
          case '1':
            app_opt.print_ips = 1;
            break;
          default:
            skAppPrintErr("Invalid --%s: Value must be 0 or 1",
                          appOptions[opt_index].name);
            return 1;
        }
        break;

      case OPT_INTEGER_IPS:
        app_opt.print_ips = 1;
        if (SKIPADDR_ZEROPAD == ip_flags) {
            skAppPrintErr("Printing IPs as integer overrides zero padding IPs");
        }
        ip_flags = SKIPADDR_DECIMAL;
        break;

      case OPT_ZERO_PAD_IPS:
        app_opt.print_ips = 1;
        if (SKIPADDR_DECIMAL == ip_flags) {
            skAppPrintErr("Printing IPs as integer overrides zero padding IPs");
        } else {
            ip_flags = SKIPADDR_ZEROPAD;
        }
        break;

      case OPT_NO_COLUMNS:
        app_opt.no_columns = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        app_opt.column_separator = opt_arg[0];
        break;

      case OPT_NO_FINAL_DELIMITER:
        app_opt.no_final_delimiter = 1;
        break;

      case OPT_DELIMITED:
        app_opt.no_columns = 1;
        app_opt.no_final_delimiter = 1;
        if (opt_arg) {
            app_opt.column_separator = opt_arg[0];
        }
        break;

      case OPT_OUTPUT_PATH:
        app_opt.output_path = opt_arg;
        break;

      case OPT_PAGER:
        app_opt.pager = opt_arg;
        break;
    }

    return 0;  /* OK */
}


static int
processOneAddress(
    const char         *addr)
{
    char final_delim[] = {'\0', '\0'};
    char cc[16];
    char ipbuf[SK_NUM2DOT_STRLEN];
    skipaddr_t ip;
    int rv;

    if (!app_opt.no_final_delimiter) {
        final_delim[0] = app_opt.column_separator;
    }

    rv = skStringParseIP(&ip, addr);
    if (rv) {
        skAppPrintErr("Invalid %s '%s': %s",
                      appOptions[OPT_ADDRESS].name, addr,
                      skStringParseStrerror(rv));
        return 1;
    }
#if SK_ENABLE_IPV6
    if (skipaddrIsV6(&ip)) {
        skAppPrintErr("Invalid %s '%s': IPv6 addresses not supported",
                      appOptions[OPT_ADDRESS].name, addr);
        return 1;
    }
#endif /* SK_ENABLE_IPV6 */

    skCountryLookupName(&ip, cc, sizeof(cc));

    if (!app_opt.print_ips) {
        skStreamPrint(out, "%s\n", cc);
    } else {
        skipaddrString(ipbuf, &ip, ip_flags);
        if (app_opt.no_columns) {
            skStreamPrint(out, "%s%c%s%s\n",
                          ipbuf, app_opt.column_separator, cc, final_delim);
        } else {
            skStreamPrint(out, "%15s%c%2s%s\n",
                          ipbuf, app_opt.column_separator, cc, final_delim);
        }
    }

    return 0;
}


/*
 *  status = processInputFile(filein);
 *
 *    For every line in 'filein', look up the address in Country Code
 *    map and print out the corresponding country code.  There should
 *    be as many lines of output as there are of input.
 */
static int
processInputFile(
    const char         *f_name)
{
    char final_delim[] = {'\0', '\0'};
    char line[2048];
    skstream_t *stream = NULL;
    skIPWildcardIterator_t iter;
    skIPWildcard_t ipwild;
    skipaddr_t ip;
    int retval = 1;
    int rv;
    int lc = 0;
    char cc[32];
    char ipbuf[SK_NUM2DOT_STRLEN];

    if (!app_opt.no_final_delimiter) {
        final_delim[0] = app_opt.column_separator;
    }

    /* open input */
    if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_TEXT))
        || (rv = skStreamBind(stream, f_name))
        || (rv = skStreamSetCommentStart(stream, "#"))
        || (rv = skStreamOpen(stream)))
    {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        goto END;
    }

    /* read until end of file */
    while ((rv = skStreamGetLine(stream, line, sizeof(line), &lc))
           != SKSTREAM_ERR_EOF)
    {
        switch (rv) {
          case SKSTREAM_OK:
            /* good, we got our line */
            break;
          case SKSTREAM_ERR_LONG_LINE:
            /* bad: line was longer than sizeof(line) */
            skAppPrintErr("Input line %d too long. ignored",
                          lc);
            continue;
          default:
            /* unexpected error */
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            goto END;
        }

        /* parse the line: fill in octet_bitmap */
        rv = skStringParseIPWildcard(&ipwild, line);
        if (rv && rv != SKUTILS_ERR_EMPTY) {
            /* error */
            skAppPrintErr("Error on line %d: %s\n",
                          lc, skStringParseStrerror(rv));
            goto END;
        }
#if SK_ENABLE_IPV6
        if (skIPWildcardIsV6(&ipwild)) {
            continue;
        }
#endif /* SK_ENABLE_IPV6 */

        skIPWildcardIteratorBind(&iter, &ipwild);
        while (skIPWildcardIteratorNext(&iter, &ip) == SK_ITERATOR_OK) {

            skCountryLookupName(&ip, cc, sizeof(cc));

            if (!app_opt.print_ips) {
                skStreamPrint(out, "%s\n", cc);
            } else {
                skipaddrString(ipbuf, &ip, ip_flags);
                if (app_opt.no_columns) {
                    skStreamPrint(out, "%s%c%s%s\n",
                                  ipbuf, app_opt.column_separator,
                                  cc, final_delim);
                } else {
                    skStreamPrint(out, "%15s%c%2s%s\n",
                                  ipbuf, app_opt.column_separator,
                                  cc, final_delim);
                }
            }
        }
    }

    retval = 0;

  END:
    skStreamDestroy(&stream);
    return retval;
}


int main(int argc, char **argv)
{
    int rv;

    appSetup(argc, argv);                       /* never returns on error */

    /* Check the address(es) */
    if (NULL != app_opt.input_file) {
        rv = processInputFile(app_opt.input_file);
    } else {
        rv = processOneAddress(app_opt.address);
    }

    /* done */
    skCountryTeardown();

    return ((0 == rv) ? EXIT_SUCCESS : EXIT_FAILURE);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
