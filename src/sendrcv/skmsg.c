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
**  SiLK message functions
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skmsg.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "intdict.h"
#include "multiqueue.h"
#include "skmsg.h"
#include "libsendrcv.h"
#include <silk/skdeque.h>
#include <silk/sklog.h>
#include <silk/skstream.h>
#include <silk/utils.h>
#include <poll.h>
#if SK_ENABLE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/pkcs12.h>

/* The GNUTLS_VERSION_NUMBER is called LIBGNUTLS_VERSION_NUMBER in
 * GnuTlS 1.4.1*/
#ifndef GNUTLS_VERSION_NUMBER
#  ifdef  LIBGNUTLS_VERSION_NUMBER
#    define GNUTLS_VERSION_NUMBER LIBGNUTLS_VERSION_NUMBER
#  else
#    error  Cannot determine version of GnuTLS
#  endif
#endif

#if GNUTLS_VERSION_NUMBER <= 0x020b00
/* avoid stupid deprecation warnings from libgcrypt 1.5.x. */
SK_DIAGNOSTIC_IGNORE_PUSH("-Wdeprecated-declarations")

#include <gcrypt.h>

GCRY_THREAD_OPTION_PTHREAD_IMPL;

SK_DIAGNOSTIC_IGNORE_POP("-Wdeprecated-declarations")
#endif  /* GNUTLS_VERSION_NUMBER */

#endif  /* SK_ENABLE_GNUTLS */

/* SENDRCV_DEBUG is defined in libsendrcv.h */
#if (SENDRCV_DEBUG) & DEBUG_SKMSG_MUTEX
#  define SKTHREAD_DEBUG_MUTEX 1
#endif
#include <silk/skthread.h>


/* Define Constants */

/* Maximum number of CA certs that can be in the CA cert file */
#define MAX_CA_CERTS 32

/* Keepalive timeout for the control channel */
#define SKMSG_CONTROL_KEEPALIVE_TIMEOUT 60 /* seconds */

/* Time used by connections without keepalive times to determine how
 * whether the connection is stagnant. */
#define SKMSG_DEFAULT_STAGNANT_TIMEOUT (2 * SKMSG_CONTROL_KEEPALIVE_TIMEOUT)

/* Time used to determine how often to check to see if the connection
 * is still alive. */
#define SKMSG_MINIMUM_READ_SELECT_TIMEOUT SKMSG_CONTROL_KEEPALIVE_TIMEOUT


/* Read and write sides of control pipes */
#define READ 0
#define WRITE 1

#define SKMERR_MEMORY  -1
#define SKMERR_PIPE    -2
#define SKMERR_MUTEX   -3
#define SKMERR_PTHREAD -4
#define SKMERR_ERROR   -5
#define SKMERR_ERRNO   -6
#define SKMERR_CLOSED  -7
#define SKMERR_SHORT   -8
#define SKMERR_PARTIAL -9
#define SKMERR_EMPTY   -10

#define LISTENQ 5

#define SKMSG_CTL_CHANNEL_ANNOUNCE  0xFFFE
#define SKMSG_CTL_CHANNEL_REPLY     0xFFFD
#define SKMSG_CTL_CHANNEL_KILL      0xFFFC
#define SKMSG_CTL_CHANNEL_KEEPALIVE 0xFFFA
#define SKMSG_WRITER_UNBLOCKER      0xFFFB

#define SKMSG_MINIMUM_SYSTEM_CTL_CHANNEL 0xFFFA

/* Diffie-Hellman bits for GnuTLS */
#define DH_BITS 1024

/* TLS read timeout, in milliseconds */
#define TLS_POLL_TIMEOUT 1000

/* IO thread check timeout, in milliseconds*/
#define SKMSG_IO_POLL_TIMEOUT 1000


/* Define Macros */

/*
 *  this macro is used when the extra-level debugging statements write
 *  to the log, since we do not want the result of the log's printf()
 *  to trash the value of 'errno'
 */
#define WRAP_ERRNO(x)                                           \
    do { int _saveerrno = errno; x; errno = _saveerrno; }       \
    while (0)

/* SENDRCV_DEBUG is defined in libsendrcv.h */
#if 0 == ((SENDRCV_DEBUG) & DEBUG_SKMSG_OTHER)
#define DEBUG_PRINT1(x)
#define DEBUG_PRINT2(x, y)
#define DEBUG_PRINT3(x, y, z)
#define DEBUG_PRINT4(x, y, z, q)
#else
#define DEBUG_PRINT1(x)          WRAP_ERRNO(SKTHREAD_DEBUG_PRINT1(x))
#define DEBUG_PRINT2(x, y)       WRAP_ERRNO(SKTHREAD_DEBUG_PRINT2(x, y))
#define DEBUG_PRINT3(x, y, z)    WRAP_ERRNO(SKTHREAD_DEBUG_PRINT3(x, y, z))
#define DEBUG_PRINT4(x, y, z, q) WRAP_ERRNO(SKTHREAD_DEBUG_PRINT4(x, y, z, q))
#endif  /* (SENDRCV_DEBUG) & DEBUG_SKMSG_OTHER */


/*
 *  Logging messages for function entry/exit.
 *
 *    Use DEBUG_ENTER_FUNC at the beginning of every function.  Use
 *    RETURN(x) for functions that return a value, and RETURN_VOID for
 *    functions that have no return value.
 *
 *    SENDRCV_DEBUG is defined in libsendrcv.h.  DEBUG_SKMSG_FN turns
 *    on function entry/exit debug print macros.
 */
#if 0 == ((SENDRCV_DEBUG) & DEBUG_SKMSG_FN)
#  define DEBUG_ENTER_FUNC
#  define RETURN(x)         return x
#  define RETURN_VOID       return
#else
#  define DEBUG_ENTER_FUNC                                      \
    WRAP_ERRNO(SKTHREAD_DEBUG_PRINT2("Entering %s", __func__))

#  define RETURN(x)                                                     \
    do {                                                                \
        WRAP_ERRNO(SKTHREAD_DEBUG_PRINT2("Exiting %s", __func__));      \
        return x;                                                       \
    } while (0)

#  define RETURN_VOID                                                   \
    do {                                                                \
        WRAP_ERRNO(SKTHREAD_DEBUG_PRINT2("Exiting %s", __func__));      \
        return;                                                         \
    } while (0)

#endif  /* ((SENDRCV_DEBUG) & DEBUG_SKMSG_FN) */


#if SK_ENABLE_GNUTLS
#  define UNUSED_IF_NOTLS(x) x
#else
#  define UNUSED_IF_NOTLS(x) UNUSED(x)
#endif /* SK_ENABLE_GNUTLS */


/* MUTEX_* macros are defined in skthread.h */

/* Mutex accessor */
#define QUEUE_MUTEX(q)      (&(q)->root->mutex)

/* Queue lock */
#define QUEUE_LOCK(q)       MUTEX_LOCK(QUEUE_MUTEX(q))

/* Queue unlock */
#define QUEUE_UNLOCK(q)     MUTEX_UNLOCK(QUEUE_MUTEX(q))

/* Queue wait */
#define QUEUE_WAIT(cond, q) MUTEX_WAIT(cond, QUEUE_MUTEX(q))

/* Queue locked assert */
#define ASSERT_QUEUE_LOCK(q)                                    \
    assert(pthread_mutex_trylock(QUEUE_MUTEX(q)) == EBUSY)


/* Any place an XASSERT occurs could be replaced with better error
   handling at some point in time. */
#define XASSERT(x) do {                                                 \
    if (!(x)) {                                                         \
        CRITMSG("Unhandled error at " __FILE__ ":%" PRIu32 " \"" #x "\"", \
                __LINE__);                                              \
        skAbort();                                                      \
    }                                                                   \
    } while (0)
#define MEM_ASSERT(x) do {                                              \
    if (!(x)) {                                                         \
        CRITMSG(("Memory allocation error creating \"" #x "\" at "      \
                 __FILE__ ":%" PRIu32), __LINE__);                      \
        abort();                                                        \
    }                                                                   \
    } while (0)

/* Macros to deal with thread creation and destruction */
#define QUEUE_TINFO(q) ((q)->root->tinfo)

#define THREAD_INFO_INIT(q)                             \
    do {                                                \
        pthread_cond_init(&QUEUE_TINFO(q).cond, NULL);  \
        QUEUE_TINFO(q).count = 0;                       \
    } while (0)

#define THREAD_INFO_DESTROY(q) pthread_cond_destroy(&QUEUE_TINFO(q).cond)


#define THREAD_START(name, rv, q, loc, fn, arg)                 \
    do {                                                        \
        DEBUG_PRINT1("THREAD_START");                           \
        ASSERT_QUEUE_LOCK(q);                                   \
        QUEUE_TINFO(q).count++;                                 \
        (rv) = skthread_create(name, (loc), (fn), (arg));       \
        if ((rv) != 0) {                                        \
            QUEUE_TINFO(q).count--;                             \
        }                                                       \
    } while (0)

#define THREAD_END(q)                                           \
    do {                                                        \
        DEBUG_PRINT1("THREAD_END");                             \
        ASSERT_QUEUE_LOCK(q);                                   \
        assert(QUEUE_TINFO(q).count != 0);                      \
        QUEUE_TINFO(q).count--;                                 \
        DEBUG_PRINT2("THREAD END COUNT decremented to %d",      \
                     QUEUE_TINFO(q).count);                     \
        MUTEX_BROADCAST(&QUEUE_TINFO(q).cond);                  \
    } while (0)

#define THREAD_WAIT_END(q, state)                               \
    do {                                                        \
        DEBUG_PRINT1("WAITING FOR THREAD_END");                 \
        ASSERT_QUEUE_LOCK(q);                                   \
        while ((state) != SKM_THREAD_ENDED) {                   \
            QUEUE_WAIT(&QUEUE_TINFO(q).cond, q);                \
        }                                                       \
        DEBUG_PRINT1("FINISHED WAITING FOR THREAD_END");        \
    } while (0)

#define THREAD_WAIT_ALL_END(q)                                  \
    do {                                                        \
        DEBUG_PRINT1("WAITING FOR ALL THREAD_END");             \
        ASSERT_QUEUE_LOCK(q);                                   \
        while (QUEUE_TINFO(q).count != 0) {                     \
            DEBUG_PRINT2("THREAD ALL END WAIT COUNT == %d",     \
                         QUEUE_TINFO(q).count);                 \
            QUEUE_WAIT(&QUEUE_TINFO(q).cond, q);                \
        }                                                       \
        DEBUG_PRINT1("FINISHED WAITING FOR ALL THREAD_END");    \
    } while (0)


/*
 *  is_stagnat = CONNECTION_STAGNANT(conn, t);
 *
 *    Compare current time 't' with time the last message was received
 *    for connection 'conn'; return 1 if the time difference is larger
 *    than the timeout or 0 otherwise.
 */
#define CONNECTION_STAGNANT(conn, t)                                    \
    (difftime(t, (conn)->last_recv) > ((conn)->keepalive                \
                                       ? (2.0 * (conn)->keepalive)      \
                                       : SKMSG_DEFAULT_STAGNANT_TIMEOUT))

/* return a string for a received event from poll() */
#define SK_POLL_EVENT_STR(ev_)                          \
    (((ev_) & POLLHUP) ? "POLLHUP"                      \
     : (((ev_) & POLLERR) ? "POLLERR"                   \
        : (((ev_) & POLLNVAL) ? "POLLNVAL" : "")))


/*
 *  ASSERT_RESULT(ar_func_args, ar_type, ar_expected);
 *
 *    ar_func_args  -- is a function and any arugments it requires
 *    ar_type       -- is the type that ar_func_args returns
 *    ar_expected   -- is the expected return value from ar_func_args
 *
 *    If assert() is disabled, simply call 'ar_func_args'.
 *
 *    If assert() is enabled, call 'ar_func_args', capture its result,
 *    and assert() that its result is 'ar_expected'.
 */
#ifdef  NDEBUG
/* asserts are disabled; just call the function */
#define ASSERT_RESULT(ar_func_args, ar_type, ar_expected)  ar_func_args
#else
#define ASSERT_RESULT(ar_func_args, ar_type, ar_expected)       \
    do {                                                        \
        ar_type ar_rv = (ar_func_args);                         \
        assert(ar_rv == (ar_expected));                         \
    } while(0)
#endif



/*** Local types ***/

/* The type of message headers */
typedef struct sk_msg_hdr_st {
    skm_channel_t channel;
    skm_type_t    type;
    skm_len_t     size;
} sk_msg_hdr_t;

/* The type of messages */
/* typedef struct sk_msg_st sk_msg_t;  // from skmsg.h */
struct sk_msg_st {
    sk_msg_hdr_t hdr;
    void       (*free_fn)(uint16_t, struct iovec *);
    void       (*simple_free)(void *);
    uint16_t     segments;
    struct iovec segment[1];
};

/* Buffer for reading an sk_msg_t; used to support partial reads */
typedef struct sk_msg_read_buf_st {
    /* The message being read */
    sk_msg_t *msg;
    /* Location to read into */
    uint8_t  *loc;
    /* Number of bytes still to read */
    uint16_t  count;
} sk_msg_read_buf_t;

/* Buffer for writing an sk_msg_t; used to support partial writes */
typedef struct sk_msg_write_buf_st {
    /* The message being written */
    sk_msg_t   *msg;
    /* Number of bytes of the message remaining to write */
    ssize_t     msg_size;
    /* The index of the iov segment currently being sent */
    uint16_t    cur_seg;
    /* Number of bytes of the current segment that have been sent */
    uint16_t    seg_offset;
} sk_msg_write_buf_t;


/* Allow tracking of thread entance and exits. */
typedef struct sk_thread_info_st {
    pthread_cond_t  cond;
    uint32_t        count;
} sk_thread_info_t;

typedef enum {
    SKM_THREAD_BEFORE,
    SKM_THREAD_RUNNING,
    SKM_THREAD_SHUTTING_DOWN,
    SKM_THREAD_ENDED
} sk_thread_state_t;

/* The type of a message queue root */
typedef struct sk_msg_root_st {
    /* Global mutex for message queue */
    pthread_mutex_t     mutex;

    /* The next channel number to try to allocate */
    skm_channel_t       next_channel;

    /* Information about active threads */
    sk_thread_info_t    tinfo;

    /* Map of channel-id to channels */
    int_dict_t         *channel;
    /* Map of read socket to connections */
    int_dict_t         *connection;
    /* Map of channel-ids to queues */
    int_dict_t         *groups;

    /* The information for binding and listening for connections */
    struct pollfd      *pfd;
    nfds_t              pfd_len;
    pthread_t           listener;
    /* The listener state */
    sk_thread_state_t   listener_state;
    /* Listener condition variable */
    pthread_cond_t      listener_cond;

    sk_msg_queue_t     *shutdownqueue;

#if SK_ENABLE_GNUTLS
    /* Auth/Encryption credentials */
    gnutls_certificate_credentials_t cred;
#endif

    unsigned            shuttingdown: 1;
#if SK_ENABLE_GNUTLS
    unsigned            cred_set: 1;
    unsigned            bind_tls: 1;
#endif
} sk_msg_root_t;


/* The type of a message queue */
/* typedef struct sk_msg_queue_st sk_msg_queue_t; */
struct sk_msg_queue_st {
    sk_msg_root_t      *root;
    /* Map of channels */
    int_dict_t         *channel;
    /* Queue group for all channels */
    mq_multi_t         *group;
    pthread_cond_t      shutdowncond;
    unsigned            shuttingdown: 1;
};


/* States for connections and channels */
typedef enum {
    SKM_CREATED,
    SKM_CONNECTING,
    SKM_CONNECTED,
    SKM_CLOSED
} sk_msg_state_t;

/* States for threads */
typedef enum {
    SKM_SEND_INTERNAL,
    SKM_SEND_REMOTE,
    SKM_SEND_CONTROL
} sk_send_type_t;

/* The type of connection to use */
typedef enum {
    CONN_TCP,
    CONN_TLS
} skm_conn_t;

/* TLS connections */
typedef enum {
    SKM_TLS_NONE,
    SKM_TLS_CLIENT,
    SKM_TLS_SERVER
} skm_tls_type_t;


/* Forward declarations; these strucures reference each other */
typedef struct sk_msg_channel_queue_st sk_msg_channel_queue_t;
typedef struct sk_msg_conn_queue_st sk_msg_conn_queue_t;

/* Protocol specific transport functions */
typedef struct sk_msg_transport_fn_st {
    int  (*send)(sk_msg_conn_queue_t   *conn,
                 sk_msg_write_buf_t    *write_buf);
    int  (*recv)(sk_msg_conn_queue_t   *conn,
                 sk_msg_t             **msg);
} sk_msg_transport_fn_t;

/* Represents a connected socket or pipe */
/* typedef struct sk_msg_conn_queue_st sk_msg_conn_queue_t; */
struct sk_msg_conn_queue_st {
    /* Read socket */
    int                     rsocket;
    /* Write socket */
    int                     wsocket;

    /* Address of connection */
    struct sockaddr        *addr;
    /* Length of address of connection */
    socklen_t               addrlen;

    /* send and receive functions */
    sk_msg_transport_fn_t   fn;
    /* Transport type */
    skm_conn_t              transport;

    /* Channel map */
    int_dict_t             *channelmap;
    /* Channel refcount */
    uint16_t                refcount;

