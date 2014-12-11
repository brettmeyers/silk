/*
** Copyright (C) 2014 by Carnegie Mellon University.
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
 *    1.  rwcombine reads SiLK Flow Records from the standard input or
 *    from named files, examines the ATTRIBUTES field, and immediately
 *    writes to the destination stream all records where both the
 *    time-out flag (T) and the continuation flag (C) field are not
 *    set.  Records where one or both of those flags are set are
 *    stored until all input records have been read.
 *
 *    2.  rwcombine groups the stored records into bins where, by
 *    default, the five tuple and several additional fields on the
 *    records, are identical for all records in the bin.
 *
 *    3.  For each bin, the records are stored by time (sTime and
 *    elapsed).
 *
 *    4.  Within a bin, rwcombine combines two records into a single
 *    record when the attributes field of the first record has the T
 *    (time-out) flag set and the second record has the C
 *    (continuation) flag set.
 *
 *    5.  If the second record's T flag was set, rwcombine checks to
 *    see if the third record's C flag is set.  If it is, the third
 *    record becomes part of the new record.
 *
 *    6.  The previous step repeats for the records in the bin until
 *    the bin contains a single record, the most recently added record
 *    did not have the T flag set, the next record in the bin does not
 *    have the C flag set.
 *
 *    7.  After examining a bin, rwcombine writes the record(s) the
 *    bin contains to the destination stream.
 *
 *    8.  Steps 3 through 7 are repeated for each bin.
 *
 *    For sorting the records, rwcombine uses the same methods as
 *    rwsort and rwdedupe.  See those files for information on the
 *    DEFAULT_BUFFER_SIZE, MAX_MERGE_FILES, and use of the temporary
 *    directory.  Since, in most cases, the number of records
 *    rwcombine needs to sort is relatively small, these are less of a
 *    consideration than in rwsort and rwdedupe that sort all of their
 *    input.
 *
 */

#include <silk/silk.h>

RCSIDENT("$SiLK: rwcombine.c 6dc464c27dd9 2014-09-23 13:15:24Z mthomas $");

#include "rwcombine.h"
#include <silk/skheap.h>


/* TYPEDEFS AND DEFINES */

#define TIMEOUT_MASK (SK_TCPSTATE_TIMEOUT_KILLED|SK_TCPSTATE_TIMEOUT_STARTED)


/* EXPORTED VARIABLES */

/* number of fields to sort over */
uint32_t num_fields = 0;

/* IDs of the fields to sort over; values are from the
 * rwrec_printable_fields_t enum. */
uint32_t sort_fields[RWREC_PRINTABLE_FIELD_COUNT];

/* output stream */
skstream_t *out_stream = NULL;

/* statistics stream */
skstream_t *print_statistics = NULL;

/* temp file context */
sk_tempfilectx_t *tmpctx;

/* maximum amount of RAM to attempt to allocate */
uint64_t buffer_size = DEFAULT_BUFFER_SIZE;

/* maximum amount of idle time to allow between flows */
int64_t max_idle_time = INT64_MAX;


/* LOCAL VARIABLES */

struct counts_st {
    uint64_t    in;
    uint64_t    out;
    uint64_t    unfrag;
    uint64_t    miss_start_end;
    uint64_t    miss_start;
    uint64_t    miss_end;
    uint64_t    combined;
    uint64_t    min_idle;
    uint64_t    max_idle;
    uint64_t    penult_idle;
} counts;


/* FUNCTION DEFINITIONS */

/*
 *    Individually invoke 'func' on the rwRec pointers 'rec_a' and
 *    'rec_b' and compare the results for sorting.
 */
#define RETURN_IF_SORTED(func, rec_a, rec_b)    \
    if (func((const rwRec*)(rec_a))             \
        < func((const rwRec*)(rec_b)))          \
    {                                           \
        return -1;                              \
    } else if (func((const rwRec*)(rec_a))      \
               > func((const rwRec*)(rec_b)))   \
    {                                           \
        return 1;                               \
    }

/*
 *    Individually invoke 'func' on the rwRec pointers 'rec_a' and
 *    'rec_b' and compare the results for sorting.  Treat as
 *    equivalent if the difference is less than 'delta'.
 */
#define RETURN_IF_SORTED_DELTA(func, rec_a, rec_b, delta)       \
    if ((delta) == 0) {                                         \
        RETURN_IF_SORTED(func, rec_a, rec_b);                   \
    } else if (func((const rwRec*)(rec_a))                      \
               < func((const rwRec*)(rec_b)))                   \
    {                                                           \
        if ((delta) < (func((const rwRec*)(rec_b))              \
                       - func((const rwRec*)(rec_a))))          \
        {                                                       \
            return -1;                                          \
        }                                                       \
    } else if ((delta) < (func((const rwRec*)(rec_a))           \
                          - func((const rwRec*)(rec_b))))       \
    {                                                           \
        return 1;                                               \
    }

#if !SK_ENABLE_IPV6
#define compareIPs(ipa, ipb)                            \
    ((skipaddrGetV4(ipa) < skipaddrGetV4(ipb))          \
     ? -1                                               \
     : (skipaddrGetV4(ipa) > skipaddrGetV4(ipb)))
#endif

