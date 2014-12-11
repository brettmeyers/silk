/*
** Copyright (C) 2001-2014 by Carnegie Mellon University.
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

/* File: hashlib_metrics.c: program for generating performance metrics */

#include <silk/silk.h>

RCSIDENT("$SiLK: hashlib_metrics.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/hashlib.h>


/* NOTE: normally these would not be changed by an application.  This
 * program is meant for tweaking of hashlib parameters. */
extern uint32_t SECONDARY_BLOCK_FRACTION;
extern uint32_t REHASH_BLOCK_COUNT;

/* Description of a test to run */
typedef struct TestDesc_st {
    uint8_t  load_factor;
    int32_t  secondary_block_fraction;
    uint32_t rehash_block_count;
    uint32_t num_entries;
    float    estimate_ratio;
} TestDesc;


static double
get_elapsed_secs(
    struct timeval     *start,
    struct timeval     *end)
{
    double start_time = ((float)start->tv_sec
                         + (((float) start->tv_usec) / 1000000));
    double end_time = (float)end->tv_sec + (((float) end->tv_usec) / 1000000);
    return end_time - start_time;
}


/* Run a test */
static HashTable *
do_test(
    TestDesc           *test_ptr)
{
    uint32_t i;
    uint32_t key;
    uint32_t *val_ptr;
    HashTable *table_ptr;
    uint32_t estimate = (uint32_t)(test_ptr->num_entries
                                   * test_ptr->estimate_ratio);
    struct timeval tv1, tv2;


    fprintf(stderr, "frac = %f, num=%u, estimate=%u\n",
            test_ptr->estimate_ratio, test_ptr->num_entries, estimate);

#ifdef HASHLIB_RECORD_STATS
    hashlib_clear_stats();
#endif
    /* Reconfigure the library */
    SECONDARY_BLOCK_FRACTION = test_ptr->secondary_block_fraction;
    REHASH_BLOCK_COUNT = test_ptr->rehash_block_count;

    fprintf(stderr, " -- BEFORE CREATE TABLE -- \n");
    gettimeofday(&tv1, NULL);

    /* Create the table */
    table_ptr = hashlib_create_table(sizeof(uint32_t),
                                     sizeof(uint32_t),
                                     HTT_INPLACE,  /* values, not pointers */
                                     NULL,         /* all 0 means empty */
                                     NULL, 0,      /* No user data */
                                     estimate,
                                     test_ptr->load_factor);
    gettimeofday(&tv2, NULL);

    fprintf(stderr, " == AFTER create table: ");
    fprintf(stderr, "took %f secs\n", get_elapsed_secs(&tv1,&tv2));

    /* Use the same sequence each time */
    srandom(0);
    for (i=0;i<test_ptr->num_entries;i++) {
      key = random();
      hashlib_insert(table_ptr, (uint8_t*) &key, (uint8_t**) &val_ptr);
      *val_ptr = 1; /* Don't care about value */
    }

    return table_ptr;
}

static double
run_test(
    FILE               *out_fp,
    TestDesc           *test_ptr)
{
    struct timeval start_time, end_time;
    double elapsed_time;
    HashTable *table_ptr;

    /* Run the test */
    fprintf(stderr, "Starting run: ");
    /* Print results */
    fprintf(stderr, "%u\t%u\t%u\t%u\t%f\n",
            test_ptr->load_factor,
            test_ptr->secondary_block_fraction,
            test_ptr->rehash_block_count,
            test_ptr->num_entries,
            test_ptr->estimate_ratio);

    gettimeofday(&start_time, NULL);
    table_ptr = do_test(test_ptr);
    gettimeofday(&end_time, NULL);
    elapsed_time = get_elapsed_secs(&start_time, &end_time);

    /* Clean up after the test */
    hashlib_free_table(table_ptr);
    fprintf(stderr, "Run complete: %f seconds elapsed.\n", elapsed_time);

    /* Print results */
    fprintf(out_fp,
            ("%u\t%3.3f\t%" PRIu64 "\t%u\t%d\t%u\t%3.3f"
#ifdef HASHLIB_RECORD_STATS
             "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%u"
#endif
             "\n"),
            test_ptr->num_entries,
            test_ptr->estimate_ratio,
            (uint64_t) (test_ptr->estimate_ratio * test_ptr->num_entries),
            test_ptr->load_factor,
            test_ptr->secondary_block_fraction,
            test_ptr->rehash_block_count,
            elapsed_time
#ifdef HASHLIB_RECORD_STATS
            , hashlib_stats_inserts,
            hashlib_stats_rehashes,
            hashlib_stats_rehash_inserts,
            hashlib_stats_blocks_allocated
#endif
            );
    fflush(out_fp);

    return elapsed_time;
}


int main()
{
    TestDesc test;
    int32_t fracs[] = { 3, 2, 1, 0, -1, -2 };
    uint32_t block_count[] = {2, 3, 4, 5};
    float ratios[] = { 0.01, 0.125, 0.25, 0.50, 0.75, 0.875, 1.0 };
    uint32_t i, j, k;
    FILE *out_fp;
    FILE *graph_fp;
    double elapsed_time;

    /* Data suitable for graphing, x is ratio, y is time for each set
     * of params */
    graph_fp = fopen("graph.csv", "w");

    /* Write to stdout */
    out_fp = stdout;

    fprintf(out_fp, ("Cnt\tRatio\tEst\tLF\tFrac\tBlks\t"
                     "Time\tIns\tRehsh\tReInst\tAllocs\n"));

    /* Setup test variables */
    test.load_factor = DEFAULT_LOAD_FACTOR;
    test.num_entries = (1<<20);
    test.num_entries = 419430;

    /* Print column headings for graph file */
    fprintf(graph_fp, "Frac\t(%u,%u)\t", 1, 1);
    for (i = 0; i < sizeof(fracs)/sizeof(fracs[0]); ++i) {
        for (j = 0; j < sizeof(block_count)/sizeof(block_count[0]); ++j) {
            fprintf(graph_fp, "(%d,%u)\t",  fracs[i], block_count[j]);
        }
    }
    fprintf(graph_fp, "\n");

    /* Loop through the different combinations */
    for (k = 0; k < sizeof(ratios)/sizeof(ratios[0]); ++k) {
        test.estimate_ratio = ratios[k];

        /* Baseline: one block */
        test.secondary_block_fraction = 1; /* ignored */
        test.rehash_block_count = 1; /* rehash when full */
        elapsed_time = run_test(out_fp, &test);

        fprintf(graph_fp, "%3.4f\t", test.estimate_ratio);
        fprintf(graph_fp, "%3.4f\t", elapsed_time);

        /* Try different combinations of block sizes & counts */
        for (i = 0; i < sizeof(fracs)/sizeof(fracs[0]); ++i) {
            for (j = 0; j < sizeof(block_count)/sizeof(block_count[0]); ++j) {
                /* Adjust test variables */
                test.secondary_block_fraction = fracs[i];
                test.rehash_block_count = block_count[j];
                elapsed_time = run_test(out_fp, &test);
                fprintf(graph_fp, "%3.3f\t", elapsed_time);
            }
        }
        fprintf(graph_fp, "\n");
        fflush(graph_fp);
    }

    fclose(graph_fp);
    return 1;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