    /* Current state of connection */
    sk_msg_state_t          state;

    /* Outgoing write queue */
    skDeque_t               queue;
    /* Writer thread handle */
    pthread_t               writer;
    /* State */
    sk_thread_state_t       writer_state;
    /* Condition variable for thread */
    pthread_cond_t          writer_cond;

    /* The reader thread */
    pthread_t               reader;
    /* The reader state */
    sk_thread_state_t       reader_state;
    /* Reader condition variable */
    pthread_cond_t          reader_cond;

    /* Keepalive timeout */
    uint16_t                keepalive;
    /* Time of last received data */
    time_t                  last_recv;

    /* Buffer to support partial read of incoming messages */
    sk_msg_read_buf_t       msg_read_buf;

    /* Pre-connected initial channel */
    sk_msg_channel_queue_t *first_channel;

#if SK_ENABLE_GNUTLS
    gnutls_session_t        session;
    unsigned                use_tls : 1;
#endif
};


/* Represents a channel */
/* typedef struct sk_msg_channel_queue_st sk_msg_channel_queue_t; */
struct sk_msg_channel_queue_st {
    /* channel's queue */
    mq_queue_t            *queue;
    /* local channel ID */
    skm_channel_t          channel;
    /* remote channel ID */
    skm_channel_t          rchannel;
    /* channel state */
    sk_msg_state_t         state;
    /* associated connection */
    sk_msg_conn_queue_t   *conn;
    /* group associated with this channel */
    sk_msg_queue_t        *group;
    /* pending condition variable */
    pthread_cond_t         pending;
    /* whether we are waiting for connection */
    unsigned               is_pending: 1;
    unsigned               flushing: 1;
};

/* Used for passing data to a writer thread */
typedef struct sk_queue_and_conn_st {
    sk_msg_queue_t         *q;
    sk_msg_conn_queue_t    *conn;
} sk_queue_and_conn_t;


/* Used to represent a local/remote channel pair */
typedef struct sk_channel_pair_st {
    skm_channel_t   lchannel;
    skm_channel_t   rchannel;
} sk_channel_pair_t;


/*** Local function prototypes ***/

static void *reader_thread(void *);
static void *writer_thread(void *);
static void *listener_thread(void *);

static int
destroy_connection(
    sk_msg_queue_t         *q,
    sk_msg_conn_queue_t    *conn);

static int
send_message(
    sk_msg_queue_t     *q,
    skm_channel_t       lchannel,
    skm_type_t          type,
    void               *message,
    skm_len_t           length,
    sk_send_type_t      send_type,
    int                 no_copy,
    void              (*free_fn)(void *));

static int
send_message_internal(
    sk_msg_channel_queue_t *chan,
    sk_msg_t               *msg,
    sk_send_type_t          send_type);


/*** Local variables ***/


#if SK_ENABLE_GNUTLS

/* Diffie-Hellman parameters and a mutex to control access to them */
static pthread_mutex_t sk_msg_gnutls_mutex = PTHREAD_MUTEX_INITIALIZER;
static int sk_msg_gnutls_initialized = 0;
static gnutls_dh_params_t dh_params;

#endif /* SK_ENABLE_GNUTLS */



/* Utility functions */


static sk_msg_channel_queue_t *
find_channel(
    sk_msg_queue_t     *q,
    skm_channel_t       channel)
{
    sk_msg_channel_queue_t **chan;

    DEBUG_ENTER_FUNC;

    chan = (sk_msg_channel_queue_t **)int_dict_get(q->root->channel,
                                                   channel, NULL);

    RETURN(chan ? *chan : NULL);
}


static void
msg_simple_free(
    uint16_t            n,
    struct iovec       *iov)
{
    while (n > 0) {
        --n;
        free(iov[n].iov_base);
    }
}


static void
sk_destroy_report_message(
    void               *vmsg)
{
    sk_msg_t *msg = (sk_msg_t *)vmsg;

    DEBUG_ENTER_FUNC;

    DEBUG_PRINT3("Queue (destroy): chan=%#x type=%#x",
                 msg->hdr.channel, msg->hdr.type);

    skMsgDestroy(msg);

    RETURN_VOID;
}


static void
set_nonblock(
    int                 fd)
{
    int flags, rv;

    DEBUG_ENTER_FUNC;

    flags = fcntl(fd, F_GETFL, 0);
    XASSERT(flags != -1);
    flags |= O_NONBLOCK;
    rv = fcntl(fd, F_SETFL, flags);
    XASSERT(rv != -1);

    RETURN_VOID;
}


static const char *
skmerr_strerror(
    int                 retval)
{
    switch (retval) {
      case SKMERR_MEMORY:
        return "Memory allocation failure";
      case SKMERR_PIPE:
        return "Failed to create pipe";
      case SKMERR_MUTEX:
        return "Failed to initialize pthread mutex or condition variable";
      case SKMERR_PTHREAD:
        return "Error with pthread";
      case SKMERR_ERROR:
        return "Generic error";
      case SKMERR_ERRNO:
        return strerror(errno);
      case SKMERR_CLOSED:
        return "Connection is closed";
      case SKMERR_SHORT:
        return "Short read or write (fail)";
      case SKMERR_PARTIAL:
        return "Partial read or write (will retry)";
      case SKMERR_EMPTY:
        return "Empty read (will retry)";
      default:
        return "Unknown SKMERR_ error code";
    }
}



#if SK_ENABLE_GNUTLS
/*
 *  status = check_cert_times(x509_cert, generic_file, file_path);
 *
 *    Check the activation and expiry times on the certificate
 *    'x509_cert'.  If the times are valid, return 0.  Otherwise,
 *    print an error about an error in 'generic_file' (which
 *    references the switch used to load the file) and 'file_path'
 *    (which is the path to the file), and return -1.
 */
static int
check_cert_times(
    gnutls_x509_crt_t   cert,
    const char         *file_generic,
    const char         *file_path)
{
    time_t now, t;
    int rv = 0;

    DEBUG_ENTER_FUNC;

    time(&now);

    t = gnutls_x509_crt_get_activation_time(cert);
    if (t == (time_t)-1) {
        INFOMSG("Error loading %s file '%s': Unable to get activation time",
                file_generic, file_path);
        rv = -1;
    } else if (now < t) {
        INFOMSG("Error loading %s file '%s': Certificate is not yet valid",
                file_generic, file_path);
        rv = -1;
    }

    t = gnutls_x509_crt_get_expiration_time(cert);
    if (t == (time_t)-1) {
        INFOMSG("Error loading %s file '%s': Unable to get expiration time",
                file_generic, file_path);
        rv = -1;
    } else if (now > t) {
        INFOMSG("Error loading %s file '%s': Certificate has expired",
                file_generic, file_path);
        rv = -1;
    }

    RETURN(rv);
}


/*
 *  status = read_trust_file(cred, ca_filename, x509_format);
 *
 *    Read the CA list from 'ca_filename', which is in the
 *    'x509_format', and store the CAs in the credential 'cred'.
 */
static int
read_trust_file(
    gnutls_certificate_credentials_t    cred,
    const char                         *ca_filename,
    gnutls_x509_crt_fmt_t               x509_format)
{
    gnutls_x509_crt_t ca_list[MAX_CA_CERTS];
    gnutls_x509_crt_t *ca;
    unsigned int ca_list_len = MAX_CA_CERTS;
    gnutls_datum_t data;
    off_t file_len;
    int fd;
    int rv = -1;

    DEBUG_ENTER_FUNC;

    if ((fd = open(ca_filename, O_RDONLY)) == -1
        || (file_len = lseek(fd, 0, SEEK_END)) == -1)
    {
        INFOMSG("Error loading x509 CA trust file '%s': %s",
                ca_filename, strerror(errno));
        goto END;
    }

    data.data = (unsigned char*)mmap(0, file_len, PROT_READ, MAP_SHARED, fd, 0);
    if (data.data == MAP_FAILED) {
        INFOMSG("Error mapping x509 CA trust file '%s': %s",
                ca_filename, strerror(errno));
        close(fd);
        goto END;
    }
    close(fd);
    data.size = (unsigned int)file_len;
    rv = gnutls_x509_crt_list_import(ca_list, &ca_list_len, &data,
                                     x509_format, 0);
    if (rv < 0) {
        INFOMSG("Failed to import x509 CA trust file '%s': %s",
                ca_filename, gnutls_strerror(rv));
        munmap(data.data, file_len);
        goto END;
    }

    rv = 0;
    ca = ca_list;
    while (ca_list_len) {
        if (check_cert_times(*ca, "x509 CA trust", ca_filename)) {
            rv = -1;
        }
        gnutls_x509_crt_deinit(*ca);
        ++ca;
        --ca_list_len;
    }

    if (rv == 0) {
        rv = gnutls_certificate_set_x509_trust_mem(cred, &data,
                                                   x509_format);
        if (rv < 0) {
            INFOMSG("Error loading x509 CA trust file '%s': %s",
                    ca_filename, gnutls_strerror(rv));
        } else {
            rv = 0;
        }
    }
    munmap(data.data, file_len);

  END:
    RETURN((0 == rv) ? 0 : -1);
}


/*
 *  status = read_key_file(cred, cert_filename, key_filename, x509_format);
 *
 *    Read the certificate from 'cert_filename' and the key from
 *    'key_filename', where both files are in in the 'x509_format'.
 *    Store the certificate and key in the credential 'cred'.
 *
 *    In addition, check the activation and expiration times on the
 *    certificates.
 *
 *    This function is similar to the following direct GnuTLS call,
 *    except our function checks times while the GnuTLS function does
 *    not.
 *
 *    rv = gnutls_certificate_set_x509_key_file(cred, cert_filename,
 *                                              key_filename, x509_format);
 *    if (rv < 0) { ERRROR; }
 */
static int
read_key_file(
    gnutls_certificate_credentials_t    cred,
    const char                         *cert_filename,
    const char                         *key_filename,
    gnutls_x509_crt_fmt_t               x509_format)
{
    skstream_t *stream = NULL;
    gnutls_x509_crt_t cert = NULL;
    gnutls_x509_privkey_t key = NULL;
    gnutls_datum_t datum;
    ssize_t file_size = 0;
    int rv;

    DEBUG_ENTER_FUNC;

    /* initialize certificate and key */
    rv = gnutls_x509_crt_init(&cert);
    if (rv != GNUTLS_E_SUCCESS) {
        goto ERROR;
    }
    rv = gnutls_x509_privkey_init(&key);
    if (rv != GNUTLS_E_SUCCESS) {
        goto ERROR;
    }

    /* Read certificate file */
    if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_OTHERBINARY))
        || (rv = skStreamBind(stream, cert_filename))
        || (rv = skStreamOpen(stream)))
    {
        skStreamPrintLastErr(stream, rv, &ERRMSG);
        goto END;
    }
    datum.data = (unsigned char*)skStreamReadToEndOfFile(stream, &file_size);
    datum.size = file_size;
    if (NULL == datum.data) {
        skStreamPrintLastErr(stream, rv, &ERRMSG);
        goto END;
    }
    skStreamDestroy(&stream);

    /* Parse certificate */
    rv = gnutls_x509_crt_import(cert, &datum, x509_format);
    free(datum.data);
    if (rv != GNUTLS_E_SUCCESS) {
        goto ERROR;
    }

    /* Read key file */
    if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_OTHERBINARY))
        || (rv = skStreamBind(stream, key_filename))
        || (rv = skStreamOpen(stream)))
    {
        skStreamPrintLastErr(stream, rv, &ERRMSG);
        goto END;
    }
    datum.data = (unsigned char*)skStreamReadToEndOfFile(stream, &file_size);
    datum.size = file_size;
    if (NULL == datum.data) {
        skStreamPrintLastErr(stream, rv, &ERRMSG);
        goto END;
    }
    skStreamDestroy(&stream);

    /* Parse key */
    rv = gnutls_x509_privkey_import(key, &datum, x509_format);
    free(datum.data);
    if (rv != GNUTLS_E_SUCCESS) {
        goto ERROR;
    }

    /* Put cert and key onto the credential */
    rv = gnutls_certificate_set_x509_key(cred, &cert, 1, key);
    if (rv != GNUTLS_E_SUCCESS) {
        goto ERROR;
    }

    /* Check activation/expire times on the certifcate */
    rv = check_cert_times(cert, "certificate", cert_filename);
    if (rv) {
        goto END;
    }

    rv = 0;

    /* Use "goto ERROR" to print a GnuTLS error; "goto END" otherwise */
  ERROR:
    if (0 != rv) {
        ERRMSG("Error loading certificate or key files '%s', '%s': %s",
               cert_filename, key_filename, gnutls_strerror(rv));
    }
  END:
    skStreamDestroy(&stream);
    if (cert) {
        gnutls_x509_crt_deinit(cert);
    }
    if (key) {
        gnutls_x509_privkey_deinit(key);
    }
    RETURN((0 == rv) ? 0 : -1);
}


/*
 *  status = read_pkcs12_file(cred, cert_filename, x509_format, password);
 *
 *    Read the PKCS12 certificates and keys from 'cert_filename',
 *    where the file is in in the 'x509_format'.  Store the
 *    certificate and key in the credential 'cred'.  If the file is
 *    password protected, the 'password' is the password; otherwise it
 *    is NULL.
 *
 *    This function is similar to the following direct GnuTLS call,
 *    except our function checks times while the GnuTLS function does
 *    not.
 *
 *    rv = gnutls_certificate_set_x509_simple_pkcs12_file(cred, cert_filename,
 *                                                        x509_format,
 *                                                        password);
 *    if (rv < 0) { ERROR; }
 */
static int
read_check_pkcs12(
    gnutls_certificate_credentials_t    cred,
    const char                         *cert_filename,
    gnutls_x509_crt_fmt_t               x509_format,
    const char                         *password)
{
    /* To verify the PKCS12 cert, we basically need to inline
     * everything that the GnuTLS functions
     * gnutls_certificate_set_x509_simple_pkcs12_mem() and
     * parse_pkcs12() do. Whee! */

    skstream_t *stream = NULL;
    gnutls_datum_t p12blob;
    ssize_t file_size = 0;
    int rv;

    gnutls_pkcs12_t p12 = NULL;
    gnutls_x509_privkey_t key = NULL;
    gnutls_x509_crt_t cert = NULL;
    gnutls_pkcs12_bag_t bag = NULL;
    int idx = 0;
    size_t cert_id_size = 0;
    size_t key_id_size = 0;
    unsigned char cert_id[20];
    unsigned char key_id[20];
    int elements_in_bag;
    int i;
    int bag_type;
    gnutls_datum_t data;

     DEBUG_ENTER_FUNC;

    /* Read pkcs12 file */
    if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_OTHERBINARY))
        || (rv = skStreamBind(stream, cert_filename))
        || (rv = skStreamOpen(stream)))
    {
        skStreamPrintLastErr(stream, rv, &ERRMSG);
        goto END;
    }
    p12blob.data = (unsigned char*)skStreamReadToEndOfFile(stream, &file_size);
    p12blob.size = file_size;
    if (NULL == p12blob.data) {
        skStreamPrintLastErr(stream, rv, &ERRMSG);
        goto END;
    }
    skStreamDestroy(&stream);

    /* Initialize PKCS#12 certificate */
    rv = gnutls_pkcs12_init(&p12);
    if (rv < 0) {
        free(p12blob.data);
        p12 = NULL;
        goto ERROR;
    }

    /* Parse PKCS#12 certificate */
    rv = gnutls_pkcs12_import(p12, &p12blob, x509_format, 0);
    free(p12blob.data);
    if (rv < 0)    {
        goto ERROR;
    }
    if (password) {
        rv = gnutls_pkcs12_verify_mac(p12, password);
        if (rv < 0) {
            goto ERROR;
        }
    }

    /* Following taken from parse_pkcs12() in GnuTLS 2.8.6 sources */

    /* find the first private key */
    for (idx = 0; NULL == key; ++idx) {
        rv = gnutls_pkcs12_bag_init(&bag);
        if (rv < 0) {
            bag = NULL;
            goto ERROR;
        }
        rv = gnutls_pkcs12_get_bag(p12, idx, bag);
        if (rv < 0) {
            /* either error or no more bags in pkcs structure */
            goto ERROR;
        }

        rv = gnutls_pkcs12_bag_get_type(bag, 0);
        if (rv < 0) {
            goto ERROR;
        }
        if (rv == GNUTLS_BAG_ENCRYPTED) {
            rv = gnutls_pkcs12_bag_decrypt(bag, password);
            if (rv < 0) {
                goto ERROR;
            }
        }

        elements_in_bag = gnutls_pkcs12_bag_get_count(bag);
        for (i = 0; NULL == key && i < elements_in_bag; ++i) {
            bag_type = gnutls_pkcs12_bag_get_type(bag, i);
            rv = gnutls_pkcs12_bag_get_data(bag, i, &data);

            switch (bag_type) {
              case GNUTLS_BAG_PKCS8_ENCRYPTED_KEY:
              case GNUTLS_BAG_PKCS8_KEY:
                rv = gnutls_x509_privkey_init(&key);
                if (rv < 0) {
                    key = NULL;
                    goto ERROR;
                }
                rv = gnutls_x509_privkey_import_pkcs8(
                    key, &data, GNUTLS_X509_FMT_DER, password,
                    ((bag_type==GNUTLS_BAG_PKCS8_KEY) ? GNUTLS_PKCS_PLAIN :0));
                if (rv < 0) {
                    goto ERROR;
                }
                /* get key_id to compare to cert_id below */
                key_id_size = sizeof(key_id);
                rv = gnutls_x509_privkey_get_key_id(key, 0, key_id,
                                                    &key_id_size);
                if (rv < 0) {
                    goto ERROR;
                }
                break;

              default:
                break;
            }
        }

        gnutls_pkcs12_bag_deinit(bag);
    }

    bag = NULL;

    /* now find the corresponding certificate */
    for (idx = 0; NULL == cert; ++idx) {
        rv = gnutls_pkcs12_bag_init(&bag);
        if (rv < 0) {
            bag = NULL;
            goto ERROR;
        }
        rv = gnutls_pkcs12_get_bag(p12, idx, bag);
        if (rv < 0) {
            /* either error or no more bags in pkcs structure */
            goto ERROR;
        }

        rv = gnutls_pkcs12_bag_get_type(bag, 0);
        if (rv < 0) {
            goto ERROR;
        }
        if (rv == GNUTLS_BAG_ENCRYPTED) {
            rv = gnutls_pkcs12_bag_decrypt(bag, password);
            if (rv < 0) {
                goto ERROR;
            }
        }

        elements_in_bag = gnutls_pkcs12_bag_get_count(bag);
        for (i = 0; NULL == cert && i < elements_in_bag; ++i) {
            bag_type = gnutls_pkcs12_bag_get_type(bag, i);
            rv = gnutls_pkcs12_bag_get_data(bag, i, &data);

            switch (bag_type) {
              case GNUTLS_BAG_CERTIFICATE:
                rv = gnutls_x509_crt_init(&cert);
                if (rv < 0) {
                    cert = NULL;
                    goto ERROR;
                }
                rv = gnutls_x509_crt_import(cert, &data,
                                            GNUTLS_X509_FMT_DER);
                if (rv < 0) {
                    goto ERROR;
                }
                /* check if the key id match */
                cert_id_size = sizeof(cert_id);
                rv = gnutls_x509_crt_get_key_id(cert, 0, cert_id,
                                                &cert_id_size);
                if (rv < 0) {
                    goto ERROR;
                }
                if (memcmp(cert_id, key_id, cert_id_size) != 0) {
                    /* they don't match - skip the certificate */
                    gnutls_x509_crt_deinit(cert);
                    cert = NULL;
                }
                break;

              case GNUTLS_BAG_CRL:
              case GNUTLS_BAG_ENCRYPTED:
              case GNUTLS_BAG_EMPTY:
              default:
                break;
            }
        }

        gnutls_pkcs12_bag_deinit(bag);
    }

    bag = NULL;

    assert(key);
    assert(cert);

    /* Put cert and key onto the credential */
    rv = gnutls_certificate_set_x509_key(cred, &cert, 1, key);
    if (rv < 0) {
        goto ERROR;
    }

    /* Check activation/expire times on the certifcate */
    rv = check_cert_times(cert, "PKCS#12", cert_filename);
    if (rv) {
        goto END;
    }

    rv = 0;

    /* Use "goto ERROR" to print a GnuTLS error; "goto END" otherwise */
  ERROR:
    if (rv != 0) {
        ERRMSG("Error getting PKCS#12 certificate from file '%s': %s",
               cert_filename, gnutls_strerror(rv));
    }
  END:
    skStreamDestroy(&stream);
    if (bag) {
        gnutls_pkcs12_bag_deinit(bag);
    }
    if (p12) {
        gnutls_pkcs12_deinit(p12);
    }
    if (cert) {
        gnutls_x509_crt_deinit(cert);
    }
    if (key) {
        gnutls_x509_privkey_deinit(key);
    }
    RETURN((0 == rv) ? 0 : -1);
}

