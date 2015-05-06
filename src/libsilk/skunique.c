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
**  skunique.c
**
**    This is an attempt to make the bulk of rwuniq into a stand-alone
**    library.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skunique.c 7801981cf8ef 2015-02-13 21:26:51Z mthomas $");

#include <silk/hashlib.h>
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


/*
 *    Return the name of the current temporary file used for output on
 *    the sk_unique_t object 'm_uniq'.
 */
#define UNIQUE_TMPNAME_OUT(m_uniq)                               \
    skTempFileGetName((m_uniq)->tmpctx, (m_uniq)->temp_idx)

/*
 *    Given the number 'm_idx' which is an index into the fps[] array
 *    on the structure 'sk_sort_unique_t', 'uniqiter_tempfiles_t', or
 *    'uniqiter_temp_nodist_t' pointed to by 'm_uniq', return the
 *    absolute number identifier for that temporary file.
 */
#define UNIQUE_TMPNUM_READ(m_uniq, m_idx)       \
    ((m_uniq)->temp_idx_base + (m_idx))



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
     * see.  DISTINCT_BITMAP is used for values up to 8bits;
     * DISTINCT_SHORTLIST is used for larger values where we have seen
     * no more than 32 distinct values.  Once the DISTINCT_SHORTLIST
     * is full, it is converted to a DISTINCT_HASHSET. */
    DISTINCT_BITMAP,
    DISTINCT_SHORTLIST,
    DISTINCT_HASHSET
} distinct_type_t;


typedef union distinct_tracker_un {
    sk_short_list_t    *dv_shortlist;
    HashSet            *dv_hashset;
    sk_bitmap_t        *dv_bitmap;
} distinct_tracker_t;

typedef struct distinct_value_st {
    uint64_t            dv_count;
    distinct_tracker_t  dv_v;
    distinct_type_t     dv_type;
    uint8_t             dv_octets;
    uint8_t             dv_offset;
} distinct_value_t;


static int
uniqDistinctShortlistCmp(
    const void         *field_buffer1,
    const void         *field_buffer2,
    void               *v_fieldlen)
{
    const uint8_t *len = (uint8_t*)v_fieldlen;

    return memcmp(field_buffer1, field_buffer2, *len);
}

/*
 *  uniqDistinctFree(field_info, distincts);
 *
 *    Free all memory that was allocated by uniqDistinctAlloc() or
 *    uniqDistinctAllocMerging().
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
          case DISTINCT_SHORTLIST:
            skShortListDestroy(&dist->dv_v.dv_shortlist);
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
 *  ok = uniqDistinctAllocMerging(field_info, &distincts);
 *
 *    Allocate the 'distincts' and initalize it with the length and
 *    offsets of each distinct field but do not create the data
 *    structures used to count them.  This function is used when
 *    merging the distinct counts from temporary files.
 *
 *    See also uniqDistinctAlloc().
 *
 *    Use uniqDistinctFree() to deallocate this data structure.
 *
 *    Return 0 on success, or -1 on failure.
 */
static int
uniqDistinctAllocMerging(
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
        TRACEMSG(("%s:%d: Error allocating distinct field_info",
                  __FILE__, __LINE__));
        return -1;
    }

    dist = distincts;

    /* determine how each field maps into the single buffer */
    skFieldListIteratorBind(field_info->distinct_fields, &fl_iter);
    while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
        dist->dv_octets = skFieldListEntryGetBinOctets(field);
        dist->dv_offset = total_octets;
        total_octets += dist->dv_octets;
        dist->dv_type = DISTINCT_BITMAP;

        ++dist;
        assert(dist <= distincts + field_info->distinct_num_fields);
    }
    assert(total_octets < HASHLIB_MAX_KEY_WIDTH);

    *new_distincts = distincts;
    return 0;
}


/*
 *  ok = uniqDistinctAlloc(field_info, &distincts);
 *
 *    Create the data structures required by 'field_info' to count
 *    distinct values and fill 'distincts' with the structures.
 *
 *    To allocate the distincts structure but not the data structures
 *    used for counting distinct items, use
 *    uniqDistinctAllocMerging().
 *
 *    Use uniqDistinctFree() to deallocate this data structure.
 *
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

    if (0 == field_info->distinct_num_fields) {
        return 0;
    }
    if (uniqDistinctAllocMerging(field_info, &distincts)) {
        return -1;
    }

    dist = distincts;

    /* create the data structures */
    skFieldListIteratorBind(field_info->distinct_fields, &fl_iter);
    while (NULL != (field = skFieldListIteratorNext(&fl_iter))) {
        if (dist->dv_octets == 1) {
            dist->dv_type = DISTINCT_BITMAP;
            if (skBitmapCreate(&dist->dv_v.dv_bitmap,
                               1 << (dist->dv_octets * CHAR_BIT)))
            {
                TRACEMSG(("%s:%d: Error allocating bitmap",
                          __FILE__, __LINE__));
                dist->dv_v.dv_bitmap = NULL;
                goto ERROR;
            }
        } else {
            dist->dv_type = DISTINCT_SHORTLIST;
            if (skShortListCreate(
                    &dist->dv_v.dv_shortlist, dist->dv_octets,
                    uniqDistinctShortlistCmp, (void*)&dist->dv_octets))
            {
                TRACEMSG(("%s:%d: Error allocating short list",
                          __FILE__, __LINE__));
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
        TRACEMSG(("%s:%d: Error allocating hashset", __FILE__, __LINE__));
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
            TRACEMSG(("%s:%d: Error inserting value into hashset",
                      __FILE__, __LINE__));
            goto ERROR;
        }
    }

    skShortListDestroy(&dist->dv_v.dv_shortlist);
    dist->dv_v.dv_hashset = hashset;
    dist->dv_type = DISTINCT_HASHSET;
    return 0;

  ERROR:
    hashset_free_table(hashset);
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
                    TRACEMSG(("%s:%d: Error inserting value into hashset",
                              __FILE__, __LINE__));
                    return -1;
                }
                break;
              default:
                skAbortBadCase(rv);
            }
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
                TRACEMSG(("%s:%d: Error inserting value into hashset",
                          __FILE__, __LINE__));
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
          case DISTINCT_SHORTLIST:
            skShortListRemoveAll(dist->dv_v.dv_shortlist);
            break;
          case DISTINCT_HASHSET:
            if (dist->dv_v.dv_hashset) {
                hashset_free_table(dist->dv_v.dv_hashset);
            }
            dist->dv_v.dv_hashset = hashset_create_set(dist->dv_octets, 256,
                                                       DEFAULT_LOAD_FACTOR);
            if (NULL == dist->dv_v.dv_hashset) {
                TRACEMSG(("%s:%d: Error allocating hashset",
                          __FILE__, __LINE__));
                return -1;
            }
            break;
        }
        dist->dv_count = 0;
    }
    return 0;
}


/* **************************************************************** */

/*    SKUNIQUE INTERNAL WRAPPERS FOR OPEN, READ AND WRITE OF TEMP FILES */

/* **************************************************************** */

/*
 *    Create and return a new temporary file, putting the index of the
 *    file in 'temp_idx'.  Exit the application on failure.
 */
static skstream_t *
uniqTempCreate(
    sk_tempfilectx_t   *tmpctx,
    int                *temp_idx)
{
    skstream_t *stream;

    stream = skTempFileCreateStream(tmpctx, temp_idx);
    if (NULL == stream) {
        skAppPrintSyserror("Error creating new temporary file");
        exit(EXIT_FAILURE);
    }
    return stream;
}

/*
 *    Re-open the existing temporary file indexed by 'temp_idx'.
 *    Return the new stream.  Return NULL if we could not open the
 *    stream due to out-of-memory or out-of-file-handles error.  Exit
 *    the application on any other error.
 */
static skstream_t *
uniqTempReopen(
    sk_tempfilectx_t   *tmpctx,
    int                 temp_idx)
{
    skstream_t *stream;

    stream = skTempFileOpenStream(tmpctx, temp_idx);
    if (NULL == stream) {
        if ((errno != EMFILE) && (errno != ENOMEM)) {
            skAppPrintSyserror(("Error opening existing temporary file '%s'"),
                               skTempFileGetName(tmpctx, temp_idx));
            exit(EXIT_FAILURE);
        }
    }
    return stream;
}

/*
 *    Close a temporary file.  Exit the application if stream was open
 *    for write and closing fails.
 */
static void
uniqTempClose(
    skstream_t         *stream)
{
    char errbuf[2 * PATH_MAX];
    ssize_t rv;

    rv = skStreamClose(stream);
    switch (rv) {
      case SKSTREAM_OK:
      case SKSTREAM_ERR_NOT_OPEN:
      case SKSTREAM_ERR_CLOSED:
        skStreamDestroy(&stream);
        return;
      case SKSTREAM_ERR_NULL_ARGUMENT:
        return;
    }

    skStreamLastErrMessage(stream, rv, errbuf, sizeof(errbuf));
    skAppPrintErr("Error closing temporary file: %s", errbuf);
    if (skStreamGetMode(stream) == SK_IO_WRITE) {
        exit(EXIT_FAILURE);
    }
    skStreamDestroy(&stream);
}

/*
 *    Read 'str_size' bytes from 'str_stream' into 'str_buf'.  Return
 *    'str_size' on success or 0 for other condition (end-of-file,
 *    short read, error).
 */
#define uniqTempRead(str_stream, str_buf, str_size)                     \
    uniqTempReadHelper(str_stream, str_buf, str_size, __FILE__, __LINE__)

static ssize_t
uniqTempReadHelper(
    skstream_t         *stream,
    void               *buf,
    size_t              size,
    const char         *file_name,
    int                 file_line)
{
    ssize_t rv;

    rv = skStreamRead(stream, buf, size);
    if (rv == (ssize_t)size) {
        return rv;
    }
#if TRACEMSG_LEVEL == 0
    (void)file_name;
    (void)file_line;
#else
    if (rv == 0) {
        TRACEMSG(("%s:%d: Failed to read %" SK_PRIuZ " bytes: EOF on '%s'",
                  file_name, file_line, size, skStreamGetPathname(stream)));
    } else if (rv > 0) {
        TRACEMSG(("%s:%d: Failed to read %" SK_PRIuZ " bytes:"
                  " Short read of %" SK_PRIdZ " on '%s'",
                  file_name, file_line, size, rv, skStreamGetPathname(stream)));
    } else {
        char errbuf[2 * PATH_MAX];

        skStreamLastErrMessage(stream, rv, errbuf, sizeof(errbuf));
        TRACEMSG(("%s:%d: Failed to read %" SK_PRIuZ " bytes: %s",
                  file_name, file_line, size, errbuf));
    }
#endif
    return 0;
}


/*
 *    Write 'stw_size' bytes from 'stw_buf' to 'stw_stream'.  Return
 *    'stw_size' on success and exit the appliation on error or short
 *    write.
 */
#define uniqTempWrite(stw_stream, stw_buf, stw_size)                    \
    uniqTempWriteHelper(stw_stream, stw_buf, stw_size, __FILE__, __LINE__)

static void
uniqTempWriteHelper(
    skstream_t         *stream,
    const void         *buf,
    size_t              size,
    const char         *file_name,
    int                 file_line)
{
    char errbuf[2 * PATH_MAX];
    ssize_t rv;

    rv = skStreamWrite(stream, buf, size);
    if (rv == (ssize_t)size) {
        return;
    }
    skStreamLastErrMessage(stream, rv, errbuf, sizeof(errbuf));

#if TRACEMSG_LEVEL == 0
    (void)file_name;
    (void)file_line;
#else
    if (rv >= 0) {
        TRACEMSG(("%s:%d: Failed to write %" SK_PRIuZ " bytes:"
                  " Short write of %" SK_PRIdZ " on '%s'",
                  file_name, file_line, size, rv, skStreamGetPathname(stream)));
    } else {
        TRACEMSG(("%s:%d: Failed to write %" SK_PRIuZ " bytes: %s",
                  file_name, file_line, size, errbuf));
    }
#endif

    if (rv >= 0) {
        snprintf(errbuf,sizeof(errbuf),
                 "Short write of %" SK_PRIdZ " bytes to '%s'",
                 rv, skStreamGetPathname(stream));
    }
    skAppPrintErr("Cannot write to temporary file: %s", errbuf);
    exit(EXIT_FAILURE);
}



/* **************************************************************** */

/*    SKUNIQUE INTERNAL SUPPORT USING TEMPORARY FILES */

/* **************************************************************** */

