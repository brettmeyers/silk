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
#ifndef _CIRCBUF_H
#define _CIRCBUF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_CIRCBUF_H, "$SiLK: circbuf.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

/*
**  circbuf.h
**
**    Circular buffer API
**
**    A circular buffer is a thread-safe FIFO with a maximum memory
**    size.
*/

/*
 *    The normal maximum size (in bytes) of a single chunk in a
 *    circular buffer.  (Circular buffers are allocated in chunks, as
 *    neeeded.)  A single chunk will will always be at least 3 times
 *    the item_size, regardless of the value of
 *    CIRCBUF_CHUNK_MAX_SIZE.
 */
#define CIRCBUF_CHUNK_MAX_SIZE 0x20000 /* 128k */

struct circBuf_st;
typedef struct circBuf_st circBuf_t;

/*
 *    Creates a circular buffer which can contain at least up to
 *    item_count items of size item_size.  Returns NULL if not enough
 *    memory.  The created circular buffer may contain space for more
 *    than item_count items, up to the size of a circbuf chunk.
 */
circBuf_t *
circBufCreate(
    uint32_t            item_size,
    uint32_t            item_count);

/*
 *    Causes all threads waiting on a circular buffer to return.
 */
void
circBufStop(
    circBuf_t          *buf);

/*
 *    Destroys a circular buffer.
 */
void
circBufDestroy(
    circBuf_t          *buf);

/*
 *    Takes a circular buffer as an argument.  Returns a pointer to an
 *    empty memory location in the buffer of size item_size.  This
 *    location should be used to add data to the circular buffer.
 *    This location will not be returned by circBufNextTail() until
 *    circBufNextHead() is called again at least once.  This call will
 *    block if the buffer is full, and return NULL if circBufStop or
 *    circBufDestroy were called while waiting.
 */
uint8_t *
circBufNextHead(
    circBuf_t          *buf);

/*
 *    Takes a circular buffer as an argument.  Returns a pointer to a
 *    full memory location from the circular buffer of size item_size.
 *    This location will be the least recently added item from a call
 *    to circBufNextHead().  This location should be used to get data
 *    from the circular buffer.  This call will block if the buffer is
 *    empty, and return NULL if circBufStop or circBufDestroy were
 *    called while waiting.
 */
uint8_t *
circBufNextTail(
    circBuf_t          *buf);

#ifdef __cplusplus
}
#endif
#endif /* _CIRCBUF_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
