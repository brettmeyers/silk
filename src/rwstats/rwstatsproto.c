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

/*
**  rwstats.c
**
**  Implementation of the rwstats suite application.
**
**  Reads packed files or reads the output from rwfilter and can
**  compute a battery of characterizations and statistics:
**
**  -- Top N or Bottom N SIPs with counts; count of unique SIPs
**  -- Top N or Bottom N DIPs with counts; count of unique DIPs
**  -- Top N or Bottom N SIP/DIP pairs with counts; count of unique
**     SIP/DIP pairs (for a limited number of records)
**  -- Top N or Bottom N Src Ports with counts; count of unique Src Ports
**  -- Top N or Bottom N Dest Ports with counts; count of unique Dest Ports
**  -- Top N or Bottom N Protocols with counts; count of unique protocols
**  -- For more continuous variables (bytes, packets, bytes/packet)
**     provide statistics such as min, max, quartiles, and intervals
**
**  Instead of specifying a Top N or Bottom N as an absolute number N,
**  the user may specify a cutoff threshold.  In this case, the Top N
**  or Bottom N required to print all counts meeting the threshold is
**  computed by the application.
**
**  Instead of specifying the threshold as an absolute count, the user
**  may specify the threshold as percentage of all input records.  For
**  this case, the absolute threshold is calculated and then that is
**  used to calculate the Top N or Bottom N.
**
**  The application will only do calculations and produce output when
**  asked to do so.  At least one argument is required to tell the
**  application what to do.
**
**  Ideas for expansion
**  -- Similarly for other variables, e.g., country code.
**  -- Output each type of data to its own file
**  -- Save intermediate data in files for faster reprocessing by this
**     application
**  -- Save intermediate data in files for processing by other
**     applications
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwstatsproto.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "rwstats.h"
#include "interval.h"

/*
**  IMPLEMENTATION NOTES
**
**  For each input type (source ip, dest ip, source port, proto, etc),
**  there are two globals: limit_<type> contains the value the user
**  entered for the input type, and wanted_stat_<type> is a member
**  of the wanted_stat_type and says what the limit_<type> value
**  represents---e.g., the Top N, the bottom threshold percentage, etc.
**
**  The application takes input (either from stdin or as files on
**  command line) and calls processFile() on each.  A count of each
**  unique source IP addresses is stored in the IpCounter hash table
**  counter_src_ip; Destinations IPs in counter_dest_ip; data for
**  flow between a Source IP and Destination IP pair are stored in
**  counter_pair_ip.
**
**  Since there are relatively few ports and protocols, two
**  65536-elements arrays, src_port_array and dest_port_array are
**  used to store a count of the records for each source and
**  destination port, respectively, and a 256-element array,
**  proto_array, is used to store a count of each protocol.
**
**  Minima, maxima, quartile, and interval data are stored for each of
**  bytes, packets, and bytes-per-packet for all flows--regardless of
**  protocol--and detailed for a limited number (RWSTATS_NUM_PROTO-1)
**  of protocols..  The minima and maxima are each stored in arrays
**  for each of bytes, packets, bpp.  For example bytes_min[0]
**  stores the smallest byte count regardless of protocol (ie, over
**  all protocols), and pkts_max[1] stores the largest packet count
**  for the first protocol the user specified.  The mapping from
**  protocol to array index is given by proto_to_stats_idx[], where
**  the index into proto_to_stats_idx[] returns an integer that is
**  the index into bytes_min[].  Data for the intervals is stored in
**  two dimensional arrays, where the first dimension is the same as
**  for the minima and maxima, and the second dimension is the number
**  of intervals, NUM_INTERVALS.
**
**  Once data is collected, it is processed.
**
**  For the IPs, the user is interested the number of unique IPs and
**  the IPs with the topN counts (things are similar for the bottomN,
**  but we use topN in this dicussion to keep things more clear).  In
**  the printTopIps() function, an array with 2*topN elements is
**  created and passed to calcTopIps(); that array will be the result
**  array and it will hold the topN IpAddr and IpCount pairs in sorted
**  order.  In calcTopIps(), a working array of 2*topN elements and a
**  Heap data structure with topN nodes are created.  The topN
**  IpCounts seen are stored as IpCount/IpAddr pairs in the
**  2*topN-element array (but not in sorted order), and the heap
**  stores pointers into that array with the lowest IpCount at the
**  root of the heap.  As the function iterates over the hash table,
**  it compares the IpCount of the current hash-table element with the
**  IpCount at the root of the heap.  When the IpCount of the
**  hash-table element is larger, the root of the heap is removed, the
**  IpCount/IpAddr pair pointed to by the former heap-root is removed
**  from the 2*topN-element array and replaced with the new
**  IpCount/IpAddr pair, and finally a new node is added to the heap
**  that points to the new IpCount/IpAddr pair.  This continues until
**  all hash-table entries are processed.  To get the list of topN IPs
**  from highest to lowest, calcTopIps() removes elements from the
**  heap and stores them in the result array from position N-1 to
**  position 0.
**
**  Finding the topN source ports, topN destination ports, and topN
**  protocols are similar to finding the topN IPs, except the ports
**  and protocols are already stored in an array, so pointers directly
**  into the src_port_array, dest_port_array, and proto_array
**  are stored in the heap.  When generating output, the number of the
**  port or protocol is determined by the diffence between the pointer
**  into the *_port_array or proto_array and its start.
**
**  Instead of specifying a topN, the user may specify a cutoff
**  threshold.  In this case, the topN required to print all counts
**  meeting the threshold is computed by looping over the IP
**  hash-table or port/protocol arrays and finding all entries with at
**  least threshold hits.
**
**  The user may specify a percentage threshold instead of an absolute
**  threshold.  Once all records are read, the total record count is
**  multiplied by the percentage threshold to get the absolute
**  threshold cutoff, and that is used to calculate the topN as
**  described in the preceeding paragraph.
**
**  For the continuous variables bytes, packets, bpp, most of the work
**  was done while reading the data, so processing is minimal.  Only
**  the quartiles must be calculated.
*/



