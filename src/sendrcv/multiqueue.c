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
**  multiqueue.c
**
**    A multiqueue is a queue of queues.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: multiqueue.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/utils.h>
#include <silk/skdllist.h>
#include "multiqueue.h"
#include "libsendrcv.h"

/* SENDRCV_DEBUG is defined in libsendrcv.h */
#if (SENDRCV_DEBUG) & DEBUG_MULTIQUEUE_MUTEX
#  define SKTHREAD_DEBUG_MUTEX 1
#endif
#include <silk/skthread.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* Lock a subqueue, making sure that the subqueue is owned by the
 * locked parent multiqueue after the locking. */
#define SUBQUEUE_LOCK(sq)                       \
    do {                                        \
        for (;;) {                              \
            mq_multi_t *_mq = (sq)->multi;      \
            MUTEX_LOCK(&_mq->mutex);            \
            if (_mq == (sq)->multi) {           \
                break;                          \
            }                                   \
            MUTEX_UNLOCK(&_mq->mutex);          \
        }                                       \
    } while (0)

/* Unlocks a subqueue */
#define SUBQUEUE_UNLOCK(sq) MUTEX_UNLOCK(&(sq)->multi->mutex)


/*
 *  ASSERT_RESULT(ar_func_args, ar_type, ar_expected);
 *
 *    ar_func_args  -- is a function and any arguments it requires
 *    ar_type       -- is the type that ar_func_args returns
 *    ar_expected   -- is the expected return value from ar_func_args
 *
 *    If assert() is disabled, simply call 'ar_func_args'.
 *
 *    If assert() is enabled, call 'ar_func_args', capture its result,
 *    and assert() that its result is 'ar_expected'.
 */
#ifdef  NDEBUG
/* asserts are disabled; just call the function */
#define ASSERT_RESULT(ar_func_args, ar_type, ar_expected)  ar_func_args
#else
#define ASSERT_RESULT(ar_func_args, ar_type, ar_expected)       \
    do {                                                        \
        ar_type ar_rv = (ar_func_args);                         \
        assert(ar_rv == (ar_expected));                         \
    } while(0)
#endif


/* typedef struct mq_multi_st mq_multi_t;   // multiqueue.h  */
struct mq_multi_st {
    uint64_t        count;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    sk_dllist_t    *queues;     /* List of subqueues */
    void          (*free_fn)(void *);
    unsigned        disable_add    : 1;
    unsigned        disable_remove : 1;
    unsigned        shutdown       : 1;
    unsigned        fair           : 1;
};

/* typedef struct mq_queue_st mq_queue_t;   // multiqueue.h  */
struct mq_queue_st {
    uint64_t     count;
    sk_dllist_t *queue;         /* List of items */
    mq_multi_t  *multi;         /* Pointer to owning multiqueue */
    unsigned     disable_add    : 1;
    unsigned     disable_remove : 1;
};


/* FUNCTION DEFINITIONS */

static void
mqFreeQueue(
    void               *vq)
{
    mq_queue_t *q = (mq_queue_t *)vq;

    assert(q);
    skDLListDestroy(q->queue);
    free(q);
}

mq_multi_t *
mqCreateUnfair(
    void              (*free_fn)(void *))
{
    mq_multi_t *multi;

    multi = (mq_multi_t *)calloc(1, sizeof(*multi));
    if (multi == NULL) {
        return NULL;
    }
    multi->queues = skDLListCreate(mqFreeQueue);
    if (multi->queues == NULL) {
        free(multi);
        return NULL;
    }

    multi->free_fn = free_fn;
    pthread_mutex_init(&multi->mutex, NULL);
    pthread_cond_init(&multi->cond, NULL);

    return multi;
}

mq_multi_t *
mqCreateFair(
    void              (*free_fn)(void *))
{
    mq_multi_t *multi = mqCreateUnfair(free_fn);

    if (multi != NULL) {
        multi->fair = 1;
    }
    return multi;
}


