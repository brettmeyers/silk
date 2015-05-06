/*
** Copyright (C) 2001-2015 by Carnegie Mellon University.
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
**  skheap.h
**
**    The heap (priority queue) data structure.
**
**    Mark Thomas
**
*/
#ifndef _SKHEAP_H
#define _SKHEAP_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKHEAP_H, "$SiLK: skheap.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/**
 *  @file
 *
 *    Implementation of a heap (priority queue) data structure.
 *
 *    This file is part of libsilk.
 */


/**
 *    Return value to indicate success
 */
#define SKHEAP_OK 0

/**
 *    Return value when attempting to add node to a full heap.
 */
#define SKHEAP_ERR_FULL 3

/**
 *    Return value when attempting to get or to delete the top element
 *    of an empty heap.
 */
#define SKHEAP_ERR_EMPTY 4

/**
 *    Return value when heap iterator reaches end-of-data.
 */
#define SKHEAP_NO_MORE_ENTRIES 5


/**
 *    The Heap object
 */
typedef struct skheap_st skheap_t;

/**
 *    The nodes stored in the heap data structure
 */
typedef void* skheapnode_t;

/**
 *    Used to iterate over the entries in the heap
 */
typedef struct skheapiterator_st skheapiterator_t;


/**
 *    The signature of the comparator function that the caller must
 *    pass to the skHeapCreate() function.  The function takes two
 *    skheapnode_t's, node1 and node2, and returns:
 *     -- an integer value > 0 if node1 should be a closer to the root
 *        of the heap than node2
 *     -- an integer value < 0 if node2 should be a closer to the root
 *        of the heap than node1

 *    For example: a heap with the lowest value at the root could
 *    return 1 if node1<node2.  If an implementation of this function
 *    wraps memcmp(), the function should
 *
 *        return -memcmp(node1,node2,...)
 *
 *    to order the values from low to high, and
 *
 *        return memcmp(node1,node2,...)
 *
 *    to order the values from high to low.
 */
typedef int (*skheapcmpfn_t)(
    const skheapnode_t  node1,
    const skheapnode_t  node2);

/**
 *    The signature of the comparator function that the caller must
 *    pass to the skHeapCreate2() function.  This function is similar
 *    to skheapcmp2fn_t, except it takes a third argument that is the
 *    pointer the caller supplied as the 'cmpfun_data' argument to
 *    skHeapCreate2().
 */
typedef int (*skheapcmp2fn_t)(
    const skheapnode_t  node1,
    const skheapnode_t  node2,
    void               *cmp_data);


/**
 *    Similar to skHeapCreate2(), but the 'cmpfun' does not take a
 *    caller-provided argument.
 */
skheap_t *
skHeapCreate(
    skheapcmpfn_t       cmpfun,
    uint32_t            init_count,
    uint32_t            entry_size,
    skheapnode_t       *memory_buf);



/**
 *    Creates a heap that is initially capable of holding 'init_count'
 *    entries (skheapnode_t's) each of size 'entry_size'.  The
 *    'cmpfun' determines how the nodes are ordered in the heap.  The
 *    'cmpfun_data' is passed as the third argument to
 *    'cmpfun'.
 *
 *    If the 'memory_buf' is NULL, the heap will manage the memory for
 *    entries itself.  An attempt to insert more than 'init_count'
 *    entries into the heap will cause the heap to reallocate memory
 *    for the entries.
 *
 *    If 'memory_buf' is non-NULL, it must be a block of memory whose
 *    size is at least init_count * entry_size bytes.  The heap will
 *    use 'memory_buf' to store the entries, and the 'init_count'
 *    value will represent the maximum size of the heap.
 */
skheap_t *
skHeapCreate2(
    skheapcmp2fn_t      cmpfun,
    uint32_t            init_count,
    uint32_t            entry_size,
    skheapnode_t       *memory_buf,
    void               *cmpfun_data);


/**
 *    Set the number of entries in the heap to 0, effectively emptying
 *    the heap.  This function does not modify the bytes in the data
 *    array.
 */
void
skHeapEmpty(
    skheap_t           *heap);