/*
 * Statistics (min, max, quartiles, intervals) for "continuous" values
 * (bytes, packets, bpp) can be computed over all protocols, and the
 * can be broken out for a limited number of specific protocols.  This
 * defines the size of the data structures to hold these statistics.
 * This is one more that the number of specific protocols allowed.
 */

#define NUM_STATS 3
#define BYTE 0
#define PKT 1
#define BPP 2


/* These arrays hold the statistics.  Position 0 is for the
 * combination of all statistics. */
static uint64_t *count; /* record count per protocol */
static uint32_t *minval;
static uint32_t *maxval;
static uint32_t (*intervals)[NUM_INTERVALS];
static uint32_t **interval_defn;

#define MK_IDX(type, idx) ((type) + (NUM_STATS * (idx)))


/* This maps the protocol number to the index in the above statistics
 * arrays.  If the value for a protocol is 0, the user did not request
 * detailed specs on that protocol. */
static int16_t proto_to_stats_idx[256];


/* LOCAL FUNCTION PROTOTYPES */

static int  protoStatsSetup(void);
static void protoStatsTeardown(void);
static void protoStatsPrintIntervals(int proto_idx, const char *col_title);
static void
protoStatsUpdateStatistics(
    int                 proto_idx,
    const rwRec        *rwrec);


/* FUNCTION DEFINITIONS */

/*
 * parseProtos
 *      Discover which protos the user wants detailed stats for
 * Arguments:
 *      arg -the command line argument
 * Returns:
 *      0 if OK; 1 if error
 * Side Effects:
 *      Sets values in the global proto_to_stats_idx[]
 */
