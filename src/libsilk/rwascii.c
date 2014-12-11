/*
** Copyright (C) 2005-2014 by Carnegie Mellon University.
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
**  Functions to support printing a record as text.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwascii.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/rwascii.h>
#include <silk/skipaddr.h>
#include <silk/skstream.h>
#include <silk/rwrec.h>
#include <silk/sksite.h>
#include <silk/skstringmap.h>
#include <silk/utils.h>


/* size of buffer that gets filled with a column's value */
#define RWASCII_BUF_SIZE  2048

/* how we know a field contains a callback */
#define RWASCII_CB_FIELD_ID        UINT32_MAX
#define RWASCII_CB_EXTRA_FIELD_ID  (UINT32_MAX-1)


typedef struct rwascii_field_st {
    uint32_t                            af_field_id;
    uint32_t                            af_width;
    void                               *af_cb_data;
    rwAsciiStreamGetTitle_t             af_cb_gettitle;
    union af_cb_getvalue_un {
        rwAsciiStreamGetValue_t         gv;
        rwAsciiStreamGetValueExtra_t    gv_extra;
    }                                   af_cb_getvalue;
} rwascii_field_t;


/* typedef rwAsciiStream_st rwAsciiStream_t; */
struct rwAsciiStream_st {
    FILE               *as_out_stream;
    rwascii_field_t    *as_field;
    uint32_t            as_field_count;
    uint32_t            as_field_capacity;
    uint32_t            as_ipformat;
    uint32_t            as_timeflags;
    sk_ipv6policy_t     as_ipv6_policy;
    uint8_t             as_initialized;
    char                as_delimiter;
    unsigned            as_not_columnar     :1;
    unsigned            as_no_titles        :1;
    unsigned            as_integer_sensors  :1;
    unsigned            as_integer_flags    :1;
    unsigned            as_no_final_delim   :1;
    unsigned            as_no_newline       :1;
    unsigned            as_legacy_icmp      :1;
};


/*
 * This struct holds the field names and their IDs.  The same names
 * are used for the column titles.
 *
 * Names that map to the same ID must be put together, with the name
 * that you want to use for the title first, then any aliases
 * afterward.

 * NOTE!! We are assuming that the stringmap code leaves things in the
 * order we insert them and doesn't re-arrange them.  Since that code
 * uses a linked-list, we are safe for now.
 */
