/*
** Copyright (C) 2006-2014 by Carnegie Mellon University.
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
**  Provide functions to daemonize an application
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skdaemon.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skdaemon.h>
#include <silk/sklog.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* daemon context */
typedef struct skdaemon_ctx_st {
    /* location of pid file */
    char           *pidfile;
    /* variable to set to '1' once the signal handler is called */
    volatile int   *shutdown_flag;
    /* whether to chdir to the root directory (0 = yes, 1 = no) */
    unsigned        no_chdir :1;
    /* whether to run as a daemon (0 = yes, 1 = no) */
    unsigned        no_daemon :1;
    /* whether the legacy logging was provided as an option */
    unsigned        legacy_log:1;
} skdaemon_ctx_t;


/* map a signal number to its name */
typedef struct sk_siglist_st {
    int         signal;
    const char *name;
} sk_siglist_t;


/* Use this macro to print an error message to the log stream and to
 * the standard error. Use double parens around arguments to this
 * macro: PRINT_AND_LOG(("%s is bad", "foo")); */
#ifdef TEST_PRINTF_FORMATS
#  define PRINT_AND_LOG(args) printf args
#else
#  define PRINT_AND_LOG(args) \
    do {                      \
        skAppPrintErr args;   \
        ERRMSG args;          \
    } while(0)
#endif


/* LOCAL VARIABLE DEFINITIONS */


/* there is a single context */
static skdaemon_ctx_t daemon_ctx;
static skdaemon_ctx_t *skdaemon = NULL;

/* Signals to ignore or to catch */
static sk_siglist_t ignored_signals[] = {
    /* {SIGCHLD, "CHLD"},  leave at default (which is ignore) */
    {SIGPIPE, "PIPE"},
    {0,NULL}  /* sentinel */
};

static sk_siglist_t caught_signals[] = {
    {SIGHUP,  "HUP"},
    {SIGINT,  "INT"},
#ifdef SIGPWR
    {SIGPWR,  "PWR"},
#endif
    {SIGQUIT, "QUIT"},
    {SIGTERM, "TERM"},
    {0,NULL}  /* sentinel */
};


/* OPTIONS SETUP */
typedef enum {
    OPT_PIDFILE,
    OPT_NO_CHDIR,
    OPT_NO_DAEMON
} daemonOptionsEnum;

static struct option daemonOptions[] = {
    {"pidfile",               REQUIRED_ARG, 0, OPT_PIDFILE},
    {"no-chdir",              NO_ARG,       0, OPT_NO_CHDIR},
    {"no-daemon",             NO_ARG,       0, OPT_NO_DAEMON},
    {0,0,0,0}                 /* sentinel */
};


/* LOCAL FUNCTION PROTOTYPES */

static void daemonHandleSignal(int sigNum);
static int daemonInstallSignalHandler(void);
static int
daemonOptionsHandler(
    clientData          cData,
    int                 opt_index,
    char               *opt_arg);
static int daemonWritePid(void);


/* FUNCTION DEFINITIONS */

/*
 *  daemonHandleSignal(sig_num);
 *
 *    Trap all signals and shutdown when told to.
 */
static void
daemonHandleSignal(
    int                 sig_num)
{
    /* determine name of our signal */
    sk_siglist_t *s;

    for (s = caught_signals; s->name && s->signal != sig_num; s++)
        ; /* empty */

    /* don't allow the writing of the log message to cause the entire
     * program to deadlock.  */
    if (s->name) {
        sklogNonBlock(LOG_NOTICE, "Shutting down due to SIG%s signal",s->name);
    } else {
        sklogNonBlock(LOG_NOTICE, "Shutting down due to unknown signal");
    }

    /* set the global shutdown variable */
    if (skdaemon && skdaemon->shutdown_flag) {
        *(skdaemon->shutdown_flag) = 1;
    }
}


/*
 *  ok = daemonInstallSignalHandler();
 *
 *    Trap all signals we can here with our own handler.
 *    Exception: SIGPIPE.  Set this to SIGIGN.
 *
 *    Returns 0 if OK. -1 else.
 */
static int
daemonInstallSignalHandler(
    void)
{
    sk_siglist_t *s;
    struct sigaction action;
    int rv;

    memset(&action, 0, sizeof(action));

    /* mask any further signals while we're inside the handler */
    sigfillset(&action.sa_mask);

    /* ignored signals */
    action.sa_handler = SIG_IGN;
    for (s = ignored_signals; s->name; ++s) {
        if (sigaction(s->signal, &action, NULL) == -1) {
            rv = errno;
            PRINT_AND_LOG(("Cannot ignore SIG%s: %s",
                           s->name, strerror(rv)));
            return -1;
        }
    }

    /* signals to catch */
    action.sa_handler = &daemonHandleSignal;
    for (s = caught_signals; s->name; ++s) {
        if (sigaction(s->signal, &action, NULL) == -1) {
            rv = errno;
            PRINT_AND_LOG(("Cannot handle SIG%s: %s",
                           s->name, strerror(rv)));
            return -1;
        }
    }

    return 0;
}