#define RETURN_IF_SORTED_IPS(func, rec_a, rec_b)        \
    {                                                   \
        skipaddr_t ip_a, ip_b;                          \
        int cmp;                                        \
        func((rwRec*)(rec_a), &ip_a);                   \
        func((rwRec*)(rec_b), &ip_b);                   \
        cmp = compareIPs(&ip_a, &ip_b);                 \
        if (cmp != 0) {                                 \
            return cmp;                                 \
        }                                               \
    }


#if SK_ENABLE_IPV6
static int
compareIPs(
    const skipaddr_t   *ipa,
    const skipaddr_t   *ipb)
{
    uint8_t ipa_v6[16];
    uint8_t ipb_v6[16];

    if (skipaddrIsV6(ipa)) {
        if (!skipaddrIsV6(ipb)) {
            /* treat the IPv4 'ipb' as less than IPv6 */
            return 1;
        }
        /* both are IPv6 */
        skipaddrGetV6(ipa, ipa_v6);
        skipaddrGetV6(ipb, ipb_v6);
        return memcmp(ipa_v6, ipb_v6, sizeof(ipa_v6));
    }
    if (skipaddrIsV6(ipb)) {
        return -1;
    }
    /* both are IPv4 */
    return ((skipaddrGetV4(ipa) < skipaddrGetV4(ipb))
            ? -1
            : (skipaddrGetV4(ipa) > skipaddrGetV4(ipb)));
}
#endif  /* SK_ENABLE_IPV6 */


static char *
timediff_str(
    char               *buf,
    size_t              bufsiz,
    uint64_t            timediff)
{
    uint64_t milli;
    uint64_t sec;
    uint64_t min;
    uint64_t hour;

    milli = timediff % 1000;
    timediff = (timediff - milli) / 1000;
    sec = timediff % 60;
    timediff = (timediff - sec) / 60;
    min = timediff % 60;
    timediff = (timediff - min) / 60;
    hour = timediff % 24;
    timediff = (timediff - hour) / 24;

    snprintf(buf, bufsiz, ("%" PRIu64 ":%02" PRIu64 ":%02" PRIu64
                           ":%02" PRIu64 ".%03" PRIu64),
             timediff, hour, min, sec, milli);
    return buf;
}


/*
 *  rwrecCompare(a, b);
 *
 *     Returns an ordering on the recs pointed to `a' and `b' by
 *     comparing the fields listed in the sort_fields[] array.
 */
static int
rwrecCompare(
    const void         *a,
    const void         *b)
{
    uint32_t i;

    if (0 == num_fields) {
        return memcmp(a, b, NODE_SIZE);
    }

    for (i = 0; i < num_fields; ++i) {
        switch (sort_fields[i]) {
          case RWREC_FIELD_SIP:
            RETURN_IF_SORTED_IPS(rwRecMemGetSIP, a, b);
            break;

          case RWREC_FIELD_DIP:
            RETURN_IF_SORTED_IPS(rwRecMemGetDIP, a, b);
            break;

          case RWREC_FIELD_NHIP:
            RETURN_IF_SORTED_IPS(rwRecMemGetNhIP, a, b);
            break;

          case RWREC_FIELD_SPORT:
            RETURN_IF_SORTED(rwRecGetSPort, a, b);
            break;

          case RWREC_FIELD_DPORT:
            RETURN_IF_SORTED(rwRecGetDPort, a, b);
            break;

          case RWREC_FIELD_PROTO:
            RETURN_IF_SORTED(rwRecGetProto, a, b);
            break;

          case RWREC_FIELD_STIME:
            RETURN_IF_SORTED(rwRecGetStartTime, a, b);
            break;

          case RWREC_FIELD_ELAPSED:
            RETURN_IF_SORTED(rwRecGetElapsed, a, b);
            break;

          case RWREC_FIELD_SID:
            RETURN_IF_SORTED(rwRecGetSensor, a, b);
            break;

          case RWREC_FIELD_INPUT:
            RETURN_IF_SORTED(rwRecGetInput, a, b);
            break;

          case RWREC_FIELD_OUTPUT:
            RETURN_IF_SORTED(rwRecGetOutput, a, b);
            break;

          case RWREC_FIELD_APPLICATION:
            RETURN_IF_SORTED(rwRecGetApplication, a, b);
            break;

          case RWREC_FIELD_FTYPE_CLASS:
          case RWREC_FIELD_FTYPE_TYPE:
            RETURN_IF_SORTED(rwRecGetFlowType, a, b);
            break;

          case RWREC_FIELD_ETIME:
          case RWREC_FIELD_STIME_MSEC:
          case RWREC_FIELD_ETIME_MSEC:
          case RWREC_FIELD_ELAPSED_MSEC:
          case RWREC_FIELD_ICMP_TYPE:
          case RWREC_FIELD_ICMP_CODE:
          case RWREC_FIELD_PKTS:
          case RWREC_FIELD_BYTES:
          case RWREC_FIELD_FLAGS:
          case RWREC_FIELD_INIT_FLAGS:
          case RWREC_FIELD_REST_FLAGS:
          case RWREC_FIELD_TCP_STATE:
            skAbortBadCase(sort_fields[i]);
        }
    }

    return 0;
}


