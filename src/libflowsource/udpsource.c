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
**  Functions to create and read from a UDP socket.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: udpsource.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#if SK_ENABLE_ZLIB
#include <zlib.h>
#endif
#include <poll.h>
#include <silk/libflowsource.h>
#include <silk/redblack.h>
#include <silk/skdllist.h>
#include <silk/sklog.h>
#include <silk/skthread.h>
#include "udpsource.h"
#include "circbuf.h"


/* LOCAL DEFINES AND TYPEDEFS */

/* Timeout to pass to the poll(2) system class, in milliseconds. */
#define POLL_TIMEOUT 500

/* Whether to compile in code to help debug accept-from-host */
#ifndef DEBUG_ACCEPT_FROM
#define DEBUG_ACCEPT_FROM 0
#endif


/* forward declarations */
struct skUDPSourceBase_st;
typedef struct skUDPSourceBase_st skUDPSourceBase_t;
struct peeraddr_source_st;
typedef struct peeraddr_source_st peeraddr_source_t;


/*
 *    There is one skUDPSource_t for every skpc_probe_t that accepts
 *    data on a UDP port.  The skUDPSource_t contains data collected
 *    for that particular probe.
 *
 *    For each source/probe, the pair (listen_address, from_address)
 *    must be unique.  That is, either the source is only thing
 *    listening on this address/port, or the sources are distinguished
 *    by the address the packets are coming from (i.e., the peer
 *    address).
 *
 *    Under the 'skUDPSource_t' there is a 'skUDPSourceBase_t' object,
 *    which handles the collection of data from the network.  When
 *    multiple 'skUDPSource_t's listen on the same address, they share
 *    the same 'skUDPSourceBase_t'.  The 'skUDPSourceBase_t' has an
 *    red-black tree (the 'addr_to_source' member) that maps back to
 *    all the 'skUDPSource_t objects' that share that base.  The key
 *    in the red-black tree is the 'from-address'.
 */
struct skUDPSource_st {
    /* callback function invoked for each received packet to determine
     * whether the packet should be rejected, and the data to pass as
     * a parameter to that function. */
    udp_source_reject_fn        reject_pkt_fn;
    void                       *fn_callback_data;

    /* network collector */
    skUDPSourceBase_t          *base;

    /* set of addresses from which this source accepts data */
    const sk_sockaddr_array_t  *from_addresses;

    /* 'data_buffer' holds packets collected for this probe but not
     * yet requested.  'pkt_buffer' is the current location in the
     * 'data_buffer' */
    circBuf_t                  *data_buffer;
    void                       *pkt_buffer;

    unsigned                    stopped : 1;
};


/* typedef struct skUDPSourceBase_st skUDPSourceBase_t; */
struct skUDPSourceBase_st {
    const char             *name;
    uint16_t                port;

    /* when a probe does not have an accept-from-host clause, any peer
     * may connect, and there is a one-to-one mapping between a source
     * object and a base object.  The 'any' member points to the
     * source, and the 'addr_to_source' member will be NULL. */
    skUDPSource_t          *any;

    /* if there is an accept-from clause, the 'addr_to_source'
     * red-black tree maps the address of the peer to a particular
     * source object (via 'peeraddr_source_t' objects), and the 'any'
     * member must be NULL. */
    struct rbtree          *addr_to_source;

#if 0
    /* the 'connections' red-black tree maps the peering host to a
     * particular source when multiple sources are listening on the
     * same port.  if 'connections' is NULL, then 'any' must not be
     * NULL, and any peer may connect. */
    struct rbtree          *connections;
    peeraddr_source_t      *any;
#endif

    /* addresses to bind() to */
    const sk_sockaddr_array_t *listen_address;

    /* Sockets to listen to */
    struct pollfd          *pfd;
    nfds_t                  pfd_len;   /* Size of array */
    nfds_t                  pfd_valid; /* Number of valid entries in array */

    /* Used with file-based sources */
    uint8_t                *file_buffer;
#if SK_ENABLE_ZLIB
    gzFile                  udpfile;
#else
    FILE                   *udpfile;
#endif

    /* Thread data */
    pthread_t               thread;
    pthread_mutex_t         mutex;
    pthread_cond_t          cond;

    /* 'data_size' is the maximum size of an individual packet.
     * 'bufsize' is the number of packets to keep in memory. */
    size_t                  data_size;
    uint32_t                bufsize;

    /* number of 'sources' that use this 'base' */
    uint32_t                refcount;
    /* number of 'sources' that are running */
    uint32_t                active_sources;

