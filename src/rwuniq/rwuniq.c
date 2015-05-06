/*
** Copyright (C) 2001-2015 by Carnegie Mellon University.
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
**  rwuniq.c
**
**  Implementation of the rwuniq application.
**
**  rwuniq reads SiLK flow records---from files listed on the command
**  line or from the standard input when no filenames are given---and
**  bins those flows by a key composed of user-selected fields of an
**  rwRec, or by fields generated from a plug-in.  For each bin, a
**  user-selected combination of bytes, packets, flows, earliest
**  start-time, latest end-time, distinct sIPs, and/or distinct dIPs
**  may be computed.
**
**  Once the input is read, the keys fields and computed values are
**  printed for each bin that meets the user-specified minimum and
**  maximum.
**
**  Normally, rwuniq uses the hashlib hash table to store the
**  key-volume pairs for each bin.  If this hash table runs out of
**  memory, the contents of the table are sorted and then saved to
**  disk in a temporary file.  More records are then read into a fresh
**  hash table.  The process repeats until all records are read or the
**  maximum number of temp files is reached.  The on-disk files are
**  then merged to produce the final output.
**
**  When the --presorted-input switch is given, rwuniq assumes rwsort
**  has been used to sort the data with the same --fields value that
**  rwuniq is using.  In this case, the hash table is not used.
**  Instead, rwuniq just watches for the key to change, and prints the
**  key-volume when it does.
**
**  For the --presorted-input case or when more than one distinct IP
**  count is requested for the unsorted case, an IPSet is used to keep
**  track of the IPs we have seen.  Since IPSets do not yet support
**  IPv6, this limits rwuniq's ability when IPv6 is active.  Also,
**  these IPSets can exhaust the ram, which would lead to an incorrect
**  count of IPs.  Could consider using a hashlib instead of an IPSet
**  for the values to get around the IPv6 issue.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwuniq.c c19d64b322b8 2015-02-09 22:11:47Z mthomas $");

#include "rwuniq.h"


/* TYPEDEFS AND DEFINES */

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



/* EXPORTED VARIABLES */

sk_unique_t *uniq;
sk_sort_unique_t *ps_uniq;

sk_fieldlist_t *key_fields;
sk_fieldlist_t *value_fields;
sk_fieldlist_t *distinct_fields;

/* to convert the key fields (as an rwRec) to ascii */
rwAsciiStream_t *ascii_str;

/* flags set by the user options */
app_flags_t app_flags;

/* which of elapsed, sTime, and eTime are part of the key. uses the
 * PARSE_KEY_* values from rwuniq.h */
unsigned int time_fields_key = 0;

/* whether dPort is part of the key */
unsigned int dport_key = 0;

/* how to handle IPv6 flows */
sk_ipv6policy_t ipv6_policy = SK_IPV6POLICY_MIX;

/* Information about each potential "value" field the user can choose
 * to compute and display.  Ensure these appear in same order as in
 * the OPT_BYTES...OPT_DIP_DISTINCT values in appOptionsEnum. */
builtin_field_t builtin_values[] = {
    /* title, min, max, text_len, id, is_distinct, all_counts, switched_on,
     * description */
    {"Bytes",          0, UINT64_MAX, 20, SK_FIELD_SUM_BYTES,     0, 1, 0,
     "Sum of bytes for all flows in the group"},
    {"Packets",        0, UINT64_MAX, 15, SK_FIELD_SUM_PACKETS,   0, 1, 0,
     "Sum of packets for all flows in the group"},
    {"Records",        0, UINT64_MAX, 10, SK_FIELD_RECORDS,       0, 1, 0,
     "Number of flow records in the group"},
    {"sTime-Earliest", 0, UINT64_MAX, 19, SK_FIELD_MIN_STARTTIME, 0, 1, 0,
     "Minimum starting time for flows in the group"},
    {"eTime-Latest",   0, UINT64_MAX, 19, SK_FIELD_MAX_ENDTIME,   0, 1, 0,
     "Maximum ending time for flows in the group"},
    {"sIP-Distinct",   0, UINT64_MAX, 10, SK_FIELD_SIPv4,         1, 0, 0,
     "Number of distinct source IPs in the group"},
    {"dIP-Distinct",   0, UINT64_MAX, 10, SK_FIELD_DIPv4,         1, 0, 0,
     "Number of distinct destination IPs in the group"},
    {"Distinct",       0, UINT64_MAX, 10, SK_FIELD_CALLER,        1, 0, 0,
     "You must append a colon and a key field to count the number of"
     " distinct values seen for that field in the group"}
};

const size_t num_builtin_values = (sizeof(builtin_values)/
                                   sizeof(builtin_field_t));


