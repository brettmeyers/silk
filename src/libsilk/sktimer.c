/*
** Copyright (C) 2004-2014 by Carnegie Mellon University.
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
**  Generic timers which will run callback functions in a separate
**  thread context after a given amount of time.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: sktimer.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>
#include <silk/sktimer.h>
#include <silk/skthread.h>

#ifdef SKTIMER_TRACE_LEVEL
#define TRACEMSG_LEVEL 1
#endif
#define TRACEMSG(x)  TRACEMSG_TO_TRACEMSGLVL(1, x)
#include <silk/sktracemsg.h>


/* LOCAL DEFINES AND TYPEDEFS */

typedef struct sk_timer_st {
    /* function to call when timer fires and data to pass to it */
    skTimerFn_t      callBack;
    void            *clientData;

    /* protect/signal the timer */
    pthread_mutex_t  mutex;
    pthread_cond_t   cond;
    pthread_cond_t   end;

    /* time to use so timer fires at predictable times  */
    struct timeval   base_time;

    /* how often the timer should fire */
    int64_t          interval;

    /* whether timer is active */
    unsigned         running : 1;
} sk_timer_t;


/* FUNCTION DEFINITIONS */

/* Thread entry point */
static void *
sk_timer_thread(
    void               *v_timer)
{
    sk_timer_t      *timer = (sk_timer_t *)v_timer;
    int              rv;
    skTimerRepeat_t  repeat;

    /* wait_time is when the timer is scheduled to fire */
    struct timespec  wait_time;

    /* next_time is one interval ahead of wait_time */
    struct timeval   next_time;

    /* current_time is now. */
    struct timeval   current_time;

    /* Lock the mutex */
    pthread_mutex_lock(&timer->mutex);

    /* Have we been asked to be destroyed before we've even started? */
    if (!timer->running) {
        goto end;
    }

    /* We do no calculations with fractional seconds in this function;
     * simply initialize the wait_time fractional seconds from the
     * fractional seconds on the base_time */
    wait_time.tv_nsec = timer->base_time.tv_usec * 1000;

    /* Initialize next_time to the base_time */
    next_time = timer->base_time;

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    do {
        /* Skip to the next interval greater than the current time. */
        gettimeofday(&current_time, NULL);
        if (next_time.tv_sec < current_time.tv_sec) {
            int64_t seconds_into_interval
                = ((int64_t)(current_time.tv_sec - timer->base_time.tv_sec)
                   % timer->interval);
            next_time.tv_sec = (current_time.tv_sec + timer->interval
                                - seconds_into_interval);
        }

        wait_time.tv_sec = next_time.tv_sec;
        next_time.tv_sec += timer->interval;

        TRACEMSG((("Timer wait_time is %" PRId64 ".%09" PRId64),
                  (int64_t)wait_time.tv_sec, (int64_t)wait_time.tv_nsec));

        /* Loop around pthread_cond_timedwait() forever until the
         * timer actually fires or the condition variable is
         * signaled (for example, during shutdown) */
        for (;;) {
            /* Mutex is released while within pthread_cond_timedwait */
            rv = pthread_cond_timedwait(&timer->cond, &timer->mutex,
                                        &wait_time);
            if (ETIMEDOUT == rv) {
                /* Timer timed out */
#ifdef CHECK_PTHREAD_COND_TIMEDWAIT
                /* Well, at least the timer thinks it timed out.
                 * Let's see whether it did....
                 *
                 * THIS IS A HORRIBLE HACK!!  BACKGROUND: We have seen
                 * instances where pthread_cond_timedwait() will fire
                 * before it is supposed to.  Since we only have
                 * second resolution in the log, it may be firing a
                 * full second early or it may firing a few
                 * milliseconds early.  This shouldn't really matter,
                 * but the early timedwait() return is causing flowcap
                 * files to get timestamps that are a second earlier
                 * than expected, which means that the --clock-time
                 * switch isn't operating as advertised.  */
                struct timeval now;
                gettimeofday(&now, NULL);
                if (now.tv_sec < wait_time.tv_sec) {
                    /* Timer fired early.  Call timedwait() again.
                     * timedwait() may return immediately, so this has
                     * the potential to spike the CPU until the
                     * wait_time is reached. */
                    TRACEMSG((("Timer pthread_cond_timedwait() fired %" PRId64
                               " nanoseconds early"),
                              (((int64_t)(wait_time.tv_sec - now.tv_sec)
                                * 1000000000)
                               + (int64_t)wait_time.tv_nsec
                               - ((int64_t)now.tv_usec * 1000))));
                    continue;
                }
                /* else timer fired at correct time (or later) */
#endif  /* CHECK_PTHREAD_COND_TIMEDWAIT */

                repeat = timer->callBack(timer->clientData);
                break;
            }
            if (0 == rv) {
                /* pthread_cond_broadcast() was called */
                repeat = SK_TIMER_END;
                break;
            }
            /* else, a signal interrupted the call; continue waiting */
            TRACEMSG(("Timer pthread_cond_timedwait() returned unexpected %d",
                      rv));
        }

    } while (SK_TIMER_REPEAT == repeat);

    timer->running = 0;

  end:
    /* Notify destroy function that we have ended properly. */
    pthread_cond_broadcast(&timer->end);
    pthread_mutex_unlock(&timer->mutex);

    return NULL;
}