void
mqShutdown(
    mq_multi_t         *q)
{
    assert(q);

    MUTEX_LOCK(&q->mutex);
    if (!q->shutdown) {
        MUTEX_BROADCAST(&q->cond);
        q->shutdown = 1;
    }
    MUTEX_UNLOCK(&q->mutex);
}


mq_err_t
mqDisable(
    mq_multi_t         *q,
    mq_function_t       which)
{
    assert(q);

    MUTEX_LOCK(&q->mutex);
    if (q->shutdown) {
        MUTEX_UNLOCK(&q->mutex);
        return MQ_SHUTDOWN;
    }

    if (which & MQ_ADD) {
        q->disable_add = 1;
    }
    if ((which & MQ_REMOVE) && !q->disable_remove) {
        q->disable_remove = 1;
        /* Unblock waiters on Get operations */
        MUTEX_BROADCAST(&q->cond);
    }
    MUTEX_UNLOCK(&q->mutex);

    return MQ_NOERROR;
}


mq_err_t
mqEnable(
    mq_multi_t         *q,
    mq_function_t       which)
{
    assert(q);

    MUTEX_LOCK(&q->mutex);
    if (q->shutdown) {
        MUTEX_UNLOCK(&q->mutex);
        return MQ_SHUTDOWN;
    }

    if ((which & MQ_ADD)) {
        q->disable_add = 0;
    }
    if ((which & MQ_REMOVE)) {
        q->disable_remove = 0;
    }
    MUTEX_UNLOCK(&q->mutex);

    return MQ_NOERROR;
}


void
mqDestroy(
    mq_multi_t         *q)
{
    assert(q);

    MUTEX_LOCK(&q->mutex);
    assert(q->shutdown);
    skDLListDestroy(q->queues);
    MUTEX_UNLOCK(&q->mutex);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);

    free(q);
}


mq_queue_t *
mqCreateQueue(
    mq_multi_t         *q)
{
    mq_queue_t *sq = NULL;
    int rv;

    assert(q);

    MUTEX_LOCK(&q->mutex);

    if (q->shutdown || q->disable_add) {
        goto end;
    }

    sq = (mq_queue_t *)calloc(1, sizeof(*sq));
    if (sq == NULL) {
        goto end;
    }

    sq->queue = skDLListCreate(q->free_fn);
    if (sq->queue == NULL) {
        free(sq);
        sq = NULL;
        goto end;
    }

    rv = skDLListPushHead(q->queues, sq);
    if (rv != 0) {
        skDLListDestroy(sq->queue);
        free(sq);
        sq = NULL;
        goto end;
    }

    sq->multi = q;

  end:
    MUTEX_UNLOCK(&q->mutex);
    return sq;
}


mq_err_t
mqQueueDisable(
    mq_queue_t         *sq,
    mq_function_t       which)
{
    assert(sq);

    SUBQUEUE_LOCK(sq);
    if (sq->multi->shutdown) {
        MUTEX_UNLOCK(&sq->multi->mutex);
        return MQ_SHUTDOWN;
    }
    if (which & MQ_ADD) {
        sq->disable_add = 1;
    }
    if ((which & MQ_REMOVE) && !sq->disable_remove) {
        sq->disable_remove = 1;
        MUTEX_BROADCAST(&sq->multi->cond);
    }
    SUBQUEUE_UNLOCK(sq);

    return MQ_NOERROR;
}


mq_err_t
mqQueueEnable(
    mq_queue_t         *sq,
    mq_function_t       which)
{
    assert(sq);

    SUBQUEUE_LOCK(sq);
    if (sq->multi->shutdown) {
        MUTEX_UNLOCK(&sq->multi->mutex);
        return MQ_SHUTDOWN;
    }
    if (which & MQ_ADD) {
        sq->disable_add = 0;
    }
    if (which & MQ_REMOVE) {
        sq->disable_remove = 0;
    }
    SUBQUEUE_UNLOCK(sq);

    return MQ_NOERROR;
}


