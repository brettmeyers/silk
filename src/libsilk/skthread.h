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
**  skthread.h
**
**    Common thread routines.
**
*/
#ifndef _SKTHREAD_H
#define _SKTHREAD_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKTHREAD_H, "$SiLK: skthread.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/sklog.h>


#define SKTHREAD_UNKNOWN_ID UINT32_MAX


/**
 *    Intitialize the skthread module.  This function is expected be
 *    called by the program's primary thread, and the function must be
 *    called before calling skthread_create() or
 *    skthread_create_detached().
 *
 *    Set the name of the current thread to `name', which must be a
 *    string that is valid for the lifetime of the thread.  Set the ID
 *    of the current thread to 0.
 *
 *    This function is a no-op if it has been called previously and
 *    succeeded.
 *
 *    Return 0 on success, -1 on failure.
 */
int
skthread_init(
    const char         *name);

/**
 *    Teardown function for the skthread module.
 */
void
skthread_teardown(
    void);

/**
 *    Spawn a simple thread and invoke the function 'fn' with the
 *    argument 'arg'.  Call skthread_ignore_signals() within the
 *    context of the new thread.
 *
 *    Set the thread's name to 'name', which must be a string that is
 *    valid for the lifetime of the thread.  Set the thread's ID to
 *    the next unused integer value.
 *
 *    Return 0 on success, errno on failure.
 */
int
skthread_create(
    const char         *name,
    pthread_t          *thread,
    void             *(*fn)(void *),
    void               *arg);

/**
 *    Similar to skthread_create(), except the thread is created with
 *    the detached attribute set.
 */
int
skthread_create_detached(
    const char         *name,
    pthread_t          *thread,
    void             *(*fn)(void *),
    void               *arg);

/**
 *    Return the name of the calling thread that was specified with
 *    skthread_init(), skthread_create(), or
 *    skthread_create_detached().
 */
const char *
skthread_name(
    void);

/**
 *    Return the id of the calling thread.
 */
uint32_t
skthread_id(
    void);

/**
 *    Tell the current thread to ignore all signals except those
 *    indicating a failure (SIGABRT, SIGBUS, SIGSEGV, ...).
 */
void
skthread_ignore_signals(
    void);



/*
 *    Thread debug logging.
 *
 *    Wrappers around DEBUGMSG() that prepend the message with the
 *    current file name, line number, thread name, and thread id.
 */
#define SKTHREAD_DEBUG_PRINT1(x)                \
    DEBUGMSG("%s:%d <%s:%" PRIu32 "> " x,       \
             __FILE__, __LINE__,                \
             skthread_name(), skthread_id())
#define SKTHREAD_DEBUG_PRINT2(x, y)             \
    DEBUGMSG("%s:%d <%s:%" PRIu32 "> " x,       \
             __FILE__, __LINE__,                \
             skthread_name(), skthread_id(),    \
             (y))
#define SKTHREAD_DEBUG_PRINT3(x, y, z)          \
    DEBUGMSG("%s:%d <%s:%" PRIu32 "> " x,       \
             __FILE__, __LINE__,                \
             skthread_name(),                   \
             skthread_id(),                     \
             (y), (z))
#define SKTHREAD_DEBUG_PRINT4(x, y, z, zz)      \
    DEBUGMSG("%s:%d <%s:%" PRIu32 "> " x,       \
             __FILE__, __LINE__,                \
             skthread_name(),                   \
             skthread_id(),                     \
             (y), (z), (zz))


/* Mutex debugging */

#ifdef SKTHREAD_DEBUG_MUTEX
#define SKT_D2(x, y)    SKTHREAD_DEBUG_PRINT2(x, y)
#define SKT_D3(x, y, z) SKTHREAD_DEBUG_PRINT3(x, y, z)
#else
#define SKT_D2(x, y)
#define SKT_D3(x, y, z)
#endif

#define MUTEX_INIT(x) pthread_mutex_init((x), NULL)
#define MUTEX_DESTROY(x) pthread_mutex_destroy((x))

/* Wrapper around pthread_mutex_lock */
#define MUTEX_LOCK(x)                           \
    do {                                        \
        SKT_D2("MUTEX LOCKING %p", (void*)x);   \
        pthread_mutex_lock(x);                  \
        SKT_D2("MUTEX IN LOCK %p", (void*)x);   \
    } while (0)