static const sk_stringmap_entry_t field_map_entries[] = {
    {"sIP",          RWREC_FIELD_SIP,
     "Source IP address", NULL},
    {"1",            RWREC_FIELD_SIP,               NULL, NULL},
    {"dIP",          RWREC_FIELD_DIP,
     "Destination IP address", NULL},
    {"2",            RWREC_FIELD_DIP,               NULL, NULL},
    {"sPort",        RWREC_FIELD_SPORT,
     "Source port for TCP, UDP, or equivalent", NULL},
    {"3",            RWREC_FIELD_SPORT,             NULL, NULL},
    {"dPort",        RWREC_FIELD_DPORT,
     "Destination port for TCP, UDP, or equivalent", NULL},
    {"4",            RWREC_FIELD_DPORT,             NULL, NULL},
    {"protocol",     RWREC_FIELD_PROTO,
     "IP protocol", NULL},
    {"5",            RWREC_FIELD_PROTO,             NULL, NULL},
    {"packets",      RWREC_FIELD_PKTS,
     "Number of packets in the flow", NULL},
    {"pkts",         RWREC_FIELD_PKTS,              NULL, NULL},
    {"6",            RWREC_FIELD_PKTS,              NULL, NULL},
    {"bytes",        RWREC_FIELD_BYTES,
     "Number of octets (bytes) in the flow", NULL},
    {"7",            RWREC_FIELD_BYTES,             NULL, NULL},
    {"flags",        RWREC_FIELD_FLAGS,
     "Bit-wise OR of TCP flags over all packets [FSRPAUEC]", NULL},
    {"8",            RWREC_FIELD_FLAGS,             NULL, NULL},
    {"sTime",        RWREC_FIELD_STIME,
     "Starting time of the flow", NULL},
    {"9",            RWREC_FIELD_STIME,             NULL, NULL},
    {"duration",     RWREC_FIELD_ELAPSED,
     "Duration of the flow", NULL},
    /* dur is here to avoid conflict between duration and dur+msec */
    {"dur",          RWREC_FIELD_ELAPSED,           NULL, NULL},
    {"10",           RWREC_FIELD_ELAPSED,           NULL, NULL},
    {"eTime",        RWREC_FIELD_ETIME,
     "Ending time of the flow", NULL},
    {"11",           RWREC_FIELD_ETIME,             NULL, NULL},
    {"sensor",       RWREC_FIELD_SID,
     "Name or ID of the sensor as assigned by rwflowpack", NULL},
    {"12",           RWREC_FIELD_SID,               NULL, NULL},
    {"in",           RWREC_FIELD_INPUT,
     "Router SNMP input interface or vlanId", NULL},
    {"13",           RWREC_FIELD_INPUT,             NULL, NULL},
    {"out",          RWREC_FIELD_OUTPUT,
     "Router SNMP output interface or postVlanId", NULL},
    {"14",           RWREC_FIELD_OUTPUT,            NULL, NULL},
    {"nhIP",         RWREC_FIELD_NHIP,
     "Router next-hop IP address", NULL},
    {"15",           RWREC_FIELD_NHIP,              NULL, NULL},
    {"initialFlags", RWREC_FIELD_INIT_FLAGS,
     "TCP flags on first packet in the flow", NULL},
    {"26",           RWREC_FIELD_INIT_FLAGS,        NULL, NULL},
    {"sessionFlags", RWREC_FIELD_REST_FLAGS,
     "Bit-wise OR of TCP flags over second through final packet", NULL},
    {"27",           RWREC_FIELD_REST_FLAGS,        NULL, NULL},
    {"attributes",   RWREC_FIELD_TCP_STATE,
     "Flow attributes set by flow generator [SFTC]", NULL},
    {"28",           RWREC_FIELD_TCP_STATE,         NULL, NULL},
    {"application",  RWREC_FIELD_APPLICATION,
     "Guess as to content of flow (appLabel)", NULL},
    {"29",           RWREC_FIELD_APPLICATION,       NULL, NULL},
    {"class",        RWREC_FIELD_FTYPE_CLASS,
     "Class of the sensor as assigned by rwflowpack", NULL},
    {"20",           RWREC_FIELD_FTYPE_CLASS,       NULL, NULL},
    {"type",         RWREC_FIELD_FTYPE_TYPE,
     "Type within the class as assigned by rwflowpack", NULL},
    {"21",           RWREC_FIELD_FTYPE_TYPE,        NULL, NULL},
    {"sTime+msec",   RWREC_FIELD_STIME_MSEC,
     "Starting time of the flow [DEPRECATED: Use sTime instead]", NULL},
    {"22",           RWREC_FIELD_STIME_MSEC,        NULL, NULL},
    {"eTime+msec",   RWREC_FIELD_ETIME_MSEC,
     "Ending time of the flow [DEPRECATED: Use eTime instead]", NULL},
    {"23",           RWREC_FIELD_ETIME_MSEC,        NULL, NULL},
    {"dur+msec",     RWREC_FIELD_ELAPSED_MSEC,
     "Duration of the flow [DEPRECATED: Use duration instead]", NULL},
    {"24",           RWREC_FIELD_ELAPSED_MSEC,      NULL, NULL},
    {"iType",        RWREC_FIELD_ICMP_TYPE,
     "ICMP type value for ICMP or ICMPv6 flows; empty otherwise", NULL},
    {"iCode",        RWREC_FIELD_ICMP_CODE,
     "ICMP code value for ICMP or ICMPv6 flows; empty otherwise", NULL},

    /* Do not add the following since the "icmp" prefix cause
     * conflicts with "icmpTypeCode" */
    /* {"icmpType",     RWREC_FIELD_ICMP_TYPE,         NULL, NULL}, */
    /* {"icmpCode",     RWREC_FIELD_ICMP_CODE,         NULL, NULL}, */

    SK_STRINGMAP_SENTINEL
};


/* FUNCTION DEFINITIONS */

static int
rwAsciiAllocFields(
    rwAsciiStream_t    *astream,
    uint32_t            capacity)
{
    rwascii_field_t *current_fields;

    if (capacity > 0) {
        if (capacity < astream->as_field_capacity) {
            return 0;
        }
    } else if (astream->as_field_capacity == 0) {
        capacity = sizeof(field_map_entries) / sizeof(field_map_entries[0]);
    } else {
        capacity = 2 * astream->as_field_capacity;
    }

    if (NULL == astream->as_field) {
        assert(0 == astream->as_field_capacity);
        astream->as_field
            = (rwascii_field_t*)calloc(capacity, sizeof(rwascii_field_t));
        if (NULL == astream->as_field) {
            return -1;
        }
        astream->as_field_capacity = capacity;
        return 0;
    }

    assert(astream->as_field_capacity > 0);

    current_fields = astream->as_field;
    astream->as_field
        = (rwascii_field_t*)realloc(astream->as_field,
                                    (capacity * sizeof(rwascii_field_t)));
    if (NULL == astream->as_field) {
        astream->as_field = current_fields;
        return -1;
    }

    memset(astream->as_field + astream->as_field_capacity, 0,
           (capacity - astream->as_field_capacity) * sizeof(rwascii_field_t));
    astream->as_field_capacity = capacity;
    return 0;
}