/*
 *  status = uniqTempWriteTriple(field_info, fp, key_buf, value_buf, distincts);
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
uniqTempWriteTriple(
    const sk_uniq_field_info_t *field_info,
    skstream_t                 *fp,
    skstream_t                 *dist_fp,
    const uint8_t              *key_buffer,
    const uint8_t              *value_buffer,
    const distinct_value_t     *dist)
{
    sk_bitmap_iter_t b_iter;
    hashset_iter h_iter;
    uint8_t *hash_key;
    uint16_t i;
    uint16_t j;
    uint32_t tmp32;
    uint8_t val8;

    /* write keys and values */
    uniqTempWrite(fp, key_buffer, field_info->key_octets);
    if (field_info->value_octets) {
        uniqTempWrite(fp, value_buffer, field_info->value_octets);
    }

    if (0 == field_info->distinct_num_fields) {
        return 0;
    }
    if (NULL == dist) {
        /* write a count of 0 for each distinct value */
        uint64_t count = 0;
        for (i = 0; i < field_info->distinct_num_fields; ++i) {
            uniqTempWrite(fp, &count, sizeof(uint64_t));
        }
        return 0;
    }

    /* handle all the distinct fields */
    for (i = 0; i < field_info->distinct_num_fields; ++i, ++dist) {
        /* write the count into the main file */
        uniqTempWrite(fp, &dist->dv_count, sizeof(uint64_t));
        /* write each value into the distinct file */
        switch (dist->dv_type) {
          case DISTINCT_BITMAP:
            assert(skBitmapGetHighCount(dist->dv_v.dv_bitmap)
                   == dist->dv_count);
            skBitmapIteratorBind(dist->dv_v.dv_bitmap, &b_iter);
            assert(1 == dist->dv_octets);
            while (SK_ITERATOR_OK == skBitmapIteratorNext(&b_iter, &tmp32)) {
                val8 = (uint8_t)tmp32;
                uniqTempWrite(dist_fp, &val8, sizeof(uint8_t));
            }
            break;

          case DISTINCT_SHORTLIST:
            assert(skShortListCountEntries(dist->dv_v.dv_shortlist)
                   == dist->dv_count);
            for (j = 0; j < dist->dv_count; ++j) {
                uniqTempWrite(
                    dist_fp, skShortListGetElement(dist->dv_v.dv_shortlist, j),
                    dist->dv_octets);
            }
            break;

          case DISTINCT_HASHSET:
            assert(hashset_count_entries(dist->dv_v.dv_hashset)
                   == dist->dv_count);
            hashset_sort_entries(dist->dv_v.dv_hashset);
            h_iter = hashset_create_iterator(dist->dv_v.dv_hashset);
            while (OK == hashset_iterate(dist->dv_v.dv_hashset,
                                         &h_iter, &hash_key))
            {
                uniqTempWrite(dist_fp, hash_key, dist->dv_octets);
            }
            break;
        }
    }

    return 0;
}


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

    /* pointer to the current intermediate temporary file; it's index
     * is given by the 'temp_idx' member */
    skstream_t             *temp_fp;

    /* when distinct fields are being computed, temporary files always
     * appear in pairs, and this is the pointer to an intermediate
     * temp file used to hold distinct values */
    skstream_t             *dist_fp;

    /* index of the intermediate temp file member 'temp_fp'. this is
     * one more than the temp file currently in use. */
    int                     temp_idx;

    /* index of highest used temporary file */
    int                     max_temp_idx;

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
    assert(0 == uniq->fi.distinct_num_fields || uniq->dist_fp);

    /* sort the hash entries using skFieldListCompareBuffers.  To sort
     * using memcmp(), we would need to ensure we use memcmp() when
     * reading/merging the values back out of the temp files. */
    hashlib_sort_entries_usercmp(uniq->ht,
                                 COMP_FUNC_CAST(skFieldListCompareBuffers),
                                 (void*)uniq->fi.key_fields);

    /* create an iterator for the hash table */
    ithash = hashlib_create_iterator(uniq->ht);

    /* iterate over the hash entries */
    if (0 == uniq->fi.distinct_num_fields) {
        UNIQUE_DEBUG(uniq,
                     ((SKUNIQUE_DEBUG_ENVAR ": Writing %u %s to '%s'"),
                      hashlib_count_entries(uniq->ht), "key/value pairs",
                      UNIQUE_TMPNAME_OUT(uniq)));

        while (hashlib_iterate(uniq->ht, &ithash, &hash_key, &hash_val)
               != ERR_NOMOREENTRIES)
        {
            if (uniqTempWriteTriple(&uniq->fi, uniq->temp_fp, NULL,
                                    hash_key, hash_val, NULL))
            {
                /* error writing, errno may or may not be set */
                uniq->err_fn(("Error writing key/value pair"
                              " to temporary file '%s': %s"),
                             UNIQUE_TMPNAME_OUT(uniq), strerror(errno));
                return -1;
            }
        }

    } else {
        UNIQUE_DEBUG(uniq,
                     ((SKUNIQUE_DEBUG_ENVAR ": Writing %u %s to '%s'"),
                      hashlib_count_entries(uniq->ht),
                      "key/value/distinct triples", UNIQUE_TMPNAME_OUT(uniq)));

        while (hashlib_iterate(uniq->ht, &ithash, &hash_key, &hash_val)
               != ERR_NOMOREENTRIES)
        {
            memcpy(&distincts, hash_val + uniq->fi.value_octets, sizeof(void*));
            if (uniqTempWriteTriple(&uniq->fi, uniq->temp_fp, uniq->dist_fp,
                                    hash_key, hash_val, distincts))
            {
                /* error writing, errno may or may not be set */
                uniq->err_fn(("Error writing key/value/distinct triple"
                              " to temporary file '%s': %s"),
                             UNIQUE_TMPNAME_OUT(uniq), strerror(errno));
                return -1;
            }
        }
    }

    /* close the temporary file(s) */
    uniqTempClose(uniq->temp_fp);
    uniq->temp_fp = NULL;
    if (uniq->dist_fp) {
        uniqTempClose(uniq->dist_fp);
        uniq->dist_fp = NULL;
    }

    /* success so far */
    UNIQUE_DEBUG(uniq, (SKUNIQUE_DEBUG_ENVAR ": Successfully wrote %s",
                        ((uniq->fi.distinct_num_fields > 0)
                         ? "key/value/distinct triples"
                         : "key/value pairs")));

    /* destroy the hash table */
    uniqueDestroyHashTable(uniq);

    /* open a new temporary file */
    uniq->temp_fp = uniqTempCreate(uniq->tmpctx, &uniq->max_temp_idx);
    if (NULL == uniq->temp_fp) {
        uniq->err_fn("Error creating temporary file: %s",
                     strerror(errno));
        return -1;
    }
    uniq->temp_idx = uniq->max_temp_idx;
    if (uniq->fi.distinct_num_fields) {
        uniq->dist_fp = uniqTempCreate(uniq->tmpctx, &uniq->max_temp_idx);
        if (NULL == uniq->dist_fp) {
            uniq->err_fn("Error creating temporary file: %s",
                         strerror(errno));
            return -1;
        }
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
    u->max_temp_idx = -1;
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
        uniqTempClose(u->temp_fp);
        u->temp_fp = NULL;
    }
    if (u->dist_fp) {
        uniqTempClose(u->dist_fp);
        u->dist_fp = NULL;
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
    uniq->temp_fp = uniqTempCreate(uniq->tmpctx, &uniq->max_temp_idx);
    if (NULL == uniq->temp_fp) {
        uniq->err_fn("Error creating intermediate temporary file: %s",
                     strerror(errno));
        return -1;
    }
    uniq->temp_idx = uniq->max_temp_idx;
    if (uniq->fi.distinct_num_fields) {
        uniq->dist_fp = uniqTempCreate(uniq->tmpctx, &uniq->max_temp_idx);
        if (NULL == uniq->dist_fp) {
            uniq->err_fn("Error creating temporary file: %s",
                         strerror(errno));
            return -1;
        }
    }

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
        TRACEMSG((("%s:%d: Memory error code is %" PRIu32),
                  __FILE__, __LINE__, memory_error));

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

/*
 *    There are two structures used when handling temporary files
 *    depending on whether there are distinct fields to handle.  the
 *    two structures have the same initial members can be cast to one
 *    another.
 *
 *    The callback function that implements skUniqueIteratorNext() is
 *    different depending on whether distinct fields are present.
 */

/* has support for distinct fields */
typedef struct uniqiter_tempfiles_st {
    /* function pointer used by skUniqueIteratorReset() */
    sk_uniqiter_reset_fn_t  reset_fn;

    /* function pointer used by skUniqueIteratorNext() */
    sk_uniqiter_next_fn_t   next_fn;

    /* function pointer used by skUniqueIteratorDestroy() */
    sk_uniqiter_free_fn_t   free_fn;

    /* the object to iterate over */
    sk_unique_t        *uniq;

    /* heap the stores the indexes used by the 'fps' and 'key'
     * members. comparison function uses the 'key' member of this
     * structure and returns the keys in ascending order */
    skheap_t           *heap;

    /* temporary files that are currently open; number of valid files
     * specified by the 'open_count' member */
    skstream_t         *fps[MAX_MERGE_FILES];

    /* for each file in 'fps', the file's current key */
    uint8_t             key[MAX_MERGE_FILES][HASHLIB_MAX_KEY_WIDTH];

    /* buffer used to hold the values the are returned to the caller
     * by skUniqueIteratorNext(). */
    uint8_t             returned_buf[HASH_MAX_NODE_BYTES];

    /* index of first temp file opened for this round of merging; used
     * when reporting temp file indexes */
    int                 temp_idx_base;

    /* number of open files: number of valid entries in fps[] */
    uint16_t            open_count;

    /* ENSURE EVERYTHING ABOVE HERE MATCHES uniqiter_temp_nodist_t */

    /* current distinct field; used by comparison function for the
     * 'dist_heap' member */
    const distinct_value_t *cur_dist;

    /* lengths and offsets of each distinct field */
    distinct_value_t   *distincts;

    /* heap that stores the indexes used by the 'distinct_value'
     * members.  comparison function uses the 'distinct_value' and
     * 'cur_dist' members and returns the distinct values in ascending
     * order. */
    skheap_t           *dist_heap;

    /* holds the current distinct value for files that share the same
     * key */
    uint8_t             distinct_value[MAX_MERGE_FILES][HASHLIB_MAX_KEY_WIDTH];

} uniqiter_tempfiles_t;

/* no support for distinct fields */
typedef struct uniqiter_temp_nodist_st {
    sk_uniqiter_reset_fn_t  reset_fn;
    sk_uniqiter_next_fn_t   next_fn;
    sk_uniqiter_free_fn_t   free_fn;
    sk_unique_t            *uniq;
    skheap_t               *heap;
    skstream_t             *fps[MAX_MERGE_FILES];
    uint8_t                 key[MAX_MERGE_FILES][HASHLIB_MAX_KEY_WIDTH];
    uint8_t                 returned_buf[HASH_MAX_NODE_BYTES];
    int                     temp_idx_base;
    uint16_t                open_count;
    /* ENSURE EVERYTHING ABOVE HERE MATCHES uniqiter_tempfiles_t */
} uniqiter_temp_nodist_t;

static int
uniqIterTempfilesHeapKeysCmp(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_iter);

static int
uniqIterTempfilesHeapDistCmp(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_iter);

static int
uniqIterTempfilesMergeOne(
    uniqiter_tempfiles_t   *iter,
    uint16_t                fps_index,
    unsigned int            write_to_temp,
    uint8_t                 merged_values[]);

static int
uniqIterTempfilesMergeValuesDist(
    uniqiter_tempfiles_t   *iter,
    const uint16_t          file_ids[],
    uint16_t                file_ids_len,
    unsigned int            write_to_temp,
    uint8_t                 merged_values[]);

static int
uniqIterTempfilesOpenAll(
    uniqiter_tempfiles_t   *iter);

/* Implements iter->next_fn when no distinct fields.*/
static int
uniqIterTempfilesNodistNext(
    sk_unique_iterator_t   *v_iter,
    uint8_t               **key_fields_buffer,
    uint8_t               **distinct_fields_buffer,
    uint8_t               **value_fields_buffer);


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
    uint16_t step;
    uint16_t j;

    /* NOTE: the idea of resetting this iterator is completely broken
     * since uniqIterTempfilesOpenAll() assumes the first temporary
     * file to process is #0, which will not be true if it has already
     * been called. */

    UNIQUE_DEBUG(iter->uniq, ((SKUNIQUE_DEBUG_ENVAR ": Resetting tempfiles"
                               " iterator; num files = %d"),
                              iter->open_count));

    /* if files are already open (e.g., caller has reset an active
     * iterator), we need to close them first */
    for (j = 0; j < iter->open_count; ++j) {
        if (iter->fps[j]) {
            uniqTempClose(iter->fps[j]);
            iter->fps[j] = NULL;
        }
    }

    /* open all temp files---this also merges temp files if there
     * are not enough file handles to open all temp files */
    iter->open_count = uniqIterTempfilesOpenAll(iter);
    if (iter->open_count == (uint16_t)-1) {
        return -1;
    }

    step = 1 + (iter->uniq->fi.distinct_num_fields > 0);

    /* Read the first key from each temp file into the 'key[]' array
     * on the iterator; add the file's index to the heap */
    for (j = 0; j < iter->open_count; j += step) {
        if (uniqTempRead(iter->fps[j], iter->key[j], iter->uniq->fi.key_octets))
        {
            skHeapInsert(iter->heap, &j);
        } else if (skStreamGetLastErrno(iter->fps[j])) {
            iter->uniq->err_fn("Cannot read first key from temporary file: %s",
                               strerror(skStreamGetLastErrno(iter->fps[j])));
            return -1;
        } else {
            UNIQUE_DEBUG(iter->uniq, (SKUNIQUE_DEBUG_ENVAR
                                      ": Ignoring empty temporary file #%u",
                                      j));
        }
    }

    if (skHeapGetNumberEntries(iter->heap) == 0) {
        iter->uniq->err_fn("Could not read records from any temporary files");
        return -1;
    }

    UNIQUE_DEBUG(iter->uniq,
                 ((SKUNIQUE_DEBUG_ENVAR
                   ": Iterator using %" PRIu32 " of %" PRIu32
                   " open temporary files"),
                  skHeapGetNumberEntries(iter->heap), iter->open_count));

    return 0;
}