#endif  /* SK_ENABLE_GNUTLS */


/*** TCP functions ***/

/*
 *    Implementation of sk_msg_transport_fn_t.send().  Use standard
 *    TCP functions to write a message.
 */
static int
tcp_send(
    sk_msg_conn_queue_t    *conn,
    sk_msg_write_buf_t     *write_buf)
{
    uint8_t *seg_base = NULL;
    sk_msg_t *msg;
    int retval = 0;
    ssize_t rv;

    DEBUG_ENTER_FUNC;

    assert(write_buf);
    msg = write_buf->msg;

    assert(msg);
    assert(conn);
    assert(msg->segments);
    assert(msg->segment[0].iov_base == &msg->hdr);
    assert(msg->segment[0].iov_len  == sizeof(msg->hdr));

    DEBUG_PRINT3("Sending chan=%#x type=%#x",
                 ntohs(msg->hdr.channel), ntohs(msg->hdr.type));

    if (write_buf->seg_offset) {
        /* there is a partially written segment; move iov_base to
         * start of unwritten data and adjust length of segment  */
        assert(msg->segment[write_buf->cur_seg].iov_len
               > write_buf->seg_offset);
        seg_base = (uint8_t*)msg->segment[write_buf->cur_seg].iov_base;
        msg->segment[write_buf->cur_seg].iov_base
            = seg_base + write_buf->seg_offset;
        msg->segment[write_buf->cur_seg].iov_len -= write_buf->seg_offset;
    }

    /* Write the message */
    while ((rv = writev(conn->wsocket, msg->segment + write_buf->cur_seg,
                        msg->segments - write_buf->cur_seg))
           != write_buf->msg_size)
    {
        if (rv == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN) {
                DEBUG_PRINT1("send: writev returned EAGAIN");
                retval = SKMERR_PARTIAL;
                break;
            }
            if (errno == EPIPE || errno == ECONNRESET) {
                DEBUG_PRINT3("send: Connection closed due to %d [%s]",
                             errno, strerror(errno));
                retval = SKMERR_CLOSED;
                break;
            }
            DEBUG_PRINT3("send: System error %d [%s]", errno, strerror(errno));
            retval = SKMERR_ERRNO;
            break;
        }
        if (rv == 0) {
            DEBUG_PRINT1("send: Connection closed due to write returning 0");
            retval = SKMERR_CLOSED;
            break;
        }
        assert(rv < write_buf->msg_size);
        DEBUG_PRINT3("send: Handling short write (%" SK_PRIdZ "/%" SK_PRIdZ ")",
                     rv, write_buf->msg_size);
        retval = SKMERR_PARTIAL;
        write_buf->msg_size -= rv;
        if (seg_base) {
            msg->segment[write_buf->cur_seg].iov_base = seg_base;
            seg_base = NULL;
            if (msg->segment[write_buf->cur_seg].iov_len > (size_t)rv) {
                /* did not complete writing this segment */
                msg->segment[write_buf->cur_seg].iov_len
                    += write_buf->seg_offset;
                write_buf->seg_offset += rv;
                break;
            }
            /* finished this segment */
            rv -= msg->segment[write_buf->cur_seg].iov_len;
            msg->segment[write_buf->cur_seg].iov_len += write_buf->seg_offset;
            ++write_buf->cur_seg;
        }
        while (rv > 0
               && msg->segment[write_buf->cur_seg].iov_len <= (size_t)rv)
        {
            rv -= msg->segment[write_buf->cur_seg].iov_len;
            ++write_buf->cur_seg;
        }
        /* partial write for this segment */
        write_buf->seg_offset = rv;
        break;
    }

    if (seg_base) {
        msg->segment[write_buf->cur_seg].iov_base = seg_base;
        msg->segment[write_buf->cur_seg].iov_len += write_buf->seg_offset;
    }

    RETURN(retval);
}


/*
 *    Implementation of sk_msg_transport_fn_t.recv().  Use standard
 *    TCP functions to read a message.
 */
static int
tcp_recv(
    sk_msg_conn_queue_t    *conn,
    sk_msg_t              **message)
{
    ssize_t             rv;
    sk_msg_read_buf_t  *buffer;
    int                 retval;
    int                 new_msg;

    DEBUG_ENTER_FUNC;

    assert(message);
    assert(conn);

    buffer = &conn->msg_read_buf;
    new_msg = (buffer->msg == NULL);
    if (new_msg) {
        /* Starting to read a new message. */

        sk_msg_hdr_t    *hdr;
        sk_msg_t        *msg;
        uint8_t         *hdr_buf;
        ssize_t          hdr_len;

        /* Create a message structure */
        buffer->msg = (sk_msg_t*)malloc(sizeof(sk_msg_t)
                                        + sizeof(struct iovec));
        msg = buffer->msg;
        MEM_ASSERT(msg != NULL);
        msg->segments = 1;
        msg->simple_free = NULL;
        msg->free_fn = msg_simple_free;
        hdr = &msg->hdr;
        msg->segment[0].iov_base = hdr;
        msg->segment[0].iov_len = sizeof(*hdr);
        memset(hdr, 0, sizeof(*hdr));

        /* Read a header */
        hdr_buf = ((uint8_t*)hdr);
        hdr_len = (ssize_t)sizeof(*hdr);
        while ((rv = read(conn->rsocket, hdr_buf, hdr_len))
               != hdr_len)
        {
            /* Did not get all of the data we expected to get */
            if (rv > 0) {
                /* Partial read, reduce number of expected bytes and
                 * try again. */
                DEBUG_PRINT3("recv: Partial read of header; trying again"
                             " (%" SK_PRIdZ "/%" SK_PRIuZ ")",
                             rv, hdr_len);
                hdr_buf += rv;
                hdr_len -= rv;
                continue;
            }
            if (rv == -1) {
                /* Error.  Close connection unless interrupt. */
                if (errno == EINTR) {
                    continue;
                }
                if (errno != EAGAIN) {
                    DEBUG_PRINT3("recv: System error %d [%s]",
                                 errno, strerror(errno));
                    retval = SKMERR_ERRNO;
                } else if (sizeof(*hdr) == (size_t)hdr_len) {
                    /* Handle EAGAIN on a completely unread header
                     * specially. This can happen if poll() says data
                     * is available when it actually is not, which can
                     * happen on Linux ("spurious readiness") */
                    DEBUG_PRINT1("recv: EAGAIN on unread header");
                    retval = SKMERR_EMPTY;
                } else {
                    /* Handle EAGAIN after we have read part of the
                     * header as a short read error. */
                    DEBUG_PRINT3("recv: Short read (%" SK_PRIdZ
                                 "/%" SK_PRIuZ ") [EAGAIN]",
                                 (sizeof(*hdr) - hdr_len), sizeof(*hdr));
                    retval = SKMERR_SHORT;
                }
            } else if (sizeof(*hdr) == (size_t)hdr_len) {
                /* This read() returned 0, and we do not have any of
                 * the header; assume connection is closed. */
                DEBUG_PRINT1("recv: Connection closed due to read returning 0");
                retval = SKMERR_CLOSED;
            } else {
                /* This read() returned 0, but we got part of the
                 * header on a previous read(); treat as error. */
                DEBUG_PRINT3("recv: Short read (%" SK_PRIdZ "/%" SK_PRIuZ ")",
                             (sizeof(*hdr) - hdr_len), sizeof(*hdr));
                retval = SKMERR_SHORT;
            }
            goto error;
        }

        /* Convert network byte order to host byte order */
        hdr->channel = ntohs(hdr->channel);
        hdr->type    = ntohs(hdr->type);
        hdr->size    = ntohs(hdr->size);

        DEBUG_PRINT4("Receiving chan=%#x type=%#x size=%d",
                     hdr->channel, hdr->type, hdr->size);

        if (hdr->size == 0) {
            /* If the size is zero, the message is complete */
            *message = msg;
            buffer->msg = NULL;
            RETURN(0);
        }
        /* Allocate space for the body of the message */
        msg->segment[1].iov_base = malloc(hdr->size);
        MEM_ASSERT(msg->segment[1].iov_base);
        msg->segment[1].iov_len = hdr->size;
        msg->segments++;

        /* Maintain state for re-entrant call */
        buffer->count = hdr->size;
        buffer->loc = (uint8_t*)msg->segment[1].iov_base;

        /* Fall through to read the rest of the message.  NOTE: Do not
         * fail if the next read() returns 0, since maybe the header
         * was the only thing available to read. */
    }
    /* else, the previous call to recv was a partial read of the
     * message. */

    assert(buffer->count);
    rv = read(conn->rsocket, buffer->loc, buffer->count);
    if (rv == -1) {
        if (errno == EINTR || errno == EAGAIN) {
            DEBUG_PRINT3("Failed to read %u bytes; return PARTIAL [%s]",
                         buffer->count, strerror(errno));
            RETURN(SKMERR_PARTIAL);
        }
        DEBUG_PRINT3("Failed to read %u bytes; return ERRNO [%s]",
                     buffer->count, strerror(errno));
        retval = SKMERR_ERRNO;
        goto error;
    } else if (rv == 0 && !new_msg) {
        /* Fail: poll() said data was available, and this first
         * attempt to read() data returned 0 bytes. */
        DEBUG_PRINT2("Failed to read %u bytes; return CLOSED [EOF]",
                     buffer->count);
        retval = SKMERR_CLOSED;
        goto error;
    }

    buffer->count -= rv;
    buffer->loc   += rv;

    if (buffer->count != 0) {
        /* We've read the available data but the message is not yet
         * complete; return to reader_thread() and call poll()
         * again */
        DEBUG_PRINT2("PARTIAL message, %u bytes remaining", buffer->count);
        RETURN(SKMERR_PARTIAL);
    }
    /* else, message is complete */

    *message = buffer->msg;
    buffer->msg = NULL;
    RETURN(0);

  error:
    if (buffer->msg) {
        skMsgDestroy(buffer->msg);
        buffer->msg = NULL;
    }
    RETURN(retval);
}


#if SK_ENABLE_GNUTLS

/*** TLS functions ***/

/*
 *    Implementation of sk_msg_transport_fn_t.send().  Use GnuTLS to
 *    write a message.
 */
static int
tls_send(
    sk_msg_conn_queue_t    *conn,
    sk_msg_write_buf_t     *write_buf)
{
    sk_msg_t *msg;
    ssize_t rv;
    uint16_t i;

    DEBUG_ENTER_FUNC;

    assert(write_buf);
    msg = write_buf->msg;

    assert(msg);
    assert(conn);
    assert(conn->use_tls);
    assert(msg->segments);
    assert(msg->segment[0].iov_base == &msg->hdr);
    assert(msg->segment[0].iov_len  == sizeof(msg->hdr));

    DEBUG_PRINT3("Sending chan=%#x type=%#x",
                 ntohs(msg->hdr.channel), ntohs(msg->hdr.type));

    for (i = 0; i < msg->segments; i++) {
        size_t remaining = msg->segment[i].iov_len;
        uint8_t *loc = (uint8_t*)msg->segment[i].iov_base;

        /* Write the message */
        while (remaining) {
            DEBUG_PRINT2("calling gnutls_record_send (%d)",
                         (skm_len_t)msg->segment[i].iov_len);
            rv = gnutls_record_send(conn->session, loc, remaining);
            DEBUG_PRINT2("gnutls_record_send -> %" SK_PRIdZ, rv);
            if (rv < 0) {
                if (rv == GNUTLS_E_INTERRUPTED || rv == GNUTLS_E_AGAIN) {
                    continue;
                }
                if (rv == GNUTLS_E_EXPIRED || rv == GNUTLS_E_INTERRUPTED ||
                    (rv == GNUTLS_E_PUSH_ERROR &&
                     (errno == EPIPE || errno == ECONNRESET)))
                {
                    RETURN(SKMERR_CLOSED);
                }
                RETURN(SKMERR_ERRNO);
            } else if (rv == 0) {
                DEBUG_PRINT1("send: Connection closed"
                             " due to write returning 0");
                RETURN(SKMERR_CLOSED);
            } else {
                remaining -= rv;
                loc += rv;
            }
        }
    }

    RETURN(0);
}


/*
 *    Implementation of sk_msg_transport_fn_t.recv().  Use GnuTLS to
 *    read a message.
 */
