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
**  skunique.c
**
**    This is an attempt to make the bulk of rwuniq into a stand-alone
**    library.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skunique.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/hashlib.h>
#include <silk/iptree.h>
#include <silk/rwascii.h>
#include <silk/rwrec.h>
#include <silk/skheap.h>
#include <silk/skstream.h>
#include <silk/sktempfile.h>
#include <silk/skunique.h>
#include <silk/skvector.h>
#include <silk/utils.h>

#ifdef SKUNIQUE_TRACE_LEVEL
#define TRACEMSG_LEVEL 1
#endif
#define TRACEMSG(x)  TRACEMSG_TO_TRACEMSGLVL(1, x)
#include <silk/sktracemsg.h>


#ifndef SKUNIQ_USE_MEMCPY
#ifdef SK_HAVE_ALIGNED_ACCESS_REQUIRED
#define SKUNIQ_USE_MEMCPY 1
#else
#define SKUNIQ_USE_MEMCPY 0
#endif
#endif


#define HASH_MAX_NODE_BYTES  (HASHLIB_MAX_KEY_WIDTH + HASHLIB_MAX_VALUE_WIDTH)

#define HASH_INITIAL_SIZE    500000

#define MAX_MERGE_FILES 1024

#define COMP_FUNC_CAST(cfc_func)                                \
    (int (*)(const void*, const void*, void*))(cfc_func)


/* Messages about failed fwrite() or fread() */
#if  TRACEMSG_LEVEL == 0
#define TRACEMSG_WRITE(tm_write_bytes)
#define TRACEMSG_READ(tm_read_bytes, tm_read_fp)
#else

#define TRACEMSG_WRITE(tm_write_bytes)                          \
    do {                                                        \
        int tm_write_errno = errno;                             \
        TRACEMSG(("Failed to write %" SK_PRIuZ " bytes: %s",    \
                  (size_t)(tm_write_bytes), strerror(errno)));  \
        errno = tm_write_errno;                                 \
    } while(0)

#define TRACEMSG_READ(tm_read_bytes, tm_read_fp)                        \
    do {                                                                \
        int tm_read_errno = errno;                                      \
        TRACEMSG(("Failed to read %" SK_PRIuZ " bytes: %s",             \
                  (size_t)(tm_read_bytes),                              \
                  (ferror(tm_read_fp) ? strerror(errno) : "EOF")));     \
        errno = tm_read_errno;                                          \
    } while(0)

#endif  /* #else of #if TRACEMSG_LEVEL == 0 */


/* Print debugging messages when this environment variable is set to a
 * positive integer. */
#define SKUNIQUE_DEBUG_ENVAR "SILK_UNIQUE_DEBUG"

/*
 *  UNIQUE_DEBUG(ud_uniq, (ud_msg, ...));
 *
 *    Print a message when the print_debug member of the unique object
 *    'ud_uniq' is active.  The message and any arguments it requires
 *    must be wrapped in parentheses.
 *
 *    UNIQUE_DEBUG(uniq, ("one is %d and two is %d", 1, 2));
 */
#define UNIQUE_DEBUG(ud_uniq, ud_msg)                   \
    if (!(ud_uniq)->print_debug) { /*no-op*/ } else {   \
        skAppPrintErr ud_msg;                           \
    }


/* FUNCTION DEFINITIONS */


/* **************************************************************** */

/*    FIELD LIST */

/* **************************************************************** */


/* Maximum number of fields that may be specified. */
#define FIELDLIST_MAX_NUM_FIELDS    (HASHLIB_MAX_KEY_WIDTH >> 1)

#define COMPARE(cmp_a, cmp_b)                                   \
    (((cmp_a) < (cmp_b)) ? -1 : (((cmp_a) > (cmp_b)) ? 1 : 0))

#define WARN_OVERFLOW(wo_max, wo_a, wo_b)                       \
    if (wo_max - wo_b >= wo_a) { /* ok */ } else {              \
        skAppPrintErr("Overflow at %s:%d", __FILE__, __LINE__); \
    }

#if !SKUNIQ_USE_MEMCPY

#define CMP_INT_PTRS(cmp_out, cmp_type, cmp_a, cmp_b)                   \
    {                                                                   \
        cmp_out = COMPARE(*(cmp_type *)(cmp_a), *(cmp_type *)(cmp_b));  \
    }

#define MERGE_INT_PTRS(mrg_max, mrg_type, mrg_a, mrg_b)                 \
    {                                                                   \
        WARN_OVERFLOW(mrg_max, *(mrg_type*)(mrg_a), *(mrg_type*)(mrg_b)); \
        *(mrg_type*)(mrg_a) += *(mrg_type*)(mrg_b);                     \
    }

#define ADD_TO_INT_PTR(mrg_type, mrg_ptr, mrg_val)      \
    {                                                   \
        *((mrg_type*)(mrg_ptr)) += mrg_val;             \
    }

#else

#define CMP_INT_PTRS(cmp_out, cmp_type, cmp_a, cmp_b)   \
    {                                                   \
        cmp_type cip_val_a;                             \
        cmp_type cip_val_b;                             \
                                                        \
        memcpy(&cip_val_a, (cmp_a), sizeof(cmp_type));  \
        memcpy(&cip_val_b, (cmp_b), sizeof(cmp_type));  \
                                                        \
        cmp_out = COMPARE(cip_val_a, cip_val_b);        \
    }

#define MERGE_INT_PTRS(mrg_max, mrg_type, mrg_a, mrg_b) \
    {                                                   \
        mrg_type mip_val_a;                             \
        mrg_type mip_val_b;                             \
                                                        \
        memcpy(&mip_val_a, (mrg_a), sizeof(mrg_type));  \
        memcpy(&mip_val_b, (mrg_b), sizeof(mrg_type));  \
                                                        \
        WARN_OVERFLOW(mrg_max, mip_val_a, mip_val_b);   \
        mip_val_a += mip_val_b;                         \
        memcpy((mrg_a), &mip_val_a, sizeof(mrg_type));  \
    }

#define ADD_TO_INT_PTR(mrg_type, mrg_ptr, mrg_val)              \
    {                                                           \
        mrg_type atip_val_a;                                    \
                                                                \
        memcpy(&atip_val_a, (mrg_ptr), sizeof(mrg_type));       \
        atip_val_a += mrg_val;                                  \
        memcpy((mrg_ptr), &atip_val_a, sizeof(mrg_type));       \
    }

#endif  /* SKUNIQ_USE_MEMCPY */

/* typedef struct sk_fieldentry_st sk_fieldentry_t; */
struct sk_fieldentry_st {
    sk_fieldlist_rec_to_bin_fn_t    rec_to_bin;
    sk_fieldlist_bin_cmp_fn_t       bin_compare;
    sk_fieldlist_rec_to_bin_fn_t    add_rec_to_bin;
    sk_fieldlist_bin_merge_fn_t     bin_merge;
    sk_fieldlist_output_fn_t        bin_output;

    int                             id;

    /* the byte-offset where this field begins in the binary key used
     * for binning. */
    size_t                          offset;
    size_t                          octets;
    void                           *context;

    uint8_t                        *initial_value;

    sk_fieldlist_t                 *parent_list;
};


/* typedef struct sk_fieldlist_st struct sk_fieldlist_t; */
struct sk_fieldlist_st {
    sk_fieldentry_t    fields[FIELDLIST_MAX_NUM_FIELDS];
    size_t             num_fields;
    size_t             total_octets;
};


/*  compare arbitrary buffers of size len */
int
skFieldCompareMemcmp(
    const void         *a,
    const void         *b,
    void               *len)
{
    /* FIXME.  size_t or unit32_t? */
    return memcmp(a, b, *(size_t*)len);
}


/*  compare buffers containing uint8_t */
int
skFieldCompareUint8(
    const void         *a,
    const void         *b,
    void        UNUSED(*ctx))
{
    return COMPARE(*(uint8_t*)a, *(uint8_t*)b);
}

/*  merge buffers containing uint8_t */
void
skFieldMergeUint8(
    void               *a,
    const void         *b,
    void        UNUSED(*ctx))
{
    WARN_OVERFLOW(UINT8_MAX, *(uint8_t*)a, *(uint8_t*)b);
    *(uint8_t*)a += *(uint8_t*)b;
}


/*  compare buffers containing uint16_t */
int
skFieldCompareUint16(
    const void         *a,
    const void         *b,
    void        UNUSED(*ctx))
{
    int rv;
    CMP_INT_PTRS(rv, uint16_t, a, b);
    return rv;
}

/*  merge buffers containing uint16_t */
void
skFieldMergeUint16(
    void               *a,
    const void         *b,
    void        UNUSED(*ctx))
{
    MERGE_INT_PTRS(UINT16_MAX, uint16_t, a, b);
}


/*  compare buffers containing uint32_t */
int
skFieldCompareUint32(
    const void         *a,
    const void         *b,
    void        UNUSED(*ctx))
{
    int rv;
    CMP_INT_PTRS(rv, uint32_t, a, b);
    return rv;
}

/*  merge buffers containing uint32_t */
void
skFieldMergeUint32(
    void               *a,
    const void         *b,
    void        UNUSED(*ctx))
{
    MERGE_INT_PTRS(UINT32_MAX, uint32_t, a, b);
}


/*  compare buffers containing uint64_t */
int
skFieldCompareUint64(
    const void         *a,
    const void         *b,
    void        UNUSED(*ctx))
{
    int rv;
    CMP_INT_PTRS(rv, uint64_t, a, b);
    return rv;
}

/*  merge buffers containing uint64_t */
void
skFieldMergeUint64(
    void               *a,
    const void         *b,
    void        UNUSED(*ctx))
{
    MERGE_INT_PTRS(UINT64_MAX, uint64_t, a, b);
}



/*  create a new field list */
int
skFieldListCreate(
    sk_fieldlist_t    **field_list)
{
    sk_fieldlist_t *fl;

    fl = (sk_fieldlist_t*)calloc(1, sizeof(sk_fieldlist_t));
    if (NULL == fl) {
        return -1;
    }

    *field_list = fl;
    return 0;
}


/*  destroy a field list */
void
skFieldListDestroy(
    sk_fieldlist_t    **field_list)
{
    sk_fieldlist_t *fl;
    sk_fieldentry_t *field;
    size_t i;

    if (NULL == field_list || NULL == *field_list) {
        return;
    }

    fl = *field_list;
    *field_list = NULL;

    for (i = 0, field = fl->fields; i < fl->num_fields; ++i, ++field) {
        if (field->initial_value) {
            free(field->initial_value);
        }
    }

    free(fl);
}


/*  add an arbitrary field to a field list */
sk_fieldentry_t *
skFieldListAddField(
    sk_fieldlist_t                 *field_list,
    const sk_fieldlist_entrydata_t *regdata,
    void                           *ctx)
{
    sk_fieldentry_t *field = NULL;
    size_t i;

    if (NULL == field_list || NULL == regdata) {
        return NULL;
    }
    if (FIELDLIST_MAX_NUM_FIELDS == field_list->num_fields) {
        return NULL;
    }

    field = &field_list->fields[field_list->num_fields];
    ++field_list->num_fields;

    memset(field, 0, sizeof(sk_fieldentry_t));
    field->offset = field_list->total_octets;
    field->context = ctx;
    field->parent_list = field_list;
    field->id = SK_FIELD_CALLER;

    field->octets = regdata->bin_octets;
    field->rec_to_bin = regdata->rec_to_bin;
    field->bin_compare = regdata->bin_compare;
    field->add_rec_to_bin = regdata->add_rec_to_bin;
    field->bin_merge = regdata->bin_merge;
    field->bin_output = regdata->bin_output;
    if (regdata->initial_value) {
        /* only create space for value if it contains non-NUL */
        for (i = 0; i < field->octets; ++i) {
            if ('\0' != regdata->initial_value[i]) {
                field->initial_value = (uint8_t*)malloc(field->octets);
                if (NULL == field->initial_value) {
                    --field_list->num_fields;
                    return NULL;
                }
                memcpy(field->initial_value, regdata->initial_value,
                       field->octets);
                break;
            }
        }
    }

    field_list->total_octets += field->octets;

    return field;
}


/*  add a defined field to a field list */
sk_fieldentry_t *
skFieldListAddKnownField(
    sk_fieldlist_t     *field_list,
    int                 field_id,
    void               *ctx)
{
    sk_fieldentry_t *field = NULL;
    int bin_octets = 0;

    if (NULL == field_list) {
        return NULL;
    }
    if (FIELDLIST_MAX_NUM_FIELDS == field_list->num_fields) {
        return NULL;
    }

    switch (field_id) {
      case SK_FIELD_SIPv4:
      case SK_FIELD_DIPv4:
      case SK_FIELD_NHIPv4:
      case SK_FIELD_PACKETS:
      case SK_FIELD_BYTES:
      case SK_FIELD_STARTTIME:
      case SK_FIELD_STARTTIME_MSEC:
      case SK_FIELD_ELAPSED:
      case SK_FIELD_ELAPSED_MSEC:
      case SK_FIELD_ENDTIME:
      case SK_FIELD_ENDTIME_MSEC:
      case SK_FIELD_RECORDS:
      case SK_FIELD_SUM_ELAPSED:
      case SK_FIELD_MIN_STARTTIME:
      case SK_FIELD_MAX_ENDTIME:
        bin_octets = 4;
        break;

      case SK_FIELD_SPORT:
      case SK_FIELD_DPORT:
      case SK_FIELD_SID:
      case SK_FIELD_INPUT:
      case SK_FIELD_OUTPUT:
      case SK_FIELD_APPLICATION:
        bin_octets = 2;
        break;

      case SK_FIELD_PROTO:
      case SK_FIELD_FLAGS:
      case SK_FIELD_INIT_FLAGS:
      case SK_FIELD_REST_FLAGS:
      case SK_FIELD_TCP_STATE:
      case SK_FIELD_FTYPE_CLASS:
      case SK_FIELD_FTYPE_TYPE:
      case SK_FIELD_ICMP_TYPE:
      case SK_FIELD_ICMP_CODE:
        bin_octets = 1;
        break;

      case SK_FIELD_SUM_PACKETS:
      case SK_FIELD_SUM_BYTES:
        bin_octets = 8;
        break;

      case SK_FIELD_SIPv6:
      case SK_FIELD_DIPv6:
      case SK_FIELD_NHIPv6:
        bin_octets = 16;
        break;

      case SK_FIELD_CALLER:
        break;
    }

    if (bin_octets == 0) {
        skAppPrintErr("Unknown field id %d", field_id);
        return NULL;
    }

    field = &field_list->fields[field_list->num_fields];
    ++field_list->num_fields;

    memset(field, 0, sizeof(sk_fieldentry_t));
    field->offset = field_list->total_octets;
    field->octets = bin_octets;
    field->parent_list = field_list;
    field->id = field_id;
    field->context = ctx;

    field_list->total_octets += bin_octets;

    return field;
}


/*  return context for a field */
void *
skFieldListEntryGetContext(
    const sk_fieldentry_t  *field)
{
    assert(field);
    return field->context;
}


/*  return integer identifier for a field */
uint32_t
skFieldListEntryGetId(
    const sk_fieldentry_t  *field)
{
    assert(field);
    return field->id;
}


/*  return (binary) length for a field */
size_t
skFieldListEntryGetBinOctets(
    const sk_fieldentry_t  *field)
{
    assert(field);
    return field->octets;
}


/*  return (binary) size of all fields in 'field_list' */
size_t
skFieldListGetBufferSize(
    const sk_fieldlist_t   *field_list)
{
    assert(field_list);
    return field_list->total_octets;
}


/*  return number of fields in the field_list */
size_t
skFieldListGetFieldCount(
    const sk_fieldlist_t   *field_list)
{
    assert(field_list);
    return field_list->num_fields;
}


/*  get pointer to a specific field in an encoded buffer */
#define FIELD_PTR(all_fields_buffer, flent)     \
    ((all_fields_buffer) + (flent)->offset)

#if !SKUNIQ_USE_MEMCPY

#define REC_TO_KEY_SZ(rtk_type, rtk_val, rtk_buf, rtk_flent)    \
    { *((rtk_type*)FIELD_PTR(rtk_buf, rtk_flent)) = rtk_val; }

#else

#define REC_TO_KEY_SZ(rtk_type, rtk_val, rtk_buf, rtk_flent)    \
    {                                                           \
        rtk_type rtk_tmp = rtk_val;                             \
        memcpy(FIELD_PTR(rtk_buf, rtk_flent), &rtk_tmp,         \
               sizeof(rtk_type));                               \
    }

#endif


#define REC_TO_KEY_64(val, all_fields_buffer, flent)            \
    REC_TO_KEY_SZ(uint64_t, val, all_fields_buffer, flent)

#define REC_TO_KEY_32(val, all_fields_buffer, flent)              \
    REC_TO_KEY_SZ(uint32_t, val, all_fields_buffer, flent)

#define REC_TO_KEY_16(val, all_fields_buffer, flent)              \
    REC_TO_KEY_SZ(uint16_t, val, all_fields_buffer, flent)

#define REC_TO_KEY_08(val, all_fields_buffer, flent)      \
    { *(FIELD_PTR(all_fields_buffer, flent)) = val; }



/*  get the binary value for each field in 'field_list' and sets that
 *  value in 'all_fields_buffer' */
