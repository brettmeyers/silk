/*
** Copyright (C) 2011-2015 by Carnegie Mellon University.
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
#ifndef _SKHEAP_RWREC_H
#define _SKHEAP_RWREC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKHEAP_RWREC_H, "$SiLK: skheap-rwrec.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/*
**  skheap-rwrec.h
**
**  A heap (priority queue) for rwRec pointers
**
**
*/

#include <silk/rwrec.h>

/* The Heap object */
typedef struct skrwrecheap_st skrwrecheap_t;

/* Returns a new heap with space for initial_entries.  Returns NULL on
 * memory allocation faulure. */
skrwrecheap_t *
skRwrecHeapCreate(
    size_t              initial_entries);

/* Destroy the heap (does not destroy the rwRecs in the heap */
void
skRwrecHeapDestroy(
    skrwrecheap_t      *heap);

/* Adds an rwRec to the heap.  Returns 0 on success, -1 on memory
 * allocation failure. */
int
skRwrecHeapInsert(
    skrwrecheap_t      *heap,
    rwRec              *rec);

/* Returns a pointer to the top entry on the heap, NULL if the heap is
 * empty */
const rwRec *
skRwrecHeapPeek(
    skrwrecheap_t      *heap);

/* Removes the top entry on the heap, returns that item.  Returns NULL
 * if the heap is empty.  */
rwRec *
skRwrecHeapPop(
    skrwrecheap_t      *heap);

/* Return the number of entries in the heap. */
size_t
skRwrecHeapCountEntries(
    const skrwrecheap_t    *heap);

/* Return the capacity of the heap */
size_t
skRwrecHeapGetCapacity(
    const skrwrecheap_t    *heap);

#ifdef __cplusplus
}
#endif
#endif /* _SKHEAP_RWREC_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