/*
 *  status = uniqIterTempfilesNext(iter, &key, &distinct, &value);
 *
 *    Implementation for skUniqueIteratorNext() when using temporary
 *    files that do have distinct values.
 *
 *    See also uniqIterTempfilesNodistNext().
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
    uint16_t merge_nodes[MAX_MERGE_FILES];
    uint16_t merge_nodes_len;
    uint16_t i;
    int rv;

    assert(iter);

    /* should only be called when distinct fields are present */
    assert(iter->uniq->fi.distinct_num_fields > 0);

    /* get the index of the file with the lowest key; which is at
     * the top of the heap; cache this low key */
    if (SKHEAP_OK != skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap)) {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }
    lowest = *top_heap;
    memcpy(cached_key, iter->key[lowest], iter->uniq->fi.key_octets);

    /* short-cut the code when only one file remains */
    if (skHeapGetNumberEntries(iter->heap) == 1) {
        if (uniqIterTempfilesMergeOne(iter, lowest, 0, merged_values)) {
            skHeapEmpty(iter->heap);
            return SK_ITERATOR_NO_MORE_ENTRIES;
        }

        /* replace the key for the record we just processed */
        if (!uniqTempRead(iter->fps[lowest], iter->key[lowest],
                          iter->uniq->fi.key_octets))
        {
            /* read failed and no more data for this file; remove it
             * from the heap */
            UNIQUE_DEBUG(iter->uniq,
                         ((SKUNIQUE_DEBUG_ENVAR
                           ": Finished reading files #%u, #%u; 0 files remain"),
                          UNIQUE_TMPNUM_READ(iter, lowest),
                          UNIQUE_TMPNUM_READ(iter, lowest + 1)));
            skHeapExtractTop(iter->heap, NULL);
        }
        goto END;
    }

    /* store the id of each file whose current key matches the
     * cached_key into the merge_nodes[] array; remove the items
     * from heap as they are added to merge_nodes[] */
    merge_nodes[0] = lowest;
    merge_nodes_len = 1;
    skHeapExtractTop(iter->heap, NULL);

    while (skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap) == SKHEAP_OK
           && 0 == (skFieldListCompareBuffers(
                        cached_key, iter->key[*top_heap],
                        iter->uniq->fi.key_fields)))
    {
        merge_nodes[merge_nodes_len++] = *top_heap;
        skHeapExtractTop(iter->heap, NULL);
    }

    if (1 == merge_nodes_len) {
        /* nothing to merge when the key occurs in one file */
        rv = uniqIterTempfilesMergeOne(iter, lowest, 0, merged_values);
    } else {
        rv = uniqIterTempfilesMergeValuesDist(
            iter, merge_nodes, merge_nodes_len, 0, merged_values);
    }
    if (rv) {
        skHeapEmpty(iter->heap);
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }

    /* for each element in merge_nodes[], read its next key and insert
     * it into the heap containing the keys */
    for (i = 0; i < merge_nodes_len; ++i) {
        if (uniqTempRead(iter->fps[merge_nodes[i]], iter->key[merge_nodes[i]],
                         iter->uniq->fi.key_octets))
        {
            /* read succeeded. insert the new entry into the heap */
            skHeapInsert(iter->heap, &merge_nodes[i]);
            /* keys within a file should always be sorted; duplicate
             * keys may appear if the distinct data structure ran out
             * of memory */
            assert(skFieldListCompareBuffers(cached_key, iter->key[lowest],
                                             iter->uniq->fi.key_fields) <= 0);
        } else {
            /* read failed and no more data for this file */
            UNIQUE_DEBUG(iter->uniq,
                         ((SKUNIQUE_DEBUG_ENVAR
                           ": Finished reading files #%u, #%u;"
                           " %d files remain"),
                          UNIQUE_TMPNUM_READ(iter, merge_nodes[i]),
                          UNIQUE_TMPNUM_READ(iter, merge_nodes[i] + 1),
                          (2 * (skHeapGetNumberEntries(iter->heap)
                                + merge_nodes_len - i - 1))));
        }
    }

  END:
    /* set user's pointers to the buffers on the iterator, and write
     * the key, values, and distincts into those buffers */
    *key_fields_buffer = iter->returned_buf;
    memcpy(*key_fields_buffer, cached_key, iter->uniq->fi.key_octets);

    *value_fields_buffer = iter->returned_buf + iter->uniq->fi.key_octets;
    memcpy(*value_fields_buffer, merged_values, iter->uniq->fi.value_octets);

    *distinct_fields_buffer = (iter->returned_buf + iter->uniq->fi.key_octets
                               + iter->uniq->fi.value_octets);
    uniqDistinctSetOutputBuf(&iter->uniq->fi, iter->distincts,
                             *distinct_fields_buffer);

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
                uniqTempClose(iter->fps[i]);
                iter->fps[i] = NULL;
            }
        }

        if (iter->uniq->fi.distinct_num_fields) {
            uniqDistinctFree(&iter->uniq->fi, iter->distincts);
            skHeapFree(iter->dist_heap);
        }

        skHeapFree(iter->heap);

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

    assert(uniq);

    if (0 == uniq->fi.distinct_num_fields) {
        /* NOTE: Allocating smaller sized object and casting it to the
         * larger object. */
        iter = (uniqiter_tempfiles_t*)calloc(1,sizeof(uniqiter_temp_nodist_t));
    } else {
        iter = (uniqiter_tempfiles_t*)calloc(1, sizeof(uniqiter_tempfiles_t));
    }
    if (NULL == iter) {
        goto ERROR;
    }

    iter->heap = skHeapCreate2(uniqIterTempfilesHeapKeysCmp, MAX_MERGE_FILES,
                               sizeof(uint16_t), NULL, iter);
    if (NULL == iter->heap) {
        goto ERROR;
    }

    iter->uniq = uniq;
    iter->reset_fn = uniqIterTempfilesReset;
    iter->free_fn = uniqIterTempfilesDestroy;

    /* set up the handling of distinct field(s) */
    if (0 == uniq->fi.distinct_num_fields) {
        iter->next_fn = uniqIterTempfilesNodistNext;
    } else {
        iter->next_fn = uniqIterTempfilesNext;

        /* create the heap that operates over the distinct fields */
        iter->dist_heap = skHeapCreate2(uniqIterTempfilesHeapDistCmp,
                                        MAX_MERGE_FILES, sizeof(uint16_t),
                                        NULL, iter);
        if (NULL == iter->dist_heap) {
            goto ERROR;
        }
        if (uniqDistinctAllocMerging(&uniq->fi, &iter->distincts)) {
            goto ERROR;
        }
    }

    if (uniqIterTempfilesReset((sk_unique_iterator_t*)iter)) {
        uniqIterTempfilesDestroy((sk_unique_iterator_t**)&iter);
        return -1;
    }

    *new_iter = (sk_unique_iterator_t*)iter;
    return 0;

  ERROR:
    if (iter) {
        skHeapFree(iter->dist_heap);
        skHeapFree(iter->heap);
        free(iter);
    }
    uniq->err_fn("Error allocating unique iterator");
    return -1;
}


/*
 *    Helper function to merge the aggregate values from multiple
 *    temporary files into a single value for a single key.
 *
 *    This function may only be used when no distinct fields are
 *    present.
 *
 *    Fill 'cached_key' with the key for the file whose index is in
 *    'lowest'.
 *
 *    Initialize the buffer in 'merged_values' to hold the merging
 *    (e.g., the sum) of the aggregate value fields.
 *
 *    Across all the temporary files open on 'iter', read the values
 *    for keys that match 'cached_key' and add that value to
 *    'merged_values'.
 *
 *    Return 0 on success or -1 on read failure.
 */
static int
uniqIterTempfilesNodistMergeValues(
    uniqiter_temp_nodist_t *iter,
    uint16_t                lowest,
    uint8_t                *cached_key,
    uint8_t                *merged_values)
{
    uint8_t buf[4096];
    uint16_t *top_heap;
    uint32_t heap_count;
    int last_errno;

    /* should only be called with value fields and no distinct fields */
    assert(0 == iter->uniq->fi.distinct_num_fields);
    assert(0 < iter->uniq->fi.value_octets);

    /* lowest should be the file index at the top of the heap */
    assert(SKHEAP_OK == skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap));
    assert(lowest == *top_heap);

    heap_count = skHeapGetNumberEntries(iter->heap);

    memcpy(cached_key, iter->key[lowest], iter->uniq->fi.key_octets);

    skFieldListInitializeBuffer(iter->uniq->fi.value_fields, merged_values);

    /* repeat as long as the key of the item at top of the heap
     * matches the cached_key */
    do {
        /* read the value and merge it into the current value */
        if (!uniqTempRead(iter->fps[lowest], buf, iter->uniq->fi.value_octets))
        {
            last_errno = skStreamGetLastErrno(iter->fps[lowest]);
            UNIQUE_DEBUG(iter->uniq,
                         ((SKUNIQUE_DEBUG_ENVAR
                           ": Cannot read from temporary file #%u"),
                          UNIQUE_TMPNUM_READ(iter, lowest)));
            iter->uniq->err_fn(
                "Cannot read value field from temporary file: %s",
                (last_errno ? strerror(last_errno) :"EOF"));
            return -1;
        }
        skFieldListMergeBuffers(
            iter->uniq->fi.value_fields, merged_values, buf);

        /* replace the key for the value we just processed */
        if (uniqTempRead(iter->fps[lowest], iter->key[lowest],
                         iter->uniq->fi.key_octets))
        {
            /* read succeeded. insert the new entry into the
             * heap. */
            skHeapReplaceTop(iter->heap, &lowest, NULL);
            /* keys within a file should always be unique and sorted
             * when no distinct fields are present */
            assert(skFieldListCompareBuffers(cached_key, iter->key[lowest],
                                             iter->uniq->fi.key_fields) < 0);
        } else {
            /* read failed or no more data for this file; remove it
             * from the heap */
            UNIQUE_DEBUG(iter->uniq,
                         ((SKUNIQUE_DEBUG_ENVAR
                           ": Finished reading file #%u, %u files remain"),
                          UNIQUE_TMPNUM_READ(iter, lowest),
                          skHeapGetNumberEntries(iter->heap) - 1));
            skHeapExtractTop(iter->heap, NULL);
            --heap_count;
            if (0 == heap_count) {
                break;
            }
        }

        /* get the new value at the top of the heap; if its key
         * matches cached_key, add its values to merged_values. */
        skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap);
        lowest = *top_heap;

    } while (skFieldListCompareBuffers(
                 cached_key, iter->key[lowest], iter->uniq->fi.key_fields)==0);

    return 0;
}


/*
 *  status = uniqIterTempfilesNodistNext(iter, &key, &distinct, &value);
 *
 *    Implementation for skUniqueIteratorNext() when using temporary
 *    files that do not have distinct values.
 *
 *    See also uniqIterTempfilesNext().
 */
static int
uniqIterTempfilesNodistNext(
    sk_unique_iterator_t   *v_iter,
    uint8_t               **key_fields_buffer,
    uint8_t        UNUSED(**distinct_fields_buffer),
    uint8_t               **value_fields_buffer)
{
    uniqiter_temp_nodist_t *iter = (uniqiter_temp_nodist_t*)v_iter;
    uint16_t lowest;
    uint16_t *top_heap;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];

    assert(iter);

    /* should only be called with value fields and no distinct fields */
    assert(0 == iter->uniq->fi.distinct_num_fields);
    assert(iter->uniq->fi.value_octets > 0);

    /* get the index of the file with the lowest key; which is at
     * the top of the heap */
    if (SKHEAP_OK != skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap)) {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }
    lowest = *top_heap;

    if (uniqIterTempfilesNodistMergeValues(
            iter, lowest, cached_key, merged_values))
    {
        /* error reading from files */
        skHeapEmpty(iter->heap);
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }

    /* set user's pointers to the buffers on the iterator, and write
     * the key, values, and distincts into those buffers */
    *key_fields_buffer = iter->returned_buf;
    memcpy(*key_fields_buffer, cached_key, iter->uniq->fi.key_octets);

    *value_fields_buffer = iter->returned_buf + iter->uniq->fi.key_octets;
    memcpy(*value_fields_buffer,merged_values,iter->uniq->fi.value_octets);

    return SK_ITERATOR_OK;
}


/*
 *  status = uniqIterTempfilesHeapKeysCmp(b, a, v_iter);
 *
 *    Comparison callback function used by the heap.
 *
 *    The values in 'b' and 'a' are integer indexes into an array of
 *    keys.  The function calls the field comparison function to
 *    compare the keys and returns the result.
 *
 *    The context value in 'v_iter' is the uniqiter_tempfiles_t object
 *    that holds the keys and the fields describing the sort order.
 *
 *    Note the order of arguments is 'b', 'a'.
 */
static int
uniqIterTempfilesHeapKeysCmp(
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
 *    Comparison function used by the heap for distinct values.
 *
 *    Similar to uniqIterTempfilesHeapKeysCmp(), except the indexes
 *    are into a different array on the iterator.
 */
static int
uniqIterTempfilesHeapDistCmp(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_iter)
{
    uniqiter_tempfiles_t *iter = (uniqiter_tempfiles_t*)v_iter;
    const uint8_t *dist_a;
    const uint8_t *dist_b;

    assert(iter);
    assert(iter->cur_dist);

    dist_a = iter->distinct_value[*(uint16_t*)a] + iter->cur_dist->dv_offset;
    dist_b = iter->distinct_value[*(uint16_t*)b] + iter->cur_dist->dv_offset;

    return memcmp(dist_a, dist_b, iter->cur_dist->dv_octets);
}


/*
 *    Helper function to merge temporary files into a new temporary
 *    file.  May only be used when there are no distinct fields; when
 *    distinct fields are present, use uniqIterTempfilesMergeFiles().
 *
 *    Read the values from the 'open_count' temporary files stored on
 *    'iter', merge the values when the keys are identical, and store
 *    the result in the file whose index is 'temp_idx_offset'.
 *
 *    Return 0 on success or -1 on failure.
 */
static int
uniqIterTempfilesNodistMergeFiles(
    uniqiter_temp_nodist_t *iter)
{
    uint16_t *top_heap;
    uint16_t lowest;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];

    /* should only be called with value fields and no distinct fields */
    assert(0 == iter->uniq->fi.distinct_num_fields);
    assert(0 < iter->uniq->fi.value_octets);

    /* exit this while() loop once all records for all opened files
     * have been read or until there is only one file remaining */
    while (skHeapGetNumberEntries(iter->heap) > 1) {
        /* get the index of the file with the lowest key; which is at
         * the top of the heap */
        skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap);
        lowest = *top_heap;

        /* for all files that match the lowest key key, merge their
         * values into 'merged_values' */
        if (uniqIterTempfilesNodistMergeValues(
                iter, lowest, cached_key, merged_values))
        {
            return -1;
        }

        /* write the lowest key/value pair to the intermediate
         * temp file */
        uniqTempWrite(iter->uniq->temp_fp, cached_key,
                      iter->uniq->fi.key_octets);
        uniqTempWrite(iter->uniq->temp_fp, merged_values,
                      iter->uniq->fi.value_octets);
    }

    /* copy the data from the remaining file as blocks */
    if (skHeapExtractTop(iter->heap, (skheapnode_t*)&top_heap) == SKHEAP_OK) {
        uint8_t buf[4096];
        ssize_t rv;

        if (skHeapGetNumberEntries(iter->heap) != 0) {
            skAbort();
        }
        lowest = *top_heap;

        /* write the key that's in memory */
        uniqTempWrite(
            iter->uniq->temp_fp, iter->key[lowest], iter->uniq->fi.key_octets);

        /* inline the body of uniqTempRead() since that function does
         * not support partial reads */

        while ((rv = skStreamRead(iter->fps[lowest], buf, sizeof(buf))) > 0) {
            uniqTempWrite(iter->uniq->temp_fp, buf, rv);
        }
        if (-1 == rv) {
            char errbuf[2 * PATH_MAX];

            skStreamLastErrMessage(
                iter->fps[lowest], rv, errbuf, sizeof(errbuf));
            TRACEMSG(("%s:%d: Failed to read %" SK_PRIuZ " bytes: %s",
                      __FILE__, __LINE__, sizeof(buf), errbuf));
            iter->uniq->err_fn("Cannot read from temporary file: %s",
                               errbuf);
            return -1;
        }
        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR
                       ": Finished reading file #%u, 0 files remain"),
                      UNIQUE_TMPNUM_READ(iter, lowest)));
    }

    return 0;
}