/* Wrapper around pthread_mutex_unlock */
#define MUTEX_UNLOCK(x)                         \
    do {                                        \
        SKT_D2("MUTEX UNLOCKING %p", (void*)x); \
        pthread_mutex_unlock(x);                \
    } while (0)

/* Wrapper around pthread_cond_wait */
#define MUTEX_WAIT(x, y)                                \
    do {                                                \
        SKT_D3("MUTEX WAIT %p (Unlocked %p)",           \
               (void*)x, (void*)y);                     \
        pthread_cond_wait(x, y);                        \
        SKT_D2("MUTEX RESUME (Locked %p)", (void*)y);   \
    } while (0)

/* Wrapper around pthread_cond_signal */
#define MUTEX_SIGNAL(x)                         \
    do {                                        \
        SKT_D2("SIGNALING %p", (void*)x);       \
        pthread_cond_signal(x);                 \
    } while (0)


/* Wrapper around pthread_cond_broadcast */
#define MUTEX_BROADCAST(x)                      \
    do {                                        \
        SKT_D2("BROADCASTING %p", (void*)x);    \
        pthread_cond_broadcast(x);              \
    } while (0)

#define ASSERT_MUTEX_LOCKED(x)  assert(pthread_mutex_trylock((x)) == EBUSY)

#ifdef SK_HAVE_PTHREAD_RWLOCK
#  define RWMUTEX pthread_rwlock_t

extern int skthread_too_many_readlocks;

#  define READ_LOCK(x)                                                  \
    do {                                                                \
        SKT_D2("READ MUTEX LOCKING %p", (void*)x);                      \
        while (pthread_rwlock_rdlock(x) == EAGAIN) {                    \
            if (!skthread_too_many_readlocks) {                         \
                skthread_too_many_readlocks = 1;                        \
                WARNINGMSG(("WARNING: Too many read locks; "            \
                            "spinlocking enabled to compensate"));      \
            }                                                           \
        }                                                               \
        SKT_D2("READ MUTEX IN LOCK %p", (void*)x);                      \
    } while (0)
#  define WRITE_LOCK(x)                                 \
    do {                                                \
        SKT_D2("WRITE MUTEX LOCKING %p", (void*)x);     \
        pthread_rwlock_wrlock(x);                       \
        SKT_D2("WRITE MUTEX IN LOCK %p", (void*)x);     \
    } while (0)
#  define RW_MUTEX_UNLOCK(x)                            \
    do {                                                \
        SKT_D2("RW MUTEX UNLOCKING %p", (void*)x);      \
        pthread_rwlock_unlock(x);                       \
    } while (0)
#  define RW_MUTEX_INIT(x)           pthread_rwlock_init((x), NULL)
#  define RW_MUTEX_DESTROY(x)        pthread_rwlock_destroy(x)
#ifdef NDEBUG
/* no-ops */
#  define ASSERT_RW_MUTEX_LOCKED(x)
#  define ASSERT_RW_MUTEX_WRITE_LOCKED(x)
#else
#  define ASSERT_RW_MUTEX_LOCKED(x)                             \
    do {                                                        \
        int wrlock_ret = pthread_rwlock_trywrlock(x);           \
        assert(EBUSY == wrlock_ret || EDEADLK == wrlock_ret);   \
    }while(0)
#  define ASSERT_RW_MUTEX_WRITE_LOCKED(x)                       \
    do {                                                        \
        int rdlock_ret = pthread_rwlock_tryrdlock(x);           \
        assert(EBUSY == rdlock_ret || EDEADLK == rdlock_ret);   \
    }while(0)
#endif  /* NDEBUG */

#else  /* #ifdef SK_HAVE_PTHREAD_RWLOCK */

#  define RWMUTEX pthread_mutex_t
#  define READ_LOCK MUTEX_LOCK
#  define WRITE_LOCK MUTEX_LOCK
#  define RW_MUTEX_UNLOCK MUTEX_UNLOCK
#  define RW_MUTEX_INIT MUTEX_INIT
#  define RW_MUTEX_DESTROY MUTEX_DESTROY
#  define ASSERT_RW_MUTEX_LOCKED(x) assert(EBUSY == pthread_mutex_trylock(x))
#  define ASSERT_RW_MUTEX_WRITE_LOCKED(x) ASSERT_RW_MUTEX_LOCKED(x)

#endif  /* #else of #ifdef SK_HAVE_PTHREAD_RWLOCK */


#ifdef __cplusplus
}
#endif
#endif /* _SKTHREAD_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
