/*
** Copyright (C) 2006-2015 by Carnegie Mellon University.
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

#include <silk/silk.h>

RCSIDENT("$SiLK: rwscan_utils.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwscan.h"


/* TYPEDEFS AND DEFINES */

/* file handle for --help output */
#define USAGE_FH stdout


/* OPTIONS */

typedef enum {
    OPT_SCAN_MODEL,
    OPT_OUTPUT_PATH,
    OPT_TRW_INTERNAL_SET,
    OPT_TRW_THETA0,
    OPT_TRW_THETA1,
    OPT_NO_TITLES,
    OPT_NO_COLUMNS,
    OPT_COLUMN_SEPARATOR,
    OPT_DELIMITED,
    OPT_NO_FINAL_DELIMITER,
    OPT_INTEGER_IPS,
    OPT_MODEL_FIELDS,
    OPT_SCANDB,
    OPT_WORKER_THREADS,
    OPT_WORK_QUEUE_DEPTH,
    OPT_VERBOSE_PROGRESS,
    OPT_VERBOSE_FLOWS,
    OPT_VERBOSE_RESULTS,
    OPT_TRW_SIP_SET
} appOptionsEnum;

static struct option appOptions[] = {
    {"scan-model",         REQUIRED_ARG, 0, OPT_SCAN_MODEL        },
    {"output-path",        REQUIRED_ARG, 0, OPT_OUTPUT_PATH       },
    {"trw-internal-set",   REQUIRED_ARG, 0, OPT_TRW_INTERNAL_SET  },
    {"trw-theta0",         REQUIRED_ARG, 0, OPT_TRW_THETA0        },
    {"trw-theta1",         REQUIRED_ARG, 0, OPT_TRW_THETA1        },
    {"no-titles",          NO_ARG,       0, OPT_NO_TITLES         },
    {"no-columns",         NO_ARG,       0, OPT_NO_COLUMNS        },
    {"column-separator",   REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR  },
    {"delimited",          OPTIONAL_ARG, 0, OPT_DELIMITED         },
    {"no-final-delimiter", NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"integer-ips",        NO_ARG,       0, OPT_INTEGER_IPS       },
    {"model-fields",       NO_ARG,       0, OPT_MODEL_FIELDS      },
    {"scandb",             NO_ARG,       0, OPT_SCANDB            },
    {"threads",            REQUIRED_ARG, 0, OPT_WORKER_THREADS    },
    {"queue-depth",        REQUIRED_ARG, 0, OPT_WORK_QUEUE_DEPTH  },
    {"verbose-progress",   REQUIRED_ARG, 0, OPT_VERBOSE_PROGRESS  },
    {"verbose-flows",      NO_ARG,       0, OPT_VERBOSE_FLOWS     },
    {"verbose-results",    OPTIONAL_ARG, 0, OPT_VERBOSE_RESULTS   },
    {"trw-sip-set",        REQUIRED_ARG, 0, OPT_TRW_SIP_SET       },
    {0, 0, 0, 0} /* sentinel entry */
};