/*
**  rwAsciiSetWidths(astream);
**
**    Set the column widths for all the columns in the 'astream'
**    struct.  The column widths will depend on flags set by the user,
**    such as whether the output is columnar, or the form of sensors
**    (names or numbers).
*/
static void
rwAsciiSetWidths(
    rwAsciiStream_t    *astream)
{
    rwascii_field_t *field;
    uint32_t i;

    if (astream->as_not_columnar) {
        return;
    }

    for (i = 0, field = astream->as_field;
         i < astream->as_field_count;
         ++i, ++field)
    {
        if (RWASCII_CB_FIELD_ID == astream->as_field[i].af_field_id
            || RWASCII_CB_EXTRA_FIELD_ID == astream->as_field[i].af_field_id)
        {
            /* callback field. we already have the width */
            continue;
        }
        switch ((rwrec_printable_fields_t)field->af_field_id) {
          case RWREC_FIELD_SIP:
          case RWREC_FIELD_DIP:
          case RWREC_FIELD_NHIP:
            /* ip numbers */
#if SK_ENABLE_IPV6
            if (astream->as_ipv6_policy >= SK_IPV6POLICY_MIX) {
                /* If IPv6 support is available and IPv6 flows are not
                 * being ignored */
                if (astream->as_ipformat == SKIPADDR_HEXADECIMAL) {
                    field->af_width = 32;
                } else {
                    field->af_width = 39;
                }
                break;
            }
#endif /* SK_ENABLE_IPV6 */
            if (astream->as_ipformat == SKIPADDR_DECIMAL) {
                field->af_width = 10;
            } else if (astream->as_ipformat == SKIPADDR_HEXADECIMAL) {
                field->af_width = 8;
            } else {
                field->af_width = 15;
            }
            break;

          case RWREC_FIELD_SPORT:
          case RWREC_FIELD_DPORT:
            /* sport and dport */
            field->af_width = 5;
            break;

          case RWREC_FIELD_PROTO:
            /* proto */
            field->af_width = 3;
            break;

          case RWREC_FIELD_PKTS:
          case RWREC_FIELD_BYTES:
            /* packets, bytes */
            field->af_width = 10;
            break;

          case RWREC_FIELD_FLAGS:
          case RWREC_FIELD_INIT_FLAGS:
          case RWREC_FIELD_REST_FLAGS:
            /* tcp flags, init-flags, non-init-flags */
            if (astream->as_integer_flags) {
                field->af_width = 3;
            } else {
                field->af_width = 8;
            }
            break;

          case RWREC_FIELD_TCP_STATE:
            /* tcp-state */
            field->af_width = 8;
            break;

          case RWREC_FIELD_APPLICATION:
            field->af_width = 5;
            break;

          case RWREC_FIELD_ELAPSED:
            /* elapsed/duration */
            if (astream->as_timeflags & SKTIMESTAMP_NOMSEC) {
                field->af_width = 5;
                break;
            }
            /* else fallthrough */

          case RWREC_FIELD_ELAPSED_MSEC:
            /* elapsed/duration with milliseconds */
            field->af_width = 9;
            break;

          case RWREC_FIELD_STIME:
          case RWREC_FIELD_ETIME:
            /* sTime and end time */
            if (astream->as_timeflags & SKTIMESTAMP_NOMSEC) {
                if (astream->as_timeflags & SKTIMESTAMP_EPOCH) {
                    field->af_width = 10;
                } else {
                    field->af_width = 19;
                }
                break;
            }
            /* else fallthrough */

          case RWREC_FIELD_STIME_MSEC:
          case RWREC_FIELD_ETIME_MSEC:
            /* sTime and end time with milliseconds */
            if (astream->as_timeflags & SKTIMESTAMP_EPOCH) {
                field->af_width = 14;
            } else {
                field->af_width = 23;
            }
            break;

          case RWREC_FIELD_SID:
            /* sensor */
            if (astream->as_integer_sensors) {
                field->af_width = 5;
            } else {
                field->af_width = (uint8_t)sksiteSensorGetMaxNameStrLen();
            }
            break;

          case RWREC_FIELD_INPUT:
          case RWREC_FIELD_OUTPUT:
            /* input,output */
            field->af_width = 5;
            break;

          case RWREC_FIELD_FTYPE_CLASS:
            /* flow-type class */
            field->af_width = (uint8_t)sksiteClassGetMaxNameStrLen();
            break;

          case RWREC_FIELD_FTYPE_TYPE:
            /* flow-type type */
            field->af_width = (uint8_t)sksiteFlowtypeGetMaxTypeStrLen();
            break;

          case RWREC_FIELD_ICMP_TYPE:
          case RWREC_FIELD_ICMP_CODE:
            /* single column with ICMP type or code */
            field->af_width = 3;
            break;

        } /* switch */
    }
    return;
}