/*
 *  status = uniqIterTempfilesMergeOne(iter, fps_index, write_to_temp, merged_values);
 *
 *    Read the values and distincts from the file in the iter->fps[]
 *    array at position 'fps_index'.
 *
 *    If 'write_to_temp' is non-zero, the value and distinct data is
 *    written to the current temporary file.  If 'write_to_temp' is
 *    zero, the value is stored in 'merged_values' and the distinct
 *    counts are stored on the 'distinct' member of the 'uniq' object.
 *
 *    Return 0 on success, or -1 on read or write error.
 */
static int
uniqIterTempfilesMergeOne(
    uniqiter_tempfiles_t   *iter,
    uint16_t                fps_index,
    unsigned int            write_to_temp,
    uint8_t                 merged_values[])
{
    distinct_value_t *dist;
    uint8_t buf[4096];
    size_t to_read;
    size_t exp_len;
    uint64_t dist_count;
    uint16_t i;
    int last_errno;

    /* Should only be called when distinct fields are present */
    assert(0 != iter->uniq->fi.distinct_num_fields);
    assert(!write_to_temp || (iter->uniq->temp_fp && iter->uniq->dist_fp));

    if (iter->uniq->fi.value_octets) {
        /* read the value */
        if (!uniqTempRead(iter->fps[fps_index], buf,
                          iter->uniq->fi.value_octets))
        {
            last_errno = skStreamGetLastErrno(iter->fps[fps_index]);
            iter->uniq->err_fn(
                "Cannot read value field from temporary file: %s",
                (last_errno ? strerror(last_errno) : "EOF"));
            return -1;
        }
        if (!write_to_temp) {
            /* store value in the 'merged_values[]' buffer */
            skFieldListInitializeBuffer(
                iter->uniq->fi.value_fields, merged_values);
            skFieldListMergeBuffers(
                iter->uniq->fi.value_fields, merged_values, buf);
        } else {
            uniqTempWrite(iter->uniq->temp_fp, buf,
                          iter->uniq->fi.value_octets);
        }
    }

    /* handle the distinct fields */
    for (i = 0; i < iter->uniq->fi.distinct_num_fields; ++i) {
        dist = &iter->distincts[i];
        /* read the number of distinct values */
        if (!uniqTempRead(iter->fps[fps_index], &dist_count, sizeof(uint64_t)))
        {
            last_errno = skStreamGetLastErrno(iter->fps[fps_index]);
            iter->uniq->err_fn(
                "Cannot read distinct count from temporary file: %s",
                (last_errno ? strerror(errno) : "EOF"));
            return -1;
        }

        if (write_to_temp) {
            uniqTempWrite(iter->uniq->temp_fp, &dist_count, sizeof(uint64_t));
        }

        /* determine the number of bytes to read */
        assert(dist->dv_octets > 0);
        to_read = dist->dv_octets * dist_count;

        if (!write_to_temp) {
            /* no need to read the data, just skip over it by using
             * 'NULL' as the buffer */
            if (!uniqTempRead(iter->fps[fps_index + 1], NULL, to_read)) {
                last_errno = skStreamGetLastErrno(iter->fps[fps_index + 1]);
                iter->uniq->err_fn(
                    "Cannot read distinct values from temporary file: %s",
                    (last_errno ? strerror(last_errno) : "EOF"));
                return -1;
            }
        } else {
            /* read and write the bytes */
            while (to_read) {
                exp_len = ((to_read < sizeof(buf)) ? to_read : sizeof(buf));
                if (!uniqTempRead(iter->fps[fps_index + 1], buf, exp_len)) {
                    last_errno = skStreamGetLastErrno(iter->fps[fps_index + 1]);
                    iter->uniq->err_fn(
                        "Cannot read distinct values from temporary file: %s",
                        (last_errno ? strerror(last_errno) : "EOF"));
                    return -1;
                }
                uniqTempWrite(iter->uniq->dist_fp, buf, exp_len);
                to_read -= exp_len;
            }
        }
        dist->dv_count = dist_count;
    }

    return 0;
}


/*
 *    Helper function used when merging files.
 *
 *    Process the set of file ids specified in 'file_ids', an array
 *    containing 'file_ids_len' entries.  These files are known to have the
 *    same key, and their read position is just after the key.
 *
 *    Read and merge the values and distinct fields from the files for
 *    this single key and write the result to the new temporary file.
 */
static int
uniqIterTempfilesMergeValuesDist(
    uniqiter_tempfiles_t   *iter,
    const uint16_t          file_ids[],
    uint16_t                file_ids_len,
    unsigned int            write_to_temp,
    uint8_t                 merged_values[])
{
    uint8_t buf[4096];
    uint8_t lowest_distinct[HASHLIB_MAX_KEY_WIDTH];
    uint64_t num_distinct[MAX_MERGE_FILES];
    uint64_t distinct_count;
    uint32_t heap_count;
    distinct_value_t *dist;
    uint16_t *top_heap;
    uint16_t lowest;
    uint16_t fps_index;
    uint16_t i;
    uint16_t j;
    int last_errno;

    /* Should only be called when distinct fields are present */
    assert(0 != iter->uniq->fi.distinct_num_fields);
    assert(!write_to_temp || (iter->uniq->temp_fp && iter->uniq->dist_fp));

    if (iter->uniq->fi.value_octets) {
        /* initialize the merged_values buffer.  for each file, read
         * the value and add it to the merged_values buffer. */

        /* initialize buffer */
        skFieldListInitializeBuffer(iter->uniq->fi.value_fields,merged_values);

        /* read and merge all values */
        for (j = 0; j < file_ids_len; ++j) {
            fps_index = file_ids[j];
            if (!uniqTempRead(iter->fps[fps_index], buf,
                              iter->uniq->fi.value_octets))
            {
                last_errno = skStreamGetLastErrno(iter->fps[fps_index]);
                UNIQUE_DEBUG(iter->uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Cannot read from temporary file #%u"),
                              UNIQUE_TMPNUM_READ(iter, fps_index)));
                iter->uniq->err_fn(
                    "Cannot read value field from temporary file: %s",
                    (last_errno ? strerror(last_errno) : "EOF"));
                return -1;
            }
            skFieldListMergeBuffers(
                iter->uniq->fi.value_fields, merged_values, buf);
        }

        if (write_to_temp) {
            /* write the merged value to the temporary file */
            uniqTempWrite(iter->uniq->temp_fp, merged_values,
                          iter->uniq->fi.value_octets);
        }
    }

    /* process each distinct field */
    for (i = 0; i < iter->uniq->fi.distinct_num_fields; ++i) {
        dist = &iter->distincts[i];
        iter->cur_dist = dist;

        /* holds the number of distinct values found for this distinct
         * field across all open files */
        distinct_count = 0;

        /* for each file: read the number of distinct entries, read
         * the first distinct value, and store the index in
         * the dist_heap using a comparitor over the distinct value */
        for (j = 0; j < file_ids_len; ++j) {
            fps_index = file_ids[j];
            if (!uniqTempRead(iter->fps[fps_index], &num_distinct[fps_index],
                              sizeof(uint64_t)))
            {
                last_errno = skStreamGetLastErrno(iter->fps[fps_index]);
                UNIQUE_DEBUG(iter->uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Cannot read from temporary file #%u"),
                              UNIQUE_TMPNUM_READ(iter, fps_index)));
                iter->uniq->err_fn(
                    "Cannot read distinct count from temporary file: %s",
                    (last_errno ? strerror(last_errno) : "EOF"));
                return -1;
            }

            if (num_distinct[fps_index]) {
                if (!uniqTempRead(iter->fps[fps_index + 1],
                                  iter->distinct_value[fps_index],
                                  dist->dv_octets))
                {
                    last_errno = skStreamGetLastErrno(iter->fps[fps_index + 1]);
                    UNIQUE_DEBUG(iter->uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Cannot read from temporary file #%u"),
                                  UNIQUE_TMPNUM_READ(iter, fps_index + 1)));
                    iter->uniq->err_fn(
                        "Cannot read distinct values from temporary file: %s",
                        (last_errno ? strerror(last_errno) : "EOF"));
                    return -1;
                }
                skHeapInsert(iter->dist_heap, &fps_index);
                --num_distinct[fps_index];
            }
        }

        heap_count = skHeapGetNumberEntries(iter->dist_heap);
        if (0 == heap_count) {
            /* strange, but okay? */
            dist->dv_count = distinct_count;
            continue;
        }

        /* get the file index that has the lowest distinct value */
        skHeapPeekTop(iter->dist_heap, (skheapnode_t*)&top_heap);
        lowest = *top_heap;

        /* process all the distinct values */
        do {
            /* FIXME: shortcut this while() when only a single file
             * remains */
            memcpy(lowest_distinct, iter->distinct_value[lowest],
                   dist->dv_octets);
            ++distinct_count;

            if (write_to_temp) {
                /* write the distinct value */
                uniqTempWrite(iter->uniq->dist_fp, &lowest_distinct,
                              dist->dv_octets);
            }

            /* ignore lowest_distinct in all other files */
            do {
                /* replace the distinct_value we just read */
                if (0 == num_distinct[lowest]) {
                    /* no more distinct values from this file; remove
                     * it from the heap */
                    skHeapExtractTop(iter->dist_heap, NULL);
                    --heap_count;
                    if (0 == heap_count) {
                        break;
                    }
                } else if (!uniqTempRead(iter->fps[lowest + 1],
                                         iter->distinct_value[lowest],
                                         dist->dv_octets))
                {
                    last_errno = skStreamGetLastErrno(iter->fps[lowest + 1]);
                    UNIQUE_DEBUG(iter->uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Cannot read from temporary file #%u"),
                                  UNIQUE_TMPNUM_READ(iter, lowest)));
                    iter->uniq->err_fn(
                        "Cannot read distinct values from temporary file: %s",
                        (last_errno ? strerror(last_errno) : "EOF"));
                    return -1;
                } else {
                    skHeapReplaceTop(iter->dist_heap, &lowest, NULL);
                    --num_distinct[lowest];
                    /* distinct values in each file must be sorted and
                     * unique */
                    assert(memcmp(lowest_distinct,iter->distinct_value[lowest],
                                  dist->dv_octets) < 0);
                }

                /* get the new value at the top of the heap and see if
                 * it matches the lowest_distinct */
                skHeapPeekTop(iter->dist_heap, (skheapnode_t*)&top_heap);
                lowest = *top_heap;
            } while (memcmp(lowest_distinct, iter->distinct_value[lowest],
                            dist->dv_octets) == 0);
        } while (heap_count > 0);

        dist->dv_count = distinct_count;
    }

    if (write_to_temp) {
        /* write the distinct count to the main temporary file */
        for (i = 0; i < iter->uniq->fi.distinct_num_fields; ++i) {
            dist = &iter->distincts[i];
            uniqTempWrite(iter->uniq->temp_fp, &dist->dv_count,
                          sizeof(dist->dv_count));
        }
    }

    return 0;
}


/*
 *    Helper function to merge temporary files into a new temporary
 *    file.  May only be used when there are distinct fields; when
 *    distinct fields not are present, use
 *    uniqIterTempfilesNodistMergeFiles().
 *
 *    Read the values (if any) and distinct fields from the
 *    'open_count' temporary files stored on 'iter', merge the values
 *    and the distinct fields when the keys are identical, and store
 *    the result in the file whose index is 'temp_idx_offset'.
 *
 *    Return 0 on success or -1 on failure.
 */