void
mqDestroyQueue(
    mq_queue_t         *sq)
{
    mq_multi_t   *q;
    mq_queue_t   *found = NULL;
    sk_dll_iter_t iter;

    assert(sq);

    SUBQUEUE_LOCK(sq);
    q = sq->multi;

    skDLLAssignIter(&iter, q->queues);
    while (skDLLIterForward(&iter, (void **)&found) == 0) {
        assert(sq->multi == q);
        if (found == sq) {
            break;
        }
    }

    assert(found == sq);

    q->count -= sq->count;
    skDLListDestroy(sq->queue);
    ASSERT_RESULT(skDLLIterDel(&iter), int, 0);

    SUBQUEUE_UNLOCK(sq);

    free(sq);
}


static mq_err_t
mq_queue_add(
    mq_queue_t         *sq,
    void               *data,
    int                 back)
{
    mq_multi_t *q;
    mq_err_t    retval;
    int         rv;

    assert(sq);

    SUBQUEUE_LOCK(sq);
    q = sq->multi;

    if (q->shutdown) {
        retval = MQ_SHUTDOWN;
        goto end;
    }

    if (q->disable_add || sq->disable_add) {
        retval = MQ_DISABLED;
        goto end;
    }

    if (back) {
        rv = skDLListPushHead(sq->queue, data);
    } else {
        rv = skDLListPushTail(sq->queue, data);
    }
    if (rv != 0) {
        retval = MQ_MEMERROR;
        goto end;
    }

    retval = MQ_NOERROR;

    if (sq->count == 0) {
        MUTEX_BROADCAST(&q->cond);
    }
    sq->count++;
    q->count++;

  end:
    SUBQUEUE_UNLOCK(sq);
    return retval;
}


mq_err_t
mqQueueAdd(
    mq_queue_t         *sq,
    void               *data)
{
    return mq_queue_add(sq, data, 1);
}


mq_err_t
mqQueuePushBack(
    mq_queue_t         *sq,
    void               *data)
{
    return mq_queue_add(sq, data, 0);
}


mq_err_t
mqQueueGet(
    mq_queue_t         *sq,
    void              **data)
{
    mq_multi_t   *q;
    mq_queue_t   *found = NULL;
    mq_err_t      retval;
    sk_dll_iter_t iter;

    assert(sq);

    for (;;) {
        SUBQUEUE_LOCK(sq);
        q = sq->multi;

        if (q->shutdown) {
            retval = MQ_SHUTDOWN;
            goto end;
        }

        while (!q->shutdown && !sq->disable_remove && sq->count == 0) {
            MUTEX_WAIT(&q->cond, &q->mutex);
        }

        /* If this queue changed multi-queues, release the lock and
         * try again */
        if (q == sq->multi) {
            break;
        }
        MUTEX_UNLOCK(&q->mutex);
    }

    if (q->shutdown) {
        retval = MQ_SHUTDOWN;
        goto end;
    }

    if (sq->disable_remove) {
        retval = MQ_DISABLED;
        goto end;
    }

    skDLLAssignIter(&iter, q->queues);
    while (skDLLIterBackward(&iter, (void **)&found) == 0) {
        if (sq == found) {
            ASSERT_RESULT(skDLListPopTail(sq->queue, data), int, 0);
            sq->count--;
            q->count--;
            if (q->fair) {
                ASSERT_RESULT(skDLLIterDel(&iter), int, 0);
                ASSERT_RESULT(skDLListPushHead(q->queues, sq), int, 0);
            }
            break;
        }
    }

    assert(found == sq);

    retval = MQ_NOERROR;

  end:
    MUTEX_UNLOCK(&q->mutex);
    return retval;
}


