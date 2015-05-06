/*
** Copyright (C) 2006-2015 by Carnegie Mellon University.
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
**  ipfixsource.c
**
**    Interface to pull flows from IPFIX/NetFlowV9/sFlow streams.
**
*/

#define LIBFLOWSOURCE_SOURCE 1
#include <silk/silk.h>

RCSIDENT("$SiLK: ipfixsource.c 2b1355d69f79 2015-02-13 23:12:41Z mthomas $");

#include <silk/libflowsource.h>
#include <silk/redblack.h>
#include <silk/rwrec.h>
#include <silk/skipfix.h>
#include <silk/sklog.h>
#include <silk/skthread.h>
#include <silk/utils.h>
#include "circbuf.h"

#ifdef  SKIPFIXSOURCE_TRACE_LEVEL
#define TRACEMSG_LEVEL SKIPFIXSOURCE_TRACE_LEVEL
#endif
#define TRACEMSG(lvl, msg)                      \
    TRACEMSG_TO_TRACEMSGLVL(lvl, msg)
#include <silk/sktracemsg.h>

/*
 *  Logging messages for function entry/return.
 *
 *    Define the macro SKIPFIXSOURCE_ENTRY_RETURN to trace entry to
 *    and return from the functions in this file.
 *
 *    Developers must use "TRACE_ENTRY" at the beginning of every
 *    function.  Use "TRACE_RETURN(x);" for functions that return the
 *    value "x", and use "TRACE_RETURN;" for functions that have no
 *    return value.
 */
/* #define SKIPFIXSOURCE_ENTRY_RETURN 1 */

#ifndef SKIPFIXSOURCE_ENTRY_RETURN
#  define TRACE_ENTRY
#  define TRACE_RETURN       return
#else
/*
 *  this macro is used when the extra-level debugging statements write
 *  to the log, since we do not want the result of the log's printf()
 *  to trash the value of 'errno'
 */
#define WRAP_ERRNO(x)                                           \
    do { int _saveerrno = errno; x; errno = _saveerrno; }       \
    while (0)

#  define TRACE_ENTRY                                   \
    WRAP_ERRNO(DEBUGMSG("Entering %s", __func__))

#  define TRACE_RETURN                                          \
    WRAP_ERRNO(DEBUGMSG("Exiting %s", __func__)); return

#endif  /* SKIPFIXSOURCE_ENTRY_RETURN */


/*
 *  IMPLEMENTATION NOTES
 *
 *  Each probe is represented by a single skIPFIXSource_t object.
 *
 *  For probes that process file-based IPFIX sources, the
 *  skIPFIXSource_t object contains an fBuf_t object.  When the caller
 *  invokes skIPFIXSourceGetGeneric(), the next record is read from
 *  the fBuf_t and the record is returned.  For consistency with
 *  network processing (described next), the file-based
 *  skIPFIXSource_t has an skIPFIXSourceBase_t object, but that object
 *  does little for file-based sources.
 *
 *  For probes that process network-based IPFIX sources, the
 *  combination of the following four values must be unique: protocol,
 *  listen-on-port, listen-as-address, accept-from-host.  (Note that
 *  an ADDR_ANY value for listen-as-address or accept-from-host
 *  matches all other addresses.)
 *
 *  Each skIPFIXSource_t references an skIPFIXSourceBase_t object.
 *  Each unique listen-as-address/listen-to-port/protocol triple is
 *  handled by a single fbListener_t object, which is contained in the
 *  skIPFIXSourceBase_t object.  When two skIPFIXSource_t's differ
 *  only by their accept-from-host addreses, the skIPFIXSource_t's
 *  reference the same skIPFIXSourceBase_t object.  The
 *  skIPFIXSourceBase_t objects contain a reference-count.  The
 *  skIPFIXSourceBase_t is destroyed when the last skIPFIXSource_t
 *  referring to it is destroyed.
 *
 *  An skIPFIXConnection_t represents a connection, which is one of
 *  two things: In the TCP case, a connection is equivalent to a TCP
 *  connection.  In the UDP case, a connection is a given set of IPFIX
 *  or NFv9 UDP packets sent from a given address, to a given address,
 *  on a given port, with a given domain ID.  The skIPFIXConnection_t
 *  object is ipfixsource's way of mapping to the fbSession_t object
 *  in libfixbuf.
 *
 *  There can be multiple active connections on a probe---consider a
 *  probe that collects from two machines that load-balance.  In the
 *  code, this is represented by having each skIPFIXConnection_t
 *  object point to its skIPFIXSource_t.  As described below, the
 *  skIPFIXConnection_t is stored as the context pointer on the
 *  libfixbuf fbCollector_t object.
 *
 *  When a new TCP connection arrives or if a new UDP connection is
 *  seen and we are using a fixbuf that supports multi-UDP, the
 *  fixbufConnect() callback function first determines whether the
 *  peer is allowed to connect.  If the peer is allowed, the function
 *  sets the context pointer for the fbCollector_t object to the a new
 *  skIPFIXConnection_t object which contains statistics information
 *  for the connection and the skIPFIXSource_t object associated with
 *  the connection.  These skIPFIXConnection_t objects are destroyed
 *  in the fixbufDisconnect() callback.
 *
 *  When a new UDP peer sends data to the listener, the actual address
 *  is not known until the underlying recvmesg() call itself, rather
 *  than in an accept()-like call similar to TCP.  What this means is
 *  that in this scenario the fixbufConnect() appInit function is not
 *  called until a call to fBufNext() or fBufNextCollectionTemplate()
 *  is called.
 *
 *  There is a similar fixbufConnectUDP() function to handle UDP
 *  connections when libfixbuf does not support multi-UDP.  However,
 *  the fundamental difference is this: TCP connections are associated
 *  with a new fbCollector_t at connection time.  Non-multi-UDP
 *  connections are associated with a new fbCollector_t during the
 *  fbListenerAlloc() call.
 *
 *  FIXBUF API ISSUE: The source objects connected to the
 *  fbCollector_t objects have to be passed to the
 *  fixbufConnect*() calls via global objects---newly created
 *  sources are put into a red-black tree; the call to
 *  fixbufConnect*() attempts to find the value in the red-black tree.
 *  It would have made more sense if fbListenerAlloc() took a
 *  caller-specified context pointer which would get passed to the
 *  fbListenerAppInit_fn() and fbListenerAppFree_fn() functions.
 *
 *  There is one ipfix_reader() thread per skIPFIXSourceBase_t object.
 *  This thread loops around fbListenerWait() returning fBuf_t
 *  objects.  The underlying skIPFIXConnection_t containing the source
 *  information is grabbed from the fBuf_t's collector.  The
 *  fBufNext() is used to read the data from the fBuf_t and this data
 *  is associated with the given source by either inserting it into
 *  the source's circular buffer, or by adding the stats information
 *  to the source.  Then we loop back determining any new connection
 *  and dealing with the next piece of data until the fBuf_t empties.
 *  We then return to fbListenerWait() to get the next fBuf_t.

 *  Since there is one thread per listener, if one source attached to
 *  a listener blocks due to the circular buffer becoming full, all
 *  sources attached to the listener will block as well.  Solving this
 *  problem would involve more threads, and moving away from the
 *  fbListenerWait() method of doing things.  We could instead have a
 *  separate thread per connection.  This would require us to handle
 *  the connections (bind/listen/accept) ourselves, and then create
 *  fBufs from the resulting file descriptors.
 */


/* LOCAL DEFINES AND TYPEDEFS */

/*
 *    The NetFlowV9/IPFIX standard stays that a 'stream' is unique if
 *    the source-address and domain are unique.  SiLK violates the
 *    standard in that it also treats the sending port as part of the
 *    unique 'stream' key.
 *
 *    To have SiLK follow the standard---that is, to treat UDP packets
 *    coming from the same source address but different source ports
 *    as being part of the same protocol stream, set the following
 *    environment variable prior to invoking rwflowpack or flowcap.
 */
#define SK_IPFIX_UDP_IGNORE_SOURCE_PORT "SK_IPFIX_UDP_IGNORE_SOURCE_PORT"

/* error codes used in callback that fixbuf calls */
#define SK_IPFIXSOURCE_DOMAIN  g_quark_from_string("silkError")
#define SK_IPFIX_ERROR_CONN    1


/* Name of environment variable that, when set, cause SiLK to ignore
 * any G_LOG_LEVEL_WARNING messages. */
#define SK_ENV_FIXBUF_SUPPRESS_WARNING "SILK_LIBFIXBUF_SUPPRESS_WARNINGS"


/* Various macros for handling yaf stats */

/* compute difference of fields at current time 'stats' and previous
 * time 'last'; last and stats are ski_yaf_stats_t structures */
#define STAT_REC_FIELD_DIFF(last, stats, field) \
    (stats.field - last.field)

/* update current counts on skIPFIXSource_t 'source' with the values
 * at current time 'stats' compared with those at previous time
 * 'last'; last and stats are ski_yaf_stats_t structures */
#define INCORPORATE_STAT_RECORD(source, last, stats)                    \
    pthread_mutex_lock(&source->stats_mutex);                           \
    source->saw_yaf_stats_pkt = 1;                                      \
    if (stats.systemInitTimeMilliseconds != last.systemInitTimeMilliseconds) \
    {                                                                   \
        memset(&(last), 0, sizeof(last));                               \
    }                                                                   \
    source->yaf_dropped_packets +=                                      \
        STAT_REC_FIELD_DIFF(last, stats, droppedPacketTotalCount);      \
    source->yaf_ignored_packets +=                                      \
        STAT_REC_FIELD_DIFF(last, stats, ignoredPacketTotalCount);      \
    source->yaf_notsent_packets +=                                      \
        STAT_REC_FIELD_DIFF(last, stats, notSentPacketTotalCount);      \
    source->yaf_expired_fragments +=                                    \
        STAT_REC_FIELD_DIFF(last, stats, expiredFragmentCount);         \
    source->yaf_processed_packets +=                                    \
        STAT_REC_FIELD_DIFF(last, stats, packetTotalCount);             \
    source->yaf_exported_flows +=                                       \
        STAT_REC_FIELD_DIFF(last, stats, exportedFlowRecordTotalCount); \
    last = stats;                                                       \
    pthread_mutex_unlock(&source->stats_mutex);

