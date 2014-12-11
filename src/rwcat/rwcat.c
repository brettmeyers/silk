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
**  rwcat.c
**
**  12/13/2002
**
**  Suresh L. Konda
**
**  Stream out a bunch of input files--given on the command
**  line--and/or stdin to stdout or given file Path
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwcat.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* TYPEDEFS AND DEFINES */

/* file handle for --help output */
#define USAGE_FH stdout

/* file handle to use for --print-filenames output */
#define FILENAMES_FH stderr

/* LOCAL VARIABLES */

/* handles input streams */
static sk_options_ctx_t *optctx = NULL;

/* output stream */
static skstream_t *out_ios = NULL;

/* the compression method to use when writing the file.
 * sksiteCompmethodOptionsRegister() will set this to the default or
 * to the value the user specifies. */
static sk_compmethod_t comp_method;

/* byte order of the files we generate; default is to write files in
 * the native byte order */
static silk_endian_t byte_order = SILK_ENDIAN_ANY;

/* how to handle IPv6 flows. the --ipv4-output switch will set this to
 * SK_IPV6POLICY_ASV4 */
static sk_ipv6policy_t ipv6_policy = SK_IPV6POLICY_MIX;


/* OPTIONS SETUP */

typedef enum {
    OPT_OUTPUT_PATH, OPT_BYTE_ORDER, OPT_IPV4_OUTPUT
} appOptionsEnum;

static struct option appOptions[] = {
    {"output-path",     REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {"byte-order",      REQUIRED_ARG, 0, OPT_BYTE_ORDER},
    {"ipv4-output",     NO_ARG,       0, OPT_IPV4_OUTPUT},
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
    "Full path name of the output file. Def. stdout",
    ("Write the output in this byte order. Def. 'native'.\n"
     "\tChoices: 'native', 'little', 'big'"),
    ("Force the output to contain only IPv4 addresses. Def. no"),
    (char *)NULL
};



/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  byteOrderParse(const char *endian_string);


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
    ("[SWITCHES] [FILES] \n"                                            \
     "\tReads SiLK Flow records from the FILES named on the comamnd\n"  \
     "\tline, or from the standard input when no FILES are provided,\n" \
     "\tand writes the SiLK records to the specified output file or\n"  \
     "\tto the standard output if it is not connected to a terminal.\n")

    FILE *fh = USAGE_FH;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
    skOptionsCtxOptionsUsage(optctx, fh);
    sksiteCompmethodOptionsUsage(fh);
    skOptionsNotesUsage(fh);
    sksiteOptionsUsage(fh);
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
    int rv;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* close output file */
    if (out_ios) {
        rv = skStreamClose(out_ios);
        if (rv && rv != SKSTREAM_ERR_NOT_OPEN) {
            skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
        }
        skStreamDestroy(&out_ios);
    }

    skOptionsNotesTeardown();
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
    sk_file_header_t *out_hdr = NULL;
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW | SK_OPTIONS_CTX_ALLOW_STDIN
                    | SK_OPTIONS_CTX_XARGS | SK_OPTIONS_CTX_PRINT_FILENAMES);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsNotesRegister(NULL)
        || sksiteCompmethodOptionsRegister(&comp_method)
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
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* create output stream to stdout if no --output-path was given */
    if (out_ios == NULL) {
        if ((rv = skStreamCreate(&out_ios, SK_IO_WRITE, SK_CONTENT_SILK_FLOW))
            || (rv = skStreamBind(out_ios, "-")))
        {
            skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
            skStreamDestroy(&out_ios);
            exit(EXIT_FAILURE);
        }
    }

    /* get the output file's header */
    out_hdr = skStreamGetSilkHeader(out_ios);

#if SK_ENABLE_IPV6
    /* write an RWGENERIC file if we know there will be no IPv6 flows */
    if (ipv6_policy < SK_IPV6POLICY_MIX) {
        rv = skHeaderSetFileFormat(out_hdr, FT_RWGENERIC);
        if (rv) {
            skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
            skStreamDestroy(&out_ios);
            exit(EXIT_FAILURE);
        }
    }
