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
**
**    Circular buffer API
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: circbuf.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/sklog.h>
#include "circbuf.h"

#ifdef CIRCBUF_TRACE_LEVEL
#define TRACEMSG_LEVEL 1
#endif
#define TRACEMSG( msg) TRACEMSG_TO_TRACEMSGLVL(1, msg)
#include <silk/sktracemsg.h>

/* Minimum number of items which should be storable in a chunk */
#define CIRCBUF_MINIMUM_ITEMS_PER_CHUNK 3

/* Maximum possible size of a single item */
#define CIRCBUF_CHUNK_MAXIMUM_ITEM_SIZE                 \
    ((1 << 28) / CIRCBUF_MINIMUM_ITEMS_PER_CHUNK)


struct circBuf_chunk_st;
typedef struct circBuf_chunk_st circBuf_chunk_t;


/*
 *    The circBuf_t hands cells to the writer which the writer fills.
 *    The circBuf_t holds onto these cells until the reader requests
 *    them.  The maxinum number of cells a circBuf_t may allocate is
 *    specified at creatation time.  However, the cells are not
 *    allocated as one block of memory.  Instead, the circBuf_t
 *    allocates smaller blocks of memory called chunks.  All chunks
 *    are the same size.  To summarize, the circBuf_t is composed of
 *    multiple chunks, and a chunk is composed of multiple cells.
 *
 *    For each chunk, the 'head' points to the cell currently in use
 *    by the writer, and the 'tail' points to the cell currently in
 *    use by the reader.
 *
 *    All cells "between" the tail and the head have data.  In the
 *    diagram below, the writer (head) has wrapped around, and all
 *    cells with 'D' have data.  'W' is where the writer is currently
 *    writing data, and 'R' is where the reader is reading.
 *        _ _ _ _ _ _ _ _ _ _ _ _
 *       |D|D|W|_|_|_|_|_|R|D|D|D|
 *            A A         A A
 *            | |         | |
 *            | next_head | next_tail
 *            |           |
 *            head        tail
 *
 *    When the writer or reader finishes with a cell, they call the
 *    appropriate function which releases the current cell and moves
 *    them to the next cell.
 *
 *    If a chunk becomes full, a new chunk is allocated and the writer
 *    starts using cells from the new chunk.  Depending on the chunk
 *    size and maximum number of cells allowed, there may be multiple
 *    chunks in the chunk list between the writer and the reader.
 *
 *    Once the reader finishes with all the cells in the current
 *    chunk, the reader moves to the first cell of the next chunk in
 *    the chunk list, and the chunk the reader just completed is
 *    discarded.  The circBuf_t is circular within a chunk, but linear
 *    between multiple chunks.
 *
 *    The first time the circBuf_t has a chunk to discard, the
 *    circBuf_t stores the chunk as spare (instead of deallocating the
 *    chunk).  When a chunk needs to be discard and the circBuf_t
 *    already has a spare chunk, the chunk is deallocated.
 *
 */
struct circBuf_chunk_st {
    /* Next chunk in chunk list */
    circBuf_chunk_t *next;
    /* Next head (writer) cell index */
    uint32_t         next_head;
    /* Current head (writer) cell index */
    uint32_t         head;
    /* Next tail (reader) cell index */
    uint32_t         next_tail;
    /* Current tail (reader) cell index */
    uint32_t         tail;
    /* Buffer containing cells */
    uint8_t         *data;
    /* True if all cells are used */
    unsigned         full   :1;
};

struct circBuf_st {
    /* Maximum number of cells */
    uint32_t         maxcells;
    /* Current number of cells in use, across all chunks */
    uint32_t         cellcount;
    /* Size of a single cell */
    uint32_t         cellsize;
    /* Number of cells per chunk */
    uint32_t         cells_per_chunk;
    /* Head (writer) chunk */
    circBuf_chunk_t *head_chunk;
    /* Tail (reader) chunk */
    circBuf_chunk_t *tail_chunk;
    /* Spare chunk */
    circBuf_chunk_t *spare_chunk;
    /* Mutex */
    pthread_mutex_t  mutex;
    /* Condition variable */
    pthread_cond_t   cond;
    /* Number of threads waiting on this buf */
    uint32_t         wait_count;
    /* True if the buf has been stopped */
    unsigned         destroyed : 1;
};


