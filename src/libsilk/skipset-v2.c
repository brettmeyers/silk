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

/*
**  skipset.c
**
**    skipset.c provides a data structure for maintaining IP
**    addresses.  The implementation uses a Radix Tree (aka Patricia
**    Trie) to keep IP addresses and their prefixes.  The
**    implementation can support IPv4 or IPv6 addresses, though each
**    instance of an IPset can only hold one type of IP address.
**
**    skipset.c is a replacement for the skIPTree_t data structure
**    defined in iptree.c.
**
**    Mark Thomas
**    Febrary 2011
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skipset-v2.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/iptree.h>
#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/skipset.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/skvector.h>
#include <silk/utils.h>


/*
**  IMPLEMENTATION
**
**    The IPset code is implemented as a type of Radix Tree, where
**    internal members of the tree are "nodes" and the terminal
**    members are "leaves".  Within an IPset instance, all nodes are
**    allocated in one array, and all leaves are allocated in another
**    array.  The Radix Tree is not binary; each node points to
**    IPSET_NUM_CHILDREN other nodes or leaves, and NUM_BITS is the
**    number of bits of an IP address that must be examined to
**    determine which branch to follow from a node.
**
**    The IPv4 node structure is given here and described below:
**
**    struct ipset_node_v4_st {
**        uint32_t        child[IPSET_NUM_CHILDREN];
**
**        SET_BMAP_DECLARE(child_is_leaf, IPSET_NUM_CHILDREN);
**
**        SET_BMAP_DECLARE(child_repeated, IPSET_NUM_CHILDREN);
**
**        uint8_t         prefix;
**
**        // reserved/unsed bytes
**        uint8_t         reserved3;
**        uint8_t         reserved2;
**        uint8_t         reserved1;
**
**        uint32_t        ip;
**    };
**
**    Each node has an 'ip' member to hold a complete IP address; a
**    'prefix' value in the node says how many of the bits in that IP
**    address are valid.  The IP/prefix on a node provides the lower
**    and upper bounds for the CIDR blocks of the children of the
**    node.  The prefix of a node will always be an even multiple of
**    the NUM_BITS value of the IPset.
**
**    IPv4 addresses are stored as uint32_t's in native byte order.
**    IPv6 addresses are stored as an ipset_ipv6_t, which contains 2
**    uint64_t's in native byte order.  The 'ip' value appears at the
**    bottom of the node, so that the layout of IPv4 nodes and IPv6
**    nodes (ipset_node_v6_st) are identical except for the IP.
**
**    Instead of nodes having pointers to other nodes or leaves, nodes
**    contain the 'child[]' array of integer values that are indexes
**    into either the array of nodes or the array of leaves.  Since a
**    given index may refer to either the node array or the leaf
**    array, nodes also contain the 'child_is_leaf' bitmap that says
**    which array the index references.
**
**    If a node points to a leaf where the leaf's prefix is
**    numerically less than (node->prefix + NUM_BITS), then more than
**    one child[] entry will point to the same leaf index.  When this
**    occurs, the lowest entry in the child[] array that points to
**    this leaf is considered the "real" entry, and the other indexes
**    to this leaf in child[] will also have their bit set within the
**    'child_repeated' bitmap on the node.
**
**    Assume an IPset where NUM_BITS is set to 4, so that each node
**    points to 16 children.  Also assume a node that contains
**    2.0.0.0/8.  If the child[] array points to any nodes, the prefix
**    on those nodes must not be larger than a /12 (that is, the value
**    of the prefix must be 12 or numerically greater).  Suppose the
**    IPset contains the value 2.32.0.0/11.  child[2] and child[3]
**    will both contain the index of the leaf; bits 2 and 3 of the
**    'child_is_leaf' bitmap will be set, and bit 3 of the
**    'child_repeated' bitmap will be set.
**
**    When a node is removed from the IPset such that it is no longer
**    needed, the node is added to a 'free_list' of nodes, which is
**    maintained by the IPset.  The list is implemented as a stack,
**    where child[0] of each node contains the index of the previous
**    node in the stack.  The index zero is reserved to mean the
**    free_list is empty.
**
**    The structure of the leaf for IPv4 is given here:
**
**    struct ipset_leaf_v4_st {
**        uint8_t         prefix;
**
**        // reserved/unsed bytes
**        uint8_t         reserved3;
**        uint8_t         reserved2;
**        uint8_t         reserved1;
**
**        uint32_t        ip;
**    };
**
**    The leaf contains only the IP and the prefix.
**
**    The IPset also maintains a 'free_list' of leaves that is
**    maintained as a stack.  For leaves, the 'ip' member is used as
**    the index to the previous leaf in the stack.
**
**    If an IPset is marked as 'clean', the array of leaves contains
**    the leaves in sorted order.  This allows for fast iteration over
**    the leaves of the IPset and allows streaming of the IPset.
**    Additional properties of a 'clean' IPset are (a)Contiguous
**    leaves have been combined to contain the largest possible CIDR
**    block, (b)The leaves array contains no holes (that is, the
**    'free_list' of leaves is empty), and (c)The nodes array contains
**    no holes.
**
**    An IPset can be made clean by calling skIPSetClean(); some
**    operations on the IPset require that the set be clean before it
**    can be operated on.
**
**    The root of the tree can be any node or leaf.  The index of the
**    root is specified in the skipset_t structure.  The skipset_t
**    structure also contains a flag denoting whether the index is a
**    node or a leaf.
**
**    The on-disk storage matches the in-core storage.  This allows us
**    to mmap() the data section of the file when reading a set---as
**    long as the set is in native byte order and the data section is
**    not compressed.
**
**
**    The IPset structure is currently optimized to hold large CIDR
**    blocks.  When a large number of widely spaced individual IPs are
**    given, the size of the IPset will explode, since each IP is
**    represented by an ipset_leaf_vX_t structure that is twice as
**    large as the individual IP address.  We need to allow some way
**    for the leaves of the IPset to hold multiple IPs.  For example,
**    if the leaves held a bitmap of which IPs were set.
**
**
*/


/* LOCAL DEFINES AND TYPEDEFS */

/* Current IPset file format version */
#define  IPSET_RECORD_VERSION          3

/* First file format version that uses this same format */
#define  IPSET_RECORD_VERSION_MINIMUM  3

/* Legacy version of the IPset file (the IPtree) */
#define  IPSET_RECORD_VERSION_LEGACY   2

#ifdef   NDEBUG
#define  ASSERT_OK(func_call)  func_call
#else
#define  ASSERT_OK(func_call)                   \
    {                                           \
        int assert_ok = func_call;              \
        assert(0 == assert_ok);                 \
    }
#endif


/*  The number of uint32_t values for each /24.  Value is computed by
 *  ((1 << 8) / (1 << 5)) ==> (1 << 3) ==> 8 */
#define WORDS_PER_SLASH24  8

#define skIPTreeNodeSet(addr, node)                                     \
    ((node)->addressBlock[(((addr) & 0xFFFF)>>5)] |= (1 << ((addr)&0x1F)))

#define skIPTreeNodeClear(addr, node)                                   \
    ((node)->addressBlock[(((addr) & 0xFFFF)>>5)] &= ~(1 << ((addr)&0x1F)))

