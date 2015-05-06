/*
** Copyright (C) 2004-2015 by Carnegie Mellon University.
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
**  skprefixmap.c
**
**  John McClary Prevost
**  December 2nd, 2004
**
**    Data structure for binding values to IP address prefixes (for
**    example, mapping the block 128.2.0.0/16 to a single value.)
**
**    A prefix map is a binary tree of records; the key is an IP
**    address (or a protocol/port pair) where the depth of the tree
**    determines which bit of IP address (or protocol/port pair) is
**    being considered.  For example, at the root of tree, the most
**    significant bit (MSB) of the IP address is considered; at the
**    next level, the second MSB is considered; etc.
**
**    The records in the tree are stored as an array, and each record
**    contains a pair of numbers called 'left' and 'right'.  These
**    members are either indexes to other entries in the array or
**    indexes into a dictionary of labels.  Which value the member
**    contains depends on the MSB of the member, as described below.
**
**    At an arbitrary level N in the tree, the Nth bit of the key is
**    considered.  When that Nth bit is low, the 'left' member of the
**    record is used.  When the bit is high, the 'right' member of the
**    record is used.
**
**    At level N in the tree, the MSB of the 'left' or 'right' member
**    being low indicates the member contains the array index for the
**    next level, N+1, of the tree.  The MSB of the member being high
**    indicates that the lower bits (that is, all bits except the MSB)
**    represent the dictionary index for a CIDR Block of size N.
**
**    (The country code prefix map does not have a dictionary.  Here,
**    the value contains the ASCII representation of the two letters
**    that make up the country code.)
**
**    The bit size of the 'left' and 'right' members of the tree limit
**    the number of unique blocks that can be assigned in the prefix
**    map.  As of June 2011, the bit size size 32 bits, which limits
**    the prefix map to 2^31 blocks.
**
**
**    The following file formats exist:
**
**    Version 1: Key is an IPv4 address.  There is no dictionary, and
**    the value represents the country code.
**
**    Version 2: (SiLK-0.9.10) Key is an IPv4 address and value is an
**    index into the dictionary.
**
**    Version 3: (SiLK-0.9.10) Key is comprised of ((protocol << 16) |
**    port) and value is an index into the dictionary.
**
**    Version 4: (SiLK-3.0.0) Key is an IPv6 address and value is an
**    index into the dictionary.  For an IPv6 prefix map, the data
**    structure is the same; however, the maximum depth is 128 instead
**    of 32.
**
**    Version 5: (SiLK-3.5.0) Key is an IPv6 address.  There is no
**    dictionary, and the value represents the country code.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skprefixmap.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skprefixmap.h>
#include <silk/skipaddr.h>
#include <silk/skstream.h>
#include <silk/rwrec.h>
#include <silk/utils.h>
#include <silk/redblack.h>
#include <silk/skmempool.h>


/* DEFINES AND TYPEDEFS */

typedef struct skPrefixMapRecord_st {
    uint32_t left;
    uint32_t right;
} skPrefixMapRecord_t;

struct skPrefixMap_st {
    /* the nodes that make up the tree */
    skPrefixMapRecord_t    *tree;
    /* the name of the map */
    char                   *mapname;
    /* all terms in dictionary joined by '\0', or NULL for vers 1,5 */
    char                   *dict_buf;
    /* Pointers into 'dict_buf', one for each word, or NULL for vers 1,5 */
    char                  **dict_words;
    /* number of nodes in the tree that are in use */
    uint32_t                tree_used;
    /* number of nodes in the tree */
    uint32_t                tree_size;
    /* size of dictionary as it would be written to disk, or 0 for vers 1,5 */
    uint32_t                dict_buf_used;
    /* number of bytes in dictionary that are in use, or 0 for vers 1,5 */
    uint32_t                dict_buf_end;
    /* number of bytes allocated to dictionary, or 0 for vers 1,5 */
    uint32_t                dict_buf_size;
    /* number of words in dictionary, or 0 for vers 1,5 */
    uint32_t                dict_words_used;
    /* number of possible words in dictionary, or 0 for vers 1,5 */
    uint32_t                dict_words_size;
    /* max len of word in dictionary, or 0 for vers 1,5 */
    uint32_t                dict_max_wordlen;
    /* map of dictionary words to values */
    struct rbtree          *word_map;
    /* Memory pool for word map entries */
    sk_mempool_t           *word_map_pool;
    /* type of data in the map */
    skPrefixMapContent_t    content_type;
};

/* Used as the nodes in the word map dictionary */
typedef struct skPrefixMapDictNode_st {
    const char *word;
    uint32_t    value;
} skPrefixMapDictNode_t;

/* if the high bit of a value is set, the value is a leaf */
#define SKPMAP_LEAF_BIT        UINT32_C(0x80000000)
#define SKPMAP_IS_LEAF(x)      (x & SKPMAP_LEAF_BIT)
#define SKPMAP_IS_NODE(x)      (!SKPMAP_IS_LEAF(x))
#define SKPMAP_LEAF_VALUE(x)   (x & ~SKPMAP_LEAF_BIT)
#define SKPMAP_MAKE_LEAF(val)  ((val) | SKPMAP_LEAF_BIT)


/* The initial sizes of and how quickly to grow the (1)tree of nodes,
 * the (2)dictionary buffer, and the (3)dictionary words index */
#define SKPMAP_TREE_SIZE_INIT      (1 << 14)
#define SKPMAP_TREE_SIZE_GROW      SKPMAP_TREE_SIZE_INIT

#define SKPMAP_DICT_BUF_INIT       (1 << 16)
#define SKPMAP_DICT_BUF_GROW       SKPMAP_DICT_BUF_INIT

#define SKPMAP_WORDS_COUNT_INIT    8192
#define SKPMAP_WORDS_COUNT_GROW    2048


#define SKPMAP_KEY_FROM_PROTO_PORT(kfpp)                \
    ((((skPrefixMapProtoPort_t*)(kfpp))->proto << 16)   \
     | ((skPrefixMapProtoPort_t*)(kfpp))->port)

/* Return the value (0 or 1) of the 'cb_bit' in 'cb_key', where the
 * 0th bit is the least significant, and the 31st is the most
 * significant. */
#define SKPMAP_GET_BIT32(cb_key, cb_bit)        \
    GET_MASKED_BITS((cb_key), (cb_bit), 1)

/* Return 1 if the least significant 'bbz_bits' of 'bbz_key' are all
 * 0; return 0 otherwise. */
#define SKPMAP_CHECK_BOTTOM_BITS_ZERO32(bbz_key, bbz_bits)      \
    (0 == GET_MASKED_BITS((bbz_key), 0, (bbz_bits)))

/* Return 1 if the least significant 'bbz_bits' of 'bbz_key' are all
 * 1; return 0 otherwise. */
#define SKPMAP_CHECK_BOTTOM_BITS_ONE32(bbo_key, bbo_bits)               \
    (GET_MASKED_BITS((bbo_key), 0, (bbo_bits)) == _BITMASK(bbo_bits))


#define SKPMAP_GET_BIT128(cb_key, cb_bit)                               \
    (0x1 & ((cb_key)[15 - ((cb_bit) >> 3)] >> ((cb_bit) & 0x7)))

#define SKPMAP_CHECK_BOTTOM_BITS_ZERO128(bbz_key, bbz_bits)             \
    ((0 == GET_MASKED_BITS((bbz_key)[15 - ((bbz_bits) >> 3)], 0,        \
                           ((bbz_bits) & 0x7)))                         \
     && (0 == memcmp(&(bbz_key)[16 - ((bbz_bits) >> 3)], min_ip128,     \
                     ((bbz_bits) >> 3))))

#define SKPMAP_CHECK_BOTTOM_BITS_ONE128(bbo_key, bbo_bits)         \
    ((GET_MASKED_BITS((bbo_key)[15 - ((bbo_bits) >> 3)], 0,             \
                      ((bbo_bits) & 0x7)) == _BITMASK((bbo_bits) & 0x7)) \
     && (0 == memcmp(&(bbo_key)[16 - ((bbo_bits) >> 3)], max_ip128,     \
                     ((bbo_bits) >> 3))))


/* LOCAL VARIABLES */

#if SK_ENABLE_IPV6
static const uint8_t min_ip128[16] = {0,0,0,0,0,0,0,0,
                                      0,0,0,0,0,0,0,0};
static const uint8_t max_ip128[16] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
#endif


/* LOCAL FUNCTION PROTOTYPES */

static skPrefixMapErr_t
prefixMapGrowDictionaryBuff(
    skPrefixMap_t      *map,
    size_t              min_bytes);