static int
tls_recv(
    sk_msg_conn_queue_t    *conn,
    sk_msg_t              **message)
{
    ssize_t             rv;
    sk_msg_read_buf_t  *buffer;
    int                 retval;
    int                 new_msg;

    DEBUG_ENTER_FUNC;

    assert(message);
    assert(conn);

    buffer = &conn->msg_read_buf;
    new_msg = (buffer->msg == NULL);
    if (new_msg) {
        /* Starting to read a new message */

        sk_msg_hdr_t    *hdr;
        sk_msg_t        *msg;

        /* Create a message structure */
        buffer->msg = (sk_msg_t*)malloc(sizeof(sk_msg_t)
                                        + sizeof(struct iovec));
        msg = buffer->msg;
        MEM_ASSERT(msg != NULL);
        msg->segments = 1;
        msg->simple_free = NULL;
        msg->free_fn = msg_simple_free;
        hdr = &msg->hdr;
        msg->segment[0].iov_base = hdr;
        msg->segment[0].iov_len = sizeof(*hdr);
        memset(hdr, 0, sizeof(*hdr));

        /* Read a header.  This code assumes we can read the entire
         * header in one call to gnutls_record_recv(). */
        DEBUG_PRINT2("calling gnutls_record_recv (%" SK_PRIuZ ")",
                     sizeof(*hdr));
        while ((rv = gnutls_record_recv(conn->session, hdr, sizeof(*hdr)))
               != sizeof(*hdr))
        {
            DEBUG_PRINT2("gnutls_record_recv -> %" SK_PRIdZ, rv);
            if (rv < 0) {
                if (rv == GNUTLS_E_INTERRUPTED || rv == GNUTLS_E_AGAIN) {
                    DEBUG_PRINT2("calling gnutls_record_recv (%" PRIu64 ")",
                                 (uint64_t)sizeof(*hdr));
                    continue;
                }
                retval = SKMERR_ERRNO;
            } else if (rv == 0) {
                DEBUG_PRINT1("recv: Connection closed due to read returning 0");
                retval = SKMERR_CLOSED;
            } else {
                DEBUG_PRINT3("recv: Short read (%" SK_PRIdZ "/%" SK_PRIuZ ")",
                             rv, sizeof(*hdr));
                retval = SKMERR_SHORT;
            }
            goto error;
        }
        DEBUG_PRINT2("gnutls_record_recv -> %" SK_PRIdZ, rv);

        /* Convert network byte order to host byte order */
        hdr->channel = ntohs(hdr->channel);
        hdr->type    = ntohs(hdr->type);
        hdr->size    = ntohs(hdr->size);

        DEBUG_PRINT4("Receiving chan=%#x type=%#x size=%d",
                     hdr->channel, hdr->type, hdr->size);

        if (hdr->size == 0) {
            /* If the size is zero, the message is complete */
            *message = msg;
            buffer->msg = NULL;
            RETURN(0);
        }
        /* Allocate space for the body of the message */
        msg->segment[1].iov_base = malloc(hdr->size);
        MEM_ASSERT(msg->segment[1].iov_base);
        msg->segment[1].iov_len = hdr->size;
        msg->segments++;

        /* Maintain state for re-entrant call */
        buffer->count = hdr->size;
        buffer->loc = (uint8_t*)msg->segment[1].iov_base;

        /* Fall through to read the rest of the message.  NOTE: Do not
         * fail if the next gnutls_record_recv() returns 0, since
         * maybe the header was the only thing available. */
    }
    /* else, the previous call to recv was a partial read of the
     * message. */

    assert(buffer->count);
    DEBUG_PRINT2("calling gnutls_record_recv (%d)", buffer->count);
    while ((rv = gnutls_record_recv(conn->session, buffer->loc, buffer->count))
           < 0)
    {
        DEBUG_PRINT2("gnutls_record_recv -> %" SK_PRIdZ, rv);
        if (rv == GNUTLS_E_INTERRUPTED || rv == GNUTLS_E_AGAIN) {
            DEBUG_PRINT2("calling gnutls_record_recv (%d)", buffer->count);
            continue;
        }
        DEBUG_PRINT2("read failure: [%s]", gnutls_strerror(rv));
        retval = SKMERR_ERRNO;
        goto error;
    }
    DEBUG_PRINT2("gnutls_record_recv -> %" SK_PRIdZ, rv);

    if (rv == 0 && !new_msg) {
        /* Fail: poll() said data was available, but this first
         * attempt to use gnutls_record_recv() returned 0 bytes. */
        DEBUG_PRINT1("read ended: [EOF]");
        retval = SKMERR_CLOSED;
        goto error;
    }

    buffer->count -= rv;
    buffer->loc   += rv;

    if (buffer->count != 0) {
        /* we've read the available data but the message is not yet
         * complete; return to reader_thread() and call poll()
         * again */
        RETURN(SKMERR_PARTIAL);
    }
    /* else, message is complete */

    *message = buffer->msg;
    buffer->msg = NULL;
    RETURN(0);

  error:
    if (buffer->msg) {
        skMsgDestroy(buffer->msg);
        buffer->msg = NULL;
    }
    RETURN(retval);
}

#endif /* SK_ENABLE_GNUTLS */


/***********************************************************************/


#if SK_ENABLE_GNUTLS

/*
 *    Initialize GnuTLS.  The caller should have the
 *    sk_msg_gnutls_mutex before calling this function.
 */
static int
skMsgGnuTLSInit(
    void)
{
    DEBUG_ENTER_FUNC;

    if (!sk_msg_gnutls_initialized) {
        int rv;

#if GNUTLS_VERSION_NUMBER <= 0x020b00
        rv = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
        if (rv == 0)
#endif
        {
            rv = gnutls_global_init();
        }
        if (rv >= 0) {
            rv = gnutls_dh_params_init(&dh_params);
        }
        if (rv >= 0) {
            INFOMSG("Generating Diffie-Hellman parameters...");
            INFOMSG("This could take some time...");
            rv = gnutls_dh_params_generate2(dh_params, DH_BITS);
            INFOMSG("Finished generating Diffie-Hellman parameters");
        }
        if (rv >= 0) {
            sk_msg_gnutls_initialized = 1;
        }
        RETURN(rv);

    }
    RETURN(0);
}


void
skMsgGnuTLSTeardown(
    void)
{
    DEBUG_ENTER_FUNC;

    pthread_mutex_lock(&sk_msg_gnutls_mutex);

    if (sk_msg_gnutls_initialized) {
        gnutls_global_deinit();

        sk_msg_gnutls_initialized = 0;
    }

    pthread_mutex_unlock(&sk_msg_gnutls_mutex);
    RETURN_VOID;
}


int
skMsgQueueAddCA(
    sk_msg_queue_t     *queue,
    const char         *ca_filename)
{
    int rv;

    DEBUG_ENTER_FUNC;

    assert(queue);
    assert(queue->root);
    assert(ca_filename);

    pthread_mutex_lock(&sk_msg_gnutls_mutex);
    if (!sk_msg_gnutls_initialized) {
        rv = skMsgGnuTLSInit();
        if (rv != 0) {
            goto END;
        }
    }
    /* allocate credentials and set Diffie-Hellman parameters */
    if (!queue->root->cred_set) {
        rv = gnutls_certificate_allocate_credentials(&queue->root->cred);
        if (rv < 0) {
            INFOMSG("Unable to allocate credentials: %s",
                    gnutls_strerror(rv));
            goto END;
        }
        gnutls_certificate_set_dh_params(queue->root->cred, dh_params);
    }

    /* add the trusted CAs from 'ca_filename'; the file should
     * be in PEM format. */
    rv = read_trust_file(queue->root->cred, ca_filename, GNUTLS_X509_FMT_PEM);
    if (rv < 0) {
        goto END;
    }

    if (!queue->root->cred_set) {
        queue->root->cred_set = 1;
        sk_msg_gnutls_initialized++;
    }
    rv = 0;

  END:
    if (0 != rv) {
        rv = -1;
        if (queue->root->cred_set) {
            gnutls_certificate_free_credentials(queue->root->cred);
            queue->root->cred_set = 0;
        }
    }
    pthread_mutex_unlock(&sk_msg_gnutls_mutex);
    RETURN(rv);
}

int
skMsgQueueAddCert(
    sk_msg_queue_t     *queue,
    const char         *cert_filename,
    const char         *key_filename)
{
    int rv;

    assert(queue);
    assert(queue->root);
    assert(cert_filename);
    assert(key_filename);

    DEBUG_ENTER_FUNC;

    pthread_mutex_lock(&sk_msg_gnutls_mutex);
    if (!sk_msg_gnutls_initialized) {
        rv = skMsgGnuTLSInit();
        if (rv != 0) {
            goto END;
        }
    }

    /* allocate credentials and set Diffie-Hellman parameters */
    if (!queue->root->cred_set) {
        rv = gnutls_certificate_allocate_credentials(&queue->root->cred);
        if (rv < 0) {
            INFOMSG("Unable to allocate credentials: %s",
                    gnutls_strerror(rv));
            goto END;
        }
        gnutls_certificate_set_dh_params(queue->root->cred, dh_params);
    }

    /* set a certificate/private-key pair in the credential from
     * separate certificate and key files, each in PEM format. */
    rv = read_key_file(queue->root->cred, cert_filename,
                       key_filename, GNUTLS_X509_FMT_PEM);
    if (rv < 0) {
        goto END;
    }

    if (!queue->root->cred_set) {
        queue->root->cred_set = 1;
        sk_msg_gnutls_initialized++;
    }

    rv = 0;

  END:
    if (0 != rv) {
        rv = -1;
        if (queue->root->cred_set) {
            gnutls_certificate_free_credentials(queue->root->cred);
            queue->root->cred_set = 0;
        }
    }
    pthread_mutex_unlock(&sk_msg_gnutls_mutex);
    RETURN(rv);
}


int
skMsgQueueAddPKCS12(
    sk_msg_queue_t     *queue,
    const char         *cert_filename,
    const char         *password)
{
    int rv;

    assert(queue);
    assert(queue->root);
    assert(cert_filename);

    DEBUG_ENTER_FUNC;

    pthread_mutex_lock(&sk_msg_gnutls_mutex);
    if (!sk_msg_gnutls_initialized) {
        rv = skMsgGnuTLSInit();
        if (rv != 0) {
            goto END;
        }
    }

    /* allocate credentials and set Diffie-Hellman parameters */
    if (!queue->root->cred_set) {
        rv = gnutls_certificate_allocate_credentials(&queue->root->cred);
        if (rv < 0) {
            INFOMSG("Unable to allocate credentials: %s",
                    gnutls_strerror(rv));
            goto END;
        }
        gnutls_certificate_set_dh_params(queue->root->cred, dh_params);
    }

    rv = read_check_pkcs12(queue->root->cred, cert_filename,
                           GNUTLS_X509_FMT_DER, password);
    if (rv < 0) {
        goto END;
    }

    if (!queue->root->cred_set) {
        queue->root->cred_set = 1;
        sk_msg_gnutls_initialized++;
    }

    rv = 0;

  END:
    if (0 != rv) {
        rv = -1;
        if (queue->root->cred_set) {
            gnutls_certificate_free_credentials(queue->root->cred);
            queue->root->cred_set = 0;
        }
    }
    pthread_mutex_unlock(&sk_msg_gnutls_mutex);
    RETURN(rv);
}

#endif /* SK_ENABLE_GNUTLS */


/* Create a channel within a message queue. */
static sk_msg_channel_queue_t *
create_channel(
    sk_msg_queue_t     *q)
{
    sk_msg_channel_queue_t *chan;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    ASSERT_QUEUE_LOCK(q);

    /* Allocate space for a new channel */
    chan = (sk_msg_channel_queue_t *)calloc(1, sizeof(*chan));
    MEM_ASSERT(chan != NULL);

    chan->queue = mqCreateQueue(q->group);
    MEM_ASSERT(chan->queue != NULL);

    /* Assign a local channel number and add the channel to the
       message queue */
    do {
        chan->channel = q->root->next_channel++;
        rv = int_dict_set(q->root->channel, chan->channel, &chan);
    } while (rv == 1);
    MEM_ASSERT(rv == 0);

    /* Channel is created, rchannel is unset */
    chan->state = SKM_CREATED;
    chan->rchannel = SKMSG_CHANNEL_CONTROL;
    rv = pthread_cond_init(&chan->pending, NULL);
    XASSERT(rv == 0);
    chan->is_pending = 0;

    rv = int_dict_set(q->root->groups, chan->channel, &q);
    MEM_ASSERT(rv == 0);
    rv = int_dict_set(q->channel, chan->channel, &chan);
    MEM_ASSERT(rv == 0);
    chan->group = q;

    DEBUG_PRINT2("create_channel() = %#x", chan->channel);

    RETURN(chan);
}


/* Attach a channel to a connection object. */
static int
set_channel_connecting(
    sk_msg_queue_t          UNUSED(*q),
    sk_msg_channel_queue_t         *chan,
    sk_msg_conn_queue_t            *conn)
{
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(chan);
    assert(conn);
    ASSERT_QUEUE_LOCK(q);
    assert(chan->state == SKM_CREATED);
    assert(conn->state != SKM_CLOSED);

    DEBUG_PRINT2("set_channel_connecting(%#x)", chan->channel);

    /* Set the channel's communication stream, set it to
       half-connected, and up the refcount on the connection
       object. */
    chan->conn = conn;
    chan->state = SKM_CONNECTING;

    /* Add an entry in the connections's channel map for the
       channel. */
    rv = int_dict_set(conn->channelmap, chan->channel, &chan);
    MEM_ASSERT(rv != -1);
    assert(rv == 0);

    conn->state = SKM_CONNECTED;
    conn->refcount++;

    RETURN(0);
}


/* Return 1 if the connection was destroyed, zero otherwise */
static int
set_channel_closed(
    sk_msg_queue_t         *q,
    sk_msg_channel_queue_t *chan,
    int                     no_destroy)
{
    sk_msg_conn_queue_t *conn;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(chan);
    ASSERT_QUEUE_LOCK(q);
    assert(chan->conn);
    if (chan->state == SKM_CLOSED) {
        RETURN(0);
    }
    assert(chan->conn->refcount > 0);
    assert(chan->state == SKM_CONNECTING ||
           chan->state == SKM_CONNECTED);

    DEBUG_PRINT2("set_channel_closed(%#x)", chan->channel);

    conn = chan->conn;

    if (chan->state == SKM_CONNECTED &&
        chan->channel != SKMSG_CHANNEL_CONTROL)
    {
        skm_channel_t lchannel = htons(chan->channel);
        DEBUG_PRINT1("Sending SKMSG_CTL_CHANNEL_DIED (Internal)");
        send_message(q, SKMSG_CHANNEL_CONTROL,
                     SKMSG_CTL_CHANNEL_DIED,
                     &lchannel, sizeof(lchannel), SKM_SEND_INTERNAL,
                     0, NULL);
    }

    ASSERT_RESULT(int_dict_del(conn->channelmap, chan->channel),
                  int, 0);

    chan->state = SKM_CLOSED;
    conn->refcount--;

    /* Notify people waiting on this channel to complete connecting
       that it is dead. */
    MUTEX_BROADCAST(&chan->pending);

    if (conn->refcount == 0 && !no_destroy) {
        RETURN(destroy_connection(q, conn));
    }

    RETURN(0);
}


static int
set_channel_connected(
    sk_msg_queue_t          UNUSED(*q),
    sk_msg_channel_queue_t         *chan,
    skm_channel_t                   rchannel)
{
    DEBUG_ENTER_FUNC;

    assert(q);
    assert(chan);
    ASSERT_QUEUE_LOCK(q);
    assert(chan->state == SKM_CONNECTING);

    DEBUG_PRINT2("set_channel_connected(%#x)", chan->channel);

    chan->rchannel = rchannel;

    chan->state = SKM_CONNECTED;
    RETURN(0);
}


static void
destroy_channel(
    sk_msg_queue_t         *q,
    sk_msg_channel_queue_t *chan)
{
    DEBUG_ENTER_FUNC;

    assert(q);
    assert(chan);
    ASSERT_QUEUE_LOCK(q);

    DEBUG_PRINT2("destroy_channel(%#x)", chan->channel);

    if (chan->state == SKM_CONNECTED &&
        chan->channel != SKMSG_CHANNEL_CONTROL)
    {
        skm_channel_t rchannel = htons(chan->rchannel);
        DEBUG_PRINT1("Sending SKMSG_CTL_CHANNEL_KILL (Ext-control)");
        send_message(q, chan->channel, SKMSG_CTL_CHANNEL_KILL,
                     &rchannel, sizeof(rchannel), SKM_SEND_CONTROL,
                     0, NULL);
    }
    if (chan->state == SKM_CONNECTED || chan->state == SKM_CONNECTING) {
        set_channel_closed(q, chan, 0);
    }

    assert(chan->state == SKM_CLOSED);

    ASSERT_RESULT(int_dict_del(q->root->channel, chan->channel), int, 0);

    ASSERT_RESULT(int_dict_del(q->root->groups, chan->channel), int, 0);
    ASSERT_RESULT(int_dict_del(chan->group->channel, chan->channel), int, 0);

    ASSERT_RESULT(pthread_cond_destroy(&chan->pending), int, 0);

    /* Disable adding to the queue (it will be destroyed when the
       group is destroyed) */
    mqQueueDisable(chan->queue, MQ_ADD);

    free(chan);

    RETURN_VOID;
}


#if SK_ENABLE_GNUTLS
static ssize_t
tls_pull(
    gnutls_transport_ptr_t  fd,
    void                   *buf,
    size_t                  len)
{
    struct pollfd pfd;
    int           rv;
    ssize_t       rlen;

    pfd.fd = (intptr_t)fd;
    pfd.events = POLLIN;

    rv = poll(&pfd, 1, TLS_POLL_TIMEOUT);
    if (rv != 1) {
        /* What should happen if rv == 0 (timed out?)  Currently we
         * just return 0, as if there was a zero read.  Possibly we
         * should return -1 with an errno of EAGAIN. */
        return rv;
    }
    if (pfd.revents & (POLLERR | POLLNVAL | POLLHUP)) {
        /* Log but ignore poll error events for now (allow the read to
         * fail.  */
        DEBUG_PRINT2("Poll returned %s", SK_POLL_EVENT_STR(pfd.revents));
    }

    rlen = read(pfd.fd, buf, len);

    return rlen;
}