#define TRACEMSG_YAF_STATS(source, stats)               \
    TRACEMSG(1, (("'%s': "                              \
                 "inittime %" PRIu64                    \
                 ", dropped %" PRIu64                   \
                 ", ignored %" PRIu64                   \
                 ", notsent %" PRIu64                   \
                 ", expired %" PRIu32                   \
                 ", pkttotal %" PRIu64                  \
                 ", exported %" PRIu64),                \
                 (source)->name,                        \
                 (stats).systemInitTimeMilliseconds,    \
                 (stats).droppedPacketTotalCount,       \
                 (stats).ignoredPacketTotalCount,       \
                 (stats).notSentPacketTotalCount,       \
                 (stats).expiredFragmentCount,          \
                 (stats).packetTotalCount,              \
                 (stats).exportedFlowRecordTotalCount))


/*
 *  SILK_PROTO_TO_FIXBUF_TRANSPORT(silk_proto, &fb_trans);
 *
 *    Set the fbTransport_t value in the memory referenced by
 *    'fb_trans' based on the SiLK protocol value 'silk_proto'.
 */
#define SILK_PROTO_TO_FIXBUF_TRANSPORT(silk_proto, fb_trans)    \
    switch (silk_proto) {                                       \
      case SKPC_PROTO_SCTP:                                     \
        *(fb_trans) = FB_SCTP;                                  \
        break;                                                  \
      case SKPC_PROTO_TCP:                                      \
        *(fb_trans) = FB_TCP;                                   \
        break;                                                  \
      case SKPC_PROTO_UDP:                                      \
        *(fb_trans) = FB_UDP;                                   \
        break;                                                  \
      default:                                                  \
        skAbortBadCase(silk_proto);                             \
    }


/* forward declarations */
struct skIPFIXSourceBase_st;
typedef struct skIPFIXSourceBase_st skIPFIXSourceBase_t;

/* The skIPFIXSource_t object represents a single source, as mapped to
 * be a single probe. */
struct skIPFIXSource_st {
    /* when reading from a file-based source, if we get both a forward
     * and reverse record (a biflow) from libfixbuf, we temporarily
     * cache the reverse record here.  For network biflows, both
     * records are stored in the circular buffer.  The 'reverse'
     * member says whether 'rvbuf' contains an unread record.  */
    skIPFIXSourceRecord_t   rvbuf;

    /* when reading from a file-based source, this contains the counts
     * of statistics for this file.  when reading from the network,
     * the statistics are maintained per connection on the
     * skIPFIXConnection_t object. */
    ski_yaf_stats_t         last_yaf_stats;

    /* for yaf sources, packets dropped by libpcap, libdag,
     * libpcapexpress.  For NetFlowV9/sFlow sources, number of packets
     * that were missed. */
    uint64_t                yaf_dropped_packets;

    /* packets ignored by yaf (unsupported packet types; bad headers) */
    uint64_t                yaf_ignored_packets;

    /* packets rejected by yaf due to being out-of-sequence */
    uint64_t                yaf_notsent_packets;

    /* packet fragments expired by yaf (e.g., never saw first frag) */
    uint64_t                yaf_expired_fragments;

    /* packets processed by yaf */
    uint64_t                yaf_processed_packets;

    /* exported flow record count */
    uint64_t                yaf_exported_flows;

    /* these next values are based on records the ipfixsource gets
     * from skipfix */
    uint64_t                forward_flows;
    uint64_t                reverse_flows;
    uint64_t                ignored_flows;

    /* mutex to protect access to the above statistics */
    pthread_mutex_t         stats_mutex;

    /* source's base */
    skIPFIXSourceBase_t    *base;

    /* probe associated with this source and its name */
    const skpc_probe_t     *probe;
    const char             *name;

    /* when reading from the network, 'data_buffer' holds packets
     * collected for this probe but not yet requested.
     * 'current_record' is the current location in the
     * 'data_buffer'. */
    circBuf_t              *data_buffer;
    skIPFIXSourceRecord_t  *current_record;

    /* buffer for file based reads */
    fBuf_t                 *readbuf;

    /* file for file-based reads */
    sk_fileptr_t            fileptr;

    /* for NetFlowV9/sFlow sources, a red-black tree of
     * skIPFIXConnection_t objects that currently point to this
     * skIPFIXSource_t, keyed by the skIPFIXConnection_t pointer. */
    struct rbtree          *connections;

    /* count of skIPFIXConnection_t's associated with this source */
    uint32_t                connection_count;

    /* Whether this source has been stopped */
    unsigned                stopped             :1;

    /* Whether this source has been marked for destructions */
    unsigned                destroy             :1;

    /* whether the 'rvbuf' field holds a valid record (1==yes) */
    unsigned                reverse             :1;

    /* Whether this source has received a STATS packet from yaf.  The
     * yaf stats are only written to the log once a stats packet has
     * been received.  */
    unsigned                saw_yaf_stats_pkt   :1;
};
/* typedef struct skIPFIXSource_st skIPFIXSource_t;  // libflowsource.h */


/* This object represents a single listening port or file */
struct skIPFIXSourceBase_st {
    /* when a probe does not have an accept-from-host clause, any peer
     * may connect, and there is a one-to-one mapping between a source
     * object and a base object.  The 'any' member points to the
     * source, and the 'addr_to_source' member must be NULL. */
    skIPFIXSource_t    *any;

    /* if there is an accept-from clause, the 'addr_to_source'
     * red-black tree maps the address of the peer to a particular
     * source object (via 'peeraddr_source_t' objects), and the 'any'
     * member must be NULL. */
    struct rbtree      *addr_to_source;

    /* address we are listening to. This is an array to support a
     * hostname that maps to multiple IPs (e.g. IPv4 and IPv6). */
    const sk_sockaddr_array_t *listen_address;

    pthread_t           thread;
    pthread_mutex_t     mutex;
    pthread_cond_t      cond;

    /* the listener and connection objects from libfixbuf */
    fbListener_t       *listener;
    fbConnSpec_t       *connspec;

    /* A count of sources associated with this base object */
    uint32_t            source_count;

    /* whether the source is in the process of being destroyed */
    unsigned            destroyed : 1;

    /* Whether the reading thread was started */
    unsigned            started   : 1;

    /* Whether the reading thread is currently running */
    unsigned            running   : 1;
};

/* Data for "active" connections */
typedef struct skIPFIXConnection_st {
    skIPFIXSource_t    *source;
    ski_yaf_stats_t     last_yaf_stats;
    /* Address of the host that contacted us */
    sk_sockaddr_t       peer_addr;
    size_t              peer_len;
    /* The observation domain id. */
    uint32_t            ob_domain;
} skIPFIXConnection_t;


/*
 *    The 'addr_to_source' member of 'skIPFIXSourceBase_t' is a
 *    red-black tree whose data members are 'peeraddr_source_t'
 *    objects.  The tree is used when multiple sources listen on the
 *    same port and the accept-from-host addresses are used to choose
 *    the source based on the peer address of the sender.
 *
 *    The 'addr_to_source' tree uses the peeraddr_compare() comparison
 *    function.
 */
typedef struct peeraddr_source_st {
    const sk_sockaddr_t *addr;
    skIPFIXSource_t     *source;
} peeraddr_source_t;


/* LOCAL VARIABLE DEFINITIONS */

/* Mutex around calls to skiCreateListener. */
static pthread_mutex_t create_listener_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Mutex around listener_to_source_base tree and count. */
static pthread_mutex_t global_tree_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Map from listeners to skIPFIXSourceBase_t objects.  Objects in
 * rbtree are skIPFIXSourceBase_t pointers. */
static struct rbtree *listener_to_source_base = NULL;

/* Number of ipfix sources (both networked and file-based) */
static uint32_t source_base_count = 0;


/* FUNCTION DEFINITIONS */

/*
 *     The listener_to_source_base_find() function is used as the
 *     comparison function for the listener_to_source_base red-black
 *     tree.  Stores objects of type skIPFIXSourceBase_t, orders by
 *     fbListener_t pointer value.
 */
static int
listener_to_source_base_find(
    const void         *va,
    const void         *vb,
    const void  UNUSED(*ctx))
{
    const fbListener_t *a = ((const skIPFIXSourceBase_t *)va)->listener;
    const fbListener_t *b = ((const skIPFIXSourceBase_t *)vb)->listener;
    return ((a < b) ? -1 : (a > b));
}


/*
 *     The peeraddr_compare() function is used as the comparison
 *     function for the skIPFIXSourceBase_t's red-black tree,
 *     addr_to_source.
 *
 *     The tree stores peeraddr_source_t objects, keyed by
 *     sk_sockaddr_t address of the accepted peers.
 */
static int
peeraddr_compare(
    const void         *va,
    const void         *vb,
    const void  UNUSED(*ctx))
{
    const sk_sockaddr_t *a = ((const peeraddr_source_t *)va)->addr;
    const sk_sockaddr_t *b = ((const peeraddr_source_t *)vb)->addr;

    return skSockaddrCompare(a, b, SK_SOCKADDRCOMP_NOPORT);
}


/*
 *     The pointer_cmp() function is used compare skIPFIXConnection_t
 *     pointers in the 'connections' red-black tree on skIPFIXSource_t
 *     objects.
 */
static int
pointer_cmp(
    const void         *va,
    const void         *vb,
    const void  UNUSED(*ctx))
{
    return ((va < vb) ? -1 : (va > vb));
}


/*
 *     The free_source() function is used to free an skIPFIXSource_t
 *     object.  This only frees the object and its data, it does not
 *     mark up any connected skIPFIXSourceBase_t object in the
 *     process.
 */
static void
free_source(
    skIPFIXSource_t    *source)
{
    TRACE_ENTRY;

    if (source == NULL) {
        TRACE_RETURN;
    }

    assert(source->connection_count == 0);

    pthread_mutex_destroy(&source->stats_mutex);
    if (source->data_buffer) {
        circBufDestroy(source->data_buffer);
    }
    if (source->connections) {
        rbdestroy(source->connections);
    }
    if (source->readbuf) {
        fBufFree(source->readbuf);
    }
    if (source->fileptr.of_fp) {
        skFileptrClose(&source->fileptr, &WARNINGMSG);
    }

    free(source);
    TRACE_RETURN;
}