/* FUNCTION DEFINITIONS */

/*
 *  writeColTitles();
 *
 *    Enable the pager, and print the column titles to the global
 *    'output.fp'.
 */
static void
writeColTitles(
    void)
{
    setOutputHandle();
    rwAsciiPrintTitles(ascii_str);
}


/*
 *  writeAsciiRecord(key, value, distinct);
 *
 *    Verifies that the 'value' and 'distincts' values are within the
 *    limits specified by the user.  If they are not, the function
 *    returns without printing anything.
 *
 *    Unpacks the fields from 'key' and prints the key fields, the
 *    value fields, and the distinct fields to the global output
 *    stream 'output.fp'.
 */
static void
writeAsciiRecord(
    uint8_t           **outbuf)
{
    rwRec rwrec;
    uint64_t val64;
    uint32_t val32;
    uint32_t eTime = 0;
    uint16_t dport = 0;
    sk_fieldlist_iterator_t fl_iter;
    sk_fieldentry_t *field;
    builtin_field_t *bf;
    int id;

#if  SK_ENABLE_IPV6
    /* whether IPv4 addresses have been added to a record */
    int added_ipv4 = 0;
    uint8_t ipv6[16];
    ipv6_distinct_t val_ip6;
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

    /* see if values are within limits */
    if (app_flags.check_limits) {
        skFieldListIteratorBind(value_fields, &fl_iter);
        while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
            bf = (builtin_field_t*)skFieldListEntryGetContext(field);
            switch (skFieldListEntryGetId(field)) {
              case SK_FIELD_SUM_BYTES:
              case SK_FIELD_SUM_PACKETS:
                skFieldListExtractFromBuffer(value_fields, outbuf[1], field,
                                             (uint8_t*)&val64);
                if ((val64 < bf->bf_min)
                    || (val64 > bf->bf_max))
                {
                    return;
                }
                break;

              case SK_FIELD_RECORDS:
              case SK_FIELD_SUM_ELAPSED:
                skFieldListExtractFromBuffer(value_fields, outbuf[1], field,
                                             (uint8_t*)&val32);
                if ((val32 < bf->bf_min)
                    || (val32 > bf->bf_max))
                {
                    return;
                }
                break;

              default:
                break;
            }
        }
        skFieldListIteratorBind(distinct_fields, &fl_iter);
        while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
            bf = (builtin_field_t*)skFieldListEntryGetContext(field);
            if (bf) {
                switch (skFieldListEntryGetId(field)) {
#if  SK_ENABLE_IPV6
                  case SK_FIELD_SIPv6:
                  case SK_FIELD_DIPv6:
                    skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                                 field, val_ip6.ip);
                    if ((val_ip6.count < bf->bf_min)
                        || (val_ip6.count > bf->bf_max))
                    {
                        return;
                    }
                    break;
#endif  /* SK_ENABLE_IPV6 */

                  case SK_FIELD_SIPv4:
                  case SK_FIELD_DIPv4:
                    skFieldListExtractFromBuffer(distinct_fields, outbuf[2],
                                                 field, (uint8_t*)&val32);
                    if ((val32 < bf->bf_min)
                        || (val32 > bf->bf_max))
                    {
                        return;
                    }
                    break;

                  default:
                    break;
                }
            }
        }
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
                            outbuf[0], key_fields, field);
            break;
          case SK_FIELD_DIPv6:
            KEY_TO_REC_IPV6(rwRecMemSetDIPv6, rwRecSetDIPv4, &rwrec,
                            outbuf[0], key_fields, field);
            break;
          case SK_FIELD_NHIPv6:
            KEY_TO_REC_IPV6(rwRecMemSetNhIPv6, rwRecSetNhIPv4, &rwrec,
                            outbuf[0], key_fields, field);
            break;