static const char *appHelp[] = {
    ("Specify scan model to use.  Available scan models:\n"
     "\t0 - hybrid TRW + BLR (default)\n"
     "\t1 - Threshold Random Walk (TRW) only\n"
     "\t2 - Bayesian Logistic Regression (BLR) only"),
    "Write scan records to this file.  Def. stdout",
    ("Specify IPset file containing ALL valid internal IPs.\n"
     "\tThe TRW model requires a list of targeted IPs."),
     NULL, /* generate dynamically */
     NULL, /* generate dynamically */
    "Do not print column headers. Def. Print titles.",
    "Disable fixed-width columnar output. Def. Columnar",
    "Use specified character between columns. Def. '|'",
    "Shortcut for --no-columns --column-sep=CHAR",
    "Suppress column delimiter at end of line. Def. No",
    "Print IP numbers as integers. Def. No",
    "Show scan model detail fields. Def. No",
    ("Produce output suitable for loading into a RDBMS. Def. No.\n"
     "\t(Shortcut for --no-titles --no-columns --model-fields\n"
     "\t--no-final-delimiter)"),
    "Set number of worker threads to specified value. Def. 1",
    "Set the work queue depth to the specified value",
    ("Report detailed progress, including a message\n"
     "\tas rwscan processes each CIDR block of the specified size. Def. No"),
    ("Write individual flows for events.  This produces\n"
     "\ta lot of output, mostly useful for debugging. Def. No"),
    ("Print verbose results for each source IP.  Def. No"),
    ("Deprecated alias for --trw-internal-set"),
    (char *)NULL
};


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
     "\tDetects scanning activity in SiLK Flow records.  The output\n"  \
     "\tis a pipe-delimited textual file suitable for loading into a\n" \
     "\trelational database.  The input records should be pre-sorted\n" \
     "\twith rwsort(1) by sip, proto, and dip.\n")

    FILE *fh = USAGE_FH;
    int   i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    for (i = 0; appOptions[i].name; i++) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch ((appOptionsEnum)appOptions[i].val) {
          case OPT_TRW_THETA0:
            fprintf(
                fh,
                "Set theta_0 for the TRW model, which is the probability\n"
                "\tthat a connection succeeds given the hypothesis that the\n"
                "\tremote source is benign.  Def. %.6f", TRW_DEFAULT_THETA0);
            break;
          case OPT_TRW_THETA1:
            fprintf(
                fh,
                "Set theta_0 for the TRW model, which is the probability\n"
                "\tthat a connection succeeds given the hypothesis that the\n"
                "\tremote source is benign.  Def. %.6f", TRW_DEFAULT_THETA1);
            break;
          default:
            fprintf(fh, "%s", appHelp[i]);
            break;
        }
        fprintf(fh, "\n");
    }
    skOptionsCtxOptionsUsage(optctx, fh);
    sksiteOptionsUsage(fh);
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
    clientData UNUSED   (cData),
    int                 opt_index,
    char               *opt_arg)
{
    uint32_t i, tmpval;
    int      rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_SCAN_MODEL:
        rv = skStringParseUint32(&options.scan_model, opt_arg, 0, 2);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_TRW_INTERNAL_SET:
      case OPT_TRW_SIP_SET:
        if (options.trw_internal_set_file) {
            skAppPrintErr("Invalid %s: Multiple internal IPsets provided",
                          appOptions[opt_index].name);
            return 1;
        }
        options.trw_internal_set_file = opt_arg;
        break;

      case OPT_TRW_THETA0:
        rv = skStringParseDouble(&options.trw_theta0, opt_arg, 0, 1);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_TRW_THETA1:
        rv = skStringParseDouble(&options.trw_theta1, opt_arg, 0, 1);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_OUTPUT_PATH:
        if (options.output_file) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        options.output_file = opt_arg;
        break;

      case OPT_INTEGER_IPS:
        options.integer_ips = 1;
        break;

      case OPT_MODEL_FIELDS:
        options.model_fields = 1;
        break;

      case OPT_NO_TITLES:
        /* no titles */
        options.no_titles = 1;
        break;

      case OPT_NO_COLUMNS:
        options.no_columns = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        options.delimiter = opt_arg[0];
        break;

      case OPT_DELIMITED:
        /* dump as delimited text */
        options.no_columns = 1;
        options.no_final_delimiter = 1;
        if (opt_arg) {
            options.delimiter = opt_arg[0];
        }
        break;

      case OPT_NO_FINAL_DELIMITER:
        options.no_final_delimiter = 1;
        break;

      case OPT_SCANDB:
        options.no_titles          = 1;
        options.no_columns         = 1;
        options.model_fields       = 1;
        options.no_final_delimiter = 1;
        options.integer_ips        = 1;
        break;

      case OPT_VERBOSE_FLOWS:
        options.verbose_flows = 1;
        break;

      case OPT_VERBOSE_RESULTS:
        if (opt_arg) {
            rv = skStringParseUint32(&options.verbose_results, opt_arg, 0, 0);
            if (rv) {
                goto PARSE_ERROR;
            }
        }
        else
        {
            options.verbose_results = 1;
        }
        break;

      case OPT_VERBOSE_PROGRESS:
        rv = skStringParseUint32(&tmpval, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        for (i = 0; i < tmpval; i++) {
            options.verbose_progress >>= 1;
            options.verbose_progress  |= 0x80000000;
        }
        break;

      case OPT_WORKER_THREADS:
        rv = skStringParseUint32(&options.worker_threads, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_WORK_QUEUE_DEPTH:
        rv = skStringParseUint32(&options.work_queue_depth, opt_arg, 0, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;
    }

    return 0;                                    /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  appOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;

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
void
appSetup(
    int                 argc,
    char              **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);
    skstream_t *stream = NULL;
    int optctx_flags;
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp) / sizeof(char *)) ==
           (sizeof(appOptions) / sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    memset(&options, 0, sizeof(options_t));

    optctx_flags = (SK_OPTIONS_CTX_INPUT_SILK_FLOW
                    | SK_OPTIONS_CTX_ALLOW_STDIN);

    options.worker_threads          = 0;
    options.work_queue_depth        = 0;
    options.no_titles               = 0;
    options.no_columns              = 0;
    options.verbose_results         = 0;
    options.delimiter               = '|';
    options.trw_theta0              = TRW_DEFAULT_THETA0;
    options.trw_theta1              = TRW_DEFAULT_THETA1;

    memset(&trw_data, 0, sizeof(trw_data_t));
    pthread_mutex_init(&trw_data.mutex, NULL);

    memset(&summary_metrics, 0, sizeof(summary_metrics));

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
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

    /* parse options; print usage if error */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();
    }

    if (options.worker_threads == 0) {
        /* if no thread options were specified, use defaults */
        options.worker_threads   = 1;
        options.work_queue_depth = 1;
    } else if (options.work_queue_depth == 0) {
        /* if threads was specified but queue depth wasn't, set the queue
         * depth to the number of threads */
        options.work_queue_depth = options.worker_threads;
    }

    if (options.scan_model == 0 || options.scan_model == 1) {
        if (options.trw_internal_set_file == NULL) {
            skAppPrintErr("TRW scan model enabled, but --%s not specified",
                          appOptions[OPT_TRW_INTERNAL_SET].name);
            exit(EXIT_FAILURE);
        }

        if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
            || (rv = skStreamBind(stream, options.trw_internal_set_file))
            || (rv = skStreamOpen(stream)))
        {
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            skStreamDestroy(&stream);
            exit(EXIT_FAILURE);
        }
        rv = skIPSetRead(&trw_data.existing, stream);
        if (rv) {
            if (SKIPSET_ERR_FILEIO == rv) {
                skStreamPrintLastErr(stream, skStreamGetLastReturnValue(stream),
                                     &skAppPrintErr);
            } else {
                skAppPrintErr("Error reading binary IPset from '%s': %s",
                              options.trw_internal_set_file,
                              skIPSetStrerror(rv));
            }
            skStreamDestroy(&stream);
            exit(EXIT_FAILURE);
        }
        skStreamDestroy(&stream);
        skIPTreeCreate(&(trw_data.benign));
        skIPTreeCreate(&(trw_data.scanners));
    }

    if ((options.worker_threads > 1) && options.verbose_results) {
        skAppPrintErr("Warning: verbose results mode enabled; this will "
                      "have an adverse effect on multi-threaded performance.");
    }

    /* if no destination was specified, use stdout */
    if (NULL == options.output_file) {
        out_scans.of_fp = stdout;
    } else {
        out_scans.of_name = options.output_file;
        rv = skFileptrOpen(&out_scans, SK_IO_WRITE);
        if (rv) {
            skAppPrintErr("Cannot open '%s' for writing: %s",
                          out_scans.of_name, skFileptrStrerror(rv));
            exit(EXIT_FAILURE);
        }
    }

    return;                     /* OK */
}


