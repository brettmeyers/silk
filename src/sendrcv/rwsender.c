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

/*
**  SiLK file transfer program (sender)
**
**  Michael Welsh Duggan
**  December 2006
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwsender.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/utils.h>
#include <silk/skdaemon.h>
#include <silk/sklog.h>
#include <silk/skpolldir.h>
#include <silk/skdllist.h>
#include "rwtransfer.h"

/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* Number of seconds to wait between polling the incoming directory */
#define DEFAULT_POLL_INTERVAL 15
#define DEFAULT_POLL_INTERVAL_STRING "15"

#define DEFAULT_PRIORITY        50
#define HIGH_PRIORITY_THRESHOLD 50
#define IS_HIGH_PRIORITY(x) ((x) > (HIGH_PRIORITY_THRESHOLD))

#define RWSENDER_PASSWORD_ENV ("RWSENDER" PASSWORD_ENV_POSTFIX)

typedef struct priority_st {
    uint16_t priority;
    regex_t  regex;
} priority_t;

typedef struct local_dest_st {
    char       *ident;
    const char *dir;
    regex_t     filter;
    unsigned    filter_exists : 1;
} local_dest_t;

typedef enum transfer_rv {
    TR_SUCCEEEDED, TR_FAILED, TR_IMPOSSIBLE, TR_FATAL
} transfer_rv_t;


/* EXPORTED VARIABLE DEFINITIONS */

/* Set to non-zero when shutting down. */
volatile int shuttingdown;

/* Per-receiver data */
struct rbtree *transfers;

/* Local-side and remote-side version type identifiers */
connection_msg_t local_version_check = CONN_SENDER_VERSION;
connection_msg_t remote_version_check = CONN_RECEIVER_VERSION;

/* Password environment variable name */
const char *password_env = RWSENDER_PASSWORD_ENV;


/* LOCAL VARIABLE DEFINITIONS */

/* Filtering regular expressions that are used to determine which
 * receivers get which files (--filter) */
static sk_dllist_t *filter_list;

/* Priority regular expressions that are used to determine which files
 * to deliver first (--priority) */
static sk_dllist_t *priority_regexps;

/* Local destination directories (--local-directory) */
static sk_dllist_t *local_dests;

/* When non-zero, do not use hard links when creating the copies in
 * the local directories (--unique-local-copies) */
static int unique_local_copies;

/* How often to poll the incoming-directory (--polling-interval) */
static uint32_t polling_interval;

/* Incoming directory (--incoming-directory) */
static char *incoming_dir;

/* Processing directory (--processing-directory) */
static char *processing_dir;

/* Error directory (--error-directory) */
static char *error_dir;

/* Block size to use when transferring file content (--block-size) */
static uint32_t file_block_size;

/* Directory poller for incoming-directory */
static skPollDir_t *polldir;

/* Set to true once skdaemonized() has been called---regardless of
 * whether the --no-daemon switch was given. */
static int daemonized = 0;

/* Thread that handles the files the directory poller finds in the
 * incoming-directory */
static pthread_t incoming_dir_thread;
static int       incoming_thread_valid;


/* OPTIONS SETUP */

typedef enum {
    /* App specific options */
    OPT_INCOMING_DIRECTORY,
    OPT_PROCESSING_DIRECTORY,
    OPT_ERROR_DIRECTORY,
    OPT_LOCAL_DIRECTORY,
    OPT_UNIQUE_LOCAL_COPIES,
    OPT_FILTER,
    OPT_PRIORITY,
    OPT_POLLING_INTERVAL,
    OPT_FILE_BLOCK_SIZE
} appOptionsEnum;

static struct option appOptions[] = {
    {"incoming-directory",   REQUIRED_ARG, 0, OPT_INCOMING_DIRECTORY},
    {"processing-directory", REQUIRED_ARG, 0, OPT_PROCESSING_DIRECTORY},
    {"error-directory",      REQUIRED_ARG, 0, OPT_ERROR_DIRECTORY},
    {"local-directory",      REQUIRED_ARG, 0, OPT_LOCAL_DIRECTORY},
    {"unique-local-copies",  NO_ARG,       0, OPT_UNIQUE_LOCAL_COPIES},
    {"filter",               REQUIRED_ARG, 0, OPT_FILTER},
    {"priority",             REQUIRED_ARG, 0, OPT_PRIORITY},
    {"polling-interval",     REQUIRED_ARG, 0, OPT_POLLING_INTERVAL},
    {"block-size",           REQUIRED_ARG, 0, OPT_FILE_BLOCK_SIZE},
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
    ("Monitor this directory for files to transfer"),
    ("Move each incoming file to this working\n"
     "\tdirectory until the file is successfully transferred"),
    ("Store in this directory files that are not accepted\n"
     "\tby an rwreceiver"),
    ("Create a duplicate of each incoming files in this\n"
     "\tdirectory as a \"local\" destination. Repeat to create multiple\n"
     "\tduplicates. Limit which files are copied to this directory by using\n"
     "\tthe --filter switch and including an identifier and a colon before\n"
     "\tthe local-directory name, specified as IDENT:DIR"),
    ("Create a unique copy of the incoming file in each\n"
     "\tlocal-directory. When not specified, files in each local-directory\n"
     "\tare a reference (hard link) to each other and to the file in the\n"
     "\tprocessing-directory"),
    ("Send files only matching this regular expression to the\n"
     "\trwreceiver or local-directory having this identifier, specified\n"
     "\tas IDENT:REGEXP. Repeat to specify multiple filters"),
    ("Use this priority for sending files matching this regular\n"
     "\texpression, specified as PRIORITY:REGEXP. Repeat to specify multiple\n"
     "\tpriorities. Priority range: 0 (low) to 100 (high). Def. 50"),
    ("Check the incoming-directory for new files this\n"
     "\toften (in seconds). Def. " DEFAULT_POLL_INTERVAL_STRING),
    ("Specify the chunk size to use to use when transferring a\n"
     "\tfile to an rwreceiver (in bytes)."
     " Def. " FILE_BLOCK_SIZE_STRING ". Range 256-65535"),
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);

