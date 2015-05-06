/*
** Copyright (C) 2007-2015 by Carnegie Mellon University.
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
**  skmsg-test.c
**
**    A testing application for the skmsg library.
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skmsg-test.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/utils.h>
#include <silk/sklog.h>
#include <silk/skthread.h>
#include <semaphore.h>
#include "skmsg.h"


/* LOCAL DEFINES AND TYPEDEFS */

#define MATCH(msg, chan, type)                                          \
    (DEBUGMSG("Chan == %d  type == %#x", skMsgChannel(msg), skMsgType(msg)), \
     (skMsgChannel(msg) == (chan) && skMsgType(msg) == (type)))

#define MATCH_TEST(msg, chan, type, test)                               \
    (MATCH(msg, chan, type) && strcmp((char *)skMsgMessage(msg), test) == 0)

#define MATCH_TEST1(msg, chan)                  \
    (MATCH_TEST(msg, chan, type1, test1))

#define MATCH_TEST2(msg, chan)                  \
    (MATCH_TEST(msg, chan, type2, test2))

#define TRYS 10


/* LOCAL VARIABLE DEFINITIONS */

static sem_t *sem1;
static sem_t *sem2;
static sem_t *sem3;
static sem_t *sem4;
static const char *sem_path1 = "skmsg-test-semphore1";
static const char *sem_path2 = "skmsg-test-semphore2";
static const char *sem_path3 = "skmsg-test-semphore3";
static const char *sem_path4 = "skmsg-test-semphore4";

static skm_type_t type1 = 0x100;
static skm_type_t type2 = 0x200;

static const char *test1 = "Test string 1";
static const char *test2 = "Test string 2";


/* FUNCTION DEFINITIONS */


static void *
threada(
    void        UNUSED(*dummy))
{
    int rv;
    sk_sockaddr_t addr;
    sk_sockaddr_array_t addra;
    sk_msg_queue_t *q;
    sk_msg_t *msg, *msg1, *msg2;
    skm_channel_t channel, c2, c3;
    char *str;
    int i;
    int chan[TRYS];

    memset(chan, 0, sizeof(chan));

    memset(&addr, 0, sizeof(addr));
    addr.v4.sin_family = AF_INET;
    addr.v4.sin_port = htons(9999);
    addr.v4.sin_addr.s_addr = htonl(INADDR_ANY);
    addra.name = NULL;
    addra.addrs = &addr;
    addra.num_addrs = 1;

    /* Setup */
    DEBUGMSG("Setup A1");
    rv = skMsgQueueCreate(&q);
    assert(rv == 0);
    rv = skMsgQueueBindTCP(q, &addra);
    assert(rv == 0);
    sem_post(sem1);

    /* Test 1/2 */
    DEBUGMSG("Test 1/2");
    rv = skMsgQueueGetMessage(q, &msg1);
    assert(rv == 0);
    rv = skMsgQueueGetMessage(q, &msg2);
    assert(rv == 0);
    if (MATCH(msg1, SKMSG_CHANNEL_CONTROL, SKMSG_CTL_NEW_CONNECTION)) {
        channel = SKMSG_CTL_MSG_GET_CHANNEL(msg1);
        assert(MATCH_TEST1(msg2, channel));
    } else {
        assert(MATCH(msg2, SKMSG_CHANNEL_CONTROL, SKMSG_CTL_NEW_CONNECTION));
        channel = SKMSG_CTL_MSG_GET_CHANNEL(msg2);
        assert(MATCH_TEST1(msg1, channel));
    }
    skMsgDestroy(msg1);
    skMsgDestroy(msg2);

    /* Test 3 */
    DEBUGMSG("Test 3");
    rv = skMsgQueueSendMessage(q, channel, type2, test2, strlen(test2) + 1);
    assert(rv == 0);

    /* Test 4 */
    DEBUGMSG("Test 4");
    rv = skMsgChannelNew(q, channel, &c2);
    assert(rv == 0);

    /* Test 5 */
    DEBUGMSG("Test 5");
    rv = skMsgQueueGetMessage(q, &msg);
    assert(rv == 0);
    assert(skMsgChannel(msg) == c2);
    assert(skMsgType(msg) == type2);
    str = (char *)skMsgMessage(msg);
    assert(strcmp(str, test2) == 0);
    skMsgDestroy(msg);

    /* Test 6 */
    DEBUGMSG("Test 6");
    rv = skMsgChannelKill(q, channel);
    assert(rv == 0);
    rv = skMsgQueueGetMessage(q, &msg);
    assert(rv == 0);
    assert(skMsgChannel(msg) == SKMSG_CHANNEL_CONTROL);
    assert(skMsgType(msg) == SKMSG_CTL_CHANNEL_DIED);
    assert(skMsgLength(msg) == sizeof(c3));
    c3 = SKMSG_CTL_MSG_GET_CHANNEL(msg);
    assert(c3 == channel);
    skMsgDestroy(msg);

    /* Test 7 */
    DEBUGMSG("Test 7");
    rv = skMsgQueueSendMessage(q, c2, type1, test1, strlen(test1) + 1);
    assert(rv == 0);

    /* Shutdown */
    sem_wait(sem2);
    DEBUGMSG("Shutdown A1");
    skMsgQueueShutdown(q);
    skMsgQueueDestroy(q);

    /* Setup */
    DEBUGMSG("Setup A2");
    rv = skMsgQueueCreate(&q);
    assert(rv == 0);
    rv = skMsgQueueBindTCP(q, &addra);
    assert(rv == 0);
    sem_post(sem3);

    /* Test 8 */
    DEBUGMSG("Test 8");
    for (i = 0; i < TRYS*2; i++) {
        rv = skMsgQueueGetMessage(q, &msg);
        assert(rv == 0);
        if (MATCH(msg, SKMSG_CHANNEL_CONTROL, SKMSG_CTL_NEW_CONNECTION)) {
            channel = SKMSG_CTL_MSG_GET_CHANNEL(msg);
            assert(channel < TRYS);
            chan[channel] += 1;
            assert(chan[channel] <= 3);
        } else {
            assert(MATCH(msg, SKMSG_CHANNEL_CONTROL, SKMSG_CTL_CHANNEL_DIED));
            channel = SKMSG_CTL_MSG_GET_CHANNEL(msg);
            assert(channel < TRYS);
            chan[channel] += 2;
            assert(chan[channel] <= 3);
        }
        skMsgDestroy(msg);
    }
    for (i = 0; i < TRYS; i++) {
        assert(chan[i] == 3);
    }

    /* Shutdown */
    sem_wait(sem4);
    DEBUGMSG("Shutdown A2");
    skMsgQueueShutdown(q);
    skMsgQueueDestroy(q);

    return NULL;
}