static skPrefixMapErr_t
prefixMapGrowDictionaryWords(
    skPrefixMap_t      *map,
    size_t              min_entries);
static skPrefixMapErr_t
prefixMapGrowTree(
    skPrefixMap_t      *map);
static int
prefixMapWordCompare(
    const void         *va,
    const void         *vb,
    const void         *ctx);


/* FUNCTION DEFINITIONS */

/*
 *  err = prefixMapAdd32(map, low, high, dict_val, node_idx, bit);
 *
 *    Add the range 'low' to 'high' to the prefix map 'map' with a
 *    value of 'dict_val'.  This function is recursive.
 */
static skPrefixMapErr_t
prefixMapAdd32(
    skPrefixMap_t      *map,
    uint32_t            low_val,
    uint32_t            high_val,
    uint32_t            dict_val,
    uint32_t            node_idx,
    uint32_t            bit)
{
    uint32_t masked;
    skPrefixMapErr_t err;

    assert(low_val <= high_val);
    assert(bit < 32);

    /* Does the left subtree overlap [low_val,high_val] ? */
    if (0 == SKPMAP_GET_BIT32(low_val, bit)) {
        /* Yes, left tree overlaps */
        /*
         * Is the left subtree completely contained in
         * [low_val,high_val] ?
         *
         * That is, is low_val equal to the lower bound of the left
         * subtree?  And, is the high_val either equal to the high
         * bound of the left subtree or located somewhere in the right
         * subtree?
         */
        if (SKPMAP_CHECK_BOTTOM_BITS_ZERO32(low_val, bit)
            && ((1 == SKPMAP_GET_BIT32(high_val, bit))
                || SKPMAP_CHECK_BOTTOM_BITS_ONE32(high_val, bit)))
        {
            /* Left subtree completely contained; set left subtree to
             * the dictionary value. */
            map->tree[node_idx].left = dict_val;
        } else {
            /* Left subtree overlaps but is not completely contained. */
            /* If left subtree is a leaf, we need to break it into two
             * subrecords */
            if (SKPMAP_IS_LEAF(map->tree[node_idx].left)) {
                if (map->tree_used == map->tree_size) {
                    if (prefixMapGrowTree(map)) {
                        return SKPREFIXMAP_ERR_MEMORY;
                    }
                }
                map->tree[map->tree_used].left = map->tree[node_idx].left;
                map->tree[map->tree_used].right = map->tree[node_idx].left;
                map->tree[node_idx].left = map->tree_used;
                ++map->tree_used;
            }

            /* Recurse down the tree.  */
            if (1 == SKPMAP_GET_BIT32(high_val, bit)) {
                /* set least significant 'bit' bits to 1 */
                masked = low_val;
                SET_MASKED_BITS(masked, UINT32_MAX, 0, bit);
                err = prefixMapAdd32(map, low_val, masked, dict_val,
                                     map->tree[node_idx].left, bit-1);
            } else {
                err = prefixMapAdd32(map, low_val, high_val, dict_val,
                                     map->tree[node_idx].left, bit-1);
            }
            if (err != SKPREFIXMAP_OK) {
                return err;
            }
        }
    }


    /* NOW, handle the right-hand side, a mirror image */

    /* Does the right subtree overlap [low_val,high_val] ? */
    if (1 == SKPMAP_GET_BIT32(high_val, bit)) {
        /* Yes, right tree overlaps */
        /*
         * Is the right subtree completely contained in
         * [low_val,high_val] ?
         *
         * That is, is high_val equal to the upper bound of the right
         * subtree?  And, is low_val either equal to the lower bound
         * of the right subtree or located somewhere in the left
         * subtree?
         */
        if (SKPMAP_CHECK_BOTTOM_BITS_ONE32(high_val, bit)
            && ((0 == SKPMAP_GET_BIT32(low_val, bit))
                || SKPMAP_CHECK_BOTTOM_BITS_ZERO32(low_val, bit)))
        {
            /* Right subtree completely contained; set right subtree
             * to the dictionary value. */
            map->tree[node_idx].right = dict_val;
        } else {
            /* Right subtree overlaps but is not completely contained. */
            /* If right subtree is a leaf, we need to break it into
             * two subrecords */
            if (SKPMAP_IS_LEAF(map->tree[node_idx].right)) {
                if (map->tree_used == map->tree_size) {
                    if (prefixMapGrowTree(map)) {
                        return SKPREFIXMAP_ERR_MEMORY;
                    }
                }
                map->tree[map->tree_used].left = map->tree[node_idx].right;
                map->tree[map->tree_used].right = map->tree[node_idx].right;
                map->tree[node_idx].right = map->tree_used;
                ++map->tree_used;
            }

            /* Recurse down the tree.  */
            if (0 == SKPMAP_GET_BIT32(low_val, bit)) {
                /* set least significant 'bit' bits to 0 */
                masked = high_val;
                SET_MASKED_BITS(masked, 0, 0, bit);
                err = prefixMapAdd32(map, masked, high_val, dict_val,
                                     map->tree[node_idx].right, bit-1);
            } else {
                err = prefixMapAdd32(map, low_val, high_val, dict_val,
                                     map->tree[node_idx].right, bit-1);
            }
            if (err != SKPREFIXMAP_OK) {
                return err;
            }
        }
    }

    return SKPREFIXMAP_OK;
}

#if SK_ENABLE_IPV6
/*
 *  err = prefixMapAdd128(map, low, high, dict_val, node_idx, bit);
 *
 *    Add the range 'low' to 'high' to the prefix map 'map' with a
 *    value of 'dict_val'.  This function is recursive.
 */
static skPrefixMapErr_t
prefixMapAdd128(
    skPrefixMap_t      *map,
    const uint8_t       low_val[],
    const uint8_t       high_val[],
    uint32_t            dict_val,
    uint32_t            node_idx,
    uint32_t            bit)
{
    uint8_t masked[16];
    skPrefixMapErr_t err;

    assert(bit < 128);

    /* Does the left subtree overlap [low_val,high_val] ? */
    if (0 == SKPMAP_GET_BIT128(low_val, bit)) {
        /* Yes, left tree overlaps */
        /* Is the left subtree completely contained in
         * [low_val,high_val] ? */
        if (SKPMAP_CHECK_BOTTOM_BITS_ZERO128(low_val, bit)
            && ((1 == SKPMAP_GET_BIT128(high_val, bit))
                || SKPMAP_CHECK_BOTTOM_BITS_ONE128(high_val, bit)))
        {
            /* Left subtree completely contained; set left subtree to
             * the dictionary value. */
            map->tree[node_idx].left = dict_val;
        } else {
            /* Left subtree overlaps but is not completely contained. */
            /* If left subtree is a leaf, we need to break it into two
             * subrecords */
            if (SKPMAP_IS_LEAF(map->tree[node_idx].left)) {
                if (map->tree_used == map->tree_size) {
                    if (prefixMapGrowTree(map)) {
                        return SKPREFIXMAP_ERR_MEMORY;
                    }
                }
                map->tree[map->tree_used].left = map->tree[node_idx].left;
                map->tree[map->tree_used].right = map->tree[node_idx].left;
                map->tree[node_idx].left = map->tree_used;
                ++map->tree_used;
            }
            /* Recurse down the tree.  */
            if (1 == SKPMAP_GET_BIT128(high_val, bit)) {
                /* set least significant 'bit' bits to 1 */
                memcpy(masked, low_val, sizeof(masked));
                SET_MASKED_BITS(masked[15 - (bit >> 3)], 0xFF, 0, bit & 0x7);
                memset(&masked[16 - (bit >> 3)], 0xFF, bit >> 3);
                err = prefixMapAdd128(map, low_val, masked, dict_val,
                                      map->tree[node_idx].left, bit-1);
            } else {
                err = prefixMapAdd128(map, low_val, high_val, dict_val,
                                      map->tree[node_idx].left, bit-1);
            }
            if (err != SKPREFIXMAP_OK) {
                return err;
            }
        }
    }

    /* NOW, handle the right-hand side, a mirror image */
    /* Does the right subtree overlap [low_val,high_val] ? */
    if (1 == SKPMAP_GET_BIT128(high_val, bit)) {
        /* Yes, right tree overlaps */
        /* Is the right subtree completely contained in
         * [low_val,high_val] ? */
        if (SKPMAP_CHECK_BOTTOM_BITS_ONE128(high_val, bit)
            && ((0 == SKPMAP_GET_BIT128(low_val, bit))
                || SKPMAP_CHECK_BOTTOM_BITS_ZERO128(low_val, bit)))
        {
            /* Right subtree completely contained; set right subtree
             * to the dictionary value. */
            map->tree[node_idx].right = dict_val;
        } else {
            /* Right subtree overlaps but is not completely contained. */
            /* If right subtree is a leaf, we need to break it into two
             * subrecords */
            if (SKPMAP_IS_LEAF(map->tree[node_idx].right)) {
                if (map->tree_used == map->tree_size) {
                    if (prefixMapGrowTree(map)) {
                        return SKPREFIXMAP_ERR_MEMORY;
                    }
                }
                map->tree[map->tree_used].left = map->tree[node_idx].right;
                map->tree[map->tree_used].right = map->tree[node_idx].right;
                map->tree[node_idx].right = map->tree_used;
                ++map->tree_used;
            }
            /* Recurse down the tree.  */
            if (0 == SKPMAP_GET_BIT128(low_val, bit)) {
                /* set least significant 'bit' bits to 0 */
                memcpy(masked, low_val, sizeof(masked));
                SET_MASKED_BITS(masked[15 - (bit >> 3)], 0, 0, bit & 0x7);
                memset(&masked[16 - (bit >> 3)], 0, bit >> 3);
                err = prefixMapAdd128(map, masked, high_val, dict_val,
                                      map->tree[node_idx].right, bit-1);
            } else {
                err = prefixMapAdd128(map, low_val, high_val, dict_val,
                                      map->tree[node_idx].right, bit-1);
            }
            if (err != SKPREFIXMAP_OK) {
                return err;
            }
        }
    }
    return SKPREFIXMAP_OK;
}
#endif  /* SK_ENABLE_IPV6 */


