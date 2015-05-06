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
**  Test program for skiobuf.c
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skiobuf-test.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skstream.h>
#include "skiobuf.h"


static char g_data[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int datalen;

#define FAIL do {assert(0); skAbort();} while (0)


static char
rval(
    void)
{
    int i = (int)((float)datalen * (rand() / (RAND_MAX + 1.0)));
    return g_data[i];
}

static char *
create_test_data(
    char              **filename,
    unsigned            recsize,
    unsigned            numrecs)
{
    int fd;
    unsigned i, j;
    ssize_t rv;
    off_t   ov;
    char c;
    void *map;
    char *name = strdup("/tmp/skiobuf-test.XXXXXX");
    char *buf;

    fd = mkstemp(name);
    if (fd == -1) {
        FAIL;
    }
    ov = lseek(fd, recsize * numrecs, SEEK_SET);
    if (ov == -1) {
        FAIL;
    }
    rv = write(fd, "", 1);
    if (rv != 1) {
        FAIL;
    }
    map = mmap(0, recsize * numrecs, PROT_READ | PROT_WRITE, MAP_SHARED,
               fd, 0);
    if (map == MAP_FAILED) {
        FAIL;
    }
    close(fd);

    buf = (char *)map;
    for (i = 0; i < numrecs; i++) {
        c = rval();
        for (j = 0; j < recsize; j++) {
            *buf++ = c;
        }
    }

    *filename = name;
    return (char *)map;
}

static void
delete_test_data(
    char               *name,
    char               *data,
    unsigned            recsize,
    unsigned            numrecs)
{
    munmap(data, recsize * numrecs);
    unlink(name);
    free(name);
}

static void
test(
    int                 method,
    char               *data,
    unsigned            recsize,
    unsigned            numrecs,
    uint32_t            blocksize,
    unsigned            skipafter,
    unsigned            skipfor)
{
    char *name = strdup("/tmp/skiobuf-test.XXXXXX");
    int fd;
    sk_iobuf_t *buf = NULL;
    int rv;
    off_t ov;
    unsigned i, j;
    unsigned skip;
    char *p;
    uint32_t i32;
    ssize_t out;
    off_t off;
    char c;
    int skipping;

    fd = mkstemp(name);
    if (fd == -1) {
        FAIL;
    }

    buf = skIOBufCreateWriter();
    if (buf == NULL) {
        FAIL;
    }

    rv = skIOBufSetRecordSize(buf, recsize);
    if (rv == -1) {
        FAIL;
    }

    rv = skIOBufSetBlockSize(buf, blocksize);
    if (rv == -1) {
        FAIL;
    }

    rv = skIOBufBind(buf, fd, method);
    if (rv == -1) {
        FAIL;
    }

    /* accept a max limit of 1MB */
    i32 = skIOBufUpperCompBlockSize(buf);
    fprintf(stderr, "%" PRIu32 "\n", i32);
    if (i32 > 1024 * 1024) {
        FAIL;
    }

    p = data;
    for (i = 0; i < numrecs; i++) {
        out = skIOBufWrite(buf, p, recsize);
        if (out != (int)recsize) {
            FAIL;
        }
        p += recsize;
    }
    off = skIOBufFlush(buf);
    if (off == -1) {
        FAIL;
    }

    off = skIOBufTotalUpperBound(buf);
    if (off == -1) {
        FAIL;
    }

    skIOBufDestroy(buf);
    buf = NULL;

    ov = lseek(fd, 0, SEEK_SET);
    if (ov == -1) {
        FAIL;
    }

    buf = skIOBufCreateReader();
    if (buf == NULL) {
        FAIL;
    }

    rv = skIOBufBind(buf, fd, method);
    if (rv == -1) {
        FAIL;
    }

    p = data;
    skip = 1;
    skipping = 0;
    for (i = 0; i < numrecs; i++) {
        if (skipping) {
            if (skip == skipfor) {
                skip = 1;
                skipping = 0;
            }
        } else {
            if (skip == skipafter) {
                skip = 1;
                skipping = 1;
            }
        }
        for (j = 0; j < recsize; j++) {
            if (skipping) {
                out = skIOBufRead(buf, NULL, 1);
            } else {
                out = skIOBufRead(buf, &c, 1);
            }
            if (out != 1) {
                FAIL;
            }
            if (!skipping && c != *p) {
                FAIL;
            }
            p++;
        }
        skip++;
    }

    out = skIOBufRead(buf, &c, 1);
    if (out != 0) {
        FAIL;
    }

    skIOBufDestroy(buf);
    buf = NULL;

    close(fd);

    unlink(name);
    free(name);
}

int main()
{
    char *testfname;
    char *testfile;

    datalen = strlen(g_data);
    testfile = create_test_data(&testfname, 10, 100000);

    test(SK_COMPMETHOD_NONE, testfile, 10, 100000, SKIOBUF_DEFAULT_BLOCKSIZE,
         0, 0);
    test(SK_COMPMETHOD_NONE, testfile, 10, 100000, 100, 50, 200);
#if SK_ENABLE_ZLIB
    test(SK_COMPMETHOD_ZLIB, testfile, 10, 100000, SKIOBUF_DEFAULT_BLOCKSIZE,
         0, 0);
    test(SK_COMPMETHOD_ZLIB, testfile, 10, 100000, 100, 50, 200);
#endif
#if SK_ENABLE_LZO
    test(SK_COMPMETHOD_LZO1X, testfile, 10, 100000, SKIOBUF_DEFAULT_BLOCKSIZE,
         0, 0);
    test(SK_COMPMETHOD_LZO1X, testfile, 10, 100000, 100, 50, 200);
#endif

    delete_test_data(testfname, testfile, 10, 100000);

    return 0;
}

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