static void *
threadb(
    void        UNUSED(*dummy))
{
    int rv;
    sk_msg_queue_t *q;
    struct sockaddr_in addr;
    skm_channel_t channel, c2, c3;
    sk_msg_t *msg1, *msg2;
    int i;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
#ifdef SK_HAVE_INET_PTON
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
#else
    inet_aton("127.0.0.1", &addr.sin_addr);
#endif /* SK_HAVE_INET_PTON */

    /* Setup */
    DEBUGMSG("Setup B1");
    rv = skMsgQueueCreate(&q);
    assert(rv == 0);

    /* Test 1 */
    sem_wait(sem1);
    DEBUGMSG("Test 1");
    rv = skMsgQueueConnectTCP(q, (struct sockaddr *)&addr, sizeof(addr),
                              &channel);
    assert(rv == 0);

    /* Test 2 */
    DEBUGMSG("Test 2");
    rv = skMsgQueueSendMessage(q, channel, type1, test1, strlen(test1) + 1);
    assert(rv == 0);

    /* Test 3/4 */
    DEBUGMSG("Test 3/4");
    rv = skMsgQueueGetMessage(q, &msg1);
    assert(rv == 0);
    rv = skMsgQueueGetMessage(q, &msg2);
    assert(rv == 0);
    if (MATCH_TEST2(msg1, channel)) {
        assert(MATCH(msg2, SKMSG_CHANNEL_CONTROL, SKMSG_CTL_NEW_CONNECTION));
        c2 = SKMSG_CTL_MSG_GET_CHANNEL(msg2);
    } else {
        assert(MATCH_TEST2(msg2, channel));
        assert(MATCH(msg1, SKMSG_CHANNEL_CONTROL, SKMSG_CTL_NEW_CONNECTION));
        c2 = SKMSG_CTL_MSG_GET_CHANNEL(msg1);
    }
    skMsgDestroy(msg1);
    skMsgDestroy(msg2);

    /* Test 5 */
    DEBUGMSG("Test 5");
    rv = skMsgQueueSendMessage(q, c2, type2, test2, strlen(test2) + 1);
    assert(rv == 0);

    /* Test 6/7 */
    DEBUGMSG("Test 6/7");
    rv = skMsgQueueGetMessage(q, &msg1);
    assert(rv == 0);
    rv = skMsgQueueGetMessage(q, &msg2);
    assert(rv == 0);
    if (MATCH_TEST1(msg1, c2)) {
        assert(MATCH(msg2, SKMSG_CHANNEL_CONTROL, SKMSG_CTL_CHANNEL_DIED));
        c3 = SKMSG_CTL_MSG_GET_CHANNEL(msg2);
    } else {
        assert(MATCH_TEST1(msg2, c2));
        assert(MATCH(msg1, SKMSG_CHANNEL_CONTROL, SKMSG_CTL_CHANNEL_DIED));
        c3 = SKMSG_CTL_MSG_GET_CHANNEL(msg1);
    }
    assert(c3 == channel);
    skMsgDestroy(msg1);
    skMsgDestroy(msg2);
    sem_post(sem2);

    /* Shutdown */
    DEBUGMSG("Shutdown B1");
    skMsgQueueShutdown(q);
    skMsgQueueDestroy(q);

    /* Setup */
    sem_wait(sem3);

    /* Test 8 */
    for (i = 0; i < TRYS; i++) {
        DEBUGMSG("Setup B2/%d", i);
        rv = skMsgQueueCreate(&q);
        assert(rv == 0);

        DEBUGMSG("Test 8/%d", i);
        rv = skMsgQueueConnectTCP(q, (struct sockaddr *)&addr, sizeof(addr),
                                  &channel);
        assert(rv == 0);

        DEBUGMSG("Shutdown B1/%d", i);
        skMsgQueueShutdown(q);
        skMsgQueueDestroy(q);
    }
    sem_post(sem4);

    return NULL;
}