void
skFieldListRecToBinary(
    const sk_fieldlist_t   *field_list,
    const rwRec            *rwrec,
    uint8_t                *bin_buffer)
{
    const rwRec *rec_ipv4 = NULL;
    const sk_fieldentry_t *f;
    size_t i;

#if !SK_ENABLE_IPV6
#define  FIELDLIST_RWREC_TO_IPV4                \
    rec_ipv4 = rwrec
#else

    const rwRec *rec_ipv6 = NULL;
    rwRec rec_tmp;

    /* ensure we have an IPv4 record when extracting IPv4
     * addresses. If record is IPv6 and cannot be converted to IPv4,
     * use 0 as the IP address. */
#define  FIELDLIST_RWREC_TO_IPV4                \
    if (rec_ipv4) { /* no-op */ }               \
    else if (!rwRecIsIPv6(rwrec)) {             \
        rec_ipv4 = rwrec;                       \
    } else {                                    \
        rec_ipv4 = &rec_tmp;                    \
        RWREC_COPY(&rec_tmp, rwrec);            \
        if (rwRecConvertToIPv4(&rec_tmp)) {     \
            RWREC_CLEAR(&rec_tmp);              \
        }                                       \
    }

    /* ensure we have an IPv6 record when extracting IPv6 addresses */
#define  FIELDLIST_RWREC_TO_IPV6                \
    if (rec_ipv6) { /* no-op */ }               \
    else if (rwRecIsIPv6(rwrec)) {              \
        rec_ipv6 = rwrec;                       \
    } else {                                    \
        rec_ipv6 = &rec_tmp;                    \
        RWREC_COPY(&rec_tmp, rwrec);            \
        rwRecConvertToIPv6(&rec_tmp);           \
    }
#endif  /* #else of #if !SK_ENABLE_IPV6 */


    for (i = 0, f = field_list->fields; i < field_list->num_fields; ++i, ++f) {
        if (f->rec_to_bin) {
            f->rec_to_bin(rwrec, FIELD_PTR(bin_buffer, f), f->context);
        } else {
            switch (f->id) {
#if SK_ENABLE_IPV6
              case SK_FIELD_SIPv6:
                FIELDLIST_RWREC_TO_IPV6;
                rwRecMemGetSIPv6(rec_ipv6, FIELD_PTR(bin_buffer, f));
                break;
              case SK_FIELD_DIPv6:
                FIELDLIST_RWREC_TO_IPV6;
                rwRecMemGetDIPv6(rec_ipv6, FIELD_PTR(bin_buffer, f));
                break;
              case SK_FIELD_NHIPv6:
                FIELDLIST_RWREC_TO_IPV6;
                rwRecMemGetNhIPv6(rec_ipv6, FIELD_PTR(bin_buffer, f));
                break;
#endif  /* SK_ENABLE_IPV6 */

              case SK_FIELD_SIPv4:
                FIELDLIST_RWREC_TO_IPV4;
                REC_TO_KEY_32(rwRecGetSIPv4(rec_ipv4), bin_buffer, f);
                break;
              case SK_FIELD_DIPv4:
                FIELDLIST_RWREC_TO_IPV4;
                REC_TO_KEY_32(rwRecGetDIPv4(rec_ipv4), bin_buffer, f);
                break;
              case SK_FIELD_NHIPv4:
                FIELDLIST_RWREC_TO_IPV4;
                REC_TO_KEY_32(rwRecGetNhIPv4(rec_ipv4), bin_buffer, f);
                break;
              case SK_FIELD_SPORT:
                REC_TO_KEY_16(rwRecGetSPort(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_DPORT:
                REC_TO_KEY_16(rwRecGetDPort(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_ICMP_TYPE:
                if (rwRecIsICMP(rwrec)) {
                    REC_TO_KEY_08(rwRecGetIcmpType(rwrec), bin_buffer, f);
                } else {
                    REC_TO_KEY_08(0, bin_buffer, f);
                }
                break;
              case SK_FIELD_ICMP_CODE:
                if (rwRecIsICMP(rwrec)) {
                    REC_TO_KEY_08(rwRecGetIcmpCode(rwrec), bin_buffer, f);
                } else {
                    REC_TO_KEY_08(0, bin_buffer, f);
                }
                break;
              case SK_FIELD_PROTO:
                REC_TO_KEY_08(rwRecGetProto(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_PACKETS:
                REC_TO_KEY_32(rwRecGetPkts(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_BYTES:
                REC_TO_KEY_32(rwRecGetBytes(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_FLAGS:
                REC_TO_KEY_08(rwRecGetFlags(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_SID:
                REC_TO_KEY_16(rwRecGetSensor(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_INPUT:
                REC_TO_KEY_16(rwRecGetInput(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_OUTPUT:
                REC_TO_KEY_16(rwRecGetOutput(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_INIT_FLAGS:
                REC_TO_KEY_08(rwRecGetInitFlags(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_REST_FLAGS:
                REC_TO_KEY_08(rwRecGetRestFlags(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_TCP_STATE:
                REC_TO_KEY_08(
                    (rwRecGetTcpState(rwrec) & SK_TCPSTATE_ATTRIBUTE_MASK),
                    bin_buffer, f);
                break;
              case SK_FIELD_APPLICATION:
                REC_TO_KEY_16(rwRecGetApplication(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_FTYPE_CLASS:
              case SK_FIELD_FTYPE_TYPE:
                REC_TO_KEY_08(rwRecGetFlowType(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_STARTTIME:
              case SK_FIELD_STARTTIME_MSEC:
                REC_TO_KEY_32(rwRecGetStartSeconds(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_ELAPSED:
              case SK_FIELD_ELAPSED_MSEC:
                REC_TO_KEY_32(rwRecGetElapsedSeconds(rwrec), bin_buffer, f);
                break;
              case SK_FIELD_ENDTIME:
              case SK_FIELD_ENDTIME_MSEC:
                REC_TO_KEY_32(rwRecGetEndSeconds(rwrec), bin_buffer, f);
                break;
              default:
                break;
            }
        }
    }
}


/*  add the binary value for each field in 'field_list' to the values
 *  in 'all_fields_buffer' */
void
skFieldListAddRecToBuffer(
    const sk_fieldlist_t   *field_list,
    const rwRec            *rwrec,
    uint8_t                *summed)
{
#if !SKUNIQ_USE_MEMCPY
    uint32_t *val_ptr;
#else
    uint32_t val_a;
#endif
    const sk_fieldentry_t *f;
    size_t i;

    for (i = 0, f = field_list->fields; i < field_list->num_fields; ++i, ++f) {
        if (f->add_rec_to_bin) {
            f->add_rec_to_bin(rwrec, FIELD_PTR(summed, f), f->context);
        } else {
            switch (f->id) {
              case SK_FIELD_RECORDS:
                ADD_TO_INT_PTR(uint32_t, FIELD_PTR(summed, f), 1);
                break;

              case SK_FIELD_SUM_BYTES:
                ADD_TO_INT_PTR(uint64_t, FIELD_PTR(summed, f),
                               rwRecGetBytes(rwrec));
                break;

              case SK_FIELD_SUM_PACKETS:
                ADD_TO_INT_PTR(uint64_t, FIELD_PTR(summed, f),
                               rwRecGetPkts(rwrec));
                break;

              case SK_FIELD_SUM_ELAPSED:
                ADD_TO_INT_PTR(uint32_t, FIELD_PTR(summed, f),
                               rwRecGetElapsedSeconds(rwrec));
                break;

#if !SKUNIQ_USE_MEMCPY
              case SK_FIELD_MIN_STARTTIME:
                val_ptr = (uint32_t*)FIELD_PTR(summed, f);
                if (rwRecGetStartSeconds(rwrec) < *val_ptr) {
                    *val_ptr = rwRecGetStartSeconds(rwrec);
                }
                break;

              case SK_FIELD_MAX_ENDTIME:
                val_ptr = (uint32_t*)FIELD_PTR(summed, f);
                if (rwRecGetEndSeconds(rwrec) > *val_ptr) {
                    *val_ptr = rwRecGetEndSeconds(rwrec);
                }
                break;

#else  /* SKUNIQ_USE_MEMCPY */
              case SK_FIELD_MIN_STARTTIME:
                memcpy(&val_a, FIELD_PTR(summed, f), f->octets);
                if (rwRecGetStartSeconds(rwrec) < val_a) {
                    val_a = rwRecGetStartSeconds(rwrec);
                    memcpy(FIELD_PTR(summed, f), &val_a, f->octets);
                }
                break;

              case SK_FIELD_MAX_ENDTIME:
                memcpy(&val_a, FIELD_PTR(summed, f), f->octets);
                if (rwRecGetEndSeconds(rwrec) > val_a) {
                    val_a = rwRecGetEndSeconds(rwrec);
                    memcpy(FIELD_PTR(summed, f), &val_a, f->octets);
                }
                break;
#endif  /* SKUNIQ_USE_MEMCPY */

              case SK_FIELD_CALLER:
                break;

              default:
                break;
            }
        }
    }
}


/*  set 'all_fields_buffer' to the initial value for each field in the
 *  field list. */
void
skFieldListInitializeBuffer(
    const sk_fieldlist_t   *field_list,
    uint8_t                *all_fields_buffer)
{
    const sk_fieldentry_t *f;
    size_t i;

    memset(all_fields_buffer, 0, field_list->total_octets);
    for (i = 0, f = field_list->fields; i < field_list->num_fields; ++i, ++f) {
        if (f->initial_value) {
            memcpy(FIELD_PTR(all_fields_buffer, f), f->initial_value,
                   f->octets);
        } else {
            switch (f->id) {
              case SK_FIELD_MIN_STARTTIME:
                memset(FIELD_PTR(all_fields_buffer, f), 0xFF, f->octets);
                break;
              default:
                break;
            }
        }
    }
}


/*  merge (e.g., add) two buffers for a field list */
void
skFieldListMergeBuffers(
    const sk_fieldlist_t   *field_list,
    uint8_t                *all_fields_buffer1,
    const uint8_t          *all_fields_buffer2)
{
#if !SKUNIQ_USE_MEMCPY
    uint32_t *a_ptr, *b_ptr;
#else
    uint32_t val_a, val_b;
#endif
    const sk_fieldentry_t *f;
    size_t i;

    for (i = 0, f = field_list->fields; i < field_list->num_fields; ++i, ++f) {
        if (f->bin_merge) {
            f->bin_merge(FIELD_PTR(all_fields_buffer1, f),
                         FIELD_PTR(all_fields_buffer2, f),
                         f->context);
        } else {
            switch (f->id) {
              case SK_FIELD_RECORDS:
              case SK_FIELD_SUM_ELAPSED:
                MERGE_INT_PTRS(UINT32_MAX, uint32_t,
                               FIELD_PTR(all_fields_buffer1, f),
                               FIELD_PTR(all_fields_buffer2, f));
                break;

              case SK_FIELD_SUM_PACKETS:
              case SK_FIELD_SUM_BYTES:
                MERGE_INT_PTRS(UINT64_MAX, uint64_t,
                               FIELD_PTR(all_fields_buffer1, f),
                               FIELD_PTR(all_fields_buffer2, f));
                break;

#if !SKUNIQ_USE_MEMCPY
              case SK_FIELD_MIN_STARTTIME:
                /* put smallest value into a */
                a_ptr = (uint32_t*)FIELD_PTR(all_fields_buffer1, f);
                b_ptr = (uint32_t*)FIELD_PTR(all_fields_buffer2, f);
                if (*b_ptr < *a_ptr) {
                    *a_ptr = *b_ptr;
                }
                break;

              case SK_FIELD_MAX_ENDTIME:
                /* put largest value into a */
                a_ptr = (uint32_t*)FIELD_PTR(all_fields_buffer1, f);
                b_ptr = (uint32_t*)FIELD_PTR(all_fields_buffer2, f);
                if (*b_ptr > *a_ptr) {
                    *a_ptr = *b_ptr;
                }
                break;

#else  /* SKUNIQ_USE_MEMCPY */
              case SK_FIELD_MIN_STARTTIME:
                memcpy(&val_a, FIELD_PTR(all_fields_buffer1, f), f->octets);
                memcpy(&val_b, FIELD_PTR(all_fields_buffer2, f), f->octets);
                if (val_b < val_a) {
                    val_a = val_b;
                    memcpy(FIELD_PTR(all_fields_buffer1, f), &val_a, f->octets);
                }
                break;

              case SK_FIELD_MAX_ENDTIME:
                memcpy(&val_a, FIELD_PTR(all_fields_buffer1, f), f->octets);
                memcpy(&val_b, FIELD_PTR(all_fields_buffer2, f), f->octets);
                if (val_b > val_a) {
                    val_a = val_b;
                    memcpy(FIELD_PTR(all_fields_buffer1, f), &val_a, f->octets);
                }
                break;
#endif  /* SKUNIQ_USE_MEMCPY */

              default:
                break;
            }
        }
    }
}


/*  compare two field buffers, return -1, 0, 1, if
 *  'all_fields_buffer1' is <, ==, > 'all_fields_buffer2' */
int
skFieldListCompareBuffers(
    const uint8_t          *all_fields_buffer1,
    const uint8_t          *all_fields_buffer2,
    const sk_fieldlist_t   *field_list)
{
    const sk_fieldentry_t *f;
    size_t i;
    int rv = 0;

    for (i = 0, f = field_list->fields;
         rv == 0 && i < field_list->num_fields;
         ++i, ++f)
    {
        if (f->bin_compare) {
            rv = f->bin_compare(FIELD_PTR(all_fields_buffer1, f),
                                FIELD_PTR(all_fields_buffer2, f),
                                f->context);
        } else {
            switch (f->id) {
              case SK_FIELD_SIPv6:
              case SK_FIELD_DIPv6:
              case SK_FIELD_NHIPv6:
                rv = memcmp(FIELD_PTR(all_fields_buffer1, f),
                            FIELD_PTR(all_fields_buffer2, f),
                            f->octets);
                break;

              case SK_FIELD_SIPv4:
              case SK_FIELD_DIPv4:
              case SK_FIELD_NHIPv4:
              case SK_FIELD_PACKETS:
              case SK_FIELD_BYTES:
              case SK_FIELD_STARTTIME:
              case SK_FIELD_STARTTIME_MSEC:
              case SK_FIELD_ELAPSED:
              case SK_FIELD_ELAPSED_MSEC:
              case SK_FIELD_ENDTIME:
              case SK_FIELD_ENDTIME_MSEC:
              case SK_FIELD_RECORDS:
              case SK_FIELD_SUM_ELAPSED:
              case SK_FIELD_MIN_STARTTIME:
              case SK_FIELD_MAX_ENDTIME:
                CMP_INT_PTRS(rv, uint32_t, FIELD_PTR(all_fields_buffer1, f),
                             FIELD_PTR(all_fields_buffer2, f));
                break;

              case SK_FIELD_SPORT:
              case SK_FIELD_DPORT:
              case SK_FIELD_SID:
              case SK_FIELD_INPUT:
              case SK_FIELD_OUTPUT:
              case SK_FIELD_APPLICATION:
                CMP_INT_PTRS(rv, uint16_t, FIELD_PTR(all_fields_buffer1, f),
                             FIELD_PTR(all_fields_buffer2, f));
                break;

              case SK_FIELD_PROTO:
              case SK_FIELD_FLAGS:
              case SK_FIELD_INIT_FLAGS:
              case SK_FIELD_REST_FLAGS:
              case SK_FIELD_TCP_STATE:
              case SK_FIELD_FTYPE_CLASS:
              case SK_FIELD_FTYPE_TYPE:
              case SK_FIELD_ICMP_TYPE:
              case SK_FIELD_ICMP_CODE:
                rv = COMPARE(*(FIELD_PTR(all_fields_buffer1, f)),
                             *(FIELD_PTR(all_fields_buffer2, f)));
                break;

              case SK_FIELD_SUM_PACKETS:
              case SK_FIELD_SUM_BYTES:
                CMP_INT_PTRS(rv, uint64_t, FIELD_PTR(all_fields_buffer1, f),
                             FIELD_PTR(all_fields_buffer2, f));
                break;

              default:
                rv = memcmp(FIELD_PTR(all_fields_buffer1, f),
                            FIELD_PTR(all_fields_buffer2, f),
                            f->octets);
                break;
            }
        }
    }

    return rv;
}


/*  Call the comparison function for a single field entry, where
 *  'field_buffer1' and 'field_buffer2' are pointing at the values
 *  specific to that field. */
int
skFieldListEntryCompareBuffers(
    const uint8_t          *field_buffer1,
    const uint8_t          *field_buffer2,
    const sk_fieldentry_t  *field_entry)
{
    int rv;

    if (field_entry->bin_compare) {
        rv = field_entry->bin_compare(field_buffer1, field_buffer2,
                                      field_entry->context);
    } else {
        switch (field_entry->id) {
          case SK_FIELD_SIPv6:
          case SK_FIELD_DIPv6:
          case SK_FIELD_NHIPv6:
            rv = memcmp(field_buffer1, field_buffer2, field_entry->octets);
            break;

          case SK_FIELD_SIPv4:
          case SK_FIELD_DIPv4:
          case SK_FIELD_NHIPv4:
          case SK_FIELD_PACKETS:
          case SK_FIELD_BYTES:
          case SK_FIELD_STARTTIME:
          case SK_FIELD_STARTTIME_MSEC:
          case SK_FIELD_ELAPSED:
          case SK_FIELD_ELAPSED_MSEC:
          case SK_FIELD_ENDTIME:
          case SK_FIELD_ENDTIME_MSEC:
          case SK_FIELD_RECORDS:
          case SK_FIELD_SUM_ELAPSED:
          case SK_FIELD_MIN_STARTTIME:
          case SK_FIELD_MAX_ENDTIME:
            CMP_INT_PTRS(rv, uint32_t, field_buffer1, field_buffer2);
            break;

          case SK_FIELD_SPORT:
          case SK_FIELD_DPORT:
          case SK_FIELD_SID:
          case SK_FIELD_INPUT:
          case SK_FIELD_OUTPUT:
          case SK_FIELD_APPLICATION:
            CMP_INT_PTRS(rv, uint16_t, field_buffer1, field_buffer2);
            break;

          case SK_FIELD_PROTO:
          case SK_FIELD_FLAGS:
          case SK_FIELD_INIT_FLAGS:
          case SK_FIELD_REST_FLAGS:
          case SK_FIELD_TCP_STATE:
          case SK_FIELD_FTYPE_CLASS:
          case SK_FIELD_FTYPE_TYPE:
          case SK_FIELD_ICMP_TYPE:
          case SK_FIELD_ICMP_CODE:
            rv = COMPARE(*field_buffer1, *field_buffer2);
            break;

          case SK_FIELD_SUM_PACKETS:
          case SK_FIELD_SUM_BYTES:
            CMP_INT_PTRS(rv, uint64_t, field_buffer1, field_buffer2);
            break;

          default:
            rv = memcmp(field_buffer1, field_buffer2, field_entry->octets);
            break;
        }
    }
    return rv;
}


/*  call the output callback for each field */
void
skFieldListOutputBuffer(
    const sk_fieldlist_t   *field_list,
    const uint8_t          *all_fields_buffer);


/* Do we still need the field iterators (as public)? */

/*  bind an iterator to a field list */
void
skFieldListIteratorBind(
    const sk_fieldlist_t       *field_list,
    sk_fieldlist_iterator_t    *iter)
{
    assert(field_list);
    assert(iter);

    memset(iter, 0, sizeof(sk_fieldlist_iterator_t));
    iter->field_list = field_list;
    iter->field_idx = 0;
}

/*  reset the fieldlist iterator */
void
skFieldListIteratorReset(
    sk_fieldlist_iterator_t    *iter)
{
    assert(iter);
    iter->field_idx = 0;
}

/*  get next field-entry from an iterator */
sk_fieldentry_t *
skFieldListIteratorNext(
    sk_fieldlist_iterator_t    *iter)
{
    const sk_fieldentry_t *f = NULL;

    assert(iter);
    if (iter->field_idx < iter->field_list->num_fields) {
        f = &iter->field_list->fields[iter->field_idx];
        ++iter->field_idx;
    }
    return (sk_fieldentry_t*)f;
}


/*  copy the value associated with 'field_id' from 'all_fields_buffer'
 *  and into 'one_field_buf' */
void
skFieldListExtractFromBuffer(
    const sk_fieldlist_t    UNUSED(*field_list),
    const uint8_t                  *all_fields_buffer,
    sk_fieldentry_t                *field_id,
    uint8_t                        *one_field_buf)
{
    assert(field_id->parent_list == field_list);
    memcpy(one_field_buf, FIELD_PTR(all_fields_buffer, field_id),
           field_id->octets);
}



/* **************************************************************** */

/*    HASH SET */

/* **************************************************************** */


/* LOCAL DEFINES AND TYPEDEFS */

typedef struct HashSet_st {
    HashTable   *table;
    uint8_t      is_sorted;
    uint8_t      key_width;
    uint8_t      mod_key;
} HashSet;

typedef struct hashset_iter {
    HASH_ITER    table_iter;
    uint8_t      key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t      val;
} hashset_iter;

typedef struct hashset_file_st {
    /* table into which to write new values */
    HashSet            *hf_hashset;
    /* file in which to store old values; every time the HashSet
     * fills, a new block of records is appended to the file */
    FILE               *hf_file;
    /* offsets in the 'hf_file' where each block ends/begins.  does
     * not include an entry for the first block, which is 0. */
    sk_vector_t        *hf_offsets;
} hashset_file_t;


/* LOCAL VARIABLE DEFINITIONS */

/* position of least significant bit, as in 1<<N */
static const uint8_t lowest_bit_in_val[] = {
    /*   0- 15 */  8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /*  16- 31 */  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /*  32- 47 */  5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /*  48- 63 */  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /*  64- 79 */  6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /*  80- 95 */  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /*  96-111 */  5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 112-127 */  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 128-143 */  7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 144-159 */  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 160-175 */  5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 176-191 */  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 192-207 */  6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 208-223 */  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 224-239 */  5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 240-255 */  4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

#ifndef NDEBUG
/* number of high bits in each value */
static const uint8_t bits_in_value[] = {
    /*   0- 15 */  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    /*  16- 31 */  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    /*  32- 47 */  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    /*  48- 63 */  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    /*  64- 79 */  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    /*  80- 95 */  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    /*  96-111 */  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    /* 112-127 */  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    /* 128-143 */  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    /* 144-159 */  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    /* 160-175 */  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    /* 176-191 */  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    /* 192-207 */  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    /* 208-223 */  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    /* 224-239 */  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    /* 240-255 */  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};
#endif  /* NDEBUG */


/*
 *  hashset = hashset_create_set(key_width, estimated_count, load_factor);
 *
 *    Create a hashlib hash table that supports storing a bit for each
 *    key.  'key_width' is the number of octets in each key.
 *    'estimated_count' is the number of entries in the hash table.
 *    'load_factor' is the load factor to use for the hashlib table.
 *    Return the new hashset, or NULL on error.
 */
static HashSet *
hashset_create_set(
    uint8_t             key_width,
    uint32_t            estimated_count,
    uint8_t             load_factor)
{
    uint8_t no_value = 0;
    HashSet *hash_set;

    hash_set = (HashSet*)calloc(1, sizeof(HashSet));
    if (NULL == hash_set) {
        return NULL;
    }
    hash_set->key_width = key_width;
    hash_set->mod_key = key_width - 1;
    hash_set->table = hashlib_create_table(key_width, 1, HTT_INPLACE,
                                           &no_value, NULL, 0,
                                           estimated_count, load_factor);
    if (hash_set->table == NULL) {
        free(hash_set);
        return NULL;
    }
    return hash_set;
}


/*
 *  status = hashset_insert(hashset, key);
 *
 *    Set the bit for 'key' in 'hashset'.  Return OK on success.
 *    Return ERR_NOMOREENTRIES or ERR_NOMOREBLOCKS on memory
 *    allocation error.
 */
static int
hashset_insert(
    HashSet            *set_ptr,
    const uint8_t      *key_ptr)
{
    uint8_t tmp_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t *value_ptr;
    uint8_t bit;
    int rv;

    /* make a new key, masking off the lowest three bits */
    memcpy(tmp_key, key_ptr, set_ptr->key_width);
    tmp_key[set_ptr->mod_key] &= 0xF8;

    /* determine which bit to check/set */
    bit = 1 << (key_ptr[set_ptr->mod_key] & 0x7);

    rv = hashlib_insert(set_ptr->table, tmp_key, &value_ptr);
    switch (rv) {
      case OK_DUPLICATE:
        if (0 == (*value_ptr & bit)) {
            rv = OK;
        }
        /* FALLTHROUGH */
      case OK:
        *value_ptr |= bit;
        break;
    }
    return rv;
}


#if 0                           /* currently unused; #if 0 to avoid warning */
/*
 *  status = hashset_lookup(hashset, key);
 *
 *    Return OK if 'key' is set in 'hashset', or ERR_NOTFOUND if it is
 *    not.
 */
static int
hashset_lookup(
    const HashSet      *set_ptr,
    const uint8_t      *key_ptr)
{
    uint8_t tmp_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t *value_ptr;
    uint8_t bit;
    int rv;

    /* make a new key, masking off the lowest three bits */
    memcpy(tmp_key, key_ptr, set_ptr->key_width);
    tmp_key[set_ptr->mod_key] &= 0xF8;

    /* determine which bit to check/set */
    bit = 1 << (key_ptr[set_ptr->mod_key] & 0x7);

    rv = hashlib_lookup(set_ptr->table, tmp_key, &value_ptr);
    if (rv == OK && (*value_ptr & bit)) {
        return OK;
    }
    return ERR_NOTFOUND;
}
#endif  /* 0 */


/*
 *  iter = hashset_create_iterator(hashset);
 *
 *    Create an iterator to loop over the bits that are set in
 *    'hashset'.
 */
static hashset_iter
hashset_create_iterator(
    const HashSet      *set_ptr)
{
    hashset_iter iter;

    memset(&iter, 0, sizeof(iter));
    iter.table_iter = hashlib_create_iterator(set_ptr->table);
    return iter;
}


#if 0
/*
 *  status = hashset_sort_entries(hashset);
 *
 *    Sort the entries in 'hashset'.  This makes the 'hashset'
 *    immutable.
 */
static int
hashset_sort_entries(
    HashSet            *set_ptr)
{
    set_ptr->is_sorted = 1;
    return hashlib_sort_entries(set_ptr->table);
}
#endif


/*
 *  status = hashset_iterate(hashset, iter, &key);
 *
 *    Modify 'key' to point to the next key that is set in 'hashset'.
 *    Return OK on success, or ERR_NOMOREENTRIES if 'iter' has visited
 *    all the netries in the 'hashset'.
 */
static int
hashset_iterate(
    const HashSet      *set_ptr,
    hashset_iter       *iter,
    uint8_t           **key_pptr)
{
    uint8_t *hash_key;
    uint8_t *hash_value;
    int rv;

    if (iter->val == 0) {
        /* need to get a key/value pair, which we stash on the iterator */
        rv = hashlib_iterate(set_ptr->table, &iter->table_iter,
                             &hash_key, &hash_value);
        if (rv != OK) {
            return rv;
        }
        memcpy(&iter->key, hash_key, set_ptr->key_width);
        iter->val = hash_value[0];
    }

    /* each key/value pair from the hash table may represent up to 8
     * distinct values.  set the 3 least significant bits of the key
     * we return to the caller based on which bit(s) are set on the
     * value, then clear that bit on the cached value so we don't
     * return it again. */

    switch (lowest_bit_in_val[iter->val]) {
      case 0:
        iter->key[set_ptr->mod_key] = (iter->key[set_ptr->mod_key] & 0xF8);
        iter->val &= 0xFE;
        break;
      case 1:
        iter->key[set_ptr->mod_key] = (iter->key[set_ptr->mod_key] & 0xF8) | 1;
        iter->val &= 0xFD;
        break;
      case 2:
        iter->key[set_ptr->mod_key] = (iter->key[set_ptr->mod_key] & 0xF8) | 2;
        iter->val &= 0xFB;
        break;
      case 3:
        iter->key[set_ptr->mod_key] = (iter->key[set_ptr->mod_key] & 0xF8) | 3;
        iter->val &= 0xF7;
        break;
      case 4:
        iter->key[set_ptr->mod_key] = (iter->key[set_ptr->mod_key] & 0xF8) | 4;
        iter->val &= 0xEF;
        break;
      case 5:
        iter->key[set_ptr->mod_key] = (iter->key[set_ptr->mod_key] & 0xF8) | 5;
        iter->val &= 0xDF;
        break;
      case 6:
        iter->key[set_ptr->mod_key] = (iter->key[set_ptr->mod_key] & 0xF8) | 6;
        iter->val &= 0xBF;
        break;
      case 7:
        iter->key[set_ptr->mod_key] = (iter->key[set_ptr->mod_key] & 0xF8) | 7;
        iter->val &= 0x7F;
        break;
      default:
        skAbortBadCase(lowest_bit_in_val[iter->val]);
    }

    *key_pptr = iter->key;
    return OK;
}


/*
 *  hashset_free_table(hashset);
 *
 *    Free the memory associated with the 'hashset'.
 */
static void
hashset_free_table(
    HashSet            *set_ptr)
{
    if (set_ptr) {
        if (set_ptr->table) {
            hashlib_free_table(set_ptr->table);
            set_ptr->table = NULL;
        }
        free(set_ptr);
    }
}


#ifndef NDEBUG                  /* used only in assert() */
/*
 *  count = hashset_count_entries(hashset);
 *
 *    Count the number the bits that are set in the 'hashset'.
 */
static uint32_t
hashset_count_entries(
    const HashSet      *set_ptr)
{
    HASH_ITER iter;
    uint8_t *key_ptr;
    uint8_t *val_ptr;
    uint32_t count = 0;

    iter = hashlib_create_iterator(set_ptr->table);

    while (hashlib_iterate(set_ptr->table, &iter, &key_ptr, &val_ptr) == OK) {
        count += bits_in_value[*val_ptr];
    }

    return count;
}
#endif /* !NDEBUG */



/* **************************************************************** */

/*    SHORT LIST */

/* **************************************************************** */

#define SK_SHORT_LIST_MAX_ELEMENTS  32

#define SK_SHORT_LIST_ELEM(sle_list, sle_pos)                   \
    ((void*)(((uint8_t*)(sle_list)->sl_data)                    \
             + (sle_pos) * (sle_list)->sl_element_size))

enum sk_short_list_en {
    SK_SHORT_LIST_OK = 0,
    SK_SHORT_LIST_OK_DUPLICATE = 1,
    SK_SHORT_LIST_ERR_ALLOC = -1,
    SK_SHORT_LIST_ERR_FULL = -2
};


typedef struct sk_short_list_st {
    /* size of elements, as specified by user */
    uint32_t    sl_element_size;
    /* number of current elements */
    uint32_t    sl_element_count;
    /* comarison function */
    int        (*sl_compare_fn)(const void *, const void *, void *);
    void        *sl_compare_data;
    /* data[] is a variable sized array; use a uint64_t to ensure data
     * is properly aligned to hold uint64_t's. */
    uint64_t    sl_data[1];
} sk_short_list_t;


/*
 *  status = skShortListCreate(&list, element_size, cmp_func, cmp_func_data);
 *
 *    Create a new short-list object at the address specified in
 *    'list', where the size of each element is 'element_size'.  The
 *    list object will use 'cmp_func' to compare keys.  Return 0 on
 *    success, or -1 on failure.
 */
int
skShortListCreate(
    sk_short_list_t   **list,
    size_t              element_size,
    int               (*compare_function)(const void *, const void *, void *),
    void               *compare_user_data);

/*
 *  skShortListDestroy(&list);
 *
 *    Destroy the short-list object at 'list'.  Does nothing if 'list'
 *    or the object that 'list' refers to is NULL.
 */
void
skShortListDestroy(
    sk_short_list_t   **list);

/*
 *  count = skShortListCountEntries(list);
 *
 *    Count the number of entries in list.
 */
uint32_t
skShortListCountEntries(
    const sk_short_list_t  *list);

/*
 *  object = skShortListGetElement(list, position);
 *
 *    Get the object in 'list' at 'position'.  Return NULL if there is
 *    no object at 'position'.  The caller must treat the returned
 *    value as immutable.
 */
const void *
skShortListGetElement(
    const sk_short_list_t  *list,
    uint32_t                position);

/*
 *  skShortListRemoveAll(list);
 *
 *    Remove all the entries in 'list'.
 */
void
skShortListRemoveAll(
    sk_short_list_t    *list);

/*
 *  status = skShortListInsert(list, object);
 *
 *    Add 'object' to the short-list 'list'.  Return SK_SHORT_LIST_OK
 *    if 'object' is a new entry in 'list'.  Return
 *    SK_SHORT_LIST_OK_DUPLICATE if 'object' already existed in
 *    'list'.  Return SK_SHORT_LIST_ERR_FULL if there is no room in
 *    'list' for the entry.
 */
int
skShortListInsert(
    sk_short_list_t    *list,
    const void         *element);


/*  create a short-list */
int
skShortListCreate(
    sk_short_list_t   **list,
    size_t              element_size,
    int               (*compare_function)(const void *, const void *, void *),
    void               *compare_user_data)
{
    assert(list);

    if (0 == element_size) {
        return -1;
    }
    *list = ((sk_short_list_t*)
             malloc(offsetof(sk_short_list_t, sl_data)
                    + (element_size * SK_SHORT_LIST_MAX_ELEMENTS)));
    if (NULL == *list) {
        return SK_SHORT_LIST_ERR_ALLOC;
    }
    (*list)->sl_element_size = element_size;
    (*list)->sl_element_count = 0;
    (*list)->sl_compare_fn = compare_function;
    (*list)->sl_compare_data = compare_user_data;
    return 0;
}


/*  destroy a short-list */
void
skShortListDestroy(
    sk_short_list_t   **list)
{
    if (list && *list) {
        free(*list);
        *list = NULL;
    }
}


/*  count number of entries in the short-list */
uint32_t
skShortListCountEntries(
    const sk_short_list_t  *list)
{
    return list->sl_element_count;
}


/*  get object at 'position' in 'list' */
const void *
skShortListGetElement(
    const sk_short_list_t  *list,
    uint32_t                position)
{
    assert(list);
    if (position >= list->sl_element_count) {
        return NULL;
    }
    return SK_SHORT_LIST_ELEM(list, position);
}


/*  remove all the entries in 'list' */
void
skShortListRemoveAll(
    sk_short_list_t    *list)
{
    assert(list);
    list->sl_element_count = 0;
}


/*  add 'element' to 'list' */
int
skShortListInsert(
    sk_short_list_t    *list,
    const void         *element)
{
    int cmp;
    int top = list->sl_element_count - 1;
    int bot = 0;
    int pos;

    assert(list);
    assert(element);

    /* binary search */
    while (top >= bot) {
        pos = (bot + top) >> 1;
        cmp = list->sl_compare_fn(element, SK_SHORT_LIST_ELEM(list, pos),
                                  list->sl_compare_data);
        if (cmp < 0) {
            top = pos - 1;
        } else if (cmp > 0) {
            bot = pos + 1;
        } else {
            return SK_SHORT_LIST_OK_DUPLICATE;
        }
    }

    if (list->sl_element_count == SK_SHORT_LIST_MAX_ELEMENTS) {
        return SK_SHORT_LIST_ERR_FULL;
    }

    if (bot < (int)list->sl_element_count) {
        /* must move elements */
        memmove(SK_SHORT_LIST_ELEM(list, bot+1), SK_SHORT_LIST_ELEM(list, bot),
                (list->sl_element_count - bot) * list->sl_element_size);
    }
    memcpy(SK_SHORT_LIST_ELEM(list, bot), element, list->sl_element_size);
    ++list->sl_element_count;
    return SK_SHORT_LIST_OK;
}



/* **************************************************************** */

/*    SKUNIQUE WRAPPER AROUND FIELD LIST */

/* **************************************************************** */

/* structure for field info; used by sk_unique_t and sk_sort_unique_t */
typedef struct sk_uniq_field_info_st {
    const sk_fieldlist_t   *key_fields;
    const sk_fieldlist_t   *value_fields;
    const sk_fieldlist_t   *distinct_fields;

#if 0
    /* temp file context used to manage hashset files */
    sk_tempfilectx_t       *tmpctx;
#endif

    uint8_t                 key_num_fields;
    uint8_t                 key_octets;

    uint8_t                 value_num_fields;
    uint8_t                 value_octets;

    /* number of distinct fields */
    uint8_t                 distinct_num_fields;
    uint8_t                 distinct_octets;
} sk_uniq_field_info_t;


#define KEY_ONLY            1
#define VALUE_ONLY          2
#define DISTINCT_ONLY       4
#define KEY_VALUE           (KEY_ONLY | VALUE_ONLY)
#define KEY_DISTINCT        (KEY_ONLY | DISTINCT_ONLY)
#define VALUE_DISTINCT      (VALUE_ONLY | DISTINCT_ONLY)
#define KEY_VALUE_DISTINCT  (KEY_ONLY | VALUE_ONLY | DISTINCT_ONLY)


static struct allowed_fieldid_st {
    sk_fieldid_t    fieldid;
    uint8_t         kvd;
} allowed_fieldid[] = {
    {SK_FIELD_SIPv4,            KEY_DISTINCT},
    {SK_FIELD_DIPv4,            KEY_DISTINCT},
    {SK_FIELD_SPORT,            KEY_DISTINCT},
    {SK_FIELD_DPORT,            KEY_DISTINCT},
    {SK_FIELD_PROTO,            KEY_DISTINCT},
    {SK_FIELD_PACKETS,          KEY_DISTINCT},
    {SK_FIELD_BYTES,            KEY_DISTINCT},
    {SK_FIELD_FLAGS,            KEY_DISTINCT},
    {SK_FIELD_STARTTIME,        KEY_DISTINCT},
    {SK_FIELD_ELAPSED,          KEY_DISTINCT},
    {SK_FIELD_ENDTIME,          KEY_DISTINCT},
    {SK_FIELD_SID,              KEY_DISTINCT},
    {SK_FIELD_INPUT,            KEY_DISTINCT},
    {SK_FIELD_OUTPUT,           KEY_DISTINCT},
    {SK_FIELD_NHIPv4,           KEY_DISTINCT},
    {SK_FIELD_INIT_FLAGS,       KEY_DISTINCT},
    {SK_FIELD_REST_FLAGS,       KEY_DISTINCT},
    {SK_FIELD_TCP_STATE,        KEY_DISTINCT},
    {SK_FIELD_APPLICATION,      KEY_DISTINCT},
    {SK_FIELD_FTYPE_CLASS,      KEY_DISTINCT},
    {SK_FIELD_FTYPE_TYPE,       KEY_DISTINCT},
    {SK_FIELD_STARTTIME_MSEC,   KEY_DISTINCT},
    {SK_FIELD_ENDTIME_MSEC,     KEY_DISTINCT},
    {SK_FIELD_ELAPSED_MSEC,     KEY_DISTINCT},
    {SK_FIELD_ICMP_TYPE,        KEY_DISTINCT},
    {SK_FIELD_ICMP_CODE,        KEY_DISTINCT},
    {SK_FIELD_SIPv6,            KEY_DISTINCT},
    {SK_FIELD_DIPv6,            KEY_DISTINCT},
    {SK_FIELD_NHIPv6,           KEY_DISTINCT},
    {SK_FIELD_RECORDS,          VALUE_ONLY},
    {SK_FIELD_SUM_PACKETS,      VALUE_ONLY},
    {SK_FIELD_SUM_BYTES,        VALUE_ONLY},
    {SK_FIELD_SUM_ELAPSED,      VALUE_ONLY},
    {SK_FIELD_MIN_STARTTIME,    VALUE_ONLY},
    {SK_FIELD_MAX_ENDTIME,      VALUE_ONLY},
    {SK_FIELD_CALLER,           KEY_VALUE_DISTINCT}
};


/*
 *  status = uniqCheckFields(uniq_fields, err_fn);
 *
 *    Verify that the fields for a unique object make sense.  The
 *    fields are given in 'uniq_fields'.  Return 0 if the fields are
 *    valid.  Return -1 if they are invalid and print an error using
 *    the 'err_fn' if it is non-NULL.
 *
 *    For the fields to make sense, there must be more or more key
 *    fields and at least one distinct field or one aggregate value
 *    field.
 */
static int
uniqCheckFields(
    sk_uniq_field_info_t   *field_info,
    sk_msg_fn_t             err_fn)
{
#define ERR_FN  if (!err_fn) { } else err_fn

#define SAFE_SET(variable, value)               \
    {                                           \
        size_t sz = (value);                    \
        if (sz > UINT8_MAX) {                   \
            ERR_FN("Overflow");                 \
            return -1;                          \
        }                                       \
        variable = (uint8_t)value;              \
    }

    sk_fieldlist_iterator_t fl_iter;
    sk_fieldlist_iterator_t fl_iter2;
    sk_fieldentry_t *field;
    sk_fieldentry_t *field2;
    size_t num_allowed;
    uint8_t field_type;
    uint32_t field_id;
    size_t i;

    assert(field_info);

    num_allowed = sizeof(allowed_fieldid)/sizeof(struct allowed_fieldid_st);

    /* must have at least one key field */
    if (NULL == field_info->key_fields) {
        ERR_FN("No key fields were specified");
        return -1;
    }
    /* must have at least one value or one distinct field */
    if (NULL == field_info->value_fields
        && NULL == field_info->distinct_fields)
    {
        ERR_FN("Neither value nor distinct fields were specified");
        return -1;
    }

    /* handle key fields */
    skFieldListIteratorBind(field_info->key_fields, &fl_iter);
    while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
        field_type = 0;
        field_id = skFieldListEntryGetId(field);
        for (i = 0; i < num_allowed; ++i) {
            if (field_id == (uint32_t)allowed_fieldid[i].fieldid) {
                field_type = allowed_fieldid[i].kvd;
                break;
            }
        }
        if (field_type == 0) {
            ERR_FN("Unknown field %d", field->id);
            return -1;
        }
        if (!(field_type & KEY_ONLY)) {
            ERR_FN("Field %d is not allowed in the key", field->id);
            return -1;
        }
    }
    SAFE_SET(field_info->key_num_fields,
             skFieldListGetFieldCount(field_info->key_fields));
    SAFE_SET(field_info->key_octets,
             skFieldListGetBufferSize(field_info->key_fields));
    if (field_info->key_num_fields == 0 || field_info->key_octets == 0) {
        ERR_FN("No key fields were specified");
        return -1;
    }

    /* handle value fields */
    if (field_info->value_fields) {
        skFieldListIteratorBind(field_info->value_fields, &fl_iter);
        while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
            field_type = 0;
            field_id = skFieldListEntryGetId(field);
            for (i = 0; i < num_allowed; ++i) {
                if (field_id == (uint32_t)allowed_fieldid[i].fieldid) {
                    field_type = allowed_fieldid[i].kvd;
                    break;
                }
            }
            if (field_type == 0) {
                ERR_FN("Unknown field %d", field->id);
                return -1;
            }
            if (!(field_type & VALUE_ONLY)) {
                ERR_FN("Field %d is not allowed in the value", field->id);
                return -1;
            }
        }

        SAFE_SET(field_info->value_num_fields,
                 skFieldListGetFieldCount(field_info->value_fields));
        SAFE_SET(field_info->value_octets,
                 skFieldListGetBufferSize(field_info->value_fields));
    }

    /* handle distinct fields */
    if (field_info->distinct_fields) {
        skFieldListIteratorBind(field_info->distinct_fields, &fl_iter);
        while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
            field_type = 0;
            field_id = skFieldListEntryGetId(field);
            for (i = 0; i < num_allowed; ++i) {
                if (field_id == (uint32_t)allowed_fieldid[i].fieldid) {
                    field_type = allowed_fieldid[i].kvd;
                    break;
                }
            }
            if (field_type == 0) {
                ERR_FN("Unknown field %d", field->id);
                return -1;
            }
            if (!(field_type & DISTINCT_ONLY)) {
                ERR_FN("Field %d is not allowed in the distinct", field->id);
                return -1;
            }

            /* ensure distinct field is not part of key */
            if (SK_FIELD_CALLER == field_id) {
                void *field_ctx = skFieldListEntryGetContext(field);
                skFieldListIteratorBind(field_info->key_fields, &fl_iter2);
                while (NULL != (field2 = skFieldListIteratorNext(&fl_iter2))) {
                    if (skFieldListEntryGetId(field2) == SK_FIELD_CALLER) {
                        if (skFieldListEntryGetContext(field2) == field_ctx) {
                            ERR_FN("Will not count distinct"
                                   " value that is also part of key");
                            return -1;
                        }
                    }
                }
            } else {
                skFieldListIteratorBind(field_info->key_fields, &fl_iter2);
                while (NULL != (field2 = skFieldListIteratorNext(&fl_iter2))) {
                    if (skFieldListEntryGetId(field2) == field_id) {
                        ERR_FN("Will not count distinct"
                               " value that is also part of key");
                        return -1;
                    }
                }
            }
        }

        SAFE_SET(field_info->distinct_num_fields,
                 skFieldListGetFieldCount(field_info->distinct_fields));
        SAFE_SET(field_info->distinct_octets,
                 skFieldListGetBufferSize(field_info->distinct_fields));
    }

    /* ensure either values or distincts are specified */
    if (((field_info->value_num_fields + field_info->distinct_num_fields) == 0)
        || ((field_info->value_octets + field_info->distinct_octets) == 0))
    {
        ERR_FN("No value or distinct fields were specified");
        return -1;
    }

    return 0;
}



/* **************************************************************** */

/*    SKUNIQUE INTERNAL SUPPORT FOR DISTINCT FIELDS */

/* **************************************************************** */

#define DISTINCT_PTR(d_buffer, d_array, d_index)        \
    ((d_buffer) + (d_array)[(d_index)].dv_offset)

typedef enum {
    /* compute the dintinct count by keeping track of each value we
     * see.  DISTINCT_BITMAP is used for values up to 16bits;
     * DISTINCT_IPTREE is used for values up to 32 bits,
     * DISTINCT_HASHSET is used for larger values.
     * DISTINCT_HASHSET_FILES indicates file backing for the
     * hashset. */
    DISTINCT_BITMAP,
    DISTINCT_IPTREE,
    DISTINCT_SHORTLIST,
    DISTINCT_HASHSET,
    DISTINCT_HASHSET_FILES
} distinct_type_t;


typedef union distinct_tracker_un {
    sk_short_list_t    *dv_shortlist;
    hashset_file_t     *dv_hf;
    HashSet            *dv_hashset;
    sk_bitmap_t        *dv_bitmap;
    skIPTree_t         *dv_iptree;
} distinct_tracker_t;

typedef struct distinct_value_st {
    uint64_t            dv_count;
    distinct_tracker_t  dv_v;
    distinct_type_t     dv_type;
    uint8_t             dv_octets;
    uint8_t             dv_offset;
    sk_fieldid_t        dv_fieldid;
} distinct_value_t;


/*
 *  uniqDistinctFree(field_info, distincts);
 *
 *    Free all memory that was allocated by uniqDistinctAlloc().
 */
static void
uniqDistinctFree(
    const sk_uniq_field_info_t *field_info,
    distinct_value_t           *distincts)
{
    distinct_value_t *dist;
    uint8_t i;

    if (NULL == distincts) {
        return;
    }

    for (i = 0; i < field_info->distinct_num_fields; ++i) {
        dist = &distincts[i];
        switch (dist->dv_type) {
          case DISTINCT_BITMAP:
            skBitmapDestroy(&dist->dv_v.dv_bitmap);
            break;
          case DISTINCT_IPTREE:
            skIPTreeDelete(&dist->dv_v.dv_iptree);
            break;
          case DISTINCT_SHORTLIST:
            skShortListDestroy(&dist->dv_v.dv_shortlist);
            break;
          case DISTINCT_HASHSET_FILES:
#if 0
            if (dist->dv_v.dv_hf) {
                if (dist->dv_v.dv_hf->hf_hashset) {
                    hashset_free_table(dist->dv_v.dv_hf->hf_hashset);
                }
                if (dist->dv_v.dv_hf->hf_offsets) {
                    skVectorDestroy(dist->dv_v.dv_hf->hf_offsets);
                }
                if (dist->dv_v.dv_hf->hf_file) {
                    skTempFileRemove(uniq->tmpctx,
                                     dist->dv_v.dv_hf->hf_tmp_index);
                }
                memset(dist->dv_v.dv_hf, 0, sizeof(hashset_file_t));
                free(dist->dv_v.dv_hf);
            }
#endif  /* 0 */
            break;
          case DISTINCT_HASHSET:
            if (dist->dv_v.dv_hashset) {
                hashset_free_table(dist->dv_v.dv_hashset);
                dist->dv_v.dv_hashset = NULL;
            }
            break;
        }
    }
    free(distincts);
}


/*
 *  ok = uniqDistinctAlloc(field_info, &distincts);
 *
 *    Create the data structures required by 'field_info' to count
 *    distinct values and fill 'distincts' with the structures.
 *    Return 0 on success, or -1 on failure.
 */
static int
uniqDistinctAlloc(
    const sk_uniq_field_info_t     *field_info,
    distinct_value_t              **new_distincts)
{
    sk_fieldlist_iterator_t fl_iter;
    sk_fieldentry_t *field;
    distinct_value_t *distincts;
    distinct_value_t *dist;
    uint8_t total_octets = 0;

    if (0 == field_info->distinct_num_fields) {
        return 0;
    }

    distincts = (distinct_value_t*)calloc(field_info->distinct_num_fields,
                                          sizeof(distinct_value_t));
    if (NULL == distincts) {
        TRACEMSG(("Error allocating distinct field_info"));
        goto ERROR;
    }

    dist = distincts;

    /* determine how each field maps into the single buffer */
    skFieldListIteratorBind(field_info->distinct_fields, &fl_iter);
    while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
        dist->dv_octets = skFieldListEntryGetBinOctets(field);
        dist->dv_offset = total_octets;
        dist->dv_fieldid = (sk_fieldid_t)skFieldListEntryGetId(field);
        total_octets += dist->dv_octets;

        if (dist->dv_octets == 1) {
            dist->dv_type = DISTINCT_BITMAP;
            if (skBitmapCreate(&dist->dv_v.dv_bitmap,
                               1 << (dist->dv_octets * CHAR_BIT)))
            {
                TRACEMSG(("Error allocating bitmap"));
                dist->dv_v.dv_bitmap = NULL;
                goto ERROR;
            }
        } else {
            dist->dv_type = DISTINCT_SHORTLIST;
            if (skShortListCreate(
                    &dist->dv_v.dv_shortlist, dist->dv_octets,
                    COMP_FUNC_CAST(skFieldListEntryCompareBuffers),
                    (void*)field))
            {
                TRACEMSG(("Error allocating short list"));
                dist->dv_v.dv_shortlist = NULL;
                goto ERROR;
            }
        }
        ++dist;
        assert(dist <= distincts + field_info->distinct_num_fields);
    }

    *new_distincts = distincts;
    return 0;

  ERROR:
    uniqDistinctFree(field_info, distincts);
    return -1;
}


/*
 *  status = uniqDistinctShortListToHashSet(dist);
 *
 *    Convert the distinct count at 'dist' from using a short-list to
 *    count entries to the hash-set.  Return 0 on success, or -1 if
 *    there is a memory allocation failure.
 */
static int
uniqDistinctShortListToHashSet(
    distinct_value_t   *dist)
{
    HashSet *hashset = NULL;
    uint32_t i;
    int rv;

    assert(DISTINCT_SHORTLIST == dist->dv_type);

    hashset = hashset_create_set(dist->dv_octets,
                                 256, DEFAULT_LOAD_FACTOR);
    if (NULL == hashset) {
        TRACEMSG(("Error allocating hashset"));
        goto ERROR;
    }

    for (i = skShortListCountEntries(dist->dv_v.dv_shortlist); i > 0; ) {
        --i;
        rv = hashset_insert(
            hashset,
            (uint8_t*)skShortListGetElement(dist->dv_v.dv_shortlist, i));
        switch (rv) {
          case OK:
            break;
          case OK_DUPLICATE:
            /* this is okay, but unexpected */
            break;
          default:
            TRACEMSG(("Error inserting value into hashset"));
            goto ERROR;
        }
    }

    skShortListDestroy(&dist->dv_v.dv_shortlist);
    dist->dv_v.dv_hashset = hashset;
    dist->dv_type = DISTINCT_HASHSET;
    return 0;

  ERROR:
    return -1;
}


/*
 *  status = uniqDistinctIncrement(uniq_fields, distincts, key);
 *
 *    Increment the distinct counters given 'key'.  Return 0 on
 *    success or -1 on memory allocation failure.
 */
static int
uniqDistinctIncrement(
    const sk_uniq_field_info_t *field_info,
    distinct_value_t           *distincts,
    const uint8_t              *key)
{
    distinct_value_t *dist;
    uint8_t i;
    int rv;

    for (i = 0; i < field_info->distinct_num_fields; ++i) {
        dist = &distincts[i];
        switch (dist->dv_type) {
          case DISTINCT_BITMAP:
            skBitmapSetBit(dist->dv_v.dv_bitmap,
                           *(uint8_t*)DISTINCT_PTR(key, distincts, i));
            dist->dv_count = skBitmapGetHighCount(dist->dv_v.dv_bitmap);
            break;
          case DISTINCT_IPTREE:
            if (!skIPTreeCheckAddress(dist->dv_v.dv_iptree,
                                      *(uint32_t*)DISTINCT_PTR(key, distincts,
                                                               i)))
            {
                skIPTreeAddAddress(dist->dv_v.dv_iptree,
                                   *(uint32_t*)DISTINCT_PTR(key,distincts,i));
                ++dist->dv_count;
            }
            break;
          case DISTINCT_SHORTLIST:
            rv = skShortListInsert(dist->dv_v.dv_shortlist,
                                   (void*)DISTINCT_PTR(key,distincts,i));
            switch (rv) {
              case SK_SHORT_LIST_OK:
                ++dist->dv_count;
                break;
              case SK_SHORT_LIST_OK_DUPLICATE:
                break;
              case SK_SHORT_LIST_ERR_FULL:
                if (uniqDistinctShortListToHashSet(dist)) {
                    return -1;
                }
                rv = hashset_insert(dist->dv_v.dv_hashset,
                                    DISTINCT_PTR(key,distincts,i));
                switch (rv) {
                  case OK:
                    ++dist->dv_count;
                    break;
                  case OK_DUPLICATE:
                    break;
                  default:
                    TRACEMSG(("Error inserting value into hashset"));
                    return -1;
                }
                break;
              default:
                skAbortBadCase(rv);
            }
            break;
          case DISTINCT_HASHSET_FILES:
#if 0
            for (j = 0, beg = end = 0;
                 j < skVectorGetCount(dist->dv_v.dv_hf->hf_offsets);
                 ++j, beg = end)
            {
                skVectorGetValue(&end, dist->dv_v.dv_hf->hf_offsets, j);
                bot = beg;
                top = end;
                while (top > bot) {
                    pos = (bot + top) >> 1;
                    cmp = memcmp(DISTINCT_PTR(key, distincts, i),



               , dist->dv_octets);


 key_in, GET_KEY_PTR(bt, node->key, pos),
                     bt->cmp_size);
        if (cmp < 0) {
            top = pos - 1;
        } else if (cmp > 0) {
            bot = pos + 1;
        } else {
            *position = pos;
            return SKBTREE_OK;
        }
    }



            rv = hashset_insert(dist->dv_v.dv_hashset,
                                DISTINCT_PTR(key, distincts, i));
            switch (rv) {
              case OK:
                ++dist->dv_count;
                break;
              case OK_DUPLICATE:
                break;
              default:
                return -1;
            }
            break;




            if (dist->dv_v.dv_hf) {
                if (dist->dv_v.dv_hf->hf_hashset) {
                    hashset_free_table(dist->dv_v.dv_hf->hf_hashset);
                }
                if (dist->dv_v.dv_hf->hf_offsets) {
                    skVectorDestroy(dist->dv_v.dv_hf->hf_offsets);
                }
                if (dist->dv_v.dv_hf->hf_file) {
                    skTempFileRemove(uniq->tmpctx,
                                     dist->dv_v.dv_hf->hf_tmp_index);
                }
                memset(dist->dv_v.dv_hf, 0, sizeof(hashset_file_t));
                free(dist->dv_v.dv_hf);
            }
#endif  /* 0 */
            break;
          case DISTINCT_HASHSET:
            rv = hashset_insert(dist->dv_v.dv_hashset,
                                DISTINCT_PTR(key, distincts, i));
            switch (rv) {
              case OK:
                ++dist->dv_count;
                break;
              case OK_DUPLICATE:
                break;
              default:
                TRACEMSG(("Error inserting value into hashset"));
                return -1;
            }
            break;
        }
    }

    return 0;
}


/*
 *  uniqDistinctSetOutputBuf(uniq_fields, distincts, out_buf);
 *
 *    For all the distinct fields, fill the buffer at 'out_buf' to
 *    contain the number of distinct values.
 */
static void
uniqDistinctSetOutputBuf(
    const sk_uniq_field_info_t *field_info,
    const distinct_value_t     *distincts,
    uint8_t                    *out_buf)
{
    const distinct_value_t *dist;
    uint8_t i;

    for (i = 0; i < field_info->distinct_num_fields; ++i) {
        dist = &distincts[i];
        switch (dist->dv_octets) {
          case 1:
            *((uint8_t*)DISTINCT_PTR(out_buf, distincts, i))
                = (uint8_t)(dist->dv_count);
            break;

          case 3:
          case 5:
          case 6:
          case 7:
            {
                union array_uint64_t {
                    uint64_t  u64;
                    uint8_t   ar[8];
                } array_uint64;
                array_uint64.u64 = dist->dv_count;
#if SK_BIG_ENDIAN
                memcpy(DISTINCT_PTR(out_buf, distincts, i),
                       &array_uint64.ar[8-dist->dv_octets], dist->dv_octets);
#else
                memcpy(DISTINCT_PTR(out_buf, distincts, i),
                       &array_uint64.ar[0], dist->dv_octets);
#endif  /* #else of #if SK_BIG_ENDIAN */
            }
            break;

#if !SKUNIQ_USE_MEMCPY
          case 2:
            *((uint16_t*)DISTINCT_PTR(out_buf, distincts, i))
                = (uint16_t)(dist->dv_count);
            break;
          case 4:
            *((uint32_t*)DISTINCT_PTR(out_buf, distincts, i))
                = (uint32_t)(dist->dv_count);
            break;
          case 8:
            *((uint64_t*)DISTINCT_PTR(out_buf, distincts, i))
                = dist->dv_count;
            break;
          default:
            *((uint64_t*)DISTINCT_PTR(out_buf, distincts, i))
                = dist->dv_count;
            break;
#else  /* SKUNIQ_USE_MEMCPY */
          case 2:
            {
                uint16_t val16 = (uint16_t)(dist->dv_count);
                memcpy(DISTINCT_PTR(out_buf, distincts, i),
                       &val16, sizeof(val16));
            }
            break;
          case 4:
            {
                uint32_t val32 = (uint32_t)(dist->dv_count);
                memcpy(DISTINCT_PTR(out_buf, distincts, i),
                       &val32, sizeof(val32));
            }
            break;
          case 8:
            memcpy(DISTINCT_PTR(out_buf, distincts, i),
                   &dist->dv_count, sizeof(uint64_t));
            break;
          default:
            memcpy(DISTINCT_PTR(out_buf, distincts, i),
                   &dist->dv_count, sizeof(uint64_t));
            break;
#endif  /* #else of #if !SKUNIQ_USE_MEMCPY */
        }
    }
}


/*
 *  status = uniqDistinctReset(uniq_fields, distincts);
 *
 *    Reset the distinct counters.  Return 0 on success, or -1 on
 *    memory allocation error.
 */
static int
uniqDistinctReset(
    const sk_uniq_field_info_t *field_info,
    distinct_value_t           *distincts)
{
    distinct_value_t *dist;
    uint8_t i;

    for (i = 0; i < field_info->distinct_num_fields; ++i) {
        dist = &distincts[i];
        switch (dist->dv_type) {
          case DISTINCT_BITMAP:
            skBitmapClearAllBits(dist->dv_v.dv_bitmap);
            break;
          case DISTINCT_IPTREE:
            skIPTreeRemoveAll(dist->dv_v.dv_iptree);
            break;
          case DISTINCT_SHORTLIST:
            skShortListRemoveAll(dist->dv_v.dv_shortlist);
            break;
          case DISTINCT_HASHSET_FILES:
#if 0
            if (dist->dv_v.dv_hf) {
                if (dist->dv_v.dv_hf->hf_hashset) {
                    hashset_free_table(dist->dv_v.dv_hf->hf_hashset);
                }
                if (dist->dv_v.dv_hf->hf_offsets) {
                    skVectorDestroy(dist->dv_v.dv_hf->hf_offsets);
                }
                if (dist->dv_v.dv_hf->hf_file) {
                    skTempFileRemove(uniq->tmpctx,
                                     dist->dv_v.dv_hf->hf_tmp_index);
                }
                memset(dist->dv_v.dv_hf, 0, sizeof(hashset_file_t));
                free(dist->dv_v.dv_hf);
            }
#endif  /* 0 */
            break;
          case DISTINCT_HASHSET:
            if (dist->dv_v.dv_hashset) {
                hashset_free_table(dist->dv_v.dv_hashset);
            }
            dist->dv_v.dv_hashset = hashset_create_set(dist->dv_octets, 256,
                                                       DEFAULT_LOAD_FACTOR);
            if (NULL == dist->dv_v.dv_hashset) {
                TRACEMSG(("Error allocating hashset"));
                return -1;
            }
            break;
        }
        dist->dv_count = 0;
    }
    return 0;
}


/* **************************************************************** */

/*    SKUNIQUE INTERNAL SUPPORT USING TEMPORARY FILES */

/* **************************************************************** */

/*
 *  status = uniqTempWrite(field_info, fp, key_buf, value_buf, distincts);
 *
 *    Write the values from 'key_buffer', 'value_buffer', and any
 *    distinct fields (located on the 'uniq' object) to the file
 *    handle 'fp'.  Return 0 on success, or -1 on failure.
 *
 *    Data is written as follows:
 *
 *      the key_buffer
 *      the value_buffer
 *      for each distinct field:
 *          number of distinct values
 *          distinct value 1, distinct value 2, ...
 */
static int
uniqTempWrite(
    const sk_uniq_field_info_t *field_info,
    FILE                       *fp,
    const uint8_t              *key_buffer,
    const uint8_t              *value_buffer,
    const distinct_value_t     *dist)
{
    sk_bitmap_iter_t b_iter;
    skIPTreeIterator_t ipt_iter;
    hashset_iter h_iter;
    uint8_t *hash_key;
    uint16_t i;
    uint16_t j;
    uint32_t tmp32;
    uint8_t val8;

    /* write keys and values */
    if (!fwrite(key_buffer, field_info->key_octets, 1, fp)
        || (field_info->value_octets
            && !fwrite(value_buffer, field_info->value_octets, 1, fp)))
    {
        TRACEMSG_WRITE(field_info->key_octets + field_info->value_octets);
        return -1;
    }

    if (0 == field_info->distinct_num_fields) {
        return 0;
    }
    if (NULL == dist) {
        /* write a count of 0 for each distinct value */
        uint64_t count = 0;
        for (i = 0; i < field_info->distinct_num_fields; ++i) {
            if (!fwrite(&count, sizeof(uint64_t), 1, fp)) {
                TRACEMSG_WRITE(sizeof(uint64_t));
                return -1;
            }
        }
        return 0;
    }

    /* handle all the distinct fields */
    for (i = 0; i < field_info->distinct_num_fields; ++i, ++dist) {
        /* write the count */
        if (!fwrite(&dist->dv_count, sizeof(uint64_t), 1, fp)) {
            TRACEMSG_WRITE(sizeof(uint64_t));
            return -1;
        }
        /* write each value */
        switch (dist->dv_type) {
          case DISTINCT_BITMAP:
            assert(skBitmapGetHighCount(dist->dv_v.dv_bitmap)
                   == dist->dv_count);
            skBitmapIteratorBind(dist->dv_v.dv_bitmap, &b_iter);
            assert(1 == dist->dv_octets);
            while (SK_ITERATOR_OK == skBitmapIteratorNext(&b_iter, &tmp32)) {
                val8 = (uint8_t)tmp32;
                if (!fwrite(&val8, sizeof(uint8_t), 1, fp)) {
                    TRACEMSG_WRITE(dist->dv_octets);
                    return -1;
                }
            }
            break;

          case DISTINCT_IPTREE:
            assert(skIPTreeCountIPs(dist->dv_v.dv_iptree)
                   == dist->dv_count);
            skIPTreeIteratorBind(&ipt_iter, dist->dv_v.dv_iptree);
            while (SK_ITERATOR_OK == skIPTreeIteratorNext(&tmp32, &ipt_iter)) {
                if (!fwrite(&tmp32, sizeof(uint32_t), 1, fp)) {
                    TRACEMSG_WRITE(dist->dv_octets);
                    return -1;
                }
            }
            break;

          case DISTINCT_SHORTLIST:
            assert(skShortListCountEntries(dist->dv_v.dv_shortlist)
                   == dist->dv_count);
            for (j = skShortListCountEntries(dist->dv_v.dv_shortlist); j > 0; ){
                --j;
                if (!fwrite(skShortListGetElement(dist->dv_v.dv_shortlist, j),
                            dist->dv_octets, 1, fp))
                {
                    TRACEMSG_WRITE(dist->dv_octets);
                    return -1;
                }
            }
            break;

          case DISTINCT_HASHSET_FILES:
          case DISTINCT_HASHSET:
            assert(hashset_count_entries(dist->dv_v.dv_hashset)
                   == dist->dv_count);
            h_iter = hashset_create_iterator(dist->dv_v.dv_hashset);
            while (OK == hashset_iterate(dist->dv_v.dv_hashset,
                                         &h_iter, &hash_key))
            {
                if (!fwrite(hash_key, dist->dv_octets, 1, fp)) {
                    TRACEMSG_WRITE(dist->dv_octets);
                    return -1;
                }
            }
            break;
        }
    }

    return 0;
}


/*
 *  status = uniqTempReadAndMerge(field_info, fp, value_buffer, distints);
 *
 *    Read everything---except the key---which was written by the call
 *    to uniqTempWrite(), and merge those value into the current
 *    values for the 'value_buffer' and the distinct fields.  Return 0
 *    on success, or -1 on a read failure or on an error inserting a
 *    value into a distinct-related data structure.
 *
 *    See also uniqTempReadFinalFile().
 */
static int
uniqTempReadAndMerge(
    const sk_uniq_field_info_t *field_info,
    FILE                       *fp,
    uint8_t                    *value_buffer,
    distinct_value_t           *dist)
{
    uint8_t buf[4096];
    uint64_t count;
    uint16_t i;
    int rv;

    /* read the value and merge it into the current value */
    if (field_info->value_octets
        && !fread(buf, field_info->value_octets, 1, fp))
    {
        TRACEMSG_READ(field_info->value_octets, fp);
        return -1;
    }
    skFieldListMergeBuffers(field_info->value_fields, value_buffer, buf);

    /* handle the distinct fields */
    for (i = 0; i < field_info->distinct_num_fields; ++i, ++dist) {
        /* read the count */
        if (!fread(&count, sizeof(uint64_t), 1, fp)) {
            TRACEMSG_READ(sizeof(uint64_t), fp);
            return -1;
        }
        /* read each value and add to the distinct object */
        switch (dist->dv_type) {
          case DISTINCT_BITMAP:
            assert(1 == dist->dv_octets);
            while (count > 0) {
                --count;
                if (!fread(buf, sizeof(uint8_t), 1, fp)) {
                    TRACEMSG_READ(dist->dv_octets, fp);
                    return -1;
                }
                skBitmapSetBit(dist->dv_v.dv_bitmap, *buf);
            }
            dist->dv_count = skBitmapGetHighCount(dist->dv_v.dv_bitmap);
            break;

          case DISTINCT_IPTREE:
            while (count > 0) {
                --count;
                if (!fread(buf, sizeof(uint32_t), 1, fp)) {
                    TRACEMSG_READ(dist->dv_octets, fp);
                    return -1;
                }
                skIPTreeAddAddress(dist->dv_v.dv_iptree, *(uint32_t*)buf);
            }
            dist->dv_count = skIPTreeCountIPs(dist->dv_v.dv_iptree);
            break;

          case DISTINCT_SHORTLIST:
            rv = SK_SHORT_LIST_OK;
            while (count > 0) {
                --count;
                if (!fread(buf, dist->dv_octets, 1, fp)) {
                    TRACEMSG_READ(dist->dv_octets, fp);
                    return -1;
                }
                rv = skShortListInsert(dist->dv_v.dv_shortlist, buf);
                if (SK_SHORT_LIST_OK == rv) {
                    ++dist->dv_count;
                } else if (SK_SHORT_LIST_ERR_FULL == rv) {
                    if (uniqDistinctShortListToHashSet(dist)) {
                        return -1;
                    }
                    break;
                }
                /* else (SK_SHORT_LIST_OK_DUPLICATE == rv) which is
                 * unexpected but ignored */
            }
            if (SK_SHORT_LIST_ERR_FULL != rv) {
                /* successfully added all entries to the shortlist
                 * (this 'if' cannot check for 0==count, since we may
                 * have read final element) */
                assert(0 == count);
                break;
            }
            /* shortlist was converted to hashset. insert the entry we
             * just read */
            rv = hashset_insert(dist->dv_v.dv_hashset, buf);
            switch (rv) {
              case OK:
                ++dist->dv_count;
                break;
              case OK_DUPLICATE:
                break;
              default:
                TRACEMSG(("Error inserting value into hashset"));
                return -1;
            }
            /* now we need to handle any remaining distinct entries as
             * a hashset */
            /* FALLTHROUGH */

          case DISTINCT_HASHSET_FILES:
          case DISTINCT_HASHSET:
            while (count > 0) {
                --count;
                if (!fread(buf, dist->dv_octets, 1, fp)) {
                    TRACEMSG_READ(dist->dv_octets, fp);
                    return -1;
                }
                rv = hashset_insert(dist->dv_v.dv_hashset, buf);
                switch (rv) {
                  case OK:
                    ++dist->dv_count;
                    break;
                  case OK_DUPLICATE:
                    break;
                  default:
                    TRACEMSG(("Error inserting value into hashset"
                              " during merge"));
                    return -1;
                }
            }
            break;
        }
    }

    return 0;
}


/*
 *  status = uniqTempReadFinalFile(field_info, fp, value_buffer, distints);
 *
 *    Read everything---except the key---which was written by the call
 *    to uniqTempWrite(), and merge those value into the current
 *    values for the 'value_buffer' and the distinct fields.  Return 0
 *    on success, or -1 on a read failure.
 *
 *    This function is similar to uniqTempReadAndMerge(), but this
 *    function is meant to be called when a single temporary file
 *    remains.  Since there is a single file, there is no need to
 *    maintain a list of distinct values, and the distinct count can
 *    be determined from the distinct count in the file.
 */
static int
uniqTempReadFinalFile(
    const sk_uniq_field_info_t *field_info,
    FILE                       *fp,
    uint8_t                    *value_buffer,
    distinct_value_t           *dist)
{
    uint8_t buf[4096];
    size_t to_read;
    size_t exp_len;
    size_t res_len;
    uint64_t count;
    uint16_t i;

    /* read the value and merge it into the current value */
    if (field_info->value_octets
        && !fread(buf, field_info->value_octets, 1, fp))
    {
        TRACEMSG_READ(field_info->value_octets, fp);
        return -1;
    }
    skFieldListMergeBuffers(field_info->value_fields, value_buffer, buf);

    /* handle the distinct fields */
    for (i = 0; i < field_info->distinct_num_fields; ++i, ++dist) {
        /* read the count */
        if (!fread(&count, sizeof(uint64_t), 1, fp)) {
            TRACEMSG_READ(sizeof(uint64_t), fp);
            return -1;
        }
        /* determine the number of bytes to read */
        assert(dist->dv_octets > 0);
        to_read = dist->dv_octets * count;

        /* read the distinct data and throw it away */
        while (to_read) {
            exp_len = ((to_read < sizeof(buf)) ? to_read : sizeof(buf));
            res_len = fread(buf, 1, exp_len, fp);
            if (res_len != exp_len) {
                if (res_len > 0) {
                    TRACEMSG(("Short read: %" SK_PRIuZ "/%" SK_PRIuZ " bytes",
                              res_len, exp_len));
                } else {
                    TRACEMSG_READ(exp_len, fp);
                    for ( ; i < field_info->distinct_num_fields; ++i, ++dist) {
                        dist->dv_count = 0;
                    }
                    return -1;
                }
            }
            to_read -= res_len;
        }
        dist->dv_count = count;
    }

    return 0;
}



#if 0                           /* currently unused */
/*
 *  uniqTempCopyNode(field_info, source, dest);
 *
 *    Read the values and distincts from the file 'source' and write
 *    to the file 'dest'.  Return 0 on success, or -1 on read or write
 *    error.
 */
static int
uniqTempCopyNode(
    const sk_uniq_field_info_t *field_info,
    const distinct_value_t     *dist,
    FILE                       *source,
    FILE                       *dest)
{
    uint8_t buf[4096];
    size_t to_read;
    size_t exp_len;
    size_t res_len;
    uint64_t count;
    uint16_t i;

    /* read the value and merge it into the current value */
    if (field_info->value_octets) {
        if (!fread(buf, field_info->value_octets, 1, source)) {
            TRACEMSG_READ(field_info->value_octets, source);
            return -1;
        }
        if (!fwrite(buf, field_info->value_octets, 1, dest)) {
            TRACEMSG_WRITE(field_info->value_octets);
            return -1;
        }
    }

    /* handle the distinct fields */
    for (i = 0; i < field_info->distinct_num_fields; ++i, ++dist) {
        /* read and write the count */
        if (!fread(&count, sizeof(uint64_t), 1, source)) {
            TRACEMSG_READ(sizeof(uint64_t), source);
            return -1;
        }
        if (!fwrite(&count, sizeof(uint64_t), 1, dest)) {
            TRACEMSG_WRITE(sizeof(uint64_t));
            return -1;
        }

        /* determine the number of bytes to read */
        assert(dist->dv_octets > 0);
        to_read = dist->dv_octets * count;

        /* read and write the bytes */
        while (to_read) {
            exp_len = ((to_read < sizeof(buf)) ? to_read : sizeof(buf));
            res_len = fread(buf, 1, exp_len, source);
            if (res_len != exp_len) {
                if (res_len > 0) {
                    TRACEMSG(("Short read: %" SK_PRIuZ "/%" SK_PRIuZ " bytes",
                              res_len, exp_len));
                    exp_len = res_len;
                } else {
                    TRACEMSG_READ(exp_len, source);
                    return -1;
                }
            }

            res_len = fwrite(buf, 1, exp_len, dest);
            if (res_len != exp_len) {
                if (res_len > 0) {
                    TRACEMSG(("Short write: %" SK_PRIuZ "/%" SK_PRIuZ " bytes",
                              res_len, exp_len));
                } else {
                    TRACEMSG_WRITE(res_len);
                }
                return -1;
            }
            to_read -= res_len;
        }
    }

    return 0;
}
#endif  /* 0 */


/* **************************************************************** */

/*    SKUNIQUE USER API FOR RANDOM INPUT */

/* **************************************************************** */

/* structure for binning records */

/* typedef struct sk_unique_st sk_unique_t; */
struct sk_unique_st {
    /* information about the fields */
    sk_uniq_field_info_t    fi;

    /* where to write temporary files */
    char                   *temp_dir;

    /* the hash table */
    HashTable              *ht;

    /* error function for reporting of errors */
    sk_msg_fn_t             err_fn;

    /* the temp file context */
    sk_tempfilectx_t       *tmpctx;

    /* pointer to the current intermediate temp file; it's index is
     * given by the 'temp_idx' member */
    FILE                   *temp_fp;

    /* index of the intermediate temp file member 'temp_fp'. this is
     * one more than the temp file currently in use. */
    int                     temp_idx;

    uint32_t                hash_value_octets;

    /* whether the output should be sorted */
    unsigned                sort_output :1;

    /* whether PrepareForInput()/PrepareForOutput() have been called */
    unsigned                ready_for_input:1;
    unsigned                ready_for_output:1;

    /* whether to print debugging information */
    unsigned                print_debug:1;
};


/*
 *  status = uniqueCreateHashTable(uniq);
 *
 *    Create a hashlib hash table using the field information on
 *    'uniq'.  Return 0 on success, or -1 on failure.
 */
static int
uniqueCreateHashTable(
    sk_unique_t        *uniq)
{
    uint8_t no_val[HASHLIB_MAX_VALUE_WIDTH];

    memset(no_val, 0, sizeof(no_val));

    uniq->ht = hashlib_create_table(uniq->fi.key_octets,
                                    uniq->hash_value_octets,
                                    HTT_INPLACE,
                                    no_val,
                                    NULL,
                                    0,
                                    HASH_INITIAL_SIZE,
                                    DEFAULT_LOAD_FACTOR);
    if (NULL == uniq->ht) {
        uniq->err_fn("Error allocating hash table");
        return -1;
    }
    return 0;
}


/*
 *  uniqueDestroyHashTable(uniq);
 *
 *    Destroy the hashlib hash table stored on 'uniq'.
 */
static void
uniqueDestroyHashTable(
    sk_unique_t        *uniq)
{
    distinct_value_t *distincts;
    uint8_t *hash_key;
    uint8_t *hash_val;
    HASH_ITER ithash;

    if (NULL == uniq->ht) {
        return;
    }
    if (0 == uniq->fi.distinct_num_fields) {
        hashlib_free_table(uniq->ht);
        uniq->ht = NULL;
        return;
    }

    /* must loop through table and free the distincts */
    ithash = hashlib_create_iterator(uniq->ht);
    while (hashlib_iterate(uniq->ht, &ithash, &hash_key, &hash_val)
           != ERR_NOMOREENTRIES)
    {
        memcpy(&distincts, hash_val + uniq->fi.value_octets, sizeof(void*));
        uniqDistinctFree(&uniq->fi, distincts);
    }

    hashlib_free_table(uniq->ht);
    uniq->ht = NULL;
    return;
}


/*
 *  status = uniqueDumpHashToTemp(uniq);
 *
 *    Write the entries in the current hash table to the current
 *    temporary file on 'uniq, destroy the hash table, and open a new
 *    temporary file.  The entries are written in sorted order, where
 *    the sort algorithm will depend on whether the user requested
 *    sorted output.  Return 0 on success, or -1 on failure.
 */
static int
uniqueDumpHashToTemp(
    sk_unique_t        *uniq)
{
    distinct_value_t *distincts;
    uint8_t *hash_key;
    uint8_t *hash_val;
    HASH_ITER ithash;

    assert(uniq);
    assert(uniq->temp_fp);

    /* sort the hash entries using skFieldListCompareBuffers.  To sort
     * using memcmp(), we would need to ensure we use memcmp() when
     * reading/merging the values back out of the temp files. */
    hashlib_sort_entries_usercmp(uniq->ht,
                                 COMP_FUNC_CAST(skFieldListCompareBuffers),
                                 (void*)uniq->fi.key_fields);

    /* create an iterator for the hash table */
    ithash = hashlib_create_iterator(uniq->ht);

    UNIQUE_DEBUG(uniq,
                 ((SKUNIQUE_DEBUG_ENVAR ": Writing %u %s to '%s'"),
                  hashlib_count_entries(uniq->ht),
                  ((uniq->fi.distinct_num_fields > 0)
                   ? "key/value/distinct triples"
                   : "key/value pairs"),
                  skTempFileGetName(uniq->tmpctx, uniq->temp_idx)));

    /* iterate over the hash entries */
    if (0 == uniq->fi.distinct_num_fields) {
        while (hashlib_iterate(uniq->ht, &ithash, &hash_key, &hash_val)
               != ERR_NOMOREENTRIES)
        {
            if (uniqTempWrite(&uniq->fi, uniq->temp_fp,
                              hash_key, hash_val, NULL))
            {
                /* error writing, errno may or may not be set */
                uniq->err_fn(("Error writing key/value pair"
                              " to temporary file '%s': %s"),
                             skTempFileGetName(uniq->tmpctx, uniq->temp_idx),
                             strerror(errno));
                return -1;
            }
        }

    } else {
        while (hashlib_iterate(uniq->ht, &ithash, &hash_key, &hash_val)
               != ERR_NOMOREENTRIES)
        {
            memcpy(&distincts, hash_val + uniq->fi.value_octets, sizeof(void*));
            if (uniqTempWrite(&uniq->fi, uniq->temp_fp,
                              hash_key, hash_val, distincts))
            {
                /* error writing, errno may or may not be set */
                uniq->err_fn(("Error writing key/value/distinct triple"
                              " to temporary file '%s': %s"),
                             skTempFileGetName(uniq->tmpctx, uniq->temp_idx),
                             strerror(errno));
                return -1;
            }
        }
    }

    /* close the file */
    if (fclose(uniq->temp_fp) == EOF) {
        uniq->err_fn("Error closing temporary file '%s': %s",
                     skTempFileGetName(uniq->tmpctx, uniq->temp_idx),
                     strerror(errno));
        return -1;
    }

    /* success so far */
    UNIQUE_DEBUG(uniq, (SKUNIQUE_DEBUG_ENVAR ": Successfully wrote %s",
                        ((uniq->fi.distinct_num_fields > 0)
                         ? "key/value/distinct triples"
                         : "key/value pairs")));

    /* destroy the hash table */
    uniqueDestroyHashTable(uniq);

    /* open a new temporary file */
    uniq->temp_fp = skTempFileCreate(uniq->tmpctx, &uniq->temp_idx, NULL);
    if (NULL == uniq->temp_fp) {
        uniq->err_fn("Error creating temporary file: %s",
                     strerror(errno));
        return -1;
    }

    return 0;
}


/*  create a new unique object */
int
skUniqueCreate(
    sk_unique_t       **uniq)
{
    sk_unique_t *u;
    const char *env_value;
    uint32_t debug_lvl;

    u = (sk_unique_t*)calloc(1, sizeof(sk_unique_t));
    if (NULL == u) {
        *uniq = NULL;
        return -1;
    }

    u->temp_idx = -1;
    u->err_fn = &skMsgNone;

    env_value = getenv(SKUNIQUE_DEBUG_ENVAR);
    if (env_value && 0 == skStringParseUint32(&debug_lvl, env_value, 1, 0)) {
        u->print_debug = 1;
        u->err_fn = &skAppPrintErr;
    }

    *uniq = u;
    return 0;
}


/*  destroy a unique object; cleans up any temporary files; etc. */
void
skUniqueDestroy(
    sk_unique_t       **uniq)
{
    sk_unique_t *u;

    if (NULL == uniq || NULL == *uniq) {
        return;
    }

    u = *uniq;
    *uniq = NULL;

    if (u->temp_fp) {
        fclose(u->temp_fp);
        u->temp_fp = NULL;
    }
    skTempFileTeardown(&u->tmpctx);
    u->temp_idx = -1;
    if (u->ht) {
        uniqueDestroyHashTable(u);
    }
    if (u->temp_dir) {
        free(u->temp_dir);
    }

    free(u);
}


/*  specify that output from 'uniq' should be sorted */
int
skUniqueSetSortedOutput(
    sk_unique_t        *uniq)
{
    assert(uniq);

    if (uniq->ready_for_input) {
        uniq->err_fn("May not call skUniqueSetSortedOutput"
                     " after calling skUniquePrepareForInput");
        return -1;
    }
    uniq->sort_output = 1;
    return 0;
}


/*  specify the temporary directory. */
void
skUniqueSetTempDirectory(
    sk_unique_t        *uniq,
    const char         *temp_dir)
{
    assert(uniq);

    if (uniq->ready_for_input) {
        uniq->err_fn("May not call skUniqueSetTempDirectory"
                     " after calling skUniquePrepareForInput");
        return;
    }

    if (uniq->temp_dir) {
        free(uniq->temp_dir);
        uniq->temp_dir = NULL;
    }
    if (temp_dir) {
        uniq->temp_dir = strdup(temp_dir);
    }
}


/* set function used to report errors */
void
skUniqueSetErrorFunction(
    sk_unique_t        *uniq,
    sk_msg_fn_t         err_fn)
{
    if (NULL == err_fn) {
        uniq->err_fn = skMsgNone;
    } else {
        uniq->err_fn = err_fn;
    }
}


/*  set the fields that 'uniq' will use. */
int
skUniqueSetFields(
    sk_unique_t            *uniq,
    const sk_fieldlist_t   *key_fields,
    const sk_fieldlist_t   *distinct_fields,
    const sk_fieldlist_t   *agg_value_fields)
{
    assert(uniq);

    if (uniq->ready_for_input) {
        uniq->err_fn("May not call skUniqueSetFields"
                     " after calling skUniquePrepareForInput");
        return -1;
    }

    memset(&uniq->fi, 0, sizeof(sk_uniq_field_info_t));
    uniq->fi.key_fields = key_fields;
    uniq->fi.distinct_fields = distinct_fields;
    uniq->fi.value_fields = agg_value_fields;

    return 0;
}


/*  tell the unique object that initialization is complete.  return an
 *  error if the object is not completely specified. */
int
skUniquePrepareForInput(
    sk_unique_t        *uniq)
{
    sk_msg_fn_t err_fn;         /* required by SAFE_SET() */

    assert(uniq);
    err_fn = uniq->err_fn;

    if (uniq->ready_for_input) {
        return 0;
    }

    if (uniqCheckFields(&uniq->fi, uniq->err_fn)) {
        return -1;
    }

    /* set sizes for the hash table */
    SAFE_SET(uniq->hash_value_octets,
             (uniq->fi.value_octets
              + (uniq->fi.distinct_num_fields ? sizeof(void*) : 0)));

    /* create the hash table */
    if (uniqueCreateHashTable(uniq)) {
        return -1;
    }

    /* initialize temp file context on the unique object */
    if (skTempFileInitialize(&uniq->tmpctx, uniq->temp_dir,
                             NULL, uniq->err_fn))
    {
        return -1;
    }

    /* open an intermediate file */
    uniq->temp_fp = skTempFileCreate(uniq->tmpctx, &uniq->temp_idx, NULL);
    if (NULL == uniq->temp_fp) {
        uniq->err_fn("Error creating intermediate temporary file: %s",
                     strerror(errno));
        return -1;
    }

#if 0
    /* initialize another temp file context of the field info list */
    if (skTempFileInitialize(&uniq->fi.tmpctx, uniq->temp_dir, NULL,
                             uniq->err_fn))
    {
        return -1;
    }
#endif

    uniq->ready_for_input = 1;
    return 0;
}


/*  add a flow record to a unique object */
int
skUniqueAddRecord(
    sk_unique_t        *uniq,
    const rwRec        *rwrec)
{
    distinct_value_t *distincts = NULL;
    uint8_t field_buf[HASHLIB_MAX_KEY_WIDTH];
    uint8_t *hash_val;
    uint32_t memory_error = 0;
    int rv;

    assert(uniq);
    assert(uniq->ht);
    assert(rwrec);

    if (!uniq->ready_for_input) {
        uniq->err_fn("May not call skUniqueAddRecord"
                     " before calling skUniquePrepareForInput");
        return -1;
    }

    for (;;) {
        skFieldListRecToBinary(uniq->fi.key_fields, rwrec, field_buf);

        /* the 'insert' will set 'hash_val' to the memory to use to
         * store the values. either fresh memory or the existing
         * value(s). */
        rv = hashlib_insert(uniq->ht, field_buf, &hash_val);
        switch (rv) {
          case OK:
            /* new key; don't increment value until we are sure we can
             * allocate the space for the distinct fields */
            skFieldListInitializeBuffer(uniq->fi.value_fields, hash_val);
            if (uniq->fi.distinct_num_fields) {
                skFieldListRecToBinary(uniq->fi.distinct_fields, rwrec,
                                       field_buf);
                if (uniqDistinctAlloc(&uniq->fi, &distincts)) {
                    memory_error |= 2;
                    break;
                }
                if (uniqDistinctIncrement(&uniq->fi, distincts, field_buf)) {
                    memory_error |= 4;
                    break;
                }
                memcpy(hash_val + uniq->fi.value_octets, &distincts,
                       sizeof(void*));
            }
            skFieldListAddRecToBuffer(uniq->fi.value_fields, rwrec, hash_val);
            return 0;

          case OK_DUPLICATE:
            /* existing key; merge the distinct fields first, then
             * merge the value */
            if (uniq->fi.distinct_num_fields) {
                memcpy(&distincts, hash_val + uniq->fi.value_octets,
                       sizeof(void*));
                skFieldListRecToBinary(uniq->fi.distinct_fields, rwrec,
                                       field_buf);
                if (uniqDistinctIncrement(&uniq->fi, distincts, field_buf)) {
                    memory_error |= 8;
                    break;
                }
            }
            skFieldListAddRecToBuffer(uniq->fi.value_fields, rwrec, hash_val);
            return 0;

          case ERR_OUTOFMEMORY:
          case ERR_NOMOREBLOCKS:
            memory_error |= 1;
            break;

          default:
            uniq->err_fn("Unexpected return code '%d' from hash table insert",
                         rv);
            return -1;
        }

        /* ran out of memory somewhere */
        TRACEMSG((("Memory error code is %" PRIu32), memory_error));

        if (memory_error > (1u << 31)) {
            /* this is our second memory error */
            if (OK != rv) {
                uniq->err_fn(("Unexpected return code '%d'"
                              " from hash table insert on new hash table"),
                             rv);
            } else {
                uniq->err_fn(("Error allocating memory after writing"
                              " hash table to temporary file"));
            }
            return -1;
        }
        memory_error |= (1u << 31);

        /*
         *  If (memory_error & 8) then there is a partially updated
         *  distinct count.  This should not matter as long as we can
         *  write the current values to disk and then reset
         *  everything.  At worst, the distinct value for this key
         *  will appear in two separate temporary files, but that
         *  should be resolved then the distinct values from the two
         *  files for this key are merged.
         */

        /* out of memory */
        if (uniqueDumpHashToTemp(uniq)) {
            return -1;
        }
        /* re-create the hash table */
        if (uniqueCreateHashTable(uniq)) {
            return -1;
        }
    }

    return 0;                   /* NOTREACHED */
}


/*  get ready to return records to the caller. */
int
skUniquePrepareForOutput(
    sk_unique_t        *uniq)
{
    if (uniq->ready_for_output) {
        return 0;
    }
    if (!uniq->ready_for_input) {
        uniq->err_fn("May not call skUniquePrepareForOutput"
                     " before calling skUniquePrepareForInput");
        return -1;
    }

    if (uniq->temp_idx > 0) {
        /* dump the current/final hash entries to a file */
        if (uniqueDumpHashToTemp(uniq)) {
            return -1;
        }
    } else if (uniq->sort_output) {
        /* need to sort using the skFieldListCompareBuffers function */
        hashlib_sort_entries_usercmp(uniq->ht,
                                     COMP_FUNC_CAST(skFieldListCompareBuffers),
                                     (void*)uniq->fi.key_fields);
    }

    uniq->ready_for_output = 1;
    return 0;
}


/****************************************************************
 * Iterator for handling one hash table, no distinct counts
 ***************************************************************/

typedef struct uniqiter_simple_st {
    sk_uniqiter_reset_fn_t  reset_fn;
    sk_uniqiter_next_fn_t   next_fn;
    sk_uniqiter_free_fn_t   free_fn;
    sk_unique_t            *uniq;
    HASH_ITER               ithash;
} uniqiter_simple_t;


/*
 *  status = uniqIterSimpleReset(iter);
 *
 *    Implementation for skUniqueIteratorReset().
 */
static int
uniqIterSimpleReset(
    sk_unique_iterator_t   *v_iter)
{
    uniqiter_simple_t *iter = (uniqiter_simple_t*)v_iter;

    UNIQUE_DEBUG(iter->uniq, ((SKUNIQUE_DEBUG_ENVAR ": Resetting simple"
                               " iterator; num entries = %" PRIu32),
                              hashlib_count_entries(iter->uniq->ht)));

    /* create the iterator */
    iter->ithash = hashlib_create_iterator(iter->uniq->ht);
    return 0;
}


/*
 *  status = uniqIterSimpleNext(iter, &key, &distinct, &value);
 *
 *    Implementation for skUniqueIteratorNext().
 */
static int
uniqIterSimpleNext(
    sk_unique_iterator_t           *v_iter,
    uint8_t                       **key_fields_buffer,
    uint8_t                UNUSED(**distinct_fields_buffer),
    uint8_t                       **value_fields_buffer)
{
    uniqiter_simple_t *iter = (uniqiter_simple_t*)v_iter;

    if (hashlib_iterate(iter->uniq->ht, &iter->ithash,
                        key_fields_buffer, value_fields_buffer)
        == ERR_NOMOREENTRIES)
    {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }
    return SK_ITERATOR_OK;
}


/*
 *  uniqIterSimpleDestroy(&iter);
 *
 *    Implementation for skUniqueIteratorDestroy().
 */
static void
uniqIterSimpleDestroy(
    sk_unique_iterator_t  **v_iter)
{
    uniqiter_simple_t *iter;

    if (v_iter && *v_iter) {
        iter = *(uniqiter_simple_t**)v_iter;
        memset(iter, 0, sizeof(uniqiter_simple_t));
        free(iter);
        *v_iter = NULL;
    }
}


/*
 *  status = uniqIterSimpleCreate(uniq, &iter);
 *
 *    Helper function for skUniqueIteratorCreate().
 */
static int
uniqIterSimpleCreate(
    sk_unique_t            *uniq,
    sk_unique_iterator_t  **new_iter)
{
    uniqiter_simple_t *iter;

    iter = (uniqiter_simple_t*)calloc(1, sizeof(uniqiter_simple_t));
    if (NULL == iter) {
        uniq->err_fn("Error allocating unique iterator");
        return -1;
    }

    iter->uniq = uniq;
    iter->reset_fn = uniqIterSimpleReset;
    iter->next_fn = uniqIterSimpleNext;
    iter->free_fn = uniqIterSimpleDestroy;

    if (uniqIterSimpleReset((sk_unique_iterator_t*)iter)) {
        uniqIterSimpleDestroy((sk_unique_iterator_t**)&iter);
        return -1;
    }

    *new_iter = (sk_unique_iterator_t*)iter;
    return 0;
}



/****************************************************************
 * Iterator for handling distinct values in one hash table
 ***************************************************************/

typedef struct uniqiter_distinct_st {
    sk_uniqiter_reset_fn_t  reset_fn;
    sk_uniqiter_next_fn_t   next_fn;
    sk_uniqiter_free_fn_t   free_fn;
    sk_unique_t            *uniq;
    HASH_ITER               ithash;
    uint8_t                 returned_buf[HASH_MAX_NODE_BYTES];
} uniqiter_distinct_t;


/*
 *  status = uniqIterDistinctReset();
 *
 *    Implementation for skUniqueIteratorReset(iter).
 */
static int
uniqIterDistinctReset(
    sk_unique_iterator_t   *v_iter)
{
    uniqiter_distinct_t *iter = (uniqiter_distinct_t*)v_iter;

    UNIQUE_DEBUG(iter->uniq, ((SKUNIQUE_DEBUG_ENVAR ": Resetting distinct"
                               " iterator; num entries = %" PRIu32),
                              hashlib_count_entries(iter->uniq->ht)));

    /* create the iterator */
    iter->ithash = hashlib_create_iterator(iter->uniq->ht);
    return 0;
}


/*
 *  status = uniqIterDistinctNext();
 *
 *    Implementation for skUniqueIteratorNext(iter, &key, &distinct, &value).
 */
static int
uniqIterDistinctNext(
    sk_unique_iterator_t   *v_iter,
    uint8_t               **key_fields_buffer,
    uint8_t               **distinct_fields_buffer,
    uint8_t               **value_fields_buffer)
{
    uniqiter_distinct_t *iter = (uniqiter_distinct_t*)v_iter;
    distinct_value_t *distincts;

    if (hashlib_iterate(iter->uniq->ht, &iter->ithash,
                        key_fields_buffer, value_fields_buffer)
        == ERR_NOMOREENTRIES)
    {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }

    memcpy(&distincts, *value_fields_buffer + iter->uniq->fi.value_octets,
           sizeof(void*));
    uniqDistinctSetOutputBuf(&iter->uniq->fi, distincts, iter->returned_buf);
    *distinct_fields_buffer = iter->returned_buf;
    return SK_ITERATOR_OK;
}


/*
 *  uniqIterDistinctDestroy(&iter);
 *
 *    Implementation for skUniqueIteratorDestroy().
 */
static void
uniqIterDistinctDestroy(
    sk_unique_iterator_t  **v_iter)
{
    uniqiter_distinct_t *iter;

    if (v_iter && *v_iter) {
        iter = *(uniqiter_distinct_t**)v_iter;
        memset(iter, 0, sizeof(uniqiter_distinct_t));
        free(iter);
        *v_iter = NULL;
    }
}


/*
 *  status = uniqIterDistinctCreate(uniq, &iter);
 *
 *    Helper function for skUniqueIteratorCreate().
 */
static int
uniqIterDistinctCreate(
    sk_unique_t            *uniq,
    sk_unique_iterator_t  **new_iter)
{
    uniqiter_distinct_t *iter;

    assert(uniq);
    assert(uniq->fi.distinct_num_fields > 0);

    iter = (uniqiter_distinct_t*)calloc(1, sizeof(uniqiter_distinct_t));
    if (NULL == iter) {
        uniq->err_fn("Error allocating unique iterator");
        return -1;
    }

    iter->uniq = uniq;
    iter->reset_fn = uniqIterDistinctReset;
    iter->next_fn = uniqIterDistinctNext;
    iter->free_fn = uniqIterDistinctDestroy;

    if (uniqIterDistinctReset((sk_unique_iterator_t*)iter)) {
        uniqIterDistinctDestroy((sk_unique_iterator_t**)iter);
        return -1;
    }

    *new_iter = (sk_unique_iterator_t*)iter;
    return 0;
}


/****************************************************************
 * Iterator for handling temporary files
 ***************************************************************/

typedef struct uniqiter_tempfiles_st {
    sk_uniqiter_reset_fn_t  reset_fn;
    sk_uniqiter_next_fn_t   next_fn;
    sk_uniqiter_free_fn_t   free_fn;
    sk_unique_t            *uniq;
    distinct_value_t       *distincts;
    skheap_t               *heap;
    FILE                   *fps[MAX_MERGE_FILES];
    uint8_t                 key[MAX_MERGE_FILES][HASHLIB_MAX_KEY_WIDTH];
    uint8_t                 returned_buf[HASH_MAX_NODE_BYTES];
    uint16_t                open_count;
} uniqiter_tempfiles_t;


/*
 *  status = uniqIterCompHeapNodes(b, a, v_uniq);
 *
 *    Callback function used by the heap.
 */
static int
uniqIterCompHeapNodes(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_iter);

/*
 *  status = uniqIterOpenAllTempFiles(iter);
 *
 *    Open all the temporary files associated with an iterator.
 */
static int
uniqIterOpenAllTempFiles(
    uniqiter_tempfiles_t   *iter);


/*
 *  status = uniqIterTempfilesReset(iter);
 *
 *    Implementation for skUniqueIteratorReset().
 */
static int
uniqIterTempfilesReset(
    sk_unique_iterator_t   *v_iter)
{
    uniqiter_tempfiles_t *iter = (uniqiter_tempfiles_t*)v_iter;
    uint16_t j;

    /* NOTE: the idea of resetting this iterator is completely broken
     * since uniqIterOpenAllTempFiles() assumes the first temporary
     * file to process is #0, which will not be true if it has already
     * been called. */

    UNIQUE_DEBUG(iter->uniq, ((SKUNIQUE_DEBUG_ENVAR ": Resetting tempfiles"
                               " iterator; num files = %d"),
                              iter->open_count));

    /* if files are already open (e.g., caller has reset an active
     * iterator), we need to close them first */
    for (j = 0; j < iter->open_count; ++j) {
        if (iter->fps[j]) {
            fclose(iter->fps[j]);
        }
        iter->fps[j] = NULL;
    }

    /* open all temp files---this will also merge temp files if there
     * are not enough file handles to open all temp files */
    iter->open_count = uniqIterOpenAllTempFiles(iter);
    if (iter->open_count == (uint16_t)-1) {
        return -1;
    }

    /* read the first key from each temp file, put into the work
     * buffer */
    for (j = 0; j < iter->open_count; ++j) {
        if (!fread(iter->key[j], iter->uniq->fi.key_octets, 1, iter->fps[j])) {
            TRACEMSG_READ(iter->uniq->fi.key_octets, iter->fps[j]);
            if (feof(iter->fps[j])) {
                UNIQUE_DEBUG(iter->uniq, (SKUNIQUE_DEBUG_ENVAR
                                          ": Ignoring empty temporary file"));
                continue;
            }
            iter->uniq->err_fn(("Error reading first key"
                                " from temporary file: %s"),
                               strerror(errno));
            return -1;
        }
        skHeapInsert(iter->heap, &j);
    }

    if (skHeapGetNumberEntries(iter->heap) == 0) {
        iter->uniq->err_fn("Could not read records from any temporary files");
        return -1;
    }

    return 0;
}


/*
 *  status = uniqIterTempfilesNext(iter, &key, &distinct, &value);
 *
 *    Implementation for skUniqueIteratorNext().
 */
static int
uniqIterTempfilesNext(
    sk_unique_iterator_t   *v_iter,
    uint8_t               **key_fields_buffer,
    uint8_t               **distinct_fields_buffer,
    uint8_t               **value_fields_buffer)
{
    uniqiter_tempfiles_t *iter = (uniqiter_tempfiles_t*)v_iter;
    uint16_t lowest;
    uint16_t *top_heap;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];

    /* get the index of the file with the lowest key; which is at
     * the top of the heap */
    if (SKHEAP_OK != skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap)) {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }
    lowest = *top_heap;

    if (skHeapGetNumberEntries(iter->heap) == 1) {
        /* down to the final file. I'm not sure how much benefit there
         * is for special casing this last file.  Code works if this
         * block is removed. */

        /* initialze values */
        skFieldListInitializeBuffer(iter->uniq->fi.value_fields,merged_values);

        /* read data from the final file and merge it into the values
         * and distincts */
        if (uniqTempReadFinalFile(&iter->uniq->fi, iter->fps[lowest],
                                  merged_values, iter->distincts))
        {
            iter->uniq->err_fn("Error merging values from temporary file");
            return -1;
        }

        /* set user's pointers to the buffers on the iterator, and
         * write the key, values, and distincts into those buffers */
        *key_fields_buffer = iter->returned_buf;
        memcpy(*key_fields_buffer, iter->key[lowest],
               iter->uniq->fi.key_octets);

        *value_fields_buffer = iter->returned_buf + iter->uniq->fi.key_octets;
        memcpy(*value_fields_buffer, merged_values,
               iter->uniq->fi.value_octets);

        if (iter->uniq->fi.distinct_num_fields) {
            *distinct_fields_buffer = (iter->returned_buf
                                       + iter->uniq->fi.key_octets
                                       + iter->uniq->fi.value_octets);
            uniqDistinctSetOutputBuf(&iter->uniq->fi, iter->distincts,
                                     *distinct_fields_buffer);
        }

        /* replace the record we just processed */
        if (!fread(iter->key[lowest], iter->uniq->fi.key_octets,
                   1, iter->fps[lowest]))
        {
            /* read failed and no more data for this file;
             * remove it from the heap */
            TRACEMSG_READ(iter->uniq->fi.key_octets, iter->fps[lowest]);
            UNIQUE_DEBUG(iter->uniq,
                         ((SKUNIQUE_DEBUG_ENVAR
                           ": Finished reading records from file #%u"),
                          lowest));
            skHeapExtractTop(iter->heap, NULL);
        }

        return SK_ITERATOR_OK;
    }

    /* cache this low key and initialze values and distincts */
    memcpy(cached_key, iter->key[lowest], iter->uniq->fi.key_octets);
    skFieldListInitializeBuffer(iter->uniq->fi.value_fields, merged_values);
    if (iter->uniq->fi.distinct_num_fields) {
        if (uniqDistinctReset(&iter->uniq->fi, iter->distincts)) {
            iter->uniq->err_fn("Error allocating table for distinct values");
            return -1;
        }
    }

    /* loop over all files until we get a key that does not match the
     * cached_key */
    for (;;) {
        /* read data from the file and merge it into the values and
         * distincts */
        if (uniqTempReadAndMerge(&iter->uniq->fi, iter->fps[lowest],
                                 merged_values, iter->distincts))
        {
            UNIQUE_DEBUG(iter->uniq,
                         ((SKUNIQUE_DEBUG_ENVAR ": uniqTempReadAndMerge()"
                           " failed for file #%u"), lowest));
            iter->uniq->err_fn("Error merging values from temporary file");
            return -1;
        }

        /* replace the record we just processed */
        if (fread(iter->key[lowest], iter->uniq->fi.key_octets,
                   1, iter->fps[lowest]))
        {
            /* read succeeded. insert the new entry into the
             * heap. */
            skHeapReplaceTop(iter->heap, &lowest, NULL);
        } else {
            /* read failed and no more data for this file;
             * remove it from the heap */
            TRACEMSG_READ(iter->uniq->fi.key_octets, iter->fps[lowest]);
            UNIQUE_DEBUG(iter->uniq,
                         ((SKUNIQUE_DEBUG_ENVAR
                           ": Finished reading records from file #%u"),
                          lowest));
            skHeapExtractTop(iter->heap, NULL);
        }

        /* get the new value at the top of the heap and see if
         * it matches the cached_key */
        if (SKHEAP_OK != skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap)) {
            /* heap is empty */
            break;
        }
        lowest = *top_heap;

        if (skFieldListCompareBuffers(cached_key, iter->key[lowest],
                                      iter->uniq->fi.key_fields))
        {
            /* keys differ */
            break;
        }
        /* else keys are the same: add this record's values */
    }

    /* set user's pointers to the buffers on the iterator, and write
     * the key, values, and distincts into those buffers */
    *key_fields_buffer = iter->returned_buf;
    memcpy(*key_fields_buffer, cached_key, iter->uniq->fi.key_octets);

    *value_fields_buffer = iter->returned_buf + iter->uniq->fi.key_octets;
    memcpy(*value_fields_buffer, merged_values, iter->uniq->fi.value_octets);

    if (iter->uniq->fi.distinct_num_fields) {
        *distinct_fields_buffer = (iter->returned_buf
                                   + iter->uniq->fi.key_octets
                                   + iter->uniq->fi.value_octets);
        uniqDistinctSetOutputBuf(&iter->uniq->fi, iter->distincts,
                                 *distinct_fields_buffer);
    }

    return SK_ITERATOR_OK;
}


/*
 *  uniqIterTempfilesDestroy(iter);
 *
 *    Implementation for skUniqueIteratorDestroy().
 */
static void
uniqIterTempfilesDestroy(
    sk_unique_iterator_t  **v_iter)
{
    uniqiter_tempfiles_t *iter;
    size_t i;

    if (v_iter && *v_iter) {
        iter = *(uniqiter_tempfiles_t**)v_iter;

        for (i = 0; i < iter->open_count; ++i) {
            if (iter->fps[i]) {
                fclose(iter->fps[i]);
            }
            iter->fps[i] = NULL;
        }

        uniqDistinctFree(&iter->uniq->fi, iter->distincts);

        if (iter->heap) {
            skHeapFree(iter->heap);
        }

        memset(iter, 0, sizeof(uniqiter_tempfiles_t));
        free(iter);
        *v_iter = NULL;
    }
}


/*
 *  status = uniqIterTempfilesCreate(uniq, &iter);
 *
 *    Helper function for skUniqueIteratorCreate().
 */
static int
uniqIterTempfilesCreate(
    sk_unique_t            *uniq,
    sk_unique_iterator_t  **new_iter)
{
    uniqiter_tempfiles_t *iter;

    iter = (uniqiter_tempfiles_t*)calloc(1, sizeof(uniqiter_tempfiles_t));
    if (NULL == iter) {
        uniq->err_fn("Error allocating unique iterator");
        return -1;
    }

    iter->uniq = uniq;
    iter->reset_fn = uniqIterTempfilesReset;
    iter->next_fn = uniqIterTempfilesNext;
    iter->free_fn = uniqIterTempfilesDestroy;

    iter->heap = skHeapCreate2(uniqIterCompHeapNodes, MAX_MERGE_FILES,
                               sizeof(uint16_t), NULL, iter);
    if (NULL == iter->heap) {
        uniq->err_fn("Error allocating heap for unique iterator");
        free(iter);
        return -1;
    }

    /* set up the handling of distinct field(s) */
    if (uniq->fi.distinct_num_fields) {
        if (uniqDistinctAlloc(&uniq->fi, &iter->distincts)) {
            uniq->err_fn("Error allocating unique iterator");
            skHeapFree(iter->heap);
            free(iter);
            return -1;
        }
    }

    if (uniqIterTempfilesReset((sk_unique_iterator_t*)iter)) {
        uniqIterTempfilesDestroy((sk_unique_iterator_t**)&iter);
        return -1;
    }

    *new_iter = (sk_unique_iterator_t*)iter;
    return 0;
}


/*  create iterator to get bins from the unique object; calls one of
 *  the helper functions above */
int
skUniqueIteratorCreate(
    sk_unique_t            *uniq,
    sk_unique_iterator_t  **new_iter)
{
    if (!uniq->ready_for_output) {
        uniq->err_fn("May not call skUniqueIteratorCreate"
                     " before calling skUniquePrepareForOutput");
        return -1;
    }
    if (uniq->temp_idx > 0) {
        return uniqIterTempfilesCreate(uniq, new_iter);
    }

    if (uniq->fi.distinct_num_fields) {
        return uniqIterDistinctCreate(uniq, new_iter);
    }

    return uniqIterSimpleCreate(uniq, new_iter);
}


/*
 *  status = uniqIterCompHeapNodes(b, a, v_uniq);
 *
 *    Callback function used by the heap two compare two heapnodes,
 *    these are just indexes into an array of records.  'v_iter' is
 *    the uniqiter_tempfiles_t object that holds the records and the
 *    fields describing the sort order.
 *
 *    Note the order of arguments is 'b', 'a'.
 */
static int
uniqIterCompHeapNodes(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_iter)
{
    uniqiter_tempfiles_t *iter = (uniqiter_tempfiles_t*)v_iter;

    return skFieldListCompareBuffers(iter->key[*(uint16_t*)a],
                                     iter->key[*(uint16_t*)b],
                                     iter->uniq->fi.key_fields);
}


/*
 *  count = uniqIterOpenAllTempFiles(iter);
 *
 *    Open all temporary files created while reading records,
 *    put the file handles in the 'fp' member of 'iter', and return
 *    the number of files opened.
 *
 *    If it is impossible to open all files due to a lack of file
 *    handles, the existing temporary files will be merged into new
 *    temporary files, and then another attempt will be made to open
 *    all files.
 *
 *    This function will only return when it is possible to return a
 *    file handle to every existing temporary file.  If it is unable
 *    to create a new temporary file, it returns -1.
 */
static int
uniqIterOpenAllTempFiles(
    uniqiter_tempfiles_t   *iter)
{
    uint16_t i;
    uint16_t *top_heap;
    uint16_t lowest;
    int j;
    int tmp_idx_a;
    int tmp_idx_b;
    int open_count;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];
    int heap_status;

    /* recall that uniq->temp_idx is the intermediate temp file; which
     * is open but unused when this function is called.  for this
     * function to be called, temp files #0 and #1 must be in use */
    assert(iter->uniq->temp_idx >= 2);
    assert(iter->uniq->temp_fp);

    /* index at which to start the merge */
    tmp_idx_a = 0;

    /* This loop repeats as long as we haven't opened all of the temp
     * files generated while reading the flows. */
    for (;;) {
        assert(skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap)
               == SKHEAP_ERR_EMPTY);

        /* index at which to stop the merge */
        if (iter->uniq->temp_idx - tmp_idx_a < MAX_MERGE_FILES) {
            /* fewer than MAX_MERGE_FILES files */
            tmp_idx_b = iter->uniq->temp_idx - 1;
        } else {
            tmp_idx_b = tmp_idx_a + MAX_MERGE_FILES - 1;
        }

        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR
                       ": Attempting to open temporary files #%d through #%d"),
                      tmp_idx_a, tmp_idx_b));

        /* number of files successfully opened */
        open_count = 0;

        /* Attempt to open up to MAX_MERGE_FILES, though an open may
         * due to lack of resources (EMFILE or ENOMEM) */
        for (j = tmp_idx_a; j <= tmp_idx_b; ++j) {
            iter->fps[open_count] = skTempFileOpen(iter->uniq->tmpctx, j);
            if (iter->fps[open_count] == NULL) {
                if ((open_count > 0)
                    && ((errno == EMFILE) || (errno == ENOMEM)))
                {
                    /* We cannot open any more temp files; we'll need
                     * to catch file 'j' the next time around. */
                    tmp_idx_b = j - 1;
                    UNIQUE_DEBUG(iter->uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR ": EMFILE limit hit"
                                   "---merging #%d through #%d into #%d"),
                                  tmp_idx_a, tmp_idx_b, iter->uniq->temp_idx));
                    break;
                } else {
                    iter->uniq->err_fn(("Error opening existing"
                                        " temporary file '%s': %s"),
                                       skTempFileGetName(iter->uniq->tmpctx, j),
                                       strerror(errno));
                    return -1;
                }
            }
            ++open_count;
        }

        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR ": Opened %d temporary files"),
                      open_count));

        /* Check to see if we've opened all temp files.  If so,
         * return */
        if (tmp_idx_b == iter->uniq->temp_idx - 1) {
            UNIQUE_DEBUG(iter->uniq,
                         (SKUNIQUE_DEBUG_ENVAR ": Successfully opened"
                          " all (remaining) temporary files"));
            return open_count;
        }
        /* Else, we could not open all temp files, so merge all opened
         * temp files into the intermediate file */

        /* Read the first key from each temp file into the work
         * buffer. */
        for (i = 0; i < open_count; ++i) {
            if (!fread(iter->key[i], iter->uniq->fi.key_octets,
                       1, iter->fps[i]))
            {
                TRACEMSG_READ(iter->uniq->fi.key_octets, iter->fps[i]);
                if (feof(iter->fps[i])) {
                    UNIQUE_DEBUG(iter->uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Ignoring empty temporary file '%s'"),
                                  skTempFileGetName(iter->uniq->tmpctx,
                                                    tmp_idx_a + i)));
                    continue;
                }
                iter->uniq->err_fn(("Error reading first key from"
                                    " temporary file '%s': %s"),
                                   skTempFileGetName(iter->uniq->tmpctx,
                                                     tmp_idx_a + i),
                                   strerror(errno));
                return -1;
            }
            /* add the index to the heap */
            skHeapInsert(iter->heap, &i);
        }

        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR
                       ": open_count: %d; read_count: %" PRIu32),
                      open_count, skHeapGetNumberEntries(iter->heap)));

        /* get the index of the file with the lowest key; which is at
         * the top of the heap */
        heap_status = skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap);
        if (SKHEAP_OK != heap_status) {
            iter->uniq->err_fn("Unexpected return value from heap %d",
                               heap_status);
            return -1;
        }
        lowest = *top_heap;

        /* exit this do...while() once all records for all opened
         * files have been read or until there is only one file
         * remaining */
        do {
            /* cache this low key and initialze values and distincts */
            memcpy(cached_key, iter->key[lowest], iter->uniq->fi.key_octets);
            skFieldListInitializeBuffer(iter->uniq->fi.value_fields,
                                        merged_values);
            if (iter->uniq->fi.distinct_num_fields) {
                if (uniqDistinctReset(&iter->uniq->fi, iter->distincts)) {
                    iter->uniq->err_fn("Error allocating table for"
                                       " distinct values");
                    return -1;
                }
            }

            /* loop over all files until we get a key that does not
             * match the cached_key */
            for (;;) {
                /* read data from the file and merge it into the
                 * values and distincts */
                if (uniqTempReadAndMerge(&iter->uniq->fi, iter->fps[lowest],
                                         merged_values, iter->distincts))
                {
                    UNIQUE_DEBUG(iter->uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": uniqTempReadAndMerge()"
                                   " failed for file #%u"), lowest));
                    iter->uniq->err_fn("Error merging values from"
                                       " temporary file");
                    return -1;
                }

                /* replace the node we just processed */
                if (fread(iter->key[lowest], iter->uniq->fi.key_octets,
                          1, (FILE*)iter->fps[lowest]))
                {
                    /* read succeeded. insert the new entry into the
                     * heap. */
                    skHeapReplaceTop(iter->heap, &lowest, NULL);
                } else {
                    /* read failed and no more data for this file;
                     * remove it from the heap */
                    TRACEMSG_READ(iter->uniq->fi.key_octets,iter->fps[lowest]);
                    UNIQUE_DEBUG(iter->uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Finished reading records from file #%u"),
                                  lowest));
                    skHeapExtractTop(iter->heap, NULL);
                }

                /* get the new value at the top of the heap and see if
                 * it matches the cached_key */
                heap_status = skHeapPeekTop(iter->heap,
                                            (skheapnode_t*)&top_heap);
                if (SKHEAP_OK != heap_status) {
                    /* heap is empty */
                    break;
                }
                lowest = *top_heap;

                if (skFieldListCompareBuffers(cached_key,
                                              iter->key[lowest],
                                              iter->uniq->fi.key_fields))
                {
                    /* keys differ */
                    break;
                }
                /* else keys are the same: add this record's values */
            }

            /* write the lowest key/value pair to the intermediate
             * temp file */
            if (uniqTempWrite(&iter->uniq->fi, iter->uniq->temp_fp,
                              cached_key, merged_values, iter->distincts))
            {
                iter->uniq->err_fn(("Error writing %s to"
                                    " temporary file '%s': %s"),
                                   ((iter->uniq->fi.distinct_num_fields > 0)
                                    ? "key/value/distinct triple"
                                    : "key/value pair"),
                                   skTempFileGetName(iter->uniq->tmpctx,
                                                     iter->uniq->temp_idx),
                                   strerror(errno));
                return -1;
            }

        } while (SKHEAP_OK == heap_status
                 && skHeapGetNumberEntries(iter->heap) > 1);

        TRACEMSG((("Handled all but one remaining file;"
                   " heap_status = %d; count = %" PRIu32),
                  heap_status, skHeapGetNumberEntries(iter->heap)));

        /* copy the records from the remaining file as blocks */
        if (SKHEAP_OK == heap_status) {
            uint8_t buf[4096];
            size_t num_recs;

            assert(skHeapGetNumberEntries(iter->heap) == 1);
            skHeapExtractTop(iter->heap, &lowest);

            /* write the key */
            if (!fwrite(iter->key[lowest], iter->uniq->fi.key_octets,
                        1, iter->uniq->temp_fp))
            {
                TRACEMSG_WRITE(iter->uniq->fi.key_octets);
                iter->uniq->err_fn(("Error writing key to"
                                    " temporary file '%s': %s"),
                                   skTempFileGetName(iter->uniq->tmpctx,
                                                         iter->uniq->temp_idx),
                                   strerror(errno));
                return -1;
            }

            /* read and write the rest of the file */
            while ((num_recs = fread(buf, 1, sizeof(buf), iter->fps[lowest]))
                   > 0)
            {
                if (fwrite(buf, 1, num_recs, iter->uniq->temp_fp) != num_recs) {
                    TRACEMSG_WRITE(num_recs);
                    iter->uniq->err_fn(("Error writing %" SK_PRIuZ
                                        " bytes to temporary file '%s': %s"),
                                       num_recs,
                                       skTempFileGetName(iter->uniq->tmpctx,
                                                         iter->uniq->temp_idx),
                                       strerror(errno));
                    return -1;
                }
            }
            if (!feof(iter->fps[lowest])) {
                iter->uniq->err_fn("Error reading from temporary file: %s",
                                   strerror(errno));
                return -1;
            }
            UNIQUE_DEBUG(iter->uniq,
                         ((SKUNIQUE_DEBUG_ENVAR
                           ": Finished reading records from file #%u"),
                          lowest));
        }

        assert(skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap)
               == SKHEAP_ERR_EMPTY);

        /* Close all the temp files that we processed this time. */
        for (i = 0; i < open_count; ++i) {
            fclose(iter->fps[i]);
        }
        /* Delete all the temp files that we opened */
        for (j = tmp_idx_a; j <= tmp_idx_b; ++j) {
            skTempFileRemove(iter->uniq->tmpctx, j);
        }

        /* Close the intermediate temp file. */
        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR ": Finished writing '%s'"),
                      skTempFileGetName(iter->uniq->tmpctx,
                                        iter->uniq->temp_idx)));
        if (EOF == fclose(iter->uniq->temp_fp)) {
            iter->uniq->err_fn("Error closing temporary file '%s': %s",
                               skTempFileGetName(iter->uniq->tmpctx,
                                                 iter->uniq->temp_idx),
                               strerror(errno));
            return -1;
        }

        /* Open a new intermediate temp file. */
        iter->uniq->temp_fp = skTempFileCreate(iter->uniq->tmpctx,
                                               &iter->uniq->temp_idx, NULL);
        if (iter->uniq->temp_fp == NULL) {
            iter->uniq->err_fn(("Error creating intermediate"
                                " temporary file: %s"),
                               strerror(errno));
            return -1;
        }

        /* Start the next merge with the next input temp file */
        tmp_idx_a = tmp_idx_b + 1;
    }

    return -1;    /* NOTREACHED */
}