/*
 *  err = prefixMapDictionaryInsertHelper(map, pdict_val, word);
 *
 *    Find or insert the text 'word' into the dictionary for 'map'.
 *
 *    Search for 'word' in the dictionary.  If it exists and its value
 *    matches the integer 'pdict_val', return SKPREFIXMAP_OK.  If it
 *    exists and its value does not match 'pdict_val', set 'pdict_val'
 *    to the word's current integer value and return
 *    SKPREFIXMAP_ERR_DUPLICATE.  If it is not found, insert 'word' as
 *    the value for 'pdict_val'.
 *
 *    This helper function is used by skPrefixMapDictionaryInsert()
 *    and skPrefixMapDictionarySearch().
 */
static skPrefixMapErr_t
prefixMapDictionaryInsertHelper(
    skPrefixMap_t      *map,
    uint32_t           *pdict_val,
    const char         *word)
{
    skPrefixMapDictNode_t *word_node;
    const skPrefixMapDictNode_t *found;
    uint32_t dict_val;
    uint32_t val;
    size_t len;

    assert(pdict_val);
    dict_val = *pdict_val;

    if (map == NULL || word == NULL) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    len = strlen(word);
    if (len == 0) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    if (dict_val > SKPREFIXMAP_MAX_VALUE) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    val = skPrefixMapDictionaryLookup(map, word);
    if (val == dict_val) {
        return SKPREFIXMAP_OK;
    }
    if (val != SKPREFIXMAP_NOT_FOUND) {
        *pdict_val = val;
        return SKPREFIXMAP_ERR_DUPLICATE;
    }

    if (dict_val < map->dict_words_used) {
        if (map->dict_words[dict_val]) {
            /* error: new word is not same as value at this entry,
             * else we would have caught it above with
             * skPrefixMapDictionaryLookup() */
            return SKPREFIXMAP_ERR_DUPLICATE;
        }
        /* Subtract out the interior \0 that we will skip during save */
        map->dict_buf_used -= 1;
    } else {
        /* Add in the new interior \0s that we will be adding on save */
        map->dict_buf_used += dict_val - map->dict_words_used;
    }

    /* We add this entry to the end of the dictionary, perhaps
     * far beyond the final value */

    /* account for NUL byte */
    ++len;

    /* grow the dictionary */
    if ((map->dict_buf_size - map->dict_buf_end) < len) {
        if (prefixMapGrowDictionaryBuff(map, len)) {
            return SKPREFIXMAP_ERR_MEMORY;
        }
    }

    /* grow the dict_words */
    if (map->dict_words_size < (1 + dict_val)) {
        if (prefixMapGrowDictionaryWords(map,
                                         1 + dict_val - map->dict_words_size))
        {
            return SKPREFIXMAP_ERR_MEMORY;
        }
    }

    map->dict_words[dict_val] = &(map->dict_buf[map->dict_buf_end]);
    strncpy(map->dict_words[dict_val], word, len);

    map->dict_buf_end += len;
    map->dict_buf_used += len;
    if (dict_val >= map->dict_words_used) {
        map->dict_words_used = 1 + dict_val;
    }

    /* Make sure we have a memory pool */
    if (map->word_map_pool == NULL) {
        if (0 != skMemoryPoolCreate(&map->word_map_pool,
                                    sizeof(skPrefixMapDictNode_t),
                                    SKPMAP_WORDS_COUNT_GROW))
        {
            return SKPREFIXMAP_ERR_MEMORY;
        }
    }

    /* Add the word to the word map */
    word_node = (skPrefixMapDictNode_t*)skMemPoolElementNew(map->word_map_pool);
    if (word_node == NULL) {
        return SKPREFIXMAP_ERR_MEMORY;
    }
    word_node->value = dict_val;
    word_node->word = map->dict_words[dict_val];
    if (map->word_map == NULL) {
        map->word_map = rbinit(prefixMapWordCompare, NULL);
        if (map->word_map == NULL) {
            return SKPREFIXMAP_ERR_MEMORY;
        }
    }
    found = (const skPrefixMapDictNode_t*)rbsearch(word_node, map->word_map);
    if (found == NULL) {
        return SKPREFIXMAP_ERR_MEMORY;
    }
    assert(found == word_node);

    *pdict_val = dict_val;

    return SKPREFIXMAP_OK;
}


/* Return the dict_val for the given key, or SKPREFIXMAP_NOT_FOUND */
static uint32_t
prefixMapFind(
    const skPrefixMap_t    *map,
    const void             *key,
    int                    *depth)
{
    uint32_t key32;
    uint32_t node = 0;          /* Start at the root node */

    switch (map->content_type) {
#if SK_ENABLE_IPV6
      case SKPREFIXMAP_CONT_ADDR_V6:
        {
            uint8_t key128[16];

            if (skipaddrIsV6((const skipaddr_t*)key)) {
                skipaddrGetV6((const skipaddr_t*)key, key128);
            } else {
                skipaddrGetAsV6((const skipaddr_t*)key, key128);
            }

            *depth = 128;                /* Start at the leftmost bit */
            while (SKPMAP_IS_NODE(node)) {
                --*depth;
                if (*depth < 0) {
                    /* This should be caught when the map is loaded. */
                    skAppPrintErr("Corrupt prefix map."
                                  "  No result found in 128 bits.");
                    return SKPREFIXMAP_NOT_FOUND;
                }
                if (SKPMAP_GET_BIT128(key128, *depth)) {
                    /* Go right if the bit is 1. */
                    node = map->tree[node].right;
                } else {
                    /* Go left if the bit is 0. */
                    node = map->tree[node].left;
                }
            }
            return SKPMAP_LEAF_VALUE(node);
        }
#endif  /* SK_ENABLE_IPV6 */

      case SKPREFIXMAP_CONT_PROTO_PORT:
        key32 = SKPMAP_KEY_FROM_PROTO_PORT(key);
        break;

      case SKPREFIXMAP_CONT_ADDR_V4:
#if !SK_ENABLE_IPV6
        key32 = skipaddrGetV4((const skipaddr_t*)key);
#else
        if (skipaddrGetAsV4((const skipaddr_t*)key, &key32)) {
            return SKPREFIXMAP_NOT_FOUND;
        }
#endif
        break;

      default:
        skAbortBadCase(map->content_type);
    }

    *depth = 32;                /* Start at the leftmost bit */
    while (SKPMAP_IS_NODE(node)) {
        --*depth;
        if (*depth < 0) {
            /* This should be caught when the map is loaded. */
            skAppPrintErr("Corrupt prefix map."
                          "  No result found in 32 bits.");
            return SKPREFIXMAP_NOT_FOUND;
        }
        if (key32 & (1 << *depth)) {
            /* Go right if the bit is 1. */
            node = map->tree[node].right;
        } else {
            /* Go left if the bit is 0. */
            node = map->tree[node].left;
        }
    }
    return SKPMAP_LEAF_VALUE(node);
}


