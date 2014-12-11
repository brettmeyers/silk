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

#include <silk/silk.h>

RCSIDENT("$SiLK: rwstats.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skheap.h>
#include "rwstats.h"


/* TYPEDEFS AND DEFINES */

/* Initial number of elements for the heap when using a threshold or
 * percentage cut-off */
#define HEAP_INITIAL_SIZE  512

/* For output, add an "s" when speaking of values other than 1 */
#define PLURAL(plural_val) (((plural_val) == 1) ? "" : "s")

/*
 *  dir_val_type = DIR_AND_TYPE(direction, value_type);
 *
 *    Return a single integer that encodes the direction (RWSTATS_DIR_TOP,
 *    RWSTATS_DIR_BTM) and the value type to compute (SK_FIELD_RECORDS,...).
 */
#define DIR_AND_TYPE(dat_t_or_b, dat_val_type)  \
    ((dat_t_or_b) | ((dat_val_type) << 1))


/*
 *  These macros extract part of a field-list buffer to get a value,
 *  and then set that value on 'rec' by calling 'func'
 */
#define KEY_TO_REC(type, func, rec, field_buffer, field_list, field)    \
    {                                                                   \
        type k2r_val;                                                   \
        skFieldListExtractFromBuffer(field_list, field_buffer,          \
                                     field, (uint8_t*)&k2r_val);        \
        func((rec), k2r_val);                                           \
    }

#define KEY_TO_REC_08(func, rec, field_buffer, field_list, field)       \
    KEY_TO_REC(uint8_t, func, rec, field_buffer, field_list, field)

#define KEY_TO_REC_16(func, rec, field_buffer, field_list, field)       \
    KEY_TO_REC(uint16_t, func, rec, field_buffer, field_list, field)

#define KEY_TO_REC_32(func, rec, field_buffer, field_list, field)       \
    KEY_TO_REC(uint32_t, func, rec, field_buffer, field_list, field)


#define MEMSET_HEAP_NODE(mhn_buf, key_buf, value_buf, distinct_buf)     \
    do {                                                                \
        memcpy(HEAP_PTR_KEY(mhn_buf), key_buf,                          \
               heap_octets_key);                                        \
        memcpy(HEAP_PTR_VALUE(mhn_buf), value_buf,                      \
               heap_octets_value);                                      \
        memcpy(HEAP_PTR_DISTINCT(mhn_buf), distinct_buf,                \
               heap_octets_distinct);                                   \
    } while(0)


/*
 *  meets = VALUE_MEETS_THRESHOLD(value);
 *
 *    Return true if 'value' meets the threshold value set by the
 *    user.  Uses the global 'limit' and 'direction' variables.
 */
#define VALUE_MEETS_THRESHOLD(vmt_value)                        \
    (((vmt_value) > limit.value[RWSTATS_THRESHOLD])             \
     ? (RWSTATS_DIR_TOP == direction)                           \
     : ((RWSTATS_DIR_BOTTOM == direction)                       \
        || ((vmt_value) == limit.value[RWSTATS_THRESHOLD])))


/* structure to get the distinct count when using IPv6 */
typedef union ipv6_distinct_un {
    uint64_t count;
    uint8_t  ip[16];
} ipv6_distinct_t;


/* EXPORTED VARIABLES */

/* user limit for this stat: N if top N or bottom N, threshold, or
 * percentage */
rwstats_limit_t limit;

rwstats_direction_t direction = RWSTATS_DIR_TOP;

/* the final delimiter on each line; assume none */
char final_delim[] = {'\0', '\0'};

int width[RWSTATS_COLUMN_WIDTH_COUNT] = {
    15, /* WIDTH_KEY:   key */
    20, /* WIDTH_VAL:   count */
    10, /* WIDTH_INTVL: interval maximum */
    10, /* WIDTH_PCT:   percentage value */
};

/* non-zero when --overall-stats or --detail-proto-stats is given */
int proto_stats = 0;

sk_unique_t *uniq;
sk_sort_unique_t *ps_uniq;

sk_fieldlist_t *key_fields;
sk_fieldlist_t *value_fields;
sk_fieldlist_t *distinct_fields;

/* for the key, value, and distinct fields used by the heap, the byte
 * lengths of each and the offsets of each when creating a heap
 * node */
size_t heap_octets_key = 0;
size_t heap_octets_value = 0;
size_t heap_octets_distinct = 0;

size_t heap_offset_key = 0;
size_t heap_offset_value = 0;
size_t heap_offset_distinct = 0;

/* the total byte length of a node in the heap */
size_t heap_octets_node = 0;

/* delimiter between output columns */
char delimiter = '|';

/* to convert the key fields (as an rwRec) to ascii */
rwAsciiStream_t *ascii_str;

/* the real output */
sk_fileptr_t output;

/* flags set by the user options */
app_flags_t app_flags;

/* number of records read */
uint64_t record_count = 0;

/* Summation of whatever value (bytes, packets, flows) we are using.
 * When counting flows, this will be equal to record_count. */
uint64_t value_total = 0;

/* how to handle IPv6 flows */
sk_ipv6policy_t ipv6_policy = SK_IPV6POLICY_MIX;

/* CIDR block mask for src and dest ips.  If 0, use all bits;
 * otherwise, the IP address should be bitwised ANDed with this
 * value. */
uint32_t cidr_sip = 0;
uint32_t cidr_dip = 0;

/* Information about each potential "value" field the user can choose
 * to compute and display.  Ensure these appear in same order as in
 * the OPT_BYTES...OPT_DIP_DISTINCT values in appOptionsEnum. */
builtin_field_t builtin_values[] = {
    /* title, min, max, text_len, id, is_distinct, description */
    {"Bytes",          1, UINT64_MAX, 20, SK_FIELD_SUM_BYTES,     0,
     "Sum of bytes for all flows in the group"},
    {"Packets",        1, UINT64_MAX, 15, SK_FIELD_SUM_PACKETS,   0,
     "Sum of packets for all flows in the group"},
    {"Records",        1, UINT64_MAX, 10, SK_FIELD_RECORDS,       0,
     "Number of flow records in the group"},
    {"sIP-Distinct",   1, UINT64_MAX, 10, SK_FIELD_SIPv4,         1,
     "Number of distinct source IPs in the group"},
    {"dIP-Distinct",   1, UINT64_MAX, 10, SK_FIELD_DIPv4,         1,
     "Number of distinct source IPs in the group"},
    {"Distinct",       1, UINT64_MAX, 10, SK_FIELD_CALLER,        1,
     "You must append a colon and a key field to count the number of"
     " distinct values seen for that field in the group"}
};

const size_t num_builtin_values = (sizeof(builtin_values)/
                                   sizeof(builtin_field_t));

/* which of elapsed, sTime, and eTime are part of the key. uses the
 * PARSE_KEY_* values from rwstats.h */
unsigned int time_fields_key = 0;

/* whether dPort is part of the key */
unsigned int dport_key = 0;


/* LOCAL VARIABLES */

/* the heap data structure */
static skheap_t *heap = NULL;

/* the comparison function to use for the heap */
static skheapcmpfn_t cmp_fn = NULL;

/* number of entries in the heap */
static uint32_t heap_num_entries;

/* whether we encounted an error counting distinct IP addresses */
static int distinct_err = 0;


/* LOCAL FUNCTION PROTOTYPES */



/* FUNCTION DEFINITIONS */


/*
 *  topnPrintHeader();
 *
 *    Print the header giving number of unique hash keys seen.  Should
 *    be called even when --no-titles is requested, since it will
 *    print a warning if no records met the threshold.
 */
static void
topnPrintHeader(
    void)
{
    char buf[128];
    const char *direction_name = "";
    const char *above_below = "";

    /* enable the pager */
    setOutputHandle();

    /* handle no titles */
    if (app_flags.no_titles) {
        return;
    }

    switch (direction) {
      case RWSTATS_DIR_TOP:
        direction_name = "Top";
        above_below = "above";
        break;
      case RWSTATS_DIR_BOTTOM:
        direction_name = "Bottom";
        above_below = "below";
        break;
    }

    /* Get a count of unique flows */
    fprintf(output.of_fp, ("INPUT: %" PRIu64 " Record%s for %" PRIu64 " Bin%s"),
            record_count, PLURAL(record_count),
            limit.entries, PLURAL(limit.entries));
    if (value_total) {
        fprintf(output.of_fp, (" and %" PRIu64 " Total %s"),
                value_total, limit.title);
    }
    fprintf(output.of_fp, "\n");

    /* handle the no data case */
    if (limit.value[RWSTATS_COUNT] < 1) {
        switch (limit.type) {
          case RWSTATS_COUNT:
            skAppPrintErr("User was allowed to enter count of 0");
            skAbortBadCase(limit.type);

          case RWSTATS_THRESHOLD:
            fprintf(output.of_fp,
                    ("OUTPUT: No bins %s threshold of %" PRIu64 " %s\n"),
                    above_below, limit.value[RWSTATS_THRESHOLD], limit.title);
            break;

          case RWSTATS_PERCENTAGE:
            fprintf(output.of_fp, ("OUTPUT: No bins %s threshold of %"
                                PRIu64 "%% (%" PRIu64 " %s)\n"),
                    above_below, limit.value[RWSTATS_PERCENTAGE],
                    limit.value[RWSTATS_THRESHOLD], limit.title);
            break;
        }
        return;
    }

    switch (limit.type) {
      case RWSTATS_COUNT:
        fprintf(output.of_fp, ("OUTPUT: %s %" PRIu64 " Bin%s by %s\n"),
                direction_name, limit.value[RWSTATS_COUNT],
                PLURAL(limit.value[RWSTATS_COUNT]), limit.title);
        break;

      case RWSTATS_THRESHOLD:
        fprintf(output.of_fp, ("OUTPUT: %s %" PRIu64 " bins by %s"
                            " (threshold %" PRIu64 ")\n"),
                direction_name, limit.value[RWSTATS_COUNT],
                limit.title, limit.value[RWSTATS_THRESHOLD]);
        break;

      case RWSTATS_PERCENTAGE:
        fprintf(output.of_fp, ("OUTPUT: %s %" PRIu64 " bins by %s"
                            " (%" PRIu64 "%% == %" PRIu64 ")\n"),
                direction_name, limit.value[RWSTATS_COUNT],
                limit.title, limit.value[RWSTATS_PERCENTAGE],
                limit.value[RWSTATS_THRESHOLD]);
        break;
    }

    if (app_flags.no_titles) {
        return;
    }

    /* print key titles */
    rwAsciiPrintTitles(ascii_str);

    if (!app_flags.no_percents) {
        snprintf(buf, sizeof(buf), "%%%s", limit.title);
        buf[sizeof(buf)-1] = '\0';

        if (app_flags.no_columns) {
            fprintf(output.of_fp, "%c%s%c%s",
                    delimiter, buf, delimiter, "cumul_%");
        } else {
            fprintf(output.of_fp, ("%c%*.*s%c%*.*s"),
                    delimiter, width[WIDTH_PCT], width[WIDTH_PCT], buf,
                    delimiter, width[WIDTH_PCT], width[WIDTH_PCT], "cumul_%");
        }
        fprintf(output.of_fp, "%s\n", final_delim);
    }
}


/*
 *  writeAsciiRecord(heap_ptr);
 *
 *    Unpacks the fields from 'key' and the value fields from 'value'.
 *    Prints the key fields and the value fields to the global output
 *    stream 'output.of_fp'.
 */
static void
writeAsciiRecord(
    skheapnode_t        heap_ptr)
{
    rwRec rwrec;
    uint32_t val32;
    uint32_t eTime = 0;
    uint16_t dport = 0;
    sk_fieldlist_iterator_t fl_iter;
    sk_fieldentry_t *field;
    int id;

#if  SK_ENABLE_IPV6
    /* whether IPv4 addresses have been added to a record */
    int added_ipv4 = 0;
    uint8_t ipv6[16];
#endif

    /* in mixed IPv4/IPv6 setting, keep record as IPv4 unless an IPv6
     * address forces us to use IPv6. */
#define KEY_TO_REC_IPV6(func_v6, func_v4, rec, field_buf, field_list, field) \
    skFieldListExtractFromBuffer(key_fields, field_buf, field, ipv6);   \
    if (rwRecIsIPv6(rec)) {                                             \
        /* record is already IPv6 */                                    \
        func_v6((rec), ipv6);                                           \
    } else if (SK_IPV6_IS_V4INV6(ipv6)) {                               \
        /* record is IPv4, and so is the IP */                          \
        func_v4((rec), ntohl(*(uint32_t*)(ipv6 + SK_IPV6_V4INV6_LEN))); \
        added_ipv4 = 1;                                                 \
    } else {                                                            \
        /* address is IPv6, but record is IPv4 */                       \
        if (added_ipv4) {                                               \
            /* record has IPv4 addrs; must convert */                   \
            rwRecConvertToIPv6(rec);                                    \
        } else {                                                        \
            /* no addresses on record yet */                            \
            rwRecSetIPv6(rec);                                          \
        }                                                               \
        func_v6((rec), ipv6);                                           \
    }

    /* Zero out rwrec to avoid display errors---specifically with msec
     * fields and eTime. */
    RWREC_CLEAR(&rwrec);

    /* Initialize the protocol to 1 (ICMP), so that if the user has
     * requested ICMP type/code but the protocol is not part of the
     * key, we still get ICMP values. */
    rwRecSetProto(&rwrec, IPPROTO_ICMP);

#if SK_ENABLE_IPV6
    if (ipv6_policy > SK_IPV6POLICY_MIX) {
        /* Force records to be in IPv6 format */
        rwRecSetIPv6(&rwrec);
    }
#endif /* SK_ENABLE_IPV6 */

    /* unpack the key into 'rwrec' */
    skFieldListIteratorBind(key_fields, &fl_iter);
    while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
        id = skFieldListEntryGetId(field);
        switch (id) {
#if SK_ENABLE_IPV6
          case SK_FIELD_SIPv6:
            KEY_TO_REC_IPV6(rwRecMemSetSIPv6, rwRecSetSIPv4, &rwrec,
                            HEAP_PTR_KEY(heap_ptr), key_fields, field);
            break;
          case SK_FIELD_DIPv6:
            KEY_TO_REC_IPV6(rwRecMemSetDIPv6, rwRecSetDIPv4, &rwrec,
                            HEAP_PTR_KEY(heap_ptr), key_fields, field);
            break;
          case SK_FIELD_NHIPv6:
            KEY_TO_REC_IPV6(rwRecMemSetNhIPv6, rwRecSetNhIPv4, &rwrec,
                            HEAP_PTR_KEY(heap_ptr), key_fields, field);
            break;
#endif  /* SK_ENABLE_IPV6 */

          case SK_FIELD_SIPv4:
            KEY_TO_REC_32(rwRecSetSIPv4, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_DIPv4:
            KEY_TO_REC_32(rwRecSetDIPv4, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_NHIPv4:
            KEY_TO_REC_32(rwRecSetNhIPv4, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_SPORT:
            KEY_TO_REC_16(rwRecSetSPort, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_DPORT:
            /* just extract dPort; we will set it later to ensure
             * dPort takes precedence over ICMP type/code */
            skFieldListExtractFromBuffer(key_fields, HEAP_PTR_KEY(heap_ptr),
                                         field, (uint8_t*)&dport);
            break;
          case SK_FIELD_ICMP_TYPE:
            KEY_TO_REC_08(rwRecSetIcmpType, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_ICMP_CODE:
            KEY_TO_REC_08(rwRecSetIcmpCode, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_PROTO:
            KEY_TO_REC_08(rwRecSetProto, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_PACKETS:
            KEY_TO_REC_32(rwRecSetPkts, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_BYTES:
            KEY_TO_REC_32(rwRecSetBytes, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_FLAGS:
            KEY_TO_REC_08(rwRecSetFlags, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_SID:
            KEY_TO_REC_16(rwRecSetSensor, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_INPUT:
            KEY_TO_REC_16(rwRecSetInput, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_OUTPUT:
            KEY_TO_REC_16(rwRecSetOutput, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_INIT_FLAGS:
            KEY_TO_REC_08(rwRecSetInitFlags, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_REST_FLAGS:
            KEY_TO_REC_08(rwRecSetRestFlags, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_TCP_STATE:
            KEY_TO_REC_08(rwRecSetTcpState, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_APPLICATION:
            KEY_TO_REC_16(rwRecSetApplication, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_FTYPE_CLASS:
          case SK_FIELD_FTYPE_TYPE:
            KEY_TO_REC_08(rwRecSetFlowType, &rwrec, HEAP_PTR_KEY(heap_ptr),
                          key_fields, field);
            break;
          case SK_FIELD_STARTTIME:
          case SK_FIELD_STARTTIME_MSEC:
            skFieldListExtractFromBuffer(key_fields, HEAP_PTR_KEY(heap_ptr),
                                         field, (uint8_t*)&val32);
            rwRecSetStartTime(&rwrec, sktimeCreate(val32, 0));
            break;
          case SK_FIELD_ELAPSED:
          case SK_FIELD_ELAPSED_MSEC:
            skFieldListExtractFromBuffer(key_fields, HEAP_PTR_KEY(heap_ptr),
                                         field, (uint8_t*)&val32);
            rwRecSetElapsed(&rwrec, val32 * 1000);
            break;
          case SK_FIELD_ENDTIME:
          case SK_FIELD_ENDTIME_MSEC:
            /* just extract eTime; we will set it later */
            skFieldListExtractFromBuffer(key_fields, HEAP_PTR_KEY(heap_ptr),
                                         field, (uint8_t*)&eTime);
            break;
          default:
            assert(skFieldListEntryGetId(field) == SK_FIELD_CALLER);
            break;
        }
    }

    if (dport_key) {
        rwRecSetDPort(&rwrec, dport);
    }

    switch (time_fields_key) {
      case PARSE_KEY_ETIME:
        /* etime only; just set sTime to eTime--elapsed is already 0 */
        rwRecSetStartTime(&rwrec, sktimeCreate(eTime, 0));
        break;
      case (PARSE_KEY_ELAPSED | PARSE_KEY_ETIME):
        /* etime and elapsed; set start time based on end time and elapsed */
        val32 = rwRecGetElapsedSeconds(&rwrec);
        rwRecSetStartTime(&rwrec, sktimeCreate((eTime - val32), 0));
        break;
      case (PARSE_KEY_STIME | PARSE_KEY_ETIME):
        /* etime and stime; set elapsed as their difference */
        val32 = rwRecGetStartSeconds(&rwrec);
        assert(val32 <= eTime);
        rwRecSetElapsed(&rwrec, (1000 * (eTime - val32)));
        break;
      case PARSE_KEY_ALL_TIMES:
        /* 'time_fields_key' should contain 0, 1, or 2 time values */
        skAbortBadCase(time_fields_key);
      default:
        assert(0 == time_fields_key
               || PARSE_KEY_STIME == time_fields_key
               || PARSE_KEY_ELAPSED == time_fields_key
               || (PARSE_KEY_STIME | PARSE_KEY_ELAPSED) == time_fields_key);
        break;
    }

    /* print everything */
    rwAsciiPrintRecExtra(ascii_str, &rwrec, heap_ptr);
}


/*
 *  rwstatsPrintHeap();
 *
 *    Loop over nodes of the heap and print each, as well as the
 *    percentage columns.
 */
static void
rwstatsPrintHeap(
    void)
{
    skheapiterator_t *itheap;
    skheapnode_t heap_ptr;
    double cumul_pct = 0.0;
    double percent;
    uint64_t val64;
    uint32_t val32;

    /* print the headings and column titles */
    topnPrintHeader();

    skHeapSortEntries(heap);

    itheap = skHeapIteratorCreate(heap, -1);

    if (app_flags.no_percents) {
        while (skHeapIteratorNext(itheap, &heap_ptr) == SKHEAP_OK) {
            writeAsciiRecord(heap_ptr);
        }
    } else {
        switch (limit.fl_id) {
          case SK_FIELD_RECORDS:
            while (skHeapIteratorNext(itheap, &heap_ptr) == SKHEAP_OK) {
                writeAsciiRecord(heap_ptr);
                skFieldListExtractFromBuffer(value_fields,
                                             HEAP_PTR_VALUE(heap_ptr),
                                             limit.fl_entry, (uint8_t*)&val32);
                percent = 100.0 * (double)val32 / value_total;
                cumul_pct += percent;
                fprintf(output.of_fp, ("%c%*.6f%c%*.6f%s\n"),
                        delimiter, width[WIDTH_PCT], percent, delimiter,
                        width[WIDTH_PCT], cumul_pct, final_delim);
            }
            break;

          case SK_FIELD_SUM_BYTES:
          case SK_FIELD_SUM_PACKETS:
            while (skHeapIteratorNext(itheap, &heap_ptr) == SKHEAP_OK) {
                writeAsciiRecord(heap_ptr);
                skFieldListExtractFromBuffer(value_fields,
                                             HEAP_PTR_VALUE(heap_ptr),
                                             limit.fl_entry, (uint8_t*)&val64);
                percent = 100.0 * (double)val64 / value_total;
                cumul_pct += percent;
                fprintf(output.of_fp, ("%c%*.6f%c%*.6f%s\n"),
                        delimiter, width[WIDTH_PCT], percent, delimiter,
                        width[WIDTH_PCT], cumul_pct, final_delim);
            }
            break;

          default:
            while (skHeapIteratorNext(itheap, &heap_ptr) == SKHEAP_OK) {
                writeAsciiRecord(heap_ptr);
                fprintf(output.of_fp, ("%c%*c%c%*c%s\n"),
                        delimiter, width[WIDTH_PCT], '?', delimiter,
                        width[WIDTH_PCT], '?', final_delim);
            }
        }
    }

    skHeapIteratorFree(itheap);
}


/*
 *  cmp = rwstatsCompareCounts{Top,Btm}{32,64}(node1, node2);
 *
 *    The following 4 functions are invoked by the skHeap library to
 *    compare counters.  'node1' and 'node2' are pointers to an
 *    integer value (either a uint32_t or a uint64_t).
 *
 *    For the *Top* functions, return 1, 0, -1 depending on whether
 *    the value in 'node1' is <, ==, > the value in 'node2'.
 *
 *    For the *Btm* functions, return -1, 0, 1 depending on whether
 *    the value in 'node1' is <, ==, > the value in 'node2'.
 */

#define COMPARE(cmp_a, cmp_b)                                   \
    (((cmp_a) < (cmp_b)) ? -1 : (((cmp_a) > (cmp_b)) ? 1 : 0))

#define CMP_INT_HEAP_VALUES(cmp_out, cmp_type, cmp_a, cmp_b)    \
    {                                                           \
        cmp_type val_a;                                         \
        cmp_type val_b;                                         \
        skFieldListExtractFromBuffer(value_fields,              \
                                     HEAP_PTR_VALUE(cmp_a),     \
                                     limit.fl_entry,            \
                                     (uint8_t*)&val_a);         \
        skFieldListExtractFromBuffer(value_fields,              \
                                     HEAP_PTR_VALUE(cmp_b),     \
                                     limit.fl_entry,            \
                                     (uint8_t*)&val_b);         \
        cmp_out = COMPARE(val_a, val_b);                        \
    }

#define CMP_INT_HEAP_DISTINCTS(cmp_out, cmp_type, cmp_a, cmp_b) \
    {                                                           \
        cmp_type val_a;                                         \
        cmp_type val_b;                                         \
        skFieldListExtractFromBuffer(distinct_fields,           \
                                     HEAP_PTR_DISTINCT(cmp_a),  \
                                     limit.fl_entry,            \
                                     (uint8_t*)&val_a);         \
        skFieldListExtractFromBuffer(distinct_fields,           \
                                     HEAP_PTR_DISTINCT(cmp_b),  \
                                     limit.fl_entry,            \
                                     (uint8_t*)&val_b);         \
        cmp_out = COMPARE(val_a, val_b);                        \
    }


static int
rwstatsCompareValuesTop32(
    const skheapnode_t  node1,
    const skheapnode_t  node2)
{
    int rv;
    CMP_INT_HEAP_VALUES(rv, uint32_t, node1, node2);
    return -rv;
}

static int
rwstatsCompareValuesBottom32(
    const skheapnode_t  node1,
    const skheapnode_t  node2)
{
    int rv;
    CMP_INT_HEAP_VALUES(rv, uint32_t, node1, node2);
    return rv;
}

static int
rwstatsCompareValuesTop64(
    const skheapnode_t  node1,
    const skheapnode_t  node2)
{
    int rv;
    CMP_INT_HEAP_VALUES(rv, uint64_t, node1, node2);
    return -rv;
}

static int
rwstatsCompareValuesBottom64(
    const skheapnode_t  node1,
    const skheapnode_t  node2)
{
    int rv;
    CMP_INT_HEAP_VALUES(rv, uint64_t, node1, node2);
    return rv;
}

static int
rwstatsComparePluginAny(
    const skheapnode_t  node1,
    const skheapnode_t  node2)
{
    skplugin_err_t err;
    int cmp;

    err = skPluginFieldRunBinCompareFn(limit.pi_field, &cmp,
                                       (const uint8_t*)node1,
                                       (const uint8_t*)node2);
    if (err != SKPLUGIN_OK) {
        const char **name;
        skPluginFieldName(limit.pi_field, &name);
        skAppPrintErr(("Plugin-based field %s failed "
                       "binary comparison with error code %d"), name[0], err);
        appExit(EXIT_FAILURE);
    }
    return ((RWSTATS_DIR_TOP == direction) ? -cmp : cmp);
}

static int
rwstatsCompareDistinctsAny(
    const skheapnode_t  node1,
    const skheapnode_t  node2)
{
    union value_un {
        uint8_t   ar[HASHLIB_MAX_VALUE_WIDTH];
        uint64_t  u64;
    } count1, count2;
    size_t len;
    int cmp;

    len = skFieldListEntryGetBinOctets(limit.fl_entry);
    switch (len) {
      case 1:
        CMP_INT_HEAP_DISTINCTS(cmp, uint8_t, node1, node2);
        break;
      case 2:
        CMP_INT_HEAP_DISTINCTS(cmp, uint16_t, node1, node2);
        break;
      case 4:
        CMP_INT_HEAP_DISTINCTS(cmp, uint32_t, node1, node2);
        break;
      case 8:
        CMP_INT_HEAP_DISTINCTS(cmp, uint64_t, node1, node2);
        break;

      case 3:
      case 5:
      case 6:
      case 7:
#if SK_BIG_ENDIAN
        cmp = memcmp(HEAP_PTR_DISTINCT(node1), HEAP_PTR_DISTINCT(node2), len);
#else
        count1.u64 = 0;
        count2.u64 = 0;
        skFieldListExtractFromBuffer(distinct_fields, HEAP_PTR_DISTINCT(node1),
                                     limit.fl_entry, count1.ar);
        skFieldListExtractFromBuffer(distinct_fields, HEAP_PTR_DISTINCT(node2),
                                     limit.fl_entry, count2.ar);
        cmp = COMPARE(count1.u64, count2.u64);
#endif  /* #else of #if SK_BIG_ENDIAN */
        break;

      default:
        skFieldListExtractFromBuffer(distinct_fields, HEAP_PTR_DISTINCT(node1),
                                     limit.fl_entry, count1.ar);
        skFieldListExtractFromBuffer(distinct_fields, HEAP_PTR_DISTINCT(node2),
                                     limit.fl_entry, count2.ar);
        cmp = COMPARE(count1.u64, count2.u64);
        break;
    }

    return ((RWSTATS_DIR_TOP == direction) ? -cmp : cmp);
}


/*
 *  rwstatsThresholdMemory(newnode);
 *
 *    Function called when an attempt to use a variable-sized heap
 *    fails due to lack of memory.
 */
static void
rwstatsThresholdMemory(
    uint8_t            *newnode)
{
    uint8_t *top_heap;

    skAppPrintErr(("Out of memory when attempting to use threshold\n"
                   "\tof %" PRIu64
                   "; using an absolute count of %" PRIu64 " instead"),
                  limit.value[RWSTATS_THRESHOLD], limit.value[RWSTATS_COUNT]);

    /* Add this record assuming a fixed heap size */

    /* Get the node at the top of heap and its value.  This is the
     * smallest value in the topN. */
    skHeapPeekTop(heap, (skheapnode_t*)&top_heap);

    if (cmp_fn(top_heap, newnode) > 0) {
        /* The skUnique element we just read is "better" (for topN,
         * higher than current heap-root's value; for bottomN, lower
         * than current heap-root's value). */
        skHeapReplaceTop(heap, newnode, NULL);
    }
}


/*
 *  ok = statsRandom();
 *
 *    Main control function that processes unsorted input (from files
 *    or from stdin) and fills the heap.  Returns 0 on success, -1 on
 *    failure.
 */
static int
statsRandom(
    void)
{
    uint8_t *top_heap;
    uint8_t newnode[HASHLIB_MAX_KEY_WIDTH + HASHLIB_MAX_VALUE_WIDTH];
    sk_unique_iterator_t *iter;
    uint8_t *outbuf[3] = {NULL, NULL, NULL};
    skstream_t *stream;
    rwRec rwrec;
    int rv = 0;
    size_t len;
    union count_un {
        uint8_t   ar[HASHLIB_MAX_VALUE_WIDTH];
        uint64_t  u64;
        uint32_t  u32;
        uint16_t  u16;
        uint8_t   u8;
    } count;

    /* read SiLK Flow records and insert into the skunique data structure */
    while (0 == (rv = appNextInput(&stream))) {
        while (SKSTREAM_OK == (rv = readRecord(stream, &rwrec))) {
            if (0 != skUniqueAddRecord(uniq, &rwrec)) {
                return -1;
            }
        }
        if (rv != SKSTREAM_ERR_EOF) {
            /* corrupt record in file */
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            skStreamDestroy(&stream);
            return -1;
        }
        skStreamDestroy(&stream);
    }
    if (rv == -1) {
        /* error opening file */
        return -1;
    }

    /* no more input; prepare for output */
    skUniquePrepareForOutput(uniq);

    if (RWSTATS_PERCENTAGE == limit.type) {
        /* the limit is a percentage of bytes, of packets, or of
         * flows; compute the threshold given that we now know the
         * total */
        limit.value[RWSTATS_THRESHOLD]
            = value_total * limit.value[RWSTATS_PERCENTAGE] / 100;
    }

    /* create the iterator over skUnique's bins */
    rv = skUniqueIteratorCreate(uniq, &iter);
    if (rv) {
        skAppPrintErr("Unable to create iterator; err = %d", rv);
        return -1;
    }

    /* branch based on type of limit and type of value */
    if (RWSTATS_COUNT == limit.type) {
        /* fixed-size heap; this is easy to handle */

        /* put the first topn entries into the heap */
        for (heap_num_entries = 0;
             ((heap_num_entries < limit.value[RWSTATS_COUNT])
              && (skUniqueIteratorNext(iter, &outbuf[0], &outbuf[2],&outbuf[1])
                  == SK_ITERATOR_OK));
             ++heap_num_entries)
        {
            ++limit.entries;
            MEMSET_HEAP_NODE(newnode, outbuf[0], outbuf[1], outbuf[2]);
            skHeapInsert(heap, newnode);
        }

        /* drop to code below where we handle adding more entries to a
         * fixed size heap */

    } else if (limit.distinct) {
        /* handling a distinct field */
        len = skFieldListEntryGetBinOctets(limit.fl_entry);
        while (skUniqueIteratorNext(iter, &outbuf[0], &outbuf[2], &outbuf[1])
               == SK_ITERATOR_OK)
        {
            ++limit.entries;

            switch (len) {
              case 1:
                skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                             limit.fl_entry, &count.u8);
                if (!VALUE_MEETS_THRESHOLD(count.u8)) {
                    continue;
                }
                break;

              case 2:
                skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                             limit.fl_entry,
                                             (uint8_t*)&count.u16);
                if (!VALUE_MEETS_THRESHOLD(count.u16)) {
                    continue;
                }
                break;

              case 4:
                skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                             limit.fl_entry,
                                             (uint8_t*)&count.u32);
                if (!VALUE_MEETS_THRESHOLD(count.u32)) {
                    continue;
                }
                break;

              case 8:
                skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                             limit.fl_entry,
                                             (uint8_t*)&count.u64);
                if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                    continue;
                }
                break;

              case 3:
              case 5:
              case 6:
              case 7:
                count.u64 = 0;
#if SK_BIG_ENDIAN
                skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                             limit.fl_entry,
                                             &count.ar[8 - len]);
#else
                skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                             limit.fl_entry, &count.ar[0]);
#endif  /* SK_BIG_ENDIAN */
                if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                    continue;
                }
                break;

              default:
                skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                             limit.fl_entry, count.ar);
                if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                    continue;
                }
                break;
            }

            /* record meets threshold; insert it */
            MEMSET_HEAP_NODE(newnode, outbuf[0], outbuf[1], outbuf[2]);
            if (skHeapInsert(heap, newnode) == SKHEAP_ERR_FULL) {
                /* Cannot grow the heap any more; process remaining
                 * records using this fixed heap size */
                rwstatsThresholdMemory(newnode);
                break;
            }
            /* else insert was successful */
            ++limit.value[RWSTATS_COUNT];
            ++heap_num_entries;
        }

    } else {
        /* handling an aggregate value field */
        len = skFieldListEntryGetBinOctets(limit.fl_entry);
        while (skUniqueIteratorNext(iter, &outbuf[0], &outbuf[2], &outbuf[1])
               == SK_ITERATOR_OK)
        {
            ++limit.entries;

            switch (len) {
              case 1:
                skFieldListExtractFromBuffer(value_fields, outbuf[1],
                                             limit.fl_entry, &count.u8);
                if (!VALUE_MEETS_THRESHOLD(count.u8)) {
                    continue;
                }
                break;

              case 2:
                skFieldListExtractFromBuffer(value_fields, outbuf[1],
                                             limit.fl_entry,
                                             (uint8_t*)&count.u16);
                if (!VALUE_MEETS_THRESHOLD(count.u16)) {
                    continue;
                }
                break;

              case 4:
                skFieldListExtractFromBuffer(value_fields, outbuf[1],
                                             limit.fl_entry,
                                             (uint8_t*)&count.u32);
                if (!VALUE_MEETS_THRESHOLD(count.u32)) {
                    continue;
                }
                break;

              case 8:
                skFieldListExtractFromBuffer(value_fields, outbuf[1],
                                             limit.fl_entry,
                                             (uint8_t*)&count.u64);
                if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                    continue;
                }
                break;

              case 3:
              case 5:
              case 6:
              case 7:
                count.u64 = 0;
#if SK_BIG_ENDIAN
                skFieldListExtractFromBuffer(value_fields, outbuf[1],
                                             limit.fl_entry,
                                             &count.ar[8 - len]);
#else
                skFieldListExtractFromBuffer(value_fields, outbuf[1],
                                             limit.fl_entry, &count.ar[0]);
#endif  /* SK_BIG_ENDIAN */
                if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                    continue;
                }
                break;

              default:
                skFieldListExtractFromBuffer(value_fields, outbuf[1],
                                             limit.fl_entry, count.ar);
                if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                    continue;
                }
                break;
            }

            /* record meets threshold; insert it */
            MEMSET_HEAP_NODE(newnode, outbuf[0], outbuf[1], outbuf[2]);
            if (skHeapInsert(heap, newnode) == SKHEAP_ERR_FULL) {
                /* Cannot grow the heap any more; process remaining
                 * records using this fixed heap size */
                rwstatsThresholdMemory(newnode);
                break;
            }
            /* else insert was successful */
            ++limit.value[RWSTATS_COUNT];
            ++heap_num_entries;
        }
    }

    /* Get the node at the top of heap and its value.  This is the
     * smallest value in the topN. */
    skHeapPeekTop(heap, (skheapnode_t*)&top_heap);

    /* At this point the size of the heap is fixed.  Process the
     * remaining entries in the skUnique hash table---if any */
    while (skUniqueIteratorNext(iter, &outbuf[0], &outbuf[2], &outbuf[1])
           == SK_ITERATOR_OK)
    {
        ++limit.entries;

        MEMSET_HEAP_NODE(newnode, outbuf[0], outbuf[1], outbuf[2]);
        if (cmp_fn(top_heap, newnode) > 0) {
            /* The skUnique element we just read is "better" (for
             * topN, higher than current heap-root's value; for
             * bottomN, lower than current heap-root's value). */
            skHeapReplaceTop(heap, newnode, NULL);

            /* the top may have changed; get the new top */
            skHeapPeekTop(heap, (skheapnode_t*)&top_heap);
        }
    }

    skUniqueIteratorDestroy(&iter);
    return 0;
}


/*
 *  presortedEntryCallback(key, distinct, value, top_heap);
 *
 *    This function is invoked by the skPresortedUnique* library code
 *    to process a key/distinct/value triplet when handling presorted
 *    input.
 */
static int
presortedEntryCallback(
    const uint8_t      *key,
    const uint8_t      *distinct,
    const uint8_t      *value,
    void               *top_heap)
{
    uint8_t newnode[HASHLIB_MAX_KEY_WIDTH + HASHLIB_MAX_VALUE_WIDTH];
    size_t len;
    union count_un {
        uint8_t   ar[HASHLIB_MAX_VALUE_WIDTH];
        uint64_t  u64;
        uint32_t  u32;
        uint16_t  u16;
        uint8_t   u8;
    } count;

    ++limit.entries;

    if (NULL != *(skheapnode_t*)top_heap) {
        /* heap is now full.  exchange entries if the current node is
         * better than the worst node in the heap, which is at the
         * root */
        MEMSET_HEAP_NODE(newnode, key, value, distinct);
        if (cmp_fn(*(skheapnode_t*)top_heap, newnode) > 0) {
            skHeapReplaceTop(heap, newnode, NULL);

            /* the top may have changed; get the new top */
            skHeapPeekTop(heap, (skheapnode_t*)top_heap);
        }

    } else if (RWSTATS_COUNT == limit.type) {
        /* there is still room in the heap */
        MEMSET_HEAP_NODE(newnode, key, value, distinct);
        skHeapInsert(heap, newnode);
        ++heap_num_entries;
        if (heap_num_entries == limit.value[RWSTATS_COUNT]) {
            /* no more room; get the top element */
            skHeapPeekTop(heap, (skheapnode_t*)top_heap);
        }

    } else if (limit.distinct) {
        /* handling a distinct field */
        len = skFieldListEntryGetBinOctets(limit.fl_entry);

        switch (len) {
          case 1:
            skFieldListExtractFromBuffer(distinct_fields, distinct,
                                         limit.fl_entry, &count.u8);
            if (!VALUE_MEETS_THRESHOLD(count.u8)) {
                return 0;
            }
            break;

          case 2:
            skFieldListExtractFromBuffer(distinct_fields, distinct,
                                         limit.fl_entry,
                                         (uint8_t*)&count.u16);
            if (!VALUE_MEETS_THRESHOLD(count.u16)) {
                return 0;
            }
            break;

          case 4:
            skFieldListExtractFromBuffer(distinct_fields, distinct,
                                         limit.fl_entry,
                                         (uint8_t*)&count.u32);
            if (!VALUE_MEETS_THRESHOLD(count.u32)) {
                return 0;
            }
            break;

          case 8:
            skFieldListExtractFromBuffer(distinct_fields, distinct,
                                         limit.fl_entry,
                                         (uint8_t*)&count.u64);
            if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                return 0;
            }
            break;

          case 3:
          case 5:
          case 6:
          case 7:
            count.u64 = 0;
#if SK_BIG_ENDIAN
            skFieldListExtractFromBuffer(distinct_fields, distinct,
                                         limit.fl_entry,
                                         &count.ar[8 - len]);
#else
            skFieldListExtractFromBuffer(distinct_fields, distinct,
                                         limit.fl_entry, &count.ar[0]);
#endif  /* SK_BIG_ENDIAN */
            if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                return 0;
            }
            break;

          default:
            skFieldListExtractFromBuffer(distinct_fields, distinct,
                                         limit.fl_entry, count.ar);
            if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                return 0;
            }
            break;
        }

        /* record meets threshold; insert it */
        MEMSET_HEAP_NODE(newnode, key, value, distinct);
        if (skHeapInsert(heap, newnode) == SKHEAP_OK) {
            ++limit.value[RWSTATS_COUNT];
        } else {
            /* Cannot grow the heap any more; process remaining
             * records using this fixed heap size */
            rwstatsThresholdMemory(newnode);
            /* no more room; get the top element */
            skHeapPeekTop(heap, (skheapnode_t*)top_heap);
        }

    } else {
        /* handling an aggregate value field */
        len = skFieldListEntryGetBinOctets(limit.fl_entry);

        switch (len) {
          case 1:
            skFieldListExtractFromBuffer(value_fields, value,
                                         limit.fl_entry, &count.u8);
            if (!VALUE_MEETS_THRESHOLD(count.u8)) {
                return 0;
            }
            break;

          case 2:
            skFieldListExtractFromBuffer(value_fields, value,
                                         limit.fl_entry,
                                         (uint8_t*)&count.u16);
            if (!VALUE_MEETS_THRESHOLD(count.u16)) {
                return 0;
            }
            break;

          case 4:
            skFieldListExtractFromBuffer(value_fields, value,
                                         limit.fl_entry,
                                         (uint8_t*)&count.u32);
            if (!VALUE_MEETS_THRESHOLD(count.u32)) {
                return 0;
            }
            break;

          case 8:
            skFieldListExtractFromBuffer(value_fields, value,
                                         limit.fl_entry,
                                         (uint8_t*)&count.u64);
            if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                return 0;
            }
            break;

          case 3:
          case 5:
          case 6:
          case 7:
            count.u64 = 0;
#if SK_BIG_ENDIAN
            skFieldListExtractFromBuffer(value_fields, value,
                                         limit.fl_entry,
                                         &count.ar[8 - len]);
#else
            skFieldListExtractFromBuffer(value_fields, value,
                                         limit.fl_entry, &count.ar[0]);
#endif  /* SK_BIG_ENDIAN */
            if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                return 0;
            }
            break;

          default:
            skFieldListExtractFromBuffer(value_fields, value,
                                         limit.fl_entry, count.ar);
            if (!VALUE_MEETS_THRESHOLD(count.u64)) {
                return 0;
            }
            break;
        }

        /* record meets threshold; insert it */
        MEMSET_HEAP_NODE(newnode, key, value, distinct);
        if (skHeapInsert(heap, newnode) == SKHEAP_OK) {
            ++limit.value[RWSTATS_COUNT];
        } else {
            /* Cannot grow the heap any more; process remaining
             * records using this fixed heap size */
            rwstatsThresholdMemory(newnode);
            /* no more room; get the top element */
            skHeapPeekTop(heap, (skheapnode_t*)top_heap);
        }
    }

    return 0;
}


