/*
** Copyright (C) 2012-2015 by Carnegie Mellon University.
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
#ifndef _LIBSENDRCV_H
#define _LIBSENDRCV_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_LIBSENDRCV_H, "$SiLK: libsendrcv.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/*
**  libsendrcv.h
**
**  Maintenance macros for the libsendrcv convenience library
*/

/*
 *  ENABLING DEBUGGING
 *
 *    Setting the SENDRCV_DEBUG macro (below) to an bitmask of the
 *    following flags turns on specific types of debug output for
 *    rwsender/rwreceiver.  To see the messages in the running
 *    application, you MUST run with --log-level=debug.
 *
 *    The bit flags are:
 *
 *    0x0001 DEBUG_SKMSG_OTHER -- Logs messages for channel
 *    creation/destruction, network connection issues, and thread
 *    start/stop in skmsg.c.
 *
 *    0x0002 DEBUG_RWTRANSFER_PROTOCOL -- Logs messages regarding the
 *    higher-level protocol between rwsender and rwreceiver and thread
 *    start/stop in rwtransfer.c, rwsender.c, rwreceiver.c.
 *
 *    0x0004 DEBUG_RWTRANSFER_CONTENT -- Logs messages consisting of
 *    offset and length information for each message sent between the
 *    sender and receiver that is part of the actual file being
 *    transferred.  Generally used to debug file corruption.
 *
 *    0x0008 DEBUG_RWRECEIVER_DISKFREE -- Logs a message reporting
 *    disk usage each time rwreceiver asks about the amount of free
 *    space available on the disk.
 *
 *    0x0010 DEBUG_SKMSG_POLL_TIMEOUT -- Logs a message once a second
 *    when the call to poll() times out---this includes reading on the
 *    internal channel, which mostly waits for events.
 *
 *    The following are mostly used to debug lock-ups:
 *
 *    0x0100 DEBUG_SKMSG_MUTEX -- Logs a message for each mutex
 *    lock/unlock in skmsg.c
 *
 *    0x0200 DEBUG_RWTRANSFER_MUTEX -- Logs a message for each mutex
 *    lock/unlock in rwtransfer.c, rwsender.c, rwreceiver.c
 *
 *    0x0400 DEBUG_MULTIQUEUE_MUTEX -- Logs a message for each mutex
 *    lock/unlock in multiqueue.c
 *
 *    0x0800 DEBUG_INTDICT_MUTEX -- Logs a message for each mutex
 *    lock/unlock in intdict.c
 *
 *    0x1000 DEBUG_SKMSG_FN -- Logs a message for each function entry
 *    and function return in skmsg.c
 *
 *
 */

#define DEBUG_SKMSG_OTHER           0x0001
#define DEBUG_RWTRANSFER_PROTOCOL   0x0002
#define DEBUG_RWTRANSFER_CONTENT    0x0004
#define DEBUG_RWRECEIVER_DISKFREE   0x0008

#define DEBUG_SKMSG_POLL_TIMEOUT    0x0010

#define DEBUG_SKMSG_MUTEX           0x0100
#define DEBUG_RWTRANSFER_MUTEX      0x0200
#define DEBUG_MULTIQUEUE_MUTEX      0x0400
#define DEBUG_INTDICT_MUTEX         0x0800

#define DEBUG_SKMSG_FN              0x1000


/* #define SENDRCV_DEBUG 0 */
#ifndef SENDRCV_DEBUG
#  if defined(GLOBAL_TRACE_LEVEL) && GLOBAL_TRACE_LEVEL == 9
#    define SENDRCV_DEBUG 0xffff
#  else
#    define SENDRCV_DEBUG 0
#  endif
#endif


#ifdef __cplusplus
}
#endif
#endif /* _LIBSENDRCV_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
