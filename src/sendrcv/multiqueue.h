/*
** Copyright (C) 2007-2015 by Carnegie Mellon University.
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
**  multiqueue.h
**
**    Creates queue sets.
**
*/
#ifndef _MULTIQUEUE_H
#define _MULTIQUEUE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_MULTIQUEUE_H, "$SiLK: multiqueue.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/*
 *    Multiqueues are sets of subqueues that can work together as a
 *    single queue.  Subqueues can be added to and moved between
 *    multiqueues at runtime.
 */


/* Type of a multiqueue */
typedef struct mq_multi_st mq_multi_t;

/* Type of a multiqueue subqueue */
typedef struct mq_queue_st mq_queue_t;

typedef enum mq_err_en {
    MQ_NOERROR = 0,
    MQ_DISABLED,
    MQ_SHUTDOWN,
    MQ_MEMERROR,
    MQ_ILLEGAL
} mq_err_t;

typedef enum mq_function_en {
    MQ_ADD    = 1,
    MQ_REMOVE = (1 << 1) ,
    MQ_BOTH   = MQ_ADD | MQ_REMOVE
} mq_function_t;


mq_multi_t *
mqCreateUnfair(
    void              (*free_fn)(void *));
/*
 *    Create an unfair multiqueue.
 *
 *    An unfair multiqueue will drain all data from its first subqueue
 *    before draining elements from subsequent subqueues.
 *
 *    The 'free_fn' is a function that is used to free elements in the
 *    multiqueue's subqueues upon destruction of the multiqueue.  It
 *    may be NULL.
 */

mq_multi_t *
mqCreateFair(
    void              (*free_fn)(void *));
/*
 *    Create an fair multiqueue.
 *
 *    An fair multiqueue will drain data from its subqueues in a
 *    round-robin fashion.
 *
 *    The 'free_fn' is a function that is used to free elements in the
 *    multiqueue's subqueues upon destruction of the multiqueue.  It
 *    may be NULL.
 */

void
mqShutdown(
    mq_multi_t         *q);
/*
 *    Shutdown a multiqueue.
 *
 *    Shutting down a multiqueue unblocks all operations on that
 *    multiqueue, and makes the multiqueue unusable.  Generally a
 *    prelude to destroying a multiqueue.
 */

mq_err_t
mqDisable(
    mq_multi_t         *q,
    mq_function_t       which);
/*
 *    Disable a part of functionality of a multiqueue.
 *
 *    One can disable either the ability to add or remove elements
 *    from a multiqueue.  Disabling adding also disables addding to
 *    the multiqueue's subqueues and adding new queues to the
 *    multiqueue.  Disabling removing will unblock Get calls on that
 *    multiqueue (but not its subqueues).  Multiqueue functionality
 *    can be reinstated using mqEnable().
 */

mq_err_t
mqEnable(
    mq_multi_t         *q,
    mq_function_t       which);
/*
 *    Re-enable a part of functionality of a multiqueue.
 *
 *    Re-enables functionality of a multiqueue that has been
 *    previously disabled.
 */

void
mqDestroy(
    mq_multi_t         *q);
/*
 *    Destroys a multiqueue and all owned subqueues.
 *
 *    If the multiqueue was created with a non-null free_fn, this
 *    function will also call that free_fn on all elements in the
 *    multiqueue's subqueues.
 */

mq_err_t
mqGet(
    mq_multi_t         *q,
    void              **data);
/*
 *    Get an element from a multiqueue.
 *
 *    This will block if the multiqueue is empty.  Returns MQ_NOERROR
 *    on success.  Returns MQ_SHUTDOWN or MQ_DISABLED if the
 *    multiqueue was shutdown or disabled.
 */

mq_err_t
mqPushBack(
    mq_multi_t         *q,
    void               *data);
/*
 *    Puts an element back on a multiqueue, such that it will be the
 *    next element returned by an mqget() call.
 */

mq_queue_t *
mqCreateQueue(
    mq_multi_t         *q);
/*
 *    Creates and adds a subqueue to a multiqueue.
 */

void
mqDestroyQueue(
    mq_queue_t         *sq);
/*
 *    Destroys a subqueue.
 */

mq_err_t
mqQueueAdd(
    mq_queue_t         *sq,
    void               *data);
/*
 *    Adds an element to a subqueue.
 */

mq_err_t
mqQueueGet(
    mq_queue_t         *sq,
    void              **data);
/*
 *    Gets an element from a subqueue.
 */

mq_err_t
mqQueuePushBack(
    mq_queue_t         *sq,
    void               *data);
/*
 *    Puts an element back on a subqueue such that mqQueueGet() will
 *    return that element.
 */

mq_err_t
mqQueueMove(
    mq_multi_t         *q,
    mq_queue_t         *sq);
/*
 *    Moves a subqueue to a particular multiqueue.
 */

mq_err_t
mqQueueDisable(
    mq_queue_t         *q,
    mq_function_t       which);
/*
 *    Disable a part of functionality of a subqueue.
 *
 *    One can disable either the ability to add or remove elements
 *    from a subqueue.  Disabling removing will unblock Get calls on
 *    that subqueue.  Subqueue functionality can be reinstated using
 *    mqQueueEnable().
 */

mq_err_t
mqQueueEnable(
    mq_queue_t         *q,
    mq_function_t       which);
/*
 *    Re-enable a part of functionality of a subqueue.
 *
 *    Re-enables functionality of a subqueue that has been previously
 *    disabled.
 */

#ifdef __cplusplus
}
#endif
#endif /* _MULTIQUEUE_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