/* **************************************************************** */

/*    SKUNIQUE USER API FOR HANDLING FILES OF PRESORTED INPUT */

/* **************************************************************** */


/* structure for binning records */
/* typedef struct sk_sort_unique_st sk_sort_unique_t; */
struct sk_sort_unique_st {
    sk_uniq_field_info_t    fi;

    int                   (*post_open_fn)(skstream_t *);
    int                   (*read_rec_fn)(skstream_t *, rwRec *);

    sk_msg_fn_t             err_fn;

    /* vector containing the names of files to process */
    sk_vector_t            *files;

    /* where to write temporary files */
    char                   *temp_dir;

    /* the temp file context */
    sk_tempfilectx_t       *tmpctx;

    /* the skstream_t or FILE* that are being read */
    void                   *fps[MAX_MERGE_FILES];

    /* array of records, one for each open file */
    rwRec                  *rec;

    /* memory to hold the key for each open file; this is allocated as
     * one large block; the 'key' member points into this buffer. */
    uint8_t                *key_data;

    /* array of keys, one for each open file, holds pointers into
     * 'key_data' */
    uint8_t               **key;

    /* maintains sorted keys */
    skheap_t               *heap;

    /* array holding information required to count distinct fields */
    distinct_value_t       *distincts;

    /* bitmap to keep track of files with same keys */
    sk_bitmap_t            *keys_match;