/*
 *  status = prefixMapGrowDictionaryBuff(map, bytes);
 *
 *    Grow the dictionary buffer at least enough to hold an entry of
 *    size 'bytes'.  Adjust the 'word' pointers into this buffer as
 *    well.
 */
static skPrefixMapErr_t
prefixMapGrowDictionaryBuff(
    skPrefixMap_t      *map,
    size_t              min_bytes)
{
    char *old_ptr = map->dict_buf;
    size_t grow;

    /* use initial size if this is first time */
    if (map->dict_buf_size == 0) {
        grow = SKPMAP_DICT_BUF_INIT;
    } else {
        grow = SKPMAP_DICT_BUF_GROW;
    }

    /* make certain it is big enough */
    while (grow < min_bytes) {
        grow += SKPMAP_DICT_BUF_GROW;
    }

    /* compute total size to allocate */
    grow += map->dict_buf_size;

    map->dict_buf = (char*)realloc(old_ptr, grow * sizeof(char));
    if (map->dict_buf == NULL) {
        map->dict_buf = old_ptr;
        return SKPREFIXMAP_ERR_MEMORY;
    }

    map->dict_buf_size = grow;

    /* move the dict_words pointers if they are no longer valid
     * because realloc() moved the memory */
    if (map->word_map && map->dict_buf != old_ptr) {
        RBLIST *iter;
        skPrefixMapDictNode_t *node;

        /* Amount to adjust pointers into the buffer by */
        ptrdiff_t diff = map->dict_buf - old_ptr;

        iter = rbopenlist(map->word_map);
        if (iter == NULL) {
            return SKPREFIXMAP_ERR_MEMORY;
        }
        while ((node = (skPrefixMapDictNode_t*)rbreadlist(iter)) != NULL) {
            node->word += diff;
            map->dict_words[node->value] += diff;
        }
        rbcloselist(iter);
    }

    return SKPREFIXMAP_OK;
}


/*
 *  status = prefixMapGrowDictionaryWords(map, entries);
 *
 *    Grow the dictionary words list at least enough to hold 'entries'
 *    new entriese.
 */
static skPrefixMapErr_t
prefixMapGrowDictionaryWords(
    skPrefixMap_t      *map,
    size_t              min_entries)
{
    char **old_ptr = map->dict_words;
    size_t grow;

    /* use initial size if this is first time */
    if (map->dict_words_size == 0) {
        grow = SKPMAP_WORDS_COUNT_INIT;
    } else {
        grow = SKPMAP_WORDS_COUNT_GROW;
    }

    /* make certain it is big enough */
    while (grow < min_entries) {
        grow += SKPMAP_WORDS_COUNT_GROW;
    }

    /* compute total size to allocate */
    grow += map->dict_words_size;

    map->dict_words = (char**)realloc(old_ptr, grow * sizeof(char*));
    if (map->dict_words == NULL) {
        map->dict_words = old_ptr;
        return SKPREFIXMAP_ERR_MEMORY;
    }
    /* clear new memory */
    memset((map->dict_words + map->dict_words_size), 0,
           ((grow - map->dict_words_size) * sizeof(void *)));

    map->dict_words_size = grow;
    return SKPREFIXMAP_OK;
}


/*
 *  status = prefixMapGrowTree(map);
 *
 *    Grow the number of nodes in the tree for the prefix map 'map'.
 */
static skPrefixMapErr_t
prefixMapGrowTree(
    skPrefixMap_t      *map)
{
    skPrefixMapRecord_t *old_ptr = map->tree;
    uint32_t grow = map->tree_size + SKPMAP_TREE_SIZE_GROW;

    map->tree = ((skPrefixMapRecord_t*)
                 realloc(map->tree, grow * sizeof(skPrefixMapRecord_t)));
    if (map->tree == NULL) {
        map->tree = old_ptr;
        return SKPREFIXMAP_ERR_MEMORY;
    }

    map->tree_size = grow;
    return SKPREFIXMAP_OK;
}


/* Compare two skPrefixMapDictNode_t's.  Used by the rbtree. */
static int
prefixMapWordCompare(
    const void         *va,
    const void         *vb,
    const void  UNUSED(*ctx))
{
    const skPrefixMapDictNode_t *a = (const skPrefixMapDictNode_t *)va;
    const skPrefixMapDictNode_t *b = (const skPrefixMapDictNode_t *)vb;
    return strcasecmp(a->word, b->word);
}


/* Set entries from 'low_val' to 'high_val' inclusive to 'dict_val' in 'map' */
skPrefixMapErr_t
skPrefixMapAddRange(
    skPrefixMap_t      *map,
    const void         *low_val,
    const void         *high_val,
    uint32_t            dict_val)
{
    uint32_t low32;
    uint32_t high32;

    if (dict_val > SKPREFIXMAP_MAX_VALUE) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    switch (map->content_type) {
#if SK_ENABLE_IPV6
      case SKPREFIXMAP_CONT_ADDR_V6:
        {
            uint8_t low128[16];
            uint8_t high128[16];

            if (skipaddrCompare((const skipaddr_t*)high_val,
                                (const skipaddr_t*)low_val) < 0)
            {
                return SKPREFIXMAP_ERR_ARGS;
            }

            if (skipaddrIsV6((const skipaddr_t*)low_val)) {
                skipaddrGetV6((const skipaddr_t*)low_val, low128);
            } else {
                skipaddrGetAsV6((const skipaddr_t*)low_val, low128);
            }
            if (skipaddrIsV6((const skipaddr_t*)high_val)) {
                skipaddrGetV6((const skipaddr_t*)high_val, high128);
            } else {
                skipaddrGetAsV6((const skipaddr_t*)high_val, high128);
            }
            return prefixMapAdd128(map, low128, high128,
                                   SKPMAP_MAKE_LEAF(dict_val), 0, 127);
        }
#endif  /* SK_ENABLE_IPV6 */

      case SKPREFIXMAP_CONT_PROTO_PORT:
        low32 = SKPMAP_KEY_FROM_PROTO_PORT(low_val);
        high32 = SKPMAP_KEY_FROM_PROTO_PORT(high_val);
        break;

      case SKPREFIXMAP_CONT_ADDR_V4:
#if !SK_ENABLE_IPV6
        low32 = skipaddrGetV4((const skipaddr_t*)low_val);
        high32 = skipaddrGetV4((const skipaddr_t*)high_val);
#else
        if (skipaddrGetAsV4((const skipaddr_t*)low_val, &low32)) {
            return SKPREFIXMAP_ERR_ARGS;
        }
        if (skipaddrGetAsV4((const skipaddr_t*)high_val, &high32)) {
            return SKPREFIXMAP_ERR_ARGS;
        }
#endif  /* SK_ENABLE_IPV6 */
        break;

      default:
        skAbortBadCase(map->content_type);
    }

    if (high32 < low32) {
        return SKPREFIXMAP_ERR_ARGS;
    }
    return prefixMapAdd32(map, low32, high32,
                          SKPMAP_MAKE_LEAF(dict_val), 0, 31);
}


/* Create a new prefix map at memory pointed at by 'map' */
skPrefixMapErr_t
skPrefixMapCreate(
    skPrefixMap_t     **map)
{
    if (NULL == map) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    *map = (skPrefixMap_t*)calloc(1, sizeof(skPrefixMap_t));
    if (NULL == *map) {
        return SKPREFIXMAP_ERR_MEMORY;
    }

    (*map)->tree_size = SKPMAP_TREE_SIZE_INIT;
    (*map)->tree = ((skPrefixMapRecord_t*)
                    calloc((*map)->tree_size, sizeof(skPrefixMapRecord_t)));
    if (NULL == (*map)->tree) {
        free(*map);
        return SKPREFIXMAP_ERR_MEMORY;
    }
    (*map)->tree[0].left = SKPMAP_MAKE_LEAF(SKPREFIXMAP_MAX_VALUE);
    (*map)->tree[0].right = SKPMAP_MAKE_LEAF(SKPREFIXMAP_MAX_VALUE);
    (*map)->tree_used = 1;

    return SKPREFIXMAP_OK;
}


/* destroy the prefix map */
void
skPrefixMapDelete(
    skPrefixMap_t      *map)
{
    if (NULL == map) {
        return;
    }
    if ( map->tree != NULL ) {
        if (map->mapname) {
            free(map->mapname);
        }
        if (map->dict_buf) {
            free(map->dict_buf);
        }
        if (map->dict_words) {
            free(map->dict_words);
        }
        if (map->tree) {
            free(map->tree);
        }
        if (map->word_map) {
            rbdestroy(map->word_map);
        }
        skMemoryPoolDestroy(&map->word_map_pool);
        memset(map, 0, sizeof(skPrefixMap_t));
    }
    free(map);
}