static int
uniqIterTempfilesMergeFiles(
    uniqiter_tempfiles_t   *iter)
{
    uint16_t *top_heap;
    uint16_t lowest;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];
    uint16_t merge_nodes[MAX_MERGE_FILES];
    uint16_t merge_nodes_len;
    uint16_t i;
    int rv;

    /* Should only be called when distinct fields are present */
    assert(0 != iter->uniq->fi.distinct_num_fields);
    assert(iter->uniq->temp_fp && iter->uniq->dist_fp);

    /* exit this while() loop once all data for all opened files have
     * been read or until there is only one file remaining */
    while (skHeapGetNumberEntries(iter->heap) > 1) {
        /* get the index of the file with the lowest key; which is at
         * the top of the heap */
        skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap);
        lowest = *top_heap;

        /* cache this low key and write it to the output file */
        memcpy(cached_key, iter->key[lowest], iter->uniq->fi.key_octets);
        uniqTempWrite(iter->uniq->temp_fp, cached_key,
                      iter->uniq->fi.key_octets);

        /* store the id of each file whose current key matches the
         * cached_key into the merge_nodes[] array; remove the items
         * from heap as they are added to merge_nodes[] */
        merge_nodes[0] = lowest;
        merge_nodes_len = 1;
        skHeapExtractTop(iter->heap, NULL);

        while (skHeapPeekTop(iter->heap, (skheapnode_t*)&top_heap) == SKHEAP_OK
               && 0 == (skFieldListCompareBuffers(
                            cached_key, iter->key[*top_heap],
                            iter->uniq->fi.key_fields)))
        {
            merge_nodes[merge_nodes_len++] = *top_heap;
            skHeapExtractTop(iter->heap, NULL);
        }

        if (1 == merge_nodes_len) {
            /* if the cached_key only appears in one file, there no need
             * to merge, just copy the bytes from source to the dest */
            rv = uniqIterTempfilesMergeOne(iter, lowest, 1, merged_values);
        } else {
            rv = uniqIterTempfilesMergeValuesDist(
                iter, merge_nodes, merge_nodes_len, 1, merged_values);
        }
        if (rv) {
            return -1;
        }

        /* for each element in merge_nodes[], read its next key and
         * insert it into the heap */
        for (i = 0; i < merge_nodes_len; ++i) {
            if (uniqTempRead(iter->fps[merge_nodes[i]],
                             iter->key[merge_nodes[i]],
                             iter->uniq->fi.key_octets))
            {
                /* read succeeded. insert the new entry into the
                 * heap. */
                skHeapInsert(iter->heap, &merge_nodes[i]);
                /* keys within a file should always be sorted;
                 * duplicate keys may appear if the distinct data
                 * structure ran out of memory */
                assert(skFieldListCompareBuffers(cached_key, iter->key[lowest],
                                                 iter->uniq->fi.key_fields)<=0);
            } else {
                UNIQUE_DEBUG(iter->uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Finished reading files #%u, #%u;"
                               " %d files remain"),
                              UNIQUE_TMPNUM_READ(iter, merge_nodes[i]),
                              UNIQUE_TMPNUM_READ(iter, merge_nodes[i] + 1),
                              (2 * (skHeapGetNumberEntries(iter->heap)
                                    + merge_nodes_len - i - 1))));
            }
        }
    }

    if (skHeapExtractTop(iter->heap, (skheapnode_t*)&top_heap) == SKHEAP_OK) {
        lowest = *top_heap;

        assert(0 == skHeapGetNumberEntries(iter->heap));

        do {
            /* write the key to the output file */
            uniqTempWrite(iter->uniq->temp_fp, iter->key[lowest],
                          iter->uniq->fi.key_octets);
            /* handle the values and distincts */
            if (uniqIterTempfilesMergeOne(iter, lowest, 1, merged_values)) {
                return -1;
            }
            /* read the next key */
        } while (uniqTempRead(iter->fps[lowest], iter->key[lowest],
                              iter->uniq->fi.key_octets));
        /* read failed and no more data for this file */
        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR
                       ": Finished reading files #%u, #%u; 0 files remain"),
                      UNIQUE_TMPNUM_READ(iter, lowest),
                      UNIQUE_TMPNUM_READ(iter, lowest + 1)));
    }

    return 0;
}


/*
 *  count = uniqIterTempfilesOpenAll(iter);
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
uniqIterTempfilesOpenAll(
    uniqiter_tempfiles_t   *iter)
{
    uint16_t i;
    uint16_t step;
    int j;
    int tmp_idx_a;
    int tmp_idx_b;
    int open_count;
    int last_errno;
    int rv;

    /* recall that uniq->temp_idx is the intermediate temp file; which
     * is open but unused when this function is called.  for this
     * function to be called, temp files #0 and #1 must be in use */
    assert(iter->uniq->temp_idx >= 2);
    assert(iter->uniq->temp_fp);
    assert(0 == iter->uniq->fi.distinct_num_fields || iter->uniq->dist_fp);

    /* index at which to start the merge */
    tmp_idx_a = 0;

    /* This loop repeats as long as we haven't opened all of the temp
     * files generated while reading the flows. */
    for (;;) {
        assert(skHeapGetNumberEntries(iter->heap) == 0);

        /* store the index of the first temporary file being processed
         * this time on the iterator */
        iter->temp_idx_base = tmp_idx_a;

        /* determine the index at which to stop the merge */
        if (iter->uniq->temp_idx - tmp_idx_a < MAX_MERGE_FILES) {
            /* fewer than MAX_MERGE_FILES files */
            tmp_idx_b = iter->uniq->temp_idx - 1;
        } else {
            tmp_idx_b = tmp_idx_a + MAX_MERGE_FILES - 1;
        }

        /* when using distinct fields, files must we opened in
         * pairs */
        if (iter->uniq->fi.distinct_num_fields) {
            assert((tmp_idx_a & 0x1) == 0);
            if (((tmp_idx_b & 0x1) == 0) && (tmp_idx_b > tmp_idx_a)) {
                --tmp_idx_b;
            }
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
            iter->fps[open_count] = uniqTempReopen(iter->uniq->tmpctx, j);
            if (iter->fps[open_count] == NULL) {
                if ((open_count > 0)
                    && ((errno == EMFILE) || (errno == ENOMEM)))
                {
                    /* We cannot open any more temp files; we'll need
                     * to catch file 'j' the next time around. */
                    tmp_idx_b = j - 1;
                    UNIQUE_DEBUG(iter->uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR ": EMFILE limit hit"
                                   "---merging #%d through #%d into #%d: %s"),
                                  tmp_idx_a, tmp_idx_b, iter->uniq->temp_idx,
                                 strerror(errno)));
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

        if (iter->uniq->fi.distinct_num_fields) {
            if ((open_count & 0x1) == 1) {
                /* number of opened files must be even */
                --tmp_idx_b;
                --open_count;
                uniqTempClose(iter->fps[open_count]);
            }
        }

        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR ": Opened %d temporary files"),
                      open_count));

        /* Check to see if we've opened all temp files.  If so,
         * return */
        if (tmp_idx_b == iter->uniq->temp_idx - 1) {
            UNIQUE_DEBUG(iter->uniq,
                         (SKUNIQUE_DEBUG_ENVAR
                          ": Successfully opened all%s temporary files",
                          ((tmp_idx_a > 0) ? " remaining" : "")));
            return open_count;
        }
        /* Else, we could not open all temp files, so merge all opened
         * temp files into the intermediate file */

        step = 1 + (iter->uniq->fi.distinct_num_fields > 0);

        /* Read the first key from each temp file into the 'key[]'
         * array on the iterator; add file's index to the heap */
        for (i = 0; i < open_count; i += step) {
            if (uniqTempRead(
                    iter->fps[i], iter->key[i], iter->uniq->fi.key_octets))
            {
                skHeapInsert(iter->heap, &i);
            } else {
                last_errno = skStreamGetLastErrno(iter->fps[i]);
                if (last_errno) {
                    iter->uniq->err_fn(
                        "Cannot read first key from temporary file '%s': %s",
                        skTempFileGetName(iter->uniq->tmpctx, tmp_idx_a + i),
                        strerror(last_errno));
                    return -1;
                }
                UNIQUE_DEBUG(iter->uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Ignoring empty temporary file '%s'"),
                              skTempFileGetName(iter->uniq->tmpctx,
                                                tmp_idx_a + i)));
            }
        }

        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR
                       ": Processing %" PRIu32 " of %" PRIu32
                       " open temporary files"),
                      skHeapGetNumberEntries(iter->heap), open_count));

        if (0 == iter->uniq->fi.distinct_num_fields) {
            rv = (uniqIterTempfilesNodistMergeFiles(
                      (uniqiter_temp_nodist_t*)iter));
        } else {
            rv = uniqIterTempfilesMergeFiles(iter);
        }
        if (rv) {
            return -1;
        }

        assert(skHeapGetNumberEntries(iter->heap) == 0);

        /* Close all the temp files that we processed this time. */
        for (i = 0; i < open_count; ++i) {
            uniqTempClose(iter->fps[i]);
        }
        /* Delete all the temp files that we opened */
        for (j = tmp_idx_a; j <= tmp_idx_b; ++j) {
            skTempFileRemove(iter->uniq->tmpctx, j);
        }

        /* Close the intermediate temp file. */
        UNIQUE_DEBUG(iter->uniq,
                     ((SKUNIQUE_DEBUG_ENVAR ": Finished writing '%s'"),
                      UNIQUE_TMPNAME_OUT(iter->uniq)));
        uniqTempClose(iter->uniq->temp_fp);

        /* Open a new intermediate temp file. */
        iter->uniq->temp_fp = uniqTempCreate(iter->uniq->tmpctx,
                                             &iter->uniq->max_temp_idx);
        if (iter->uniq->temp_fp == NULL) {
            iter->uniq->err_fn(("Error creating intermediate"
                                " temporary file: %s"),
                               strerror(errno));
            return -1;
        }
        iter->uniq->temp_idx = iter->uniq->max_temp_idx;
        if (iter->uniq->fi.distinct_num_fields) {
            iter->uniq->dist_fp = uniqTempCreate(iter->uniq->tmpctx,
                                                 &iter->uniq->max_temp_idx);
            if (NULL == iter->uniq->dist_fp) {
                iter->uniq->err_fn("Error creating temporary file: %s",
                                   strerror(errno));
                return -1;
            }
        }

        /* Start the next merge with the next input temp file */
        tmp_idx_a = tmp_idx_b + 1;
    }

    return -1;    /* NOTREACHED */
}


/****************************************************************
 * Public Interface for Iterating over the bins
 ***************************************************************/

/*  create iterator to get bins from the unique object; calls one of
 *  the helper functions above */
int
skUniqueIteratorCreate(
    sk_unique_t            *uniq,
    sk_unique_iterator_t  **new_iter)
{
    UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR ": Initializing iterator")));

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


#if 0
/*
 *    Destroy the interator.
 *
 *    Implemented as a macro in skunique.h that invokes the function
 *    pointer specified in the free_fn member of 'iter'.
 */
void
skUniqueIteratorDestroy(
    sk_unique_iterator_t  **iter);

/*
 *    Get the next set of values for the keys, values, and distinct
 *    counts.
 *
 *    Implemented as a macro in skunique.h that invokes the function
 *    pointer specified in the next_fn member of 'iter'.
 */
int
skUniqueIteratorNext(
    sk_unique_iterator_t   *iter,
    uint8_t               **key_fields_buffer,
    uint8_t               **distinct_fields_buffer,
    uint8_t               **agg_value_fields_buffer);

/*
 *    Reset the iterator so it can be used again.
 *
 *    Implemented as a macro in skunique.h that invokes the function
 *    pointer specified in the rest_fn member of 'iter'.
 */
int
skUniqueIteratorReset(
    sk_unique_iterator_t   *iter);
#endif  /* 0 */



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

    /* the skstream_t that are being read; there are SiLK Flow files
     * for the initial pass; if temporary files are created, this
     * array holds the temporary files during the file-merging */
    skstream_t             *fps[MAX_MERGE_FILES];

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

    /* the temp file context */
    sk_tempfilectx_t       *tmpctx;

    /* pointer to the current intermediate temp file; it's index is
     * given by the 'temp_idx' member */
    skstream_t             *temp_fp;

    /* when distinct fields are being computed, temporary files always
     * appear in pairs, and this is the pointer to an intermediate
     * temp file used to hold distinct values */
    skstream_t             *dist_fp;

    /* index of the intermediate temp file member 'temp_fp'. this is
     * one more than the temp file currently in use. */
    int                     temp_idx;

    /* index of highest used temporary file */
    int                     max_temp_idx;

    /* when merging temporary files, the index of the first temporary
     * file that is being merged */
    int                     temp_idx_base;

    /* current position in the 'files' vector */
    int                     files_position;

    /* current distinct field; used by comparison function for the
     * 'dist_heap' member */
    const distinct_value_t *cur_dist;

    /* heap that stores the indexes used by the 'distinct_value'
     * members.  comparison function uses the 'distinct_value' and
     * 'cur_dist' members and returns the distinct values in ascending
     * order. */
    skheap_t           *dist_heap;

    /* holds the current distinct value for files that share the same
     * key */
    uint8_t             distinct_value[MAX_MERGE_FILES][HASHLIB_MAX_KEY_WIDTH];

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

    rv = uniq->read_rec_fn(uniq->fps[idx], &uniq->rec[idx]);
    if (rv) {
        if (rv != SKSTREAM_ERR_EOF) {
            skStreamPrintLastErr(uniq->fps[idx], rv, uniq->err_fn);
        }
        return 0;
    }

    skFieldListRecToBinary(uniq->fi.key_fields, &uniq->rec[idx],
                           uniq->key[idx]);
    return 1;
}


/*
 *  status = sortuniqHeapKeysCmp(b, a, v_uniq);
 *
 *    Comparison callback function used by the heap when processing
 *    presorted-data.
 *
 *    The values in 'b' and 'a' are integer indexes into an array of
 *    keys.  The function calls the field comparison function to
 *    compare the keys and returns the result.
 *
 *    The context value in 'v_uniq' is the sk_sort_unique_t object
 *    that holds the keys and the fields describing the sort order.
 *
 *    Note the order of arguments is 'b', 'a'.
 */
static int
sortuniqHeapKeysCmp(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_uniq)
{
    sk_sort_unique_t *uniq = (sk_sort_unique_t*)v_uniq;

    return skFieldListCompareBuffers(uniq->key[*(uint16_t*)a],
                                     uniq->key[*(uint16_t*)b],
                                     uniq->fi.key_fields);
}

/*
 *    Comparison function used by the heap for distinct values.
 *
 *    Similar to sortuniqHeapKeysCmp(), except the indexes are into a
 *    different array on the sk_sort_unique_t object.
 */