int
skTimerCreate(
    sk_timer_t        **new_timer,
    uint32_t            interval,
    skTimerFn_t         callBack,
    void               *callBackData)
{
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    current_time.tv_sec += interval;
    return skTimerCreateAtTime(new_timer, interval,
                               sktimeCreateFromTimeval(&current_time),
                               callBack, callBackData);
}


int
skTimerCreateAtTime(
    sk_timer_t        **new_timer,
    uint32_t            interval,
    sktime_t            start,
    skTimerFn_t         callBack,
    void               *callBackData)
{
    sk_timer_t *timer;
    pthread_t   thread;
    int         err;

#ifdef SKTIMER_TRACE_LEVEL
    char tstamp[SKTIMESTAMP_STRLEN];
    TRACEMSG((("Creating timer interval=%" PRIu32 ", start_time=%s"),
              interval, sktimestamp_r(tstamp, start, 0)));
#endif

    timer = (sk_timer_t *)malloc(sizeof(sk_timer_t));
    timer->interval = (int64_t)interval;
    timer->callBack = callBack;
    timer->clientData = callBackData;
    timer->running = 1;
    timer->base_time.tv_sec = sktimeGetSeconds(start);
    timer->base_time.tv_usec = sktimeGetMilliseconds(start) * 1000;
    pthread_mutex_init(&timer->mutex, NULL);
    pthread_cond_init(&timer->cond, NULL);
    pthread_cond_init(&timer->end, NULL);

    /* Mutex starts locked */
    pthread_mutex_lock(&timer->mutex);
    err = skthread_create_detached("sktimer", &thread, sk_timer_thread,
                                   (void *)timer);
    if (err) {
        pthread_mutex_unlock(&timer->mutex);
        pthread_mutex_destroy(&timer->mutex);
        pthread_cond_destroy(&timer->cond);
        pthread_cond_destroy(&timer->end);
        free(timer);
        return err;
    }

    pthread_mutex_unlock(&timer->mutex);
    *new_timer = timer;
    return 0;
}


int
skTimerDestroy(
    sk_timer_t         *timer)
{
    /* Grab the mutex */
    pthread_mutex_lock(&timer->mutex);
    if (timer->running) {
        timer->running = 0;
        /* Broadcast a stop to the timer */
        pthread_cond_broadcast(&timer->cond);
        /* Wait for timer process to end */
        pthread_cond_wait(&timer->end, &timer->mutex);
    }
    /* Unlock and destroy mutexes */
    pthread_mutex_unlock(&timer->mutex);
    pthread_mutex_destroy(&timer->mutex);
    pthread_cond_destroy(&timer->cond);
    pthread_cond_destroy(&timer->end);
    free(timer);
    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
