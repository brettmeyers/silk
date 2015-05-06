/*
** Copyright (C) 2010-2015 by Carnegie Mellon University.
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
**  skmempool-test.c
**
**  Test program for the memory pool allocator.
**
*/

#include <silk/silk.h>

RCSIDENTVAR(rcs_SKMEMPOOL_TEST, "$SiLK: skmempool-test.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/* Note: including C file here */
#include <skmempool.c>

/* LOCAL DEFINES AND TYPEDEFS */

#define KEEP_COUNT 100
#define ELEMS_PER_BLOCK 10


typedef struct test1_st {
    uint64_t a;
    uint32_t b;
} test1_t;


typedef struct test2_st {
    uint32_t a;
    uint32_t b;
    uint32_t c;
} test2_t;


/* LOCAL VARIABLES */

static test1_t *test1_array[KEEP_COUNT];
static const test1_t empty1 = {0, 0};
static const test1_t redzone1 = {UINT64_C(0xaaaaaaaaaaaaaaaa), 0};

static test2_t *test2_array[KEEP_COUNT];
static const test2_t empty2 = {0, 0, 0};
static const test2_t redzone2 = {UINT32_C(0xaaaaaaaa), 0,UINT32_C(0x55555555)};


#define MY_ASSERT(expr)                                                 \
    if (expr) { } else {                                                \
        fprintf(stderr, "Assertion failed: (%s), file %s, line %d\n",   \
                #expr, __FILE__, __LINE__);                             \
        skAbort();                                                      \
    }

#define ASSERT_IS_EMPTY_1(vp)                                   \
    MY_ASSERT(0 == memcmp((vp), &empty1, sizeof(empty1)))

/* set value of test1_t including the red-zones */
#define SET_VALUE_1(vp, val)                    \
    do {                                        \
        (vp)->a = redzone1.a;                   \
        (vp)->b = (val);                        \
    } while(0)

/* print value of test1_t and check the red-zones */
#define PRINT_VALUE_1(vp)                       \
    do {                                        \
        MY_ASSERT(redzone1.a == (vp)->a);       \
        printf(("%" PRIu32 " "), (vp)->b);      \
    } while(0)


#define ASSERT_IS_EMPTY_2(vp)                                   \
    MY_ASSERT(0 == memcmp((vp), &empty2, sizeof(empty2)))

/* set value of test1_t including the red-zones */
#define SET_VALUE_2(vp, val)                    \
    do {                                        \
        (vp)->a = redzone2.a;                   \
        (vp)->c = redzone2.c;                   \
        (vp)->b = (val);                        \
    } while(0)

/* print value of test1_t and check the red-zones */
#define PRINT_VALUE_2(vp)                       \
    do {                                        \
        MY_ASSERT(redzone2.a == (vp)->a);       \
        MY_ASSERT(redzone2.c == (vp)->c);       \
        printf(("%" PRIu32 " "), (vp)->b);      \
    } while(0)


/* FUNCTION DEFINITIONS */

static void
print_test1_array(
    void)
{
    uint32_t i;

    /* print all values in test1_array array */
    for (i = 0; i < KEEP_COUNT; ++i) {
        PRINT_VALUE_1(test1_array[i]);
    }
    printf("\n");
}


static void
print_test1_blocks(
    const sk_mempool_t *p)
{
    const sk_mempool_block_t *block;
    const test1_t *tmp;
    uint32_t i;
    uint32_t j;

    for (block = p->block_list, i = 0;
         block != NULL;
         block = block->next.p, ++i)
    {
        printf("block %3u:  ", i);
        for (j = 0, tmp = (test1_t*)(block->element_data);
             j < ELEMS_PER_BLOCK;
             ++j, ++tmp)
        {
            PRINT_VALUE_1(tmp);
        }
        printf("\n");
    }

}


static void
run_test1(
    void)
{
    sk_mempool_t *p;
    test1_t *tmp;
    uint32_t i;
    uint32_t j;

    memset(test1_array, 0, sizeof(test1_array));

    /* create the pool */
    if (skMemoryPoolCreate(&p, sizeof(test1_t), ELEMS_PER_BLOCK)) {
        exit(EXIT_FAILURE);
    }

    /* on each loop get two test1_t's from the pool, one is used
     * as a placeholder---we "lose" the reference to it---and the
     * other gets added to the test1_array array. */
    for (i = 0, j = 2 * KEEP_COUNT; i < KEEP_COUNT; ++i, ++j) {
        tmp = (test1_t*)skMemPoolElementNew(p);
        ASSERT_IS_EMPTY_1(tmp);
        SET_VALUE_1(tmp, i);

        test1_array[i] = (test1_t*)skMemPoolElementNew(p);
        ASSERT_IS_EMPTY_1(test1_array[i]);
        SET_VALUE_1(test1_array[i], j);
    }

    /* print all IPs in test1_array array */
    print_test1_array();

    /* look at the blocks inside the pool */
    print_test1_blocks(p);

    /* "free" the elements we have access to; that is, add them back
     * to the pool. */
    for (i = 0; i < KEEP_COUNT; ++i) {
        skMemPoolElementFree(p, test1_array[i]);
    }

    /* get elements from the pool.  these should be the same elements
     * we had on the first pass */
    for (i = 0, j = 3 * KEEP_COUNT; i < KEEP_COUNT; ++i, ++j) {
        test1_array[i] = (test1_t*)skMemPoolElementNew(p);
        ASSERT_IS_EMPTY_1(test1_array[i]);
        SET_VALUE_1(test1_array[i], j);
    }

    /* print all IPs in test1_array array */
    print_test1_array();

    /* look at the blocks inside the pool */
    print_test1_blocks(p);

    /* destroy the pool */
    skMemoryPoolDestroy(&p);
}



static void
print_test2_array(
    void)
{
    uint32_t i;

    /* print all values in test2_array array */
    for (i = 0; i < KEEP_COUNT; ++i) {
        PRINT_VALUE_2(test2_array[i]);
    }
    printf("\n");
}


static void
print_test2_blocks(
    const sk_mempool_t *p)
{
    const sk_mempool_block_t *block;
    const test2_t *tmp;
    uint32_t i;
    uint32_t j;

    for (block = p->block_list, i = 0;
         block != NULL;
         block = block->next.p, ++i)
    {
        printf("block %3u:  ", i);
        for (j = 0, tmp = (test2_t*)(block->element_data);
             j < ELEMS_PER_BLOCK;
             ++j, ++tmp)
        {
            PRINT_VALUE_2(tmp);
        }
        printf("\n");
    }

}


static void
run_test2(
    void)
{
    sk_mempool_t *p;
    test2_t *tmp;
    uint32_t i;
    uint32_t j;

    memset(test2_array, 0, sizeof(test2_array));

    /* create the pool */
    if (skMemoryPoolCreate(&p, sizeof(test2_t), ELEMS_PER_BLOCK)) {
        exit(EXIT_FAILURE);
    }

    /* on each loop get two test2_t's from the pool, one is used
     * as a placeholder---we "lose" the reference to it---and the
     * other gets added to the test2_array array. */
    for (i = 0, j = 6 * KEEP_COUNT; i < KEEP_COUNT; ++i, ++j) {
        tmp = (test2_t*)skMemPoolElementNew(p);
        ASSERT_IS_EMPTY_2(tmp);
        SET_VALUE_2(tmp, i);

        test2_array[i] = (test2_t*)skMemPoolElementNew(p);
        ASSERT_IS_EMPTY_2(test2_array[i]);
        SET_VALUE_2(test2_array[i], j);
    }

    /* print all IPs in test2_array array */
    print_test2_array();

    /* look at the blocks inside the pool */
    print_test2_blocks(p);

    /* "free" the elements we have access to; that is, add them back
     * to the pool. */
    for (i = 0; i < KEEP_COUNT; ++i) {
        skMemPoolElementFree(p, test2_array[i]);
    }

    /* get elements from the pool.  these should be the same elements
     * we had on the first pass */
    for (i = 0, j = 7 * KEEP_COUNT; i < KEEP_COUNT; ++i, ++j) {
        test2_array[i] = (test2_t*)skMemPoolElementNew(p);
        ASSERT_IS_EMPTY_2(test2_array[i]);
        SET_VALUE_2(test2_array[i], j);
    }

    /* print all IPs in test2_array array */
    print_test2_array();

    /* look at the blocks inside the pool */
    print_test2_blocks(p);

    /* destroy the pool */
    skMemoryPoolDestroy(&p);
}


int main(int UNUSED(argc), char **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);

    run_test1();
    run_test2();

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