/*
 *     The fixbufConnect() function is passed to fbListenerAlloc() as
 *     its 'appinit' callback (fbListenerAppInit_fn) for TCP sources
 *     and UDP sources if libfixbuf supports multi-UDP (v1.2.0 or later).
 *     This function is called from within the fbListenerWait() call
 *     when a new connection to the listening socket is made.  (In
 *     addition, for UDP sources, it is called directly by
 *     fbListenerAlloc() with a NULL peer.)
 *
 *     Its primary purposes are to accept/reject the connection,
 *     create an skIPFIXConnection_t, and set the the collector's
 *     context to the skIPFIXConnection_t.  The skIPFIXConnection_t
 *     remembers the peer information, contains the stats for this
 *     connection, and references the source object.
 */
static gboolean
fixbufConnect(
    fbListener_t           *listener,
    void                  **ctx,
    int              UNUSED(fd),
    struct sockaddr        *peer,
    size_t                  peerlen,
    GError                **err)
{
    fbCollector_t *collector;
    char addr_buf[2 * SK_NUM2DOT_STRLEN];
    skIPFIXSourceBase_t target_base;
    skIPFIXSourceBase_t *base;
    const peeraddr_source_t *found_peer;
    peeraddr_source_t target_peer;
    skIPFIXSource_t *source;
    skIPFIXConnection_t *conn = NULL;
    sk_sockaddr_t addr;
    gboolean retval = 0;

    TRACE_ENTRY;

    if (peer == NULL) {
        /* This function is being called for a UDP listener at init
         * time.  Ignore this. */
        TRACE_RETURN(1);
    }
    if (peerlen > sizeof(addr)) {
        TRACEMSG(1, (("ipfixsource rejected connection:"
                      " peerlen too large: %" SK_PRIuZ " > %" SK_PRIuZ),
                     peerlen, sizeof(addr)));
        g_set_error(err, SK_IPFIXSOURCE_DOMAIN, SK_IPFIX_ERROR_CONN,
                    ("peerlen unexpectedly large: %" SK_PRIuZ), peerlen);
        TRACE_RETURN(0);
    }

    memcpy(&addr.sa, peer, peerlen);
    skSockaddrString(addr_buf, sizeof(addr_buf), &addr);

    TRACEMSG(3, (("ipfixsource processing connection from '%s'"), addr_buf));

    /* Find the skIPFIXSourceBase_t object associated with this
     * listener */
    target_base.listener = listener;
    pthread_mutex_lock(&global_tree_mutex);
    base = ((skIPFIXSourceBase_t *)
            rbfind(&target_base, listener_to_source_base));
    pthread_mutex_unlock(&global_tree_mutex);
    if (base == NULL) {
        TRACEMSG(1, (("ipfixsource rejected connection from '%s':"
                      " unable to find base given listener"), addr_buf));
        g_set_error(err, SK_IPFIXSOURCE_DOMAIN, SK_IPFIX_ERROR_CONN,
                    "Unable to find base for listener");
        TRACE_RETURN(0);
    }

    conn = (skIPFIXConnection_t*)calloc(1, sizeof(skIPFIXConnection_t));
    if (conn == NULL) {
        TRACEMSG(1, (("ipfixsource rejected connection from '%s':"
                      " unable to allocate connection object"), addr_buf));
        g_set_error(err, SK_IPFIXSOURCE_DOMAIN, SK_IPFIX_ERROR_CONN,
                    "Unable to allocate connection object");
        TRACE_RETURN(0);
    }

    pthread_mutex_lock(&base->mutex);

    if (base->any) {
        /* When there is no accept-from address on the probe, there is
         * a one-to-one mapping between source and base, and all
         * connections are permitted. */
        source = base->any;
    } else {
        /* Using the address of the incoming connection, search for
         * the source object associated with this address. */
        assert(base->addr_to_source);
        target_peer.addr = &addr;
        found_peer = ((const peeraddr_source_t*)
                      rbfind(&target_peer, base->addr_to_source));
        if (NULL == found_peer) {
            /* Reject hosts that do not appear in accept-from-host */
            TRACEMSG(1, (("ipfixsource rejected connection from '%s':"
                          " host prohibited"), addr_buf));
            g_set_error(err, SK_IPFIXSOURCE_DOMAIN, SK_IPFIX_ERROR_CONN,
                        "Connection prohibited from %s", addr_buf);
            free(conn);
            goto END;
        }
        source = found_peer->source;
    }

    if (source->stopped) {
        TRACEMSG(1, (("ipfixsource rejected connection from '%s':"
                      " source is stopping"), addr_buf));
        g_set_error(err, SK_IPFIXSOURCE_DOMAIN, SK_IPFIX_ERROR_CONN,
                    "Source is stopping");
        free(conn);
        goto END;
    }

    /* If this is an NetFlowV9/sFLow source, store the
     * skIPFIXConnection_t in the red-black tree on the source so we
     * can log about missing NetFlowV9/sFlow packets. */
    if (source->connections) {
        skIPFIXConnection_t *found_conn;

        pthread_mutex_lock(&source->stats_mutex);
        found_conn = ((skIPFIXConnection_t*)
                      rbsearch(conn, source->connections));
        pthread_mutex_unlock(&source->stats_mutex);
        if (found_conn != conn) {
            TRACEMSG(1, (("ipfixsource rejected connection from '%s':"
                          " unable to store connection on source"), addr_buf));
            g_set_error(err, SK_IPFIXSOURCE_DOMAIN, SK_IPFIX_ERROR_CONN,
                        "Unable to store connection on source");
            free(conn);
            goto END;
        }
    }

    /* Update the skIPFIXConnection_t with the information necessary
     * to provide a useful log message at disconnect.  This info is
     * also used to get NetFlowV9/sFlow missed packets. */
    if (peerlen <= sizeof(conn->peer_addr)) {
        memcpy(&conn->peer_addr.sa, peer, peerlen);
        conn->peer_len = peerlen;
    }

    TRACEMSG(4, ("Creating new conn = %p for source = %p", conn, source));

    /* Set the skIPFIXConnection_t to point to the source, increment
     * the source's connection_count, and set the context pointer to
     * the connection.  */
    conn->source = source;
    ++source->connection_count;
    retval = 1;
    *ctx = conn;

    /* Get the domain (also needed for NetFlowV9/sFlow missed pkts).
     * In the TCP case, the collector does not exist yet, and the
     * following call returns false. */
    if (fbListenerGetCollector(listener, &collector, NULL)) {
        conn->ob_domain = fbCollectorGetObservationDomain(collector);
        INFOMSG("'%s': accepted connection from %s, domain 0x%04x",
                source->name, addr_buf, conn->ob_domain);
    } else {
        INFOMSG("'%s': accepted connection from %s",
                source->name, addr_buf);
    }

  END:
    pthread_mutex_unlock(&base->mutex);
    TRACE_RETURN(retval);
}


/*
 *     The fixbufDisconnect() function is passed to fbListenerAlloc()
 *     as its 'appfree' callback (fbListenerAppFree_fn).  This
 *     function is called by fBufFree().  The argument to this
 *     function is the context (the skIPFIXConnection_t) that was set
 *     by fixbufConnect().
 *
 *     The function decrefs the source and frees it if the
 *     connection_count hits zero and the source has been asked to be
 *     destroyed.  It then frees the connection object.
 */
static void
fixbufDisconnect(
    void               *ctx)
{
    skIPFIXConnection_t *conn = (skIPFIXConnection_t *)ctx;

    TRACE_ENTRY;

    if (conn == NULL) {
        TRACE_RETURN;
    }

    TRACEMSG(3, (("fixbufDisconnection connection_count = %" PRIu32),
                 conn->source->connection_count));

    /* Remove the connection from the source. */
    --conn->source->connection_count;
    if (conn->source->connections) {
        pthread_mutex_lock(&conn->source->stats_mutex);
        rbdelete(conn, conn->source->connections);
        pthread_mutex_unlock(&conn->source->stats_mutex);
    }

    /* For older fixbuf, only TCP connections contain the peer addr */
    if (conn->peer_len) {
        char addr_buf[2 * SK_NUM2DOT_STRLEN];

        skSockaddrString(addr_buf, sizeof(addr_buf), &conn->peer_addr);
        if (conn->ob_domain) {
            INFOMSG("'%s': noticed disconnect by %s, domain 0x%04x",
                    conn->source->name, addr_buf, conn->ob_domain);
        } else {
            INFOMSG("'%s': noticed disconnect by %s",
                    conn->source->name, addr_buf);
        }
    }

    TRACEMSG(4, ("Destroying conn = %p for source %p", conn, conn->source));

    /* Destroy it if this is the last reference to the source. */
    if (conn->source->destroy && conn->source->connection_count == 0) {
        free_source(conn->source);
    }
    free(conn);
    TRACE_RETURN;
}


/*
 *    THREAD ENTRY POINT
 *
 *    The ipfix_reader() function is the main thread for listening to
 *    data from a single fbListener_t object.  It is passed the
 *    skIPFIXSourceBase_t object containing that fbListener_t object.
 *    This thread is started from the ipfixSourceCreateFromSockaddr()
 *    function.
 */
