/*
** Copyright (C) 2003-2015 by Carnegie Mellon University.
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
 *  rwrandomizeip
 *
 *    Read any SiLK Flow file (rwpacked file, rwfilter output, etc)
 *    and output a file with the source IPs and destination IPs
 *    modified to obfuscate them.
 *
 *    Though the IPs are gone, the port numbers, protocols, sensor
 *    IDs, etc. remain.  These randomized files could provide some
 *    information to a malicious party, e.g., letting them know that a
 *    particular service is in use.
 *
 *    TODO:
 *
 *    --It would be nice if the user could optionally provide the cidr
 *    block into which source and/or destination IPs should be placed.
 *
 *    --Randomize the ports.
 *
 */

#include <silk/silk.h>

RCSIDENT("$SiLK: rwrandomizeip.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwrandomizeip.h"
#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skvector.h>


/* TYPEDEFS AND DEFINES */

/* File handle for --help output */
#define USAGE_FH stdout

/* An interface to a randomization back-end */
typedef struct randomizer_st {
    randomizer_activate_fn_t    activate_fn;
    randomizer_modifyip_fn_t    modifyip_fn;
    randomizer_deactivate_fn_t  deactivate_fn;
    randomizer_unload_fn_t      unload_fn;
    void                       *back_end_data;
    int                         id;
} randomizer_t;

/* Struct to define wrapper to options registered by the back-ends */
typedef struct backend_option_st {
    randomizer_optioncb_fn_t    handler_fn;
    void                       *back_end_data;
    char                       *name;
    char                       *help;
    int                         has_arg;
    int                         backend_id;
    int                         seen;
} backend_option_t;


/* LOCAL VARIABLES */

/* input and output file names */
static const char *in_path;
static const char *out_path;

/* IPsets that list IPs to exclude or include */
static skipset_t *dont_change_set = NULL;
static skipset_t *only_change_set = NULL;

/* whether the user specified the seed */
static int seed_specified = 0;

static randomizer_load_fn_t randomizer_load[] = {
    &rwrandShuffleLoad,
    NULL /* sentinel */
};

/* potential randomization back-ends */
static sk_vector_t *backend_vec = NULL;

/* options that come from the randomization back-ends */
static sk_vector_t *options_vec = NULL;

/* array of options created from the options_vec */
static struct option *options_array = NULL;

/* the back-end to use to randomize the IP addresses.  If this is
 * NULL, the randomizeIP function is used. */
static randomizer_t *g_randomizer = NULL;

/* back-end ID.  this is only used when registering back-ends */
static int back_end_id = -1;


/* OPTIONS */

enum {
    OPT_SEED, OPT_ONLY_CHANGE_SET, OPT_DONT_CHANGE_SET
};

static struct option appOptions[] = {
    {"seed",                    REQUIRED_ARG, 0, OPT_SEED},
    {"only-change-set",         REQUIRED_ARG, 0, OPT_ONLY_CHANGE_SET},
    {"dont-change-set",         REQUIRED_ARG, 0, OPT_DONT_CHANGE_SET},
    {0,0,0,0}  /* sentinel entry */
};

static const char *appHelp[] = {
    "The seed to use for randomizing the IPs",
    ("Only modify IPs that appear in the specified IPset\n"
     "\tfile. Def. Change all IPs"),
    ("Do not modify IPs that appear in the specified IPset\n"
     "\tfile.  Supersedes IPs in only-change-set. Def. Change all IPs"),
    NULL
};



/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  addBackends(void);
static int  determineBackend(randomizer_t **back_end);
static int  randomizeFile(const char *in, const char *out);
static void randomizeIP(uint32_t *ip);


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
#define USAGE_MSG                                                         \
    ("[SWITCHES] <INPUT_FILE> <OUTPUT_FILE>\n"                            \
     "\tSubstitute a pseudo-random IP address for the source and\n"       \
     "\tdestination IP addresses of <INPUT_FILE> and write the result\n"  \
     "\tto <OUTPUT_FILE>.  You may use \"stdin\" for <INPUT_FILE> and\n"  \
     "\t\"stdout\" for <OUTPUT_FILE>.  Gzipped files are o.k.\n")

    FILE *fh = USAGE_FH;
    size_t count;
    size_t i;
    backend_option_t *opt;

    skAppStandardUsage(fh, USAGE_MSG, appOptions, appHelp);
    sksiteOptionsUsage(fh);
    if (options_vec) {
        count = skVectorGetCount(options_vec);
        for (i = 0; i < count; ++i) {
            skVectorGetValue(&opt, options_vec, i);
            if (opt->name && opt->help) {
                fprintf(fh, "--%s %s. %s\n", opt->name,
                        SK_OPTION_HAS_ARG(*opt), opt->help);
            }
        }
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
    size_t count;
    size_t i;
    randomizer_t *backend;
    backend_option_t *opt;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* free the array of 'struct option' that we created */
    if (options_array) {
        free(options_array);
        options_array = NULL;
    }

    /* free each option that the back-ends registered */
    if (options_vec) {
        count = skVectorGetCount(options_vec);
        for (i = 0; i < count; ++i) {
            skVectorGetValue(&opt, options_vec, i);
            if (opt->name) {
                free(opt->name);
            }
            if (opt->help) {
                free(opt->help);
            }
            free(opt);
        }
        skVectorDestroy(options_vec);
        options_vec = NULL;
    }

    /* call each back-end's unload function and then free it */
    if (backend_vec) {
        count = skVectorGetCount(backend_vec);
        for (i = 0; i < count; ++i) {
            skVectorGetValue(&backend, backend_vec, i);
            if (backend->unload_fn) {
                backend->unload_fn(backend->back_end_data);
            }
            free(backend);
        }
        skVectorDestroy(backend_vec);
        backend_vec = NULL;
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
    int arg_index;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL)
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

    /* add randomization back-ends */
    if (addBackends()) {
        exit(EXIT_FAILURE);
    }

    /* parse options */
    arg_index = skOptionsParse(argc, argv);
    assert(arg_index <= argc);
    if (arg_index < 0) {
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    sksiteConfigure(0);

    /* Check that we have an input file */
    if (arg_index >= argc) {
        skAppPrintErr("Expecting input and output file names");
        skAppUsage();           /* never returns */
    }
    in_path = argv[arg_index];
    ++arg_index;

    /* check that we have an output location */
    if (arg_index >= argc) {
        skAppPrintErr("Expecting output file name");
        skAppUsage();           /* never returns */
    }
    out_path = argv[arg_index];
    ++arg_index;

    /* check for extra arguments on command line */
    if (argc != arg_index) {
        skAppPrintErr("Too many arguments or unrecognized switch '%s'",
                      argv[arg_index]);
        skAppUsage();           /* never returns */
    }

    /* determine which back-end to use */
    if (determineBackend(&g_randomizer)) {
        skAppPrintErr("Error determining randomization back-end");
        exit(EXIT_FAILURE);
    }

    return;                       /* OK */
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
    int rv;

    switch (opt_index) {
      case OPT_SEED:
        {
            unsigned long tmp;
#if (SK_SIZEOF_LONG == 4)
            uint32_t t32 = 0;
            rv = skStringParseUint32(&t32, opt_arg, 0, 0);
            tmp = (unsigned long)t32;
#else
            uint64_t t64 = 0;
            rv = skStringParseUint64(&t64, opt_arg, 0, 0);
            tmp = (unsigned long)t64;
#endif
            if (rv) {
                skAppPrintErr("Invalid %s '%s': %s",
                              appOptions[opt_index].name, opt_arg,
                              skStringParseStrerror(rv));
                return 1;
            }
            srandom(tmp);
            seed_specified = 1;
        }
        break;

      case OPT_DONT_CHANGE_SET:
        if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
            || (rv = skStreamBind(stream, opt_arg))
            || (rv = skStreamOpen(stream)))
        {
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            skStreamDestroy(&stream);
            return 1;
        }
        rv = skIPSetRead(&dont_change_set, stream);
        if (rv) {
            if (SKIPSET_ERR_FILEIO == rv) {
                skStreamPrintLastErr(stream, skStreamGetLastReturnValue(stream),
                                     &skAppPrintErr);
            } else {
                skAppPrintErr("Unable to read %s from '%s': %s",
                              appOptions[opt_index].name, opt_arg,
                              skIPSetStrerror(rv));
            }
            skStreamDestroy(&stream);
            return 1;
        }
        skStreamDestroy(&stream);
        break;

      case OPT_ONLY_CHANGE_SET:
        if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
            || (rv = skStreamBind(stream, opt_arg))
            || (rv = skStreamOpen(stream)))
        {
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            skStreamDestroy(&stream);
            return 1;
        }
        rv = skIPSetRead(&only_change_set, stream);
        if (rv) {
            if (SKIPSET_ERR_FILEIO == rv) {
                skStreamPrintLastErr(stream, skStreamGetLastReturnValue(stream),
                                     &skAppPrintErr);
            } else {
                skAppPrintErr("Unable to read %s from '%s': %s",
                              appOptions[opt_index].name, opt_arg,
                              skIPSetStrerror(rv));
            }
            skStreamDestroy(&stream);
            return 1;
        }
        skStreamDestroy(&stream);
        break;
    }

    return 0;                     /* OK */
}