static int
rwrecCombine(
    rwRec              *rec1,
    const rwRec        *rec2)
{
    sktime_t sTime1, sTime2;
    sktime_t eTime1, eTime2;
    uint32_t bytes1, bytes2;
    uint32_t pkts1, pkts2;
    double idle_time;
    size_t i;

    /* First record must have been killed by an active timeout */
    if (!(rwRecGetTcpState(rec1) & SK_TCPSTATE_TIMEOUT_KILLED)) {
        return -1;
    }
    /* Second record must be marked as a continuation record */
    if (!(rwRecGetTcpState(rec2) & SK_TCPSTATE_TIMEOUT_STARTED)) {
        return -1;
    }

    /* All fields other than time must be identical */
    for (i = 0; i < num_fields; ++i) {
        switch (sort_fields[i]) {
          case RWREC_FIELD_STIME:
          case RWREC_FIELD_ELAPSED:
            break;

          case RWREC_FIELD_SIP:
            RETURN_IF_SORTED_IPS(rwRecMemGetSIP, rec1, rec2);
            break;

          case RWREC_FIELD_DIP:
            RETURN_IF_SORTED_IPS(rwRecMemGetDIP, rec1, rec2);
            break;

          case RWREC_FIELD_NHIP:
            RETURN_IF_SORTED_IPS(rwRecMemGetNhIP, rec1, rec2);
            break;

          case RWREC_FIELD_SPORT:
            RETURN_IF_SORTED(rwRecGetSPort, rec1, rec2);
            break;

          case RWREC_FIELD_DPORT:
            RETURN_IF_SORTED(rwRecGetDPort, rec1, rec2);
            break;

          case RWREC_FIELD_PROTO:
            RETURN_IF_SORTED(rwRecGetProto, rec1, rec2);
            break;

          case RWREC_FIELD_SID:
            RETURN_IF_SORTED(rwRecGetSensor, rec1, rec2);
            break;

          case RWREC_FIELD_INPUT:
            RETURN_IF_SORTED(rwRecGetInput, rec1, rec2);
            break;

          case RWREC_FIELD_OUTPUT:
            RETURN_IF_SORTED(rwRecGetOutput, rec1, rec2);
            break;

          case RWREC_FIELD_APPLICATION:
            RETURN_IF_SORTED(rwRecGetApplication, rec1, rec2);
            break;

          case RWREC_FIELD_FTYPE_CLASS:
          case RWREC_FIELD_FTYPE_TYPE:
            RETURN_IF_SORTED(rwRecGetFlowType, rec1, rec2);
            break;

          case RWREC_FIELD_ETIME:
          case RWREC_FIELD_STIME_MSEC:
          case RWREC_FIELD_ETIME_MSEC:
          case RWREC_FIELD_ELAPSED_MSEC:
          case RWREC_FIELD_ICMP_TYPE:
          case RWREC_FIELD_ICMP_CODE:
          case RWREC_FIELD_PKTS:
          case RWREC_FIELD_BYTES:
          case RWREC_FIELD_FLAGS:
          case RWREC_FIELD_INIT_FLAGS:
          case RWREC_FIELD_REST_FLAGS:
          case RWREC_FIELD_TCP_STATE:
            skAbortBadCase(sort_fields[i]);
        }
    }

    sTime1 = rwRecGetStartTime(rec1);
    sTime2 = rwRecGetStartTime(rec2);
    eTime1 = rwRecGetEndTime(rec1);
    eTime2 = rwRecGetEndTime(rec2);

    idle_time = (double)sTime2 - (double)eTime1;
    if (idle_time > max_idle_time) {
        return -1;
    }

    /* check for overflow in duration field */
    if ((sTime1 < eTime2)
        && (eTime2 - sTime1 > UINT32_MAX))
    {
        return -1;
    }

    /* Checking for overflow on bytes */
    bytes1 = rwRecGetBytes(rec1);
    bytes2 = rwRecGetBytes(rec2);
    if (UINT32_MAX - bytes1 < bytes2) {
        return -1;
    }

    /* Checking for overflow on pkts */
    pkts1 = rwRecGetPkts(rec1);
    pkts2 = rwRecGetPkts(rec2);
    if (UINT32_MAX - pkts1 < pkts2) {
        return -1;
    }

    /* If rec2 timed-out, maintain the timeout flag; otherwise reset
     * it */
    if (!(rwRecGetTcpState(rec2) & SK_TCPSTATE_TIMEOUT_KILLED)) {
        rwRecSetTcpState(rec1, (rwRecGetTcpState(rec1)
                                & ~SK_TCPSTATE_TIMEOUT_KILLED));
    }

    /* Update TCP flags */
    rwRecSetFlags(rec1, (rwRecGetFlags(rec1) | rwRecGetFlags(rec2)));

    /* Use InitFlags() and RestFlags() on rec2 so that the
     * SK_TCPSTATE_EXPANDED bit is not violated */
    rwRecSetRestFlags(rec1,
                      (rwRecGetRestFlags(rec1)
                       | rwRecGetRestFlags(rec2) | rwRecGetInitFlags(rec2)));

    /* A bug in YAF resets the S-attribute on the last flow fragment
     * if the number of packets in the flow fragment is 1. */
    if (rwRecGetTcpState(rec1) & SK_TCPSTATE_UNIFORM_PACKET_SIZE) {
        if (rwRecGetTcpState(rec2) & SK_TCPSTATE_UNIFORM_PACKET_SIZE) {
            if ((bytes1 / pkts1) != (bytes2 / pkts2)) {
                /* different byte-per-pkt ratios */
                rwRecSetTcpState(rec1, (rwRecGetTcpState(rec1)
                                        & ~SK_TCPSTATE_UNIFORM_PACKET_SIZE));
            }
            /* else ratios are the same, leave the flag enabled */
        } else if ((pkts2 > 1) || ((bytes1 / pkts1) != bytes2)) {
            /* different byte-per-pkt ratios */
            rwRecSetTcpState(rec1, (rwRecGetTcpState(rec1)
                                    & ~SK_TCPSTATE_UNIFORM_PACKET_SIZE));
        }
        /* else rec2 has a single packet and the bpp ratio is
         * consistent with that on rec1, so leave flag enabled */
    } else if ((rwRecGetTcpState(rec2) & SK_TCPSTATE_UNIFORM_PACKET_SIZE)
               && 1 == pkts1)
    {
        /* check whether byte-per-pkt ratio on rec1 is consistent with
         * that on rec2 */
        if ((bytes2 / pkts2) == bytes1) {
            /* enable flag on rec1 */
            rwRecSetTcpState(rec1, (rwRecGetTcpState(rec1)
                                    | SK_TCPSTATE_UNIFORM_PACKET_SIZE));
        }
    } else if (1 == pkts1 && 1 == pkts2 && bytes1 == bytes2) {
        /* enable flag for two single packet flows with same byte
         * count */
        rwRecSetTcpState(rec1, (rwRecGetTcpState(rec1)
                                | SK_TCPSTATE_UNIFORM_PACKET_SIZE));
    }

    /* Update duration */
    rwRecSetElapsed(rec1, eTime2 - sTime1);

    /* Update byte and packet counts */
    rwRecSetBytes(rec1, bytes1 + bytes2);
    rwRecSetPkts(rec1, pkts1 + pkts2);

    /* idle times */
    if (idle_time < counts.min_idle) {
        if (idle_time < 0.0) {
        }
        if (counts.min_idle == UINT64_MAX) {
            counts.max_idle = (uint64_t)idle_time;
        }
        counts.min_idle = (uint64_t)idle_time;
    } else if (counts.max_idle < idle_time) {
        counts.penult_idle = counts.max_idle;
        counts.max_idle = (uint64_t)idle_time;
    }

    return 0;
}