/*
 *  ok = statsPresorted();
 *
 *    Main control function that reads presorted flow records from
 *    files or stdin and fills the heap.  Returns 0 on success, -1 on
 *    failure.
 */
static int
statsPresorted(
    void)
{
    skheapnode_t top_heap = NULL;

    if (skPresortedUniqueProcess(ps_uniq, presortedEntryCallback, &top_heap)) {
        skAppPrintErr("Unique processing failed");
        return -1;
    }
    return 0;
}


/*
 *  stats = topnMain();
 *
 *    Function used when the user requests a top-N or bottom-N
 *    calculation.  This function initializes parameters used by the
 *    heap, creates the heap, invokes a function to handle the input
 *    and filling of the heap, and finally prints the heap, and
 *    destroys it.
 */
static int
topnMain(
    void)
{
    uint32_t initial_entries;
    int rv;

    /* set comparison function */
    if (limit.distinct) {
        cmp_fn = &rwstatsCompareDistinctsAny;
    } else {
        switch (DIR_AND_TYPE(direction, limit.fl_id)) {
          case DIR_AND_TYPE(RWSTATS_DIR_TOP, SK_FIELD_RECORDS):
            cmp_fn = &rwstatsCompareValuesTop32;
            break;

          case DIR_AND_TYPE(RWSTATS_DIR_BOTTOM, SK_FIELD_RECORDS):
            cmp_fn = &rwstatsCompareValuesBottom32;
            break;

          case DIR_AND_TYPE(RWSTATS_DIR_TOP, SK_FIELD_SUM_BYTES):
          case DIR_AND_TYPE(RWSTATS_DIR_TOP, SK_FIELD_SUM_PACKETS):
            cmp_fn = &rwstatsCompareValuesTop64;
            break;

          case DIR_AND_TYPE(RWSTATS_DIR_BOTTOM, SK_FIELD_SUM_BYTES):
          case DIR_AND_TYPE(RWSTATS_DIR_BOTTOM, SK_FIELD_SUM_PACKETS):
            cmp_fn = &rwstatsCompareValuesBottom64;
            break;

          case DIR_AND_TYPE(RWSTATS_DIR_TOP, SK_FIELD_CALLER):
          case DIR_AND_TYPE(RWSTATS_DIR_BOTTOM, SK_FIELD_CALLER):
            cmp_fn = &rwstatsComparePluginAny;
            break;

          default:
            skAbortBadCase(DIR_AND_TYPE(direction, limit.fl_id));
        }
    }

    /* set up the byte lengths and offsets for the heap */
    heap_octets_key = skFieldListGetBufferSize(key_fields);
    heap_octets_value = skFieldListGetBufferSize(value_fields);
    heap_octets_distinct = skFieldListGetBufferSize(distinct_fields);

    heap_octets_node = heap_octets_key+heap_octets_value+heap_octets_distinct;

    /* heap node contains (VALUE, DISTINCT, KEY) */
    heap_offset_value = 0;
    heap_offset_distinct = heap_offset_value + heap_octets_value;
    heap_offset_key = heap_offset_distinct + heap_octets_distinct;

    /* get the initial size of the heap */
    if (RWSTATS_COUNT == limit.type) {
        /* fixed size */
        initial_entries = limit.value[RWSTATS_COUNT];
    } else {
        /* guess the initial size of the heap and allow the heap to
         * grow if the guess is too small */
        initial_entries = HEAP_INITIAL_SIZE;
    }

    /* create the heap */
    heap = skHeapCreate(cmp_fn, initial_entries, heap_octets_node, NULL);
    if (NULL == heap) {
        skAppPrintErr(("Unable to create heap of %" PRIu32
                       " %" PRIu32 "-byte elements"),
                      initial_entries, (uint32_t)heap_octets_node);
        exit(EXIT_FAILURE);
    }

    /* read the flow records and fill the heap */
    if (app_flags.presorted_input) {
        rv = statsPresorted();
    } else {
        rv = statsRandom();
    }
    if (rv) {
        skHeapFree(heap);
        appExit(EXIT_FAILURE);
    }

    /* print the results */
    rwstatsPrintHeap();

    skHeapFree(heap);

    return (distinct_err ? RWSTATS_NO_MEMORY_EXIT_CODE : 0);
}


int main(int argc, char **argv)
{
    int rv = 0;

    /* Global setup */
    appSetup(argc, argv);

    if (proto_stats) {
        rv = protoStatsMain();
    } else {
        rv = topnMain();
    }

    /* Done, do cleanup */
    appTeardown();
    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