/*
 *  rwAsciiSetDefaultFields(astream);
 *
 *    Configure the 'astream' to print all fields.
 */
static void
rwAsciiSetDefaultFields(
    rwAsciiStream_t    *astream)
{
    uint32_t i;

    assert(astream->as_field == NULL);

    astream->as_field_count = RWREC_PRINTABLE_FIELD_COUNT;

    if (rwAsciiAllocFields(astream, astream->as_field_count)) {
        /* out of memory */
        skAppPrintOutOfMemory(NULL);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < astream->as_field_count; ++i) {
        astream->as_field[i].af_field_id = i;
    }
}


/*
 *  rwAsciiVerifyIcmpColumns(astream);
 *
 *    If the ICMP type and code are to be written into the sPort and
 *    dPort columns, verify that the sPort and dPort columns are
 *    present, adding them if required.
 */
static void
rwAsciiVerifyIcmpColumns(
    rwAsciiStream_t    *astream)
{
    uint32_t i;
    int have_sport = 0;
    int have_dport = 0;

    for (i = 0; i < astream->as_field_count; ++i) {
        switch (astream->as_field[i].af_field_id) {
          case RWREC_FIELD_SPORT:
            if (have_dport) {
                return;
            }
            have_sport = 1;
            break;

          case RWREC_FIELD_DPORT:
            if (have_sport) {
                return;
            }
            have_dport = 1;
            break;
        }
    }

    /* if we make it here, sport and/or dport are needed */
    if ((astream->as_field_count + 2 - have_dport - have_sport)
        > astream->as_field_capacity)
    {
        if (rwAsciiAllocFields(astream, astream->as_field_count + 2)) {
            /* out of memory */
            skAppPrintOutOfMemory(NULL);
            exit(EXIT_FAILURE);
        }
    }

    if (!have_sport) {
        astream->as_field[astream->as_field_count].af_field_id
            = RWREC_FIELD_SPORT;
        ++astream->as_field_count;
    }
    if (!have_dport) {
        astream->as_field[astream->as_field_count].af_field_id
            = RWREC_FIELD_DPORT;
        ++astream->as_field_count;
    }
}


/*
 *  rwAsciiPreparePrint(astream);
 *
 *    Do any final initialization prior to printing the column titles
 *    or the first row: Set the field list to the default list if the
 *    caller did not choose specific columns; if ICMP type and code
 *    output was requested, make certain the correct columns exist in
 *    the output; and set the width of the columns.
 */
static void
rwAsciiPreparePrint(
    rwAsciiStream_t    *astream)
{
    astream->as_initialized = 1;

    if (astream->as_field_count == 0) {
        rwAsciiSetDefaultFields(astream);
    }

    if (astream->as_legacy_icmp) {
        rwAsciiVerifyIcmpColumns(astream);
    }

    rwAsciiSetWidths(astream);
}


int
rwAsciiFlush(
    rwAsciiStream_t    *astream)
{
    return fflush(astream->as_out_stream);
}


void
rwAsciiStreamDestroy(
    rwAsciiStream_t   **astream)
{
    assert(astream);

    if (NULL == *astream) {
        return;
    }

    if ((*astream)->as_field) {
        free((*astream)->as_field);
        (*astream)->as_field = NULL;
    }
    free(*astream);
    *astream = NULL;
}


int
rwAsciiStreamCreate(
    rwAsciiStream_t   **astream)
{
    assert(astream);

    /* create the object */
    *astream = (rwAsciiStream_t*)calloc(1, sizeof(rwAsciiStream_t));
    if (!*astream) {
        /* out of memory */
        skAppPrintOutOfMemory(NULL);
        return -1;
    }

    /* non-zero defaults */
    (*astream)->as_out_stream = stdout;
    (*astream)->as_delimiter = '|';
#if SK_ENABLE_IPV6
    (*astream)->as_ipv6_policy = SK_IPV6POLICY_MIX;
#else
    (*astream)->as_ipv6_policy = SK_IPV6POLICY_IGNORE;
#endif

    return 0;
}


