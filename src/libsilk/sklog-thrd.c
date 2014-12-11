/*
** Copyright (C) 2007-2014 by Carnegie Mellon University.
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
**  Create a version of sklog that works well in a multi-threaded
**  environment by creating a mutex and using it when logging.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: sklog-thrd.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/sklog.h>


/* Mutex for the log in the non-syslog case. */
static pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;


static void
preforkLock(
    void)
{
    pthread_mutex_lock(&logmutex);
}

static void
postforkUnlockParent(
    void)
{
    pthread_mutex_unlock(&logmutex);
}

static void
postforkUnlockChild(
    void)
{
    int count;

    /*
     *  HACK ALLERT!!  On OSX 10.7 (Lion), when the log file is
     *  extremely busy, the first call to pthread_mutex_unlock()
     *  does not seem to work.  We use the following do/while loop
     *  to ensure the log is unlocked.
     *
     *  Of course, this assumes that once we unlock the mutex we
     *  inherited across the fork() and get a new lock with trylock(),
     *  we can successfully unlock the mutex.
     */
    count = 0;
    do {
        ++count;
        pthread_mutex_unlock(&logmutex);
    } while (pthread_mutex_trylock(&logmutex));

    pthread_mutex_unlock(&logmutex);
    if (count > 1) {
        DEBUGMSG("Required %d attempts for child to release log mutex",
                 count);
    }
}


int
sklogEnableThreadedLogging(
    void)
{
    /* When we fork, ensure the child gets an unlocked handle to
     * log. */
    pthread_atfork(&preforkLock, &postforkUnlockParent, &postforkUnlockChild);

    /* Set the lock/unlock function pointers on the log and the mutex
     * on which they operate. */
    return sklogSetLocking((sklog_lock_fn_t)&pthread_mutex_lock,
                           (sklog_lock_fn_t)&pthread_mutex_unlock,
                           (sklog_lock_fn_t)&pthread_mutex_trylock,
                           &logmutex);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
