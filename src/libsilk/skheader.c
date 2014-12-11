/*
** Copyright (C) 2006-2014 by Carnegie Mellon University.
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
**  skheader.c
**
**    Routines to read, write, and manipulate the header of a SiLK file
**
**    Mark Thomas
**    November 2006
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skheader.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skbag.h>
#include <silk/skheader.h>
#include "skstream_priv.h"

/* TYPDEFS AND DEFINES */

/* This struct is invariant over all Output File Formats that we write
 * to disk. */
typedef struct genericHeader_st {
    uint8_t magic1;       /* fixed byte order 4byte magic number: 0xdeadbeef */
    uint8_t magic2;
    uint8_t magic3;
    uint8_t magic4;
    uint8_t isBigEndian;  /* endian order on hw creating file 1/0 big/little */
    uint8_t type;         /* output file format; values defined below */
    uint8_t version;      /* version of above */
    uint8_t compMethod;   /* compression method */
} genericHeader;

/* in order to be byte order independent, define 4 bytes size magic values */
#define MAGIC1  0xDE
#define MAGIC2  0xAD
#define MAGIC3  0xBE
#define MAGIC4  0xEF

#define CHECKMAGIC(h)                           \
    ( (h)->magic1 != MAGIC1                     \
      ||  (h)->magic2 != MAGIC2                 \
      ||  (h)->magic3 != MAGIC3                 \
      ||  (h)->magic4 != MAGIC4 )


/* Initial size to allocate for a header-entry; this can grow as
 * required */
#define HENTRY_INIT_BUFSIZE 512


/*
 *  end_of_headers = HENTRY_SPEC_EOH(header_entry);
 *
 *    Return a TRUE value if 'header_entry' is the final header entry.
 *    False otherwise.
 */
#define HENTRY_SPEC_EOH(he)                                     \
    (skHeaderEntryGetTypeId((sk_header_entry_t*)(he)) == 0)


/*
 *  SK_HENTRY_SPEC_UNPACK(header_entry_spec, raw_bytes);
 *
 *    Copy the octet array 'raw_bytes' that contains the header entry
 *    spec into 'header_entry_spec'.  'raw_bytes' should be in network
 *    byte order, and 'header_entry_spec' will be in native byte
 *    order.
 */
#define SK_HENTRY_SPEC_UNPACK(new_spec, raw_bytes)                      \
    {                                                                   \
        memcpy((new_spec),(raw_bytes),sizeof(sk_header_entry_spec_t));  \
        (new_spec)->hes_id = ntohl((new_spec)->hes_id);                 \
        (new_spec)->hes_len = ntohl((new_spec)->hes_len);               \
    }


/*
 *  SK_HENTRY_SPEC_PACK(raw_bytes, header_entry_spec);
 *
 *    Copy the header entry spec 'header_entry_spec' into the octet
 *    array 'raw_bytes'.  'header_entry_spec' should be in native byte
 *    order, and 'raw_bytes' will be in network byte order.
 */
#define SK_HENTRY_SPEC_PACK(raw_bytes, old_spec)                        \
    {                                                                   \
        sk_header_entry_spec_t _spec;                                   \
        _spec.hes_id = htonl((old_spec)->hes_id);                       \
        _spec.hes_len = htonl((old_spec)->hes_len);                     \
        memcpy((raw_bytes),(&_spec), sizeof(sk_header_entry_spec_t));   \
    }


/* LOCAL VARIABLES */

/* A linked list of all registered header types */
static sk_hentry_type_t *hentry_type_list = NULL;



/* LOCAL FUNCTION PROTOTYPES */

static sk_header_entry_t *
skHentryDefaultCopy(
    const sk_header_entry_t    *hentry);

static void
skHentryDefaultFree(
    sk_header_entry_t  *hentry);

static ssize_t
skHentryDefaultPacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize);

static void
skHentryDefaultPrint(
    sk_header_entry_t  *hentry,
    FILE               *fh);

static sk_header_entry_t *
skHentryDefaultUnpacker(
    uint8_t            *in_packed);


/* FUNCTION DEFINITIONS */


int
skHeaderAddEntry(
    sk_file_header_t   *hdr,
    sk_header_entry_t  *hentry)
{
    sk_hentry_node_t *new_node;

    assert(hdr);
    if (NULL == hentry) {
        return SKHEADER_ERR_NULL_ARGUMENT;
    }
    if (hdr->header_lock == SKHDR_LOCK_FIXED) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    if (HENTRY_SPEC_EOH(hentry)) {
        return SKHEADER_ERR_INVALID_ID;
    }

    /* create a new node to hold the entry */
    new_node = (sk_hentry_node_t*)calloc(1, sizeof(sk_hentry_node_t));
    if (NULL == new_node) {
        return SKHEADER_ERR_ALLOC;
    }
    new_node->hen_entry = hentry;
    new_node->hen_type = skHentryTypeLookup(skHeaderEntryGetTypeId(hentry));

    /* the new entry's node goes just before the end-of-header
     * marker, which is the root node */
    new_node->hen_prev = (hdr->fh_rootnode)->hen_prev;
    new_node->hen_next = (hdr->fh_rootnode);
    new_node->hen_prev->hen_next = new_node;
    new_node->hen_next->hen_prev = new_node;

    return SKHEADER_OK;
}


int
skHeaderCopy(
    sk_file_header_t       *dst_hdr,
    const sk_file_header_t *src_hdr,
    uint32_t                copy_flags)
{
    sk_header_start_t *dst_start = &(dst_hdr->fh_start);
    const sk_header_start_t *src_start = &(src_hdr->fh_start);
    sk_hentry_node_t *hnode;
    sk_hentry_type_t *htype;
    sk_header_entry_t *dst_hentry;
    sk_header_entry_t *src_hentry;
    int rv = SKHEADER_OK;

    /* do not overwrite a locked header */
    if (dst_hdr->header_lock == SKHDR_LOCK_FIXED) {
        return SKHEADER_ERR_IS_LOCKED;
    }
    if ((dst_hdr->header_lock == SKHDR_LOCK_ENTRY_OK)
        && (copy_flags != SKHDR_CP_ENTRIES))
    {
        return SKHEADER_ERR_IS_LOCKED;
    }

    /* do not copy the file version if it is older than the minimum we
     * support */
    if (src_start->file_version < SKHDR_EXPANDED_INIT_VERS) {
        copy_flags &= ~SKHDR_CP_FILE_VERS;
    }

    if ((copy_flags & SKHDR_CP_START) == SKHDR_CP_START) {
        /* copy the entire header_start except silk_version */
        uint32_t silk_vers = dst_start->silk_version;
        memcpy(dst_start, src_start, sizeof(sk_header_start_t));
        dst_start->silk_version = silk_vers;
    } else if (copy_flags & SKHDR_CP_START) {
        /* copy parts of header_start */
        if (copy_flags & SKHDR_CP_FORMAT) {
            dst_start->file_format = src_start->file_format;
            /* Always set the record size to 0 */
            dst_start->rec_size = 0;
        }
        if (copy_flags & SKHDR_CP_FILE_VERS) {
            dst_start->file_version = src_start->file_version;
        }
        if (copy_flags & SKHDR_CP_COMPMETHOD) {
            dst_start->comp_method = src_start->comp_method;
        }
        if (copy_flags & SKHDR_CP_REC_LEN) {
            /* Always set the record size to 0 */
            dst_start->rec_size = 0;
        }
        if (copy_flags & SKHDR_CP_REC_VERS) {
            dst_start->rec_version = src_start->rec_version;
        }
        if ((copy_flags & SKHDR_CP_FILE_FLAGS) == SKHDR_CP_FILE_FLAGS) {
            dst_start->file_flags = src_start->file_flags;
        } else if (copy_flags & SKHDR_CP_FILE_FLAGS) {
            int i;
            for (i = 0; i < 8; ++i) {
                if (copy_flags & (1 << i)) {
                    dst_start->file_flags
                        = ((dst_start->file_flags & (uint8_t)~(1 << i))
                           | (src_start->file_flags & (uint8_t)(1 << i)));
                }
            }
        }
    }

    if (copy_flags & SKHDR_CP_ENTRIES) {
        /* we have a circular linked list; where the rootnode always
         * exists---it is the end-of-header marker */
        hnode = src_hdr->fh_rootnode->hen_next;
        src_hentry = hnode->hen_entry;
        while (!HENTRY_SPEC_EOH(src_hentry)) {
            /* call the appropriate function to copy the header_entry */
            htype = skHentryTypeLookup(skHeaderEntryGetTypeId(src_hentry));

            if (htype && htype->het_copy) {
                dst_hentry = htype->het_copy(src_hentry);
            } else {
                dst_hentry = skHentryDefaultCopy(src_hentry);
            }

            if (dst_hentry == NULL) {
                rv = SKHEADER_ERR_ALLOC;
                break;
            }

            rv = skHeaderAddEntry(dst_hdr, dst_hentry);
            if (rv) {
                break;
            }

            hnode = hnode->hen_next;
            src_hentry = hnode->hen_entry;
        }
    }

    return rv;
}


int
skHeaderCopyEntries(
    sk_file_header_t       *dst_hdr,
    const sk_file_header_t *src_hdr,
    sk_hentry_type_id_t     entry_id)
{
    sk_hentry_node_t *hnode;
    sk_hentry_type_t *htype;
    sk_header_entry_t *dst_hentry;
    sk_header_entry_t *src_hentry;
    sk_hentry_copy_fn_t copy_fn;
    int rv = SKHEADER_OK;

    /* do not overwrite a locked header */
    if (dst_hdr->header_lock == SKHDR_LOCK_FIXED) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    /* get a handle to the copy function */
    htype = skHentryTypeLookup(entry_id);
    if (htype && htype->het_copy) {
        copy_fn = htype->het_copy;
    } else {
        copy_fn = &skHentryDefaultCopy;
    }

    /* we have a circular linked list; where the rootnode always
     * exists---it is the end-of-header marker */
    hnode = src_hdr->fh_rootnode;
    do {
        hnode = hnode->hen_next;
        src_hentry = hnode->hen_entry;

        if (entry_id != skHeaderEntryGetTypeId(src_hentry)) {
            continue;
        }

        dst_hentry = copy_fn(src_hentry);
        if (dst_hentry == NULL) {
            rv = SKHEADER_ERR_ALLOC;
            break;
        }

        rv = skHeaderAddEntry(dst_hdr, dst_hentry);
        if (rv) {
            break;
        }
    } while (!HENTRY_SPEC_EOH(src_hentry));

    return rv;
}