static void freeLocalDest(void *local);
static void freePriority(void *p);
static void addLocalDest(const char *arg);
static void addPriority(const char *arg);
static void parseFilterData(void);
static int  rwsenderVerifyOptions(void);


/* FUNCTION DEFINITIONS */

/*
 *  appUsageLong();
 *
 *    Print complete usage information to USAGE_FH.  This function is
 *    passed to optionsSetup(); skOptionsParse() will call this funciton
 *    and then exit the program when the --help option is given.
 */
static void
appUsageLong(
    void)
{
#define USAGE_MSG                                                       \
    ("<SWITCHES>\n"                                                     \
     "\tAccepts files placed in a directory and transfers those files\n" \
     "\tto one or more receiver daemons (rwreceiver).\n")

    transferUsageLong(USAGE_FH, USAGE_MSG, appOptions, appHelp);
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
    RBLIST *iter;
    transfer_t *rcvr;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    if (!daemonized) {
        if (filter_list) {
            skDLListDestroy(filter_list);
            filter_list = NULL;
        }
        rbdestroy(transfers);
        skDLListDestroy(priority_regexps);
        skDLListDestroy(local_dests);
        skdaemonTeardown();
        skAppUnregister();
        return;
    }

    NOTICEMSG("Begin shutting down...");

    shuttingdown = 1;

    /* End the file thread. */
    if (polldir) {
        skPollDirStop(polldir);
    }

    transferShutdown();
    iter = rbopenlist(transfers);
    CHECK_ALLOC(iter);
    while ((rcvr = (transfer_t *)rbreadlist(iter)) != NULL) {
        mqShutdown(rcvr->app.r.queue);
    }
    rbcloselist(iter);


    /* Wait for threads here */
    if (incoming_thread_valid) {
        DEBUGMSG("Waiting for incoming file thread to end...");
        pthread_join(incoming_dir_thread, NULL);
        DEBUGMSG("Incoming file thread has ended.");
    }

    if (polldir) {
        skPollDirDestroy(polldir);
    }

    transferTeardown();

    iter = rbopenlist(transfers);
    CHECK_ALLOC(iter);
    while ((rcvr = (transfer_t *)rbreadlist(iter)) != NULL) {
        mqShutdown(rcvr->app.r.queue);
        mqDestroyQueue(rcvr->app.r.high);
        mqDestroyQueue(rcvr->app.r.low);
        mqDestroy(rcvr->app.r.queue);
        if (rcvr->app.r.filter_exists) {
            regfree(&rcvr->app.r.filter);
        }
        if (rcvr->ident != NULL) {
            free(rcvr->ident);
        }
        if (rcvr->addr) {
            skSockaddrArrayDestroy(rcvr->addr);
        }
        free(rcvr);
    }
    rbcloselist(iter);
    rbdestroy(transfers);
    skDLListDestroy(priority_regexps);
    skDLListDestroy(local_dests);

    NOTICEMSG("Finished shutting down.");

    skdaemonTeardown();
    skthread_teardown();
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

    /* check that we have the same number of options entries and help*/
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    shuttingdown          = 0;
    incoming_dir          = NULL;
    processing_dir        = NULL;
    error_dir             = NULL;
    incoming_thread_valid = 0;
    polldir               = NULL;
    polling_interval      = DEFAULT_POLL_INTERVAL;
    file_block_size       = FILE_BLOCK_SIZE;
    unique_local_copies   = 0;

    transfers = transferIdentTreeCreate();
    if (transfers == NULL) {
        skAppPrintErr("Unable to allocate receiver data structure");
        exit(EXIT_FAILURE);
    }

    filter_list = skDLListCreate(NULL);
    if (filter_list == NULL) {
        skAppPrintErr("Unable to allocate list for filter options");
        exit(EXIT_FAILURE);
    }

    priority_regexps = skDLListCreate(freePriority);
    if (priority_regexps == NULL) {
        skAppPrintErr("Unable to allocate list for priority options");
        exit(EXIT_FAILURE);
    }

    local_dests = skDLListCreate(freeLocalDest);
    if (local_dests == NULL) {
        skAppPrintErr("Unable to allocate list for local destination options");
        exit(EXIT_FAILURE);
    }

    /* register the options and handler */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL))
    {
        skAppPrintErr("Unable to register application options");
        exit(EXIT_FAILURE);
    }

    /* Register the other transfer options */
    if (transferSetup()) {
        exit(EXIT_FAILURE);
    }

    /* rwsender runs as a daemon */
    if (skdaemonSetup((SKLOG_FEATURE_LEGACY | SKLOG_FEATURE_SYSLOG),
                      argc, argv))
    {
        exit(EXIT_FAILURE);
    }

    /* register the teardown handler */
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

    /* Parse the filter data */
    parseFilterData();

    /* Verify the options */
    rv = rwsenderVerifyOptions();

    /* verify the required options for logging */
    if (skdaemonOptionsVerify()) {
        rv = -1;
    }

    if (rv) {
        skAppUsage();           /* never returns */
    }

    /* check for extraneous arguments */
    if (arg_index != argc) {
        skAppPrintErr("Too many arguments or unrecognized switch '%s'",
                      argv[arg_index]);
        skAppUsage();           /* never returns */
    }

    /* Identify the main thread */
    skthread_init("main");

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
    int rv;

    switch ((appOptionsEnum)opt_index) {

      case OPT_FILTER:
        rv = skDLListPushHead(filter_list, opt_arg);
        if (rv != 0) {
            skAppPrintErr("Failed to add filter to filter list "
                          "(memory error)");
            return 1;
        }
        break;

      case OPT_PRIORITY:
        addPriority(opt_arg);
        break;

      case OPT_LOCAL_DIRECTORY:
        addLocalDest(opt_arg);
        break;

      case OPT_UNIQUE_LOCAL_COPIES:
        unique_local_copies = 1;
        break;

      case OPT_INCOMING_DIRECTORY:
        if (skOptionsCheckDirectory(opt_arg, appOptions[opt_index].name)) {
            return 1;
        }
        incoming_dir = opt_arg;
        break;

      case OPT_PROCESSING_DIRECTORY:
        if (skOptionsCheckDirectory(opt_arg, appOptions[opt_index].name)) {
            return 1;
        }
        processing_dir = opt_arg;
        break;

      case OPT_ERROR_DIRECTORY:
        if (skOptionsCheckDirectory(opt_arg, appOptions[opt_index].name)) {
            return 1;
        }
        error_dir = opt_arg;
        break;

      case OPT_POLLING_INTERVAL:
        rv = skStringParseUint32(&polling_interval, opt_arg, 1, 0);
        if (rv) {
            goto PARSE_ERROR;
        }
        break;

      case OPT_FILE_BLOCK_SIZE:
        rv = skStringParseUint32(&file_block_size, opt_arg,
                                 MINIMUM_FILE_BLOCK_SIZE, UINT16_MAX);
        if (rv) {
            goto PARSE_ERROR;
        }
        /* Adjust for message overhead to get file block size */
        file_block_size -= offsetof(block_info_t, block)
                           + SKMSG_MESSAGE_OVERHEAD;
        break;

    }

    return 0;  /* OK */

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  appOptions[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return 1;
}