/**
 *    Remove the entry at the top of the heap.  If 'top_node' is
 *    non-NULL, the removed entry is copied there---that is,
 *    'entry_size' bytes of data will be written to the location
 *    pointed to by 'top_node'.  Return SKHEAP_ERR_EMPTY if the heap
 *    is empty.
 *
 *    See also skHeapPeekTop() and skHeapReplaceTop().
 */
int
skHeapExtractTop(
    skheap_t           *heap,
    skheapnode_t        top_node);


/**
 *    Destroy an existing heap.  This function does not modify the
 *    data array when using caller-supplied data---that is, when a
 *    non-NULL 'memory_buf' value was passed to skHeapCreate().
 *
 *    If the 'heap' parameter is NULL, this function immediately
 *    returns.
 */
void
skHeapFree(
    skheap_t           *heap);


/**
 *    Return the number of entries the heap can accommodate.  To get
 *    the number of free entries in the heap, subtract the result of
 *    skHeapGetNumberEntries() from this function's result.
 */
uint32_t
skHeapGetCapacity(
    const skheap_t     *heap);


/**
 *    Return the size of each element that is stored in the heap.
 */
uint32_t
skHeapGetEntrySize(
    const skheap_t     *heap);


/**
 *    Return the number of entries currently in the heap.
 */
uint32_t
skHeapGetNumberEntries(
    const skheap_t     *heap);


/**
 *    Add the entry at 'new_node' to the heap.  Return SKHEAP_ERR_FULL
 *    if the heap is full.  This function will read 'entry_size' bytes
 *    of data from the location pointed to by 'new_node'.
 */
int
skHeapInsert(
    skheap_t           *heap,
    const skheapnode_t  new_node);


/**
 *    Return an skheapiterator_t that can be used to iterate over the
 *    nodes in 'heap'.  Return NULL if the iterator cannot be created.
 *    If 'direction' is non-negative, the iterator starts at the root
 *    and works toward the leaves; otherwise, the iterator works from
 *    the leaves to the root.  The iterator visits all nodes on one
 *    level before moving to the next.  By calling skHeapSortEntries()
 *    before creating the iterator, the nodes will be traversed in the
 *    order determined by the 'cmpfun' that was specified when the
 *    heap was created.
 */
skheapiterator_t *
skHeapIteratorCreate(
    const skheap_t     *heap,
    int                 direction);


/**
 *    Free the memory associated with the heap iterator 'iter'.
 *    Does nothing if 'iter' is NULL.
 */
void
skHeapIteratorFree(
    skheapiterator_t   *iter);


/**
 *    Set 'heap_node' to the memory location of the next entry.
 *    Return SKHEAP_OK if 'heap_node' was set to the next node; return
 *    SKHEAP_NO_MORE_ENTRIES if all nodes have been visited.
 */
int
skHeapIteratorNext(
    skheapiterator_t   *iter,
    skheapnode_t       *heap_node);


/**
 *    Set the value of 'top_node' to point at the entry at the top of
 *    the heap.  This function does not modify the heap.  The caller
 *    must not modify the data that 'top_node' is pointing to.  Return
 *    SKHEAP_ERR_EMPTY if the heap is empty.
 *
 *    See also skHeapExtractTop() and skHeapReplaceTop().
 */
int
skHeapPeekTop(
    const skheap_t     *heap,
    skheapnode_t       *top_node);


/**
 *    Remove the entry at the top of the heap and insert a new entry
 *    into the heap.  If 'top_node' is non-NULL, the removed entry is
 *    copied there---that is, 'entry_size' bytes of data will be
 *    written to the location pointed to by 'top_node'.  This function
 *    will read 'entry_size' bytes of data from the location pointed
 *    to by 'new_node'.  Return SKHEAP_ERR_EMPTY if the heap is empty
 *    and do NOT add 'new_node' to the heap.
 *
 *    See also skHeapExtractTop() and skHeapPeekTop().
 */
int
skHeapReplaceTop(
    skheap_t           *heap,
    const skheapnode_t  new_node,
    skheapnode_t        top_node);


/**
 *    Sort the entries in 'heap'.  (Note that a sorted heap is still a
 *    heap).  This can be used to order the entries before calling
 *    skHeapIteratorCreate(), or for sorting the entries in the
 *    user-defined storage.
 */
int
skHeapSortEntries(
    skheap_t           *heap);

#ifdef __cplusplus
}
#endif
#endif /* _SKHEAP_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