/*
 *  status = compHeapNodes(b, a, v_recs);
 *
 *    Callback function used by the heap to compare two heapnodes,
 *    which are just indexes into an array of records.  'v_recs' is
 *    the array of records, where each record is NODE_SIZE bytes.
 *
 *    Note the order of arguments is 'b', 'a'.
 */
static int
compHeapNodes(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_recs)
{
    uint8_t *recs = (uint8_t*)v_recs;

    return rwrecCompare(&recs[*(uint16_t*)a * NODE_SIZE],
                        &recs[*(uint16_t*)b * NODE_SIZE]);
}


/*
 *  mergeFiles(temp_file_idx)
 *
 *    Merge the temporary files numbered from 0 to 'temp_file_idx'
 *    inclusive into the output file 'out_ios', maintaining sorted
 *    order.  Exits the application if an error occurs.
 */
static void
mergeFiles(
    int                 temp_file_idx)
{
    FILE *fps[MAX_MERGE_FILES];
    uint8_t recs[MAX_MERGE_FILES][NODE_SIZE];
    uint8_t lowest_rec[NODE_SIZE];
    int j;
    uint16_t open_count;
    uint16_t i;
    uint16_t lowest;
    uint16_t *top_heap;
    int tmp_idx_a;
    int tmp_idx_b;
    FILE *fp_intermediate = NULL;
    int tmp_idx_intermediate;
    skheap_t *heap;
    int heap_status;
    int no_more_temps = 0;
    int rv;

    TRACEMSG(("Merging #%d through #%d into '%s'",
              0, temp_file_idx, skStreamGetPathname(out_stream)));

    heap = skHeapCreate2(compHeapNodes, MAX_MERGE_FILES, sizeof(uint16_t),
                         NULL, recs);
    if (NULL == heap) {
        skAppPrintOutOfMemory("heap");
        appExit(EXIT_FAILURE);
    }

    /* the index of the first temp file to the merge */
    tmp_idx_a = 0;

    /* This loop repeats as long as we haven't read all of the temp
     * files generated in the sorting stage. */
    do {
        assert(SKHEAP_ERR_EMPTY==skHeapPeekTop(heap,(skheapnode_t*)&top_heap));

        /* the index of the last temp file to merge */
        tmp_idx_b = temp_file_idx;

        /* open an intermediate temp file.  The merge-sort will have
         * to write records here if there are not enough file handles
         * available to open all the existing tempoary files. */
        fp_intermediate = skTempFileCreate(tmpctx, &tmp_idx_intermediate,NULL);
        if (fp_intermediate == NULL) {
            skAppPrintSyserror("Error creating new temporary file");
            appExit(EXIT_FAILURE);
        }

        /* count number of files we open */
        open_count = 0;

        /* Attempt to open up to MAX_MERGE_FILES, though we an open
         * may fail due to lack of resources (EMFILE or ENOMEM) */
        for (j = tmp_idx_a; j <= tmp_idx_b; ++j) {
            fps[open_count] = skTempFileOpen(tmpctx, j);
            if (fps[open_count] == NULL) {
                if ((open_count > 0)
                    && ((errno == EMFILE) || (errno == ENOMEM)))
                {
                    /* Blast!  We can't open any more temp files.  So,
                     * we rewind by one to catch this one the next
                     * time around. */
                    tmp_idx_b = j - 1;
                    TRACEMSG((("EMFILE limit hit--"
                               "merging #%d through #%d into #%d"),
                              tmp_idx_a, tmp_idx_b, tmp_idx_intermediate));
                    break;
                } else {
                    skAppPrintSyserror(("Error opening existing"
                                        " temporary file '%s'"),
                                       skTempFileGetName(tmpctx, j));
                    appExit(EXIT_FAILURE);
                }
            }

            /* read the first record */
            if (!fread(recs[open_count], NODE_SIZE, 1, fps[open_count])) {
                if (feof(fps[open_count])) {
                    TRACEMSG(("Ignoring empty temporary file '%s'",
                              skTempFileGetName(tmpctx, j)));
                    continue;
                }
                skAppPrintSyserror(("Error reading first record from"
                                    " temporary file '%s'"),
                                   skTempFileGetName(tmpctx, j));
                appExit(EXIT_FAILURE);
            }

            /* insert the file index into the heap */
            skHeapInsert(heap, &open_count);

            ++open_count;
            if (open_count == MAX_MERGE_FILES) {
                /* We've reached the limit for this pass.  Set
                 * tmp_idx_b to the file we just opened. */
                tmp_idx_b = j;
                TRACEMSG((("MAX_MERGE_FILES limit hit--"
                           "merging #%d through #%d to #%d"),
                          tmp_idx_a, tmp_idx_b, tmp_idx_intermediate));
                break;
            }
        }

        /* Here, we check to see if we've opened all temp files.  If
         * so, set a flag so we write data to final destination and
         * break out of the loop after we're done. */
        if (tmp_idx_b == temp_file_idx) {
            no_more_temps = 1;
            /* no longer need the intermediate temp file */
            fclose(fp_intermediate);
            fp_intermediate = NULL;
        } else {
            /* we could not open all temp files, so merge all opened
             * temp files into the intermediate file.  Add the
             * intermediate file to the list of files to merge */
            temp_file_idx = tmp_idx_intermediate;
        }

        TRACEMSG((("Merging temporary files... open_count: %" PRIu16
                   "; read_count: %" PRIu32),
                  open_count, skHeapGetNumberEntries(heap)));

        /* get the index of the file with the lowest record; which is
         * at the top of the heap */
        heap_status = skHeapPeekTop(heap, (skheapnode_t*)&top_heap);
        if (SKHEAP_OK != heap_status) {
            skAppPrintErr("Unexpected return value from heap %d",
                          heap_status);
            break;
        }
        lowest = *top_heap;

        /* exit this do...while() once all records for all opened
         * files have been read */
        do {
            /* lowest_rec is the record pointed to by the index at the
             * top of the heap */
            memcpy(lowest_rec, recs[lowest], NODE_SIZE);

            /* replace the record we just processed and loop over all
             * files until we get a record that cannot be combined
             * with 'lowest_rec' */
            for (;;) {
                if (!fread(recs[lowest], NODE_SIZE, 1, fps[lowest])) {
                    /* read failed.  there is no more data for this
                     * file; remove it from the heap */
                    TRACEMSG(("Finished reading records from file #%u",
                              lowest));
                    skHeapExtractTop(heap, NULL);

                } else {
                    /* read succeeded */
                    skHeapReplaceTop(heap, &lowest, NULL);
                }

                /* get the record at the top of the heap and see if it
                 * can be combined with 'lowest_rec' */
                heap_status = skHeapPeekTop(heap, (skheapnode_t*)&top_heap);
                if (SKHEAP_OK != heap_status) {
                    /* heap is empty */
                    break;
                }
                lowest = *top_heap;

                if (rwrecCombine((rwRec*)lowest_rec, (rwRec*)recs[lowest])) {
                    /* records differ */
                    break;
                }
                /* else record was combined with 'lowest_rec'; read
                 * another */
            }

            /* write the record */
            if (fp_intermediate) {
                /* write the record to intermediate tmp file */
                if (!fwrite(lowest_rec, NODE_SIZE, 1, fp_intermediate)) {
                    skAppPrintSyserror(("Error writing record to"
                                        " temporary file '%s'"),
                                       skTempFileGetName(tmpctx,
                                                         tmp_idx_intermediate));
                    appExit(EXIT_FAILURE);
                }
            } else {
                /* we successfully opened all (remaining) temp files,
                 * write to record to the final destination */
                switch (rwRecGetTcpState((rwRec*)lowest_rec) & TIMEOUT_MASK) {
                  case 0:
                    ++counts.combined;
                    break;
                  case TIMEOUT_MASK:
                    ++counts.miss_start_end;
                    break;
                  case SK_TCPSTATE_TIMEOUT_KILLED:
                    ++counts.miss_end;
                    break;
                  case SK_TCPSTATE_TIMEOUT_STARTED:
                    ++counts.miss_start;
                    break;
                }
                rv = skStreamWriteRecord(out_stream, (rwRec*)lowest_rec);
                if (0 != rv) {
                    skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
                    if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                        appExit(EXIT_FAILURE);
                    }
                }
            }

        } while (SKHEAP_OK == heap_status);

        TRACEMSG((("Finished processing #%d through #%d"),
                  tmp_idx_a, tmp_idx_b));

        /* Close all open temp files */
        for (i = 0; i < open_count; ++i) {
            fclose(fps[i]);
        }
        /* Delete all temp files we opened (or attempted to open) this
         * time */
        for (j = tmp_idx_a; j <= tmp_idx_b; ++j) {
            skTempFileRemove(tmpctx, j);
        }

        /* Close the intermediate temp file. */
        if (fp_intermediate) {
            if (EOF == fclose(fp_intermediate)) {
                skAppPrintSyserror("Error closing temporary file '%s'",
                                   skTempFileGetName(tmpctx,
                                                     tmp_idx_intermediate));
                appExit(EXIT_FAILURE);
            }
            fp_intermediate = NULL;
        }

        /* Start the next merge with the next input temp file */
        tmp_idx_a = tmp_idx_b + 1;

    } while (!no_more_temps);

    skHeapFree(heap);
}