#define IPTREE_NODE_ALLOC(high16)                                       \
    if (ipset->iptree->nodes[(high16)]) { /*no-op*/ } else {            \
        ipset->iptree->nodes[(high16)]                                  \
            = (skIPNode_t*)calloc(1, sizeof(skIPNode_t));               \
        if (NULL == ipset->iptree->nodes[(high16)]) {                   \
            return SKIPSET_ERR_ALLOC;                                   \
        }                                                               \
    }



/* THE IPSET Structure */
struct skipset_st {
    skIPTree_t                 *iptree;
    /* options used when writing an IPset */
    const skipset_options_t    *options;
    /* whether the tree has changed since last call to skIPSetClean() */
    unsigned                    is_dirty:1;
    /* whether an attempt to insert an IPv6 address into an IPv4 tree
     * should returnan error.  when this is 0, the tree will
     * automatically be converted to IPv6. */
    unsigned                    no_autoconvert :1;
};




typedef int (ipset_walk_v4_fn_t)(
    uint32_t            ipv4,
    uint32_t            prefix,
    void               *cb_data);


/* Support strucuture for writing an IPset in the old IPTree format;
 * used by ipsetWriteIPTree() and ipsetWriteIPTreeCallback(). */
typedef struct ipset_write_iptree_st {
    /* the stream to write to */
    skstream_t *stream;
    /* the current /24 is in buffer[0]; remaining values are a bitmap
     * of addresses for that /24. */
    uint32_t    buffer[9];
    /* true when the buffer contains valid data */
    unsigned    buffer_is_dirty :1;
} ipset_write_iptree_t;


/*  IPSET OPTIONS  */

enum ipset_options_en {
    OPT_IPSET_RECORD_VERSION,
    OPT_IPSET_INVOCATION_STRIP
};
static const struct option ipset_options[] = {
    {"record-version",      REQUIRED_ARG, 0, OPT_IPSET_RECORD_VERSION},
    {"invocation-strip",    NO_ARG,       0, OPT_IPSET_INVOCATION_STRIP},
    {0, 0, 0, 0}            /* sentinel entry */
};
static const char *ipset_options_help[] = {
    "Specify version to use for writing IPset records.",
    "Strip invocation history from the IPset file. Def. no",
    (char*)NULL
};


/* FUNCTION DEFINITIONS */

/*
 *  status = ipsetOptionsHandler(cData, opt_index, opt_arg);
 *
 *    Parse an option that was registered by skIPSetOptionsRegister().
 *    Return 0 on success, or non-zero on failure.
 */
static int
ipsetOptionsHandler(
    clientData          cData,
    int                 opt_index,
    char               *opt_arg)
{
    skipset_options_t *ipset_opts = (skipset_options_t*)cData;
    uint32_t tmp32;
    int rv;

    switch (opt_index) {
      case OPT_IPSET_RECORD_VERSION:
        rv = skStringParseUint32(&tmp32, opt_arg, IPSET_RECORD_VERSION_LEGACY,
                                 IPSET_RECORD_VERSION_LEGACY);
        if (rv) {
            goto PARSE_ERROR;
        }
        ipset_opts->record_version = (uint16_t)tmp32;
        break;

      case OPT_IPSET_INVOCATION_STRIP:
        ipset_opts->invocation_strip = 1;
        break;

      default:
        skAbortBadCase(opt_index);
    }

    return 0;

  PARSE_ERROR:
    skAppPrintErr("Invalid %s '%s': %s",
                  ipset_options[opt_index].name, opt_arg,
                  skStringParseStrerror(rv));
    return -1;
}


/*
 *  status = ipsetCreate(ipset);
 *
 *    Create a new IPset at the address specified by 'ipset'.  Return
 *    SKIPSET_OK on success, or SKIPSET_ERR_ALLOC for memory
 *    allocation failure.
 */
static int
ipsetCreate(
    skipset_t         **ipset)
{
    int rv = 0;

    *ipset = (skipset_t*)calloc(1, sizeof(skipset_t));
    if (!*ipset) {
        return SKIPSET_ERR_ALLOC;
    }
    rv = skIPTreeCreate(&((*ipset)->iptree));
    if (SKIP_ERR_ALLOC == rv) {
        free(*ipset);
        *ipset = NULL;
        return SKIPSET_ERR_ALLOC;
    }
    assert(SKIP_OK == rv);
    return SKIPSET_OK;
}


static int
ipsetInsertAddress(
    skipset_t          *ipset,
    uint32_t            ipv4,
    uint32_t            prefix)
{
    uint32_t ipv4_end;

    if (32 == prefix) {
        IPTREE_NODE_ALLOC(ipv4 >> 16);
        skIPTreeNodeSet(ipv4, ipset->iptree->nodes[ipv4 >> 16]);

    } else if (prefix <= 16) {
        ipv4_end = ((UINT32_MAX >> prefix) | ipv4) >> 16;
        ipv4 >>= 16;
        do {
            IPTREE_NODE_ALLOC(ipv4);
            memset(ipset->iptree->nodes[ipv4]->addressBlock,
                   0xFF, sizeof(skIPNode_t));
        } while (ipv4++ < ipv4_end);

    } else if (prefix <= 27) {
        ipv4_end = ((UINT32_MAX >> prefix) | ipv4) >> 5;
        ipv4 >>= 5;
        IPTREE_NODE_ALLOC(ipv4 >> 11);
        do {
            /* nodes are allocated on /16, already shifted by 5, need
             * to shift by 11 more to get the complete shift by 16.
             * Mask by 0x7ff (0xffff>>5) to get addressBlock */
            memset(&ipset->iptree->nodes[ipv4>>11]->addressBlock[ipv4 & 0x07FF],
                   0xFF, sizeof(uint32_t));
        } while (ipv4++ < ipv4_end);

    } else {
        ipv4_end = (UINT32_MAX >> prefix) | ipv4;
        IPTREE_NODE_ALLOC(ipv4 >> 16);
        do {
            skIPTreeNodeSet(ipv4, ipset->iptree->nodes[ipv4 >> 16]);
        } while (ipv4++ < ipv4_end);
    }
    return SKIPSET_OK;
}


/*
 *  status = ipsetReadV3(&ipset, stream, hdr);
 *
 *    Read a SiLK IPset created in the default SiLK-3.x format from
 *    'stream'.  Return SKIPSET_OK on success.
 */
