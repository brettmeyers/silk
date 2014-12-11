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
**  iptree.h
**
**    Michael Collins
**    May 6th, 2003
**
**    This is a tree structure for ip addresses containing a bitmap of
**    ip addresses.
**
*/
#ifndef _IPTREE_H
#define _IPTREE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_IPTREE_H, "$SiLK: iptree.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/silk_types.h>

/**
 *  @file
 *
 *    An skIPTree is the form of the IPset used in SiLK versions prior
 *    to SiLK-3.0.
 *
 *    This file is part of libsilk.
 */


/*
 * Now we define our basic tree types.
 */
#define SKIP_BBLOCK_COUNT 65536
#define SKIP_BBLOCK_SIZE 2048

/**
 *    Return values for skIPTree* functions.
 */
typedef enum skIPTreeErrors_en {
    /**  Success */
    SKIP_OK = 0,
    /**  Unable to allocate memory */
    SKIP_ERR_ALLOC,
    /**  Empty input value */
    SKIP_ERR_BADINPUT,
    /**  Error in read/write */
    SKIP_ERR_FILEIO,
    /**  Input is not an IPset */
    SKIP_ERR_FILETYPE,
    /**  Input IPset is not empty */
    SKIP_ERR_NONEMPTY,
    /**  Error opening file */
    SKIP_ERR_OPEN,
    /**  IPsets do not support IPv6 addresses */
    SKIP_ERR_IPV6,
    /**  This application does not support the new IPset file format */
    SKIP_ERR_FILEVERSION
} skIPTreeErrors_t;


/*
 *  IPSet
 *
 *    An IPSet (aka IP-Tree) consists of a 64k pointers, each of which
 *    is either NULL or a pointer to a node; a node contains 64k bits
 *    for marking addresses.  Dividing 64k by 32 means there are 2k
 *    addressBlocks, each of which represents a /27.
 */
typedef struct skIPNode_st skIPNode_t;

/* THE TREE */
typedef struct skIPTree_st {
    skIPNode_t *nodes[SKIP_BBLOCK_COUNT];
} skIPTree_t;

/* A NODE */
struct skIPNode_st {
    uint32_t    addressBlock[SKIP_BBLOCK_SIZE];
};

/**
 *    INTERNAL.  Check for bit 'lower16' in 'node'.  The value in
 *    'lower16' should be the 16 least significant bits of an address;
 *    'node' is the skIPNode_t for the 16 most significant bits of the
 *    address.
 */
#define skIPTreeNodeHasMark(lower16, node)                              \
    ((node)->addressBlock[(lower16) >> 5] & (1 << ((lower16) & 0x1F)))


/**
 *    Return TRUE if 'ipAddr' is set on tree 'ipset'; FALSE otherwise
 */
#define skIPTreeCheckAddress(ipset, addr)                               \
    ((ipset)->nodes[((addr) >> 16)]                                     \
     && skIPTreeNodeHasMark(((addr) & 0xFFFF),(ipset)->nodes[((addr) >> 16)]))


/**
 *    An iterator structure for looping over the elements in an IPset.
 */
typedef struct skIPTreeIterator_st {
    const skIPTree_t   *tree;
    uint32_t            top_16;
    uint16_t            mid_11;
    uint16_t            bot_5;
} skIPTreeIterator_t;

/**
 *    The skIPTreeCIDRBlockIterator_t iterates over CIDR blocks and
 *    fills an skIPTreeCIDRBlock_t.
 */
typedef struct skIPTreeCIDRBlockIterator_st {
    /**  the underlying iterator over the IPs */
    skIPTreeIterator_t  tree_iter;
    /**  the number of /27s yet to be returned */
    uint32_t            count;
    /**  the maximum number of trailing 0s; used to set the CIDR mask.
     *  'count' is always <= (1 << (trail_zero - 5)) */
    uint32_t            trail_zero;
    /**  the starting address for the next cidr block */
    uint32_t            base_ip;
} skIPTreeCIDRBlockIterator_t;