int
skHeaderCreate(
    sk_file_header_t  **hdr)
{
    sk_file_header_t *new_hdr;
    char *envar;
    int rv = SKHEADER_ERR_ALLOC;

    new_hdr = (sk_file_header_t*)calloc(1, sizeof(sk_file_header_t));
    if (NULL == new_hdr) {
        goto END;
    }

    /* fill out the sk_header_start_t */
    new_hdr->fh_start.magic1 = MAGIC1;
    new_hdr->fh_start.magic2 = MAGIC2;
    new_hdr->fh_start.magic3 = MAGIC3;
    new_hdr->fh_start.magic4 = MAGIC4;
#if SK_LITTLE_ENDIAN
    new_hdr->fh_start.file_flags = 0;
#else
    new_hdr->fh_start.file_flags = 1;
#endif
    new_hdr->fh_start.file_format = UINT8_MAX;
    new_hdr->fh_start.file_version = SK_DEFAULT_FILE_VERSION;
    new_hdr->fh_start.comp_method = SK_COMPMETHOD_DEFAULT;
    new_hdr->fh_start.rec_size = 0;
    new_hdr->fh_start.rec_version = SK_RECORD_VERSION_ANY;

#ifdef SILK_HEADER_NOVERSION_ENV
    envar = getenv(SILK_HEADER_NOVERSION_ENV);
#else
    envar = NULL;
#endif
    if (envar && envar[0]) {
        new_hdr->fh_start.silk_version = 0;
    } else {
        new_hdr->fh_start.silk_version = SK_VERSION_INTEGER;
    }

    /* create the node to hold the end-of-header marker.  This is a
     * circular, doubly-linked list. */
    new_hdr->fh_rootnode
        = (sk_hentry_node_t*)calloc(1, sizeof(sk_hentry_node_t));
    if (NULL == new_hdr->fh_rootnode) {
        goto END;
    }
    new_hdr->fh_rootnode->hen_next = new_hdr->fh_rootnode;
    new_hdr->fh_rootnode->hen_prev = new_hdr->fh_rootnode;
    new_hdr->fh_rootnode->hen_type = NULL;

    /* create the entry */
    new_hdr->fh_rootnode->hen_entry
        = (sk_header_entry_t*)calloc(1, sizeof(sk_header_entry_t));
    if (NULL == new_hdr->fh_rootnode->hen_entry) {
        goto END;
    }
    new_hdr->fh_rootnode->hen_entry->he_spec.hes_id = 0;
    new_hdr->fh_rootnode->hen_entry->he_spec.hes_len
        = sizeof(sk_header_entry_spec_t);

    /* Success */
    rv = SKHEADER_OK;

  END:
    if (SKHEADER_OK == rv) {
        *hdr = new_hdr;
    } else {
        if (new_hdr) {
            if (new_hdr->fh_rootnode) {
                if (new_hdr->fh_rootnode->hen_entry) {
                    free(new_hdr->fh_rootnode->hen_entry);
                }
                free(new_hdr->fh_rootnode);
            }
            free(new_hdr);
        }
    }
    return rv;
}


int
skHeaderDestroy(
    sk_file_header_t  **hdr)
{
    sk_hentry_node_t *hnode;
    sk_header_entry_t *hentry;
    sk_hentry_type_t *htype;

    if (NULL == hdr || NULL == *hdr) {
        return SKHEADER_OK;
    }

    /* we have a circular linked list; where the rootnode always
     * exists---it is the end-of-header marker */
    hnode = (*hdr)->fh_rootnode->hen_next;
    while ( !HENTRY_SPEC_EOH(hnode->hen_entry)) {
        hentry = hnode->hen_entry;

        /* call the appropriate function to destroy the header_entry */
        htype = skHentryTypeLookup(skHeaderEntryGetTypeId(hentry));
        if (htype && htype->het_free) {
            htype->het_free(hentry);
        } else {
            skHentryDefaultFree(hentry);
        }
        hnode->hen_entry = NULL;
        /* now destroy the node */
        assert(hnode == hnode->hen_next->hen_prev);
        hnode = hnode->hen_next;
        free(hnode->hen_prev);
    }

    /* destroy the root node */
    assert(hnode == (*hdr)->fh_rootnode);
    hentry = hnode->hen_entry;
    free(hentry);
    free(hnode);
    (*hdr)->fh_rootnode = NULL;

    /* destroy the header */
    free(*hdr);
    *hdr = NULL;

    return SKHEADER_OK;
}


sk_header_entry_t *
skHeaderEntryCopy(
    const sk_header_entry_t    *src_hentry)
{
    sk_hentry_type_t *htype;
    sk_header_entry_t *dst_hentry = NULL;

    if (HENTRY_SPEC_EOH(src_hentry)) {
        return NULL;
    }

    /* call the appropriate function to copy the header_entry */
    htype = skHentryTypeLookup(skHeaderEntryGetTypeId(src_hentry));

    if (htype && htype->het_copy) {
        dst_hentry = htype->het_copy(src_hentry);
    } else {
        dst_hentry = skHentryDefaultCopy(src_hentry);
    }

    return dst_hentry;
}


void
skHeaderEntryPrint(
    sk_header_entry_t  *hentry,
    FILE               *fp)
{
    sk_hentry_type_t *htype;

    fprintf(fp, ("HDR id = %" PRIu32 " / len = %" PRIu32 " / "),
            (uint32_t)skHeaderEntryGetTypeId(hentry),
            (uint32_t)hentry->he_spec.hes_len);

    htype = skHentryTypeLookup(skHeaderEntryGetTypeId(hentry));
    if (htype && htype->het_print) {
        htype->het_print(hentry, fp);
    } else {
        skHentryDefaultPrint(hentry, fp);
    }
}


sk_compmethod_t
skHeaderGetCompressionMethod(
    const sk_file_header_t *hdr)
{
    return hdr->fh_start.comp_method;
}


silk_endian_t
skHeaderGetByteOrder(
    const sk_file_header_t *hdr)
{
    return ((hdr->fh_start.file_flags & 0x01)
            ? SILK_ENDIAN_BIG
            : SILK_ENDIAN_LITTLE);
}


fileFormat_t
skHeaderGetFileFormat(
    const sk_file_header_t *hdr)
{
    return hdr->fh_start.file_format;
}


fileVersion_t
skHeaderGetFileVersion(
    const sk_file_header_t *hdr)
{
    return hdr->fh_start.file_version;
}


sk_header_entry_t *
skHeaderGetFirstMatch(
    const sk_file_header_t *hdr,
    sk_hentry_type_id_t     entry_id)
{
    sk_hentry_node_t *hnode;
    sk_header_entry_t *hentry;

    assert(hdr);

    /* we have a circular linked list; where the rootnode always
     * exists---it is the end-of-header marker */
    hnode = hdr->fh_rootnode;
    do {
        hnode = hnode->hen_next;
        hentry = hnode->hen_entry;
        if (skHeaderEntryGetTypeId(hentry) == entry_id) {
            return hentry;
        }
    } while (!HENTRY_SPEC_EOH(hentry));

    return NULL;
}


size_t
skHeaderGetLength(
    const sk_file_header_t *hdr)
{
    return hdr->header_length;
}


sk_header_lock_t
skHeaderGetLockStatus(
    const sk_file_header_t *hdr)
{
    return hdr->header_lock;
}


size_t
skHeaderGetRecordLength(
    const sk_file_header_t *hdr)
{
    return hdr->fh_start.rec_size;
}


fileVersion_t
skHeaderGetRecordVersion(
    const sk_file_header_t *hdr)
{
    return hdr->fh_start.rec_version;
}


uint32_t
skHeaderGetSilkVersion(
    const sk_file_header_t *hdr)
{
    return hdr->fh_start.silk_version;
}


int
skHeaderIsNativeByteOrder(
    const sk_file_header_t *hdr)
{
    return ((hdr->fh_start.file_flags & 0x01) == (SK_BIG_ENDIAN));
}


void
skHeaderIteratorBind(
    sk_hentry_iterator_t   *iter,
    const sk_file_header_t *hdr)
{
    assert(hdr);
    assert(iter);

    iter->hdr = hdr;
    iter->node = hdr->fh_rootnode;
    iter->htype_filter = 0;
}


void
skHeaderIteratorBindType(
    sk_hentry_iterator_t   *iter,
    const sk_file_header_t *hdr,
    sk_hentry_type_id_t     htype)
{
    skHeaderIteratorBind(iter, hdr);
    iter->htype_filter = htype;
}


sk_header_entry_t *
skHeaderIteratorNext(
    sk_hentry_iterator_t   *iter)
{
    do {
        iter->node = iter->node->hen_next;
        if (HENTRY_SPEC_EOH(iter->node->hen_entry)) {
            return NULL;
        }
    } while (iter->htype_filter
             != skHeaderEntryGetTypeId(iter->node->hen_entry));

    return iter->node->hen_entry;
}


int
skHeaderInitialize(
    void)
{
    static int initialized = 0;
    int rv = SKHEADER_OK;

    if (initialized) {
        return rv;
    }
    initialized = 1;

    rv |= skHentryTypeRegister(SK_HENTRY_PACKEDFILE_ID,
                               &skHentryPackedfilePacker,
                               &skHentryPackedfileUnpacker,
                               &skHentryPackedfileCopy,
                               &skHentryPackedfileFree,
                               &skHentryPackedfilePrint);
    rv |= skHentryTypeRegister(SK_HENTRY_INVOCATION_ID,
                               &skHentryInvocationPacker,
                               &skHentryInvocationUnpacker,
                               &skHentryInvocationCopy,
                               &skHentryInvocationFree,
                               &skHentryInvocationPrint);
    rv |= skHentryTypeRegister(SK_HENTRY_ANNOTATION_ID,
                               &skHentryAnnotationPacker,
                               &skHentryAnnotationUnpacker,
                               &skHentryAnnotationCopy,
                               &skHentryAnnotationFree,
                               &skHentryAnnotationPrint);
    rv |= skHentryTypeRegister(SK_HENTRY_PROBENAME_ID,
                               &skHentryProbenamePacker,
                               &skHentryProbenameUnpacker,
                               &skHentryProbenameCopy,
                               &skHentryProbenameFree,
                               &skHentryProbenamePrint);
    rv |= skHentryTypeRegister(SK_HENTRY_PREFIXMAP_ID,
                               &skHentryPrefixmapPacker,
                               &skHentryPrefixmapUnpacker,
                               &skHentryPrefixmapCopy,
                               &skHentryPrefixmapFree,
                               &skHentryPrefixmapPrint);
    rv |= skHentryTypeRegister(SK_HENTRY_BAG_ID,
                               &skHentryBagPacker,
                               &skHentryBagUnpacker,
                               &skHentryBagCopy,
                               &skHentryBagFree,
                               &skHentryBagPrint);
    rv |= skHentryTypeRegister(SK_HENTRY_IPSET_ID,
                               &skHentryIPSetPacker,
                               &skHentryIPSetUnpacker,
                               &skHentryIPSetCopy,
                               &skHentryIPSetFree,
                               &skHentryIPSetPrint);

    rv |= skHeaderLegacyInitialize();

    return rv;
}