/*
 *  appTeardown()
 *
 *    Teardown all modules, close all files, and tidy up all
 *    application state.
 *
 *    This function is idempotent.
 */
void
appTeardown(
    void)
{
    static int teardownFlag = 0;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    if (out_scans.of_name) {
        skFileptrClose(&out_scans, &skAppPrintErr);
    }

    if (trw_data.benign != NULL) {
        skIPTreeDelete(&(trw_data.benign));
    }
    if (trw_data.scanners != NULL) {
        skIPTreeDelete(&(trw_data.scanners));
    }

    if (trw_data.existing != NULL) {
        skIPSetDestroy(&(trw_data.existing));
    }

    skOptionsCtxDestroy(&optctx);
    skAppUnregister();
}

int
rwrec_compare_proto_stime(
    const void         *a,
    const void         *b)
{
    rwRec *pa = (rwRec *) a;
    rwRec *pb = (rwRec *) b;

    if (rwRecGetProto(pa) > rwRecGetProto(pb)) {
        return 1;
    } else if (rwRecGetProto(pa) < rwRecGetProto(pb)) {
        return -1;
    } else if (rwRecGetStartTime(pa) > rwRecGetStartTime(pb)) {
        return 1;
    } else if (rwRecGetStartTime(pa) < rwRecGetStartTime(pb)) {
        return -1;
    } else {
        return 0;
    }
}

int
rwrec_compare_dip(
    const void         *a,
    const void         *b)
{
    rwRec *pa = (rwRec *) a;
    rwRec *pb = (rwRec *) b;

    /*
     * TODOjds:  we could (should) use the comparator here
     */

    if (rwRecGetDIPv4(pa) > rwRecGetDIPv4(pb)) {
        return 1;
    } else if (rwRecGetDIPv4(pa) < rwRecGetDIPv4(pb)) {
        return -1;
    } else {
        return 0;
    }
}

