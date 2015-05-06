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
**  Read rwrec input and write the output to every known rwio file
**  format.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwallformats.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout


/* LOCAL VARIABLES */

/* basename of outputs.  Will append -{B,L}-<type>-<version>.dat to this. */
static const char *base_name = NULL;

/* if non-zero, do not include invocation in the output file.  I added
 * this to deal with libtool since, when running within the build
 * directory, the command name may or may not include an "lt-" prefix,
 * and this makes comparing complete files impossible. */
static int no_invocation = 0;

/* where to write the records as they are read to avoid having to open
 * many file handles */
static const char *temp_directory = NULL;

/* file handle to temporary file */
static FILE *tmpf = NULL;

/* support for input files */
static sk_options_ctx_t *optctx = NULL;

/* reference to argc and argv for filter output */
static int g_argc;
static char **g_argv;

/* file formats to output */
static unsigned int stream_format[] = {
    FT_FLOWCAP,
    FT_RWAUGMENTED,
    FT_RWAUGROUTING,
    FT_RWAUGWEB,
    FT_RWAUGSNMPOUT,
    FT_RWFILTER,
    FT_RWGENERIC,
    FT_RWIPV6,
    FT_RWIPV6ROUTING,
    FT_RWNOTROUTED,
    FT_RWROUTED,
    FT_RWSPLIT,
    FT_RWWWW
};


/* OPTIONS SETUP */

typedef enum {
    OPT_BASENAME, OPT_NO_INVOCATION
} appOptionsEnum;

static struct option appOptions[] = {
    {"basename",                REQUIRED_ARG, 0, OPT_BASENAME},
    {"no-invocation",           NO_ARG,       0, OPT_NO_INVOCATION},
    {0,0,0,0}                   /* sentinel entry */
};

static const char *appHelp[] = {
    "Begin each output file with this text",
    "Do not include command line invocation in output",
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static FILE *openTempFile(void);
static size_t openOutput(skstream_t **out_rwios, const rwRec *rwrec);


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
    ("[SWITCHES] [FILES]\n"                                             \
     "\tRead SiLK Flow records as input and write them to files using\n" \
     "\tevery known SiLK Flow file format and byte order.  Files are\n" \
     "\tnamed FT_<format>-v<version>-c<compmethod>-{B,L}.dat, where\n"  \
     "\t<version> is file version, <compmethod> is the compression\n"   \
     "\tmethod, and {B,L} is the byte order (big,little).  The names will\n" \
     "\tbe prefixed by \"<basename>-\" when --basename is given.\n")

    FILE *fh = USAGE_FH;
    int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);

    for (i = 0; appOptions[i].name; i++ ) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch (i) {
          default:
            /* Simple help text from the appHelp array */
            assert(appHelp[i]);
            fprintf(fh, "%s\n", appHelp[i]);
            break;
        }
    }

    skOptionsCtxOptionsUsage(optctx, fh);
    skOptionsTempDirUsage(fh);
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
    static int teardownFlag = 0;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    if (tmpf) {
        fclose(tmpf);
        tmpf = NULL;
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

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW
                    | SK_OPTIONS_CTX_ALLOW_STDIN);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsTempDirRegister(&temp_directory)
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

    /* parse the options */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();
    }

    /* ensure the site config is available */
    if (sksiteConfigure(1)) {
        exit(EXIT_FAILURE);
    }

    /* open a temporary file */
    tmpf = openTempFile();
    if (tmpf == NULL) {
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
      case OPT_BASENAME:
        base_name = opt_arg;
        break;

      case OPT_NO_INVOCATION:
        no_invocation = 1;
        break;
    }

    return 0;  /* OK */
}


/*
 *  fp = openTempFile();
 *
 *    Open a temporary file; the file is immediately unlinked, so it
 *    will be removed when the program exits.
 */
static FILE *
openTempFile(
    void)
{
    static char temp_name[PATH_MAX];
    int len;
    int fd;
    FILE *fp;

    temp_directory = skTempDir(temp_directory, &skAppPrintErr);
    if (temp_directory == NULL) {
        return NULL;
    }

    /* attempt to open a temp file, then remove it */
    len = snprintf(temp_name, sizeof(temp_name), "%s/%s.XXXXXXXX",
                   temp_directory, skAppName());
    if (len < 0 || (size_t)len > sizeof(temp_name)) {
        skAppPrintErr("Error creating temp file name");
        return NULL;
    }
    fd = mkstemp(temp_name);
    if (fd == -1) {
        skAppPrintSyserror("Cannot create temp file %s", temp_name);
        return NULL;
    }
    fp = fdopen(fd, "wb+");
    if (fp == NULL) {
        skAppPrintSyserror("Cannot reopen temp file %s", temp_name);
        return NULL;
    }
    if (unlink(temp_name)) {
        skAppPrintSyserror("Cannot remove temp file '%s'", temp_name);
        return NULL;
    }

    return fp;
}