static ssize_t
tls_push(
    gnutls_transport_ptr_t  fd,
    const void             *buf,
    size_t                  len)
{
    struct pollfd pfd;
    int           rv;
    ssize_t       wlen;

    pfd.fd = (intptr_t)fd;
    pfd.events = POLLOUT;

    rv = poll(&pfd, 1, TLS_POLL_TIMEOUT);
    if (rv != 1) {
        /* What should happen if rv == 0 (timed out?)  Currently we
         * just return 0, as if there was a zero write.  Possibly we
         * should return -1 with an errno of EAGAIN. */
        return rv;
    }
    if (pfd.revents & (POLLERR | POLLNVAL | POLLHUP)) {
        /* Log but ignore poll error events for now (allow the write
         * to fail.  */
        DEBUG_PRINT2("Poll returned %s", SK_POLL_EVENT_STR(pfd.revents));
    }

    wlen = write(pfd.fd, buf, len);

    return wlen;
}


static int
setup_tls(
    sk_msg_queue_t         *q,
    sk_msg_conn_queue_t    *conn,
    int                     rsocket,
    int                     wsocket,
    skm_tls_type_t          tls)
{
    int rv;
    unsigned int status;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(conn);
    assert(q->root->cred_set);

    /* initialize the TLS session object depending on whether it is
     * the client or the server */
    switch (tls) {
      case SKM_TLS_CLIENT:
        rv = gnutls_init(&conn->session, GNUTLS_CLIENT);
        break;
      case SKM_TLS_SERVER:
        rv = gnutls_init(&conn->session, GNUTLS_SERVER);
        break;
      default:
        skAbortBadCase(tls);
    }

    if (rv < 0) {
        ERRMSG("Failed TLS init: %s", gnutls_strerror(rv));
        RETURN(-1);
    }

    /* use "NORMAL" priority */
    rv = gnutls_set_default_priority(conn->session);
    XASSERT(rv >= 0);

    /* tell the session to use the public/private keys loaded earlier */
    rv = gnutls_credentials_set(conn->session, GNUTLS_CRD_CERTIFICATE,
                                q->root->cred);
    XASSERT(rv >= 0);

    /* tell TLS to use our read and write functions (tls_pull,
     * tls_push) instead of the defaults.  The call to set_ptr2()
     * controls what will be passed to tls_pull() and tls_push(). */
    gnutls_transport_set_ptr2(conn->session,
                              (gnutls_transport_ptr_t)(intptr_t)rsocket,
                              (gnutls_transport_ptr_t)(intptr_t)wsocket);
    gnutls_transport_set_pull_function(conn->session, tls_pull);
    gnutls_transport_set_push_function(conn->session, tls_push);
#ifdef SK_HAVE_GNUTLS_TRANSPORT_SET_LOWAT
    /* Used to set the low water value in order for select() to check
     * if there are data pending to be read from the socket buffer.
     * This function no longer exists in GnuTLS 3. */
    gnutls_transport_set_lowat(conn->session, 0);
#endif
    set_nonblock(rsocket);

    /* force the client to send its certificate to the server */
    if (tls == SKM_TLS_SERVER) {
        gnutls_certificate_server_set_request(conn->session,
                                              GNUTLS_CERT_REQUIRE);
    }

    DEBUG_PRINT1("Attempting TLS handshake");
    while ((rv = gnutls_handshake(conn->session)) < 0) {
        if (rv == GNUTLS_E_AGAIN || rv == GNUTLS_E_INTERRUPTED) {
            DEBUG_PRINT1("Received AGAIN/INTERRUPTED;"
                         " Re-attempting TLS handshake");
            continue;
        }
        if (rv == GNUTLS_E_PUSH_ERROR) {
            NOTICEMSG("Remote side disconnected during TLS handshake.");
        } else {
            NOTICEMSG("TLS handshake failed: %s", gnutls_strerror(rv));
        }
        gnutls_deinit(conn->session);
        RETURN(-1);
    }
    DEBUG_PRINT1("TLS handshake succeeded");

    status = 0;
    rv = gnutls_certificate_verify_peers2(conn->session, &status);
    if (rv < 0) {
        const char *reason = "Unknown reason";

        NOTICEMSG("Certificate verification failed: %s",
                  gnutls_strerror(rv));

        if (status & GNUTLS_CERT_REVOKED) {
            reason = "Certificate was revoked";
        } else if (status & GNUTLS_CERT_INVALID) {
            if (status & GNUTLS_CERT_SIGNER_NOT_FOUND) {
                reason = "Certificate issuer unknown";
            } else if (status & GNUTLS_CERT_SIGNER_NOT_CA) {
                reason = "Certificate signer is not a CA";
            } else if (status & GNUTLS_CERT_INSECURE_ALGORITHM) {
                reason = "Insecure algorithm";
#if defined SK_HAVE_DECL_GNUTLS_CERT_EXPIRED && SK_HAVE_DECL_GNUTLS_CERT_EXPIRED
            } else if (status & GNUTLS_CERT_NOT_ACTIVATED) {
                reason = "Certificate is not yet activated";
            } else if (status & GNUTLS_CERT_EXPIRED) {
                reason = "Certificate has expired";
#endif
            }
        }

        NOTICEMSG("Certificate verification failed: %s", reason);

        /* teardown connection */
        do {
            rv = gnutls_bye(conn->session, GNUTLS_SHUT_RDWR);
        } while (rv == GNUTLS_E_AGAIN || rv == GNUTLS_E_INTERRUPTED);
        gnutls_deinit(conn->session);
        RETURN(-1);
    }

    conn->use_tls = 1;

    RETURN(0);
}
#endif /* SK_ENABLE_GNUTLS */


static int
create_connection(
    sk_msg_queue_t         *q,
    int                     rsocket,
    int                     wsocket,
    struct sockaddr        *addr,
    socklen_t               addrlen,
    sk_msg_conn_queue_t   **rconn,
    skm_tls_type_t          tls)
{
    sk_msg_conn_queue_t *conn;
    sk_queue_and_conn_t *qac;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(rconn);
    ASSERT_QUEUE_LOCK(q);

    DEBUG_PRINT3("create_connection() = %d, %d", rsocket, wsocket);

    /* Allocate space for the connection */
    conn = (sk_msg_conn_queue_t *)calloc(1, sizeof(*conn));
    MEM_ASSERT(conn != NULL);

#if SK_ENABLE_GNUTLS
    if (tls != SKM_TLS_NONE) {
        rv = setup_tls(q, conn, rsocket, wsocket, tls);
        if (rv != 0) {
            free(conn);
            RETURN(-1);
        }
    }
#endif /* SK_ENABLE_GNUTLS */

    /* Set the read and write sockets */
    conn->rsocket = rsocket;
    conn->wsocket = wsocket;

    /* And the address */
    conn->addr = addr;
    conn->addrlen = addrlen;

    /* Set the transport functions */
#if SK_ENABLE_GNUTLS
    if (tls != SKM_TLS_NONE) {
        conn->fn.send = tls_send;
        conn->fn.recv = tls_recv;
    } else
#endif
    {
        conn->fn.send = tcp_send;
        conn->fn.recv = tcp_recv;
    }
    conn->transport = (tls == SKM_TLS_NONE) ? CONN_TCP : CONN_TLS;

    /* Set up the channel queue and refcount */
    conn->channelmap = int_dict_create(sizeof(sk_msg_channel_queue_t *));
    MEM_ASSERT(conn->channelmap != NULL);
    conn->refcount = 0;

    /* Set the connections initial state */
    conn->state = SKM_CREATED;

    /* Set up the write queue */
    conn->queue = skDequeCreate();
    XASSERT(conn->queue != NULL);

    /* Initialize the writer thread start state */
    pthread_cond_init(&conn->writer_cond, NULL);
    conn->writer_state = SKM_THREAD_BEFORE;

    /* Initialize the reader thread start state */
    pthread_cond_init(&conn->reader_cond, NULL);
    conn->reader_state = SKM_THREAD_BEFORE;

    /* Set up and start the writer thread */
    qac = (sk_queue_and_conn_t *)malloc(sizeof(*qac));
    MEM_ASSERT(qac != NULL);
    qac->q = q;
    qac->conn = conn;
    THREAD_START("skmsg_writer", rv, q, &conn->writer, writer_thread, qac);
    XASSERT(rv == 0);

    /* Start the reader thread, */
    qac = (sk_queue_and_conn_t *)malloc(sizeof(*qac));
    MEM_ASSERT(qac != NULL);
    qac->q = q;
    qac->conn = conn;
    THREAD_START("skmsg_reader", rv, q, &conn->reader, reader_thread, qac);
    XASSERT(rv == 0);

    *rconn = conn;
    RETURN(0);
}

static void
start_connection(
    sk_msg_queue_t      UNUSED(*q),
    sk_msg_conn_queue_t        *conn)
{
    DEBUG_ENTER_FUNC;

    assert(q);
    assert(conn);
    ASSERT_QUEUE_LOCK(q);

    assert(conn->reader_state == SKM_THREAD_BEFORE);
    assert(conn->writer_state == SKM_THREAD_BEFORE);
    conn->reader_state = SKM_THREAD_RUNNING;
    conn->writer_state = SKM_THREAD_RUNNING;
    MUTEX_BROADCAST(&conn->reader_cond);
    MUTEX_BROADCAST(&conn->writer_cond);

    RETURN_VOID;
}

static void
unblock_connection(
    sk_msg_queue_t      UNUSED(*q),
    sk_msg_conn_queue_t        *conn)
{
    static sk_msg_t unblocker = {{SKMSG_CHANNEL_CONTROL,
                                  SKMSG_WRITER_UNBLOCKER, 0},
                                 NULL, NULL, 1, {{NULL, 0}}};
    skDQErr_t err;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(conn);
    ASSERT_QUEUE_LOCK(q);

    /* Add a special messaage to the writers queue to guarantee it
       will unblock */
    DEBUG_PRINT1("Sending SKMSG_WRITER_UNBLOCKER message");
    err = skDequePushBack(conn->queue, &unblocker);
    XASSERT(err == SKDQ_SUCCESS);

    RETURN_VOID;
}


/* Stops and destroys a connection.  A return value of 0 means the
 * connection object still exists; another thread is destroying that
 * connection right now.  A return value of 1 means the connection has
 * been destroyed.  */
static int
destroy_connection(
    sk_msg_queue_t         *q,
    sk_msg_conn_queue_t    *conn)
{
    sk_msg_channel_queue_t *chan;
    void *cont;
    sk_msg_t *msg;
    skDQErr_t err;
    pthread_t self;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(conn);
    ASSERT_QUEUE_LOCK(q);

    DEBUG_PRINT3("destroy_connection() = %d, %d",
                 conn->rsocket, conn->wsocket);

    /* Check to see if this connection is already being shut down */
    if (conn->state == SKM_CLOSED) {
        RETURN(0);
    }

    /* Okay, start closing */
    conn->state = SKM_CLOSED;
    conn->writer_state = SKM_THREAD_SHUTTING_DOWN;
    conn->reader_state = SKM_THREAD_SHUTTING_DOWN;
    unblock_connection(q, conn);

    /* Empty the queue */
    while ((err = skDequePopBackNB(conn->queue, (void**)&msg))
           == SKDQ_SUCCESS)
    {
        skMsgDestroy(msg);
    }
    assert(err == SKDQ_EMPTY);

    /* Shut down the queue */
    err = skDequeUnblock(conn->queue);
    assert(err == SKDQ_SUCCESS);

    /* Mark all channels using this connection as closed. */
    if (conn->first_channel) {
        assert(conn->first_channel->state == SKM_CREATED);
        conn->first_channel->state = SKM_CLOSED;
        destroy_channel(q, conn->first_channel);
        conn->first_channel = NULL;
    }
    cont = int_dict_get_first(conn->channelmap, NULL, &chan);
    while (cont != NULL) {
        intkey_t channel = chan->channel;
        if ((chan->state == SKM_CONNECTING ||
             chan->state == SKM_CONNECTED))
        {
            set_channel_closed(q, chan, 1);
        }

        cont = int_dict_get_next(conn->channelmap, &channel, &chan);
    }
    assert(conn->refcount == 0);

    /* End the threads */
    self = pthread_self();
    if (!pthread_equal(self, conn->writer)) {
        /* Wait for the writer thread to end */
        THREAD_WAIT_END(q, conn->writer_state);
        pthread_join(conn->writer, NULL);
    }
    if (!pthread_equal(self, conn->reader)) {
        /* Wait for the reader  */
        THREAD_WAIT_END(q, conn->reader_state);
        pthread_join(conn->reader, NULL);
    }
    if (pthread_equal(self, conn->reader)
        || pthread_equal(self, conn->writer))
    {
        DEBUG_PRINT1("Detaching self");
        pthread_detach(self);
    }

    /* Destroy the channelmap */
    int_dict_destroy(conn->channelmap);

    /* Close the socket(s) */
    close(conn->rsocket);
    if (conn->rsocket != conn->wsocket) {
        close(conn->wsocket);
    }

    /* Destroy the queue */
    err = skDequeDestroy(conn->queue);
    assert(err == SKDQ_SUCCESS);

#if SK_ENABLE_GNUTLS
    /* Destroy the session */
    if (conn->use_tls) {
        int rv;
        do {
            rv = gnutls_bye(conn->session, GNUTLS_SHUT_RDWR);
        } while (rv == GNUTLS_E_AGAIN || rv == GNUTLS_E_INTERRUPTED);
        gnutls_deinit(conn->session);
    }
#endif /* SK_ENABLE_GNUTLS */

    /* Destroy the condition variables */
    ASSERT_RESULT(pthread_cond_destroy(&conn->writer_cond), int, 0);
    ASSERT_RESULT(pthread_cond_destroy(&conn->reader_cond), int, 0);

    /* Destroy the address */
    if (conn->addr != NULL) {
        free(conn->addr);
    }

    /* Remove any incomplete buffers */
    if (conn->msg_read_buf.msg) {
        skMsgDestroy(conn->msg_read_buf.msg);
    }

    /* Finally, free the connection object */
    free(conn);

    RETURN(1);
}


static int
accept_connection(
    sk_msg_queue_t     *q,
    int                 listen_sock)
{
    int fd;
    sk_msg_conn_queue_t *conn;
    int rv;
    sk_sockaddr_t addr;
    struct sockaddr *addr_copy;
    socklen_t addrlen = sizeof(addr);

    DEBUG_ENTER_FUNC;

    assert(q);
    ASSERT_QUEUE_LOCK(q);
    assert(q->root->listener_state == SKM_THREAD_RUNNING);

    while ((fd = accept(listen_sock, &addr.sa, &addrlen)) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            DEBUG_PRINT1("Properly handling EAGAIN/EWOULDBLOCK");
            RETURN(1);
        }
        if (errno == EINTR) {
            DEBUGMSG("accept() [%s]", strerror(errno));
            continue;
        }
        if (errno == EBADF) {
            DEBUGMSG("accept() [%s]", strerror(errno));
            RETURN(-1);
        }
        CRITMSG("Unexpected accept() error: %s", strerror(errno));
        XASSERT(0);
        skAbort();
    }

    /* Create the queue and both references */
    addr_copy = (struct sockaddr *)malloc(addrlen);
    if (addr_copy != NULL) {
        memcpy(addr_copy, &addr, addrlen);
    }
#if SK_ENABLE_GNUTLS
    rv = create_connection(q, fd, fd, addr_copy, addrlen, &conn,
                           q->root->bind_tls ? SKM_TLS_SERVER : SKM_TLS_NONE);
#else
    rv = create_connection(q, fd, fd, addr_copy, addrlen, &conn, SKM_TLS_NONE);
#endif /* SK_ENABLE_GNUTLS */

    if (rv != 0) {
        close(fd);
        free(addr_copy);
        RETURN(-1);
    }

    conn->first_channel = create_channel(q);

    start_connection(q, conn);

    RETURN(0);
}