int
rwrec_compare_dip_sport(
    const void         *a,
    const void         *b)
{
    rwRec *pa = (rwRec *) a;
    rwRec *pb = (rwRec *) b;

    /*
     * TODOjds:  comparator
     */
    if (rwRecGetDIPv4(pa) > rwRecGetDIPv4(pb)) {
        return 1;
    } else if (rwRecGetDIPv4(pa) < rwRecGetDIPv4(pb)) {
        return -1;
    } else if (!(rwRecGetProto(pa) == IPPROTO_TCP)
               || (rwRecGetProto(pa) == IPPROTO_UDP))
    {
        return 0;
    } else if (rwRecGetSPort(pa) > rwRecGetSPort(pb)) {
        return 1;
    } else if (rwRecGetSPort(pa) < rwRecGetSPort(pb)) {
        return -1;
    } else {
        return 0;
    }
}



void
calculate_shared_metrics(
    rwRec              *event_flows,
    event_metrics_t    *metrics)
{
    uint32_t last_dip;
    uint32_t last_sp;
    uint32_t last_dp  = 0xffffffff;
    uint32_t i;
    rwRec   *rwcurr   = NULL;

    metrics->sp_count    = 1;
    metrics->unique_dips = 1;
    metrics->unique_dsts = 0;

    last_dip = rwRecGetDIPv4(&event_flows[0]);
    last_sp  = rwRecGetSPort(&event_flows[0]);

    for (i = 0; i < metrics->event_size; i++) {
        rwcurr = &(event_flows[i]);

        metrics->pkts  += rwRecGetPkts(rwcurr);
        metrics->bytes += rwRecGetBytes(rwcurr);

        if (rwRecGetDIPv4(rwcurr)== last_dip) {
            if ((rwRecGetSPort(rwcurr) != last_sp)) {
                metrics->sp_count++;
            }
        } else {
            metrics->sp_count = 1;
            metrics->unique_dips++;
        }
        /* FIXME: should "unique_dsts be unique dips, or unique dip+dport ? */
        if ((rwRecGetDIPv4(rwcurr) != last_dip)
            || (rwRecGetDPort(rwcurr) != last_dp))
        {
            metrics->unique_dsts++;
        }

        last_sp  = rwRecGetSPort(rwcurr);
        last_dp  = rwRecGetDPort(rwcurr);
        last_dip = rwRecGetDIPv4(rwcurr);
    }

}


void
print_flow(
    const rwRec        *rwcurr)
{
    char sipstr[SK_NUM2DOT_STRLEN];
    char dipstr[SK_NUM2DOT_STRLEN];
    char timestr[SKTIMESTAMP_STRLEN];
    char flag_string[SK_TCPFLAGS_STRLEN];

    num2dot_r(rwRecGetSIPv4(rwcurr), sipstr);
    num2dot_r(rwRecGetDIPv4(rwcurr), dipstr);
    sktimestamp_r(timestr, rwRecGetStartTime(rwcurr), 0);
    switch (rwRecGetProto(rwcurr)) {
      case IPPROTO_ICMP:
      {
          uint8_t type = 0, code = 0;

          type = rwRecGetIcmpType(rwcurr);
          code = rwRecGetIcmpCode(rwcurr);

          fprintf(RWSCAN_VERBOSE_FH,
                  "%-4d %16s -> %16s icmp(%03u,%03u) %-24s %6u %3u %6u %8s\n",
                  rwRecGetProto(rwcurr), sipstr, dipstr, type, code, timestr,
                  rwRecGetBytes(rwcurr), rwRecGetPkts(rwcurr),
                  (rwRecGetBytes(rwcurr) / rwRecGetPkts(rwcurr)),
                  skTCPFlagsString(rwRecGetFlags(rwcurr), flag_string,
                                   SK_PADDED_FLAGS));
      }
        break;

      case IPPROTO_TCP:
      case IPPROTO_UDP:
        fprintf(RWSCAN_VERBOSE_FH,
                "%-4d %16s:%5d -> %16s:%5d %-24s %6u %3u %6u %8s\n",
                rwRecGetProto(rwcurr), sipstr, rwRecGetSPort(rwcurr),
                dipstr, rwRecGetDPort(rwcurr), timestr,
                rwRecGetBytes(rwcurr), rwRecGetPkts(rwcurr),
                (rwRecGetBytes(rwcurr) / rwRecGetPkts(rwcurr)),
                skTCPFlagsString(rwRecGetFlags(rwcurr), flag_string,
                                 SK_PADDED_FLAGS));
        break;

      default:
        break;
    }
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
