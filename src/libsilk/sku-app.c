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
**  sku-app.c
**
**    a collection of utility routines for dealing with the
**    application's setup and printing of errors.
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: sku-app.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/utils.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/skheader.h>


/* TYPEDEFS AND DEFINES */

typedef struct skAppContext_st skAppContext_t;
/*
 *    Struct that contains global information about the application:
 *    its name, full path, options.
 */

struct skAppContext_st {
    /* complete pathname to the application */
    char                name_fullpath[PATH_MAX];
    /* argv[0] used to invoke application */
    const char         *name_argv0;
    /* basename of the application, pointer into 'name_argv0' */
    const char         *name_short;
    /* pointer into 'name_fullpath' that points at the last char in
     * the parent directory of the directory containing the app.  For
     * example, this points to the '/' after "local" in
     * "/usr/local/bin/rwfilter" */
    const char         *parent_lastchar;
    /* where to send errors */
    FILE               *err_stream; /* stderr for now */
    /* function used by skAppPrintErr() */
    sk_msg_vargs_fn_t   err_print_fn;
    /* function used by skAppPrintSyserror() */
    sk_msg_vargs_fn_t   errsys_print_fn;
    /* function used by skAppPrintAbort*() functions */
    sk_msg_fn_t         fatal_print_fn;
};


/* LOCAL VARIABLES */

static const char unregistered_app_name[] = "UNREGISTERED-APPLICATION";

static skAppContext_t app_context_static = {
    /* app's full path */     "",
    /* argv[0] */             unregistered_app_name,
    /* app's short name */    unregistered_app_name,
    /* parent dir ends */     NULL,
    /* error stream */        NULL,
    /* print err function */  &skAppPrintErrV,
    /* print err function */  &skAppPrintSyserrorV,
    /* print exit function */ &skAppPrintErr
};

static skAppContext_t *app_context = &app_context_static;


/* FUNCTION DEFINITONS */

#ifdef SK_PAUSE_AT_EXIT
static void
skAppPauseAtExit(
    void)
{
    skAppPrintErr("Pausing during shutdown...");
    pause();
}
#endif