/*
 *  status = rwrandBackendOptionHandler(cData, opt_index, opt_arg);
 *
 *    Like appOptionsHandler(), except it handles the options that the
 *    back-ends registered.  It will call the appropriate function on
 *    the back-end to really handle the option.
 */
static int
rwrandBackendOptionHandler(
    clientData   UNUSED(cData),
    int                 opt_index,
    char               *opt_arg)
{
    backend_option_t *opt;

    if (skVectorGetValue(&opt, options_vec, opt_index)) {
        return 1;
    }
    opt->seen++;
    return ((opt->handler_fn)(opt_arg, opt->back_end_data));
}


/*
 *  status = addBackends();
 *
 *    Add each of the randomization back-ends by calling its 'load'
 *    function which should in turn call rwrandomizerRegister() and
 *    rwrandomizerRegisterOption().
 */
static int
addBackends(
    void)
{
    randomizer_load_fn_t rand_load;
    size_t count;
    size_t i;
    backend_option_t *opt;

    backend_vec = skVectorNew(sizeof(randomizer_t*));
    if (NULL == backend_vec) {
        skAppPrintErr("Unable to create back-end vector");
        exit(EXIT_FAILURE);
    }

    options_vec = skVectorNew(sizeof(backend_option_t*));
    if (NULL == options_vec) {
        skAppPrintErr("Unable to create options vector");
        exit(EXIT_FAILURE);
    }

    /* load (initialize) each randomization back-end */
    for (i = 0; ((rand_load = randomizer_load[i]) != NULL); ++i) {
        back_end_id = i;
        if (rand_load()) {
            skAppPrintErr("Unable to setup randomization back-end");
            exit(EXIT_FAILURE);
        }
    }

    /* Register with the options module all the options that the
     * back-ends registered with us. */
    count = skVectorGetCount(options_vec);
    if (count == 0) {
        return 0;
    }

    /* add one for the sentinel */
    options_array = (struct option*)calloc(1+count, sizeof(struct option));
    if (NULL == options_array) {
        return -1;
    }

    for (i = 0; i < count; ++i) {
        skVectorGetValue(&opt, options_vec, i);
        options_array[i].name    = opt->name;
        options_array[i].has_arg = opt->has_arg;
        options_array[i].flag    = NULL;
        options_array[i].val     = i;
    }

    if (skOptionsRegister(options_array, &rwrandBackendOptionHandler, NULL)) {
        skAppPrintErr("Unable to register options for back-ends");
        return -1;
    }

    return 0;
}