/* Free a local destination */
static void
freeLocalDest(
    void               *vlocal)
{
    local_dest_t *local = (local_dest_t *)vlocal;
    regfree(&local->filter);
    free(local->ident);
    free(local);
}


/* Parse a local destination */
static void
addLocalDest(
    const char         *arg)
{
    char *colon;
    local_dest_t *local;
    const char *dir;
    int rv;
    size_t ident_len;

    colon = strchr(arg, ':');
    dir = colon ? colon + 1 : arg;
    if (skOptionsCheckDirectory(dir, appOptions[OPT_LOCAL_DIRECTORY].name)) {
        exit(EXIT_FAILURE);
    }

    local = (local_dest_t*)calloc(1, sizeof(local_dest_t));
    CHECK_ALLOC(local);

    local->dir = dir;
    ident_len = colon ? (colon - arg) : 0;
    if (ident_len) {
        sk_dll_iter_t iter;
        const local_dest_t *d;

        local->ident = (char*)malloc(sizeof(char) * ident_len + 1);
        CHECK_ALLOC(local->ident);
        strncpy(local->ident, arg, ident_len);
        local->ident[ident_len] = '\0';

        checkIdent(local->ident, appOptions[OPT_LOCAL_DIRECTORY].name);

        skDLLAssignIter(&iter, local_dests);
        while (skDLLIterForward(&iter, (void **)&d) == 0) {
            if (d->ident && strcmp(local->ident, d->ident) == 0) {
                skAppPrintErr("Invalid %s: Duplicate ident %s",
                              appOptions[OPT_LOCAL_DIRECTORY].name,
                              local->ident);
                exit(EXIT_FAILURE);
            }
        }
    }

    rv = skDLListPushHead(local_dests, local);
    CHECK_ALLOC(rv == 0);
}


/* Free a priority */
static void
freePriority(
    void               *vp)
{
    priority_t *p = (priority_t *)vp;
    regfree(&p->regex);
    free(p);
}


/* Parse a priority */
static void
addPriority(
    const char         *arg)
{
    char *pstr = strdup(arg);
    char *rstr;
    priority_t p;
    priority_t *pcopy;
    uint32_t tmp_32;
    int rv;

    CHECK_ALLOC(pstr);

    rstr = strchr(pstr, ':');
    if (rstr == NULL) {
        skAppPrintErr("Invalid %s '%s': Expected to find ':'",
                      appOptions[OPT_PRIORITY].name, arg);
        exit(EXIT_FAILURE);
    }
    *rstr++ = '\0';

    rv = skStringParseUint32(&tmp_32, pstr, 0, 100);
    if (rv) {
        skAppPrintErr("Invalid %s '%s': %s",
                      appOptions[OPT_PRIORITY].name, arg,
                      skStringParseStrerror(rv));
        exit(EXIT_FAILURE);
    }
    p.priority = tmp_32;

    rv = regcomp(&p.regex, rstr, REG_EXTENDED | REG_NOSUB);
    if (rv != 0) {
        char *buf;
        size_t bufsize;

        bufsize = regerror(rv, &p.regex, NULL, 0);
        buf = (char *)malloc(bufsize);
        CHECK_ALLOC(buf);
        regerror(rv, &p.regex, buf, bufsize);
        skAppPrintErr("Invalid %s: Regular expression error in '%s': %s",
                      appOptions[OPT_PRIORITY].name, rstr, buf);
        free(buf);
        exit(EXIT_FAILURE);
    }

    pcopy = (priority_t *)malloc(sizeof(*pcopy));
    CHECK_ALLOC(pcopy);
    memcpy(pcopy, &p, sizeof(p));

    rv = skDLListPushHead(priority_regexps, pcopy);
    CHECK_ALLOC(rv == 0);

    free(pstr);
}