static int
ipsetReadV3(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    typedef struct ipset_leaf_v4_st {
        /* prefix is number of significant bits in the IP. */
        uint8_t         prefix;
        /* reserved/unused bytes */
        uint8_t         reserved3;
        uint8_t         reserved2;
        uint8_t         reserved1;
        /* IP address */
        uint32_t        ip;
    } ipset_leaf_v4_t;

    typedef struct ipset_leaf_v6_st {
        uint8_t         prefix;
        uint8_t         reserved3;
        uint8_t         reserved2;
        uint8_t         reserved1;
        uint32_t        pad_align;
        uint64_t        ip[2];
    } ipset_leaf_v6_t;

    skipset_t *ipset = NULL;
    ipset_leaf_v4_t leaf;
    sk_header_entry_t *hentry;
    ssize_t bytes;
    ssize_t b;
    size_t count = 0;
    int rv;

    /* Ensure we are reading a file we understand */
    rv = skStreamCheckSilkHeader(stream, FT_IPSET,
                                 IPSET_RECORD_VERSION_MINIMUM,
                                 IPSET_RECORD_VERSION, NULL);
    switch (rv) {
      case SKSTREAM_OK:
        break;
      case SKSTREAM_ERR_UNSUPPORT_FORMAT:
        /* not an IPset file */
        return SKIPSET_ERR_FILETYPE;
      case SKSTREAM_ERR_UNSUPPORT_VERSION:
        return SKIPSET_ERR_FILEVERSION;
      default:
        return SKIPSET_ERR_FILEHEADER;
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        return SKIPSET_ERR_FILEHEADER;
    }
    hentry = skHeaderGetFirstMatch(hdr, SK_HENTRY_IPSET_ID);
    if (NULL == hentry) {
        return SKIPSET_ERR_FILEHEADER;
    }

    /* make certain leaf size is for IPv4 and what we expect */
    if (sizeof(ipset_leaf_v6_t) == skHentryIPSetGetLeafSize(hentry)) {
        /* IPv6 IPSet not supported by this build of SiLK */
        return SKIPSET_ERR_IPV6;
    }
    if (sizeof(ipset_leaf_v4_t) != skHentryIPSetGetLeafSize(hentry)) {
        /* Unrecognized record size */
        return SKIPSET_ERR_FILEHEADER;
    }
    /* else we are good */

    /* create the set */
    rv = ipsetCreate(&ipset);
    if (rv != 0) {
        goto END;
    }

    /* check for an empty IPset */
    if (0 == skHentryIPSetGetNodeCount(hentry)
        && 0 == skHentryIPSetGetLeafCount(hentry))
    {
        rv = SKIPSET_OK;
        goto END;
    }

    /* skip over the nodes */
    bytes = (skHentryIPSetGetNodeCount(hentry)
             * skHentryIPSetGetNodeSize(hentry));
    b = skStreamRead(stream, NULL, bytes);
    if (b != bytes) {
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    /* skip the first leaf, which contains no data */
    bytes = skHentryIPSetGetLeafSize(hentry);
    b = skStreamRead(stream, &leaf, bytes);
    if (b != bytes) {
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }
    ++count;

    if (!skHeaderIsNativeByteOrder(hdr)) {
        /* if the data is not in native byte order, we need to
         * byte-swap the values */
        while ((b = skStreamRead(stream, &leaf, bytes)) == bytes) {
            ++count;
            rv = ipsetInsertAddress(ipset, BSWAP32(leaf.ip), leaf.prefix);
            if (rv) {
                goto END;
            }
        }
    } else {
        while ((b = skStreamRead(stream, &leaf, bytes)) == bytes) {
            ++count;
            rv = ipsetInsertAddress(ipset, leaf.ip, leaf.prefix);
            if (rv) {
                goto END;
            }
        }
    }

    if (0 != b) {
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }
    assert(skHentryIPSetGetNodeCount(hentry) == count);

    rv = SKIPSET_OK;

  END:
    if (SKIPSET_OK == rv) {
        *ipset_out = ipset;
    } else {
        /* Do cleanup and return */
        skIPSetDestroy(&ipset);
    }
    return rv;
}


/* ****  PUBLIC FUNCTION DEFINITIONS BEGIN HERE  **** */

void
skIPSetAutoConvertDisable(
    skipset_t          *ipset)
{
    ipset->no_autoconvert = 1;
}

void
skIPSetAutoConvertEnable(
    skipset_t          *ipset)
{
    ipset->no_autoconvert = 0;
}

int
skIPSetAutoConvertIsEnabled(
    const skipset_t    *ipset)
{
    return !ipset->no_autoconvert;
}


/* Return true if 'ipset' contains 'ipaddr' */
int
skIPSetCheckAddress(
    const skipset_t    *ipset,
    const skipaddr_t   *ipaddr)
{
    uint32_t ipv4;

#if SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr)) {
        /* attempt to convert to IPv4 */
        if (skipaddrGetAsV4(ipaddr, &ipv4)) {
            /* conversion failed; this IP is not in the IPSet */
            return 0;
        }
    } else
#endif
    {
        ipv4 = skipaddrGetV4(ipaddr);
    }

    return skIPTreeCheckAddress(ipset->iptree, ipv4);
}


/* Return true if 'ipset1' and 'ipset2' have any IPs in common. */
int
skIPSetCheckIPSet(
    const skipset_t    *ipset1,
    const skipset_t    *ipset2)
{
    return skIPTreeCheckIntersectIPTree(ipset1->iptree, ipset2->iptree);
}


/* Return true if 'ipset' contains any IPs represened by 'ipwild' */
int
skIPSetCheckIPWildcard(
    const skipset_t        *ipset,
    const skIPWildcard_t   *ipwild)
{
    /* To be consistent with the IPset v3 version of this function, we
     * should not punt when the 'ipwild' contain IPv6 addresses.
     * Instead, check the ::ffff:0/96 prefix of 'ipwild'.  */
    return skIPTreeCheckIntersectIPWildcard(ipset->iptree, ipwild);
}


/* Return true if the specified address on 'rwrec' is in the 'ipset' */
int
skIPSetCheckRecord(
    const skipset_t    *ipset,
    const rwRec        *rwrec,
    int                 src_dst_nh)
{
    uint32_t ipv4;

#if SK_ENABLE_IPV6
    skipaddr_t ipaddr;

    if (rwRecIsIPv6(rwrec)) {
        /* attempt to convert to IPv4 */
        switch (src_dst_nh) {
          case 1:
            rwRecMemGetSIPv6(rwrec, &ipaddr);
            break;
          case 2:
            rwRecMemGetDIPv6(rwrec, &ipaddr);
            break;
          case 4:
            rwRecMemGetNhIPv6(rwrec, &ipaddr);
            break;
          default:
            skAbortBadCase(src_dst_nh);
        }
        if (skipaddrGetAsV4(&ipaddr, &ipv4)) {
            /* conversion failed; this IP is not in the IPSet */
            return 0;
        }
    } else
#endif
    {
        switch (src_dst_nh) {
          case 1:
            ipv4 = rwRecGetSIPv4(rwrec);
            break;
          case 2:
            ipv4 = rwRecGetDIPv4(rwrec);
            break;
          case 4:
            ipv4 = rwRecGetNhIPv4(rwrec);
            break;
          default:
            skAbortBadCase(src_dst_nh);
        }
    }

    return skIPTreeCheckAddress(ipset->iptree, ipv4);
}


/* Make the ipset use as few nodes as possible and make sure the ipset
 * uses a contiguous region of memory. */
int
skIPSetClean(
    skipset_t          *ipset)
{
    ipset->is_dirty = 0;
    return SKIPSET_OK;
}


/* Return true if 'ipset' contains any IPs that cannot be represented
 * as an IPv4 address. */
int
skIPSetContainsV6(
    const skipset_t UNUSED(*ipset))
{
    return 0;
}


/* Convert 'ipset' to hold IPs of the specific version. */
int
skIPSetConvert(
    skipset_t          *ipset,
    int                 target_ip_version)
{
    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    switch (target_ip_version) {
      case 4:
        return SKIPSET_OK;
      case 6:
        return SKIPSET_ERR_IPV6;
      default:
        return SKIPSET_ERR_BADINPUT;
    }
}