int
protoStatsParse(
    const char         *arg)
{
    uint32_t i;
    uint32_t parse_count;
    uint32_t *parsed_list;
    int32_t proto_idx = 1; /* 0 is global stats */
    int rv;

    rv = skStringParseNumberList(&parsed_list, &parse_count, arg, 0, 255, 0);
    if (rv) {
        skAppPrintErr("Invalid protocol list '%s': %s",
                      arg, skStringParseStrerror(rv));
        return 1;
    }

    for (i = 0; i < parse_count; ++i) {
        if (0 != proto_to_stats_idx[parsed_list[i]]) {
            skAppPrintErr(("Duplicate protocol %" PRIu32 " ignored"),
                          parsed_list[i]);
        } else {
            proto_to_stats_idx[parsed_list[i]] = proto_idx;
            ++proto_idx;
        }
    }

    free(parsed_list);
    return 0;
}


static int
protoStatsSetup(
    void)
{
    int proto_idx = 1; /* ALL stats */
    int i, j;

    for (i = 0; i < 256; ++i) {
        if (proto_to_stats_idx[i]) {
            ++proto_idx;
        }
    }

    /* Allocate space for all the stats */
    count = (uint64_t*)calloc(proto_idx, sizeof(uint64_t));
    minval = (uint32_t*)malloc(NUM_STATS * proto_idx * sizeof(uint32_t));
    maxval = (uint32_t*)calloc(NUM_STATS * proto_idx, sizeof(uint32_t));
    intervals = (uint32_t (*)[10])calloc(NUM_STATS * proto_idx * NUM_INTERVALS,
                                         sizeof(uint32_t*));
    interval_defn = (uint32_t**)calloc(NUM_STATS * proto_idx,
                                       sizeof(uint32_t*));

    if (!minval || !maxval || !intervals || !interval_defn) {
        skAppPrintErr("Cannot allocate space for protocol statistics");
        protoStatsTeardown();
        return 1;
    }

    /* Set the minima to a big value, like INT_MAX */
    for (i = 0; i < proto_idx; ++i) {
        for (j = 0; j < NUM_STATS; ++j) {
            minval[MK_IDX(j, i)] = INT_MAX;
        }
    }

    /* Set the interval definitions for TCP */
    i = proto_to_stats_idx[6];
    if (i != 0) {
        interval_defn[MK_IDX(BYTE, i)] = tcpByteIntervals;
        interval_defn[MK_IDX(PKT,  i)] = tcpPktIntervals;
        interval_defn[MK_IDX(BPP,  i)] = tcpBppIntervals;
    }

    /* Since TCP is dominate protocol; use the TCP interval
     * definitions for stats across ALL protocols. */
    interval_defn[MK_IDX(BYTE, 0)] = tcpByteIntervals;
    interval_defn[MK_IDX(PKT,  0)] = tcpPktIntervals;
    interval_defn[MK_IDX(BPP,  0)] = tcpBppIntervals;

    /* Set all other interval definitions to UDP */
    for (i = 1; i < proto_idx; ++i) {
        if (i != proto_to_stats_idx[6]) {
            interval_defn[MK_IDX(BYTE, i)] = udpByteIntervals;
            interval_defn[MK_IDX(PKT,  i)] = udpPktIntervals;
            interval_defn[MK_IDX(BPP,  i)] = udpBppIntervals;
        }
    }

    return 0;
}


static void
protoStatsTeardown(
    void)
{
    if (count) {
        free(count);
        count = NULL;
    }
    if (minval) {
        free(minval);
        minval = NULL;
    }
    if (maxval) {
        free(maxval);
        maxval = NULL;
    }
    if (intervals) {
        free(intervals);
        intervals = NULL;
    }
    if (interval_defn) {
        free(interval_defn);
        interval_defn = NULL;
    }
}


/*
 *  ok = protoStatsProcessFile(stream);
 *
 *    Read SiLK Flow records from the stream and update the counters.
 */