static int
handle_system_control_message(
    sk_msg_queue_t         *q,
    sk_msg_conn_queue_t    *conn,
    sk_msg_t               *msg)
{
    int rv;
    int retval = 0;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(msg);
    ASSERT_QUEUE_LOCK(q);

    switch (msg->hdr.type) {
      case SKMSG_CTL_CHANNEL_ANNOUNCE:
        /* Handle the announcement of a connection */
        {
            skm_channel_t rchannel;
            skm_channel_t lchannel;
            sk_msg_channel_queue_t *chan;
            sk_channel_pair_t pair;
            sk_new_channel_info_t info;

            DEBUG_PRINT1("Handling SKMSG_CTL_CHANNEL_ANNOUNCE");
            assert(msg->hdr.size == sizeof(rchannel));
            assert(msg->segments == 2);

            /* Decode the remote channel */
            rchannel = SKMSG_CTL_MSG_GET_CHANNEL(msg);

            /* Create a local channel */
            if (conn->first_channel) {
                chan = conn->first_channel;
                conn->first_channel = NULL;
            } else {
                chan = create_channel(q);
            }
            lchannel = chan->channel;

            /* Attach the channel to the connection */
            ASSERT_RESULT(set_channel_connecting(q, chan, conn), int, 0);

            /* Set the remote channel */
            ASSERT_RESULT(set_channel_connected(q, chan, rchannel), int, 0);

            /* Respond to the announcement with the channel pair */
            pair.rchannel = htons(rchannel);
            pair.lchannel = htons(lchannel);
            DEBUG_PRINT1("Sending SKMSG_CTL_CHANNEL_REPLY (Ext-control)");
            rv = send_message(q, lchannel,
                              SKMSG_CTL_CHANNEL_REPLY,
                              &pair, sizeof(pair), SKM_SEND_CONTROL,
                              0, NULL);
            if (rv != 0) {
                DEBUG_PRINT1("Sending SKMSG_CTL_CHANNEL_REPLY failed");
                retval = -11;
                break;
            }

            /* Announce the new channel internally */
            info.channel = pair.lchannel;
            if (conn->addr != NULL) {
                memcpy(&info.addr, conn->addr, conn->addrlen);
                info.known = 1;
            } else {
                info.known = 0;
            }
            DEBUG_PRINT1("Sending SKMSG_CTL_NEW_CONNECTION (Internal)");
            rv = send_message(q, SKMSG_CHANNEL_CONTROL,
                              SKMSG_CTL_NEW_CONNECTION,
                              &info, sizeof(info),
                              SKM_SEND_INTERNAL, 0, NULL);
            XASSERT(rv == 0);
        }
        break;

      case SKMSG_CTL_CHANNEL_REPLY:
        /* Handle the reply to a channel announcement */
        {
            sk_channel_pair_t *pair;
            sk_msg_channel_queue_t *chan;
            skm_channel_t rchannel;
            skm_channel_t lchannel;

            DEBUG_PRINT1("Handling SKMSG_CTL_CHANNEL_REPLY");
            assert(sizeof(*pair) == msg->hdr.size);
            assert(2 == msg->segments);

            /* Decode the channels: Reversed directionality is on
               purpose. */
            pair = (sk_channel_pair_t *)msg->segment[1].iov_base;
            rchannel = ntohs(pair->lchannel);
            lchannel = ntohs(pair->rchannel);

            /* Get the channel object */
            chan = find_channel(q, lchannel);
            XASSERT(chan != NULL);

            /* Set the remote channel */
            ASSERT_RESULT(set_channel_connected(q, chan, rchannel), int, 0);

            chan->conn->state = SKM_CONNECTED;

            /* Complete the connection */
            assert(chan->state != SKM_CONNECTING);
            assert(chan->is_pending);
            MUTEX_BROADCAST(&chan->pending);
        }
        break;

      case SKMSG_CTL_CHANNEL_KILL:
        /* Handle the death of a remote channel */
        {
            skm_channel_t channel;
            sk_msg_channel_queue_t *chan;

            DEBUG_PRINT1("Handling SKMSG_CTL_CHANNEL_KILL");

            assert(msg->hdr.size == sizeof(channel));
            assert(msg->segments == 2);

            /* Decode the channel. */
            channel = SKMSG_CTL_MSG_GET_CHANNEL(msg);

            /* Get the channel object. */
            chan = find_channel(q, channel);
            XASSERT(chan != NULL);

            /* Close the channel. */
            retval = set_channel_closed(q, chan, 0);
        }
        break;

      case SKMSG_CTL_CHANNEL_KEEPALIVE:
        DEBUG_PRINT1("Handling SKMSG_CTL_CHANNEL_KEEPALIVE");
        assert(msg->hdr.size == 0);
        /* Do nothing on KEEPALIVE*/
        break;

      default:
        skAbortBadCase(msg->hdr.type);
    }

    skMsgDestroy(msg);

    RETURN(retval);
}


/*
 *    THREAD ENTRY POINT
 *
 *    Entry point for the "skmsg_listener" thread, started from
 *    skMsgQueueBind()
 */
static void *
listener_thread(
    void               *vq)
{
    sk_msg_queue_t *q = (sk_msg_queue_t *)vq;
    int count;
    nfds_t len, valid;
    struct pollfd *pfd;

    DEBUG_ENTER_FUNC;

    DEBUG_PRINT1("Started listener_thread");

    assert(q);

    QUEUE_LOCK(q);

    pfd = q->root->pfd;
    len = q->root->pfd_len;
    valid = len;
    q->root->listener_state = SKM_THREAD_RUNNING;
    MUTEX_BROADCAST(&q->root->listener_cond);
    QUEUE_UNLOCK(q);

    while (valid && q->root->listener_state == SKM_THREAD_RUNNING) {
        unsigned i;
        int rv;

        /* Call poll */
        count = poll(pfd, len, SKMSG_IO_POLL_TIMEOUT);
        if (count == -1) {
            if (errno == EINTR || errno == EBADF) {
                DEBUG_PRINT2("Ignoring expected poll() error: %s",
                             strerror(errno));
                continue;
            }
            CRITMSG("Unexpected poll() error: %s", strerror(errno));
            skAbort();
        }

        /* Check the file descriptor results */
        for (i = 0; i < len; i++) {
            if (pfd[i].fd < 0) {
                continue;
            }
            if (pfd[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                DEBUG_PRINT3("Poll returned %d, but revents was %d",
                             count, pfd[i].revents);
                pfd[i].fd = -1;
                valid--;
            } else if (pfd[i].revents & POLLIN) {
                DEBUG_PRINT1("Accepting connection: trying");
                QUEUE_LOCK(q);
                if (q->root->listener_state != SKM_THREAD_RUNNING) {
                    QUEUE_UNLOCK(q);
                    DEBUG_PRINT1("Accepting connection: thread is ending");
                    break;
                }
                rv = accept_connection(q, pfd[i].fd);
                QUEUE_UNLOCK(q);
                if (rv == 0) {
                    DEBUG_PRINT1("Accepting connection: succeeded");
                } else {
                    DEBUG_PRINT1("Accepting connection: failed");
                }
            }
        }
    }

    QUEUE_LOCK(q);
    q->root->listener_state = SKM_THREAD_ENDED;
    THREAD_END(q);
    QUEUE_UNLOCK(q);

    DEBUG_PRINT1("STOPPED listener_thread");

    RETURN(NULL);
}


/*
 *    THREAD ENTRY POINT
 *
 *    Entry point for the "skmsg_reader" thread, started from
 *    create_connection().  Argument is a temporary structure that
 *    contains pointers to the queue and the connection.
 */
static void *
reader_thread(
    void               *vconn)
{
    sk_queue_and_conn_t *both = (sk_queue_and_conn_t *)vconn;
    sk_msg_conn_queue_t *conn = both->conn;
    sk_msg_queue_t *q         = both->q;
    sk_msg_channel_queue_t *chan;
    int destroyed = 0;
    struct pollfd pfd;
    sk_sockaddr_t addr;
    char addr_buf[128] = "<unknown>";

    DEBUG_ENTER_FUNC;

    DEBUG_PRINT1("STARTED reader_thread");

    assert(conn);
    assert(q);

    free(both);
    both = NULL;

    /* Wait for a signal to start */
    QUEUE_LOCK(q);
    while (conn->reader_state == SKM_THREAD_BEFORE) {
        QUEUE_WAIT(&conn->reader_cond, q);
    }
    QUEUE_UNLOCK(q);

    /* Get the peer's address for error reporting */
    if (conn->addr != NULL) {
        memcpy(&addr.sa, conn->addr, conn->addrlen);
        skSockaddrString(addr_buf, sizeof(addr_buf), &addr);
    }

    /* Add the current time to the connection */
    conn->last_recv = time(NULL);

    /* Set up the poll descriptor */
    pfd.fd = conn->rsocket;
    pfd.events = POLLIN;

    while (!destroyed && conn->state != SKM_CLOSED
           && conn->reader_state == SKM_THREAD_RUNNING)
    {
        int           rv;
        sk_msg_t     *message = NULL;

#if SK_ENABLE_GNUTLS
        /* If the transport is TLS and there are still pending bytes
         * in the session, don't bother polling; we have data. */
        if (conn->transport != CONN_TLS
            || !gnutls_record_check_pending(conn->session))
#endif
        {
            /* Poll for new data on the socket */
            rv = poll(&pfd, 1, SKMSG_IO_POLL_TIMEOUT);
            if (rv == -1) {
                if (errno == EINTR || errno == EBADF) {
                    DEBUG_PRINT2("Ignoring expected poll(POLLIN) error: %s",
                                 strerror(errno));
                    continue;
                }
                CRITMSG("Unexpected poll(POLLIN) error for %s: %s",
                        addr_buf, strerror(rv));
                skAbort();
            }
            if (rv == 0) {
                if (CONNECTION_STAGNANT(conn, time(NULL))) {
                    /* It's been too long since we have heard something.
                     * Assume the connection died. */
                    INFOMSG(("Destroying connection to %s due to"
                             " %.0f seconds of inactivity"),
                            addr_buf, difftime(time(NULL), conn->last_recv));
                    QUEUE_LOCK(q);
                    destroyed = destroy_connection(q, conn);
                    QUEUE_UNLOCK(q);
                    break;
                }
#if SENDRCV_DEBUG & DEBUG_SKMSG_POLL_TIMEOUT
                WRAP_ERRNO(SKTHREAD_DEBUG_PRINT3(
                               "Timeout on poll(%d, POLLIN) for %s",
                               pfd.fd, addr_buf));
#endif
                continue;
            }
            if (pfd.revents & POLLNVAL) {
                /* This is handled the same as EBADF from poll().  The
                 * standard states that we should get POLLNVAL for a
                 * bad file descriptor.  Under Linux, we might get a
                 * EBADF errno from the poll instead. */
                DEBUG_PRINT1("poll(POLLIN) returned POLLNVAL");
                continue;
            }
            if (pfd.revents & (POLLHUP | POLLERR)) {
                /* Handle a disconnect or device error  */
                INFOMSG("Closing connection to %s due to a disconnect (%s)",
                        addr_buf, SK_POLL_EVENT_STR(pfd.revents));
                QUEUE_LOCK(q);
                destroyed = destroy_connection(q, conn);
                QUEUE_UNLOCK(q);
                break;
            }
        }
#if SK_ENABLE_GNUTLS && ((SENDRCV_DEBUG) & DEBUG_SKMSG_OTHER)
        else {
            DEBUG_PRINT2("Skipping poll(); %" SK_PRIuZ " bytes are pending",
                         gnutls_record_check_pending(conn->session));
        }
#endif

        /* Update time for last received data; used by CONNECTION_STAGNANT */
        conn->last_recv = time(NULL);

        /* Read a message */
        DEBUG_PRINT1("Calling recv");
#if SK_ENABLE_GNUTLS
        if (CONN_TLS == conn->transport) {
            rv = tls_recv(conn, &message);
        } else
#endif
        {
            rv = tcp_recv(conn, &message);
        }
        if (rv == SKMERR_PARTIAL || rv == SKMERR_EMPTY) {
            assert(NULL == message);
            /* the recv() was successful, but only part of the message
             * was available to read, or nothing was read at all */
            continue;
        }
        if (rv != 0) {
            /* Treat the connection as closed */
            INFOMSG("Closing connection to %s due to failed read: %s",
                    addr_buf, skmerr_strerror(rv));
            QUEUE_LOCK(q);
            destroyed = destroy_connection(q, conn);
            QUEUE_UNLOCK(q);
            break;
        }

        assert(message);

        /* Handle control messages */
        if (message->hdr.channel == SKMSG_CHANNEL_CONTROL &&
            message->hdr.type >= SKMSG_MINIMUM_SYSTEM_CTL_CHANNEL)
        {
            QUEUE_LOCK(q);
            rv = handle_system_control_message(q, conn, message);
            QUEUE_UNLOCK(q);
            if (rv == 1) {
                destroyed = 1;
            }
            continue;
        }

        /* Handle ordinary messages */
        chan = find_channel(q, message->hdr.channel);
        if (chan == NULL) {
            skMsgDestroy(message);
        } else {
            /* Put the message on the queue */
            DEBUG_PRINT3("Enqueue: chan=%#x type=%#x",
                         message->hdr.channel, message->hdr.type);
            DEBUG_PRINT2("From reader: %p", (void *)message);
            rv = mqQueueAdd(chan->queue, message);
            if (rv != 0) {
                XASSERT(conn->state == SKM_CLOSED ||
                        conn->reader_state != SKM_THREAD_RUNNING);
                skMsgDestroy(message);
            }
        }
    }

    QUEUE_LOCK(q);
    if (!destroyed) {
        conn->reader_state = SKM_THREAD_ENDED;
    }
    THREAD_END(q);
    QUEUE_UNLOCK(q);

    DEBUG_PRINT1("STOPPED reader_thread");

    RETURN(NULL);
}


/*
 *    THREAD ENTRY POINT
 *
 *    Entry point for the "skmsg_writer" thread, started from
 *    create_connection().  Argument is a temporary structure that
 *    contains pointers to the queue and the connection.
 */
static void *
writer_thread(
    void               *vconn)
{
    sk_queue_and_conn_t *both      = (sk_queue_and_conn_t *)vconn;
    sk_msg_conn_queue_t *conn      = both->conn;
    sk_msg_queue_t      *q         = both->q;
    int                  have_msg  = 0;
    int                  destroyed = 0;
    sk_msg_write_buf_t   write_buf;
    struct pollfd pfd;
    sk_sockaddr_t addr;
    char addr_buf[128] = "<unknown>";

    DEBUG_ENTER_FUNC;

    DEBUG_PRINT1("STARTED writer_thread");

    assert(conn);
    assert(q);

    free(both);

    memset(&write_buf, 0, sizeof(write_buf));

    /* Wait for a signal to start */
    QUEUE_LOCK(q);
    while (conn->writer_state == SKM_THREAD_BEFORE) {
        QUEUE_WAIT(&conn->writer_cond, q);
    }
    QUEUE_UNLOCK(q);

    /* Get the peer's address for error reporting */
    if (conn->addr != NULL) {
        memcpy(&addr.sa, conn->addr, conn->addrlen);
        skSockaddrString(addr_buf, sizeof(addr_buf), &addr);
    }

    /* Set up the poll descriptor */
    pfd.fd = conn->wsocket;
    pfd.events = POLLOUT;

    while (conn->writer_state == SKM_THREAD_RUNNING) {
        int           rv;
        skDQErr_t     err;
        int           block = (conn->state != SKM_CLOSED);

        /* If not currently sending a message, get a new message to
         * send from the queue  */
        if (!have_msg) {
            if (!block) {
                /* If the connection is closed, don't block in the deque */
                err = skDequePopBackNB(conn->queue, (void**)&write_buf.msg);
            } else if (conn->keepalive == 0) {
                err = skDequePopBack(conn->queue, (void**)&write_buf.msg);
            } else {
                err = skDequePopBackTimed(conn->queue, (void**)&write_buf.msg,
                                          conn->keepalive);
                if (err == SKDQ_TIMEDOUT) {
                    /* Create a keepalive message */
                    write_buf.msg = (sk_msg_t*)calloc(1, sizeof(sk_msg_t));
                    MEM_ASSERT(write_buf.msg);
                    write_buf.msg->segments = 1;
                    write_buf.msg->segment[0].iov_base = &write_buf.msg->hdr;
                    write_buf.msg->segment[0].iov_len
                        = sizeof(write_buf.msg->hdr);
                    write_buf.msg->hdr.channel = SKMSG_CHANNEL_CONTROL;
                    write_buf.msg->hdr.type = SKMSG_CTL_CHANNEL_KEEPALIVE;
                    /* Pretend it came from the queue */
                    err = SKDQ_SUCCESS;
                    DEBUG_PRINT1("Sending SKMSG_CTL_CHANNEL_KEEPALIVE");
                }
            }
            if (err != SKDQ_SUCCESS) {
                assert(err == SKDQ_UNBLOCKED || err == SKDQ_DESTROYED
                       || err == SKDQ_EMPTY);
                break;
            }
            if (write_buf.msg->hdr.channel == SKMSG_CHANNEL_CONTROL
                && write_buf.msg->hdr.type == SKMSG_WRITER_UNBLOCKER)
            {
                /* Do not destroy message, as this is a special static
                 * message. */
                write_buf.msg = NULL;
                DEBUG_PRINT1("Handling SKMSG_WRITER_UNBLOCKER message");
                continue;
            }
            have_msg = 1;
            write_buf.msg_size
                = sizeof(write_buf.msg->hdr) + write_buf.msg->hdr.size;
            write_buf.cur_seg = 0;
            write_buf.seg_offset = 0;

            /* Convert data to network byte order */
            write_buf.msg->hdr.channel = htons(write_buf.msg->hdr.channel);
            write_buf.msg->hdr.type    = htons(write_buf.msg->hdr.type);
            write_buf.msg->hdr.size    = htons(write_buf.msg->hdr.size);
        }

        /* Wait for socket to be available for writing */
        rv = poll(&pfd, 1, SKMSG_IO_POLL_TIMEOUT);
        if (rv == -1) {
            if (errno == EINTR || errno == EBADF) {
                DEBUG_PRINT2("Ignoring expected poll(POLLOUT) error: %s",
                             strerror(errno));
                continue;
            }
            CRITMSG("Unexpected poll(POLLOUT) error for %s: %s",
                    addr_buf, strerror(errno));
            skAbort();
        }
        if (rv == 0) {
#if SENDRCV_DEBUG & DEBUG_SKMSG_POLL_TIMEOUT
            WRAP_ERRNO(SKTHREAD_DEBUG_PRINT3(
                           "Timeout on poll(%d, POLLOUT) for %s",
                           pfd.fd, addr_buf));
#endif
            continue;
        }
        if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            /* Handle a disconnect or device error  */
            INFOMSG("Closing connection to %s due to a disconnect (%s)",
                    addr_buf, SK_POLL_EVENT_STR(pfd.revents));
            QUEUE_LOCK(q);
            destroyed = destroy_connection(q, conn);
            QUEUE_UNLOCK(q);
            break;
        }

#if SK_ENABLE_GNUTLS
        if (CONN_TLS == conn->transport) {
            rv = tls_send(conn, &write_buf);
        } else
#endif
        {
            rv = tcp_send(conn, &write_buf);
        }
        if (rv == SKMERR_PARTIAL) {
            /* partial write */
            continue;
        }
        have_msg = 0;
        skMsgDestroy(write_buf.msg);
        write_buf.msg = NULL;
        if (rv != 0) {
            /* Treat the connection as closed */
            INFOMSG("Closing connection to %s due to failed write: %s",
                    addr_buf, skmerr_strerror(rv));
            QUEUE_LOCK(q);
            destroyed = destroy_connection(q, conn);
            QUEUE_UNLOCK(q);
            break;
        }
    }

    if (write_buf.msg) {
        skMsgDestroy(write_buf.msg);
    }

    QUEUE_LOCK(q);
    if (!destroyed) {
        conn->writer_state = SKM_THREAD_ENDED;
    }
    THREAD_END(q);
    QUEUE_UNLOCK(q);

    DEBUG_PRINT1("STOPPED writer_thread");

    RETURN(NULL);
}