/* Fill 'out_buf' with dictionary entry given the value 'dict_val' */
int
skPrefixMapDictionaryGetEntry(
    const skPrefixMap_t    *map,
    uint32_t                dict_val,
    char                   *out_buf,
    size_t                  bufsize)
{
    if ((map->dict_buf_size == 0) || (dict_val >= map->dict_words_used)) {
        if (dict_val == SKPREFIXMAP_NOT_FOUND
            || dict_val == SKPREFIXMAP_MAX_VALUE)
        {
            return snprintf(out_buf, bufsize, "UNKNOWN");
        }
        return snprintf(out_buf, bufsize, ("%" PRIu32), dict_val);
    } else if (map->dict_words[dict_val] != NULL) {
        return snprintf(out_buf, bufsize, "%s", map->dict_words[dict_val]);
    } else {
        if (bufsize) {
            out_buf[0] = '\0';
        }
        return 0;
    }
}


/* Returns the max length of any word in the dictionary. */
uint32_t
skPrefixMapDictionaryGetMaxWordSize(
    const skPrefixMap_t    *map)
{
    if (map->dict_words_used) {
        return map->dict_max_wordlen;
    }
    /* size necessary to hold string representation of UINT32_MAX. */
    return 10;
}


/* Returns the number of words in the prefix map's dictionary. */
uint32_t
skPrefixMapDictionaryGetWordCount(
    const skPrefixMap_t    *map)
{
    return map->dict_words_used;
}


/* Create a new dictionary entry mapping 'dict_val' to 'label' */
skPrefixMapErr_t
skPrefixMapDictionaryInsert(
    skPrefixMap_t      *map,
    uint32_t            dict_val,
    const char         *word)
{
    return prefixMapDictionaryInsertHelper(map, &dict_val, word);
}


/* Returns the dict_val for a given dictionary word
 * (case-insensitive), or SKPREFIXMAP_NOT_FOUND */
uint32_t
skPrefixMapDictionaryLookup(
    const skPrefixMap_t    *map,
    const char             *word)
{
    skPrefixMapDictNode_t target;
    const skPrefixMapDictNode_t *found;

    if (map->word_map == NULL) {
        return SKPREFIXMAP_NOT_FOUND;
    }

    target.word = word;
    found = (const skPrefixMapDictNode_t*)rbfind(&target, map->word_map);
    if (found) {
        return found->value;
    }

    return SKPREFIXMAP_NOT_FOUND;
}


/* Create a new dictionary entry mapping 'out_dict_val' to 'label' */
skPrefixMapErr_t
skPrefixMapDictionarySearch(
    skPrefixMap_t      *map,
    const char         *word,
    uint32_t           *out_dict_val)
{
    skPrefixMapErr_t err;

    if (map == NULL || word == NULL || out_dict_val == NULL) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    /* Try to insert the word using a new unused value */
    *out_dict_val = map->dict_words_used;
    err = prefixMapDictionaryInsertHelper(map, out_dict_val, word);
    if (err == SKPREFIXMAP_ERR_DUPLICATE) {
        /* If the word already existed with a different value, that
         * value was returned in out_dict_val.  This is fine. */
        return SKPREFIXMAP_OK;
    }

    return err;
}


/* Return the dict_val for the given key and the start and end of the
 * range that contains key, or SKPREFIXMAP_NOT_FOUND */
uint32_t
skPrefixMapFindRange(
    const skPrefixMap_t    *map,
    const void             *key,
    void                   *start_range,
    void                   *end_range)
{
    int depth;
    uint32_t val;

    val = prefixMapFind(map, key, &depth);
    if (val != SKPREFIXMAP_NOT_FOUND) {
        if (SKPREFIXMAP_CONT_PROTO_PORT == map->content_type) {
            if (depth < 16) {
                ((skPrefixMapProtoPort_t*)start_range)->proto =
                    ((skPrefixMapProtoPort_t*)end_range)->proto =
                    ((skPrefixMapProtoPort_t*)key)->proto;
                ((skPrefixMapProtoPort_t*)start_range)->port =
                    UINT16_MAX & (((skPrefixMapProtoPort_t*)key)->port
                                  & (~(UINT16_MAX >> (16 - depth))));
                ((skPrefixMapProtoPort_t*)end_range)->port =
                    UINT16_MAX & (((skPrefixMapProtoPort_t*)key)->port
                                  | (UINT16_MAX >> (16 - depth)));
            } else if (depth == 16) {
                ((skPrefixMapProtoPort_t*)start_range)->proto =
                    ((skPrefixMapProtoPort_t*)end_range)->proto =
                    ((skPrefixMapProtoPort_t*)key)->proto;
                ((skPrefixMapProtoPort_t*)start_range)->port = 0;
                ((skPrefixMapProtoPort_t*)end_range)->port = UINT16_MAX;
            } else if (depth < 24) {
                ((skPrefixMapProtoPort_t*)start_range)->proto =
                    UINT8_MAX & (((skPrefixMapProtoPort_t*)key)->proto
                                 & (~(UINT8_MAX) >> (24 - depth)));
                ((skPrefixMapProtoPort_t*)end_range)->proto =
                    UINT8_MAX & (((skPrefixMapProtoPort_t*)key)->proto
                                 | (UINT8_MAX >> (24- depth)));
                ((skPrefixMapProtoPort_t*)start_range)->port = 0;
                ((skPrefixMapProtoPort_t*)end_range)->port = UINT16_MAX;
            } else {
                skAbort();
            }
        } else if (SKPREFIXMAP_CONT_ADDR_V6 == map->content_type) {
            if (depth > 128) {
                skAbort();
            }
            skCIDR2IPRange((const skipaddr_t*)key, 128-depth,
                           (skipaddr_t*)start_range, (skipaddr_t*)end_range);
        } else {
            if (depth > 32) {
                skAbort();
            }
            skCIDR2IPRange((const skipaddr_t*)key, 32-depth,
                           (skipaddr_t*)start_range, (skipaddr_t*)end_range);
        }
    }
    return val;
}


/* Fill 'out_buf' with dictionary entry name given the key 'key' */
int
skPrefixMapFindString(
    const skPrefixMap_t    *map,
    const void             *key,
    char                   *out_buf,
    size_t                  bufsize)
{
    int depth;
    return skPrefixMapDictionaryGetEntry(map, prefixMapFind(map, key, &depth),
                                         out_buf, bufsize);
}


/* Return the dict_val for the given key, or SKPREFIXMAP_NOT_FOUND */
uint32_t
skPrefixMapFindValue(
    const skPrefixMap_t    *map,
    const void             *key)
{
    int depth;
    return prefixMapFind(map, key, &depth);
}


/* Return a textual representation of the content type. */
const char *
skPrefixMapGetContentName(
    int                 content_id)
{
    static char buf[256];

    switch ((skPrefixMapContent_t)content_id) {
      case SKPREFIXMAP_CONT_ADDR_V4:
        return "IPv4-address";
      case SKPREFIXMAP_CONT_ADDR_V6:
        return "IPv6-address";
      case SKPREFIXMAP_CONT_PROTO_PORT:
        return "proto-port";
    }

    snprintf(buf, sizeof(buf), "Unrecognized prefix map content type id %d",
             content_id);
    return buf;
}


/* Returns the content type for a given map */
skPrefixMapContent_t
skPrefixMapGetContentType(
    const skPrefixMap_t    *map)
{
    return map->content_type;
}


/* Return the prefix map's name */
const char *
skPrefixMapGetMapName(
    const skPrefixMap_t    *map)
{
    return map->mapname;
}


/* Bind an iterator to a prefix map */
int
skPrefixMapIteratorBind(
    skPrefixMapIterator_t  *iter,
    const skPrefixMap_t    *map)
{
    if (iter == NULL || map == NULL) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    iter->map = map;
    skPrefixMapIteratorReset(iter);
    return SKPREFIXMAP_OK;
}


/* Create an iterator */
int
skPrefixMapIteratorCreate(
    skPrefixMapIterator_t **out_iter,
    const skPrefixMap_t    *map)
{
    assert(out_iter);

    *out_iter = (skPrefixMapIterator_t*)malloc(sizeof(skPrefixMapIterator_t));
    if (*out_iter == NULL) {
        return SKPREFIXMAP_ERR_MEMORY;
    }

    if (skPrefixMapIteratorBind(*out_iter, map)) {
        skPrefixMapIteratorDestroy(out_iter);
        return SKPREFIXMAP_ERR_ARGS;
    }

    return SKPREFIXMAP_OK;
}