int
skHeaderReadEntries(
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    sk_header_entry_spec_t *spec;
    sk_header_entry_t *hentry;
    int rv = SKHEADER_OK;
    uint32_t len;
    ssize_t saw;
    size_t bufsize;
    sk_hentry_type_t *htype;

    assert(hdr);

    if (hdr->fh_start.file_version < SKHDR_EXPANDED_INIT_VERS) {
        return skHeaderLegacyDispatch(stream, hdr);
    }

    /* create a buffer that we read the headers into; assign it to a
     * sk_header_entry_spec_t* since that is how we read the first few
     * bytes, but it is really just a buffer of uint8_t's. */
    bufsize = HENTRY_INIT_BUFSIZE;
    spec = (sk_header_entry_spec_t*)malloc(bufsize);
    if (NULL == spec) {
        rv = SKHEADER_ERR_ALLOC;
        goto END;
    }

    for (;;) {
        /* read the header_entry_spec */
        saw = skStreamRead(stream, spec, sizeof(sk_header_entry_spec_t));
        if (saw < 0) {
            rv = -1;
            goto END;
        }
        hdr->header_length += saw;
        if (saw < (ssize_t)sizeof(sk_header_entry_spec_t)) {
            /* header claims to be smaller than the amount of data
             * we've already read */
            rv = SKHEADER_ERR_ENTRY_READ;
            goto END;
        }

        /* length of header_entry */
        len = ntohl(spec->hes_len);
        if (len < (uint32_t)saw) {
            /* header claims to be smaller than the amount of data
             * we've already read */
            rv = SKHEADER_ERR_ENTRY_READ;
            goto END;
        }

        /* if buffer is too small, grow it to hold the complete
         * header_entry */
        if (bufsize < len) {
            sk_header_entry_spec_t *old_spec = spec;
            spec = (sk_header_entry_spec_t*)realloc(spec, len);
            if (!spec) {
                spec = old_spec;
                rv = SKHEADER_ERR_ALLOC;
                goto END;
            }
            bufsize = len;
        }

        /* read the bytes into the buffer */
        len -= saw;
        saw = skStreamRead(stream, &(((uint8_t*)spec)[saw]), len);
        if (saw < 0) {
            rv = -1;
            goto END;
        }
        hdr->header_length += saw;
        if (saw < (ssize_t)len) {
            /* short read */
            rv = SKHEADER_ERR_SHORTREAD;
            goto END;
        }

        if (spec->hes_id == 0) {
            /* stop if this is the end-of-header marker */
            break;
        }

        /* call the appropriate function to unpack the header_entry */
        htype = skHentryTypeLookup(ntohl(spec->hes_id));
        if (htype && htype->het_unpacker) {
            hentry = htype->het_unpacker((uint8_t*)spec);
        } else {
            hentry = skHentryDefaultUnpacker((uint8_t*)spec);
        }
        if (NULL == hentry) {
            rv = SKHEADER_ERR_ENTRY_UNPACK;
            goto END;
        }

        /* and add the header_entry to the list of headers */
        rv = skHeaderAddEntry(hdr, hentry);
        if (rv) {
            goto END;
        }
    }

  END:
    if (spec) {
        free(spec);
    }
    return rv;
}


int
skHeaderReadStart(
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    size_t len;
    ssize_t saw;

    assert(stream);
    assert(hdr);

    /* set header length to 0 */
    hdr->header_length = 0;

    /* read the traditional "genericHeader" */
    len = sizeof(genericHeader);
    saw = skStreamRead(stream, &(hdr->fh_start), len);
    if (saw != (ssize_t)len) {
        if (saw == -1) {
            return SKSTREAM_ERR_READ;
        }
        return SKHEADER_ERR_SHORTREAD;
    }
    hdr->header_length += saw;

    /* verify is SiLK file */
    if (CHECKMAGIC((genericHeader*)hdr)) {
        return SKSTREAM_ERR_BAD_MAGIC;
    }

    /* if this file's version indicates it was written when we only
     * had the generic header, return */
    if (hdr->fh_start.file_version < SKHDR_EXPANDED_INIT_VERS) {
        return SKSTREAM_OK;
    }

    /* read the remainder of the sk_header_start_t */
    len = sizeof(sk_header_start_t) - sizeof(genericHeader);
    saw = skStreamRead(stream, &(hdr->fh_start.silk_version), len);
    if (saw != (ssize_t)len) {
        if (saw == -1) {
            return SKSTREAM_ERR_READ;
        }
        return SKHEADER_ERR_SHORTREAD;
    }
    hdr->header_length += saw;
    hdr->fh_start.silk_version = ntohl(hdr->fh_start.silk_version);
    hdr->fh_start.rec_size = ntohs(hdr->fh_start.rec_size);
    hdr->fh_start.rec_version = ntohs(hdr->fh_start.rec_version);

    return SKHEADER_OK;
}


int
skHeaderRemoveAllMatching(
    sk_file_header_t       *hdr,
    sk_hentry_type_id_t     entry_id)
{
    sk_hentry_node_t *hnode;
    sk_hentry_node_t *hnode_next;
    sk_header_entry_t *hentry;
    sk_hentry_type_t *htype;

    assert(hdr);

    if (entry_id == 0) {
        return SKHEADER_ERR_INVALID_ID;
    }

    /* Do not modify a locked header */
    if (hdr->header_lock) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    htype = skHentryTypeLookup(entry_id);

    /* we have a circular linked list; where the rootnode always
     * exists---it is the end-of-header marker */
    hnode = hdr->fh_rootnode->hen_next;
    hentry = hnode->hen_entry;
    while (!HENTRY_SPEC_EOH(hentry)) {
        hnode_next = hnode->hen_next;
        if (skHeaderEntryGetTypeId(hentry) == entry_id) {
            /* fix linked lists */
            hnode->hen_prev->hen_next = hnode_next;
            hnode_next->hen_prev = hnode->hen_prev;
            /* free the header entry and the node */
            if (htype && htype->het_free) {
                htype->het_free(hentry);
            } else {
                skHentryDefaultFree(hentry);
            }
            hnode->hen_entry = NULL;
            free(hnode);
        }
        hnode = hnode_next;
        hentry = hnode->hen_entry;
    }

    return SKHEADER_OK;
}


int
skHeaderReplaceEntry(
    sk_file_header_t           *hdr,
    sk_header_entry_t          *old_entry,
    sk_header_entry_t          *new_entry,
    sk_hentry_callback_fn_t     hentry_cb)
{
    sk_hentry_node_t *hnode;
    sk_header_entry_t *hentry;
    int found = 0;

    assert(hdr);
    assert(old_entry);

    /* Cannot replace the end of header marker */
    if (HENTRY_SPEC_EOH(old_entry)) {
        return SKHEADER_ERR_INVALID_ID;
    }
    if (new_entry && HENTRY_SPEC_EOH(new_entry)) {
        return SKHEADER_ERR_INVALID_ID;
    }
    /* Do not modify a locked header */
    if (hdr->header_lock) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    hnode = hdr->fh_rootnode;
    do {
        hnode = hnode->hen_next;
        hentry = hnode->hen_entry;
        if (hentry == old_entry) {
            found = 1;
            if (hentry_cb) {
                hentry_cb(old_entry);
            }
            if (new_entry) {
                hnode->hen_entry = new_entry;
            } else {
                hnode->hen_prev->hen_next = hnode->hen_next;
                hnode->hen_next->hen_prev = hnode->hen_prev;
                hnode->hen_entry = NULL;
                free(hnode);
            }
            break;
        }
    } while (!HENTRY_SPEC_EOH(hnode->hen_entry));

    if (!found) {
        return SKHEADER_ERR_ENTRY_NOTFOUND;
    }

    return SKHEADER_OK;
}


int
skHeaderSetByteOrder(
    sk_file_header_t   *hdr,
    silk_endian_t       byte_order)
{
    if (hdr == NULL) {
        return SKHEADER_ERR_NULL_ARGUMENT;
    }

    /* Do not modify a locked header */
    if (hdr->header_lock) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    switch (byte_order) {
      case SILK_ENDIAN_BIG:
        hdr->fh_start.file_flags = (hdr->fh_start.file_flags | 0x01);
        break;
      case SILK_ENDIAN_LITTLE:
        hdr->fh_start.file_flags = (hdr->fh_start.file_flags & 0xFE);
        break;
      case SILK_ENDIAN_NATIVE:
      case SILK_ENDIAN_ANY:
#if SK_LITTLE_ENDIAN
        hdr->fh_start.file_flags = (hdr->fh_start.file_flags & 0xFE);
#else
        hdr->fh_start.file_flags = (hdr->fh_start.file_flags | 0x01);
#endif
        break;
    }

    return SKHEADER_OK;
}


int
skHeaderSetCompressionMethod(
    sk_file_header_t   *hdr,
    uint8_t             comp_method)
{
    if (hdr == NULL) {
        return SKHEADER_ERR_NULL_ARGUMENT;
    }

    /* Do not modify a locked header */
    if (hdr->header_lock) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    switch (sksiteCompmethodCheck(comp_method)) {
      case SK_COMPMETHOD_IS_AVAIL:
      case SK_COMPMETHOD_IS_KNOWN:
        /* known, valid, and available, or undecided which will
         * eventually be available */
        hdr->fh_start.comp_method = comp_method;
        break;
      case SK_COMPMETHOD_IS_VALID:
        /* method is not available--missing library at configure */
        return SKSTREAM_ERR_COMPRESS_UNAVAILABLE;
      default:
        /* value is completely unknown */
        return SKSTREAM_ERR_COMPRESS_INVALID;
    }

    return SKHEADER_OK;
}


int
skHeaderSetFileFormat(
    sk_file_header_t   *hdr,
    fileFormat_t        file_format)
{
    if (hdr == NULL) {
        return SKHEADER_ERR_NULL_ARGUMENT;
    }

    /* Do not modify a locked header */
    if (hdr->header_lock) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    if (sksiteFileformatIsValid(file_format)) {
        hdr->fh_start.file_format = file_format;
    } else {
        return SKSTREAM_ERR_INVALID_INPUT;
    }

    return SKHEADER_OK;
}


int
skHeaderSetLock(
    sk_file_header_t   *hdr,
    sk_header_lock_t    lock)
{
    if (hdr == NULL) {
        return SKHEADER_ERR_NULL_ARGUMENT;
    }

    hdr->header_lock = lock;
    return SKHEADER_OK;
}


int
skHeaderSetPaddingModulus(
    sk_file_header_t   *hdr,
    uint32_t            mod)
{
    if (hdr == NULL) {
        return SKHEADER_ERR_NULL_ARGUMENT;
    }

    /* Do not modify a locked header */
    if (hdr->header_lock) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    hdr->padding_modulus = mod;
    return SKHEADER_OK;
}


int
skHeaderSetRecordLength(
    sk_file_header_t   *hdr,
    size_t              rec_len)
{
    if (hdr == NULL)

    /* Do not modify a locked header */
    if (hdr->header_lock) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    hdr->fh_start.rec_size = rec_len;
    return SKHEADER_OK;
}


int
skHeaderSetRecordVersion(
    sk_file_header_t   *hdr,
    fileVersion_t       version)
{
    if (hdr == NULL) {
        return SKHEADER_ERR_NULL_ARGUMENT;
    }

    /* Do not modify a locked header */
    if (hdr->header_lock) {
        return SKHEADER_ERR_IS_LOCKED;
    }

    hdr->fh_start.rec_version = version;
    return SKHEADER_OK;
}