/* Return number of IPs in the IPset */
uint64_t
skIPSetCountIPs(
    const skipset_t    *ipset,
    double             *count)
{
    uint64_t c;

    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (!count) {
        return skIPTreeCountIPs(ipset->iptree);
    }

    c = skIPTreeCountIPs(ipset->iptree);
    *count = (double)c;
    return c;
}


/* Create a new IPset */
int
skIPSetCreate(
    skipset_t         **ipset,
    int                 support_ipv6)
{
    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (support_ipv6) {
        return SKIPSET_ERR_IPV6;
    }

    return ipsetCreate(ipset);
}


/* Destroy an IPset. */
void
skIPSetDestroy(
    skipset_t         **ipset)
{
    if (!ipset || !*ipset) {
        return;
    }
    skIPTreeDelete(&((*ipset)->iptree));
    free(*ipset);
    *ipset = NULL;
}


/* Insert the IP address into the IPset. */
int
skIPSetInsertAddress(
    skipset_t          *ipset,
    const skipaddr_t   *ipaddr,
    uint32_t            prefix)
{
    uint32_t ipv4;

#if  SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr)) {
        if (skipaddrGetAsV4(ipaddr, &ipv4)) {
            /* cannot convert IPv6 address to IPv4 */
            return SKIPSET_ERR_IPV6;
        }
        if (128 == prefix || 0 == prefix) {
            return ipsetInsertAddress(ipset, ipv4, 32);
        }
        if (prefix > 128) {
            return SKIPSET_ERR_PREFIX;
        }
        if (prefix <= 96) {
            return SKIPSET_ERR_IPV6;
        }
        ipv4 &= ~(UINT32_MAX >> (prefix - 96));
        return ipsetInsertAddress(ipset, ipv4, prefix - 96);
    }
#endif  /* SK_ENABLE_IPV6 */

    if (32 == prefix || 0 == prefix) {
        return ipsetInsertAddress(ipset, skipaddrGetV4(ipaddr), 32);
    }
    if (prefix > 32) {
        return SKIPSET_ERR_PREFIX;
    }
    ipv4 = skipaddrGetV4(ipaddr) & ~(UINT32_MAX >> prefix);
    return ipsetInsertAddress(ipset, ipv4, prefix);
}


/* Add each IP in the IPWildcard to the IPset. */
int
skIPSetInsertIPWildcard(
    skipset_t              *ipset,
    const skIPWildcard_t   *ipwild)
{
    int rv;

    rv = skIPTreeAddIPWildcard(ipset->iptree, ipwild);
    switch (rv) {
      case SKIP_OK:
        return SKIPSET_OK;
      case SKIP_ERR_ALLOC:
        return SKIPSET_ERR_ALLOC;
      case SKIP_ERR_IPV6:
        return SKIPSET_ERR_IPV6;
      default:
        skAbortBadCase(rv);
    }
}


int
skIPSetInsertRange(
    skipset_t          *ipset,
    const skipaddr_t   *ipaddr_start,
    const skipaddr_t   *ipaddr_end)
{
    skipaddr_t ipaddr4_start;
    skipaddr_t ipaddr4_end;
    skipaddr_t ipaddr4_next;
    uint32_t prefix;
    int rv;

    rv = skipaddrCompare(ipaddr_start, ipaddr_end);
    if (rv > 0) {
        /* end is less than start */
        return SKIPSET_ERR_BADINPUT;
    }
    if (0 == rv) {
        /* end is equal to start, thus single ip */
#if  SK_ENABLE_IPV6
        if (skipaddrIsV6(ipaddr_start)) {
            uint32_t ipv4;
            /* attempt to convert V6 ipaddr to V4 */
            if (skipaddrGetAsV4(ipaddr_start, &ipv4)) {
                /* cannot store V6 ipaddr in a V4 IPSet */
                return SKIPSET_ERR_IPV6;
            }
            return ipsetInsertAddress(ipset, ipv4, 32);
        }
#endif  /* SK_ENABLE_IPV6 */
        return ipsetInsertAddress(ipset, skipaddrGetV4(ipaddr_start), 32);
    }

    /* else range of IPs */
#if  SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr_start)) {
        if (skipaddrV6toV4(ipaddr_start, &ipaddr4_start)) {
            return SKIPSET_ERR_IPV6;
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        skipaddrCopy(&ipaddr4_start, ipaddr_start);
    }

#if  SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr_end)) {
        if (skipaddrV6toV4(ipaddr_end, &ipaddr4_end)) {
            return SKIPSET_ERR_IPV6;
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        skipaddrCopy(&ipaddr4_end, ipaddr_end);
    }

    do {
        prefix = skCIDRComputePrefix(&ipaddr4_start, &ipaddr4_end,
                                     &ipaddr4_next);
        rv = ipsetInsertAddress(ipset, skipaddrGetV4(&ipaddr4_start), prefix);
        skipaddrCopy(&ipaddr4_start, &ipaddr4_next);
    } while (SKIPSET_OK == rv && !skipaddrIsZero(&ipaddr4_start));

    return rv;
}


/* Make 'result_ipset' hold the intersection of itself with 'ipset' */
int
skIPSetIntersect(
    skipset_t          *result_ipset,
    const skipset_t    *ipset)
{
    /* check input */
    if (!result_ipset || !ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    skIPTreeIntersect(result_ipset->iptree, ipset->iptree);
    return SKIPSET_OK;
}


/* Return true if 'ipset' can hold IPv6 addresses */
int
skIPSetIsV6(
    const skipset_t UNUSED(*ipset))
{
    return 0;
}


/* Bind the iterator to an IPset */
int
skIPSetIteratorBind(
    skipset_iterator_t *iter,
    const skipset_t    *ipset,
    uint32_t            cidr_blocks,
    sk_ipv6policy_t     v6_policy)
{
    if (!ipset || !iter) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (ipset->is_dirty) {
        return SKIPSET_ERR_REQUIRE_CLEAN;
    }

    memset(iter, 0, sizeof(skipset_iterator_t));
    iter->v6policy = v6_policy;
    iter->cidr_blocks = (cidr_blocks ? 1 : 0);
    if (cidr_blocks) {
        ASSERT_OK(skIPTreeCIDRBlockIteratorBind(&iter->iter.cidr,
                                                ipset->iptree));
    } else {
        ASSERT_OK(skIPTreeIteratorBind(&iter->iter.ip, ipset->iptree));
    }
    return SKIPSET_OK;
}


/* Return the next CIDR block associated with the IPset */
int
skIPSetIteratorNext(
    skipset_iterator_t *iter,
    skipaddr_t         *ipaddr,
    uint32_t           *prefix)
{
    skIPTreeCIDRBlock_t cidr;
    int rv = SK_ITERATOR_NO_MORE_ENTRIES;

    switch (iter->v6policy) {
      case SK_IPV6POLICY_ONLY:
#if !SK_ENABLE_IPV6
      case SK_IPV6POLICY_FORCE:
#endif
        /* since we do not support IPv6 addresses in the IPTree data
         * structure, there is nothing to return */
        break;

#if SK_ENABLE_IPV6
      case SK_IPV6POLICY_FORCE:
        if (iter->cidr_blocks) {
            rv = skIPTreeCIDRBlockIteratorNext(&cidr, &iter->iter.cidr);
            if (SK_ITERATOR_OK == rv) {
                skipaddrSetV6FromUint32(ipaddr, &cidr.addr);
                *prefix = 96 + cidr.mask;
            }
        } else {
            rv = skIPTreeIteratorNext(&cidr.addr, &iter->iter.ip);
            if (SK_ITERATOR_OK == rv) {
                skipaddrSetV6FromUint32(ipaddr, &cidr.addr);
                *prefix = 128;
            }
        }
        break;
#endif  /* SK_ENABLE_IPV6 */

      case SK_IPV6POLICY_MIX:
      case SK_IPV6POLICY_ASV4:
      case SK_IPV6POLICY_IGNORE:
        if (iter->cidr_blocks) {
            rv = skIPTreeCIDRBlockIteratorNext(&cidr, &iter->iter.cidr);
            if (SK_ITERATOR_OK == rv) {
                skipaddrSetV4(ipaddr, &cidr.addr);
                *prefix = cidr.mask;
            }
        } else {
            rv = skIPTreeIteratorNext(&cidr.addr, &iter->iter.ip);
            if (SK_ITERATOR_OK == rv) {
                skipaddrSetV4(ipaddr, &cidr.addr);
                *prefix = 32;
            }
        }
        break;
    }
    return rv;
}


/* Reset iterator so we can visit the IPs again */
void
skIPSetIteratorReset(
    skipset_iterator_t *iter)
{
    if (iter->cidr_blocks) {
        skIPTreeCIDRBlockIteratorReset(&iter->iter.cidr);
    } else {
        skIPTreeIteratorReset(&iter->iter.ip);
    }
}


/* Read IPSet from filename---a wrapper around skIPSetRead(). */
int
skIPSetLoad(
    skipset_t         **ipset,
    const char         *filename)
{
    skstream_t *stream = NULL;
    int rv;

    if (filename == NULL || ipset == NULL) {
        return -1;
    }

    if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
        || (rv = skStreamBind(stream, filename))
        || (rv = skStreamOpen(stream)))
    {
        /* skStreamPrintLastErr(stream, rv, &skAppPrintErr); */
        rv = SKIPSET_ERR_OPEN;
        goto END;
    }

    rv = skIPSetRead(ipset, stream);
    if (rv) {
        goto END;
    }

  END:
    skStreamDestroy(&stream);
    return rv;
}