static void *
ipfix_reader(
    void               *vsource_base)
{
#define IS_UDP (base->connspec->transport == FB_UDP)
    skIPFIXSourceBase_t *base = (skIPFIXSourceBase_t *)vsource_base;
    skIPFIXConnection_t *conn = NULL;
    skIPFIXSource_t *source = NULL;
    GError *err = NULL;
    fBuf_t *ipfix_buf = NULL;
    skIPFIXSourceRecord_t reverse;
    int rv;

    TRACE_ENTRY;

    /* Ignore all signals */
    skthread_ignore_signals();

    /* Communicate that the thread has started */
    pthread_mutex_lock(&base->mutex);
    pthread_cond_signal(&base->cond);
    base->started = 1;
    base->running = 1;
    DEBUGMSG("fixbuf listener started for [%s]:%s",
             base->connspec->host ? base->connspec->host : "*",
             base->connspec->svc);
    pthread_mutex_unlock(&base->mutex);

    TRACEMSG(3, ("base %p started for [%s]:%s",
                 base, base->connspec->host ? base->connspec->host : "*",
                 base->connspec->svc));

    /* Loop until destruction of the base object */
    while (!base->destroyed) {

        /* wait for a new connection */
        ipfix_buf = fbListenerWait(base->listener, &err);
        if (NULL == ipfix_buf) {
            if (NULL == err) {
                /* got an unknown error---treat as fatal */
                NOTICEMSG("fixbuf listener shutting down:"
                          " unknown error from fbListenerWait");
                break;
            }

            if (g_error_matches(err,SK_IPFIXSOURCE_DOMAIN,SK_IPFIX_ERROR_CONN))
            {
                /* the callback rejected the connection (TCP only) */
                DEBUGMSG("fixbuf listener rejected connection: %s",
                         err->message);
                g_clear_error(&err);
                continue;
            }

            /* FB_ERROR_NLREAD indicates interrupted read, either
             * because the socket received EINTR or because
             * fbListenerInterrupt() was called.
             *
             * FB_ERROR_EOM indicates an end-of-message, and needs to
             * be ignored when running in manual mode. */
            if (g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_NLREAD)
                || g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_EOM))
            {
                TRACEMSG(1, (("fixbuf listener received %s"
                              " while waiting for a connection: %s"),
                             ((FB_ERROR_EOM == err->code)
                              ? "end-of-message" : "interrupted read"),
                             err->message));
                g_clear_error(&err);
                continue;
            }

            /* treat any other error as fatal */
            NOTICEMSG(("fixbuf listener shutting down: %s"
                       " (d=%" PRIu32 ",c=%" PRId32 ")"),
                      err->message, (uint32_t)err->domain, (int32_t)err->code);
            g_clear_error(&err);
            break;
        }

        /* Make sure the fbuf is in manual mode.  Manual mode is
         * required to multiplex among multiple collectors using
         * fbListenerWait().  Without this, fBufNext() blocks once the
         * buffer is empty until it has messages again.  Instead, we
         * want to switch to a different fbuf once we read all records
         * in the current buffer. */
        fBufSetAutomaticMode(ipfix_buf, 0);

        /* Invoke a callback when a new template arrives that tells
         * fixbuf how to map from the subTemplateMultiList used by YAF
         * for TCP information to our internal strucure. */
        skiAddSessionCallback(fBufGetSession(ipfix_buf));

        /* Loop over fBufNext() until the buffer empties, we begin to
         * shutdown, or there is an error.  skiYafNextStats() and
         * skiRwNextRecord() call fBufNext() internally.
         *
         * There is a 'break' statement after the switch(), so any
         * "normal" record (no error condition and buffer is not
         * empty) should call 'continue' after processing to continue
         * the loop. */
        while (!base->destroyed) {
            ski_rectype_t rectype;
            ski_yaf_stats_t stats;

            /* Determine what type of record is next */
            rectype = skiGetNextRecordType(ipfix_buf, &err);

            /* Get the connection data associated with this fBuf_t object */
            conn = ((skIPFIXConnection_t *)
                    fbCollectorGetContext(fBufGetCollector(ipfix_buf)));
            if (conn == NULL) {
                /* If conn is NULL, we must have rejected a UDP
                 * connection from the appInit function. */
                assert(rectype == SKI_RECTYPE_ERROR);
                TRACEMSG(2, ("<UNKNOWN>: SKI_RECTYPE_ERROR"));
                break;
            }
            source = conn->source;
            assert(source != NULL);

            TRACEMSG(5, ("'%s': conn = %p, source = %p, ipfix_buf = %p",
                         source->name, conn, source, ipfix_buf));

            /* If this source has been stopped, end the connection */
            if (source->stopped) {
                TRACEMSG(1,("'%s': Closing connection since source is stopping",
                            source->name));
                if (!IS_UDP) {
                    fBufFree(ipfix_buf);
                    ipfix_buf = NULL;
                }
                if (rectype == SKI_RECTYPE_ERROR) {
                    g_clear_error(&err);
                }
                break;
            }

            /* All successful code in this switch() must use
             * 'continue'.  Any 'break' indicates an error. */
            switch (rectype) {
              case SKI_RECTYPE_ERROR:
                TRACEMSG(2, ("'%s': SKI_RECTYPE_ERROR", source->name));
                break;          /* error */

              case SKI_RECTYPE_UNKNOWN:
                /* This occurs when there is an unknown options
                 * template.  In this case, the safe thing to do is to
                 * transcode it with a stats template (a valid options
                 * template) and ignore the result.  */
                if (!skiYafNextStats(ipfix_buf, source->probe, &stats, &err)) {
                    /* should have been able to read something */
                    TRACEMSG(2, (("'%s': SKI_RECTYPE_UNKNOWN and"
                                  " NextStats() is FALSE"), source->name));
                    break;      /* error */
                }
                continue;

              case SKI_RECTYPE_STATS:
                if (!skiYafNextStats(ipfix_buf, source->probe, &stats, &err)) {
                    /* should have been able to read the stats */
                    TRACEMSG(2, (("'%s': SKI_RECTYPE_STATS and"
                                  " NextStats is FALSE"), source->name));
                    break;      /* error */
                }
                DEBUGMSG("'%s': Got a yaf stats record", source->name);
                TRACEMSG_YAF_STATS(source, stats);

                /* There is a guarantee that new connections to
                 * yaf always start with zeroed statistics */
                INCORPORATE_STAT_RECORD(source, conn->last_yaf_stats, stats);
                continue;

#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
              case SKI_RECTYPE_NF9_SAMPLING:
                if (!skiNextSamplingOptionsTemplate(
                        ipfix_buf, source->probe, &err))
                {
                    /* should have been able to read something */
                    TRACEMSG(2, (("'%s': SKI_RECTYPE_UNKNOWN and"
                                  " NextStats() is FALSE"), source->name));
                    break;      /* error */
                }
                continue;
#endif

              case SKI_RECTYPE_FLOW:
                /* Get the next SiLK record. */
                assert(source->current_record);
                rv = skiRwNextRecord(ipfix_buf, source->probe,
                                     source->current_record, &reverse, &err);
                switch (rv) {
                  case -1:
                    TRACEMSG(2, ("'%s': SKI_RECTYPE_FLOW and NextRecord is -1",
                                 source->name));
                    break;      /* error */

                  case 0:
                    /* Ignore record */
                    pthread_mutex_lock(&source->stats_mutex);
                    ++source->ignored_flows;
                    pthread_mutex_unlock(&source->stats_mutex);
                    continue;

                  case 1:
                    /* We have filled the empty source->current_record slot.
                     * Advance to the next record location.  */
                    source->current_record
                        = ((skIPFIXSourceRecord_t*)
                           circBufNextHead(source->data_buffer));
                    if (source->current_record == NULL) {
                        assert(source->stopped);
                        continue;
                    }
                    pthread_mutex_lock(&source->stats_mutex);
                    ++source->forward_flows;
                    pthread_mutex_unlock(&source->stats_mutex);
                    continue;

                  case 2:
                    source->current_record
                        = ((skIPFIXSourceRecord_t*)
                           circBufNextHead(source->data_buffer));
                    if (source->current_record) {
                        memcpy(source->current_record, &reverse,
                               sizeof(skIPFIXSourceRecord_t));
                        source->current_record
                            = ((skIPFIXSourceRecord_t*)
                               circBufNextHead(source->data_buffer));
                    }
                    if (source->current_record == NULL) {
                        assert(source->stopped);
                        continue;
                    }
                    pthread_mutex_lock(&source->stats_mutex);
                    ++source->forward_flows;
                    ++source->reverse_flows;
                    pthread_mutex_unlock(&source->stats_mutex);
                    continue;

                  default:
                    skAbortBadCase(rv);
                }
                break;
            } /* switch (rectype) */

            /* If we get here, stop reading from the current fbuf.
             * This may be because the fbuf is empty, because we are
             * shutting down, or due to an error. */
            break;

        } /* while (!base->destroyed) */

        /* Handle shutdown events */
        if (base->destroyed) {
            break;
        }

        /* Source has stopped, loop for the next source. */
        if (conn && source->stopped) {
            continue;
        }

        /* If we reach here, there is an error condition.  Any
         * non-error condition in the switch() should have called
         * continue.  The error could be something as simple an
         * end-of-message. */

        /* Handle FB_ERROR_NLREAD and FB_ERROR_EOM returned by
         * fBufNext() in the same way as when they are returned by
         * fbListenerWait().
         *
         * FB_ERROR_NLREAD is also returned when a previously rejected
         * UDP client attempts to send more data. */
        if (g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_NLREAD)
            || g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_EOM))
        {
            TRACEMSG(1, ("'%s': Ignoring %s: %s",
                         (conn ? source->name : "<UNKNOWN>"),
                         ((FB_ERROR_EOM == err->code)
                          ? "end-of-message" : "interrupted read"),
                         err->message));
            /* Do not free the fbuf here.  The fbuf is owned by the
             * listener, and will be freed when the listener is freed.
             * Calling fBufFree() here would cause fixbuf to forget
             * the current template, which would cause it to ignore
             * records until a new template is transmitted. */
            g_clear_error(&err);
            continue;
        }

        /* SK_IPFIX_ERROR_CONN indicates that a new UDP "connection"
         * was rejected by the appInit function in a multi-UDP
         * libfixbuf session.  Do not free the fbuf since we do not
         * have a connection yet; wait for another connection. */
        if (g_error_matches(err, SK_IPFIXSOURCE_DOMAIN, SK_IPFIX_ERROR_CONN)) {
            assert(IS_UDP);
            INFOMSG("Closing connection: %s", err->message);
            g_clear_error(&err);
            continue;
        }

        /* The remainder of the code in this while() block assumes
         * that 'source' is valid, which is only true if 'conn' is
         * non-NULL.  Trap that here, just in case. */
        if (NULL == conn) {
            if (NULL == err) {
                /* give up when error code is unknown */
                NOTICEMSG("'<UNKNOWN>': fixbuf listener shutting down:"
                          " unknown error from fBufNext");
                break;
            }
            DEBUGMSG("Ignoring packet: %s (d=%" PRIu32 ",c=%" PRId32 ")",
                     err->message, (uint32_t)err->domain, (int32_t)err->code);
            g_clear_error(&err);
            continue;
        }

        /* FB_ERROR_NETFLOWV9 indicates an anomalous netflow v9
         * record; these do not disturb fixbuf state, and so should be
         * ignored. */
        if (g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_NETFLOWV9)) {
            DEBUGMSG("'%s': Ignoring NetFlowV9 record: %s",
                     source->name, err->message);
            g_clear_error(&err);
            continue;
        }

#if SK_ENABLE_IPFIX_SFLOW
        /* FB_ERROR_SFLOW indicates an anomalous sFlow
         * record; these do not disturb fixbuf state, and so should be
         * ignored. */
        if (g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_SFLOW)) {
            DEBUGMSG("'%s': Ignoring sFlow record: %s",
                     source->name, err->message);
            g_clear_error(&err);
            continue;
        }