int
skHeaderSkipEntries(
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    char buf[HENTRY_INIT_BUFSIZE];
    sk_header_entry_spec_t *spec;
    uint32_t len;
    ssize_t saw;
    size_t to_read;

    for (;;) {
        saw = skStreamRead(stream, buf, sizeof(sk_header_entry_spec_t));
        if (saw == -1) {
            /* error */
            return -1;
        }
        hdr->header_length += saw;
        if ((size_t)saw < sizeof(sk_header_entry_spec_t)) {
            /* short read */
            return SKHEADER_ERR_SHORTREAD;
        }

        spec = (sk_header_entry_spec_t*)buf;
        len = ntohl(spec->hes_len);

        if (len < sizeof(sk_header_entry_spec_t)) {
            /* header claims to be smaller than the amount of data
             * we've already read */
            return SKHEADER_ERR_ENTRY_READ;
        }

        /* read the data into 'buf' and ignore it */
        len -= saw;
        while (len > 0) {
            to_read = ((len < sizeof(buf)) ? len : sizeof(buf));
            saw = skStreamRead(stream, buf, to_read);
            if (saw < 0) {
                return -1;
            }
            if (saw == 0) {
                return SKHEADER_ERR_SHORTREAD;
            }
            len -= saw;
            hdr->header_length += saw;
        }

        if (spec->hes_id == 0) {
            break;
        }
    }

    return SKHEADER_OK;
}


const char *
skHeaderStrerror(
    ssize_t             err_code)
{
    static char buf[128];

    switch ((skHeaderErrorCodes_t)err_code) {
      case SKHEADER_OK:
        return "Command completed successfully";

      case SKHEADER_ERR_ALLOC:
        return "Memory allocation failed";

      case SKHEADER_ERR_NULL_ARGUMENT:
        return "NULL passed as argument to function";

      case SKHEADER_ERR_BAD_FORMAT:
        return "The file format is not supported";

      case SKHEADER_ERR_BAD_VERSION:
        return "The file version is not supported";

      case SKHEADER_ERR_ENTRY_NOTFOUND:
        return "Attempt to replace a header entry that does not exist";

      case SKHEADER_ERR_ENTRY_PACK:
        return "Error in packing a header entry";

      case SKHEADER_ERR_ENTRY_READ:
        return "Error in reading a header entry from disk";

      case SKHEADER_ERR_ENTRY_UNPACK:
        return "Error in unpacking a header entry";

      case SKHEADER_ERR_INVALID_ID:
        return "The entry ID is invalid";

      case SKHEADER_ERR_IS_LOCKED:
        return "Attempt to modify a locked header";

      case SKHEADER_ERR_LEGACY:
        return "Error handling a legacy header";

      case SKHEADER_ERR_BAD_COMPRESSION:
        return "The compression value is invalid";

      case SKHEADER_ERR_SHORTREAD:
        return "Unexpected end of file while reading header";
    }

    snprintf(buf, sizeof(buf), "Unrecognized skHeader error code %" SK_PRIdZ,
             err_code);
    return buf;
}


void
skHeaderTeardown(
    void)
{
    sk_hentry_type_t *htype;
    sk_hentry_type_t *next_htype;

    for (htype = hentry_type_list; htype != NULL; htype = next_htype) {
        next_htype = htype->het_next;
        free(htype);
    }
    hentry_type_list = NULL;

    skHeaderLegacyTeardown();
}


int
skHeaderWrite(
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    sk_header_start_t *hstart;
    sk_header_entry_t *hentry;
    sk_hentry_node_t *hnode;
    sk_hentry_type_t *htype;
    int rv = SKHEADER_OK;
    ssize_t sz;
    ssize_t said;
    size_t len;
    size_t bufsize;
    uint8_t *buf;
    int i;
    uint32_t tmp32;
    uint16_t tmp16;
    uint32_t pad_len;
    uint8_t *pos;

    assert(hdr);

    /* create a buffer that the header will be packed into */
    bufsize = HENTRY_INIT_BUFSIZE;
    buf = (uint8_t*)malloc(bufsize);
    if (NULL == buf) {
        rv = SKHEADER_ERR_ALLOC;
        goto END;
    }

    hdr->header_length = 0;

    hstart = &(hdr->fh_start);

    /* make certain file format is valid */
    if (!sksiteFileformatIsValid(hstart->file_format)) {
        rv = SKHEADER_ERR_BAD_FORMAT;
        goto END;
    }
    /* make certain compression is available */
    if (SK_COMPMETHOD_IS_AVAIL != sksiteCompmethodCheck(hstart->comp_method)) {
        rv = SKHEADER_ERR_BAD_COMPRESSION;
    }

    /* we cannot write old versions of the headers */
    if (hstart->file_version != SKHDR_EXPANDED_INIT_VERS) {
        rv = SKHEADER_ERR_BAD_VERSION;
        skAppPrintErr("Cannont write header version %u",
                      (unsigned)hstart->file_version);
        goto END;
    }

    /* check for valid record size */
    if (hstart->rec_size == 0) {
        hstart->rec_size = 1;
    }

    /* Padding modulus of 0 is the record size */
    if (hdr->padding_modulus == 0) {
        hdr->padding_modulus = hstart->rec_size;
    }

    /* pack the header_start into 'buf' */
    memcpy(buf, hstart, 8);
    tmp32 = htonl(hstart->silk_version);
    memcpy(&(buf[8]), &tmp32, 4);
    tmp16 = htons(hstart->rec_size);
    memcpy(&(buf[12]), &tmp16, 2);
    tmp16 = htons(hstart->rec_version);
    memcpy(&(buf[14]), &tmp16, 2);

    len = sizeof(sk_header_start_t);
    said = skStreamWrite(stream, buf, len);
    if (said != (ssize_t)len) {
        rv = -1;
        goto END;
    }
    hdr->header_length += len;

    /* we have a circular linked list; where the rootnode always
     * exists---it is the end-of-header marker */
    hnode = hdr->fh_rootnode;
    do {
        hnode = hnode->hen_next;
        hentry = hnode->hen_entry;

        /* call the appropriate function to pack the header_entry */
        htype = skHentryTypeLookup(skHeaderEntryGetTypeId(hentry));

        /* potentially loop twice; first time may fail if the buffer
         * is too small */
        for (i = 0; i < 2; ++i) {
            memset(buf, 0, bufsize);

            if (HENTRY_SPEC_EOH(hentry)) {
                /* handle final entry */
                sz = sizeof(sk_header_entry_spec_t);
                if (hdr->padding_modulus > 1) {
                    pad_len = hdr->padding_modulus - ((hdr->header_length + sz)
                                                      % hdr->padding_modulus);
                    if (pad_len != hdr->padding_modulus) {
                        sz += pad_len;
                    }
                }
                if (sz < (int)bufsize) {
                    tmp32 = htonl(sz);
                    memcpy(&buf[4], &tmp32, 4);
                }
            } else if (htype && htype->het_packer) {
                sz = htype->het_packer(hentry, buf, bufsize);
            } else {
                sz = skHentryDefaultPacker(hentry, buf, bufsize);
            }

            if (sz < 0) {
                rv = SKHEADER_ERR_ENTRY_PACK;
                break;
            }

            len = (size_t)sz;
            if (len <= bufsize) {
                /* packing was successful */
                break;
            }

            /* buffer is too small; grow it to 'len' and try again */
            pos = buf;
            buf = (uint8_t*)realloc(buf, len);
            if (!buf) {
                buf = pos;
                rv = SKHEADER_ERR_ALLOC;
                goto END;
            }
            bufsize = len;
        }

        if (rv != SKHEADER_OK) {
            /* error */
            goto END;
        }

        /* write the bytes from buffer */
        pos = buf;
        while (len > 0) {
            said = skStreamWrite(stream, pos, len);
            if (said <= 0) {
                rv = -1;
                goto END;
            }
            len -= said;
            pos += said;
            hdr->header_length += said;
        }
    } while (!HENTRY_SPEC_EOH(hnode->hen_entry));

  END:
    if (buf) {
        free(buf);
    }
    return rv;
}


/*  Header Entry Types */


/* Register a header type */
int
skHentryTypeRegister(
    sk_hentry_type_id_t     entry_id,
    sk_hentry_pack_fn_t     pack_fn,
    sk_hentry_unpack_fn_t   unpack_fn,
    sk_hentry_copy_fn_t     copy_fn,
    sk_hentry_callback_fn_t free_fn,
    sk_hentry_print_fn_t    print_fn)
{
    sk_hentry_type_t *new_reg;

    if (0 == entry_id) {
        return SKHEADER_ERR_INVALID_ID;
    }
    if (skHentryTypeLookup(entry_id)) {
        return SKHEADER_ERR_INVALID_ID;
    }

    new_reg = (sk_hentry_type_t*)calloc(1, sizeof(sk_hentry_type_t));
    if (NULL == new_reg) {
        return SKHEADER_ERR_ALLOC;
    }
    new_reg->het_id = entry_id;
    new_reg->het_packer = pack_fn;
    new_reg->het_unpacker = unpack_fn;
    new_reg->het_copy = copy_fn;
    new_reg->het_free = free_fn;
    new_reg->het_print = print_fn;
    new_reg->het_next = hentry_type_list;
    hentry_type_list = new_reg;

    return SKHEADER_OK;
}


sk_hentry_type_t *
skHentryTypeLookup(
    sk_hentry_type_id_t entry_id)
{
    sk_hentry_type_t *htype = NULL;

    for (htype = hentry_type_list; htype != NULL; htype = htype->het_next) {
        if (htype->het_id == entry_id) {
            break;
        }
    }

    return htype;
}


/*  Default pack/unpack rules */


static sk_header_entry_t *
skHentryDefaultCopy(
    const sk_header_entry_t    *hentry)
{
    sk_header_entry_t *new_hentry;
    uint32_t len;

    /* create space for new header */
    new_hentry = (sk_header_entry_t*)calloc(1, sizeof(sk_header_entry_t));
    if (NULL == new_hentry) {
        return NULL;
    }

    /* copy the spec */
    memcpy(&(new_hentry->he_spec), &(hentry->he_spec),
           sizeof(sk_header_entry_spec_t));

    /* create byte array to hold rest of data */
    len = hentry->he_spec.hes_len;
    if (len < sizeof(sk_header_entry_spec_t)) {
        free(new_hentry);
        return NULL;
    }
    len -= sizeof(sk_header_entry_spec_t);
    if (len == 0) {
        new_hentry->he_data = NULL;
    } else {
        new_hentry->he_data = malloc(len);
        if (NULL == new_hentry->he_data) {
            free(new_hentry);
            return NULL;
        }
        memcpy(new_hentry->he_data, hentry->he_data, len);
    }

    return new_hentry;
}


/*
 *  skHentryDefaultFree(hentry);
 *
 *    Free the memory associated with the header entry 'hentry.'
 */
static void
skHentryDefaultFree(
    sk_header_entry_t  *hentry)
{
    if (hentry == NULL) {
        return;
    }

    if (hentry->he_data) {
        free(hentry->he_data);
        hentry->he_data = NULL;
    }
    hentry->he_spec.hes_id = UINT32_MAX;
    free(hentry);
}


/*
 *  size = skHentryDefaultPacker(in_hentry, out_packed, bufsize);
 *
 *    Copies the data from 'in_hentry' to 'out_packed', converting the
 *    header entry spec portion of the header to network byte order.
 *    The data section of the header entry is copied as-is.  The
 *    caller should specify 'bufsize' as the size of 'out_packed'; the
 *    return value is the number of bytes written to 'out_packed'.  If
 *    the return value is larger than 'bufsize', 'out_packed' was too
 *    small to hold the header.
 */