/* For each occupied block of size 'mask_prefix', set a single IP. */
int
skIPSetMask(
    skipset_t          *ipset,
    uint32_t            mask_prefix)
{
    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }

    /* verify mask_prefix value is valid */
    if (mask_prefix >= 32 || mask_prefix == 0) {
        return SKIPSET_ERR_PREFIX;
    }

    skIPTreeMask(ipset->iptree, mask_prefix);
    return SKIPSET_OK;
}


/* For each occupied block of size 'mask_prefix', set all IPs. */
int
skIPSetMaskAndFill(
    skipset_t          *ipset,
    uint32_t            mask_prefix)
{
    uint32_t all_ones;
    int step;
    int i, j, k;

    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }

    /* verify mask_prefix value is valid */
    if (mask_prefix >= 32 || mask_prefix == 0) {
        return SKIPSET_ERR_PREFIX;
    }

    ipset->is_dirty = 1;

    if (mask_prefix <= 16) {
        step = 1 << (16 - mask_prefix);
        for (i = 0; i < SKIP_BBLOCK_COUNT; i += step) {
            for (k = (i + step - 1); k >= i; --k) {
                if (ipset->iptree->nodes[k] != NULL) {
                    break;
                }
            }
            if (k < i) {
                continue;
            }
            for (k = (i + step - 1); k >= i; --k) {
                IPTREE_NODE_ALLOC(k);
                memset(ipset->iptree->nodes[k]->addressBlock, 0xff,
                       sizeof(ipset->iptree->nodes[k]->addressBlock));
            }
        }
        return SKIPSET_OK;
    }

    if (mask_prefix <= 27) {
        step = 1 << (27 - mask_prefix);
        for (i = 0; i < SKIP_BBLOCK_COUNT; ++i) {
            if (ipset->iptree->nodes[i] == NULL) {
                continue;
            }
            for (j = 0; j < SKIP_BBLOCK_SIZE; j += step) {
                for (k = (j + step - 1); k >= j; --k) {
                    if (ipset->iptree->nodes[i]->addressBlock[k]) {
                        break;
                    }
                }
                if (k < j) {
                    continue;
                }
                for (k = (j + step - 1); k >= j; --k) {
                    ipset->iptree->nodes[i]->addressBlock[k] = UINT32_MAX;
                }
            }
        }
        return SKIPSET_OK;
    }

    /* operate on each integer */
    step = 1 << (32 - mask_prefix);
    all_ones = UINT32_MAX >> (32 - step);
    for (i = 0; i < SKIP_BBLOCK_COUNT; ++i) {
        if (ipset->iptree->nodes[i] == NULL) {
            continue;
        }
        for (j = 0; j < SKIP_BBLOCK_SIZE; ++j) {
            for (k = 0; k < 32; k += step) {
                if (GET_MASKED_BITS(ipset->iptree->nodes[i]->addressBlock[j],
                                    k, step))
                {
                    SET_MASKED_BITS(ipset->iptree->nodes[i]->addressBlock[j],
                                    all_ones, k, step);
                }
            }
        }
    }

    return SKIPSET_OK;
}


/* Set the parameters to use when writing an IPset */
void
skIPSetOptionsBind(
    skipset_t                  *ipset,
    const skipset_options_t    *set_options)
{
    assert(ipset);

    ipset->options = set_options;
}


/* Initialize 'ipset_opts' and register options */
int
skIPSetOptionsRegister(
    skipset_options_t  *ipset_opts)
{
    assert(ipset_opts);
    assert(sizeof(ipset_options)/sizeof(struct option)
           == sizeof(ipset_options_help)/sizeof(char*));

    /* set default record version */
    ipset_opts->record_version = IPSET_RECORD_VERSION_LEGACY;
    ipset_opts->invocation_strip = 0;
    ipset_opts->comp_method = 0;
    ipset_opts->note_strip = 0;

    if (skOptionsRegister(ipset_options, ipsetOptionsHandler,
                          (clientData)ipset_opts)
        || skOptionsNotesRegister(ipset_opts->existing_silk_files
                                  ? &ipset_opts->note_strip
                                  : NULL)
        || sksiteCompmethodOptionsRegister(&ipset_opts->comp_method))
    {
        return -1;
    }
    return 0;
}


/* Clean up memory used by the IPset options */
void
skIPSetOptionsTeardown(
    void)
{
    skOptionsNotesTeardown();
}


/* Print the usage strings for the options that the library registers */
void
skIPSetOptionsUsage(
    FILE               *fh)
{
    int i;

    for (i = 0; ipset_options[i].name; ++i) {
        fprintf(fh, "--%s %s. %s", ipset_options[i].name,
                SK_OPTION_HAS_ARG(ipset_options[i]), ipset_options_help[i]);
        switch (ipset_options[i].val) {
          case OPT_IPSET_RECORD_VERSION:
            fprintf(fh, ("\n\tValid value: %d. Def. %d."
                         " (Use %d for compatibility with SiLK 2.x.)"),
                    IPSET_RECORD_VERSION_LEGACY, IPSET_RECORD_VERSION_LEGACY,
                    IPSET_RECORD_VERSION_LEGACY);
            break;

          default:
            break;
        }
        fprintf(fh, "\n");
    }
    skOptionsNotesUsage(fh);
    sksiteCompmethodOptionsUsage(fh);
}