    /* Is this a file source? */
    unsigned                file       : 1;
    /* Is the udp_reader thread running? */
    unsigned                running    : 1;

    /* Set to 1 to signal the udp_reader thread to stop running */
    unsigned                stop       : 1;
    /* Was the previous packet from an unknown host? */
    unsigned                unknown_host:1;
};


/*
 *    The 'addr_to_source' member of 'skUDPSourceBase_t' is a
 *    red-black tree whose data members are 'peeraddr_source_t'
 *    objects.  They are used when multiple sources have
 *    accept-from-host clauses and listen on the same port so that the
 *    base can choose the source based on the peer address.
 */
/* typedef struct peeraddr_source_st peeraddr_source_t; */
struct peeraddr_source_st {
    const sk_sockaddr_t *addr;
    skUDPSource_t       *source;
};


/* LOCAL VARIABLE DEFINITIONS */

/* The 'source_bases' list contains pointers to all existing
 * skUDPSourceBase_t objects.  When creating a new skUDPSource_t, the
 * list is checked for existing sources listening on the same port. */
static sk_dllist_t *source_bases = NULL;
static pthread_mutex_t source_bases_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t source_bases_count = 0;

/* The 'sockets_count' variable maintains the number of open sockets;
 * used when setting the socket buffer size. */
static uint32_t sockets_count = 0;

/* FUNCTION DEFINITIONS */

/* Comparison function for the 'addr_to_source' red-black tree. */
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
 *    THREAD ENTRY POINT
 *
 *    The udp_reader() function is the thread for listening to data on
 *    a single UDP port.  The skUDPSourceBase_t object containing
 *    information about the port is passed into this function.  This
 *    thread is started from the udpSourceCreateBase() function.
 */