#endif

        /* FB_ERROR_TMPL indicates a set references a template ID for
         * which there is no template.  Log and continue. */
        if (g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_TMPL)) {
            DEBUGMSG("'%s': Ignoring data set: %s",
                     source->name, err->message);
            g_clear_error(&err);
            continue;
        }

        /* FB_ERROR_IPFIX indicates invalid IPFIX.  We could simply
         * choose to log and continue; instead we choose to log, close
         * the connection, and continue. */
        if (g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_IPFIX)) {
            if (IS_UDP) {
                DEBUGMSG("'%s': Ignoring invalid IPFIX: %s",
                         source->name, err->message);
            } else {
                INFOMSG("'%s': Closing connection; received invalid IPFIX: %s",
                        source->name, err->message);
                fBufFree(ipfix_buf);
                ipfix_buf = NULL;
            }
            g_clear_error(&err);
            continue;
        }

        /* FB_ERROR_EOF indicates that the connection associated with
         * this fBuf_t object has finished.  In this case, free the
         * fBuf_t object to close the connection.  Do not free the
         * fBuf_t for UDP connections, since these UDP-based fBuf_t
         * objects are freed with the listener. */
        if (g_error_matches(err, FB_ERROR_DOMAIN, FB_ERROR_EOF)) {
            if (!IS_UDP) {
                INFOMSG("'%s': Closing connection: %s",
                        source->name, err->message);
                fBufFree(ipfix_buf);
                ipfix_buf = NULL;
            }
            g_clear_error(&err);
            continue;
        }

        /* Handle an unexpected error generated by fixbuf */
        if (err && err->domain == FB_ERROR_DOMAIN) {
            if (IS_UDP) {
                DEBUGMSG(("'%s': Ignoring UDP packet: %s"
                          " (d=%" PRIu32 ",c=%" PRId32 ")"),
                         source->name, err->message,
                         (uint32_t)err->domain, (int32_t)err->code);
            } else {
                INFOMSG(("'%s': Closing connection: %s"
                         " (d=%" PRIu32 ",c=%" PRId32 ")"),
                        source->name, err->message,
                        (uint32_t)err->domain, (int32_t)err->code);
                fBufFree(ipfix_buf);
                ipfix_buf = NULL;
            }
            g_clear_error(&err);
            continue;
        }

        /* In the event of an unhandled error, end the thread. */
        if (NULL == err) {
            NOTICEMSG(("'%s': fixbuf listener shutting down:"
                       " unknown error from fBufNext"),
                      source->name);
        } else {
            NOTICEMSG(("'%s': fixbuf listener shutting down: %s"
                       " (d=%" PRIu32 ",c=%" PRId32 ")"),
                      source->name, err->message,
                      (uint32_t)err->domain, (int32_t)err->code);
            g_clear_error(&err);
        }
        break;
    }

    TRACEMSG(3, ("base %p exited while() loop", base));

    /* Free the fbuf if it exists.  (If it's UDP, it will be freed by
     * the destruction of the listener below.) */
    if (ipfix_buf && !IS_UDP) {
        TRACEMSG(3, ("base %p calling fBufFree", base));
        fBufFree(ipfix_buf);
    }

    /* Note that the thread is ending, and wait for
     * skIPFIXSourceDestroy() to mark this as destroyed */
    DEBUGMSG("fixbuf listener ending for [%s]:%s...",
             base->connspec->host ? base->connspec->host : "*",
             base->connspec->svc);
    pthread_mutex_lock(&base->mutex);
    while (!base->destroyed) {
        pthread_cond_wait(&base->cond, &base->mutex);
    }

    TRACEMSG(3, ("base %p is set to destroyed", base));

    /* Remove this base object from the listener_to_source_base
     * red-black tree */
    pthread_mutex_lock(&global_tree_mutex);
    rbdelete(base, listener_to_source_base);
    pthread_mutex_unlock(&global_tree_mutex);

    TRACEMSG(3, ("base %p calling fbListenerFree", base));

    /* Destroy the fbListener_t object.  This destroys the fbuf if the
     * stream is UDP. */
    fbListenerFree(base->listener);
    base->listener = NULL;

    /* Notify skIPFIXSourceDestroy() that the thread is ending */
    base->running = 0;
    pthread_cond_signal(&base->cond);
    DEBUGMSG("fixbuf listener ended for [%s]:%s.",
             base->connspec->host ? base->connspec->host : "*",
             base->connspec->svc);

    pthread_mutex_unlock(&base->mutex);

    TRACE_RETURN(NULL);
#undef IS_UDP
}


/*
 *     The free_connspec() function frees a fbConnSpec_t object.
 */
static void
free_connspec(
    fbConnSpec_t       *connspec)
{
    TRACE_ENTRY;

    if (connspec->host) {
        free(connspec->host);
    }
    if (connspec->svc) {
        free(connspec->svc);
    }
    free(connspec);

    TRACE_RETURN;
}


/*
 *     The ipfixSourceCreateBase() function allocates a new
 *     skIPFIXSourceBase_t object.
 */
static skIPFIXSourceBase_t *
ipfixSourceCreateBase(
    void)
{
    skIPFIXSourceBase_t *base;

    TRACE_ENTRY;

    base = (skIPFIXSourceBase_t*)calloc(1, sizeof(skIPFIXSourceBase_t));
    if (base == NULL) {
        TRACE_RETURN(NULL);
    }

    pthread_mutex_init(&base->mutex, NULL);
    pthread_cond_init(&base->cond, NULL);

    TRACE_RETURN(base);
}


/*
 *     The ipfixSourceCreateFromFile() function creates a new
 *     skIPFIXSource_t object and associated base object for a
 *     file-based IPFIX stream.
 */
static skIPFIXSource_t *
ipfixSourceCreateFromFile(
    const skpc_probe_t *probe,
    const char         *path_name)
{
    skIPFIXSourceBase_t *base   = NULL;
    skIPFIXSource_t     *source = NULL;
    GError              *err    = NULL;
    int                  rv;

    TRACE_ENTRY;

    /* Create the base object */
    base = ipfixSourceCreateBase();
    if (base == NULL) {
        goto ERROR;
    }

    /* Create the source object */
    source = (skIPFIXSource_t*)calloc(1, sizeof(*source));
    if (source == NULL) {
        goto ERROR;
    }

    /* Open the file */
    source->fileptr.of_name = path_name;
    rv = skFileptrOpen(&source->fileptr, SK_IO_READ);
    if (rv) {
        ERRMSG("Unable to open file '%s': %s",
               path_name, skFileptrStrerror(rv));
        goto ERROR;
    }
    if (SK_FILEPTR_IS_PROCESS == source->fileptr.of_type) {
        skAppPrintErr("Reading from gzipped files is not supported");
        goto ERROR;
    }

    /* Attach the source and base objects */
    source->base = base;
    base->any = source;
    ++base->source_count;

    /* Set the source's name from the probe name */
    source->probe = probe;
    source->name = skpcProbeGetName(probe);

    /* Create a file-based fBuf_t for the source */
    pthread_mutex_lock(&create_listener_mutex);
    source->readbuf = skiCreateReadBufferForFP(source->fileptr.of_fp, &err);
    pthread_mutex_unlock(&create_listener_mutex);
    if (source->readbuf == NULL) {
        if (err) {
            ERRMSG("%s: %s", "skiCreateReadBufferForFP", err->message);
        }
        goto ERROR;
    }
    pthread_mutex_lock(&global_tree_mutex);
    ++source_base_count;
    pthread_mutex_unlock(&global_tree_mutex);

    pthread_mutex_init(&source->stats_mutex, NULL);

    TRACE_RETURN(source);

  ERROR:
    g_clear_error(&err);
    if (source) {
        if (NULL != source->fileptr.of_fp) {
            skFileptrClose(&source->fileptr, &WARNINGMSG);
        }
        if (source->readbuf) {
            fBufFree(source->readbuf);
        }
        free(source);
    }
    if (base) {
        free(base);
    }
    TRACE_RETURN(NULL);
}


/*
 *    Add the 'source' object to the 'base' object (or for an
 *    alternate view, have the 'source' wrap the 'base').  Return 0 on
 *    success, or -1 on failure.
 */
static int
ipfixSourceBaseAddIPFIXSource(
    skIPFIXSourceBase_t *base,
    skIPFIXSource_t     *source)
{
    const sk_sockaddr_array_t **accept_from;
    peeraddr_source_t *peeraddr;
    const peeraddr_source_t *found;
    fbTransport_t transport;
    uint32_t accept_from_count;
    uint32_t i;
    uint32_t j;
    int rv = -1;

    TRACE_ENTRY;

    assert(base);
    assert(source);
    assert(source->probe);
    assert(NULL == source->base);

    accept_from_count = skpcProbeGetAcceptFromHost(source->probe,&accept_from);

    /* Lock the base */
    pthread_mutex_lock(&base->mutex);

    /* Base must not be configured to accept packets from any host. */
    if (base->any) {
        goto END;
    }
    if (NULL == accept_from || 0 == accept_from_count) {
        /* When no accept-from-host is specified, this source accepts
         * packets from any address and there should be a one-to-one
         * mapping between source and base */
        if (base->addr_to_source) {
            /* The base already references another source. */
            goto END;
        }
        base->any = source;
        source->base = base;
        ++base->source_count;
    } else {
        /* Make sure the sources's protocol match the base's protocol */
        SILK_PROTO_TO_FIXBUF_TRANSPORT(
            skpcProbeGetProtocol(source->probe), &transport);
        if (base->connspec->transport != transport) {
            goto END;
        }

        /* Connect the base to the source */
        source->base = base;

        if (NULL == base->addr_to_source) {
            base->addr_to_source = rbinit(peeraddr_compare, NULL);
            if (base->addr_to_source == NULL) {
                goto END;
            }
        }

        /* Add a mapping on the base for each accept-from-host address
         * on this source. */
        for (j = 0; j < accept_from_count; ++j) {
            for (i = 0; i < skSockaddrArraySize(accept_from[j]); ++i) {
                peeraddr = ((peeraddr_source_t*)
                            calloc(1, sizeof(peeraddr_source_t)));
                if (peeraddr == NULL) {
                    goto END;
                }
                peeraddr->source = source;
                peeraddr->addr = skSockaddrArrayGet(accept_from[j], i);
                found = ((const peeraddr_source_t*)
                         rbsearch(peeraddr, base->addr_to_source));
                if (found != peeraddr) {
                    if (found && (found->source == peeraddr->source)) {
                        /* Duplicate address, same connection */
                        free(peeraddr);
                        continue;
                    }
                    /* Memory error adding to tree */
                    free(peeraddr);
                    goto END;
                }
            }
        }

        ++base->source_count;
    }

    rv = 0;

  END:
    pthread_mutex_unlock(&base->mutex);
    TRACE_RETURN(rv);
}