typedef struct skIPTreeCIDRBlock_st {
    uint32_t addr;
    uint32_t mask;
} skIPTreeCIDRBlock_t;



/* FUNCTION PROTOTYPES */


/**
 *    Add the IP Address 'addr' to the binary IPSet 'ipset'.  Returns
 *    SKIP_OK for success, or SKIP_ERR_ALLOC if there is not enough
 *    memory to allocate space for the new IP address.
 */
int
skIPTreeAddAddress(
    skIPTree_t         *ipset,
    uint32_t            addr);


/**
 *    Add all the addresses in the IPWildcard 'ipwild' to the IPset
 *    'ipset'.
 *
 *    When 'ipwild' contains IPv6 addresses, map each address within
 *    the ::FFFF:0:0/96 netblock to an IPv4 address and add it to
 *    'ipset'.  This behavior is new as of SiLK 3.9.0.  In prior
 *    releases of SiLK, the entire contents of the wildcard was
 *    ignored and SKIP_ERR_IPV6 was returned when 'ipwild' contained
 *    any IPv6 addresses.
 *
 *    Returns SKIP_OK for success or SKIP_ERR_ALLOC if there is a
 *    memory allocation error.
 */
int
skIPTreeAddIPWildcard(
    skIPTree_t             *ipset,
    const skIPWildcard_t   *ipwild);


/**
 *    Return 1 if the IPsets 'ipset1' and 'ipset2' have any IPs in
 *    common; otherwise, return 0.
 */
int
skIPTreeCheckIntersectIPTree(
    const skIPTree_t   *ipset1,
    const skIPTree_t   *ipset2);


/**
 *    Return 1 if the IPset 'ipset' and IPWildcard 'ipwild' have any
 *    IPs in common; otherwise, return 0.
 *
 *    When 'ipwild' contains IPv6 addresses, map each address within
 *    the ::FFFF:0:0/96 netblock to an IPv4 address and determine if
 *    it appears in 'ipset'.  This behavior is new as of SiLK 3.9.0.
 *    In prior relases of SiLK, the entire contents of the wildcard
 *    was ignored and 0 was returned when 'ipwild' contained any IPv6
 *    addresses.
 */
int
skIPTreeCheckIntersectIPWildcard(
    const skIPTree_t       *ipset,
    const skIPWildcard_t   *ipwild);


/**
 *    Like skIPTreeCheckIntersectIPTree(), except compares the in-core
 *    IPset 'ipset' with the IPset contained in the file 'ipset_path'.
 *    Returns 1 if the IPsets have an IPs in common; returns 0 if they
 *    do not or to signify an error, in which case the error code will
 *    be put into the memory referenced 'err_code', if that value is
 *    non-NULL.
 */
int
skIPTreeCheckIntersectIPTreeFile(
    const skIPTree_t   *ipset,
    const char         *ipset_path,
    skIPTreeErrors_t   *err_code);


/**
 *    Returns a count of the number of IP addresses marked in the
 *    'ipset'.
 */
uint64_t
skIPTreeCountIPs(
    const skIPTree_t   *ipset);


/**
 *    Allocation and creation function; initializes a new ipset at the
 *    space specified by '*ipset' and sets the contents to empty.
 *
 *    Returns SKIP_OK if everything went well, else SKIP_ERR_ALLOC
 *    on a malloc failure, or SKIP_ERR_BADINPUT if ipset was NULL.
 *
 *    skIPTreeDelete() is the corresponding free function.
 */
int
skIPTreeCreate(
    skIPTree_t        **ipset);


/**
 *    Frees the space associated with *ipset.  On completion
 *    *ipset == NULL
 */
void
skIPTreeDelete(
    skIPTree_t        **ipset);


/**
 *    Perform an intersection of 'result_ipset' and 'ipset', with the
 *    result in the 'result_ipset'; i.e., turn off all addresses in
 *    'result_ipset' that are off in 'ipset'.
 */
void
skIPTreeIntersect(
    skIPTree_t         *result_ipset,
    const skIPTree_t   *ipset);