int
rwAsciiAppendOneField(
    rwAsciiStream_t    *astream,
    const uint32_t      field_id)
{
    return rwAsciiAppendFields(astream, &field_id, 1);
}


int
rwAsciiAppendFields(
    rwAsciiStream_t    *astream,
    const uint32_t     *field_ids,
    uint32_t            field_count)
{
    uint8_t i;

    assert(astream);

    /* verify input */
    if ((field_count == 0) || (field_ids == NULL)) {
        return -1;
    }

    for (i = 0; i < field_count; ++i) {
        if (field_ids[i] >= RWREC_PRINTABLE_FIELD_COUNT) {
            /* bad value */
            skAppPrintErr(("Value '%" PRIu32 "' is not a value field id"),
                          field_ids[i]);
            return -1;
        }
    }

    /* allocate fields */
    if (astream->as_field_count >= astream->as_field_capacity) {
        if (rwAsciiAllocFields(astream, 0)) {
            /* out of memory */
            skAppPrintOutOfMemory(NULL);
            return -1;
        }
    }

    for (i = 0; i < field_count; ++i) {
        astream->as_field[astream->as_field_count].af_field_id = field_ids[i];
        ++astream->as_field_count;
    }

    return 0;
}


static int
asciiAppendCallbackHelper(
    rwAsciiStream_t                *astream,
    rwAsciiStreamGetTitle_t         get_title_fn,
    rwAsciiStreamGetValue_t         get_value_fn,
    rwAsciiStreamGetValueExtra_t    get_value_extra_fn,
    void                           *callback_data,
    uint32_t                        width,
    uint32_t                        field_id)
{
    rwascii_field_t *field;

    if (get_title_fn == NULL) {
        return -1;
    }

    /* allocate fields */
    if (astream->as_field_count >= astream->as_field_capacity) {
        if (rwAsciiAllocFields(astream, 0)) {
            /* out of memory */
            skAppPrintOutOfMemory(NULL);
            return -1;
        }
    }

    field = &astream->as_field[astream->as_field_count];
    field->af_field_id = field_id;
    field->af_cb_gettitle = get_title_fn;
    field->af_cb_data = callback_data;
    field->af_width = width;
    if (RWASCII_CB_FIELD_ID == field_id) {
        field->af_cb_getvalue.gv = get_value_fn;
    } else {
        assert(RWASCII_CB_EXTRA_FIELD_ID == field_id);
        field->af_cb_getvalue.gv_extra = get_value_extra_fn;
    }

    astream->as_field_count++;

    return 0;
}


int
rwAsciiAppendCallbackField(
    rwAsciiStream_t            *astream,
    rwAsciiStreamGetTitle_t     get_title_fn,
    rwAsciiStreamGetValue_t     get_value_fn,
    void                       *callback_data,
    uint32_t                    width)
{
    return asciiAppendCallbackHelper(astream, get_title_fn, get_value_fn,
                                     NULL, callback_data, width,
                                     RWASCII_CB_FIELD_ID);
}


int
rwAsciiAppendCallbackFieldExtra(
    rwAsciiStream_t                *astream,
    rwAsciiStreamGetTitle_t         get_title_fn,
    rwAsciiStreamGetValueExtra_t    get_value_extra_fn,
    void                           *callback_data,
    uint32_t                        width)
{
    return asciiAppendCallbackHelper(astream, get_title_fn, NULL,
                                     get_value_extra_fn, callback_data, width,
                                     RWASCII_CB_EXTRA_FIELD_ID);
}


void
rwAsciiSetOutputHandle(
    rwAsciiStream_t    *astream,
    FILE               *fh)
{
    assert(astream);
    if (fh == NULL) {
        astream->as_out_stream = stdout;
    } else {
        astream->as_out_stream = fh;
    }
}


void
rwAsciiSetDelimiter(
    rwAsciiStream_t    *astream,
    char                delimiter)
{
    assert(astream);
    astream->as_delimiter = delimiter;
}


void
rwAsciiSetNoColumns(
    rwAsciiStream_t    *astream)
{
    assert(astream);
    astream->as_not_columnar = 1;
}


void
rwAsciiSetIPFormatFlags(
    rwAsciiStream_t    *astream,
    uint32_t            ip_format)
{
    assert(astream);
    astream->as_ipformat = ip_format;
}