#endif  /* SK_ENABLE_IPV6 */

    /* set the header, add the notes (if given), open output, and
     * write header */
    if ((rv = skHeaderSetCompressionMethod(out_hdr, comp_method))
        || (rv = skHeaderSetByteOrder(out_hdr, byte_order))
        || (rv = skOptionsNotesAddToStream(out_ios))
        || (rv = skStreamOpen(out_ios))
        || (rv = skStreamWriteSilkHeader(out_ios)))
    {
        skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
        skStreamDestroy(&out_ios);
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
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_OUTPUT_PATH:
        if (out_ios != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if ((rv = skStreamCreate(&out_ios, SK_IO_WRITE, SK_CONTENT_SILK_FLOW))
            || (rv = skStreamBind(out_ios, opt_arg)))
        {
            skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
            skStreamDestroy(&out_ios);
            return 1;
        }
        break;

      case OPT_BYTE_ORDER:
        if (byteOrderParse(opt_arg)) {
            return 1;
        }
        break;

      case OPT_IPV4_OUTPUT:
        ipv6_policy = SK_IPV6POLICY_ASV4;
        break;
    } /* switch */

    return 0;                     /* OK */
}


/*
 *  ok = byteOrderParse(argument)
 *
 *    Parse the argument to the --byte-order switch and set the global
 *    variable 'byte_order'.
 */
static int
byteOrderParse(
    const char         *endian_string)
{
    static int option_seen = 0;
    int i;
    size_t len;
    /* Options for byte-order switch */
    struct {
        const char     *name;
        silk_endian_t   value;
    } byte_order_opts[] = {
        {"native", SILK_ENDIAN_NATIVE},
        {"little", SILK_ENDIAN_LITTLE},
        {"big",    SILK_ENDIAN_BIG},
        {NULL,     SILK_ENDIAN_ANY} /* sentinel */
    };

    /* only process option one time */
    if (option_seen != 0) {
        skAppPrintErr("Invalid %s: Switch used multiple times",
                      appOptions[OPT_BYTE_ORDER].name);
        return 1;
    }
    option_seen = 1;

    len = strlen(endian_string);
    if (len == 0) {
        skAppPrintErr("Invalid %s: Empty string",
                      appOptions[OPT_BYTE_ORDER].name);
        return 1;
    }

    /* parse user's input */
    for (i = 0; byte_order_opts[i].name; ++i) {
        if ((len <= strlen(byte_order_opts[i].name))
            && (0 == strncmp(byte_order_opts[i].name, endian_string, len)))
        {
            byte_order = byte_order_opts[i].value;
            option_seen = 2;
            break;
        }
    }

    if (option_seen != 2) {
        skAppPrintErr("Invalid %s '%s': Unrecognized value",
                      appOptions[OPT_BYTE_ORDER].name, endian_string);
        return 1;
    }

    if (byte_order == SILK_ENDIAN_NATIVE) {
#if SK_LITTLE_ENDIAN
        byte_order = SILK_ENDIAN_LITTLE;
#else
        byte_order = SILK_ENDIAN_BIG;
#endif
    }

    return 0;
}


/*
 *  catFile(in_ios);
 *
 *    Write all the records from 'in_ios' to out_ios.
 */
static void
catFile(
    skstream_t         *in_ios)
{
    rwRec rwrec;
    uint64_t in_count = 0;
    uint64_t out_count = 0;
    FILE *print_filenames;
    int in_rv;
    int rv = SKSTREAM_OK;

    skStreamSetIPv6Policy(in_ios, ipv6_policy);

    while ((in_rv = skStreamReadRecord(in_ios, &rwrec)) == SKSTREAM_OK) {
        in_count++;
        rv = skStreamWriteRecord(out_ios, &rwrec);
        if (SKSTREAM_OK != rv) {
            skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
            if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                break;
            }
        }
        out_count++;
    }
    if ((in_rv != SKSTREAM_ERR_EOF) && (in_rv != SKSTREAM_OK)) {
        skStreamPrintLastErr(in_ios, in_rv, &skAppPrintErr);
    }

    print_filenames = skOptionsCtxGetPrintFilenames(optctx);
    if (print_filenames) {
        fprintf(FILENAMES_FH, ("Read %" PRIu64 " Wrote %" PRIu64 "\n"),
                in_count, out_count);
    }
}


int main(int argc, char **argv)
{
    skstream_t *rwios;
    int rv = 0;

    appSetup(argc, argv);

    /* process input */
    while ((rv = skOptionsCtxNextSilkFile(optctx, &rwios, &skAppPrintErr))
           == 0)
    {
        catFile(rwios);
        skStreamDestroy(&rwios);
    }
    if (rv < 0) {
        exit(EXIT_FAILURE);
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