int
skMsgQueueCreate(
    sk_msg_queue_t    **queue)
{
    sk_msg_queue_t *q;
    int retval = 0;
    int fd[2];
    int rv;
    sk_msg_conn_queue_t *conn;
    sk_msg_channel_queue_t *chan;

    DEBUG_ENTER_FUNC;

    q = (sk_msg_queue_t*)calloc(1, sizeof(sk_msg_queue_t));
    if (q == NULL) {
        RETURN(SKMERR_MEMORY);
    }

    q->root = (sk_msg_root_t*)calloc(1, sizeof(sk_msg_root_t));
    if (q->root == NULL) {
        free(q);
        RETURN(SKMERR_MEMORY);
    }

    THREAD_INFO_INIT(q);

    q->root->channel = int_dict_create(sizeof(sk_msg_channel_queue_t *));
    if (q->root->channel == NULL) {
        retval = SKMERR_MEMORY;
        goto error;
    }
    q->root->groups = int_dict_create(sizeof(sk_msg_queue_t *));
    if (q->root->groups == NULL) {
        retval = SKMERR_MEMORY;
        goto error;
    }
    q->channel = int_dict_create(sizeof(sk_msg_channel_queue_t *));
    if (q->channel == NULL) {
        retval = SKMERR_MEMORY;
        goto error;
    }

    rv = pthread_mutex_init(QUEUE_MUTEX(q), NULL);
    if (rv != 0) {
        retval = SKMERR_MUTEX;
        goto error;
    }

    rv = pthread_cond_init(&q->shutdowncond, NULL);
    if (rv != 0) {
        retval = SKMERR_MUTEX;
        goto error;
    }

    q->group = mqCreateFair(sk_destroy_report_message);
    if (q->group == NULL) {
        goto error;
    }

    rv = pipe(fd);
    if (rv == -1) {
        retval = SKMERR_PIPE;
        goto error;
    }

    /* Initialize the listener thread start state */
    pthread_cond_init(&q->root->listener_cond, NULL);
    q->root->listener_state = SKM_THREAD_BEFORE;

    /* Lock the mutex to satisfy preconditions for mucking with
       connections and channels. */
    QUEUE_LOCK(q);

    /* Create an internal connection for the control channel. */
    rv = create_connection(q, fd[READ], fd[WRITE], NULL, 0, &conn,
                           SKM_TLS_NONE);
    conn->keepalive = SKMSG_CONTROL_KEEPALIVE_TIMEOUT;
    unblock_connection(q, conn);
    XASSERT(rv == 0);

    /* Create a channel for the control channel. */
    q->root->next_channel = SKMSG_CHANNEL_CONTROL;
    chan = create_channel(q);

    /* Start the connection */
    start_connection(q, conn);

    /* Attach the internal connection to the control channel. */
    ASSERT_RESULT(set_channel_connecting(q, chan, conn), int, 0);

    /* And let's completely connect it. */
    rv = set_channel_connected(q, chan, SKMSG_CHANNEL_CONTROL);
    conn->state = SKM_CONNECTED;

    /* Unlock the mutex */
    QUEUE_UNLOCK(q);

    *queue = q;
    RETURN(0);

  error:
    XASSERT(0);
    skMsgQueueDestroy(q);
    RETURN(retval);
}

static void
sk_msg_queue_shutdown(
    sk_msg_queue_t     *q)
{
    sk_msg_channel_queue_t *chan;
    void *cont;

    DEBUG_ENTER_FUNC;

    assert(q);
    ASSERT_QUEUE_LOCK(q);

    if (q->shuttingdown) {
        RETURN_VOID;
    }

    q->shuttingdown = 1;

    /* Shut down a queue by shutting down all its channels. */
    cont = int_dict_get_first(q->channel, NULL, &chan);
    while (cont != NULL) {
        intkey_t key = chan->channel;
        if (chan->state == SKM_CONNECTED || chan->state == SKM_CONNECTING) {
            set_channel_closed(q, chan, 0);
        }
        cont = int_dict_get_next(q->channel, &key, &chan);
    }

    /* And then shutting down the multiqueue */
    mqShutdown(q->group);

    q->shuttingdown = 0;

    MUTEX_BROADCAST(&q->shutdowncond);

    RETURN_VOID;
}


void
skMsgQueueShutdown(
    sk_msg_queue_t     *q)
{
    DEBUG_ENTER_FUNC;

    assert(q);

    QUEUE_LOCK(q);

    sk_msg_queue_shutdown(q);

    QUEUE_UNLOCK(q);
    RETURN_VOID;
}


void
skMsgQueueShutdownAll(
    sk_msg_queue_t     *q)
{
    sk_msg_channel_queue_t *chan;
    void *cont;

    DEBUG_ENTER_FUNC;

    assert(q);

    QUEUE_LOCK(q);

    if (q->root->shuttingdown) {
        QUEUE_UNLOCK(q);
        RETURN_VOID;
    }

    q->root->shuttingdown = 1;
    q->root->shutdownqueue = q;

    q->root->listener_state = SKM_THREAD_SHUTTING_DOWN;

    /* Shut down all channels */
    cont = int_dict_get_first(q->root->channel, NULL, &chan);
    while (cont != NULL) {
        intkey_t key = chan->channel;
        sk_msg_queue_shutdown(chan->group);
        cont = int_dict_get_next(q->root->channel, &key, &chan);
    }

    if (q->root->pfd) {
        nfds_t i;

        for (i = 0; i < q->root->pfd_len; i++) {
            if (q->root->pfd[i].fd >= 0) {
                close(q->root->pfd[i].fd);
                q->root->pfd[i].fd = -1;
            }
        }
    }

    THREAD_WAIT_ALL_END(q);

    if (q->root->pfd) {
        pthread_join(q->root->listener, NULL);
        free(q->root->pfd);
        q->root->pfd = NULL;
    }

    q->root->shuttingdown = 0;

    MUTEX_BROADCAST(&q->shutdowncond);

    QUEUE_UNLOCK(q);

    RETURN_VOID;
}


static void
skMsgQueueDestroyAll(
    sk_msg_queue_t     *q)
{
    DEBUG_ENTER_FUNC;

    assert(q);
    ASSERT_QUEUE_LOCK(q);

    /* Verify that all channels have been destroyed */
    assert(int_dict_get_first(q->root->channel, NULL, NULL) == NULL);

    int_dict_destroy(q->root->channel);
    int_dict_destroy(q->root->groups);

    QUEUE_UNLOCK(q);

    THREAD_INFO_DESTROY(q);

#if SK_ENABLE_GNUTLS
    if (q->root->cred_set) {
        gnutls_certificate_free_credentials(q->root->cred);
    }
#endif

    ASSERT_RESULT(pthread_cond_destroy(&q->root->listener_cond), int, 0);
    ASSERT_RESULT(pthread_mutex_destroy(QUEUE_MUTEX(q)), int, 0);

    free(q->root);
    free(q);

    RETURN_VOID;
}


void
skMsgQueueDestroy(
    sk_msg_queue_t     *q)
{
    sk_msg_root_t *root;
    sk_msg_channel_queue_t *chan;
    void *cont;

    DEBUG_ENTER_FUNC;

    assert(q);

    QUEUE_LOCK(q);

    root = q->root;

    while (q->shuttingdown
           || (root->shuttingdown && root->shutdownqueue == q))
    {
        QUEUE_WAIT(&q->shutdowncond, q);
    }

    /* Destroy the channels */
    cont = int_dict_get_first(q->channel, NULL, &chan);
    while (cont != NULL) {
        intkey_t channel = chan->channel;
        destroy_channel(q, chan);
        cont = int_dict_get_next(q->channel, &channel, &chan);
    }

    mqShutdown(q->group);
    mqDestroy(q->group);

    int_dict_destroy(q->channel);

    if (int_dict_get_first(q->root->groups, NULL, NULL) == NULL) {
        skMsgQueueDestroyAll(q);
        RETURN_VOID;
    }

    free(q);

    MUTEX_UNLOCK(&root->mutex);

    RETURN_VOID;
}

static int
skMsgQueueBind(
    sk_msg_queue_t             *q,
    const sk_sockaddr_array_t  *listen_addrs,
    int                         UNUSED_IF_NOTLS(conn_type))
{
    static int on = 1;
    uint32_t i, n;
    struct pollfd *pfd;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(listen_addrs);
    assert(skSockaddrArraySize(listen_addrs) > 0);

    pfd = (struct pollfd*)calloc(skSockaddrArraySize(listen_addrs),
                                 sizeof(struct pollfd));
    MEM_ASSERT(pfd);

    n = 0;
    DEBUGMSG(("Attempting to bind %" PRIu32 " addresses for %s"),
             skSockaddrArraySize(listen_addrs),
             skSockaddrArrayNameSafe(listen_addrs));
    for (i = 0; i < skSockaddrArraySize(listen_addrs); i++) {
        char addr_string[PATH_MAX];
        int sock;
        sk_sockaddr_t *addr = skSockaddrArrayGet(listen_addrs, i);

        skSockaddrString(addr_string, sizeof(addr_string), addr);

        /* Bind a socket to the address*/
        sock = socket(addr->sa.sa_family, SOCK_STREAM, 0);
        if (sock == -1) {
            DEBUGMSG("Skipping %s: Unable to create stream socket: %s",
                     addr_string, strerror(errno));
            pfd[i].fd = -1;
            continue;
        }
        rv = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                        &on, sizeof(on));
        XASSERT(rv != -1);
        rv = bind(sock, &addr->sa, skSockaddrLen(addr));
        if (rv == 0) {
            DEBUGMSG("Succeeded binding to %s", addr_string);
            rv = listen(sock, LISTENQ);
            XASSERT(rv != -1);
            set_nonblock(sock);
            pfd[i].events = POLLIN;
            n++;
        } else {
            DEBUGMSG("Skipping %s: Unable to bind: %s",
                     addr_string, strerror(errno));
            close(sock);
            sock = -1;
        }
        pfd[i].fd = sock;
    }
    if (n == 0) {
        ERRMSG("Failed to bind any addresses for %s",
               skSockaddrArrayNameSafe(listen_addrs));
        free(pfd);
        RETURN(-1);
    }

    DEBUGMSG(("Bound %" PRIu32 "/%" PRIu32 " addresses for %s"),
             (uint32_t)n, skSockaddrArraySize(listen_addrs),
             skSockaddrArrayNameSafe(listen_addrs));

    QUEUE_LOCK(q);

    if (q->root->listener_state != SKM_THREAD_BEFORE) {
        QUEUE_UNLOCK(q);
        for (i = 0; i < skSockaddrArraySize(listen_addrs); i++) {
            if (pfd[i].fd >= 0) {
                close(pfd[i].fd);
            }
        }
        free(pfd);
        RETURN(-1);
    }

    /* Set the listen sock for the queue. */
    assert(q->root->pfd == NULL);
    q->root->pfd = pfd;
    q->root->pfd_len = skSockaddrArraySize(listen_addrs);

#if SK_ENABLE_GNUTLS
    q->root->bind_tls = (conn_type == CONN_TLS);
#endif

    THREAD_START("skmsg_listener", rv, q, &q->root->listener,
                 listener_thread, q);
    XASSERT(rv == 0);

    while (q->root->listener_state == SKM_THREAD_BEFORE) {
        QUEUE_WAIT(&q->root->listener_cond, q);
    }
    assert(q->root->listener_state == SKM_THREAD_RUNNING);

    QUEUE_UNLOCK(q);

    RETURN(0);
}

/* Start a listener */
int
skMsgQueueBindTCP(
    sk_msg_queue_t             *queue,
    const sk_sockaddr_array_t  *addr)
{
    return skMsgQueueBind(queue, addr, CONN_TCP);
}

#if SK_ENABLE_GNUTLS
/* Start a listener */
int
skMsgQueueBindTLS(
    sk_msg_queue_t             *queue,
    const sk_sockaddr_array_t  *addr)
{
    return skMsgQueueBind(queue, addr, CONN_TLS);
}
#endif /* SK_ENABLE_GNUTLS */


static int
skMsgQueueConnect(
    sk_msg_queue_t     *q,
    struct sockaddr    *addr,
    socklen_t           addrlen,
    skm_channel_t      *channel,
    skm_conn_t          tls)
{
    int rv;
    int sock;
    sk_msg_conn_queue_t *conn;
    sk_msg_channel_queue_t *chan;
    int retval;
    skm_channel_t lchannel;
    struct sockaddr *copy;

    DEBUG_ENTER_FUNC;

    assert(q);
    sock = socket(addr->sa_family, SOCK_STREAM, 0);
    if (sock == -1) {
        RETURN(-1);
    }

    /* Connect to the remote side */
    rv = connect(sock, addr, addrlen);
    if (rv == -1) {
        DEBUGMSG("Failed to connect: %s", strerror(errno));
        close(sock);
        RETURN(-1);
    }

    /* Create a channel and connection, and bind them */
    QUEUE_LOCK(q);
    if (q->shuttingdown) {
        close(sock);
        QUEUE_UNLOCK(q);
        RETURN(-1);
    }
    copy = (struct sockaddr *)malloc(addrlen);
    if (copy != NULL) {
        memcpy(copy, addr, addrlen);
    }
    rv = create_connection(q, sock, sock, copy, addrlen, &conn,
                           (tls == CONN_TLS) ? SKM_TLS_CLIENT : SKM_TLS_NONE);
    if (rv == -1) {
        close(sock);
        free(copy);
        QUEUE_UNLOCK(q);
        RETURN(-1);
    }

    chan = create_channel(q);
    start_connection(q, conn);
    rv = set_channel_connecting(q, chan, conn);
    XASSERT(rv == 0);

    /* Announce the channel id to the remote queue */
    lchannel = htons(chan->channel);
    DEBUG_PRINT1("Sending SKMSG_CTL_CHANNEL_ANNOUNCE (Ext-control)");
    rv = send_message(q, chan->channel,
                      SKMSG_CTL_CHANNEL_ANNOUNCE,
                      &lchannel, sizeof(lchannel), SKM_SEND_CONTROL, 0, NULL);
    if (rv != 0) {
        DEBUG_PRINT1("Sending SKMSG_CTL_CHANNEL_ANNOUNCE failed");
        destroy_connection(q, conn);
        close(sock);
        QUEUE_UNLOCK(q);
        RETURN(-1);
    }

    /* Wait for a reply */
    chan->is_pending = 1;
    while (chan->is_pending && chan->state == SKM_CONNECTING) {
        QUEUE_WAIT(&chan->pending, q);
    }
    chan->is_pending = 0;

    if (chan->state == SKM_CLOSED) {
        destroy_channel(q, chan);
        retval = -1;
    } else {
        retval = 0;
        *channel = chan->channel;
    }

    QUEUE_UNLOCK(q);

    RETURN(retval);
}

int
skMsgQueueConnectTCP(
    sk_msg_queue_t     *q,
    struct sockaddr    *addr,
    socklen_t           addrlen,
    skm_channel_t      *channel)
{
    return skMsgQueueConnect(q, addr, addrlen, channel, CONN_TCP);
}

#if SK_ENABLE_GNUTLS
int
skMsgQueueConnectTLS(
    sk_msg_queue_t     *q,
    struct sockaddr    *addr,
    socklen_t           addrlen,
    skm_channel_t      *channel)
{
    return skMsgQueueConnect(q, addr, addrlen, channel, CONN_TLS);
}
#endif /* SK_ENABLE_GNUTLS */