void
rwAsciiSetIPv6Policy(
    rwAsciiStream_t    *astream,
    sk_ipv6policy_t     policy)
{
    assert(astream);
    astream->as_ipv6_policy = policy;
}

void
rwAsciiSetNoTitles(
    rwAsciiStream_t    *astream)
{
    assert(astream);
    astream->as_no_titles = 1;
}


void
rwAsciiSetTimestampFlags(
    rwAsciiStream_t    *astream,
    uint32_t            time_flags)
{
    assert(astream);
    astream->as_timeflags = time_flags;
}


void
rwAsciiSetIntegerTcpFlags(
    rwAsciiStream_t    *astream)
{
    assert(astream);
    astream->as_integer_flags = 1;
}


void
rwAsciiSetIntegerSensors(
    rwAsciiStream_t    *astream)
{
    assert(astream);
    astream->as_integer_sensors = 1;
}


void
rwAsciiSetIcmpTypeCode(
    rwAsciiStream_t    *astream)
{
    assert(astream);
    astream->as_legacy_icmp = 1;
}


void
rwAsciiSetNoNewline(
    rwAsciiStream_t    *astream)
{
    assert(astream);
    astream->as_no_newline = 1;
}


void
rwAsciiSetNoFinalDelimiter(
    rwAsciiStream_t    *astream)
{
    assert(astream);
    astream->as_no_final_delim = 1;
}


void
rwAsciiGetFieldName(
    char                       *buf,
    size_t                      buf_len,
    rwrec_printable_fields_t    field_id)
{
    size_t i;

    assert(buf);
    assert(buf_len > 0);

    buf[0] = '\0';

    for (i = 0; field_map_entries[i].name; ++i) {
        if (field_map_entries[i].id == (uint32_t)field_id) {
            strncpy(buf, field_map_entries[i].name, buf_len-1);
            buf[buf_len-1] = '\0';
            break;
        }
    }
}


void
rwAsciiPrintTitles(
    rwAsciiStream_t    *astream)
{
    const rwascii_field_t *field;
    char buf[RWASCII_BUF_SIZE];
    uint32_t i;

    /* initialize */
    if (astream->as_initialized == 0) {
        rwAsciiPreparePrint(astream);
    }

    /* don't print titles if we are not supposed to or if we already
     * have */
    if (astream->as_no_titles) {
        return;
    }
    astream->as_no_titles = 1;

    for (i = 0, field = astream->as_field;
         i < astream->as_field_count;
         ++i, ++field)
    {
        if (i > 0) {
            fprintf(astream->as_out_stream, "%c", astream->as_delimiter);
        }
        switch (field->af_field_id) {
          case RWASCII_CB_FIELD_ID:
          case RWASCII_CB_EXTRA_FIELD_ID:
            /* invoke callback */
            field->af_cb_gettitle(buf, sizeof(buf), field->af_cb_data);
            break;

          default:
            rwAsciiGetFieldName(buf, sizeof(buf),
                                (rwrec_printable_fields_t)field->af_field_id);
            break;
        }

        if (astream->as_not_columnar) {
            fprintf(astream->as_out_stream, "%s", buf);
        } else {
            fprintf(astream->as_out_stream, "%*.*s",
                    (int)field->af_width, (int)field->af_width,
                    buf);
        }
    } /* for */

    if ( !astream->as_no_final_delim) {
        fprintf(astream->as_out_stream, "%c", astream->as_delimiter);
    }
    if ( !astream->as_no_newline) {
        fprintf(astream->as_out_stream, "\n");
    }
}