static int
sortuniqHeapDistCmp(
    const skheapnode_t  b,
    const skheapnode_t  a,
    void               *v_uniq)
{
    sk_sort_unique_t *uniq = (sk_sort_unique_t*)v_uniq;
    const uint8_t *dist_a;
    const uint8_t *dist_b;

    assert(uniq);
    assert(uniq->cur_dist);

    dist_a = uniq->distinct_value[*(uint16_t*)a] + uniq->cur_dist->dv_offset;
    dist_b = uniq->distinct_value[*(uint16_t*)b] + uniq->cur_dist->dv_offset;

    return memcmp(dist_a, dist_b, uniq->cur_dist->dv_octets);
}


/*
 *    Helper function for skPresortedUniqueProcess() that process SiLK
 *    Flow input files files when distinct counts are not being
 *    computed.
 *
 *    Use the open SiLK streams in the fps[] array on the 'uniq'
 *    object and merge the values when the keys are identical.
 *
 *    If the 'output_fn' is not-NULL, send the key and merged values
 *    to that function.  When the 'output_fn' is NULL, the merged
 *    values are written to a temporary file.
 *
 *    This function returns once all the data in the open files has
 *    been read.
 *
 *    See also sortuniqReadSilkTotemp().
 */
static int
sortuniqReadSilkNodist(
    sk_sort_unique_t       *uniq,
    sk_unique_output_fn_t   output_fn,
    void                   *callback_data)
{
    uint16_t *top_heap;
    uint16_t lowest;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t distinct_buffer[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];
    uint32_t heap_count;
    int rv = -1;

    /* should only be called with value fields and no distinct fields */
    assert(0 == uniq->fi.distinct_num_fields);
    assert(0 < uniq->fi.value_octets);

    heap_count = skHeapGetNumberEntries(uniq->heap);
    if (0 == heap_count) {
        return 0;
    }

    /* get the index of the file with the lowest key; which is at
     * the top of the heap */
    skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap);
    lowest = *top_heap;

    /* exit this do...while() once all records for all opened SiLK
     * input files have been read */
    do {
        /* cache this low key and initialze values and distincts */
        memcpy(cached_key, uniq->key[lowest], uniq->fi.key_octets);
        skFieldListInitializeBuffer(uniq->fi.value_fields, merged_values);

        /* loop over all files until we get a key that does not
         * match the cached_key */
        do {
            /* add the values and distincts */
            skFieldListAddRecToBuffer(
                uniq->fi.value_fields, &uniq->rec[lowest], merged_values);

            /* replace the record we just processed */
            if (!sortuniqFillRecordAndKey(uniq, lowest)) {
                /* read failed and no more data for this file; remove
                 * the file's index from the heap; exit the loop if
                 * the heap is empty */
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Finished reading records from file #%u,"
                               " %u files remain"),
                              lowest, skHeapGetNumberEntries(uniq->heap) - 1));
                skHeapExtractTop(uniq->heap, NULL);
                --heap_count;
                if (0 == heap_count) {
                    break;
                }
            } else if (skFieldListCompareBuffers(
                           cached_key, uniq->key[lowest], uniq->fi.key_fields)
                       == 0)
            {
                /* FIXME: This comparison reduces work when the keys
                 * are the same, but it adds another comparison when
                 * the keys are different; is this an overall win or
                 * lose? */
                /* read succeeded and keys are the same.  no need to
                 * insert it into heap, just add this record's value
                 * to our total */
                continue;
            } else {
                /* read succeeded and keys differ. insert the new
                 * entry into the heap */
                skHeapReplaceTop(uniq->heap, &lowest, NULL);
            }

            /* get the new value at the top of the heap and see if it
             * matches the cached_key */
            skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap);
            lowest = *top_heap;
        } while (skFieldListCompareBuffers(
                     cached_key, uniq->key[lowest], uniq->fi.key_fields)
                 == 0);

        /* output this key and its values. */
        if (output_fn) {
            rv = output_fn(cached_key, distinct_buffer, merged_values,
                           callback_data);
            if (rv != 0) {
                UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                                     ": output_fn returned non-zero %d"),
                                    rv));
                return -1;
            }
        } else if (uniqTempWriteTriple(
                       &uniq->fi, uniq->temp_fp, NULL,
                       cached_key, merged_values, uniq->distincts))
        {
            uniq->err_fn(("Error writing merged keys/values"
                          " to temporary file '%s': %s"),
                         UNIQUE_TMPNAME_OUT(uniq), strerror(errno));
            return -1;
        }
    } while (heap_count > 0);

    return 0;
}


/*
 *    Helper function for skPresortedUniqueProcess() that process SiLK
 *    Flow input files files when distinct counts are being computed.
 *
 *    Use the open SiLK streams in the fps[] array on the 'uniq'
 *    object and merge the values and distinct counts when the keys
 *    are identical.
 *
 *    The merged values and distinct counts are written to temporary
 *    files.
 *
 *    This function returns once all the data in the open files has
 *    been read.
 *
 *    See also sortuniqReadSilkNodist().
 */
static int
sortuniqReadSilkTotemp(
    sk_sort_unique_t       *uniq)
{
    uint16_t *top_heap;
    uint16_t lowest;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t distinct_buffer[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];
    uint32_t heap_count;

    /* Should only be called when distinct fields are present */
    assert(0 != uniq->fi.distinct_num_fields);
    assert(uniq->temp_fp && uniq->dist_fp);

    heap_count = skHeapGetNumberEntries(uniq->heap);
    if (0 == heap_count) {
        /* probably should be an error */
        return 0;
    }

    /* get the index of the file with the lowest key; which is at
     * the top of the heap */
    skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap);
    lowest = *top_heap;

    /* exit this do...while() once all records for all opened files
     * have been read */
    do {
        /* cache this low key */
        memcpy(cached_key, uniq->key[lowest], uniq->fi.key_octets);

        /* reset the values and distincts */
        skFieldListInitializeBuffer(uniq->fi.value_fields, merged_values);
        if (uniqDistinctReset(&uniq->fi, uniq->distincts)) {
            uniq->err_fn("Error allocating table for distinct values");
            return -1;
        }

        /* loop over all SiLK input files until we get a key that does
         * not match the cached_key */
        do {
            /* add the distinct value to the data structure */
            skFieldListRecToBinary(
                uniq->fi.distinct_fields, &uniq->rec[lowest], distinct_buffer);
            if (uniqDistinctIncrement(
                    &uniq->fi, uniq->distincts, distinct_buffer))
            {
                /* increment failed; write the current values to disk
                 * and then reset the values and distincts */
                if (uniqTempWriteTriple(
                        &uniq->fi, uniq->temp_fp, uniq->dist_fp,
                        cached_key, merged_values, uniq->distincts))
                {
                    uniq->err_fn(("Error writing merged keys/values/distincts"
                                  " to temporary file '%s': %s"),
                                 UNIQUE_TMPNAME_OUT(uniq), strerror(errno));
                    return -1;
                }
                skFieldListInitializeBuffer(
                    uniq->fi.value_fields, merged_values);
                if (uniqDistinctReset(&uniq->fi, uniq->distincts)) {
                    uniq->err_fn("Error allocating table for distinct values");
                    return -1;
                }
            }

            /* add the value */
            skFieldListAddRecToBuffer(
                uniq->fi.value_fields, &uniq->rec[lowest], merged_values);

            /* replace the record we just processed */
            if (!sortuniqFillRecordAndKey(uniq, lowest)) {
                /* read failed and no more data for this file;
                 * remove it from the heap */
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Finished reading records from file #%u,"
                               " %" PRIu32 " files remain"),
                              lowest, heap_count - 1));
                skHeapExtractTop(uniq->heap, NULL);
                --heap_count;
                if (0 == heap_count) {
                    break;
                }
            } else if (skFieldListCompareBuffers(
                           cached_key, uniq->key[lowest], uniq->fi.key_fields)
                       == 0)
            {
                /* FIXME: This comparison reduces work when the keys
                 * are the same, but it adds another comparison when
                 * the keys are different; is this an overall win or
                 * lose? */
                /* read succeeded and keys are the same.  no need
                 * to insert it into heap, just add this record's
                 * value to our total */
                continue;
            } else {
                /* read succeeded and keys differ. insert the new
                 * entry into the heap and get the new value at
                 * the top of the heap */
                skHeapReplaceTop(uniq->heap, &lowest, NULL);
            }

            /* get the new index at top of heap and see if the file's
             * key matches the cached_key */
            skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap);
            lowest = *top_heap;
        } while (skFieldListCompareBuffers(
                     cached_key, uniq->key[lowest], uniq->fi.key_fields)
                 == 0);

        /* write the current values to the disk. */
        if (uniqTempWriteTriple(
                &uniq->fi, uniq->temp_fp, uniq->dist_fp,
                cached_key, merged_values, uniq->distincts))
        {
            uniq->err_fn(("Error writing merged  keys/values/distincts"
                          " to temporary file '%s': %s"),
                         UNIQUE_TMPNAME_OUT(uniq), strerror(errno));
            return -1;
        }
    } while (heap_count > 0);

    return 0;
}


/*
 *    Helper function for sortuniqMergeFilesDist() to get a value and
 *    distinct counts from a single file for a single unique key.
 *
 *    The key being merged is specified in 'cached_key'.
 *
 *    The index of the file to be read is specified in 'fps_index'.
 *    This is an index into the uniq->fps[] array.
 *
 *    If the 'output_fn' is not-NULL, send the key, merged values, and
 *    distinct counts to that function.  When the 'output_fn' is NULL,
 *    the merged values are written to another temporary file.
 *
 *    This function returns once entries for the 'cached_key' have
 *    been processed.
 *
 *    See also sortuniqMergeValuesDist().
 */
static int
sortuniqMergeSingleFile(
    sk_sort_unique_t       *uniq,
    const uint8_t           cached_key[],
    uint16_t                fps_index,
    sk_unique_output_fn_t   output_fn,
    void                   *callback_data)
{
    distinct_value_t *dist;
    uint8_t distinct_buffer[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];
    uint8_t buf[4096];
    size_t to_read;
    size_t exp_len;
    uint64_t dist_count;
    int last_errno;
    uint16_t i;
    ssize_t rv;

    /* Should only be called when distinct fields are present */
    assert(0 != uniq->fi.distinct_num_fields);
    assert(output_fn || (uniq->temp_fp && uniq->dist_fp));

    /* handle the values field */
    if (uniq->fi.value_octets) {
        if (!uniqTempRead(uniq->fps[fps_index], buf, uniq->fi.value_octets)) {
            last_errno = skStreamGetLastErrno(uniq->fps[fps_index]);
            uniq->err_fn("Cannot read value field from temporary file: %s",
                         (last_errno ? strerror(last_errno) : "EOF"));
            return -1;
        }
        if (output_fn) {
            skFieldListInitializeBuffer(uniq->fi.value_fields, merged_values);
            skFieldListMergeBuffers(uniq->fi.value_fields, merged_values, buf);
        } else {
            uniqTempWrite(uniq->temp_fp, buf, uniq->fi.value_octets);
        }
    }

    /* handle the distinct fields */
    for (i = 0; i < uniq->fi.distinct_num_fields; ++i) {
        dist = &uniq->distincts[i];
        /* handle the count of distinct entries */
        if (!uniqTempRead(uniq->fps[fps_index], &dist_count, sizeof(uint64_t)))
        {
            last_errno = skStreamGetLastErrno(uniq->fps[fps_index]);
            uniq->err_fn("Cannot read distinct count from temporary file: %s",
                         (last_errno ? strerror(last_errno) : "EOF"));
            return -1;
        }
        if (output_fn) {
            dist->dv_count = dist_count;
        } else {
            uniqTempWrite(uniq->temp_fp, &dist_count, sizeof(uint64_t));
        }

        /* determine the number of bytes to read */
        assert(dist->dv_octets > 0);
        to_read = dist->dv_octets * dist_count;

        if (output_fn) {
            /* no need to read the data, just skip over it by using
             * 'NULL' as the buffer */
            if (!uniqTempRead(uniq->fps[fps_index + 1], NULL, to_read)) {
                last_errno = skStreamGetLastErrno(uniq->fps[fps_index + 1]);
                uniq->err_fn(
                    "Cannot read distinct values from temporary file: %s",
                    (last_errno ? strerror(last_errno) : "EOF"));
                return -1;
            }
        } else {
            /* read and write the bytes */
            while (to_read) {
                exp_len = ((to_read < sizeof(buf)) ? to_read : sizeof(buf));
                if (!uniqTempRead(uniq->fps[fps_index + 1], buf, exp_len)) {
                    last_errno = skStreamGetLastErrno(uniq->fps[fps_index + 1]);
                    uniq->err_fn(
                        "Cannot read distinct values from temporary file: %s",
                        (last_errno ? strerror(last_errno) : "EOF"));
                    return -1;
                }
                uniqTempWrite(uniq->dist_fp, buf, exp_len);
                to_read -= exp_len;
            }
        }
    }

    if (output_fn) {
        uniqDistinctSetOutputBuf(&uniq->fi, uniq->distincts, distinct_buffer);
        rv = output_fn(cached_key, distinct_buffer, merged_values,
                       callback_data);
        if (rv != 0) {
            UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                                 ": output_fn returned non-zero %" SK_PRIdZ),
                                rv));
            return -1;
        }
    }

    return 0;
}


/*
 *    Helper function for sortuniqMergeFilesDist() to merge values and
 *    distinct counts from multiple input files for a single unique
 *    key.
 *
 *    The key being merged is specified in 'cached_key'.
 *
 *    The index of the files to be merged are specified in 'file_ids',
 *    an array of length 'file_ids_len'.  These values are indexes
 *    into the uniq->fps[] array.
 *
 *    If the 'output_fn' is not-NULL, send the key, merged values, and
 *    distinct counts to that function.  When the 'output_fn' is NULL,
 *    the merged values are written to another temporary file.
 *
 *    This function returns once entries for the 'cached_key' have
 *    been processed.
 *
 *    See also sortuniqMergeSingleFile().
 */