static void *
udp_reader(
    void               *vbase)
{
    skUDPSourceBase_t *base = (skUDPSourceBase_t*)vbase;
    char name[PATH_MAX];
    sk_sockaddr_t addr;
    void *data;
    peeraddr_source_t target;
    socklen_t len;
    skUDPSource_t *source = NULL;
    const peeraddr_source_t *match_address;

    assert(base != NULL);

    /* ignore all signals */
    skthread_ignore_signals();

    /* set 'name' to a printable name for this socket */
    if (base->port) {
        snprintf(name, sizeof(name), ("[%s]:%" PRIu16), base->name,base->port);
    } else {
        strncpy(name, base->name, sizeof(name));
    }

    DEBUGMSG("UDP listener started for %s", name);

    /* Lock for initialization */
    pthread_mutex_lock(&base->mutex);

    /* Note run state */
    base->running = 1;

    /* Disable cancelling */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    /* Allocate a space to read data into */
    data = (void *)malloc(base->data_size);
    if (NULL == data) {
        NOTICEMSG("Unable to create UDP listener data buffer for %s: %s",
                  name, strerror(errno));
        base->running = 0;
        pthread_cond_signal(&base->cond);
        pthread_mutex_unlock(&base->mutex);
        return NULL;
    }

    /* Signal completion of initialization */
    pthread_cond_broadcast(&base->cond);

    /* Wait for initial source to be connected to this base*/
    while (!base->stop && !base->active_sources) {
        pthread_cond_wait(&base->cond, &base->mutex);
    }
    pthread_mutex_unlock(&base->mutex);

    /* Main loop */
    while (!base->stop && base->active_sources && base->pfd_valid) {
        nfds_t i;
        ssize_t rv;

        /* Wait for data */
        rv = poll(base->pfd, base->pfd_len, POLL_TIMEOUT);
        if (rv == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                /* Interrupted by a signal, or internal alloc failed,
                 * try again. */
                continue;
            }
            /* Error */
            ERRMSG("Poll error for %s (%d) [%s]",
                   name, errno, strerror(errno));
            break;
        }

        /* See if we timed out.  We time out every now and then in
         * order to see if we need to shut down. */
        if (rv == 0) {
            continue;
        }

        /* Loop around file descriptors */
        for (i = 0; i < base->pfd_len; i++) {
            struct pollfd *pfd = &base->pfd[i];

            if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (!(pfd->revents & POLLNVAL)) {
                    close(pfd->fd);
                }
                pfd->fd = -1;
                base->pfd_valid--;
                DEBUGMSG("Poll for %s encountered a (%s,%s,%s) condition",
                         name, (pfd->revents & POLLERR) ? "ERR": "",
                         (pfd->revents & POLLHUP) ? "HUP": "",
                         (pfd->revents & POLLNVAL) ? "NVAL": "");
                DEBUGMSG("Closing file handle, %d remaining",
                         (int)base->pfd_valid);
                continue;
            }

            if (!(pfd->revents & POLLIN)) {
                continue;
            }

            /* Read the data */
            len = sizeof(addr);
            rv = recvfrom(pfd->fd, data, base->data_size, 0,
                          (struct sockaddr *)&addr, &len);

            /* Check for error or recv from wrong address */
            if (rv == -1) {
                switch (errno) {
                  case EINTR:
                    /* Interrupted by a signal: ignore now, try again
                     * later. */
                    continue;
                  case EAGAIN:
                    /* We should not be getting this, but have seen them
                     * in the field nonetheless.  Note and ignore them. */
                    NOTICEMSG(("Ignoring spurious EAGAIN from recvfrom() "
                               "call on %s"), name);
                    continue;
                  default:
                    ERRMSG("recvfrom error from %s (%d) [%s]",
                           name, errno, strerror(errno));
                    goto BREAK_WHILE;
                }
            }

            /* Match the address on the packet against the list of
             * from_addresses for each source that uses this base. */
            assert((base->any && !base->addr_to_source)
                   || (!base->any && base->addr_to_source));
            pthread_mutex_lock(&base->mutex);
            if (!base->addr_to_source) {
                /* we accept data from any address */
                source = base->any;
            } else {
                /* check whether we recognoze the sender */
                target.addr = &addr;
                match_address = ((const peeraddr_source_t*)
                                 rbfind(&target, base->addr_to_source));
                if (match_address) {
                    /* we recognize the sender */
                    source = match_address->source;
                    base->unknown_host = 0;
#if  !DEBUG_ACCEPT_FROM
                } else if (!base->unknown_host) {
                    /* additional packets seen from one or more
                     * distinct unknown senders; ignore */
                    pthread_mutex_unlock(&base->mutex);
                    continue;
#endif
                } else {
                    /* first packet seen from unknown sender after
                     * receiving packet from valid sensder; log */
                    char addr_buf[2 * SK_NUM2DOT_STRLEN];
                    base->unknown_host = 1;
                    pthread_mutex_unlock(&base->mutex);
                    skSockaddrString(addr_buf, sizeof(addr_buf), &addr);
                    INFOMSG("Ignoring packets from host %s", addr_buf);
                    continue;
                }
            }

            if (source->stopped) {
                pthread_mutex_unlock(&base->mutex);
                continue;
            }

            /* Copy the data */
            memcpy(source->pkt_buffer, data, rv);
            pthread_mutex_unlock(&base->mutex);

            if (source->reject_pkt_fn
                && source->reject_pkt_fn(rv, source->pkt_buffer,
                                         source->fn_callback_data))
            {
                /* reject the packet; do not advance to next location */
                continue;
            }

            /* Acquire the next location */
            source->pkt_buffer = (void*)circBufNextHead(source->data_buffer);
            if (source->pkt_buffer == NULL) {
                NOTICEMSG("Non-existent data buffer for %s", name);
                break;
            }
        } /* for (i = 0; i < base->nfds_t; i++) */
    } /* while (!base->stop && base->pfd_valid) */

  BREAK_WHILE:

    free(data);

    /* Set running to zero, and notify waiters of our exit */
    pthread_mutex_lock(&base->mutex);
    base->running = 0;
    pthread_cond_broadcast(&base->cond);
    pthread_mutex_unlock(&base->mutex);

    DEBUGMSG("UDP listener stopped for %s", name);

    return NULL;
}

/* Adjust socket buffer sizes */
static void
adjust_socketbuffers(
    void)
{
    static int sbufmin = SOCKETBUFFER_MINIMUM;
    static int sbufnominaltotal = SOCKETBUFFER_NOMINAL_TOTAL;
    static int env_calculated = 0;
    int sbufsize;
    const skUDPSourceBase_t *base;
    sk_dll_iter_t iter;

    assert(pthread_mutex_trylock(&source_bases_mutex) == EBUSY);

    if (!env_calculated) {
        const char *env;
        char *end;

        env = getenv(SOCKETBUFFER_NOMINAL_TOTAL_ENV);
        if (env) {
            long int val = strtol(env, &end, 0);
            if (end != env && *end == '\0') {
                if (val > INT_MAX) {
                    val = INT_MAX;
                }
                sbufnominaltotal = val;
            }
        }
        env = getenv(SOCKETBUFFER_MINIMUM_ENV);
        if (env) {
            long int val = strtol(env, &end, 0);
            if (end != env && *end == '\0') {
                if (val > INT_MAX) {
                    val = INT_MAX;
                }
                sbufmin = val;
            }
        }
        env_calculated = 1;
    }

    if (sockets_count) {
        assert(source_bases);
        sbufsize = sbufnominaltotal / sockets_count;
        if (sbufsize < sbufmin) {
            sbufsize = sbufmin;
        }

        skDLLAssignIter(&iter, source_bases);
        while (skDLLIterForward(&iter, (void **)&base) == 0) {
            nfds_t i;
            for (i = 0; i < base->pfd_len; i++) {
                if (base->pfd[i].fd >= 0) {
                    skGrowSocketBuffer(base->pfd[i].fd, SO_RCVBUF, sbufsize);
                }
            }
        }
    }
}