/*
 *  status = openOutput(&rwios, rwrec);
 *
 *    Open an output stream for every known type-version-endian
 *    combination.  The files are opened one at a time; a different
 *    file is opened each time this function is called.  The caller
 *    should skStreamClose() the file when finished with it.  The newly
 *    opened file is placed into the memory pointed at by 'rwios'.
 *    The 'rwrec' is used to write headers to the file.
 */
static size_t
openOutput(
    skstream_t        **out_rwios,
    const rwRec        *first_rec)
{
    const size_t num_formats = (sizeof(stream_format)/sizeof(unsigned int));
    const silk_endian_t endians[] = {SILK_ENDIAN_BIG, SILK_ENDIAN_LITTLE};
    const char *endian_name[] = {"B", "L"};

    static unsigned int num_compmethod = 0;
    static unsigned int f, e, c;
    static fileVersion_t v;

    silk_endian_t byte_order;
    skstream_t *rwIOS;
    sk_file_header_t *hdr;
    int rv = SKSTREAM_OK;
    char path[PATH_MAX];
    char format_name[SK_MAX_STRLEN_FILE_FORMAT+1];

    if (num_compmethod == 0) {
        /* need to initialize */

        /* find the number of compression methods */
        for (num_compmethod = 0;
             sksiteCompmethodCheck(num_compmethod);
             ++num_compmethod)
            ; /* no-op */

        c = 0;
        f = 0;
        v = 0;
        e = 0;
    }

    /* loop over compression methods */
    for ( ; c < num_compmethod; ++c) {
        if (SK_COMPMETHOD_IS_AVAIL != sksiteCompmethodCheck(c)) {
            continue;
        }

        /* loop over formats */
        for ( ; f < num_formats; ++f) {
            /* loop over versions of that format */
            for (;;) {
                /* only RWGENERIC supports v=0 */
                if (v == 0) {
                    if (stream_format[f] != FT_RWGENERIC) {
                        goto NEXT_VERSION;
                    }
                }
                if (stream_format[f] == FT_FLOWCAP && v < 2) {
                    goto NEXT_VERSION;
                }

                /* loop over byte-orders */
                while (e < 2) {
                    byte_order = endians[e];

                    sksiteFileformatGetName(format_name, sizeof(format_name),
                                            stream_format[f]);
                    if ((size_t)snprintf(path, sizeof(path),
                                         "%s%s%s-v%d-c%u-%s.dat",
                                         ((base_name != NULL) ? base_name :""),
                                         ((base_name != NULL) ? "-" : ""),
                                         format_name, v, c, endian_name[e])
                        >= sizeof(path))
                    {
                        skAppPrintErr("File name overflow");
                        return -1;
                    }

                    /* create and open the file */
                    rv = SKSTREAM_OK;
                    if ( !rv) {
                        rv = skStreamCreate(&rwIOS, SK_IO_WRITE,
                                            SK_CONTENT_SILK_FLOW);
                    }
                    if ( !rv) {
                        rv = skStreamBind(rwIOS, path);
                    }
                    if ( !rv) {
                        hdr = skStreamGetSilkHeader(rwIOS);
                        rv = skHeaderSetFileFormat(hdr, stream_format[f]);
                    }
                    if ( !rv) {
                        rv = skHeaderSetRecordVersion(hdr, v);
                    }
                    if ( !rv) {
                        rv = skHeaderSetByteOrder(hdr, byte_order);
                    }
                    if ( !rv) {
                        rv = skHeaderSetCompressionMethod(hdr, c);
                    }
#if 0
                    if ( !rv) {
                        /* force output to be in IPv4 for comparison
                         * with formats that do not support IPv6 */
                        rv = skStreamSetIPv6Policy(rwIOS, SK_IPV6POLICY_ASV4);
                    }
#endif  /* 0 */
                    if ( !rv) {
                        rv = skHeaderAddProbename(hdr, "DUMMY_PROBE");
                    }
                    if ( !rv) {
                        rv=skHeaderAddPackedfile(hdr,
                                                 rwRecGetStartTime(first_rec),
                                                 rwRecGetFlowType(first_rec),
                                                 rwRecGetSensor(first_rec));
                    }
                    if ( !rv && !no_invocation) {
                        rv = skHeaderAddInvocation(hdr, 1, g_argc, g_argv);
                    }
                    if ( !rv) {
                        rv = skStreamOpen(rwIOS);
                    }
                    if ( !rv) {
                        rv = skStreamWriteSilkHeader(rwIOS);
                    }

                    if (rv) {
                        if (rv == SKSTREAM_ERR_UNSUPPORT_VERSION) {
                            /* Reached max version for this type.  Try
                             * next type. */
                            skStreamDestroy(&rwIOS);
                            if (skFileExists(path)) {
                                unlink(path);
                            }
                            goto NEXT_TYPE;
                        }

                        /* Unexpected error.  Bail */
                        skStreamPrintLastErr(rwIOS, rv, &skAppPrintErr);
                        skAppPrintErr("Error opening '%s'\n", path);
                        skStreamDestroy(&rwIOS);
                        return -1;
                    }

                    /* increment 'e' for the next run */
                    ++e;

                    /* and return */
                    *out_rwios = rwIOS;
                    return 0;

                } /* for e */

              NEXT_VERSION:
                ++v;
                e = 0;
            } /* while 1 */
          NEXT_TYPE:
            v = 0;
            e = 0;
        } /* for f */
        f = 0;
        v = 0;
        e = 0;
    } /* for c */

    return 1;
}