/* Parse the user-supplied filter data */
static void
parseFilterData(
    void)
{
#define FMT_MEM_FAILURE "Memory allocation failure when parsing fiter data"
    const char *arg;
    transfer_t *temp_item = NULL;

    if (!skDLListIsEmpty(filter_list)) {
        temp_item = initTemp();
        if (temp_item == NULL) {
            skAppPrintErr(FMT_MEM_FAILURE);
            exit(EXIT_FAILURE);
        }
    }

    while (skDLListPopTail(filter_list, (void **)&arg) == 0) {
        char *colon;
        char *regexp;
        char *ident = strdup(arg);
        transfer_t *old, *item;
        int rv;
        regex_t *filter = NULL;

        assert(temp_item);
        if (ident == NULL) {
            skAppPrintErr(FMT_MEM_FAILURE);
            exit(EXIT_FAILURE);
        }
        colon = strchr(ident, ':');
        if (colon == NULL) {
            skAppPrintErr("Invalid %s '%s': Expected to find ':'",
                          appOptions[OPT_FILTER].name, arg);
            exit(EXIT_FAILURE);
        }
        *colon = '\0';
        regexp = colon + 1;

        checkIdent(ident, appOptions[OPT_FILTER].name);
        temp_item->ident = ident;

        /* Find the ident from the transfers */
        old = (transfer_t *)rbfind(temp_item, transfers);
        if (old == NULL) {
            sk_dll_iter_t iter;
            local_dest_t *local;

            /* ident was not in the transfers, so check local dests */
            skDLLAssignIter(&iter, local_dests);
            while (skDLLIterForward(&iter, (void **)&local) == 0) {
                if (local->ident && strcmp(ident, local->ident) == 0) {
                    if (local->filter_exists) {
                        skAppPrintErr(("Invalid %s:"
                                       " Multiple filters for ident %s"),
                                      appOptions[OPT_FILTER].name, ident);
                        exit(EXIT_FAILURE);
                    }
                    filter = &local->filter;
                    local->filter_exists = 1;
                    break;
                }
            }
        }
        if (filter == NULL && old == NULL) {
            /* Not in transfers or local dests, so add to transfers */
            old = (transfer_t *)rbsearch(temp_item, transfers);
        }
        if (filter == NULL && old != NULL && old->app.r.filter_exists) {
            skAppPrintErr("Invalid %s: Multiple filters for ident %s",
                          appOptions[OPT_FILTER].name, ident);
            exit(EXIT_FAILURE);
        }
        if (filter) {
            /* local destination */
            item = NULL;
        } else {
            /* receiver destination */
            item = old ? old : temp_item;
            filter = &item->app.r.filter;
            item->app.r.filter_exists = 1;
        }

        if (*regexp == '\0') {
            skAppPrintErr("Invalid %s '%s': Empty regular expression",
                          appOptions[OPT_FILTER].name, arg);
            exit(EXIT_FAILURE);
        }

        rv = regcomp(filter, regexp, REG_EXTENDED | REG_NOSUB);
        if (rv != 0) {
            char *buf;
            size_t bufsize;

            bufsize = regerror(rv, filter, NULL, 0);
            buf = (char *)malloc(bufsize);
            if (buf == NULL) {
                skAppPrintErr(FMT_MEM_FAILURE);
                exit(EXIT_FAILURE);
            }
            regerror(rv, filter, buf, bufsize);
            skAppPrintErr("Invalid %s: Regular expression error in '%s': %s",
                          appOptions[OPT_FILTER].name, regexp, buf);
            free(buf);
            exit(EXIT_FAILURE);
        }

        if (item != NULL && old == NULL) {
            item->ident = strdup(item->ident);
            if (item->ident == NULL) {
                skAppPrintErr(FMT_MEM_FAILURE);
                exit(EXIT_FAILURE);
            }
            clearTemp();
        }

        free(ident);
    }

    /* We are now finished with the list */
    skDLListDestroy(filter_list);
    filter_list = NULL;
}

/*
 *  link_or_copy_file(from, to);
 *
 *    Create the file "to" to have the same content as "from".  First
 *    attempt to make a hard link; if that fails, copy the file.
 */
static int
link_or_copy_file(
    const char         *from,
    const char         *to)
{
    struct stat st_from, st_to;
    int rv;

    rv = link(from, to);
    if (-1 == rv) {
        /* store 'errno' since stat() may overwrite */
        rv = errno;
        if (EEXIST == rv) {
            /* check whether files are already linked */
            if ((stat(from, &st_from) == 0)
                && (stat(to, &st_to) == 0)
                && (st_from.st_dev == st_to.st_dev)
                && (st_from.st_ino == st_to.st_ino))
            {
                /* files are already linked */
                INFOMSG("Files '%s' and '%s' are already hard-linked",
                        from, to);
                return 0;
            }
            /* else files are different.  drop to WARNINGMSG() and
             * then copy the new file over the existing file */
        }
        if (EXDEV != rv) {
            /* warn unless this is a cross-device link */
            WARNINGMSG("Attempting copy; failed to hard-link '%s' to '%s': %s",
                       from, to, strerror(rv));
        }

        rv = skCopyFile(from, to);
        if (0 != rv) {
            WARNINGMSG("Failed to copy '%s' to '%s': %s",
                       from, to, strerror(rv));
            return -1;
        }
    }

    return 0;
}



/*
 *  status = read_processing_directory();
 *
 *    Pass the name of each readable file in the global
 *    'processing_dir' to the appropriate queue.
 *
 */