/**
 *    Create(allocate) a new IPset at the location pointed at by
 *    'ipset' and fill it with the data that the function reads from
 *    the file 'filename'.
 *
 *    This function is similar to skIPTreeRead(), except that this
 *    function will create the skstream_t from the specified filename.
 *
 *    In addition to the possible return values listed for
 *    skIPTreeRead(), this function may return:
 *      SKIP_ERR_OPEN - error in opening the file
 */
int
skIPTreeLoad(
    skIPTree_t        **ipset,
    const char         *filename);


/**
 *    Modify in place the specified 'ipset' so it contains at most 1
 *    IP address for every block of bitmask length 'mask', a value
 *    from 1 to 32.  If the 'ipset' has any IP active within each
 *    block, all IPs in that block are turned off except for the IP at
 *    the start of the block.
 *
 *    Specify mask==16 with an IPset containing these IPs:
 *        10.0.0.23
 *        10.0.1.0/24
 *        10.7.1.0/24
 *        20.20.0.243
 *    produces an IPset with these three IPs:
 *        10.0.0.0
 *        10.7.0.0
 *        20.20.0.0
 */
void
skIPTreeMask(
    skIPTree_t         *ipset,
    uint32_t            mask);


/**
 *    Print, to the stream 'stream', a textual representation of the
 *    IPset given by 'ipset'.  The parameter 'ip_format' decribes how
 *    to print the ipset (see utils.h).  If 'as_cidr' is non-zero, the
 *    output will be in CIDR notation.
 */
void
skIPTreePrint(
    const skIPTree_t   *ipset,
    skstream_t         *stream,
    skipaddr_flags_t    ip_format,
    int                 as_cidr);


/**
 *    Allocate a new IPset at the location pointed at by 'ipset' and
 *    fill it with the data that the function reads from the stream
 *    'stream'.  'stream' should be bound to a file and open.
 *
 *    Returns one of the following:
 *      SKIP_OK on success
 *      SKIP_ERR_BADINPUT - one of the input values is NULL
 *      SKIP_ERR_ALLOC  - failure to allocate memory.
 *      SKIP_ERR_NONEMPTY - the '*ipset' is not empty
 *      SKIP_ERR_FILETYPE - the file is not of the correct type
 *      SKIP_ERR_FILEIO   - error during a read call
 *
 *    On failure, 'ipset' is set back to null and deleted.
 */
int
skIPTreeRead(
    skIPTree_t        **ipset,
    skstream_t         *stream);


/**
 *    Remove all IPs from an IPset.
 */
int
skIPTreeRemoveAll(
    skIPTree_t         *ipset);


/**
 *    Write the IPset at 'ipset' to the disk file 'filename'; the
 *    disk format is specified in iptree.api.
 *
 *    This function is similar to skIPTreeWrite(), except this
 *    function writes directly to a file using the default compression
 *    method.
 *
 *    In addition to the possible return values listed for
 *    skIPTreeWrite(), this function may return:
 *      SKIP_ERR_OPEN - error in opening the file
 */
int
skIPTreeSave(
    const skIPTree_t   *ipset,
    const char         *filename);


/**
 *    Return a text string describing 'err_code'.
 */
const char *
skIPTreeStrError(
    int                 err_code);


/**
 *    Subtract 'ipset' from 'result_ipset' with the result in the
 *    'result_ipset'; i.e., if an address is off in 'ipset', do not
 *    modify the value in 'result_ipset', otherwise, turn off that
 *    address in 'result_ipset'.
 */
void
skIPTreeSubtract(
    skIPTree_t         *result_ipset,
    const skIPTree_t   *ipset);


/**
 *    Perform the union of 'result_ipset' and 'ipset', with the result
 *    in 'result_ipset'; i.e., merge 'ipset' into 'result_ipset'.
 *    Returns 0 on success, or 1 on memory allocation error.
 */
int
skIPTreeUnion(
    skIPTree_t         *result_ipset,
    const skIPTree_t   *ipset);