int
skMsgChannelNew(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    skm_channel_t      *new_channel)
{
    sk_msg_channel_queue_t *chan;
    sk_msg_channel_queue_t *newchan;
    skm_channel_t lchannel;
    int retval;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(new_channel);

    QUEUE_LOCK(q);

    if (q->shuttingdown) {
        QUEUE_UNLOCK(q);
        RETURN(-1);
    }

    chan = find_channel(q, channel);
    XASSERT(chan != NULL);
    XASSERT(chan->state == SKM_CONNECTED);
    assert(chan->conn != NULL);

    /* Create a channel and connection, and bind it to the connection */
    newchan = create_channel(q);
    ASSERT_RESULT(set_channel_connecting(q, newchan, chan->conn), int, 0);

    lchannel = htons(newchan->channel);

    /* Announce the channel id to the remote queue */
    DEBUG_PRINT1("Sending SKMSG_CTL_CHANNEL_ANNOUNCE (Ext-control))");
    rv = send_message(q, newchan->channel,
                      SKMSG_CTL_CHANNEL_ANNOUNCE,
                      &lchannel, sizeof(lchannel), SKM_SEND_CONTROL, 0, NULL);
    if (rv != 0) {
        destroy_channel(q, newchan);
        QUEUE_UNLOCK(q);
        RETURN(-1);
    }

    /* Wait for a response */
    newchan->is_pending = 1;
    while (newchan->is_pending && newchan->state == SKM_CONNECTING) {
        QUEUE_WAIT(&newchan->pending, q);
    }
    newchan->is_pending = 0;

    if (newchan->state == SKM_CLOSED) {
        retval = -1;
        destroy_channel(q, newchan);
    } else {
        retval = 0;
        *new_channel = newchan->channel;
    }

    QUEUE_UNLOCK(q);

    RETURN(retval);
}


int
skMsgChannelSplit(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    sk_msg_queue_t    **new_queue)
{
    sk_msg_queue_t *new_q;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);

    new_q = (sk_msg_queue_t *)calloc(1, sizeof(*new_q));
    if (new_q == NULL) {
        return -1;
    }

    rv = pthread_cond_init(&new_q->shutdowncond, NULL);
    if (rv != 0) {
        free(new_q);
        return -1;
    }

    new_q->channel = int_dict_create(sizeof(sk_msg_channel_queue_t *));
    if (new_q->channel == NULL) {
        free(new_q);
        return -1;
    }

    new_q->group = mqCreateFair(sk_destroy_report_message);
    if (new_q->group == NULL) {
        int_dict_destroy(new_q->channel);
        free(new_q);
        return -1;
    }

    new_q->root = q->root;

    rv = skMsgChannelMove(channel, new_q);
    if (rv != 0) {
        skMsgQueueDestroy(new_q);
    } else {
        *new_queue = new_q;
    }

    return rv;
}


int
skMsgChannelMove(
    skm_channel_t       channel,
    sk_msg_queue_t     *q)
{
    sk_msg_channel_queue_t *chan;
    int retval = 0;

    DEBUG_ENTER_FUNC;

    assert(q);

    QUEUE_LOCK(q);

    chan = find_channel(q, channel);
    if (chan == NULL) {
        retval = -1;
        goto end;
    }

    ASSERT_RESULT(mqQueueMove(q->group, chan->queue), int, 0);
    ASSERT_RESULT(int_dict_del(chan->group->channel, channel), int, 0);
    ASSERT_RESULT(int_dict_set(q->channel, channel, &chan), int, 0);
    ASSERT_RESULT(int_dict_set(q->root->groups, channel, &q), int, 0);

    chan->group = q;

  end:
    QUEUE_UNLOCK(q);

    RETURN(retval);
}


int
skMsgChannelKill(
    sk_msg_queue_t     *q,
    skm_channel_t       channel)
{
    sk_msg_channel_queue_t *chan;

    DEBUG_ENTER_FUNC;

    assert(q);

    QUEUE_LOCK(q);

    if (!q->shuttingdown) {
        chan = find_channel(q, channel);
        XASSERT(chan != NULL);

        destroy_channel(q, chan);
    }

    QUEUE_UNLOCK(q);

    RETURN(0);
}


static int
send_message(
    sk_msg_queue_t     *q,
    skm_channel_t       lchannel,
    skm_type_t          type,
    void               *message,
    skm_len_t           length,
    sk_send_type_t      send_type,
    int                 no_copy,
    void              (*free_fn)(void *))
{
    sk_msg_channel_queue_t *chan;
    sk_msg_t *msg;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(message || length == 0);
    ASSERT_QUEUE_LOCK(q);

    if (int_dict_get(q->root->channel, lchannel, &chan) == NULL) {
        RETURN(-1);
    }

    if (chan->state == SKM_CLOSED && send_type != SKM_SEND_INTERNAL) {
        RETURN(0);
    }

    msg = (sk_msg_t*)malloc(sizeof(sk_msg_t)
                            + (length ? sizeof(struct iovec) : 0));
    MEM_ASSERT(msg);

    msg->free_fn = msg_simple_free;
    msg->simple_free = NULL;

    msg->segment[0].iov_base = &msg->hdr;
    msg->segment[0].iov_len = sizeof(msg->hdr);

    if (!length) {
        msg->segments = 1;
    } else {
        msg->segments = 2;
        msg->segment[1].iov_len = length;
        if (no_copy) {
            msg->simple_free = free_fn;
            msg->segment[1].iov_base = message;
        } else {
            msg->segment[1].iov_base = malloc(length);
            if (msg->segment[1].iov_base == NULL) {
                free(msg);
                RETURN(-1);
            }
            memcpy(msg->segment[1].iov_base, message, length);
        }
    }

    msg->hdr.type = type;
    msg->hdr.size = length;

    rv = send_message_internal(chan, msg, send_type);
    if (rv != 0) {
        skMsgDestroy(msg);
        RETURN(-1);
    }

    RETURN(0);
}


static int
send_message_internal(
    sk_msg_channel_queue_t *chan,
    sk_msg_t               *msg,
    sk_send_type_t          send_type)
{
    skDQErr_t err;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(chan);
    assert(msg);

    switch (send_type) {
      case SKM_SEND_INTERNAL:
        msg->hdr.channel = chan->channel;
        DEBUG_PRINT3("Enqueue: chan=%#x type=%#x",
                     msg->hdr.channel, msg->hdr.type);
        rv = mqQueueAdd(chan->queue, msg);
        if (rv != 0) {
            RETURN(-1);
        }
        break;
      case SKM_SEND_REMOTE:
        msg->hdr.channel = chan->rchannel;
        err = skDequePushFront(chan->conn->queue, msg);
        if (err != SKDQ_SUCCESS) {
            RETURN(-1);
        }
        break;
      case SKM_SEND_CONTROL:
        msg->hdr.channel = SKMSG_CHANNEL_CONTROL;
        err = skDequePushFront(chan->conn->queue, msg);
        if (err != SKDQ_SUCCESS) {
            RETURN(-1);
        }
        break;
      default:
        skAbortBadCase(send_type);
    }

    RETURN(0);
}


int
skMsgQueueSendMessage(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    skm_type_t          type,
    const void         *message,
    skm_len_t           length)
{
    int rv;

    DEBUG_ENTER_FUNC;

    QUEUE_LOCK(q);
    rv = send_message(q, channel, type, (void *)message,
                      length, SKM_SEND_REMOTE, 0, NULL);
    QUEUE_UNLOCK(q);
    RETURN(rv);
}


int
skMsgQueueInjectMessage(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    skm_type_t          type,
    const void         *message,
    skm_len_t           length)
{
    int rv;

    DEBUG_ENTER_FUNC;

    QUEUE_LOCK(q);
    rv = send_message(q, channel, type, (void *)message,
                      length, SKM_SEND_INTERNAL, 0, NULL);
    QUEUE_UNLOCK(q);
    RETURN(rv);
}

int
skMsgQueueSendMessageNoCopy(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    skm_type_t          type,
    void               *message,
    skm_len_t           length,
    void              (*free_fn)(void *))
{
    int rv;

    DEBUG_ENTER_FUNC;

    QUEUE_LOCK(q);
    rv = send_message(q, channel, type, message, length, SKM_SEND_REMOTE,
                      1, free_fn);
    QUEUE_UNLOCK(q);
    RETURN(rv);
}

int
skMsgQueueScatterSendMessageNoCopy(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    skm_type_t          type,
    uint16_t            num_segments,
    struct iovec       *segments,
    void              (*free_fn)(uint16_t, struct iovec *))
{
    sk_msg_channel_queue_t *chan;
    sk_msg_t               *msg;
    size_t                  size;
    uint16_t                i;
    int                     rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert((num_segments && segments) || (!num_segments && !segments));

    QUEUE_LOCK(q);

    if (int_dict_get(q->root->channel, channel, &chan) == NULL) {
        rv = -1;
        goto end;
    }

    if (chan->state == SKM_CLOSED) {
        rv = 0;
        goto end;
    }

    msg = (sk_msg_t*)malloc(sizeof(sk_msg_t)
                            + sizeof(struct iovec) * num_segments);
    MEM_ASSERT(msg);

    msg->free_fn = free_fn;
    msg->simple_free = NULL;

    msg->segment[0].iov_base = &msg->hdr;
    msg->segment[0].iov_len = sizeof(msg->hdr);
    msg->segments = 1;

    msg->hdr.type = type;
    size = 0;

    for (i = 0; i < num_segments; i++) {
        msg->segment[i + 1] = segments[i];
        size += segments[i].iov_len;
        msg->segments++;
        if (size > UINT16_MAX) {
            skMsgDestroy(msg);
            rv = -1;
            goto end;
        }
    }

    msg->hdr.size = size;

    rv = send_message_internal(chan, msg, SKM_SEND_REMOTE);
    if (rv != 0) {
        skMsgDestroy(msg);
    }

  end:
    QUEUE_UNLOCK(q);

    RETURN(rv);
}


int
skMsgQueueInjectMessageNoCopy(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    skm_type_t          type,
    void               *message,
    skm_len_t           length,
    void              (*free_fn)(void *))
{
    int rv;

    DEBUG_ENTER_FUNC;

    QUEUE_LOCK(q);
    rv = send_message(q, channel, type, message, length, SKM_SEND_INTERNAL,
                      1, free_fn);
    QUEUE_UNLOCK(q);
    RETURN(rv);
}


int
skMsgQueueGetMessage(
    sk_msg_queue_t     *q,
    sk_msg_t          **message)
{
    sk_msg_t *msg;
    sk_msg_channel_queue_t *chan;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(message);

    do {
        rv = mqGet(q->group, (void **)&msg);
        if (rv != 0) {
            RETURN(-1);
        }
        DEBUG_PRINT2("From GetMessage: %p", (void *)msg);
        DEBUG_PRINT4("Dequeue: chan=%#x type=%#x size=%d",
                     msg->hdr.channel, msg->hdr.type, msg->hdr.size);

        chan = find_channel(q, msg->hdr.channel);
    } while (chan == NULL);

    *message = msg;

    RETURN(0);
}


int
skMsgQueueGetMessageFromChannel(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    sk_msg_t          **message)
{
    sk_msg_t *msg;
    sk_msg_channel_queue_t *chan;
    int rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(message);

    chan = find_channel(q, channel);

    if (chan == NULL) {
        RETURN(-1);
    }

    rv = mqQueueGet(chan->queue, (void **)&msg);
    if (rv != 0) {
        RETURN(-1);
    }
    DEBUG_PRINT4("Dequeue: chan=%#x type=%#x size=%d",
                 msg->hdr.channel, msg->hdr.type, msg->hdr.size);

    assert(msg->hdr.channel == channel);

    chan = find_channel(q, msg->hdr.channel);

    if (chan == NULL) {
        RETURN(-1);
    }

    *message = msg;

    RETURN(0);
}

int
skMsgGetRemoteChannelID(
    sk_msg_queue_t     *q,
    skm_channel_t       lchannel,
    skm_channel_t      *rchannel)
{
    sk_msg_channel_queue_t *chan;
    int                     retval;

    DEBUG_ENTER_FUNC;

    assert(q);

    chan = find_channel(q, lchannel);
    if (chan == NULL) {
        retval = -1;
    } else {
        *rchannel = chan->rchannel;
        retval = 0;
    }

    return retval;
}


int
skMsgSetKeepalive(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    uint16_t            keepalive)
{
    sk_msg_channel_queue_t *chan;
    int                     retval;

    DEBUG_ENTER_FUNC;

    assert(q);

    QUEUE_LOCK(q);

    chan = find_channel(q, channel);
    if (chan == NULL || chan->state != SKM_CONNECTED) {
        retval = -1;
    } else {
        chan->conn->keepalive = keepalive;
        unblock_connection(q, chan->conn);
        retval = 0;
    }
    QUEUE_UNLOCK(q);

    return retval;
}


int
skMsgGetConnectionInformation(
    sk_msg_queue_t     *q,
    skm_channel_t       channel,
    char               *buffer,
    size_t              buffer_size)
{
    sk_msg_channel_queue_t *chan;
    sk_msg_conn_queue_t    *conn;
    int                     rv;

    DEBUG_ENTER_FUNC;

    assert(q);
    assert(buffer);

    QUEUE_LOCK(q);
    chan = find_channel(q, channel);
    if (chan == NULL) {
        QUEUE_UNLOCK(q);
        RETURN(-1);
    }
    conn = chan->conn;
    if (conn == NULL) {
        QUEUE_UNLOCK(q);
        RETURN(-1);
    }

#if SK_ENABLE_GNUTLS
    if (conn->use_tls) {
        const char *protocol;
        const char *encryption;

        protocol = gnutls_protocol_get_name(
            gnutls_protocol_get_version(conn->session));
        encryption = gnutls_cipher_get_name(gnutls_cipher_get(conn->session));
        QUEUE_UNLOCK(q);

        rv = snprintf(buffer, buffer_size, "TCP, %s, %s",
                      protocol, encryption);
        RETURN(rv);
    }
#endif  /* SK_ENABLE_GNUTLS */

    QUEUE_UNLOCK(q);
    rv = snprintf(buffer, buffer_size, "TCP");
    RETURN(rv);
}


void
skMsgDestroy(
    sk_msg_t           *msg)
{
    DEBUG_ENTER_FUNC;

    assert(msg);

    if (msg->segments == 2 && msg->simple_free) {
        msg->simple_free(msg->segment[1].iov_base);
    } else if (msg->segments > 1 && msg->free_fn) {
        msg->free_fn(msg->segments - 1, &msg->segment[1]);
    }
    /* We use a static message to unblock the writer queue.  It should
     * not be freed. */
    if (msg->hdr.channel != SKMSG_CHANNEL_CONTROL ||
        msg->hdr.type != SKMSG_WRITER_UNBLOCKER)
    {
        free(msg);
    }

    RETURN_VOID;
}


skm_channel_t
skMsgChannel(
    const sk_msg_t     *msg)
{
    DEBUG_ENTER_FUNC;

    assert(msg);
    RETURN(msg->hdr.channel);
}


skm_type_t
skMsgType(
    const sk_msg_t     *msg)
{
    DEBUG_ENTER_FUNC;

    assert(msg);
    RETURN(msg->hdr.type);
}


skm_len_t
skMsgLength(
    const sk_msg_t     *msg)
{
    DEBUG_ENTER_FUNC;

    assert(msg);
    RETURN(msg->hdr.size);
}


const void *
skMsgMessage(
    const sk_msg_t     *msg)
{
    DEBUG_ENTER_FUNC;

    assert(msg);
    if (msg->segments == 0) {
        RETURN(NULL);
    } else {
        RETURN(msg->segment[1].iov_base);
    }
}


#if !SK_ENABLE_GNUTLS

/* When GnuTLS is not available, create functions that report a useful
 * error message.  This way the user is not faced with a bizarre
 * "missing symbol" message. */

/* provide prototypes to avoid gcc warning */
int
skMsgQueueAddCA(
    sk_msg_queue_t     *queue,
    const char         *cred_filename);
int
skMsgQueueAddCert(
    sk_msg_queue_t     *queue,
    const char         *cert_filename,
    const char         *key_filename);
int
skMsgQueueAddPKCS12(
    sk_msg_queue_t     *queue,
    const char         *cert_filename,
    const char         *password);
int
skMsgQueueBindTLS(
    sk_msg_queue_t     *queue,
    struct sockaddr    *addr,
    socklen_t           addrlen);
int
skMsgQueueConnectTLS(
    sk_msg_queue_t     *queue,
    struct sockaddr    *addr,
    socklen_t           addrlen,
    skm_channel_t      *channel);


#define FUNC_BODY_NO_TLS                                        \
    CRITMSG("FATAL ERROR! Attempting to use libskmsg that "     \
            "does not include GnuTLS support");                 \
    exit(EXIT_FAILURE);

int
skMsgQueueAddCA(
    sk_msg_queue_t  UNUSED(*queue),
    const char      UNUSED(*ca_filename))
{
    FUNC_BODY_NO_TLS
}

int
skMsgQueueAddCert(
    sk_msg_queue_t  UNUSED(*queue),
    const char      UNUSED(*cert_filename),
    const char      UNUSED(*key_filename))
{
    FUNC_BODY_NO_TLS
}

int
skMsgQueueAddPKCS12(
    sk_msg_queue_t  UNUSED(*queue),
    const char      UNUSED(*cert_filename),
    const char      UNUSED(*password))
{
    FUNC_BODY_NO_TLS
}

int
skMsgQueueBindTLS(
    sk_msg_queue_t  UNUSED(*queue),
    struct sockaddr UNUSED(*addr),
    socklen_t        UNUSED(addrlen))
{
    FUNC_BODY_NO_TLS
}

int
skMsgQueueConnectTLS(
    sk_msg_queue_t  UNUSED(*q),
    struct sockaddr UNUSED(*addr),
    socklen_t        UNUSED(addrlen),
    skm_channel_t   UNUSED(*channel))
{
    FUNC_BODY_NO_TLS
}

#endif /* !SK_ENABLE_GNUTLS */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