static int
protoStatsProcessFile(
    skstream_t         *rwIOS)
{
    rwRec rwrec;
    int proto_idx;
    int rv;

    while ((rv = skStreamReadRecord(rwIOS, &rwrec)) == SKSTREAM_OK) {
        /* Statistics across ALL protocols */
        protoStatsUpdateStatistics(0, &rwrec);

        /* Compute statistics for specific protocol if requested */
        proto_idx = proto_to_stats_idx[rwRecGetProto(&rwrec)];
        if (proto_idx) {
            protoStatsUpdateStatistics(proto_idx, &rwrec);
        }

    } /* while skStreamReadRecord() */
    if (SKSTREAM_ERR_EOF != rv) {
        skStreamPrintLastErr(rwIOS, rv, &skAppPrintErr);
    }

    return 0;
}


/*
 * protoStatsUpdateStatistics:
 *      Update the minima, maxima, and intervals for bytes, packets,
 *      and bytes-per-packet for the specified protocol.
 * Arguments:
 *      proto -number of the protocol to be updated.  Used to
 *              determine which intervals to use.  This is equal to
 *              256 for the overall stats
 *      proto_idx -the index---determined by proto---into the
 *              various *_min, *_max, and *_intervals arrays.
 *      rwrecPtr -the rwrec data
 * Returns: NONE.
 * Side Effects:
 *      Global counters, minima, maxima, intervals are modified.
 */
static void
protoStatsUpdateStatistics(
    int                 proto_idx,
    const rwRec        *rwrec)
{
    uint32_t stat_src[NUM_STATS];
    int i;
    int s;

    /* Update count */
    ++count[proto_idx];

    stat_src[BYTE] = rwRecGetBytes(rwrec);
    stat_src[PKT]  = rwRecGetPkts(rwrec);
    stat_src[BPP]  = rwRecGetBytes(rwrec) / rwRecGetPkts(rwrec);

    /* Find min/max/intervals for bytes, packets, bpp */
    for (s = 0; s < NUM_STATS; ++s) {
        if (stat_src[s] < minval[MK_IDX(s, proto_idx)]) {
            minval[MK_IDX(s, proto_idx)] = stat_src[s];
            if (0 == maxval[MK_IDX(s, proto_idx)]) {
                maxval[MK_IDX(s, proto_idx)] = stat_src[s];
            }
        } else if (stat_src[s] > maxval[MK_IDX(s, proto_idx)]) {
            maxval[MK_IDX(s, proto_idx)] = stat_src[s];
        }
        for (i = 0; i < NUM_INTERVALS; ++i) {
            if (stat_src[s] <= interval_defn[MK_IDX(s, proto_idx)][i])
            {
                intervals[s + NUM_STATS * proto_idx][i]++;
                break;
            }
        }
    }
}


/*
 *  protoStatsGenerateOutput()
 *
 *    Generate the output when processing protocols.
 */
static void
protoStatsPrintResults(
    void)
{
    int proto;
    int proto_idx;
    int print_all_protos = 1;

    if (count[0] > 0) {
        /* Check to see if a single protocol has all the flows.  If
         * so, do not print the all-protos stats, since it will be
         * identical to the single-proto stats. */
        for (proto = 0; proto < 256; ++proto) {
            proto_idx = proto_to_stats_idx[proto];
            if (proto_idx == 0) {
                continue;
            }
            if (count[proto_idx] == count[0]) {
                print_all_protos = 0;
                break;
            }
        }
    }

    /* Print all proto stats only when there are multiple protocols. */
    if (print_all_protos) {
        fprintf(output.of_fp, "FLOW STATISTICS--ALL PROTOCOLS:  ");
        protoStatsPrintIntervals(0, "%_of_input");
    }

    /* Return if no records were read */
    if (0 == count[0]) {
        return;
    }

    for (proto = 0; proto < 256; ++proto) {
        proto_idx = proto_to_stats_idx[proto];
        if (proto_idx == 0) {
            /* nothing to do for this protocol */
            continue;
        }

        fprintf(output.of_fp, "\nFLOW STATISTICS--PROTOCOL %d:  ", proto);
        protoStatsPrintIntervals(proto_idx, "%_of_proto");
    }
}