    /* current position in the 'files' vector */
    int                     files_position;

    /* flag to detect recursive calls to skPresortedUniqueProcess() */
    unsigned                processing : 1;

    /* whether to print debugging information */
    unsigned                print_debug:1;
};


/*
 *  status = sortuniqOpenNextInput(uniq, &stream);
 *
 *    Get the name of the next SiLK Flow record file to open, and set
 *    'stream' to that stream.
 *
 *    Return 0 on success.  Return 1 if no more files to open.  Return
 *    -2 if the file cannot be opened due to lack of memory or file
 *    handles.  Return -1 on other error.
 */
static int
sortuniqOpenNextInput(
    sk_sort_unique_t   *uniq,
    skstream_t        **out_stream)
{
    skstream_t *stream = NULL;
    const char *filename;
    int rv;

    do {
        rv = skVectorGetValue(&filename, uniq->files,
                              uniq->files_position);
        if (rv != 0) {
            /* no more files */
            return 1;
        }
        ++uniq->files_position;

        errno = 0;
        rv = skStreamOpenSilkFlow(&stream, filename, SK_IO_READ);
        if (rv) {
            if (errno == EMFILE || errno == ENOMEM) {
                rv = -2;
                /* decrement counter to try this file next time */
                --uniq->files_position;
                UNIQUE_DEBUG(uniq,
                             (SKUNIQUE_DEBUG_ENVAR ": Unable to open '%s': %s",
                              filename, strerror(errno)));
            } else {
                skStreamPrintLastErr(stream, rv, uniq->err_fn);
                rv = -1;
            }
            skStreamDestroy(&stream);
            return rv;
        }

        /* call the user's PostOpenFn if they provided one. */
        if (uniq->post_open_fn) {
            rv = uniq->post_open_fn(stream);
            if (rv == 1 || rv == -1) {
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Caller's post_open_fn returned %d"), rv));
                skStreamDestroy(&stream);
                return rv;
            }
            if (rv != 0) {
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Caller's post_open_fn returned %d"), rv));
                skStreamDestroy(&stream);
            }
        }
    } while (0 != rv);

    *out_stream = stream;
    return 0;
}