/*
 *  status = daemonOptionsHandler(cData, opt_index, opt_arg);
 *
 *    Handle the options that we registered in skdaemonSetup().
 */
static int
daemonOptionsHandler(
    clientData   UNUSED(cData),
    int                 opt_index,
    char               *opt_arg)
{
    switch ((daemonOptionsEnum)opt_index) {
      case OPT_PIDFILE:
        if (skdaemon->pidfile) {
            skAppPrintErr("The --%s switch is given mutliple times",
                          daemonOptions[opt_index].name);
            return -1;
        }
        if (opt_arg[0] != '/') {
            skAppPrintErr(("Must use full path to %s\n"
                           "\t('%s' does not begin with a slash)"),
                          daemonOptions[opt_index].name, opt_arg);
            return -1;
        }
        skdaemon->pidfile = strdup(opt_arg);
        break;

      case OPT_NO_CHDIR:
        skdaemon->no_chdir = 1;
        break;

      case OPT_NO_DAEMON:
        skdaemon->no_daemon = 1;
        break;
    }

    return 0;
}


/*
 *  status = daemonWritePid();
 *
 *    Write the process ID (PID) to the pidfile the user specified.
 *    If no pidfile was specified but a log directory was specified,
 *    write it to that directory.  Otherwise, do not write the PID to
 *    disk.  Store the location of the pidfile on the global context.
 *
 *    Return 0 on success, or errno on failure.
 */
static int
daemonWritePid(
    void)
{
    char pidfile[PATH_MAX+1];
    char pidstr[24];
    int fd;
    ssize_t len;
    ssize_t len2;
    const char *log_directory;
    int saveerr;

    if (!skdaemon->pidfile) {
        /* No pidfile on command line. */
        log_directory = sklogGetDirectory(pidfile, sizeof(pidfile));
        if (!log_directory) {
            return 0;
        }
        /* We do have a log-directory; store the PID there using the
         * application name as the file's base name */
        len = strlen(pidfile);
        len2 = snprintf(&pidfile[len], sizeof(pidfile)-len, "/%s.pid",
                        skAppName());
        if ((size_t)(len + len2) >= sizeof(pidfile)) {
            /* make up an errno */
            return ENAMETOOLONG;
        }
        skdaemon->pidfile = strdup(pidfile);
        if (!skdaemon->pidfile) {
            return errno;
        }
    }

    /* Filesystem Hierarchy Standard says the pid file contains
     * the PID in ASCII-encoded decimal followed by a newline. */
    len = snprintf(pidstr, sizeof(pidstr), "%ld\n", (long)getpid());
    if ((size_t)len >= sizeof(pidstr)) {
        return ENAMETOOLONG;
    }
    fd = open(skdaemon->pidfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        return errno;
    }
    if (skwriten(fd, pidstr, len) == -1) {
        saveerr = errno;
        close(fd);
        unlink(skdaemon->pidfile);
        return saveerr;
    }
    if (close(fd) == -1) {
        saveerr = errno;
        unlink(skdaemon->pidfile);
        return saveerr;
    }

    return 0;
}


/* force the application not to fork */
void
skdaemonDontFork(
    void)
{
    if (skdaemon) {
        skdaemon->no_daemon = 1;
    }
}


/* print the usage of the options defined by this library */
void
skdaemonOptionsUsage(
    FILE               *fh)
{
    int i;

    sklogOptionsUsage(fh);
    for (i = 0; daemonOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. ", daemonOptions[i].name,
                SK_OPTION_HAS_ARG(daemonOptions[i]));
        switch ((daemonOptionsEnum)i) {
          case OPT_PIDFILE:
            if (skdaemon && skdaemon->legacy_log) {
                fprintf(fh, ("Complete path to the process ID file."
                             "  Overrides the path\n"
                             "\tbased on the --log-directory argument."));
            } else {
                fprintf(fh, ("Complete path to the process ID file."
                             "  Def. None"));
            }
            break;
          case OPT_NO_CHDIR:
            fprintf(fh, ("Do not change directory to the root directory.\n"
                         "\tDef. Change directory unless --%s is specified"),
                    daemonOptions[OPT_NO_DAEMON].name);
            break;
          case OPT_NO_DAEMON:
            fprintf(fh, ("Do not fork off as a daemon (for debugging)."
                         " Def. Fork"));
            break;
        }
        fprintf(fh, "\n");
    }
}


/* verify that the options are valid and that all required options
 * were provided. */