static ssize_t
skHentryDefaultPacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize)
{
    sk_header_entry_spec_t *spec;
    uint32_t tmp;

    spec = &(in_hentry->he_spec);
    tmp = spec->hes_len;
    if (bufsize < tmp) {
        return tmp;
    }

    /* copy the data */
    memcpy(&(out_packed[8]), in_hentry->he_data, tmp);

    /* copy the length */
    tmp = htonl(tmp);
    memcpy(&(out_packed[4]), &tmp, 4);

    /* copy the ID */
    tmp = htonl(spec->hes_id);
    memcpy(&(out_packed[0]), &tmp, 4);

    return spec->hes_len;
}


/*
 *  skHentryDefaultPrint(hentry, fh);
 *
 *    Print the header entry 'hentry' as human readable text to the
 *    file handle 'fh'.
 */
static void
skHentryDefaultPrint(
    sk_header_entry_t  *hentry,
    FILE               *fh)
{
    fprintf(fh, ("unknown; length %" PRIu32), hentry->he_spec.hes_len);
}


/*
 *  hentry = skHentryDefaultUnacker(in_packed);
 *
 *    Creates a new header entry, copies the data from 'in_packed'
 *    into the new header entry, and returns the new header entry.
 *    The header entry spec will be in native byte order.  The data
 *    section of the header entry is copied as-is into newly allocated
 *    memory.  The function returns NULL if memory allocation fails or
 *    if the header entry's size is less then the mimimum.
 */
static sk_header_entry_t *
skHentryDefaultUnpacker(
    uint8_t            *in_packed)
{
    sk_header_entry_t *hentry;
    uint32_t len;

    assert(in_packed);

    /* create space for new header */
    hentry = (sk_header_entry_t*)calloc(1, sizeof(sk_header_entry_t));
    if (NULL == hentry) {
        return NULL;
    }

    /* copy the spec */
    SK_HENTRY_SPEC_UNPACK(&(hentry->he_spec), in_packed);

    /* create byte array to hold rest of data */
    len = hentry->he_spec.hes_len;
    if (len < sizeof(sk_header_entry_spec_t)) {
        free(hentry);
        return NULL;
    }
    len -= sizeof(sk_header_entry_spec_t);
    if (len == 0) {
        hentry->he_data = NULL;
    } else {
        hentry->he_data = malloc(len);
        if (NULL == hentry->he_data) {
            free(hentry);
            return NULL;
        }
        memcpy(hentry->he_data, &(in_packed[sizeof(sk_header_entry_spec_t)]),
               len);
    }

    return hentry;
}


/*
 *
 *  Packed File Headers
 *
 */


int
skHeaderAddPackedfile(
    sk_file_header_t   *hdr,
    sktime_t            start_time,
    flowtypeID_t        flowtype_id,
    sensorID_t          sensor_id)
{
    int rv;
    sk_header_entry_t *pfh;

    pfh = skHentryPackedfileCreate(start_time, flowtype_id, sensor_id);
    if (pfh == NULL) {
        return SKHEADER_ERR_ALLOC;
    }

    rv = skHeaderAddEntry(hdr, pfh);
    if (rv) {
        skHentryPackedfileFree(pfh);
    }
    return rv;
}


sk_header_entry_t *
skHentryPackedfileCopy(
    const sk_header_entry_t    *hentry)
{
    sk_hentry_packedfile_t *pf_hdr = (sk_hentry_packedfile_t*)hentry;

    assert(hentry);
    return skHentryPackedfileCreate(skHentryPackedfileGetStartTime(pf_hdr),
                                    skHentryPackedfileGetFlowtypeID(pf_hdr),
                                    skHentryPackedfileGetSensorID(pf_hdr));
}


sk_header_entry_t *
skHentryPackedfileCreate(
    sktime_t            start_time,
    flowtypeID_t        flowtype_id,
    sensorID_t          sensor_id)
{
    sk_hentry_packedfile_t *pf_hdr;

    pf_hdr = (sk_hentry_packedfile_t*)calloc(1, sizeof(sk_hentry_packedfile_t));
    if (NULL == pf_hdr) {
        return NULL;
    }

    pf_hdr->he_spec.hes_id   = SK_HENTRY_PACKEDFILE_ID;
    pf_hdr->he_spec.hes_len  = sizeof(sk_hentry_packedfile_t);
    pf_hdr->start_time       = start_time - (start_time % 3600000);
    pf_hdr->flowtype_id      = flowtype_id;
    pf_hdr->sensor_id        = sensor_id;

    return (sk_header_entry_t*)pf_hdr;
}


void
skHentryPackedfileFree(
    sk_header_entry_t  *hentry)
{
    /* allocated in a single block */
    if (hentry) {
        assert(skHeaderEntryGetTypeId(hentry) == SK_HENTRY_PACKEDFILE_ID);
        hentry->he_spec.hes_id = UINT32_MAX;
        free(hentry);
    }
}


ssize_t
skHentryPackedfilePacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize)
{
    sk_hentry_packedfile_t *pf_hdr = (sk_hentry_packedfile_t*)in_hentry;
    sk_hentry_packedfile_t tmp_hdr;

    assert(pf_hdr);
    assert(out_packed);
    assert(skHeaderEntryGetTypeId(pf_hdr) == SK_HENTRY_PACKEDFILE_ID);

    if (bufsize >= sizeof(sk_hentry_packedfile_t)) {
        SK_HENTRY_SPEC_PACK(&tmp_hdr, &(pf_hdr->he_spec));
        tmp_hdr.start_time   = hton64(pf_hdr->start_time);
        tmp_hdr.flowtype_id  = htonl(pf_hdr->flowtype_id);
        tmp_hdr.sensor_id    = htonl(pf_hdr->sensor_id);

        memcpy(out_packed, &tmp_hdr, sizeof(sk_hentry_packedfile_t));
    }

    return sizeof(sk_hentry_packedfile_t);
}


void
skHentryPackedfilePrint(
    sk_header_entry_t  *hentry,
    FILE               *fh)
{
    sk_hentry_packedfile_t *pf_hdr = (sk_hentry_packedfile_t*)hentry;
    char buf[512];

    assert(skHeaderEntryGetTypeId(pf_hdr) == SK_HENTRY_PACKEDFILE_ID);
    fprintf(fh, "%s ",
            sktimestamp_r(buf, skHentryPackedfileGetStartTime(pf_hdr),
                          SKTIMESTAMP_NOMSEC));

    sksiteFlowtypeGetName(buf, sizeof(buf),
                          skHentryPackedfileGetFlowtypeID(pf_hdr));
    fprintf(fh, "%s ", buf);

    sksiteSensorGetName(buf, sizeof(buf),
                        skHentryPackedfileGetSensorID(pf_hdr));
    fprintf(fh, "%s", buf);
}


sk_header_entry_t *
skHentryPackedfileUnpacker(
    uint8_t            *in_packed)
{
    sk_hentry_packedfile_t *pf_hdr;

    assert(in_packed);

    /* create space for new header */
    pf_hdr = (sk_hentry_packedfile_t*)calloc(1, sizeof(sk_hentry_packedfile_t));
    if (NULL == pf_hdr) {
        return NULL;
    }

    /* copy the spec */
    SK_HENTRY_SPEC_UNPACK(&(pf_hdr->he_spec), in_packed);
    assert(skHeaderEntryGetTypeId(pf_hdr) == SK_HENTRY_PACKEDFILE_ID);

    /* copy the data */
    if (pf_hdr->he_spec.hes_len != sizeof(sk_hentry_packedfile_t)) {
        free(pf_hdr);
        return NULL;
    }
    memcpy(&(pf_hdr->start_time), &(in_packed[sizeof(sk_header_entry_spec_t)]),
           sizeof(sk_hentry_packedfile_t)-sizeof(sk_header_entry_spec_t));
    pf_hdr->start_time   = ntoh64(pf_hdr->start_time);
    pf_hdr->flowtype_id  = ntohl(pf_hdr->flowtype_id);
    pf_hdr->sensor_id    = ntohl(pf_hdr->sensor_id);

    return (sk_header_entry_t*)pf_hdr;
}



/*
 *
 *  Invocation (Command Line) History
 *
 */


int
skHeaderAddInvocation(
    sk_file_header_t   *hdr,
    int                 strip_path,
    int                 argc,
    char              **argv)
{
    int rv;
    sk_header_entry_t *ci_hdr;

    ci_hdr = skHentryInvocationCreate(strip_path, argc, argv);
    if (ci_hdr == NULL) {
        return SKHEADER_ERR_ALLOC;
    }

    rv = skHeaderAddEntry(hdr, ci_hdr);
    if (rv) {
        skHentryInvocationFree(ci_hdr);
    }
    return rv;
}


sk_header_entry_t *
skHentryInvocationCopy(
    const sk_header_entry_t    *hentry)
{
    const sk_hentry_invocation_t *ci_hdr = (sk_hentry_invocation_t*)hentry;

    return skHentryInvocationCreate(0, 1, (char**)&(ci_hdr->command_line));
}


sk_header_entry_t *
skHentryInvocationCreate(
    int                 strip_path,
    int                 argc,
    char              **argv)
{
    const char *libtool_prefix = "lt-";
    sk_hentry_invocation_t *ci_hdr;
    int len = 0;
    int sz;
    char *cp;
    char *appname = argv[0];
    int i;

    if (strip_path) {
        /* remove pathname from the application */
        cp = strrchr(appname, '/');
        if (cp) {
            appname = cp + 1;
            if (*appname == '\0') {
                return NULL;
            }
        }
        /* handle libtool's "lt-" prefix */
        if ((strlen(appname) > strlen(libtool_prefix))
            && (0 == strncmp(appname, libtool_prefix, strlen(libtool_prefix))))
        {
            appname += 3;
        }
    }

    /* get length of header */
    len = strlen(appname) + 1;
    for (i = 1; i < argc; ++i) {
        len += strlen(argv[i]) + 1;
    }

    /* create the header */
    ci_hdr = (sk_hentry_invocation_t*)calloc(1, sizeof(sk_hentry_invocation_t));
    if (NULL == ci_hdr) {
        return NULL;
    }
    ci_hdr->he_spec.hes_id   = SK_HENTRY_INVOCATION_ID;
    ci_hdr->he_spec.hes_len  = sizeof(sk_header_entry_spec_t) + len;

    /* allocate the buffer for command line */
    ci_hdr->command_line = (char*)calloc(len, sizeof(char));
    if (NULL == ci_hdr->command_line) {
        free(ci_hdr);
        return NULL;
    }

    /* copy the command line into place */
    cp = ci_hdr->command_line;
    sz = strlen(appname);
    strncpy(cp, appname, len);
    cp += sz;
    len -= sz;
    for (i = 1; i < argc; ++i) {
        *cp = ' ';
        ++cp;
        --len;
        sz = strlen(argv[i]);
        strncpy(cp, argv[i], len);
        cp += sz;
        len -= sz;
    }

    return (sk_header_entry_t*)ci_hdr;
}


void
skHentryInvocationFree(
    sk_header_entry_t  *hentry)
{
    sk_hentry_invocation_t *ci_hdr = (sk_hentry_invocation_t*)hentry;

    if (ci_hdr) {
        assert(skHeaderEntryGetTypeId(ci_hdr) == SK_HENTRY_INVOCATION_ID);
        ci_hdr->he_spec.hes_id = UINT32_MAX;
        if (ci_hdr->command_line) {
            free(ci_hdr->command_line);
            ci_hdr->command_line = NULL;
        }
        free(ci_hdr);
    }
}