#endif  /* SK_ENABLE_IPV6 */

          case SK_FIELD_SIPv4:
            KEY_TO_REC_32(rwRecSetSIPv4, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_DIPv4:
            KEY_TO_REC_32(rwRecSetDIPv4, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_NHIPv4:
            KEY_TO_REC_32(rwRecSetNhIPv4, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_SPORT:
            KEY_TO_REC_16(rwRecSetSPort, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_DPORT:
            /* just extract dPort; we will set it later to ensure
             * dPort takes precedence over ICMP type/code */
            skFieldListExtractFromBuffer(key_fields, outbuf[0],
                                         field, (uint8_t*)&dport);
            break;
          case SK_FIELD_ICMP_TYPE:
            KEY_TO_REC_08(rwRecSetIcmpType, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_ICMP_CODE:
            KEY_TO_REC_08(rwRecSetIcmpCode, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_PROTO:
            KEY_TO_REC_08(rwRecSetProto, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_PACKETS:
            KEY_TO_REC_32(rwRecSetPkts, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_BYTES:
            KEY_TO_REC_32(rwRecSetBytes, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_FLAGS:
            KEY_TO_REC_08(rwRecSetFlags, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_SID:
            KEY_TO_REC_16(rwRecSetSensor, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_INPUT:
            KEY_TO_REC_16(rwRecSetInput, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_OUTPUT:
            KEY_TO_REC_16(rwRecSetOutput, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_INIT_FLAGS:
            KEY_TO_REC_08(rwRecSetInitFlags, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_REST_FLAGS:
            KEY_TO_REC_08(rwRecSetRestFlags, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_TCP_STATE:
            KEY_TO_REC_08(rwRecSetTcpState, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_APPLICATION:
            KEY_TO_REC_16(rwRecSetApplication, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_FTYPE_CLASS:
          case SK_FIELD_FTYPE_TYPE:
            KEY_TO_REC_08(rwRecSetFlowType, &rwrec, outbuf[0],
                          key_fields, field);
            break;
          case SK_FIELD_STARTTIME:
          case SK_FIELD_STARTTIME_MSEC:
            skFieldListExtractFromBuffer(key_fields, outbuf[0],
                                         field, (uint8_t*)&val32);
            rwRecSetStartTime(&rwrec, sktimeCreate(val32, 0));
            break;
          case SK_FIELD_ELAPSED:
          case SK_FIELD_ELAPSED_MSEC:
            skFieldListExtractFromBuffer(key_fields, outbuf[0],
                                         field, (uint8_t*)&val32);
            rwRecSetElapsed(&rwrec, val32 * 1000);
            break;
          case SK_FIELD_ENDTIME:
          case SK_FIELD_ENDTIME_MSEC:
            /* just extract eTime; we will set it later */
            skFieldListExtractFromBuffer(key_fields, outbuf[0],
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
    rwAsciiPrintRecExtra(ascii_str, &rwrec, outbuf);
}


/*
 *  uniqRandom();
 *
 *    Main control function that creates a hash table, processes the
 *    input (files or stdin), and prints the results.
 */
static void
uniqRandom(
    void)
{
    sk_unique_iterator_t *iter;
    uint8_t *outbuf[3];
    skstream_t *stream;
    rwRec rwrec;
    int rv = 0;

    while (0 == (rv = appNextInput(&stream))) {
        while (SKSTREAM_OK == (rv = readRecord(stream, &rwrec))) {
            if (0 != skUniqueAddRecord(uniq, &rwrec)) {
                appExit(EXIT_FAILURE);
            }
        }
        if (rv != SKSTREAM_ERR_EOF) {
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            skStreamDestroy(&stream);
            return;
        }
        skStreamDestroy(&stream);
    }
    if (rv == -1) {
        /* error reading file */
        appExit(EXIT_FAILURE);
    }

    /* Write out the headings */
    writeColTitles();

    skUniquePrepareForOutput(uniq);

    /* create the iterator */
    rv = skUniqueIteratorCreate(uniq, &iter);
    if (rv) {
        skAppPrintErr("Unable to create iterator; err = %d", rv);
        appExit(EXIT_FAILURE);
    }

    while (skUniqueIteratorNext(iter, &outbuf[0], &outbuf[2], &outbuf[1])
           == SK_ITERATOR_OK)
    {
        writeAsciiRecord(outbuf);
    }
    skUniqueIteratorDestroy(&iter);

    return;
}


static int
presortedEntryCallback(
    const uint8_t          *key,
    const uint8_t          *distinct,
    const uint8_t          *value,
    void            UNUSED(*callback_data))
{
    uint8_t *outbuf[3];

    outbuf[0] = (uint8_t*)key;
    outbuf[1] = (uint8_t*)value;
    outbuf[2] = (uint8_t*)distinct;

    writeAsciiRecord(outbuf);
    return 0;
}


/*
 *  uniqPresorted();
 *
 *    Main control function that reads presorted flow records from
 *    files or stdin and prints the results.
 */
static void
uniqPresorted(
    void)
{
    /* Write the headings */
    writeColTitles();

    if (skPresortedUniqueProcess(ps_uniq, presortedEntryCallback, NULL)) {
        skAppPrintErr("Unique processing failed");
    }
}


int main(int argc, char **argv)
{
    /* Global setup */
    appSetup(argc, argv);

    if (app_flags.presorted_input) {
        uniqPresorted();
    } else {
        uniqRandom();
    }

    /* Done, do cleanup */
    appTeardown();

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
