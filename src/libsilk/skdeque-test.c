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
**  simple test code for skdeque library
**
*/
#undef NDEBUG

#include <silk/silk.h>

RCSIDENT("$SiLK: skdeque-test.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>
#include <silk/skdeque.h>
#include <silk/sklog.h>

#define SKTHREAD_DEBUG_MUTEX 1
#include <silk/skthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  conda  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  condb  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  waitc  = PTHREAD_COND_INITIALIZER;
static int waita = 0;
static int waitb = 0;
static int resulta = 0;
static int resultb = 0;

static skDeque_t da, db, dc, dd;

static const char *xa = "a";
static const char *xb = "b";
static const char *xc = "c";
static const char *xx = "x";
static const char *xy = "y";
static const char *xz = "z";


#define DEBUG_FMT   "%s:%d <%s:%d> "
#define DEBUG_ARGS  __FILE__, __LINE__, skthread_name(), skthread_id()

#define DEBUG_PRINT1(x)                         \
    DEBUGMSG(DEBUG_FMT  x, DEBUG_ARGS)

#define DEBUG_PRINT2(x, y)                      \
    DEBUGMSG(DEBUG_FMT  x, DEBUG_ARGS, y)

#define XASSERT(x)                              \
    if (x) {                                    \
        DEBUG_PRINT1("PASS");                   \
    } else {                                    \
        assert(x);                              \
    }

#define SLEEP(x)                                \
    do {                                        \
        DEBUG_PRINT2("Sleeping %d seconds", x); \
        sleep(x);                               \
        DEBUG_PRINT1("Finished sleeping");      \
    } while (0)

#define CHECK_EMPTY(qq)                                 \
    XASSERT(0 == skDequeSize(qq));                      \
    XASSERT(skDequeStatus(qq) == SKDQ_EMPTY);           \
                                                        \
    err = skDequeBack(qq, (void**)&v);                  \
    XASSERT(err == SKDQ_EMPTY);                         \
    err = skDequeFront(qq, (void**)&v);                 \
    XASSERT(err == SKDQ_EMPTY);                         \
                                                        \
    err = skDequePopFrontNB(qq, (void**)&v);            \
    XASSERT(err == SKDQ_EMPTY);                         \
    err = skDequePopBackNB(qq, (void**)&v);             \
    XASSERT(err == SKDQ_EMPTY);                         \
                                                        \
    err = skDequePopFrontTimed(qq, (void**)&v, 1);      \
    XASSERT(err == SKDQ_TIMEDOUT);                      \
    err = skDequePopBackTimed(qq, (void**)&v, 1);       \
    XASSERT(err == SKDQ_TIMEDOUT)


/*
 *    Prefix log messages with the current time.
 */
static size_t
log_stamp(
    char               *buf,
    size_t              buflen)
{
    struct timeval t;
    struct tm ts;
    size_t len;

    gettimeofday(&t, NULL);
    localtime_r(&t.tv_sec, &ts);
    /* Format time as 01:02:03.0000006" */
    len = strftime(buf, buflen, "%H:%M:%S", &ts);
    assert(len < buflen);
    len += snprintf(buf+len, buflen-len, ".%06u ", (unsigned int)t.tv_usec);
    return len;
}


static void
starta(
    void)
{
    MUTEX_LOCK(&mutex);
    {
        waita = 0;
        DEBUG_PRINT1("C Broadcast A");
        MUTEX_BROADCAST(&conda);
    }
    MUTEX_UNLOCK(&mutex);
}


static void
startb(
    void)
{
    MUTEX_LOCK(&mutex);
    {
        waitb = 0;
        DEBUG_PRINT1("C Broadcast B");
        MUTEX_BROADCAST(&condb);
    }
    MUTEX_UNLOCK(&mutex);
}


static void
holda(
    void)
{
    MUTEX_LOCK(&mutex);
    waita = 1;
    DEBUG_PRINT1("A Broadcast C");
    MUTEX_BROADCAST(&waitc);
    while (waita) {
        DEBUG_PRINT1("A Waiting");
        MUTEX_WAIT(&conda, &mutex);
    }
    DEBUG_PRINT1("A Continuing");
    MUTEX_UNLOCK(&mutex);
}


static void
holdb(
    void)
{
    MUTEX_LOCK(&mutex);
    waitb = 1;
    DEBUG_PRINT1("B Broadcast C");
    MUTEX_BROADCAST(&waitc);
    while (waitb) {
        DEBUG_PRINT1("B Waiting");
        MUTEX_WAIT(&condb, &mutex);
    }
    DEBUG_PRINT1("B Continuing");
    MUTEX_UNLOCK(&mutex);
}


static void
meet(
    void)
{
    MUTEX_LOCK(&mutex);
    while (!(waita && waitb)) {
        DEBUG_PRINT1("C Waiting");
        MUTEX_WAIT(&waitc, &mutex);
    }
    DEBUG_PRINT1("C Continuing");
    MUTEX_UNLOCK(&mutex);
}


static void *
thread_a(
    void        UNUSED(*dummy))
{
    skDQErr_t err;
    char *v;

    holda();

    /* test 1 */
    err = skDequePopFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    holda();

    /* test 2 */
    err = skDequePopFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    holda();

    /* test 3 */
    err = skDequePopBack(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa || v == xb);
    if (v == xa) {
        resulta = 1;
    } else {
        resultb = 1;
    }
    holda();

    return NULL;
}


static void *
thread_b(
    void        UNUSED(*dummy))
{
    skDQErr_t err;
    char *v;

    holdb();

    /* Test 3 */
    err = skDequePopBack(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa || v == xb);
    if (v == xa) {
        resulta = 1;
    } else {
        resultb = 1;
    }
    holdb();

    return NULL;
}

static void *
thread_c(
    void        UNUSED(*dummy))
{
    skDQErr_t err;

    meet();

    INFOMSG("**** Test 1 ****");
    err = skDequePushFront(da, (void*)xa);
    XASSERT(err == SKDQ_SUCCESS);
    starta();
    meet();

    INFOMSG("**** Test 2 ****");
    starta();
    SLEEP(1);
    err = skDequePushFront(da, (void*)xa);
    XASSERT(err == SKDQ_SUCCESS);
    meet();

    INFOMSG("**** Test 3 ****");
    starta();
    SLEEP(1);
    startb();
    SLEEP(1);
    err = skDequePushFront(da, (void*)xa);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequePushFront(da, (void*)xb);
    XASSERT(err == SKDQ_SUCCESS);
    meet();
    XASSERT(resulta == 1 && resultb == 1);

    /* End */
    starta();
    startb();

    return NULL;
}


int main(int UNUSED(argc), char UNUSED(**argv))
{
    skDQErr_t err;
    pthread_t a, b, c;
    int rv;
    char *v;

    skAppRegister("skdeque-test");
    skthread_init("main");

    rv = sklogSetup(0);
    assert(rv == 0);
    rv = sklogSetDestination("stderr");
    assert(rv == 0);
    rv = sklogOpen();
    assert(rv == 0);
    rv = sklogSetLevel("debug");
    assert(rv == 0);
    rv = sklogEnableThreadedLogging();
    assert(rv == 0);
    rv = sklogSetStampFunction(&log_stamp);
    assert(rv == 0);


    da = skDequeCreate();
    db = skDequeCreate();
    dc = skDequeCreate();

    /*** Single threaded tests ***/

    /* check empty deque */
    CHECK_EMPTY(da);

    /* push first element onto 'da' and check */
    err = skDequePushFront(da, (void*)xa);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequeBack(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    err = skDequeFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    XASSERT(1 == skDequeSize(da));

    /* push first element onto 'db' and check */
    err = skDequePushBack(db, (void*)xb);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequeBack(db, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xb);
    err = skDequeFront(db, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xb);
    XASSERT(1 == skDequeSize(db));

    /* push first element onto 'dc' */
    err = skDequePushFront(dc, (void*)xc);
    XASSERT(err == SKDQ_SUCCESS);

    /* push second element onto front 'da' and check */
    err = skDequePushFront(da, (void*)xx);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequeBack(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    err = skDequeFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);
    XASSERT(2 == skDequeSize(da));

    /* push second element onto 'db' and 'dc' */
    err = skDequePushFront(db, (void*)xy);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequePushFront(dc, (void*)xz);
    XASSERT(err == SKDQ_SUCCESS);

    /* push third element onto back of 'da' and check */
    err = skDequePushBack(da, (void*)xa);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequeBack(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    err = skDequeFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);
    XASSERT(3 == skDequeSize(da));

    /* push third element onto back of 'db' and 'dc' */
    err = skDequePushBack(db, (void*)xb);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequePushBack(dc, (void*)xc);
    XASSERT(err == SKDQ_SUCCESS);

    /* push fourth element onto back of 'da' and check */
    err = skDequePushBack(da, (void*)xx);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequeBack(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);
    err = skDequeFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);
    XASSERT(4 == skDequeSize(da));

    /* push fourth element onto back of 'db' and 'dc' */
    err = skDequePushBack(db, (void*)xy);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequePushBack(dc, (void*)xz);
    XASSERT(err == SKDQ_SUCCESS);

    /* pop four elements from 'da', each from the front */
    err = skDequePopFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);
    XASSERT(3 == skDequeSize(da));
    err = skDequePopFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    XASSERT(2 == skDequeSize(da));
    err = skDequePopFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    XASSERT(1 == skDequeSize(da));
    err = skDequePopFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);
    XASSERT(0 == skDequeSize(da));

    /* verify 'da' is empty */
    CHECK_EMPTY(da);

    /* pop four elements from 'db', each from the back */
    err = skDequePopBack(db, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xy);
    XASSERT(3 == skDequeSize(db));
    err = skDequePopBack(db, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xb);
    XASSERT(2 == skDequeSize(db));
    err = skDequePopBack(db, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xb);
    XASSERT(1 == skDequeSize(db));
    err = skDequePopBack(db, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xy);
    XASSERT(0 == skDequeSize(db));

    /* verify 'db' is empty */
    CHECK_EMPTY(db);

    /* pop four elements from 'dc' */
    err = skDequePopFrontNB(dc, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xz);
    err = skDequePopBackNB(dc, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xz);
    err = skDequePopFrontNB(dc, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xc);
    err = skDequePopBackNB(dc, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xc);
    err = skDequePopBackNB(dc, (void**)&v);
    XASSERT(err == SKDQ_EMPTY);

    /* verify 'dc' is empty */
    CHECK_EMPTY(dc);

    /* create a merged queue from 'da' and 'db' */
    dd = skDequeCreateMerged(da, db);

    /* verify 'dd' is empty */
    CHECK_EMPTY(dd);

    /* push the first element onto each of 'da' and 'db' */
    err = skDequePushBack(da, (void*)xa);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequePushBack(db, (void*)xx);
    XASSERT(err == SKDQ_SUCCESS);

    /* check sizes */
    XASSERT(1 == skDequeSize(da));
    XASSERT(1 == skDequeSize(db));
    XASSERT(2 == skDequeSize(dd));

    /* peek at front and back of merged deck */
    err = skDequeBack(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);
    err = skDequeFront(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);

    /* push the second element onto each of 'da' and 'db' */
    err = skDequePushBack(da, (void*)xb);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequePushBack(db, (void*)xy);
    XASSERT(err == SKDQ_SUCCESS);

    /* peek at front and back of merged deck */
    err = skDequeBack(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xy);
    err = skDequeFront(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);

    /* push a third element onto front and back of merged deck */
    err = skDequePushFront(dd, (void*)xc);
    XASSERT(err == SKDQ_SUCCESS);
    err = skDequePushBack(db, (void*)xz);
    XASSERT(err == SKDQ_SUCCESS);

    /* check sizes */
    XASSERT(3 == skDequeSize(da));
    XASSERT(3 == skDequeSize(db));
    XASSERT(6 == skDequeSize(dd));

    /* peek at front and back of 'da' */
    err = skDequeBack(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xb);
    err = skDequeFront(da, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xc);

    /* peek at front and back of 'db' */
    err = skDequeBack(db, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xz);
    err = skDequeFront(db, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);

    /* pop three elements from front of 'dd' */
    XASSERT(6 == skDequeSize(dd));
    err = skDequePopFrontNB(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xc);
    err = skDequePopFrontNB(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xa);
    err = skDequePopFrontNB(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xb);

    /* verify 'da' is empty */
    CHECK_EMPTY(da);

    /* pop three elements from front of 'dd' */
    XASSERT(3 == skDequeSize(dd));
    XASSERT(3 == skDequeSize(db));
    err = skDequePopFrontNB(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xx);
    err = skDequePopFrontNB(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xy);
    err = skDequePopFrontNB(dd, (void**)&v);
    XASSERT(err == SKDQ_SUCCESS);
    XASSERT(v == xz);

    /* verify 'db' is empty */
    CHECK_EMPTY(db);

    /* verify 'dd' is empty */
    CHECK_EMPTY(dd);

    /* done with 'dd' */
    skDequeDestroy(dd);

    /* verify 'da' and 'db' are still empty (and valid) */
    CHECK_EMPTY(da);
    CHECK_EMPTY(db);


    /*** Multi-threaded tests ***/

    rv = skthread_create("a", &a, thread_a, NULL);
    XASSERT(rv == 0);
    rv = skthread_create("b", &b, thread_b, NULL);
    XASSERT(rv == 0);
    rv = skthread_create("c", &c, thread_c, NULL);
    XASSERT(rv == 0);

    rv = pthread_join(a, NULL);
    XASSERT(rv == 0);
    rv = pthread_join(b, NULL);
    XASSERT(rv == 0);
    rv = pthread_join(c, NULL);
    XASSERT(rv == 0);

    skDequeDestroy(dc);
    skDequeDestroy(db);
    skDequeDestroy(da);

    sklogClose();
    sklogTeardown();
    skthread_teardown();
    skAppUnregister();

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