/*
 * protoStatsPrintIntervals
 *      Print min, max, and intervals for bytes, packets, and bpp for
 *      the given protocol
 * Arguments:
 *      proto - the protocol to print
 *      proto_idx - the index to use into the *_min, *_max,
 *              *_interval arrays to access data for this protocol
 * Returns: NONE
 * Side Effects:
 *      Prints output to outstream
 */
static void
protoStatsPrintIntervals(
    int                 proto_idx,
    const char         *col_title)
{
    static const char *stat_name[] = {"BYTES", "PACKETS", "BYTES/PACKET"};
    double *quartiles;
    int i;
    int s;
    double percent;
    double cumul_pct;

    fprintf(output.of_fp, ("%" PRIu64), count[proto_idx]);
    if (proto_idx != 0) {
        fprintf(output.of_fp, ("/%" PRIu64), count[0]);
    }
    fprintf(output.of_fp, " records\n");

    if (0 == count[proto_idx]) {
        /* no records, so no data to print */
        return;
    }

    for (s = 0; s < NUM_STATS; ++s) {
        /* Print title and min, max */
        fprintf(output.of_fp, ("*%s min %" PRIu32 "; max %" PRIu32 "\n"),
                stat_name[s], minval[MK_IDX(s, proto_idx)],
                maxval[MK_IDX(s, proto_idx)]);

        /* Compute and print quartitles */
        quartiles = intervalQuartiles(intervals[MK_IDX(s, proto_idx)],
                                      interval_defn[MK_IDX(s, proto_idx)],
                                      NUM_INTERVALS);
        fprintf(output.of_fp,
                "  quartiles LQ %.5f Med %.5f UQ %.5f UQ-LQ %.5f\n",
                quartiles[0], quartiles[1], quartiles[2],
                (quartiles[2] - quartiles[0]));

        /* Column titles for intervals */
        if (!app_flags.no_titles) {
            fprintf(output.of_fp, "%*s%c%*s%c%*s%c%*s%s\n",
                    width[WIDTH_KEY],   "interval_max", delimiter,
                    width[WIDTH_INTVL], "count<=max", delimiter,
                    width[WIDTH_PCT],   col_title, delimiter,
                    width[WIDTH_PCT],   "cumul_%", final_delim);
        }

        /* Print intervals and percentages */
        cumul_pct = 0.0;
        for (i = 0; i < NUM_INTERVALS; ++i) {
            percent = (100.0 * (double)intervals[MK_IDX(s, proto_idx)][i]
                       / (double)count[proto_idx]);
            cumul_pct += percent;

            fprintf(output.of_fp,
                    ("%*" PRIu32 "%c%*" PRIu32 "%c%*.6f%c%*.6f%s\n"),
                    width[WIDTH_KEY],
                    interval_defn[MK_IDX(s, proto_idx)][i], delimiter,
                    width[WIDTH_INTVL],
                    intervals[MK_IDX(s, proto_idx)][i], delimiter,
                    width[WIDTH_PCT], percent, delimiter,
                    width[WIDTH_PCT], cumul_pct, final_delim);
        }
    }
}


int
protoStatsMain(
    void)
{
    skstream_t *rwIOS;
    int rv = 0;

    rv = protoStatsSetup();
    if (rv) {
        return rv;
    }

    while ((rv = appNextInput(&rwIOS)) == 0) {
        rv = protoStatsProcessFile(rwIOS);
        skStreamDestroy(&rwIOS);
    }
    if (rv > 0) {
        /* processed all files */
        rv = 0;
    } else {
        /* error opening file */
        rv = 1;
    }

    /* enable the pager */
    setOutputHandle();

    /* Generate output */
    protoStatsPrintResults();

    protoStatsTeardown();

    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
