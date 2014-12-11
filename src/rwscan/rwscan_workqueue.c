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

#include <silk/silk.h>

RCSIDENT("$SiLK: rwscan_workqueue.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "rwscan_workqueue.h"



work_queue_t *
workqueue_create(
    uint32_t            maxdepth)
{
    work_queue_t *q;

    q = (work_queue_t *) calloc(1, sizeof(work_queue_t));
    if (q == NULL) {
        return (work_queue_t *) NULL;
    }

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_posted, NULL);
    pthread_cond_init(&q->cond_avail, NULL);

    q->maxdepth = maxdepth;
    q->active   = 1;

    return q;
}

int
workqueue_activate(
    work_queue_t       *q)
{
    if (pthread_mutex_lock(&q->mutex)) {
        return 0;
    }
    q->active = 1;
    pthread_mutex_unlock(&q->mutex);
    pthread_cond_broadcast(&q->cond_posted);
    return 1;
}

int
workqueue_deactivate(
    work_queue_t       *q)
{
    if (pthread_mutex_lock(&q->mutex)) {
        return 0;
    }
    q->active = 0;
    pthread_mutex_unlock(&q->mutex);
    pthread_cond_broadcast(&q->cond_posted);
    return 1;
}



void
workqueue_destroy(
    work_queue_t       *q)
{
    if (q == NULL) {
        return;
    }

    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_posted);
    pthread_cond_destroy(&q->cond_avail);
    free(q);
}


int
workqueue_put(
    work_queue_t       *q,
    work_queue_node_t  *newnode)
{
    if (newnode == NULL || q == NULL) {
        return -1;
    }

    pthread_mutex_lock(&q->mutex);

    while ((q->maxdepth > 0) && (q->depth + q->pending >= q->maxdepth)) {
        /* queue is full - wait on cond var for notification that a slot has
         * become open */
        pthread_cond_wait(&q->cond_avail, &q->mutex);
    }

    if (q->tail == NULL) {
        q->tail = q->head = newnode;
    } else {
        q->tail->next = newnode;
        q->tail       = newnode;
    }

    newnode->next = NULL;

    q->depth++;

    /* release queue mutex and signal consumer that an item is ready */
    pthread_mutex_unlock(&q->mutex);
    pthread_cond_signal(&q->cond_posted);


#ifdef RWSCN_WORKQUEUE_DEBUG
    /* update the peak depth, if needed */
    if (q->depth > q->peakdepth) {
        q->peakdepth = q->depth;
    }

    q->produced++;
#endif

    return q->depth;
}


int
workqueue_get(
    work_queue_t       *q,
    work_queue_node_t **retnode)
{
    work_queue_node_t *node;

    if (q->head == NULL || q->depth == 0) {
        retnode = NULL;
        return -1;
    }

    node = q->head;
    if (node->next) {
        q->head = node->next;
    } else {
        q->head = NULL;
        q->tail = q->head = NULL;
    }

    node->next = NULL;
    *retnode   = node;

    q->depth--;
    q->pending++;

#ifdef RWSCN_WORKQUEUE_DEBUG
    q->consumed++;
#endif

    return 0;
}

int
workqueue_depth(
    work_queue_t       *q)
{
    return q->depth;
}

int
workqueue_pending(
    work_queue_t       *q)
{
    return q->pending;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