/* Print the IPs in ipset to 'stream'. */
void
skIPSetPrint(
    const skipset_t    *ipset,
    skstream_t         *stream,
    skipaddr_flags_t    ip_format,
    int                 as_cidr)
{
    skIPTreePrint(ipset->iptree, stream, ip_format, as_cidr);
}


#if 0
/* Debugging print function. */
void
skIPSetDebugPrint(
    const skipset_t    *ipset)
{
    const ipset_node_t *node;
    const ipset_leaf_t *leaf;
    uint32_t node_idx;
    uint32_t bitmap_size;
    sk_bitmap_t *isfree;
    int width = 0;

    if (ipset->leaves.entry_count) {
        width = 2 + (int)log10(ipset->leaves.entry_count);
    }

    /* print root ID */
    fprintf(stderr,
            ">> %*sROOT %u%c      NODE_FREE %uN      LEAF_FREE %uL\n",
            width, "", (unsigned)IPSET_ROOT_INDEX(ipset),
            (IPSET_ROOT_IS_LEAF(ipset) ? 'L' : 'N'),
            (unsigned)ipset->nodes.free_list,
            (unsigned)ipset->leaves.free_list);

    if (IPSET_ISEMPTY(ipset)) {
        return;
    }

    /* this function creates a bitmap to note which nodes and leaves
     * are on the free list.  this is the size of the bitmap */
    if (ipset->nodes.entry_count > ipset->leaves.entry_count) {
        bitmap_size = ipset->nodes.entry_count;
    } else {
        bitmap_size = ipset->leaves.entry_count;
    }

    /* create the bitmap */
    if (skBitmapCreate(&isfree, bitmap_size)) {
        /* unable to create bitmap; use simple printing (this also
         * allows us to use the functions and avoid warnings from
         * gcc) */

        /* print nodes */
        for (node_idx = 0; node_idx < ipset->nodes.entry_count; ++node_idx) {
            fprintf(stderr, "** %*uN  ", width, node_idx);
            ipsetDebugPrintByIndex(ipset, node_idx, 0);
        }

        /* print leaves */
        fprintf(stderr, "\n");
        for (node_idx = 0; node_idx < ipset->leaves.entry_count; ++node_idx) {
            fprintf(stderr, "** %*uL  ", width, node_idx);
            ipsetDebugPrintByIndex(ipset, node_idx, 1);
        }

        return;
    }

    /* fill the bitmap for nodes on the free list */
    for (node_idx = ipset->nodes.free_list;
         0 != node_idx;
         node_idx = NODEIDX_FREE_LIST(ipset, node_idx))
    {
        assert(node_idx < ipset->nodes.entry_count);
        skBitmapSetBit(isfree, node_idx);
    }

    /* print the nodes */
    for (node_idx = 0; node_idx < ipset->nodes.entry_count; ++node_idx) {
        node = NODE_PTR(ipset, node_idx);
        fprintf(stderr, "** %*uN  ", width, node_idx);
#if SK_ENABLE_IPV6
        if (ipset->is_ipv6) {
            ipsetDebugPrintAddrV6(&node->v6.ip, node->v6.prefix);
        } else
#endif
        {
            ipsetDebugPrintAddrV4(node->v4.ip, node->v4.prefix);
        }

        /* note whether this entry is on free-list */
        fprintf(stderr, "  %c",
                (skBitmapGetBit(isfree, node_idx) ? 'F' : ' '));

        ipsetDebugPrintChildren(node, width);
        fprintf(stderr, "\n");
    }

    skBitmapClearAllBits(isfree);

    /* mark leaves that are on the free list */
    for (node_idx = ipset->leaves.free_list;
         0 != node_idx;
         node_idx = LEAFIDX_FREE_LIST(ipset, node_idx))
    {
        assert(node_idx < ipset->leaves.entry_count);
        skBitmapSetBit(isfree, node_idx);
    }

    /* print the leaves */
    fprintf(stderr, "\n");
    for (node_idx = 0; node_idx < ipset->leaves.entry_count; ++node_idx) {
        leaf = LEAF_PTR(ipset, node_idx);
        fprintf(stderr, "** %*uL  ", width, node_idx);
#if SK_ENABLE_IPV6
        if (ipset->is_ipv6) {
            ipsetDebugPrintAddrV6(&leaf->v6.ip, leaf->v6.prefix);
        } else
#endif
        {
            ipsetDebugPrintAddrV4(leaf->v4.ip, leaf->v4.prefix);
        }
        /* note whether this entry is on free-list */
        fprintf(stderr, "%s",
                (skBitmapGetBit(isfree, node_idx) ? "  F\n" : "\n"));
    }

    skBitmapDestroy(&isfree);
}
#endif  /* 0 */