static int
sortuniqMergeValuesDist(
    sk_sort_unique_t       *uniq,
    const uint8_t           cached_key[],
    const uint16_t          file_ids[],
    uint16_t                file_ids_len,
    sk_unique_output_fn_t   output_fn,
    void                   *callback_data)
{
    uint8_t buf[4096];
    uint8_t distinct_buffer[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];
    uint8_t lowest_distinct[HASHLIB_MAX_KEY_WIDTH];
    uint64_t num_distinct[MAX_MERGE_FILES];
    uint64_t distinct_count;
    uint32_t heap_count;
    distinct_value_t *dist;
    uint16_t *top_heap;
    uint16_t lowest;
    uint16_t fps_index;
    uint16_t i;
    uint16_t j;
    int last_errno;
    int rv;

    /* Should only be called when distinct fields are present */
    assert(0 != uniq->fi.distinct_num_fields);
    assert(output_fn || (uniq->temp_fp && uniq->dist_fp));

    if (uniq->fi.value_octets) {
        /* initialize the merged_values buffer.  for each file, read
         * the value and add it to the merged_values buffer. */

        /* initialize the merge_values buffer */
        skFieldListInitializeBuffer(uniq->fi.value_fields, merged_values);

        /* read and merge all values */
        for (j = 0; j < file_ids_len; ++j) {
            fps_index = file_ids[j];
            if (!uniqTempRead(uniq->fps[fps_index], buf, uniq->fi.value_octets))
            {
                last_errno = skStreamGetLastErrno(uniq->fps[fps_index]);
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Cannot read from temporary file #%u"),
                              UNIQUE_TMPNUM_READ(uniq, fps_index)));
                uniq->err_fn("Cannot read values field from temporary file: %s",
                             (last_errno ? strerror(last_errno) : "EOF"));
                return -1;
            }
            skFieldListMergeBuffers(
                uniq->fi.value_fields, merged_values, buf);
        }

        if (!output_fn) {
            /* write the merged value to the temporary file */
            uniqTempWrite(uniq->temp_fp, merged_values, uniq->fi.value_octets);
        }
    }

    /* process each distinct field */
    for (i = 0; i < uniq->fi.distinct_num_fields; ++i) {
        dist = &uniq->distincts[i];
        uniq->cur_dist = dist;

        /* holds the number of distinct values found for this distinct
         * field across all open files */
        distinct_count = 0;

        /* for each file: read the number of distinct entries, read
         * the first distinct value, and store the index in
         * the dist_heap using a comparitor over the distinct value */
        for (j = 0; j < file_ids_len; ++j) {
            fps_index = file_ids[j];
            if (!uniqTempRead(uniq->fps[fps_index], &num_distinct[fps_index],
                              sizeof(uint64_t)))
            {
                last_errno = skStreamGetLastErrno(uniq->fps[fps_index]);
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Cannot read from temporary file #%u"),
                              UNIQUE_TMPNUM_READ(uniq, fps_index)));
                uniq->err_fn(
                    "Cannot read distinct count from temporary file: %s",
                    (last_errno ? strerror(last_errno) : "EOF"));
                return -1;
            }

            if (num_distinct[fps_index]) {
                if (!uniqTempRead(uniq->fps[fps_index + 1],
                                  uniq->distinct_value[fps_index],
                                  dist->dv_octets))
                {
                    last_errno = skStreamGetLastErrno(uniq->fps[fps_index + 1]);
                    UNIQUE_DEBUG(uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Cannot read from temporary file #%u"),
                                  UNIQUE_TMPNUM_READ(uniq, fps_index + 1)));
                    uniq->err_fn(
                        "Cannot read distinct values from temporary file: %s",
                        (last_errno ? strerror(last_errno) : "EOF"));
                    return -1;
                }
                skHeapInsert(uniq->dist_heap, &fps_index);
                --num_distinct[fps_index];
            }
        }

        heap_count = skHeapGetNumberEntries(uniq->dist_heap);
        if (0 == heap_count) {
            /* strange, but okay? */
            dist->dv_count = distinct_count;
            continue;
        }

        /* get the file index that has the lowest distinct value */
        skHeapPeekTop(uniq->dist_heap, (skheapnode_t*)&top_heap);
        lowest = *top_heap;

        /* process all the distinct values */
        do {
            /* FIXME: shortcut this while() when only a single file
             * remains */
            memcpy(lowest_distinct, uniq->distinct_value[lowest],
                   dist->dv_octets);
            ++distinct_count;

            if (!output_fn) {
                /* write the distinct value */
                uniqTempWrite(uniq->dist_fp, &lowest_distinct, dist->dv_octets);
            }

            /* ignore this lowest_distinct value in all other files */
            do {
                /* replace the distinct_value we just read */
                if (0 == num_distinct[lowest]) {
                    /* no more distinct values from this file; remove
                     * it from the heap */
                    skHeapExtractTop(uniq->dist_heap, NULL);
                    --heap_count;
                    if (0 == heap_count) {
                        break;
                    }
                } else if (!uniqTempRead(uniq->fps[lowest + 1],
                                         uniq->distinct_value[lowest],
                                         dist->dv_octets))
                {
                    last_errno = skStreamGetLastErrno(uniq->fps[lowest + 1]);
                    UNIQUE_DEBUG(uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR
                                   ": Cannot read from temporary file #%u"),
                                  UNIQUE_TMPNUM_READ(uniq, lowest + 1)));
                    uniq->err_fn(
                        "Cannot read distinct count from temporary file: %s",
                        (last_errno ? strerror(last_errno) : "EOF"));
                    return -1;
                } else {
                    skHeapReplaceTop(uniq->dist_heap, &lowest, NULL);
                    --num_distinct[lowest];
                    /* distinct values in each file must be sorted and
                     * unique */
                    assert(memcmp(lowest_distinct, uniq->distinct_value[lowest],
                                  dist->dv_octets) < 0);
                }

                /* get the new value at the top of the heap and see if
                 * it matches the lowest_distinct */
                skHeapPeekTop(uniq->dist_heap, (skheapnode_t*)&top_heap);
                lowest = *top_heap;
            } while (memcmp(lowest_distinct, uniq->distinct_value[lowest],
                            dist->dv_octets) == 0);
        } while (heap_count > 0);

        dist->dv_count = distinct_count;
    }

    if (!output_fn) {
        /* write the distinct count to the main temporary file */
        for (i = 0; i < uniq->fi.distinct_num_fields; ++i) {
            dist = &uniq->distincts[i];
            uniqTempWrite(uniq->temp_fp, &dist->dv_count,
                          sizeof(dist->dv_count));
        }
    } else {
        uniqDistinctSetOutputBuf(&uniq->fi, uniq->distincts, distinct_buffer);
        rv = output_fn(cached_key, distinct_buffer, merged_values,
                       callback_data);
        if (rv != 0) {
            UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                                 ": output_fn returned non-zero %d"),
                                rv));
            return -1;
        }
    }

    return 0;
}


/*
 *    Helper function for skPresortedUniqueProcess() that merges
 *    temporary files when distinct counts are being computed.
 *
 *    Use the open file handles in the fps[] array on the 'uniq'
 *    object and merge the values and distinct counts when the keys
 *    are identical.
 *
 *    If the 'output_fn' is not-NULL, send the key, merged values, and
 *    distinct counts to that function.  When the 'output_fn' is NULL,
 *    the merged values are written to another temporary file.
 *
 *    This function returns once all the data in the open files has
 *    been read.
 *
 *    See also sortuniqMergeFilesNodist().
 */
static int
sortuniqMergeFilesDist(
    sk_sort_unique_t       *uniq,
    sk_unique_output_fn_t   output_fn,
    void                   *callback_data)
{
    uint16_t *top_heap;
    uint16_t lowest;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint16_t merge_nodes[MAX_MERGE_FILES];
    uint16_t merge_nodes_len;
    uint16_t i;
    int rv = -1;

    /* Should only be called when distinct fields are present */
    assert(0 != uniq->fi.distinct_num_fields);
    assert(output_fn || (uniq->temp_fp && uniq->dist_fp));

    /* exit this do...while() once all records for all opened
     * files have been read */
    while (skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap) == SKHEAP_OK) {
        /* cache this low key */
        lowest = *top_heap;
        memcpy(cached_key, uniq->key[lowest], uniq->fi.key_octets);

        /* store the id of each file whose current key matches the
         * cached_key into the merge_nodes[] array; remove the items
         * from heap as they are added to merge_nodes[] */
        merge_nodes[0] = lowest;
        merge_nodes_len = 1;
        skHeapExtractTop(uniq->heap, NULL);

        while (skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap)
               == SKHEAP_OK
               && 0 == (skFieldListCompareBuffers(
                            cached_key, uniq->key[*top_heap],
                            uniq->fi.key_fields)))
        {
            merge_nodes[merge_nodes_len++] = *top_heap;
            skHeapExtractTop(uniq->heap, NULL);
        }

        if (!output_fn) {
            /* write the key to the temporary file */
            uniqTempWrite(uniq->temp_fp, cached_key, uniq->fi.key_octets);
        }

        if (1 == merge_nodes_len) {
            /* if the cached_key only appears in one file, there no need
             * to merge, just copy the bytes from source to the dest */
            rv = sortuniqMergeSingleFile(uniq, cached_key, lowest,
                                         output_fn, callback_data);
        } else {
            rv = sortuniqMergeValuesDist(uniq, cached_key,
                                         merge_nodes, merge_nodes_len,
                                         output_fn, callback_data);
        }
        if (rv) {
            return -1;
        }

        /* for each element in merge_nodes[], read its next key and
         * insert it into the heap */
        for (i = 0; i < merge_nodes_len; ++i) {
            if (uniqTempRead(uniq->fps[merge_nodes[i]],
                             uniq->key[merge_nodes[i]], uniq->fi.key_octets))
            {
                /* read succeeded. insert the new entry into the heap */
                skHeapInsert(uniq->heap, &merge_nodes[i]);
            } else {
                /* read failed and no more data for this file */
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Finished reading files #%u, #%u;"
                               " %d files remain"),
                              UNIQUE_TMPNUM_READ(uniq, merge_nodes[i]),
                              UNIQUE_TMPNUM_READ(uniq, merge_nodes[i] + 1),
                              (2 * (skHeapGetNumberEntries(uniq->heap)
                                    + merge_nodes_len - i - 1))));
            }
        }
    }

    return 0;
}


/*
 *    Helper function for skPresortedUniqueProcess() that merges
 *    temporary files when distinct counts are not being computed.
 *
 *    Use the open file handles in the fps[] array on the 'uniq'
 *    object and merge the values the keys are identical.
 *
 *    If the 'output_fn' is not-NULL, send the key and merged values
 *    to that function.  When the 'output_fn' is NULL, the merged
 *    values are written to another temporary file.
 *
 *    This function returns once all the data in the open files has
 *    been read.
 *
 *    See also sortuniqMergeFilesDist().
 */
