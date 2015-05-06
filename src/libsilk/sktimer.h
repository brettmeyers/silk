/*
** Copyright (C) 2004-2015 by Carnegie Mellon University.
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
**  sktimer.h
**
**    Generic timers which will run callback functions in a separate
**    thread context after a given amount of time.
**
*/
#ifndef _SKTIMER_H
#define _SKTIMER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKTIMER_H, "$SiLK: sktimer.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/silk_types.h>

/**
 *  @file
 *
 *    Implemention of a timer.
 *
 *    This file is part of libsilk-thrd.
 *
 *    Each timer runs in a separate thread.  The timer invokes the
 *    specified callback functions after a given amount of time unless
 *    the timer is destroyed before the timeout occurs.  The return
 *    status of the callback specifies whether the timer should repeat
 *    or end.
 */


/**
 *    The return type of 'skTimer_fn's.  This value indicates whether
 *    the timer should stop, or repeat.
 */
typedef enum {
    SK_TIMER_END, SK_TIMER_REPEAT
} skTimerRepeat_t;

/**
 *    The type of callback functions for timers.  The function will be
 *    called after the timer has waited for the desired duration.  The
 *    'client_data' value passed into the timer creation function will
 *    be passed to the callback.  The return value of the callback is
 *    of type skTimerContinue_t and determines whether the timer
 *    repeats or not.
 */
typedef skTimerRepeat_t (*skTimerFn_t)(
    void       *client_data);


/**
 *    The type of timer handles.
 */
typedef struct sk_timer_st *skTimer_t;


/**
 *    Creates a timer.  The timer starts at time 'start'.  After the
 *    number of seconds determined by the parameter 'secs' has passed,
 *    the callback function 'call_back' will be executed with
 *    'client_data' passed as an argument to it.
 *
 *    Based on the return value of the callback, the timer will repeat
 *    or stop.  The timer handle is passed back as the 'timer' value.
 *    Returns zero on success, non-zero on failure.
 */
int
skTimerCreateAtTime(
    skTimer_t          *timer,
    uint32_t            secs,
    sktime_t            start,
    skTimerFn_t         callback,
    void               *client_data);

/**
 *    Creates a timer.  The timer starts immediately after creation.
 *    After the number of seconds determined by the parameter 'secs'
 *    has passed, the callback function 'call_back' will be executed
 *    with 'client_data' passed as an argument to it.
 *
 *    Based on the return value of the callback, the timer will repeat
 *    or stop.  The timer handle is passed back as the 'timer' value.
 *    Returns zero on success, non-zero on failure.
 */
int
skTimerCreate(
    skTimer_t          *timer,
    uint32_t            secs,
    skTimerFn_t         callback,
    void               *client_data);


/**
 *    Stops and destroys a timer. Returns zero on success, non-zero on
 *    failure.  Does nothing if 'timer' is NULL.
 */
int
skTimerDestroy(
    skTimer_t           timer);

#ifdef __cplusplus
}
#endif
#endif /* _SKTIMER_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
