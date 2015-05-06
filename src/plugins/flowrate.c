/*
** Copyright (C) 2008-2015 by Carnegie Mellon University.
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

#include <silk/silk.h>

RCSIDENT("$SiLK: flowrate.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skplugin.h>
#include <silk/rwrec.h>
#include <silk/utils.h>


/*
**  flowrate.c
**
**    Plug-in to allow filtering, sorting, counting, and printing of
**    the following values:
**
**    -- packets-per-second
**    -- bytes-per-second
**    -- bytes-per-packet (not for rwfilter; it already exists)
**    -- payload-bytes
**    -- payload-bytes-per-second
**
**    Mark Thomas
**    July 2008
*/


/* DEFINES AND TYPEDEFS */

/* Plugin protocol version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0

/* identifiers for the fields */
#define PCKTS_PER_SEC_KEY       1
#define BYTES_PER_SEC_KEY       2
#define BYTES_PER_PACKET_KEY    3
#define PAYLOAD_BYTES_KEY       4
#define PAYLOAD_RATE_KEY        5
#define PCKTS_PER_SEC_AGG      11
#define BYTES_PER_SEC_AGG      12
#define BYTES_PER_PACKET_AGG   13
#define PAYLOAD_BYTES_AGG      14
#define PAYLOAD_RATE_AGG       15

/* the size of the binary value used as a key in rwsort, rwstats, and
 * rwuniq */
#define RATE_BINARY_SIZE_KEY  sizeof(uint64_t)

/* the aggregate value size in rwstats and rwuniq */
#define RATE_BINARY_SIZE_AGG  (2 * sizeof(uint64_t))

/* preferred width of textual columns */
#define RATE_TEXT_WIDTH   15

/* number of decimal places to use */
#define PRECISION               3

/* a %f format that includes PRECISION digits after the decimal */
#define FORMAT_PRECISION        FLOAT_FORMAT2(PRECISION)

/* binary values must be stored in rwuniq and rwstats.  these
 * conversions keep the PRECISION digits. values beyond PRECISION
 * digits are rounded to nearest number */
#define DOUBLE_TO_UINT64(v)     DBL_TO_INT2(v, PRECISION)
#define UINT64_TO_DOUBLE(v)     INT_TO_DBL2(v, PRECISION)

/* make values in rwcut consistent with those in rwuniq */
#define TRUNC_PRECISION(v)      UINT64_TO_DOUBLE(DOUBLE_TO_UINT64(v))

/* how to calculate the packets per second.  rwRecGetElapsed() returns
 * the value in milliseconds.  If the elapsed time is 0, just return
 * the packet count. */
#define PCKT_RATE_DOUBLE(r)                                             \
    ((rwRecGetElapsed(r) == 0)                                          \
     ? rwRecGetPkts(r)                                                  \
     : ((double)(rwRecGetPkts(r)) * 1000.0 / (double)(rwRecGetElapsed(r))))

/* as above, but for bytes-per-second */
#define BYTE_RATE_DOUBLE(r)                                             \
    ((rwRecGetElapsed(r) == 0)                                          \
     ? rwRecGetBytes(r)                                                 \
     : ((double)(rwRecGetBytes(r)) * 1000.0 / (double)(rwRecGetElapsed(r))))

/* as above, for the payload-rate */
#define PAYLOAD_RATE_DOUBLE(r)                                          \
    ((rwRecGetElapsed(r) == 0)                                          \
     ? getPayload(r)                                                    \
     : ((double)getPayload(r) * 1000.0 / (double)(rwRecGetElapsed(r))))

/* how to calculate bytes-per-packet */
#define BYTES_PER_PACKET_DOUBLE(r)                              \
    ((double)rwRecGetBytes(r) / (double)rwRecGetPkts(r))


/* ah the joys of the C preprocessor...  this mess is here to allow us
 * to use the PRECISION macro as a parameter to other macros. */

#define FLOAT_FORMAT1(ff1_num)      "%." #ff1_num "f"
#define FLOAT_FORMAT2(ff2_macro)    FLOAT_FORMAT1(ff2_macro)