/*
 *  sortRandom();
 *
 *    Don't make any assumptions about the input.  Store the input
 *    records in a large buffer, and sort those in-core records once
 *    all records are processed or the buffer is full.  If the buffer
 *    fills up, store the sorted records into temporary files.  Once
 *    all records are read, use mergeFiles() above to merge-sort the
 *    temporary files.
 *
 *    Exits the application if an error occurs.
 */
static void
sortRandom(
    void)
{
    int temp_file_idx = -1;
    skstream_t *input_stream = NULL;/* input stream */
    uint8_t *record_buffer = NULL;  /* Region of memory for records */
    uint8_t *cur_node = NULL;       /* Ptr into record_buffer */
    uint8_t *next_node = NULL;      /* Ptr into record_buffer */
    uint32_t buffer_max_recs;       /* max buffer size (in number of recs) */
    uint32_t buffer_recs;           /* current buffer size (# records) */
    uint32_t buffer_chunk_recs;     /* how to grow from current to max buf */
    uint32_t num_chunks;            /* how quickly to grow buffer */
    uint32_t record_count = 0;      /* Number of records read */
    int rv;

    counts.min_idle = UINT64_MAX;

    /* Determine the maximum number of records that will fit into the
     * buffer if it grows the maximum size */
    buffer_max_recs = buffer_size / NODE_SIZE;
    TRACEMSG((("buffer_size = %" PRIu64
               "\nnode_size = %" PRIu32
               "\nbuffer_max_recs = %" PRIu32),
              buffer_size, NODE_SIZE, buffer_max_recs));

    /* We will grow to the maximum size in chunks */
    num_chunks = NUM_CHUNKS;
    if (num_chunks <= 0) {
        num_chunks = 1;
    }

    /* Attempt to allocate the initial chunk.  If we fail, increment
     * the number of chunks---which will decrease the amount we
     * attempt to allocate at once---and try again. */
    for (;;) {
        buffer_chunk_recs = buffer_max_recs / num_chunks;
        TRACEMSG((("num_chunks = %" PRIu32
                   "\nbuffer_chunk_recs = %" PRIu32),
                  num_chunks, buffer_chunk_recs));

        record_buffer = (uint8_t*)malloc(NODE_SIZE * buffer_chunk_recs);
        if (record_buffer) {
            /* malloc was successful */
            break;
        } else if (buffer_chunk_recs < MIN_IN_CORE_RECORDS) {
            /* give up at this point */
            skAppPrintErr("Error allocating space for %d records",
                          MIN_IN_CORE_RECORDS);
            appExit(EXIT_FAILURE);
        } else {
            /* reduce the amount we allocate at once by increasing the
             * number of chunks and try again */
            TRACEMSG(("malloc() failed"));
            ++num_chunks;
        }
    }

    buffer_recs = buffer_chunk_recs;
    TRACEMSG((("buffer_recs = %" PRIu32), buffer_recs));

    /* open first file */
    rv = appNextInput(&input_stream);
    if (rv < 0) {
        free(record_buffer);
        appExit(EXIT_FAILURE);
    }

    /* write header to output */
    rv = skStreamWriteSilkHeader(out_stream);
    if (0 != rv) {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        if (SKSTREAM_ERROR_IS_FATAL(rv)) {
            free(record_buffer);
            appExit(EXIT_FAILURE);
        }
    }

    record_count = 0;
    cur_node = record_buffer;
    while (input_stream != NULL) {
        /* read record */
        if ((rv = skStreamReadRecord(input_stream, (rwRec*)cur_node))
            != SKSTREAM_OK)
        {
            if (rv != SKSTREAM_ERR_EOF) {
                skStreamPrintLastErr(input_stream, rv, &skAppPrintErr);
            }
            /* end of file: close current and open next */
            skStreamDestroy(&input_stream);
            rv = appNextInput(&input_stream);
            if (rv < 0) {
                free(record_buffer);
                appExit(EXIT_FAILURE);
            }
            continue;
        }
        ++counts.in;

        if (0 == (rwRecGetTcpState((rwRec*)cur_node) & TIMEOUT_MASK)) {
            /* nothing to do to this record */
            rv = skStreamWriteRecord(out_stream, (rwRec*)cur_node);
            if (0 != rv) {
                skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
                if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                    free(record_buffer);
                    appExit(EXIT_FAILURE);
                }
            }
            ++counts.unfrag;
            continue;
        }

        ++record_count;
        cur_node += NODE_SIZE;

        if (record_count == buffer_recs) {
            /* Filled the current buffer */

            /* If buffer not at max size, see if we can grow it */
            if (buffer_recs < buffer_max_recs) {
                uint8_t *old_buf = record_buffer;

                /* add a chunk of records.  if we are near the max,
                 * set the size to the max */
                buffer_recs += buffer_chunk_recs;
                if (buffer_recs + buffer_chunk_recs > buffer_max_recs) {
                    buffer_recs = buffer_max_recs;
                }
                TRACEMSG((("Buffer full---attempt to grow to %" PRIu32
                           " records, %" PRIu32 " bytes"),
                          buffer_recs, NODE_SIZE * buffer_recs));

                /* attempt to grow */
                record_buffer = (uint8_t*)realloc(record_buffer,
                                                  NODE_SIZE * buffer_recs);
                if (record_buffer) {
                    /* Success, make certain cur_node points into the
                     * new buffer */
                    cur_node = (record_buffer + (record_count * NODE_SIZE));
                } else {
                    /* Unable to grow it */
                    TRACEMSG(("realloc() failed"));
                    record_buffer = old_buf;
                    buffer_max_recs = buffer_recs = record_count;
                }
            }

            /* Either buffer at maximum size or attempt to grow it
             * failed. */
            if (record_count == buffer_max_recs) {
                /* Sort */
                skQSort(record_buffer, record_count, NODE_SIZE, &rwrecCompare);

                /* Write to temp file */
                if (skTempFileWriteBuffer(tmpctx, &temp_file_idx, record_buffer,
                                          NODE_SIZE, record_count))
                {
                    skAppPrintSyserror("Error writing sorted buffer to"
                                       " temporary file");
                    free(record_buffer);
                    appExit(EXIT_FAILURE);
                }

                /* Reset record buffer to 'empty' */
                record_count = 0;
                cur_node = record_buffer;
            }
        }
    }

    /* Sort (and maybe store) last batch of records */
    if (record_count > 0) {
        skQSort(record_buffer, record_count, NODE_SIZE, &rwrecCompare);

        if (temp_file_idx >= 0) {
            /* Write last batch to temp file */
            if (skTempFileWriteBuffer(tmpctx, &temp_file_idx, record_buffer,
                                      NODE_SIZE, record_count))
            {
                skAppPrintSyserror("Error writing sorted buffer to"
                                   " temporary file");
                free(record_buffer);
                appExit(EXIT_FAILURE);
            }
        }
    }

    /* Generate the output */

    if (record_count == 0 && temp_file_idx == -1) {
        /* There are no records that we need to combine */

    } else if (temp_file_idx == -1) {
        /* No temp files written, just output batch of records */
        uint32_t c;

        TRACEMSG((("Writing %" PRIu32 " records to '%s'"),
                  record_count, skStreamGetPathname(out_stream)));
        /* get first two records from the sorted buffer */
        cur_node = record_buffer;
        next_node = record_buffer + NODE_SIZE;
        for (c = 1; c < record_count; ++c) {
            if (0 != rwrecCombine((rwRec*)cur_node, (rwRec*)next_node)) {
                /* records differ. print earlier record */
                switch (rwRecGetTcpState((rwRec*)cur_node) & TIMEOUT_MASK) {
                  case 0:
                    ++counts.combined;
                    break;
                  case TIMEOUT_MASK:
                    ++counts.miss_start_end;
                    break;
                  case SK_TCPSTATE_TIMEOUT_KILLED:
                    ++counts.miss_end;
                    break;
                  case SK_TCPSTATE_TIMEOUT_STARTED:
                    ++counts.miss_start;
                    break;
                }
                rv = skStreamWriteRecord(out_stream, (rwRec*)cur_node);
                if (0 != rv) {
                    skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
                    if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                        free(record_buffer);
                        appExit(EXIT_FAILURE);
                    }
                }
                cur_node = next_node;
            }
            /* else "next_node" was a continuation of "cur_node" and
             * has been combined with it */
            next_node += NODE_SIZE;
        }
        /* print remaining record */
        switch (rwRecGetTcpState((rwRec*)cur_node) & TIMEOUT_MASK) {
          case 0:
            ++counts.combined;
            break;
          case TIMEOUT_MASK:
            ++counts.miss_start_end;
            break;
          case SK_TCPSTATE_TIMEOUT_KILLED:
            ++counts.miss_end;
            break;
          case SK_TCPSTATE_TIMEOUT_STARTED:
            ++counts.miss_start;
            break;
        }
        rv = skStreamWriteRecord(out_stream, (rwRec*)cur_node);
        if (0 != rv) {
            skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
            if (SKSTREAM_ERROR_IS_FATAL(rv)) {
                free(record_buffer);
                appExit(EXIT_FAILURE);
            }
        }
    } else {
        /* no longer have a need for the record buffer */
        free(record_buffer);
        record_buffer = NULL;

        /* now merge all the temp files */
        mergeFiles(temp_file_idx);
    }

    if (record_buffer) {
        free(record_buffer);
    }
}