/* Allocate a new chunk */
static circBuf_chunk_t *
alloc_chunk(
    circBuf_t          *buf)
{
    circBuf_chunk_t *chunk;

    if (buf->spare_chunk) {
        /* If there is a spare chunk, use it.  We maintain a spare
         * chunk to avoid reallocating frequently when items are
         * removed more quickly then they are added. */
        chunk = buf->spare_chunk;
        buf->spare_chunk = NULL;
        chunk->next_head = chunk->tail = 0;
    } else {
        /* Otherwise, allocate a new chunk. */
        chunk = (circBuf_chunk_t*)calloc(1, sizeof(circBuf_chunk_t));
        if (chunk == NULL) {
            return NULL;
        }
        chunk->data = (uint8_t*)malloc(buf->cells_per_chunk * buf->cellsize);
        if (chunk->data == NULL) {
            free(chunk);
            return NULL;
        }
    }
    chunk->head = buf->cells_per_chunk - 1;
    chunk->next_tail = 1;
    chunk->next = NULL;

    return chunk;
}


circBuf_t *
circBufCreate(
    uint32_t            item_size,
    uint32_t            item_count)
{
    circBuf_t *buf;
    uint32_t chunks;

    if (item_count == 0
        || item_size == 0
        || item_size > CIRCBUF_CHUNK_MAXIMUM_ITEM_SIZE)
    {
        return NULL;
    }

    buf = (circBuf_t*)calloc(1, sizeof(circBuf_t));
    if (buf == NULL) {
        return NULL;
    }

    buf->cellsize = item_size;

    buf->cells_per_chunk = CIRCBUF_CHUNK_MAX_SIZE / item_size;
    if (buf->cells_per_chunk < CIRCBUF_MINIMUM_ITEMS_PER_CHUNK) {
        buf->cells_per_chunk = CIRCBUF_MINIMUM_ITEMS_PER_CHUNK;
    }

    /* Number of chunks required to handle item_count cells */
    chunks = 1 + (item_count - 1) / buf->cells_per_chunk;
    buf->maxcells = buf->cells_per_chunk * chunks;

    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->cond, NULL);
    return buf;
}


uint8_t *
circBufNextHead(
    circBuf_t          *buf)
{
    uint8_t *retval;

    assert(buf);

    pthread_mutex_lock(&buf->mutex);

    ++buf->wait_count;

    /* Wait for an empty cell */
    while (!buf->destroyed && (buf->cellcount == buf->maxcells)) {
        TRACEMSG((("circBufNextHead() full, count is %" PRIu32),
                  buf->cellcount));
        pthread_cond_wait(&buf->cond, &buf->mutex);
    }

    if (buf->cellcount <= 1) {
        /* If previously, the buffer was empty, signal waiters */
        pthread_cond_broadcast(&buf->cond);

        /* Create the initial chunk.  This should only happen once */
        if (buf->head_chunk == NULL) {
            buf->tail_chunk = buf->head_chunk = alloc_chunk(buf);
            if (buf->tail_chunk == NULL) {
                retval = NULL;
                goto END;
            }
            /* The initial chunk need to pretend that its tail starts
             * at -1 instead of 0, because its tail is not coming from
             * a previous chunk.  This is a special case that should
             * only happen once.  */
            buf->tail_chunk->tail = buf->cells_per_chunk - 1;
            buf->tail_chunk->next_tail = 0;
        }
    }

    /* Increment the cell count */
    ++buf->cellcount;

    if (buf->destroyed) {
        retval = NULL;
        pthread_cond_broadcast(&buf->cond);
    } else {
        /* Get the head chunk */
        circBuf_chunk_t *chunk = buf->head_chunk;

        /* If the head chunk is full */
        if (chunk->full) {
            assert(chunk->next == NULL);
            chunk->next = alloc_chunk(buf);
            if (chunk->next == NULL) {
                retval = NULL;
                goto END;
            }

            /* Make the next chunk the new head chunk*/
            chunk = chunk->next;
            assert(chunk->next == NULL);
            buf->head_chunk = chunk;
        }
        /* Return value is the next head position */
        retval = &chunk->data[chunk->next_head * buf->cellsize];

        /* Increment the current head and the next_head */
        chunk->head = chunk->next_head;
        ++chunk->next_head;

        /* Account for wrapping around the next head */
        if (chunk->next_head == buf->cells_per_chunk) {
            chunk->next_head = 0;
        }

        /* Check to see if we have filled this chunk */
        if (chunk->next_head == chunk->tail) {
            chunk->full = 1;
        }
    }

  END:

    --buf->wait_count;

    pthread_mutex_unlock(&buf->mutex);

    return retval;
}