/*
 *  ok = sortuniqFillRecordAndKey(uniq, idx);
 *
 *    Read a record from a stream and compute the key for that record.
 *    The stream to read and the destinations for the record and key
 *    are determined by the index 'idx'.
 *
 *    Return 1 if a record was read; 0 otherwise.
 */
static int
sortuniqFillRecordAndKey(
    sk_sort_unique_t   *uniq,
    uint16_t            idx)
{
    int rv;

    rv = uniq->read_rec_fn((skstream_t*)uniq->fps[idx], &uniq->rec[idx]);
    if (rv) {
        if (rv != SKSTREAM_ERR_EOF) {
            skStreamPrintLastErr((skstream_t*)uniq->fps[idx], rv,
                                 uniq->err_fn);
        }
        return 0;
    }

    skFieldListRecToBinary(uniq->fi.key_fields, &uniq->rec[idx],
                           uniq->key[idx]);
    return 1;
}


static int
sortuniqCompHeapNodes(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_uniq)
{
    sk_sort_unique_t *uniq = (sk_sort_unique_t*)v_uniq;

    return skFieldListCompareBuffers(uniq->key[*(uint16_t*)a],
                                     uniq->key[*(uint16_t*)b],
                                     uniq->fi.key_fields);
}


/*  create an object to bin fields where the data is coming from files
 *  that have been sorted using the same keys as specified in the
 *  'uniq' object. */
