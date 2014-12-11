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

/*
**  skmsg.h
**
**    Two-sided message queues over sockets
**
*/
#ifndef _SKMSG_H
#define _SKMSG_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKMSG_H, "$SiLK: skmsg.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/silk_types.h>

/* The number of bytes of network overhead when sending a message  */
#define SKMSG_MESSAGE_OVERHEAD 6

/* The control channel */
#define SKMSG_CHANNEL_CONTROL 0xFFFF
/* The message type of non-user-defined error messages */
#define SKMSG_TYPE_ERROR      0xFFFF


/*** Control channel messages ***/

#define SKMSG_CTL_NEW_CONNECTION 0 /* New connection.  Contains the
                                      channel ID of the initial
                                      channel for the new
                                      connection.  */

#define SKMSG_CTL_CHANNEL_DIED   1 /* Channel died.  Contains the
                                      channel ID if the channel that
                                      died. */


/* Use for getting a single channel id out of a message */
#define SKMSG_CTL_MSG_GET_CHANNEL(x) ntohs(*(uint16_t *)skMsgMessage(x))


/*** Fundamental types ***/

/* The type of message queues */
struct sk_msg_queue_st;
typedef struct sk_msg_queue_st sk_msg_queue_t;

/* The type of messages */
struct sk_msg_st;
typedef struct sk_msg_st sk_msg_t;

/* Type of channel IDs */
typedef uint16_t skm_channel_t;

/* Type of message types */
typedef uint16_t skm_type_t;

/* Type of message lengths */
typedef uint16_t skm_len_t;

/* Type of address info for new channels */
typedef struct sk_new_channel_info_st {
    skm_channel_t   channel;
    sk_sockaddr_t   addr;
    unsigned        known : 1;    /* Whether addr is usable */
} sk_new_channel_info_t;


/*** Message queue API ***/

/* Create a message queue */
int
skMsgQueueCreate(
    sk_msg_queue_t    **queue);

/* Start a listener */
int
skMsgQueueBindTCP(
    sk_msg_queue_t             *queue,
    const sk_sockaddr_array_t  *addr);

#if SK_ENABLE_GNUTLS
/* Start a listener */
int
skMsgQueueBindTLS(
    sk_msg_queue_t             *queue,
    const sk_sockaddr_array_t  *addr);
#endif

/* Connect to an listening message queue */
int
skMsgQueueConnectTCP(
    sk_msg_queue_t     *queue,
    struct sockaddr    *addr,
    socklen_t           addrlen,
    skm_channel_t      *channel);

#if SK_ENABLE_GNUTLS
/* Connect to an listening message queue */
int
skMsgQueueConnectTLS(
    sk_msg_queue_t     *queue,
    struct sockaddr    *addr,
    socklen_t           addrlen,
    skm_channel_t      *channel);

/* Set a message queue's CA public key */
int
skMsgQueueAddCA(
    sk_msg_queue_t     *queue,
    const char         *cred_filename);

/* Set a message queue's Certificate/Keyfile from PKCS 1 PEM files*/
int
skMsgQueueAddCert(
    sk_msg_queue_t     *queue,
    const char         *cert_filename,
    const char         *key_filename);

/* Set a message queue's Certificate/Keyfile from PKCS 12 PEM files*/
int
skMsgQueueAddPKCS12(
    sk_msg_queue_t     *queue,
    const char         *cert_filename,
    const char         *password);

/* Cleans up after GNU TLS initialization allocations.  Only necessary
 * during shutdown if TLS functions have been used.  Is safe to call
 * even if TLS functions were not used. */
void
skMsgGnuTLSTeardown(
    void);

#endif /* SK_ENABLE_GNUTLS */

/* Shut down a message queue */
void
skMsgQueueShutdown(
    sk_msg_queue_t     *queue);

/* Shut down all associated message queues */
void
skMsgQueueShutdownAll(
    sk_msg_queue_t     *queue);

/* Destroy a message queue */
void
skMsgQueueDestroy(
    sk_msg_queue_t     *queue);

/* Send a message to the remote message queue */
int
skMsgQueueSendMessage(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    skm_type_t          type,
    const void         *message,
    skm_len_t           length);

/* Inject a message (into this message queue) */
int
skMsgQueueInjectMessage(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    skm_type_t          type,
    const void         *message,
    skm_len_t           length);

/* Send a message to the remote message queue.  Message is to be freed
   with free_fn */
int
skMsgQueueSendMessageNoCopy(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    skm_type_t          type,
    void               *message,
    skm_len_t           length,
    void              (*free_fn)(void *));

/* Send a message scattered across multiple pointers to the remote
   message queue.  Data is to be freed with free_fn */
int
skMsgQueueScatterSendMessageNoCopy(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    skm_type_t          type,
    uint16_t            num_sections,
    struct iovec       *sections,
    void              (*free_fn)(uint16_t, struct iovec *));


/* Inject a message (into this message queue). Message is to be freed
   with free_fn  */
int
skMsgQueueInjectMessageNoCopy(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    skm_type_t          type,
    void               *message,
    skm_len_t           length,
    void              (*free_fn)(void *));

/* Create a new stream from a channel */
int
skMsgChannelNew(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    skm_channel_t      *new_channel);

/* Split a channel onto a new queue */
int
skMsgChannelSplit(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    sk_msg_queue_t    **new_queue);

/* Move a channel to a different queue */
int
skMsgChannelMove(
    skm_channel_t       channel,
    sk_msg_queue_t     *queue);

/* Shut down a channel */
int
skMsgChannelKill(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel);

/* Get the next message */
int
skMsgQueueGetMessage(
    sk_msg_queue_t     *queue,
    sk_msg_t          **msg);

/* Get the next message from a specific channel */
int
skMsgQueueGetMessageFromChannel(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    sk_msg_t          **msg);

/* Get a channel's remote ID */
int
skMsgGetRemoteChannelID(
    sk_msg_queue_t     *queue,
    skm_channel_t       lchannel,
    skm_channel_t      *rchannel);

/* Fill a string with information about the connection associated with
   a channel.  Will return -1 on error, or number of characters it
   wrote or would have written (not counting terminating null) on
   success. */
int
skMsgGetConnectionInformation(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    char               *buffer,
    size_t              buffer_size);

/* Free a message */
void
skMsgDestroy(
    sk_msg_t           *msg);

/* Set the keepalive timeout for a connection (in seconds).  Zero
   means no keepalive.  Default: 0 */
int
skMsgSetKeepalive(
    sk_msg_queue_t     *queue,
    skm_channel_t       channel,
    uint16_t            keepalive);

/* Accessor functions on messages */
skm_channel_t
skMsgChannel(
    const sk_msg_t     *msg);
skm_type_t
skMsgType(
    const sk_msg_t     *msg);
skm_len_t
skMsgLength(
    const sk_msg_t     *msg);
const void *
skMsgMessage(
    const sk_msg_t     *msg);

#ifdef __cplusplus
}
#endif
#endif /* _SKMSG_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