int main(int UNUSED(argc), char **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);
    int rv;
    int status;
    int retval;
    pid_t pa, pb, pr;

    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skthread_init("main");

    sklogSetup(0);
    sklogSetDestination("stderr");
    sklogSetLevel("debug");
    sklogOpen();

    sem1 = sem_open(sem_path1, O_CREAT, 0600, 1);
    assert(sem1 != (sem_t*)(SEM_FAILED));
    sem2 = sem_open(sem_path2, O_CREAT, 0600, 1);
    assert(sem2 != (sem_t*)(SEM_FAILED));
    sem3 = sem_open(sem_path3, O_CREAT, 0600, 1);
    assert(sem3 != (sem_t*)(SEM_FAILED));
    sem4 = sem_open(sem_path4, O_CREAT, 0600, 1);
    assert(sem4 != (sem_t*)(SEM_FAILED));

    sem_wait(sem1);
    sem_wait(sem2);
    sem_wait(sem3);
    sem_wait(sem4);

    pa = fork();
    assert(pa != -1);
    if (pa == 0) {
        threada(NULL);
        sem_close(sem1);
        sem_close(sem2);
        sem_close(sem3);
        sem_close(sem4);
        INFOMSG("EXIT: A");
        return 0;
    }

    pb = fork();
    assert(pb != -1);
    if (pb == 0) {
        threadb(NULL);
        sem_close(sem1);
        sem_close(sem2);
        sem_close(sem3);
        sem_close(sem4);
        INFOMSG("EXIT: B");
        return 0;
    }

    pr = waitpid(pa, &status, 0);
    if (WIFEXITED(status)) {
        retval = WEXITSTATUS(status);
        INFOMSG("Thread A exited %d", retval);
    } else {
        if (WIFSIGNALED(status)) {
            INFOMSG("Thread A died signal %d", WTERMSIG(status));
        }
        retval = EXIT_FAILURE;
    }
    pr = waitpid(pb, &status, 0);
    if (retval == EXIT_SUCCESS) {
        if (WIFEXITED(status)) {
            retval = WEXITSTATUS(status);
            INFOMSG("Thread B exited %d", retval);
        } else {
            if (WIFSIGNALED(status)) {
                INFOMSG("Thread B died signal %d", WTERMSIG(status));
            }
            retval = EXIT_FAILURE;
        }
    }

    sem_close(sem1);
    sem_close(sem2);
    sem_close(sem3);
    sem_close(sem4);
    rv = sem_unlink(sem_path1);
    assert(rv == 0);
    rv = sem_unlink(sem_path2);
    assert(rv == 0);
    rv = sem_unlink(sem_path3);
    assert(rv == 0);
    rv = sem_unlink(sem_path4);
    assert(rv == 0);

    sklogClose();
    sklogTeardown();
    skthread_teardown();
    skAppUnregister();

    return retval;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