int
skPresortedUniqueCreate(
    sk_sort_unique_t  **uniq)
{
    sk_sort_unique_t *u;
    const char *env_value;
    uint32_t debug_lvl;

    *uniq = NULL;

    u = (sk_sort_unique_t*)calloc(1, sizeof(sk_sort_unique_t));
    if (NULL == u) {
        return -1;
    }
    u->files = skVectorNew(sizeof(char*));
    if (NULL == u->files) {
        free(u);
        return -1;
    }

    u->read_rec_fn = &skStreamReadRecord;
    u->err_fn = &skMsgNone;

    env_value = getenv(SKUNIQUE_DEBUG_ENVAR);
    if (env_value && 0 == skStringParseUint32(&debug_lvl, env_value, 1, 0)) {
        u->print_debug = 1;
        u->err_fn = &skAppPrintErr;
    }

    *uniq = u;
    return 0;
}


/*  destroy the unique object */
void
skPresortedUniqueDestroy(
    sk_sort_unique_t  **uniq)
{
    sk_sort_unique_t *u;
    char *filename;
    size_t i;

    if (NULL == uniq || NULL == *uniq) {
        return;
    }

    u = *uniq;
    *uniq = NULL;

    skTempFileTeardown(&u->tmpctx);
    if (u->temp_dir) {
        free(u->temp_dir);
    }
    if (u->files) {
        for (i = 0; 0 == skVectorGetValue(&filename, u->files, i); ++i) {
            free(filename);
        }
        skVectorDestroy(u->files);
    }

    if (u->rec) {
        free(u->rec);
    }
    if (u->key) {
        free(u->key);
    }
    if (u->key_data) {
        free(u->key_data);
    }
    if (u->heap) {
        skHeapFree(u->heap);
    }
    if (u->distincts) {
        uniqDistinctFree(&u->fi, u->distincts);
    }

    free(u);
}