/*
 *  status = determineBackend();
 *
 *    Determine which backend should be used and set the global
 *    variable 'backend' to it.  If no backend was selected, set
 *    'backend' to NULL.
 */
static int
determineBackend(
    randomizer_t      **back_end)
{
    backend_option_t *opt;
    size_t count, i;
    int backend_id = -1;
    char *option_name = NULL;

    /* make certain global backend is set to NULL. */
    *back_end = NULL;

    /* check for options from multiple back-ends */
    count = skVectorGetCount(options_vec);
    for (i = 0; i < count; ++i) {
        skVectorGetValue(&opt, options_vec, i);
        if (opt->seen) {
            if (backend_id == -1) {
                backend_id = opt->backend_id;
                option_name = opt->name;
            } else if (backend_id != opt->backend_id) {
                skAppPrintErr("Conflicting options given: --%s and --%s",
                              option_name, opt->name);
                return -1;
            }
        }
    }

    /* if no options for any back-end were specified, return */
    if (backend_id == -1) {
        return 0;
    }

    /* get a handle to the back-end */
    skVectorGetValue(back_end, backend_vec, backend_id);

    return 0;
}


/* Register a backend.  See rwrandomizeip.h */
int
rwrandomizerRegister(
    randomizer_activate_fn_t    activate_fn,
    randomizer_modifyip_fn_t    modifyip_fn,
    randomizer_deactivate_fn_t  deactivate_fn,
    randomizer_unload_fn_t      unload_fn,
    void                       *back_end_data)
{
    randomizer_t *backend;

    /* check input */
    if ( !modifyip_fn) {
        return -1;
    }

    assert(backend_vec);
    assert(back_end_id != -1);

    /* create and fill in the data for this randomizer back-end */
    backend = (randomizer_t*)calloc(1, sizeof(randomizer_t));
    if (NULL == backend) {
        return -1;
    }
    backend->id = back_end_id;
    backend->activate_fn = activate_fn;
    backend->modifyip_fn = modifyip_fn;
    backend->deactivate_fn = deactivate_fn;
    backend->unload_fn = unload_fn;
    backend->back_end_data = back_end_data;

    if (skVectorAppendValue(backend_vec, &backend)) {
        skAppPrintErr("Unable to regsiter a randomization back-end");
        return -1;
    }

    return 0;
}