/*
 *    Creates a IPFIX source listening on the network.
 *
 *    'probe' is the probe associated with the source.  'max_flows' is
 *    the number of IPFIX flows the created source can buffer in
 *    memory.
 *
 *    Returns a IPFIX source on success, or NULL on failure.
 */
static skIPFIXSource_t *
ipfixSourceCreateFromSockaddr(
    const skpc_probe_t *probe,
    uint32_t            max_flows)
{
    skIPFIXSource_t *source = NULL;
    skIPFIXSourceBase_t *localbase = NULL;
    skIPFIXSourceBase_t *base;
    const sk_sockaddr_array_t *listen_address;
    const sk_sockaddr_array_t **accept_from;
    peeraddr_source_t target;
    const peeraddr_source_t *found;
    skpc_proto_t protocol;
    GError *err = NULL;
    char port_string[7];
    uint32_t accept_from_count;
    uint32_t i;
    uint32_t j;
    int rv;

    TRACE_ENTRY;

    /* Check the protocol */
    protocol = skpcProbeGetProtocol(probe);

    /* Get the list of accept-from-host addresses. */
    accept_from_count = skpcProbeGetAcceptFromHost(probe, &accept_from);

    /* Get the listen address. */
    rv = skpcProbeGetListenOnSockaddr(probe, &listen_address);
    if (rv == -1) {
        goto ERROR;
    }

    /* Check to see if there is an existing base object for that
     * listen address */
    pthread_mutex_lock(&global_tree_mutex);
    if (!listener_to_source_base) {
        base = NULL;
    } else {
        /* Loop through all current bases, and compare based on
         * listen_address */
        RBLIST *iter = rbopenlist(listener_to_source_base);
        while ((base = (skIPFIXSourceBase_t *)rbreadlist(iter)) != NULL) {
            if (skSockaddrArrayMatches(base->listen_address,
                                       listen_address, 0))
            {
                /* Found a match.  'base' is now set to the matching
                 * base */
                break;
            }
        }
        rbcloselist(iter);
    }
    pthread_mutex_unlock(&global_tree_mutex);

    if (base) {
        if (accept_from == NULL) {
            /* The new listener wants to be promiscuous but another
             * listener already exists. */
            goto ERROR;
        }

        pthread_mutex_lock(&base->mutex);
        if (base->any) {
            /* Already have a listener, and it is promiscuous. */
            pthread_mutex_unlock(&base->mutex);
            goto ERROR;
        }

        /* Ensure the accept-from addresses are unique. */
        for (j = 0; j < accept_from_count; ++j) {
            for (i = 0; i < skSockaddrArraySize(accept_from[j]); ++i) {
                target.addr = skSockaddrArrayGet(accept_from[j], i);
                found = ((const peeraddr_source_t*)
                         rbfind(&target, base->addr_to_source));
                if (found != NULL) {
                    pthread_mutex_unlock(&base->mutex);
                    goto ERROR;
                }
            }
        }
        pthread_mutex_unlock(&base->mutex);
    }

    /* Create a new source object */
    source = (skIPFIXSource_t *)calloc(1, sizeof(*source));
    if (source == NULL) {
        goto ERROR;
    }

    /* Keep a handle the probe and the probe's name */
    source->probe = probe;
    source->name = skpcProbeGetName(probe);

    if (PROBE_ENUM_NETFLOW_V9 == skpcProbeGetType(probe)
        || PROBE_ENUM_SFLOW == skpcProbeGetType(probe))
    {
        /* Create the look-up table for skIPFIXConnection_t's */
        source->connections = rbinit(pointer_cmp, NULL);
        if (NULL == source->connections) {
            goto ERROR;
        }
    }

    /* Create the circular buffer */
    source->data_buffer
        = circBufCreate(sizeof(skIPFIXSourceRecord_t), max_flows);
    if (source->data_buffer == NULL) {
        goto ERROR;
    }
    /* Ready the first location in the circular buffer for writing */
    source->current_record = ((skIPFIXSourceRecord_t*)
                              circBufNextHead(source->data_buffer));
    assert(source->current_record);

    pthread_mutex_init(&source->stats_mutex, NULL);

    if (base != NULL) {
        /* If there is an existing base, add the source to it. */
        if (ipfixSourceBaseAddIPFIXSource(base, source)) {
            goto ERROR;
        }
    } else {
        /* No existing base, create a new one */

        /* Create the base object */
        base = localbase = ipfixSourceCreateBase();
        if (base == NULL) {
            goto ERROR;
        }

        /* Set the listen_address */
        base->listen_address = listen_address;

        /* Create a connspec in order to create a listener */
        base->connspec = (fbConnSpec_t *)calloc(1, sizeof(*base->connspec));
        if (base->connspec == NULL) {
            goto ERROR;
        }
        if (skSockaddrArrayName(listen_address)) {
            base->connspec->host = strdup(skSockaddrArrayName(listen_address));
            if (base->connspec->host == NULL) {
                goto ERROR;
            }
        }
        rv = snprintf(port_string, sizeof(port_string), "%i",
                      skSockaddrPort(skSockaddrArrayGet(listen_address, 0)));
        assert((size_t)rv < sizeof(port_string));
        base->connspec->svc = strdup(port_string);
        if (base->connspec->svc == NULL) {
            goto ERROR;
        }
        SILK_PROTO_TO_FIXBUF_TRANSPORT(protocol, &base->connspec->transport);

        /* Create the listener */
        pthread_mutex_lock(&create_listener_mutex);
        if (protocol != SKPC_PROTO_UDP) {
            /* In the TCP case, the listener does not create a
             * collector immediately, and as such does not need to
             * stash the source object before allocating the listener.
             * Instead, the fixbufConnect() call will find the base
             * object and source in the listener_to_source_base tree
             * when a new connection necessitates the creation of a
             * new collector. */
            assert(protocol == SKPC_PROTO_TCP);
            base->listener = skiCreateListener(base->connspec,
                                               fixbufConnect,
                                               fixbufDisconnect, &err);
            if (base->listener == NULL) {
                pthread_mutex_unlock(&create_listener_mutex);
                goto ERROR;
            }
        } else {
            fbCollector_t *collector;
            const char *env;
            int consider_sport = 1;

            /* Create the listener */
            base->listener = skiCreateListener(
                base->connspec, fixbufConnect, fixbufDisconnect, &err);
            if (base->listener == NULL) {
                pthread_mutex_unlock(&create_listener_mutex);
                goto ERROR;
            }
            if (!fbListenerGetCollector(base->listener, &collector, &err)) {
                pthread_mutex_unlock(&create_listener_mutex);
                goto ERROR;
            }
            /* Enable the multi-UDP support in libfixbuf. */
            fbCollectorSetUDPMultiSession(collector, 1);

            /* Treat UDP streams from the same address but different
             * ports as different streams unless
             * SK_IPFIX_UDP_IGNORE_SOURCE_PORT is set to non-zero. */
            env = getenv(SK_IPFIX_UDP_IGNORE_SOURCE_PORT);
            if (NULL != env && *env && *env != '0') {
                consider_sport = 0;
            }
            fbCollectorManageUDPStreamByPort(collector, consider_sport);

            /* If this is a Netflow v9 source or an sFlow source, tell
             * the collector. */
            switch (skpcProbeGetType(source->probe)) {
              case PROBE_ENUM_IPFIX:
                break;
              case PROBE_ENUM_NETFLOW_V9:
                if (!fbCollectorSetNetflowV9Translator(collector, &err)) {
                    pthread_mutex_unlock(&create_listener_mutex);
                    goto ERROR;
                }
                break;
#if SK_ENABLE_IPFIX_SFLOW
              case PROBE_ENUM_SFLOW:
                if (!fbCollectorSetSFlowTranslator(collector, &err)) {
                    pthread_mutex_unlock(&create_listener_mutex);
                    goto ERROR;
                }
                break;
#endif
              default:
                skAbortBadCase(skpcProbeGetType(source->probe));
            }
        }

        pthread_mutex_unlock(&create_listener_mutex);

        pthread_mutex_init(&base->mutex, NULL);
        pthread_cond_init(&base->cond, NULL);

        /* add the source to the base */
        if (ipfixSourceBaseAddIPFIXSource(base, source)) {
            goto ERROR;
        }

        /* Add base to list of bases */
        pthread_mutex_lock(&global_tree_mutex);
        if (listener_to_source_base == NULL) {
            listener_to_source_base = rbinit(listener_to_source_base_find,
                                             NULL);
            if (listener_to_source_base == NULL) {
                pthread_mutex_unlock(&global_tree_mutex);
                goto ERROR;
            }
        }
        base = ((skIPFIXSourceBase_t *)
                rbsearch(localbase, listener_to_source_base));
        if (base != localbase) {
            pthread_mutex_unlock(&global_tree_mutex);
            goto ERROR;
        }
        ++source_base_count;
        pthread_mutex_unlock(&global_tree_mutex);

        /* Start the listener thread */
        pthread_mutex_lock(&base->mutex);
        rv = skthread_create(skSockaddrArrayNameSafe(listen_address),
                             &base->thread, ipfix_reader, (void*)base);
        if (rv != 0) {
            pthread_mutex_unlock(&base->mutex);
            WARNINGMSG("Unable to spawn new thread for '%s': %s",
                       skSockaddrArrayNameSafe(listen_address), strerror(rv));
            goto ERROR;
        }

        /* Wait for the thread to really begin */
        do {
            pthread_cond_wait(&base->cond, &base->mutex);
        } while (!base->started);
        pthread_mutex_unlock(&base->mutex);
    }

    TRACE_RETURN(source);

  ERROR:
    if (err) {
        ERRMSG("'%s': %s", source->name, err->message);
    }
    g_clear_error(&err);
    if (localbase) {
        if (localbase->listener) {
            fbListenerFree(localbase->listener);
        }
        if (localbase->connspec) {
            free_connspec(localbase->connspec);
        }
        if (localbase->addr_to_source) {
            rbdestroy(localbase->addr_to_source);
        }
        free(localbase);
    }
    if (source) {
        if (source->data_buffer) {
            circBufDestroy(source->data_buffer);
        }
        if (source->connections) {
            rbdestroy(source->connections);
        }
        free(source);
    }
    TRACE_RETURN(NULL);
}