/*  tell the unique object to process the records in 'filename' */
int
skPresortedUniqueAddInputFile(
    sk_sort_unique_t   *uniq,
    const char         *filename)
{
    char *copy;

    assert(uniq);
    assert(filename);

    if (uniq->processing) {
        return -1;
    }

    copy = strdup(filename);
    if (NULL == copy) {
        return -1;
    }
    if (skVectorAppendValue(uniq->files, &copy)) {
        free(copy);
        return -1;
    }

    return 0;
}


/* set function used to report errors */
void
skPresortedUniqueSetErrorFunction(
    sk_sort_unique_t   *uniq,
    sk_msg_fn_t         err_fn)
{
    if (NULL == err_fn) {
        uniq->err_fn = skMsgNone;
    } else {
        uniq->err_fn = err_fn;
    }
}


/*  set the temporary directory used by 'uniq' to 'temp_dir' */
void
skPresortedUniqueSetTempDirectory(
    sk_sort_unique_t   *uniq,
    const char         *temp_dir)
{
    if (uniq->temp_dir) {
        free(uniq->temp_dir);
        uniq->temp_dir = NULL;
    }
    if (temp_dir) {
        uniq->temp_dir = strdup(temp_dir);
    }
}


/*  set a function that gets called when the 'uniq' object opens a
 *  file that was specified in skPresortedUniqueAddInputFile(). */