ssize_t
skHentryInvocationPacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize)
{
    sk_hentry_invocation_t *ci_hdr = (sk_hentry_invocation_t*)in_hentry;
    uint32_t check_len;

    assert(in_hentry);
    assert(out_packed);
    assert(skHeaderEntryGetTypeId(ci_hdr) == SK_HENTRY_INVOCATION_ID);

    /* adjust the length recorded in the header it if it too small */
    check_len = (1 + strlen(ci_hdr->command_line)
                 + sizeof(sk_header_entry_spec_t));
    if (check_len > ci_hdr->he_spec.hes_len) {
        ci_hdr->he_spec.hes_len = check_len;
    }

    if (bufsize >= ci_hdr->he_spec.hes_len) {
        SK_HENTRY_SPEC_PACK(out_packed, &(ci_hdr->he_spec));
        memcpy(&(out_packed[sizeof(sk_header_entry_spec_t)]),
               ci_hdr->command_line,
               (ci_hdr->he_spec.hes_len - sizeof(sk_header_entry_spec_t)));
    }

    return ci_hdr->he_spec.hes_len;
}


void
skHentryInvocationPrint(
    sk_header_entry_t  *hentry,
    FILE               *fh)
{
    sk_hentry_invocation_t *ci_hdr = (sk_hentry_invocation_t*)hentry;

    assert(skHeaderEntryGetTypeId(ci_hdr) == SK_HENTRY_INVOCATION_ID);
    fprintf(fh, "%s", ci_hdr->command_line);
}


sk_header_entry_t *
skHentryInvocationUnpacker(
    uint8_t            *in_packed)
{
    sk_hentry_invocation_t *ci_hdr;
    uint32_t len;

    assert(in_packed);

    /* create space for new header */
    ci_hdr = (sk_hentry_invocation_t*)calloc(1, sizeof(sk_hentry_invocation_t));
    if (NULL == ci_hdr) {
        return NULL;
    }

    /* copy the spec */
    SK_HENTRY_SPEC_UNPACK(&(ci_hdr->he_spec), in_packed);
    assert(skHeaderEntryGetTypeId(ci_hdr) == SK_HENTRY_INVOCATION_ID);

    /* copy the data */
    len = ci_hdr->he_spec.hes_len;
    if (len < sizeof(sk_header_entry_spec_t)) {
        free(ci_hdr);
        return NULL;
    }
    len -= sizeof(sk_header_entry_spec_t);
    ci_hdr->command_line = (char*)calloc(len, sizeof(char));
    if (NULL == ci_hdr->command_line) {
        free(ci_hdr);
        return NULL;
    }
    memcpy(ci_hdr->command_line, &(in_packed[sizeof(sk_header_entry_spec_t)]),
           len);

    return (sk_header_entry_t*)ci_hdr;
}


/*
 *
 *  Annotation
 *
 */


int
skHeaderAddAnnotation(
    sk_file_header_t   *hdr,
    const char         *annotation)
{
    int rv;
    sk_header_entry_t *an_hdr;

    an_hdr = skHentryAnnotationCreate(annotation);
    if (an_hdr == NULL) {
        return SKHEADER_ERR_ALLOC;
    }

    rv = skHeaderAddEntry(hdr, an_hdr);
    if (rv) {
        skHentryAnnotationFree(an_hdr);
    }
    return rv;
}


int
skHeaderAddAnnotationFromFile(
    sk_file_header_t   *hdr,
    const char         *pathname)
{
    int rv;
    sk_header_entry_t *an_hdr;

    an_hdr = skHentryAnnotationCreateFromFile(pathname);
    if (an_hdr == NULL) {
        return SKHEADER_ERR_ALLOC;
    }

    rv = skHeaderAddEntry(hdr, an_hdr);
    if (rv) {
        skHentryAnnotationFree(an_hdr);
    }
    return rv;
}


sk_header_entry_t *
skHentryAnnotationCopy(
    const sk_header_entry_t    *hentry)
{
    const sk_hentry_annotation_t *an_hdr = (sk_hentry_annotation_t*)hentry;

    return skHentryAnnotationCreate(an_hdr->annotation);
}


sk_header_entry_t *
skHentryAnnotationCreate(
    const char         *annotation)
{
    sk_hentry_annotation_t *an_hdr;
    int len;

    if (!annotation) {
        /* always provide an annotation string */
        annotation = "";
    }
    len = 1 + strlen(annotation);

    an_hdr = (sk_hentry_annotation_t*)calloc(1, sizeof(sk_hentry_annotation_t));
    if (NULL == an_hdr) {
        return NULL;
    }
    an_hdr->he_spec.hes_id   = SK_HENTRY_ANNOTATION_ID;
    an_hdr->he_spec.hes_len  = sizeof(sk_header_entry_spec_t) + len;

    an_hdr->annotation = strdup(annotation);
    if (NULL == an_hdr->annotation) {
        free(an_hdr);
        return NULL;
    }

    return (sk_header_entry_t*)an_hdr;
}


sk_header_entry_t *
skHentryAnnotationCreateFromFile(
    const char         *pathname)
{
    sk_hentry_annotation_t *an_hdr = NULL;
    skstream_t *stream = NULL;
    char *content = NULL;
    char *pos = NULL;
    size_t bufsize = HENTRY_INIT_BUFSIZE;
    ssize_t wanted;
    ssize_t saw;
    int len = 0;

    if (!pathname || !pathname[0]) {
        return NULL;
    }

    /* open the stream */
    if (skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_TEXT)
        || skStreamBind(stream, pathname)
        || skStreamOpen(stream))
    {
        goto END;
    }

    /* create buffer to hold the stream's content */
    content = (char*)malloc(bufsize);
    if (content == NULL) {
        goto END;
    }

    /* read from the stream until we read the end of file or an error.
     * grow the content buffer as needed. */
    for (;;) {
        pos = &content[len];
        wanted = bufsize - len - 1;
        saw = skStreamRead(stream, pos, wanted);
        if (saw == -1) {
            /* error */
            goto END;
        }
        if (saw == 0) {
            /* assume end of file */
            break;
        }
        if (saw == wanted) {
            /* buffer is full, grow it */
            bufsize = 2 * bufsize;
            pos = content;
            content = (char*)realloc(content, bufsize);
            if (content == NULL) {
                free(pos);
                goto END;
            }
        }
        len += saw;
    }
    content[len] = '\0';
    ++len;

    skStreamDestroy(&stream);

    /* shink the buffer */
    pos = content;
    content = (char*)realloc(content, len);
    if (content == NULL) {
        free(pos);
        goto END;
    }

    /* create the annotion object */
    an_hdr = (sk_hentry_annotation_t*)calloc(1, sizeof(sk_hentry_annotation_t));
    if (NULL == an_hdr) {
        free(content);
        return NULL;
    }
    an_hdr->he_spec.hes_id   = SK_HENTRY_ANNOTATION_ID;
    an_hdr->he_spec.hes_len  = sizeof(sk_header_entry_spec_t) + len;
    an_hdr->annotation = content;

  END:
    skStreamDestroy(&stream);
    return (sk_header_entry_t*)an_hdr;
}


void
skHentryAnnotationFree(
    sk_header_entry_t  *hentry)
{
    sk_hentry_annotation_t *an_hdr = (sk_hentry_annotation_t*)hentry;

    if (an_hdr) {
        assert(skHeaderEntryGetTypeId(an_hdr) == SK_HENTRY_ANNOTATION_ID);
        an_hdr->he_spec.hes_id = UINT32_MAX;
        if (an_hdr->annotation) {
            free(an_hdr->annotation);
            an_hdr->annotation = NULL;
        }
        free(an_hdr);
    }
}


ssize_t
skHentryAnnotationPacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize)
{
    sk_hentry_annotation_t *an_hdr = (sk_hentry_annotation_t*)in_hentry;
    uint32_t check_len;

    assert(in_hentry);
    assert(out_packed);
    assert(skHeaderEntryGetTypeId(an_hdr) == SK_HENTRY_ANNOTATION_ID);

    /* adjust the length recorded in the header it if it too small */
    check_len = (1 + strlen(an_hdr->annotation)
                 + sizeof(sk_header_entry_spec_t));
    if (check_len > an_hdr->he_spec.hes_len) {
        an_hdr->he_spec.hes_len = check_len;
    }

    if (bufsize >= an_hdr->he_spec.hes_len) {
        SK_HENTRY_SPEC_PACK(out_packed, &(an_hdr->he_spec));
        memcpy(&(out_packed[sizeof(sk_header_entry_spec_t)]),
               an_hdr->annotation,
               (an_hdr->he_spec.hes_len - sizeof(sk_header_entry_spec_t)));
    }

    return an_hdr->he_spec.hes_len;
}


void
skHentryAnnotationPrint(
    sk_header_entry_t  *hentry,
    FILE               *fh)
{
    sk_hentry_annotation_t *an_hdr = (sk_hentry_annotation_t*)hentry;

    assert(skHeaderEntryGetTypeId(an_hdr) == SK_HENTRY_ANNOTATION_ID);
    fprintf(fh, "%s", an_hdr->annotation);
}


sk_header_entry_t *
skHentryAnnotationUnpacker(
    uint8_t            *in_packed)
{
    sk_hentry_annotation_t *an_hdr;
    uint32_t len;

    assert(in_packed);

    /* create space for new header */
    an_hdr = (sk_hentry_annotation_t*)calloc(1, sizeof(sk_hentry_annotation_t));
    if (NULL == an_hdr) {
        return NULL;
    }

    /* copy the spec */
    SK_HENTRY_SPEC_UNPACK(&(an_hdr->he_spec), in_packed);
    assert(skHeaderEntryGetTypeId(an_hdr) == SK_HENTRY_ANNOTATION_ID);

    /* copy the data */
    len = an_hdr->he_spec.hes_len;
    if (len < sizeof(sk_header_entry_spec_t)) {
        free(an_hdr);
        return NULL;
    }
    len -= sizeof(sk_header_entry_spec_t);
    an_hdr->annotation = (char*)calloc(len, sizeof(char));
    if (NULL == an_hdr->annotation) {
        free(an_hdr);
        return NULL;
    }
    memcpy(an_hdr->annotation, &(in_packed[sizeof(sk_header_entry_spec_t)]),
           len);

    return (sk_header_entry_t*)an_hdr;
}


/*
 *
 *  Probename
 *
 */


int
skHeaderAddProbename(
    sk_file_header_t   *hdr,
    const char         *probe_name)
{
    int rv;
    sk_header_entry_t *pn_hdr;

    pn_hdr = skHentryProbenameCreate(probe_name);
    if (pn_hdr == NULL) {
        return SKHEADER_ERR_ALLOC;
    }

    rv = skHeaderAddEntry(hdr, pn_hdr);
    if (rv) {
        skHentryProbenameFree(pn_hdr);
    }
    return rv;
}


sk_header_entry_t *
skHentryProbenameCopy(
    const sk_header_entry_t    *hentry)
{
    const sk_hentry_probename_t *pn_hdr = (sk_hentry_probename_t*)hentry;

    return skHentryProbenameCreate(pn_hdr->probe_name);
}


