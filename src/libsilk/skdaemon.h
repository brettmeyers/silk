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
**  skdaemon.h
**
**    Setup logging, create a pid file, install a signal handler and
**    fork an application in order to run it as a daemon.
**
*/
#ifndef _SKDAEMON_H
#define _SKDAEMON_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKDAEMON_H, "$SiLK: skdaemon.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

/**
 *  @file
 *
 *    Functions for daemonizing a SiLK application.
 *
 *    This file is part of libsilk.
 */


/**
 *    By default, skdaemonize() will cause the application to fork,
 *    though the user's --no-daemon option can override that behavior.
 *    When this function is called, the application will not fork,
 *    regardless of the user's --no-daemon option.
 */
void
skdaemonDontFork(
    void);


/**
 *    Write the usage strings for options that skdaemonSetup() added
 *    to the global list of options.
 */
void
skdaemonOptionsUsage(
    FILE               *fh);


/**
 *    Verify that all the required options were specified and that
 *    their values are valid.
 */
int
skdaemonOptionsVerify(
    void);


/**
 *    Register the options used when running as a daemon.  The
 *    'log_features' value will be passed to sklogSetup().
 *
 *    The 'argc' and 'argv' contain the commmand line used to start
 *    the program.  They will be written to the log.
 */
int
skdaemonSetup(
    int                 log_features,
    int                 argc,
    char   * const     *argv);


/**
 *    Stop logging and remove the PID file.
 */
void
skdaemonTeardown(
    void);


/**
 *    In the general case: start the logger, fork the application,
 *    register the specified 'exit_handler', create a pid file, and
 *    install a signal handler in order to run an application as a
 *    daemon.  When the signal handler is called, it will set
 *    'shutdown_flag' to a non-zero value.
 *
 *    The application will not fork if the user requested --no-daemon.
 *
 *    Returns 0 if the application forked and everything succeeds.
 *    Returns 1 if everything succeeds but the application did not
 *    fork.  Returns -1 to indicate an error.
 */
int
skdaemonize(
    volatile int       *shutdown_flag,
    void                (*exit_handler)(void));

#ifdef __cplusplus
}
#endif
#endif /* _SKDAEMON_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