/* Destroy a base object and its associated thread */
static void
udpSourceDestroyBase(
    skUDPSourceBase_t  *base)
{
    nfds_t i;

    assert(base);

    pthread_mutex_lock(&base->mutex);

    assert(base->refcount == 0);

    if (base->file) {
        /* Close file */
        if (base->udpfile) {
#if SK_ENABLE_ZLIB
            gzclose(base->udpfile);
#else
            fclose(base->udpfile);
#endif
        }

        /* Free data structures */
        if (base->file_buffer) {
            free(base->file_buffer);
        }
    } else {
        /* If running, notify thread to stop, and then wait for exit */
        if (base->running) {
            base->stop = 1;
            while (base->running) {
                pthread_cond_wait(&base->cond, &base->mutex);
            }
        }
        /* Reap thread */
        pthread_join(base->thread, NULL);

        /* Close sockets */
        for (i = 0; i < base->pfd_len; i++) {
            if (base->pfd[i].fd >= 0) {
                pthread_mutex_lock(&source_bases_mutex);
                close(base->pfd[i].fd);
                base->pfd[i].fd = -1;
                --base->pfd_valid;
                --sockets_count;
                pthread_mutex_unlock(&source_bases_mutex);
            }
        }
        free(base->pfd);
        base->pfd = NULL;

        /* Free addr_to_source tree */
        if (base->addr_to_source) {
            assert(rblookup(RB_LUFIRST, NULL, base->addr_to_source) == NULL);
            rbdestroy(base->addr_to_source);
        }

        /* Remove from source_bases list */
        if (base->listen_address) {
            sk_dll_iter_t iter;
            skUDPSourceBase_t *b;

            pthread_mutex_lock(&source_bases_mutex);
            assert(source_bases);
            skDLLAssignIter(&iter, source_bases);
            while (skDLLIterForward(&iter, (void **)&b) == 0) {
                if (b == base) {
                    skDLLIterDel(&iter);
                    --source_bases_count;
                    if (source_bases_count == 0) {
                        skDLListDestroy(source_bases);
                        source_bases = NULL;
                    } else {
                        adjust_socketbuffers();
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&source_bases_mutex);
        }
    }

    pthread_mutex_unlock(&base->mutex);
    pthread_mutex_destroy(&base->mutex);
    if (!base->file) {
        pthread_cond_destroy(&base->cond);
    }

    free(base);
}


/*
 *    Create a base object and its associated thread.  The file
 *    descriptors for the base to monitor are in the 'pfd_addry'.  If
 *    an error occurs, close the descriptors and return NULL.
 */
static skUDPSourceBase_t *
udpSourceCreateBase(
    const char         *name,
    uint16_t            port,
    struct pollfd      *pfd_array,
    nfds_t              pfd_len,
    nfds_t              pfd_valid,
    uint32_t            itemsize,
    uint32_t            itemcount)
{
    skUDPSourceBase_t *base;
    int rv;

    /* Create base structure */
    base = (skUDPSourceBase_t*)calloc(1, sizeof(skUDPSourceBase_t));
    if (base == NULL) {
        nfds_t i;

        for (i = 0; i < pfd_len; i++) {
            if (pfd_array[i].fd >= 0) {
                close(pfd_array[i].fd);
                pfd_array[i].fd = -1;
            }
        }
        return NULL;
    }

    /* Fill the data structure */
    base->name = name;
    base->port = port;
    base->pfd = pfd_array;
    base->pfd_len = pfd_len;
    base->pfd_valid = pfd_valid;
    base->data_size = itemsize;
    base->bufsize = itemcount;
    pthread_mutex_init(&base->mutex, NULL);
    pthread_cond_init(&base->cond, NULL);

    /* Start the collection thread */
    pthread_mutex_lock(&base->mutex);
    rv = skthread_create(base->name, &base->thread, udp_reader, (void*)base);
    if (rv != 0) {
        pthread_mutex_unlock(&base->mutex);
        WARNINGMSG("Unable to spawn new thread for '%s': %s",
                   base->name, strerror(rv));
        udpSourceDestroyBase(base);
        return NULL;
    }

    /* Wait for the thread to finish initializing before returning. */
    pthread_cond_wait(&base->cond, &base->mutex);
    pthread_mutex_unlock(&base->mutex);

    return base;
}


/*
 *    Create a new source object that wraps 'base', where
 *    'from_address' is the peer address from which this source
 *    accepts packets.  If 'from_address' is NULL, the source accepts
 *    packets from any peer.
 */
static skUDPSource_t *
udpSourceCreateSource(
    skUDPSourceBase_t          *base,
    const sk_sockaddr_array_t  *from_address,
    udp_source_reject_fn        reject_pkt_fn,
    void                       *fn_callback_data)
{
    skUDPSource_t *source;

    if (base == NULL || base->file || base->any) {
        return NULL;
    }
    if (from_address == NULL && base->addr_to_source) {
        /* When no from_address is specified, this source accepts
         * packets from any address and there should be a one-to-one
         * mapping between source and base; however, the base already
         * references another source. */
        return NULL;
    }

    /* Create and initialize a new source object */
    source = (skUDPSource_t*)calloc(1, sizeof(skUDPSource_t));
    if (source == NULL) {
        return NULL;
    }
    source->reject_pkt_fn = reject_pkt_fn;
    source->fn_callback_data = fn_callback_data;
    source->from_addresses = from_address;

    source->data_buffer = circBufCreate(base->data_size, base->bufsize);
    if (source->data_buffer == NULL) {
        goto error;
    }
    source->pkt_buffer = (void *)circBufNextHead(source->data_buffer);
    assert(source->pkt_buffer != NULL);

    /* Lock the base */
    pthread_mutex_lock(&base->mutex);

    /* Attach the new source to the base */
    source->base = base;
    base->refcount++;

    if (!from_address) {
        /* one-to-one mapping between source and base */
        base->any = source;

    } else {
        /* Otherwise, we need to update the base so that it knows
         * packets coming from the 'from_address' should be processed
         * by this source */
        uint32_t i;

        if (base->addr_to_source == NULL) {
            base->addr_to_source = rbinit(peeraddr_compare, NULL);
            if (base->addr_to_source == NULL) {
                goto error;
            }
        }

        for (i = 0; i < skSockaddrArraySize(from_address); ++i) {
            peeraddr_source_t *addr;
            const peeraddr_source_t *found;

            /* Create the mapping between this from_address and the
             * source */
            addr = (peeraddr_source_t*)calloc(1, sizeof(peeraddr_source_t));
            if (addr == NULL) {
                goto error;
            }
            addr->source = source;
            addr->addr = skSockaddrArrayGet(from_address, i);

            /* Add the from_address to the tree */
            found = ((const peeraddr_source_t*)
                     rbsearch(addr, base->addr_to_source));
            if (found != addr) {
                if (found && (found->source == addr->source)) {
                    /* Duplicate address, same connection */
                    free(addr);
                    continue;
                }
                /* Memory error adding to tree */
                free(addr);
                goto error;
            }
        }

#if DEBUG_ACCEPT_FROM
        {
            char addr_buf[2 * SK_NUM2DOT_STRLEN];
            RBLIST *iter;
            peeraddr_source_t *addr;

            iter = rbopenlist(base->addr_to_source);
            while ((addr = (peeraddr_source_t *)rbreadlist(iter)) != NULL) {
                skSockaddrString(addr_buf, sizeof(addr_buf), addr->addr);
                DEBUGMSG("Base '%s' accepts packets from '%s'",
                         base->name, addr_buf);
            }
            rbcloselist(iter);
        }
#endif  /* DEBUG_ACCEPT_FROM */
    }

    /* Increase the number of active sources */
    base->active_sources++;
    pthread_cond_broadcast(&base->cond);

    pthread_mutex_unlock(&base->mutex);

    return source;

  error:
    pthread_mutex_unlock(&base->mutex);
    if (source->base != base && base->refcount == 0) {
        udpSourceDestroyBase(source->base);
    }
    skUDPSourceDestroy(source);
    return NULL;
}


/* Creates a UDP source representing a Berkeley socket. */
int
skUDPSourceCreateFromSockaddr(
    skUDPSource_t             **rsource,
    const sk_sockaddr_array_t  *from_address,
    const sk_sockaddr_array_t  *listen_address,
    uint32_t                    itemsize,
    uint32_t                    itemcount,
    udp_source_reject_fn        reject_pkt_fn,
    void                       *fn_callback_data)
{
    const sk_sockaddr_t *addr;
    skUDPSourceBase_t *base;
    skUDPSourceBase_t *cleanup_base = NULL;
    struct pollfd *pfd_array = NULL;
    skUDPSource_t *source = NULL;
    nfds_t pfd_valid;
    uint32_t i;
    int rv;
    uint16_t arrayport;
    int retval = -1;

    assert(rsource);
    assert(listen_address);

    pthread_mutex_lock(&source_bases_mutex);
    /* Look for any existing source-bases that are already listening
     * on this port. */
    if (source_bases) {
        sk_dll_iter_t iter;
        skDLLAssignIter(&iter, source_bases);
        while (skDLLIterForward(&iter, (void **)&base) == 0) {
            if (skSockaddrArrayEqual(listen_address, base->listen_address,
                                     SK_SOCKADDRCOMP_NOT_V4_AS_V6))
            {
                if ((base->data_size != itemsize)
                    || (base->bufsize != itemcount)
                    || (!skSockaddrArrayEqual(listen_address,
                                              base->listen_address,
                                              SK_SOCKADDRCOMP_NOT_V4_AS_V6)))
                {
                    /* errror: all sources that listen on this address
                     * must accept the same size packets.  Also, sources
                     * that listen to the same address must listen to
                     * *all* the same addresses. */
                    goto END;
                }
                /* found one. create the source using this source-base */
                *rsource = udpSourceCreateSource(base, from_address,
                                                 reject_pkt_fn,
                                                 fn_callback_data);
                if (*rsource == NULL) {
                    goto END;
                }
                /* Successful */
                retval = 0;
                goto END;
            }
            if (skSockaddrArrayMatches(listen_address, base->listen_address,
                                       SK_SOCKADDRCOMP_NOT_V4_AS_V6))
            {
                /* If two arrays match imperfectly, bail out */
                goto END;
            }
        }
    }

    /* If not, attempt to bind the address/port pairs */
    pfd_array = (struct pollfd*)calloc(skSockaddrArraySize(listen_address),
                                       sizeof(struct pollfd));
    if (pfd_array == NULL) {
        goto END;
    }
    pfd_valid = 0;

    /* arrayport holds the port of the listen_address array (0 ==
     * undecided) */
    arrayport = 0;

    DEBUGMSG(("Attempting to bind %" PRIu32 " addresses for %s"),
             skSockaddrArraySize(listen_address),
             skSockaddrArrayNameSafe(listen_address));
    for (i = 0; i < skSockaddrArraySize(listen_address); i++) {
        char addr_name[PATH_MAX];
        struct pollfd *pfd = &pfd_array[i];
        uint16_t port;

        addr = skSockaddrArrayGet(listen_address, i);

        skSockaddrString(addr_name, sizeof(addr_name), addr);

        /* Get a socket */
        pfd->fd = socket(addr->sa.sa_family, SOCK_DGRAM, 0);
        if (pfd->fd == -1) {
            DEBUGMSG("Skipping %s: Unable to create dgram socket: %s",
                     addr_name, strerror(errno));
            continue;
        }
        /* Bind socket to port */
        if (bind(pfd->fd, &addr->sa, skSockaddrLen(addr)) == -1) {
            DEBUGMSG("Skipping %s: Unable to bind: %s",
                     addr_name, strerror(errno));
            close(pfd->fd);
            pfd->fd = -1;
            continue;
        }
        DEBUGMSG("Bound %s for listening", addr_name);
        pfd_valid++;
        pfd->events = POLLIN;

        port = skSockaddrPort(addr);
        if (0 == arrayport) {
            arrayport = port;
        } else {
            /* All ports in the listen_address array should be the same */
            assert(arrayport == port);
        }
    }

    if (pfd_valid == 0) {
        ERRMSG("Failed to bind any addresses for %s",
               skSockaddrArrayNameSafe(listen_address));
        goto END;
    }

    DEBUGMSG(("Bound %" PRIu32 "/%" PRIu32 " addresses for %s"),
             (uint32_t)pfd_valid, skSockaddrArraySize(listen_address),
             skSockaddrArrayNameSafe(listen_address));

    assert(arrayport != 0);
    base = udpSourceCreateBase(skSockaddrArrayNameSafe(listen_address),
                               arrayport, pfd_array,
                               skSockaddrArraySize(listen_address),
                               pfd_valid, itemsize, itemcount);
    if (base == NULL) {
        goto END;
    }
    /* Mark base as destroyable on cleanup */
    cleanup_base = base;

    /* The base steals the reference to the array */
    pfd_array = NULL;

    base->listen_address = listen_address;

    /* Create the source */
    source = udpSourceCreateSource(base, from_address, reject_pkt_fn,
                                   fn_callback_data);
    if (source == NULL) {
        goto END;
    }

    if (source_bases == NULL) {
        source_bases = skDLListCreate(NULL);
        if (source_bases == NULL) {
            goto END;
        }
    }

    rv = skDLListPushTail(source_bases, base);
    if (rv != 0) {
        goto END;
    }
    /* Base is good.  Don't destroy it on cleanup */
    cleanup_base = NULL;

    *rsource = source;
    source = NULL;
    ++source_bases_count;
    sockets_count += pfd_valid;

    adjust_socketbuffers();

    /* successful */
    retval = 0;

  END:
    pthread_mutex_unlock(&source_bases_mutex);

    free(pfd_array);
    /* These could lock source_bases_mutex, so must be it must be unlocked
     * beforehand. */
    if (cleanup_base) {
        udpSourceDestroyBase(cleanup_base);
    }
    if (source) {
        skUDPSourceDestroy(source);
    }

    return retval;
}


/* Creates a UDP source representing a Unix domain socket.*/
int
skUDPSourceCreateFromUnixDomain(
    skUDPSource_t         **source_ret,
    const char             *uds,
    uint32_t                itemsize,
    uint32_t                itemcount,
    udp_source_reject_fn    reject_pkt_fn,
    void                   *fn_callback_data)
{
    sk_sockaddr_t addr;
    skUDPSource_t *source = NULL;
    skUDPSourceBase_t *base = NULL;
    struct pollfd *pfd = NULL;
    int sock;

    assert(source_ret);
    assert(uds);

    /* Get a socket */
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock == -1) {
        ERRMSG("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    /* Remove the domain socket if it exists. */
    if ((unlink(uds) == -1) && (errno != ENOENT)) {
        ERRMSG("Failed to unlink existing socket '%s': %s",
               uds, strerror(errno));
        return -1;
    }

    /* Fill in sockaddr */
    memset(&addr, 0, sizeof(addr));
    addr.un.sun_family = AF_UNIX;
    strncpy(addr.un.sun_path, uds, sizeof(addr.un.sun_path) - 1);

    /* Bind socket to port */
    if (bind(sock, &addr.sa, skSockaddrLen(&addr)) == -1) {
        ERRMSG("Failed to bind address '%s': %s", uds, strerror(errno));
        goto error;
    }

    /* Create the pfd object */
    pfd = (struct pollfd*)calloc(1, sizeof(struct pollfd));
    if (pfd == NULL) {
        goto error;
    }
    pfd->fd = sock;
    pfd->events = POLLIN;

    /* Create a base object */
    base = udpSourceCreateBase(uds, 0, pfd, 1, 1, itemsize, itemcount);
    if (base == NULL) {
        goto error;
    }
    pfd = NULL;

    /* Create the source object */
    source = udpSourceCreateSource(base, NULL, reject_pkt_fn,fn_callback_data);
    if (source == NULL) {
        goto error;
    }
    *source_ret = source;

    return 0;

  error:
    free(pfd);
    if (source != NULL) {
        skUDPSourceDestroy(source);
    }
    if (base != NULL) {
        udpSourceDestroyBase(base);
    }
    return -1;
}


/* Creates a UDP source representing a file of collected traffic. */
int
skUDPSourceCreateFromFile(
    skUDPSource_t         **source_ret,
    const char             *path,
    uint32_t                itemsize,
    udp_source_reject_fn    reject_pkt_fn,
    void                   *fn_callback_data)
{
    skUDPSourceBase_t *base = NULL;
    skUDPSource_t *source = NULL;

    assert(source_ret);
    assert(path);

    /* Create and initialize base structure */
    base = (skUDPSourceBase_t*)calloc(1, sizeof(skUDPSourceBase_t));
    if (base == NULL) {
        goto error;
    }
    pthread_mutex_init(&base->mutex, NULL);
    base->file = 1;
    base->data_size = itemsize;
    base->file_buffer = (uint8_t *)malloc(base->data_size);
    if (base->file_buffer == NULL) {
        goto error;
    }

    /* Create and initialize source structure */
    source = (skUDPSource_t*)calloc(1, sizeof(skUDPSource_t));
    if (source == NULL) {
        goto error;
    }
    source->reject_pkt_fn = reject_pkt_fn;
    source->fn_callback_data = fn_callback_data;

    /* open the file */
#if SK_ENABLE_ZLIB
    base->udpfile = gzopen(path, "r");
#else
    base->udpfile = fopen(path, "r");
#endif
    if (base->udpfile == NULL) {
        ERRMSG("Unable to open file '%s': %s", path, strerror(errno));
        goto error;
    }

    source->base = base;
    base->refcount = 1;
    base->active_sources = 1;
    *source_ret = source;

    return 0;

  error:
    if (base != NULL) {
        udpSourceDestroyBase(base);
    }
    if (source != NULL) {
        skUDPSourceDestroy(source);
    }

    return -1;
}


void
skUDPSourceStop(
    skUDPSource_t      *source)
{
    assert(source);

    if (!source->stopped) {
        skUDPSourceBase_t *base = source->base;

        /* Mark the source as stopped */
        source->stopped = 1;

        /* Notify the base that the source has stopped */
        if (base) {
            pthread_mutex_lock(&base->mutex);

            /* Decrement the base's active source count */
            assert(base->active_sources);
            --base->active_sources;

            /* If the count has reached zero, wait for the base thread
             * to stop running. */
            if (base->active_sources == 0) {
                while (base->running) {
                    pthread_cond_wait(&base->cond, &base->mutex);
                }
            }

            pthread_mutex_unlock(&base->mutex);
        }

        /* Unblock the data buffer */
        if (source->data_buffer) {
            circBufStop(source->data_buffer);
        }
    }
}


void
skUDPSourceDestroy(
    skUDPSource_t      *source)
{
    peeraddr_source_t target;
    const peeraddr_source_t *a2;
    uint32_t i;

    assert(source);

    /* Stop the source */
    if (!source->stopped) {
        skUDPSourceStop(source);
    }

    if (source->base) {
        pthread_mutex_lock(&source->base->mutex);

        if (source->base->any) {
            source->base->any = NULL;
        } else if (source->from_addresses && source->base->addr_to_source) {
            /* Remove the source's from_addresses from
             * base->addr_to_source */
            for (i = 0; i < skSockaddrArraySize(source->from_addresses); ++i) {
                target.addr = skSockaddrArrayGet(source->from_addresses, i);
                a2 = ((const peeraddr_source_t *)
                      rbdelete(&target, source->base->addr_to_source));
                assert(a2->source == source);
                free((void*)a2);
            }
        }
    }

    /* Destroy the circular buffer */
    if (source->data_buffer) {
        circBufDestroy(source->data_buffer);
    }

    /* Decref and possibly delete the base */
    if (source->base) {
        assert(source->base->refcount);
        source->base->refcount--;
        if (source->base->refcount == 0) {
            pthread_mutex_unlock(&source->base->mutex);
            udpSourceDestroyBase(source->base);
        } else {
            pthread_mutex_unlock(&source->base->mutex);
        }
    }

    free(source);
}


uint8_t *
skUDPSourceNext(
    skUDPSource_t      *source)
{
    skUDPSourceBase_t *base;
    uint8_t *data;

    assert(source);
    assert(source->base);

    base = source->base;

    pthread_mutex_lock(&base->mutex);

    if (base->stop) {
        data = NULL;
        goto END;
    }
    if (!base->file) {
        /* network based UDP source. circBufNextTail() will block
         * until data is ready */
        pthread_mutex_unlock(&base->mutex);
        if (source->data_buffer) {
            return circBufNextTail(source->data_buffer);
        }
        return NULL;
    }
    /* else file-based "UDP" source */

    for (;;) {
        int size;
#if SK_ENABLE_ZLIB
        size = gzread(base->udpfile, base->file_buffer, base->data_size);
#else
        size = (int)fread(base->file_buffer, 1, base->data_size,
                          base->udpfile);
#endif
        if (size <= 0 || (uint32_t)size < base->data_size) {
            /* error, end of file, or short read */
            data = NULL;
            break;
        }
        if (source->reject_pkt_fn
            && source->reject_pkt_fn(base->data_size, base->file_buffer,
                                     source->fn_callback_data))
        {
            /* reject the packet */
            continue;
        }
        data = base->file_buffer;
        break;
    }

  END:
    pthread_mutex_unlock(&base->mutex);

    return data;
}

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