static int
sortuniqMergeFilesNodist(
    sk_sort_unique_t       *uniq,
    sk_unique_output_fn_t   output_fn,
    void                   *callback_data)
{
    uint8_t buf[4096];
    uint16_t *top_heap;
    uint16_t lowest;
    uint8_t cached_key[HASHLIB_MAX_KEY_WIDTH];
    uint8_t distinct_buffer[HASHLIB_MAX_KEY_WIDTH];
    uint8_t merged_values[HASHLIB_MAX_VALUE_WIDTH];
    uint32_t heap_count;
    int last_errno;
    int rv = -1;

    /* should only be called with value fields and no distinct fields */
    assert(0 == uniq->fi.distinct_num_fields);
    assert(0 < uniq->fi.value_octets);
    assert(output_fn || uniq->temp_fp);

    heap_count = skHeapGetNumberEntries(uniq->heap);
    if (0 == heap_count) {
        return 0;
    }

    /* get the index of the file with the lowest key; which is at
     * the top of the heap */
    skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap);
    lowest = *top_heap;

    /* exit this do...while() once all records for all opened files
     * have been read */
    do {
        /* cache this low key, initialize the values */
        memcpy(cached_key, uniq->key[lowest], uniq->fi.key_octets);
        skFieldListInitializeBuffer(uniq->fi.value_fields, merged_values);

        /* loop over all files until we get a key that does not
         * match the cached_key */
        do {
            /* read the value from the file and merge it */
            if (!uniqTempRead(uniq->fps[lowest], buf, uniq->fi.value_octets)) {
                last_errno = skStreamGetLastErrno(uniq->fps[lowest]);
                UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                                     ": Cannot read from temporary file #%u"),
                                    UNIQUE_TMPNUM_READ(uniq, lowest)));
                uniq->err_fn("Cannot read value field from temporary file: %s",
                             (last_errno ? strerror(last_errno) : "EOF"));
                return -1;
            }
            skFieldListMergeBuffers(
                uniq->fi.value_fields, merged_values, buf);

            /* replace the key for the value we just processed */
            if (uniqTempRead(uniq->fps[lowest], uniq->key[lowest],
                             uniq->fi.key_octets))
            {
                /* insert the new key into the heap; get the new
                 * lowest key */
                skHeapReplaceTop(uniq->heap, &lowest, NULL);
            } else {
                /* read failed.  there is no more data for this file;
                 * remove it from the heap; get the new top of the
                 * heap, or end the while if no more files */
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Finished reading records from file #%u,"
                               " %" PRIu32 " files remain"),
                              UNIQUE_TMPNUM_READ(uniq, lowest), heap_count-1));
                skHeapExtractTop(uniq->heap, NULL);
                --heap_count;
                if (0 == heap_count) {
                    break;
                }
            }
            skHeapPeekTop(uniq->heap, (skheapnode_t*)&top_heap);
            lowest = *top_heap;

        } while (skFieldListCompareBuffers(
                     cached_key, uniq->key[lowest], uniq->fi.key_fields)
                 == 0);

        /* write the key and value */
        if (output_fn) {
            rv = output_fn(cached_key, distinct_buffer, merged_values,
                           callback_data);
            if (rv != 0) {
                UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                                     ": output_fn returned non-zero %d"),
                                    rv));
                return -1;
            }
        } else if (uniqTempWriteTriple(
                       &uniq->fi, uniq->temp_fp, NULL,
                       cached_key, merged_values, uniq->distincts))
        {
            uniq->err_fn(("Error writing merged key/values"
                          " to temporary file '%s': %s"),
                         UNIQUE_TMPNAME_OUT(uniq), strerror(errno));
            return -1;
        }
    } while (heap_count > 0);

    return 0;
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

    u->temp_idx = -1;
    u->max_temp_idx = -1;

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

    if (u->temp_fp) {
        uniqTempClose(u->temp_fp);
        u->temp_fp = NULL;
    }
    if (u->dist_fp) {
        uniqTempClose(u->dist_fp);
        u->dist_fp = NULL;
    }
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
    if (u->dist_heap) {
        skHeapFree(u->dist_heap);
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
    uint16_t tmp_idx_a;
    uint16_t tmp_idx_b;
    uint16_t open_count;
    uint16_t i;
    uint16_t step;
    int no_more_inputs = 0;
    int opened_all_temps;
    int last_errno;
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

    if (skTempFileInitialize(&uniq->tmpctx, uniq->temp_dir, NULL,uniq->err_fn))
    {
        return -1;
    }

    /* set up distinct fields */
    if (uniq->fi.distinct_num_fields) {
        if (uniqDistinctAlloc(&uniq->fi, &uniq->distincts)) {
            uniq->err_fn("Error allocating space for distinct counts");
            return -1;
        }

        /* create the heap that operates over the distinct fields */
        uniq->dist_heap = skHeapCreate2(sortuniqHeapDistCmp,
                                        MAX_MERGE_FILES, sizeof(uint16_t),
                                        NULL, uniq);
        if (NULL == uniq->dist_heap) {
            uniq->err_fn("Error allocating distinct heap");
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
        uniq->temp_fp = uniqTempCreate(uniq->tmpctx, &uniq->max_temp_idx);
        if (uniq->temp_fp == NULL) {
            uniq->err_fn("Error creating intermediate temporary file: %s",
                         strerror(errno));
            return -1;
        }
        uniq->temp_idx = uniq->max_temp_idx;
        if (uniq->fi.distinct_num_fields) {
            uniq->dist_fp = uniqTempCreate(uniq->tmpctx, &uniq->max_temp_idx);
            if (uniq->dist_fp == NULL) {
                uniq->err_fn("Error creating intermediate temporary file: %s",
                             strerror(errno));
                return -1;
            }
        }

        /* Attempt to open up to MAX_MERGE_FILES SILK input files,
         * though any open may fail due to lack of resources (EMFILE
         * or ENOMEM) */
        for (open_count = 0; open_count < MAX_MERGE_FILES; ++open_count) {
            rv = sortuniqOpenNextInput(uniq, &uniq->fps[open_count]);
            if (rv != 0) {
                break;
            }
        }
        switch (rv) {
          case 1:
            /* successfully opened all (remaining) input files */
            UNIQUE_DEBUG(uniq, (SKUNIQUE_DEBUG_ENVAR
                                ": Opened all%s input files",
                                (uniq->rec ? " remaining" : "")));
            no_more_inputs = 1;
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
            if (open_count != MAX_MERGE_FILES) {
                /* no other way that rv == 0 */
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR ": rv == 0 but"
                               " open_count == %d; max_merge == %d. Abort"),
                              open_count, MAX_MERGE_FILES));
                skAbort();
            }
            /* ran out of pointers for this run */
            UNIQUE_DEBUG(uniq,
                         ((SKUNIQUE_DEBUG_ENVAR ": Unable to open all inputs"
                           "---max_merge (%d) limit reached"),
                          MAX_MERGE_FILES));
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
            uniq->rec = (rwRec*)malloc(MAX_MERGE_FILES * sizeof(rwRec));
            if (NULL == uniq->rec) {
                uniq->err_fn("Error allocating space for %u records",
                             MAX_MERGE_FILES);
                return -1;
            }
            uniq->key_data = ((uint8_t*)
                              malloc(MAX_MERGE_FILES * uniq->fi.key_octets));
            if (NULL == uniq->key_data) {
                uniq->err_fn("Error allocating space for %u keys",
                             MAX_MERGE_FILES);
                return -1;
            }
            uniq->key = (uint8_t**)malloc(MAX_MERGE_FILES * sizeof(uint8_t*));
            if (NULL == uniq->key) {
                uniq->err_fn("Error allocating space for %u key pointers",
                             MAX_MERGE_FILES);
                return -1;
            }
            for (i = 0, n = uniq->key_data;
                 i < MAX_MERGE_FILES;
                 ++i, n += uniq->fi.key_octets)
            {
                uniq->key[i] = n;
            }
            uniq->heap = skHeapCreate2(sortuniqHeapKeysCmp, MAX_MERGE_FILES,
                                       sizeof(uint16_t), NULL, uniq);
            if (NULL == uniq->heap) {
                uniq->err_fn("Error allocating space for %u heap entries",
                             MAX_MERGE_FILES);
                return -1;
            }
        }

        /* Read the first record from each file into the rec[] array;
         * generate and store the key in the key[] array.  Insert the
         * index into the heap. */
        for (i = 0; i < open_count; ++i) {
            if (sortuniqFillRecordAndKey(uniq, i)) {
                skHeapInsert(uniq->heap, &i);
            }
        }

        UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                             ": Processing %" PRIu32
                             " of %" PRIu32 " open input files"),
                            skHeapGetNumberEntries(uniq->heap), open_count));

        /* process this set of files */
        if (uniq->fi.distinct_num_fields) {
            rv = sortuniqReadSilkTotemp(uniq);
        } else if (no_more_inputs && uniq->temp_idx == 0) {
            /* opened everything in one pass; no longer need the
             * intermediate temp file */
            uniqTempClose(uniq->temp_fp);
            uniq->temp_fp = NULL;
            uniq->temp_idx = -1;
            uniq->max_temp_idx = -1;
            rv = sortuniqReadSilkNodist(uniq, output_fn, callback_data);
        } else {
            rv = sortuniqReadSilkNodist(uniq, NULL, NULL);
        }
        if (rv) {
            return rv;
        }

        /* Close the input files that we processed this time. */
        for (i = 0; i < open_count; ++i) {
            skStreamDestroy(&uniq->fps[i]);
        }

        /* Close the intermediate temp file. */
        if (uniq->temp_fp) {
            UNIQUE_DEBUG(uniq,
                         ((SKUNIQUE_DEBUG_ENVAR ": Finished writing '%s'"),
                          UNIQUE_TMPNAME_OUT(uniq)));
            uniqTempClose(uniq->temp_fp);
            uniq->temp_fp = NULL;
        }
        if (uniq->dist_fp) {
            uniqTempClose(uniq->dist_fp);
            uniq->dist_fp = NULL;
        }

    } while (!no_more_inputs);

    /* we are finished processing records; free the 'rec' array */
    free(uniq->rec);
    uniq->rec = NULL;

    /* If any temporary files were written, we now have to merge them.
     * Otherwise, we didn't write any temporary files, and we are
     * done. */
    if (uniq->temp_idx < 0) {
        /* no temporary files were written */
        return 0;
    }

    UNIQUE_DEBUG(uniq, ("Finished reading SiLK Flow records"));

    /* index at which to start the merge */
    tmp_idx_a = 0;

    opened_all_temps = 0;

    /* This loop repeats as long as we haven't opened all of the temp
     * files generated while reading the flows. */
    do {
        /* index at which to stop the merge */
        if (uniq->max_temp_idx - tmp_idx_a < MAX_MERGE_FILES - 1) {
            /* number of temp files is less than max_merge files */
            tmp_idx_b = uniq->max_temp_idx;
        } else {
            /* must stop at max_merge */
            tmp_idx_b = tmp_idx_a + MAX_MERGE_FILES - 1;
        }

        /* when using distinct fields, files must we opened in
         * pairs */
        if (uniq->fi.distinct_num_fields) {
            assert((tmp_idx_a & 0x1) == 0);
            if (((tmp_idx_b & 0x1) == 0) && (tmp_idx_b > tmp_idx_a)) {
                --tmp_idx_b;
            }
        }

        UNIQUE_DEBUG(uniq,
                     ((SKUNIQUE_DEBUG_ENVAR
                       ": Attempting to open temporary files #%d through #%d"),
                      tmp_idx_a, tmp_idx_b));

        uniq->temp_idx_base = tmp_idx_a;

        /* open an intermediate temp file.  The merge-sort will have
         * to write nodes here if there are not enough file handles
         * available to open all the temporary files we wrote while
         * reading the data. */
        uniq->temp_fp = uniqTempCreate(uniq->tmpctx, &uniq->max_temp_idx);
        if (uniq->temp_fp == NULL) {
            uniq->err_fn("Error creating intermediate temporary file: %s",
                         strerror(errno));
            return -1;
        }
        uniq->temp_idx = uniq->max_temp_idx;
        if (uniq->fi.distinct_num_fields) {
            uniq->dist_fp = uniqTempCreate(uniq->tmpctx, &uniq->max_temp_idx);
            if (uniq->dist_fp == NULL) {
                uniq->err_fn("Error creating intermediate temporary file: %s",
                             strerror(errno));
                return -1;
            }
        }

        /* number of files successfully opened */
        open_count = 0;

        /* Attempt to open up to MAX_MERGE_FILES, though an open may
         * fail due to lack of resources (EMFILE or ENOMEM) */
        for (i = tmp_idx_a; i <= tmp_idx_b; ++i) {
            uniq->fps[open_count] = uniqTempReopen(uniq->tmpctx, i);
            if (uniq->fps[open_count] == NULL) {
                if ((open_count > 0)
                    && ((errno == EMFILE) || (errno == ENOMEM)))
                {
                    /* We cannot open any more temp files; we'll need
                     * to catch file 'i' the next time around. */
                    tmp_idx_b = i - 1;
                    UNIQUE_DEBUG(uniq,
                                 ((SKUNIQUE_DEBUG_ENVAR ": EMFILE limit hit"
                                   "---merging #%d through #%d to #%d: %s"),
                                  tmp_idx_a, tmp_idx_b, uniq->temp_idx,
                                  strerror(errno)));
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

        if (uniq->fi.distinct_num_fields) {
            if ((open_count & 0x1) == 1) {
                /* number of opened files must be even */
                --tmp_idx_b;
                --open_count;
                uniqTempClose(uniq->fps[open_count]);
            }
        }

        UNIQUE_DEBUG(uniq,
                     ((SKUNIQUE_DEBUG_ENVAR ": Opened %d temporary files"),
                      open_count));

        /* Check to see if we've opened all temp files.  If so, close
         * the intermediate file. */
        if (tmp_idx_b == uniq->temp_idx - 1) {
            /* no longer need the intermediate temp file */
            UNIQUE_DEBUG(uniq,
                         (SKUNIQUE_DEBUG_ENVAR
                          ": Successfully opened all%s temporary files",
                          ((tmp_idx_a > 0) ? " remaining" : "")));
            uniqTempClose(uniq->temp_fp);
            uniq->temp_fp = NULL;
            opened_all_temps = 1;
            if (uniq->dist_fp) {
                uniqTempClose(uniq->dist_fp);
                uniq->dist_fp = NULL;
            }
        }

        step = 1 + (uniq->fi.distinct_num_fields > 0);

        /* Read the first key from each temp file into the 'key[]'
         * array on the uniq object; add file's index to the heap */
        for (i = 0; i < open_count; i += step) {
            if (uniqTempRead(uniq->fps[i], uniq->key[i], uniq->fi.key_octets)){
                skHeapInsert(uniq->heap, &i);
            } else {
                last_errno = skStreamGetLastErrno(uniq->fps[i]);
                if (last_errno) {
                    uniq->err_fn(
                        "Cannot read first key from temporary file '%s'; %s",
                        skTempFileGetName(uniq->tmpctx, tmp_idx_a + i),
                        strerror(last_errno));
                    return -1;
                }
                UNIQUE_DEBUG(uniq,
                             ((SKUNIQUE_DEBUG_ENVAR
                               ": Ignoring empty temporary file '%s'"),
                              skTempFileGetName(uniq->tmpctx, tmp_idx_a + i)));
            }
        }

        UNIQUE_DEBUG(uniq, ((SKUNIQUE_DEBUG_ENVAR
                             ": Processing %" PRIu32
                             " of %" PRIu32 " open temporary files"),
                            skHeapGetNumberEntries(uniq->heap), open_count));

        /* process this set of files */
        if (uniq->temp_fp) {
            if (uniq->fi.distinct_num_fields) {
                rv = sortuniqMergeFilesDist(uniq, NULL, NULL);
            } else {
                rv = sortuniqMergeFilesNodist(uniq, NULL, NULL);
            }
        } else {
            if (uniq->fi.distinct_num_fields) {
                rv = sortuniqMergeFilesDist(uniq, output_fn, callback_data);
            } else {
                rv = sortuniqMergeFilesNodist(uniq, output_fn, callback_data);
            }
        }
        if (rv) {
            return rv;
        }

        /* Close all the temp files that we processed this time. */
        for (i = 0; i < open_count; ++i) {
            uniqTempClose(uniq->fps[i]);
        }
        /* Delete all the temp files that we opened */
        for (i = tmp_idx_a; i <= tmp_idx_b; ++i) {
            skTempFileRemove(uniq->tmpctx, i);
        }

        /* Close the intermediate temp file(s). */
        if (uniq->temp_fp) {
            uniqTempClose(uniq->temp_fp);
            uniq->temp_fp = NULL;
        }
        if (uniq->dist_fp) {
            uniqTempClose(uniq->dist_fp);
            uniq->dist_fp = NULL;
        }

        /* start the next merge with the next temp file */
        tmp_idx_a = tmp_idx_b + 1;
    } while (!opened_all_temps);

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