#if 0
static void
do_statistics_table(
    void)
{
    /* Prints statistics in a table */
/*
    char td[3][40];
    const char SEP[] = "    ";
    const int w_sub = 24;
    const int w_num = 12;
    int64_t rec_count;

    skStreamPrint(
        print_statistics, "%*s%s%*s\n",
        -(w_sub + 1 + w_num), "Input Record Counts", SEP,
        -(w_sub + 1 + w_num), "Output Record Counts");

    rec_count = (int64_t)counts.in;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s\n",
        -w_sub, "Read:", "", w_num, counts.in, SEP);

    rec_count -= counts.unfrag;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s%*s%1s%*" PRIu64 "\n",
        -w_sub, "Initially Complete:", "-", w_num, counts.unfrag, SEP,
        -w_sub, "Initially Complete:", "+", w_num, counts.unfrag);

    skStreamPrint(
        print_statistics, "%*s%1s%*" PRId64 "%s\n",
        -w_sub, "Examined:", "=", w_num, rec_count, SEP);

    rec_count -= counts.miss_end;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s%*s%1s%*" PRIu64 "\n",
        -w_sub, "Missing end:", "-", w_num, counts.miss_end, SEP,
        -w_sub, "Missing end:", "+", w_num, counts.miss_end);

    rec_count -= counts.miss_start_end;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s%*s%1s%*" PRIu64 "\n",
        -w_sub, "Missing start & end:", "-", w_num, counts.miss_start_end,
        SEP,
        -w_sub, "Missing start & end:", "+", w_num, counts.miss_start_end);

    rec_count -= counts.miss_start;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s%*s%1s%*" PRIu64 "\n",
        -w_sub, "Missing start:", "-", w_num, counts.miss_start, SEP,
        -w_sub, "Missing start:", "+", w_num, counts.miss_start);

    skStreamPrint(
        print_statistics, "%*s%1s%*" PRId64 "%s\n",
        -w_sub, "Components:", "=", w_num, rec_count, SEP);

    rec_count -= counts.combined;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRId64 "%s\n",
        -w_sub, "Elimiated:", "-", w_num, rec_count, SEP);

    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s%*s%1s%*" PRIu64 "\n",
        -w_sub, "Made complete:", "=", w_num, counts.combined, SEP,
        -w_sub, "Made complete:", "+", w_num, counts.combined);

    skStreamPrint(
        print_statistics, "%*s%s%*s%1s%*" PRIu64 "\n",
        -(w_sub + 1 + w_num), "", SEP,
        -w_sub, "Written:", "=", w_num, counts.out);

    skStreamPrint(print_statistics,
                  ("Idle Times:\n"
                   "Minimum:      %16s\n"
                   "Penultimate:  %16s\n"
                   "Maximum:      %16s\n"),
                  timediff_str(td[0], sizeof(td[0]), counts.min_idle),
                  timediff_str(td[1], sizeof(td[1]), counts.penult_idle),
                  timediff_str(td[2], sizeof(td[2]), counts.max_idle));
*/
}
#endif  /* 0 */


