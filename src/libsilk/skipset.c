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

RCSIDENT("$SiLK: skipset.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/iptree.h>
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

/*
 *    IPset file format version that contains a dump of the radix-tree
 *    data strucutre.  This format can be mmap()ed so that the data
 *    does not need to be read into RAM.  Can store either IPv4 or
 *    IPv6 addresses.  Introduced in SiLK-3.0.
 */
#define IPSET_REC_VERSION_RADIX             3

/*
 *    Version of the IPset file compatible with SiLK versions prior to
 *    SiLK 3.0.
 *
 *    IPs are stored on disk in blocks of nine 32-bit words which
 *    represent a /24.  All values are in native byte order.  The
 *    first uint32_t (position [0]) is the base IP of the /24
 *    (a.b.c.0); the remaining eight uint32_t's are a 256-bit bitmap
 *    that represents the IPs in that /24.  Each uint32_t is a /27.  A
 *    high bit indicates the address is present.  The least
 *    significant bit in position [1] is a.b.c.0, and the most
 *    significant bit in position [8] is a.b.c.255.
 */
#define IPSET_REC_VERSION_LEGACY            2

/*
 *    IPset file format version introduced in SiLK-3.7, but it is not
 *    the default to maintain backwards compatibility.
 *
 *    File contains either only IPv4 or only IPv6 addresses.  Each
 *    "block" starts with an IP address and a single byte value.  If
 *    the byte less than or equal to 0x80, the address is the base
 *    address of a netblock and the next single octet is the CIDR mask
 *    for the netblock.
 *
 *    If the byte is strictly greater than 0x80, the address is the
 *    base address for a bitmap.  The value of 0x81 means the bitmap
 *    has 256 bits containing a /24 in IPv4 or a /120 in IPv6.  These
 *    bitmaps are similar to those in IPSET_REC_VERSION_LEGACY.
 *
 *    IPv4 addresses and values in the bitmap are in native byte
 *    order.  IPv6 addresses are stored in network byte order.
 */
#define IPSET_REC_VERSION_CIDR_BMAP         4

/*
 *    Number to tell writer to use the default IPset file version,
 *    which is LEGACY for IPv4 and RADIX for IPv6.
 */
#define IPSET_REC_VERSION_DEFAULT           0

/*
 *    When writing a IPset that contains IPv6 addresses, this is the
 *    default version file format to use.
 */
#define IPSET_REC_VERSION_IPV6_DEFAULT      IPSET_REC_VERSION_RADIX

/*
 *    Maximum file version available
 */
#define IPSET_REC_VERSION_MAX               IPSET_REC_VERSION_CIDR_BMAP

/*
 *    Name of an environment variable that, when set, is used in place
 *    of IPSET_REC_VERSION_DEFAULT.  This is advertised.
 */
#define IPSET_REC_VERSION_ENVAR             "SILK_IPSET_RECORD_VERSION"

/*
 *    Name of an environment variable used to determine how
 *    IPv4-IPsets are represented internally.  Legal values are
 *    "iptree" for the SiLK-2 IPTree structure and "radix" for the
 *    Radix-Tree structure.  Not intended for end users.  Used by
 *    IPSET_USE_IPTREE and ipset_use_iptree.
 */
#define IPSET_ENVAR_INCORE_FORMAT           "SKIPSET_INCORE_FORMAT"

/*
 *    Name of an environment variable used to force the Radix-tree to
 *    print, to standard error, its structure when it is destoryed.
 *    Not intended for end users.
 */
#define IPSET_ENVAR_DESTROY_PRINT           "SKIPSET_DESTROY_PRINT"


/*
 *    Whether to use the IPTree or Radix-Tree data structure for
 *    in-memory IPv4-IPsets.  Uses the environment variable specified
 *    by the IPSET_ENVAR_INCORE_FORMAT macro.
 */
#define IPSET_USE_IPTREE                                                \
    (ipset_use_iptree >= 0 ? ipset_use_iptree : ipsetCheckFormatEnvar())

/*
 *    Default value for IPSET_USE_IPTREE if the envar is not set.  See
 *    the 'ipset_use_iptree' variable below.
 */
#define IPSET_USE_IPTREE_DEFAULT           1

/* Number of nodes to create initially */
#define  IPSET_INITIAL_ENTRY_COUNT       2048

/* Number of bits if IP to examine when branching at a node */
#define  NUM_BITS  4

/* Number of children in each interior node */
#define  IPSET_NUM_CHILDREN         (1 << NUM_BITS)

/* Number of uint32_t's required to hold a bitmap of
 * IPSET_NUM_CHILDREN bits */
#define  BITMAP_SIZE_NUM_CHILDREN               \
    ((IPSET_NUM_CHILDREN + 31) >> 5)

/* Number of bytes in an IPv6 address */
#define  IPSET_LEN_V6  16

/* Number of bytes in an IPv4 address */
#define  IPSET_LEN_V4   4

/* The library uses recursion in few places, and instead maintains a
 * stack of nodes; this constant is the required depth of the
 * stack.  */
#define  IPSET_MAX_DEPTH_V4                                             \
    (IPSET_NUM_CHILDREN * (1 + ((IPSET_LEN_V4 * CHAR_BIT) / NUM_BITS)))
#define  IPSET_MAX_DEPTH_V6                                             \
    (IPSET_NUM_CHILDREN * (1 + ((IPSET_LEN_V6 * CHAR_BIT) / NUM_BITS)))
#if SK_ENABLE_IPV6
#  define IPSET_MAX_DEPTH  IPSET_MAX_DEPTH_V6
#else
#  define IPSET_MAX_DEPTH  IPSET_MAX_DEPTH_V4
#endif

/* Index of first leaf when iterating over the leaves in a clean IPset */
#define IPSET_LINK_LIST_ANCHOR         1
#define IPSET_ITER_FIRST_LEAF          1

/* Magic parent_idx value to denote that ipsetFindVx() returned the
 * root of the tree */
#define IPSET_NO_PARENT                UINT32_MAX

/* Return the root index for an IPset */
#define IPSET_ROOT_INDEX(iri_set)      ((iri_set)->s.v3->root_idx)

/* Return a non-zero value if the root index references a leaf */
#define IPSET_ROOT_IS_LEAF(iril_set)   ((iril_set)->s.v3->root_is_leaf)

/* Set the root index of the IPset */
#define IPSET_ROOT_INDEX_SET(iris_set, iris_index, iris_is_leaf)        \
    {                                                                   \
        ((iris_set)->s.v3->root_idx) = (iris_index);                    \
        ((iris_set)->s.v3->root_is_leaf) = (iris_is_leaf);              \
    }

/* Is the IPset empty? */
#define IPSET_ISEMPTY(iie_ipset)                \
    (0 == (iie_ipset)->s.v3->nodes.entry_count)

/* Helper macro to get the node at 'ge_index' */
#define GET_ENTRY(ge_buf, ge_index)                     \
    (&((ge_buf).buf[(ge_buf).entry_size * (ge_index)]))

/* Get the node at position 'np_index' and cast to correct type */
#define NODE_PTR(np_set, np_index)                              \
    ((ipset_node_t*)GET_ENTRY((np_set)->s.v3->nodes, np_index))

#define NODE_PTR_V4(np_set, np_index)                                   \
    (&(((ipset_node_v4_t*)((np_set)->s.v3->nodes.buf))[(np_index)]))

#define NODE_PTR_V6(np_set, np_index)                                   \
    (&(((ipset_node_v6_t*)((np_set)->s.v3->nodes.buf))[(np_index)]))

/* Get the member of the node structure to use when handling nodes on
 * the free list */
#define NODEPTR_FREE_LIST(npmfl_node)           \
    ((npmfl_node)->v4.child[0])
#define NODEIDX_FREE_LIST(nimfl_set, nimfl_nodeidx)             \
    NODEPTR_FREE_LIST(NODE_PTR((nimfl_set), (nimfl_nodeidx)))

/* "Free" the node at position 'nf_index' by pusing onto free list */
#define NODEIDX_FREE(nf_set, nf_index)                  \
    {                                                   \
        NODEIDX_FREE_LIST((nf_set), (nf_index))         \
            = (nf_set)->s.v3->nodes.free_list;          \
        (nf_set)->s.v3->nodes.free_list = (nf_index);   \
    }

/* Get the leaf at position 'np_index' and cast to correct type */
#define LEAF_PTR(np_set, np_index)                                      \
    ((ipset_leaf_t*)GET_ENTRY((np_set)->s.v3->leaves, np_index))

#define LEAF_PTR_V4(np_set, np_index)                                   \
    (&(((ipset_leaf_v4_t*)((np_set)->s.v3->leaves.buf))[(np_index)]))

#define LEAF_PTR_V6(np_set, np_index)                                   \
    (&(((ipset_leaf_v6_t*)((np_set)->s.v3->leaves.buf))[(np_index)]))

/* Get the member of the leaf structure to use for handling leaves on
 * the free list */
#define LEAFPTR_FREE_LIST(lpmfl_leaf)           \
    ((lpmfl_leaf)->v4.ip)
#define LEAFIDX_FREE_LIST(limfl_set, limfl_leafidx)             \
    LEAFPTR_FREE_LIST(LEAF_PTR((limfl_set), (limfl_leafidx)))

/* "Free" the leaf at position 'lf_index' by pushing onto free list */
#define LEAFIDX_FREE(lf_set, lf_index)                  \
    {                                                   \
        LEAFIDX_FREE_LIST((lf_set), (lf_index))         \
            = (lf_set)->s.v3->leaves.free_list;         \
        (lf_set)->s.v3->leaves.free_list = (lf_index);  \
    }


/* Return a non-zero value if the child at index position 'ncil_pos'
 * on the node 'ncil_node' is a leaf.  'ncil_node' must be an
 * ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_IS_LEAF(ncil_node, ncil_pos)              \
    SET_BMAP_GET((ncil_node)->child_is_leaf, (ncil_pos))

/* Similar to NODEPTR_CHILD_IS_LEAF(), but specifies whether the child
 * at 'ncir_pos' is a repeat of the child to its left---which may also
 * be a repeat of the child to its left, etc. */
#define NODEPTR_CHILD_IS_REPEAT(ncir_node, ncir_pos)            \
    SET_BMAP_GET((ncir_node)->child_repeated, (ncir_pos))

/* Specify that the child at index position 'ncsl_pos' on the node
 * 'ncsl_node' is a leaf not a node.  'ncsl_node' must an
 * ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_SET_LEAF(ncsl_node, ncsl_pos)             \
    SET_BMAP_SET((ncsl_node)->child_is_leaf, (ncsl_pos))

/* Specify that the child at index position 'nccl_pos' on the node
 * 'nccl_node' is NOT a leaf.  'nccl_node' must an
 * ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_CLEAR_LEAF(nccl_node, nccl_pos)           \
    SET_BMAP_CLEAR((nccl_node)->child_is_leaf, (nccl_pos))

/* Specify that the children between index position 'ncsl_pos1' and
 * 'ncsl_pos2' on the node 'ncsl_node' are leaves.  'ncsl_node' must
 * an ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_SET_LEAF2(ncsl_node, ncsl_pos1, ncsl_pos2)        \
    SET_BMAP_RANGE_SET((ncsl_node)->child_is_leaf, (ncsl_pos1), (ncsl_pos2))

/* Specify that the children between index position 'nccl_pos1' and
 * 'nccl_pos2' on the node 'nccl_node' are NOT leaves.  'nccl_node'
 * must an ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_CLEAR_LEAF2(nccl_node, nccl_pos1, nccl_pos2)      \
    SET_BMAP_RANGE_CLEAR((nccl_node)->child_is_leaf, (nccl_pos1), (nccl_pos2))

/* Specify that the child at index position 'ncsr_pos' on the node
 * 'ncsr_node' is a repeat of the child to its left.  'ncsr_node' must
 * an ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_SET_REPEAT(ncsr_node, ncsr_pos)           \
    SET_BMAP_SET((ncsr_node)->child_repeated, (ncsr_pos))

/* Specify the the children between index position 'ncsr_pos1' and
 * 'ncsr_pos2' on the node 'ncsr_node' are leaves.  'ncsr_node' must
 * an ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_SET_REPEAT2(ncsr_node, ncsr_pos1, ncsr_pos2)      \
    SET_BMAP_RANGE_SET((ncsr_node)->child_repeated, (ncsr_pos1), (ncsr_pos2))

/* Specify that the child at index position 'nccr_pos' on the node
 * 'nccr_node' is NOT a repeat.  'nccr_node' must an
 * ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_CLEAR_REPEAT(nccr_node, nccr_pos)         \
    SET_BMAP_CLEAR((nccr_node)->child_repeated, (nccr_pos))

/* Specify that the children between index position 'nccr_pos1' and
 * 'nccr_pos2' on the node 'nccr_node' are NOT repeats.  'nccr_node'
 * must an ipset_node_v{4,6}_t, not a generic ipset_node_t. */
#define NODEPTR_CHILD_CLEAR_REPEAT2(nccr_node, nccr_pos1, nccr_pos2)    \
    SET_BMAP_RANGE_CLEAR((nccr_node)->child_repeated, (nccr_pos1), (nccr_pos2))



/* Ensure that an IPset is in-core.  If required, it calls the
 * function to change an mmap()ed IPset to an in-core IPset */
#define IPSET_COPY_ON_WRITE(cow_ipset)                          \
    if (!(cow_ipset)->s.v3->mapped_file) { /* no-op */ } else { \
        int cow_rv = ipsetCopyOnWrite(cow_ipset);               \
        if (cow_rv) {                                           \
            return cow_rv;                                      \
        }                                                       \
    }

/* If the leaf buffer was reallocated, check to see if any leaves can
 * be reclaimed by combining adjacent CIDR blocks */
#define IPSET_MAYBE_COMBINE(mc_ipset)                           \
    if (!(mc_ipset)->s.v3->realloc_leaves) { /*no-op*/ } else { \
        ipsetCombineAdjacentCIDR(mc_ipset);                     \
        (mc_ipset)->s.v3->realloc_leaves = 0;                   \
    }


/* Macros for checking and manipulating the bitmaps on the
 * ipset_node_vX_t structures. */
#define SET_BMAP_DECLARE(sbd_map, sbd_size)     \
    BITMAP_DECLARE(sbd_map, sbd_size)

#define SET_BMAP_SET(sbs_map, sbs_pos)          \
    BITMAP_SETBIT(sbs_map, sbs_pos)

#define SET_BMAP_GET(sbg_map, sbg_pos)          \
    BITMAP_GETBIT(sbg_map, sbg_pos)

#define SET_BMAP_CLEAR(sbc_map, sbc_pos)        \
    BITMAP_CLEARBIT(sbc_map, sbc_pos)

#define SET_BMAP_CLEAR_ALL(sbca_map)            \
    BITMAP_INIT(sbca_map)

#if BITMAP_SIZE_NUM_CHILDREN == 1

#define SET_BMAP_RANGE_SET(sbrs_map, sbrs_beg, sbrs_end)                \
    SET_MASKED_BITS((sbrs_map)[_BMAP_INDEX(sbrs_beg)], UINT32_MAX,      \
                    (sbrs_beg) & 0x1F, 1+(sbrs_end)-(sbrs_beg))

#define SET_BMAP_RANGE_CLEAR(sbrs_map, sbrs_beg, sbrs_end)      \
    SET_MASKED_BITS((sbrs_map)[_BMAP_INDEX(sbrs_beg)], 0,       \
                    (sbrs_beg) & 0x1F, 1+(sbrs_end)-(sbrs_beg))

#else  /* #if BITMAP_SIZE_NUM_CHILDREN == 1 */

#define SET_BMAP_RANGE_SET(sbrs_map, sbrs_beg, sbrs_end)                \
    do {                                                                \
        uint32_t sbrs_i = _BMAP_INDEX(sbrs_beg);                        \
        uint32_t sbrs_j = _BMAP_INDEX(sbrs_end);                        \
        if (sbrs_i == sbrs_j) {                                         \
            /* single word */                                           \
            SET_MASKED_BITS((sbrs_map)[sbrs_i], UINT32_MAX,             \
                            (sbrs_beg) & 0x1F, 1+(sbrs_end)-(sbrs_beg)); \
        } else {                                                        \
            SET_MASKED_BITS((sbrs_map)[sbrs_i], UINT32_MAX,             \
                            (sbrs_beg) & 0x1F, 32-((sbrs_beg) & 0x1F)); \
            ++sbrs_i;                                                   \
            memset(&(sbrs_map)[sbrs_i], 0xFF,                           \
                   (sbrs_j - sbrs_i) * sizeof(uint32_t));               \
            SET_MASKED_BITS((sbrs_map)[sbrs_j], UINT32_MAX,             \
                            0, 1 + ((sbrs_end) & 0x1F));                \
        }                                                               \
    } while(0)

#define SET_BMAP_RANGE_CLEAR(sbrs_map, sbrs_beg, sbrs_end)              \
    do {                                                                \
        uint32_t sbrs_i = _BMAP_INDEX(sbrs_beg);                        \
        uint32_t sbrs_j = _BMAP_INDEX(sbrs_end);                        \
        if (sbrs_i == sbrs_j) {                                         \
            /* single word */                                           \
            SET_MASKED_BITS((sbrs_map)[sbrs_i], 0,                      \
                            (sbrs_beg) & 0x1F, 1+(sbrs_end)-(sbrs_beg)); \
        } else {                                                        \
            SET_MASKED_BITS((sbrs_map)[sbrs_i], 0,                      \
                            (sbrs_beg) & 0x1F, 32-((sbrs_beg) & 0x1F)); \
            ++sbrs_i;                                                   \
            memset(&(sbrs_map)[sbrs_i], 0,                              \
                   (sbrs_j - sbrs_i) * sizeof(uint32_t));               \
            SET_MASKED_BITS((sbrs_map)[sbrs_j], 0,                      \
                            0, 1 + ((sbrs_end) & 0x1F));                \
        }                                                               \
    } while(0)

#endif  /* #else of BITMAP_SIZE_NUM_CHILDREN == 1 */


/* Given an IPv4 address of a child and the prefix of the node holding
 * that child, return the index of child[] where the address
 * belongs */
#define WHICH_CHILD_V4(wc_ip, wc_prefix)                                \
    (((wc_ip) >> (32 - NUM_BITS - ((wc_prefix) & ~(NUM_BITS - 1))))     \
     & ((1u << NUM_BITS) - 1))

#if   NUM_BITS == 3
#elif NUM_BITS == 5
#elif NUM_BITS == 7
#else
#define WHICH_CHILD_V6(wc_ip, wc_prefix)                        \
    (((wc_prefix) >= 64)                                        \
     ? (((wc_ip)->ip[1]                                         \
         >> (128 - NUM_BITS - ((wc_prefix) & ~(NUM_BITS - 1)))) \
        & ((1u << NUM_BITS) - 1))                               \
     : (((wc_ip)->ip[0]                                         \
         >> (64 - NUM_BITS - ((wc_prefix) & ~(NUM_BITS - 1))))  \
        & ((1u << NUM_BITS) - 1)))
#endif


/* Convert from an IPv6 skipaddr_t 'aviv_addr' to ipset_ipv6_t
 * 'aviv_ip' */
#define IPSET_IPV6_FROM_ADDRV6(aviv_ip, aviv_addr6)     \
    {                                                   \
        skipaddrGetV6((aviv_addr6), (aviv_ip));         \
        (aviv_ip)->ip[0] = ntoh64((aviv_ip)->ip[0]);    \
        (aviv_ip)->ip[1] = ntoh64((aviv_ip)->ip[1]);    \
    }

/* Convert from an IPv4 skipaddr_t 'aviv_addr' to ipset_ipv6_t 'aviv_ip' */
#define IPSET_IPV6_FROM_ADDRV4(aviv_ip, aviv_addr4)     \
    {                                                   \
        skipaddrGetAsV6((aviv_addr4), (aviv_ip));       \
        (aviv_ip)->ip[0] = ntoh64((aviv_ip)->ip[0]);    \
        (aviv_ip)->ip[1] = ntoh64((aviv_ip)->ip[1]);    \
    }

/* Convert from ipset_ipv6_t 'aviv_ip' to an IPv6 skipaddr_t 'aviv_addr' */
#define IPSET_IPV6_TO_ADDR(aviv_ip, aviv_addr)          \
    {                                                   \
        ipset_ipv6_t aviv_tmp;                          \
        aviv_tmp.ip[0] = hton64((aviv_ip)->ip[0]);      \
        aviv_tmp.ip[1] = hton64((aviv_ip)->ip[1]);      \
        skipaddrSetV6((aviv_addr), &aviv_tmp);          \
    }

/* Convert from ipset_ipv6_t 'aviv_ip' to an IPv4 skipaddr_t 'aviv_addr' */
#define IPSET_IPV6_TO_ADDR_V4(aviv_ip, aviv_addr)                       \
    {                                                                   \
        uint32_t aviv_ipv4;                                             \
        assert(0 == (aviv_ip)->ip[0]);                                  \
        assert(UINT64_C(0x0000ffff00000000)                             \
               == (UINT64_C(0xffffffff00000000) & (aviv_ip)->ip[1]));   \
        aviv_ipv4 = (uint32_t)(UINT64_C(0x00000000ffffffff)             \
                               & (aviv_ip)->ip[1]);                     \
        skipaddrSetV4((aviv_addr), &aviv_ipv4);                         \
    }

/* Convert from ipset_ipv6_t 'aviv_ip' to an array[16] of uint8_t
 * 'aviv_array' */
#define IPSET_IPV6_TO_ARRAY(aviv_ip, aviv_array)                \
    {                                                           \
        ipset_ipv6_t aviv_tmp;                                  \
        aviv_tmp.ip[0] = hton64((aviv_ip)->ip[0]);              \
        aviv_tmp.ip[1] = hton64((aviv_ip)->ip[1]);              \
        memcpy((aviv_array), &aviv_tmp, IPSET_LEN_V6);          \
    }

/* Convert from an array[16] of uint8_t 'aviv_array' to an
 * ipset_ipv6_t 'aviv_ip' */
#define IPSET_IPV6_FROM_ARRAY(aviv_ip, aviv_array)              \
    {                                                           \
        ipset_ipv6_t aviv_tmp;                                  \
        memcpy(&aviv_tmp, (aviv_array), IPSET_LEN_V6);          \
        (aviv_ip)->ip[0] = hton64(aviv_tmp.ip[0]);              \
        (aviv_ip)->ip[1] = hton64(aviv_tmp.ip[1]);              \
    }

/* Copy an 'ipset_ipv6_t' */
#define IPSET_IPV6_COPY(ipc_dest, ipc_src)      \
    memcpy(ipc_dest, ipc_src, IPSET_LEN_V6)

/* Apply netblock mask 'ipac_prefix' to the ipset_ipv6_t 'ipac_ip' */
#define IPSET_IPV6_APPLY_CIDR(ipac_ip, ipac_prefix)                     \
    if ((ipac_prefix) > 64) {                                           \
        (ipac_ip)->ip[1] &= ~(UINT64_MAX >> ((ipac_prefix) - 64));      \
    } else {                                                            \
        (ipac_ip)->ip[1] = 0;                                           \
        if ((ipac_prefix) < 64) {                                       \
            (ipac_ip)->ip[0] &= ~(UINT64_MAX >> (ipac_prefix));         \
        }                                                               \
    }

/* Copy an 'ipset_ipv6_t' and apply the CIDR mask */
#define IPSET_IPV6_COPY_AND_MASK(ipcam_dest, ipcam_src, ipcam_prefix)   \
    if ((ipcam_prefix) > 64) {                                          \
        (ipcam_dest)->ip[0] = (ipcam_src)->ip[0];                       \
        (ipcam_dest)->ip[1] = ((ipcam_src)->ip[1]                       \
                               & ~(UINT64_MAX >> ((ipcam_prefix)-64))); \
    } else if ((ipcam_prefix) == 64) {                                  \
        (ipcam_dest)->ip[0] = (ipcam_src)->ip[0];                       \
        (ipcam_dest)->ip[1] = 0;                                        \
    } else {                                                            \
        (ipcam_dest)->ip[0] = ((ipcam_src)->ip[0]                       \
                               & ~(UINT64_MAX >> (ipcam_prefix)));      \
        (ipcam_dest)->ip[1] = 0;                                        \
    }

/* Return true if 'ipset_ipv6_t' is 0 */
#define IPSET_IPV6_IS_ZERO(ipiz)                        \
    ((0 == (ipiz)->ip[0]) && (0 == (ipiz)->ip[1]))

/* Set 'mb_result_ptr' to the number of leading 0 bits in
 * 'mb_expression', a 32 bit value */
#define COUNT_MATCHING_BITS32(mb_result_ptr, mb_expression)     \
    {                                                           \
        uint32_t t1, t2;                                        \
        t1 = (mb_expression);                                   \
        t2 = t1 >> 16;                                          \
        if (t2) {                                               \
            t1 = t2 >> 8;                                       \
            *(mb_result_ptr) = (t1                              \
                                ? bit_diff_offset[t1]           \
                                : (8 + bit_diff_offset[t2]));   \
        } else {                                                \
            t2 = t1 >> 8;                                       \
            *(mb_result_ptr) = (t2                              \
                                ? (16 + bit_diff_offset[t2])    \
                                : (24 + bit_diff_offset[t1]));  \
        }                                                       \
    }

/* Set 'mb_result_ptr' to the number of leading 0 bits in
 * 'mb_expression', a 64 bit value */
#define COUNT_MATCHING_BITS64(mb_result_ptr, mb_expression)             \
    {                                                                   \
        uint64_t t1, t2;                                                \
        t2 = (mb_expression);                                           \
        t1 = t2 >> 32;                                                  \
        if (t1) {                                                       \
            t2 = t1 >> 16;                                              \
            if (t2) {                                                   \
                t1 = t2 >> 8;                                           \
                *(mb_result_ptr) = (t1                                  \
                                    ? bit_diff_offset[t1]               \
                                    : (8 + bit_diff_offset[t2]));       \
            } else {                                                    \
                t2 = t1 >> 8;                                           \
                *(mb_result_ptr) = (t2                                  \
                                    ? (16 + bit_diff_offset[t2])        \
                                    : (24 + bit_diff_offset[t1]));      \
            }                                                           \
        } else {                                                        \
            t1 = t2 >> 16;                                              \
            if (t1) {                                                   \
                t2 = t1 >> 8;                                           \
                *(mb_result_ptr) = (t2                                  \
                                    ? (32 + bit_diff_offset[t2])        \
                                    : (40 + bit_diff_offset[t1]));      \
            } else {                                                    \
                t1 = t2 >> 8;                                           \
                *(mb_result_ptr) = (t1                                  \
                                    ? (48 + bit_diff_offset[t1])        \
                                    : (56 + bit_diff_offset[t2]));      \
            }                                                           \
        }                                                               \
    }


/* In IPSET_REC_VERSION_CIDR_BMAP files, mapping from the 'prefix'
 * byte to size of the bitmap */
#define SET_CIDR_BMAP_256   0x81


#if   !defined(NUM_BITS)
#  error "NUM_BITS must be defined"
#elif NUM_BITS <= 0
#  errror "NUM_BITS must be > 0"
#endif


/* Macros used by the SiLK-2 (IPTree) version of IPsets */

/*  The number of uint32_t values for each /24.  Value is computed by
 *  ((1 << 8) / (1 << 5)) ==> (1 << 3) ==> 8 */
#define IPTREE_WORDS_PER_SLASH24  8

#define skIPTreeNodeSet(addr, node)                                     \
    ((node)->addressBlock[(((addr) & 0xFFFF)>>5)] |= (1 << ((addr)&0x1F)))

#define skIPTreeNodeClear(addr, node)                                   \
    ((node)->addressBlock[(((addr) & 0xFFFF)>>5)] &= ~(1 << ((addr)&0x1F)))

#define IPTREE_NODE_ALLOC(ipset, high16)                        \
    if ((ipset)->s.v2->nodes[(high16)]) { /*no-op*/ } else {    \
        (ipset)->s.v2->nodes[(high16)]                          \
            = (skIPNode_t*)calloc(1, sizeof(skIPNode_t));       \
        if (NULL == (ipset)->s.v2->nodes[(high16)]) {           \
            return SKIPSET_ERR_ALLOC;                           \
        }                                                       \
    }

#ifdef   NDEBUG
#define  ASSERT_OK(func_call)  func_call
#else
#define  ASSERT_OK(func_call)                   \
    {                                           \
        int assert_ok = func_call;              \
        assert(0 == assert_ok);                 \
    }
#endif


/* define an ipv6 address type */
typedef struct ipset_ipv6_st {
    uint64_t ip[2];
} ipset_ipv6_t;

/* Forward declare the node types */
typedef struct ipset_node_v4_st ipset_node_v4_t;
typedef struct ipset_node_v6_st ipset_node_v6_t;
typedef union ipset_node_un ipset_node_t;

typedef struct ipset_leaf_v4_st ipset_leaf_v4_t;
typedef struct ipset_leaf_v6_st ipset_leaf_v6_t;
typedef union ipset_leaf_un ipset_leaf_t;

typedef struct ipset_buffer_st {
    uint8_t        *buf;
    /* the size of an entry in the array */
    uint32_t        entry_size;
    /* the number of nodes in the array of entries. bytes allocated is
     * given by (entry_capacity * entry_size). */
    uint32_t        entry_capacity;
    /* maximum index used in the array.  this is not the number of
     * valid entries since it also includes nodes on the free-list. */
    uint32_t        entry_count;
    /* location of first available element in free-list, or 0 if empty */
    uint32_t        free_list;
} ipset_buffer_t;


/* THE IPSET Structure as of SiLK 3 */
typedef struct skipset_v3_st {
    /* pointer to mmap()ed file */
    void                       *mapped_file;
    /* the size of the mmap()ed space */
    size_t                      mapped_size;
    /* structures to hold the nodes and leaves */
    ipset_buffer_t              nodes;
    ipset_buffer_t              leaves;
    /* location of the root of tree; may be a node or a leaf; which it
     * is is given by the root_is_leaf member below */
    uint32_t                    root_idx;
    /* whether root_idx is a leaf or a node */
    unsigned                    root_is_leaf :1;
    /* whether changes to tree caused the "leaves" buffer to be
     * reallocated */
    unsigned                    realloc_leaves :1;
} skipset_v3_t;

/* A COMMON IPSET Structure */
struct skipset_st {
    /* options used when writing an IPset */
    const skipset_options_t    *options;
    /* which pointer to use in this union depends on the setting of
     * the 'is_iptree' member */
    union body_un {
        skipset_v3_t   *v3;
        skIPTree_t     *v2;
    }                           s;
    /* whether the 's' member holds a SiLK-3 IPset (==0) or a SiLK-2
     * IPTree (==1).  See also 'is_ipv6'. */
    unsigned                    is_iptree :1;
    /* whether the SiLK-3 IPset holds IPv6 addresses.  When this is 1,
     * the 'is_iptree' member must be 0. */
    unsigned                    is_ipv6 :1;
    /* whether the tree has changed since last call to skIPSetClean() */
    unsigned                    is_dirty:1;
    /* whether an attempt to insert an IPv6 address into an IPv4 tree
     * should returnan error.  when this is 0, the tree will
     * automatically be converted to IPv6. */
    unsigned                    no_autoconvert :1;
};



/* IPSET NODE */

/* The nodes of a tree hold either IPv4 or IPv6 addresses.  The
 * structures are indentical except for the IP address, which allows
 * safe access to all non-IP elements. */

struct ipset_node_v4_st {
    /* index of the children, which may be nodes or leaves. for a node
     * on free_list, child[0] is the next of the next node on the
     * free_list */
    uint32_t        child[IPSET_NUM_CHILDREN];

    /* bitmap that specifies whether a child[] refers to another node
     * or to a leaf. */
    SET_BMAP_DECLARE(child_is_leaf, IPSET_NUM_CHILDREN);

    /* bitmap that specifies whether a child[] is a repeat of the
     * child to its left.  this is used when a leaf's prefix is larger
     * (holds more IPs) than node->prefix + NUM_BITS. */
    SET_BMAP_DECLARE(child_repeated, IPSET_NUM_CHILDREN);

    /* prefix is number of significant bits in the IP; on the nodes,
     * prefix is the number of bits to compare before deciding which
     * child[] to branch to. */
    uint8_t         prefix;

    /* reserved/unsed bytes */
    uint8_t         reserved3;
    uint8_t         reserved2;
    uint8_t         reserved1;

    /* IP address */
    uint32_t        ip;
};

struct ipset_node_v6_st {
    uint32_t        child[IPSET_NUM_CHILDREN];

    SET_BMAP_DECLARE(child_is_leaf, IPSET_NUM_CHILDREN);
    SET_BMAP_DECLARE(child_repeated, IPSET_NUM_CHILDREN);

    uint8_t         prefix;
    uint8_t         reserved3;
    uint8_t         reserved2;
    uint8_t         reserved1;

#if SK_ENABLE_IPV6
    uint32_t        pad_align;
    ipset_ipv6_t    ip;
#else
    uint32_t        ip;
#endif
};

/* A generic "node" pointer */
/* from above: typedef union ipset_node_un ipset_node_t; */
union ipset_node_un {
    ipset_node_v6_t v6;
    ipset_node_v4_t v4;
};


/*  IPSET LEAF  */

struct ipset_leaf_v4_st {
    /* prefix is number of significant bits in the IP. */
    uint8_t         prefix;
    /* reserved/unused bytes */
    uint8_t         reserved3;
    uint8_t         reserved2;
    uint8_t         reserved1;

    /* IP address */
    uint32_t        ip;
};

struct ipset_leaf_v6_st {
    uint8_t         prefix;
    uint8_t         reserved3;
    uint8_t         reserved2;
    uint8_t         reserved1;

#if SK_ENABLE_IPV6
    uint32_t        pad_align;
    ipset_ipv6_t    ip;
#else
    uint32_t        ip;
#endif
};

/* A generic "leaf" pointer */
/* from above: typedef union ipset_leaf_un ipset_leaf_t; */
union ipset_leaf_un {
    ipset_leaf_v6_t v6;
    ipset_leaf_v4_t v4;
};

/* Support structure for counting the IPs in an IPset, used by
 * skIPSetCount() and ipsetCountCallbackV*() */
typedef struct ipset_count_st {
    uint64_t    upper;
    uint64_t    lower;
} ipset_count_t;

/* Support structure for ipsetFind() */
typedef struct ipset_find_st {
    uint32_t    parent_idx;
    uint32_t    node_idx;
    uint32_t    parents_child;
    int         result;
    uint8_t     bitpos;
    uint8_t     node_is_leaf;
} ipset_find_t;

/* Support structure for intersecting two IPsets; used by
 * skIPSetIntersect() and ipsetIntersectCallback() */
typedef struct ipset_intersect_st {
    skipaddr_t          addr[2];
    sk_vector_t        *vec_add;
    skipset_iterator_t  iter;
} ipset_intersect_t;

/* Support structure for printing the IPs in an IPset, used by
 * skIPSetPrint() and ipsetPrintCallback() */
typedef struct ipset_print_st {
    const skipset_t    *ipset;
    skstream_t         *stream;
    skipaddr_flags_t    ip_format;
} ipset_print_t;

typedef int (ipset_walk_v4_fn_t)(
    uint32_t            ipv4,
    uint32_t            prefix,
    void               *cb_data);

typedef int (ipset_walk_v6_fn_t)(
    const ipset_ipv6_t *ipv6,
    uint32_t            prefix,
    void               *cb_data);


/* Support structure for walking over the IPset */
typedef struct ipset_walk_st {
    /* user function to invoke for each IP/CIDR block */
    skipset_walk_fn_t   callback;
    /* user-provided data to pass to the callback */
    void               *cb_data;
    /* how to handle IP addresses */
    sk_ipv6policy_t     v6policy;
    /* whether to return CIDR blocks (1) or individual IPs (0).  (This
     * may eventually become the maximum size block to return.) */
    uint8_t             cidr_blocks;
} ipset_walk_t;

/* Support strucuture for writing a SiLK-3 IPset in the SiLK-2 file
 * format; used by ipsetWriteLegacyFromRadix() and
 * ipsetWriteLegacyFromRadixCallback(). */
typedef struct ipset_write_silk2_st {
    /* the stream to write to */
    skstream_t *stream;
    /* the current /24 is in buffer[0]; remaining values are a bitmap
     * of addresses for that /24. */
    uint32_t    buffer[1+IPTREE_WORDS_PER_SLASH24];
    /* true when the buffer contains valid data */
    unsigned    buffer_is_dirty :1;
} ipset_write_silk2_t;


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


/* LOCAL VARIABLE DEFINITIONS */

/*
 *    Whether to use the IPTree or Radix-Tree data structure for
 *    in-memory IPv4-IPsets.  Used by IPSET_USE_IPTREE.  This is -1 if
 *    the value has not been initialized; 0 for RADIX, and 1 for
 *    IPTREE.  See ipsetCheckFormatEnvar().
 */
static int ipset_use_iptree = -1;

/* for handling IPset files Version 2 and Version 4 */
static const uint32_t bmap256_zero[IPTREE_WORDS_PER_SLASH24] =
    {0, 0, 0, 0, 0, 0, 0, 0};
static const uint32_t bmap256_full[IPTREE_WORDS_PER_SLASH24] =
    {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX,
     UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};

/* this is used to find which bit of two uint8_t's differ. */
static const uint8_t bit_diff_offset[256] =
{
    /*   0- 15 */  8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    /*  16- 31 */  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    /*  32- 47 */  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    /*  48- 63 */  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    /*  64- 79 */  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*  80- 95 */  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*  96-111 */  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 112-127 */  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 128-143 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 144-159 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 160-175 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 176-191 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 192-207 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 208-223 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 224-239 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 240-255 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/* LOCAL FUNCTION PROTOTYPES */

static int
ipsetCheckFormatEnvar(
    void);
static int
ipsetCreate(
    skipset_t         **ipset_out,
    int                 support_ipv6,
    int                 force_radix);
static void
ipsetDestroySubtree(
    skipset_t          *ipset,
    uint32_t            destroy_node_idx,
    int                 destroy_self);
static int
ipsetFindV4(
    const skipset_t    *ipset,
    const uint32_t      ipv4,
    const uint32_t      prefix,
    ipset_find_t       *find_state);
#if SK_ENABLE_IPV6
static int
ipsetFindV6(
    const skipset_t    *ipset,
    const ipset_ipv6_t *ipv6,
    const uint32_t      prefix,
    ipset_find_t       *find_state);
#endif
static int
ipsetNewEntries(
    skipset_t          *ipset,
    uint32_t            num_nodes,
    uint32_t            num_leaves,
    uint32_t           *node_indexes,
    uint32_t           *leaf_indexes);
static uint32_t
ipsetReplaceNodeWithLeaf(
    skipset_t          *ipset,
    ipset_node_t       *parent,
    uint32_t            which_child);
static int
ipsetVerify(
    const skipset_t    *ipset);


/* FUNCTION DEFINITIONS */

/*
 *  status = ipsetAllocEntries(ibuf, new_cap);
 *
 *    Grow or shink the entries list in 'ibuf' to hold 'new_cap'
 *    nodes.  If 'new_cap' is 0, the size of the current array is
 *    doubled.  If no memory is currently allocated, an array of
 *    IPSET_INITIAL_ENTRY_COUNT nodes is allocated.
 *
 *    Return 0 on success, or SKIPSET_ERR_ALLOC if memory cannot be
 *    allocated.
 */
static int
ipsetAllocEntries(
    ipset_buffer_t     *ibuf,
    size_t              new_cap)
{
    size_t old_cap;
    uint8_t *old_entries;

    assert(ibuf);

    /* the current capacity of the tree */
    old_cap = ibuf->entry_capacity;

    /* handle empty tree */
    if (0 == old_cap) {
        if (0 == new_cap) {
            new_cap = IPSET_INITIAL_ENTRY_COUNT;
        }
        /* allocate new memory */
        ibuf->buf = (uint8_t*)calloc(new_cap, ibuf->entry_size);
        if (NULL == ibuf->buf) {
            return SKIPSET_ERR_ALLOC;
        }
        ibuf->entry_capacity = new_cap;
        return 0;
    }

    /* handle case where no new capacity is specified */
    if (0 == new_cap) {
        new_cap = 2 * old_cap;
        if (new_cap < IPSET_INITIAL_ENTRY_COUNT) {
            new_cap = IPSET_INITIAL_ENTRY_COUNT;
        }
    }

    /* realloc */
    old_entries = ibuf->buf;
    ibuf->buf = (uint8_t*)realloc(old_entries, (new_cap * ibuf->entry_size));
    /* handle failure */
    if (NULL == ibuf->buf) {
        ibuf->buf = old_entries;
        return SKIPSET_ERR_ALLOC;
    }

    /* bzero the new memory */
    if (old_cap < new_cap) {
        memset((ibuf->buf + (old_cap * ibuf->entry_size)),
               0, ((new_cap - old_cap) * ibuf->entry_size));
    }

    ibuf->entry_capacity = new_cap;
    return 0;
}


/*
 *  use_iptree = ipsetCheckFormatEnvar();
 *
 *    Check the value of the environment variable specified by the
 *    IPSET_ENVAR_INCORE_FORMAT macro and set the global
 *    'ipset_use_iptree' variable appropriately.  If the envar is not
 *    set, set 'ipset_use_iptree' to the default structure specified
 *    by IPSET_USE_IPTREE_DEFAULT.
 */
static int
ipsetCheckFormatEnvar(
    void)
{
    const char *envar;

    if (ipset_use_iptree >= 0) {
        return ipset_use_iptree;
    }
    envar = getenv(IPSET_ENVAR_INCORE_FORMAT);
    if (envar) {
        if (0 == strcasecmp("iptree", envar)) {
            ipset_use_iptree = 1;
            return ipset_use_iptree;
        }
        if (0 == strcasecmp("radix", envar)) {
            ipset_use_iptree = 0;
            return ipset_use_iptree;
        }
    }
    ipset_use_iptree = IPSET_USE_IPTREE_DEFAULT;
    return ipset_use_iptree;
}


/*
 *  status = ipsetCheckIPSetCallbackV4(ipaddr, prefix, ipset);
 *  status = ipsetCheckIPSetCallbackV6(ipaddr, prefix, ipset);
 *
 *    Callback functions used by skIPSetCheckIPSet().
 *
 *    Check whether the 'ipaddr'/'prefix' is in 'ipset'.  If it is
 *    not, return SKIPSET_OK so iteration continues.  Otherwise,
 *    return SKIPSET_ERR_SUBSET so iteration stops.
 */
static int
ipsetCheckIPSetCallbackV4(
    skipaddr_t         *ipaddr,
    uint32_t            prefix,
    void               *v_search_set)
{
    uint32_t ipv4;

    if (skipaddrGetAsV4(ipaddr, &ipv4)) {
        return SKIPSET_OK;
    }
    switch (ipsetFindV4((const skipset_t*)v_search_set, ipv4, prefix, NULL)) {
      case SKIPSET_OK:
      case SKIPSET_ERR_SUBSET:
        return SKIPSET_ERR_SUBSET;
      default:
        break;
    }
    return SKIPSET_OK;
}

#if SK_ENABLE_IPV6
static int
ipsetCheckIPSetCallbackV6(
    skipaddr_t         *ipaddr,
    uint32_t            prefix,
    void               *v_search_set)
{
    ipset_ipv6_t ipv6;

    IPSET_IPV6_FROM_ADDRV6(&ipv6, ipaddr);

    switch (ipsetFindV6((const skipset_t*)v_search_set, &ipv6, prefix, NULL)) {
      case SKIPSET_OK:
      case SKIPSET_ERR_SUBSET:
        return SKIPSET_ERR_SUBSET;
      default:
        break;
    }
    return SKIPSET_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  ipsetCombineSubtreeV4(ipset, parent_idx, node_idx, which_child);
 *  ipsetCombineSubtreeV6(ipset, parent_idx, node_idx, which_child);
 *
 *    Helper function for ipsetCombineAdjacentCIDR().
 *
 *    Reduce the number of nodes used in the 'ipset' by combining
 *    nodes that form a contiguous block into a larger block and by
 *    removing nodes that contain a single leaf.
 *
 *    The algorithm uses a recursive, depth first traversal from
 *    'node_idx'.
 *
 *    The 'node_idx' passed into this function must not be a leaf
 *    node.
 */
static void
ipsetCombineSubtreeV4(
    skipset_t          *ipset,
    uint32_t            parent_idx,
    uint32_t            node_idx,
    uint32_t            which_child)
{
    ipset_node_v4_t *parent;
    ipset_node_v4_t *node;
    ipset_leaf_v4_t *leaf1;
    ipset_leaf_v4_t *leaf2;
    uint32_t first_child;
    uint32_t last_child;
    uint32_t child[IPSET_NUM_CHILDREN];
    uint32_t depth;
    uint32_t i;
    uint32_t j;

    /* note the position of the first and last child on the node so we
     * can later determine whether all children on this node point to
     * the same leaf. */
    first_child = IPSET_NUM_CHILDREN;
    last_child = IPSET_NUM_CHILDREN;

    /* the child[] array is a stack of child positions waiting to be
     * merged into a larger contiguous block if possible.  depth is
     * the index into the child[] array. */
    depth = 0;

    node = NODE_PTR_V4(ipset, node_idx);

    /* find child in highest position */
    for (i = IPSET_NUM_CHILDREN; i > 0; ) {
        --i;
        if (node->child[i]) {
            last_child = i;
            break;
        }
    }

    if (IPSET_NUM_CHILDREN == last_child) {
        /* node has no children!! */
        if (IPSET_NO_PARENT == parent_idx) {
            skIPSetRemoveAll(ipset);
        } else {
            parent = NODE_PTR_V4(ipset, parent_idx);
            parent->child[which_child] = 0;
        }
        return;
    }

    do {
        if (0 == node->child[i]) {
            depth = 0;
            continue;
        }
        if (NODEPTR_CHILD_IS_REPEAT(node, i)) {
            continue;
        }
        first_child = i;
        if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
            ipsetCombineSubtreeV4(ipset, node_idx, node->child[i], i);
            if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                depth = 0;
                continue;
            }
        }
        if ((0 == depth) || (i & 0x1)) {
            child[depth++] = i;
            continue;
        }
        while (depth) {
            /* for two leaves to be joinable, they must have the same
             * prefix and the XOR of their values must be less than
             * the IP range covered by the proposed prefix, which is
             * one less than the current prefix. */
            leaf1 = LEAF_PTR_V4(ipset, node->child[i]);
            leaf2 = LEAF_PTR_V4(ipset, node->child[child[depth-1]]);
            if (leaf1->prefix != leaf2->prefix
                || ((leaf1->ip ^ leaf2->ip) >= (1u << (33 - leaf1->prefix))))
            {
                /* cannot be joined */
                break;
            }

            /* combine these two leaves */
            --leaf1->prefix;
            leaf1->ip &= ~(UINT32_MAX >> leaf1->prefix);
            j = child[depth-1];
            LEAFIDX_FREE(ipset, node->child[j]);
            NODEPTR_CHILD_SET_REPEAT2(node, i+1, j);
            do {
                node->child[j] = node->child[i];
                ++j;
            } while (j < IPSET_NUM_CHILDREN
                     && NODEPTR_CHILD_IS_REPEAT(node, j));
            --depth;
        }
        child[depth++] = i;

    } while (i-- > 0);

    if (node->child[first_child] != node->child[last_child]
        || !NODEPTR_CHILD_IS_LEAF(node, first_child)
        || !NODEPTR_CHILD_IS_LEAF(node, last_child))
    {
        return;
    }

    /* replace the node with the leaf */
    if (IPSET_NO_PARENT == parent_idx) {
        IPSET_ROOT_INDEX_SET(ipset, node->child[first_child], 1);
    } else {
        parent = NODE_PTR_V4(ipset, parent_idx);
        parent->child[which_child] = node->child[first_child];
        SET_BMAP_SET(parent->child_is_leaf, which_child);
    }
    NODEIDX_FREE(ipset, node_idx);
}

#if SK_ENABLE_IPV6
static void
ipsetCombineSubtreeV6(
    skipset_t          *ipset,
    uint32_t            parent_idx,
    uint32_t            node_idx,
    uint32_t            which_child)
{
    ipset_node_v6_t *parent;
    ipset_node_v6_t *node;
    ipset_leaf_v6_t *leaf1;
    ipset_leaf_v6_t *leaf2;
    uint32_t first_child;
    uint32_t last_child;
    uint32_t child[IPSET_NUM_CHILDREN];
    uint32_t depth;
    uint32_t i;
    uint32_t j;

    /* note the position of the first and last child on the node so we
     * can later determine whether all children on this node point to
     * the same leaf. */
    first_child = IPSET_NUM_CHILDREN;
    last_child = IPSET_NUM_CHILDREN;

    /* the child[] array is a stack of child positions waiting to be
     * merged into a larger contiguous block if possible.  depth is
     * the index into the child[] array. */
    depth = 0;

    node = NODE_PTR_V6(ipset, node_idx);

    /* find child in highest position */
    for (i = IPSET_NUM_CHILDREN; i > 0; ) {
        --i;
        if (node->child[i]) {
            last_child = i;
            break;
        }
    }

    if (IPSET_NUM_CHILDREN == last_child) {
        /* node has no children!! */
        if (IPSET_NO_PARENT == parent_idx) {
            skIPSetRemoveAll(ipset);
        } else {
            parent = NODE_PTR_V6(ipset, parent_idx);
            parent->child[which_child] = 0;
        }
        return;
    }

    do {
        if (0 == node->child[i]) {
            depth = 0;
            continue;
        }
        if (NODEPTR_CHILD_IS_REPEAT(node, i)) {
            continue;
        }
        first_child = i;
        if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
            ipsetCombineSubtreeV6(ipset, node_idx, node->child[i], i);
            if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                depth = 0;
                continue;
            }
        }
        if ((0 == depth) || (i & 0x1)) {
            child[depth++] = i;
            continue;
        }
        while (depth) {
            /* for two leaves to be joinable, the must have the same
             * prefix and the XOR of their values must be less than 1
             * shifted by the proposed prefix subtracted from 32,
             * where the proposed prefix is one less than the current
             * prefix. */
            leaf1 = LEAF_PTR_V6(ipset, node->child[i]);
            leaf2 = LEAF_PTR_V6(ipset, node->child[child[depth-1]]);
            if (leaf1->prefix != leaf2->prefix) {
                break;
            }
            if (leaf1->prefix <= 64) {
                /* this change affects the most significant 64 bits */
                if ((leaf1->ip.ip[0] ^ leaf2->ip.ip[0])
                    >= (UINT64_C(1) << (65 - leaf1->prefix)))
                {
                    /* cannot be joined */
                    break;
                }
                /* combine these two leaves */
                --leaf1->prefix;
                leaf1->ip.ip[0] &= ~(UINT64_MAX >> leaf1->prefix);
            } else if (leaf1->prefix == 65) {
                /* this change affects the least significant 64 bits */
                if (leaf1->ip.ip[0] != leaf2->ip.ip[0]) {
                    /* cannot be joined. */
                    break;
                }
                /* a prefix of 65 must succeed, since the ranges must
                 * be W:X:Y:Z:0::/65 and W:X:Y:Z:8000::/65 */
                --leaf1->prefix;
                leaf1->ip.ip[1] = 0;
            } else {
                /* this change affects the least significant 64 bits */
                if ((leaf1->ip.ip[0] != leaf2->ip.ip[0])
                    || ((leaf1->ip.ip[1] ^ leaf2->ip.ip[1])
                        >= (UINT64_C(1) << (129 - leaf1->prefix))))
                {
                    /* cannot be joined. */
                    break;
                }
                --leaf1->prefix;
                leaf1->ip.ip[1] &= ~(UINT64_MAX >> (leaf1->prefix - 64));
            }

            j = child[depth-1];
            LEAFIDX_FREE(ipset, node->child[j]);
            NODEPTR_CHILD_SET_REPEAT2(node, i+1, j);
            do {
                node->child[j] = node->child[i];
                ++j;
            } while (j < IPSET_NUM_CHILDREN
                     && NODEPTR_CHILD_IS_REPEAT(node, j));
            --depth;
        }
        child[depth++] = i;

    } while (i-- > 0);

    if (node->child[first_child] != node->child[last_child]
        || !NODEPTR_CHILD_IS_LEAF(node, first_child)
        || !NODEPTR_CHILD_IS_LEAF(node, last_child))
    {
        return;
    }

    /* replace the node with the leaf */
    if (IPSET_NO_PARENT == parent_idx) {
        IPSET_ROOT_INDEX_SET(ipset, node->child[first_child], 1);
    } else {
        parent = NODE_PTR_V6(ipset, parent_idx);
        parent->child[which_child] = node->child[first_child];
        NODEPTR_CHILD_SET_LEAF(parent, which_child);
    }
    NODEIDX_FREE(ipset, node_idx);
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  ipsetCombineAdjacentCIDR(ipset);
 *
 *    Visit the nodes in the tree looking for leaves that form a
 *    contiguous block of IPs.  When found, combine the leaves into a
 *    single leaf.  This will make reduce the size required by the
 *    tree.
 */
static void
ipsetCombineAdjacentCIDR(
    skipset_t          *ipset)
{
    if (!IPSET_ROOT_IS_LEAF(ipset)) {
#if SK_ENABLE_IPV6
        if (ipset->is_ipv6) {
            ipsetCombineSubtreeV6(ipset, IPSET_NO_PARENT,
                                  IPSET_ROOT_INDEX(ipset), IPSET_NUM_CHILDREN);
        } else
#endif  /* SK_ENABLE_IPV6 */
        {
            ipsetCombineSubtreeV4(ipset, IPSET_NO_PARENT,
                                  IPSET_ROOT_INDEX(ipset), IPSET_NUM_CHILDREN);
        }
    }
}


/*
 *  ipsetCompact(ipset);
 *
 *    Make the nodes and leaves arrays in the 'ipset' use contiguous
 *    blocks of memory.  Any holes in the arrays---represented by
 *    nodes/leaves in the free-list---will become occupied.  When this
 *    function returns, the memory will be contiguous and the free
 *    list will be empty.
 */
static void
ipsetCompact(
    skipset_t          *ipset)
{
    ipset_node_t *node;
    ipset_leaf_t *leaf;
    uint32_t nodes_in_use;
    uint32_t nodes_free_idx;
    uint32_t leaves_in_use;
    uint32_t leaves_free_idx;
    uint32_t to_visit[IPSET_MAX_DEPTH];
    uint32_t depth;
    int i;

    /* if the free lists are empty, the tree is compact */
    if (0 == ipset->s.v3->nodes.free_list
        && 0 == ipset->s.v3->leaves.free_list)
    {
        return;
    }

    /* handle a leaf at the root as a special case */
    if (IPSET_ROOT_IS_LEAF(ipset)) {
        /* values 1 and 2 due to node#0 and leaf#0 never used */
        nodes_in_use = 1;
        leaves_in_use = 2;
        if (IPSET_ROOT_INDEX(ipset) >= leaves_in_use) {
            leaves_free_idx = 1;
            /* copy the child to this location */
            memcpy(LEAF_PTR(ipset, leaves_free_idx),
                   LEAF_PTR(ipset, IPSET_ROOT_INDEX(ipset)),
                   ipset->s.v3->leaves.entry_size);
            /* update the root's pointer to the child */
            IPSET_ROOT_INDEX_SET(ipset, leaves_free_idx, 1);
        }
        goto CLEAR_MEMORY;
    }

    /* determine the number of nodes currently in use by subtracting
     * the nodes on the free list from the total number of nodes */
    nodes_in_use = ipset->s.v3->nodes.entry_count;
    for (nodes_free_idx = ipset->s.v3->nodes.free_list;
         0 != nodes_free_idx;
         nodes_free_idx = NODEPTR_FREE_LIST(node))
    {
        assert(nodes_free_idx < ipset->s.v3->nodes.entry_count);
        --nodes_in_use;
        node = NODE_PTR(ipset, nodes_free_idx);
    }

    if (ipset->s.v3->nodes.entry_count < nodes_in_use) {
        /* the only way for this to happen is for the nodes_in_use
         * value to underflow 0 */
        skAbort();
    }

    /* repeat the process for the leaves */
    leaves_in_use = ipset->s.v3->leaves.entry_count;
    for (leaves_free_idx = ipset->s.v3->leaves.free_list;
         0 != leaves_free_idx;
         leaves_free_idx = LEAFPTR_FREE_LIST(leaf))
    {
        assert(leaves_free_idx < ipset->s.v3->leaves.entry_count);
        --leaves_in_use;
        leaf = LEAF_PTR(ipset, leaves_free_idx);
    }

    if (ipset->s.v3->leaves.entry_count < leaves_in_use) {
        skAbort();
    }

    /* Since Node#0 and Leaf#0 are never used, nodes_in_use and
     * leaves_in_use can be 0 or they can be a value >= 2, but they
     * should never be 1. */
    /* However, I am leary of enabling these asserts.
     * assert(nodes_in_use != 1);
     * assert(leaves_in_use != 1);
     */

    /* handle the root */
    assert(0 == IPSET_ROOT_IS_LEAF(ipset));
    if (IPSET_ROOT_INDEX(ipset) >= nodes_in_use) {
        /* pop nodes from the free list until we find one smaller
         * than nodes_in_use. */
        do {
            nodes_free_idx = ipset->s.v3->nodes.free_list;
            ipset->s.v3->nodes.free_list
                = NODEIDX_FREE_LIST(ipset, nodes_free_idx);
        } while (nodes_free_idx >= nodes_in_use);
        assert(nodes_free_idx != 0);

        /* copy the child to the location we just popped
         * from the free_list */
        memcpy(NODE_PTR(ipset, nodes_free_idx),
               NODE_PTR(ipset, IPSET_ROOT_INDEX(ipset)),
               ipset->s.v3->nodes.entry_size);

        /* update the root's pointer to the child */
        IPSET_ROOT_INDEX_SET(ipset, nodes_free_idx, 0);
    }

    /* visit the nodes in the tree. */
    depth = 0;
    to_visit[depth++] = IPSET_ROOT_INDEX(ipset);

    while (depth) {
        node = NODE_PTR(ipset, to_visit[--depth]);

        /* if the indexes of the children of this node are larger than
         * 'nodes_in_use' or leaves_in_use', pop an entry from
         * the appropriate free-list and copy that child to the new
         * location.  handled just like the root. */
        for (i = 0; i < IPSET_NUM_CHILDREN; ++i) {
            if (node->v4.child[i]) {
                if (!NODEPTR_CHILD_IS_LEAF(&node->v4, i)) {
                    /* this is a node */
                    if (node->v4.child[i] >= nodes_in_use) {
                        do {
                            nodes_free_idx = ipset->s.v3->nodes.free_list;
                            ipset->s.v3->nodes.free_list
                                = NODEIDX_FREE_LIST(ipset, nodes_free_idx);
                        } while (nodes_free_idx >= nodes_in_use);
                        assert(nodes_free_idx != 0);
                        memcpy(NODE_PTR(ipset, nodes_free_idx),
                               NODE_PTR(ipset, node->v4.child[i]),
                               ipset->s.v3->nodes.entry_size);
                        node->v4.child[i] = nodes_free_idx;
                    }
                    /* we must visit this child */
                    to_visit[depth++] = node->v4.child[i];

                } else {
                    /* this is a leaf: it is handled just slightly
                     * differently than a node */
                    if (node->v4.child[i] >= leaves_in_use) {
                        do {
                            leaves_free_idx = ipset->s.v3->leaves.free_list;
                            ipset->s.v3->leaves.free_list
                                = LEAFIDX_FREE_LIST(ipset, leaves_free_idx);
                        } while (leaves_free_idx >= leaves_in_use);
                        assert(leaves_free_idx != 0);
                        memcpy(LEAF_PTR(ipset, leaves_free_idx),
                               LEAF_PTR(ipset, node->v4.child[i]),
                               ipset->s.v3->leaves.entry_size);
                        node->v4.child[i] = leaves_free_idx;

                        /* handle repeated leaf */
                        while ((i < (IPSET_NUM_CHILDREN - 1))
                               && NODEPTR_CHILD_IS_REPEAT(&node->v4, i+1))
                        {
                            ++i;
                            node->v4.child[i] = leaves_free_idx;
                        }
                    }
                }
            }
        }
    }

  CLEAR_MEMORY:
    /* we have compacted the tree.  bzero the memory we just cleared,
     * set the entry_counts to the number of active entries, and reset
     * the free lists. */
    memset((ipset->s.v3->nodes.buf
            + (nodes_in_use * ipset->s.v3->nodes.entry_size)),
           0, ((ipset->s.v3->nodes.entry_count - nodes_in_use)
               * ipset->s.v3->nodes.entry_size));
    ipset->s.v3->nodes.entry_count = nodes_in_use;
    ipset->s.v3->nodes.free_list = 0;

    memset((ipset->s.v3->leaves.buf
            + (leaves_in_use * ipset->s.v3->leaves.entry_size)),
           0, ((ipset->s.v3->leaves.entry_count - leaves_in_use)
               * ipset->s.v3->leaves.entry_size));
    ipset->s.v3->leaves.entry_count = leaves_in_use;
    ipset->s.v3->leaves.free_list = 0;
}


#if SK_ENABLE_IPV6
/*
 *  status = ipsetConvertIPTreetoV6(ipset);
 *
 *    Convert the contents of a SiLK-2 based IPset (IPTree) to a
 *    SiLK-3 based IPv6 IPset.  The IPv4 addresses in the IPset will
 *    be mapped into the "::ffff:0.0.0.0/96" address space.  Return
 *    SKIPSET_OK on success, or SKIPSET_ERR_ALLOC if memory cannot be
 *    allocated to hold the IPv6 addresses.
 */
static int
ipsetConvertIPTreetoV6(
    skipset_t          *ipset)
{
    skipset_t *tmp_ipset;
    skIPTree_t *iptree;
    skIPTreeCIDRBlockIterator_t cidr_iter;
    skIPTreeCIDRBlock_t cidr;
    skipaddr_t ipaddr;
    int rv;

    assert(ipset);
    assert(1 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);
    assert(0 == ipset->no_autoconvert);

    rv = ipsetCreate(&tmp_ipset, 1, 1);
    if (rv) {
        return rv;
    }

    iptree = ipset->s.v2;
    ipset->s.v3 = tmp_ipset->s.v3;
    ipset->is_iptree = 0;
    ipset->is_ipv6 = 1;

    ASSERT_OK(skIPTreeCIDRBlockIteratorBind(&cidr_iter, iptree));
    while ((rv = skIPTreeCIDRBlockIteratorNext(&cidr, &cidr_iter))
           == SK_ITERATOR_OK)
    {
        skipaddrSetV6FromUint32(&ipaddr, &cidr.addr);
        cidr.mask += 96;

        rv = skIPSetInsertAddress(ipset, &ipaddr, cidr.mask);
        if (rv) {
            /* restore ipset to its original settings */
            ipset->s.v2 = iptree;
            ipset->is_iptree = 1;
            ipset->is_ipv6 = 0;

            skIPSetDestroy(&tmp_ipset);
            return rv;
        }
    }

    tmp_ipset->is_ipv6 = 0;
    tmp_ipset->is_iptree = 1;
    tmp_ipset->s.v2 = iptree;

    skIPSetDestroy(&tmp_ipset);

    skIPSetClean(ipset);

    return SKIPSET_OK;
}


/*
 *  status = ipsetConvertV4toV6(ipset);
 *
 *    Helper function for skIPSetConvert();
 *
 *    Convert the contents of a Radix-Tree based 'ipset' so it
 *    contains IPv6 addresses.
 *
 *    The IPv4 addresses in the IPset will be mapped into the
 *    "::ffff:0.0.0.0/96" address space.  Return SKIPSET_OK on
 *    success, or SKIPSET_ERR_ALLOC if memory cannot be allocated to
 *    hold the IPv6 addresses.
 */
static int
ipsetConvertV4toV6(
    skipset_t          *ipset)
{
    ipset_node_v6_t *node6;
    ipset_node_v4_t *node4;
    ipset_leaf_v6_t *leaf6;
    ipset_leaf_v4_t *leaf4;
    size_t num_entries;
    size_t ip_offset;
    uint32_t i;
    int rv;

    assert(ipset);
    assert(0 == ipset->is_iptree);
    assert(sizeof(ipset_node_v4_t) == ipset->s.v3->nodes.entry_size);

    if (IPSET_ISEMPTY(ipset)) {
        goto SET_ATTRIBUTES;
    }

    /* compute the number of bytes required to hold the nodes once
     * they are converted to IPv6, then divide by the IPv4 node size
     * to get the number of IPv4 nodes to request.  The -1/+1 handle a
     * partial node. */
    num_entries
        = (((ipset->s.v3->nodes.entry_count * sizeof(ipset_node_v6_t) - 1)
            / sizeof(ipset_node_v4_t)) + 1);
    if (num_entries > ipset->s.v3->nodes.entry_capacity) {
        rv = ipsetAllocEntries(&ipset->s.v3->nodes, num_entries);
        if (rv) {
            return rv;
        }
    }

    /* repeat for the leaves */
    num_entries
        = (((ipset->s.v3->leaves.entry_count * sizeof(ipset_leaf_v6_t) - 1)
            / sizeof(ipset_leaf_v4_t)) + 1);
    if (num_entries > ipset->s.v3->leaves.entry_capacity) {
        rv = ipsetAllocEntries(&ipset->s.v3->leaves, num_entries);
        if (rv) {
            return rv;
        }
    }

    /* modify the nodes */

    ip_offset = offsetof(ipset_node_v4_t, ip);
    assert(offsetof(ipset_node_v6_t, ip) >= ip_offset);

    /* break the NODE_PTR() abstraction here.  Count from
     * 'entry_count' down to 1, converting each node from a V4 node to
     * a V6 node. */
    node4 = &(((ipset_node_v4_t*)ipset->s.v3->nodes.buf)
              [ipset->s.v3->nodes.entry_count-1]);
    node6 = &(((ipset_node_v6_t*)ipset->s.v3->nodes.buf)
              [ipset->s.v3->nodes.entry_count-1]);

    for (i = ipset->s.v3->nodes.entry_count - 1; i > 0; --i, --node4, --node6){
        /* copy and convert the IP; must do the IP first, since its
         * location is overwritten by the memmove() call */
        node6->ip.ip[1] = UINT64_C(0xffff00000000) | node4->ip;
        node6->ip.ip[0] = 0;
        /* use memmove to copy the node except for the IP; must use
         * memmove() since the nodes overlap */
        memmove(node6, node4, ip_offset);
        node6->prefix += 96;
        /* node6->pad_align = 0 */
    }
    assert(0 == i);
    assert((void*)node4 == (void*)ipset->s.v3->nodes.buf);
    assert((void*)node6 == (void*)ipset->s.v3->nodes.buf);

    /* node-0 is unused; clear out the IP address */
    node6->ip.ip[0] = 0;
    node6->ip.ip[1] = 0;

    /* repeat for the leaves */

    ip_offset = offsetof(ipset_leaf_v4_t, ip);
    assert(offsetof(ipset_leaf_v6_t, ip) >= ip_offset);

    /* break the LEAF_PTR() abstraction here.  Count from
     * 'entry_count' down to 0, converting each leaf from a V4 leaf to
     * a V6 leaf. */
    leaf4 = &(((ipset_leaf_v4_t*)ipset->s.v3->leaves.buf)
              [ipset->s.v3->leaves.entry_count-1]);
    leaf6 = &(((ipset_leaf_v6_t*)ipset->s.v3->leaves.buf)
              [ipset->s.v3->leaves.entry_count-1]);

    for (i = ipset->s.v3->leaves.entry_count - 1; i > 0; --i, --leaf4, --leaf6)
    {
        /* use memcpy to copy the leaf; can use memcpy, since leaf6 is
         * twice the size of leaf4 */
        memcpy(leaf6, leaf4, ip_offset);
        leaf6->prefix += 96;
        /* copy and convert the IP */
        leaf6->ip.ip[1] = UINT64_C(0xffff00000000) | leaf4->ip;
        leaf6->ip.ip[0] = 0;
    }
    assert(0 == i);
    assert((void*)leaf4 == (void*)ipset->s.v3->leaves.buf);
    assert((void*)leaf6 == (void*)ipset->s.v3->leaves.buf);

    /* clear the IP address in leaf-0 */
    leaf6->ip.ip[0] = 0;
    leaf6->ip.ip[1] = 0;

  SET_ATTRIBUTES:
    /* set the IPset attributes for IPv6 */
    ipset->s.v3->nodes.entry_size = sizeof(ipset_node_v6_t);
    ipset->s.v3->leaves.entry_size = sizeof(ipset_leaf_v6_t);
    ipset->is_ipv6 = 1;
    ipset->is_dirty = 1;

    /* adjust the capacities.  this may result in a few wasted bytes
     * until the next realloc() */
    ipset->s.v3->nodes.entry_capacity
        = (ipset->s.v3->nodes.entry_capacity
           * sizeof(ipset_node_v4_t) / sizeof(ipset_node_v6_t));
    assert(ipset->s.v3->nodes.entry_capacity >= ipset->s.v3->nodes.entry_count);
    ipset->s.v3->leaves.entry_capacity
        = (ipset->s.v3->leaves.entry_capacity
           * sizeof(ipset_leaf_v4_t) / sizeof(ipset_leaf_v6_t));
    assert(ipset->s.v3->leaves.entry_capacity
           >= ipset->s.v3->leaves.entry_count);

    return SKIPSET_OK;
}


/*
 *  status = ipsetConvertV6toV4(ipset);
 *
 *    Helper function for skIPSetConvert();
 *
 *    Convert the contents of 'ipset' so it contains only IPv4
 *    addresses.  This function should not be called on IPsets that
 *    contain IPv6 addresses outside of "::ffff:0.0.0.0/96".
 */
static int
ipsetConvertV6toV4(
    skipset_t          *ipset)
{
    ipset_node_v6_t *node6;
    ipset_node_v4_t *node4;
    ipset_leaf_v6_t *leaf6;
    ipset_leaf_v4_t *leaf4;
    size_t ip_offset;
    uint32_t i;

    assert(ipset);
    assert(0 == ipset->is_iptree);
    assert(sizeof(ipset_node_v6_t) == ipset->s.v3->nodes.entry_size);
    assert(sizeof(ipset_leaf_v6_t) == ipset->s.v3->leaves.entry_size);
    assert(0 == skIPSetContainsV6(ipset));

    if (IPSET_ISEMPTY(ipset)) {
        goto SET_ATTRIBUTES;
    }

    ip_offset = offsetof(ipset_node_v4_t, ip);
    assert(offsetof(ipset_node_v6_t, ip) >= ip_offset);

    /* break the NODE_PTR() abstraction here.  Count from 0 to
     * 'entry_count', converting each node from a V6 node to a V4
     * node. */
    node4 = (ipset_node_v4_t*)ipset->s.v3->nodes.buf;
    node6 = (ipset_node_v6_t*)ipset->s.v3->nodes.buf;

    /* just need to set the IP to 0 for node 0 */
    node4->ip = 0;

    for (i = 1, ++node4, ++node6;
         i < ipset->s.v3->nodes.entry_count;
         ++i, ++node4, ++node6)
    {
        memmove(node4, node6, ip_offset);
        node4->ip = (uint32_t)(node6->ip.ip[1] & UINT32_MAX);
        node4->prefix -= 96;
    }


    /* Repeat the same process for the leaves */

    ip_offset = offsetof(ipset_leaf_v4_t, ip);
    assert(offsetof(ipset_leaf_v6_t, ip) >= ip_offset);

    leaf4 = (ipset_leaf_v4_t*)ipset->s.v3->leaves.buf;
    leaf6 = (ipset_leaf_v6_t*)ipset->s.v3->leaves.buf;

    leaf4->ip = 0;

    for (i = 1, ++leaf4, ++leaf6;
         i < ipset->s.v3->leaves.entry_count;
         ++i, ++leaf4, ++leaf6)
    {
        memcpy(leaf4, leaf6, ip_offset);
        leaf4->ip = (uint32_t)(leaf6->ip.ip[1] & UINT32_MAX);
        leaf4->prefix -= 96;
    }

  SET_ATTRIBUTES:
    /* set the IPset attributes for IPv4 */
    ipset->s.v3->nodes.entry_size = sizeof(ipset_node_v4_t);
    ipset->s.v3->leaves.entry_size = sizeof(ipset_leaf_v4_t);
    ipset->is_ipv6 = 0;
    ipset->is_dirty = 1;

    /* adjust the capacities.  this may result in a waste of a few
     * bytes */
    ipset->s.v3->nodes.entry_capacity
        = (ipset->s.v3->nodes.entry_capacity
           * sizeof(ipset_node_v6_t) / sizeof(ipset_node_v4_t));
    ipset->s.v3->leaves.entry_capacity
        = (ipset->s.v3->leaves.entry_capacity
           * sizeof(ipset_leaf_v6_t) / sizeof(ipset_leaf_v4_t));

    return SKIPSET_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status  = ipsetCopyOnWrite(ipset);
 *
 *    Use the IPSET_COPY_ON_WRITE() macro to invoke this function,
 *    which only calls this function when necessary.
 *
 *    Change the IPset 'ipset' so that it uses in-core memory to hold
 *    its nodes instead of using mmap()ed data.  Return 0 on success,
 *    or SKIPSET_ERR_ALLOC if memory cannot be allocated.
 */
static int
ipsetCopyOnWrite(
    skipset_t          *ipset)
{
    uint8_t *node_buf;
    uint8_t *leaf_buf;
    int rv;

    assert(ipset->s.v3->mapped_file && ipset->s.v3->mapped_size);

    /* cache the current buffer values in case the allocation fails */
    node_buf = ipset->s.v3->nodes.buf;
    leaf_buf = ipset->s.v3->leaves.buf;

    /* clear the values so we can allocate the space */
    ipset->s.v3->nodes.buf = NULL;
    ipset->s.v3->nodes.entry_capacity = 0;
    ipset->s.v3->leaves.buf = NULL;
    ipset->s.v3->leaves.entry_capacity = 0;

    rv = ipsetAllocEntries(&ipset->s.v3->nodes, ipset->s.v3->nodes.entry_count);
    if (rv) {
        ipset->s.v3->nodes.buf = node_buf;
        ipset->s.v3->nodes.entry_capacity = ipset->s.v3->nodes.entry_count;
        ipset->s.v3->leaves.buf = leaf_buf;
        ipset->s.v3->leaves.entry_capacity = ipset->s.v3->leaves.entry_count;
        return rv;
    }
    rv = ipsetAllocEntries(&ipset->s.v3->nodes, ipset->s.v3->nodes.entry_count);
    if (rv) {
        free(ipset->s.v3->nodes.buf);
        ipset->s.v3->nodes.buf = node_buf;
        ipset->s.v3->nodes.entry_capacity = ipset->s.v3->nodes.entry_count;
        ipset->s.v3->leaves.buf = leaf_buf;
        ipset->s.v3->leaves.entry_capacity = ipset->s.v3->leaves.entry_count;
        return rv;
    }

    /* copy the mmap()ed data into the newly allocated space */
    memcpy(ipset->s.v3->nodes.buf, node_buf,
           (ipset->s.v3->nodes.entry_count * ipset->s.v3->nodes.entry_size));
    memcpy(ipset->s.v3->leaves.buf, leaf_buf,
           (ipset->s.v3->leaves.entry_count * ipset->s.v3->leaves.entry_size));

    /* unmap the space */
    munmap(ipset->s.v3->mapped_file, ipset->s.v3->mapped_size);

    ipset->s.v3->mapped_file = NULL;
    ipset->s.v3->mapped_size = 0;

    return SKIPSET_OK;
}


/*
 *  status = ipsetCountCallbackV4(ipv4, prefix, &count_state)
 *  status = ipsetCountCallbackV6(ipv6, prefix, &count_state)
 *
 *    Helper function for skIPSetCount().
 *
 *    Compute (1 << 'prefix') to get the number of IPs in this block
 *    and update the count in 'count_state'.
 */
static int
ipsetCountCallbackV4(
    uint32_t     UNUSED(ipv4),
    uint32_t            prefix,
    void               *v_count_state)
{
    ipset_count_t *count_state = (ipset_count_t*)v_count_state;

    count_state->lower += ((uint64_t)1) << (32 - prefix);
    return 0;
}

#if SK_ENABLE_IPV6
static int
ipsetCountCallbackV6(
    const ipset_ipv6_t  UNUSED(*ipv6),
    uint32_t                    prefix,
    void                       *v_count_state)
{
    ipset_count_t *count_state = (ipset_count_t*)v_count_state;
    uint64_t tmp;

    if (prefix <= 64) {
        count_state->upper += (((uint64_t)1) << (64 - prefix));
    } else {
        tmp = (((uint64_t)1) << (128 - prefix));
        if ((UINT64_MAX - count_state->lower) > tmp) {
            count_state->lower += tmp;
        } else {
            ++count_state->upper;
            count_state->lower -= ((UINT64_MAX - tmp) + 1);
        }
    }

    return 0;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  occupied = ipsetCountOccupiedLeaves(ipset);
 *
 *    Return the number of occupied leaves in 'ipset'.
 */
static uint32_t
ipsetCountOccupiedLeaves(
    const skipset_t    *ipset)
{
    const ipset_leaf_t *leaf;
    uint32_t leaves_in_use;
    uint32_t leaves_free_idx;

    /* handle a leaf at the root as a special case */
    if (IPSET_ROOT_IS_LEAF(ipset)) {
        return 1;
    }

    /* determine the number of leaves currently in use by subtracting
     * the leaves on the free list from the total number of leaves */
    leaves_in_use = ipset->s.v3->leaves.entry_count;
    for (leaves_free_idx = ipset->s.v3->leaves.free_list;
         0 != leaves_free_idx;
         leaves_free_idx = LEAFPTR_FREE_LIST(leaf))
    {
        assert(leaves_free_idx < ipset->s.v3->leaves.entry_count);
        --leaves_in_use;
        leaf = LEAF_PTR(ipset, leaves_free_idx);
    }

    if (ipset->s.v3->leaves.entry_count < leaves_in_use) {
        skAbort();
    }

    return leaves_in_use;
}


/*
 *  status = ipsetCreate(ipset, support_ipv6, force_radix);
 *
 *    Helper function for skIPSetCreate() and used internally.
 *
 *    Allocates and initializes a new IPset at the space specified by
 *    '*ipset'.  The set is initially empty.  When 'support_ipv6' is
 *    non-zero, the IPset will be initialized to store IPv6 addresses;
 *    otherwise it will be initialized to hold IPv4 addresses.  When
 *    creating an IPv4 IPset, the IPTree format from SiLK-2 will be
 *    used unless the 'force_radix' parameter has a non-zero value.
 */
static int
ipsetCreate(
    skipset_t         **ipset_out,
    int                 support_ipv6,
    int                 force_radix)
{
    skipset_t *set;

    assert(ipset_out);

    set = (skipset_t*)calloc(1, sizeof(skipset_t));
    if (!set) {
        return SKIPSET_ERR_ALLOC;
    }

    if (support_ipv6) {
        set->s.v3 = (skipset_v3_t*)calloc(1, sizeof(skipset_v3_t));
        if (!set->s.v3) {
            free(set);
            return SKIPSET_ERR_ALLOC;
        }
        set->s.v3->nodes.entry_size = sizeof(ipset_node_v6_t);
        set->s.v3->leaves.entry_size = sizeof(ipset_leaf_v6_t);
        set->is_ipv6 = 1;
        set->is_iptree = 0;
    } else if (force_radix) {
        set->s.v3 = (skipset_v3_t*)calloc(1, sizeof(skipset_v3_t));
        if (!set->s.v3) {
            free(set);
            return SKIPSET_ERR_ALLOC;
        }
        set->s.v3->nodes.entry_size = sizeof(ipset_node_v4_t);
        set->s.v3->leaves.entry_size = sizeof(ipset_leaf_v4_t);
        set->is_ipv6 = 0;
        set->is_iptree = 0;
    } else {
        if (skIPTreeCreate(&set->s.v2)) {
            free(set);
            return SKIPSET_ERR_ALLOC;
        }
        set->is_ipv6 = 0;
        set->is_iptree = 1;
    }

    *ipset_out = set;
    return SKIPSET_OK;


}


/*
 *  ipsetDestroySubtree(ipset, node_idx, destroy_self);
 *
 *    Puts all the child nodes of 'node_idx' onto the free list.  If
 *    'destroy_self' is non-zero, the 'node_idx' node is also added to
 *    the free list.  If 'destroy_self' is zero, the child indexes on
 *    'node_idx' are set to 0.
 */
static void
ipsetDestroySubtree(
    skipset_t          *ipset,
    uint32_t            node_idx,
    int                 destroy_self)
{
    uint32_t to_visit[IPSET_MAX_DEPTH];
    uint32_t depth = 0;
    ipset_node_t *node;
    int i;

    if (destroy_self) {
        /* add this node to the list of nodes to destroy */
        to_visit[depth++] = node_idx;
    } else {
        /* destroy any leaves off of this node; add this node's child
         * nodes to the list list of nodes to destroy */
        node = NODE_PTR(ipset, node_idx);
        for (i = 0; i < IPSET_NUM_CHILDREN; ++i) {
            if (node->v4.child[i]
                && !NODEPTR_CHILD_IS_REPEAT(&node->v4, i))
            {
                if (NODEPTR_CHILD_IS_LEAF(&node->v4, i)) {
                    LEAFIDX_FREE(ipset, node->v4.child[i]);
                } else {
                    to_visit[depth++] = node->v4.child[i];
                }
            }
        }
        /* clear the child array and bitmaps */
        memset(node->v4.child, 0, sizeof(node->v4.child));
        SET_BMAP_CLEAR_ALL(node->v4.child_is_leaf);
        SET_BMAP_CLEAR_ALL(node->v4.child_repeated);
    }

    while (depth) {
        node_idx = to_visit[--depth];
        node = NODE_PTR(ipset, node_idx);
        for (i = 0; i < IPSET_NUM_CHILDREN; ++i) {
            if (node->v4.child[i]
                && !NODEPTR_CHILD_IS_REPEAT(&node->v4, i))
            {
                if (NODEPTR_CHILD_IS_LEAF(&node->v4, i)) {
                    LEAFIDX_FREE(ipset, node->v4.child[i]);
                } else {
                    to_visit[depth++] = node->v4.child[i];
                }
            }
        }
        NODEIDX_FREE(ipset, node_idx);
    }
}


/*
 *  status = ipsetFindV4(ipset, ip, prefix, &find_state);
 *  status = ipsetFindV6(ipset, ip, prefix, &find_state);
 *
 *    Searches for 'ip'/'prefix' in the Radix-tree based 'ipset'.
 *    'ip' is an integer presentation of the IP address in native byte
 *    order.
 *
 *    Returns SKIPSET_OK if it is found exactly or if a CIDR block
 *    containing it is found: searching for 10.0.0.2 and find
 *    10.0.0.0/24.
 *
 *    Returns SKIPSET_ERR_SUBSET when a subset of target is found:
 *    search for 10.0.0.0/24 and find 10.0.0.2.
 *
 *    Returns SKIPSET_ERR_EMPTY if the IPset is empty.
 *
 *    Returns SKIPSET_ERR_MULTILEAF if it is not found but the search
 *    failed at a position where a leaf can be inserted.
 *
 *    Returns SKIPSET_ERR_NOTFOUND if it is not found and the
 *    SKIPSET_ERR_MULTILEAF case does not apply.
 *
 *    When 'find_state' is non-NULL, its fields will be set to the
 *    final location visited in the tree.  Specifically, 'node_idx' is
 *    set to the index of the node where the search terminated;
 *    'bitpos' is set to the number of bits of the IP address that
 *    were searched; 'parent_idx' is set to the index of the parent of
 *    the node in 'node_idx'.  If 'node_idx' is the root of the tree,
 *    'parent_idx' will be UINT32_MAX; 'result' will be the same as
 *    the return status of the function.
 */
static int
ipsetFindV4(
    const skipset_t    *ipset,
    const uint32_t      ipv4,
    const uint32_t      prefix,
    ipset_find_t       *find_state)
{
    const ipset_node_v4_t *node;
    const ipset_leaf_v4_t *leaf = NULL;
    uint32_t parent_idx;
    uint32_t node_idx;
    uint32_t which_child;
    uint32_t j;
    uint8_t bitpos;
    int rv;

    /*
     *  The 'bitpos' value specifies the number of bits that match
     *  between the IP on the current node/leaf and the search IP
     *  (starting from the most significant bit).
     *
     *  On a node at any level in the tree, if bitpos is less than the
     *  prefix on that node, we cannot descend to the next level in
     *  the tree.  Otherwise, before descending, we check to see
     *  whether the node's prefix is greater than the search prefix;
     *  if it is, there is no need to continue and the function
     *  returns SUBSET.
     *
     *  Whether the descent into the tree stops due to reaching a leaf
     *  or due to not matching enough bits on a node, the return
     *  status depends on three values: (1)the number of bits that
     *  were matched, (2)the prefix on the node/leaf, and (3)the
     *  search prefix.  If bitpos is less than the search prefix, the
     *  search failed and NOTFOUND is returned.  If both bitpos and
     *  the search prefix are at least equal to the prefix on the
     *  node/leaf, the search returns OK.  For any other combination,
     *  the search returns SUBSET.
     *
     *  If the search is on the node and wants to descend into the
     *  tree but there is no child at that position, the search either
     *  returns SUBSET or MULTILEAF.  The search returns SUBSET if the
     *  search prefix covers multiple children on the node and at
     *  least one of those children is occupied.  Otherwise, the
     *  search returns MULTILEAF, denoting that a leaf can be safely
     *  added at this position.
     */

    assert(ipset);
    assert(prefix > 0 || ipv4 == 0);
    assert(prefix <= 32);
    assert(0 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);

    parent_idx = IPSET_NO_PARENT;
    node_idx = IPSET_ROOT_INDEX(ipset);
    which_child = IPSET_NUM_CHILDREN;
    bitpos = 0;
    rv = SKIPSET_ERR_SUBSET;

    if (IPSET_ISEMPTY(ipset)) {
        rv = SKIPSET_ERR_EMPTY;
        goto END;
    }

    if (IPSET_ROOT_IS_LEAF(ipset)) {
        leaf = LEAF_PTR_V4(ipset, node_idx);
        /* find number of bits this leaf's IP and the search IP have
         * in common */
        COUNT_MATCHING_BITS32(&bitpos, (ipv4 ^ (leaf->ip)));
        if (bitpos < leaf->prefix) {
            if (bitpos < prefix) {
                /* not found */
                rv = SKIPSET_ERR_NOTFOUND;
            } else {
                bitpos = prefix;
                /* rv = SKIPSET_ERR_SUBSET; */
            }
        } else {
            /* make certain bitpos is not greater than this leaf's
             * prefix */
            bitpos = leaf->prefix;
            if (prefix >= leaf->prefix) {
                rv = SKIPSET_OK;
            }
            /* else rv = SKIPSET_ERR_SUBSET; */
        }
        goto END;
    }

    do {
        assert(node_idx < ipset->s.v3->nodes.entry_count);
        node = NODE_PTR_V4(ipset, node_idx);
        if (bitpos < node->prefix) {
            COUNT_MATCHING_BITS32(&bitpos, (ipv4 ^ (node->ip)));
            if (bitpos < node->prefix) {
                /* cannot descend any farther into tree */
                if (bitpos < prefix) {
                    /* not found */
                    rv = SKIPSET_ERR_NOTFOUND;
                    goto END;
                }
                bitpos = prefix;
                /* rv = SKIPSET_ERR_SUBSET; */
                goto END;
            }
            /* else bitpos >= node->prefix */
            if (prefix <= node->prefix) {
                bitpos = prefix;
                /* rv = SKIPSET_ERR_SUBSET; */
                goto END;
            }
            /* else descend to next level */
        }

        /* go to the appropriate child */
        parent_idx = node_idx;
        which_child = WHICH_CHILD_V4(ipv4, node->prefix);
        node_idx = node->child[which_child];
        if (0 == node_idx) {
            if (NUM_BITS > prefix - node->prefix) {
                /* the 'prefix' covers multiple child[] entries; see
                 * if any others are occupied */
                for (j = 1;
                     ((j < (1u << (NUM_BITS - (prefix - node->prefix))))
                      && ((which_child + j) < IPSET_NUM_CHILDREN));
                     ++j)
                {
                    if (node->child[which_child + j]) {
                        /* rv = SKIPSET_ERR_SUBSET; */
                        goto END;
                    }
                }
            }
            node_idx = which_child;
            rv = SKIPSET_ERR_MULTILEAF;
            goto END;
        }
        if (BITMAP_GETBIT(node->child_is_leaf, which_child)) {
            leaf = LEAF_PTR_V4(ipset, node_idx);
            COUNT_MATCHING_BITS32(&bitpos, (ipv4 ^ (leaf->ip)));
            if (bitpos < leaf->prefix) {
                if (bitpos < prefix) {
                    /* not found */
                    rv = SKIPSET_ERR_NOTFOUND;
                } else {
                    bitpos = prefix;
                    /* rv = SKIPSET_ERR_SUBSET; */
                }
            } else {
                bitpos = leaf->prefix;
                if (prefix >= leaf->prefix) {
                    rv = SKIPSET_OK;
                }
                /* else rv = SKIPSET_ERR_SUBSET; */
            }
            goto END;
        }

        bitpos = node->prefix + NUM_BITS;

    } while (bitpos < prefix);

  END:
    if (find_state) {
        find_state->parent_idx = parent_idx;
        find_state->node_idx = node_idx;
        find_state->parents_child = which_child;
        find_state->bitpos = bitpos;
        find_state->result = rv;
        find_state->node_is_leaf = (leaf != NULL);
    }
    return rv;
}

#if SK_ENABLE_IPV6
static int
ipsetFindV6(
    const skipset_t    *ipset,
    const ipset_ipv6_t *ipv6,
    const uint32_t      prefix,
    ipset_find_t       *find_state)
{
    const ipset_node_v6_t *node;
    const ipset_leaf_v6_t *leaf = NULL;
    uint32_t parent_idx;
    uint32_t node_idx;
    uint32_t which_child;
    uint8_t bitpos;
    uint32_t j;
    int ip_idx;
    int rv;

    assert(ipset);
    assert(ipv6);
    assert(0 < prefix && prefix <= 128);
    assert(0 == ipset->is_iptree);
    assert(1 == ipset->is_ipv6);

    parent_idx = IPSET_NO_PARENT;
    node_idx = IPSET_ROOT_INDEX(ipset);
    which_child = IPSET_NUM_CHILDREN;
    bitpos = 0;
    ip_idx = 0;
    rv = SKIPSET_ERR_SUBSET;

    if (IPSET_ISEMPTY(ipset)) {
        rv = SKIPSET_ERR_EMPTY;
        goto END;
    }

    if (IPSET_ROOT_IS_LEAF(ipset)) {
        leaf = LEAF_PTR_V6(ipset, node_idx);
        /* find number of bits this leaf's IP and the search IP have
         * in common */
        if (ipv6->ip[0] == leaf->ip.ip[0]) {
            COUNT_MATCHING_BITS64(&bitpos, (ipv6->ip[1] ^ leaf->ip.ip[1]));
            bitpos += 64;
        } else {
            COUNT_MATCHING_BITS64(&bitpos, (ipv6->ip[0] ^ leaf->ip.ip[0]));
        }
        if (bitpos < leaf->prefix) {
            if (bitpos < prefix) {
                /* not found */
                rv = SKIPSET_ERR_NOTFOUND;
            } else {
                bitpos = prefix;
                /* rv = SKIPSET_ERR_SUBSET; */
            }
        } else {
            /* make certain bitpos is not greater than this leaf's
             * prefix */
            bitpos = leaf->prefix;
            if (prefix >= leaf->prefix) {
                rv = SKIPSET_OK;
            }
            /* else rv = SKIPSET_ERR_SUBSET; */
        }
        goto END;
    }

    do {
        assert(node_idx < ipset->s.v3->nodes.entry_count);
        node = NODE_PTR_V6(ipset, node_idx);
        if (bitpos < node->prefix) {
            COUNT_MATCHING_BITS64(&bitpos,
                                  (ipv6->ip[ip_idx] ^ node->ip.ip[ip_idx]));
            bitpos += (ip_idx * 64);
            if (bitpos < node->prefix) {
                if (64 == bitpos && 0 == ip_idx) {
                    ++ip_idx;
                    continue;
                }
                /* cannot descend any farther into tree */
                if (bitpos < prefix) {
                    /* not found */
                    rv = SKIPSET_ERR_NOTFOUND;
                    goto END;
                }
                bitpos = prefix;
                /* rv = SKIPSET_ERR_SUBSET; */
                goto END;
            }
            /* else bitpos >= node->prefix */
            if (prefix <= node->prefix) {
                bitpos = prefix;
                /* rv = SKIPSET_ERR_SUBSET; */
                goto END;
            }
        }

        /* go to the appropriate child */
        parent_idx = node_idx;
        which_child = WHICH_CHILD_V6(ipv6, node->prefix);
        node_idx = node->child[which_child];
        if (0 == node_idx) {
            if (NUM_BITS > prefix - node->prefix) {
                /* the 'prefix' covers multiple child[] entries; see
                 * if any others are occupied */
                for (j = 1;
                     ((j < (1u << (NUM_BITS - (prefix - node->prefix))))
                      && ((which_child + j) < IPSET_NUM_CHILDREN));
                     ++j)
                {
                    if (node->child[which_child + j]) {
                        /* rv = SKIPSET_ERR_SUBSET; */
                        goto END;
                    }
                }
            }
            node_idx = which_child;
            rv = SKIPSET_ERR_MULTILEAF;
            goto END;
        }
        if (BITMAP_GETBIT(node->child_is_leaf, which_child)) {
            leaf = LEAF_PTR_V6(ipset, node_idx);
            if ((1 == ip_idx) || (ipv6->ip[0] == leaf->ip.ip[0])) {
                COUNT_MATCHING_BITS64(&bitpos, (ipv6->ip[1] ^ leaf->ip.ip[1]));
                bitpos += 64;
            } else {
                COUNT_MATCHING_BITS64(&bitpos, (ipv6->ip[0] ^ leaf->ip.ip[0]));
            }
            if (bitpos < leaf->prefix) {
                if (bitpos < prefix) {
                    /* not found */
                    rv = SKIPSET_ERR_NOTFOUND;
                } else {
                    bitpos = prefix;
                    /* rv = SKIPSET_ERR_SUBSET; */
                }
            } else {
                bitpos = leaf->prefix;
                if (prefix >= leaf->prefix) {
                    rv = SKIPSET_OK;
                }
                /* else rv = SKIPSET_ERR_SUBSET; */
            }
            goto END;
        }

        bitpos = node->prefix + NUM_BITS;

    } while (bitpos < prefix);

  END:
    if (find_state) {
        find_state->parent_idx = parent_idx;
        find_state->node_idx = node_idx;
        find_state->parents_child = which_child;
        find_state->bitpos = bitpos;
        find_state->result = rv;
        find_state->node_is_leaf = (leaf != NULL);
    }
    return rv;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  remove_count = ipsetFixNodeSingleChild(ipset, node_idx, non_recursive);
 *
 *    Determine if the node specified by 'node_idx' has multiple
 *    children.  If it does, do not modify 'ipset' and return 0.
 *
 *    If the node specified by 'node_idx' as one child---which may be
 *    a leaf or another node---modify the node's parent to replace the
 *    node with node's child and return 1.
 *
 *    If the node specified by 'node_idx' has no children, remove the
 *    node from the node's parent.  If 'non_recursive' is non-zero, do
 *    no further processing and return 1.  If 'non_recursive' is 0,
 *    repeat the process using the parent of the node that was just
 *    removed.  For this final case, the return status will be a count
 *    of the number of nodes removed.
 *
 *    I do not believe anything uses the return value; this function
 *    could be changed to "return" void.
 */
static int
ipsetFixNodeSingleChild(
    skipset_t          *ipset,
    uint32_t            node_idx,
    int                 non_recursive)
{
    ipset_find_t find_state;
    ipset_node_t *parent;
    ipset_node_t *node;
    uint32_t which_child;
    uint32_t child_idx = 0;
    uint32_t i;
    int remove_count = 0;

    assert(ipset);
    assert(0 == ipset->is_iptree);
    assert(node_idx > 0 && node_idx < ipset->s.v3->nodes.entry_count);

    node = NODE_PTR(ipset, node_idx);

    for (;;) {
        which_child = IPSET_NUM_CHILDREN;
        for (i = 0; i < IPSET_NUM_CHILDREN; ++i) {
            if ((node->v4.child[i])
                && !NODEPTR_CHILD_IS_REPEAT(&node->v4, i))
            {
                if (which_child != IPSET_NUM_CHILDREN) {
                    /* more than one child; we are done. */
                    return remove_count;
                }
                which_child = i;
            }
        }

        if (which_child < IPSET_NUM_CHILDREN) {
            /* the node has a single child; modify the node's parent
             * to replace the node with the node's child */
            if (!NODEPTR_CHILD_IS_LEAF(&node->v4, which_child)) {
                /* the single child is another node; the easiest way
                 * to handle this case is to copy the child over the
                 * node, and leave the parent untouched */
                node_idx = node->v4.child[which_child];
                memcpy(node, NODE_PTR(ipset, node_idx),
                       ipset->s.v3->nodes.entry_size);
                break;
            }

            child_idx = node->v4.child[which_child];
        }

        /* find the node in order to get a handle to the node's
         * parent */
        if (0 == node->v4.prefix) {
            /* cannot call ipsetFindVx() with a prefix of 0, but that
             * can only happen at the root */
            find_state.parent_idx = IPSET_NO_PARENT;
#if SK_ENABLE_IPV6
        } else if (ipset->is_ipv6) {
#ifndef NDEBUG
            int rv =
#endif
                ipsetFindV6(ipset, &node->v6.ip, node->v6.prefix, &find_state);
            assert(SKIPSET_OK == rv || SKIPSET_ERR_SUBSET == rv);
            assert(find_state.node_idx == node_idx && !find_state.node_is_leaf);
#endif  /* SK_ENABLE_IPV6 */
        } else {
#ifndef NDEBUG
            int rv =
#endif
                ipsetFindV4(ipset, node->v4.ip, node->v4.prefix, &find_state);
            assert(SKIPSET_OK == rv || SKIPSET_ERR_SUBSET == rv);
            assert(find_state.node_idx == node_idx && !find_state.node_is_leaf);
        }

        if (which_child < IPSET_NUM_CHILDREN) {
            /* wire the nodes's parent to the child (which is a leaf),
             * then free the node */
            if (IPSET_NO_PARENT == find_state.parent_idx) {
                /* this node was the root */
                IPSET_ROOT_INDEX_SET(ipset, child_idx, 1);
            } else {
                parent = NODE_PTR(ipset, find_state.parent_idx);
                parent->v4.child[find_state.parents_child] = child_idx;
                SET_BMAP_SET(parent->v4.child_is_leaf,
                             find_state.parents_child);
            }
            break;
        }
        /* delete the node from the parent */
        if (IPSET_NO_PARENT == find_state.parent_idx) {
            /* this node was the root */
            skIPSetRemoveAll(ipset);
            ++remove_count;
            return remove_count;
        }

        parent = NODE_PTR(ipset, find_state.parent_idx);
        parent->v4.child[find_state.parents_child] = 0;
        if (non_recursive) {
            break;
        }

        /* we have deleted the node from the parent.  see if the
         * parent now has a single child  */
        NODEIDX_FREE(ipset, node_idx);
        ++remove_count;
        node_idx = find_state.parent_idx;
        node = parent;
    }

    NODEIDX_FREE(ipset, node_idx);
    ++remove_count;
    return remove_count;
}


/*
 *  status = ipsetInsertAddressIPTree(ipset, ip, prefix);
 *
 *    Helper function for skIPSetInsertAddress(); may also be called
 *    by other internal functions.
 *
 *    Insert the CIDR block 'ip'/'prefix' into 'ipset'.
 */
static int
ipsetInsertAddressIPTree(
    skipset_t          *ipset,
    uint32_t            ipv4,
    uint32_t            prefix)
{
    const uint32_t prefix_as_bits[] = {
        0xFFFFFFFF, 0xFFFF, 0xFF, 0xF, 0x3, 0x1
    };
    uint32_t ipv4_end;

    assert(ipset);
    assert(1 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);
    assert(prefix > 0 || ipv4 == 0);
    assert(prefix <= 32);

    if (prefix >= 27) {
        IPTREE_NODE_ALLOC(ipset, ipv4 >> 16);
        ipset->s.v2->nodes[ipv4 >> 16]->addressBlock[(ipv4 & 0xFFFF) >> 5]
            |= (prefix_as_bits[prefix - 27] << ((ipv4) & 0x1F));

    } else if (prefix <= 16) {
        ipv4_end = ((UINT32_MAX >> prefix) | ipv4) >> 16;
        ipv4 >>= 16;
        do {
            IPTREE_NODE_ALLOC(ipset, ipv4);
            memset(ipset->s.v2->nodes[ipv4]->addressBlock,
                   0xFF, sizeof(skIPNode_t));
        } while (ipv4++ < ipv4_end);

    } else {
        /* 16 < prefix < 27 */

        IPTREE_NODE_ALLOC(ipset, ipv4 >> 16);
        memset(&ipset->s.v2->nodes[ipv4>>16]->addressBlock[(ipv4 & 0xFFFF)>>5],
               0xFF, (sizeof(skIPNode_t) >> (prefix - 16)));
    }

    return SKIPSET_OK;
}


/*
 *  status = ipsetInsertAddressV4(ipset, ip, prefix, find_state);
 *  status = ipsetInsertAddressV6(ipset, ip, prefix, find_state);
 *
 *    Helper function for skIPSetInsertAddress(); may also be called
 *    by other internal functions.
 *
 *    Insert the CIDR block 'ip'/'prefix' into the Radix-Tree based
 *    'ipset'.  'find_state' may be NULL.  If not NULL, 'find_state'
 *    should be the result of calling ipsetFindVx() for the
 *    'ip'/'prefix'.  Do not call this function with a non-NULL
 *    'find_state' if ipsetFindVx() found the IP address.
 *
 *    Return SKIPSET_OK when the IP was successfully inserted, or
 *    SKIPSET_ERR_ALLOC if there is not enough memory to insert the
 *    IP.
 */
static int
ipsetInsertAddressV4(
    skipset_t          *ipset,
    const uint32_t      ipv4,
    const uint32_t      prefix,
    const ipset_find_t *find_state)
{
    ipset_find_t find_state_local;
    ipset_node_v4_t *parent = NULL;
    ipset_leaf_v4_t *leaf = NULL;
    ipset_node_v4_t *new_node;
    uint32_t new_node_idx;
    uint32_t new_leaf_idx[IPSET_NUM_CHILDREN];
    uint32_t which_child;
    uint32_t bitpos;
    uint32_t i;
    uint32_t j;
    int rv;

    assert(ipset);
    assert(prefix > 0 || ipv4 == 0);
    assert(prefix <= 32);
    assert(0 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);

    /* use the passed in 'find_state' if given */
    if (find_state) {
        rv = find_state->result;
    } else {
        rv = ipsetFindV4(ipset, ipv4, prefix, &find_state_local);
        /* if IP was found, we can return */
        if (SKIPSET_OK == rv) {
            return SKIPSET_OK;
        }
        find_state = &find_state_local;
    }
    ipset->is_dirty = 1;

    if (SKIPSET_ERR_EMPTY == rv) {
        /* tree was previously empty */
        /* create a new leaf to hold the IP. create an extra node and
         * extra leaf since node#0 and leaf#0 are always empty */
        if (ipsetNewEntries(ipset, 1, 2, &new_node_idx, new_leaf_idx)) {
            return SKIPSET_ERR_ALLOC;
        }
        assert(0 == new_node_idx);
        assert(0 == new_leaf_idx[0]);
        assert(1 == new_leaf_idx[1]);
        IPSET_ROOT_INDEX_SET(ipset, new_leaf_idx[1], 1);
        leaf = LEAF_PTR_V4(ipset, new_leaf_idx[IPSET_ROOT_INDEX(ipset)]);
        leaf->ip = ipv4;
        leaf->prefix = prefix;
        return SKIPSET_OK;
    }

    if (SKIPSET_ERR_SUBSET == rv) {
        /* we're adding an IP/PREFIX where part of the IP space
         * already exists in the IPSet.  Modify this node's values to
         * hold the larger block, and remove any nodes below here. */
        if (IPSET_NO_PARENT == find_state->parent_idx) {
            if (IPSET_ROOT_IS_LEAF(ipset)) {
                leaf = LEAF_PTR_V4(ipset, find_state->node_idx);
            } else {
                new_leaf_idx[0] = ipsetReplaceNodeWithLeaf(ipset, NULL, 0);
                leaf = LEAF_PTR_V4(ipset, new_leaf_idx[0]);
            }
            leaf->ip = ipv4;
            leaf->prefix = prefix;
            return SKIPSET_OK;
        }

        /* get a handle to the parent */
        parent = NODE_PTR_V4(ipset, find_state->parent_idx);

        if (NUM_BITS <= prefix - parent->prefix) {
            /* leaf has a single child[] entry on the parent */
            if (find_state->node_is_leaf) {
                /* we can modify the leaf and be done */
                leaf = LEAF_PTR_V4(ipset, find_state->node_idx);
            } else {
                /* need to replace the node with a leaf */
                new_leaf_idx[0]
                    = ipsetReplaceNodeWithLeaf(ipset, (ipset_node_t*)parent,
                                               find_state->parents_child);
                leaf = LEAF_PTR_V4(ipset, new_leaf_idx[0]);
            }
            leaf->ip = ipv4;
            leaf->prefix = prefix;
            return SKIPSET_OK;
        }

        /* this leaf will cover several child[] entries on the parent.
         * destroy any existing children and attempt to find a leaf on
         * this level to use  */
        new_leaf_idx[0] = 0;
        for (i = find_state->parents_child, j = 0;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            if (parent->child[i]
                && !NODEPTR_CHILD_IS_REPEAT(parent, i))
            {
                if (!NODEPTR_CHILD_IS_LEAF(parent, i)) {
                    /* delete this subtree */
                    ipsetDestroySubtree(ipset, parent->child[i], 1);
                } else if (0 == new_leaf_idx[0]) {
                    /* found a leaf to use */
                    new_leaf_idx[0] = parent->child[i];
                } else {
                    /* do not need this leaf */
                    LEAFIDX_FREE(ipset, parent->child[i]);
                }
            }
        }

        if (0 == new_leaf_idx[0]) {
            /* no leaves available, need to allocate one */
            if (ipsetNewEntries(ipset, 0, 1, NULL, new_leaf_idx)) {
                return SKIPSET_ERR_ALLOC;
            }
        }

        leaf = LEAF_PTR_V4(ipset, new_leaf_idx[0]);
        leaf->ip = ipv4;
        leaf->prefix = prefix;

        /* set child[] entries on 'parent' to point at the new leaf */
        /* set additional child[]s to also point at the leaf */
        for (i = find_state->parents_child, j = 0;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            parent->child[i] = new_leaf_idx[0];
        }
        NODEPTR_CHILD_SET_LEAF2(parent, find_state->parents_child, i - 1);
        NODEPTR_CHILD_SET_REPEAT2(parent, 1+find_state->parents_child, i - 1);
        return SKIPSET_OK;
    }

    if (SKIPSET_ERR_MULTILEAF == rv) {
        /* get a handle to the current node */
        parent = NODE_PTR_V4(ipset, find_state->parent_idx);

        if (NUM_BITS <= prefix - parent->prefix) {
            /* need to add a single leaf and have a single child[]
             * entry on 'parent' point to that leaf */
            if (ipsetNewEntries(ipset, 0, 1, NULL, new_leaf_idx)) {
                return SKIPSET_ERR_ALLOC;
            }
            /* get a handle to the newly created leaf and copy the new
             * IP there */
            leaf = LEAF_PTR_V4(ipset, new_leaf_idx[0]);
            leaf->ip = ipv4;
            leaf->prefix = prefix;

            /* set pointer on 'parent' to point at the new leaf */
            parent->child[find_state->parents_child] = new_leaf_idx[0];
            NODEPTR_CHILD_SET_LEAF(parent, find_state->parents_child);
            return SKIPSET_OK;
        }

        /* this leaf will cover several child[] entries on the parent.
         * see if any are occupied. */
        new_leaf_idx[0] = 0;
        for (i = find_state->parents_child + 1, j = 1u;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            if (parent->child[i] && NODEPTR_CHILD_IS_LEAF(parent, i)) {
                new_leaf_idx[0] = parent->child[i];
                break;
            }
        }
        if (new_leaf_idx[0] == 0) {
            /* no leaves available, need to allocate one */
            if (ipsetNewEntries(ipset, 0, 1, NULL, new_leaf_idx)) {
                return SKIPSET_ERR_ALLOC;
            }
        }

        leaf = LEAF_PTR_V4(ipset, new_leaf_idx[0]);
        leaf->ip = ipv4;
        leaf->prefix = prefix;

        /* set child[] on 'parent' to point at the new leaf */
        parent->child[find_state->parents_child] = new_leaf_idx[0];

        /* set additional child[]s to also point at the leaf */
        for (i = find_state->parents_child + 1, j = 1u;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            if (parent->child[i]) {
                if (!NODEPTR_CHILD_IS_LEAF(parent, i)) {
                    /* delete this subtree */
                    ipsetDestroySubtree(ipset, parent->child[i], 1);
                } else if (parent->child[i] != new_leaf_idx[0]) {
                    /* do not need this leaf */
                    LEAFIDX_FREE(ipset, parent->child[i]);
                }
            }
            parent->child[i] = new_leaf_idx[0];
        }
        NODEPTR_CHILD_SET_LEAF2(parent, find_state->parents_child, i - 1);
        if (j > 1) {
            NODEPTR_CHILD_SET_REPEAT2(parent, 1 + find_state->parents_child,
                                      i - 1);
        }
        return SKIPSET_OK;
    }

    /* we must add a new node and a new leaf to the tree.  The leaf
     * holds the IP being inserted.  The node holds the CIDR block
     * containing the leaf and our current node---that is, it becomes
     * a new parent; we must also update the index on the current
     * node's parent to point to the new parent. */

    /* create the two new entries */
    if (ipsetNewEntries(ipset, 1, 1, &new_node_idx, new_leaf_idx)) {
        return SKIPSET_ERR_ALLOC;
    }

    /* get a handle to the newly created node */
    new_node = NODE_PTR_V4(ipset, new_node_idx);

    bitpos = find_state->bitpos & ~(NUM_BITS - 1);

    /* get a handle to the newly created leaf, copy the inserted IP
     * there, and link it to the new_node. */
    leaf = LEAF_PTR_V4(ipset, new_leaf_idx[0]);
    leaf->ip = ipv4;
    leaf->prefix = prefix;
    which_child = WHICH_CHILD_V4(ipv4, bitpos);
    if (NUM_BITS <= prefix - bitpos) {
        /* adding a single child[] entry */
        new_node->child[which_child] = new_leaf_idx[0];
        NODEPTR_CHILD_SET_LEAF(new_node, which_child);

    } else {
        /* this leaf will cover several child[] values on the node. */
        for (i = which_child, j = 0;
             j < (1u << (NUM_BITS - (prefix - bitpos)));
             ++i, ++j)
        {
            new_node->child[i] = new_leaf_idx[0];
        }
        NODEPTR_CHILD_SET_LEAF2(new_node, which_child, i - 1);
        NODEPTR_CHILD_SET_REPEAT2(new_node, 1 + which_child, i - 1);
    }

    /* the following code is slightly different depending on whether
     * search stopped on a node or on a leaf */
    if (find_state->node_is_leaf) {
        /* get a handle to the current leaf that is being moved down
         * the tree */
        leaf = LEAF_PTR_V4(ipset, find_state->node_idx);

        /* copy the leaf's IP to the new parent, masking off the lower
         * bits */
        new_node->prefix = bitpos;
        new_node->ip = leaf->ip & ~(UINT32_MAX >> new_node->prefix);

        /* put the leaf under new_node */
        which_child = WHICH_CHILD_V4(leaf->ip, new_node->prefix);
        if (NUM_BITS <= leaf->prefix - new_node->prefix) {
            /* leaf occupies a single child[] entry */
            new_node->child[which_child] = find_state->node_idx;
            NODEPTR_CHILD_SET_LEAF(new_node, which_child);

        } else {
            for (i = which_child, j = 0;
                 j < (1u << (NUM_BITS - (leaf->prefix - new_node->prefix)));
                 ++i, ++j)
            {
                new_node->child[i] = find_state->node_idx;
            }
            NODEPTR_CHILD_SET_LEAF2(new_node, which_child, i - 1);
            NODEPTR_CHILD_SET_REPEAT2(new_node, 1 + which_child, i - 1);
        }

        /* get a handle to the parent, and update the child pointer on
         * the parent */
        if (find_state->parent_idx == IPSET_NO_PARENT) {
            IPSET_ROOT_INDEX_SET(ipset, new_node_idx, 0);
        } else {
            parent = NODE_PTR_V4(ipset, find_state->parent_idx);
            which_child = WHICH_CHILD_V4(leaf->ip, parent->prefix);
            assert(parent->child[which_child] == find_state->node_idx);
            parent->child[which_child] = new_node_idx;
            NODEPTR_CHILD_CLEAR_LEAF(parent, which_child);
        }

    } else {
        /* get a handle to the current node that is being moved down
         * the tree */
        ipset_node_v4_t *node = NODE_PTR_V4(ipset, find_state->node_idx);

        /* copy the current node's IP to the new parent, masking off
         * the lower bits */
        new_node->prefix = bitpos;
        new_node->ip = node->ip & ~(UINT32_MAX >> bitpos);

        /* put the current node under new_node */
        which_child = WHICH_CHILD_V4(node->ip, new_node->prefix);
        new_node->child[which_child] = find_state->node_idx;

        /* get a handle to the parent, and update the child pointer on
         * the parent */
        if (find_state->parent_idx == IPSET_NO_PARENT) {
            IPSET_ROOT_INDEX_SET(ipset, new_node_idx, 0);
        } else {
            parent = NODE_PTR_V4(ipset, find_state->parent_idx);
            which_child = WHICH_CHILD_V4(node->ip, parent->prefix);
            assert(parent->child[which_child] == find_state->node_idx);
            parent->child[which_child] = new_node_idx;
        }
    }

    return SKIPSET_OK;
}

#if SK_ENABLE_IPV6
static int
ipsetInsertAddressV6(
    skipset_t          *ipset,
    const ipset_ipv6_t *ipv6,
    const uint32_t      prefix,
    const ipset_find_t *find_state)
{
    ipset_find_t find_state_local;
    ipset_node_v6_t *parent = NULL;
    ipset_leaf_v6_t *leaf = NULL;
    ipset_node_v6_t *new_node;
    uint32_t new_node_idx;
    uint32_t new_leaf_idx[IPSET_NUM_CHILDREN];
    uint32_t which_child;
    uint32_t bitpos;
    uint32_t i;
    uint32_t j;
    int rv;

    assert(ipset);
    assert(ipv6);
    assert(0 < prefix && prefix <= 128);
    assert(0 == ipset->is_iptree);
    assert(1 == ipset->is_ipv6);

    if (find_state) {
        rv = find_state->result;
    } else {
        rv = ipsetFindV6(ipset, ipv6, prefix, &find_state_local);
        /* if IP was found, we can return */
        if (SKIPSET_OK == rv) {
            return SKIPSET_OK;
        }
        find_state = &find_state_local;
    }
    ipset->is_dirty = 1;

    if (SKIPSET_ERR_EMPTY == rv) {
        /* tree was previously empty */
        /* create a new node to hold the IP */
        if (ipsetNewEntries(ipset, 1, 2, &new_node_idx, new_leaf_idx)) {
            return SKIPSET_ERR_ALLOC;
        }
        assert(0 == new_node_idx);
        assert(0 == new_leaf_idx[0]);
        assert(1 == new_leaf_idx[1]);
        IPSET_ROOT_INDEX_SET(ipset, new_leaf_idx[1], 1);
        leaf = LEAF_PTR_V6(ipset, new_leaf_idx[IPSET_ROOT_INDEX(ipset)]);
        IPSET_IPV6_COPY(&leaf->ip, ipv6);
        leaf->prefix = prefix;
        return SKIPSET_OK;
    }

    if (SKIPSET_ERR_SUBSET == rv) {
        /* we're adding an IP/PREFIX where part of the IP space
         * already exists in the IPSet.  Modify this node's values to
         * hold the larger block, and remove any nodes below here. */
        if (IPSET_NO_PARENT == find_state->parent_idx) {
            if (IPSET_ROOT_IS_LEAF(ipset)) {
                leaf = LEAF_PTR_V6(ipset, find_state->node_idx);
            } else {
                new_leaf_idx[0] = ipsetReplaceNodeWithLeaf(ipset, NULL, 0);
                leaf = LEAF_PTR_V6(ipset, new_leaf_idx[0]);
            }
            IPSET_IPV6_COPY(&leaf->ip, ipv6);
            leaf->prefix = prefix;
            return SKIPSET_OK;
        }

        /* get a handle to the parent */
        parent = NODE_PTR_V6(ipset, find_state->parent_idx);

        if (NUM_BITS <= prefix - parent->prefix) {
            /* leaf has a single child[] entry on the parent */
            if (find_state->node_is_leaf) {
                /* we can modify the leaf and be done */
                leaf = LEAF_PTR_V6(ipset, find_state->node_idx);
            } else {
                /* need to replace the node with a leaf */
                new_leaf_idx[0]
                    = ipsetReplaceNodeWithLeaf(ipset, (ipset_node_t*)parent,
                                               find_state->parents_child);
                leaf = LEAF_PTR_V6(ipset, new_leaf_idx[0]);
            }
            IPSET_IPV6_COPY(&leaf->ip, ipv6);
            leaf->prefix = prefix;
            return SKIPSET_OK;
        }

        /* this leaf will cover several child[] entries on the parent.
         * destroy any existing children and attempt to find a leaf on
         * this level to use  */
        new_leaf_idx[0] = 0;
        for (i = find_state->parents_child, j = 0;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            if (parent->child[i]
                && !NODEPTR_CHILD_IS_REPEAT(parent, i))
            {
                if (!NODEPTR_CHILD_IS_LEAF(parent, i)) {
                    /* delete this subtree */
                    ipsetDestroySubtree(ipset, parent->child[i], 1);
                } else if (0 == new_leaf_idx[0]) {
                    /* found a leaf to use */
                    new_leaf_idx[0] = parent->child[i];
                } else {
                    /* do not need this leaf */
                    LEAFIDX_FREE(ipset, parent->child[i]);
                }
            }
        }

        if (0 == new_leaf_idx[0]) {
            /* no leaves available, need to allocate one */
            if (ipsetNewEntries(ipset, 0, 1, NULL, new_leaf_idx)) {
                return SKIPSET_ERR_ALLOC;
            }
        }

        leaf = LEAF_PTR_V6(ipset, new_leaf_idx[0]);
        IPSET_IPV6_COPY(&leaf->ip, ipv6);
        leaf->prefix = prefix;

        /* set child[] entries on 'parent' to point at the new leaf */
        /* set additional child[]s to also point at the leaf */
        for (i = find_state->parents_child, j = 0;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            parent->child[i] = new_leaf_idx[0];
        }
        NODEPTR_CHILD_SET_LEAF2(parent, find_state->parents_child, i - 1);
        NODEPTR_CHILD_SET_REPEAT2(parent, 1+find_state->parents_child, i - 1);
        return SKIPSET_OK;
    }

    if (SKIPSET_ERR_MULTILEAF == rv) {
        /* get a handle to the current node */
        parent = NODE_PTR_V6(ipset, find_state->parent_idx);

        if (NUM_BITS <= prefix - parent->prefix) {
            /* need to add a single leaf and have a single child[]
             * entry on 'parent' point to that leaf */
            if (ipsetNewEntries(ipset, 0, 1, NULL, new_leaf_idx)) {
                return SKIPSET_ERR_ALLOC;
            }
            /* get a handle to the newly created leaf and copy the new
             * IP there */
            leaf = LEAF_PTR_V6(ipset, new_leaf_idx[0]);
            IPSET_IPV6_COPY(&leaf->ip, ipv6);
            leaf->prefix = prefix;

            /* set pointer on 'parent' to point at the new leaf */
            parent->child[find_state->parents_child] = new_leaf_idx[0];
            NODEPTR_CHILD_SET_LEAF(parent, find_state->parents_child);
            return SKIPSET_OK;
        }

        /* this leaf will cover several child[] entries on the parent.
         * see if any are occupied. */
        new_leaf_idx[0] = 0;
        for (i = find_state->parents_child + 1, j = 1u;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            if (parent->child[i] && NODEPTR_CHILD_IS_LEAF(parent, i)) {
                new_leaf_idx[0] = parent->child[i];
                break;
            }
        }
        if (new_leaf_idx[0] == 0) {
            /* no leaves available, need to allocate one */
            if (ipsetNewEntries(ipset, 0, 1, NULL, new_leaf_idx)) {
                return SKIPSET_ERR_ALLOC;
            }
        }

        leaf = LEAF_PTR_V6(ipset, new_leaf_idx[0]);
        IPSET_IPV6_COPY(&leaf->ip, ipv6);
        leaf->prefix = prefix;

        /* set child[] on 'parent' to point at the new leaf */
        parent->child[find_state->parents_child] = new_leaf_idx[0];

        /* set additional child[]s to also point at the leaf */
        for (i = find_state->parents_child + 1, j = 1u;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            if (parent->child[i]) {
                if (!NODEPTR_CHILD_IS_LEAF(parent, i)) {
                    /* delete this subtree */
                    ipsetDestroySubtree(ipset, parent->child[i], 1);
                } else if (parent->child[i] != new_leaf_idx[0]) {
                    /* do not need this leaf */
                    LEAFIDX_FREE(ipset, parent->child[i]);
                }
            }
            parent->child[i] = new_leaf_idx[0];
        }
        NODEPTR_CHILD_SET_LEAF2(parent, find_state->parents_child, i - 1);
        if (j > 1) {
            NODEPTR_CHILD_SET_REPEAT2(parent, 1 + find_state->parents_child,
                                      i - 1);
        }
        return SKIPSET_OK;
    }

    /* we must add a new node and a new leaf to the tree.  The leaf
     * holds the IP being inserted.  The node holds the CIDR block
     * containing the leaf and our current node---that is, it becomes
     * a new parent; we must also update the index on the current
     * node's parent to point to the new parent. */

    /* create the two new entries */
    if (ipsetNewEntries(ipset, 1, 1, &new_node_idx, new_leaf_idx)) {
        return SKIPSET_ERR_ALLOC;
    }

    /* get a handle to the newly created node */
    new_node = NODE_PTR_V6(ipset, new_node_idx);

    bitpos = find_state->bitpos & ~(NUM_BITS - 1);

    /* get a handle to the newly created leaf, copy the inserted IP
     * there, and link it to the new_node */
    leaf = LEAF_PTR_V6(ipset, new_leaf_idx[0]);
    IPSET_IPV6_COPY(&leaf->ip, ipv6);
    leaf->prefix = prefix;
    which_child = WHICH_CHILD_V6(ipv6, bitpos);
    if (NUM_BITS <= prefix - bitpos) {
        /* adding a single child[] entry */
        new_node->child[which_child] = new_leaf_idx[0];
        NODEPTR_CHILD_SET_LEAF(new_node, which_child);

    } else {
        /* this leaf will cover several child[] values on the node. */
        for (i = which_child, j = 0;
             j < (1u << (NUM_BITS - (prefix - bitpos)));
             ++i, ++j)
        {
            new_node->child[i] = new_leaf_idx[0];
        }
        NODEPTR_CHILD_SET_LEAF2(new_node, which_child, i - 1);
        NODEPTR_CHILD_SET_REPEAT2(new_node, 1 + which_child, i - 1);
    }

    /* the following code is slightly different depending on whether
     * search stopped on a node or on a leaf */
    if (find_state->node_is_leaf) {
        /* get a handle to the current leaf that is being moved down
         * the tree */
        leaf = LEAF_PTR_V6(ipset, find_state->node_idx);

        /* copy the leaf's IP to the new parent, masking off the lower
         * bits */
        new_node->prefix = bitpos;
        IPSET_IPV6_COPY_AND_MASK(&new_node->ip, &leaf->ip, bitpos);

        /* put the leaf under new_node */
        which_child = WHICH_CHILD_V6(&leaf->ip, new_node->prefix);
        if (NUM_BITS <= leaf->prefix - new_node->prefix) {
            /* leaf occupies a single child[] entry */
            new_node->child[which_child] = find_state->node_idx;
            NODEPTR_CHILD_SET_LEAF(new_node, which_child);

        } else {
            for (i = which_child, j = 0;
                 j < (1u << (NUM_BITS - (leaf->prefix - new_node->prefix)));
                 ++i, ++j)
            {
                new_node->child[i] = find_state->node_idx;
            }
            NODEPTR_CHILD_SET_LEAF2(new_node, which_child, i - 1);
            NODEPTR_CHILD_SET_REPEAT2(new_node, 1 + which_child, i - 1);
        }

        /* get a handle to the parent, and update the child pointer on
         * the parent */
        if (find_state->parent_idx == IPSET_NO_PARENT) {
            IPSET_ROOT_INDEX_SET(ipset, new_node_idx, 0);
        } else {
            parent = NODE_PTR_V6(ipset, find_state->parent_idx);
            which_child = WHICH_CHILD_V6(&leaf->ip, parent->prefix);
            assert(parent->child[which_child] == find_state->node_idx);
            parent->child[which_child] = new_node_idx;
            NODEPTR_CHILD_CLEAR_LEAF(parent, which_child);
        }

    } else {
        /* get a handle to the current node that is being moved down
         * the tree */
        ipset_node_v6_t *node = NODE_PTR_V6(ipset, find_state->node_idx);

        /* copy the current node's IP to the new parent, masking off
         * the lower bits */
        new_node->prefix = bitpos;
        IPSET_IPV6_COPY_AND_MASK(&new_node->ip, &node->ip, bitpos);

        /* put the current node under new_node */
        which_child = WHICH_CHILD_V6(&node->ip, new_node->prefix);
        new_node->child[which_child] = find_state->node_idx;

        /* get a handle to the parent, and update the child pointer on
         * the parent */
        if (find_state->parent_idx == IPSET_NO_PARENT) {
            IPSET_ROOT_INDEX_SET(ipset, new_node_idx, 0);
        } else {
            parent = NODE_PTR_V6(ipset, find_state->parent_idx);
            which_child = WHICH_CHILD_V6(&node->ip, parent->prefix);
            assert(parent->child[which_child] == find_state->node_idx);
            parent->child[which_child] = new_node_idx;
        }
    }

    return SKIPSET_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetInsertRangeIPTree(ipset, ipaddr_start, ipaddr_end);
 *
 *    Insert all IPv4 addresses from ipaddr_start to ipaddr_end
 *    inclusive into the IPset 'ipset'.
 */
static int
ipsetInsertRangeIPTree(
    skipset_t          *ipset,
    const skipaddr_t   *ipaddr_start,
    const skipaddr_t   *ipaddr_end)
{
    skipaddr_t ipaddr4_start;
    skipaddr_t ipaddr4_end;
    skipaddr_t ipaddr4_next;
    uint32_t prefix;
    int rv = SKIPSET_OK;

    assert(ipset);
    assert(1 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);

    /* must be a valid range of IPv4 addresses containing two or more
     * IP addresses */
    assert(skipaddrCompare(ipaddr_start, ipaddr_end) < 0);
    assert(!skipaddrIsV6(ipaddr_start) && !skipaddrIsV6(ipaddr_end));

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

    ipset->is_dirty = 1;

    do {
        prefix = skCIDRComputePrefix(&ipaddr4_start, &ipaddr4_end,
                                     &ipaddr4_next);
        rv = ipsetInsertAddressIPTree(ipset, skipaddrGetV4(&ipaddr4_start),
                                      prefix);
        skipaddrCopy(&ipaddr4_start, &ipaddr4_next);
    } while (SKIPSET_OK == rv && !skipaddrIsZero(&ipaddr4_start));

    return rv;
}


/*
 *  status = ipsetInsertRangeV4(ipset, v4_start, num_ips);
 *
 *    Insert a CIDR block of size 'num_ips' that starts at 'v4_start'.
 *    'num_ips' must be an even multiple of 256 (a /24) and 'v4_start'
 *    must be the start of the /24.  'v4_start' is in native byte
 *    order.
 */
static int
ipsetInsertRangeV4(
    skipset_t          *ipset,
    uint32_t            v4_start,
    uint32_t            num_ips)
{
    static const uint32_t bitmask[] = {
        /*  0- 3 */ 0xffffffff, 0x7fffffff, 0x3fffffff, 0x1fffffff,
        /*  4- 7 */  0xfffffff,  0x7ffffff,  0x3ffffff,  0x1ffffff,
        /*  8-11 */   0xffffff,   0x7fffff,   0x3fffff,   0x1fffff,
        /* 12-15 */    0xfffff,    0x7ffff,    0x3ffff,    0x1ffff,
        /* 16-19 */     0xffff,     0x7fff,     0x3fff,     0x1fff,
        /* 20-23 */      0xfff,      0x7ff,      0x3ff,      0x1ff,
        /* 24-27 */       0xff,       0x7f,       0x3f,       0x1f,
        /* 28-31 */        0xf,        0x7,        0x3,        0x1,
        /* 32    */        0x0
    };

    uint32_t range_start;
    int prefix;
    int rv;

    assert(0 == (v4_start & 0xFF));
    assert(0 == (num_ips & 0xFF));
    assert(num_ips > 0);

    do {
        /* compute the log-base-2 (position of most significant bit)
         * of the number of IPs in the range, and subtract that from
         * 32 to get the widest possible CIDR block */
        prefix = 32 - skIntegerLog2(num_ips);

        /* tighten the range if we need to, in case the IPs don't fall
         * into a single CIDR block (e.g., 10.0.6.0--10.0.9.0) */
        while ((range_start = (v4_start & ~(bitmask[prefix]))) < v4_start) {
            ++prefix;
        }

        /* assert that the CIDR block is within the limits */
        assert(range_start == v4_start);
        assert((range_start | (bitmask[prefix])) <= (num_ips - 1 + v4_start));

        /* insert the CIDR block */
        rv = ipsetInsertAddressV4(ipset, v4_start, prefix, NULL);
        if (rv) {
            break;
        }

        /* compute the start of the next CIDR block and the number of
         * IPs yet to process */
        v4_start += (1u << (32 - prefix));
        num_ips -= (1u << (32 - prefix));

    } while (num_ips);

    return rv;
}


/*
 *  status = ipsetIntersectCallback(ipaddr, prefix, state);
 *
 *    Callback function used by skIPSetIntersect().
 *
 *    This function will return SKIPSET_OK to continue walking over
 *    the IPs, SKIPSET_ERR_ALLOC for an allocation error, or
 *    SKIPSET_ERR_SUBSET when the iterator runs out of IPs to
 *    visit---which will stop the skIPSetWalk().
 */
static int
ipsetIntersectCallback(
    skipaddr_t         *start_addr,
    uint32_t            prefix,
    void               *v_state)
{
    ipset_intersect_t *state = (ipset_intersect_t*)v_state;
    skipaddr_t walk_addr[2];
    int walk_next = 0;

    /* get the start and end IPs for the CIDR block */
    skCIDR2IPRange(start_addr, prefix, &walk_addr[0], &walk_addr[1]);

    for (;;) {
        /* test (iter.final <= walk.final) */
        if (skipaddrCompare(&state->addr[1], &walk_addr[1]) <= 0) {

            /* test (iter.begin >= walk.begin) */
            if (skipaddrCompare(&state->addr[0], &walk_addr[0]) >= 0) {
                /* The iter block is completely contained in---or
                 * identical to---the walk block.  Add the iter
                 * block to the vector.
                 *
                 * iter block:             |---------|
                 * walk block: |---------------------------------|
                 */
                if (skVectorAppendValue(state->vec_add, state->addr)) {
                    return SKIPSET_ERR_ALLOC;
                }

                /* test (iter.final >= walk.begin) */
            } else if (skipaddrCompare(&state->addr[1], &walk_addr[0]) >= 0) {
                /* Add the IPs between walk.begin and iter.final.
                 *
                 * iter block:             |---------|
                 * walk block:                 |---------|
                 */
                skipaddrCopy(&state->addr[0], &walk_addr[0]);
                if (skVectorAppendValue(state->vec_add, state->addr)) {
                    return SKIPSET_ERR_ALLOC;
                }
            }

            /* Else, the iter block is completely below the current walk
             * block; and there is nothing to do.
             *
             * iter block:             |---------|
             * walk block:                         |---------|
             */

            /* If the end IPs are identical, refresh both blocks; else
             * go to next iter block */
            if (skipaddrCompare(&state->addr[1], &walk_addr[1]) == 0) {
                walk_next = 1;
            }

            if (skIPSetIteratorNext(&state->iter, &state->addr[0], &prefix)) {
                /* return some value so we stop walking over the IPs */
                return SKIPSET_ERR_SUBSET;
            }
            skCIDR2IPRange(&state->addr[0], prefix,
                           &state->addr[0], &state->addr[1]);
            if (walk_next) {
                break;
            }

        } else {

            /* test (iter.begin <= walk.begin) */
            if (skipaddrCompare(&state->addr[0], &walk_addr[0]) <= 0) {
                /* The walk is completely contained in the iter block.
                 * Add the walk block to the vector.
                 *
                 * iter block:             |---------|
                 * walk block:               |-----|
                 */
                if (skVectorAppendValue(state->vec_add, walk_addr)) {
                    return SKIPSET_ERR_ALLOC;
                }

                /* test (iter.begin <= walk.final) */
            } else if (skipaddrCompare(&state->addr[0], &walk_addr[1]) <= 0) {
                /* Add the IPs between iter.begin and walk.final.
                 *
                 * iter block:             |---------|
                 * walk block:         |---------|
                 */
                skipaddrCopy(&walk_addr[0], &state->addr[0]);
                if (skVectorAppendValue(state->vec_add, walk_addr)) {
                    return SKIPSET_ERR_ALLOC;
                }
            }

            /* Else, the iter block is completely above the walk block,
             * and there is nothing to do.
             *
             * iter block:             |---------|
             * walk block: |---------|
             */

            /* Go to next walk block */
            break;
        }
    }
    return SKIPSET_OK;
}


/*
 *  ipsetIteratorNextIPTree(iter, ipaddr, prefix);
 *
 *    Helper function for skIPSetIteratorNext() when the IPset is a
 *    SiLK-2 style IPTree.
 */
static int
ipsetIteratorNextIPTree(
    skipset_iterator_t *iter,
    skipaddr_t         *ipaddr,
    uint32_t           *prefix)
{
    skIPTreeCIDRBlock_t cidr;
    int rv = SK_ITERATOR_NO_MORE_ENTRIES;

    switch (iter->v6policy) {
      case SK_IPV6POLICY_ONLY:
        /* since we do not support IPv6 addresses in the IPTree data
         * structure, there is nothing to return */
        break;

      case SK_IPV6POLICY_FORCE:
#if !SK_ENABLE_IPV6
        break;
#else
        if (iter->cidr_blocks) {
            rv = skIPTreeCIDRBlockIteratorNext(&cidr, &iter->iter.v2cidr);
            if (SK_ITERATOR_OK == rv) {
                skipaddrSetV6FromUint32(ipaddr, &cidr.addr);
                *prefix = 96 + cidr.mask;
            }
        } else {
            rv = skIPTreeIteratorNext(&cidr.addr, &iter->iter.v2ip);
            if (SK_ITERATOR_OK == rv) {
                skipaddrSetV6FromUint32(ipaddr, &cidr.addr);
                *prefix = 128;
            }
        }
        break;
#endif  /* #else of #if !SK_ENABLE_IPV6 */

      case SK_IPV6POLICY_MIX:
      case SK_IPV6POLICY_ASV4:
      case SK_IPV6POLICY_IGNORE:
        if (iter->cidr_blocks) {
            rv = skIPTreeCIDRBlockIteratorNext(&cidr, &iter->iter.v2cidr);
            if (SK_ITERATOR_OK == rv) {
                skipaddrSetV4(ipaddr, &cidr.addr);
                *prefix = cidr.mask;
            }
        } else {
            rv = skIPTreeIteratorNext(&cidr.addr, &iter->iter.v2ip);
            if (SK_ITERATOR_OK == rv) {
                skipaddrSetV4(ipaddr, &cidr.addr);
                *prefix = 32;
            }
        }
        break;
    }
    return rv;
}


/*
 *  ipsetIteratorNextRangeV4(iter);
 *  ipsetIteratorNextRangeV6(iter);
 *
 *    Helper functions for skIPSetIteratorNext().
 *
 *    Update the IPset iterator 'iter' to contain the starting and
 *    stopping IPs values for the CIDR block on the current leaf.
 *    Assumes the 'cur' field on the iterator is pointing at a valid
 *    leaf.
 */
static void
ipsetIteratorNextRangeV4(
    skipset_iterator_t *iter)
{
    ipset_leaf_v4_t *leaf = LEAF_PTR_V4(iter->ipset, iter->iter.v3.cur);

    if (32 == leaf->prefix) {
        iter->iter.v3.data[0] = iter->iter.v3.data[2] = leaf->ip;
    } else {
        iter->iter.v3.data[0] = leaf->ip;
        iter->iter.v3.data[2] = (leaf->ip | (UINT32_MAX >> leaf->prefix));
    }
}

#if SK_ENABLE_IPV6
static void
ipsetIteratorNextRangeV6(
    skipset_iterator_t *iter)
{
    ipset_leaf_v6_t *leaf = LEAF_PTR_V6(iter->ipset, iter->iter.v3.cur);

    if (SK_IPV6POLICY_ASV4 == iter->v6policy) {
        /* check stopping condition */
        if (leaf->ip.ip[0] != 0
            || ((UINT64_C(0xffffffff00000000) & leaf->ip.ip[1])
                != UINT64_C(0x0000ffff00000000)))
        {
            iter->iter.v3.cur = iter->ipset->s.v3->leaves.entry_count;
            return;
        }
    }

    if (leaf->prefix > 64) {
        if (leaf->prefix == 128) {
            iter->iter.v3.data[0] = iter->iter.v3.data[2] = leaf->ip.ip[0];
            iter->iter.v3.data[1] = iter->iter.v3.data[3] = leaf->ip.ip[1];
        } else {
            iter->iter.v3.data[0] = iter->iter.v3.data[2] = leaf->ip.ip[0];
            iter->iter.v3.data[1] = leaf->ip.ip[1];
            iter->iter.v3.data[3] = (leaf->ip.ip[1]
                             | (UINT64_MAX >> (leaf->prefix - 64)));
        }
    } else if (leaf->prefix == 64) {
        iter->iter.v3.data[0] = iter->iter.v3.data[2] = leaf->ip.ip[0];
        iter->iter.v3.data[1] = iter->iter.v3.data[3] = UINT64_MAX;
    } else {
        iter->iter.v3.data[0] = leaf->ip.ip[0];
        iter->iter.v3.data[2] = (leaf->ip.ip[0]
                         | (UINT64_MAX >> leaf->prefix));
        iter->iter.v3.data[1] = iter->iter.v3.data[3] = UINT64_MAX;
    }
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  cmp = ipsetLeafCompareV4(leaf_a, leaf_b);
 *  cmp = ipsetLeafCompareV6(leaf_a, leaf_b);
 *
 *    Return -1, 0, 1 depending on whether the IP on leaf_a is less
 *    than, equal to, or greater than the IP on leaf_b.
 *
 *    This function compares the first IP in each leaf; it does not
 *    consider the case where one leaf contains a netblock that is a
 *    subset of the netblock on another leaf.
 */
static int
ipsetLeafCompareV4(
    const void         *va,
    const void         *vb)
{
    const uint32_t a = ((const ipset_leaf_v4_t *)va)->ip;
    const uint32_t b = ((const ipset_leaf_v4_t *)vb)->ip;
    if (a < b) {
        return -1;
    }
    return (a > b);
}

#if SK_ENABLE_IPV6
static int
ipsetLeafCompareV6(
    const void         *va,
    const void         *vb)
{
    const ipset_ipv6_t *a = &((const ipset_leaf_v6_t *)va)->ip;
    const ipset_ipv6_t *b = &((const ipset_leaf_v6_t *)vb)->ip;
    if (a->ip[0] < b->ip[0]) {
        return -1;
    }
    if (a->ip[0] > b->ip[0]) {
        return 1;
    }
    if (a->ip[1] < b->ip[1]) {
        return -1;
    }
    return (a->ip[1] > b->ip[1]);
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetMaskAddLeavesV4(ipset, mask_prefix, leaf);
 *  status = ipsetMaskAddLeavesV6(ipset, mask_prefix, leaf);
 *
 *    Helper function for ipsetMaskVx().
 *
 *    Given 'leaf' that contains a CIDR block, remove 'leaf' from the
 *    'ipset' and add new leaves, where each new leaf is a single IP
 *    address (a /32 or /128), and the step size between the IPs is
 *    1<<(32-mask_prefix) or 1<<(128-mask_prefix).
 *
 *    Return SKIPSET_OK or SKIPSET_ERR_ALLOC.
 */
static int
ipsetMaskAddLeavesV4(
    skipset_t          *ipset,
    const uint32_t      mask_prefix,
    ipset_leaf_v4_t    *leaf)
{
    const uint32_t step = 1u << (32 - mask_prefix);
    uint32_t ipv4;
    uint32_t final;
    int rv;

    /* get the ip address */
    ipv4 = leaf->ip;

    /* compute the final ip address for our stopping condition */
    final = ((ipv4 | (UINT32_MAX >> leaf->prefix))
             & ~(UINT32_MAX >> mask_prefix));

    /* set the prefix so this leaf is a single IP */
    leaf->prefix = 32;

    while (ipv4 < final) {
        ipv4 += step;
        rv = ipsetInsertAddressV4(ipset, ipv4, 32, NULL);
        if (rv) {
            return rv;
        }
    }
    return SKIPSET_OK;
}

#if SK_ENABLE_IPV6
static int
ipsetMaskAddLeavesV6(
    skipset_t          *ipset,
    const uint32_t      mask_prefix,
    ipset_leaf_v6_t    *leaf)
{
    ipset_ipv6_t ipv6;
    ipset_ipv6_t final;
    ipset_ipv6_t step;
    int rv;

    /* get the first address in this block */
    ipv6.ip[0] = leaf->ip.ip[0];
    ipv6.ip[1] = leaf->ip.ip[1];

    /* code in the bodies of the following if/else differ by size of
     * prefix */
    if (mask_prefix <= 64) {
        /* this affects only the most significant 64 bits */
        assert(leaf->prefix < 64);

        /* determine the final address in this block */
        final.ip[0] = ((ipv6.ip[0] | (UINT64_MAX >> leaf->prefix))
                       & ((mask_prefix < 64)
                          ? ~(UINT64_MAX >> mask_prefix)
                          : UINT64_MAX));

        /* get the step size */
        step.ip[0] = UINT64_C(1) << (64 - mask_prefix);

        /* modify starting leaf to be a single IP */
        leaf->prefix = 128;

        while (ipv6.ip[0] < final.ip[0]) {
            ipv6.ip[0] += step.ip[0];
            rv = ipsetInsertAddressV6(ipset, &ipv6, 128, NULL);
            if (rv) { return rv; }
        }

    } else if (leaf->prefix > 64) {
        /* this affects only the least significant 64 bits */
        assert(mask_prefix > 64);

        /* determine the final address in this block */
        final.ip[1] = ((ipv6.ip[1] | (UINT64_MAX >> (leaf->prefix - 64)))
                       & ~(UINT64_MAX >> (mask_prefix - 64)));

        /* get the step size */
        step.ip[1] = UINT64_C(1) << (128 - mask_prefix);

        /* modify starting leaf to be a single IP */
        leaf->prefix = 128;

        while (ipv6.ip[1] < final.ip[1]) {
            ipv6.ip[1] += step.ip[1];
            rv = ipsetInsertAddressV6(ipset, &ipv6, 128, NULL);
            if (rv) { return rv; }
        }

    } else {
        assert(mask_prefix > 64);
        assert(leaf->prefix <= 64);

        /* determine the final address in this block */
        final.ip[0] = ipv6.ip[0] | ((leaf->prefix < 64)
                                    ? (UINT64_MAX >> leaf->prefix)
                                    : 0);
        final.ip[1] = ~(UINT64_MAX >> (mask_prefix - 64));

        /* get the step size */
        step.ip[1] = UINT64_C(1) << (128 - mask_prefix);

        /* modify starting leaf to be a single IP */
        leaf->prefix = 128;

        while (ipv6.ip[0] < final.ip[0]) {
            if (ipv6.ip[1] <= UINT64_MAX - step.ip[1]) {
                ipv6.ip[1] += step.ip[1];
            } else {
                /* handle rollover */
                ++ipv6.ip[0];
                ipv6.ip[1] -= (UINT64_MAX - step.ip[1]) + 1;
            }
            rv = ipsetInsertAddressV6(ipset, &ipv6, 128, NULL);
            if (rv) { return rv; }
        }
        while (ipv6.ip[1] < final.ip[1]) {
            ipv6.ip[1] += step.ip[1];
            rv = ipsetInsertAddressV6(ipset, &ipv6, 128, NULL);
            if (rv) { return rv; }
        }
    }

    return SKIPSET_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetMaskV4(ipset, mask_prefix)
 *  status = ipsetMaskV6(ipset, mask_prefix)
 *
 *    Helper function for skIPSetMask().
 *
 *    For each block of size 'mask_prefix', if any IPs in that block
 *    are set, remove all IPs in the block and replace with with a
 *    single IP.
 */
static int
ipsetMaskV4(
    skipset_t          *ipset,
    const uint32_t      mask_prefix)
{
    ipset_node_v4_t *node;
    ipset_leaf_v4_t *leaf;
    uint32_t child_idx;
    uint32_t node_idx;
    uint32_t new_leaf_idx;
    uint32_t children_per_leaf;
    uint32_t ipv4;
    uint32_t i;
    uint32_t j;
    uint32_t depth;
    uint32_t to_visit[IPSET_MAX_DEPTH_V4];
    int rv = SKIPSET_OK;

    ipset->is_dirty = 1;

    /* handle case where root is a leaf */
    if (IPSET_ROOT_IS_LEAF(ipset)) {
        leaf = LEAF_PTR_V4(ipset, IPSET_ROOT_INDEX(ipset));
        if (leaf->prefix >= mask_prefix) {
            /* leaf spans fewer IPs than specified by 'mask_prefix'.
             * Modify the leaf's IP and prefix and return */
            leaf->ip &= ~(UINT32_MAX >> mask_prefix);
            leaf->prefix = 32;
            return SKIPSET_OK;
        }
        /* else, this leaf's prefix is more course (holds more IPs)
         * than what we want; we must split the leaf into multiple
         * nodes and leaves */
        return ipsetMaskAddLeavesV4(ipset, mask_prefix, leaf);
    }

    /* handle case where root contains a node whose prefix is
     * more-precise than the 'mask_prefix' */
    node = NODE_PTR_V4(ipset, IPSET_ROOT_INDEX(ipset));
    if (node->prefix >= mask_prefix) {
        ipv4 = node->ip;
        new_leaf_idx = ipsetReplaceNodeWithLeaf(ipset, NULL, 0);
        leaf = LEAF_PTR_V4(ipset, new_leaf_idx);
        leaf->ip = ipv4 &  ~(UINT32_MAX >> mask_prefix);
        leaf->prefix = 32;
        return SKIPSET_OK;
    }

    /* visit the nodes in the tree */
    depth = 0;
    to_visit[depth++] = IPSET_ROOT_INDEX(ipset);

    while (depth) {
        node_idx = to_visit[--depth];
        node = NODE_PTR_V4(ipset, node_idx);

        if (mask_prefix - node->prefix > NUM_BITS) {
            /* this node is not at the bottom level of the tree; add
             * child nodes of this node to the 'to_visit' list. modify
             * any leaves to hold a single IP */
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if ((0 == node->child[i])
                    || NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    /* no-op */
                } else if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                    /* node; add to vist list unless its prefix is
                     * larger than the mask, in which case replace
                     * with a leaf */
                    if (NODE_PTR_V4(ipset, node->child[i])->prefix
                        <= mask_prefix)
                    {
                        to_visit[depth++] = node->child[i];
                    } else {
                        ipv4 = NODE_PTR_V4(ipset,node->child[i])->ip;
                        ipsetReplaceNodeWithLeaf(ipset,(ipset_node_t*)node,i);
                        leaf = LEAF_PTR_V4(ipset, node->child[i]);
                        leaf->ip = ipv4 & ~(UINT32_MAX >> mask_prefix);
                        leaf->prefix = 32;
                    }
                } else {
                    leaf = LEAF_PTR_V4(ipset, node->child[i]);
                    if (leaf->prefix >= mask_prefix) {
                        leaf->ip &= ~(UINT32_MAX >> mask_prefix);
                        leaf->prefix = 32;
                    } else {
                        /* this leaf's prefix is more course (holds
                         * more IPs) than what we want; similar to
                         * code above that handle leaf at the root */

                        if (NUM_BITS > leaf->prefix - node->prefix) {
                            /* leaf occupies multiple child[] entries;
                             * clear all but the current one */
                            for (j = 1;
                                 ((j < (1u << (NUM_BITS -
                                               (leaf->prefix - node->prefix))))
                                  && ((i + j) < IPSET_NUM_CHILDREN));
                                 ++j)
                            {
                                node->child[i+j] = 0;
                            }
                            NODEPTR_CHILD_CLEAR_LEAF2(node, i + 1, i + j - 1);
                            NODEPTR_CHILD_CLEAR_REPEAT2(node, i + 1, i + j - 1);
                        }
                        rv = ipsetMaskAddLeavesV4(ipset, mask_prefix, leaf);
                        if (rv) { return rv; }
                        node = NODE_PTR_V4(ipset, node_idx);
                    }
                }
            }

        } else if (mask_prefix - node->prefix == NUM_BITS) {
            /* this node is on the bottom level of the tree, and every
             * occupied child[] will become an independent leaf */
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if (node->child[i]) {
                    if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                        ipsetReplaceNodeWithLeaf(ipset,(ipset_node_t*)node,i);
                    } else if (NODEPTR_CHILD_IS_REPEAT(node, i)) {
                        if (ipsetNewEntries(ipset, 0, 1, NULL, &new_leaf_idx)){
                            return SKIPSET_ERR_ALLOC;
                        }
                        node->child[i] = new_leaf_idx;
                        NODEPTR_CHILD_CLEAR_REPEAT(node, i);
                    }
                    leaf = LEAF_PTR_V4(ipset, node->child[i]);
                    leaf->ip = ((node->ip
                                 | (i << (32 - NUM_BITS - node->prefix)))
                                & ~(UINT32_MAX >> mask_prefix));
                    leaf->prefix = 32;
                }
            }

            /* if node has a single child, we need to replace the node
             * with the child on the node's parent */
            ipsetFixNodeSingleChild(ipset, node_idx, 1);

        } else {
            /* this node is on the bottom level of the tree. multiple
             * child[] entries will become a single leaf */
            children_per_leaf = 1u << (NUM_BITS- (mask_prefix - node->prefix));
            child_idx = UINT32_MAX;
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                for (j = 0; j < children_per_leaf; ++j) {
                    --i;
                    if (node->child[i]) {
                        if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                            if (UINT32_MAX == child_idx) {
                                child_idx = IPSET_NUM_CHILDREN;
                            }
                            ipsetDestroySubtree(ipset, node->child[i], 1);
                            node->child[i] = 0;
                        } else if (NODEPTR_CHILD_IS_REPEAT(node, i)) {
                            if (UINT32_MAX == child_idx) {
                                child_idx = IPSET_NUM_CHILDREN;
                            }
                            node->child[i] = 0;
                        } else if (child_idx >= IPSET_NUM_CHILDREN) {
                            child_idx = i;
                        } else {
                            LEAFIDX_FREE(ipset, node->child[child_idx]);
                            NODEPTR_CHILD_CLEAR_LEAF(node, child_idx);
                            node->child[child_idx] = 0;
                            child_idx = i;
                        }
                    }
                }

                /* the if() is entered when 'i' is looking at the
                 * start of a group of child[] entries and any of the
                 * child[] entries for this group of entries was
                 * occupied. */
                if (child_idx != UINT32_MAX) {
                    if (IPSET_NUM_CHILDREN == child_idx) {
                        if (ipsetNewEntries(ipset, 0, 1, NULL, &new_leaf_idx)){
                            return SKIPSET_ERR_ALLOC;
                        }
                        node->child[i] = new_leaf_idx;
                    } else if (child_idx != i) {
                        node->child[i] = node->child[child_idx];
                        node->child[child_idx] = 0;
                    }
                    leaf = LEAF_PTR_V4(ipset, node->child[i]);
                    leaf->ip = ((node->ip
                                 | (i << (32 - NUM_BITS - node->prefix)))
                                & ~(UINT32_MAX >> mask_prefix));
                    leaf->prefix = 32;
                    NODEPTR_CHILD_SET_LEAF(node, i);
                    NODEPTR_CHILD_CLEAR_LEAF2(node, i+1,
                                              i + children_per_leaf - 1);
                    NODEPTR_CHILD_CLEAR_REPEAT2(node, i,
                                                i + children_per_leaf - 1);

                    /* reset for next group */
                    child_idx = UINT32_MAX;
                }
            }

            /* if node has a single child, we need to replace the node
             * with the child on the node's parent */
            ipsetFixNodeSingleChild(ipset, node_idx, 1);
        }
    }

    return SKIPSET_OK;
}

#if SK_ENABLE_IPV6
static int
ipsetMaskV6(
    skipset_t          *ipset,
    const uint32_t      mask_prefix)
{
    ipset_node_v6_t *node;
    ipset_leaf_v6_t *leaf;
    uint32_t child_idx;
    uint32_t node_idx;
    uint32_t new_leaf_idx;
    uint32_t children_per_leaf;
    ipset_ipv6_t ipv6;
    uint32_t i;
    uint32_t j;
    uint32_t depth;
    uint32_t to_visit[IPSET_MAX_DEPTH_V6];
    int rv = SKIPSET_OK;

    ipset->is_dirty = 1;

    /* handle case where root is a leaf */
    if (IPSET_ROOT_IS_LEAF(ipset)) {
        leaf = LEAF_PTR_V6(ipset, IPSET_ROOT_INDEX(ipset));
        if (leaf->prefix >= mask_prefix) {
            /* leaf spans fewer IPs than specified by 'mask_prefix'.
             * Modify the leaf's IP and prefix and return */
            IPSET_IPV6_APPLY_CIDR(&leaf->ip, mask_prefix);
            leaf->prefix = 128;
            return SKIPSET_OK;
        }
        /* else, this leaf's prefix is more course (holds more IPs)
         * than what we want; we must split the leaf into multiple
         * nodes and leaves */
        return ipsetMaskAddLeavesV6(ipset, mask_prefix, leaf);
    }

    /* handle case where root contains a node whose prefix is
     * more-precise than the 'mask_prefix' */
    node = NODE_PTR_V6(ipset, IPSET_ROOT_INDEX(ipset));
    if (node->prefix >= mask_prefix) {
        IPSET_IPV6_COPY(&ipv6, &node->ip);
        new_leaf_idx = ipsetReplaceNodeWithLeaf(ipset, NULL, 0);
        leaf = LEAF_PTR_V6(ipset, new_leaf_idx);
        IPSET_IPV6_COPY_AND_MASK(&leaf->ip, &ipv6, mask_prefix);
        leaf->prefix = 128;
        return SKIPSET_OK;
    }

    /* visit the nodes in the tree */
    depth = 0;
    to_visit[depth++] = IPSET_ROOT_INDEX(ipset);

    while (depth) {
        node_idx = to_visit[--depth];
        node = NODE_PTR_V6(ipset, node_idx);

        if (mask_prefix - node->prefix > NUM_BITS) {
            /* this node is not at the bottom level of the tree; add
             * child nodes of this node to the 'to_visit' list. modify
             * any leaves to hold a single IP. */
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if ((0 == node->child[i])
                    || NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    /* no-op */
                } else if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                    /* node; add to vist list unless its prefix is
                     * larger than the mask, in which case replace
                     * with a leaf */
                    if (NODE_PTR_V6(ipset, node->child[i])->prefix
                        <= mask_prefix)
                    {
                        to_visit[depth++] = node->child[i];
                    } else {
                        IPSET_IPV6_COPY(&ipv6,
                                        &NODE_PTR_V6(ipset,node->child[i])->ip);
                        ipsetReplaceNodeWithLeaf(ipset,(ipset_node_t*)node,i);
                        leaf = LEAF_PTR_V6(ipset, node->child[i]);
                        IPSET_IPV6_COPY_AND_MASK(&leaf->ip, &ipv6, mask_prefix);
                        leaf->prefix = 128;
                    }
                } else {
                    leaf = LEAF_PTR_V6(ipset, node->child[i]);
                    if (leaf->prefix >= mask_prefix) {
                        IPSET_IPV6_APPLY_CIDR(&leaf->ip, mask_prefix);
                        leaf->prefix = 128;
                    } else {
                        /* this leaf's prefix is more course (holds
                         * more IPs) than what we want; similar to
                         * code above that handle leaf at the root */

                        if (NUM_BITS > leaf->prefix - node->prefix) {
                            /* leaf occupies multiple child[] entries;
                             * clear all but the current one */
                            for (j = 1;
                                 ((j < (1u << (NUM_BITS -
                                               (leaf->prefix - node->prefix))))
                                  && ((i + j) < IPSET_NUM_CHILDREN));
                                 ++j)
                            {
                                node->child[i+j] = 0;
                            }
                            NODEPTR_CHILD_CLEAR_LEAF2(node, i + 1, i + j - 1);
                            NODEPTR_CHILD_CLEAR_REPEAT2(node, i + 1, i + j - 1);
                        }
                        rv = ipsetMaskAddLeavesV6(ipset, mask_prefix, leaf);
                        if (rv) { return rv; }
                        node = NODE_PTR_V6(ipset, node_idx);
                    }
                }
            }

        } else if (mask_prefix - node->prefix == NUM_BITS) {
            /* this node is on the bottom level of the tree, and every
             * occupied child[] will become an independent leaf */
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if (node->child[i]) {
                    if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                        ipsetReplaceNodeWithLeaf(ipset,(ipset_node_t*)node,i);
                    } else if (NODEPTR_CHILD_IS_REPEAT(node, i)) {
                        if (ipsetNewEntries(ipset, 0, 1, NULL, &new_leaf_idx)){
                            return SKIPSET_ERR_ALLOC;
                        }
                        node->child[i] = new_leaf_idx;
                        NODEPTR_CHILD_CLEAR_REPEAT(node, i);
                    }
                    leaf = LEAF_PTR_V6(ipset, node->child[i]);
                    leaf->prefix = 128;
                    if (node->prefix <= 64 - NUM_BITS) {
                        leaf->ip.ip[0] = ((node->ip.ip[0]
                                           | (((uint64_t)i)
                                              << (64-NUM_BITS- node->prefix)))
                                          & ((mask_prefix < 64)
                                             ? ~(UINT64_MAX >> mask_prefix)
                                             : UINT64_MAX));
                        leaf->ip.ip[1] = 0;
                    } else {
                        leaf->ip.ip[0] = node->ip.ip[0];
                        leaf->ip.ip[1] = ((node->ip.ip[1]
                                           | (((uint64_t)i)
                                              << (128-NUM_BITS- node->prefix)))
                                          & ~(UINT64_MAX >> (mask_prefix-64)));
                    }
                }
            }

            /* if node has a single child, we need to replace the node
             * with the child on the node's parent */
            ipsetFixNodeSingleChild(ipset, node_idx, 1);

        } else {
            /* this node is on the bottom level of the tree. multiple
             * child[] entries will become a single leaf */
            children_per_leaf = 1u << (NUM_BITS- (mask_prefix - node->prefix));
            child_idx = UINT32_MAX;
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                for (j = 0; j < children_per_leaf; ++j) {
                    --i;
                    if (node->child[i]) {
                        if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                            if (UINT32_MAX == child_idx) {
                                child_idx = IPSET_NUM_CHILDREN;
                            }
                            ipsetDestroySubtree(ipset, node->child[i], 1);
                            node->child[i] = 0;
                        } else if (NODEPTR_CHILD_IS_REPEAT(node, i)) {
                            if (UINT32_MAX == child_idx) {
                                child_idx = IPSET_NUM_CHILDREN;
                            }
                            node->child[i] = 0;
                        } else if (child_idx >= IPSET_NUM_CHILDREN) {
                            child_idx = i;
                        } else {
                            LEAFIDX_FREE(ipset, node->child[child_idx]);
                            NODEPTR_CHILD_CLEAR_LEAF(node, child_idx);
                            node->child[child_idx] = 0;
                            child_idx = i;
                        }
                    }
                }

                /* the if() is entered when 'i' is looking at the
                 * start of a group of child[] entries and any of the
                 * child[] entries for this group of entries was
                 * occupied. */
                if (child_idx != UINT32_MAX) {
                    if (IPSET_NUM_CHILDREN == child_idx) {
                        if (ipsetNewEntries(ipset, 0, 1, NULL, &new_leaf_idx)){
                            return SKIPSET_ERR_ALLOC;
                        }
                        node->child[i] = new_leaf_idx;
                    } else if (child_idx != i) {
                        node->child[i] = node->child[child_idx];
                        node->child[child_idx] = 0;
                    }
                    leaf = LEAF_PTR_V6(ipset, node->child[i]);
                    leaf->prefix = 128;
                    if (node->prefix <= 64 - NUM_BITS) {
                        leaf->ip.ip[0] = ((node->ip.ip[0]
                                           | (((uint64_t)i)
                                              << (64-NUM_BITS- node->prefix)))
                                          & ((mask_prefix < 64)
                                             ? ~(UINT64_MAX >> mask_prefix)
                                             : UINT64_MAX));
                        leaf->ip.ip[1] = 0;
                    } else {
                        leaf->ip.ip[0] = node->ip.ip[0];
                        leaf->ip.ip[1] = ((node->ip.ip[1]
                                           | (((uint64_t)i)
                                              << (128-NUM_BITS- node->prefix)))
                                          & ~(UINT64_MAX >> (mask_prefix-64)));
                    }
                    NODEPTR_CHILD_SET_LEAF(node, i);
                    NODEPTR_CHILD_CLEAR_LEAF2(node, i+1,
                                              i + children_per_leaf - 1);
                    NODEPTR_CHILD_CLEAR_REPEAT2(node, i,
                                                i + children_per_leaf - 1);

                    child_idx = UINT32_MAX;
                }
            }

            /* if node has a single child, we need to replace the node
             * with the child on the node's parent */
            ipsetFixNodeSingleChild(ipset, node_idx, 1);
        }
    }

    return SKIPSET_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetMaskAndFillIPTree(ipset, mask_prefix)
 *
 *    Helper function for skIPSetMaskAndFill().
 *
 *    For each block of size 'mask_prefix', if any IPs in that block
 *    are set, fill the entire block.
 */
static int
ipsetMaskAndFillIPTree(
    skipset_t          *ipset,
    const uint32_t      mask_prefix)
{
    uint32_t all_ones;
    int step;
    int i, j, k;

    assert(ipset);
    assert(1 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);
    assert(mask_prefix < 32 && mask_prefix > 0);

    ipset->is_dirty = 1;

    if (mask_prefix <= 16) {
        step = 1 << (16 - mask_prefix);
        for (i = 0; i < SKIP_BBLOCK_COUNT; i += step) {
            for (k = (i + step - 1); k >= i; --k) {
                if (ipset->s.v2->nodes[k] != NULL) {
                    break;
                }
            }
            if (k < i) {
                continue;
            }
            for (k = (i + step - 1); k >= i; --k) {
                IPTREE_NODE_ALLOC(ipset, k);
                memset(ipset->s.v2->nodes[k]->addressBlock, 0xff,
                       sizeof(ipset->s.v2->nodes[k]->addressBlock));
            }
        }
        return SKIPSET_OK;
    }

    if (mask_prefix <= 27) {
        step = 1 << (27 - mask_prefix);
        for (i = 0; i < SKIP_BBLOCK_COUNT; ++i) {
            if (ipset->s.v2->nodes[i] == NULL) {
                continue;
            }
            for (j = 0; j < SKIP_BBLOCK_SIZE; j += step) {
                for (k = (j + step - 1); k >= j; --k) {
                    if (ipset->s.v2->nodes[i]->addressBlock[k]) {
                        break;
                    }
                }
                if (k < j) {
                    continue;
                }
                for (k = (j + step - 1); k >= j; --k) {
                    ipset->s.v2->nodes[i]->addressBlock[k] = UINT32_MAX;
                }
            }
        }
        return SKIPSET_OK;
    }

    /* operate on each integer */
    step = 1 << (32 - mask_prefix);
    all_ones = UINT32_MAX >> (32 - step);
    for (i = 0; i < SKIP_BBLOCK_COUNT; ++i) {
        if (ipset->s.v2->nodes[i] == NULL) {
            continue;
        }
        for (j = 0; j < SKIP_BBLOCK_SIZE; ++j) {
            for (k = 0; k < 32; k += step) {
                if (GET_MASKED_BITS(ipset->s.v2->nodes[i]->addressBlock[j],
                                    k, step))
                {
                    SET_MASKED_BITS(ipset->s.v2->nodes[i]->addressBlock[j],
                                    all_ones, k, step);
                }
            }
        }
    }

    return SKIPSET_OK;
}


/*
 *  status = ipsetMaskAndFillV4(ipset, mask_prefix)
 *  status = ipsetMaskAndFillV6(ipset, mask_prefix)
 *
 *    Helper function for skIPSetMaskAndFill().
 *
 *    For each block of size 'mask_prefix', if any IPs in that block
 *    are set, fill the entire block.
 */
static int
ipsetMaskAndFillV4(
    skipset_t          *ipset,
    const uint32_t      mask_prefix)
{
    ipset_node_v4_t *node;
    ipset_leaf_v4_t *leaf;
    uint32_t child_idx;
    uint32_t node_idx;
    uint32_t new_leaf_idx;
    uint32_t children_per_leaf;
    uint32_t ipv4;
    uint32_t i;
    uint32_t j;
    uint32_t depth;
    uint32_t to_visit[IPSET_MAX_DEPTH_V4];

    ipset->is_dirty = 1;

    /* handle case where root is a leaf */
    if (IPSET_ROOT_IS_LEAF(ipset)) {
        leaf = LEAF_PTR_V4(ipset, IPSET_ROOT_INDEX(ipset));
        if (leaf->prefix > mask_prefix) {
            /* leaf spans fewer IPs than specified by 'mask_prefix'.
             * Modify the leaf's IP and prefix and return */
            leaf->ip &= ~(UINT32_MAX >> mask_prefix);
            leaf->prefix = mask_prefix;
            return SKIPSET_OK;
        }
        /* else, this leaf's prefix is more course (holds more IPs)
         * than the mask_prefix, and there is no need to change it. */
        return SKIPSET_OK;
    }

    /* handle case where root contains a node whose prefix is
     * more-precise than the 'mask_prefix' */
    node = NODE_PTR_V4(ipset, IPSET_ROOT_INDEX(ipset));
    if (node->prefix >= mask_prefix) {
        ipv4 = node->ip;
        new_leaf_idx = ipsetReplaceNodeWithLeaf(ipset, NULL, 0);
        leaf = LEAF_PTR_V4(ipset, new_leaf_idx);
        leaf->ip = ipv4 &  ~(UINT32_MAX >> mask_prefix);
        leaf->prefix = mask_prefix;
        return SKIPSET_OK;
    }

    /* visit the nodes in the tree */
    depth = 0;
    to_visit[depth++] = IPSET_ROOT_INDEX(ipset);

    while (depth) {
        node_idx = to_visit[--depth];
        node = NODE_PTR_V4(ipset, node_idx);

        if (mask_prefix - node->prefix > NUM_BITS) {
            /* this node is not at the bottom level of the tree; add
             * child nodes of this node to the 'to_visit' list. modify
             * any leaves to hold a single block */
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if ((0 == node->child[i])
                    || NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    /* no-op */
                } else if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                    /* node; add to vist list unless its prefix is
                     * larger than or equal to the mask, in which case
                     * replace with a leaf */
                    if (NODE_PTR_V4(ipset, node->child[i])->prefix
                        < mask_prefix)
                    {
                        to_visit[depth++] = node->child[i];
                    } else {
                        ipv4 = NODE_PTR_V4(ipset,node->child[i])->ip;
                        ipsetReplaceNodeWithLeaf(ipset,(ipset_node_t*)node,i);
                        leaf = LEAF_PTR_V4(ipset, node->child[i]);
                        leaf->ip = ipv4 & ~(UINT32_MAX >> mask_prefix);
                        leaf->prefix = mask_prefix;
                    }
                } else {
                    leaf = LEAF_PTR_V4(ipset, node->child[i]);
                    if (leaf->prefix > mask_prefix) {
                        leaf->ip &= ~(UINT32_MAX >> mask_prefix);
                        leaf->prefix = mask_prefix;
                    }
                }
            }

        } else if (mask_prefix - node->prefix == NUM_BITS) {
            /* this node is on the bottom level of the tree, and every
             * occupied child[] will become an independent leaf */
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if (node->child[i]
                    && !NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                        ipsetReplaceNodeWithLeaf(ipset,(ipset_node_t*)node,i);
                        leaf = LEAF_PTR_V4(ipset, node->child[i]);
                    } else {
                        leaf = LEAF_PTR_V4(ipset, node->child[i]);
                        if (leaf->prefix <= mask_prefix) {
                            continue;
                        }
                    }
                    leaf->ip = ((node->ip
                                 | (i << (32 - NUM_BITS - node->prefix)))
                                & ~(UINT32_MAX >> mask_prefix));
                    leaf->prefix = mask_prefix;
                }
            }

        } else {
            /* this node is on the bottom level of the tree. multiple
             * child[] entries will become a single leaf */
            children_per_leaf = 1u << (NUM_BITS- (mask_prefix - node->prefix));
            child_idx = UINT32_MAX;
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                for (j = 0; j < children_per_leaf; ++j) {
                    --i;
                    if (node->child[i]) {
                        if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                            if (UINT32_MAX == child_idx) {
                                child_idx = IPSET_NUM_CHILDREN;
                            }
                            ipsetDestroySubtree(ipset, node->child[i], 1);
                        } else if (NODEPTR_CHILD_IS_REPEAT(node, i)) {
                            leaf = LEAF_PTR_V4(ipset, node->child[i]);
                            if (leaf->prefix <= mask_prefix) {
                                assert(0 == j);
                                i -= children_per_leaf - 1;
                                break;
                            }
                            if (UINT32_MAX == child_idx) {
                                child_idx = IPSET_NUM_CHILDREN;
                            }
                            node->child[i] = 0;
                        } else if (child_idx >= IPSET_NUM_CHILDREN) {
                            child_idx = i;
                        } else {
                            LEAFIDX_FREE(ipset, node->child[child_idx]);
                            node->child[child_idx] = 0;
                            child_idx = i;
                        }
                    }
                }

                /* the if() is entered when 'i' is looking at the
                 * start of a group of child[] entries and any of the
                 * child[] entries for this group of entries was
                 * occupied. */
                if (child_idx != UINT32_MAX) {
                    if (IPSET_NUM_CHILDREN == child_idx) {
                        if (ipsetNewEntries(ipset, 0, 1, NULL, &new_leaf_idx)){
                            return SKIPSET_ERR_ALLOC;
                        }
                        node->child[i] = new_leaf_idx;
                    } else if (child_idx != i) {
                        node->child[i] = node->child[child_idx];
                    }
                    leaf = LEAF_PTR_V4(ipset, node->child[i]);
                    leaf->ip = ((node->ip
                                 | (i << (32 - NUM_BITS - node->prefix)))
                                & ~(UINT32_MAX >> mask_prefix));
                    leaf->prefix = mask_prefix;
                    NODEPTR_CHILD_SET_LEAF2(node, i,
                                            i + children_per_leaf - 1);
                    NODEPTR_CHILD_SET_REPEAT2(node, i+1,
                                              i + children_per_leaf - 1);
                    for (j = 1; j < children_per_leaf; ++j) {
                        node->child[i + j] = node->child[i];
                    }
                    /* reset for next group */
                    child_idx = UINT32_MAX;
                }
            }
        }
    }

    return SKIPSET_OK;
}

#if SK_ENABLE_IPV6
static int
ipsetMaskAndFillV6(
    skipset_t          *ipset,
    const uint32_t      mask_prefix)
{
    ipset_node_v6_t *node;
    ipset_leaf_v6_t *leaf;
    uint32_t child_idx;
    uint32_t node_idx;
    uint32_t new_leaf_idx;
    uint32_t children_per_leaf;
    ipset_ipv6_t ipv6;
    uint32_t i;
    uint32_t j;
    uint32_t depth;
    uint32_t to_visit[IPSET_MAX_DEPTH_V6];

    ipset->is_dirty = 1;

    /* handle case where root is a leaf */
    if (IPSET_ROOT_IS_LEAF(ipset)) {
        leaf = LEAF_PTR_V6(ipset, IPSET_ROOT_INDEX(ipset));
        if (leaf->prefix > mask_prefix) {
            /* leaf spans fewer IPs than specified by 'mask_prefix'.
             * Modify the leaf's IP and prefix and return */
            IPSET_IPV6_APPLY_CIDR(&leaf->ip, mask_prefix);
            leaf->prefix = mask_prefix;
            return SKIPSET_OK;
        }
        /* else, this leaf's prefix is more course (holds more IPs)
         * than the mask_prefix, and there is no need to change it */
        return SKIPSET_OK;
    }

    /* handle case where root contains a node whose prefix is
     * more-precise than the 'mask_prefix' */
    node = NODE_PTR_V6(ipset, IPSET_ROOT_INDEX(ipset));
    if (node->prefix >= mask_prefix) {
        IPSET_IPV6_COPY(&ipv6, &node->ip);
        new_leaf_idx = ipsetReplaceNodeWithLeaf(ipset, NULL, 0);
        leaf = LEAF_PTR_V6(ipset, new_leaf_idx);
        IPSET_IPV6_COPY_AND_MASK(&leaf->ip, &ipv6, mask_prefix);
        leaf->prefix = mask_prefix;
        return SKIPSET_OK;
    }

    /* visit the nodes in the tree */
    depth = 0;
    to_visit[depth++] = IPSET_ROOT_INDEX(ipset);

    while (depth) {
        node_idx = to_visit[--depth];
        node = NODE_PTR_V6(ipset, node_idx);

        if (mask_prefix - node->prefix > NUM_BITS) {
            /* this node is not at the bottom level of the tree; add
             * child nodes of this node to the 'to_visit' list. modify
             * any leaves to hold a single block. */
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if ((0 == node->child[i])
                    || NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    /* no-op */
                } else if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                    /* node; add to vist list unless its prefix is
                     * larger than or equal to the mask, in which case
                     * replace with a leaf */
                    if (NODE_PTR_V6(ipset, node->child[i])->prefix
                        < mask_prefix)
                    {
                        to_visit[depth++] = node->child[i];
                    } else {
                        IPSET_IPV6_COPY(&ipv6,
                                        &NODE_PTR_V6(ipset,node->child[i])->ip);
                        ipsetReplaceNodeWithLeaf(ipset,(ipset_node_t*)node,i);
                        leaf = LEAF_PTR_V6(ipset, node->child[i]);
                        IPSET_IPV6_COPY_AND_MASK(&leaf->ip, &ipv6, mask_prefix);
                        leaf->prefix = mask_prefix;
                    }
                } else {
                    leaf = LEAF_PTR_V6(ipset, node->child[i]);
                    if (leaf->prefix > mask_prefix) {
                        IPSET_IPV6_APPLY_CIDR(&leaf->ip, mask_prefix);
                        leaf->prefix = mask_prefix;
                    }
                }
            }

        } else if (mask_prefix - node->prefix == NUM_BITS) {
            /* this node is on the bottom level of the tree, and every
             * occupied child[] will become an independent leaf */
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if (node->child[i]
                    && !NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                        ipsetReplaceNodeWithLeaf(ipset,(ipset_node_t*)node,i);
                        leaf = LEAF_PTR_V6(ipset, node->child[i]);
                    } else {
                        leaf = LEAF_PTR_V6(ipset, node->child[i]);
                        if (leaf->prefix <= mask_prefix) {
                            continue;
                        }
                    }
                    leaf->prefix = mask_prefix;
                    if (node->prefix <= 64 - NUM_BITS) {
                        leaf->ip.ip[0] = ((node->ip.ip[0]
                                           | (((uint64_t)i)
                                              << (64-NUM_BITS- node->prefix)))
                                          & ((mask_prefix < 64)
                                             ? ~(UINT64_MAX >> mask_prefix)
                                             : UINT64_MAX));
                        leaf->ip.ip[1] = 0;
                    } else {
                        leaf->ip.ip[0] = node->ip.ip[0];
                        leaf->ip.ip[1] = ((node->ip.ip[1]
                                           | (((uint64_t)i)
                                              << (128-NUM_BITS- node->prefix)))
                                          & ~(UINT64_MAX >> (mask_prefix-64)));
                    }
                }
            }

        } else {
            /* this node is on the bottom level of the tree. multiple
             * child[] entries will become a single leaf */
            children_per_leaf = 1u << (NUM_BITS- (mask_prefix - node->prefix));
            child_idx = UINT32_MAX;
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                for (j = 0; j < children_per_leaf; ++j) {
                    --i;
                    if (node->child[i]) {
                        if (!NODEPTR_CHILD_IS_LEAF(node, i)) {
                            if (UINT32_MAX == child_idx) {
                                child_idx = IPSET_NUM_CHILDREN;
                            }
                            ipsetDestroySubtree(ipset, node->child[i], 1);
                        } else if (NODEPTR_CHILD_IS_REPEAT(node, i)) {
                            leaf = LEAF_PTR_V6(ipset, node->child[i]);
                            if (leaf->prefix <= mask_prefix) {
                                assert(0 == j);
                                i -= children_per_leaf - 1;
                                break;
                            }
                            if (UINT32_MAX == child_idx) {
                                child_idx = IPSET_NUM_CHILDREN;
                            }
                        } else if (child_idx >= IPSET_NUM_CHILDREN) {
                            child_idx = i;
                        } else {
                            LEAFIDX_FREE(ipset, node->child[child_idx]);
                            NODEPTR_CHILD_CLEAR_LEAF(node, child_idx);
                            child_idx = i;
                        }
                    }
                }

                /* the if() is entered when 'i' is looking at the
                 * start of a group of child[] entries and any of the
                 * child[] entries for this group of entries was
                 * occupied. */
                if (child_idx != UINT32_MAX) {
                    if (IPSET_NUM_CHILDREN == child_idx) {
                        if (ipsetNewEntries(ipset, 0, 1, NULL, &new_leaf_idx)){
                            return SKIPSET_ERR_ALLOC;
                        }
                        node->child[i] = new_leaf_idx;
                    } else if (child_idx != i) {
                        node->child[i] = node->child[child_idx];
                    }
                    leaf = LEAF_PTR_V6(ipset, node->child[i]);
                    leaf->prefix = mask_prefix;
                    if (node->prefix <= 64 - NUM_BITS) {
                        leaf->ip.ip[0] = ((node->ip.ip[0]
                                           | (((uint64_t)i)
                                              << (64-NUM_BITS- node->prefix)))
                                          & ((mask_prefix < 64)
                                             ? ~(UINT64_MAX >> mask_prefix)
                                             : UINT64_MAX));
                        leaf->ip.ip[1] = 0;
                    } else {
                        leaf->ip.ip[0] = node->ip.ip[0];
                        leaf->ip.ip[1] = ((node->ip.ip[1]
                                           | (((uint64_t)i)
                                              << (128-NUM_BITS- node->prefix)))
                                          & ~(UINT64_MAX >> (mask_prefix-64)));
                    }
                    NODEPTR_CHILD_SET_LEAF2(node, i,
                                            i + children_per_leaf - 1);
                    NODEPTR_CHILD_SET_REPEAT2(node, i+1,
                                              i + children_per_leaf - 1);
                    for (j = 1; j < children_per_leaf; ++j) {
                        node->child[i + j] = node->child[i];
                    }

                    child_idx = UINT32_MAX;
                }
            }
        }
    }

    return SKIPSET_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetNewEntries(ipset, num_nodes, num_leaves, node_indexes, leaf_indexes);
 *
 *    Determine where 'num_nodes' new nodes and 'num_leaves' new
 *    leaves can be stored and return the index(es) to that/those
 *    location(s) in the arrays specified by the 'node_indexes' and
 *    'leaf_indexes' parameters.  The new nodes and leaves will be
 *    zeroed before returning.
 *
 *    When a size value is 0, its corresponding array may be NULL.
 *
 *    If a value is 0, its array will not be modified, so any pointers
 *    into that array remain valid.  Specifically, if you have a
 *    handle to a node and request new leaves, the handle to the node
 *    remains valid.
 *
 *    Uses space at the end of the currently allocated arrays first,
 *    then returns entries from the free list.  Finally, calls
 *    ipsetAllocEntries() if no nodes or leaves are available.  Return
 *    0 on success, or SKIPSET_ERR_ALLOC on failure.
 *
 *    If the leaves buffer is reallocated, this function sets the
 *    'realloc_leaves' bit on the 'ipset'.
 */
static int
ipsetNewEntries(
    skipset_t          *ipset,
    uint32_t            num_nodes,
    uint32_t            num_leaves,
    uint32_t           *node_indexes,
    uint32_t           *leaf_indexes)
#ifndef IPSET_DEBUG_MEMORY
{
    /* this is the normal function defintion of ipsetNewEntries() */

    assert(ipset);
    assert(ipset->s.v3->nodes.entry_capacity >= ipset->s.v3->nodes.entry_count);
    assert(ipset->s.v3->leaves.entry_capacity
           >= ipset->s.v3->leaves.entry_count);

    if (num_leaves <= (ipset->s.v3->leaves.entry_capacity
                       - ipset->s.v3->leaves.entry_count))
    {
        /* grab leaves from end of the "fresh" memory */
        while (num_leaves) {
            *leaf_indexes = ipset->s.v3->leaves.entry_count;
            ++ipset->s.v3->leaves.entry_count;
            ++leaf_indexes;
            --num_leaves;
        }
        if (num_nodes) {
            goto ALLOC_NODES;
        }
        return SKIPSET_OK;
    }
    while (num_leaves && (ipset->s.v3->leaves.entry_count
                          < ipset->s.v3->leaves.entry_capacity))
    {
        /* grab as many leaves from end of the "fresh" memory as we can */
        *leaf_indexes = ipset->s.v3->leaves.entry_count;
        ++ipset->s.v3->leaves.entry_count;
        ++leaf_indexes;
        --num_leaves;
    }
    while (num_leaves && ipset->s.v3->leaves.free_list) {
        /* grab as many leaves from the free list as we can */
        ipset_leaf_t *leaf;
        *leaf_indexes = ipset->s.v3->leaves.free_list;
        leaf = LEAF_PTR(ipset, *leaf_indexes);
        ipset->s.v3->leaves.free_list = LEAFPTR_FREE_LIST(leaf);
        memset(leaf, 0, ipset->s.v3->leaves.entry_size);
        ++leaf_indexes;
        --num_leaves;
    }
    if (num_leaves) {
        /* allocate new memory and use it; set the flag to denote that
         * new leaves have been allocated. */
        if (ipset->s.v3->leaves.entry_capacity > 0) {
            ipset->s.v3->realloc_leaves = 1;
        }
        do {
            if (ipsetAllocEntries(&ipset->s.v3->leaves, 0)) {
                return SKIPSET_ERR_ALLOC;
            }
            while (num_leaves && (ipset->s.v3->leaves.entry_count
                                  < ipset->s.v3->leaves.entry_capacity))
            {
                *leaf_indexes = ipset->s.v3->leaves.entry_count;
                ++ipset->s.v3->leaves.entry_count;
                ++leaf_indexes;
                --num_leaves;
            }
        } while (num_leaves);
    }

  ALLOC_NODES:
    if (num_nodes
        <= (ipset->s.v3->nodes.entry_capacity - ipset->s.v3->nodes.entry_count))
    {
        /* grab nodes from end of the "fresh" memory */
        while (num_nodes) {
            *node_indexes = ipset->s.v3->nodes.entry_count;
            ++ipset->s.v3->nodes.entry_count;
            ++node_indexes;
            --num_nodes;
        }
        return SKIPSET_OK;
    }
    while (num_nodes && (ipset->s.v3->nodes.entry_count
                         < ipset->s.v3->nodes.entry_capacity))
    {
        /* grab as many nodes from end of the "fresh" memory as we can */
        *node_indexes = ipset->s.v3->nodes.entry_count;
        ++ipset->s.v3->nodes.entry_count;
        ++node_indexes;
        --num_nodes;
    }
    while (num_nodes && ipset->s.v3->nodes.free_list) {
        /* grab as many nodes from the free list as we can */
        ipset_node_t *node;
        *node_indexes = ipset->s.v3->nodes.free_list;
        node = NODE_PTR(ipset, *node_indexes);
        ipset->s.v3->nodes.free_list = NODEPTR_FREE_LIST(node);
        memset(node, 0, ipset->s.v3->nodes.entry_size);
        ++node_indexes;
        --num_nodes;
    }
    while (num_nodes) {
        /* allocate new memory and use it */
        if (ipsetAllocEntries(&ipset->s.v3->nodes, 0)) {
            return SKIPSET_ERR_ALLOC;
        }
        while (num_nodes && (ipset->s.v3->nodes.entry_count
                             < ipset->s.v3->nodes.entry_capacity))
        {
            *node_indexes = ipset->s.v3->nodes.entry_count;
            ++ipset->s.v3->nodes.entry_count;
            ++node_indexes;
            --num_nodes;
        }
    }

    return SKIPSET_OK;
}
#else  /* #ifndef IPSET_DEBUG_MEMORY */
{
    /*
     *  This is a debugging defintion of ipsetNewEntries().
     *
     *  This definition uses the free list first, so we properly
     *  handle cases where a call to ipsetNewEntries() follows a call
     *  to ipsetDestroySubtree().  (The calling code does not expect
     *  the ipsetNewEntries() call to fail in this case.)
     *
     *  If additional nodes and/or leaves are required, the function
     *  moves the location of the nodes and/or leaves array(s).  The
     *  objective is to find places the in code that are holding a
     *  stale pointer---that is, places that do not properly handle
     *  the case where a realloc may occur.
     *
     *  In addition, the function allocates only as many nodes/leaves
     *  as are requested, so writing beyond the array can be detected
     *  by memory checkers.
     */

    void *new_mem;
    uint32_t new_size;

    assert(ipset);
    assert(ipset->s.v3->nodes.entry_capacity >= ipset->s.v3->nodes.entry_count);
    assert(ipset->s.v3->leaves.entry_capacity
           >= ipset->s.v3->leaves.entry_count);

    if (num_leaves) {
        while (num_leaves && ipset->s.v3->leaves.free_list) {
            ipset_leaf_t *leaf;
            *leaf_indexes = ipset->s.v3->leaves.free_list;
            leaf = LEAF_PTR(ipset, *leaf_indexes);
            ipset->s.v3->leaves.free_list = LEAFPTR_FREE_LIST(leaf);
            memset(leaf, 0, ipset->s.v3->leaves.entry_size);
            ++leaf_indexes;
            --num_leaves;
        }
        if (num_leaves) {
            if (ipset->s.v3->leaves.entry_capacity
                && ((ipset->s.v3->leaves.entry_capacity + num_leaves)
                    % IPSET_INITIAL_ENTRY_COUNT) < num_leaves)
            {
                ipset->s.v3->realloc_leaves = 1;
            }
            new_size = ipset->s.v3->leaves.entry_capacity + num_leaves;
            new_mem = malloc(new_size * ipset->s.v3->leaves.entry_size);
            if (!new_mem) {
                return SKIPSET_ERR_ALLOC;
            }
            if (ipset->s.v3->leaves.buf) {
                memcpy(new_mem, ipset->s.v3->leaves.buf,
                       (ipset->s.v3->leaves.entry_count
                        * ipset->s.v3->leaves.entry_size));
                free(ipset->s.v3->leaves.buf);
            }
            ipset->s.v3->leaves.buf = new_mem;
            ipset->s.v3->leaves.entry_capacity = new_size;
            do {
                *leaf_indexes = ipset->s.v3->leaves.entry_count;
                memset(LEAF_PTR(ipset, *leaf_indexes), 0,
                       ipset->s.v3->leaves.entry_size);
                ++ipset->s.v3->leaves.entry_count;
                ++leaf_indexes;
                --num_leaves;
                assert(ipset->s.v3->leaves.entry_capacity
                       >= ipset->s.v3->leaves.entry_count);
            } while (num_leaves);
        }
    }

    if (num_nodes) {
        while (num_nodes && ipset->s.v3->nodes.free_list) {
            ipset_node_t *node;
            *node_indexes = ipset->s.v3->nodes.free_list;
            node = NODE_PTR(ipset, *node_indexes);
            ipset->s.v3->nodes.free_list = NODEPTR_FREE_LIST(node);
            memset(node, 0, ipset->s.v3->nodes.entry_size);
            ++node_indexes;
            --num_nodes;
        }
        if (num_nodes) {
            new_size = ipset->s.v3->nodes.entry_capacity + num_nodes;
            new_mem = malloc(new_size * ipset->s.v3->nodes.entry_size);
            if (!new_mem) {
                return SKIPSET_ERR_ALLOC;
            }
            if (ipset->s.v3->nodes.buf) {
                memcpy(new_mem, ipset->s.v3->nodes.buf,
                       (ipset->s.v3->nodes.entry_count
                        * ipset->s.v3->nodes.entry_size));
                free(ipset->s.v3->nodes.buf);
            }
            ipset->s.v3->nodes.buf = new_mem;
            ipset->s.v3->nodes.entry_capacity = new_size;
            do {
                *node_indexes = ipset->s.v3->nodes.entry_count;
                memset(NODE_PTR(ipset, *node_indexes), 0,
                       ipset->s.v3->nodes.entry_size);
                ++ipset->s.v3->nodes.entry_count;
                ++node_indexes;
                --num_nodes;
                assert(ipset->s.v3->nodes.entry_capacity
                       >= ipset->s.v3->nodes.entry_count);
            } while (num_nodes);
        }
    }

    return SKIPSET_OK;
}
#endif  /* #else of #ifndef IPSET_DEBUG_MEMORY */


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
        rv = skStringParseUint32(&tmp32, opt_arg, IPSET_REC_VERSION_DEFAULT,
                                 IPSET_REC_VERSION_MAX);
        if (rv) {
            goto PARSE_ERROR;
        }
        if (1 == tmp32) {
            skAppPrintErr("Invalid %s '%s': Illegal version number",
                          ipset_options[opt_index].name, opt_arg);
            return -1;
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
 *  status = ipsetPrintCallback(ip, prefix, &print_state);
 *
 *    Helper function for skIPSetPrint().
 *
 *    Print a textual representation of the 'ip' and 'prefix'.  The
 *    stream to print to and the format to use are present in the
 *    'print_state' argument.
 */
static int
ipsetPrintCallback(
    skipaddr_t         *ip,
    uint32_t            prefix,
    void               *v_print_state)
{
    ipset_print_t *print_state = (ipset_print_t*)v_print_state;
    char ipbuf[SK_NUM2DOT_STRLEN+1];
    int rv = SKIPSET_OK;

    /* print ip and its prefix, then return */
    skipaddrString(ipbuf, ip, print_state->ip_format);
    if (skipaddrIsV6(ip) ? (prefix == 128) : (prefix == 32)) {
        if (skStreamPrint(print_state->stream, "%s\n", ipbuf)) {
            rv = SKIPSET_ERR_FILEIO;
        }
    } else {
        if (skStreamPrint(print_state->stream, ("%s/%" PRIu32 "\n"),
                          ipbuf, prefix))
        {
            rv = SKIPSET_ERR_FILEIO;
        }
    }

    return rv;
}


/*
 *  status = ipsetReadCidrbmapIntoIPTree(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetRead() via ipsetReadCidrbmap().
 *
 *    Read an IPset containing only IPv4 addresses in the
 *    IPSET_REC_VERSION_CIDR_BMAP format from 'stream' and create a
 *    SiLK-2 IPTree data structure.
 *
 *    See '#define IPSET_REC_VERSION_CIDR_BMAP' for description of the
 *    file format.
 */
static int
ipsetReadCidrbmapIntoIPTree(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    skipset_t *ipset = NULL;
    sk_header_entry_t *hentry;
    uint8_t read_buf[sizeof(uint32_t) + sizeof(uint8_t)];
    uint32_t bmap[IPTREE_WORDS_PER_SLASH24];
    int swap_flag;
    uint32_t slash24;
    uint32_t slash16;
    ssize_t b;
    int i;
    int rv;

    /* sanity check input */
    assert(ipset_out);
    assert(stream);
    assert(hdr);
    if (skStreamCheckSilkHeader(stream, FT_IPSET, IPSET_REC_VERSION_CIDR_BMAP,
                                IPSET_REC_VERSION_CIDR_BMAP, NULL))
    {
        skAbort();
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        skAbort();
    }
    hentry = skHeaderGetFirstMatch(hdr, SK_HENTRY_IPSET_ID);
    if (NULL == hentry) {
        skAbort();
    }
    if (sizeof(uint32_t) != skHentryIPSetGetLeafSize(hentry)) {
        skAbort();
    }

    swap_flag = !skHeaderIsNativeByteOrder(hdr);

    rv = ipsetCreate(&ipset, 0, 0);
    if (rv != SKIPSET_OK) {
        goto END;
    }

    while ((b = skStreamRead(stream, read_buf, sizeof(read_buf)))
           == sizeof(read_buf))
    {
        if (swap_flag) {
            slash24 = BSWAP32(*(uint32_t*)read_buf);
        } else {
            slash24 = *(uint32_t*)read_buf;
        }
        /* handle the prefix bit */
        if (read_buf[sizeof(uint32_t)] <= 32) {
            /* IPv4 and one byte that is CIDR block */
            rv = ipsetInsertAddressIPTree(ipset, slash24,
                                          read_buf[sizeof(uint32_t)]);
            if (rv) {
                goto END;
            }
        } else if (read_buf[sizeof(uint32_t)] != SET_CIDR_BMAP_256) {
            skAppPrintErr("Unexpected value for prefix %u",
                          read_buf[sizeof(uint32_t)]);
            skAbort();
        } else {
            /* IPv4 and 256-element bitmap */
            if ((b = skStreamRead(stream, bmap, sizeof(bmap)))
                != sizeof(bmap))
            {
                rv = SKIPSET_ERR_FILEIO;
                goto END;
            }
            if (swap_flag) {
                for (i = 0; i < IPTREE_WORDS_PER_SLASH24; ++i) {
                    bmap[i] = BSWAP32(bmap[i]);
                }
            }
            /* Put the first two octets of the base IP into slash16,
             * and allocate the space for the /16 if we need to */
            slash16 = (slash24 >> 16);
            if (NULL == ipset->s.v2->nodes[slash16]) {
                ipset->s.v2->nodes[slash16]
                    = (skIPNode_t*)calloc(1, sizeof(skIPNode_t));
                if (NULL == ipset->s.v2->nodes[slash16]) {
                    rv = SKIPSET_ERR_ALLOC;
                    goto END;
                }
            }
            /* Locate where this /24 occurs inside the larger /16,
             * then copy it into place.  Following is equivalent to
             * (((slash24 >> 8) & 0xFF) * 8); */
            slash24 = ((slash24 & 0x0000FF00) >> 5);
            memcpy((ipset->s.v2->nodes[slash16]->addressBlock + slash24),
                   bmap, sizeof(bmap));
        }
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
        skIPSetDestroy(&ipset);
    }
    return rv;
}


/*
 *  status = ipsetReadCidrbmapIntoRadixV4(&ipset, stream, hdr, is_ipv6);
 *  status = ipsetReadCidrbmapIntoRadixV6(&ipset, stream, hdr, is_ipv6);
 *
 *    Helper function for skIPSetRead() via ipsetReadCidrbmap().
 *
 *    Read an IPset in the IPSET_REC_VERSION_CIDR_BMAP format from
 *    'stream' and create a radix-tree IPset data structure.
 *
 *    See '#define IPSET_REC_VERSION_CIDR_BMAP' for description of the
 *    file format.
 */
static int
ipsetReadCidrbmapIntoRadixV4(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    skipset_t *ipset = NULL;
    sk_header_entry_t *hentry;
    uint8_t read_buf[sizeof(uint32_t) + sizeof(uint8_t)];
    uint32_t bmap[IPTREE_WORDS_PER_SLASH24];
    int swap_flag;
    uint32_t slash24;
    uint32_t ipv4;
    ssize_t b;
    int i;
    int rv;

    /* sanity check input */
    assert(ipset_out);
    assert(stream);
    assert(hdr);
    if (skStreamCheckSilkHeader(stream, FT_IPSET, IPSET_REC_VERSION_CIDR_BMAP,
                                IPSET_REC_VERSION_CIDR_BMAP, NULL))
    {
        skAbort();
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        skAbort();
    }
    hentry = skHeaderGetFirstMatch(hdr, SK_HENTRY_IPSET_ID);
    if (NULL == hentry) {
        skAbort();
    }
    if (sizeof(uint32_t) != skHentryIPSetGetLeafSize(hentry)) {
        skAbort();
    }

    swap_flag = !skHeaderIsNativeByteOrder(hdr);

    rv = ipsetCreate(&ipset, 0, 1);
    if (rv != SKIPSET_OK) {
        goto END;
    }

    while ((b = skStreamRead(stream, read_buf, sizeof(read_buf)))
           == sizeof(read_buf))
    {
        if (swap_flag) {
            slash24 = BSWAP32(*(uint32_t*)read_buf);
        } else {
            slash24 = *(uint32_t*)read_buf;
        }
        /* handle the prefix bit */
        if (read_buf[sizeof(uint32_t)] <= 32) {
            /* IPv4 and one byte that is CIDR block */
            rv = ipsetInsertAddressV4(ipset, slash24,
                                      read_buf[sizeof(uint32_t)], NULL);
            if (rv) { goto END; }
        } else if (read_buf[sizeof(uint32_t)] != SET_CIDR_BMAP_256) {
            skAppPrintErr("Unexpected value for prefix %u",
                          read_buf[sizeof(uint32_t)]);
            skAbort();
        } else {
            /* IPv4 base address and 256-bit bitmap */
            if ((b = skStreamRead(stream, bmap, sizeof(bmap)))
                != sizeof(bmap))
            {
                rv = SKIPSET_ERR_FILEIO;
                goto END;
            }
            /* loop over the uint32_t in this block */
            i = 0;
            while (i < 8) {
                switch (i) {
                  case 0: case 4:
                    if (memcmp(&bmap[i], bmap256_zero, sizeof(uint32_t)*4)==0){
                        i += 4;
                        continue;
                    }
                    if (memcmp(&bmap[i], bmap256_full, sizeof(uint32_t)*4)==0){
                        ipv4 = slash24 | (i << 5);
                        rv = ipsetInsertAddressV4(ipset, ipv4, 25, NULL);
                        if (rv) { goto END; }
                        i += 4;
                        continue;
                    }
                    /* FALLTHROUGH */
                  case 2: case 6:
                    if (memcmp(&bmap[i], bmap256_zero, sizeof(uint32_t)*2)==0){
                        i += 2;
                        continue;
                    }
                    if (memcmp(&bmap[i], bmap256_full, sizeof(uint32_t)*2)==0){
                        ipv4 = slash24 | (i << 5);
                        rv = ipsetInsertAddressV4(ipset, ipv4, 26, NULL);
                        if (rv) { goto END; }
                        i += 2;
                        continue;
                    }
                    /* FALLTHROUGH */
                  default:
                    if (0 == bmap[i]) {
                        ++i;
                        continue;
                    }
                    if (UINT32_MAX == bmap[i]) {
                        ipv4 = slash24 | (i << 5);
                        rv = ipsetInsertAddressV4(ipset, ipv4, 27, NULL);
                        if (rv) { goto END; }
                        ++i;
                        continue;
                    }
                    break;
                }

                ipv4 = slash24 | (i << 5);

                if (swap_flag) {
                    bmap[i] = BSWAP32(bmap[i]);
                }
                while (bmap[i]) {
                    /* find position of least significant bit */
                    if (!(bmap[i] & 0x1)) {
                        if ((bmap[i] & 0xFFFF) == 0) {
                            bmap[i] >>= 16;
                            ipv4 += 16;
                        }
                        if ((bmap[i] & 0xFF) == 0) {
                            bmap[i] >>= 8;
                            ipv4 += 8;
                        }
                        if ((bmap[i] & 0xF) == 0) {
                            bmap[i] >>= 4;
                            ipv4 += 4;
                        }
                        if ((bmap[i] & 0x3) == 0) {
                            bmap[i] >>= 2;
                            ipv4 += 2;
                        }
                        if ((bmap[i] & 0x1) == 0) {
                            bmap[i] >>= 1;
                            ipv4 += 1;
                        }
                    }
                    /* find number of consecutive high bits that
                     * map into a CIDR block */
                    switch (ipv4 & 0x1F) {
                      case 0: case 16:
                        if ((bmap[i] & 0xFFFF) == 0xFFFF) {
                            rv = ipsetInsertAddressV4(ipset, ipv4, 28, NULL);
                            if (rv) { goto END; }
                            bmap[i] >>= 16;
                            ipv4 += 16;
                            continue;
                        }
                        /* FALLTHROUGH */
                      case 8: case 24:
                        if ((bmap[i] & 0xFF) == 0xFF) {
                            rv = ipsetInsertAddressV4(ipset, ipv4, 29, NULL);
                            if (rv) { goto END; }
                            bmap[i] >>= 8;
                            ipv4 += 8;
                            continue;
                        }
                        /* FALLTHROUGH */
                      case 4: case 12: case 20: case 28:
                        if ((bmap[i] & 0xF) == 0xF) {
                            rv = ipsetInsertAddressV4(ipset, ipv4, 30, NULL);
                            if (rv) { goto END; }
                            bmap[i] >>= 4;
                            ipv4 += 4;
                            continue;
                        }
                        /* FALLTHROUGH */
                      case  2: case  6: case 10: case 14:
                      case 18: case 22: case 26: case 30:
                        if ((bmap[i] & 0x3) == 0x3) {
                            rv = ipsetInsertAddressV4(ipset, ipv4, 31, NULL);
                            if (rv) { goto END; }
                            bmap[i] >>= 2;
                            ipv4 += 2;
                            continue;
                        }
                        /* FALLTHROUGH */
                      default:
                        rv = ipsetInsertAddressV4(ipset, ipv4, 32, NULL);
                        if (rv) { goto END; }
                        bmap[i] >>= 1;
                        ++ipv4;
                        break;
                    }
                }
            }
        }
    }
    if (b != 0) {
        /* read error */
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    skIPSetClean(ipset);

    /* Success */
    *ipset_out = ipset;
    rv = SKIPSET_OK;

  END:
    if (rv != SKIPSET_OK) {
        skIPSetDestroy(&ipset);
    }
    return rv;
}

#if SK_ENABLE_IPV6
static int
ipsetReadCidrbmapIntoRadixV6(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    skipset_t *ipset = NULL;
    sk_header_entry_t *hentry;
    uint8_t read_buf[IPSET_LEN_V6 + sizeof(uint8_t)];
    uint32_t bmap[IPTREE_WORDS_PER_SLASH24];
    int swap_flag;
    ipset_ipv6_t slash120;
    ipset_ipv6_t ipv6;
    ssize_t b;
    int i;
    int rv;

    /* sanity check input */
    assert(ipset_out);
    assert(stream);
    assert(hdr);
    if (skStreamCheckSilkHeader(stream, FT_IPSET, IPSET_REC_VERSION_CIDR_BMAP,
                                IPSET_REC_VERSION_CIDR_BMAP, NULL))
    {
        skAbort();
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        skAbort();
    }
    hentry = skHeaderGetFirstMatch(hdr, SK_HENTRY_IPSET_ID);
    if (NULL == hentry) {
        skAbort();
    }
    if (IPSET_LEN_V6 != skHentryIPSetGetLeafSize(hentry)) {
        skAbort();
    }

    swap_flag = !skHeaderIsNativeByteOrder(hdr);

    rv = ipsetCreate(&ipset, 1, 1);
    if (rv != SKIPSET_OK) {
        goto END;
    }

    while ((b = skStreamRead(stream, read_buf, sizeof(read_buf)))
           == sizeof(read_buf))
    {
        IPSET_IPV6_FROM_ARRAY(&slash120, read_buf);
        /* handle the prefix bit */
        if (read_buf[IPSET_LEN_V6] <= 128) {
            /* IPv6 and one byte CIDR prefix */
            rv = ipsetInsertAddressV6(ipset, &slash120,
                                      read_buf[IPSET_LEN_V6], NULL);
            if (rv) { goto END; }
        } else if (read_buf[IPSET_LEN_V6] != SET_CIDR_BMAP_256) {
            skAppPrintErr("Unexpected value for prefix %u",
                          read_buf[sizeof(uint32_t)]);
            skAbort();
        } else {
            /* IPv6 base address and 256-bit bitmap */
            if ((b = skStreamRead(stream, bmap, sizeof(bmap)))
                != sizeof(bmap))
            {
                rv = SKIPSET_ERR_FILEIO;
                goto END;
            }

            ipv6.ip[0] = slash120.ip[0];

            /* loop over the uint32_t in this block */
            i = 0;
            while (i < 8) {
                switch (i) {
                  case 0: case 4:
                    if (memcmp(&bmap[i], bmap256_zero, sizeof(uint32_t)*4)==0){
                        i += 4;
                        continue;
                    }
                    if (memcmp(&bmap[i], bmap256_full, sizeof(uint32_t)*4)==0){
                        ipv6.ip[1] = slash120.ip[1] | (i << 5);
                        rv = ipsetInsertAddressV6(ipset, &ipv6, 121, NULL);
                        if (rv) { goto END; }
                        i += 4;
                        continue;
                    }
                    /* FALLTHROUGH */
                  case 2: case 6:
                    if (memcmp(&bmap[i], bmap256_zero, sizeof(uint32_t)*2)==0){
                        i += 2;
                        continue;
                    }
                    if (memcmp(&bmap[i], bmap256_full, sizeof(uint32_t)*2)==0){
                        ipv6.ip[1] = slash120.ip[1] | (i << 5);
                        rv = ipsetInsertAddressV6(ipset, &ipv6, 122, NULL);
                        if (rv) { goto END; }
                        i += 2;
                        continue;
                    }
                    /* FALLTHROUGH */
                  default:
                    if (0 == bmap[i]) {
                        ++i;
                        continue;
                    }
                    if (UINT32_MAX == bmap[i]) {
                        ipv6.ip[1] = slash120.ip[1] | (i << 5);
                        rv = ipsetInsertAddressV6(ipset, &ipv6, 123, NULL);
                        if (rv) { goto END; }
                        ++i;
                        continue;
                    }
                    break;
                }

                ipv6.ip[1] = slash120.ip[1] | (i << 5);

                if (swap_flag) {
                    bmap[i] = BSWAP32(bmap[i]);
                }

                while (bmap[i]) {
                    /* find position of least significant bit */
                    if (!(bmap[i] & 0x1)) {
                        if ((bmap[i] & 0xFFFF) == 0) {
                            bmap[i] >>= 16;
                            ipv6.ip[1] += 16;
                        }
                        if ((bmap[i] & 0xFF) == 0) {
                            bmap[i] >>= 8;
                            ipv6.ip[1] += 8;
                        }
                        if ((bmap[i] & 0xF) == 0) {
                            bmap[i] >>= 4;
                            ipv6.ip[1] += 4;
                        }
                        if ((bmap[i] & 0x3) == 0) {
                            bmap[i] >>= 2;
                            ipv6.ip[1] += 2;
                        }
                        if ((bmap[i] & 0x1) == 0) {
                            bmap[i] >>= 1;
                            ipv6.ip[1] += 1;
                        }
                    }
                    /* find number of consecutive high bits that
                     * map into a CIDR block */
                    switch (ipv6.ip[1] & 0x1F) {
                      case 0: case 16:
                        if ((bmap[i] & 0xFFFF) == 0xFFFF) {
                            rv = ipsetInsertAddressV6(ipset, &ipv6, 124, NULL);
                            if (rv) { goto END; }
                            bmap[i] >>= 16;
                            ipv6.ip[1] += 16;
                            continue;
                        }
                        /* FALLTHROUGH */
                      case 8: case 24:
                        if ((bmap[i] & 0xFF) == 0xFF) {
                            rv = ipsetInsertAddressV6(ipset, &ipv6, 125, NULL);
                            if (rv) { goto END; }
                            bmap[i] >>= 8;
                            ipv6.ip[1] += 8;
                            continue;
                        }
                        /* FALLTHROUGH */
                      case 4: case 12: case 20: case 28:
                        if ((bmap[i] & 0xF) == 0xF) {
                            rv = ipsetInsertAddressV6(ipset, &ipv6, 126, NULL);
                            if (rv) { goto END; }
                            bmap[i] >>= 4;
                            ipv6.ip[1] += 4;
                            continue;
                        }
                        /* FALLTHROUGH */
                      case  2: case  6: case 10: case 14:
                      case 18: case 22: case 26: case 30:
                        if ((bmap[i] & 0x3) == 0x3) {
                            rv = ipsetInsertAddressV6(ipset, &ipv6, 127, NULL);
                            if (rv) { goto END; }
                            bmap[i] >>= 2;
                            ipv6.ip[1] += 2;
                            continue;
                        }
                        /* FALLTHROUGH */
                      default:
                        rv = ipsetInsertAddressV6(ipset, &ipv6, 128, NULL);
                        if (rv) { goto END; }
                        bmap[i] >>= 1;
                        ++ipv6.ip[1];
                        break;
                    }
                }
            }
        }
    }
    if (b != 0) {
        /* read error */
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    skIPSetClean(ipset);

    /* Success */
    *ipset_out = ipset;
    rv = SKIPSET_OK;

  END:
    if (rv != SKIPSET_OK) {
        skIPSetDestroy(&ipset);
    }
    return rv;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetReadCidrbmap(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetRead().
 *
 *    Read an IPset in the IPSET_REC_VERSION_CIDR_BMAP format from
 *    'stream'.
 *
 *    This function reads the header then calls another helper
 *    function depending on the data structure being used.
 */
static int
ipsetReadCidrbmap(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    sk_header_entry_t *hentry;
    int is_ipv6;

    /* sanity check input */
    assert(ipset_out);
    assert(stream);
    assert(hdr);
    if (skStreamCheckSilkHeader(stream, FT_IPSET, IPSET_REC_VERSION_CIDR_BMAP,
                                IPSET_REC_VERSION_CIDR_BMAP, NULL))
    {
        skAbort();
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        skAbort();
    }

    /* read and verify the header */
    hentry = skHeaderGetFirstMatch(hdr, SK_HENTRY_IPSET_ID);
    if (NULL == hentry) {
        return SKIPSET_ERR_FILEHEADER;
    }
    /* most sizes are zero */
    if (0 != skHentryIPSetGetChildPerNode(hentry)
        || 0 != skHentryIPSetGetRootIndex(hentry)
        || 0 != skHentryIPSetGetNodeCount(hentry)
        || 0 != skHentryIPSetGetNodeSize(hentry)
        || 0 != skHentryIPSetGetLeafCount(hentry))
    {
        return SKIPSET_ERR_FILEHEADER;
    }
    if (sizeof(uint32_t) == skHentryIPSetGetLeafSize(hentry)) {
        is_ipv6 = 0;
    } else if (IPSET_LEN_V6 == skHentryIPSetGetLeafSize(hentry)) {
        is_ipv6 = 1;
    } else {
        /* Unrecognized record size */
        return SKIPSET_ERR_FILEHEADER;
    }

    if (is_ipv6) {
#if  !SK_ENABLE_IPV6
        /* IPv6 IPSet not supported by this build of SiLK */
        return SKIPSET_ERR_IPV6;
#else
        return ipsetReadCidrbmapIntoRadixV6(ipset_out, stream, hdr);
#endif
    }
    if (IPSET_USE_IPTREE) {
        /* Read IPv4-only file into the IPTree format */
        return ipsetReadCidrbmapIntoIPTree(ipset_out, stream, hdr);
    }
    return ipsetReadCidrbmapIntoRadixV4(ipset_out, stream, hdr);
}


/*
 *  status = ipsetReadLegacyIntoIPTree(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetRead() via ipsetReadLegacy().
 *
 *    Read an IPset in the IPSET_REC_VERSION_LEGACY format from
 *    'stream' and create a SiLK-2 IPTree data structure.
 *
 *    See '#define IPSET_REC_VERSION_LEGACY' for description of the
 *    file format.
 */
static int
ipsetReadLegacyIntoIPTree(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    skipset_t *ipset = NULL;
    int swap_flag;
    uint32_t block24[1 + IPTREE_WORDS_PER_SLASH24];
    uint32_t slash24;
    uint32_t slash16;
    ssize_t b;
    int i;
    int rv;

    /* sanity check input */
    assert(ipset_out);
    assert(stream);
    assert(hdr);
    if (skStreamCheckSilkHeader(stream, FT_IPSET, 0,
                                IPSET_REC_VERSION_LEGACY, NULL))
    {
        skAbort();
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        skAbort();
    }

    swap_flag = !skHeaderIsNativeByteOrder(hdr);

    rv = ipsetCreate(&ipset, 0, 0);
    if (rv != SKIPSET_OK) {
        goto END;
    }

    while ((b = skStreamRead(stream, block24, sizeof(block24)))
           == sizeof(block24))
    {
        if (swap_flag) {
            for (i = 0; i < 1+IPTREE_WORDS_PER_SLASH24; ++i) {
                block24[i] = BSWAP32(block24[i]);
            }
        }

        /* Put the first two octets of the base IP into slash16, and
         * allocate the space for the /16 if we need to */
        slash16 = (block24[0] >> 16);
        if (NULL == ipset->s.v2->nodes[slash16]) {
            ipset->s.v2->nodes[slash16]
                = (skIPNode_t*)calloc(1, sizeof(skIPNode_t));
            if (NULL == ipset->s.v2->nodes[slash16]) {
                rv = SKIPSET_ERR_ALLOC;
                goto END;
            }
        }

        /* Locate where this /24 occurs inside the larger /16, then
         * copy it into place.  Following is equivalent to
         * (((block24[0] >> 8) & 0xFF) * 8); */
        slash24 = ((block24[0] & 0x0000FF00) >> 5);
        memcpy((ipset->s.v2->nodes[slash16]->addressBlock + slash24),
               block24 + 1, IPTREE_WORDS_PER_SLASH24 * sizeof(uint32_t));
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
        skIPSetDestroy(&ipset);
    }
    return rv;
}


/*
 *  status = ipsetReadLegacyIntoRadix(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetRead() via ipsetReadLegacy().
 *
 *    Read an IPset in the IPSET_REC_VERSION_LEGACY format from
 *    'stream' and create a radix-tree IPset data structure.
 *
 *    See '#define IPSET_REC_VERSION_LEGACY' for description of the
 *    file format.
 */
static int
ipsetReadLegacyIntoRadix(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    skipset_t *ipset = NULL;
    int swap_flag;
    uint32_t contig_start = 0;
    uint32_t contig_length = 0;
    uint32_t block24[1+IPTREE_WORDS_PER_SLASH24];
    uint32_t slash24;
    uint32_t ipv4;
    ssize_t b;
    int i;
    int rv;

    /* sanity check input */
    assert(ipset_out);
    assert(stream);
    assert(hdr);
    if (skStreamCheckSilkHeader(stream, FT_IPSET, 0,
                                IPSET_REC_VERSION_LEGACY, NULL))
    {
        skAbort();
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        skAbort();
    }

    swap_flag = !skHeaderIsNativeByteOrder(hdr);

    rv = ipsetCreate(&ipset, 0, 1);
    if (rv != SKIPSET_OK) {
        goto END;
    }

    while ((b = skStreamRead(stream, block24, sizeof(block24)))
           == sizeof(block24))
    {
        /* the /24 value is the first value in the array */
        slash24 = (((swap_flag) ? BSWAP32(block24[0]) : block24[0])
                   & 0xFFFFFF00);

        /* handle case where the entire /24 is set */
        if (0 == memcmp(&block24[1], bmap256_full, sizeof(bmap256_full))) {
            if (contig_length) {
                /* we have a previous /24 */
                if ((contig_start + contig_length) == slash24) {
                    /* add this block to the current block */
                    contig_length += 256;
                    continue;
                }

                /* there is a hole between these CIDR blocks. insert
                 * the previous block of contiguous IPs to the IPset */
                rv = ipsetInsertRangeV4(ipset, contig_start, contig_length);
                if (rv) {
                    goto END;
                }
            }
            /* start a new run of contiguous IPs */
            contig_start = slash24;
            contig_length = 256;
            continue;
        }

        if (contig_length) {
            /* we have a previous /24, but the block we just read
             * contains holes and cannot be added to it; handle this
             * existing CIDR block first */
            rv = ipsetInsertRangeV4(ipset, contig_start, contig_length);
            if (rv) {
                goto END;
            }
            contig_length = 0;
        }

        /* loop over the uint32_t in this block */
        i = 1;
        while (i < 9) {
            if (0 == block24[i]) {
                ++i;
                continue;
            }

            ipv4 = slash24 | ((i - 1) << 5);

            if (0xFFFFFFFF == block24[i]) {
                /* Check for a longer run of high bits by checking the
                 * value in adjacent uint32_t's. */
                switch (i) {
                  case 1: case 5:
                    /* check 3 addtional uint32_t's to attempt to
                     * make a /25 */
                    if (memcmp(&block24[i+1], bmap256_full, sizeof(uint32_t)*3)
                        == 0)
                    {
                        rv = ipsetInsertAddressV4(ipset, ipv4, 25, NULL);
                        i += 4;
                        break;
                    }
                    /* FALLTHROUGH */

                  case 3: case 7:
                    /* check one additional uint32_t to attempt to
                     * make a /26 */
                    if (block24[i+1] == 0xFFFFFFFF) {
                        rv = ipsetInsertAddressV4(ipset, ipv4, 26, NULL);
                        i += 2;
                        break;
                    }
                    /* FALLTHROUGH */

                  default:
                    /* we only have a /27 */
                    rv = ipsetInsertAddressV4(ipset, ipv4, 27, NULL);
                    ++i;
                    break;
                }
                if (rv) {
                    goto END;
                }
                continue;
            }

            if (swap_flag) {
                block24[i] = BSWAP32(block24[i]);
            }
            while (block24[i]) {
                assert((ipv4 - (slash24 | ((i - 1) << 5))) < 32);
                /* find position of least significant bit */
                if (!(block24[i] & 0x1)) {
                    if ((block24[i] & 0xFFFF) == 0) {
                        block24[i] >>= 16;
                        ipv4 += 16;
                    }
                    if ((block24[i] & 0xFF) == 0) {
                        block24[i] >>= 8;
                        ipv4 += 8;
                    }
                    if ((block24[i] & 0xF) == 0) {
                        block24[i] >>= 4;
                        ipv4 += 4;
                    }
                    if ((block24[i] & 0x3) == 0) {
                        block24[i] >>= 2;
                        ipv4 += 2;
                    }
                    if ((block24[i] & 0x1) == 0) {
                        block24[i] >>= 1;
                        ipv4 += 1;
                    }
                }

                /* find number of consecutive high bits that map into
                 * a CIDR block */
                switch (ipv4 & 0x1F) {
                  case 0: case 16:
                    /* we can consider at least 16 bits in this
                     * uint32_t---no need to check for 32 bits since
                     * we would have caught that above */
                    if ((block24[i] & 0xFFFF) == 0xFFFF) {
                        rv = ipsetInsertAddressV4(ipset, ipv4, 28, NULL);
                        block24[i] >>= 16;
                        ipv4 += 16;
                        break;
                    }
                    /* FALLTHROUGH */

                  case 8: case 24:
                    /* we can consider at least 8 bits in this uint32_t */
                    if ((block24[i] & 0xFF) == 0xFF) {
                        rv = ipsetInsertAddressV4(ipset, ipv4, 29, NULL);
                        block24[i] >>= 8;
                        ipv4 += 8;
                        break;
                    }
                    /* FALLTHROUGH */

                  case 4: case 12: case 20: case 28:
                    if ((block24[i] & 0xF) == 0xF) {
                        rv = ipsetInsertAddressV4(ipset, ipv4, 30, NULL);
                        block24[i] >>= 4;
                        ipv4 += 4;
                        break;
                    }
                    /* FALLTHROUGH */

                  case  2: case  6: case 10: case 14:
                  case 18: case 22: case 26: case 30:
                    if ((block24[i] & 0x3) == 0x3) {
                        rv = ipsetInsertAddressV4(ipset, ipv4, 31, NULL);
                        block24[i] >>= 2;
                        ipv4 += 2;
                        break;
                    }
                    /* FALLTHROUGH */

                  default:
                    rv = ipsetInsertAddressV4(ipset, ipv4, 32, NULL);
                    block24[i] >>= 1;
                    ++ipv4;
                    break;
                }
                if (rv) {
                    goto END;
                }
            }
        }
    }

    if (b == -1) {
        /* read error */
        rv = -1;
        goto END;
    }

    /* close final block */
    if (contig_length) {
        rv = ipsetInsertRangeV4(ipset, contig_start, contig_length);
        if (rv) {
            goto END;
        }
    }

    /* ipset should already be clean since we went to all the effort
     * to add the largest possible blocks */
    skIPSetClean(ipset);

    *ipset_out = ipset;
    rv = SKIPSET_OK;

  END:
    if (rv != SKIPSET_OK) {
        skIPSetDestroy(&ipset);
    }
    return rv;
}


/*
 *  status = ipsetReadLegacy(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetRead().
 *
 *    Read an IPset in the IPSET_REC_VERSION_LEGACY format from
 *    'stream'.
 *
 *    This function reads the header then calls another helper
 *    function depending on the data structure being used.
 */
static int
ipsetReadLegacy(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    if (IPSET_USE_IPTREE) {
        return ipsetReadLegacyIntoIPTree(ipset_out, stream, hdr);
    }
    return ipsetReadLegacyIntoRadix(ipset_out, stream, hdr);
}


/*
 *  status = ipsetReadRadixIntoIPTree(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetRead() via ipsetReadRadix().
 *
 *    Read an IPset in the IPSET_REC_VERSION_RADIX format containing
 *    only IPv4 addresses from 'stream' and create a SiLK-2 IPTree
 *    data structure.
 *
 *    See '#define IPSET_REC_VERSION_RADIX' for description of the
 *    file format.
 */
static int
ipsetReadRadixIntoIPTree(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    skipset_t *ipset = NULL;
    sk_header_entry_t *hentry;
    ipset_leaf_v4_t leaf;
    ssize_t bytes;
    ssize_t b;
    size_t count = 0;
    int rv;

    /* sanity check input */
    assert(ipset_out);
    assert(stream);
    if (skStreamCheckSilkHeader(stream, FT_IPSET, IPSET_REC_VERSION_RADIX,
                                IPSET_REC_VERSION_RADIX, NULL))
    {
        skAbort();
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        skAbort();
    }
    hentry = skHeaderGetFirstMatch(hdr, SK_HENTRY_IPSET_ID);
    if (NULL == hentry) {
        skAbort();
    }
    if (sizeof(ipset_leaf_v4_t) != skHentryIPSetGetLeafSize(hentry)) {
        skAbort();
    }

    /* create the set */
    rv = ipsetCreate(&ipset, 0, 0);
    if (rv != SKIPSET_OK) {
        goto END;
    }

    /* skip over the nodes since we only need the IPs in the leaves */
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
            rv = ipsetInsertAddressIPTree(ipset, BSWAP32(leaf.ip),leaf.prefix);
            if (rv) {
                goto END;
            }
        }
    } else {
        while ((b = skStreamRead(stream, &leaf, bytes)) == bytes) {
            ++count;
            rv = ipsetInsertAddressIPTree(ipset, leaf.ip, leaf.prefix);
            if (rv) {
                goto END;
            }
        }
    }

    if (0 != b) {
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }
    assert(skHentryIPSetGetLeafCount(hentry) == count);

    *ipset_out = ipset;
    rv = SKIPSET_OK;

  END:
    if (rv != SKIPSET_OK) {
        skIPSetDestroy(&ipset);
    }
    return rv;
}


/*
 *  status = ipsetReadRadixIntoRadix(&ipset, stream, hdr, is_ipv6);
 *
 *    Helper function for skIPSetRead() via ipsetReadRadix().
 *
 *    Read an IPset in the IPSET_REC_VERSION_RADIX format from
 *    'stream' and create a radix-tree IPset data structure.
 *
 *    See '#define IPSET_REC_VERSION_RADIX' for description of the
 *    file format.
 */
static int
ipsetReadRadixIntoRadix(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr,
    int                 is_ipv6)
{
    skipset_t *ipset = NULL;
    sk_header_entry_t *hentry;
    ssize_t bytes;
    ssize_t b;
    size_t i;
    int rv;

    /* sanity check input */
    assert(ipset_out);
    assert(stream);
    if (skStreamCheckSilkHeader(stream, FT_IPSET, IPSET_REC_VERSION_RADIX,
                                IPSET_REC_VERSION_RADIX, NULL))
    {
        skAbort();
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        skAbort();
    }
    hentry = skHeaderGetFirstMatch(hdr, SK_HENTRY_IPSET_ID);
    if (NULL == hentry) {
        skAbort();
    }

    /* create the set */
    rv = ipsetCreate(&ipset, is_ipv6, 1);
    if (rv != SKIPSET_OK) {
        goto END;
    }

    /* if no nodes or only node#0, root is a leaf */
    IPSET_ROOT_INDEX_SET(ipset, skHentryIPSetGetRootIndex(hentry),
                         (skHentryIPSetGetNodeCount(hentry) <= 1));

    if (skStreamIsSeekable(stream)
        && skHeaderIsNativeByteOrder(hdr)
        && (SK_COMPMETHOD_NONE == skHeaderGetCompressionMethod(hdr)))
    {
        /* attempt to mmap() the file */

        /* get file size and offset where the data begins */
        off_t file_size = skFileSize(skStreamGetPathname(stream));
        off_t data_start = skStreamTell(stream);
        uint8_t *buf;

        if ((0 < data_start) && (data_start < file_size)
            && (UINT32_MAX > file_size))
        {
            ipset->s.v3->mapped_size = (size_t)file_size;
            ipset->s.v3->mapped_file = mmap(0, ipset->s.v3->mapped_size,
                                            PROT_READ, MAP_SHARED,
                                            skStreamGetDescriptor(stream), 0);
            if (MAP_FAILED == ipset->s.v3->mapped_file) {
                ipset->s.v3->mapped_file = NULL;
                ipset->s.v3->mapped_size = 0;
            } else {
                /* this is the start of the file */
                buf = (uint8_t*)ipset->s.v3->mapped_file;

                /* move to start of data section (the nodes) */
                buf += data_start;
                ipset->s.v3->nodes.buf = buf;
                ipset->s.v3->nodes.entry_count
                    = skHentryIPSetGetNodeCount(hentry);

                /* move over the nodes (to the leaves)  */
                buf += (skHentryIPSetGetNodeCount(hentry)
                        * ipset->s.v3->nodes.entry_size);
                ipset->s.v3->leaves.buf = buf;
                ipset->s.v3->nodes.entry_count
                    = skHentryIPSetGetLeafCount(hentry);

                /* move over the leaves to the end of the file */
                buf += (skHentryIPSetGetLeafCount(hentry)
                        * ipset->s.v3->leaves.entry_size);
                if (buf >= ((uint8_t*)ipset->s.v3->mapped_file
                            + ipset->s.v3->mapped_size))
                {
                    /* error */
                    munmap(ipset->s.v3->mapped_file, ipset->s.v3->mapped_size);
                    ipset->s.v3->mapped_file = NULL;
                    ipset->s.v3->mapped_size = 0;

                    ipset->s.v3->nodes.buf = NULL;
                    ipset->s.v3->nodes.entry_count = 0;

                    ipset->s.v3->leaves.buf = NULL;
                    ipset->s.v3->nodes.entry_count = 0;
                }
            }
        }
    }

    if (NULL == ipset->s.v3->mapped_file) {
        /* Allocate and read the nodes */
        rv = ipsetAllocEntries(&ipset->s.v3->nodes,
                               skHentryIPSetGetNodeCount(hentry));
        if (rv) {
            goto END;
        }

        bytes = (skHentryIPSetGetNodeCount(hentry)
                 * ipset->s.v3->nodes.entry_size);
        b = skStreamRead(stream, ipset->s.v3->nodes.buf, bytes);
        if (b != bytes) {
            if (b > 0) {
                /* short read */
                rv = SKIPSET_ERR_FILEIO;
                goto END;
            }
            if (b == -1) {
                /* read error */
                rv = SKIPSET_ERR_FILEIO;
                goto END;
            }
        }
        ipset->s.v3->nodes.entry_count = skHentryIPSetGetNodeCount(hentry);

        /* now, if the data is not in native byte order, we need to
         * byte-swap the values */
        if (!skHeaderIsNativeByteOrder(hdr)) {
#if SK_ENABLE_IPV6
            if (ipset->is_ipv6) {
                ipset_node_v6_t *node;
                for (i = 0, node = (ipset_node_v6_t*)ipset->s.v3->nodes.buf;
                     i < ipset->s.v3->nodes.entry_count;
                     ++i, ++node)
                {
                    node->child[0] = BSWAP32(node->child[0]);
                    node->child[1] = BSWAP32(node->child[1]);
                    node->ip.ip[0] = BSWAP64(node->ip.ip[0]);
                    node->ip.ip[1] = BSWAP64(node->ip.ip[1]);
                }
            } else
#endif  /* SK_ENABLE_IPV6 */
            {
                ipset_node_v4_t *node;
                for (i = 0, node = (ipset_node_v4_t*)ipset->s.v3->nodes.buf;
                     i < ipset->s.v3->nodes.entry_count;
                     ++i, ++node)
                {
                    node->child[0] = BSWAP32(node->child[0]);
                    node->child[1] = BSWAP32(node->child[1]);
                    node->ip = BSWAP32(node->ip);
                }
            }
        }

        /* Allocate and read the leaves */
        rv = ipsetAllocEntries(&ipset->s.v3->leaves,
                               skHentryIPSetGetLeafCount(hentry));
        if (rv) {
            goto END;
        }

        bytes = (skHentryIPSetGetLeafCount(hentry)
                 * ipset->s.v3->leaves.entry_size);
        b = skStreamRead(stream, ipset->s.v3->leaves.buf, bytes);
        if (b != bytes) {
            if (b > 0) {
                /* short read */
                rv = SKIPSET_ERR_FILEIO;
                goto END;
            }
            if (b == -1) {
                /* read error */
                rv = SKIPSET_ERR_FILEIO;
                goto END;
            }
        }
        ipset->s.v3->leaves.entry_count = skHentryIPSetGetLeafCount(hentry);

        /* now, if the data is not in native byte order, we need to
         * byte-swap the values */
        if (!skHeaderIsNativeByteOrder(hdr)) {
#if SK_ENABLE_IPV6
            if (ipset->is_ipv6) {
                ipset_leaf_v6_t *leaf;
                for (i = 0, leaf = (ipset_leaf_v6_t*)ipset->s.v3->leaves.buf;
                     i < ipset->s.v3->leaves.entry_count;
                     ++i, ++leaf)
                {
                    leaf->ip.ip[0] = BSWAP64(leaf->ip.ip[0]);
                    leaf->ip.ip[1] = BSWAP64(leaf->ip.ip[1]);
                }
            } else
#endif  /* SK_ENABLE_IPV6 */
            {
                ipset_leaf_v4_t *leaf;
                for (i = 0, leaf = (ipset_leaf_v4_t*)ipset->s.v3->leaves.buf;
                     i < ipset->s.v3->leaves.entry_count;
                     ++i, ++leaf)
                {
                    leaf->ip = BSWAP32(leaf->ip);
                }
            }
        }
    }

    /* check that the file is valid */
    rv = ipsetVerify(ipset);
    if (rv) {
        goto END;
    }

    *ipset_out = ipset;
    rv = SKIPSET_OK;

  END:
    if (rv != SKIPSET_OK) {
        skIPSetDestroy(&ipset);
    }
    return rv;
}


/*
 *  status = ipsetReadRadix(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetRead().
 *
 *    Read an IPset in the IPSET_REC_VERSION_RADIX format from
 *    'stream'.
 *
 *    This function reads the header then calls another helper
 *    function depending on the data structure being used.
 */
static int
ipsetReadRadix(
    skipset_t         **ipset_out,
    skstream_t         *stream,
    sk_file_header_t   *hdr)
{
    sk_header_entry_t *hentry;
    int is_ipv6;

    hentry = skHeaderGetFirstMatch(hdr, SK_HENTRY_IPSET_ID);
    if (NULL == hentry) {
        return SKIPSET_ERR_FILEHEADER;
    }

    /* Verify that children/node is what we expect */
    if (IPSET_NUM_CHILDREN != skHentryIPSetGetChildPerNode(hentry)) {
        return SKIPSET_ERR_FILEHEADER;
    }

    /* See if leaf and node sizes are for IPv4 or IPv6.  We must check
     * IPv4 first, since IPv4 and IPv6 sizes are identical when SiLK
     * is built without IPv6 support */
    if (sizeof(ipset_leaf_v4_t) == skHentryIPSetGetLeafSize(hentry)
        && sizeof(ipset_node_v4_t) == skHentryIPSetGetNodeSize(hentry))
    {
        is_ipv6 = 0;
    }
    else if (sizeof(ipset_leaf_v6_t) == skHentryIPSetGetLeafSize(hentry)
             && sizeof(ipset_node_v6_t) == skHentryIPSetGetNodeSize(hentry))
    {
        is_ipv6 = 1;
#if  !SK_ENABLE_IPV6
        /* IPv6 IPSet not supported by this build of SiLK */
        return SKIPSET_ERR_IPV6;
#endif
    } else {
        /* Unrecognized record sizes */
        return SKIPSET_ERR_FILEHEADER;
    }

    /* check for an empty IPset */
    if (0 == skHentryIPSetGetNodeCount(hentry)
        && 0 == skHentryIPSetGetLeafCount(hentry))
    {
        if (!is_ipv6 && IPSET_USE_IPTREE) {
            return ipsetCreate(ipset_out, 0, 0);
        }
        return ipsetCreate(ipset_out, is_ipv6, 1);
    }

    if (!is_ipv6 && IPSET_USE_IPTREE) {
        /* Read IPv4-only file into the IPTree format */
        return ipsetReadRadixIntoIPTree(ipset_out, stream, hdr);
    }
    return ipsetReadRadixIntoRadix(ipset_out, stream, hdr, is_ipv6);
}


/*
 *  rv = ipsetRemoveAddressIPTree(ipset, ipv4, prefix);
 *
 *    Helper function for skIPSetRemoveAddress(); may also be called
 *    by other internal functions.
 */
static int
ipsetRemoveAddressIPTree(
    skipset_t          *ipset,
    uint32_t            ipv4,
    uint32_t            prefix)
{
    uint32_t ipv4_end;

    assert(ipset);
    assert(1 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);
    assert(prefix > 0 || ipv4 == 0);
    assert(prefix <= 32);

    if (32 == prefix) {
        if (ipset->s.v2->nodes[ipv4 >> 16]) {
            skIPTreeNodeClear(ipv4, ipset->s.v2->nodes[ipv4 >> 16]);
        }

    } else if (prefix <= 16) {
        ipv4_end = ((UINT32_MAX >> prefix) | ipv4) >> 16;
        ipv4 >>= 16;
        do {
            if (ipset->s.v2->nodes[ipv4]) {
                free(ipset->s.v2->nodes[ipv4]);
                ipset->s.v2->nodes[ipv4] = NULL;
            }
        } while (ipv4++ < ipv4_end);

    } else if (prefix <= 27) {
        ipv4_end = ((UINT32_MAX >> prefix) | ipv4) >> 5;
        ipv4 >>= 5;
        if (NULL == ipset->s.v2->nodes[ipv4 >> 11]) {
            goto END;
        }
        do {
            /* nodes are allocated on /16, already shifted by 5, need
             * to shift by 11 more to get the complete shift by 16.
             * Mask by 0x7ff (0xffff>>5) to get addressBlock */
            memset(&ipset->s.v2->nodes[ipv4>>11]->addressBlock[ipv4 & 0x07FF],
                   0, sizeof(uint32_t));
        } while (ipv4++ < ipv4_end);

    } else {
        if (NULL == ipset->s.v2->nodes[ipv4 >> 16]) {
            goto END;
        }
        ipv4_end = (UINT32_MAX >> prefix) | ipv4;
        do {
            skIPTreeNodeClear(ipv4, ipset->s.v2->nodes[ipv4 >> 16]);
        } while (ipv4++ < ipv4_end);
    }

  END:
    return SKIPSET_OK;
}


/*
 *  rv = ipsetRemoveAddressV4(ipset, ipv4, prefix, find_state);
 *  rv = ipsetRemoveAddressV6(ipset, ipv6, prefix, find_state);
 *
 *    Helper function for skIPSetRemoveAddress(); may also be called
 *    by other internal functions.
 *
 *    Remove the CIDR block ipv4/prefix or ipv6/prefix from the IPset.
 *
 *    'find_state' should be NULL or the result of calling
 *    ipsetFindV4() or ipsetFindV6() on the ip/prefix pair.  When
 *    'find_state' is NULL, this function does the address search
 *    itself.
 *
 *    Return SKIPSET_OK if the address block was successfully removed
 *    or if the address block does not appear in the IPset.  Return
 *    SKIPSET_ERR_ALLOC if there is a memory allocation error.
 */
static int
ipsetRemoveAddressV4(
    skipset_t          *ipset,
    const uint32_t      ipv4,
    const uint32_t      prefix,
    const ipset_find_t *find_state)
{
    ipset_find_t find_state_local;
    ipset_node_v4_t *parent = NULL;
    ipset_leaf_v4_t *leaf;
    uint32_t which_child;
    uint32_t i;
    uint32_t j;
    int rv;

    assert(ipset);
    assert(sizeof(ipset_node_v4_t) == ipset->s.v3->nodes.entry_size);
    assert(prefix > 0 || ipv4 == 0);
    assert(prefix <= 32);
    assert(0 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);

    /* use the passed in 'find_state' if given */
    if (find_state) {
        rv = find_state->result;
    } else {
        rv = ipsetFindV4(ipset, ipv4, prefix, &find_state_local);
        /* if IP was not found, we can return */
        if (SKIPSET_ERR_NOTFOUND == rv
            || SKIPSET_ERR_EMPTY == rv
            || SKIPSET_ERR_MULTILEAF == rv)
        {
            return SKIPSET_OK;
        }
        find_state = &find_state_local;
    }
    ipset->is_dirty = 1;

    /* if we matched a CIDR block in the IPset that is larger than the
     * block/address we want to remove, we must break the CIDR
     * block. */
    if ((SKIPSET_OK == rv) && (find_state->bitpos < prefix)) {
        uint32_t old_ipv4, mid_ipv4, old_prefix;

        assert(1 == find_state->node_is_leaf);

        leaf = LEAF_PTR_V4(ipset, find_state->node_idx);
        old_ipv4 = leaf->ip;
        old_prefix = leaf->prefix;

        /* remove the current IP */
        if (IPSET_NO_PARENT == find_state->parent_idx) {
            skIPSetRemoveAll(ipset);
        } else {
            parent = NODE_PTR_V4(ipset, find_state->parent_idx);
            which_child = WHICH_CHILD_V4(leaf->ip, parent->prefix);
            if (NUM_BITS <= leaf->prefix - parent->prefix) {
                /* leaf occupies a single child[] entry */
                parent->child[which_child] = 0;
                NODEPTR_CHILD_CLEAR_LEAF(parent, which_child);
            } else {
                for (i = which_child, j = 0;
                     j < (1u << (NUM_BITS - (leaf->prefix - parent->prefix)));
                     ++i, ++j)
                {
                    parent->child[i] = 0;
                }
                NODEPTR_CHILD_CLEAR_LEAF2(parent, which_child, i - 1);
                NODEPTR_CHILD_CLEAR_REPEAT2(parent, which_child, i - 1);
            }
            LEAFIDX_FREE(ipset, find_state->node_idx);
        }

        /* insert all IPs not part of the removed block */
        do {
            ++old_prefix;
            /* find midpoint of this block */
            mid_ipv4 = old_ipv4 | (1u << (32 - old_prefix));
            if (ipv4 < mid_ipv4) {
                /* insert upper half of old CIDR block */
                rv = ipsetInsertAddressV4(ipset, mid_ipv4, old_prefix, NULL);
                /* no change to old_ipv4 */
            } else {
                /* insert lower half of old CIDR block */
                rv = ipsetInsertAddressV4(ipset, old_ipv4, old_prefix, NULL);
                /* adjust old_ipv4 to the base of the upper half */
                old_ipv4 = mid_ipv4;
            }
        } while (0 == rv && old_prefix < prefix);

        /* all done */
        return rv;
    }

    /* otherwise, we exactly matched the node we want to remove, or we
     * are removing a CIDR block that is only partially populated.
     * These cases are handled the same way: modify the parent of the
     * node to remove the node we found. */

    /* handle the simple case */
    if (IPSET_NO_PARENT == find_state->parent_idx) {
        return skIPSetRemoveAll(ipset);
    }
    parent = NODE_PTR_V4(ipset, find_state->parent_idx);

    if (SKIPSET_ERR_SUBSET == rv
        && (NUM_BITS > prefix - parent->prefix))
    {
        /* need to clear multiple child[] entries */
        which_child = WHICH_CHILD_V4(ipv4, parent->prefix);
        for (i = which_child, j = 0;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            if (parent->child[i]) {
                if (!NODEPTR_CHILD_IS_LEAF(parent, i)) {
                    ipsetDestroySubtree(ipset, parent->child[i], 1);
                    parent->child[i] = 0;
                } else if (NODEPTR_CHILD_IS_REPEAT(parent, i)) {
                    parent->child[i] = 0;
                } else {
                    LEAFIDX_FREE(ipset, parent->child[i]);
                    parent->child[i] = 0;
                }
            }
        }
        NODEPTR_CHILD_CLEAR_LEAF2(parent, which_child, i - 1);
        NODEPTR_CHILD_CLEAR_REPEAT2(parent, which_child, i - 1);

        /* handle case where node has a single child */
        ipsetFixNodeSingleChild(ipset, find_state->parent_idx, 0);
        return SKIPSET_OK;
    }

    if (!find_state->node_is_leaf) {
        /* remove this node from the parent, and delete the node and
         * everything below it */
        parent->child[find_state->parents_child] = 0;
        ipsetDestroySubtree(ipset, find_state->node_idx, 1);
    } else {
        /* remove this leaf from the parent */
        leaf = LEAF_PTR_V4(ipset, find_state->node_idx);
        if (NUM_BITS <= leaf->prefix - parent->prefix) {
            /* leaf occupies a single child[] entry */
            parent->child[find_state->parents_child] = 0;
            NODEPTR_CHILD_CLEAR_LEAF(parent, find_state->parents_child);
        } else {
            /* leaf occupies multiple child[] entries */
            which_child = WHICH_CHILD_V4(leaf->ip, parent->prefix);
            for (i = which_child, j = 0;
                 j < (1u << (NUM_BITS - (leaf->prefix - parent->prefix)));
                 ++i, ++j)
            {
                parent->child[i] = 0;
            }
            NODEPTR_CHILD_CLEAR_LEAF2(parent, which_child, i - 1);
            NODEPTR_CHILD_CLEAR_REPEAT2(parent, which_child, i - 1);
        }
        LEAFIDX_FREE(ipset, find_state->node_idx);
    }

    /* handle case where node has a single child */
    ipsetFixNodeSingleChild(ipset, find_state->parent_idx, 0);
    return SKIPSET_OK;
}

#if SK_ENABLE_IPV6
static int
ipsetRemoveAddressV6(
    skipset_t          *ipset,
    const ipset_ipv6_t *ipv6,
    const uint32_t      prefix,
    const ipset_find_t *find_state)
{
    ipset_find_t find_state_local;
    ipset_node_v6_t *parent = NULL;
    ipset_leaf_v6_t *leaf;
    uint32_t which_child;
    uint32_t i;
    uint32_t j;
    int rv;

    assert(ipset);
    assert(sizeof(ipset_node_v6_t) == ipset->s.v3->nodes.entry_size);
    assert(0 < prefix && prefix <= 128);
    assert(0 == ipset->is_iptree);
    assert(1 == ipset->is_ipv6);

    /* use the passed in 'find_state' if given */
    if (find_state) {
        rv = find_state->result;
    } else {
        rv = ipsetFindV6(ipset, ipv6, prefix, &find_state_local);
        /* if IP was not found, we can return */
        if (SKIPSET_ERR_NOTFOUND == rv
            || SKIPSET_ERR_EMPTY == rv
            || SKIPSET_ERR_MULTILEAF == rv)
        {
            return SKIPSET_OK;
        }
        find_state = &find_state_local;
    }
    ipset->is_dirty = 1;

    /* if we matched a CIDR block in the IPset that is larger than the
     * block/address we want to remove, we must break the CIDR
     * block. */
    if ((SKIPSET_OK == rv) && (find_state->bitpos < prefix)) {
        ipset_ipv6_t old_ipv6, mid_ipv6;
        uint32_t old_prefix;

        assert(1 == find_state->node_is_leaf);

        leaf = LEAF_PTR_V6(ipset, find_state->node_idx);
        IPSET_IPV6_COPY(&old_ipv6, &leaf->ip);
        old_prefix = leaf->prefix;

        /* remove the current IP */
        if (IPSET_NO_PARENT == find_state->parent_idx) {
            skIPSetRemoveAll(ipset);
        } else {
            parent = NODE_PTR_V6(ipset, find_state->parent_idx);
            which_child = WHICH_CHILD_V6(&leaf->ip, parent->prefix);
            if (NUM_BITS <= leaf->prefix - parent->prefix) {
                /* leaf occupies a single child[] entry */
                parent->child[which_child] = 0;
                NODEPTR_CHILD_CLEAR_LEAF(parent, which_child);
            } else {
                for (i = which_child, j = 0;
                     j < (1u << (NUM_BITS - (leaf->prefix - parent->prefix)));
                     ++i, ++j)
                {
                    parent->child[i] = 0;
                }
                NODEPTR_CHILD_CLEAR_LEAF2(parent, which_child, i - 1);
                NODEPTR_CHILD_CLEAR_REPEAT2(parent, which_child, i - 1);
            }
            LEAFIDX_FREE(ipset, find_state->node_idx);
        }

        /* insert all IPs not part of the removed block */
        IPSET_IPV6_COPY(&mid_ipv6, &old_ipv6);
        do {
            ++old_prefix;
            if (old_prefix <= 64) {
                /* find midpoint of this block */
                mid_ipv6.ip[0]
                    = old_ipv6.ip[0] | (UINT64_C(1) << (64 - old_prefix));
                if (ipv6->ip[0] < mid_ipv6.ip[0]) {
                    /* insert upper half of old CIDR block */
                    rv = ipsetInsertAddressV6(ipset, &mid_ipv6, old_prefix,
                                              NULL);
                    /* no change to old_ipv6 */
                    if (64 == old_prefix) {
                        /* reset mid_ipv6 so it is correct for lower
                         * half of the IP */
                        mid_ipv6.ip[0] = old_ipv6.ip[0];
                    }
                } else {
                    /* insert lower hald of old CIDR block */
                    rv = ipsetInsertAddressV6(ipset, &old_ipv6, old_prefix,
                                              NULL);
                    /* adjust old_ipv6 to the base of the upper half */
                    old_ipv6.ip[0] = mid_ipv6.ip[0];
                }
            } else {
                mid_ipv6.ip[1]
                    = old_ipv6.ip[1] | (UINT64_C(1) << (128 - old_prefix));
                if (ipv6->ip[1] < mid_ipv6.ip[1]) {
                    rv = ipsetInsertAddressV6(ipset, &mid_ipv6, old_prefix,
                                              NULL);
                } else {
                    rv = ipsetInsertAddressV6(ipset, &old_ipv6, old_prefix,
                                              NULL);
                    old_ipv6.ip[1] = mid_ipv6.ip[1];
                }
            }
        } while (0 == rv && old_prefix < prefix);

        /* done */
        return rv;
    }

    /* otherwise, we exactly matched the node we want to remove, or we
     * are removing a CIDR block that is only partially populated.
     * These cases are handled the same way: modify the parent of the
     * node to remove the node we found. */

    /* handle the simple case */
    if (IPSET_NO_PARENT == find_state->parent_idx) {
        return skIPSetRemoveAll(ipset);
    }

    parent = NODE_PTR_V6(ipset, find_state->parent_idx);

    if (SKIPSET_ERR_SUBSET == rv
        && (NUM_BITS > prefix - parent->prefix))
    {
        /* need to clear multiple child[] entries */
        which_child = WHICH_CHILD_V6(ipv6, parent->prefix);
        for (i = which_child, j = 0;
             j < (1u << (NUM_BITS - (prefix - parent->prefix)));
             ++i, ++j)
        {
            if (parent->child[i]) {
                if (!NODEPTR_CHILD_IS_LEAF(parent, i)) {
                    ipsetDestroySubtree(ipset, parent->child[i], 1);
                    parent->child[i] = 0;
                } else if (NODEPTR_CHILD_IS_REPEAT(parent, i)) {
                    parent->child[i] = 0;
                } else {
                    LEAFIDX_FREE(ipset, parent->child[i]);
                    parent->child[i] = 0;
                }
            }
        }
        NODEPTR_CHILD_CLEAR_LEAF2(parent, which_child, i - 1);
        NODEPTR_CHILD_CLEAR_REPEAT2(parent, which_child, i - 1);

        /* handle case where node has a single child */
        ipsetFixNodeSingleChild(ipset, find_state->parent_idx, 0);
        return SKIPSET_OK;
    }

    if (!find_state->node_is_leaf) {
        /* remove this node from the parent, and delete the node and
         * everything below it */
        parent->child[find_state->parents_child] = 0;
        ipsetDestroySubtree(ipset, find_state->node_idx, 1);
    } else {
        /* remove this leaf from the parent */
        leaf = LEAF_PTR_V6(ipset, find_state->node_idx);
        if (NUM_BITS <= leaf->prefix - parent->prefix) {
            /* leaf occupies a single child[] entry */
            parent->child[find_state->parents_child] = 0;
            NODEPTR_CHILD_CLEAR_LEAF(parent, find_state->parents_child);
        } else {
            /* leaf occupies multiple child[] entries */
            which_child = WHICH_CHILD_V6(&leaf->ip, parent->prefix);
            for (i = which_child, j = 0;
                 j < (1u << (NUM_BITS - (leaf->prefix - parent->prefix)));
                 ++i, ++j)
            {
                parent->child[i] = 0;
            }
            NODEPTR_CHILD_CLEAR_LEAF2(parent, which_child, i - 1);
            NODEPTR_CHILD_CLEAR_REPEAT2(parent, which_child, i - 1);
        }
        LEAFIDX_FREE(ipset, find_state->node_idx);
    }

    /* handle case where node has a single child */
    ipsetFixNodeSingleChild(ipset, find_state->parent_idx, 0);
    return SKIPSET_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  new_leaf_id = ipsetReplaceNodeWithLeaf(ipset, parent, which_child);
 *
 *    Replace the node at position child['which_child'] on the node
 *    'parent' with a leaf.  Return the index of the new leaf.
 *
 *    If 'parent' is NULL, the node at the root of the 'ipset' is
 *    replaced with a leaf.
 *
 *    This function always succeeds, since destroying the node and its
 *    subtree provides leaf entries on the free_list.
 */
static uint32_t
ipsetReplaceNodeWithLeaf(
    skipset_t          *ipset,
    ipset_node_t       *parent,
    uint32_t            which_child)
{
    uint32_t new_leaf_idx;

    assert(ipset);
    assert(which_child < IPSET_NUM_CHILDREN);

    if (NULL == parent) {
        assert(0 == IPSET_ROOT_IS_LEAF(ipset));
        ipsetDestroySubtree(ipset, IPSET_ROOT_INDEX(ipset), 1);
        ASSERT_OK(ipsetNewEntries(ipset, 0, 1, NULL, &new_leaf_idx));
        IPSET_ROOT_INDEX_SET(ipset, new_leaf_idx, 1);
    } else {
        assert(0 == NODEPTR_CHILD_IS_LEAF(&parent->v4, which_child));
        ipsetDestroySubtree(ipset, parent->v4.child[which_child], 1);
        ASSERT_OK(ipsetNewEntries(ipset, 0, 1, NULL, &new_leaf_idx));
        parent->v4.child[which_child] = new_leaf_idx;
        NODEPTR_CHILD_SET_LEAF(&parent->v4, which_child);
    }

    return new_leaf_idx;
}



/*
 *  ipsetSortLeaves(ipset);
 *
 *    Sort the leaves so they occur in ascending order in memory.
 */
static void
ipsetSortLeaves(
    skipset_t          *ipset)
{
    uint32_t to_visit[IPSET_MAX_DEPTH];
#if   IPSET_NUM_CHILDREN < 256
    uint8_t children[IPSET_MAX_DEPTH];
#else
    uint16_t children[IPSET_MAX_DEPTH];
#endif
    uint32_t child_idx;
    uint32_t depth;
    uint32_t cur;
    ipset_node_t *node;

    /* first, ensure the tree is compact */
    ipsetCompact(ipset);

    /* nothing to do if the root is a leaf */
    if (IPSET_ROOT_IS_LEAF(ipset)) {
        return;
    }

    /* the new index to assign to each leaf */
    child_idx = 0;

    /* visit the leaves in the tree to update the indexes */
    depth = 0;
    to_visit[depth] = IPSET_ROOT_INDEX(ipset);
    children[depth++] = 0;

    cur = 0;
    while (depth) {
      VISIT_NODE:
        node = NODE_PTR(ipset, to_visit[cur]);
        for ( ; children[cur] < IPSET_NUM_CHILDREN; ++children[cur]) {
            if (0 == node->v4.child[children[cur]]) {
                /* no-op */
            } else if (!NODEPTR_CHILD_IS_LEAF(&node->v4, children[cur])) {
                /* this is a node; push this node onto the visit list
                 * and break out of the for() loop to visit the node
                 * immediately */
                to_visit[depth] = node->v4.child[children[cur]];
                children[depth++] = 0;
                /* we're done with this entry on the node */
                ++children[cur];
                ++cur;
                goto VISIT_NODE;
            } else if (NODEPTR_CHILD_IS_REPEAT(&node->v4, children[cur])) {
                /* leaf is a repeat, use current child_idx */
                node->v4.child[children[cur]] = child_idx;
            } else {
                /* this is a new leaf */
                ++child_idx;
                node->v4.child[children[cur]] = child_idx;
            }
        }
        assert(IPSET_NUM_CHILDREN == children[cur]);
        --cur;
        --depth;
    }

    /* child_idx is number of leaves if we ignore the entry at 0 */
    if (child_idx+1 != ipset->s.v3->leaves.entry_count) {
        skAbort();
    }

    /* sort the leaves, ignoring the entry at position 0 */
#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        skQSort(ipset->s.v3->leaves.buf + ipset->s.v3->leaves.entry_size,
                child_idx, ipset->s.v3->leaves.entry_size, ipsetLeafCompareV6);
    } else
#endif
    {
        skQSort(ipset->s.v3->leaves.buf + ipset->s.v3->leaves.entry_size,
                child_idx, ipset->s.v3->leaves.entry_size, ipsetLeafCompareV4);
    }
}


/*
 *  status = ipsetSubtractCallbackV4(ipv4, prefix, ipset);
 *  status = ipsetSubtractCallbackV6(ipv6, prefix, ipset);
 *  status = ipsetSubtractCallback(ipaddr, prefix, ipset);
 *
 *    Helper callback function for skIPSetSubtract().
 *
 *    Removes the CIDR block specified by 'ipaddr'/'prefix' from
 *    'ipset'.
 */
static int
ipsetSubtractCallbackV4(
    uint32_t            ipv4,
    uint32_t            prefix,
    void               *v_ipset)
{
    return ipsetRemoveAddressV4((skipset_t*)v_ipset, ipv4, prefix, NULL);
}

#if SK_ENABLE_IPV6
static int
ipsetSubtractCallbackV6(
    const ipset_ipv6_t *ipv6,
    uint32_t            prefix,
    void               *v_ipset)
{
    return ipsetRemoveAddressV6((skipset_t*)v_ipset, ipv6, prefix, NULL);
}
#endif  /* SK_ENABLE_IPV6 */

static int
ipsetSubtractCallback(
    skipaddr_t         *ipaddr,
    uint32_t            prefix,
    void               *v_ipset)
{
    return skIPSetRemoveAddress((skipset_t*)v_ipset, ipaddr, prefix);
}


/*
 *  status = ipsetUnionCallbackIPTree(ipv4, prefix, ipset);
 *  status = ipsetUnionCallbackV4(ipv4, prefix, ipset);
 *  status = ipsetUnionCallbackV6(ipv6, prefix, ipset);
 *  status = ipsetUnionCallback(ipaddr, prefix, ipset);
 *
 *    Helper callback function for skIPSetUnion().
 *
 *    Adds the CIDR block specified by 'ipaddr'/'prefix' to 'ipset'.
 */
static int
ipsetUnionCallbackIPTree(
    uint32_t            ipv4,
    uint32_t            prefix,
    void               *v_ipset)
{
    return ipsetInsertAddressIPTree((skipset_t*)v_ipset, ipv4, prefix);
}

static int
ipsetUnionCallbackV4(
    uint32_t            ipv4,
    uint32_t            prefix,
    void               *v_ipset)
{
    return ipsetInsertAddressV4((skipset_t*)v_ipset, ipv4, prefix, NULL);
}

#if SK_ENABLE_IPV6
static int
ipsetUnionCallbackV6(
    const ipset_ipv6_t *ipv6,
    uint32_t            prefix,
    void               *v_ipset)
{
    return ipsetInsertAddressV6((skipset_t*)v_ipset, ipv6, prefix, NULL);
}
#endif  /* SK_ENABLE_IPV6 */

static int
ipsetUnionCallback(
    skipaddr_t         *ipaddr,
    uint32_t            prefix,
    void               *v_ipset)
{
    return skIPSetInsertAddress((skipset_t*)v_ipset, ipaddr, prefix);
}


/*
 *  status = ipsetVerify(ipset);
 *
 *    Verify that all the indexes in a Radix-Tree based IPset are
 *    valid---that is, less than number of allocated nodes and leaves.
 *    Return SKIPSET_OK on success, or SKIPSET_ERR_CORRUPT on failure.
 *    Verify that every prefix is less than the maximum.
 */
static int
ipsetVerify(
    const skipset_t    *ipset)
{
    uint32_t bitmap_size;
    sk_bitmap_t *isfree = NULL;
    uint32_t node_idx;
    uint32_t j;
    int rv = SKIPSET_ERR_CORRUPT;

    if (NULL == ipset || 1 == ipset->is_iptree) {
        return SKIPSET_OK;
    }
    if (ipset->s.v3->nodes.entry_count > ipset->s.v3->nodes.entry_capacity) {
        return rv;
    }
    if (ipset->s.v3->leaves.entry_count > ipset->s.v3->leaves.entry_capacity) {
        return rv;
    }

    if (IPSET_ISEMPTY(ipset)) {
        return SKIPSET_OK;
    }

    if (ipset->s.v3->root_is_leaf) {
        ipset_leaf_t *leaf;

        if (IPSET_ROOT_INDEX(ipset) >= ipset->s.v3->leaves.entry_count) {
            return rv;
        }
        leaf = LEAF_PTR(ipset, IPSET_ROOT_INDEX(ipset));
        if (ipset->is_ipv6) {
            if (leaf->v4.prefix > 128) {
                return rv;
            }
        } else if (leaf->v4.prefix > 32) {
            return rv;
        }
        return SKIPSET_OK;

    } else if (IPSET_ROOT_INDEX(ipset) >= ipset->s.v3->nodes.entry_count) {
        return rv;
    }

    /* create a bitmap to note which nodes and leaves are on the free
     * list.  this is the size of the bitmap */
    if (ipset->s.v3->nodes.entry_count > ipset->s.v3->leaves.entry_count) {
        bitmap_size = ipset->s.v3->nodes.entry_count;
    } else {
        bitmap_size = ipset->s.v3->leaves.entry_count;
    }
    if (skBitmapCreate(&isfree, bitmap_size)) {
        rv = SKIPSET_ERR_ALLOC;
        goto END;
    }

    /* fill the bitmap for nodes on the free list */
    for (node_idx = ipset->s.v3->nodes.free_list;
         0 != node_idx;
         node_idx = NODEIDX_FREE_LIST(ipset, node_idx))
    {
        if (node_idx >= ipset->s.v3->nodes.entry_count) {
            goto END;
        }
        skBitmapSetBit(isfree, node_idx);
    }

#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        ipset_node_v6_t *node;
        ipset_leaf_v6_t *leaf;

        for (node_idx = 0, node = (ipset_node_v6_t*)ipset->s.v3->nodes.buf;
             node_idx < ipset->s.v3->nodes.entry_count;
             ++node_idx, ++node)
        {
            if (skBitmapGetBit(isfree, node_idx)) {
                continue;
            }
            if (node->prefix > 128) {
                goto END;
            }
            for (j = 0; j < IPSET_NUM_CHILDREN; ++j) {
                if (NODEPTR_CHILD_IS_LEAF(node, j)) {
                    if (node->child[j] >= ipset->s.v3->leaves.entry_count) {
                        goto END;
                    }
                } else if (node->child[j] >= ipset->s.v3->nodes.entry_count) {
                    goto END;
                }
            }
        }

        skBitmapClearAllBits(isfree);
        /* mark leaves that are on the free list */
        for (node_idx = ipset->s.v3->leaves.free_list;
             0 != node_idx;
             node_idx = LEAFIDX_FREE_LIST(ipset, node_idx))
        {
            if (node_idx >= ipset->s.v3->leaves.entry_count) {
                goto END;
            }
            skBitmapSetBit(isfree, node_idx);
        }

        for (node_idx = 0, leaf = (ipset_leaf_v6_t*)ipset->s.v3->leaves.buf;
             node_idx < ipset->s.v3->leaves.entry_count;
             ++node_idx, ++leaf)
        {
            if (skBitmapGetBit(isfree, node_idx)) {
                continue;
            }
            if (leaf->prefix > 128) {
                goto END;
            }
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        ipset_node_v4_t *node;
        ipset_leaf_v4_t *leaf;

        for (node_idx = 0, node = (ipset_node_v4_t*)ipset->s.v3->nodes.buf;
             node_idx < ipset->s.v3->nodes.entry_count;
             ++node_idx, ++node)
        {
            if (skBitmapGetBit(isfree, node_idx)) {
                continue;
            }
            if (node->prefix > 32) {
                goto END;
            }
            for (j = 0; j < IPSET_NUM_CHILDREN; ++j) {
                if (NODEPTR_CHILD_IS_LEAF(node, j)) {
                    if (node->child[j] >= ipset->s.v3->leaves.entry_count) {
                        goto END;
                    }
                } else if (node->child[j] >= ipset->s.v3->nodes.entry_count) {
                    goto END;
                }
            }
        }

        skBitmapClearAllBits(isfree);
        /* mark leaves that are on the free list */
        for (node_idx = ipset->s.v3->leaves.free_list;
             0 != node_idx;
             node_idx = LEAFIDX_FREE_LIST(ipset, node_idx))
        {
            if (node_idx >= ipset->s.v3->leaves.entry_count) {
                goto END;
            }
            skBitmapSetBit(isfree, node_idx);
        }

        for (node_idx = 0, leaf = (ipset_leaf_v4_t*)ipset->s.v3->leaves.buf;
             node_idx < ipset->s.v3->leaves.entry_count;
             ++node_idx, ++leaf)
        {
            if (skBitmapGetBit(isfree, node_idx)) {
                continue;
            }
            if (leaf->prefix > 128) {
                goto END;
            }
        }
    }

    rv = SKIPSET_OK;

  END:
    if (isfree) {
        skBitmapDestroy(&isfree);
    }
    return rv;
}


/*
 *  status = ipsetWalkInternalV4(ipset, callback, cb_data);
 *  status = ipsetWalkInternalV6(ipset, callback, cb_data);
 *
 *    These are internal functions for walking over the contents a
 *    Radix-Tree based IPset.  They are NOT helper functions for
 *    skIPSetWalk().  The helper functions for skIPSetWalk() are named
 *    ipsetWalkV4() and ipsetWalkV6().
 *
 *    These function visit the leaves in an IPset in order from lowest
 *    to highest.  They will use the iterator if the IPset is clean,
 *    or recursively walk the nodes if the IPset is dirty.
 *
 *    These functions always invoke the 'callback' with the IP/prefix
 *    pair---that is they do not visit individual IP addresses.
 *
 *    The callback will be invoked with the IP in the form stored
 *    internally by the IPset, eliminating the conversion to/from an
 *    skipaddr_t that skIPSetWalk() performs.
 *
 *    The function stops visiting the leaves when the callback returns
 *    a value other than SKIPSET_OK.
 *
 *    Return SKIPSET_OK if the IPset is empty; otherwise, return the
 *    value returned by the final call to the callback.
 */
static int
ipsetWalkInternalV4(
    const skipset_t    *ipset,
    ipset_walk_v4_fn_t  callback,
    void               *cb_data)
{
    uint8_t is_leaf[IPSET_MAX_DEPTH_V4];
    uint32_t to_visit[IPSET_MAX_DEPTH_V4];
    const ipset_node_v4_t *node;
    const ipset_leaf_v4_t *leaf;
    uint32_t depth;
    uint32_t i;
    int rv = SKIPSET_OK;

    assert(ipset);
    assert(0 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);
    assert(callback);

    if (IPSET_ISEMPTY(ipset)) {
        return SKIPSET_OK;
    }

    /* use the iterator if we can */
    if (!ipset->is_dirty) {
        uint32_t cur = IPSET_ITER_FIRST_LEAF;
        while (0 == rv && cur < ipset->s.v3->leaves.entry_count) {
            leaf = LEAF_PTR_V4(ipset, cur);
            rv = callback(leaf->ip, leaf->prefix, cb_data);
            ++cur;
        }
        return rv;
    }

    depth = 0;
    is_leaf[depth] = IPSET_ROOT_IS_LEAF(ipset);
    to_visit[depth++] = IPSET_ROOT_INDEX(ipset);

    while (depth) {
        if (is_leaf[--depth]) {
            /* handle a leaf node */
            leaf = LEAF_PTR_V4(ipset, to_visit[depth]);
            rv = callback(leaf->ip, leaf->prefix, cb_data);
            if (0 != rv) {
                break;
            }
        } else {
            /* push children onto the stack in reverse order */
            node = NODE_PTR_V4(ipset, to_visit[depth]);
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if (node->child[i]
                    && !NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    is_leaf[depth] = NODEPTR_CHILD_IS_LEAF(node, i);
                    to_visit[depth++] = node->child[i];
                }
            }
        }
    }
    return rv;
}

#if SK_ENABLE_IPV6
static int
ipsetWalkInternalV6(
    const skipset_t    *ipset,
    ipset_walk_v6_fn_t  callback,
    void               *cb_data)
{
    uint8_t is_leaf[IPSET_MAX_DEPTH_V6];
    uint32_t to_visit[IPSET_MAX_DEPTH_V6];
    const ipset_node_v6_t *node;
    const ipset_leaf_v6_t *leaf;
    uint32_t depth;
    uint32_t i;
    int rv = SKIPSET_OK;

    assert(ipset);
    assert(0 == ipset->is_iptree);
    assert(1 == ipset->is_ipv6);
    assert(callback);

    if (IPSET_ISEMPTY(ipset)) {
        return SKIPSET_OK;
    }

    /* use the iterator if we can */
    if (!ipset->is_dirty) {
        uint32_t cur = IPSET_ITER_FIRST_LEAF;
        while (0 == rv && cur < ipset->s.v3->leaves.entry_count) {
            leaf = LEAF_PTR_V6(ipset, cur);
            rv = callback(&leaf->ip, leaf->prefix, cb_data);
            ++cur;
        }
        return rv;
    }

    depth = 0;
    is_leaf[depth] = IPSET_ROOT_IS_LEAF(ipset);
    to_visit[depth++] = IPSET_ROOT_INDEX(ipset);

    while (depth) {
        if (is_leaf[--depth]) {
            /* handle a leaf node */
            leaf = LEAF_PTR_V6(ipset, to_visit[depth]);
            rv = callback(&leaf->ip, leaf->prefix, cb_data);
            if (0 != rv) {
                break;
            }
        } else {
            /* push children onto the stack in reverse order */
            node = NODE_PTR_V6(ipset, to_visit[depth]);
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if (node->child[i]
                    && !NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    is_leaf[depth] = NODEPTR_CHILD_IS_LEAF(node, i);
                    to_visit[depth++] = node->child[i];
                }
            }
        }
    }
    return rv;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetWalkIPTree(ipset, walk_state);
 *
 *    Helper function for skIPSetWalk().
 *
 *    Invoke the 'callback' function on all IPs in the 'ipset'.
 */
static int
ipsetWalkIPTree(
    const skipset_t    *ipset,
    uint32_t            cidr_blocks,
    sk_ipv6policy_t     v6_policy,
    skipset_walk_fn_t   callback,
    void               *cb_data)
{
    skipaddr_t ipaddr;
    int rv = SKIPSET_OK;

    assert(ipset);
    assert(1 == ipset->is_iptree);
    assert(0 == ipset->is_ipv6);

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

        ASSERT_OK(skIPTreeCIDRBlockIteratorBind(&cidr_iter, ipset->s.v2));

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

        ASSERT_OK(skIPTreeIteratorBind(&ip_iter, ipset->s.v2));

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


/*
 *  status = ipsetWalkV4(ipset, walk_state);
 *  status = ipsetWalkV6(ipset, walk_state);
 *
 *    Helper function for skIPSetWalk().
 *
 *    These function visit the leaves in an IPset in order from lowest
 *    to highest.  They will use the iterator if the IPset is clean,
 *    or walk the nodes if the IPset is dirty.
 *
 *    Depending on 'walk_state', the callback is invoked on the CIDR
 *    blocks as stored in the tree or on individual IP addresses.
 *
 *    The callback will be invoked with the IP converted to an
 *    skipaddr_t.  The 'walk_state' determines whether the skipaddr_t
 *    contains an IPv4 or IPv6 representation of the IP address.
 *
 *    The function stops visiting the leaves when the callback returns
 *    a value other than SKIPSET_OK.
 *
 *    Return SKIPSET_OK if the IPset is empty; otherwise, return the
 *    value returned by the final call to the callback.
 *
 *    Do not confuse these helper functions for skIPSetWalk() with the
 *    internal functions for walking over an IPset, which are
 *    ipsetWalkInternalV4() and ipsetWalkInternalV6().
 */
static int
ipsetWalkV4(
    const skipset_t    *ipset,
    const ipset_walk_t *walk_state)
{
    uint8_t is_leaf[IPSET_MAX_DEPTH_V4];
    uint32_t to_visit[IPSET_MAX_DEPTH_V4];
    const ipset_node_v4_t *node;
    const ipset_leaf_v4_t *leaf;
    skipaddr_t ipaddr;
    uint32_t depth;
    uint32_t ipv4;
    uint32_t fin4;
    uint32_t i;
    int rv = 0;

    assert(ipset);
    assert(walk_state);
    assert(walk_state->callback);
    assert(walk_state->v6policy != SK_IPV6POLICY_ONLY);

    depth = 0;
    is_leaf[depth] = IPSET_ROOT_IS_LEAF(ipset);
    to_visit[depth++] = IPSET_ROOT_INDEX(ipset);

    if (walk_state->cidr_blocks) {
        while (depth) {
            if (!is_leaf[--depth]) {
                node = NODE_PTR_V4(ipset, to_visit[depth]);
                for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                    --i;
                    if (node->child[i]
                        && !NODEPTR_CHILD_IS_REPEAT(node, i))
                    {
                        is_leaf[depth] = NODEPTR_CHILD_IS_LEAF(node, i);
                        to_visit[depth++] = node->child[i];
                    }
                }
            } else {
                /* handle a leaf node */
                leaf = LEAF_PTR_V4(ipset, to_visit[depth]);
#if SK_ENABLE_IPV6
                if (SK_IPV6POLICY_FORCE == walk_state->v6policy) {
                    skipaddrSetV6FromUint32(&ipaddr, &leaf->ip);
                    rv = walk_state->callback(&ipaddr, 96 + leaf->prefix,
                                              walk_state->cb_data);
                } else
#endif  /* SK_ENABLE_IPV6 */
                {
                    skipaddrSetV4(&ipaddr, &leaf->ip);
                    rv = walk_state->callback(&ipaddr, leaf->prefix,
                                              walk_state->cb_data);
                }
                if (0 != rv) {
                    break;
                }
            }
        }
        return rv;
    }

    /* Walk over IPset, running the callback function with each
     * individual IP address */
    while (depth) {
        if (!is_leaf[--depth]) {
            node = NODE_PTR_V4(ipset, to_visit[depth]);
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if (node->child[i]
                    && !NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    is_leaf[depth] = NODEPTR_CHILD_IS_LEAF(node, i);
                    to_visit[depth++] = node->child[i];
                }
            }

#if SK_ENABLE_IPV6
        } else if (SK_IPV6POLICY_FORCE == walk_state->v6policy) {
            /* handle a leaf node where IPv6 addresses are to be
             * returned */
            leaf = LEAF_PTR_V4(ipset, to_visit[depth]);
            if (32 == leaf->prefix) {
                skipaddrSetV6FromUint32(&ipaddr, &leaf->ip);
                rv = walk_state->callback(&ipaddr, 128, walk_state->cb_data);
            } else {
                /* else visit each IP in the block */
                ipv4 = leaf->ip;
                fin4 = leaf->ip | (UINT32_MAX >> leaf->prefix);

                do {
                    skipaddrSetV6FromUint32(&ipaddr, &ipv4);
                    rv = walk_state->callback(&ipaddr, 128,
                                              walk_state->cb_data);
                } while ((0 == rv) && (ipv4++ < fin4));
            }
            if (0 != rv) {
                break;
            }
#endif  /* SK_ENABLE_IPV6 */

        } else {
            /* handle a leaf node */
            leaf = LEAF_PTR_V4(ipset, to_visit[depth]);
            if (32 == leaf->prefix) {
                skipaddrSetV4(&ipaddr, &leaf->ip);
                rv = walk_state->callback(&ipaddr, leaf->prefix,
                                          walk_state->cb_data);
            } else {
                /* else visit each IP in the block */
                ipv4 = leaf->ip;
                fin4 = leaf->ip | (UINT32_MAX >> leaf->prefix);
                do {
                    skipaddrSetV4(&ipaddr, &ipv4);
                    rv = walk_state->callback(&ipaddr, 32, walk_state->cb_data);
                } while ((0 == rv) && (ipv4++ < fin4));
            }
            if (0 != rv) {
                break;
            }
        }
    }
    return rv;
}

#if SK_ENABLE_IPV6
static int
ipsetWalkV6(
    const skipset_t    *ipset,
    const ipset_walk_t *walk_state)
{
    uint8_t is_leaf[IPSET_MAX_DEPTH_V6];
    uint32_t to_visit[IPSET_MAX_DEPTH_V6];
    const ipset_node_v6_t *node;
    const ipset_leaf_v6_t *leaf;
    skipaddr_t ipaddr;
    uint32_t depth;
    ipset_ipv6_t ipv6;
    ipset_ipv6_t fin6;
    int i;
    int rv = 0;

    assert(ipset);
    assert(walk_state);
    assert(walk_state->callback);
    assert(walk_state->v6policy != SK_IPV6POLICY_IGNORE);

    if (SK_IPV6POLICY_ASV4 != walk_state->v6policy) {
        /* visit all the nodes in the tree */
        depth = 0;
        is_leaf[depth] = IPSET_ROOT_IS_LEAF(ipset);
        to_visit[depth++] = IPSET_ROOT_INDEX(ipset);
    } else {
        /* limit to the nodes under ::ffff:0:0/96 */
        ipset_find_t find_state;

        ipv6.ip[0] = 0;
        ipv6.ip[1] = UINT64_C(0xffff00000000);

        rv = ipsetFindV6(ipset, &ipv6, 96, &find_state);
        if (SKIPSET_OK != rv && SKIPSET_ERR_SUBSET != rv) {
            /* no IPv4 addresses in the IPset */
            return SKIPSET_OK;
        }
        depth = 0;
        is_leaf[depth] = find_state.node_is_leaf;
        to_visit[depth++] = find_state.node_idx;
        rv = 0;
    }

    if (walk_state->cidr_blocks) {
        while (depth) {
            if (!is_leaf[--depth]) {
                node = NODE_PTR_V6(ipset, to_visit[depth]);
                for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                    --i;
                    if (node->child[i]
                        && !NODEPTR_CHILD_IS_REPEAT(node, i))
                    {
                        is_leaf[depth] = NODEPTR_CHILD_IS_LEAF(node, i);
                        to_visit[depth++] = node->child[i];
                    }
                }
            } else {
                /* handle a leaf node */
                leaf = LEAF_PTR_V6(ipset, to_visit[depth]);
                if (SK_IPV6POLICY_ASV4 == walk_state->v6policy) {
                    IPSET_IPV6_TO_ADDR_V4(&leaf->ip, &ipaddr);
                    rv = walk_state->callback(&ipaddr, leaf->prefix - 96,
                                              walk_state->cb_data);
                } else {
                    IPSET_IPV6_TO_ADDR(&leaf->ip, &ipaddr);
                    rv = walk_state->callback(&ipaddr, leaf->prefix,
                                              walk_state->cb_data);
                }
                if (0 != rv) {
                    break;
                }
            }
        }
        return rv;
    }

    /* Walk over IPset, running the callback function with each
     * individual IP address */
    while (depth) {
        if (!is_leaf[--depth]) {
            node = NODE_PTR_V6(ipset, to_visit[depth]);
            for (i = IPSET_NUM_CHILDREN; i > 0; ) {
                --i;
                if (node->child[i]
                    && !NODEPTR_CHILD_IS_REPEAT(node, i))
                {
                    is_leaf[depth] = NODEPTR_CHILD_IS_LEAF(node, i);
                    to_visit[depth++] = node->child[i];
                }
            }

        } else if (SK_IPV6POLICY_ASV4 == walk_state->v6policy) {
            /* handle a leaf node where IPv4 addresses are to be
             * returned */
            leaf = LEAF_PTR_V6(ipset, to_visit[depth]);
            if (128 == leaf->prefix) {
                IPSET_IPV6_TO_ADDR_V4(&leaf->ip, &ipaddr);
                rv = walk_state->callback(&ipaddr, 32, walk_state->cb_data);
            } else {
                /* else visit each IP in the block; convert CIDR block
                 * to current IP (ipv4) and final IP (fin4) */
                uint32_t ipv4;
                uint32_t fin4;

                assert(96 <= leaf->prefix);
                assert(0 == leaf->ip.ip[0]);
                assert(UINT64_C(0x0000ffff00000000)
                       == (UINT64_C(0xffffffff00000000) & leaf->ip.ip[1]));
                ipv4 = (uint32_t)(UINT64_C(0x00000000ffffffff)
                                  & leaf->ip.ip[1]);
                fin4 = ipv4 | (UINT32_MAX >> (leaf->prefix - 96));

                do {
                    skipaddrSetV4(&ipaddr, &ipv4);
                    rv = walk_state->callback(&ipaddr, 32,
                                              walk_state->cb_data);
                } while ((0 == rv) && (ipv4++ < fin4));
            }
            if (0 != rv) {
                break;
            }

        } else {
            /* handle a leaf node */
            leaf = LEAF_PTR_V6(ipset, to_visit[depth]);
            if (128 == leaf->prefix) {
                IPSET_IPV6_TO_ADDR(&leaf->ip, &ipaddr);
                rv = walk_state->callback(&ipaddr, leaf->prefix,
                                          walk_state->cb_data);
            } else {
                /* else visit each IP in the block; convert CIDR block
                 * to current IP (ipv6) and final IP (fin6) */
                if (leaf->prefix > 64) {
                    ipv6.ip[0] = fin6.ip[0] = leaf->ip.ip[0];
                    ipv6.ip[1] = leaf->ip.ip[1];
                    fin6.ip[1] = (leaf->ip.ip[1]
                                  | (UINT64_MAX >> (leaf->prefix - 64)));
                } else if (leaf->prefix == 64) {
                    ipv6.ip[0] = fin6.ip[0] = leaf->ip.ip[0];
                    ipv6.ip[1] = fin6.ip[1] = UINT64_MAX;
                } else {
                    ipv6.ip[0] = leaf->ip.ip[0];
                    fin6.ip[0] = (leaf->ip.ip[0]
                                  | (UINT64_MAX >> leaf->prefix));
                    ipv6.ip[1] = fin6.ip[1] = UINT64_MAX;
                }

                for (;;) {
                    IPSET_IPV6_TO_ADDR(&ipv6, &ipaddr);
                    rv = walk_state->callback(&ipaddr,128,walk_state->cb_data);
                    if (rv) {
                        return rv;
                    }
                    if (ipv6.ip[1] < fin6.ip[1]) {
                        ++ipv6.ip[1];
                    } else if (ipv6.ip[0] < fin6.ip[0]) {
                        if (ipv6.ip[1] == UINT64_MAX) {
                            ++ipv6.ip[0];
                            ipv6.ip[1] = 0;
                        } else {
                            ++ipv6.ip[1];
                        }
                    } else {
                        break;
                    }
                }
            }
        }
    }

    return rv;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetWriteCidrbmapFromIPTree(ipset, stream);
 *
 *    Helper function for skIPSetWrite() via ipsetWriteCidrbmap().
 *
 *    Write an IPset from the SiLK-2 IPTree data structure to 'stream'
 *    in the IPSET_REC_VERSION_CIDR_BMAP format.
 *
 *    See '#define IPSET_REC_VERSION_CIDR_BMAP' for description of the
 *    file format.
 */
static int
ipsetWriteCidrbmapFromIPTree(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    uint8_t write_buf[sizeof(uint32_t) + sizeof(uint8_t)];
    skIPNode_t *slash16;
    uint32_t slash24;
    uint32_t i;
    uint32_t j;
    ssize_t rv;
    struct build_cidr_st {
        /* starting ip for this netblock */
        uint32_t start;
        /* number of trailing 0s; used to set CIDR mask */
        uint32_t trail_zero;
        /* maximum number of /24s this block can hold */
        uint32_t max_block;
        /* number of the max_block we have seen so far */
        uint32_t count;
    } build_cidr;

#define WRITE_BUILD_CIDR                                                \
    while (build_cidr.count) {                                          \
        --build_cidr.trail_zero;                                        \
        if (build_cidr.count >= (1u << (build_cidr.trail_zero - 8))) {  \
            *(uint32_t*)write_buf = build_cidr.start;                   \
            write_buf[sizeof(uint32_t)]                                 \
                = (uint8_t)(32 - build_cidr.trail_zero);                \
            rv = skStreamWrite(stream, write_buf, sizeof(write_buf));   \
            if (rv != sizeof(write_buf)) {                              \
                return SKIPSET_ERR_FILEIO;                              \
            }                                                           \
            build_cidr.count -= (1 << (build_cidr.trail_zero - 8));     \
            build_cidr.start |= (0x100 << (build_cidr.trail_zero - 8)); \
        }                                                               \
    }

    /* sanity check input */
    assert(ipset);
    assert(stream);
    if (ipset->is_dirty || !ipset->is_iptree || ipset->is_ipv6) {
        skAbort();
    }
    hdr = skStreamGetSilkHeader(stream);
    if (skHeaderGetRecordVersion(hdr) != IPSET_REC_VERSION_CIDR_BMAP) {
        skAbort();
    }

    memset(&build_cidr, 0, sizeof(build_cidr));

    for (i = 0; i < SKIP_BBLOCK_COUNT; ++i) {
        /* slash16 represents a /16 */
        slash16 = ipset->s.v2->nodes[i];
        if (NULL == slash16) {
            /* no data in this /16; write any CIDR range that was
             * being built */
            WRITE_BUILD_CIDR;
            continue;
        }

        /* There is data in this /16, walk over the addressBlocks by
         * 8s, which is equivalent to visiting each /24. */
        for (j = 0; j < SKIP_BBLOCK_SIZE; j += IPTREE_WORDS_PER_SLASH24) {

            if (0 == memcmp(slash16->addressBlock + j,
                            bmap256_zero, sizeof(bmap256_zero)))
            {
                /* no data in this /24; write existing CIDR block */
                WRITE_BUILD_CIDR;
                continue;
            }
            if (0 == memcmp(slash16->addressBlock + j,
                            bmap256_full, sizeof(bmap256_full)))
            {
                /* this /24 is full; if CIDR block is being built,
                 * this block must be contiguous with it */
                if (build_cidr.count) {
                    ++build_cidr.count;
                    if (build_cidr.count == build_cidr.max_block) {
                        /* cidr block is at its maximum size */
                        *(uint32_t*)write_buf = build_cidr.start;
                        write_buf[sizeof(uint32_t)]
                            = (uint8_t)(32 - build_cidr.trail_zero);
                        rv = skStreamWrite(stream,write_buf,sizeof(write_buf));
                        if (rv != sizeof(write_buf)) {
                            return SKIPSET_ERR_FILEIO;
                        }
                        build_cidr.count = 0;
                    }
                    continue;
                }

                /* no existing CIDR block to join; start a new one */
                build_cidr.start = ((i << 16) | (j << 5)) & 0xFFFFFF00;
                if (build_cidr.start & 0x100) {
                    /* the third octet is odd, so this block cannot be
                     * combined with another */
                    *(uint32_t*)write_buf = build_cidr.start;
                    write_buf[sizeof(uint32_t)] = 24;
                    rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
                    if (rv != sizeof(write_buf)) {
                        return SKIPSET_ERR_FILEIO;
                    }
                    continue;
                }

                /* compute the maximum number of blocks that can be
                 * joined with this one by computing the number of
                 * trailing 0s on the IP---we know the least
                 * significant 8 bits are already 0. */
                slash24 = build_cidr.start >> 8;
                if (0 == slash24) {
                    build_cidr.trail_zero = 32;
                } else {
                    build_cidr.trail_zero = 9;
                    if ((slash24 & 0xFFFF) == 0) {
                        slash24 >>= 16;
                        build_cidr.trail_zero += 16;
                    }
                    if ((slash24 & 0xFF) == 0) {
                        slash24 >>= 8;
                        build_cidr.trail_zero += 8;
                    }
                    if ((slash24 & 0xF) == 0) {
                        slash24 >>= 4;
                        build_cidr.trail_zero += 4;
                    }
                    if ((slash24 & 0x3) == 0) {
                        slash24 >>= 2;
                        build_cidr.trail_zero += 2;
                    }
                    build_cidr.trail_zero -= (slash24 & 1);
                }
                build_cidr.max_block = (1 << (build_cidr.trail_zero - 8));
                build_cidr.count = 1;
                continue;
            }

            /* there is some data in this /24.  First, handle any CIDR
             * range that was being build.  Then, write this block as
             * a base address and bitmap. */
            WRITE_BUILD_CIDR;

            /* write the IP and the 'bitmap-follows' value  */
            *(uint32_t*)write_buf = (((i << 16) | (j << 5)) & 0xFFFFFF00);
            write_buf[sizeof(uint32_t)] = SET_CIDR_BMAP_256;
            rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
            if (rv != sizeof(write_buf)) {
                return SKIPSET_ERR_FILEIO;
            }
            /* write the complete /24: 8 uint32_t's */
            rv = skStreamWrite(stream, slash16->addressBlock + j,
                               sizeof(bmap256_zero));
            if (rv != sizeof(bmap256_zero)) {
                return SKIPSET_ERR_FILEIO;
            }
        }
    }

    rv = skStreamFlush(stream);
    if (rv) {
        return SKIPSET_ERR_FILEIO;
    }

    return SKIPSET_OK;
}


/*
 *  status = ipsetWriteCidrbmapFromRadixV4(&ipset, stream, hdr);
 *  status = ipsetWriteCidrbmapFromRadixV6(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetWrite() via ipsetWriteCidrbmap().
 *
 *    Write an IPset from the radix-tree IPset data structure to
 *    'stream' in the IPSET_REC_VERSION_CIDR_BMAP format.
 *
 *    See '#define IPSET_REC_VERSION_CIDR_BMAP' for description of the
 *    file format.
 */
static int
ipsetWriteCidrbmapFromRadixV4(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    skipset_iterator_t iter;
    struct state_st {
        /* starting ip for this netblock, or an unmasked IP if
         * 'is_first' is set. */
        uint32_t  base_ip;
        /* the 256-bit bitmap */
        uint32_t bmap[IPTREE_WORDS_PER_SLASH24];
        /* the prefix for 'base_ip' if 'is_first' is set */
        uint8_t   prefix;
        /* if set, this structure contains data */
        uint8_t   is_dirty;
        /* if set, base_ip holds a single, unmasked IP, and prefix is
         * its prefix */
        uint8_t   is_first;
    } state;
    /* the bitmap holds 256 bits or a /24 */
    const uint32_t bmap_prefix = 24;
    /* each uint32_t in the bitmap holds a /27 */
    const uint32_t word_prefix = 27;
    uint8_t write_buf[sizeof(uint32_t) + 1];
    uint32_t ipv4;
    skipaddr_t ipaddr;
    uint32_t prefix;
    uint32_t buf_idx;
    ssize_t rv;

    /* sanity check input */
    assert(ipset);
    assert(stream);
    if (ipset->is_dirty || ipset->is_iptree || ipset->is_ipv6) {
        skAbort();
    }
    hdr = skStreamGetSilkHeader(stream);
    if (skHeaderGetRecordVersion(hdr) != IPSET_REC_VERSION_CIDR_BMAP) {
        skAbort();
    }

    memset(&state, 0, sizeof(state));

    skIPSetIteratorBind(&iter, ipset, 1, SK_IPV6POLICY_ASV4);
    while (skIPSetIteratorNext(&iter, &ipaddr, &prefix) == SK_ITERATOR_OK){
        ipv4 = skipaddrGetV4(&ipaddr);
        if (state.is_dirty && (prefix <= bmap_prefix
                               || (state.base_ip ^ ipv4) > 0xFF))
        {
            /* 'ipv4' is not in the same /24 as 'state'; write the IP
             * or bitmap currently in 'state' */
            if (state.is_first) {
                /* write the single ip and prefix */
                *(uint32_t*)write_buf = state.base_ip;
                write_buf[sizeof(uint32_t)] = (uint8_t)state.prefix;
                rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
                if (rv != sizeof(write_buf)) {
                    return SKIPSET_ERR_FILEIO;
                }
                state.is_first = 0;
            } else {
                /* write base ip, 'bitmap-follows' value for the
                 * prefix, and the bitmap */
                *(uint32_t*)write_buf = state.base_ip;
                write_buf[sizeof(uint32_t)] = SET_CIDR_BMAP_256;
                rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
                if (rv != sizeof(write_buf)) {
                    return SKIPSET_ERR_FILEIO;
                }
                rv = skStreamWrite(stream, state.bmap, sizeof(state.bmap));
                if (rv != sizeof(state.bmap)) {
                    return SKIPSET_ERR_FILEIO;
                }
            }
            state.is_dirty = 0;
        }

        if (prefix <= bmap_prefix) {
            /* prefix is larger than the bitmap; write the block as an
             * IP and prefix */
            *(uint32_t*)write_buf = ipv4;
            write_buf[sizeof(uint32_t)] = (uint8_t)prefix;
            rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
            if (rv != sizeof(write_buf)) {
                return SKIPSET_ERR_FILEIO;
            }
            continue;
        }
        if (!state.is_dirty) {
            /* 'state' is empty; initialize 'state' with this IP and
             * look for more IPs in this /24. */
            state.is_dirty = 1;
            state.base_ip = ipv4;
            state.prefix = (uint8_t)prefix;
            state.is_first = 1;
            continue;
        }
        if (state.is_first) {
            /* found a second IP in this /24; convert 'state' to hold
             * a bitmap */
            state.is_first = 0;
            memset(state.bmap, 0, sizeof(state.bmap));

            buf_idx = ((state.base_ip & 0xFF) >> 5);
            if (state.prefix <= word_prefix) {
                /* prefix spans one or more uint32_t's */
                memset(&(state.bmap[buf_idx]), 0xff,
                       (sizeof(uint32_t) << (word_prefix - state.prefix)));
            } else {
                state.bmap[buf_idx]
                    |= (((1 << (1 << (32 - state.prefix))) - 1)
                        << (state.base_ip & 0x1f));
            }
            /* mask base_ip as a /24 */
            state.base_ip = state.base_ip & 0xFFFFFF00;
        }
        assert((ipv4 & 0xFFFFFF00) == state.base_ip);

        /* add the current IP/prefix to the bitmap */
        buf_idx = ((ipv4 & 0xFF) >> 5);
        if (prefix <= word_prefix) {
            /* set several uint32_t's */
            memset(&(state.bmap[buf_idx]), 0xff,
                   (sizeof(uint32_t) << (word_prefix - prefix)));
        } else {
            state.bmap[buf_idx] |= (((1 << (1 << (32 - prefix))) - 1)
                                    << (ipv4 & 0x1f));
        }
    }

    /* write any IP or bitmap still in 'state' */
    if (state.is_dirty) {
        if (state.is_first) {
            *(uint32_t*)write_buf = state.base_ip;
            write_buf[sizeof(uint32_t)] = (uint8_t)state.prefix;
            rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
            if (rv != sizeof(write_buf)) {
                return SKIPSET_ERR_FILEIO;
            }
        } else {
            *(uint32_t*)write_buf = state.base_ip;
            write_buf[sizeof(uint32_t)] = SET_CIDR_BMAP_256;
            rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
            if (rv != sizeof(write_buf)) {
                return SKIPSET_ERR_FILEIO;
            }
            rv = skStreamWrite(stream, state.bmap, sizeof(state.bmap));
            if (rv != sizeof(state.bmap)) {
                return SKIPSET_ERR_FILEIO;
            }
        }
    }

    return SKIPSET_OK;
}

#if SK_ENABLE_IPV6
static int
ipsetWriteCidrbmapFromRadixV6(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    skipset_iterator_t iter;
    struct state_st {
        /* starting ip for this netblock, or an unmasked IP if
         * 'is_first' is set. */
        ipset_ipv6_t  base_ip;
        /* the 256-bit bitmap */
        uint32_t bmap[IPTREE_WORDS_PER_SLASH24];
        /* the prefix for 'base_ip' if 'is_first' is set */
        uint8_t   prefix;
        /* if set, this structure contains data */
        uint8_t   is_dirty;
        /* if set, base_ip holds a single, unmasked IP, and prefix is
         * its prefix */
        uint8_t   is_first;
    } state;
    /* the bitmap holds 256 bits or a /120 */
    const uint32_t bmap_prefix = 120;
    /* each uint32_t in the bitmap holds a /123 */
    const uint32_t word_prefix = 123;
    uint8_t write_buf[IPSET_LEN_V6 + sizeof(uint8_t)];
    ipset_ipv6_t ipv6;
    skipaddr_t ipaddr;
    uint32_t prefix;
    uint32_t buf_idx;
    ssize_t rv;

    /* sanity check input */
    assert(ipset);
    assert(stream);
    if (ipset->is_dirty || ipset->is_iptree || !ipset->is_ipv6) {
        skAbort();
    }
    hdr = skStreamGetSilkHeader(stream);
    if (skHeaderGetRecordVersion(hdr) != IPSET_REC_VERSION_CIDR_BMAP) {
        skAbort();
    }

    memset(&state, 0, sizeof(state));

    skIPSetIteratorBind(&iter, ipset, 1, SK_IPV6POLICY_FORCE);
    while (skIPSetIteratorNext(&iter, &ipaddr, &prefix) == SK_ITERATOR_OK) {
        IPSET_IPV6_FROM_ADDRV6(&ipv6, &ipaddr);
        if (state.is_dirty
            && (prefix <= bmap_prefix
                || (state.base_ip.ip[0] != ipv6.ip[0])
                || ((state.base_ip.ip[1] ^ ipv6.ip[1]) > 0xFF)))
        {
            /* 'ipv6' is not in the same /120 as 'state'; write the IP
             * or bitmap currently in 'state' */
            if (state.is_first) {
                /* write the single ip and prefix */
                IPSET_IPV6_TO_ARRAY(&state.base_ip, write_buf);
                write_buf[IPSET_LEN_V6] = (uint8_t)state.prefix;
                rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
                if (rv != sizeof(write_buf)) {
                    return SKIPSET_ERR_FILEIO;
                }
                state.is_first = 0;
            } else {
                /* write base ip, 'bitmap-follows' value for the
                 * prefix, and the bitmap */
                IPSET_IPV6_TO_ARRAY(&state.base_ip, write_buf);
                write_buf[IPSET_LEN_V6] = SET_CIDR_BMAP_256;
                rv = skStreamWrite(stream, &write_buf, sizeof(write_buf));
                if (rv != sizeof(write_buf)) {
                    return SKIPSET_ERR_FILEIO;
                }
                rv = skStreamWrite(stream, state.bmap, sizeof(state.bmap));
                if (rv != sizeof(state.bmap)) {
                    return SKIPSET_ERR_FILEIO;
                }
            }
            state.is_dirty = 0;
        }

        if (prefix <= bmap_prefix) {
            /* prefix is larger than the bitmap; write the block as an
             * IP and prefix */
            IPSET_IPV6_TO_ARRAY(&ipv6, write_buf);
            write_buf[IPSET_LEN_V6] = (uint8_t)prefix;
            rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
            if (rv != sizeof(write_buf)) {
                return SKIPSET_ERR_FILEIO;
            }
            continue;
        }
        if (!state.is_dirty) {
            /* 'state' is empty; initialize 'state' with this IP and
             * look for more IPs in this /120. */
            state.is_dirty = 1;
            IPSET_IPV6_COPY(&state.base_ip, &ipv6);
            state.prefix = (uint8_t)prefix;
            state.is_first = 1;
            continue;
        }
        if (state.is_first) {
            /* found a second IP in this /120; convert 'state' to hold
             * a bitmap */
            state.is_first = 0;
            memset(state.bmap, 0, sizeof(state.bmap));

            buf_idx = ((state.base_ip.ip[1] & 0xFF) >> 5);
            if (state.prefix <= word_prefix) {
                /* prefix spans one or more uint32_t's */
                memset(&(state.bmap[buf_idx]), 0xff,
                       (sizeof(uint32_t) << (word_prefix-state.prefix)));
            } else {
                state.bmap[buf_idx]
                    |= (((1 << (1 << (128 - state.prefix))) - 1)
                        << (state.base_ip.ip[1] & 0x1f));
            }
            /* mask base_ip as a /120 */
            state.base_ip.ip[1] = (state.base_ip.ip[1] & ~UINT64_C(0xFF));
        }
        assert(ipv6.ip[0] == state.base_ip.ip[0]);
        assert((ipv6.ip[1] & ~UINT64_C(0xFF)) == state.base_ip.ip[1]);

        /* add the current IP/prefix to the bitmap */
        buf_idx = ((ipv6.ip[1] & 0xFF) >> 5);
        if (prefix <= word_prefix) {
            /* set several uint32_t's */
            memset(&(state.bmap[buf_idx]), 0xff,
                   (sizeof(uint32_t) << (word_prefix - prefix)));
        } else {
            state.bmap[buf_idx] |= (((1 << (1 << (128 - prefix))) - 1)
                                    << (ipv6.ip[1] & 0x1f));
        }
    }

    /* write any IP or bitmap still in 'state' */
    if (state.is_dirty) {
        if (state.is_first) {
            IPSET_IPV6_TO_ARRAY(&state.base_ip, write_buf);
            write_buf[IPSET_LEN_V6] = (uint8_t)state.prefix;
            rv = skStreamWrite(stream, write_buf, sizeof(write_buf));
            if (rv != sizeof(write_buf)) {
                return SKIPSET_ERR_FILEIO;
            }
        } else {
            IPSET_IPV6_TO_ARRAY(&state.base_ip, write_buf);
            write_buf[IPSET_LEN_V6] = SET_CIDR_BMAP_256;
            rv = skStreamWrite(stream, &write_buf, sizeof(write_buf));
            if (rv != sizeof(write_buf)) {
                return SKIPSET_ERR_FILEIO;
            }
            rv = skStreamWrite(stream, state.bmap, sizeof(state.bmap));
            if (rv != sizeof(state.bmap)) {
                return SKIPSET_ERR_FILEIO;
            }
        }
    }

    return SKIPSET_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetWriteCidrbmap(&ipset, stream, hdr);
 *
 *    Helper function for skIPSetWrite().
 *
 *    Write an IPset to 'stream' in the IPSET_REC_VERSION_CIDR_BMAP
 *    format.
 *
 *    This function writes the header then calls another helper
 *    function depending on the data structure being used.
 */
static int
ipsetWriteCidrbmap(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    int rv;

    /* sanity check input */
    assert(ipset);
    assert(stream);
    if (ipset->is_dirty) {
        skAbort();
    }
    hdr = skStreamGetSilkHeader(stream);
    if (skHeaderGetRecordVersion(hdr) != IPSET_REC_VERSION_CIDR_BMAP) {
        skAbort();
    }

    /* Add the appropriate header */
    rv = skHeaderAddIPSet(hdr, 0, 0,
                          ipset->is_ipv6 ? IPSET_LEN_V6 : sizeof(uint32_t),
                          0, 0, 0);
    if (rv) {
        skAppPrintErr("%s", skHeaderStrerror(rv));
        return SKIPSET_ERR_FILEIO;
    }
    rv = skStreamWriteSilkHeader(stream);
    if (rv) {
        return SKIPSET_ERR_FILEIO;
    }

    if (ipset->is_iptree) {
        return ipsetWriteCidrbmapFromIPTree(ipset, stream);
    }
#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        return ipsetWriteCidrbmapFromRadixV6(ipset, stream);
    }
#endif
    return ipsetWriteCidrbmapFromRadixV4(ipset, stream);
}


/*
 *  status = ipsetWriteLegacyFromIPTree(ipset, stream);
 *
 *    Helper function for skIPSetWrite() via ipsetWriteLegacy().
 *
 *    Write an IPset from the SiLK-2 IPTree data structure to 'stream'
 *    in the IPSET_REC_VERSION_LEGACY format.
 *
 *    See '#define IPSET_REC_VERSION_LEGACY' for description of the
 *    file format.
 */
static int
ipsetWriteLegacyFromIPTree(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    skIPNode_t *slash16;
    uint32_t slash24;
    uint32_t i;
    uint32_t j;
    ssize_t rv;

    /* sanity check input */
    assert(ipset);
    assert(stream);
    if (ipset->is_dirty || !ipset->is_iptree || ipset->is_ipv6) {
        skAbort();
    }
    hdr = skStreamGetSilkHeader(stream);
    if (skHeaderGetRecordVersion(hdr) != IPSET_REC_VERSION_LEGACY) {
        skAbort();
    }

    for (i = 0; i < SKIP_BBLOCK_COUNT; i++) {
        /* slash16 represents a /16 */
        slash16 = ipset->s.v2->nodes[i];
        if (NULL == slash16) {
            continue;
        }

        /* There is data in this /16, walk over the addressBlocks by
         * 8s, which is equivalent to visiting each /24. */
        for (j = 0; j < SKIP_BBLOCK_SIZE; j += IPTREE_WORDS_PER_SLASH24) {
            if (memcmp(slash16->addressBlock + j,
                       bmap256_zero, sizeof(bmap256_zero)))
            {
                /* there is data in this /24; write the base address. */
                slash24 = ((i << 16) | (j << 5)) & 0xFFFFFF00;
                rv = skStreamWrite(stream, &slash24, sizeof(uint32_t));
                if (rv != sizeof(uint32_t)) {
                    return SKIPSET_ERR_FILEIO;
                }
                /* write the complete /24: 8 uint32_t's */
                rv = skStreamWrite(stream, slash16->addressBlock + j,
                                   sizeof(bmap256_zero));
                if (rv != sizeof(bmap256_zero)) {
                    return SKIPSET_ERR_FILEIO;
                }
            }
        }
    }

    rv = skStreamFlush(stream);
    if (rv) {
        return SKIPSET_ERR_FILEIO;
    }

    return SKIPSET_OK;
}


/*
 *  status = ipsetWriteLegacyFromRadixCallbackV4(ipv4, prefix, &state);
 *  status = ipsetWriteLegacyFromRadixCallback(ipaddr, prefix, &state);
 *
 *    Callback function used by ipsetWriteLegacyFromRadix().
 *
 *    This function is the callback invoked by skIPSetWalk().  When
 *    the current 'ipaddr' and 'prefix' are part of the same /24
 *    contained in 'state'->buffer, the IPs are added to that buffer.
 *    Otherwise, the buffer is printed if it contains data, and then
 *    the buffer is initialized with new data.
 */
static int
ipsetWriteLegacyFromRadixCallbackV4(
    uint32_t            ipv4,
    uint32_t            prefix,
    void               *v_state)
{
    ipset_write_silk2_t *state = (ipset_write_silk2_t*)v_state;
    uint32_t slash24;
    uint32_t buf_idx;
    int i;
    ssize_t rv;

    /* get the /24 that holds this IP */
    slash24 = ipv4 & 0xFFFFFF00;

    if (prefix <= 24) {
        /* write the buffer if it contains data */
        if (state->buffer_is_dirty) {
            rv = skStreamWrite(state->stream, state->buffer,
                               sizeof(state->buffer));
            if (rv != sizeof(state->buffer)) {
                return SKIPSET_ERR_FILEIO;
            }
            state->buffer_is_dirty = 0;
        }

        /* initialize the buffer to all-1s */
        memset(state->buffer, 0xff, sizeof(state->buffer));

        /* loop over all /24s in this block and print each one */
        for (i = 0; i < (1 << (24 - prefix)); ++i, slash24 += 256) {
            state->buffer[0] = slash24;

            rv = skStreamWrite(state->stream, state->buffer,
                               sizeof(state->buffer));
            if (rv != sizeof(state->buffer)) {
                return SKIPSET_ERR_FILEIO;
            }
        }

        return SKIPSET_OK;
    }

    if (!state->buffer_is_dirty) {
        /* initialize the block to hold this /24 */
        memset(state->buffer, 0, sizeof(state->buffer));
        state->buffer[0] = slash24;
        state->buffer_is_dirty = 1;
    } else if (state->buffer[0] != slash24) {
        /* the /24s are different---print the block and reset it */
        rv = skStreamWrite(state->stream, state->buffer,sizeof(state->buffer));
        if (rv != sizeof(state->buffer)) {
            return SKIPSET_ERR_FILEIO;
        }
        memset(state->buffer, 0, sizeof(state->buffer));
        state->buffer[0] = slash24;
    }
    /* else data is in the same /24 */

    /* add our IPs to the bitmap */
    buf_idx = 1 + ((ipv4 & 0xFF) >> 5);

    if (prefix <= 27) {
        /* set several uint32_t's */
        memset(&(state->buffer[buf_idx]), 0xff,
               ((1 << (27 - prefix)) * sizeof(uint32_t)));
    } else {
        state->buffer[buf_idx] |= ((UINT32_MAX >> (32 - (1 << (32 - prefix))))
                                   << (ipv4 & 0x1f & (0x1f << (32 - prefix))));
    }

    return SKIPSET_OK;
}

#if SK_ENABLE_IPV6
static int
ipsetWriteLegacyFromRadixCallback(
    skipaddr_t         *ipaddr,
    uint32_t            prefix,
    void               *v_state)
{
    uint32_t ipv4;

    /* get the IPv4 address */
    if (skipaddrGetAsV4(ipaddr, &ipv4)) {
        return SKIPSET_ERR_IPV6;
    }
    return ipsetWriteLegacyFromRadixCallbackV4(ipv4, prefix, v_state);
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  status = ipsetWriteLegacyFromRadix(ipset, stream);
 *
 *    Helper function for skIPSetWrite() via ipsetWriteLegacy().
 *
 *    Write an IPset from the radix-tree IPset data structure to
 *    'stream' in the IPSET_REC_VERSION_LEGACY format.
 *
 *    See '#define IPSET_REC_VERSION_LEGACY' for description of the
 *    file format.
 */
static int
ipsetWriteLegacyFromRadix(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    ipset_write_silk2_t write_state;
    ssize_t rv;

    /* sanity check input */
    assert(ipset);
    assert(stream);
    if (ipset->is_dirty || ipset->is_iptree || skIPSetContainsV6(ipset)) {
        skAbort();
    }
    hdr = skStreamGetSilkHeader(stream);
    if (skHeaderGetRecordVersion(hdr) != IPSET_REC_VERSION_LEGACY) {
        skAbort();
    }

    memset(&write_state, 0, sizeof(write_state));
    write_state.stream = stream;

#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        rv = skIPSetWalk(ipset, 1, SK_IPV6POLICY_ASV4,
                         &ipsetWriteLegacyFromRadixCallback,
                         (void*)&write_state);
    } else
#endif
    {
        rv = ipsetWalkInternalV4(ipset, ipsetWriteLegacyFromRadixCallbackV4,
                                 (void*)&write_state);
    }
    if (rv != 0) {
        return rv;
    }

    if (write_state.buffer_is_dirty) {
        rv = skStreamWrite(stream, write_state.buffer,
                           sizeof(write_state.buffer));
        if (rv != sizeof(write_state.buffer)) {
            return SKIPSET_ERR_FILEIO;
        }
    }

    rv = skStreamFlush(stream);
    if (rv) {
        return SKIPSET_ERR_FILEIO;
    }

    return SKIPSET_OK;
}


/*
 *  status = ipsetWriteLegacy(&ipset, stream);
 *
 *    Helper function for skIPSetWrite().
 *
 *    Write an IPset to 'stream' in the IPSET_REC_VERSION_LEGACY
 *    format.
 *
 *    This function writes the header then calls another helper
 *    function depending on the data structure being used.
 */
static int
ipsetWriteLegacy(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    int rv;

    /* sanity check input */
    assert(ipset);
    assert(stream);
    if (ipset->is_dirty || skIPSetContainsV6(ipset)) {
        skAbort();
    }
    hdr = skStreamGetSilkHeader(stream);
    if (skHeaderGetRecordVersion(hdr) != IPSET_REC_VERSION_LEGACY) {
        skAbort();
    }

    rv = skStreamWriteSilkHeader(stream);
    if (rv) {
        return SKIPSET_ERR_FILEIO;
    }

    if (ipset->is_iptree) {
        return ipsetWriteLegacyFromIPTree(ipset, stream);
    }
    return ipsetWriteLegacyFromRadix(ipset, stream);
}


/*
 *  status = ipsetWriteRadix(ipset, stream);
 *
 *    Helper function for skIPSetWrite().
 *
 *    Write an IPset to 'stream' in the IPSET_REC_VERSION_RADIX
 *    format.
 *
 *    See '#define IPSET_REC_VERSION_RADIX' for description of the
 *    file format.
 */
static int
ipsetWriteRadix(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    skipset_t *set3 = NULL;
    ssize_t rv;

    /* sanity check input */
    assert(ipset);
    assert(stream);
    if (ipset->is_dirty) {
        skAbort();
    }
    hdr = skStreamGetSilkHeader(stream);
    if (skHeaderGetRecordVersion(hdr) != IPSET_REC_VERSION_RADIX) {
        skAbort();
    }

    /* we must convert the IPTree to a Radix-Tree when writing an
     * IPTree-based IPset to the IPSET_REC_VERSION_RADIX format */
    if (ipset->is_iptree) {
        skIPTreeCIDRBlockIterator_t cidr_iter;
        skIPTreeCIDRBlock_t cidr;

        /* create a Radix-based IPv4 IPset */
        rv = ipsetCreate(&set3, 0, 1);
        if (rv) {
            goto END;
        }

        /* add the elements from 'ipset' to 'set3' */
        ASSERT_OK(skIPTreeCIDRBlockIteratorBind(&cidr_iter, ipset->s.v2));
        while ((rv = skIPTreeCIDRBlockIteratorNext(&cidr, &cidr_iter))
               == SK_ITERATOR_OK)
        {
            rv = ipsetInsertAddressV4(set3, cidr.addr, cidr.mask, NULL);
            if (rv) {
                goto END;
            }
        }
        rv = skIPSetClean(set3);
        if (rv) {
            goto END;
        }

        ipset = set3;
    }

    if (ipset->is_iptree) {
        skAbort();
    }

    /* Add the appropriate header */
    rv = skHeaderAddIPSet(hdr, IPSET_NUM_CHILDREN,
                          ipset->s.v3->leaves.entry_count,
                          ipset->s.v3->leaves.entry_size,
                          ipset->s.v3->nodes.entry_count,
                          ipset->s.v3->nodes.entry_size,
                          IPSET_ROOT_INDEX(ipset));
    if (rv) {
        skAppPrintErr("%s", skHeaderStrerror(rv));
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }
    rv = skStreamWriteSilkHeader(stream);
    if (rv) {
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    /* write the arrays */
    if (ipset->s.v3->nodes.entry_count) {
        rv = skStreamWrite(stream, ipset->s.v3->nodes.buf,
                           ((size_t)ipset->s.v3->nodes.entry_size
                            * ipset->s.v3->nodes.entry_count));
        if ((size_t)rv != ((size_t)ipset->s.v3->nodes.entry_size
                           * ipset->s.v3->nodes.entry_count))
        {
            rv = SKIPSET_ERR_FILEIO;
            goto END;
        }
    }
    if (ipset->s.v3->leaves.entry_count) {
        rv = skStreamWrite(stream, ipset->s.v3->leaves.buf,
                           ((size_t)ipset->s.v3->leaves.entry_size
                            * ipset->s.v3->leaves.entry_count));
        if ((size_t)rv != ((size_t)ipset->s.v3->leaves.entry_size
                           * ipset->s.v3->leaves.entry_count))
        {
            rv = SKIPSET_ERR_FILEIO;
            goto END;
        }
    }

    rv = skStreamFlush(stream);
    if (rv) {
        rv = SKIPSET_ERR_FILEIO;
        goto END;
    }

    /* success */
    rv = SKIPSET_OK;

  END:
    skIPSetDestroy(&set3);
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

    if (ipset->is_iptree) {
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

        return skIPTreeCheckAddress(ipset->s.v2, ipv4);
    }

#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        ipset_ipv6_t ipv6;

        IPSET_IPV6_FROM_ADDRV4(&ipv6, ipaddr);
        return (ipsetFindV6(ipset, &ipv6, 128, NULL) == SKIPSET_OK);
    }
    /* else IPset is IPv4 */

    if (skipaddrIsV6(ipaddr)) {
        if (skipaddrGetAsV4(ipaddr, &ipv4)) {
            /* conversion failed; this IP is not in the IPSet */
            return 0;
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        ipv4 = skipaddrGetV4(ipaddr);
    }

    return (ipsetFindV4(ipset, ipv4, 32, NULL) == SKIPSET_OK);
}


/* Return true if 'ipset1' and 'ipset2' have any IPs in common. */
int
skIPSetCheckIPSet(
    const skipset_t    *ipset1,
    const skipset_t    *ipset2)
{
    const skipset_t *walk_set = NULL;
    const skipset_t *search_set = NULL;
    uint32_t leaf_count1;
    uint32_t leaf_count2;
    int rv = SKIPSET_OK;

    /* check input */
    if (!ipset1 || !ipset2) {
        return 0;
    }

    if (ipset1->is_iptree) {
        if (ipset2->is_iptree) {
            /* both are SiLK-2 IPTrees */
            return skIPTreeCheckIntersectIPTree(ipset1->s.v2, ipset2->s.v2);
        }

        walk_set = ipset1;
        search_set = ipset2;

    } else if (ipset2->is_iptree) {
        walk_set = ipset2;
        search_set = ipset1;

    } else {
        /* handle trivial case */
        if (IPSET_ISEMPTY(ipset1) || IPSET_ISEMPTY(ipset2)) {
            return 0;
        }

        /*
         *  The current implementation walks over the IPset that has the
         *  fewest leaves asking whether those IPs are in the other IPset.
         *  If we could guarantee that at least one IPset was clean, we
         *  could implement the check similar to how skIPSetIntersect()
         *  works with an iterator and a walker.  This would allow the
         *  code to stop walking one IPset once it knew the other IPset
         *  was out of IPs.
         */

        /* check which set as the fewest number of leaves */
        leaf_count1 = ipsetCountOccupiedLeaves(ipset1);
        leaf_count2 = ipsetCountOccupiedLeaves(ipset2);
        if (leaf_count1 < leaf_count2) {
            walk_set = ipset1;
            search_set = ipset2;
        } else {
            walk_set = ipset2;
            search_set = ipset1;
        }
    }

    /* visit the IPs on the walk_set */
#if SK_ENABLE_IPV6
    if (search_set->is_ipv6) {
        rv = skIPSetWalk(walk_set, 1, SK_IPV6POLICY_FORCE,
                         &ipsetCheckIPSetCallbackV6, (void*)search_set);
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        rv = skIPSetWalk(walk_set, 1, SK_IPV6POLICY_ASV4,
                         &ipsetCheckIPSetCallbackV4, (void*)search_set);
    }
    switch (rv) {
      case SKIPSET_OK:
        /* reached end of walk_set with no matches */
        break;

      case SKIPSET_ERR_SUBSET:
        /* found IPs in common */
        return 1;

      default:
        skAbortBadCase(rv);
    }

    return 0;
}


/* Return true if 'ipset' contains any IPs represened by 'ipwild' */
int
skIPSetCheckIPWildcard(
    const skipset_t        *ipset,
    const skIPWildcard_t   *ipwild)
{
    skIPWildcardIterator_t iter;
    skipaddr_t ipaddr;
    uint32_t prefix;

    if (ipset->is_iptree) {
        return skIPTreeCheckIntersectIPWildcard(ipset->s.v2, ipwild);
    }

    /* Iterate over the IPs from the wildcard */
#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        ipset_ipv6_t ipv6;

        skIPWildcardIteratorBindV6(&iter, ipwild);
        if (skIPWildcardIteratorNextCidr(&iter, &ipaddr, &prefix)
            != SK_ITERATOR_OK)
        {
            return 0;
        }
        if (0 == prefix) {
            /* wildcard was x:x:x:x:x:x:x:x */
            if (!skipaddrIsZero(&ipaddr)) {
                skAppPrintErr("Wildcard iterator bug: prefix == 0 but IP != 0");
                skAbort();
            }
            return !IPSET_ISEMPTY(ipset);
        }

        do {
            assert(0 < prefix && prefix <= 128);
            IPSET_IPV6_FROM_ADDRV6(&ipv6, &ipaddr);
            switch (ipsetFindV6(ipset, &ipv6, prefix, NULL)) {
              case SKIPSET_OK:
              case SKIPSET_ERR_SUBSET:
                return 1;
              default:
                break;
            }
        } while (skIPWildcardIteratorNextCidr(&iter, &ipaddr, &prefix)
                 == SK_ITERATOR_OK);
        /* no intersesction */
        return 0;
    }
    /* else IPset is IPv4 */

    if (skIPWildcardIsV6(ipwild)) {
        /* only visit the ::FFFF:0:0/96 netblock and return as IPv4 */
        skIPWildcardIteratorBindV4(&iter, ipwild);
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        /* both wildcard and IPset are IPv4 */
        skIPWildcardIteratorBind(&iter, ipwild);
    }

    if (skIPWildcardIteratorNextCidr(&iter, &ipaddr, &prefix)
        != SK_ITERATOR_OK)
    {
        return 0;
    }
    assert(!skipaddrIsV6(&ipaddr));
    if (0 == prefix) {
        /* wildcard was x.x.x.x */
        if (!skipaddrIsZero(&ipaddr)) {
            skAppPrintErr("Wildcard iterator bug: prefix == 0 but IP != 0");
            skAbort();
        }
        return !IPSET_ISEMPTY(ipset);
    }

    do {
        assert(prefix <= 32);
        assert(!skipaddrIsV6(&ipaddr));
        switch (ipsetFindV4(ipset, skipaddrGetV4(&ipaddr), prefix, NULL)) {
          case SKIPSET_OK:
          case SKIPSET_ERR_SUBSET:
            return 1;
          default:
            break;
        }
    } while (skIPWildcardIteratorNextCidr(&iter, &ipaddr, &prefix)
             == SK_ITERATOR_OK);
    /* no intersesction */
    return 0;
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
    if (ipset->is_ipv6) {
        ipset_ipv6_t ipv6;

        switch (src_dst_nh) {
          case 1:
            rwRecMemGetSIPv6(rwrec, &ipv6);
            break;
          case 2:
            rwRecMemGetDIPv6(rwrec, &ipv6);
            break;
          case 4:
            rwRecMemGetNhIPv6(rwrec, &ipv6);
            break;
          default:
            skAbortBadCase(src_dst_nh);
        }
        ipv6.ip[0] = ntoh64(ipv6.ip[0]);
        ipv6.ip[1] = ntoh64(ipv6.ip[1]);

        return (ipsetFindV6(ipset, &ipv6, 128, NULL) == SKIPSET_OK);
    }
    /* else IPset is IPv4 */

    if (rwRecIsIPv6(rwrec)) {
        /* try to handle addresses as IPv4 */
        skipaddr_t ipaddr;

        switch (src_dst_nh) {
          case 1:
            rwRecMemGetSIP(rwrec, &ipaddr);
            break;
          case 2:
            rwRecMemGetDIP(rwrec, &ipaddr);
            break;
          case 4:
            rwRecMemGetNhIP(rwrec, &ipaddr);
            break;
          default:
            skAbortBadCase(src_dst_nh);
        }
        if (skipaddrGetAsV4(&ipaddr, &ipv4)) {
            /* conversion failed; this IP is not in the IPSet */
            return 0;
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        /* neither IPSet nor rwRec are V6 */
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

    if (ipset->is_iptree) {
        return skIPTreeCheckAddress(ipset->s.v2, ipv4);
    }
    return (ipsetFindV4(ipset, ipv4, 32, NULL) == SKIPSET_OK);
}


/* Make the ipset use as few nodes as possible and make sure the ipset
 * uses a contiguous region of memory. */
int
skIPSetClean(
    skipset_t          *ipset)
{
    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (ipset->is_iptree) {
        ipset->is_dirty = 0;
        return SKIPSET_OK;
    }
    if (!ipset->is_dirty) {
        return SKIPSET_OK;
    }
    if (0 == ipset->s.v3->nodes.entry_count) {
        skIPSetRemoveAll(ipset);
        ipset->is_dirty = 0;
        return SKIPSET_OK;
    }

    IPSET_COPY_ON_WRITE(ipset);

    ipsetCombineAdjacentCIDR(ipset);
    /* ipsetCompact(ipset); -- ipsetSortLeaves() calles this */
    ipsetSortLeaves(ipset);

    ipset->is_dirty = 0;

    return SKIPSET_OK;
}


/* Return true if 'ipset' contains any IPs that cannot be represented
 * as an IPv4 address. */
int
skIPSetContainsV6(
    const skipset_t    *ipset)
{
#if !SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        skAbort();
    }
    return 0;
#else  /* SK_ENABLE_IPV6 */

    if (!ipset->is_ipv6) {
        return 0;
    }
    assert(0 == ipset->is_iptree);
    if (IPSET_ISEMPTY(ipset)) {
        return 0;
    }

    /* For the IPset to containly only IPv4 addresses, the root node
     * must be something like "::ffff:0.0.0.0/96" or more specific,
     * such as "::ffff.10.11.12.0/120" */

    if (IPSET_ROOT_IS_LEAF(ipset)) {
        const ipset_leaf_v6_t *leaf;

        leaf = LEAF_PTR_V6(ipset, IPSET_ROOT_INDEX(ipset));
        if (leaf->prefix < 96) {
            return 1;
        }
        return (leaf->ip.ip[0] != 0
                || ((leaf->ip.ip[1] >> 32) != 0x0000ffff));

    } else {
        const ipset_node_v6_t *node;

        node = NODE_PTR_V6(ipset, IPSET_ROOT_INDEX(ipset));
        if (node->prefix < 96) {
            return 1;
        }
        return (node->ip.ip[0] != 0
                || ((node->ip.ip[1] >> 32) != 0x0000ffff));
    }
#endif  /* SK_ENABLE_IPV6 */
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

#if !SK_ENABLE_IPV6
    if (4 != target_ip_version) {
        return SKIPSET_ERR_IPV6;
    }
    if (1 == ipset->is_ipv6) {
        skAbort();
    }
    return SKIPSET_OK;

#else  /* SK_ENABLE_IPV6 */

    switch (target_ip_version) {
      case 4:
        if (!ipset->is_ipv6) {
            /* IPset is already IPv4 */
            return SKIPSET_OK;
        }
        if (skIPSetContainsV6(ipset)) {
            /* can't convert when IPv6 addresses are present */
            return SKIPSET_ERR_IPV6;
        }
        break;

      case 6:
        if (ipset->is_ipv6) {
            /* IPset is already IPv6 */
            return SKIPSET_OK;
        }
        break;

      default:
        return SKIPSET_ERR_BADINPUT;
    }

    if (ipset->is_iptree) {
        return ipsetConvertIPTreetoV6(ipset);
    }

    IPSET_COPY_ON_WRITE(ipset);

    /* ensure the tree is compact */
    skIPSetClean(ipset);

    if (ipset->is_ipv6) {
        return ipsetConvertV6toV4(ipset);
    }
    return ipsetConvertV4toV6(ipset);
#endif  /* SK_ENABLE_IPV6 */
}


/* Return number of IPs in the IPset */
uint64_t
skIPSetCountIPs(
    const skipset_t    *ipset,
    double             *count)
{
    ipset_count_t count_state;

    if (!ipset) {
        return 0;
    }
    if (ipset->is_iptree) {
        uint64_t c;

        c = skIPTreeCountIPs(ipset->s.v2);
        if (count) {
            *count = (double)c;
        }
        return c;
    }

    memset(&count_state, 0, sizeof(ipset_count_t));

#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        ipsetWalkInternalV6(ipset, ipsetCountCallbackV6, (void*)&count_state);
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        ipsetWalkInternalV4(ipset, ipsetCountCallbackV4, (void*)&count_state);
    }

    if (count_state.upper) {
        if (count) {
            *count = ((double)count_state.upper * ((double)UINT64_MAX + 1.0)
                      + (double)count_state.lower);
        }
        return UINT64_MAX;
    }

    if (count) {
        *count = (double)count_state.lower;
    }
    return count_state.lower;
}


/* Fill 'buf' with number of IPs in the set */
char *
skIPSetCountIPsString(
    const skipset_t    *ipset,
    char               *buf,
    size_t              buflen)
{
    uint64_t i_count;
    double   d_count;
    ssize_t sz;

#if SK_ENABLE_IPV6
    if (ipset && ipset->is_ipv6) {
        static const uint64_t lim = UINT64_C(10000000000);
        static const uint64_t map_ipv6_to_dec[][4] =
            {{          UINT64_C(1),                   0,  0, 0}, /* 1 <<  0 */
             { UINT64_C(4294967296),                   0,  0, 0}, /* 1 << 32 */
             { UINT64_C(3709551616), UINT64_C(1844674407), 0, 0}, /* 1 << 64 */
             { UINT64_C(3543950336), UINT64_C(1426433759), UINT64_C(792281625),
               0}, /* 1 << 96 */
             };
        /* the decimal value being calculated */
        uint64_t decimal[5] = {0, 0, 0, 0, 0};
        ipset_count_t count_state;
        uint64_t tmp;
        uint64_t tmp2 = 0;
        int i, j;

        memset(&count_state, 0, sizeof(count_state));
        ipsetWalkInternalV6(ipset, ipsetCountCallbackV6, (void*)&count_state);
        if (0 == count_state.upper) {
            sz = snprintf(buf, buflen, ("%" PRIu64), count_state.lower);
            if ((size_t)sz >= buflen) {
                return NULL;
            }
            return buf;
        }
        for (i = 0; i < 4; ++i) {
            switch (i) {
              case 0: tmp2 = (count_state.lower & UINT32_MAX);         break;
              case 1: tmp2 = ((count_state.lower >> 32) & UINT32_MAX); break;
              case 2: tmp2 = (count_state.upper & UINT32_MAX);         break;
              case 3: tmp2 = ((count_state.upper >> 32) & UINT32_MAX); break;
            }
            if (tmp2) {
                for (j = 0; j < 4 && map_ipv6_to_dec[i][j] > 0; ++j) {
                    tmp = tmp2 * map_ipv6_to_dec[i][j];
                    if (tmp < lim) {
                        decimal[j] += tmp;
                    } else {
                        /* handle overflow */
                        decimal[j] += tmp % lim;
                        decimal[j+1] += tmp / lim;
                    }
                }
            }
        }
        /* Final check for overflow and determine number of
         * 'decimal' elements that have a value */
        i = 0;
        for (j = 0; j < 4; ++j) {
            if (decimal[j] >= lim) {
                i = j;
                tmp = decimal[j];
                decimal[j] %= lim;
                decimal[j+1] += tmp / lim;
            } else if (decimal[j] > 0) {
                i = j;
            }
        }
        switch (i) {
          case 0:
            sz = snprintf(buf, buflen, "%" PRIu64, decimal[0]);
            break;
          case 1:
            sz = snprintf(buf, buflen, "%" PRIu64 "%010" PRIu64,
                          decimal[1], decimal[0]);
            break;
          case 2:
            sz = snprintf(buf, buflen, "%" PRIu64 "%010" PRIu64 "%010" PRIu64,
                          decimal[2], decimal[1], decimal[0]);
            break;
          case 3:
            sz = snprintf(buf, buflen,
                          "%" PRIu64 "%010" PRIu64 "%010" PRIu64 "%010" PRIu64,
                          decimal[3], decimal[2], decimal[1], decimal[0]);
            break;
          case 4:
            sz = snprintf(buf, buflen,
                          ("%" PRIu64 "%010" PRIu64 "%010" PRIu64
                           "%010" PRIu64 "%010" PRIu64),
                          decimal[4], decimal[3], decimal[2],
                          decimal[1], decimal[0]);
            break;
          default:
            skAbortBadCase(i);
        }
        if ((size_t)sz >= buflen) {
            return NULL;
        }
        return buf;
    }
#endif  /* SK_ENABLE_IPV6 */

    i_count = skIPSetCountIPs(ipset, &d_count);
    if (i_count == UINT64_MAX) {
        sz = snprintf(buf, buflen, ("%.0f"), d_count);
    } else {
        sz = snprintf(buf, buflen, ("%" PRIu64), i_count);
    }
    if ((size_t)sz >= buflen) {
        return NULL;
    }
    return buf;
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

#if !SK_ENABLE_IPV6
    if (support_ipv6) {
        return SKIPSET_ERR_IPV6;
    }
#endif  /* SK_ENABLE_IPV6 */

    if (IPSET_USE_IPTREE) {
        return ipsetCreate(ipset, support_ipv6, 0);
    }
    return ipsetCreate(ipset, support_ipv6, 1);
}


/* Destroy an IPset. */
void
skIPSetDestroy(
    skipset_t         **ipset)
{
    if (!ipset || !*ipset) {
        return;
    }

    if ((*ipset)->is_iptree) {
        skIPTreeDelete(&((*ipset)->s.v2));
        free(*ipset);
        *ipset = NULL;
        return;
    }

    if (getenv(IPSET_ENVAR_DESTROY_PRINT)) {
        skIPSetDebugPrint(*ipset);
    }

    if ((*ipset)->s.v3->mapped_file) {
        munmap((*ipset)->s.v3->mapped_file, (*ipset)->s.v3->mapped_size);
        (*ipset)->s.v3->mapped_file = NULL;
        (*ipset)->s.v3->mapped_size = 0;
    } else {
        skIPSetRemoveAll(*ipset);
        if ((*ipset)->s.v3->nodes.buf) {
            free((*ipset)->s.v3->nodes.buf);
            (*ipset)->s.v3->nodes.buf = NULL;
            (*ipset)->s.v3->nodes.entry_count = 0;
        }
        if ((*ipset)->s.v3->leaves.buf) {
            free((*ipset)->s.v3->leaves.buf);
            (*ipset)->s.v3->leaves.buf = NULL;
            (*ipset)->s.v3->leaves.entry_count = 0;
        }
    }

    free((*ipset)->s.v3);
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
    ipset_find_t find_state;
    uint32_t ipv4;
    int rv;

#if  SK_ENABLE_IPV6
    /* handle auto-conversion */
    if (skipaddrIsV6(ipaddr) && !ipset->is_ipv6) {
        if (skipaddrGetAsV4(ipaddr, &ipv4)
            || (prefix <= 96 && 0 != prefix))
        {
            /* cannot convert IPv6 address to IPv4 */
            if (ipset->no_autoconvert) {
                return SKIPSET_ERR_IPV6;
            }
            rv = skIPSetConvert(ipset, 6);
            if (rv) {
                return rv;
            }
        }
    }

    if (ipset->is_ipv6) {
        ipset_ipv6_t ipv6;

        if (skipaddrIsV6(ipaddr)) {
            /* both set and address are V6 */
            IPSET_IPV6_FROM_ADDRV6(&ipv6, ipaddr);
            if (128 == prefix) {
                /* no-op */
            } else if (0 == prefix) {
                prefix = 128;
            } else if (prefix > 128) {
                return SKIPSET_ERR_PREFIX;
            } else {
                IPSET_IPV6_APPLY_CIDR(&ipv6, prefix);
            }
        } else {
            /* set is V6 and address is V4 */
            IPSET_IPV6_FROM_ADDRV4(&ipv6, ipaddr);
            if (0 == prefix || 32 == prefix) {
                prefix = 128;
            } else if (prefix > 32) {
                return SKIPSET_ERR_PREFIX;
            } else {
                prefix += 96;
                /* apply mask */
                IPSET_IPV6_APPLY_CIDR(&ipv6, prefix);
            }
        }
        rv = ipsetFindV6(ipset, &ipv6, prefix, &find_state);
        if (SKIPSET_OK == rv) {
            return rv;
        }
        IPSET_COPY_ON_WRITE(ipset);
        rv = ipsetInsertAddressV6(ipset, &ipv6, prefix, &find_state);
        if (rv) {
            return rv;
        }
        IPSET_MAYBE_COMBINE(ipset);
        return rv;
    }

    /* To get here, IPset must be IPv4 */
    if (skipaddrIsV6(ipaddr)) {
        /* set is V4 and address is V6 */
        /* attempt to convert V6 ipaddr to V4 */
        if (skipaddrGetAsV4(ipaddr, &ipv4)) {
            /* cannot store V6 ipaddr in a V4 IPSet */
            return SKIPSET_ERR_IPV6;
        }
        if (0 == prefix || 128 == prefix) {
            prefix = 32;
        } else if (prefix > 128) {
            return SKIPSET_ERR_PREFIX;
        } else if (prefix <= 96) {
            return SKIPSET_ERR_IPV6;
        } else {
            prefix -= 96;
            /* apply mask */
            ipv4 &= ~(UINT32_MAX >> prefix);
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        /* both set and address are V4 */
        ipv4 = skipaddrGetV4(ipaddr);
        if (prefix == 32) {
            /* no-op */
        } else if (0 == prefix) {
            prefix = 32;
        } else if (prefix > 32) {
            return SKIPSET_ERR_PREFIX;
        } else {
            /* apply mask */
            ipv4 &= ~(UINT32_MAX >> prefix);
        }
    }

    if (ipset->is_iptree) {
        ipset->is_dirty = 1;
        return ipsetInsertAddressIPTree(ipset, ipv4, prefix);
    }

    rv = ipsetFindV4(ipset, ipv4, prefix, &find_state);
    if (SKIPSET_OK == rv) {
        return rv;
    }
    IPSET_COPY_ON_WRITE(ipset);
    rv = ipsetInsertAddressV4(ipset, ipv4, prefix, &find_state);
    if (rv) {
        return rv;
    }
    IPSET_MAYBE_COMBINE(ipset);
    return rv;
}


/* Add each IP in the IPWildcard to the IPset. */
int
skIPSetInsertIPWildcard(
    skipset_t              *ipset,
    const skIPWildcard_t   *ipwild)
{
    skIPWildcardIterator_t iter;
    skipaddr_t ip;
    uint32_t prefix;
    int rv = SKIPSET_OK;

#if  SK_ENABLE_IPV6
    /* handle auto-conversion */
    if (skIPWildcardIsV6(ipwild) && !ipset->is_ipv6) {
        if (ipset->no_autoconvert) {
            return SKIPSET_ERR_IPV6;
        }
        rv = skIPSetConvert(ipset, 6);
        if (rv) {
            return rv;
        }
    }
#endif  /* SK_ENABLE_IPV6 */

    if (ipset->is_iptree) {
        ipset->is_dirty = 1;

        rv = skIPTreeAddIPWildcard(ipset->s.v2, ipwild);
        switch (rv) {
          case SKIP_OK:
            return SKIPSET_OK;
          case SKIP_ERR_ALLOC:
            return SKIPSET_ERR_ALLOC;
          case SKIP_ERR_IPV6:
          default:
            skAbortBadCase(rv);
        }
    }

    /* Insert the netblocks contained in the wildcard */
#if  SK_ENABLE_IPV6
    if (ipset->is_ipv6 && !skIPWildcardIsV6(ipwild)) {
        skIPWildcardIteratorBindV6(&iter, ipwild);
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        skIPWildcardIteratorBind(&iter, ipwild);
    }

    if (skIPWildcardIteratorNextCidr(&iter, &ip, &prefix)
        != SK_ITERATOR_OK)
    {
        return rv;
    }
    if (0 == prefix) {
        /* wildcard was x.x.x.x or x:x:x:x:x:x:x:x */
        if (!skipaddrIsZero(&ip)) {
            skAppPrintErr("Wildcard iterator bug: prefix == 0 but IP != 0");
            skAbort();
        }
        /* code thinks prefix of 0 means "default", so must insert two
         * CIDR blocks of prefix 1 */
        prefix = 1;
        rv = skIPSetInsertAddress(ipset, &ip, prefix);
        if (rv) { return rv; }
        skCIDRComputeEnd(&ip, prefix, &ip);
        skipaddrIncrement(&ip);
        return skIPSetInsertAddress(ipset, &ip, prefix);
    }

    /* it would be more efficient to inline skIPSetInsertAddress()
     * here; but often there is only one CIDR block in a wildcard */
    do {
        rv = skIPSetInsertAddress(ipset, &ip, prefix);
    } while (skIPWildcardIteratorNextCidr(&iter, &ip, &prefix)==SK_ITERATOR_OK
             && SKIPSET_OK == rv);

    return rv;
}


int
skIPSetInsertRange(
    skipset_t          *ipset,
    const skipaddr_t   *ipaddr_start,
    const skipaddr_t   *ipaddr_end)
{
    skipaddr_t this_start;
    skipaddr_t next_start;
    uint32_t prefix;
    int rv;

    rv = skipaddrCompare(ipaddr_start, ipaddr_end);
    if (rv > 0) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (0 == rv) {
        /* single ip */
        return skIPSetInsertAddress(ipset, ipaddr_start, 0);
    }

    if (ipset->is_iptree) {
#if !SK_ENABLE_IPV6
        return ipsetInsertRangeIPTree(ipset, ipaddr_start, ipaddr_end);
#else
        if (!skipaddrIsV6(ipaddr_start) && !skipaddrIsV6(ipaddr_end)) {
            return ipsetInsertRangeIPTree(ipset, ipaddr_start, ipaddr_end);
        }
        if (ipset->no_autoconvert) {
            return SKIPSET_ERR_IPV6;
        }
        rv = ipsetConvertIPTreetoV6(ipset);
        if (rv) {
            return rv;
        }
#endif  /* #else of #if !SK_ENABLE_IPV6 */
    }

    /* get a modifiable version of the beginning IP */
    skipaddrCopy(&this_start, ipaddr_start);

    do {
        prefix = skCIDRComputePrefix(&this_start, ipaddr_end, &next_start);
        rv = skIPSetInsertAddress(ipset, &this_start, prefix);
        if (rv) {
            return rv;
        }
        skipaddrCopy(&this_start, &next_start);
    } while (!skipaddrIsZero(&this_start));

    return SKIPSET_OK;
}


/* Make 'result_ipset' hold the intersection of itself with 'ipset' */
int
skIPSetIntersect(
    skipset_t          *result_ipset,
    const skipset_t    *ipset)
{
    ipset_intersect_t state;
    uint32_t prefix;
    skipaddr_t tmp_ip;
    int rv = SKIPSET_OK;
    int i;

    /* check input */
    if (!result_ipset || !ipset) {
        return SKIPSET_ERR_BADINPUT;
    }

    if (result_ipset->is_iptree && ipset->is_iptree) {
        /* both are in the SiLK-2 format (IPTree) */
        result_ipset->is_dirty = 1;
        skIPTreeIntersect(result_ipset->s.v2, ipset->s.v2);
        return SKIPSET_OK;
    }

    /* handle trivial cases */
    if (!result_ipset->is_iptree && IPSET_ISEMPTY(result_ipset)) {
        return SKIPSET_OK;
    }
    if (!ipset->is_iptree && IPSET_ISEMPTY(ipset)) {
        skIPSetRemoveAll(result_ipset);
        return SKIPSET_OK;
    }

    /* ensure we can use the iterator for the result_ipset */
    if (result_ipset->is_dirty) {
        skIPSetClean(result_ipset);
    }

    /* clear memory */
    memset(&state, 0, sizeof(ipset_intersect_t));

    /* create a vector to hold the blocks that the two IPsets have in
     * common; after visiting the IPsets, the result_ipset will be
     * updated with the IPs in this vector */
    state.vec_add = skVectorNew(sizeof(state.addr));
    if (NULL == state.vec_add) {
        return SKIPSET_ERR_ALLOC;
    }

    /* bind the iterator for the result_ipset, get its first CIDR
     * block, and convert the block to start and end IPs */
    skIPSetIteratorBind(&state.iter, result_ipset, 1, SK_IPV6POLICY_MIX);
    if (skIPSetIteratorNext(&state.iter, &state.addr[0], &prefix)
        != SK_ITERATOR_OK)
    {
        skIPSetRemoveAll(result_ipset);
        goto END;
    }
    skCIDR2IPRange(&state.addr[0], prefix, &state.addr[0], &state.addr[1]);

    /* visit the IPs on the ipset */
    rv = skIPSetWalk(ipset, 1, (result_ipset->is_ipv6
                                ? SK_IPV6POLICY_FORCE : SK_IPV6POLICY_MIX),
                     ipsetIntersectCallback, (void*)&state);
    if (SKIPSET_ERR_ALLOC == rv) {
        goto END;
    }
    rv = SKIPSET_OK;

    /* remove all IPs from the result IPset */
    skIPSetRemoveAll(result_ipset);

    /* loop over the start_addr/final_addr IP pairs in the vec_add
     * vector, convert them to CIDR blocks, and add them to the result
     * IPset. */
    for (i = 0; (0 == skVectorGetValue(state.addr, state.vec_add, i)); ++i) {
        do {
            prefix = skCIDRComputePrefix(&state.addr[0], &state.addr[1],
                                         &tmp_ip);
            rv = skIPSetInsertAddress(result_ipset, &state.addr[0], prefix);
            if (rv != SKIPSET_OK) {
                goto END;
            }
            skipaddrCopy(&state.addr[0], &tmp_ip);
        } while (!skipaddrIsZero(&tmp_ip));
    }

  END:
    if (state.vec_add) {
        skVectorDestroy(state.vec_add);
    }
    return rv;
}


/* Return true if 'ipset' can hold IPv6 addresses */
int
skIPSetIsV6(
    const skipset_t    *ipset)
{
    return ipset->is_ipv6;
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
    iter->ipset = ipset;
    iter->v6policy = v6_policy;
    iter->cidr_blocks = (cidr_blocks ? 1 : 0);

    if (ipset->is_iptree) {
        if (cidr_blocks) {
            ASSERT_OK(skIPTreeCIDRBlockIteratorBind(&iter->iter.v2cidr,
                                                    ipset->s.v2));
        } else {
            ASSERT_OK(skIPTreeIteratorBind(&iter->iter.v2ip, ipset->s.v2));
        }
    } else {
        skIPSetIteratorReset(iter);
    }

    return 0;
}


/* Return the next CIDR block associated with the IPset */
int
skIPSetIteratorNext(
    skipset_iterator_t *iter,
    skipaddr_t         *ipaddr,
    uint32_t           *prefix)
{
    ipset_leaf_v4_t *leaf4;
    uint32_t ipv4;

    if (iter->ipset->is_iptree) {
        return ipsetIteratorNextIPTree(iter, ipaddr, prefix);
    }

    if (iter->iter.v3.cur >= iter->ipset->s.v3->leaves.entry_count) {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }
    assert(0 == iter->ipset->is_dirty);

#if SK_ENABLE_IPV6
    if (iter->ipset->is_ipv6) {
        ipset_leaf_v6_t *leaf6;
        ipset_ipv6_t *ipv6;
        ipset_ipv6_t *fin6;

        if (iter->cidr_blocks) {
            leaf6 = LEAF_PTR_V6(iter->ipset, iter->iter.v3.cur);
            if (SK_IPV6POLICY_ASV4 == iter->v6policy) {
                /* check stopping condition */
                if (leaf6->ip.ip[0] != 0
                    || ((UINT64_C(0xffffffff00000000) & leaf6->ip.ip[1])
                        != UINT64_C(0x0000ffff00000000)))
                {
                    iter->iter.v3.cur = iter->ipset->s.v3->leaves.entry_count;
                    return SK_ITERATOR_NO_MORE_ENTRIES;
                }
                IPSET_IPV6_TO_ADDR_V4(&leaf6->ip, ipaddr);
                *prefix = leaf6->prefix - 96;
            } else {
                IPSET_IPV6_TO_ADDR(&leaf6->ip, ipaddr);
                *prefix = leaf6->prefix;
            }
            ++iter->iter.v3.cur;

        } else {
            /* return value is value on the iterator */
            ipv6 = (ipset_ipv6_t*)&iter->iter.v3.data[0];
            fin6 = (ipset_ipv6_t*)&iter->iter.v3.data[2];
            if (SK_IPV6POLICY_ASV4 == iter->v6policy) {
                IPSET_IPV6_TO_ADDR_V4(ipv6, ipaddr);
                *prefix = 32;
            } else {
                IPSET_IPV6_TO_ADDR(ipv6, ipaddr);
                *prefix = 128;
            }

            /* point iterator at next value */
            if (ipv6->ip[1] < fin6->ip[1]) {
                ++ipv6->ip[1];
            } else if (ipv6->ip[0] < fin6->ip[0]) {
                if (ipv6->ip[1] == UINT64_MAX) {
                    ++ipv6->ip[0];
                    ipv6->ip[1] = 0;
                } else {
                    ++ipv6->ip[1];
                }
            } else {
                ++iter->iter.v3.cur;
                if (iter->iter.v3.cur < iter->ipset->s.v3->leaves.entry_count) {
                    ipsetIteratorNextRangeV6(iter);
                }
            }
        }
        return SK_ITERATOR_OK;
    }

    if (SK_IPV6POLICY_FORCE == iter->v6policy) {
        if (iter->cidr_blocks) {
            leaf4 = LEAF_PTR_V4(iter->ipset, iter->iter.v3.cur);
            skipaddrSetV6FromUint32(ipaddr, &leaf4->ip);
            *prefix = 96 + leaf4->prefix;
            ++iter->iter.v3.cur;

        } else {
            /* return value is value on the iterator */
            ipv4 = (uint32_t)iter->iter.v3.data[0];
            skipaddrSetV6FromUint32(ipaddr, &ipv4);
            *prefix = 128;

            /* point iterator at next value */
            if (iter->iter.v3.data[0] < iter->iter.v3.data[2]) {
                ++iter->iter.v3.data[0];
            } else {
                ++iter->iter.v3.cur;
                if (iter->iter.v3.cur < iter->ipset->s.v3->leaves.entry_count) {
                    ipsetIteratorNextRangeV4(iter);
                }
            }
        }
        return SK_ITERATOR_OK;
    }
#endif  /* SK_ENABLE_IPV6 */

    if (iter->cidr_blocks) {
        leaf4 = LEAF_PTR_V4(iter->ipset, iter->iter.v3.cur);
        skipaddrSetV4(ipaddr, &leaf4->ip);
        *prefix = leaf4->prefix;
        ++iter->iter.v3.cur;

    } else {
        /* return value is value on the iterator */
        ipv4 = (uint32_t)iter->iter.v3.data[0];
        skipaddrSetV4(ipaddr, &ipv4);
        *prefix = 32;

        /* point iterator at next value */
        if (iter->iter.v3.data[0] < iter->iter.v3.data[2]) {
            ++iter->iter.v3.data[0];
        } else {
            ++iter->iter.v3.cur;
            if (iter->iter.v3.cur < iter->ipset->s.v3->leaves.entry_count) {
                ipsetIteratorNextRangeV4(iter);
            }
        }
    }

    return SK_ITERATOR_OK;
}


/* Reset iterator so we can visit the IPs again */
void
skIPSetIteratorReset(
    skipset_iterator_t *iter)
{
    if (iter->ipset->is_iptree) {
        if (iter->cidr_blocks) {
            skIPTreeCIDRBlockIteratorReset(&iter->iter.v2cidr);
        } else {
            skIPTreeIteratorReset(&iter->iter.v2ip);
        }
        return;
    }

    iter->iter.v3.cur = IPSET_ITER_FIRST_LEAF;
    if (IPSET_ISEMPTY(iter->ipset)) {
        return;
    }

#if SK_ENABLE_IPV6
    if (iter->ipset->is_ipv6) {
        if (iter->v6policy == SK_IPV6POLICY_IGNORE) {
            /* caller wants only IPv4 addresses, and there are none in
             * an IPv6 IPset. */
            iter->iter.v3.cur = iter->ipset->s.v3->leaves.entry_count;
            return;
        }
        if (iter->v6policy == SK_IPV6POLICY_ASV4) {
            /* find the left-most leaf under ::ffff:0:0/96 */
            ipset_find_t find_state;
            ipset_ipv6_t ipv6;
            const ipset_node_v6_t *node;
            uint32_t i;
            int rv;

            ipv6.ip[0] = 0;
            ipv6.ip[1] = UINT64_C(0xffff00000000);

            rv = ipsetFindV6(iter->ipset, &ipv6, 96, &find_state);
            if (SKIPSET_OK != rv && SKIPSET_ERR_SUBSET != rv) {
                /* no IPv4 addresses in the IPset */
                iter->iter.v3.cur = iter->ipset->s.v3->leaves.entry_count;
                return;
            }

            if (find_state.node_is_leaf) {
                iter->iter.v3.cur = find_state.node_idx;
            } else {
                node = NODE_PTR_V6(iter->ipset, find_state.node_idx);
                for (;;) {
                    for (i=0; i < IPSET_NUM_CHILDREN && !node->child[i]; ++i)
                        ;       /* empty */
                    if (NODEPTR_CHILD_IS_LEAF(node, i)) {
                        iter->iter.v3.cur = node->child[i];
                        break;
                    }
                    node = NODE_PTR_V6(iter->ipset, node->child[i]);
                }
            }
        }
        assert(iter->iter.v3.cur < iter->ipset->s.v3->leaves.entry_count);
        if (!iter->cidr_blocks) {
            ipsetIteratorNextRangeV6(iter);
        }
        return;
    }
    /* else, IPset contains IPv4 addresses */

    if (iter->v6policy == SK_IPV6POLICY_ONLY) {
        /* caller wants only IPv6 addresses, and there are none in an
         * IPv4 IPset. */
        iter->iter.v3.cur = iter->ipset->s.v3->leaves.entry_count;
        return;
    }

#else  /* #if SK_ENABLE_IPV6 */

    if (iter->v6policy > SK_IPV6POLICY_MIX) {
        /* caller wants IPv6 addresses, which are not supported in
         * IPv4-only SiLK. */
        iter->iter.v3.cur = iter->ipset->s.v3->leaves.entry_count;
        return;
    }

#endif  /* #else of #if SK_ENABLE_IPV6 */

    assert(iter->iter.v3.cur < iter->ipset->s.v3->leaves.entry_count);
    if (!iter->cidr_blocks) {
        ipsetIteratorNextRangeV4(iter);
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

#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        /* verify mask_prefix value is valid */
        if (mask_prefix >= 128 || mask_prefix == 0) {
            return SKIPSET_ERR_PREFIX;
        }

        if (IPSET_ISEMPTY(ipset)) {
            return SKIPSET_OK;
        }
        IPSET_COPY_ON_WRITE(ipset);

        return ipsetMaskV6(ipset, mask_prefix);
    }
#endif  /* SK_ENABLE_IPV6 */

    /* verify mask_prefix value is valid */
    if (mask_prefix >= 32 || mask_prefix == 0) {
        return SKIPSET_ERR_PREFIX;
    }

    if (ipset->is_iptree) {
        ipset->is_dirty = 1;
        skIPTreeMask(ipset->s.v2, mask_prefix);
        return SKIPSET_OK;
    }

    if (IPSET_ISEMPTY(ipset)) {
        return SKIPSET_OK;
    }
    IPSET_COPY_ON_WRITE(ipset);

    return ipsetMaskV4(ipset, mask_prefix);
}


/* For each occupied block of size 'mask_prefix', set all IPs. */
int
skIPSetMaskAndFill(
    skipset_t          *ipset,
    uint32_t            mask_prefix)
{
    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }

#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        /* verify mask_prefix value is valid */
        if (mask_prefix >= 128 || mask_prefix == 0) {
            return SKIPSET_ERR_PREFIX;
        }

        if (IPSET_ISEMPTY(ipset)) {
            return SKIPSET_OK;
        }
        IPSET_COPY_ON_WRITE(ipset);

        return ipsetMaskAndFillV6(ipset, mask_prefix);
    }
#endif  /* SK_ENABLE_IPV6 */

    /* verify mask_prefix value is valid */
    if (mask_prefix >= 32 || mask_prefix == 0) {
        return SKIPSET_ERR_PREFIX;
    }

    if (ipset->is_iptree) {
        return ipsetMaskAndFillIPTree(ipset, mask_prefix);
    }

    if (IPSET_ISEMPTY(ipset)) {
        return SKIPSET_OK;
    }
    IPSET_COPY_ON_WRITE(ipset);

    return ipsetMaskAndFillV4(ipset, mask_prefix);
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
    const char *envar;
    uint32_t tmp32 = 0;

    assert(ipset_opts);
    assert(sizeof(ipset_options)/sizeof(struct option)
           == sizeof(ipset_options_help)/sizeof(char*));

    /* set default record version */
    ipset_opts->record_version = IPSET_REC_VERSION_DEFAULT;
    ipset_opts->invocation_strip = 0;
    ipset_opts->comp_method = 0;
    ipset_opts->note_strip = 0;

    /* override default from the environment */
    envar = getenv(IPSET_REC_VERSION_ENVAR);
    if (envar
        && 0 == skStringParseUint32(&tmp32, envar, IPSET_REC_VERSION_DEFAULT,
                                    IPSET_REC_VERSION_MAX)
        && 1 != tmp32)
    {
        ipset_opts->record_version = (uint16_t)tmp32;
    }

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
            fprintf(fh, ("\n\tValid values: %d,%d-%d. Def. %d."
                         " Value of %d means use SiLK-2 format for\n"
                         "\tIPv4 IPsets and SiLK-3 for IPv6 IPsets."),
                    IPSET_REC_VERSION_DEFAULT,
                    IPSET_REC_VERSION_LEGACY, IPSET_REC_VERSION_MAX,
                    IPSET_REC_VERSION_DEFAULT, IPSET_REC_VERSION_DEFAULT);
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
    ipset_print_t print_state;

    if (ipset->is_iptree) {
        skIPTreePrint(ipset->s.v2, stream, ip_format, as_cidr);
        return;
    }

    print_state.ipset = ipset;
    print_state.stream = stream;
    print_state.ip_format = ip_format;

    skIPSetWalk(ipset, (as_cidr ? 1 : 0), SK_IPV6POLICY_MIX,
                &ipsetPrintCallback, (void*)&print_state);
}


/*
 *  ipsetDebugPrintAddrV4(ip, prefix);
 *  ipsetDebugPrintAddrV6(ip, prefix);
 *
 *    Helper functions for the various skIPSetDebugPrint*() functions
 *    to print an IP address and its prefix.
 */
static void
ipsetDebugPrintAddrV4(
    uint32_t            ipv4,
    uint32_t            prefix)
{
    int i;

    /* print IP (2 ways) */
    for (i = 24; i >= 0; i -= 8) {
        fprintf(stderr, "%02x%c",
                0xFF & (ipv4 >> i), (i ? '.' : '/'));
    }
    fprintf(stderr, "%2u [", prefix);
    for (i = 24; i >= 0; i -= 8) {
        fprintf(stderr, "%3d%c",
                0xFF & (ipv4 >> i), (i ? '.' : '/'));
    }
    fprintf(stderr, "%2u]", prefix);
}

#if SK_ENABLE_IPV6
static void
ipsetDebugPrintAddrV6(
    const ipset_ipv6_t *ipv6,
    uint32_t            prefix)
{
    uint32_t i;
    uint32_t j;

    /* print IP */
    fprintf(stderr, "[");
    for (j = 0; j < 2; ++j) {
        for (i = 48; i > 0; i -= 16) {
            fprintf(stderr, ("%4" PRIx64 ":"),
                    0xFFFF & (ipv6->ip[j] >> i));
        }
        fprintf(stderr, ("%4" PRIx64 "%c"),
                (0xFFFF & ipv6->ip[j]), (j ? '/' : ':'));
    }
    fprintf(stderr, "%3u]", prefix);
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  ipsetDebugPrintChildren(node, width);
 *
 *    A helper function for the various skIPSetDebugPrint*()
 *    functions.  This function takes a node and prints the indexes of
 *    the children that the node points to.
 */
static void
ipsetDebugPrintChildren(
    const ipset_node_t *node,
    int                 width)
{
    uint32_t i;

    /* print children's IDs, followed by N for node, L for
     * leaf, or R for repeated leaf */
    for (i = 0; i < IPSET_NUM_CHILDREN; ++i) {
        if (0 == node->v4.child[i]) {
            fprintf(stderr, " %*c ", width, '-');
        } else {
            fprintf(stderr, " %*u%c",
                    width, node->v4.child[i],
                    (!NODEPTR_CHILD_IS_LEAF(&node->v4, i)
                     ? 'N'
                     : (NODEPTR_CHILD_IS_REPEAT(&node->v4, i)
                        ? 'R' : 'L')));
        }
    }
}


/*
 *  ipsetDebugPrintLeaf(ipset, leaf);
 *
 *    Print the IP address on a leaf.
 */
static void
ipsetDebugPrintLeaf(
    const skipset_t    *ipset,
    const ipset_leaf_t *leaf)
{
    if (ipset->is_ipv6) {
#if SK_ENABLE_IPV6
        ipsetDebugPrintAddrV6(&leaf->v6.ip, leaf->v6.prefix);
#endif  /* SK_ENABLE_IPV6 */
        fprintf(stderr, "\n");
    } else {
        ipsetDebugPrintAddrV4(leaf->v4.ip, leaf->v4.prefix);
        fprintf(stderr, "\n");
    }
}


/*
 *  ipsetDebugPrintNode(ipset, node);
 *
 *    Print the IP address and child values from a node.
 */
static void
ipsetDebugPrintNode(
    const skipset_t    *ipset,
    const ipset_node_t *node)
{
    int width = 0;

    if (ipset->s.v3->leaves.entry_count) {
        width = 2 + (int)log10(ipset->s.v3->leaves.entry_count);
    }

#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        ipsetDebugPrintAddrV6(&node->v6.ip, node->v6.prefix);
        fprintf(stderr, "  ");
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        ipsetDebugPrintAddrV4(node->v4.ip, node->v4.prefix);
        fprintf(stderr, "  ");
    }

    ipsetDebugPrintChildren(node, width);

    fprintf(stderr, "\n");
}


/*
 *  ipsetDebugPrintByIndex(ipset, node_idx, is_leaf);
 *
 *    Print information about a node or a leaf whose index is
 *    'node_idx'.
 */
static void
ipsetDebugPrintByIndex(
    const skipset_t    *ipset,
    uint32_t            node_idx,
    int                 is_leaf)
{
    if (is_leaf) {
        if (node_idx < ipset->s.v3->leaves.entry_count) {
            ipsetDebugPrintLeaf(ipset, LEAF_PTR(ipset, node_idx));
        } else {
            fprintf(stderr, "%" PRIu32 "L is too large\n", node_idx);
        }
    } else {
        if (node_idx < ipset->s.v3->nodes.entry_count) {
            ipsetDebugPrintNode(ipset, NODE_PTR(ipset, node_idx));
        } else {
            fprintf(stderr, "%" PRIu32 "N is too large\n", node_idx);
        }
    }
}


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

    if (ipset->is_iptree) {
        return;
    }

    if (ipset->s.v3->leaves.entry_count) {
        width = 2 + (int)log10(ipset->s.v3->leaves.entry_count);
    }

    /* print root ID */
    fprintf(stderr,
            ">> %*sROOT %u%c      NODE_FREE %uN      LEAF_FREE %uL\n",
            width, "", (unsigned)IPSET_ROOT_INDEX(ipset),
            (IPSET_ROOT_IS_LEAF(ipset) ? 'L' : 'N'),
            (unsigned)ipset->s.v3->nodes.free_list,
            (unsigned)ipset->s.v3->leaves.free_list);

    if (IPSET_ISEMPTY(ipset)) {
        return;
    }

    /* this function creates a bitmap to note which nodes and leaves
     * are on the free list.  this is the size of the bitmap */
    if (ipset->s.v3->nodes.entry_count > ipset->s.v3->leaves.entry_count) {
        bitmap_size = ipset->s.v3->nodes.entry_count;
    } else {
        bitmap_size = ipset->s.v3->leaves.entry_count;
    }

    /* create the bitmap */
    if (skBitmapCreate(&isfree, bitmap_size)) {
        /* unable to create bitmap; use simple printing (this also
         * allows us to use the functions and avoid warnings from
         * gcc) */

        /* print nodes */
        for (node_idx = 0;
             node_idx < ipset->s.v3->nodes.entry_count;
             ++node_idx)
        {
            fprintf(stderr, "** %*uN  ", width, node_idx);
            ipsetDebugPrintByIndex(ipset, node_idx, 0);
        }

        /* print leaves */
        fprintf(stderr, "\n");
        for (node_idx = 0;
             node_idx < ipset->s.v3->leaves.entry_count;
             ++node_idx)
        {
            fprintf(stderr, "** %*uL  ", width, node_idx);
            ipsetDebugPrintByIndex(ipset, node_idx, 1);
        }

        return;
    }

    /* fill the bitmap for nodes on the free list */
    for (node_idx = ipset->s.v3->nodes.free_list;
         0 != node_idx;
         node_idx = NODEIDX_FREE_LIST(ipset, node_idx))
    {
        assert(node_idx < ipset->s.v3->nodes.entry_count);
        skBitmapSetBit(isfree, node_idx);
    }

    /* print the nodes */
    for (node_idx = 0; node_idx < ipset->s.v3->nodes.entry_count; ++node_idx) {
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
    for (node_idx = ipset->s.v3->leaves.free_list;
         0 != node_idx;
         node_idx = LEAFIDX_FREE_LIST(ipset, node_idx))
    {
        assert(node_idx < ipset->s.v3->leaves.entry_count);
        skBitmapSetBit(isfree, node_idx);
    }

    /* print the leaves */
    fprintf(stderr, "\n");
    for (node_idx = 0; node_idx < ipset->s.v3->leaves.entry_count; ++node_idx) {
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


int
skIPSetRead(
    skipset_t         **ipset_out,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    int rv;

    if (!stream || !ipset_out) {
        return SKIPSET_ERR_BADINPUT;
    }
    *ipset_out = NULL;

    rv = skStreamReadSilkHeader(stream, &hdr);
    if (rv) {
        if (SKSTREAM_ERR_COMPRESS_UNAVAILABLE == rv) {
            return SKIPSET_ERR_FILEHEADER;
        }
        return SKIPSET_ERR_FILEIO;
    }

    /* Ensure we are reading a file we understand */
    rv = skStreamCheckSilkHeader(stream, FT_IPSET,
                                 0, IPSET_REC_VERSION_MAX, NULL);
    switch (rv) {
      case SKSTREAM_OK:
        break;
      case SKSTREAM_ERR_UNSUPPORT_FORMAT:
        /* not an IPset file */
        return SKIPSET_ERR_FILETYPE;
      case SKSTREAM_ERR_UNSUPPORT_VERSION:
        /* newer than we understand */
        return SKIPSET_ERR_FILEVERSION;
      default:
        return SKIPSET_ERR_FILEHEADER;
    }
    if (skHeaderGetRecordLength(hdr) != 1) {
        return SKIPSET_ERR_FILEHEADER;
    }

    /* Go to helper function depending on record version */

    if (skHeaderGetRecordVersion(hdr) < IPSET_REC_VERSION_RADIX) {
        /* Handle files in IPSET_REC_VERSION_LEGACY format */
        return ipsetReadLegacy(ipset_out, stream, hdr);
    }
    if (skHeaderGetRecordVersion(hdr) == IPSET_REC_VERSION_RADIX) {
        return ipsetReadRadix(ipset_out, stream, hdr);
    }
    if (skHeaderGetRecordVersion(hdr) == IPSET_REC_VERSION_CIDR_BMAP) {
        return ipsetReadCidrbmap(ipset_out, stream, hdr);
    }

    skAbort();
}


int
skIPSetRemoveAddress(
    skipset_t          *ipset,
    const skipaddr_t   *ipaddr,
    uint32_t            prefix)
{
    ipset_find_t find_state;
    uint32_t ipv4;
    int rv;

#if  SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        ipset_ipv6_t ipv6;

        if (skipaddrIsV6(ipaddr)) {
            /* both set and address are V6 */
            IPSET_IPV6_FROM_ADDRV6(&ipv6, ipaddr);
            if (128 == prefix) {
                /* no-op */
            } else if (0 == prefix) {
                prefix = 128;
            } else if (prefix > 128) {
                return SKIPSET_ERR_PREFIX;
            } else {
                IPSET_IPV6_APPLY_CIDR(&ipv6, prefix);
            }
        } else {
            /* set is V6 and address is V4 */
            IPSET_IPV6_FROM_ADDRV4(&ipv6, ipaddr);
            if (0 == prefix || 32 == prefix) {
                prefix = 128;
            } else if (prefix > 32) {
                return SKIPSET_ERR_PREFIX;
            } else {
                prefix += 96;
                /* apply mask */
                IPSET_IPV6_APPLY_CIDR(&ipv6, prefix);
            }
        }
        rv = ipsetFindV6(ipset, &ipv6, prefix, &find_state);
        /* if IP was not found, we can return */
        if (SKIPSET_ERR_NOTFOUND == rv
            || SKIPSET_ERR_EMPTY == rv
            || SKIPSET_ERR_MULTILEAF == rv)
        {
            return SKIPSET_OK;
        }
        IPSET_COPY_ON_WRITE(ipset);
        rv = ipsetRemoveAddressV6(ipset, &ipv6, prefix, &find_state);
        if (rv) {
            return rv;
        }
        IPSET_MAYBE_COMBINE(ipset);
        return rv;
    }

    /* To get here, IPset must be IPv4 */
    if (skipaddrIsV6(ipaddr)) {
        /* set is V4 and address is V6 */
        /* attempt to convert V6 ipaddr to V4 */
        if (skipaddrGetAsV4(ipaddr, &ipv4)) {
            /* an V6 ipaddr is not going to be in a V4 IPSet */
            return SKIPSET_OK;
        }
        if (0 == prefix || 128 == prefix) {
            prefix = 32;
        } else if (prefix > 128) {
            return SKIPSET_ERR_PREFIX;
        } else if (prefix <= 96) {
            return SKIPSET_OK;
        } else {
            prefix -= 96;
            /* apply mask */
            ipv4 &= ~(UINT32_MAX >> prefix);
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        /* both set and address are V4 */
        ipv4 = skipaddrGetV4(ipaddr);
        if (prefix == 32) {
            /* no-op */
        } else if (0 == prefix) {
            prefix = 32;
        } else if (prefix > 32) {
            return SKIPSET_ERR_PREFIX;
        } else {
            /* apply mask */
            ipv4 &= ~(UINT32_MAX >> prefix);
        }
    }

    if (ipset->is_iptree) {
        return ipsetRemoveAddressIPTree(ipset, ipv4, prefix);
    }

    rv = ipsetFindV4(ipset, ipv4, prefix, &find_state);
    /* if IP was not found, we can return */
    if (SKIPSET_ERR_NOTFOUND == rv
        || SKIPSET_ERR_EMPTY == rv
        || SKIPSET_ERR_MULTILEAF == rv)
    {
        return SKIPSET_OK;
    }
    IPSET_COPY_ON_WRITE(ipset);
    rv = ipsetRemoveAddressV4(ipset, ipv4, prefix, &find_state);
    if (rv) {
        return rv;
    }
    IPSET_MAYBE_COMBINE(ipset);
    return rv;
}


int
skIPSetRemoveAll(
    skipset_t          *ipset)
{
    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }

    if (ipset->is_iptree) {
        ipset->is_dirty = 1;
        ASSERT_OK(skIPTreeRemoveAll(ipset->s.v2));
        return SKIPSET_OK;
    }

    IPSET_COPY_ON_WRITE(ipset);

    IPSET_ROOT_INDEX_SET(ipset, 0, 0);

    if (ipset->s.v3->nodes.buf) {
        memset(ipset->s.v3->nodes.buf, 0, (ipset->s.v3->nodes.entry_capacity
                                           * ipset->s.v3->nodes.entry_size));
        ipset->s.v3->nodes.entry_count = 0;
        ipset->is_dirty = 0;
    }
    if (ipset->s.v3->leaves.buf) {
        memset(ipset->s.v3->leaves.buf, 0, (ipset->s.v3->leaves.entry_capacity
                                            * ipset->s.v3->leaves.entry_size));
        ipset->s.v3->leaves.entry_count = 0;
        ipset->is_dirty = 0;
        ipset->s.v3->realloc_leaves = 0;
    }
    return SKIPSET_OK;
}


/* Remove each IP in the IPWildcard from the IPset */
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
#if  SK_ENABLE_IPV6
    if (ipset->is_ipv6 && !skIPWildcardIsV6(ipwild)) {
        skIPWildcardIteratorBindV6(&iter, ipwild);
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        skIPWildcardIteratorBind(&iter, ipwild);
    }

    if (skIPWildcardIteratorNextCidr(&iter, &ip, &prefix)
        != SK_ITERATOR_OK)
    {
        return rv;
    }
    if (0 == prefix) {
        /* wildcard was x.x.x.x or x:x:x:x:x:x:x:x */
        if (!skipaddrIsZero(&ip)) {
            skAppPrintErr("Wildcard iterator bug: prefix == 0 but IP != 0");
            skAbort();
        }
        return skIPSetRemoveAll(ipset);
    }

    /* it would be more efficient to inline skIPSetRemoveAddress()
     * here; but often there is only one CIDR block in a wildcard */
    do {
        rv = skIPSetRemoveAddress(ipset, &ip, prefix);
    } while (skIPWildcardIteratorNextCidr(&iter, &ip, &prefix)==SK_ITERATOR_OK
             && SKIPSET_OK == rv);

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
    int rv;

    if (!result_ipset) {
        return SKIPSET_ERR_BADINPUT;
    }
    if (!ipset) {
        return SKIPSET_OK;
    }

    if (ipset->is_iptree) {
        if (result_ipset->is_iptree) {
            /* both are in the SiLK-2 format (IPTree) */
            result_ipset->is_dirty = 1;
            skIPTreeSubtract(result_ipset->s.v2, ipset->s.v2);
            return SKIPSET_OK;
        }
        /* result_ipset is SiLK-3 and other is SiLK-2.  Walk over the
         * entries of the SiLK-2 IPset and remove from the SiLK-3
         * IPset. */
        IPSET_COPY_ON_WRITE(result_ipset);
        return ipsetWalkIPTree(ipset, 1, SK_IPV6POLICY_MIX,
                               &ipsetSubtractCallback, (void*)result_ipset);
    }
    if (result_ipset->is_iptree) {
        /* only the result_ipset is in SiLK-2 format */
        return skIPSetWalk(ipset, 1, SK_IPV6POLICY_ASV4,
                           &ipsetSubtractCallback, (void*)result_ipset);
    }

    IPSET_COPY_ON_WRITE(result_ipset);

#if SK_ENABLE_IPV6
    if (result_ipset->is_ipv6) {
        if (ipset->is_ipv6) {
            /* both are IPv6 */
            rv = ipsetWalkInternalV6(ipset, ipsetSubtractCallbackV6,
                                     (void*)result_ipset);
        } else {
            rv = skIPSetWalk(ipset, 1, SK_IPV6POLICY_FORCE,
                             &ipsetSubtractCallback, (void*)result_ipset);
        }
    } else if (ipset->is_ipv6) {
        rv = skIPSetWalk(ipset, 1, SK_IPV6POLICY_ASV4,
                         &ipsetSubtractCallback, (void*)result_ipset);
    } else
#endif
    {
        /* both are IPv4 */
        rv = ipsetWalkInternalV4(ipset, ipsetSubtractCallbackV4,
                                 (void*)result_ipset);
    }
    if (rv) {
        return rv;
    }
    IPSET_MAYBE_COMBINE(result_ipset);
    return rv;
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

    if (ipset->is_iptree) {
        if (result_ipset->is_iptree) {
            /* both are in the SiLK-2 format (IPTree) */
            result_ipset->is_dirty = 1;
            rv = skIPTreeUnion(result_ipset->s.v2, ipset->s.v2);
            if (SKIP_ERR_ALLOC == rv) {
                return SKIPSET_ERR_ALLOC;
            }
            return SKIPSET_OK;
        }
        /* result_ipset is SiLK-3 and other is SiLK-2.  Walk over the
         * entries of the SiLK-2 IPset and add to the SiLK-3 IPset. */
        IPSET_COPY_ON_WRITE(result_ipset);
        return ipsetWalkIPTree(ipset, 1, SK_IPV6POLICY_MIX,
                               &ipsetUnionCallback, (void*)result_ipset);
    }
    if (result_ipset->is_iptree) {
        /* only the result_ipset is in SiLK-2 format */
#if !SK_ENABLE_IPV6
        return ipsetWalkInternalV4(ipset, ipsetUnionCallbackIPTree,
                                   (void*)result_ipset);
#else
        if (!skIPSetContainsV6(ipset)) {
            if (ipset->is_ipv6) {
                return skIPSetWalk(ipset, 1, SK_IPV6POLICY_ASV4,
                                   &ipsetUnionCallback, (void*)result_ipset);
            }
            return ipsetWalkInternalV4(ipset, ipsetUnionCallbackIPTree,
                                       (void*)result_ipset);
        }
        if (result_ipset->no_autoconvert) {
            return SKIPSET_ERR_IPV6;
        }
        rv = ipsetConvertIPTreetoV6(result_ipset);
        if (rv) {
            return rv;
        }
#endif  /* #else of #if !SK_ENABLE_IPV6 */
    }

    if (result_ipset->no_autoconvert && !result_ipset->is_ipv6
        && skIPSetContainsV6(ipset))
    {
        return SKIPSET_ERR_IPV6;
    }
    IPSET_COPY_ON_WRITE(result_ipset);

#if SK_ENABLE_IPV6
    if (result_ipset->is_ipv6 || ipset->is_ipv6) {
        if (result_ipset->is_ipv6 == ipset->is_ipv6) {
            /* both are IPv6 */
            rv = ipsetWalkInternalV6(ipset, ipsetUnionCallbackV6,
                                     (void*)result_ipset);
        } else {
            rv = skIPSetWalk(ipset, 1, SK_IPV6POLICY_FORCE,
                             &ipsetUnionCallback, (void*)result_ipset);
        }
    } else
#endif
    {
        /* both are IPv4 */
        rv = ipsetWalkInternalV4(ipset, ipsetUnionCallbackV4,
                                 (void*)result_ipset);
    }
    if (rv) {
        return rv;
    }
    IPSET_MAYBE_COMBINE(result_ipset);
    return rv;
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
    ipset_walk_t walk_state;
    skipaddr_t ipaddr;
    uint32_t prefix;
    int rv;

    if (!ipset) {
        return SKIPSET_ERR_BADINPUT;
    }

    if (ipset->is_iptree) {
        return ipsetWalkIPTree(ipset, cidr_blocks, v6_policy,
                               callback, cb_data);
    }

    if (IPSET_ISEMPTY(ipset)) {
        return SKIPSET_OK;
    }

    /* use the iterator if we can */
    if (!ipset->is_dirty) {
        skipset_iterator_t iter;

        rv = skIPSetIteratorBind(&iter, ipset, cidr_blocks, v6_policy);
        while (rv == 0
               && (skIPSetIteratorNext(&iter, &ipaddr, &prefix)
                   == SK_ITERATOR_OK))
        {
            rv = callback(&ipaddr, prefix, cb_data);
        }
        return rv;
    }

    memset(&walk_state, 0, sizeof(ipset_walk_t));
    walk_state.cidr_blocks = (uint8_t)(cidr_blocks ? 1 : 0);
    walk_state.v6policy    = v6_policy;
    walk_state.callback    = callback;
    walk_state.cb_data     = cb_data;

#if SK_ENABLE_IPV6
    if (ipset->is_ipv6) {
        if (v6_policy == SK_IPV6POLICY_IGNORE) {
            /* caller wants only IPv4 addresses, and there are none in
             * an IPv6 IPset. */
            return SKIPSET_OK;
        }
        return ipsetWalkV6(ipset, &walk_state);
    }
    if (v6_policy == SK_IPV6POLICY_ONLY) {
        /* caller wants only IPv6 addresses, and there are none in an
         * IPv4 IPset. */
        return SKIPSET_OK;
    }
    return ipsetWalkV4(ipset, &walk_state);

#else  /* #if SK_ENABLE_IPV6 */

    if (ipset->is_ipv6) {
        /* impossible to have an IPv6 set */
        skAbort();
    }
    if (v6_policy > SK_IPV6POLICY_MIX) {
        /* caller wants IPv6 addresses, which are not supported in
         * IPv4-only SiLK */
        return SKIPSET_OK;
    }
    return ipsetWalkV4(ipset, &walk_state);

#endif  /* #else of #if SK_ENABLE_IPV6 */
}


int
skIPSetWrite(
    const skipset_t    *ipset,
    skstream_t         *stream)
{
    const skipset_options_t *opts;
    fileVersion_t record_version = 0;
    sk_file_header_t *hdr;
    ssize_t rv;

    if (!ipset || !stream) {
        return SKIPSET_ERR_BADINPUT;
    }

    /* do not write an unclean ipset */
    if (ipset->is_dirty) {
        return SKIPSET_ERR_REQUIRE_CLEAN;
    }

    opts = ipset->options;

    /* Use the version that is explicitly requested.  If none
     * requested, use LEGACY (Version-2) for IPv4 IPsets and
     * IPV6_DEFAULT (Version-3) for IPv6 IPsets. */
    if (NULL == opts
        || IPSET_REC_VERSION_DEFAULT == opts->record_version)
    {
        if (skIPSetContainsV6(ipset)) {
            record_version = IPSET_REC_VERSION_IPV6_DEFAULT;
        } else {
            record_version = IPSET_REC_VERSION_LEGACY;
        }
    } else if (skIPSetContainsV6(ipset)) {
        switch (opts->record_version) {
          case IPSET_REC_VERSION_LEGACY:
            /* Cannot write an IPv6 IPset into a legacy IPset file */
            return SKIPSET_ERR_IPV6;
          case IPSET_REC_VERSION_RADIX:
          case IPSET_REC_VERSION_CIDR_BMAP:
            record_version = opts->record_version;
            break;
          default:
            break;
        }
    } else {
        switch (opts->record_version) {
          case IPSET_REC_VERSION_LEGACY:
          case IPSET_REC_VERSION_RADIX:
          case IPSET_REC_VERSION_CIDR_BMAP:
            record_version = opts->record_version;
            break;
          default:
            break;
        }
    }
    if (0 == record_version) {
        return SKIPSET_ERR_BADINPUT;
    }

    /* prep the header */
    hdr = skStreamGetSilkHeader(stream);
    skHeaderSetFileFormat(hdr, FT_IPSET);
    skHeaderSetRecordVersion(hdr, record_version);
    skHeaderSetRecordLength(hdr, 1);

    if (opts) {
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

    if (IPSET_REC_VERSION_LEGACY == record_version) {
        return ipsetWriteLegacy(ipset, stream);
    }
    if (IPSET_REC_VERSION_RADIX == record_version) {
        return ipsetWriteRadix(ipset, stream);
    }
    if (IPSET_REC_VERSION_CIDR_BMAP == record_version) {
        return ipsetWriteCidrbmap(ipset, stream);
    }

    skAbort();
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