uint8_t *
circBufNextTail(
    circBuf_t          *buf)
{
    uint8_t *retval;

    assert(buf);

    pthread_mutex_lock(&buf->mutex);

    ++buf->wait_count;

    /* Wait for a full cell */
    while (!buf->destroyed && (buf->cellcount <= 1)) {
        pthread_cond_wait(&buf->cond, &buf->mutex);
    }

    /* If previously, the buffer was full, signal waiters */
    if (buf->cellcount == buf->maxcells) {
        pthread_cond_broadcast(&buf->cond);
    }

    /* Decrement the cell count */
    --buf->cellcount;

    if (buf->destroyed) {
        retval = NULL;
        pthread_cond_broadcast(&buf->cond);
    } else {
        /* Get the tail chunk */
        circBuf_chunk_t *chunk = buf->tail_chunk;

        /* Mark the chunk as not full */
        chunk->full = 0;

        /* Increment the tail and the next_tail */
        chunk->tail = chunk->next_tail;
        ++chunk->next_tail;

        /* Account for wrapping around the next tail */
        if (chunk->next_tail == buf->cells_per_chunk) {
            chunk->next_tail = 0;
        }

        /* Move to next chunk if we have emptied this one (and not last) */
        if (chunk->tail == chunk->next_head) {
            circBuf_chunk_t *next_chunk;

            next_chunk = chunk->next;

            /* Free the tail chunk.  Save as spare_chunk if empty */
            if (buf->spare_chunk) {
                free(chunk->data);
                free(chunk);
            } else {
                buf->spare_chunk = chunk;
            }

            chunk = buf->tail_chunk = next_chunk;
            assert(chunk);
        }

        /* Return value is the current tail position */
        retval = &chunk->data[chunk->tail * buf->cellsize];
    }

    --buf->wait_count;

    pthread_mutex_unlock(&buf->mutex);

    return retval;
}


void
circBufStop(
    circBuf_t          *buf)
{
    pthread_mutex_lock(&buf->mutex);
    buf->destroyed = 1;
    pthread_cond_broadcast(&buf->cond);
    while (buf->wait_count) {
        pthread_cond_wait(&buf->cond, &buf->mutex);
    }
    pthread_mutex_unlock(&buf->mutex);
}


void
circBufDestroy(
    circBuf_t          *buf)
{
    circBuf_chunk_t *chunk;
    circBuf_chunk_t *next_chunk;

    pthread_mutex_lock(&buf->mutex);
    if (!buf->destroyed) {
        buf->destroyed = 1;
        pthread_cond_broadcast(&buf->cond);
        while (buf->wait_count) {
            pthread_cond_wait(&buf->cond, &buf->mutex);
        }
    }
    TRACEMSG((("circBufDestroy(): Buffer has %" PRIu32 " records"),
              buf->cellcount));
    pthread_mutex_unlock(&buf->mutex);

    pthread_mutex_destroy(&buf->mutex);
    pthread_cond_destroy(&buf->cond);

    chunk = buf->tail_chunk;
    while (chunk) {
        next_chunk = chunk->next;
        free(chunk->data);
        free(chunk);
        chunk = next_chunk;
    }

    if (buf->spare_chunk) {
        free(buf->spare_chunk->data);
        free(buf->spare_chunk);
    }

    free(buf);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