/**
 *   Write the IPset at 'ipset' the output stream 'stream'.  'stream'
 *   should be bound to a file and open.  The caller may set the
 *   compression method of the stream before calling this function.
 *   If not set, the default compression method is used.
 *
 *   Returns one of the following values:
 *        SKIP_OK          on success.
 *        SKIP_ERR_OPEN    if the file already exists.
 *        SKIP_ERR_FILEIO  if there's an error writing the data to disk.
 *        SKIP_ERR_ALLOC   on a memory allocatin problem for the ipset.
 */
int
skIPTreeWrite(
    const skIPTree_t   *ipset,
    skstream_t         *stream);



/*
 *   Iteration over the members of an IPset
 */


/**
 *    Bind the IPset iterator 'iter' to iterate over all the entries
 *    in the IPSet 'ipset'.  Return 0 on success, non-zero otherwise.
 */
int
skIPTreeIteratorBind(
    skIPTreeIterator_t *iter,
    const skIPTree_t   *ipset);


/**
 *    Create a new iterator at the address pointed to by 'out_iter'
 *    and bind it to iterate over all the entries in the IPSet
 *    'ipset'.  Return 0 on success, non-zero otherwise.
 */
int
skIPTreeIteratorCreate(
    skIPTreeIterator_t    **out_iter,
    const skIPTree_t       *ipset);


/**
 *    Destroy the iterator pointed to by 'out_iter'.
 */
void
skIPTreeIteratorDestroy(
    skIPTreeIterator_t    **out_iter);


/**
 *    If there are more entries in the IPSet, put the
 *    next IP Address into the location referenced by 'out_addr' and
 *    return SK_ITERATOR_OK.  Otherwise, to not  modify 'out_addr'
 *    and return SK_ITERATOR_NO_MORE_ENTRIES.
 */
skIteratorStatus_t
skIPTreeIteratorNext(
    uint32_t           *out_addr,
    skIPTreeIterator_t *iter);


/**
 *    Reset the iterator 'iter' to begin looping through the entries
 *    in the IPSet again.
 */
void
skIPTreeIteratorReset(
    skIPTreeIterator_t *iter);



/*
 *   Iteration over the CIDR Blocks of an IPset
 */


/**
 *    Bind the IPset CIDR Block iterator 'iter' to iterate over all
 *    the CIDR blocks in the IPSet 'ipset'.  Return 0 on success,
 *    non-zero otherwise.
 */
int
skIPTreeCIDRBlockIteratorBind(
    skIPTreeCIDRBlockIterator_t    *iter,
    const skIPTree_t               *ipset);


/**
 *    Create a new CIDR Block iterator at the address pointed to by
 *    'out_iter' and bind it to iterate over all the CIDR Blocks in
 *    the IPSet 'ipset'.  Return 0 on success, non-zero otherwise.
 */
int
skIPTreeCIDRBlockIteratorCreate(
    skIPTreeCIDRBlockIterator_t   **out_iter,
    const skIPTree_t               *ipset);


/**
 *    Destroy the iterator pointed to by 'out_iter'.
 */
void
skIPTreeCIDRBlockIteratorDestroy(
    skIPTreeCIDRBlockIterator_t   **out_iter);


/**
 *    If there are more CIDR Blocks in the IPSet, fill
 *    next CIDR Block pointer 'out_cidr' with that CIDR Block and
 *    return SK_ITERATOR_OK.  Otherwise, do not modify 'out_cidr'
 *    and return SK_ITERATOR_NO_MORE_ENTRIES.
 */
skIteratorStatus_t
skIPTreeCIDRBlockIteratorNext(
    skIPTreeCIDRBlock_t            *out_cidr,
    skIPTreeCIDRBlockIterator_t    *iter);


/**
 *    Reset the iterator 'iter' to begin looping through the entries
 *    in the IPSet again.
 */
void
skIPTreeCIDRBlockIteratorReset(
    skIPTreeCIDRBlockIterator_t    *iter);


#ifdef __cplusplus
}
#endif
#endif /* _IPTREE_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