/* Destroy the iterator */
void
skPrefixMapIteratorDestroy(
    skPrefixMapIterator_t **out_iter)
{
    if (*out_iter) {
        free(*out_iter);
        *out_iter = NULL;
    }
}


/* Fill 'start' and 'end' with next range */
skIteratorStatus_t
skPrefixMapIteratorNext(
    skPrefixMapIterator_t  *iter,
    void                   *out_key_start,
    void                   *out_key_end,
    uint32_t               *out_value)
{
    skPrefixMapProtoPort_t key_pp;
    skipaddr_t key_addr;
    uint32_t key32;
    int depth;
    uint32_t val;
    uint32_t old_val = SKPREFIXMAP_NOT_FOUND;

    assert(out_key_start);
    assert(out_key_end);
    assert(out_value);

#if SK_ENABLE_IPV6
    if (SKPREFIXMAP_CONT_ADDR_V6 == iter->map->content_type) {
        uint8_t key128[16];
        unsigned int i;

        /* check for the stoping condition */
        skipaddrGetV6(&iter->end.addr, key128);
        if (0 == memcmp(key128, max_ip128, sizeof(max_ip128))) {
            return SK_ITERATOR_NO_MORE_ENTRIES;
        }

        /* check for the starting condition */
        if (skipaddrCompare(&iter->end.addr, &iter->start.addr) < 0) {
            skipaddrCopy(&iter->start.addr, &iter->end.addr);
        } else {
            /* move key to start of next range */
            skipaddrCopy(&iter->start.addr, &iter->end.addr);
            skipaddrIncrement(&iter->start.addr);
        }

        skipaddrCopy(&key_addr, &iter->start.addr);
        for (;;) {
            val = prefixMapFind(iter->map, &key_addr, &depth);
            if (old_val == SKPREFIXMAP_NOT_FOUND) {
                old_val = val;
            }
            if (old_val == val) {
                /* grow current range */
                skipaddrGetV6(&key_addr, key128);
                i = 15 - (depth >> 3);
                key128[i] += (1 << (depth & 0x7));
                if (0 == key128[i]) {
                    while (0 == key128[i] && i > 0) {
                        --i;
                        ++key128[i];
                    }
                    if (0 == i && 0 == key128[0]) {
                        skipaddrSetV6(&iter->end.addr, max_ip128);
                        break;
                    }
                }
                skipaddrSetV6(&key_addr, key128);
            } else {
                skipaddrCopy(&iter->end.addr, &key_addr);
                skipaddrDecrement(&iter->end.addr);
                break;
            }
        }

        /* set the output values to our current location */
        skipaddrCopy((skipaddr_t*)out_key_start, &iter->start.addr);
        skipaddrCopy((skipaddr_t*)out_key_end, &iter->end.addr);
        *out_value = old_val;
        return SK_ITERATOR_OK;
    }
#endif  /* SK_ENABLE_IPV6 */

    /* check for the stoping condition */
    if (iter->end.u32 == UINT32_MAX) {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }

    /* check for the starting condition */
    if (iter->end.u32 < iter->start.u32) {
        memset(&iter->start, 0, sizeof(iter->start));
    } else {
        /* move key to start of next range */
        iter->start.u32 = iter->end.u32 + 1;
    }

    if (SKPREFIXMAP_CONT_ADDR_V4 == iter->map->content_type) {
        key32 = iter->start.u32;
        for (;;) {
            skipaddrSetV4(&key_addr, &key32);
            val = prefixMapFind(iter->map, &key_addr, &depth);
            if (old_val == SKPREFIXMAP_NOT_FOUND) {
                old_val = val;
            }
            if (old_val == val) {
                /* grow current range */
                key32 += (1 << depth);
                if (key32 == 0) {
                    iter->end.u32 = UINT32_MAX;
                    break;
                }
            } else {
                iter->end.u32 = key32 - 1;
                break;
            }
        }

        /* set the output values to our current location */
        skipaddrSetV4((skipaddr_t*)out_key_start, &iter->start.u32);
        skipaddrSetV4((skipaddr_t*)out_key_end, &iter->end.u32);
        *out_value = old_val;
        return SK_ITERATOR_OK;
    }

    if (SKPREFIXMAP_CONT_PROTO_PORT != iter->map->content_type) {
        skAbortBadCase(iter->map->content_type);
    }

    key32 = iter->start.u32;
    for (;;) {
        key_pp.proto = 0xFF & (key32 >> 16);
        key_pp.port = 0xFFFF & key32;
        val = prefixMapFind(iter->map, &key_pp, &depth);
        if (old_val == SKPREFIXMAP_NOT_FOUND) {
            old_val = val;
        }
        if (old_val == val) {
            /* grow current range */
            key32 += (1 << depth);
            if (key32 == 0 || key32 >= (1 << 24)) {
                iter->end.u32 = UINT32_MAX;
                break;
            }
        } else {
            iter->end.u32 = key32 - 1;
            break;
        }
    }

    /* set the output values to our current location */
    ((skPrefixMapProtoPort_t*)out_key_start)->proto
        = 0xFF & (iter->start.u32 >> 16);
    ((skPrefixMapProtoPort_t*)out_key_start)->port = 0xFFFF & iter->start.u32;
    ((skPrefixMapProtoPort_t*)out_key_end)->proto
        = 0xFF & (iter->end.u32 >> 16);
    ((skPrefixMapProtoPort_t*)out_key_end)->port = 0xFFFF & iter->end.u32;
    *out_value = old_val;
    return SK_ITERATOR_OK;
}


/* Reset iterator */
void
skPrefixMapIteratorReset(
    skPrefixMapIterator_t  *iter)
{
    assert(iter);

    /* starting condition is end < start */
#if SK_ENABLE_IPV6
    if (SKPREFIXMAP_CONT_ADDR_V6 == iter->map->content_type) {
        skipaddrSetV6(&iter->end.addr, min_ip128);
        skipaddrSetV6(&iter->start.addr, max_ip128);
    } else
#endif
    {
        iter->end.u32 = 0;
        iter->start.u32 = 1;
    }
}


/* Allocate new prefix map; open and read contents from 'path' */
skPrefixMapErr_t
skPrefixMapLoad(
    skPrefixMap_t     **map,
    const char         *path)
{
    skstream_t *in;
    skPrefixMapErr_t err = SKPREFIXMAP_OK;
    int rv;

    /* Check arguments for sanity */
    if ( NULL == map ) {
        skAppPrintErr("No place was provided to store new prefix map.");
        return SKPREFIXMAP_ERR_ARGS;
    }
    if ( NULL == path ) {
        skAppPrintErr("No input file provided from which to read prefix map.");
        return SKPREFIXMAP_ERR_ARGS;
    }

    if ((rv = skStreamCreate(&in, SK_IO_READ, SK_CONTENT_SILK))
        || (rv = skStreamBind(in, path))
        || (rv = skStreamOpen(in)))
    {
        skStreamPrintLastErr(in, rv, &skAppPrintErr);
        err = SKPREFIXMAP_ERR_IO;
        goto END;
    }

    err = skPrefixMapRead(map, in);

  END:
    skStreamDestroy(&in);
    return err;
}