void
skAppRegister(
    const char         *name)
{
    static char static_name_argv0[PATH_MAX];
    const char *libtool_prefix = "lt-";
    char *cp;

    if (app_context->name_argv0
        && (app_context->name_argv0 != unregistered_app_name))
    {
        /* been here before */
        return;
    }

    /* copy 'name' parameter into static buffer */
    strncpy(static_name_argv0, name, sizeof(static_name_argv0));
    static_name_argv0[sizeof(static_name_argv0) - 1] = '\0';

    app_context->name_argv0 = static_name_argv0;
    cp = strrchr(name, '/');
    if ( cp ) {
        app_context->name_short = cp+1;
    } else {
        app_context->name_short = name;
    }

    /* work around the fact that, when running in the build tree,
     * libtool may prefix the command name with "lt-" */
    if ((strlen(app_context->name_short) > strlen(libtool_prefix))
        && (0 == strncmp(app_context->name_short, libtool_prefix,
                         strlen(libtool_prefix))))
    {
        app_context->name_short += strlen(libtool_prefix);
    }

#ifdef SK_PAUSE_AT_EXIT
    if (atexit(&skAppPauseAtExit) == -1) {
        perror("Unable to add 'skAppPauseAtExit' to atexit");
    }
#endif

#if 0
    {
        /* redirect stderr to stdout; set mystderr to the real stderr */

        extern FILE *mystderr;
        int fd;

        mystderr = NULL;

        /* create new handle to stderr */
        fd = dup(fileno(stderr));
        if (fd != -1) {
            mystderr = fdopen(fd, "w");
        }
        if (mystderr == NULL) {
            fprintf(stderr, "Unable to dup stderr: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        /* redirect stderr to stdout */
        fd = dup2(fileno(stdout), fileno(stderr));
        if (fd == -1) {
            fprintf(stderr, "Unable to dup stdout: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
#endif /* 0 */

    app_context->err_stream = stderr;

    sksiteInitialize(0);
    skOptionsSetup();
    skHeaderInitialize();
    skStreamInitialize();
}


void
skAppUnregister(
    void)
{
    skStreamTeardown();
    skHeaderTeardown();
    sksiteTeardown();
    skOptionsTeardown();
}


const char *
skAppName(
    void)
{
    return app_context->name_short;
}


const char *
skAppRegisteredName(
    void)
{
    return app_context->name_argv0;
}


void
skAppUsage(
    void)
{
    FILE *fh = app_context->err_stream;
    if (fh != NULL) {
        fprintf(fh, "Use '%s --help' for usage\n", skAppName());
    }
    skAppUnregister();
    exit(EXIT_FAILURE);
}


void
skAppStandardUsage(
    FILE                   *fh,
    const char             *usage_msg,
    const struct option    *app_options,
    const char            **app_help)
{
    int i;

    fprintf(fh, "%s %s", skAppName(), usage_msg);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    if (app_options) {
        for (i = 0; app_options[i].name; i++ ) {
            if (app_help[i]) {
                fprintf(fh, "--%s %s. %s\n", app_options[i].name,
                        SK_OPTION_HAS_ARG(app_options[i]), app_help[i]);
            }
        }
    }
}


/*
 * skAppFullPathname()
 *
 *      Return the full path of the executable.  Caller should treat
 *      the result as read-only.
 */
const char *
skAppFullPathname(
    void)
{
    size_t fullpath_len = sizeof(app_context->name_fullpath);
    size_t appname_len;
    char *cp1, *cp2;
    size_t len;

    if (app_context->name_fullpath[0] != '\0') {
        goto END_OK;
    }
    if (NULL == app_context->name_argv0) {
        goto END_ERROR;
    }

    appname_len = strlen(app_context->name_argv0);
    app_context->name_fullpath[0] = '\0';

    if (appname_len >= fullpath_len) {
        skAppPrintErr(("skAppFullPathname: fullpath(%lu) too small "
                       "for app(%lu) '%s'"),
                      (unsigned long)fullpath_len, (unsigned long)appname_len,
                      app_context->name_argv0);
        goto END_ERROR;
    }

    if (app_context->name_argv0[0] == '/') {
        /* an absolute path */
        if (skFileExists(app_context->name_argv0)) {
            strncpy(app_context->name_fullpath, app_context->name_argv0,
                    appname_len+1);
            goto END_OK;
        }
    }

    if (strchr(app_context->name_argv0, '/') == (char *)NULL ) {
        /* no path at all. Try all directories in $PATH */
        cp1 = getenv("PATH");
        if (!cp1) {
            skAppPrintErr("No $PATH");
            goto END_ERROR;
        }
        /* printf("looking along PATH %s\n", cp1); */
        while (cp1) {
            cp2 = strchr(cp1, ':');
            if (cp2) {
                len = cp2-cp1;
                cp2++;
            } else {
                len = strlen(cp1);
            }
            if (len + appname_len + 2 < fullpath_len) {
                strncpy(app_context->name_fullpath, cp1, len);
                app_context->name_fullpath[len] = '/';
                strncpy(&(app_context->name_fullpath[len+1]),
                        app_context->name_argv0, appname_len+1);
                /* printf("looking for %s\n", app_context->name_fullpath); */
                if (skFileExists(app_context->name_fullpath)) {
                    goto END_OK;
                }
            }
            cp1 = cp2;
        }
    }

    /*
     * neither an absolute path nor on  $PATH. Must be a relative path with
     * ./ or ../ in it
     */
    if (!getcwd(app_context->name_fullpath, fullpath_len)) {
        perror(__FILE__ " skAppFullPathname (getcwd)");
        goto END_ERROR;
    }
    len = strlen(app_context->name_fullpath);
    if (len + appname_len + 2 < fullpath_len) {
        app_context->name_fullpath[len] = '/';
        strncpy(&(app_context->name_fullpath[len+1]), app_context->name_argv0,
                appname_len+1);
        if (skFileExists(app_context->name_fullpath)) {
            goto END_OK;
        }
    }

    /* counldn't find it anywhere! */
    skAppPrintErr("%s not found anywhere", app_context->name_argv0);
  END_ERROR:
    app_context->name_fullpath[0] = '\0';
    return (char *)NULL;

  END_OK:
    return (const char*)app_context->name_fullpath;
}


#if 0
char *
skAppDirShare(
    char               *buf,
    size_t              buf_len)
{
    if (app_context->dir_share) {
        len = strlen app_context->dir_share;
        if (0 == len) {
            /* empty string signifies that we looked for the "share"
             * directory but didn't find it
             */
            return NULL;
        }
        if (len >= buf_len) {
            /* not enough space */
            return NULL;
        }
        strcpy(buf, app_context->dir_share);
        return buf;
    }

    return NULL;
}
#endif


/*
 *  skAppDirParentDir(buf, buf_len)
 *
 *  Return the application's directory's parent directory in buf, a
 *  character array of buf_len bytes. e.g., if the rwfilter
 *  application lives in "/usr/local/bin/rwfilter", this function puts
 *  "/usr/local" into buf.  Return value is a pointer to buf, or NULL
 *  on error.
 *
 *  Depending on your point of view, this may or may not handle
 *  symbolic links correctly.  Suppose /usr/local/bin/rwfilter is a
 *  symbolic link to /home/silk/bin/rwfilter.  This function will
 *  treat the application's parent dir as "/usr/local".
 */
char *
skAppDirParentDir(
    char               *buf,
    size_t              buf_len)
{
    /* Once we store the application's full path in
     * app_context->name_fullpath, we store the last character of the
     * parent directory in parent_lastchar; then filling the buffer the
     * caller handed us is a simple strncpy()
     */
    const char *app_path = app_context->name_fullpath;
    size_t app_length;
    const char *endp = app_context->parent_lastchar;

    buf[0] = '\0';

    if (NULL == endp) {
        /* make certain we have the full path to app */
        if (('\0' == app_path[0])&&((app_path = skAppFullPathname()) == NULL)){
            return NULL;
        }

        /* Consider that app_path contains "/usr/local/bin/rwfilter".
         * Move endp backwards over the "rwfilter" token */
        endp = strrchr(app_path, '/');
        if (!endp) {
            skAppPrintErr("Cannot find parent dir of '%s'", app_path);
            return NULL;
        }

        do {
            /* app_path could contain "/usr/local/bin///////".  Move endp
             * backward until we find a non-slash. */
            while ((endp > app_path) && ('/' == *endp)) {
                --endp;
            }
            if ('/' == *endp) {
                /* The total app_path was "/rwfilter"?
                 * Are we running from root? */
                skAppPrintErr("Cannot find parent dir of '%s'", app_path);
                return NULL;
            }

            /* app_path[0] to app_path[end_p] contains "/usr/local/bin".  Move
             * endp backwards over the "bin" token */
            while ((endp > app_path) && ('/' != *endp)) {
                --endp;
            }
            if ('/' != *endp) {
                /* something strange: app_name was "bin/rwfilter".  Note no
                 * leading '/' char */
                skAppPrintErr("Cannot find parent dir of '%s'", app_path);
                return NULL;
            }
        } while (0 == strncmp(endp, "/./", 3));

        /* Once again back over '/' characters */
        while ((endp > app_path) && ('/' == *endp)) {
            --endp;
        }
        /* Put endp on the final '/' */
        ++endp;
        app_context->parent_lastchar = endp;
    }

    /* strcpy from app_name[0] to app_name[end_p] into buf */
    app_length = endp - app_path;
    if (app_length+1 > buf_len) {
        return NULL;
    }
    strncpy(buf, app_path, app_length);
    buf[app_length] = '\0';
    return buf;
}


void
skAppVerifyFeatures(
    const silk_features_t          *app_features,
    void                    UNUSED(*future_use))
{
#define FEATURE_ERROR_MSG                                                \
    ("Application/Library mismatch for feature %s\n"                     \
     "\tThe set of features enabled in the SiLK library are different\n" \
     "\tthan those enabled in this application.  All of SiLK must be\n"  \
     "\tconfigured, built, and installed with the same settings.\n"      \
     "\tRebuild from a clean source tree and reinstall.")

#define FEATURE_COMPARE(fc_feat)                                \
    if (silk_features.fc_feat != app_features->fc_feat) {       \
        skAppPrintErr(FEATURE_ERROR_MSG, #fc_feat);             \
        exit(EXIT_FAILURE);                                     \
    }

    SILK_FEATURES_DEFINE_STRUCT(silk_features);

    FEATURE_COMPARE(struct_version);
    FEATURE_COMPARE(big_endian);
    FEATURE_COMPARE(enable_ipv6);
    FEATURE_COMPARE(enable_ipfix);
    FEATURE_COMPARE(enable_localtime);
    FEATURE_COMPARE(enable_silk3_ipsets);
    /* FEATURE_COMPARE(enable_gnutls); */
}


FILE *
skAppSetErrStream(
    FILE               *f)
{
    FILE *old_stream = app_context->err_stream;
    app_context->err_stream = f;
    return old_stream;
}


void
skAppSetFuncPrintErr(
    sk_msg_vargs_fn_t   fn)
{
    if (NULL == fn) {
        app_context->err_print_fn = &skAppPrintErrV;
    } else {
        app_context->err_print_fn = fn;
    }
}


void
skAppSetFuncPrintSyserror(
    sk_msg_vargs_fn_t   fn)
{
    if (NULL == fn) {
        app_context->errsys_print_fn = &skAppPrintSyserrorV;
    } else {
        app_context->errsys_print_fn = fn;
    }
}


void
skAppSetFuncPrintFatalErr(
    sk_msg_fn_t         fn)
{
    if (fn != NULL) {
        app_context->fatal_print_fn = &skAppPrintErr;
    } else {
        app_context->fatal_print_fn = fn;
    }
}


int
skMsgNone(
    const char  UNUSED(*msg),
    ...)
{
    return 0;
}


int
skMsgNoneV(
    const char  UNUSED(*msg),
    va_list      UNUSED(args))
{
    return 0;
}


int
skAppPrintErrV(
    const char         *fmt,
    va_list             args)
{
    int rv = 0;
    if (NULL == app_context->err_stream) {
        return 0;
    }
    rv += fprintf(app_context->err_stream, "%s: ", app_context->name_short);

SK_DIAGNOSTIC_FORMAT_NONLITERAL_PUSH

    rv += vfprintf(app_context->err_stream, fmt, args);

SK_DIAGNOSTIC_FORMAT_NONLITERAL_POP

    rv += fprintf(app_context->err_stream, "\n");
    return rv;
}


int
skAppPrintSyserrorV(
    const char         *fmt,
    va_list             args)
{
    int rv = 0;
    int cache_errno = errno;

    if (NULL == app_context->err_stream) {
        return 0;
    }

    rv += fprintf(app_context->err_stream, "%s: ", app_context->name_short);

SK_DIAGNOSTIC_FORMAT_NONLITERAL_PUSH

    rv += vfprintf(app_context->err_stream, fmt, args);

SK_DIAGNOSTIC_FORMAT_NONLITERAL_POP

    rv += fprintf(app_context->err_stream, ": %s\n", strerror(cache_errno));

    return rv;
}


#ifndef skAppPrintErr
/*
 *  Sometimes it's useful to
 *
 *    #define skAppPrintErr printf
 *
 *  to see where we've messed up our formatting.  When doing that, we
 *  obviously do not want to compile this function.
 */
int
skAppPrintErr(
    const char         *fmt,
    ...)
{
    va_list args;
    int rv;

    va_start(args, fmt);
    rv = app_context->err_print_fn(fmt, args);
    va_end(args);

    return rv;
}
#endif /* !defined skAppPrintErr */


#ifndef skAppPrintSyserror
/* Don't compile when TEST_PRINTF_FORMATS is specified */
int
skAppPrintSyserror(
    const char         *fmt,
    ...)
{
    va_list args;
    int rv;

    va_start(args, fmt);
    rv = app_context->errsys_print_fn(fmt, args);
    va_end(args);

    return rv;
}
#endif /* skAppPrintSyserror */


SK_DIAGNOSTIC_FORMAT_NONLITERAL_PUSH

int
skTraceMsg(
    const char         *fmt,
    ...)
{
    int rv;
    va_list args;

    va_start(args, fmt);
    rv = 1 + vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

    return rv;
}

SK_DIAGNOSTIC_FORMAT_NONLITERAL_POP


/* install a signal handler */
int skAppSetSignalHandler(void (*sig_handler)(int signal))
{
    /* list of handled signals */
    const char *names[] = {"INT", "PIPE", "QUIT", "TERM"};
    int sigs[] = {SIGINT, SIGPIPE, SIGQUIT, SIGTERM};
    struct sigaction act;
    size_t i;

    assert(sizeof(sigs)/sizeof(sigs[0]) == sizeof(names)/sizeof(names[0]));

    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif

    for (i = 0; i < (sizeof(sigs)/sizeof(int)); ++i) {
        if (sigaction(sigs[i], &act, NULL) < 0) {
            skAppPrintErr("Cannot register handler for SIG%s",
                          names[i]);
            return -1;
        }
    }
    return 0;
}


void
skAppPrintAbortMsg(
    const char         *func_name,
    const char         *file_name,
    int                 line_number)
{
#ifndef SK_HAVE_C99___FUNC__
    (void)func_name;
#endif
    if (NULL == app_context || NULL == app_context->fatal_print_fn) {
        return;
    }
    app_context->fatal_print_fn(
        ("Unexpected fatal error "
#ifdef SK_HAVE_C99___FUNC__
         "in %s() "
#endif
         "at %s:%d.\n"
         "\tPlease help us improve " SK_PACKAGE_NAME
         " by submitting a bug report to\n\t<" SK_PACKAGE_BUGREPORT
         "> and providing as much detail about\n"
         "\tthe events that caused this error as you can.  Thanks."),
#ifdef SK_HAVE_C99___FUNC__
        func_name,
#endif
        file_name, line_number);
}


void
skAppPrintBadCaseMsg(
    const char         *func_name,
    const char         *file_name,
    int                 line_number,
    int64_t             value,
    const char         *value_expr)
{
#ifndef SK_HAVE_C99___FUNC__
    (void)func_name;
#endif
    if (NULL == app_context || NULL == app_context->fatal_print_fn) {
        return;
    }
    app_context->fatal_print_fn(
        ("Unexpected switch(%s) value %" PRId64 "\n\t"
#ifdef SK_HAVE_C99___FUNC__
         "in %s() "
#endif
         "at %s:%d.\n"
         "\tPlease help us improve " SK_PACKAGE_NAME
         " by submitting a bug report to\n\t<" SK_PACKAGE_BUGREPORT
         "> and providing as much detail about\n"
         "\tthe events that caused this error as you can.  Thanks."),
        value_expr, value,
#ifdef SK_HAVE_C99___FUNC__
        func_name,
#endif
        file_name, line_number);
}


void
skAppPrintOutOfMemoryMsgFunction(
    const char         *func_name,
    const char         *file_name,
    int                 line_number,
    const char         *object_name)
{
#ifndef SK_HAVE_C99___FUNC__
    (void)func_name;
#endif
    if (NULL == app_context || NULL == app_context->fatal_print_fn) {
        return;
    }
    if (object_name) {
        app_context->fatal_print_fn(
            ("Out of memory---unable to allocate %s "
#ifdef SK_HAVE_C99___FUNC__
             "in %s() "
#endif
             "at %s:%d."),
            object_name,
#ifdef SK_HAVE_C99___FUNC__
            func_name,
#endif
            file_name, line_number);
    } else {
        app_context->fatal_print_fn(
            ("Out of memory "
#ifdef SK_HAVE_C99___FUNC__
             "in %s() "
#endif
             "at %s:%d."),
#ifdef SK_HAVE_C99___FUNC__
            func_name,
#endif
            file_name, line_number);
    }
}


#ifdef TEST_APPNAME

int main(int argc, char **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);
    char buf[PATH_MAX];

    skAppPrintErr("Oops!  I'm printing an error before registering!");

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    if (argc) {
        /* should have no effect */
        skAppRegister(argv[1]);
    }

    skAppPrintErr("Registered Name '%s'", skAppRegisteredName());
    skAppPrintErr("Short Name      '%s'", skAppName());
    skAppPrintErr("Parent Dir Dir  '%s'", skAppDirParentDir(buf, sizeof(buf)));
    skAppPrintErr("Full Path       '%s'", skAppFullPathname());

    skAppPrintErr("\nData Struct");
    skAppPrintErr("Registered Name '%s'", app_context->name_argv0);
    skAppPrintErr("Short Name      '%s'", app_context->name_short);
    skAppPrintErr("Parent Dir Dir  '%s'", app_context->parent_lastchar);
    skAppPrintErr("Full Path       '%s'", app_context->name_fullpath);

    skAppPrintErr("\nAnd Again...");
    skAppPrintErr("Registered Name '%s'", skAppRegisteredName());
    skAppPrintErr("Short Name      '%s'", skAppName());
    skAppPrintErr("Parent Dir Dir  '%s'", skAppDirParentDir(buf, sizeof(buf)));
    skAppPrintErr("Full Path       '%s'", skAppFullPathname());

    return 0;
}

#endif /* TEST_APPNAME */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
