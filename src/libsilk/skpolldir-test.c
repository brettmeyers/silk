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
**  Simple tester for the skpolldir library
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skpolldir-test.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skpolldir.h>
#include <silk/sklog.h>
#include <silk/utils.h>


static skPollDir_t *pd = NULL;


/*
 *  appHandleSignal(signal_value)
 *
 *    Stop polling the directory
 */
static void
appHandleSignal(
    int          UNUSED(sig))
{
    if (pd) {
        skPollDirStop(pd);
    }
}


/*
 *    Prefix any error messages from skpolldir with the program name
 *    and an abbreviated time instead of the standard logging tag.
 */
static size_t
logprefix(
    char               *buffer,
    size_t              bufsize)
{
    time_t t;
    struct tm ts;

    t = time(NULL);
    localtime_r(&t, &ts);

    return (size_t)snprintf(buffer, bufsize, "%s: %2d:%02d:%02d: ",
                            skAppName(), ts.tm_hour, ts.tm_min, ts.tm_sec);
}


int main(int argc, char **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);
    const char *dirname;
    uint32_t interval = 5;
    char path[PATH_MAX];
    char *file;

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);

    /* make certain there are enough args.  If first arg begins with
     * hyphen, print usage. */
    if (argc < 2 || argc > 3 || argv[1][0] == '-') {
        fprintf(stderr, "Usage: %s <dirname> [<poll-interval>]\n",
                skAppName());
        return EXIT_FAILURE;
    }

    /* get directory to poll */
    dirname = argv[1];
    if (!skDirExists(dirname)) {
        skAppPrintErr("Polling dir '%s' does not exist", dirname);
        return EXIT_FAILURE;
    }

    /* get interval if given */
    if (argc == 3) {
        int rv = skStringParseUint32(&interval, argv[2], 1, 0);
        if (rv != 0) {
            skAppPrintErr("Invalid interval '%s': %s",
                          argv[2], skStringParseStrerror(rv));
            return EXIT_FAILURE;
        }
    }

    /* set signal handler to clean up temp files on SIGINT, SIGTERM, etc */
    if (skAppSetSignalHandler(&appHandleSignal)) {
        exit(EXIT_FAILURE);
    }

    /* Must enable the logger */
    sklogSetup(0);
    sklogSetDestination("stderr");
    sklogSetLevel("debug");
    sklogSetStampFunction(&logprefix);
    sklogOpen();

    pd = skPollDirCreate(dirname, interval);
    if (pd == NULL) {
        skAppPrintErr("Failed to set up polling for directory %s", dirname);
        return EXIT_FAILURE;
    }

    printf("%s: Polling '%s' every %" PRIu32 " seconds\n",
           skAppName(), dirname, interval);
    while (PDERR_NONE == skPollDirGetNextFile(pd, path, &file)) {
        printf("%s\n", file);
    }

    skPollDirDestroy(pd);
    pd = NULL;

    /* set level to "warning" to avoid the "Stopped logging" message */
    sklogSetLevel("warning");
    sklogTeardown();
    skAppUnregister();

    return EXIT_SUCCESS;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