/* Register an option for a back-end.  See rwrandomizeip.h */
int
rwrandomizerRegisterOption(
    const char                 *option_name,
    const char                 *option_help,
    randomizer_optioncb_fn_t    option_cb,
    void                       *back_end_data,
    int                         has_arg)
{
    backend_option_t *opt = NULL;
    int rv = -1;

    /* check input */
    if ( !(option_name && option_help && option_cb)) {
        goto END;
    }
    switch (has_arg) {
      case REQUIRED_ARG:
      case OPTIONAL_ARG:
      case NO_ARG:
        break;
      default:
        goto END;
    }

    assert(options_vec);
    assert(back_end_id != -1);

    /* create and fill in the back-end options-wrapper */
    opt = (backend_option_t*)calloc(1, sizeof(backend_option_t));
    if (NULL == opt) {
        goto END;
    }
    opt->backend_id = back_end_id;
    opt->has_arg = has_arg;
    opt->handler_fn = option_cb;
    opt->back_end_data = back_end_data;
    opt->name = strdup(option_name);
    if (opt->name == NULL) {
        goto END;
    }
    opt->help = strdup(option_help);
    if (opt->help == NULL) {
        goto END;
    }

    /* Add it to the list of options */
    rv = skVectorAppendValue(options_vec, &opt);

  END:
    if (rv != 0) {
        if (opt) {
            if (opt->name) {
                free(opt->name);
            }
            if (opt->help) {
                free(opt->help);
            }
            free(opt);
        }
    }
    return rv;
}


/*
 *  randomizeIP(&ip)
 *
 *    Write a new random IP address into the location pointed to by
 *    'ip'.
 */
static void
randomizeIP(
    uint32_t*           ip)
{
    int x, y, z;

    /*
     * 'y' and 'z' are bottom two octects.
     */
    y = (int) (256.0*random()/(SK_MAX_RANDOM+1.0));
    z = (int) (256.0*random()/(SK_MAX_RANDOM+1.0));

    /*
     * 'x' determines the "Class B" address:
     *      0 <= x < 256        10 .   x   . y . z
     *    256 <= x < (256+16)  172 . x-256 . y . z
     *    272 == x             192 .  168  . y . z
     */
    x = (int) ((256.0 + 16.0 + 1.0)*random()/(SK_MAX_RANDOM+1.0));

    if (x < 256) {
        *ip = 10<<24 | x<<16 | y<<8 | z;
    }
    else if (x == (256 + 16)) {
        *ip = 192u<<24 | 168<<16 | y<<8 | z;
    }
    else {
        *ip = 172u<<24 | (x - 256 + 16)<<16 | y<<8 | z;
    }
}


/*
 *  randomizeFile(input_path, output_path)
 *
 *    Write the data in the 'input_path' to the 'output_path'
 *    randomizing the source and destination IPs.  Returns 0 if ok, or
 *    non-zero on error.
 */