int
skIPSetRead(
    skipset_t         **ipset_out,
    skstream_t         *stream)
{
    skipset_t *ipset = NULL;
    sk_file_header_t *hdr;
    int swap;
    uint32_t block24[1 + WORDS_PER_SLASH24];
    uint32_t slash24;
    uint32_t slash16;
    ssize_t b;
    int i;
    int rv;

    if (!stream || !ipset_out) {
        return SKIPSET_ERR_BADINPUT;
    }
    *ipset_out = NULL;

    rv = skStreamReadSilkHeader(stream, &hdr);
    if (rv) {
        if (SKSTREAM_ERR_COMPRESS_UNAVAILABLE == rv) {
            rv = SKIPSET_ERR_FILEHEADER;
        } else {
            rv = SKIPSET_ERR_FILEIO;
        }
        goto END;
    }

    /* Ensure we are reading a file we understand */
    rv = skStreamCheckSilkHeader(stream, FT_IPSET,
                                 0, IPSET_RECORD_VERSION_LEGACY, NULL);
    switch (rv) {
      case SKSTREAM_OK:
        break;
      case SKSTREAM_ERR_UNSUPPORT_FORMAT:
        /* not an IPset file */
        return SKIPSET_ERR_FILETYPE;
      case SKSTREAM_ERR_UNSUPPORT_VERSION:
        /* try to read this file as a newer IPset file */
        return ipsetReadV3(ipset_out, stream, hdr);
      default:
        return SKIPSET_ERR_FILEHEADER;
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        return SKIPSET_ERR_FILEHEADER;
    }

    rv = ipsetCreate(&ipset);
    if (rv != SKIPSET_OK) {
        goto END;
    }

    swap = !skHeaderIsNativeByteOrder(hdr);

    /*
     * IPs are stored on disk in blocks of nine 32-bit words which
     * represent a /24.  The first uint32_t is the base IP of the /24
     * (a.b.c.0); the remaining eight uint32_t's have a bit for each
     * address in the /24.
     */
    while ((b = skStreamRead(stream, block24, sizeof(block24)))
           == sizeof(block24))
    {
        if (swap) {
            for (i = 0; i < 1+WORDS_PER_SLASH24; ++i) {
                block24[i] = BSWAP32(block24[i]);
            }
        }

        /* Put the first two octets into slash16, and allocate the
         * space for the /16 if we need to */
        slash16 = (block24[0] >> 16);
        if (NULL == ipset->iptree->nodes[slash16]) {
            ipset->iptree->nodes[slash16]
                = (skIPNode_t*)calloc(1, sizeof(skIPNode_t));
            if (NULL == ipset->iptree->nodes[slash16]) {
                rv = SKIPSET_ERR_ALLOC;
                goto END;
            }
        }

        /* Locate where this /24 occurs inside the larger /16, then
         * copy it into place.  Following is equivalent to
         * (((block24[0] >> 8) & 0xFF) * 8); */
        slash24 = ((block24[0] & 0x0000FF00) >> 5);
        memcpy((ipset->iptree->nodes[slash16]->addressBlock + slash24),
               block24 + 1, WORDS_PER_SLASH24 * sizeof(uint32_t));
    }
    if (b != 0) {
        /* read error */
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    *ipset_out = ipset;
    rv = SKIPSET_OK;

  END:
    if (rv != SKIPSET_OK) {
        /* Do cleanup (Delete tree) and return */
        skIPSetDestroy(&ipset);
    }
    return rv;
}


int
skIPSetRemoveAddress(
    skipset_t          *ipset,
    const skipaddr_t   *ipaddr,
    uint32_t            prefix)
{
    uint32_t ipv4;
    uint32_t ipv4_end;

    assert(ipset);

#if  SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr)) {
        /* attempt to convert V6 ipaddr to V4 */
        if (skipaddrGetAsV4(ipaddr, &ipv4)) {
            /* a V6 ipaddr is not in a V4 IPSet, so nothing to remove */
            return SKIPSET_OK;
        }
        if (prefix > 128) {
            return SKIPSET_ERR_PREFIX;
        }
        if ((128 == prefix) || (0 == prefix)) {
            prefix = 32;
        } else if (prefix <= 96) {
            return SKIPSET_ERR_IPV6;
        } else {
            prefix -= 96;
            ipv4 &= ~(UINT32_MAX >> prefix);
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        if (prefix > 32) {
            return SKIPSET_ERR_PREFIX;
        }
        if ((32 == prefix) || (0 == prefix)) {
            ipv4 = skipaddrGetV4(ipaddr);
        } else {
            ipv4 = skipaddrGetV4(ipaddr) & ~(UINT32_MAX >> prefix);
        }
    }

    if (32 == prefix) {
        if (ipset->iptree->nodes[ipv4 >> 16]) {
            skIPTreeNodeClear(ipv4, ipset->iptree->nodes[ipv4 >> 16]);
        }

    } else if (prefix <= 16) {
        ipv4_end = ((UINT32_MAX >> prefix) | ipv4) >> 16;
        ipv4 >>= 16;
        do {
            if (ipset->iptree->nodes[ipv4]) {
                free(ipset->iptree->nodes[ipv4]);
                ipset->iptree->nodes[ipv4] = NULL;
            }
        } while (ipv4++ < ipv4_end);

    } else if (prefix <= 27) {
        ipv4_end = ((UINT32_MAX >> prefix) | ipv4) >> 5;
        ipv4 >>= 5;
        if (NULL == ipset->iptree->nodes[ipv4>>11]) {
            goto END;
        }
        do {
            /* nodes are allocated on /16, already shifted by 5, need
             * to shift by 11 more to get the complete shift by 16.
             * Mask by 0x7ff (0xffff>>5) to get addressBlock */
            memset(&ipset->iptree->nodes[ipv4>>11]->addressBlock[ipv4 & 0x07FF],
                   0, sizeof(uint32_t));
        } while (ipv4++ < ipv4_end);

    } else {
        if (NULL == ipset->iptree->nodes[ipv4 >> 16]) {
            goto END;
        }
        ipv4_end = (UINT32_MAX >> prefix) | ipv4;
        do {
            skIPTreeNodeClear(ipv4, ipset->iptree->nodes[ipv4 >> 16]);
        } while (ipv4++ < ipv4_end);
    }

  END:
    return SKIPSET_OK;
}


int
skIPSetRemoveAll(
    skipset_t          *ipset)
{
    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    ASSERT_OK(skIPTreeRemoveAll(ipset->iptree));
    return SKIPSET_OK;
}


int
skIPSetRemoveIPWildcard(
    skipset_t              *ipset,
    const skIPWildcard_t   *ipwild)
{
    skIPWildcardIterator_t iter;
    skipaddr_t ip;
    uint32_t prefix;
    int rv = SKIPSET_OK;

    /* Remove the netblocks contained in the wildcard */
    skIPWildcardIteratorBind(&iter, ipwild);
    while (skIPWildcardIteratorNextCidr(&iter, &ip, &prefix) == SK_ITERATOR_OK
           && SKIPSET_OK == rv)
    {
        rv = skIPSetRemoveAddress(ipset, &ip, prefix);
    }

    return rv;
}


/* Write 'ipset' to 'filename'--a wrapper around skIPSetWrite(). */
int
skIPSetSave(
    const skipset_t    *ipset,
    const char         *filename)
{
    skstream_t *stream = NULL;
    int rv;

    if (filename == NULL || ipset == NULL) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (ipset->is_dirty) {
        return SKIPSET_ERR_REQUIRE_CLEAN;
    }

    if ((rv = skStreamCreate(&stream, SK_IO_WRITE, SK_CONTENT_SILK))
        || (rv = skStreamBind(stream, filename))
        || (rv = skStreamOpen(stream)))
    {
        /* skStreamPrintLastErr(stream, rv, &skAppPrintErr); */
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    rv = skIPSetWrite(ipset, stream);

  END:
    skStreamDestroy(&stream);
    return rv;
}


/* Convert 'error_code' to a string. */
const char *
skIPSetStrerror(
    int                 error_code)
{
    static char errbuf[128];

    switch ((skipset_return_t)error_code) {
      case SKIPSET_OK:
        return "Success";
      case SKIPSET_ERR_EMPTY:
        return "IPset is empty";
      case SKIPSET_ERR_PREFIX:
        return "Invalid prefix";
      case SKIPSET_ERR_NOTFOUND:
        return "Value not found in IPset";
      case SKIPSET_ERR_ALLOC:
        return "Unable to allocate memory";
      case SKIPSET_ERR_BADINPUT:
        return "Empty input value";
      case SKIPSET_ERR_FILEIO:
        return "Error in read/write";
      case SKIPSET_ERR_FILETYPE:
        return "Input is not an IPset";
      case SKIPSET_ERR_FILEHEADER:
        return "File header values incompatible with this compile of SiLK";
      case SKIPSET_ERR_FILEVERSION:
        return "IPset version unsupported by this SiLK release";
      case SKIPSET_ERR_OPEN:
        return "Error opening file";
      case SKIPSET_ERR_IPV6:
        return "IPset does not allow IPv6 addresses";
      case SKIPSET_ERR_REQUIRE_CLEAN:
        return "Function requires a clean IPset";
      case SKIPSET_ERR_CORRUPT:
        return "IPset state is inconsistent (corrupt file?)";
      case SKIPSET_ERR_SUBSET:
        return "Part of netblock exists in IPset";
      case SKIPSET_ERR_MULTILEAF:
        return "Search ended at missing branch";
    }

    snprintf(errbuf, sizeof(errbuf),
             "Unrecognized IPset error code %d", error_code);
    return errbuf;
}


/* Turn off IPs of 'result_ipset' that are on in 'ipset'. */
int
skIPSetSubtract(
    skipset_t          *result_ipset,
    const skipset_t    *ipset)
{
    if (!result_ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (!ipset) {
        return SKIPSET_OK;
    }
    skIPTreeSubtract(result_ipset->iptree, ipset->iptree);
    return SKIPSET_OK;
}


/* Turn on IPs of 'result_ipset' that are on in 'ipset'. */
int
skIPSetUnion(
    skipset_t          *result_ipset,
    const skipset_t    *ipset)
{
    int rv;

    if (!result_ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (!ipset) {
        return SKIPSET_OK;
    }

    rv = skIPTreeUnion(result_ipset->iptree, ipset->iptree);
    if (SKIP_ERR_ALLOC == rv) {
        return SKIPSET_ERR_ALLOC;
    }
    return SKIPSET_OK;
}


/* Invoke the 'callback' function on all IPs in the 'ipset' */
int
skIPSetWalk(
    const skipset_t    *ipset,
    uint32_t            cidr_blocks,
    sk_ipv6policy_t     v6_policy,
    skipset_walk_fn_t   callback,
    void               *cb_data)
{
    skipaddr_t ipaddr;
    int rv = SKIPSET_OK;

    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (SK_IPV6POLICY_ONLY == v6_policy) {
        /* caller wants only IPv6 addresses, and there are none in an
         * IPv4 IPset. */
        return SKIPSET_OK;
    }
#if !SK_ENABLE_IPV6
    if (SK_IPV6POLICY_FORCE == v6_policy) {
        return SKIPSET_OK;
    }
#endif

    if (cidr_blocks) {
        skIPTreeCIDRBlockIterator_t cidr_iter;
        skIPTreeCIDRBlock_t cidr;

        ASSERT_OK(skIPTreeCIDRBlockIteratorBind(&cidr_iter, ipset->iptree));

#if SK_ENABLE_IPV6
        if (SK_IPV6POLICY_FORCE == v6_policy) {
            while ((rv = skIPTreeCIDRBlockIteratorNext(&cidr, &cidr_iter))
                   == SK_ITERATOR_OK)
            {
                skipaddrSetV6FromUint32(&ipaddr, &cidr.addr);
                cidr.mask += 96;
                rv = (*callback)(&ipaddr, cidr.mask, cb_data);
                if (rv) {
                    return rv;
                }
            }
        } else
#endif  /* SK_ENABLE_IPV6 */
        {
            while ((rv = skIPTreeCIDRBlockIteratorNext(&cidr, &cidr_iter))
                   == SK_ITERATOR_OK)
            {
                skipaddrSetV4(&ipaddr, &cidr.addr);
                rv = (*callback)(&ipaddr, cidr.mask, cb_data);
                if (rv) {
                    return rv;
                }
            }
        }

    } else {
        skIPTreeIterator_t ip_iter;
        uint32_t ip;

        ASSERT_OK(skIPTreeIteratorBind(&ip_iter, ipset->iptree));

#if SK_ENABLE_IPV6
        if (SK_IPV6POLICY_FORCE == v6_policy) {
            while ((rv = skIPTreeIteratorNext(&ip, &ip_iter))
                   == SK_ITERATOR_OK)
            {
                skipaddrSetV6FromUint32(&ipaddr, &ip);
                rv = (*callback)(&ipaddr, 128, cb_data);
                if (rv) {
                    return rv;
                }
            }
        } else
#endif  /* SK_ENABLE_IPV6 */
        {
            while ((rv = skIPTreeIteratorNext(&ip, &ip_iter))
                   == SK_ITERATOR_OK)
            {
                skipaddrSetV4(&ipaddr, &ip);
                rv = (*callback)(&ipaddr, 32, cb_data);
                if (rv) {
                    return rv;
                }
            }
        }
    }

    return SKIPSET_OK;
}


int
skIPSetWrite(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    skIPNode_t *slash16;
    uint32_t slash24_empty[WORDS_PER_SLASH24];
    uint32_t slash24;
    uint32_t i;
    uint32_t j;
    int rv;

    memset(slash24_empty, 0, sizeof(slash24_empty));

    if (!ipset || !stream) {
        return SKIPSET_ERR_BADINPUT;
    }

    /* do not write an unclean ipset */
    if (ipset->is_dirty) {
        return SKIPSET_ERR_REQUIRE_CLEAN;
    }

    /* prep and write the header */
    hdr = skStreamGetSilkHeader(stream);
    skHeaderSetFileFormat(hdr, FT_IPSET);
    skHeaderSetRecordVersion(hdr, IPSET_RECORD_VERSION_LEGACY);
    skHeaderSetRecordLength(hdr, 1);

    if (ipset->options) {
        const skipset_options_t *opts = ipset->options;

        if (opts->record_version != IPSET_RECORD_VERSION_LEGACY) {
            return SKIPSET_ERR_BADINPUT;
        }

        if (opts->note_strip) {
            skHeaderRemoveAllMatching(hdr, SK_HENTRY_ANNOTATION_ID);
        }
        if (opts->invocation_strip) {
            skHeaderRemoveAllMatching(hdr, SK_HENTRY_INVOCATION_ID);
        } else if (opts->argc && opts->argv) {
            rv = skHeaderAddInvocation(hdr, 1, opts->argc, opts->argv);
            if (rv) {
                return SKIPSET_ERR_FILEIO;
            }
        }
        if ((rv = skHeaderSetCompressionMethod(hdr, opts->comp_method))
            || (rv = skOptionsNotesAddToStream(stream)))
        {
            return SKIPSET_ERR_FILEIO;
        }
    }
    rv = skStreamWriteSilkHeader(stream);
    if (rv) {
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    /*
     * IPs are stored on disk in blocks of nine 32-bit words which
     * represent a /24.  The first uint32_t is the base IP of the /24
     * (a.b.c.0); the remaining eight uint32_t's have a bit for each
     * address in the /24.
     */
    for (i = 0; i < SKIP_BBLOCK_COUNT; i++) {
        /* slash16 represents a /16 */
        slash16 = ipset->iptree->nodes[i];
        if (NULL == slash16) {
            continue;
        }

        /*
         * There is data in this /16, walk over the addressBlocks by
         * 8s, which is equivalent to visiting each /24.
         */
        for (j = 0; j < SKIP_BBLOCK_SIZE; j += WORDS_PER_SLASH24) {
            if (memcmp(slash16->addressBlock + j, slash24_empty,
                       sizeof(slash24_empty)))
            {
                /* there is data in this /24; write the base address. */
                slash24 = ((i << 16) | (j << 5)) & 0xFFFFFF00;
                if (skStreamWrite(stream, &slash24, sizeof(uint32_t)) == -1) {
                    rv = SKIPSET_ERR_FILEIO;
                    goto END;
                }
                /* write the complete /24: 8 uint32_t's */
                if (skStreamWrite(stream, slash16->addressBlock + j,
                                  sizeof(slash24_empty))
                    == -1)
                {
                    rv = SKIPSET_ERR_FILEIO;
                    goto END;
                }
            }
        }
    }

    rv = skStreamFlush(stream);
    if (rv) {
        /* skStreamPrintLastErr(stream, rv, &skAppPrintErr); */
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    rv = SKIPSET_OK;

  END:
    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