/*
 *  writeOutputs();
 *
 *    Writes the records stored in the global 'tmpf' to a file in
 *    every know file format, version, compression method, and byte
 *    order.  The openOutput() function is called to open the next
 *    file, then the records in tmpf are written there.
 */
static void
writeOutputs(
    void)
{
    skstream_t *rwIOS;
    rwRec rwrec;
    int rv;

    /* write each real output file */
    for (;;) {
        if (fseek(tmpf, 0, SEEK_SET) == -1) {
            skAppPrintSyserror("Cannot seek in temp file");
            exit(EXIT_FAILURE);
        }
        if (!fread(&rwrec, sizeof(rwRec), 1, tmpf)) {
            skAppPrintErr("Cannot read from temp file");
            exit(EXIT_FAILURE);
        }

        rv = openOutput(&rwIOS, &rwrec);
        if (rv != 0) {
            if (rv == 1) {
                /* done */
                break;
            }
            /* error */
            exit(EXIT_FAILURE);
        }

        do {
            rv = skStreamWriteRecord(rwIOS, &rwrec);
            if (SKSTREAM_OK != rv) {
                skStreamPrintLastErr(rwIOS, rv, &skAppPrintErr);
                if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                    break;
                }
            }
        } while (fread(&rwrec, sizeof(rwRec), 1, tmpf));

        rv = skStreamClose(rwIOS);
        if (SKSTREAM_OK != rv) {
            skStreamPrintLastErr(rwIOS, rv, &skAppPrintErr);
        }
        skStreamDestroy(&rwIOS);
    }
}


/*
 *  status = readFileToTemp(input_path);
 *
 *    Append the contents of input_path to the temporary file.
 */
static int
readFileToTemp(
    skstream_t         *rwio_in)
{
    rwRec rwrec;
    int rv;

    while ((rv = skStreamReadRecord(rwio_in, &rwrec)) == SKSTREAM_OK) {
        if (!fwrite(&rwrec, sizeof(rwRec), 1, tmpf)) {
            skAppPrintSyserror("Cannot write to temp file");
            return -1;
        }
    }
    if (SKSTREAM_ERR_EOF != rv) {
        skStreamPrintLastErr(rwio_in, rv, &skAppPrintErr);
    }

    return 0;
}


int main(int argc, char **argv)
{
    skstream_t *rwios;
    int rv = 0;

    g_argc = argc;
    g_argv = argv;

    appSetup(argc, argv);                       /* never returns on error */

    /* process input */
    while ((rv = skOptionsCtxNextSilkFile(optctx, &rwios, &skAppPrintErr))
           == 0)
    {
        if (readFileToTemp(rwios)) {
            skStreamDestroy(&rwios);
            exit(EXIT_FAILURE);
        }
        skStreamDestroy(&rwios);
    }
    if (rv < 0) {
        exit(EXIT_FAILURE);
    }

    /* Write the records from the temp file to each output file */
    writeOutputs();

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
