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
**  Am implementation of doubly-linked lists.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skdllist.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>
#include <silk/skdllist.h>

/* LOCAL DEFINES AND TYPEDEFS */

typedef enum sk_dll_dir_en {
    FORWARD = 1,
    BACKWARD = 0,
    TAIL = 0,
    HEAD = 1
} sk_dll_dir_t;

/* a node is equivalent to an iterator */
typedef sk_dll_iter_t sk_dll_node_t;

struct sk_dllist_st {
    sk_dll_node_t    list;
    sk_dll_free_fn_t data_free_fn;
};


/* LOCAL VARIABLE DEFINITIONS */

static void *null_value = &null_value;


/* FUNCTION DEFINITIONS */

sk_dllist_t *
skDLListCreate(
    sk_dll_free_fn_t    free_fn)
{
    sk_dllist_t *list = (sk_dllist_t *)malloc(sizeof(*list));

    if (list != NULL) {
        list->list.data = null_value;
        list->list.link[TAIL] = list->list.link[HEAD] = &list->list;
        list->data_free_fn = free_fn;
    }

    return list;
}


void
skDLListDestroy(
    sk_dllist_t        *list)
{
    sk_dll_node_t *node, *next;

    assert(list);

    node = list->list.link[TAIL];
    while (node->data != null_value) {
        if (list->data_free_fn) {
            list->data_free_fn(node->data);
        }
        next = node->link[BACKWARD];
        free(node);
        node = next;
    }
    free(list);
}


int
skDLListIsEmpty(
    const sk_dllist_t  *list)
{
    assert(list);
    return list->list.link[TAIL] == &list->list;
}


static int
sk_dll_peek(
    const sk_dllist_t  *list,
    void              **data,
    sk_dll_dir_t        dir)
{
    sk_dll_node_t *node;

    assert(list);
    assert(data);

    node = list->list.link[dir];

    if (node->data == null_value) {
        return -1;
    }
    *data = node->data;

    return 0;
}


int
skDLListPeekTail(
    const sk_dllist_t  *list,
    void              **data)
{
    return sk_dll_peek(list, data, TAIL);
}


int
skDLListPeekHead(
    const sk_dllist_t  *list,
    void              **data)
{
    return sk_dll_peek(list, data, HEAD);
}


static void
sk_dll_node_del(
    sk_dll_node_t      *node)
{
    node->link[FORWARD]->link[BACKWARD] = node->link[BACKWARD];
    node->link[BACKWARD]->link[FORWARD] = node->link[FORWARD];
    free(node);
}


static int
sk_dll_pop(
    sk_dllist_t        *list,
    void              **data,
    sk_dll_dir_t        dir)
{
    sk_dll_node_t *node;

    assert(list);

    node = list->list.link[dir];
    if (node->data == null_value) {
        return -1;
    }
    if (data != NULL) {
        *data = node->data;
    }
    sk_dll_node_del(node);

    return 0;
}


int
skDLListPopTail(
    sk_dllist_t        *list,
    void              **data)
{
    return sk_dll_pop(list, data, TAIL);
}


int
skDLListPopHead(
    sk_dllist_t        *list,
    void              **data)
{
    return sk_dll_pop(list, data, HEAD);
}


static int
sk_dll_node_add(
    sk_dll_iter_t      *iter,
    void               *data,
    sk_dll_dir_t        dir)
{
    sk_dll_node_t *node, *truenode;;

    assert(iter);

    node = (sk_dll_node_t *)malloc(sizeof(*node));
    if (node == NULL) {
        return -1;
    }

    truenode = iter->link[FORWARD]->link[BACKWARD];

    node->data = data;
    node->link[dir] = truenode->link[dir];
    node->link[1-dir] = truenode;

    node->link[FORWARD]->link[BACKWARD] = node;
    node->link[BACKWARD]->link[FORWARD] = node;

    if (truenode != iter) {
        iter->link[FORWARD] = truenode->link[FORWARD];
        iter->link[BACKWARD] = truenode->link[BACKWARD];
    }

    return 0;
}


int
skDLListPushTail(
    sk_dllist_t        *list,
    void               *data)
{
    return sk_dll_node_add(&list->list, data, TAIL);
}


int
skDLListPushHead(
    sk_dllist_t        *list,
    void               *data)
{
    return sk_dll_node_add(&list->list, data, HEAD);
}


int
skDLListJoin(
    sk_dllist_t        *head,
    sk_dllist_t        *tail)
{
    sk_dll_node_t *tail_h, *tail_t;

    assert(head);
    assert(tail);

    /* Return an error if the free functions do not match. */
    if (head->data_free_fn != tail->data_free_fn) {
        return -1;
    }

    /* If tail is empty, destroy the tail and return */
    if (skDLListIsEmpty(tail)) {
        skDLListDestroy(tail);
        return 0;
    }

    /* Save links to the head and tail nodes of tail */
    tail_h = tail->list.link[HEAD];
    tail_t = tail->list.link[TAIL];

    /* Reset tail to empty, and destroy */
    tail->list.link[HEAD] = tail->list.link[TAIL] = &tail->list;
    skDLListDestroy(tail);

    /* Update the links to insert the list */
    tail_h->link[BACKWARD] = head->list.link[TAIL];
    tail_t->link[FORWARD]  = &head->list;
    head->list.link[TAIL]->link[FORWARD] = tail_h;
    head->list.link[TAIL] = tail_t;

    return 0;
}


void
skDLLAssignIter(
    sk_dll_iter_t      *iter,
    sk_dllist_t        *list)
{
    assert(list);
    (*iter) = list->list;
}


static int
sk_dll_iter_get_next(
    sk_dll_iter_t      *iter,
    void              **data,
    sk_dll_dir_t        dir)
{
    *iter = *iter->link[dir];
    if (iter->data == null_value) {
        return -1;
    }
    if (data != NULL) {
        *data = iter->data;
    }
    return 0;
}


int
skDLLIterForward(
    sk_dll_iter_t      *iter,
    void              **data)
{
    return sk_dll_iter_get_next(iter, data, FORWARD);
}


int
skDLLIterBackward(
    sk_dll_iter_t      *iter,
    void              **data)
{
    return sk_dll_iter_get_next(iter, data, BACKWARD);
}


int
skDLLIterDel(
    sk_dll_iter_t      *iter)
{
    assert(iter);
    if (iter->data == null_value) {
        return -1;
    }
    sk_dll_node_del(iter->link[FORWARD]->link[BACKWARD]);
    return 0;
}


int
skDLLIterAddAfter(
    sk_dll_iter_t      *iter,
    void               *data)
{
    return sk_dll_node_add(iter, data, FORWARD);
}


int
skDLLIterAddBefore(
    sk_dll_iter_t      *iter,
    void               *data)
{
    return sk_dll_node_add(iter, data, BACKWARD);
}


int
skDLLIterValue(
    const sk_dll_iter_t    *iter,
    void                  **data)
{
    assert(iter);
    assert(data);

    if (iter->data == null_value) {
        return -1;
    }
    *data = iter->data;
    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