/*  Print 'rwrec' to the ascii stream 'astream'.  See header for details */
void
rwAsciiPrintRecExtra(
    rwAsciiStream_t    *astream,
    const rwRec        *rwrec,
    void               *extra)
{
    static char buffer[RWASCII_BUF_SIZE];
    const rwascii_field_t *field;
    skipaddr_t ip;
    int flags_flags;
    imaxdiv_t idiv;
    uint32_t i;

    assert(astream);
    assert(rwrec);
    assert(sizeof(buffer) > 1+SK_NUM2DOT_STRLEN);
    assert(sizeof(buffer) > 1+SKTIMESTAMP_STRLEN);

    /* initialize */
    if (astream->as_initialized == 0) {
        rwAsciiPreparePrint(astream);
    }

    /* print titles if we haven't */
    if (astream->as_no_titles == 0) {
        /* print titles */
        rwAsciiPrintTitles(astream);
    }

    flags_flags = 0;
    if (!astream->as_not_columnar) {
        flags_flags |= SK_PADDED_FLAGS;
    }

    for (i = 0, field = astream->as_field;
         i < astream->as_field_count;
         ++i, ++field)
    {
        if (i > 0) {
            fprintf(astream->as_out_stream, "%c", astream->as_delimiter);
        }
        switch (field->af_field_id) {
          case RWREC_FIELD_SIP:
            rwRecMemGetSIP(rwrec, &ip);
            skipaddrString(buffer, &ip, astream->as_ipformat);
            break;

          case RWREC_FIELD_DIP:
            rwRecMemGetDIP(rwrec, &ip);
            skipaddrString(buffer, &ip, astream->as_ipformat);
            break;

          case RWREC_FIELD_NHIP:
            rwRecMemGetNhIP(rwrec, &ip);
            skipaddrString(buffer, &ip, astream->as_ipformat);
            break;

          case RWREC_FIELD_SPORT:
            if (astream->as_legacy_icmp && rwRecIsICMP(rwrec)) {
                /* Put the ICMP type in this column. */
                sprintf(buffer, "%u", (unsigned int)rwRecGetIcmpType(rwrec));
            } else {
                /* Put the sPort value here, regardless of protocol */
                sprintf(buffer, "%u", (unsigned int)rwRecGetSPort(rwrec));
            }
            break;

          case RWREC_FIELD_DPORT:
            if (astream->as_legacy_icmp && rwRecIsICMP(rwrec)) {
                /* Put the ICMP code in this column. */
                sprintf(buffer, "%u", (unsigned int)rwRecGetIcmpCode(rwrec));
            } else {
                /* Put the dPort value here, regardless of protocol */
                sprintf(buffer, "%u", (unsigned int)rwRecGetDPort(rwrec));
            }
            break;

          case RWREC_FIELD_ICMP_TYPE:
            if (!rwRecIsICMP(rwrec)) {
                /* not ICMP; leave column blank */
                buffer[0] = '\0';
            } else {
                sprintf(buffer, "%u", (unsigned int)rwRecGetIcmpType(rwrec));
            }
            break;

          case RWREC_FIELD_ICMP_CODE:
            if (!rwRecIsICMP(rwrec)) {
                /* not ICMP; leave column blank */
                buffer[0] = '\0';
            } else {
                sprintf(buffer, "%u", (unsigned int)rwRecGetIcmpCode(rwrec));
            }
            break;

          case RWREC_FIELD_PROTO:
            sprintf(buffer, "%u", (unsigned int)rwRecGetProto(rwrec));
            break;

          case RWREC_FIELD_PKTS:
            sprintf(buffer, ("%" PRIu32), rwRecGetPkts(rwrec));
            break;

          case RWREC_FIELD_BYTES:
            sprintf(buffer, ("%" PRIu32), rwRecGetBytes(rwrec));
            break;

          case RWREC_FIELD_FLAGS:
            if (astream->as_integer_flags) {
                sprintf(buffer, "%u", (unsigned int)rwRecGetFlags(rwrec));
            } else {
                skTCPFlagsString(rwRecGetFlags(rwrec), buffer, flags_flags);
            }
            break;

          case RWREC_FIELD_INIT_FLAGS:
            if (astream->as_integer_flags) {
                sprintf(buffer, "%u", (unsigned int)rwRecGetInitFlags(rwrec));
            } else {
                skTCPFlagsString(rwRecGetInitFlags(rwrec), buffer, flags_flags);
            }
            break;

          case RWREC_FIELD_REST_FLAGS:
            if (astream->as_integer_flags) {
                sprintf(buffer, "%u", (unsigned int)rwRecGetRestFlags(rwrec));
            } else {
                skTCPFlagsString(rwRecGetRestFlags(rwrec), buffer, flags_flags);
            }
            break;

          case RWREC_FIELD_TCP_STATE:
            skTCPStateString(rwRecGetTcpState(rwrec), buffer, flags_flags);
            break;

          case RWREC_FIELD_APPLICATION:
            sprintf(buffer, "%u", (unsigned int)rwRecGetApplication(rwrec));
            break;

          case RWREC_FIELD_ELAPSED:
            if (astream->as_timeflags & SKTIMESTAMP_NOMSEC) {
                sprintf(buffer, ("%" PRIu32), rwRecGetElapsedSeconds(rwrec));
                break;
            }
            /* else fallthough */
          case RWREC_FIELD_ELAPSED_MSEC:
            idiv = imaxdiv(rwRecGetElapsed(rwrec), 1000);
            sprintf(buffer, ("%" PRIdMAX ".%03" PRIdMAX),
                    idiv.quot, idiv.rem);
            break;

          case RWREC_FIELD_STIME:
            sktimestamp_r(buffer, rwRecGetStartTime(rwrec),
                          astream->as_timeflags);
            break;

          case RWREC_FIELD_STIME_MSEC:
            sktimestamp_r(buffer, rwRecGetStartTime(rwrec),
                          (astream->as_timeflags & ~SKTIMESTAMP_NOMSEC));
            break;

          case RWREC_FIELD_ETIME:
            sktimestamp_r(buffer, rwRecGetEndTime(rwrec),
                          astream->as_timeflags);
            break;

          case RWREC_FIELD_ETIME_MSEC:
            sktimestamp_r(buffer, rwRecGetEndTime(rwrec),
                          (astream->as_timeflags & ~SKTIMESTAMP_NOMSEC));
            break;

          case RWREC_FIELD_SID:
            /* sensor ID */
            if ( !astream->as_integer_sensors ) {
                sksiteSensorGetName(buffer, sizeof(buffer),
                                    rwRecGetSensor(rwrec));
            } else if (SK_INVALID_SENSOR == rwRecGetSensor(rwrec)) {
                strcpy(buffer, "-1");
            } else {
                sprintf(buffer, "%u", (unsigned int)rwRecGetSensor(rwrec));
            }
            break;

          case RWREC_FIELD_INPUT:
            sprintf(buffer, "%u", (unsigned int)rwRecGetInput(rwrec));
            break;

          case RWREC_FIELD_OUTPUT:
            /* output */
            sprintf(buffer, "%u", (unsigned int)rwRecGetOutput(rwrec));
            break;

          case RWREC_FIELD_FTYPE_CLASS:
            sksiteFlowtypeGetClass(buffer, sizeof(buffer),
                                   rwRecGetFlowType(rwrec));
            break;

          case RWREC_FIELD_FTYPE_TYPE:
            sksiteFlowtypeGetType(buffer, sizeof(buffer),
                                  rwRecGetFlowType(rwrec));
            break;

          case RWASCII_CB_FIELD_ID:
            /* invoke callback */
            field->af_cb_getvalue.gv(rwrec, buffer, sizeof(buffer),
                                     field->af_cb_data);
            break;

          case RWASCII_CB_EXTRA_FIELD_ID:
            /* invoke callback */
            field->af_cb_getvalue.gv_extra(rwrec, buffer, sizeof(buffer),
                                           field->af_cb_data, extra);
            break;

          default:
            skAbortBadCase(field->af_field_id);
        } /* switch */

        if (astream->as_not_columnar) {
            fprintf(astream->as_out_stream, "%s", buffer);
        } else {
            fprintf(astream->as_out_stream, "%*s",
                    (int)field->af_width, buffer);
        }
    } /* for */

    if ( !astream->as_no_final_delim) {
        fprintf(astream->as_out_stream, "%c", astream->as_delimiter);
    }
    if ( !astream->as_no_newline) {
        fprintf(astream->as_out_stream, "\n");
    }

    return;
}


