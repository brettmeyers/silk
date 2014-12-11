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
**  skthread.c
**
**    Common thread routines, useful for debugging.
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skthread.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>
#include <silk/skthread.h>

/* LOCAL DEFINES AND TYPEDEFS */

typedef struct skthread_data_st {
    const char *name;
    void *(*fn)(void *);
    void *arg;
} skthread_data_t;


/* EXPORTED VARIABLE DEFINITIONS */

/* Used as a flag so we warn on too many read locks only once.  */
int skthread_too_many_readlocks = 0;


/* LOCAL VARIABLE DEFINITIONS */

static int initialized = 0;
static pthread_key_t skthread_name_key;
static pthread_key_t skthread_id_key;

/* mutex for protecting next_thread_id */
static pthread_mutex_t mutex;
static uint32_t next_thread_id = 0;


/* FUNCTION DEFINITIONS */

/*
 *    Allocate a uint32_t, set that value to the next avaible thread
 *    id, and set the thread's ID to that value.
 */
static void
skthread_set_id(
    void)
{
    uint32_t *id = (uint32_t*)malloc(sizeof(uint32_t));
    if (id != NULL) {
        pthread_mutex_lock(&mutex);
        *id = next_thread_id++;
        pthread_setspecific(skthread_id_key, id);
        pthread_mutex_unlock(&mutex);
    }
}

int
skthread_init(
    const char         *name)
{
    if (initialized) {
        return 0;
    }
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        return -1;
    }
    if (pthread_key_create(&skthread_name_key, NULL) != 0) {
        return -1;
    }
    if (pthread_key_create(&skthread_id_key, free) != 0) {
        return -1;
    }
    pthread_setspecific(skthread_name_key, name);
    skthread_set_id();

    initialized = 1;
    return 0;
}

void
skthread_teardown(
    void)
{
    void *val;
    if (!initialized) {
        return;
    }
    initialized = 0;
    val = pthread_getspecific(skthread_id_key);
    pthread_setspecific(skthread_id_key, NULL);
    pthread_key_delete(skthread_id_key);
    pthread_key_delete(skthread_name_key);
    free(val);
}

const char *
skthread_name(
    void)
{
    if (initialized) {
        const char *rv = (const char *)pthread_getspecific(skthread_name_key);
        if (rv != NULL) {
            return rv;
        }
    }
    return "unknown";
}

uint32_t
skthread_id(
    void)
{
    if (initialized) {
        uint32_t *id = (uint32_t *)pthread_getspecific(skthread_id_key);
        if (id != NULL) {
            return *id;
        }
    }
    return SKTHREAD_UNKNOWN_ID;
}


/*
 *    Wrapper function that is invoked by pthread_create().
 *
 *    Sets the thread's name, sets the thread's signal mask to ignore
 *    all signals, then invokes the caller's function with the
 *    caller's argument.  The 'vdata' parameter contains the thread's
 *    name, the caller's function and argument.
 */
static void *
skthread_create_init(
    void               *vdata)
{
    skthread_data_t *data = (skthread_data_t *)vdata;
    void *(*fn)(void *) = data->fn;
    void *arg = data->arg;

    /* ignore all signals */
    skthread_ignore_signals();

    if (initialized) {
        pthread_setspecific(skthread_name_key, data->name);
        skthread_set_id();
    }
    free(data);

    return fn(arg);
}


int
skthread_create(
    const char         *name,
    pthread_t          *thread,
    void             *(*fn)(void *),
    void               *arg)
{
    skthread_data_t *data;
    int rv;

    data = (skthread_data_t *)malloc(sizeof(*data));
    if (NULL == data) {
        return errno;
    }
    data->name = name;
    data->fn = fn;
    data->arg = arg;

    rv = pthread_create(thread, NULL, skthread_create_init, data);
    if (rv != 0) {
        free(data);
    }
    return rv;
}


int
skthread_create_detached(
    const char         *name,
    pthread_t          *thread,
    void             *(*fn)(void *),
    void               *arg)
{
    skthread_data_t *data;
    pthread_attr_t attr;
    int rv;

    rv = pthread_attr_init(&attr);
    if (rv != 0) {
        return rv;
    }
    rv = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    assert(rv == 0);

    data = (skthread_data_t *)malloc(sizeof(*data));
    if (NULL == data) {
        pthread_attr_destroy(&attr);
        return errno;
    }
    data->name = name;
    data->fn = fn;
    data->arg = arg;

    rv = pthread_create(thread, &attr, skthread_create_init, data);
    if (rv != 0) {
        free(data);
    }
    pthread_attr_destroy(&attr);

    return rv;
}


void
skthread_ignore_signals(
    void)
{
    sigset_t sigs;

    sigfillset(&sigs);
    sigdelset(&sigs, SIGABRT);
    sigdelset(&sigs, SIGBUS);
    sigdelset(&sigs, SIGILL);
    sigdelset(&sigs, SIGSEGV);

#ifdef SIGEMT
    sigdelset(&sigs, SIGEMT);
#endif
#ifdef SIGIOT
    sigdelset(&sigs, SIGIOT);
#endif
#ifdef SIGSYS
    sigdelset(&sigs, SIGSYS);
#endif

    pthread_sigmask(SIG_SETMASK, &sigs, NULL);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