/*
 *    Handler to print log messages.  This will be invoked by g_log()
 *    and the other logging functions from GLib2.
 */
static void
ipfixGLogHandler(
    const gchar     UNUSED(*log_domain),
    GLogLevelFlags          log_level,
    const gchar            *message,
    gpointer         UNUSED(user_data))
{
    /* In syslog, CRIT is worse than ERR; in Glib2 ERROR is worse than
     * CRITICAL. */

    switch (log_level & G_LOG_LEVEL_MASK) {
      case G_LOG_LEVEL_CRITICAL:
        ERRMSG("%s", message);
        break;
      case G_LOG_LEVEL_WARNING:
        WARNINGMSG("%s", message);
        break;
      case G_LOG_LEVEL_MESSAGE:
        NOTICEMSG("%s", message);
        break;
      case G_LOG_LEVEL_INFO:
        INFOMSG("%s", message);
        break;
      case G_LOG_LEVEL_DEBUG:
        DEBUGMSG("%s", message);
        break;
      default:
        CRITMSG("%s", message);
        break;
    }
}

/*
 *    GLib Log handler to discard messages.
 */
static void
ipfixGLogHandlerVoid(
    const gchar     UNUSED(*log_domain),
    GLogLevelFlags   UNUSED(log_level),
    const gchar     UNUSED(*message),
    gpointer         UNUSED(user_data))
{
    return;
}


/*
 *  ipfixSourceGlibInitialize();
 *
 *    Initialize the GLib slice allocator.  Since there is no way to
 *    de-initialize the slice allocator, valgrind will report this
 *    memory as "still-reachable".  We would rather have this
 *    "still-reachable" memory reported in a well-known location,
 *    instead of hidden somewhere within fixbuf.
 */
static void
ipfixSourceGlibInitialize(
    void)
{
#if (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 10)
#define MEMORY_SIZE 128
    gpointer memory;

    memory = g_slice_alloc(MEMORY_SIZE);
    g_slice_free1(MEMORY_SIZE, memory);
#endif
}


/*
 *    Performs any initialization required prior to creating the IPFIX
 *    sources.  Returns 0 on success, or -1 on failure.
 */
int
skIPFIXSourcesSetup(
    void)
{
    const char *env;
    GLogLevelFlags log_levels = (GLogLevelFlags)(G_LOG_LEVEL_CRITICAL
                                                 | G_LOG_LEVEL_WARNING
                                                 | G_LOG_LEVEL_MESSAGE
                                                 | G_LOG_LEVEL_INFO
                                                 | G_LOG_LEVEL_DEBUG);

    /* initialize the slice allocator */
    ipfixSourceGlibInitialize();

    /* As of glib 2.32, g_thread_init() is deprecated. */
#if (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 32)
    /* tell fixbuf (glib) we are a threaded program.  this will abort
     * if glib does not have thread support. */
    if (!g_thread_supported()) {
        g_thread_init(NULL);
    }
#endif

    /* set a log handler for messages from glib, which we always want
     * to include in our log file.
     * http://developer.gnome.org/glib/stable/glib-Message-Logging.html */
    g_log_set_handler("GLib", log_levels, &ipfixGLogHandler, NULL);

    /* set a log handler for messages from fixbuf, maybe using a void
     * handler for warnings. */
    env = getenv(SK_ENV_FIXBUF_SUPPRESS_WARNING);
    if (env && *env && 0 == strcmp("1", env)) {
        /* suppress warnings by setting a void handler */
        log_levels = (GLogLevelFlags)((unsigned int)log_levels
                                      & ~(unsigned int)G_LOG_LEVEL_WARNING);
        g_log_set_handler(
            NULL, G_LOG_LEVEL_WARNING, &ipfixGLogHandlerVoid, NULL);
    }
    g_log_set_handler(NULL, log_levels, &ipfixGLogHandler, NULL);

    return 0;
}


/*
 *    Creates a IPFIX source based on an skpc_probe_t.
 *
 *    If the source is a network-based probe, this function also
 *    starts the collection process.
 *
 *    When creating a source from a network-based probe, the 'params'
 *    union should have the 'max_pkts' member specify the maximum
 *    number of packets to buffer in memory for this source.
 *
 *    When creating a source from a probe that specifies either a file
 *    or a directory that is polled for files, the 'params' union must
 *    have the 'path_name' specify the full path of the file to
 *    process.
 *
 *    Return the new source, or NULL on error.
 */
skIPFIXSource_t *
skIPFIXSourceCreate(
    const skpc_probe_t         *probe,
    const skFlowSourceParams_t *params)
{
    skIPFIXSource_t *source;

    TRACE_ENTRY;

    /* Check whether this is a file-based probe---either handles a
     * single file or files pulled from a directory poll */
    if (NULL != skpcProbeGetPollDirectory(probe)
        || NULL != skpcProbeGetFileSource(probe))
    {
        if (NULL == params->path_name) {
            TRACE_RETURN(NULL);
        }
        source = ipfixSourceCreateFromFile(probe, params->path_name);

    } else {
        /* must be a network-based source */
        source = ipfixSourceCreateFromSockaddr(probe, params->max_pkts);
    }

    TRACE_RETURN(source);
}


/*
 *    Stops processing of packets.  This will cause a call to any
 *    skIPFIXSourceGetGeneric() function to stop blocking.  Meant to
 *    be used as a prelude to skIPFIXSourceDestroy() in threaded code.
 */
void
skIPFIXSourceStop(
    skIPFIXSource_t    *source)
{
    TRACE_ENTRY;

    assert(source);

    /* Mark the source as stopped, and unblock the circular buffer */
    source->stopped = 1;
    if (source->data_buffer) {
        circBufStop(source->data_buffer);
    }
    TRACE_RETURN;
}


/*
 *    Destroys a IPFIX source.
 */
void
skIPFIXSourceDestroy(
    skIPFIXSource_t    *source)
{
    skIPFIXSourceBase_t *base;
    const sk_sockaddr_array_t **accept_from;
    peeraddr_source_t target;
    const peeraddr_source_t *found;
    uint32_t accept_from_count;
    uint32_t i;
    uint32_t j;

    TRACE_ENTRY;

    if (!source) {
        TRACE_RETURN;
    }

    accept_from_count = skpcProbeGetAcceptFromHost(source->probe,&accept_from);

    assert(source->base);

    base = source->base;

    pthread_mutex_lock(&base->mutex);

    /* Remove the source from the red-black tree */
    if (base->addr_to_source && accept_from) {
        /* Remove the source's accept-from-host addresses from
         * base->addr_to_source */
        for (j = 0; j < accept_from_count; ++j) {
            for (i = 0; i < skSockaddrArraySize(accept_from[j]); ++i) {
                target.addr = skSockaddrArrayGet(accept_from[j], i);
                found = ((const peeraddr_source_t*)
                         rbdelete(&target, base->addr_to_source));
                if (found && (found->source == source)) {
                    free((void*)found);
                }
            }
        }
    }

    /* Stop the source */
    skIPFIXSourceStop(source);

    /* If the source is not currently being referenced by an fBuf_t,
     * free it, otherwise mark it to be destroyed when the fBuf_t is
     * freed by fixbufDisconnect(). */
    if (source->connection_count == 0) {
        free_source(source);
    } else {
        source->destroy = 1;
    }

    /* Decrement the source reference count */
    assert(base->source_count);
    --base->source_count;

    TRACEMSG(3, ("base %p source_count is %u", base, base->source_count));

    /* If this base object is still referenced by sources, return */
    if (base->source_count != 0) {
        pthread_mutex_unlock(&base->mutex);
        TRACE_RETURN;
    }

    /* Otherwise, we must destroy the base stop its thread */
    base->destroyed = 1;

    if (base->listener) {
        TRACEMSG(3, ("base %p calling fbListenerInterrupt", base));

        /* Unblock the fbListenerWait() call */
        fbListenerInterrupt(base->listener);

        /* Signal that the thread is to exit */
        pthread_cond_broadcast(&base->cond);

        TRACEMSG(3, ("base %p waiting for running variable", base));

        /* Wait for the thread to exit */
        while (base->running) {
            pthread_cond_wait(&base->cond, &base->mutex);
        }

        TRACEMSG(3, ("base %p joining its thread", base));

        /* Acknowledge that the thread has exited */
        pthread_join(base->thread, NULL);

        assert(base->listener == NULL);

        /* Free the connspec */
        free_connspec(base->connspec);

        /* Destroy the red-black tree */
        if (base->addr_to_source) {
            rbdestroy(base->addr_to_source);
        }

        pthread_cond_destroy(&base->cond);

        pthread_mutex_unlock(&base->mutex);
        pthread_mutex_destroy(&base->mutex);
    }

    TRACEMSG(3, ("base %p is free", base));

    free(base);

    pthread_mutex_lock(&global_tree_mutex);
    --source_base_count;
    if (0 == source_base_count) {
        /* When the last base is removed, destroy the global base
         * list, and call the teardown function for the libskipfix
         * library to free any global objects allocated there. */
        if (listener_to_source_base) {
            rbdestroy(listener_to_source_base);
            listener_to_source_base = NULL;
        }
        skiTeardown();
    }
    pthread_mutex_unlock(&global_tree_mutex);
    TRACE_RETURN;
}


/*
 *    Requests a record from the file-based IPFIX source 'source'.
 *
 *    Returns 0 on success, -1 on failure.
 */