void
rwAsciiPrintRec(
    rwAsciiStream_t    *astream,
    const rwRec        *rwrec)
{
    rwAsciiPrintRecExtra(astream, rwrec, NULL);
}


sk_stringmap_status_t
rwAsciiFieldMapAddDefaultFields(
    sk_stringmap_t    **field_map)
{
    sk_stringmap_status_t sm_err;

    assert(field_map);

    /* Create the map if necessary */
    if (NULL == *field_map) {
        sm_err = skStringMapCreate(field_map);
        if (SKSTRINGMAP_OK != sm_err) {
            return sm_err;
        }
    }

    /* add entries */
    return skStringMapAddEntries(*field_map, -1, field_map_entries);
}


sk_stringmap_status_t
rwAsciiFieldMapAddIcmpTypeCode(
    sk_stringmap_t     *field_map,
    uint32_t            id)
{
    sk_stringmap_entry_t icmp_type_code[] = {
        {"icmpTypeCode",  0, "Equivalent to iType,iCode [DEPRECATED]", NULL},
        {"25",            0, NULL, NULL},
        SK_STRINGMAP_SENTINEL
    };
    sk_stringmap_entry_t *sm_entry;
    sk_stringmap_status_t sm_err = SKSTRINGMAP_OK;
    size_t i;

    assert(field_map);

    for (i = 0, sm_entry = icmp_type_code;
         sm_entry->name != NULL && sm_err == SKSTRINGMAP_OK;
         ++i, ++sm_entry)
    {
        sm_entry->id = id;
        sm_err = skStringMapAddEntries(field_map, 1, sm_entry);
    }
    return sm_err;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