static int
randomizeFile(
    const char         *input_path,
    const char         *output_path)
{
    skstream_t *in_ios = NULL;
    skstream_t *out_ios = NULL;
    rwRec rwrec;
    randomizer_modifyip_fn_t rand_ip_fn;
    int in_rv = SKSTREAM_OK;
    int rv = 0; /* return value */

    /* If the global back-end is set, use it; otherwise use
     * randomizeIP() to randomize the IP addresses. */
    if (NULL == g_randomizer) {
        rand_ip_fn = &randomizeIP;
    } else {
        /* call the back-ends activate function */
        if (g_randomizer->activate_fn) {
            if (g_randomizer->activate_fn(g_randomizer->back_end_data)) {
                return -1;
            }
        }
        rand_ip_fn = g_randomizer->modifyip_fn;
    }

    /* Open input file */
    rv = skStreamOpenSilkFlow(&in_ios, input_path, SK_IO_READ);
    if (rv) {
        skStreamPrintLastErr(in_ios, rv, &skAppPrintErr);
        skStreamDestroy(&in_ios);
        return 1;
    }
    skStreamSetIPv6Policy(in_ios, SK_IPV6POLICY_ASV4);

    /* Create the output file, copy the headers from the source file,
     * open it, and write the header */
    rv = skStreamCreate(&out_ios, SK_IO_WRITE, SK_CONTENT_SILK_FLOW);
    if (rv == SKSTREAM_OK) {
        rv = skStreamBind(out_ios, output_path);
    }
    if (rv == SKSTREAM_OK) {
        rv = skHeaderCopy(skStreamGetSilkHeader(out_ios),
                          skStreamGetSilkHeader(in_ios),
                          SKHDR_CP_ALL);
    }
    if (rv == SKSTREAM_OK) {
        rv = skStreamOpen(out_ios);
    }
    if (rv == SKSTREAM_OK) {
        rv = skStreamWriteSilkHeader(out_ios);
    }
    if (rv != SKSTREAM_OK) {
        goto END;
    }

    /* read the records and randomize the IP addresses */
    while ((in_rv = skStreamReadRecord(in_ios, &rwrec)) == SKSTREAM_OK) {
        if ((!dont_change_set
             || !skIPSetCheckRecordSIP(dont_change_set, &rwrec))
            && (!only_change_set
                || skIPSetCheckRecordSIP(only_change_set, &rwrec)))
        {
            uint32_t ipv4 = rwRecGetSIPv4(&rwrec);
            rand_ip_fn(&ipv4);
            rwRecSetSIPv4(&rwrec, ipv4);
        }

        if ((!dont_change_set
             || !skIPSetCheckRecordDIP(dont_change_set, &rwrec))
            && (!only_change_set
                || skIPSetCheckRecordDIP(only_change_set, &rwrec)))
        {
            uint32_t ipv4 = rwRecGetDIPv4(&rwrec);
            rand_ip_fn(&ipv4);
            rwRecSetDIPv4(&rwrec, ipv4);
        }

        rv = skStreamWriteRecord(out_ios, &rwrec);
        if (SKSTREAM_ERROR_IS_FATAL(rv)) {
            goto END;
        }
    }
    if (SKSTREAM_ERR_EOF != in_rv) {
        skStreamPrintLastErr(in_ios, in_rv, &skAppPrintErr);
    }

  END:
    if (rv) {
        skStreamPrintLastErr(out_ios, rv, &skAppPrintErr);
    }
    /* call the back-end's activate function */
    if (g_randomizer && g_randomizer->deactivate_fn) {
        if (g_randomizer->deactivate_fn(g_randomizer->back_end_data)) {
            rv = -1;
        }
    }
    if (out_ios) {
        /* Close output; if there is an error on close, print it
         * unless we've already encountered an error. */
        int tmp_rv = skStreamClose(out_ios);
        if ((tmp_rv != 0) && (rv == 0)) {
            skStreamPrintLastErr(out_ios, tmp_rv, &skAppPrintErr);
            rv = tmp_rv;
        }
        skStreamDestroy(&out_ios);
    }
    if (in_ios) {
        skStreamDestroy(&in_ios);
    }

    return rv;
}


int main(int argc, char **argv)
{
    appSetup(argc, argv);

    /* Initialize the random number generator unless the user
     * specified the seed. */
    if (!seed_specified) {
        srandom((unsigned int) (time(NULL) / getpid()));
    }

    randomizeFile(in_path, out_path);

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