static void
do_statistics(
    void)
{
    char td[3][40];
    const char was_output[] = " *";
    const int w_sub = 30;
    const int w_num = 12;
    int64_t rec_count;

    if (0 == counts.in) {
        /* show sensible minimum time when no input */
        counts.min_idle = 0;
    }

    skStreamPrint(print_statistics, "FLOW RECORD COUNTS:\n");

    rec_count = (int64_t)counts.in;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s\n",
        -w_sub, "Read:", "", w_num, counts.in, "");

    rec_count -= counts.unfrag;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s\n",
        -w_sub, "Initially Complete:", "-", w_num, counts.unfrag, was_output);

    skStreamPrint(
        print_statistics, "%*s%1s%*" PRId64 "%s\n",
        -w_sub, "Sorted & Examined:", "=", w_num, rec_count, "");

    rec_count -= counts.miss_end;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s\n",
        -w_sub, "Missing end:", "-", w_num, counts.miss_end, was_output);

    rec_count -= counts.miss_start_end;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s\n",
        -w_sub, "Missing start & end:", "-", w_num, counts.miss_start_end,
        was_output);

    rec_count -= counts.miss_start;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s\n",
        -w_sub, "Missing start:", "-", w_num, counts.miss_start, was_output);

    skStreamPrint(
        print_statistics, "%*s%1s%*" PRId64 "%s\n",
        -w_sub, "Prior to combining:", "=", w_num, rec_count, "");

    rec_count -= counts.combined;
    skStreamPrint(
        print_statistics, "%*s%1s%*" PRId64 "%s\n",
        -w_sub, "Eliminated:", "-", w_num, rec_count, "");

    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 "%s\n",
        -w_sub, "Made complete:", "=", w_num, counts.combined, was_output);

    skStreamPrint(
        print_statistics, "%*s%1s%*" PRIu64 " (sum of%s)\n",
        -w_sub, "Written:", "", w_num, counts.out, was_output);

    skStreamPrint(print_statistics,
                  ("\nIDLE TIMES:\n"
                   "Minimum:      %16s\n"
                   "Penultimate:  %16s\n"
                   "Maximum:      %16s\n"),
                  timediff_str(td[0], sizeof(td[0]), counts.min_idle),
                  timediff_str(td[1], sizeof(td[1]), counts.penult_idle),
                  timediff_str(td[2], sizeof(td[2]), counts.max_idle));
}


int main(int argc, char **argv)
{
    int rv;

    appSetup(argc, argv);                 /* never returns on error */

    sortRandom();

    counts.out = skStreamGetRecordCount(out_stream);

    /* close the file */
    if ((rv = skStreamClose(out_stream))
        || (rv = skStreamDestroy(&out_stream)))
    {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        appExit(EXIT_FAILURE);
    }
    out_stream = NULL;

    if (print_statistics) {
        do_statistics();
    }

    appExit(EXIT_SUCCESS);
    return 0; /* NOTREACHED */
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