static int
ipfixSourceGetRecordFromFile(
    skIPFIXSource_t        *source,
    skIPFIXSourceRecord_t  *ipfix_rec)
{
    ski_yaf_stats_t stats;
    GError *err = NULL;
    int rv;

    TRACE_ENTRY;

    /* Reading from a file */
    pthread_mutex_lock(&source->base->mutex);
    assert(source->readbuf);

    if (source->reverse) {
        /* A reverse record exists from the previous flow */
        memcpy(ipfix_rec, &source->rvbuf, sizeof(skIPFIXSourceRecord_t));
        ++source->reverse_flows;
        source->reverse = 0;
    } else {
        /* Initialize the control variable for the do{}while() loop.
         * 0: ignore; 1: uniflow; 2: biflow; -1: error */
        rv = 0;
        do {
            /* Similar to the switch() block in ipfix_reader() above */
            switch (skiGetNextRecordType(source->readbuf, &err)) {
              case SKI_RECTYPE_ERROR:
                rv = -1;
                break;          /* error */

#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
              case SKI_RECTYPE_NF9_SAMPLING:
#endif
              case SKI_RECTYPE_UNKNOWN:
                if (!skiYafNextStats(
                        source->readbuf, source->probe, &stats, &err))
                {
                    /* should have been able to read something */
                    TRACEMSG(2, (("'%s': SKI_RECTYPE_UNKNOWN and"
                                  " NextStats() is FALSE"), source->name));
                    rv = -1;
                    break;      /* error */
                }
                continue;

              case SKI_RECTYPE_STATS:
                if (!skiYafNextStats(
                        source->readbuf, source->probe, &stats, &err))
                {
                    /* should have been able to read the stats */
                    TRACEMSG(2, (("'%s': SKI_RECTYPE_STATS and"
                                  " NextStats is FALSE"), source->name));
                    rv = -1;
                    break;      /* error */
                }
                TRACEMSG_YAF_STATS(source, stats);
                INCORPORATE_STAT_RECORD(source, source->last_yaf_stats, stats);
                continue;

              case SKI_RECTYPE_FLOW:
                rv = skiRwNextRecord(source->readbuf, source->probe,
                                     ipfix_rec, &source->rvbuf, &err);
                if (rv == 0) {
                    ++source->ignored_flows;
                }
                break;
            }
        } while (rv == 0);  /* Continue while current record is ignored */

        if (rv == -1) {
            /* End of file or other problem */
            g_clear_error(&err);
            pthread_mutex_unlock(&source->base->mutex);
            TRACE_RETURN(-1);
        }

        assert(rv == 1 || rv == 2);
        ++source->forward_flows;

        /* We have the next flow.  Set reverse if there is a
         * reverse record.  */
        source->reverse = (rv == 2);
    }

    pthread_mutex_unlock(&source->base->mutex);

    TRACE_RETURN(0);
}


/*
 *    Requests a SiLK Flow record from the IPFIX source 'source'.
 *
 *    This function will block if there are no IPFIX flows available
 *    from which to create a SiLK Flow record.
 *
 *    Returns 0 on success, -1 on failure.
 */
int
skIPFIXSourceGetGeneric(
    skIPFIXSource_t    *source,
    rwRec              *rwrec)
{
    skIPFIXSourceRecord_t *rec;
    skIPFIXSourceRecord_t ipfix_rec;
    int rv;

    TRACE_ENTRY;

    assert(source);
    assert(rwrec);

    if (source->data_buffer) {
        /* Reading from the circular buffer */
        rec = (skIPFIXSourceRecord_t*)circBufNextTail(source->data_buffer);
        if (rec == NULL) {
            TRACE_RETURN(-1);
        }
        RWREC_COPY(rwrec, skIPFIXSourceRecordGetRwrec(rec));
        TRACE_RETURN(0);
    }

    rv = ipfixSourceGetRecordFromFile(source, &ipfix_rec);
    if (0 == rv) {
        RWREC_COPY(rwrec, skIPFIXSourceRecordGetRwrec(&ipfix_rec));
    }
    TRACE_RETURN(rv);
}


/*
 *    Requests a record from the IPFIX source 'source'.
 *
 *    This function will block if there are no IPFIX flows available
 *    from which to create a record.
 *
 *    Returns 0 if SiLK Flow record only, -1 on failure.
 */
int
skIPFIXSourceGetRecord(
    skIPFIXSource_t        *source,
    skIPFIXSourceRecord_t  *ipfix_rec)
{
    skIPFIXSourceRecord_t *rec;
    int rv;

    TRACE_ENTRY;

    assert(source);
    assert(ipfix_rec);

    if (source->data_buffer) {
        /* Reading from the circular buffer */
        rec = (skIPFIXSourceRecord_t*)circBufNextTail(source->data_buffer);
        if (rec == NULL) {
            TRACE_RETURN(-1);
        }
        memcpy(ipfix_rec, rec, sizeof(skIPFIXSourceRecord_t));
    } else {
        rv = ipfixSourceGetRecordFromFile(source, ipfix_rec);
        if (-1 == rv) {
            TRACE_RETURN(rv);
        }
    }
    TRACE_RETURN(0);
}


/* Constants used to create source_do_stats()'s flags argument */
#define SOURCE_DO_STATS_LOG     0x01
#define SOURCE_DO_STATS_CLEAR   0x02

/*
 *  source_do_stats(source, flags);
 *
 *    Log and/or clear the statistics for the 'source'.  'flags' is a
 *    combination of SOURCE_DO_STATS_LOG and SOURCE_DO_STATS_CLEAR.
 */
static void
source_do_stats(
    skIPFIXSource_t    *source,
    unsigned int        flags)
{
    TRACE_ENTRY;

    pthread_mutex_lock(&source->stats_mutex);

    /* print log message giving the current statistics on the
     * skIPFIXSource_t pointer 'source' */
    if (flags & SOURCE_DO_STATS_LOG) {
        fbCollector_t *collector = NULL;
        GError *err = NULL;

        if (source->saw_yaf_stats_pkt) {
            /* IPFIX from yaf: print the stats */

            INFOMSG(("'%s': forward %" PRIu64
                     ", reverse %" PRIu64
                     ", ignored %" PRIu64
                     "; yaf: recs %" PRIu64
                     ", pkts %" PRIu64
                     ", dropped-pkts %" PRIu64
                     ", ignored-pkts %" PRIu64
                     ", bad-sequence-pkts %" PRIu64
                     ", expired-frags %" PRIu64),
                    source->name,
                    source->forward_flows,
                    source->reverse_flows,
                    source->ignored_flows,
                    source->yaf_exported_flows,
                    source->yaf_processed_packets,
                    source->yaf_dropped_packets,
                    source->yaf_ignored_packets,
                    source->yaf_notsent_packets,
                    source->yaf_expired_fragments);

        } else if (!source->connections
                   || !source->base
                   || !source->base->listener)
        {
            /* no data or other IPFIX; print count of SiLK flows
             * created */

            INFOMSG(("'%s': forward %" PRIu64
                     ", reverse %" PRIu64
                     ", ignored %" PRIu64),
                    source->name,
                    source->forward_flows,
                    source->reverse_flows,
                    source->ignored_flows);

        } else if (!fbListenerGetCollector(source->base->listener,
                                           &collector, &err))
        {
            /* sFlow or NetFlowV9, but no collector */

            DEBUGMSG("'%s': Unable to get collector for source: %s",
                     source->name, err->message);
            g_clear_error(&err);

            INFOMSG(("'%s': forward %" PRIu64
                     ", reverse %" PRIu64
                     ", ignored %" PRIu64),
                    source->name,
                    source->forward_flows,
                    source->reverse_flows,
                    source->ignored_flows);

        } else {
            /* sFlow or NetFlowV9 */
            skIPFIXConnection_t *conn;
            RBLIST *iter;
            uint64_t prev;

            iter = rbopenlist(source->connections);
            while ((conn = (skIPFIXConnection_t *)rbreadlist(iter)) != NULL) {
                /* store the previous number of dropped NF9/sFlow packets
                 * and get the new number of dropped packets. */
                prev = conn->last_yaf_stats.droppedPacketTotalCount;
#if SK_ENABLE_IPFIX_SFLOW
                if (skpcProbeGetType(source->probe) == PROBE_ENUM_SFLOW) {
                    conn->last_yaf_stats.droppedPacketTotalCount
                        = fbCollectorGetSFlowMissed(
                            collector, &conn->peer_addr.sa, conn->peer_len,
                            conn->ob_domain);
                } else
#endif  /* SK_ENABLE_IPFIX_SFLOW */
                {
                    conn->last_yaf_stats.droppedPacketTotalCount
                        = fbCollectorGetNetflowMissed(
                            collector, &conn->peer_addr.sa, conn->peer_len,
                            conn->ob_domain);
                }
                if (prev > conn->last_yaf_stats.droppedPacketTotalCount) {
                    /* assume a new collector */
                    TRACEMSG(4, (("Assuming new collector: NF9 loss dropped"
                                  " old = %" PRIu64 ", new = %" PRIu64),
                                 prev,
                                 conn->last_yaf_stats.droppedPacketTotalCount));
                    prev = 0;
                }
                source->yaf_dropped_packets
                    += conn->last_yaf_stats.droppedPacketTotalCount - prev;
            }
            rbcloselist(iter);

            INFOMSG(("'%s': forward %" PRIu64
                     ", reverse %" PRIu64
                     ", ignored %" PRIu64
                     ", %s: missing-pkts %" PRIu64),
                    source->name,
                    source->forward_flows,
                    source->reverse_flows,
                    source->ignored_flows,
                    ((skpcProbeGetType(source->probe) == PROBE_ENUM_SFLOW)
                     ? "sflow" : "nf9"),
                    source->yaf_dropped_packets);
        }
    }

    /* reset (set to zero) statistics on the skIPFIXSource_t
     * 'source' */
    if (flags & SOURCE_DO_STATS_CLEAR) {
        source->yaf_dropped_packets = 0;
        source->yaf_ignored_packets = 0;
        source->yaf_notsent_packets = 0;
        source->yaf_expired_fragments = 0;
        source->yaf_processed_packets = 0;
        source->yaf_exported_flows = 0;
        source->forward_flows = 0;
        source->reverse_flows = 0;
        source->ignored_flows = 0;
    }

    pthread_mutex_unlock(&source->stats_mutex);
    TRACE_RETURN;
}


/* Log statistics associated with a IPFIX source. */
void
skIPFIXSourceLogStats(
    skIPFIXSource_t    *source)
{
    source_do_stats(source, SOURCE_DO_STATS_LOG);
}

/* Log statistics associated with a IPFIX source, and then clear the
 * statistics. */
void
skIPFIXSourceLogStatsAndClear(
    skIPFIXSource_t    *source)
{
    source_do_stats(source, SOURCE_DO_STATS_LOG | SOURCE_DO_STATS_CLEAR);
}

/* Clear out current statistics */
void
skIPFIXSourceClearStats(
    skIPFIXSource_t    *source)
{
    source_do_stats(source, SOURCE_DO_STATS_CLEAR);
}

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