#define DBL_TO_INT1(d2i_v, d2i_num)                             \
    ((uint64_t)(((d2i_v) + 0.5e- ## d2i_num) * 1e+ ## d2i_num))
#define DBL_TO_INT2(d2i_v, d2i_macro)   DBL_TO_INT1(d2i_v, d2i_macro)

#define INT_TO_DBL1(i2d_v, i2d_num)             \
    ((double)(i2d_v) / 1e+ ## i2d_num)
#define INT_TO_DBL2(i2d_v, i2d_macro)   INT_TO_DBL1(i2d_v, i2d_macro)


/* structures to hold min and max values */
typedef struct double_range_st {
    double      min;
    double      max;
    unsigned    is_active :1;
} double_range_t;

typedef struct u64_range_st {
    uint64_t    min;
    uint64_t    max;
    unsigned    is_active :1;
} u64_range_t;


/* LOCAL VARIABLES */

/* for filtering, pass records whose packets-per-second,
 * bytes-per-second, payload-bytes, and payload-bytes-per-second
 * values fall within these ranges. */
static double_range_t pckt_rate = {0, DBL_MAX, 0};
static double_range_t byte_rate = {0, DBL_MAX, 0};
static double_range_t payload_rate = {0, DBL_MAX, 0};
static u64_range_t payload_bytes = {0, UINT64_MAX, 0};

typedef enum plugin_options_en {
    OPT_PACKETS_PER_SECOND,
    OPT_BYTES_PER_SECOND,
    OPT_PAYLOAD_BYTES,
    OPT_PAYLOAD_RATE
} plugin_options_enum;

static struct option plugin_options[] = {
    {"packets-per-second",  REQUIRED_ARG, 0, OPT_PACKETS_PER_SECOND},
    {"bytes-per-second",    REQUIRED_ARG, 0, OPT_BYTES_PER_SECOND},
    {"payload-bytes",       REQUIRED_ARG, 0, OPT_PAYLOAD_BYTES},
    {"payload-rate",        REQUIRED_ARG, 0, OPT_PAYLOAD_RATE},
    {0, 0, 0, 0}            /* sentinel */
};

static const char *plugin_help[] = {
    "Packets-per-second is within decimal range X-Y.",
    "Bytes-per-second is within decimal range X-Y.",
    "Payload-byte count is within integer range X-Y.",
    "Payload-bytes-per-second is within decimal range X-Y.",
    NULL
};

/* fields for rwcut, rwuniq, etc */
static struct plugin_fields_st {
    const char *name;
    uint32_t    val;
} plugin_fields[] = {
    {"pckts/sec",       PCKTS_PER_SEC_KEY},
    {"bytes/sec",       BYTES_PER_SEC_KEY},
    {"bytes/packet",    BYTES_PER_PACKET_KEY},
    {"payload-bytes",   PAYLOAD_BYTES_KEY},
    {"payload-rate",    PAYLOAD_RATE_KEY},
    {NULL,              UINT32_MAX},    /* end of key fields */
    {"pckts/sec",       PCKTS_PER_SEC_AGG},
    {"bytes/sec",       BYTES_PER_SEC_AGG},
    {"bytes/packet",    BYTES_PER_PACKET_AGG},
    {"payload-bytes",   PAYLOAD_BYTES_AGG},
    {"payload-rate",    PAYLOAD_RATE_AGG},
    {NULL,              UINT32_MAX}     /* sentinel */
};




/* LOCAL FUNCTION PROTOTYPES */

static skplugin_err_t
filter(
    const rwRec        *rwrec,
    void               *cbdata,
    void              **extra);


/* FUNCTION DEFINITIONS */

/*
 *  status = optionsHandler(opt_arg, &index);
 *
 *    Handles options for the plugin.  'opt_arg' is the argument, or
 *    NULL if no argument was given.  'index' is the enum passed in
 *    when the option was registered.
 *
 *    Returns SKPLUGIN_OK on success, or SKPLUGIN_ERR if there was a
 *    problem.
 */
static skplugin_err_t
optionsHandler(
    const char         *opt_arg,
    void               *cbdata)
{
    skplugin_callbacks_t regdata;
    plugin_options_enum opt_index = *((plugin_options_enum*)cbdata);
    static int filter_registered = 0;
    int rv;

    switch (opt_index) {
      case OPT_PAYLOAD_BYTES:
        rv = skStringParseRange64(&payload_bytes.min, &payload_bytes.max,
                                  opt_arg, 0, 0, SKUTILS_RANGE_SINGLE_OPEN);
        if (rv) {
            goto PARSE_ERROR;
        }
        payload_bytes.is_active = 1;
        break;

      case OPT_PAYLOAD_RATE:
        rv = skStringParseDoubleRange(&payload_rate.min, &payload_rate.max,
                                      opt_arg, 0.0, 0.0,
                                      SKUTILS_RANGE_SINGLE_OPEN);
        if (rv) {
            goto PARSE_ERROR;
        }
        payload_rate.is_active = 1;
        break;

      case OPT_PACKETS_PER_SECOND:
        rv = skStringParseDoubleRange(&pckt_rate.min, &pckt_rate.max,
                                      opt_arg, 0.0, 0.0,
                                      SKUTILS_RANGE_SINGLE_OPEN);
        if (rv) {
            goto PARSE_ERROR;
        }
        pckt_rate.is_active = 1;
        break;

      case OPT_BYTES_PER_SECOND:
        rv = skStringParseDoubleRange(&byte_rate.min, &byte_rate.max,
                                      opt_arg, 0.0, 0.0,
                                      SKUTILS_RANGE_SINGLE_OPEN);
        if (rv) {
            goto PARSE_ERROR;
        }
        byte_rate.is_active = 1;
        break;
    }

    if (filter_registered) {
        return SKPLUGIN_OK;
    }
    filter_registered = 1;

    memset(&regdata, 0, sizeof(regdata));
    regdata.filter = filter;
    return skpinRegFilter(NULL, &regdata, NULL);

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  plugin_options[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return SKPLUGIN_ERR;
}


/*
 *  payload = getPayload(rwrec);
 *
 *    Compute the bytes of payload contained in 'rwrec' by multiplying
 *    the number of packets by the packet overhead and subtracting
 *    that from the byte count.  Return 0 if that value would be
 *    negative.
 *
 *    This function assumes minimal packet headers---that is, there
 *    are no options in the packets.  For TCP, assumes there are no
 *    TCP timestamps in the packet.  The returned value will be the
 *    maximum possible bytes of payload.
 */
static uint64_t
getPayload(
    const rwRec        *rwrec)
{
    uint64_t overhead;

#if SK_ENABLE_IPV6
    if (rwRecIsIPv6(rwrec)) {
        /* IPv6 IP-header header with no options is 40 bytes */
        switch (rwRecGetProto(rwrec)) {
          case IPPROTO_TCP:
            /* TCP header is 20 (no TCP timestamps) */
            overhead = rwRecGetPkts(rwrec) * 60;
            break;
          case IPPROTO_UDP:
            /* UDP header is 8 bytes */
            overhead = rwRecGetPkts(rwrec) * 48;
            break;
          default:
            overhead = rwRecGetPkts(rwrec) * 40;
            break;
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        /* IPv4 IP-header header with no options is 20 bytes */
        switch (rwRecGetProto(rwrec)) {
          case IPPROTO_TCP:
            overhead = rwRecGetPkts(rwrec) * 40;
            break;
          case IPPROTO_UDP:
            overhead = rwRecGetPkts(rwrec) * 28;
            break;
          default:
            overhead = rwRecGetPkts(rwrec) * 20;
            break;
        }
    }

    if (overhead > rwRecGetBytes(rwrec)) {
        return 0;
    }

    return (rwRecGetBytes(rwrec) - overhead);
}


/*
 *  status = filter(rwrec, data, NULL);
 *
 *    The function actually used to implement filtering for the
 *    plugin.  Returns SKPLUGIN_FILTER_PASS if the record passes the
 *    filter, SKPLUGIN_FILTER_FAIL if it fails the filter.
 */
static skplugin_err_t
filter(
    const rwRec            *rwrec,
    void            UNUSED(*cbdata),
    void           UNUSED(**extra))
{
    uint64_t payload;
    double rate;

    /* filter by payload-bytes */
    if (payload_bytes.is_active) {
        payload = getPayload(rwrec);
        if (payload < payload_bytes.min || payload > payload_bytes.max) {
            /* failed */
            return SKPLUGIN_FILTER_FAIL;
        }
    }

    /* filter by payload-rate */
    if (payload_rate.is_active) {
        rate = PAYLOAD_RATE_DOUBLE(rwrec);
        if (rate < payload_rate.min || rate > payload_rate.max) {
            /* failed */
            return SKPLUGIN_FILTER_FAIL;
        }
    }

    /* filter by packets-per-second */
    if (pckt_rate.is_active) {
        rate = PCKT_RATE_DOUBLE(rwrec);
        if (rate < pckt_rate.min || rate > pckt_rate.max) {
            /* failed */
            return SKPLUGIN_FILTER_FAIL;
        }
    }

    /* filter by bytes-per-second */
    if (byte_rate.is_active) {
        rate = BYTE_RATE_DOUBLE(rwrec);
        if (rate < byte_rate.min || rate > byte_rate.max) {
            /* failed */
            return SKPLUGIN_FILTER_FAIL;
        }
    }

    return SKPLUGIN_FILTER_PASS;
}


/*
 *  status = recToTextKey(rwrec, text_val, text_len, &index, NULL);
 *
 *    Given the SiLK Flow record 'rwrec', compute the flow-rate ratio
 *    specified by '*index', and write a textual representation of
 *    that value into 'text_val', a buffer of 'text_len' characters.
 */
static skplugin_err_t
recToTextKey(
    const rwRec            *rwrec,
    char                   *text_value,
    size_t                  text_size,
    void                   *idx,
    void           UNUSED(**extra))
{
    switch (*((unsigned int*)(idx))) {
      case PAYLOAD_BYTES_KEY:
        snprintf(text_value, text_size, ("%" PRIu64), getPayload(rwrec));
        return SKPLUGIN_OK;

      case PAYLOAD_RATE_KEY:
        snprintf(text_value, text_size, FORMAT_PRECISION,
                 TRUNC_PRECISION(PAYLOAD_RATE_DOUBLE(rwrec)));
        return SKPLUGIN_OK;

      case PCKTS_PER_SEC_KEY:
        snprintf(text_value, text_size, FORMAT_PRECISION,
                 TRUNC_PRECISION(PCKT_RATE_DOUBLE(rwrec)));
        return SKPLUGIN_OK;

      case BYTES_PER_SEC_KEY:
        snprintf(text_value, text_size, FORMAT_PRECISION,
                 TRUNC_PRECISION(BYTE_RATE_DOUBLE(rwrec)));
        return SKPLUGIN_OK;

      case BYTES_PER_PACKET_KEY:
        snprintf(text_value, text_size, FORMAT_PRECISION,
                 TRUNC_PRECISION(BYTES_PER_PACKET_DOUBLE(rwrec)));
        return SKPLUGIN_OK;

    }
    return SKPLUGIN_ERR_FATAL;
}


/*
 *  status = recToBinKey(rwrec, bin_val, &index, NULL);
 *
 *    Given the SiLK Flow record 'rwrec', compute the flow-rate ratio
 *    specified by '*index', and write a binary representation of
 *    that value into 'bin_val'.
 */
static skplugin_err_t
recToBinKey(
    const rwRec            *rwrec,
    uint8_t                *bin_value,
    void                   *idx,
    void           UNUSED(**extra))
{
    uint64_t val_u64;

    switch (*((unsigned int*)(idx))) {
      case PAYLOAD_BYTES_KEY:
        val_u64 = getPayload(rwrec);
        break;
      case PAYLOAD_RATE_KEY:
        val_u64 = DOUBLE_TO_UINT64(PAYLOAD_RATE_DOUBLE(rwrec));
        break;
      case PCKTS_PER_SEC_KEY:
        val_u64 = DOUBLE_TO_UINT64(PCKT_RATE_DOUBLE(rwrec));
        break;
      case BYTES_PER_SEC_KEY:
        val_u64 = DOUBLE_TO_UINT64(BYTE_RATE_DOUBLE(rwrec));
        break;
      case BYTES_PER_PACKET_KEY:
        val_u64 = DOUBLE_TO_UINT64(BYTES_PER_PACKET_DOUBLE(rwrec));
        break;
      default:
        return SKPLUGIN_ERR_FATAL;
    }

    val_u64 = hton64(val_u64);
    memcpy(bin_value, &val_u64, RATE_BINARY_SIZE_KEY);
    return SKPLUGIN_OK;
}


/*
 *  status = binToTextKey(bin_val, text_val, text_len, &index);
 *
 *    Given the buffer 'bin_val' which was filled by calling
 *    recToBinKey(), write a textual representation of that value into
 *    'text_val', a buffer of 'text_len' characters.
 */
static skplugin_err_t
binToTextKey(
    const uint8_t      *bin_value,
    char               *text_value,
    size_t              text_size,
    void               *idx)
{
    uint64_t val_u64;

    switch (*((unsigned int*)(idx))) {
      case PAYLOAD_BYTES_KEY:
        memcpy(&val_u64, bin_value, RATE_BINARY_SIZE_KEY);
        snprintf(text_value, text_size, ("%" PRIu64),
                 ntoh64(val_u64));
        return SKPLUGIN_OK;

      case PAYLOAD_RATE_KEY:
      case PCKTS_PER_SEC_KEY:
      case BYTES_PER_SEC_KEY:
      case BYTES_PER_PACKET_KEY:
        memcpy(&val_u64, bin_value, RATE_BINARY_SIZE_KEY);
        snprintf(text_value, text_size, FORMAT_PRECISION,
                 UINT64_TO_DOUBLE(ntoh64(val_u64)));
        return SKPLUGIN_OK;
    }

    return SKPLUGIN_ERR_FATAL;
}


static skplugin_err_t
addRecToBinAgg(
    const rwRec            *rwrec,
    uint8_t                *dest,
    void                   *idx,
    void           UNUSED(**extra))
{
    uint64_t val[2];

    switch (*((unsigned int*)(idx))) {
      case PAYLOAD_BYTES_AGG:
        memcpy(val, dest, sizeof(uint64_t));
        val[0] += getPayload(rwrec);
        memcpy(dest, val, sizeof(uint64_t));
        return SKPLUGIN_OK;

      case PAYLOAD_RATE_AGG:
        memcpy(val, dest, RATE_BINARY_SIZE_AGG);
        val[0] += getPayload(rwrec);
        val[1] += rwRecGetElapsed(rwrec);
        memcpy(dest, val, RATE_BINARY_SIZE_AGG);
        return SKPLUGIN_OK;

      case PCKTS_PER_SEC_AGG:
        memcpy(val, dest, RATE_BINARY_SIZE_AGG);
        val[0] += rwRecGetPkts(rwrec);
        val[1] += rwRecGetElapsed(rwrec);
        memcpy(dest, val, RATE_BINARY_SIZE_AGG);
        return SKPLUGIN_OK;

      case BYTES_PER_SEC_AGG:
        memcpy(val, dest, RATE_BINARY_SIZE_AGG);
        val[0] += rwRecGetBytes(rwrec);
        val[1] += rwRecGetElapsed(rwrec);
        memcpy(dest, val, RATE_BINARY_SIZE_AGG);
        return SKPLUGIN_OK;

      case BYTES_PER_PACKET_AGG:
        memcpy(val, dest, RATE_BINARY_SIZE_AGG);
        val[0] += rwRecGetBytes(rwrec);
        val[1] += rwRecGetPkts(rwrec);
        memcpy(dest, val, RATE_BINARY_SIZE_AGG);
        return SKPLUGIN_OK;
    }
    return SKPLUGIN_ERR_FATAL;
}


static skplugin_err_t
binToTextAgg(
    const uint8_t      *bin,
    char               *text_value,
    size_t              text_size,
    void               *idx)
{
    uint64_t val[2];

    switch (*((unsigned int*)(idx))) {
      case PAYLOAD_BYTES_AGG:
        memcpy(val, bin, sizeof(uint64_t));
        snprintf(text_value, text_size, ("%" PRIu64), val[0]);
        return SKPLUGIN_OK;

      case PAYLOAD_RATE_AGG:
      case PCKTS_PER_SEC_AGG:
      case BYTES_PER_SEC_AGG:
        memcpy(val, bin, RATE_BINARY_SIZE_AGG);
        if (val[1]) {
            snprintf(text_value, text_size, FORMAT_PRECISION,
                     TRUNC_PRECISION((double)val[0] * 1000.0 / val[1]));
        } else {
            snprintf(text_value, text_size, FORMAT_PRECISION,
                     (double)val[0]);
        }
        return SKPLUGIN_OK;

      case BYTES_PER_PACKET_AGG:
        memcpy(val, bin, RATE_BINARY_SIZE_AGG);
        snprintf(text_value, text_size, FORMAT_PRECISION,
                 TRUNC_PRECISION((double)val[0] / val[1]));
        return SKPLUGIN_OK;
    }

    return SKPLUGIN_ERR_FATAL;
}


static skplugin_err_t
binMergeAgg(
    uint8_t            *bin_a,
    const uint8_t      *bin_b,
    void               *idx)
{
    uint64_t val_a[2];
    uint64_t val_b[2];

    switch (*((unsigned int*)(idx))) {
      case PAYLOAD_BYTES_AGG:
        memcpy(val_a, bin_a, sizeof(uint64_t));
        memcpy(val_b, bin_b, sizeof(uint64_t));
        val_a[0] += val_b[0];
        memcpy(bin_a, val_a, sizeof(uint64_t));
        return SKPLUGIN_OK;

      case PAYLOAD_RATE_AGG:
      case PCKTS_PER_SEC_AGG:
      case BYTES_PER_SEC_AGG:
      case BYTES_PER_PACKET_AGG:
        memcpy(val_a, bin_a, RATE_BINARY_SIZE_AGG);
        memcpy(val_b, bin_b, RATE_BINARY_SIZE_AGG);
        val_a[0] += val_b[0];
        val_a[1] += val_b[1];
        memcpy(bin_a, val_a, RATE_BINARY_SIZE_AGG);
        return SKPLUGIN_OK;
    }
    return SKPLUGIN_ERR_FATAL;
}


static skplugin_err_t
binCompareAgg(
    int                *cmp,
    const uint8_t      *bin_a,
    const uint8_t      *bin_b,
    void               *idx)
{
    uint64_t val_a[2];
    uint64_t val_b[2];
    double ratio_a, ratio_b;

    switch (*((unsigned int*)(idx))) {
      case PAYLOAD_BYTES_AGG:
        memcpy(val_a, bin_a, sizeof(uint64_t));
        memcpy(val_b, bin_b, sizeof(uint64_t));
        if (val_a[0] < val_b[0]) {
            *cmp = -1;
        } else {
            *cmp = (val_a[0] > val_b[0]);
        }
        return SKPLUGIN_OK;

      case PAYLOAD_RATE_AGG:
      case PCKTS_PER_SEC_AGG:
      case BYTES_PER_SEC_AGG:
        memcpy(val_a, bin_a, RATE_BINARY_SIZE_AGG);
        memcpy(val_b, bin_b, RATE_BINARY_SIZE_AGG);
        ratio_a = (double)val_a[0] / (val_a[1] ? val_a[1] : 1.0);
        ratio_b = (double)val_b[0] / (val_b[1] ? val_b[1] : 1.0);
        if (ratio_a < ratio_b) {
            *cmp = -1;
        } else {
            *cmp = (ratio_a > ratio_b);
        }
        return SKPLUGIN_OK;

      case BYTES_PER_PACKET_AGG:
        memcpy(val_a, bin_a, RATE_BINARY_SIZE_AGG);
        memcpy(val_b, bin_b, RATE_BINARY_SIZE_AGG);
        ratio_a = (double)val_a[0] / val_a[1];
        ratio_b = (double)val_b[0] / val_b[1];
        if (ratio_a < ratio_b) {
            *cmp = -1;
        } else {
            *cmp = (ratio_a > ratio_b);
        }
        return SKPLUGIN_OK;
    }
    return SKPLUGIN_ERR_FATAL;
}



/* the registration function called by skplugin.c */
skplugin_err_t
SKPLUGIN_SETUP_FN(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*pi_data))
{
    int i;
    skplugin_field_t *field;
    skplugin_err_t rv;
    skplugin_callbacks_t regdata;

    /* Check API version */
    rv = skpinSimpleCheckVersion(major_version, minor_version,
                                 PLUGIN_API_VERSION_MAJOR,
                                 PLUGIN_API_VERSION_MINOR,
                                 skAppPrintErr);
    if (rv != SKPLUGIN_OK) {
        return rv;
    }

    assert((sizeof(plugin_options)/sizeof(struct option))
           == (sizeof(plugin_help)/sizeof(char*)));

    /* register the options to use for rwfilter.  when the option is
     * given, we will call skpinRegFilter() to register the filter
     * function. */
    for (i = 0; plugin_options[i].name; ++i) {
        rv = skpinRegOption2(plugin_options[i].name,
                             plugin_options[i].has_arg, plugin_help[i],
                             NULL, &optionsHandler,
                             (void*)&plugin_options[i].val,
                             1, SKPLUGIN_FN_FILTER);
        if (SKPLUGIN_OK != rv && SKPLUGIN_ERR_DID_NOT_REGISTER != rv) {
            return rv;
        }
    }

    /* register the key fields to use for rwcut, rwuniq, rwsort,
     * rwstats */
    memset(&regdata, 0, sizeof(regdata));
    regdata.column_width = RATE_TEXT_WIDTH;
    regdata.bin_bytes    = RATE_BINARY_SIZE_KEY;
    regdata.rec_to_text  = recToTextKey;
    regdata.rec_to_bin   = recToBinKey;
    regdata.bin_to_text  = binToTextKey;

    for (i = 0; plugin_fields[i].name; ++i) {
        rv = skpinRegField(&field, plugin_fields[i].name, NULL,
                           &regdata, (void*)&plugin_fields[i].val);
        if (SKPLUGIN_OK != rv) {
            return rv;
        }
    }

    /* register the aggregate value fields to use for rwuniq and
     * rwstats */
    memset(&regdata, 0, sizeof(regdata));
    regdata.column_width    = RATE_TEXT_WIDTH;
    regdata.bin_bytes       = RATE_BINARY_SIZE_AGG;
    regdata.add_rec_to_bin  = addRecToBinAgg;
    regdata.bin_to_text     = binToTextAgg;
    regdata.bin_merge       = binMergeAgg;
    regdata.bin_compare     = binCompareAgg;

    for (++i; plugin_fields[i].name; ++i) {
        if (PAYLOAD_BYTES_AGG == plugin_fields[i].val) {
            /* special case size of payload-bytes */
            regdata.bin_bytes = sizeof(uint64_t);
            rv = skpinRegField(&field, plugin_fields[i].name, NULL,
                               &regdata, (void*)&plugin_fields[i].val);
            regdata.bin_bytes = RATE_BINARY_SIZE_AGG;
        } else {
            rv = skpinRegField(&field, plugin_fields[i].name, NULL,
                               &regdata, (void*)&plugin_fields[i].val);
        }
        if (SKPLUGIN_OK != rv) {
            return rv;
        }
    }

    return SKPLUGIN_OK;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
