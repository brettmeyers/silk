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
#ifndef _UDPSOURCE_H
#define _UDPSOURCE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_UDPSOURCE_H, "$SiLK: udpsource.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>

struct skUDPSource_st;
typedef struct skUDPSource_st skUDPSource_t;


typedef int (*udp_source_reject_fn)(
    ssize_t         recv_data_len,
    void           *recv_data,
    void           *callback_data);
/*
 *    Signature of callback function.
 *
 *    The UDP source calls this function for each packet it collects
 *    from network or reads from a file.  The function is called with
 *    the length of the packet, the packet's data, and a
 *    'callback_data' object supplied by the caller.
 *
 *    If the function returns non-zero, the packet is rejected.  If
 *    the function returns 0, the packet is stored until request by
 *    the caller via the skUDPSourceNext() function.
 */


int
skUDPSourceCreateFromSockaddr(
    skUDPSource_t             **source,
    const sk_sockaddr_array_t  *from_address,
    const sk_sockaddr_array_t  *listen_address,
    uint32_t                    itemsize,
    uint32_t                    itemcount,
    udp_source_reject_fn        reject_pkt_fn,
    void                       *fn_callback_data);
/*
 *    Creates a UDP source representing a Berkeley socket.  The new
 *    source will be placed at the memory referenced by 'source'.
 *
 *    'from_address' contains the host that may connect to this
 *    socket; if it is NULL, any host may connect.  'listen_address'
 *    is the address/port pair to which this socket should bind().
 *
 *    'itemsize' is the maximum size of an individual packet.
 *    'itemcount' is the number of packets the created source can
 *    buffer in memory.
 *
 *    'reject_pkt_fn' is a function that will be called for every
 *    packet the UDP source receives, and 'fn_callback_data' is a
 *    parameter passed to that function.  If the 'reject_pkt_fn'
 *    returns a true value, the packet will be ignored.
 *
 *    Returns 0 on success, or -1 on failure.
 */


int
skUDPSourceCreateFromUnixDomain(
    skUDPSource_t         **source,
    const char             *uds,
    uint32_t                itemsize,
    uint32_t                itemcount,
    udp_source_reject_fn    reject_pkt_fn,
    void                   *fn_callback_data);
/*
 *    Creates a UDP source representing a Unix domain socket.  The new
 *    source will be placed at the memory referenced by 'source'.
 *
 *    'uds' is the path to a UNIX domain socket, which this function
 *    will remove (if existent) and create.
 *
 *    'itemsize' is the maximum size of an individual packet.
 *    'itemcount' is the number of packets the created source can
 *    buffer in memory.
 *
 *    'reject_pkt_fn' is a function that will be called for every
 *    packet the UDP source receives, and 'fn_callback_data' is a
 *    parameter passed to that function.  If the 'reject_pkt_fn'
 *    returns a true value, the packet will be ignored.
 *
 *    Returns 0 on success, or -1 on failure.
 */


int
skUDPSourceCreateFromFile(
    skUDPSource_t         **source,
    const char             *path,
    uint32_t                itemsize,
    udp_source_reject_fn    reject_pkt_fn,
    void                   *fn_callback_data);
/*
 *    Creates a UDP source representing a file of collected traffic.
 *    The new source will be placed at the memory referenced by
 *    'source'.
 *
 *    'path' is the filesystem path to the file to process.
 *
 *    'itemsize' is the size of an individual packet in the collected
 *    file.  All packets in this file must be of this size.
 *
 *    'reject_pkt_fn' is a function that will be called for every
 *    packet the UDP source receives, and 'fn_callback_data' is a
 *    parameter passed to that function.  If the 'reject_pkt_fn'
 *    returns a true value, the packet will be ignored.
 *
 *    Returns 0 on success, or -1 on failure.
 */


void
skUDPSourceStop(
    skUDPSource_t      *source);
/*
 *    Have the UDP Source stop processing data.
 */


void
skUDPSourceDestroy(
    skUDPSource_t      *source);
/*
 *    Free all memory associated with the UDP Source.
 */


uint8_t *
skUDPSourceNext(
    skUDPSource_t      *source);
/*
 *    Get the next piece of data collected/read by the UDP Source.
 */

#ifdef __cplusplus
}
#endif
#endif /* _UDPSOURCE_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
