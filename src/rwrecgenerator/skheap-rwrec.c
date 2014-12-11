/*
** Copyright (C) 2011-2014 by Carnegie Mellon University.
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
**  A heap (priority queue) for rwRec pointers
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skheap-rwrec.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "skheap-rwrec.h"

/* LOCAL DEFINES AND TYPEDEFS */

/* Means of getting the correct sktime_t from the record. */
#define GET_TIME(rec) rwRecGetEndTime(rec)

/* How much larger (multiplicative) to attempt to realloc heap upon
   heap growth.  (Must be positive.) */
#define RESIZE_FACTOR 1.0       /* Double the size on growth */

struct skrwrecheap_st {
    rwRec  **data;
    size_t   max_entries;
    size_t   num_entries;
};


/* FUNCTION DEFINITIONS */

skrwrecheap_t *
skRwrecHeapCreate(
    size_t              initial_entries)
{
    skrwrecheap_t *heap;

    heap = (skrwrecheap_t*)calloc(1, sizeof(skrwrecheap_t));
    if (heap == NULL) {
        return NULL;
    }

    if (initial_entries == 0) {
        initial_entries = 1;
    }
    heap->data = (rwRec**)malloc(initial_entries * sizeof(rwRec *));
    if (heap->data == NULL) {
        free(heap);
        return NULL;
    }
    heap->max_entries = initial_entries;

    return heap;
}


void
skRwrecHeapDestroy(
    skrwrecheap_t      *heap)
{
    free(heap->data);
    free(heap);
}

static int
skRwrecHeapResize(
    skrwrecheap_t      *heap,
    size_t              num_entries)
{
    rwRec **new_data;

    assert(heap);

    new_data = (rwRec**)realloc(heap->data, sizeof(rwRec *) * num_entries);
    if (new_data == NULL) {
        return -1;
    }
    heap->data = new_data;
    heap->max_entries = num_entries;
    if (num_entries < heap->num_entries) {
        heap->num_entries = num_entries;
    }

    return 0;
}


/* Grow the heap.  Return the change in size of the heap. */
static size_t
skRwrecHeapGrow(
    skrwrecheap_t      *heap)
{
    size_t target_size;
    size_t target_growth;
    double factor = RESIZE_FACTOR;
    int rv = 0;

    assert(heap);

    /* Determine new target size, adjust for possible overflow */
    target_size = (size_t)((double)heap->max_entries * (1.0 + factor));
    while (target_size < heap->max_entries) {
        factor /= 2.0;
        target_size = (size_t)((double)heap->max_entries * (1.0 + factor));
    }

    /* Resize the heap to target_size, adjust target_size if resize
       fails */
    while (target_size > heap->max_entries) {
        target_growth = target_size - heap->max_entries;
        rv = skRwrecHeapResize(heap, target_size);
        if (rv == 0) {
            return target_growth;
        }
        factor /= 2.0;
        target_size = (size_t)((double)heap->max_entries * (1.0 + factor));
    }

    return 0;
}


int
skRwrecHeapInsert(
    skrwrecheap_t      *heap,
    rwRec              *rec)
{
    size_t parent;
    size_t child;
    sktime_t rec_time;
    rwRec **data;

    assert(heap);
    assert(rec);

    /* If the heap is full, resize */
    if (heap->num_entries == heap->max_entries
        && !skRwrecHeapGrow(heap))
    {
        return -1;
    }

    rec_time = GET_TIME(rec);
    data = heap->data;
    for (child = heap->num_entries; child > 0; child = parent) {
        parent = (child - 1) >> 1;
        if (GET_TIME(data[parent]) <= rec_time) {
            break;
        }
        data[child] = data[parent];
    }
    data[child] = rec;
    ++heap->num_entries;

    return 0;
}


const rwRec *
skRwrecHeapPeek(
    skrwrecheap_t      *heap)
{
    assert(heap);

    if (heap->num_entries) {
        return heap->data[0];
    }
    return NULL;
}


rwRec *
skRwrecHeapPop(
    skrwrecheap_t      *heap)
{
    size_t parent;
    size_t child;
    sktime_t rec_time, c1, c2;
    rwRec *retval;
    rwRec **data;

    assert(heap);

    if (heap->num_entries < 1) {
        return NULL;
    }
    retval = heap->data[0];

    --heap->num_entries;
    if (heap->num_entries) {
        data = heap->data;
        rec_time = GET_TIME(data[heap->num_entries]);
        parent = 0;             /* The empty slot */
        child = 1;              /* Children of empty slot */
        while (child < heap->num_entries) {
            /* Expanded the possibilities out for speed.  We minimize
               the number of times we have to call GET_TIME. */
            if (child + 1 == heap->num_entries) {
                /* Only one child */
                if (GET_TIME(data[child]) < rec_time) {
                    data[parent] = data[child];
                    parent = child;
                } else {
                    break;
                }
            } else {
                /* Two children */
                c1 = GET_TIME(data[child]);
                c2 = GET_TIME(data[child + 1]);
                if ((c1 <= c2)) {
                    /* Child 1 is smaller than (or equal to) child 2 */
                    if (c1 < rec_time) {
                        data[parent] = data[child];
                        parent = child;
                    } else {
                        break;
                    }
                } else if (c2 < rec_time) {
                    /* Child 2 is smaller than child 1 */
                    data[parent] = data[child + 1];
                    parent = child + 1;
                } else {
                    break;
                }
            }
            child = (parent << 1) + 1;
        }

        data[parent] = data[heap->num_entries];
    }

    return retval;
}


size_t
skRwrecHeapCountEntries(
    const skrwrecheap_t    *heap)
{
    assert(heap);
    return heap->num_entries;
}

size_t
skRwrecHeapGetCapacity(
    const skrwrecheap_t    *heap)
{
    assert(heap);
    return heap->max_entries;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