/* Allocate new prefix map and read contents from 'in' */
skPrefixMapErr_t
skPrefixMapRead(
    skPrefixMap_t     **map,
    skstream_t         *in)
{
    union h_un {
        sk_header_entry_t      *he;
        sk_hentry_prefixmap_t  *pm;
    } h;
    sk_file_header_t *hdr;
    fileVersion_t vers;
    int has_dictionary;
    uint32_t record_count;
    uint32_t max_key_used;
    uint32_t swapFlag;
    uint32_t i;
    char *current;
    char *start;
    char *end;
    skPrefixMapErr_t err;
    int rv;

    /* Check arguments for sanity */
    if ( NULL == map ) {
        skAppPrintErr("No place was provided to store new prefix map.");
        return SKPREFIXMAP_ERR_ARGS;
    }
    if ( NULL == in ) {
        skAppPrintErr("No input stream provided from which to read prefix map");
        return SKPREFIXMAP_ERR_ARGS;
    }

    *map = NULL;

    rv = skStreamReadSilkHeader(in, &hdr);
    if (rv) {
        skStreamPrintLastErr(in, rv, &skAppPrintErr);
        return SKPREFIXMAP_ERR_IO;
    }

    if (skStreamCheckSilkHeader(in, FT_PREFIXMAP, 1, 5, &skAppPrintErr)){
        return SKPREFIXMAP_ERR_HEADER;
    }
    vers = skHeaderGetRecordVersion(hdr);

    if (SK_COMPMETHOD_NONE != skHeaderGetCompressionMethod(hdr) ) {
        skAppPrintErr("Unrecognized prefix map compression method.");
        return SKPREFIXMAP_ERR_HEADER;
    }

    swapFlag = !skHeaderIsNativeByteOrder(hdr);

    /* Read record count */
    if (skStreamRead(in, &(record_count), sizeof(record_count))
        != sizeof(record_count))
    {
        skAppPrintErr("Error reading header from input file (short read).");
        return SKPREFIXMAP_ERR_IO;
    }

    if ( swapFlag ) {
        record_count = BSWAP32(record_count);
    }

    if ( record_count < 1 ) {
        skAppPrintErr("Input file contains invalid prefix map (no data).");
        return SKPREFIXMAP_ERR_HEADER;
    }

    /* Allocate a prefix map, and a storage buffer */
    *map = (skPrefixMap_t*)calloc(1, sizeof(skPrefixMap_t));
    if (NULL == *map) {
        skAppPrintErr("Failed to allocate memory for prefix map.");
        return SKPREFIXMAP_ERR_MEMORY;
    }

    /* most files have a dictionary */
    has_dictionary = 1;

    switch (vers) {
      case 3:
        (*map)->content_type = SKPREFIXMAP_CONT_PROTO_PORT;
        break;
      case 1:
        /* IPv4 country code map (no dictionary) */
        has_dictionary = 0;
        (*map)->content_type = SKPREFIXMAP_CONT_ADDR_V4;
        break;
      case 2:
        /* IPv4 general prefix map */
        (*map)->content_type = SKPREFIXMAP_CONT_ADDR_V4;
        break;
#if !SK_ENABLE_IPV6
      case 4:
      case 5:
        skAppPrintErr("Support for IPv6 prefix maps not included"
                      " in this installation");
        return SKPREFIXMAP_ERR_HEADER;
#else
      case 4:
        /* IPv6 general prefix map */
        (*map)->content_type = SKPREFIXMAP_CONT_ADDR_V6;
        break;
      case 5:
        /* IPv6 country code map (no dictionary) */
        has_dictionary = 0;
        (*map)->content_type = SKPREFIXMAP_CONT_ADDR_V6;
        break;
#endif  /* SK_ENABLE_IPV6 */
      default:
        skAbortBadCase(vers);
    }

    (*map)->tree = ((skPrefixMapRecord_t*)
                    malloc(record_count * sizeof(skPrefixMapRecord_t)));
    if (NULL == (*map)->tree) {
        skAppPrintErr("Failed to allocate memory for prefix map data.");
        err = SKPREFIXMAP_ERR_MEMORY;
        goto ERROR;
    }
    (*map)->tree_size = record_count;
    (*map)->tree_used = record_count;

    /* Get the mapname from the header if it was specified and if the
     * header-entry version is 1. */
    h.he = skHeaderGetFirstMatch(hdr, SK_HENTRY_PREFIXMAP_ID);
    if ((h.he) && (1 == skHentryPrefixmapGetVersion(h.pm))) {
        (*map)->mapname = strdup(skHentryPrefixmapGetMapmame(h.pm));
        if (NULL == (*map)->mapname) {
            skAppPrintErr("Failed to allocate memory for prefix map name.");
            err = SKPREFIXMAP_ERR_MEMORY;
            goto ERROR;
        }
    }

    /* Allocation completed successfully, read in the records. */
    if (skStreamRead(in, (*map)->tree,
                     (record_count * sizeof(skPrefixMapRecord_t)))
        != (ssize_t)(record_count * sizeof(skPrefixMapRecord_t)))
    {
        skAppPrintErr("Failed to read all records from input file.");
        err = SKPREFIXMAP_ERR_IO;
        goto ERROR;
    }

    /* Swap the byte order of the data if needed. */
    if ( swapFlag ) {
        for ( i = 0; i < record_count; i++ ) {
            (*map)->tree[i].left = BSWAP32((*map)->tree[i].left);
            (*map)->tree[i].right = BSWAP32((*map)->tree[i].right);
        }
    }

    /* Allocate and read the dictionary. */
    if ( has_dictionary ) {
        /* Get number of entries */
        if (skStreamRead(in, &((*map)->dict_buf_size), sizeof(uint32_t))
            != (ssize_t)sizeof(uint32_t))
        {
            skAppPrintErr("Error reading dictionary from input file.");
            err = SKPREFIXMAP_ERR_IO;
            goto ERROR;
        }
        if (swapFlag) {
            (*map)->dict_buf_size = BSWAP32((*map)->dict_buf_size);
        }

        (*map)->dict_buf = (char*)malloc((*map)->dict_buf_size * sizeof(char));
        if ( NULL == (*map)->dict_buf ) {
            skAppPrintErr("Failed to allocate memory for prefix map dict.");
            err = SKPREFIXMAP_ERR_MEMORY;
            goto ERROR;
        }

        /* Dictionary is allocated; now read in data. */
        if (skStreamRead(in, (*map)->dict_buf, (*map)->dict_buf_size)
            != (ssize_t)(*map)->dict_buf_size)
        {
            skAppPrintErr("Failed to read dictionary from input file.");
            err = SKPREFIXMAP_ERR_IO;
            goto ERROR;
        }
        (*map)->dict_buf_end = (*map)->dict_buf_size;
        (*map)->dict_buf_used = (*map)->dict_buf_size;

        /* Index the dictionary data */

        /* First pass: count the words */
        start = (*map)->dict_buf;
        end = (*map)->dict_buf + (*map)->dict_buf_end;
        (*map)->dict_words_used = 0;
        for ( current = start; current < end; current++ ) {
            if ( (*current) == '\0' ) {
                (*map)->dict_words_used++;
            }
        }
        (*map)->dict_words_size = (*map)->dict_words_used;

        /* Allocate space for index */
        (*map)->dict_words
            = (char**)malloc((*map)->dict_words_size * sizeof(char*));
        if ((*map)->dict_words == NULL) {
            skAppPrintErr("Failed to allocated memory for prefix map index.");
            err = SKPREFIXMAP_ERR_MEMORY;
            goto ERROR;
        }
        if (0 != skMemoryPoolCreate(&(*map)->word_map_pool,
                                    sizeof(skPrefixMapDictNode_t),
                                    SKPMAP_WORDS_COUNT_GROW))
        {
            skAppPrintErr("Failed to allocated memory for prefix map "
                          "index memory pool.");
            err = SKPREFIXMAP_ERR_MEMORY;
            goto ERROR;
        }
        (*map)->word_map = rbinit(prefixMapWordCompare, NULL);
        if ((*map)->word_map == NULL) {
            skAppPrintErr("Failed to allocated memory for prefix map "
                          "index tree.");
            err = SKPREFIXMAP_ERR_MEMORY;
            goto ERROR;
        }

        /* Now build index */
        current = (*map)->dict_buf;
        start = current;
        end = current + (*map)->dict_buf_used;
        for ( i = 0; i < (*map)->dict_words_used; ++i ) {
            while ( (*current != '\0') && (current < end) ) {
                current++;
            }
            (*map)->dict_words[i] = ((current == start) ? NULL : start);
            if (current != start) {
                skPrefixMapDictNode_t *node;
                const skPrefixMapDictNode_t *found;

                node = ((skPrefixMapDictNode_t*)
                        skMemPoolElementNew((*map)->word_map_pool));
                if (node == NULL) {
                    err = SKPREFIXMAP_ERR_MEMORY;
                    goto ERROR;
                }
                node->word = start;
                node->value = i;
                found = ((const skPrefixMapDictNode_t*)
                         rbsearch(node, (*map)->word_map));
                if (found == NULL) {
                    err = SKPREFIXMAP_ERR_MEMORY;
                    goto ERROR;
                }
                if (found != node) {
                    err = SKPREFIXMAP_ERR_DUPLICATE;
                    goto ERROR;
                }
            }

            if ((uint32_t)(current - start) > (*map)->dict_max_wordlen) {
                (*map)->dict_max_wordlen = (current - start);
            }

            current++;
            start = current;
        }
    }

    /*
     * Check the invariants that ensure that the prefixmap is a total
     * mapping (every input maps to a single output.)
     *
     * In theory, the invariants are:
     *
     * 1) No node may point to a non-existent child (>= record_count).
     * See note below.
     *
     * 2) No node may point to a node earlier than itself (prevents
     * cycles)
     *
     *
     * The first invariant as written is too strict as there is
     * sometimes extra data at the end of the map.  Instead, we make
     * certain that no node points to locatation beyond the first node
     * that contains an invalid child.  To determine this, visit the
     * nodes until we find one that points beyond the end of the tree,
     * while keeping track of the highest key in use.  If the highest
     * key points beyond the invalid node, the tree is invalid.
     *
     * The second invariant assumes the nodes were created in a
     * top-down manner, which may not be true, so the rule that
     * implements the second invariant is commented out below.
     */
    max_key_used = 0;
    for ( i = 0; i < record_count; ++i ) {
        uint32_t L = (*map)->tree[i].left;
        uint32_t R = (*map)->tree[i].right;
        if (SKPMAP_IS_NODE(L)) {
            if (L >= record_count /* || L <= i */) {
                break;
            }
            if (L > max_key_used) {
                max_key_used = L;
            }
        }
        if (SKPMAP_IS_NODE(R)) {
            if (R >= record_count /* || R <= i */) {
                break;
            }
            if (R > max_key_used) {
                max_key_used = R;
            }
        }
    }
    if (i < record_count) {
        if (max_key_used >= i) {
            skAppPrintErr("Prefix map is malformed (contains invalid child).");
            free((*map)->tree);
            free(*map);
            *map = NULL;
            return SKPREFIXMAP_ERR_IO;
        }
        (*map)->tree_used = i;
    }

    /* Hrm.  We should also try to look for chains longer than 32
     * steps.  What's a good way to do that?
     */

    return SKPREFIXMAP_OK;

  ERROR:
    if (*map) {
        skPrefixMapDelete(*map);
        *map = NULL;
    }
    return err;
}