mq_err_t
mqGet(
    mq_multi_t         *q,
    void              **data)
{
    mq_queue_t   *sq;
    sk_dll_iter_t iter;
    mq_err_t      retval = MQ_MEMERROR;

    assert(q);

    MUTEX_LOCK(&q->mutex);

    while (!q->shutdown && !q->disable_remove && q->count == 0) {
        MUTEX_WAIT(&q->cond, &q->mutex);
    }

    if (q->shutdown) {
        retval = MQ_SHUTDOWN;
        goto end;
    }

    if (q->disable_remove) {
        retval = MQ_DISABLED;
        goto end;
    }

    skDLLAssignIter(&iter, q->queues);
    while (skDLLIterBackward(&iter, (void **)&sq) == 0) {
        assert(sq->multi == q);
        if (sq->count != 0) {
            ASSERT_RESULT(skDLListPopTail(sq->queue, data), int, 0);
            sq->count--;
            q->count--;
            if (q->fair) {
                ASSERT_RESULT(skDLLIterDel(&iter), int, 0);
                ASSERT_RESULT(skDLListPushHead(q->queues, sq), int, 0);
            }
            retval = MQ_NOERROR;
            break;
        }
    }

    assert(retval == MQ_NOERROR);

  end:
    MUTEX_UNLOCK(&q->mutex);
    return retval;
}


mq_err_t
mqPushBack(
    mq_multi_t         *q,
    void               *data)
{
    mq_queue_t   *sq;
    mq_err_t      retval;
    int           rv;

    assert(q);

    MUTEX_LOCK(&q->mutex);

    if (q->shutdown) {
        retval = MQ_SHUTDOWN;
        goto end;
    }

    rv = skDLListPeekTail(q->queues, (void **)&sq);
    if (rv != 0) {
        retval = MQ_ILLEGAL;
        goto end;
    }

    if (q->disable_add || sq->disable_add) {
        retval = MQ_DISABLED;
        goto end;
    }

    rv = skDLListPushTail(sq->queue, data);
    if (rv != 0) {
        retval = MQ_MEMERROR;
        goto end;
    }

    retval = MQ_NOERROR;

    if (sq->count == 0) {
        MUTEX_BROADCAST(&q->cond);
    }
    sq->count++;
    q->count++;

  end:
    MUTEX_UNLOCK(&q->mutex);
    return retval;
}


mq_err_t
mqQueueMove(
    mq_multi_t         *q,
    mq_queue_t         *sq)
{
    mq_multi_t      *oq;
    mq_err_t         retval;
    pthread_mutex_t *a, *b, *c, *d;
    sk_dll_iter_t    iter;
    mq_queue_t      *found;
    int              rv;

    assert(q);
    assert(sq);

    if (q->free_fn != sq->multi->free_fn) {
        return MQ_ILLEGAL;
    }

    /* Attempt to enforce an ordering on locking the mutexes */
    for (;;) {
        a = d = &sq->multi->mutex;
        b = &q->mutex;
        if (b == a) {
            b = NULL;
            MUTEX_LOCK(a);
        } else {
            if (b > a) {
                c = b;
                b = a;
                a = c;
            }
            MUTEX_LOCK(a);
            MUTEX_LOCK(b);
        }

        /* If the queue's mq changed before being locked, release the
         * locks and try again */
        if (d == &sq->multi->mutex) {
            break;
        }
        if (b) {
            MUTEX_UNLOCK(b);
        }
        MUTEX_UNLOCK(a);
    }

    oq = sq->multi;

    if (q == oq) {
        retval = MQ_NOERROR;
        goto end;
    }

    skDLLAssignIter(&iter, oq->queues);
    while (skDLLIterForward(&iter, (void **)&found) == 0) {
        assert(sq->multi == oq);
        if (found == sq) {
            break;
        }
    }

    assert(found == sq);

    rv = skDLListPushHead(q->queues, sq);
    if (rv != 0) {
        retval = MQ_MEMERROR;
        goto end;
    }

    rv = skDLLIterDel(&iter);
    assert(rv == 0);

    oq->count -= sq->count;

    if (q->count == 0 && sq->count != 0) {
        MUTEX_BROADCAST(&q->cond);
    }
    q->count  += sq->count;

    sq->multi = q;
    retval = MQ_NOERROR;

  end:
    if (b) {
        MUTEX_UNLOCK(b);
    }
    MUTEX_UNLOCK(a);
    return retval;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