sk_header_entry_t *
skHentryProbenameCreate(
    const char         *probe_name)
{
    sk_hentry_probename_t *pn_hdr;
    int len;

    /* verify name is specified */
    if (probe_name == NULL || probe_name[0] == '\0') {
        return NULL;
    }
    len = 1 + strlen(probe_name);

    pn_hdr = (sk_hentry_probename_t*)calloc(1, sizeof(sk_hentry_probename_t));
    if (NULL == pn_hdr) {
        return NULL;
    }
    pn_hdr->he_spec.hes_id  = SK_HENTRY_PROBENAME_ID;
    pn_hdr->he_spec.hes_len = sizeof(sk_header_entry_spec_t) + len;

    pn_hdr->probe_name = strdup(probe_name);
    if (NULL == pn_hdr->probe_name) {
        free(pn_hdr);
        return NULL;
    }

    return (sk_header_entry_t*)pn_hdr;
}


void
skHentryProbenameFree(
    sk_header_entry_t  *hentry)
{
    sk_hentry_probename_t *pn_hdr = (sk_hentry_probename_t*)hentry;

    if (pn_hdr) {
        assert(skHeaderEntryGetTypeId(pn_hdr) == SK_HENTRY_PROBENAME_ID);
        pn_hdr->he_spec.hes_id = UINT32_MAX;
        if (pn_hdr->probe_name) {
            free(pn_hdr->probe_name);
            pn_hdr->probe_name = NULL;
        }
        free(pn_hdr);
    }
}


ssize_t
skHentryProbenamePacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize)
{
    sk_hentry_probename_t *pn_hdr = (sk_hentry_probename_t*)in_hentry;
    uint32_t check_len;
    uint32_t probe_name_len;

    assert(in_hentry);
    assert(out_packed);
    assert(skHeaderEntryGetTypeId(pn_hdr) == SK_HENTRY_PROBENAME_ID);

    /* adjust the length recorded in the header it if it too small */
    probe_name_len = 1 + strlen(pn_hdr->probe_name);
    check_len = probe_name_len + sizeof(sk_header_entry_spec_t);
    if (check_len > pn_hdr->he_spec.hes_len) {
        pn_hdr->he_spec.hes_len = check_len;
    }

    if (bufsize >= pn_hdr->he_spec.hes_len) {
        SK_HENTRY_SPEC_PACK(out_packed, &(pn_hdr->he_spec));
        memcpy(&(out_packed[sizeof(sk_header_entry_spec_t)]),
               pn_hdr->probe_name, probe_name_len);
    }

    return pn_hdr->he_spec.hes_len;
}


void
skHentryProbenamePrint(
    sk_header_entry_t  *hentry,
    FILE               *fh)
{
    sk_hentry_probename_t *pn_hdr = (sk_hentry_probename_t*)hentry;

    assert(skHeaderEntryGetTypeId(pn_hdr) == SK_HENTRY_PROBENAME_ID);
    fprintf(fh, "%s",
            (pn_hdr->probe_name ? pn_hdr->probe_name : "NULL"));
}


sk_header_entry_t *
skHentryProbenameUnpacker(
    uint8_t            *in_packed)
{
    sk_hentry_probename_t *pn_hdr;
    uint32_t len;

    assert(in_packed);

    /* create space for new header */
    pn_hdr = (sk_hentry_probename_t*)calloc(1, sizeof(sk_hentry_probename_t));
    if (NULL == pn_hdr) {
        return NULL;
    }

    /* copy the spec */
    SK_HENTRY_SPEC_UNPACK(&(pn_hdr->he_spec), in_packed);
    assert(skHeaderEntryGetTypeId(pn_hdr) == SK_HENTRY_PROBENAME_ID);

    /* copy the data */
    len = pn_hdr->he_spec.hes_len;
    if (len < sizeof(sk_header_entry_spec_t)) {
        free(pn_hdr);
        return NULL;
    }
    len -= sizeof(sk_header_entry_spec_t);
    pn_hdr->probe_name = (char*)calloc(len, sizeof(char));
    if (NULL == pn_hdr->probe_name) {
        free(pn_hdr);
        return NULL;
    }
    memcpy(pn_hdr->probe_name, &(in_packed[sizeof(sk_header_entry_spec_t)]),
           len);

    return (sk_header_entry_t*)pn_hdr;
}


/*
 *
 *  Prefixmap
 *
 */


int
skHeaderAddPrefixmap(
    sk_file_header_t   *hdr,
    const char         *mapname)
{
    int rv;
    sk_header_entry_t *pn_hdr;

    pn_hdr = skHentryPrefixmapCreate(mapname);
    if (pn_hdr == NULL) {
        return SKHEADER_ERR_ALLOC;
    }

    rv = skHeaderAddEntry(hdr, pn_hdr);
    if (rv) {
        skHentryPrefixmapFree(pn_hdr);
    }
    return rv;
}


sk_header_entry_t *
skHentryPrefixmapCopy(
    const sk_header_entry_t    *hentry)
{
    const sk_hentry_prefixmap_t *pn_hdr = (sk_hentry_prefixmap_t*)hentry;

    return skHentryPrefixmapCreate(pn_hdr->mapname);
}


sk_header_entry_t *
skHentryPrefixmapCreate(
    const char         *mapname)
{
    sk_hentry_prefixmap_t *pn_hdr;
    int len;

    /* verify name is specified */
    if (mapname == NULL || mapname[0] == '\0') {
        return NULL;
    }
    len = 1 + strlen(mapname);

    pn_hdr = (sk_hentry_prefixmap_t*)calloc(1, sizeof(sk_hentry_prefixmap_t));
    if (NULL == pn_hdr) {
        return NULL;
    }
    pn_hdr->he_spec.hes_id  = SK_HENTRY_PREFIXMAP_ID;
    pn_hdr->he_spec.hes_len = sizeof(sk_hentry_prefixmap_t) + len;

    pn_hdr->version = 1;
    pn_hdr->mapname = strdup(mapname);
    if (NULL == pn_hdr->mapname) {
        free(pn_hdr);
        return NULL;
    }

    return (sk_header_entry_t*)pn_hdr;
}


void
skHentryPrefixmapFree(
    sk_header_entry_t  *hentry)
{
    sk_hentry_prefixmap_t *pn_hdr = (sk_hentry_prefixmap_t*)hentry;

    if (pn_hdr) {
        assert(skHeaderEntryGetTypeId(pn_hdr) == SK_HENTRY_PREFIXMAP_ID);
        pn_hdr->he_spec.hes_id = UINT32_MAX;
        if (pn_hdr->mapname) {
            free(pn_hdr->mapname);
            pn_hdr->mapname = NULL;
        }
        free(pn_hdr);
    }
}


ssize_t
skHentryPrefixmapPacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize)
{
    sk_hentry_prefixmap_t *pn_hdr = (sk_hentry_prefixmap_t*)in_hentry;
    uint32_t check_len;
    uint32_t mapname_len;
    uint32_t version;
    uint8_t *pos;

    assert(in_hentry);
    assert(out_packed);
    assert(skHeaderEntryGetTypeId(pn_hdr) == SK_HENTRY_PREFIXMAP_ID);

    /* adjust the length recorded in the header it if it too small */
    mapname_len = 1 + strlen(pn_hdr->mapname);
    check_len = (mapname_len + sizeof(pn_hdr->version)
                 + sizeof(sk_header_entry_spec_t));
    if (check_len > pn_hdr->he_spec.hes_len) {
        pn_hdr->he_spec.hes_len = check_len;
    }

    if (bufsize >= pn_hdr->he_spec.hes_len) {
        SK_HENTRY_SPEC_PACK(out_packed, &(pn_hdr->he_spec));
        pos = out_packed + sizeof(sk_header_entry_spec_t);
        version = htonl(pn_hdr->version);
        memcpy(pos, &version, sizeof(uint32_t));
        pos += sizeof(uint32_t);
        memcpy(pos, pn_hdr->mapname, mapname_len);
    }

    return pn_hdr->he_spec.hes_len;
}


void
skHentryPrefixmapPrint(
    sk_header_entry_t  *hentry,
    FILE               *fh)
{
    sk_hentry_prefixmap_t *pn_hdr = (sk_hentry_prefixmap_t*)hentry;

    assert(skHeaderEntryGetTypeId(pn_hdr) == SK_HENTRY_PREFIXMAP_ID);
    fprintf(fh, ("v%" PRIu32 ": %s"),
            pn_hdr->version, (pn_hdr->mapname ? pn_hdr->mapname : "NULL"));
}


sk_header_entry_t *
skHentryPrefixmapUnpacker(
    uint8_t            *in_packed)
{
    sk_hentry_prefixmap_t *pn_hdr;
    uint32_t len;
    uint8_t *pos;

    assert(in_packed);

    /* create space for new header */
    pn_hdr = (sk_hentry_prefixmap_t*)calloc(1, sizeof(sk_hentry_prefixmap_t));
    if (NULL == pn_hdr) {
        return NULL;
    }

    /* copy the spec */
    SK_HENTRY_SPEC_UNPACK(&(pn_hdr->he_spec), in_packed);
    assert(skHeaderEntryGetTypeId(pn_hdr) == SK_HENTRY_PREFIXMAP_ID);

    /* copy the data */
    len = pn_hdr->he_spec.hes_len;
    if (len < sizeof(sk_header_entry_spec_t)) {
        free(pn_hdr);
        return NULL;
    }
    len -= sizeof(sk_header_entry_spec_t);
    pos = in_packed + sizeof(sk_header_entry_spec_t);

    if (len < sizeof(uint32_t)) {
        free(pn_hdr);
        return NULL;
    }
    memcpy(&(pn_hdr->version), pos, sizeof(uint32_t));
    pn_hdr->version = ntohl(pn_hdr->version);
    len -= sizeof(uint32_t);
    pos += sizeof(uint32_t);

    pn_hdr->mapname = (char*)calloc(len, sizeof(char));
    if (NULL == pn_hdr->mapname) {
        free(pn_hdr);
        return NULL;
    }
    memcpy(pn_hdr->mapname, pos, len);

    return (sk_header_entry_t*)pn_hdr;
}


/*
 *
 *  Bag
 *
 */


int
skHeaderAddBag(
    sk_file_header_t   *hdr,
    uint16_t            key_type,
    uint16_t            key_length,
    uint16_t            counter_type,
    uint16_t            counter_length)
{
    int rv;
    sk_header_entry_t *bag_hdr;

    bag_hdr = skHentryBagCreate(key_type, key_length,
                                counter_type, counter_length);
    if (bag_hdr == NULL) {
        return SKHEADER_ERR_ALLOC;
    }

    rv = skHeaderAddEntry(hdr, bag_hdr);
    if (rv) {
        skHentryBagFree(bag_hdr);
    }
    return rv;
}


sk_header_entry_t *
skHentryBagCopy(
    const sk_header_entry_t    *hentry)
{
    const sk_hentry_bag_t *bag_hdr = (sk_hentry_bag_t*)hentry;

    return skHentryBagCreate(bag_hdr->key_type, bag_hdr->key_length,
                             bag_hdr->counter_type, bag_hdr->counter_length);
}