/* Write 'map' to the file specified by 'pathname'. Wraps skPrefixMapWrite. */
skPrefixMapErr_t
skPrefixMapSave(
    skPrefixMap_t      *map,
    const char         *pathname)
{
    skstream_t *stream = NULL;
    skPrefixMapErr_t err;
    int rv;

    if (pathname == NULL || map == NULL) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    if ((rv = skStreamCreate(&stream, SK_IO_WRITE, SK_CONTENT_SILK))
        || (rv = skStreamBind(stream, pathname))
        || (rv = skStreamOpen(stream)))
    {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        err = SKPREFIXMAP_ERR_IO;
        goto END;
    }

    err = skPrefixMapWrite(map, stream);
    if (err) {
        goto END;
    }

    rv = skStreamClose(stream);
    if (rv) {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        err = SKPREFIXMAP_ERR_IO;
        goto END;
    }

  END:
    skStreamDestroy(&stream);
    return err;
}


skPrefixMapErr_t
skPrefixMapSetContentType(
    skPrefixMap_t          *map,
    skPrefixMapContent_t    content_type)
{
    map->content_type = content_type;
    return SKPREFIXMAP_OK;

}

/* Set default value in 'map' to 'dict_val'.  'map' must be empty. */
skPrefixMapErr_t
skPrefixMapSetDefaultVal(
    skPrefixMap_t      *map,
    uint32_t            dict_val)
{
    if (dict_val > SKPREFIXMAP_MAX_VALUE) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    /* ensure no entries have been added to the tree and default has
     * not been set.  This does not detect if default is set to
     * 0x7fffffff or if 0.0.0.0/1 or 128.0.0.0/1 has been set to
     * 0x7fffffff, but those are extremely rare possibilities. */
    if ((map->tree_used > 1)
        || (map->tree[0].right != SKPMAP_MAKE_LEAF(SKPREFIXMAP_MAX_VALUE))
        || (map->tree[0].left != SKPMAP_MAKE_LEAF(SKPREFIXMAP_MAX_VALUE)))
    {
        return SKPREFIXMAP_ERR_NOTEMPTY;
    }

    map->tree[0].left = SKPMAP_MAKE_LEAF(dict_val);
    map->tree[0].right = SKPMAP_MAKE_LEAF(dict_val);
    return SKPREFIXMAP_OK;
}


/* Set the prefix map's name */
skPrefixMapErr_t
skPrefixMapSetMapName(
    skPrefixMap_t      *map,
    const char         *name)
{
    char *duplicate = NULL;

    if (name) {
        duplicate = strdup(name);
        if (duplicate == NULL) {
            return SKPREFIXMAP_ERR_MEMORY;
        }
    }

    if (map->mapname) {
        free(map->mapname);
    }
    map->mapname = duplicate;
    return SKPREFIXMAP_OK;
}


/* Return a textual representation of the specified error code. */
const char *
skPrefixMapStrerror(
    int                 error_code)
{
    static char buf[256];

    switch ((skPrefixMapErr_t)error_code) {
      case SKPREFIXMAP_OK:
        return "Success";
      case SKPREFIXMAP_ERR_ARGS:
        return "Invalid arguments";
      case SKPREFIXMAP_ERR_MEMORY:
        return "Out of memory";
      case SKPREFIXMAP_ERR_IO:
        return "I/O error";
      case SKPREFIXMAP_ERR_DUPLICATE:
        return "Duplicate dictionary ID or word";
      case SKPREFIXMAP_ERR_NOTEMPTY:
        return "Cannot set default in non-empty map";
      case SKPREFIXMAP_ERR_HEADER:
        return "Invalid version, type, or compression method in file header";
    }

    snprintf(buf, sizeof(buf), "Unrecognized prefix map error code %d",
             error_code);
    return buf;
}


/* Write the binary prefix map 'map' to the specified stream */
skPrefixMapErr_t
skPrefixMapWrite(
    skPrefixMap_t      *map,
    skstream_t         *stream)
{
    sk_file_header_t *hdr;
    ssize_t rv;
    fileVersion_t vers;

    if (stream == NULL || map == NULL) {
        return SKPREFIXMAP_ERR_ARGS;
    }

    if (map->content_type == SKPREFIXMAP_CONT_PROTO_PORT) {
        vers = 3;
    } else if (map->content_type == SKPREFIXMAP_CONT_ADDR_V4) {
        if (map->dict_buf == NULL) {
            vers = 1;
        } else {
            vers = 2;
        }
    } else if (map->content_type == SKPREFIXMAP_CONT_ADDR_V6) {
        if (map->dict_buf == NULL) {
            vers = 5;
        } else {
            vers = 4;
        }
    } else {
        return SKPREFIXMAP_ERR_ARGS;
    }

    /* create the header */
    hdr = skStreamGetSilkHeader(stream);
    skHeaderSetFileFormat(hdr, FT_PREFIXMAP);
    skHeaderSetRecordVersion(hdr, vers);
    skHeaderSetCompressionMethod(hdr, SK_COMPMETHOD_NONE);
    skHeaderSetRecordLength(hdr, 1);

    /* add the prefixmap header if a mapname was given */
    if (map->mapname) {
        rv = skHeaderAddPrefixmap(hdr, map->mapname);
        if (rv) {
            skAppPrintErr("%s", skHeaderStrerror(rv));
            return SKPREFIXMAP_ERR_IO;
        }
    }

    /* write the header */
    rv = skStreamWriteSilkHeader(stream);
    if (rv) {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        return SKPREFIXMAP_ERR_IO;
    }

    /* write the number of records */
    rv = skStreamWrite(stream, &map->tree_used, sizeof(uint32_t));
    if (rv == -1) {
        goto ERROR;
    }

    /* write the records */
    rv = skStreamWrite(stream, map->tree,
                       (size_t)map->tree_used * sizeof(skPrefixMapRecord_t));
    if (rv == -1) {
        goto ERROR;
    }

    if (map->dict_buf) {
        uint32_t i;

        /* write the number of characters in the dictionary */
        rv = skStreamWrite(stream, &map->dict_buf_used, sizeof(uint32_t));
        if (rv == -1) {
            goto ERROR;
        }

        /* write the dictionary entries */
        for (i = 0; i < map->dict_words_used; i++) {
            static const char zero = '\0';
            if (map->dict_words[i] == NULL) {
                rv = skStreamWrite(stream, &zero, sizeof(char));
            } else {
                rv = skStreamWrite(stream, map->dict_words[i],
                                   (sizeof(char)
                                    * (strlen(map->dict_words[i]) + 1)));
            }
            if (rv == -1) {
                goto ERROR;
            }
        }
    }

    /* Success */
    return SKPREFIXMAP_OK;

  ERROR:
    skStreamPrintLastErr(stream, rv, &skAppPrintErr);
    return SKPREFIXMAP_ERR_IO;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
