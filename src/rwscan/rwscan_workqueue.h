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
#ifndef _RWSCAN_WORKQUEUE_H
#define _RWSCAN_WORKQUEUE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWSCAN_WORKQUEUE_H, "$SiLK: rwscan_workqueue.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");


typedef struct work_queue_node_st {
    struct work_queue_node_st *next;       /* next request in queue */
} work_queue_node_t;

/*
 * This threaded queue structure is specialized for a
 * producer/consumer design in two ways.  First, queues can be created
 * with a maximum queue depth parameter, which governs how large the
 * queue can grow in size.  Second, the queue can be "deactivated" to
 * shut down producer threads when the program exits.
 *
 * The queue just maintains node pointers; it does not manage node
 * memory in any way.
 *
 */
typedef struct work_queue_st {
    work_queue_node_t *head;        /* pointer to first node */
    work_queue_node_t *tail;        /* pointer to last node */

    pthread_mutex_t    mutex;       /* master queue lock mutex */
    pthread_cond_t     cond_posted; /* used to wake up a consumer */
    pthread_cond_t     cond_avail;  /* used to signal a producer */

    int                depth;       /* number of items in queue */
    int                maxdepth;    /* max items allowed in queue */
    int                pending;     /* numitems being processed */
    int                active;      /* if work queue has been activated */
#ifdef RWSCN_WORKQUEUE_DEBUG
    int                consumed;    /* num items consumed */
    int                produced;    /* num items posted by producers */
    int                peakdepth;   /* highest queue depth */
#endif
} work_queue_t;


/* Public work queue API */
work_queue_t *
workqueue_create(
    uint32_t            maxdepth);
int
workqueue_put(
    work_queue_t       *q,
    work_queue_node_t  *newnode);
int
workqueue_get(
    work_queue_t       *q,
    work_queue_node_t **retnode);
int
workqueue_depth(
    work_queue_t       *q);

#if 1
int
workqueue_pending(
    work_queue_t       *q);
#endif
int
workqueue_activate(
    work_queue_t       *q);
int
workqueue_deactivate(
    work_queue_t       *q);
void
workqueue_destroy(
    work_queue_t       *q);

#ifdef __cplusplus
}
#endif
#endif /* _RWSCAN_WORKQUEUE_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/