static void
read_processing_directory(
    void)
{
    RBLIST *list;
    transfer_t *rcvr;

    list = rbopenlist(transfers);
    CHECK_ALLOC(list);
    while ((rcvr = (transfer_t *)rbreadlist(list)) != NULL) {
        DIR *dir;
        struct dirent *entry;
        char path_buffer[PATH_MAX];
        int rv;

        rv = snprintf(path_buffer, sizeof(path_buffer), "%s/%s",
                      processing_dir, rcvr->ident);
        if ((size_t)rv >= sizeof(path_buffer)) {
            CRITMSG("Path too long: '%s/%s'", incoming_dir, rcvr->ident);
            exit(EXIT_FAILURE);
        }

        if (!skDirExists(path_buffer)) {
            rv = skMakeDir(path_buffer);
            if (rv != 0) {
                CRITMSG("Could not create directory '%s': %s",
                        path_buffer, strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        dir = opendir(path_buffer);

        if (dir == NULL) {
            CRITMSG("Could not open processing-directory");
            exit(EXIT_FAILURE);
        }

        while ((entry = readdir(dir)) != NULL) {
            char *filename;
            char filename_buffer[PATH_MAX];
            sk_dll_iter_t iter;
            priority_t *p;
            uint16_t priority = DEFAULT_PRIORITY;
            mq_err_t err;

            /* ignore dot-files */
            if ('.' == entry->d_name[0]) {
                continue;
            }

            INFOMSG("Adding %s to the %s queue",
                    entry->d_name, rcvr->ident);

            rv = snprintf(filename_buffer, sizeof(filename_buffer), "%s/%s/%s",
                          processing_dir, rcvr->ident, entry->d_name);
            assert((size_t)rv < sizeof(filename_buffer));

            filename = strdup(filename_buffer);
            CHECK_ALLOC(filename);

            skDLLAssignIter(&iter, priority_regexps);
            while (skDLLIterForward(&iter, (void **)&p) == 0) {
                rv = regexec(&p->regex, entry->d_name, 0, NULL, 0);
                if (REG_NOMATCH != rv) {
                    priority = p->priority;
                    break;
                }
            }

            err = mqQueueAdd(IS_HIGH_PRIORITY(priority) ?
                             rcvr->app.r.high : rcvr->app.r.low,
                             filename);
            CHECK_ALLOC(err != MQ_MEMERROR);
            if (err != MQ_NOERROR) {
                assert(shuttingdown);
                break;
            }
            assert(err == MQ_NOERROR);
        }
        closedir(dir);

    }
    rbcloselist(list);
}



/*
 *  handle_new_file(path, filename);
 *
 *    Add the file at 'path' (having basename of 'filename') to the
 *    queue so that it will be transferred to the receiver daemon(s).
 */
static void
handle_new_file(
    const char         *path,
    const char         *name)
{
    transfer_t *rcvr;
    int rv;
    char destination[PATH_MAX];
    char *initial = NULL;
    const char *source;
    RBLIST *iter;
    sk_dll_iter_t node;
    uint16_t priority;
    priority_t *p;
    mq_err_t err;
    char *dest_copy;
    int handled = 0;
    int matched = 0;
    sk_dllist_t *high, *low;
    sk_dll_iter_t local_iter;
    const local_dest_t *local;

    iter = rbopenlist(transfers);
    CHECK_ALLOC(iter);

    /* high and low are temporary queues.  while looping over the
     * rwreceivers, files are queued here.  once we finish looping,
     * the files are moved to the actual rwreceivers' queues. this
     * prevents the first receiver from removing a file before the
     * file is linked to the other rwreceivers. */
    high = skDLListCreate(NULL);
    CHECK_ALLOC(high);
    low = skDLListCreate(NULL);
    CHECK_ALLOC(low);

    /* loop over the local destinations */
    skDLLAssignIter(&local_iter, local_dests);
    while (skDLLIterForward(&local_iter, (void **)&local) == 0) {
        if (local->filter_exists) {
            rv = regexec(&local->filter, name, 0, NULL, 0);
            if (REG_NOMATCH == rv) {
                continue;
            }
        }
        matched = 1;

        rv = snprintf(destination, sizeof(destination), "%s/%s",
                      local->dir, name);
        if ((size_t)rv >= sizeof(destination)) {
            WARNINGMSG(("Cannot copy '%s' to local destination due to overlong"
                        " processing path name"), name);
            continue;
        }
        if (unique_local_copies) {
            rv = skCopyFile(path, destination);
            if (rv != 0) {
                WARNINGMSG("Failed to copy '%s' to '%s': %s",
                           path, destination, strerror(rv));
                ERRMSG(("File '%s' will not be delivered to local"
                        " destination '%s'"),
                       name, local->dir);
                continue;
            }
        } else {
            rv = link_or_copy_file(path, destination);
            if (rv == -1) {
                ERRMSG(("File '%s' will not be delivered to local"
                        " destination '%s'"),
                       name, local->dir);
                continue;
            }
        }

        handled = 1;
    }

    /* the 'source' variable is used so we can attempt to hard-link
     * files among the subdirectories of the processing-directory */
    source = path;

    /* loop over the list of rwreceivers */
    while ((rcvr = (transfer_t *)rbreadlist(iter)) != NULL) {
        /* don't process file if it doesn't match the filter */
        if (rcvr->app.r.filter_exists) {
            rv = regexec(&rcvr->app.r.filter, name, 0, NULL, 0);
            if (REG_NOMATCH == rv) {
                continue;
            }
        }
        matched = 1;

        /* rwreceiver-specific destination path */
        rv = snprintf(destination, sizeof(destination),
                      "%s/%s/%s",  processing_dir, rcvr->ident, name);
        if ((size_t)rv >= sizeof(destination)) {
            WARNINGMSG(("Cannot send '%s' to receiver %s due to overlong"
                        " processing path name"), name, rcvr->ident);
            continue;
        }

        rv = link_or_copy_file(source, destination);
        if (-1 == rv) {
            ERRMSG("File '%s' will not be delivered to receiver %s",
                   name, local->dir);
            continue;
        }

        if (initial == NULL) {
            initial = strdup(destination);
            CHECK_ALLOC(initial);
            source = initial;
        }

        dest_copy = strdup(destination);
        CHECK_ALLOC(dest_copy);

        /* determine the priority for the file */
        priority = DEFAULT_PRIORITY;
        skDLLAssignIter(&node, priority_regexps);
        while (skDLLIterForward(&node, (void **)&p) == 0) {
            rv = regexec(&p->regex, name, 0, NULL, 0);
            if (REG_NOMATCH != rv) {
                priority = p->priority;
                break;
            }
        }

        /* add a (filename, rwreceiver) pair to the appropriate
         * temporary queue */
        if (IS_HIGH_PRIORITY(priority)) {
            rv = skDLListPushTail(high, dest_copy);
            CHECK_ALLOC(rv == 0);
            rv = skDLListPushTail(high, rcvr);
            CHECK_ALLOC(rv == 0);
        } else {
            rv = skDLListPushTail(low, dest_copy);
            CHECK_ALLOC(rv == 0);
            rv = skDLListPushTail(low, rcvr);
            CHECK_ALLOC(rv == 0);
        }

        handled = 1;
    }
    rbcloselist(iter);

    /* move the files from the temporary queues to the real queue on
     * each rwreceiver */
    while (skDLListPopHead(high, (void **)&dest_copy) == 0) {
        if (skDLListPopHead(high, (void **)&rcvr)) {
            ERRMSG("Unable to pop item from high queue");
            skAbort();
        }
        err = mqQueueAdd(rcvr->app.r.high, dest_copy);
        CHECK_ALLOC(err != MQ_MEMERROR);
        assert(err == MQ_NOERROR || shuttingdown);
    }
    while (skDLListPopHead(low, (void **)&dest_copy) == 0) {
        if (skDLListPopHead(low, (void **)&rcvr)) {
            ERRMSG("Unable to pop item from low queue");
            skAbort();
        }
        err = mqQueueAdd(rcvr->app.r.low, dest_copy);
        CHECK_ALLOC(err != MQ_MEMERROR);
        assert(err == MQ_NOERROR || shuttingdown);
    }
    skDLListDestroy(high);
    skDLListDestroy(low);

    if (initial != NULL) {
        free(initial);
    }

    if (handled) {
        if (unlink(path) != 0) {
            CRITMSG("Unable to unlink '%s': %s", path, strerror(errno));
            exit(EXIT_FAILURE);
        }
    } else if (!matched) {
        NOTICEMSG("No filter matched '%s'", path);
    }
}


/*
 *  handle_incoming_directory(ignored);
 *
 *    THREAD ENTRY POINT FOR THE incoming_dir_thread.
 *
 *    As long as we are not shutting down, poll for new files in the
 *    incoming directory and pass the filenames to handleNewFile().
 */
static void *
handle_incoming_directory(
    void        UNUSED(*dummy))
{
    char *filename;
    char path[PATH_MAX];
    skPollDirErr_t pderr;

    incoming_thread_valid = 1;

    INFOMSG("Incoming file handling thread started.");

    while (!shuttingdown) {
        pderr = skPollDirGetNextFile(polldir, path, &filename);
        if (pderr != PDERR_NONE) {
            if (pderr == PDERR_STOPPED || shuttingdown) {
                continue;
            }
            CRITMSG("Polldir error ocurred: %s",
                    ((pderr == PDERR_SYSTEM)
                     ? strerror(errno)
                     : skPollDirStrError(pderr)));
            threadExit(EXIT_FAILURE, NULL);
        }
        assert(filename);

        handle_new_file(path, filename);
    }

    INFOMSG("Incoming file handling thread stopped.");

    return NULL;
}


static int
rwsenderVerifyOptions(
    void)
{
    RBLIST *list;
    transfer_t *item;
    sk_dll_iter_t iter;
    const local_dest_t *local;
    int rv;

    /* Verify the transfer options */
    rv = transferVerifyOptions();

    /* Check for the existence of incoming and work directories */
    if (incoming_dir == NULL) {
        skAppPrintErr("The --%s switch is required",
                      appOptions[OPT_INCOMING_DIRECTORY].name);
        rv = -1;
    }
    if (processing_dir == NULL) {
        skAppPrintErr("The --%s switch is required",
                      appOptions[OPT_PROCESSING_DIRECTORY].name);
        rv = -1;
    }

    /* Check for the existence of the error directory */
    if (error_dir == NULL) {
        skAppPrintErr("The --%s switch is required",
                      appOptions[OPT_ERROR_DIRECTORY].name);
        rv = -1;
    }

    if (rv != 0) {
        return rv;
    }

    /* Check for ident collisions */
    skDLLAssignIter(&iter, local_dests);
    while (skDLLIterForward(&iter, (void **)&local) == 0) {
        if (local->ident) {
            transfer_t target;

            target.ident = local->ident;
            if (rbfind(&target, transfers)) {
                skAppPrintErr("Invalid %s: Duplicate ident %s",
                              appOptions[OPT_LOCAL_DIRECTORY].name,
                              local->ident);
                exit(EXIT_FAILURE);
            }
        }
    }

    /* Create the queues */
    list = rbopenlist(transfers);
    if (list == NULL) {
        skAppPrintErr("Memory allocation failure verifying options");
        exit(EXIT_FAILURE);
    }
    while ((item = (transfer_t *)rbreadlist(list)) != NULL) {
        item->app.r.queue = mqCreateUnfair(free);
        CHECK_ALLOC(item->app.r.queue);
        item->app.r.high  = mqCreateQueue(item->app.r.queue);
        CHECK_ALLOC(item->app.r.high);
        item->app.r.low   = mqCreateQueue(item->app.r.queue);
        CHECK_ALLOC(item->app.r.low);
    }
    rbcloselist(list);

    return 0;
}


int
transferUnblock(
    transfer_t         *item)
{
    mqDisable(item->app.r.queue, MQ_REMOVE);
    return 0;
}

static void
free_map(
    file_map_t         *map)
{
    pthread_mutex_destroy(&map->mutex);
    munmap(map->map, map->map_size);
    free(map);
}

static void
decref_map(
    file_map_t         *map)
{
    int nonzero;

    pthread_mutex_lock(&map->mutex);
    nonzero = --map->count;
    pthread_mutex_unlock(&map->mutex);
    if (!nonzero) {
        free_map(map);
    }
}

static void
free_block(
    uint16_t            count,
    struct iovec       *iov)
{
    if (count != 0) {
        sender_block_info_t *block = (sender_block_info_t *)iov->iov_base;
        decref_map(block->ref);
        free(block);
    }
}


/* Move the file 'path' to the error directory associated with the
 * receiver 'ident'.  'name' is the filename of the file, and is used
 * for logging purposes.  */
static void
handleErrorFile(
    const char         *path,
    const char         *name,
    const char         *ident)
{
    char path_buffer[PATH_MAX];
    int rv;

    rv = snprintf(path_buffer, sizeof(path_buffer), "%s/%s",
                  error_dir, ident);
    if ((size_t)rv >= sizeof(path_buffer)) {
        CRITMSG("Path too long: '%s/%s'", error_dir, ident);
        exit(EXIT_FAILURE);
    }
    if (!skDirExists(path_buffer)) {
        rv = skMakeDir(path_buffer);
        if (rv != 0) {
            CRITMSG("Could not create directory '%s': %s",
                    path_buffer, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    INFOMSG("Moving %s to %s", name, path_buffer);
    rv = skMoveFile(path, path_buffer);
    if (rv != 0) {
        WARNINGMSG("Failed to move '%s' to '%s': %s",
                   path, path_buffer, strerror(rv));
    }
}


static transfer_rv_t
transferFile(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    transfer_t         *rcvr,
    const char         *path)
{
    skm_type_t t;
    file_info_t *finfo;
    sender_block_info_t *block = NULL;
    file_map_t *map = NULL;
    uint32_t block_size = 0;
    int fd = -1;
    uint64_t offset = 0;
    uint64_t size = 0;
    const char *name = NULL;
    uint32_t infolen;
    struct stat st;
    sk_msg_t *msg;
    uint8_t *map_pointer = NULL;
    int proto_err;
    int rv;
    time_t dropoff_time = 0;
    time_t send_time = 0;
    time_t finished_time;
    enum transfer_state_en {
        File_info, File_info_ack,
        Send_file, Complete,
        Complete_ack, Done, Error
    } state;
    transfer_rv_t retval = TR_FAILED;

    state = File_info;
    proto_err = 0;

    while (!shuttingdown && !proto_err && !rcvr->disconnect
           && state != Done && state != Error)
    {
        /* Handle reads */
        switch (state) {
          case File_info_ack:
          case Complete_ack:
            rv = skMsgQueueGetMessage(q, &msg);
            if (rv == -1) {
                ASSERT_ABORT(shuttingdown);
                continue;
            }
            rv = handleDisconnect(msg, rcvr->ident);
            if (rv != 0) {
                retval = (rv == -1) ? TR_IMPOSSIBLE : TR_FAILED;
                state = Error;
            }
            break;
          case Done:
          case Error:
            ASSERT_ABORT(0);
            break;
          default:
            msg = NULL;
        }

        /* Handle all states */
        switch (state) {
          case File_info:
            /* Open file and send the file's name and size to
             * rwreceiver */
            if (fd != -1) {
                close(fd);
            }
            fd = open(path, O_RDONLY);
            if (fd == -1) {
                ERRMSG("Could not open '%s' for reading: %s",
                       path, strerror(errno));
                retval = TR_IMPOSSIBLE;
                state = Error;
                break;
            }
            rv = fstat(fd, &st);
            if (rv != 0) {
                ERRMSG("Could not stat '%s': %s",
                       path, strerror(errno));
                retval = TR_IMPOSSIBLE;
                state = Error;
                break;
            }
            if ((size_t)st.st_size > SIZE_MAX) {
                /* TODO: allow files larger than size_t bytes */
                ERRMSG("The file '%s' was too large to be mapped", path);
                retval = TR_IMPOSSIBLE;
                state = Error;
                break;
            }
            size = st.st_size;
            block_size = (size > file_block_size) ? file_block_size : size;

            name = strrchr(path, '/');
            if (name == NULL) {
                name = path;
            } else {
                name++;
            }
            infolen = offsetof(file_info_t, filename) + strlen(name) + 1;

            INFOMSG("Transferring to %s: %s (%" PRIu64 " bytes)",
                    rcvr->ident, name, size);

            /* dropoff_time is the time that we move/link the file
             * from the incoming_dir into the processing_dir.  The
             * file may have been waiting up to two polling_interval
             * cycles before being moved.  We don't use the st_mtime
             * here, since that will give a nonsensical reading if the
             * user puts an old file into the incoming_dir. */
            dropoff_time = st.st_ctime;
            send_time = time(NULL);

            finfo = (file_info_t*)malloc(infolen);
            CHECK_ALLOC(finfo);
            finfo->high_filesize = size >> 32;
            finfo->low_filesize  = size & UINT32_MAX;
            strcpy(finfo->filename, name); /* Should be safe due to
                                              precalculated size */
            finfo->high_filesize = htonl(finfo->high_filesize);
            finfo->low_filesize  = htonl(finfo->low_filesize);
            finfo->block_size    = htonl(block_size);
            finfo->mode          = htonl(st.st_mode & 0777);

            proto_err = skMsgQueueSendMessageNoCopy(q, channel, CONN_NEW_FILE,
                                                    finfo, infolen, free);
            state = File_info_ack;
            break;

          case File_info_ack:
            /* If rwreceiver wants the file, mmap() it */
            if (rcvr->remote_version > 1) {
                t = skMsgType(msg);
                if (t == CONN_DUPLICATE_FILE) {
                    WARNINGMSG("Duplicate instance of %s on %s.  %s",
                               name, rcvr->ident, (char *)skMsgMessage(msg));
                    handleErrorFile(path, name, rcvr->ident);
                    state = Error;
                    retval = TR_IMPOSSIBLE;
                    break;
                } else if (t == CONN_REJECT_FILE) {
                    WARNINGMSG("File %s was rejected by %s. %s",
                               name, rcvr->ident, (char *)skMsgMessage(msg));
                    handleErrorFile(path, name, rcvr->ident);
                    state = Error;
                    retval = TR_IMPOSSIBLE;
                    break;
                }
            }
            if ((proto_err = checkMsg(msg, q, CONN_NEW_FILE_READY))) {
                retval = TR_FAILED;
                break;
            }
            DEBUG_PRINT1("Reveived CONN_NEW_FILE_READY");

            map = (file_map_t*)malloc(sizeof(file_map_t));
            CHECK_ALLOC(map);
            rv = pthread_mutex_init(&map->mutex, NULL);
            if (rv != 0) {
                free(map);
                map = NULL;
                ERRMSG("Failed to create mutex");
                state = Error;
                retval = TR_FAILED;
                break;
            }
            map->map = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
            if (map->map == MAP_FAILED) {
                free(map);
                map = NULL;
                ERRMSG("Could not map '%s': %s", path, strerror(errno));
                state = Error;
                retval = TR_IMPOSSIBLE;
                break;
            }
            map->count = 1;
            map->map_size = size;
            close(fd);
            fd = -1;
            map_pointer = (uint8_t*)map->map;

            state = Send_file;
            break;

          case Send_file:
            /* Allocate a sender_block_info_t that points into the
             * mmap()ed file at a particular offset and queue the
             * block for sending */
            {
                uint32_t len = (size < block_size) ? size : block_size;
                struct iovec iov[2];

                block = (sender_block_info_t *)malloc(sizeof(*block));
                CHECK_ALLOC(block);

                block->high_offset = offset >> 32;
                block->low_offset  = offset & UINT32_MAX;
                block->high_offset = htonl(block->high_offset);
                block->low_offset  = htonl(block->low_offset);

                DEBUG_CONTENT_PRINT("Sending offset=%" PRIu64 " len=%" PRIu32,
                                    offset, len);

                iov[0].iov_base = block;
                iov[0].iov_len = offsetof(sender_block_info_t, ref);
                iov[1].iov_base = map_pointer;
                iov[1].iov_len = len;

                pthread_mutex_lock(&map->mutex);
                block->ref = map;
                map->count++;
                pthread_mutex_unlock(&map->mutex);

                proto_err = skMsgQueueScatterSendMessageNoCopy(
                    q, channel, CONN_FILE_BLOCK, 2, iov, free_block);

                block = NULL;
                map_pointer += len;
                offset      += len;
                size        -= len;
                if (size == 0) {
                    state = Complete;
                }
            }
            break;

          case Complete:
            /* Tell rwreceiver transfer is complete */
            DEBUG_PRINT1("Sending CONN_FILE_COMPLETE");
            proto_err = skMsgQueueSendMessage(q, channel,
                                              CONN_FILE_COMPLETE, NULL, 0);
            state = Complete_ack;
            break;

          case Complete_ack:
            /* Wait for rwreceiver to accept the file */
            if ((proto_err = checkMsg(msg, q, CONN_FILE_COMPLETE))) {
                retval = TR_FAILED;
                state = Error;
                break;
            }
            DEBUG_PRINT1("Received CONN_FILE_COMPLETE");
            finished_time = time(NULL);
            rv = unlink(path);
            if (rv != 0) {
                CRITMSG("Unable to remove '%s' after sending: %s",
                        path, strerror(errno));
                retval = TR_FATAL;
                state = Error;
                break;
            }
            INFOMSG(("Finished transferring to %s: %s  "
                     "total: %.0f secs.  wait: %.0f secs.  "
                     "send: %.0f secs.  size: %" PRIu64 " bytes."),
                    rcvr->ident, name,
                    difftime(finished_time, dropoff_time),
                    difftime(send_time, dropoff_time),
                    difftime(finished_time, send_time),
                    (uint64_t)st.st_size);
            retval = TR_SUCCEEEDED;
            state = Done;
            break;

          case Error:
            break;

          case Done:
            ASSERT_ABORT(0);
        }

        if (msg != NULL) {
            skMsgDestroy(msg);
        }
    }

    if (fd != -1) {
        close(fd);
    }

    if (block) {
        struct iovec iov;
        iov.iov_base = block;
        free_block(1, &iov);
    }

    if (map) {
        decref_map(map);
    }

    return retval;
}


/*
 *    This function is called by the handleConnection() function in
 *    rwtransfer.c once the connection has been established.  This
 *    function returns -1 on error, 0 if no files were transferred, or
 *    1 if one or more files were successfully sent.
 */
int
transferFiles(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    transfer_t         *rcvr)
{
    int transferred_file = 0;

    mqEnable(rcvr->app.r.queue, MQ_REMOVE);

    while (!shuttingdown && !rcvr->disconnect) {
        char *path;
        mq_err_t err;
        transfer_rv_t rv;

        err = mqGet(rcvr->app.r.queue, (void **)&path);
        if (err == MQ_DISABLED || err == MQ_SHUTDOWN) {
            assert(shuttingdown || rcvr->disconnect);
            break;
        }
        assert(err == MQ_NOERROR);

        if (shuttingdown) {
            free(path);
            break;
        }

        if (rcvr->disconnect) {
            /* If we are disconnecting, put the path back on the queue
               for the next time we are connecting */
            err = mqPushBack(rcvr->app.r.queue, path);
            CHECK_ALLOC(err != MQ_MEMERROR);
            if (err != MQ_NOERROR) {
                assert(shuttingdown);
            }
            break;
        }

        rv = transferFile(q, channel, rcvr, path);
        switch (rv) {
          case TR_SUCCEEEDED:
            transferred_file = 1;
            INFOMSG("Succeeded sending %s to %s", path, rcvr->ident);
            free(path);
            break;
          case TR_IMPOSSIBLE:
            INFOMSG("Remote side %s rejected %s", rcvr->ident, path);
            free(path);
            break;
          case TR_FAILED:
            err = mqPushBack(rcvr->app.r.queue, path);
            CHECK_ALLOC(err != MQ_MEMERROR);
            if (err == MQ_NOERROR) {
                INFOMSG("Remote side %s died unexpectedly.", rcvr->ident);
                INFOMSG("Will attempt to re-send %s", path);
            } else{
                assert(shuttingdown);
                INFOMSG("Not scheduling %s to %s for retrying",
                        path, rcvr->ident);
                free(path);
            }
            break;
          case TR_FATAL:
            free(path);
            return -1;
        }
    }
    return transferred_file;
}


int main(int argc, char **argv)
{
    int rv;

    appSetup(argc, argv);       /* never returns on error */

    /* start the logger and become a daemon */
    if (skdaemonize(&shuttingdown, NULL) == -1
        || sklogEnableThreadedLogging() == -1)
    {
        exit(EXIT_FAILURE);
    }
    daemonized = 1;

    /* Add outstanding files to queues */
    read_processing_directory();

    /* Set up directory polling */
    polldir = skPollDirCreate(incoming_dir, polling_interval);
    if (NULL == polldir) {
        CRITMSG("Could not initiate polling for '%s'", incoming_dir);
        exit(EXIT_FAILURE);
    }

    /* Run in client or server mode */
    rv = startTransferDaemon();
    if (rv != 0) {
        exit(EXIT_FAILURE);
    }

    rv = skthread_create("incoming", &incoming_dir_thread,
                         handle_incoming_directory, NULL);
    if (rv != 0) {
        CRITMSG("Failed to create incoming file handling thread: %s",
                strerror(rv));
        exit(EXIT_FAILURE);
    }
    incoming_thread_valid = 1;

    /* We now run forever, accepting signals */
    while (!shuttingdown) {
        pause();
    }

    /* done */
    appTeardown();
    return main_retval;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
