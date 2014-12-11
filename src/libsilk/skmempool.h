/*
** Copyright (C) 2010-2014 by Carnegie Mellon University.
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
**  skmempool.h
**
**    Memory Pool Allocator
**
*/
#ifndef _SKMEMPOOL_H
#define _SKMEMPOOL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKMEMPOOL_H, "$SiLK: skmempool.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

/**
 *  @file
 *
 *    Implementation of a memory pool.
 *
 *    This file is part of libsilk.
 *
 *
 *
 *    The memory pool is an efficient way to allocate elements that
 *    all have the same size, the 'element_size'.  When you create the
 *    pool, you specify the number of bytes per element and the number
 *    of elements the pool should allocate at once (internally the
 *    pool calls this a block), that is, the 'elements_per_block'.
 *
 *    The 'elements_per_block' should be large enough to avoid a lot
 *    of calls to malloc()---that is why use are using the memory
 *    pool---but not so large that there is a lot of wasted space.
 *
 *    To use the memory pool, you request an element from the pool and
 *    the pool returns an element to you (the memory in the element is
 *    cleared).  Behind the scenes, the pool may reuse memory or
 *    allocate fresh memory.
 *
 *    When you are finished with the element, return it to the pool
 *    and the pool will reuse it on subsequent requests for memory.
 *    Never "return" memory to a pool that you have created via other
 *    means, and ensure that you return memory to the pool where it
 *    was allocated.
 *
 *    Internally, the pool never uses realloc(), so all existing
 *    pointers remain valid until the pool is destroyed.
 *
 *    The memory used by the pool never decreases; that is, allocated
 *    memory is never freed until the pool is destroyed.
 *
 */


/**
 *    The type of a memory pool.
 */
typedef struct sk_mempool_st sk_mempool_t;


/**
 *    Creates a new memory pool to hand out memory in 'element_size'
 *    pieces.  This should be the size of the item you are creating.
 *    The element_size should be specified with sizeof() to ensure
 *    that structures are properly aligned.
 *
 *    Due to the way the pool maintains freed data, the smallest
 *    element_size that can be used is sizeof(void*).  If a smaller
 *    element size is specified by the caller, internally the memory
 *    pool will use sizeof(void*).
 *
 *    When the pool requires memory, it allocates blocks of memory,
 *    where each block of memory holds 'elements_per_block' items.
 *
 *    This call only allocates the pool itself; this call does not
 *    allocate any elements.
 *
 *    A pointer to the new memory pool is put into the location
 *    specified by 'pool' and 0 is returned.  Returns -1 if either
 *    size value is 0, if the product of the sizes is larger than
 *    UINT32_MAX, or if pool cannot be created due to lack of memory.
 */
int
skMemoryPoolCreate(
    sk_mempool_t      **pool,
    size_t              element_size,
    size_t              elements_per_block);


/**
 *    Destroys the memory pool at the location specify by '*pool'.
 *    The pool and all the elements it has created are destroyed.  If
 *    'pool' is NULL, no action is taken.
 */
void
skMemoryPoolDestroy(
    sk_mempool_t      **pool);


/**
 *    Return a true value if the element 'elem' appears to be from the
 *    memory pool 'pool', or a false value otherwise.
 */
int
skMemoryPoolOwnsElement(
    const sk_mempool_t *pool,
    const void         *elem);


/**
 *    Returns the element 'elem' to the memory pool 'pool'.
 *
 *    Be careful to only return memory to the pool that has been
 *    allocated using skMemPoolElementNew() for that particular pool.
 */
void
skMemPoolElementFree(
    sk_mempool_t       *pool,
    void               *elem);


/**
 *    Returns element_size bytes of cleared memory from the pool
 *    'pool', where the element size was specified when the pool was
 *    created.  Returns NULL if memory cannot be allocated.
 */
void *
skMemPoolElementNew(
    sk_mempool_t       *pool);

#ifdef __cplusplus
}
#endif
#endif /* _SKMEMPOOL_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