int
skdaemonOptionsVerify(
    void)
{
    /* skdaemon doesn't have any options that it requires, but the
     * logging library does. */
    return sklogOptionsVerify();
}


/* register our options and the options for logging */
int
skdaemonSetup(
    int                 log_features,
    int                 argc,
    char   * const     *argv)
{
    if (skdaemon) {
        /* called mulitple times */
        return -1;
    }

    skdaemon = &daemon_ctx;
    memset(skdaemon, 0, sizeof(skdaemon_ctx_t));

    /* setup the log. have it write the invocation when we open it */
    if (sklogSetup(log_features)) {
        return -1;
    }
    sklogCommandLine(argc, argv);

    /* note whether legacy logging was requested so we know how to
     * print the help for the --pidfile switch */
    if (log_features & SKLOG_FEATURE_LEGACY) {
        skdaemon->legacy_log = 1;
    }

    return skOptionsRegister(daemonOptions, &daemonOptionsHandler, NULL);
}


/* remove the PID file and shutdown the logger */
void
skdaemonTeardown(
    void)
{
    if (skdaemon == NULL) {
        return;
    }

    sklogTeardown();

    if (skdaemon->pidfile != NULL) {
        (void)unlink(skdaemon->pidfile);
        free(skdaemon->pidfile);
        skdaemon->pidfile = NULL;
    }

    skdaemon = NULL;
}


/* start logging, install the signal handler, fork off the daemon, and
 * write the PID file */
int
skdaemonize(
    volatile int       *shutdown_flag,
    void                (*exit_handler)(void))
{
    char errbuf[512];
    pid_t pid;
    int rv = -1;
    int fd_devnull = -1;

    /* Must call setup before daemonize; make certain we have a
     * shutdown variable */
    assert(skdaemon);
    assert(shutdown_flag);

    /* Store the shutdown flag */
    skdaemon->shutdown_flag = shutdown_flag;

    /* Start the logger */
    if (sklogOpen()) {
        return -1;
    }

    /* Install the signal handler */
    rv = daemonInstallSignalHandler();
    if (rv) {
        goto ERROR;
    }

    /* Fork a child and exit the parent. */
    if ( !skdaemon->no_daemon) {
        if ( !skdaemon->no_chdir) {
            if (chdir("/") == -1) {
                rv = errno;
                PRINT_AND_LOG(("Cannot change directory: %s", strerror(rv)));
                goto ERROR;
            }
        }
        if ((pid = fork()) == -1) {
            rv = errno;
            PRINT_AND_LOG(("Cannot fork for daemon: %s", strerror(rv)));
            goto ERROR;
        } else if (pid != 0) {
            NOTICEMSG("Forked child %ld.  Parent exiting", (long)pid);
            _exit(EXIT_SUCCESS);
        }

        setsid();
    }

    /* Set umask */
    umask(0022);

    /* Install the exit handler; do this after the fork() so the
     * parent does not execute it. */
    if (exit_handler) {
        rv = atexit(exit_handler);
        if (rv == -1) {
            PRINT_AND_LOG(("Unable to register function with atexit(): %s",
                           strerror(rv)));
            goto ERROR;
        }
    }

    /* Write the pidfile when running as a daemon */
    if ( !skdaemon->no_daemon) {
        rv = daemonWritePid();
        if (rv) {
            if (skdaemon->pidfile) {
                PRINT_AND_LOG(("Error creating pid file '%s': %s",
                               skdaemon->pidfile, strerror(rv)));
            } else {
                PRINT_AND_LOG(("Unable to create pid file path: %s",
                               strerror(rv)));
            }
            goto ERROR;
        }

        /* redirect stdin to /dev/null */
        fd_devnull = open("/dev/null", O_RDWR);
        if (fd_devnull == -1) {
            rv = errno;
            PRINT_AND_LOG(("Error opening /dev/null: %s", strerror(rv)));
            goto ERROR;
        }
        if (dup2(fd_devnull, STDIN_FILENO) == -1) {
            rv = errno;
            PRINT_AND_LOG(("Cannot dup(stdin): %s", strerror(rv)));
            goto ERROR;
        }
        close(fd_devnull);

        /* handle redirection of stdout and stderr to the log */
        if (sklogRedirectStandardStreams(errbuf, sizeof(errbuf))) {
            PRINT_AND_LOG(("%s", errbuf));
            goto ERROR;
        }
    }

    /* Send all error messages to the log */
    skAppSetFuncPrintErr(&WARNINGMSG_v);
    skAppSetFuncPrintSyserror(&WARNINGMSG_v);
    skAppSetFuncPrintFatalErr(&CRITMSG);

    /* Success! */
    if (skdaemon->no_daemon) {
        return 1;
    } else {
        return 0;
    }

  ERROR:
    skdaemonTeardown();
    return -1;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