sk_header_entry_t *
skHentryBagCreate(
    uint16_t            key_type,
    uint16_t            key_length,
    uint16_t            counter_type,
    uint16_t            counter_length)
{
    sk_hentry_bag_t *bag_hdr;

    bag_hdr = (sk_hentry_bag_t*)calloc(1, sizeof(sk_hentry_bag_t));
    if (NULL == bag_hdr) {
        return NULL;
    }
    bag_hdr->he_spec.hes_id  = SK_HENTRY_BAG_ID;
    bag_hdr->he_spec.hes_len = sizeof(sk_hentry_bag_t);
    bag_hdr->key_type        = key_type;
    bag_hdr->key_length      = key_length;
    bag_hdr->counter_type    = counter_type;
    bag_hdr->counter_length  = counter_length;

    return (sk_header_entry_t*)bag_hdr;
}


void
skHentryBagFree(
    sk_header_entry_t  *hentry)
{
    sk_hentry_bag_t *bag_hdr = (sk_hentry_bag_t*)hentry;

    if (bag_hdr) {
        assert(skHeaderEntryGetTypeId(bag_hdr) == SK_HENTRY_BAG_ID);
        bag_hdr->he_spec.hes_id = UINT32_MAX;
        free(bag_hdr);
    }
}


ssize_t
skHentryBagPacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize)
{
    sk_hentry_bag_t *bag_hdr = (sk_hentry_bag_t*)in_hentry;
    sk_hentry_bag_t tmp_hdr;

    assert(in_hentry);
    assert(out_packed);
    assert(skHeaderEntryGetTypeId(bag_hdr) == SK_HENTRY_BAG_ID);

    if (bufsize >= sizeof(sk_hentry_bag_t)) {
        SK_HENTRY_SPEC_PACK(&tmp_hdr, &(bag_hdr->he_spec));
        tmp_hdr.key_type       = htons(bag_hdr->key_type);
        tmp_hdr.key_length     = htons(bag_hdr->key_length);
        tmp_hdr.counter_type   = htons(bag_hdr->counter_type);
        tmp_hdr.counter_length = htons(bag_hdr->counter_length);

        memcpy(out_packed, &tmp_hdr, sizeof(sk_hentry_bag_t));
    }

    return sizeof(sk_hentry_bag_t);
}


void
skHentryBagPrint(
    sk_header_entry_t  *hentry,
    FILE               *fh)
{
    sk_hentry_bag_t *bag_hdr = (sk_hentry_bag_t*)hentry;
    char key_buf[64];
    char counter_buf[64];

    assert(skHeaderEntryGetTypeId(bag_hdr) == SK_HENTRY_BAG_ID);

    if (!skBagFieldTypeAsString((skBagFieldType_t)bag_hdr->key_type,
                                key_buf, sizeof(key_buf)))
    {
        snprintf(key_buf, sizeof(key_buf), "UNKNOWN[%" PRIu16 "]",
                 bag_hdr->key_type);
    }
    if (!skBagFieldTypeAsString((skBagFieldType_t)bag_hdr->counter_type,
                                counter_buf, sizeof(counter_buf)))
    {
        snprintf(counter_buf,  sizeof(counter_buf), "UNKNOWN[%" PRIu16 "]",
                 bag_hdr->counter_type);
    }

    fprintf(fh, ("key: %s @ %" PRIu16 " octets; "
                 "counter: %s @ %" PRIu16 " octets"),
            key_buf, bag_hdr->key_length,
            counter_buf, bag_hdr->counter_length);
}


sk_header_entry_t *
skHentryBagUnpacker(
    uint8_t            *in_packed)
{
    sk_hentry_bag_t *bag_hdr;

    assert(in_packed);

    /* create space for new header */
    bag_hdr = (sk_hentry_bag_t*)calloc(1, sizeof(sk_hentry_bag_t));
    if (NULL == bag_hdr) {
        return NULL;
    }

    /* copy the spec */
    SK_HENTRY_SPEC_UNPACK(&(bag_hdr->he_spec), in_packed);
    assert(skHeaderEntryGetTypeId(bag_hdr) == SK_HENTRY_BAG_ID);

    /* copy the data */
    if (bag_hdr->he_spec.hes_len != sizeof(sk_hentry_bag_t)) {
        free(bag_hdr);
        return NULL;
    }
    memcpy(&(bag_hdr->key_type),
           &(in_packed[sizeof(sk_header_entry_spec_t)]),
           sizeof(sk_hentry_bag_t) - sizeof(sk_header_entry_spec_t));
    bag_hdr->key_type       = htons(bag_hdr->key_type);
    bag_hdr->key_length     = htons(bag_hdr->key_length);
    bag_hdr->counter_type   = htons(bag_hdr->counter_type);
    bag_hdr->counter_length = htons(bag_hdr->counter_length);

    return (sk_header_entry_t*)bag_hdr;
}


/*
 *
 *  IPSet
 *
 */


int
skHeaderAddIPSet(
    sk_file_header_t   *hdr,
    uint32_t            child_node,
    uint32_t            leaf_count,
    uint32_t            leaf_size,
    uint32_t            node_count,
    uint32_t            node_size,
    uint32_t            root_idx)
{
    int rv;
    sk_header_entry_t *ipset_hdr = NULL;

    ipset_hdr = skHentryIPSetCreate(child_node, leaf_count, leaf_size,
                                  node_count, node_size, root_idx);
    if (ipset_hdr == NULL) {
        return SKHEADER_ERR_ALLOC;
    }

    rv = skHeaderAddEntry(hdr, ipset_hdr);
    if (rv) {
        skHentryIPSetFree(ipset_hdr);
    }
    return rv;
}


sk_header_entry_t *
skHentryIPSetCopy(
    const sk_header_entry_t    *hentry)
{
    const sk_hentry_ipset_t *ipset_hdr = (sk_hentry_ipset_t*)hentry;

    return skHentryIPSetCreate(ipset_hdr->child_node, ipset_hdr->leaf_count,
                               ipset_hdr->leaf_size, ipset_hdr->node_count,
                               ipset_hdr->node_size, ipset_hdr->root_idx);
}


sk_header_entry_t *
skHentryIPSetCreate(
    uint32_t            child_node,
    uint32_t            leaf_count,
    uint32_t            leaf_size,
    uint32_t            node_count,
    uint32_t            node_size,
    uint32_t            root_idx)
{
    sk_hentry_ipset_t *ipset_hdr;

    ipset_hdr = (sk_hentry_ipset_t*)calloc(1, sizeof(sk_hentry_ipset_t));
    if (NULL == ipset_hdr) {
        return NULL;
    }
    ipset_hdr->he_spec.hes_id  = SK_HENTRY_IPSET_ID;
    ipset_hdr->he_spec.hes_len = sizeof(sk_hentry_ipset_t);
    ipset_hdr->child_node      = child_node;
    ipset_hdr->leaf_count      = leaf_count;
    ipset_hdr->leaf_size       = leaf_size;
    ipset_hdr->node_count      = node_count;
    ipset_hdr->node_size       = node_size;
    ipset_hdr->root_idx        = root_idx;

    return (sk_header_entry_t*)ipset_hdr;
}


void
skHentryIPSetFree(
    sk_header_entry_t  *hentry)
{
    if (hentry) {
        assert(skHeaderEntryGetTypeId(hentry) == SK_HENTRY_IPSET_ID);
        hentry->he_spec.hes_id = UINT32_MAX;
        free(hentry);
    }
}


ssize_t
skHentryIPSetPacker(
    sk_header_entry_t  *in_hentry,
    uint8_t            *out_packed,
    size_t              bufsize)
{
    sk_hentry_ipset_t *ipset_hdr = (sk_hentry_ipset_t*)in_hentry;
    sk_hentry_ipset_t tmp_hdr;

    assert(in_hentry);
    assert(out_packed);
    assert(skHeaderEntryGetTypeId(ipset_hdr) == SK_HENTRY_IPSET_ID);

    if (bufsize >= sizeof(sk_hentry_ipset_t)) {
        SK_HENTRY_SPEC_PACK(&tmp_hdr, &(ipset_hdr->he_spec));
        tmp_hdr.child_node = htonl(ipset_hdr->child_node);
        tmp_hdr.leaf_count = htonl(ipset_hdr->leaf_count);
        tmp_hdr.leaf_size  = htonl(ipset_hdr->leaf_size);
        tmp_hdr.node_count = htonl(ipset_hdr->node_count);
        tmp_hdr.node_size  = htonl(ipset_hdr->node_size);
        tmp_hdr.root_idx   = htonl(ipset_hdr->root_idx);

        memcpy(out_packed, &tmp_hdr, sizeof(sk_hentry_ipset_t));
    }

    return sizeof(sk_hentry_ipset_t);
}


void
skHentryIPSetPrint(
    sk_header_entry_t  *hentry,
    FILE               *fh)
{
    sk_hentry_ipset_t *ipset_hdr = (sk_hentry_ipset_t*)hentry;

    assert(skHeaderEntryGetTypeId(ipset_hdr) == SK_HENTRY_IPSET_ID);
    if ((0 == ipset_hdr->child_node) && (0 == ipset_hdr->root_idx)) {
        /* assume this is a RecordVersion 4 file */
        fprintf(fh, "IPv%d",
                ((sizeof(uint32_t) == ipset_hdr->leaf_size) ? 4 : 6));
    } else {
        fprintf(fh, ("%" PRIu32 "-way branch, root@%" PRIu32 ", "
                     "%" PRIu32 " x %" PRIu32 "b node%s, "
                     "%" PRIu32 " x %" PRIu32 "b leaves"),
            ipset_hdr->child_node, ipset_hdr->root_idx,
                ipset_hdr->node_count, ipset_hdr->node_size,
                ((ipset_hdr->node_count > 1) ? "s" : ""),
                ipset_hdr->leaf_count, ipset_hdr->leaf_size);
    }
}


sk_header_entry_t *
skHentryIPSetUnpacker(
    uint8_t            *in_packed)
{
    sk_hentry_ipset_t *ipset_hdr;

    assert(in_packed);

    /* create space for new header */
    ipset_hdr = (sk_hentry_ipset_t*)calloc(1, sizeof(sk_hentry_ipset_t));
    if (NULL == ipset_hdr) {
        return NULL;
    }

    /* copy the spec */
    SK_HENTRY_SPEC_UNPACK(&(ipset_hdr->he_spec), in_packed);
    assert(skHeaderEntryGetTypeId(ipset_hdr) == SK_HENTRY_IPSET_ID);

    /* copy the data */
    if (ipset_hdr->he_spec.hes_len != sizeof(sk_hentry_ipset_t)) {
        free(ipset_hdr);
        return NULL;
    }
    memcpy(&(ipset_hdr->child_node),
           &(in_packed[sizeof(sk_header_entry_spec_t)]),
           sizeof(sk_hentry_ipset_t) - sizeof(sk_header_entry_spec_t));
    ipset_hdr->child_node  = htonl(ipset_hdr->child_node);
    ipset_hdr->leaf_count  = htonl(ipset_hdr->leaf_count);
    ipset_hdr->leaf_size   = htonl(ipset_hdr->leaf_size);
    ipset_hdr->node_count  = htonl(ipset_hdr->node_count);
    ipset_hdr->node_size   = htonl(ipset_hdr->node_size);
    ipset_hdr->root_idx    = htonl(ipset_hdr->root_idx);

    return (sk_header_entry_t*)ipset_hdr;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