int
skPresortedUniqueSetPostOpenFn(
    sk_sort_unique_t   *uniq,
    int               (*stream_post_open)(skstream_t *))
{
    assert(uniq);

    if (uniq->processing) {
        return -1;
    }

    uniq->post_open_fn = stream_post_open;
    return 0;
}


/*  set a function to read a record from an input stream */
int
skPresortedUniqueSetReadFn(
    sk_sort_unique_t   *uniq,
    int               (*stream_read)(skstream_t *, rwRec *))
{
    assert(uniq);

    if (uniq->processing) {
        return -1;
    }

    if (NULL == stream_read) {
        uniq->read_rec_fn = &skStreamReadRecord;
    } else {
        uniq->read_rec_fn = stream_read;
    }
    return 0;
}


/*  set the key, distinct, and value fields for 'uniq' */
int
skPresortedUniqueSetFields(
    sk_sort_unique_t       *uniq,
    const sk_fieldlist_t   *key_fields,
    const sk_fieldlist_t   *distinct_fields,
    const sk_fieldlist_t   *agg_value_fields)
{
    assert(uniq);

    if (uniq->processing) {
        return -1;
    }

    memset(&uniq->fi, 0, sizeof(sk_uniq_field_info_t));
    uniq->fi.key_fields = key_fields;
    uniq->fi.value_fields = agg_value_fields;
    uniq->fi.distinct_fields = distinct_fields;

    return 0;
}


/*  set callback function that 'uniq' will call once it determines
 *  that a bin is complete */
int
skPresortedUniqueProcess(
    sk_sort_unique_t       *uniq,
    sk_unique_output_fn_t   output_fn,
    void                   *callback_data)
{
    uint16_t max_merge = MAX_MERGE_FILES;
    uint16_t *top_heap;
    uint16_t lowest;
    FILE *fp_intermediate;
    uint16_t tmp_idx_a;
    uint16_t tmp_idx_b;
    int temp_file_idx = -1;
    uint16_t open_count;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t distinct_buffer[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];
    uint16_t i;
    int no_more_inputs = 0;
    int heap_status;
    int rv = -1;

    assert(uniq);
    assert(output_fn);

    /* no recursive processing */
    if (uniq->processing) {
        return -1;
    }
    uniq->processing = 1;

    if (uniqCheckFields(&uniq->fi, uniq->err_fn)) {
        return -1;
    }

    if (skTempFileInitialize(&uniq->tmpctx, uniq->temp_dir,
                             NULL, uniq->err_fn))
    {
        return -1;
    }

    /* set up distinct fields */
    if (uniq->fi.distinct_num_fields) {
        if (uniqDistinctAlloc(&uniq->fi, &uniq->distincts)) {
            uniq->err_fn("Error allocating space for distinct counts");
            return -1;
        }
    }

    /* This outer loop is over the SiLK Flow input files and it
     * repeats as long as we haven't read all the records from all the
     * input files */
    do {
        /* open an intermediate temp file that we will use if there
         * are not enough file handles available to open all the input
         * files. */
        fp_intermediate = skTempFileCreate(uniq->tmpctx, &temp_file_idx, NULL);
        if (fp_intermediate == NULL) {
            uniq->err_fn("Error creating intermediate temporary file: %s",
                         strerror(errno));
            return -1;
        }

        /* Attempt to open up to max_merge files, though any open may
         * fail due to lack of resources (EMFILE or ENOMEM) */
        for (open_count = 0; open_count < max_merge; ++open_count) {
            rv = sortuniqOpenNextInput(uniq,
                                       (skstream_t**)&uniq->fps[open_count]);
            if (rv != 0) {
                break;
            }
        }
        switch (rv) {
          case 1:
            /* successfully opened all (remaining) input files */
            UNIQUE_DEBUG(uniq, (SKUNIQUE_DEBUG_ENVAR
                                ": Opened all (remaining) input files"));
            no_more_inputs = 1;
            if (temp_file_idx == 0) {
                /* we opened all the input files in a single pass.  we
                 * no longer need the intermediate temp file */
                fclose(fp_intermediate);
                fp_intermediate = NULL;
                temp_file_idx = -1;
            }
            break;
          case -1:
            /* unexpected error opening a file */
            return -1;
          case -2:
            /* ran out of memory or file descriptors */
            UNIQUE_DEBUG(uniq, (SKUNIQUE_DEBUG_ENVAR ": Unable to open all"
                                " inputs---out of memory or file handles"));
            break;
          case 0:
            if (open_count != max_merge) {
                /* no other way that rv == 0 */
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR ": rv == 0 but"
                               " open_count == %d; max_merge == %d. Abort"),
                              open_count, max_merge));
                skAbort();
            }
            /* ran out of pointers for this run */
            UNIQUE_DEBUG(uniq,
                         ((SKUNIQUE_DEBUG_ENVAR ": Unable to open all inputs"
                           "---max_merge (%d) limit reached"),
                          max_merge));
            break;
          default:
            /* unexpected error */
            UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                                 ": Got unexpected rv value = %d"), rv));
            skAbortBadCase(rv);
        }

        /* if this is the first iteration, allocate space for the
         * records and keys we will use while processing the files */
        if (NULL == uniq->rec) {
            uint8_t *n;
            max_merge = ((open_count > 0) ? open_count : 1);
            uniq->rec = (rwRec*)malloc(max_merge * sizeof(rwRec));
            if (NULL == uniq->rec) {
                uniq->err_fn("Error allocating space for %u records",
                             max_merge);
                return -1;
            }
            uniq->key_data = (uint8_t*)malloc(max_merge * uniq->fi.key_octets);
            if (NULL == uniq->key_data) {
                uniq->err_fn("Error allocating space for %u keys",
                             max_merge);
                return -1;
            }
            uniq->key = (uint8_t**)malloc(max_merge * sizeof(uint8_t*));
            if (NULL == uniq->key) {
                uniq->err_fn("Error allocating space for %u key pointers",
                             max_merge);
                return -1;
            }
            for (i = 0, n = uniq->key_data;
                 i < max_merge;
                 ++i, n += uniq->fi.key_octets)
            {
                uniq->key[i] = n;
            }
            uniq->heap = skHeapCreate2(sortuniqCompHeapNodes, max_merge,
                                       sizeof(uint16_t), NULL, uniq);
            if (NULL == uniq->heap) {
                uniq->err_fn("Error allocating space for %u heap entries",
                             max_merge);
                return -1;
            }
        }

        /* Read the first record from each file into the work buffer */
        for (i = 0; i < open_count; ++i) {
            if (sortuniqFillRecordAndKey(uniq, i)) {
                heap_status = skHeapInsert(uniq->heap, &i);
            }
        }

        UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                             ": open_count: %" PRIu16 "; read_count: %" PRIu32),
                            open_count, skHeapGetNumberEntries(uniq->heap)));

        /* get the index of the file with the lowest key; which is at
         * the top of the heap */
        heap_status = skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap);
        if (SKHEAP_OK == heap_status) {
            lowest = *top_heap;
        }

        /* exit this while() loop once all records for all opened
         * files have been read */
        while (SKHEAP_OK == heap_status) {
            /* cache this low key and initialze values and distincts */
            memcpy(cached_key, uniq->key[lowest], uniq->fi.key_octets);
            skFieldListInitializeBuffer(uniq->fi.value_fields, merged_values);
            if (uniq->fi.distinct_num_fields) {
                if (uniqDistinctReset(&uniq->fi, uniq->distincts)) {
                    uniq->err_fn("Error allocating table for distinct values");
                    return -1;
                }
            }

            /* loop over all files until we get a key that does not
             * match the cached_key */
            for (;;) {
                /* add the values and distincts */
                skFieldListAddRecToBuffer(uniq->fi.value_fields,
                                          &uniq->rec[lowest], merged_values);
                if (uniq->fi.distinct_num_fields) {
                    skFieldListRecToBinary(uniq->fi.distinct_fields,
                                           &uniq->rec[lowest],
                                           distinct_buffer);
                    if (uniqDistinctIncrement(&uniq->fi, uniq->distincts,
                                              distinct_buffer))
                    {
                        uniq->err_fn("Allocation error when incrementing"
                                     " distinct counts");
                        return -1;
                    }
                }

                /* replace the record we just processed */
                if (!sortuniqFillRecordAndKey(uniq, lowest)) {
                    /* read failed and no more data for this file;
                     * remove it from the heap */
                    UNIQUE_DEBUG(uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Finished reading records from file #%u"),
                                  lowest));
                    skHeapExtractTop(uniq->heap, NULL);

                } else if (skFieldListCompareBuffers(cached_key,
                                                     uniq->key[lowest],
                                                     uniq->fi.key_fields))
                {
                    /* read succeeded but keys differ. insert the new
                     * entry into the heap */
                    skHeapReplaceTop(uniq->heap, &lowest, NULL);
                } else {
                    /* read succeeded and keys are the same.  no need
                     * to insert it into heap, just add this record's
                     * value to our total */
                    continue;
                }

                /* get the new value at the top of the heap and see if
                 * it matches the cached_key */
                heap_status = skHeapPeekTop(uniq->heap,
                                            (skheapnode_t*)&top_heap);
                if (SKHEAP_OK != heap_status) {
                    /* heap is empty */
                    break;
                }
                lowest = *top_heap;

                if (skFieldListCompareBuffers(cached_key,
                                              uniq->key[lowest],
                                              uniq->fi.key_fields))
                {
                    /* keys differ */
                    break;
                }
                /* else keys are the same: add this record's values */
            }

            /* output this key and its values.  If we opened all input
             * files, call the output callback.  Else, write the key,
             * values, and distincts to a temp file.  The temp files
             * will be merged after all input files have been
             * processed. */
            if (fp_intermediate) {
                if (uniqTempWrite(&uniq->fi, fp_intermediate,
                                  cached_key, merged_values, uniq->distincts))
                {
                    uniq->err_fn(("Error writing merged %s"
                                  " to temporary file '%s': %s"),
                                 (uniq->fi.distinct_num_fields > 0
                                  ? "keys/values/distincts"
                                  : "keys/values"),
                                 skTempFileGetName(uniq->tmpctx,temp_file_idx),
                                 strerror(errno));
                    return -1;
                }
            } else {
                if (uniq->fi.distinct_num_fields) {
                    uniqDistinctSetOutputBuf(&uniq->fi, uniq->distincts,
                                             distinct_buffer);
                }
                rv = output_fn(cached_key, distinct_buffer, merged_values,
                               callback_data);
                if (rv != 0) {
                    UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                                         ": output_fn returned non-zero %d"),
                                        rv));
                    return -1;
                }
            }
        }

        /* Close the input files that we processed this time. */
        for (i = 0; i < open_count; ++i) {
            skStreamDestroy((skstream_t**)&uniq->fps[i]);
        }

        /* Close the intermediate temp file. */
        if (fp_intermediate) {
            UNIQUE_DEBUG(uniq,
                         ((SKUNIQUE_DEBUG_ENVAR ": Finished writing '%s'"),
                          skTempFileGetName(uniq->tmpctx, temp_file_idx)));
            if (EOF == fclose(fp_intermediate)) {
                uniq->err_fn("Error closing temporary file '%s': %s",
                             skTempFileGetName(uniq->tmpctx, temp_file_idx),
                             strerror(errno));
                return -1;
            }
            fp_intermediate = NULL;
        }

    } while (!no_more_inputs);

    /* we are finished processing records; free the 'rec' array */
    free(uniq->rec);
    uniq->rec = NULL;

    /* If any temporary files were written, we now have to merge them.
     * Otherwise, we didn't write any temporary files, and we are
     * done. */

    /* index at which to start the merge */
    tmp_idx_a = 0;

    /* This loop repeats as long as we haven't opened all of the temp
     * files generated while reading the flows. */
    while (tmp_idx_a < temp_file_idx) {
        /* index at which to stop the merge */
        if (temp_file_idx - tmp_idx_a < max_merge - 1) {
            /* number of temp files is less than max_merge files */
            tmp_idx_b = temp_file_idx;
        } else {
            /* must stop at max_merge */
            tmp_idx_b = tmp_idx_a + max_merge - 1;
        }

        UNIQUE_DEBUG(uniq,
                     ((SKUNIQUE_DEBUG_ENVAR
                       ": Attempting to open temporary files #%d through #%d"),
                      tmp_idx_a, tmp_idx_b));

        /* open an intermediate temp file.  The merge-sort will have
         * to write nodes here if there are not enough file handles
         * available to open all the temporary files we wrote while
         * reading the data. */
        fp_intermediate = skTempFileCreate(uniq->tmpctx, &temp_file_idx, NULL);
        if (fp_intermediate == NULL) {
            uniq->err_fn("Error creating intermediate temporary file: %s",
                         strerror(errno));
            return -1;
        }

        /* number of files successfully opened */
        open_count = 0;

        /* Attempt to open up to max_merge, though an open may fail
         * due to lack of resources (EMFILE or ENOMEM) */
        for (i = tmp_idx_a; i <= tmp_idx_b; ++i) {
            uniq->fps[open_count] = skTempFileOpen(uniq->tmpctx, i);
            if (uniq->fps[open_count] == NULL) {
                if ((open_count > 0)
                    && ((errno == EMFILE) || (errno == ENOMEM)))
                {
                    /* We cannot open any more temp files; we'll need
                     * to catch file 'i' the next time around. */
                    tmp_idx_b = i - 1;
                    UNIQUE_DEBUG(uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR ": EMFILE limit hit"
                                   "---merging #%d through #%d to #%d"),
                                  tmp_idx_a, tmp_idx_b, temp_file_idx));
                    break;
                } else {
                    uniq->err_fn(("Error opening existing"
                                  " temporary file '%s': %s"),
                                 skTempFileGetName(uniq->tmpctx, i),
                                 strerror(errno));
                    return -1;
                }
            }
            ++open_count;
        }

        UNIQUE_DEBUG(uniq,
                     ((SKUNIQUE_DEBUG_ENVAR ": Opened %d temporary files"),
                      open_count));

        /* Check to see if we've opened all temp files.  If so, close
         * the intermediate file. */
        if (tmp_idx_b == temp_file_idx - 1) {
            /* no longer need the intermediate temp file */
            UNIQUE_DEBUG(uniq,
                         (SKUNIQUE_DEBUG_ENVAR ": Successfully opened all"
                          " (remaining) temporary files"));
            fclose(fp_intermediate);
            fp_intermediate = NULL;
        }

        /* read the first key from each file into the work buffer */
        for (i = 0; i < open_count; ++i) {
            if (!fread(uniq->key[i], uniq->fi.key_octets,
                       1, (FILE*)uniq->fps[i]))
            {
                TRACEMSG_READ(uniq->fi.key_octets, (FILE*)uniq->fps[i]);
                if (feof((FILE*)uniq->fps[i])) {
                    UNIQUE_DEBUG(uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Ignoring empty temporary file '%s'"),
                                  skTempFileGetName(uniq->tmpctx,
                                                    tmp_idx_a + i)));
                    continue;
                }
                uniq->err_fn(("Error reading first key from"
                              " temporary file '%s'; %s"),
                             skTempFileGetName(uniq->tmpctx, tmp_idx_a + i),
                             strerror(errno));
                return -1;
            }
            skHeapInsert(uniq->heap, &i);
        }

        UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                             ": open_count: %" PRIu16 "; read_count: %" PRIu32),
                            open_count, skHeapGetNumberEntries(uniq->heap)));

        /* get the index of the file with the lowest key; which is at
         * the top of the heap */
        heap_status = skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap);
        if (SKHEAP_OK != heap_status) {
            uniq->err_fn("Unexpected return value from heap %d",
                         heap_status);
            return -1;
        }
        lowest = *top_heap;

        /* exit this do...while() once all records for all opened
         * files have been read */
        do {
            /* cache this low key, initialize the values and distincts */
            memcpy(cached_key, uniq->key[lowest], uniq->fi.key_octets);
            skFieldListInitializeBuffer(uniq->fi.value_fields, merged_values);
            if (uniq->fi.distinct_num_fields) {
                if (uniqDistinctReset(&uniq->fi, uniq->distincts)) {
                    uniq->err_fn("Error allocating table for distinct values");
                    return -1;
                }
            }

            /* loop over all files until we get a key that does not
             * match the cached_key */
            for (;;) {
                /* read data from the file and merge it into the
                 * values and distincts */
                if (uniqTempReadAndMerge(&uniq->fi, (FILE*)uniq->fps[lowest],
                                         merged_values, uniq->distincts))
                {
                    UNIQUE_DEBUG(uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": uniqTempReadAndMerge()"
                                   " failed for file #%u"), lowest));
                    uniq->err_fn("Error merging values from temporary file");
                    return -1;
                }

                /* replace the node we just processed */
                if (!fread(uniq->key[lowest], uniq->fi.key_octets,
                           1, (FILE*)uniq->fps[lowest]))
                {
                    /* read failed.  there is no more data for this
                     * file; remove it from the heap */
                    TRACEMSG_READ(uniq->fi.key_octets,
                                  (FILE*)uniq->fps[lowest]);
                    UNIQUE_DEBUG(uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Finished reading records from file #%u"),
                                  lowest));
                    skHeapExtractTop(uniq->heap, NULL);

                } else if (skFieldListCompareBuffers(cached_key,
                                                     uniq->key[lowest],
                                                     uniq->fi.key_fields))
                {
                    /* read succeeded but keys differ.  insert the new
                     * key into the heap. */
                    skHeapReplaceTop(uniq->heap, &lowest, NULL);

                } else {
                    /* read succeeded and keys are the same. merge the
                     * values.  FIXME: will this ever happen when
                     * merging files? */
                    continue;
                }

                /* get the new value at the top of the heap and see if
                 * it matches the cached_key */
                heap_status = skHeapPeekTop(uniq->heap,
                                            (skheapnode_t*)&top_heap);
                if (SKHEAP_OK != heap_status) {
                    /* heap is empty */
                    break;
                }
                lowest = *top_heap;

                if (skFieldListCompareBuffers(cached_key,
                                              uniq->key[lowest],
                                              uniq->fi.key_fields))
                {
                    /* keys differ */
                    break;
                }
                /* else keys are the same: add this record's values */
            }

            /* write the key, value, and distincts */
            if (fp_intermediate) {
                if (uniqTempWrite(&uniq->fi, fp_intermediate,
                                  cached_key, merged_values, uniq->distincts))
                {
                    uniq->err_fn(("Error writing merged %s"
                                  " to temporary file '%s': %s"),
                                 (uniq->fi.distinct_num_fields > 0
                                  ? "keys/values/distincts"
                                  : "keys/values"),
                                 skTempFileGetName(uniq->tmpctx,temp_file_idx),
                                 strerror(errno));
                    return -1;
                }
            } else {
                /* output this key and its values */
                if (uniq->fi.distinct_num_fields) {
                    uniqDistinctSetOutputBuf(&uniq->fi, uniq->distincts,
                                             distinct_buffer);
                }
                rv = output_fn(cached_key, distinct_buffer, merged_values,
                               callback_data);
                if (rv != 0) {
                    UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                                         ": output_fn returned non-zero %d"),
                                        rv));
                    return -1;
                }
            }
        } while (SKHEAP_OK == heap_status);

        /* Close all the temp files that we processed this time. */
        for (i = 0; i < open_count; ++i) {
            fclose((FILE*)uniq->fps[i]);
        }
        /* Delete all the temp files that we opened */
        for (i = tmp_idx_a; i <= tmp_idx_b; ++i) {
            skTempFileRemove(uniq->tmpctx, i);
        }

        /* Close the intermediate temp file. */
        if (fp_intermediate) {
            if (EOF == fclose(fp_intermediate)) {
                uniq->err_fn("Error closing temporary file '%s': %s",
                             skTempFileGetName(uniq->tmpctx, temp_file_idx),
                             strerror(errno));
                return -1;
            }
            fp_intermediate = NULL;
        }

        /* start the next merge with the next temp file */
        tmp_idx_a = tmp_idx_b + 1;
    }

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
