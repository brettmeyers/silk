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

/* File: hashlib.c: implements core hash library. */

#include <silk/silk.h>

RCSIDENT("$SiLK: hashlib.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/hashlib.h>
#include <silk/utils.h>

#ifdef HASHLIB_TRACE_LEVEL
#define TRACEMSG_LEVEL HASHLIB_TRACE_LEVEL
#endif
#define TRACEMSG(lvl, x)  TRACEMSG_TO_TRACEMSGLVL(lvl, x)
#include <silk/sktracemsg.h>


/* typedef struct HashBlock_st HashBlock; */
struct HashBlock_st {
    uint8_t *_data_ptr;     /* Pointer to an array of variable-sized entries */

    /* this value is a repeat of values from HashTable so a single
     * set of macros will work on either the HashTable or HashBlock */
    uint8_t *no_value_ptr;  /* Pointer to representation of an empty value */

    uint32_t block_size;    /* Total number of entries in the block */
    uint32_t num_entries;   /* Number of occupied entries in the block */
    uint32_t block_full;    /* Point at which block meets the load_factor */

    /* these values are a repeat of values from HashTable so a single
     * set of macros will work on either the HashTable or HashBlock.
     * the load_factor really isn't needed; but keep it here since we
     * have space for it. */
    uint8_t  key_width;     /* Size of a key in bytes */
    uint8_t  value_width;   /* Size of a value in bytes */
    uint8_t  load_factor;   /* Point at which to resize (fraction of 255) */
};


/* this structure is used when serializing the table */
typedef struct BlockHeader_st {
    uint32_t block_size;
    uint32_t num_entries;
} BlockHeader;


/* Configuration */

/* Important: minimum block size cannot be less than 256 */
static uint32_t MIN_BLOCK_SIZE = (1 << 8);

/* Maximum size of a hash block. Should be tweaked for a particular
 * platform.  Default if not defined in hashlib_conf.h. */
#ifndef MAX_MEMORY_BLOCK
#define MAX_MEMORY_BLOCK (1<<27)
#endif

/* Point at which a rehash is triggered (unless we're at max block size) */
/* This value is not static, since hashlib_metrics.c may set it. */
uint32_t REHASH_BLOCK_COUNT = 4;

/* Distinguished values for block */
#define HASH_ITER_BEGIN -1
#define HASH_ITER_END -2


/*
 * Secondary table size is (table_size >> SECONDARY_BLOCK_FRACTION).
 * May also have one of the following values:
 *
 * -1 means to keep halving
 *
 * -2 means to keep halving starting at a secondary block size 1/4 of
 * the main block
 *
 * -3 means block 1 is 1/2 block 0, and all other blocks are 1/4 block 0.
 *
 * -4 means block 1 is 1/4 block 0, and all other blocks are 1/8 block 0.
 *
 * In all cases, the size of blocks REHASH_BLOCK_COUNT through
 * MAX_BLOCKS is fixed.
 */
/* This value is not static, since hashlib_metrics.c may set it. */
int32_t SECONDARY_BLOCK_FRACTION = -3;

/* Compute the maximum number of entries per block */
#define HASH_GET_MAX_BLOCK_ENTRIES(tbl_ptr)  \
    ((uint32_t)(MAX_MEMORY_BLOCK/HASH_GET_ENTRY_SIZE(tbl_ptr)))

/* Private functions for manipulating blocks */
static HashBlock *
hashlib_create_block(
    HashTable          *table_ptr,
    uint32_t            block_size);
static void
hashlib_free_block(
    HashBlock          *block_ptr);
static int
hashlib_block_find_entry(
    const HashBlock    *block_ptr,
    const uint8_t      *key_ptr,
    uint8_t           **entry_pptr);
static int
hashlib_block_lookup(
    const HashBlock    *block_ptr,
    const uint8_t      *key_ptr,
    uint8_t           **value_pptr);
static uint32_t
hashlib_block_count_nonempties(
    const HashBlock    *block_ptr);
static void
hashlib_dump_block(
    FILE               *fp,
    const HashBlock    *block_ptr);
static void
hashlib_dump_block_header(
    FILE               *fp,
    const HashBlock    *table_ptr);
static int
hashlib_iterate_sorted(
    const HashTable    *table_ptr,
    HASH_ITER          *iter_ptr,
    uint8_t           **key_pptr,
    uint8_t           **val_pptr);
static int hashlib_cmp_fn(const void *a, const void *b, void *v_length);


#ifdef HASHLIB_RECORD_STATS
uint64_t hashlib_stats_rehashes = 0;
uint64_t hashlib_stats_rehash_inserts = 0;
uint64_t hashlib_stats_inserts = 0;
uint64_t hashlib_stats_lookups = 0;
uint32_t hashlib_stats_blocks_allocated = 0;
#endif  /* HASHLIB_RECORD_STATS */

/* pull in the code that defines the hash function */
#ifdef HASHLIB_LOOKUP2
/* hash code used up to and including SiLK 2.3.x */
unsigned long
hash(
    const uint8_t      *k,
    unsigned long       len,
    unsigned long       initval);
unsigned long
hash2(
    unsigned long      *k,
    unsigned long       len,
    unsigned long       initval);
unsigned long
hash3(
    uint8_t            *k,
    unsigned long       len,
    unsigned long       initval);
#include "hashlib-lookup2.c"

#else
/* hash code used in SiLK 2.4 and beyond */

uint32_t
hashword(
    const uint32_t     *k,
    size_t              length,
    uint32_t            initval);
void
hashword2(
    const uint32_t     *k,
    size_t              length,
    uint32_t           *pc,
    uint32_t           *pb);
uint32_t
hashlittle(
    const void         *key,
    size_t              length,
    uint32_t            initval);
void
hashlittle2(
    const void         *key,
    size_t              length,
    uint32_t           *pc,
    uint32_t           *pb);
uint32_t
hashbig(
    const void         *key,
    size_t              length,
    uint32_t            initval);

#include "hashlib-lookup3.c"
#if SK_BIG_ENDIAN
#  define hash  hashbig
#else
#  define hash  hashlittle
#endif
#endif  /* HASHLIB_LOOKUP2 */


/* Get the size of a key, value, or entry in bytes; the 'tbl_ptr'
 * parameter may be a HashTable or a HashBlock */
#define HASH_GET_KEY_SIZE(tbl_ptr)            \
    ((tbl_ptr)->key_width)
#define HASH_GET_VALUE_SIZE(tbl_ptr)          \
    ((tbl_ptr)->value_width)
#define HASH_GET_ENTRY_SIZE(tbl_ptr)                  \
    ((tbl_ptr)->key_width + (tbl_ptr)->value_width)

/* Get a pointer to the entry at index hash_index; the 'blk_ptr' must
 * be a HashBlock */
#define HASH_GET_ENTRY(blk_ptr, hash_index)                           \
    ((blk_ptr)->_data_ptr + (HASH_GET_ENTRY_SIZE(blk_ptr) * (hash_index)))
/* Get a pointer to a key or value in an entry */
#define HASH_GET_KEY(blk_ptr, entry_ptr)      \
    (entry_ptr)
#define HASH_GET_VALUE(blk_ptr, entry_ptr)    \
    ((entry_ptr) + HASH_GET_KEY_SIZE(blk_ptr))

/* Get a pointer to the key or value at index hash_index */
#define HASH_GET_ENTRY_KEY(blk_ptr, hash_index) \
    HASH_GET_KEY((blk_ptr), HASH_GET_ENTRY((blk_ptr), (hash_index)))
#define HASH_GET_ENTRY_VALUE(blk_ptr, hash_index) \
    HASH_GET_VALUE((blk_ptr), HASH_GET_ENTRY((blk_ptr), (hash_index)))

/* Return 1 if all bytes in a pair of keys match, otherwise 0 */
#define HASH_COMPARE_KEYS(key1_ptr, key2_ptr, key_width)                \
    (0 == memcmp((key1_ptr), (key2_ptr), (key_width)))

/* Return 1 if the bytes in 'value_ptr' match the empty value, otherwise 0.*/
#define HASH_VALUE_ISEMPTY(tbl_ptr, value_ptr)          \
    (0 == memcmp((value_ptr), (tbl_ptr)->no_value_ptr,  \
                 HASH_GET_VALUE_SIZE(tbl_ptr)))

/* Return 1 if the value for 'entry_ptr' matches empty value, otherwise 0.*/
#define HASH_ENTRY_ISEMPTY(tbl_ptr, entry_ptr)                  \
    (0 == memcmp(HASH_GET_VALUE((tbl_ptr), (entry_ptr)),        \
                 (tbl_ptr)->no_value_ptr,                       \
                 HASH_GET_VALUE_SIZE(tbl_ptr)))

#ifdef NDEBUG
#define HASH_ASSERT_SIZE_IS_POWER_2(blk_size)
#else
#define HASH_ASSERT_SIZE_IS_POWER_2(blk_size)   \
    do {                                        \
        uint32_t high_bits;                     \
        BITS_IN_WORD32(&high_bits, (blk_size)); \
        assert(1 == high_bits);                 \
    } while(0)
#endif  /* NDEBUG */


/* HASH_COMPUTE_HASH - A simple wrapper around hash() that
 * encapsulates the probing algorithm and takes care of scaling the
 * index to the block size. Before calling this function,
 * hash_probe_increment should be set to 0. Each time it is called,
 * the probe increment is added to to the hash_value and scaled by
 * the block size (note: we don't use modulo since this is too
 * expensive, instead since sizes our powers of 2 we can mask).
 *
 * Args:
 *  index      Variable to set with the computed index
 *  key_ptr    Pointer to the key value to hash
 *  key_width  Number of bytes in the key
 *  block_size Number of bytes in the block (must be a power of 2)
 *  hash_value Variable (uint32_t) to hold the hash value
 *  hash_probe_increment Variable to hold the current probe increment

 * NOTE: This function is not thread safe, nor may a particular thread
 * interleave calls that are logically associated with
 * different tables. */

#define HASH_COMPUTE_HASH(index_ptr, key_ptr, key_width, block_size,    \
                          hash_value, hash_probe_increment)             \
    {                                                                   \
        if ((hash_probe_increment) == 0) {                              \
            (hash_value) = hash((key_ptr), (key_width), 0);             \
            (hash_probe_increment) = (hash_value) | 0x01; /* must be odd */ \
        } else {                                                        \
            (hash_value) = (hash_value) + (hash_probe_increment);       \
        }                                                               \
        *(index_ptr) = (hash_value) & ((uint32_t) (block_size)-1);      \
    }



/* Choose the size for initial block. This will be a power of 2 with at
 * least MIN_BLOCK_SIZE entries that accomodates the data at a load
 * less than the given load factor. */
static uint32_t
hashlib_calculate_block_size(
    const HashTable    *table_ptr,
    uint32_t            estimated_count)
{
    uint32_t initial_size;
    uint32_t max_size = HASH_GET_MAX_BLOCK_ENTRIES(table_ptr);

    if (estimated_count < MIN_BLOCK_SIZE) {
        initial_size = MIN_BLOCK_SIZE;
    } else {
        initial_size = MIN_BLOCK_SIZE << 1;
        while (initial_size <= max_size) {
            if (initial_size
                >= ((((uint64_t) estimated_count) << 8)
                    / table_ptr->load_factor))
            {
                break;          /* big enough */
            }
            initial_size = initial_size << 1;   /* double it */
        }
    }
    return initial_size;
}


HashTable *
hashlib_create_table(
    uint8_t             key_width,
    uint8_t             value_width,
    uint8_t             value_type,
    uint8_t            *no_value_ptr,
    uint8_t            *appdata_ptr,
    uint32_t            appdata_size,
    uint32_t            estimated_count,
    uint8_t             load_factor)
{
    HashTable *table_ptr = NULL;
    HashBlock *block_ptr = NULL;
    uint32_t initial_size;

    /* Validate arguments */
    assert(value_type == HTT_INPLACE || value_type == HTT_BYREFERENCE);
    if ((value_type != HTT_INPLACE) && (value_type != HTT_BYREFERENCE)) {
        TRACEMSG(1,("hashlib_create_table: invalid value_type argument %d.",
                    value_type));
        assert(0);
        return NULL;
    }

    /* Allocate memory for the table and initialize attributes.  */
    table_ptr = (HashTable*)calloc(1, sizeof(HashTable));
    if (table_ptr == NULL) {
        TRACEMSG(1,("Failed to allocate new HashTable."));
        return NULL;
    }

    /* Initialize the table structure */
    table_ptr->value_type = value_type;
    table_ptr->key_width = key_width;
    table_ptr->value_width = value_width;
    table_ptr->load_factor = load_factor;

    /* Application data */
    table_ptr->appdata_ptr = appdata_ptr;
    table_ptr->appdata_size = appdata_size;

    /* Initialize value_ptr to string of zero-valued bytes if NULL */
    table_ptr->no_value_ptr
        = (uint8_t*)calloc(table_ptr->value_width, sizeof(uint8_t));
    if (table_ptr->no_value_ptr == NULL) {
        free(table_ptr);
        TRACEMSG(1,("Failed to allocate new no_value_ptr for new HashTable."));
        return NULL;
    }
    if (no_value_ptr == NULL) {
        table_ptr->can_memset_val = 1;
    } else if (table_ptr->value_width == 1) {
        table_ptr->can_memset_val = 1;
        table_ptr->no_value_ptr[0] = no_value_ptr[0];
    } else {
        uint32_t i;

        memcpy(table_ptr->no_value_ptr, no_value_ptr, table_ptr->value_width);

        /* Determine if no_value_ptr contains a single byte value
         * repeated (such as all 0's or all 1's, so we can memset the
         * memory in one step.  Assume we can, and unset if we
         * can't. */
        table_ptr->can_memset_val = 1;
        for (i = 1; i < table_ptr->value_width; ++i) {
            if (no_value_ptr[0] != no_value_ptr[i]) {
                table_ptr->can_memset_val = 0;
                break;
            }
        }
    }

    /* Calculate initial block size */
    initial_size = hashlib_calculate_block_size(table_ptr, estimated_count);

    TRACEMSG(1,("Adding block #1..."));

    /* Start with one block */
    table_ptr->num_blocks = 1;
    block_ptr = hashlib_create_block(table_ptr, initial_size);
    if (block_ptr == NULL) {
        free(table_ptr->no_value_ptr);
        free(table_ptr);
        return NULL;
    }
    table_ptr->block_ptrs[0] = block_ptr;

    TRACEMSG(1,("Added block #%u.", table_ptr->num_blocks));

    return table_ptr;
}


/* Note: doesn't free app data.  This is the responsibility of the
 * caller (e.g., the wrapper) */
void
hashlib_free_table(
    HashTable          *table_ptr)
{
    int i;

    /* Free all the blocks in the table */
    for (i = 0; i < table_ptr->num_blocks; i++) {
        hashlib_free_block(table_ptr->block_ptrs[i]);
    }

    /* Free the empty pointer memory */
    free(table_ptr->no_value_ptr);

    if (table_ptr->cmp_fn == hashlib_cmp_fn && table_ptr->user_data) {
        free(table_ptr->user_data);
    }

    /* Free the table structure itself */
    free(table_ptr);
}


/* NOTE: Assumes block_size is a power of 2.  Very important! */
static HashBlock *
hashlib_create_block(
    HashTable          *table_ptr,
    uint32_t            block_size)
{
    HashBlock *block_ptr;
    uint32_t entry_i;

    HASH_ASSERT_SIZE_IS_POWER_2(block_size);

#ifdef HASHLIB_RECORD_STATS
    hashlib_stats_blocks_allocated++;
#endif

    TRACEMSG(1,(("Creating block; requesting %" PRIu32
                 " %" PRIu32 "-byte entries (%" PRIu32 " total bytes)..."),
                block_size, HASH_GET_ENTRY_SIZE(table_ptr),
                (HASH_GET_ENTRY_SIZE(table_ptr) * block_size)));

    /* Allocate memory for the block and initialize attributes.  */
    block_ptr = (HashBlock*)malloc(sizeof(HashBlock));
    if (block_ptr == NULL) {
        TRACEMSG(1,("Failed to allocate new HashBlock."));
        return NULL;
    }

    block_ptr->block_size = block_size;
    block_ptr->num_entries = 0;
    block_ptr->block_full = table_ptr->load_factor * (block_size >> 8);

    block_ptr->key_width = table_ptr->key_width;
    block_ptr->value_width = table_ptr->value_width;
    block_ptr->load_factor = table_ptr->load_factor;
    block_ptr->no_value_ptr = table_ptr->no_value_ptr;

    /* Allocate memory for the data */
    block_ptr->_data_ptr = (uint8_t*)malloc(HASH_GET_ENTRY_SIZE(block_ptr)
                                            * block_ptr->block_size);
    if (block_ptr->_data_ptr == NULL) {
        free(block_ptr);
        TRACEMSG(1,("Failed to allocate new data block."));
        return NULL;
    }

    /* Copy "empty" value to each entry.  Garbage key values are
     * ignored, so we don't bother writing to the keys.  When the
     * application overestimates the amount of memory needed, this can
     * be bottleneck.  */
    if (table_ptr->can_memset_val) {
        memset(block_ptr->_data_ptr, block_ptr->no_value_ptr[0],
               (block_ptr->block_size * HASH_GET_ENTRY_SIZE(block_ptr)));
    } else {
        uint8_t *data_ptr = block_ptr->_data_ptr;
        uint8_t entry_width = HASH_GET_ENTRY_SIZE(block_ptr);

        /* Point to first value */
        data_ptr += block_ptr->key_width;
        for (entry_i = 0; entry_i < block_ptr->block_size; entry_i++) {
            memcpy(data_ptr, block_ptr->no_value_ptr, block_ptr->value_width);
            /* Advance to next */
            data_ptr += entry_width;
        }
    }

    return block_ptr;
}


static void
hashlib_free_block(
    HashBlock          *block_ptr)
{
    /* Free the data and the table itself */
    free(block_ptr->_data_ptr);
    free(block_ptr);
}


/* Rehash entire table into a single block */
int
hashlib_rehash(
    HashTable          *table_ptr)
{
    HashBlock *new_block_ptr = NULL;
    HashBlock *block_ptr = NULL;
    uint64_t num_entries = 0;
    uint32_t initial_size;
    uint8_t *key_ref;
    uint8_t *val_ref;
    uint8_t *entry_ptr;
    uint8_t *new_entry_ptr;
    int rv;
    int i;
    uint32_t block_index;
    uint32_t idx;
    uint32_t max_entries;

#ifdef HASHLIB_RECORD_STATS
    hashlib_stats_rehashes++;
#endif

    if (table_ptr->is_sorted) {
        TRACEMSG(1,("ERROR: Attempt to rehash a sorted table"));
        assert(0 == table_ptr->is_sorted);
        return ERR_SORTTABLE;
    }

    max_entries = HASH_GET_MAX_BLOCK_ENTRIES(table_ptr);

    /* Count the total number of entries so we know what we need to
     * allocate.  We base this on the actual size of the blocks, and
     * use the power of 2 that's double the smallest power of 2 bigger
     * than the sum of block sizes. It's justified by the intuition
     * that once we reach this point, we've decided that we're going
     * to explore an order of magnitude larger table. This particular
     * scheme seems to work well in practice although it's difficult
     * to justify theoretically--this is a rather arbitrary definition
     * of "order of magnitude". */
    for (i = 0; i < table_ptr->num_blocks; i++) {
        num_entries += table_ptr->block_ptrs[i]->block_size;
    }

    TRACEMSG(1,(("Rehashing table having %" PRIu64
                 " %" PRIu32 "-byte entries..."),
                num_entries, HASH_GET_ENTRY_SIZE(table_ptr)));

    if (num_entries > max_entries) {
        TRACEMSG(1,(("Too many entries for rehash; "
                     " num_entries=%" PRIu64 " > max_entries=%" PRIu32 "."),
                    num_entries, max_entries));
        return ERR_OUTOFMEMORY;
    }

#if 0                           /* block_size should account for padding */

    /* .. and add the padding we need. */
    num_entries = ((num_entries * 255) / table_ptr->load_factor);
#endif

    /* Choose the size for the initial block. */
    initial_size = MIN_BLOCK_SIZE;      /* Minimum size */
    for (i = 0; i < 24; i++) {
        if (initial_size >= num_entries) {
            /* big enough */
            break;
        }
        /* double it */
        initial_size <<= 1;
    }

    /* double it once more */
    if (max_entries > (initial_size << 1)) {
        initial_size <<= 1;
    }
    if (initial_size > max_entries) {
        TRACEMSG(1,(("Will not rehash table; new initial_size=%" PRIu32
                     " > max_entries=%" PRIu32 "."),
                    initial_size, max_entries));
        return ERR_OUTOFMEMORY;
    }

    TRACEMSG(1,("Allocating new rehash block..."));

    /* Create the new block */
    new_block_ptr = hashlib_create_block(table_ptr, initial_size);
    if (new_block_ptr == NULL) {
        TRACEMSG(1,(("Allocating rehash block failed for %" PRIu32 " entries."),
                    initial_size));
        return ERR_OUTOFMEMORY;
    }
    TRACEMSG(1,("Allocated rehash block."));

    /* Walk through the table looking for non-empty entries, and
     * insert them into the new block. */
    for (block_index = table_ptr->num_blocks; block_index > 0; ) {
        --block_index;
        block_ptr = table_ptr->block_ptrs[block_index];
        entry_ptr = HASH_GET_ENTRY(block_ptr, 0);
        for (idx = 0; idx < block_ptr->block_size; idx++) {
            key_ref = HASH_GET_KEY(block_ptr, entry_ptr);
            val_ref = HASH_GET_VALUE(block_ptr, entry_ptr);

            /* If not empty, then copy the entry into the new block */
            if (!HASH_VALUE_ISEMPTY(block_ptr, val_ref)) {
                rv = hashlib_block_find_entry(new_block_ptr,
                                              key_ref, &new_entry_ptr);
                if (rv != ERR_NOTFOUND) {
                    /* value is not-empty, but we cannot find the key
                     * in the hash table. either the hashlib code is
                     * broken, or the user set a value to the
                     * no_value_ptr value and broke the collision
                     * resolution mechanism.  if assert() is active,
                     * the next line will call abort(). */
                    assert(rv == ERR_NOTFOUND);
                    free(new_block_ptr);
                    table_ptr->num_blocks = 1 + block_index;
                    return ERR_INTERNALERROR;
                }
                /* Copy the key and value */
                memcpy(HASH_GET_KEY(new_block_ptr, new_entry_ptr), key_ref,
                       HASH_GET_KEY_SIZE(block_ptr));
                memcpy(HASH_GET_VALUE(new_block_ptr, new_entry_ptr), val_ref,
                       HASH_GET_VALUE_SIZE(block_ptr));
                new_block_ptr->num_entries++;

#ifdef HASHLIB_RECORD_STATS
                hashlib_stats_rehash_inserts++;
#endif
            } /* !HASH_VALUE_ISEMPTY */
            entry_ptr += HASH_GET_ENTRY_SIZE(block_ptr);
        }                       /* entries */

        /* Free the block */
        hashlib_free_block(block_ptr);
        table_ptr->block_ptrs[block_index] = NULL;
    }                           /* blocks */

    /* Associate the new block with the table */
    table_ptr->num_blocks = 1;
    table_ptr->block_ptrs[0] = new_block_ptr;

    TRACEMSG(1,("Rehashed table."));

    return OK;
}


/* Add a new block to a table. */
static int
hashlib_add_block(
    HashTable          *table_ptr,
    uint32_t            new_block_size)
{
    HashBlock *block_ptr = NULL;

    assert(table_ptr->num_blocks < MAX_BLOCKS);
    if (table_ptr->num_blocks >= MAX_BLOCKS) {
        TRACEMSG(1,(("Cannot allocate another block:"
                     " num_blocks=%" PRIu32 " >= MAX_BLOCKS=%u."),
                  table_ptr->num_blocks, MAX_BLOCKS));
        return ERR_NOMOREBLOCKS;
    }
    /* Create the new block */
    TRACEMSG(1,(("Adding block #%u..."), 1 + table_ptr->num_blocks));
    block_ptr = hashlib_create_block(table_ptr, new_block_size);
    if (block_ptr == NULL) {
        TRACEMSG(1,("Adding block #%u failed.", 1+table_ptr->num_blocks));
        return ERR_OUTOFMEMORY;
    }

    /* Add it to the table */
    table_ptr->block_ptrs[table_ptr->num_blocks] = block_ptr;
    table_ptr->num_blocks++;
    TRACEMSG(1,("Added block #%u.", table_ptr->num_blocks));

    return OK;
}


/* See what size the next hash block should be. */
static uint32_t
hashlib_compute_next_block_size(
    HashTable          *table_ptr)
{
    uint32_t block_size = 0;

    /* This condition will only be true when the primary block has
     * reached the maximum block size. */
    if (table_ptr->num_blocks >= REHASH_BLOCK_COUNT) {
        return table_ptr->block_ptrs[table_ptr->num_blocks-1]->block_size;
    }
    /* Otherwise, it depends on current parameters */
    if (SECONDARY_BLOCK_FRACTION >= 0) {
        block_size =
            (table_ptr->block_ptrs[0]->block_size >> SECONDARY_BLOCK_FRACTION);
    } else if (SECONDARY_BLOCK_FRACTION == -1) {
        /* Keep halving blocks */
        block_size =
            (table_ptr->block_ptrs[table_ptr->num_blocks-1]->block_size >> 1);
    } else if (SECONDARY_BLOCK_FRACTION == -2) {
        if (table_ptr->num_blocks == 1) {
            /* First secondary block is 1/4 size of main block */
            block_size =
                table_ptr->block_ptrs[table_ptr->num_blocks-1]->block_size >>2;
        } else {
            /* Other secondary blocks are halved */
            block_size =
                table_ptr->block_ptrs[table_ptr->num_blocks-1]->block_size >>1;
        }
    } else if (SECONDARY_BLOCK_FRACTION == -3) {
        if (table_ptr->num_blocks == 1) {
            /* First secondary block is 1/2 size of main block */
            block_size = table_ptr->block_ptrs[0]->block_size >> 1;
        } else {
            /* All others are 1/4 size of main block */
            block_size = table_ptr->block_ptrs[0]->block_size >> 2;
        }
    } else if (SECONDARY_BLOCK_FRACTION == -4) {
        if (table_ptr->num_blocks == 1) {
            /* First secondary block is 1/4 size of main block */
            block_size = table_ptr->block_ptrs[0]->block_size >> 2;
        } else {
            /* All others are 1/8 size of main block */
            block_size = table_ptr->block_ptrs[0]->block_size >> 3;
        }
    } else {
        skAbort();
    }

    return block_size;
}

/*
 *  Algorithm:
 *  - If the primary block is at its maximum, never rehash, only add
 *    new blocks.
 *  - If we have a small table, then don't bother creating
 *    secondary tables.  Simply rehash into a new block.
 *  - If we've exceeded the maximum number of blocks, rehash
 *    into a new block.
 *  - Otherwise, create a new block
 */

static int
hashlib_resize_table(
    HashTable          *table_ptr)
{
    uint32_t new_block_size;
    int rv;

    TRACEMSG(1,("Resizing the table..."));

    /* Compute the (potential) size of the new block */
    new_block_size = hashlib_compute_next_block_size(table_ptr);
    assert(new_block_size != 0);

    /* If we're at the maximum number of blocks (which implies that
     * the first block is at its max, and we can't resize, then that's
     * it. */
    if (table_ptr->num_blocks == MAX_BLOCKS) {
        TRACEMSG(1,(("Unable to resize table: no more blocks;"
                     " table contains %" PRIu32 " %" PRIu32 "-byte entries"
                     " in %" PRIu32 " buckets across %u blocks"),
                    hashlib_count_entries(table_ptr),
                    HASH_GET_ENTRY_SIZE(table_ptr),
                    hashlib_count_buckets(table_ptr), table_ptr->num_blocks));
        return ERR_NOMOREBLOCKS;
    }
    /* If the first block is at its maximum size or if we have tried
     * and failed to rehash in the past, then add a new block. Once we
     * reach the maximum block size, we don't rehash.  Instead we keep
     * adding blocks until we reach the maximum. */
    if ((table_ptr->block_ptrs[0]->block_size
         == HASH_GET_MAX_BLOCK_ENTRIES(table_ptr))
        || table_ptr->rehash_failed)
    {
        assert(new_block_size > MIN_BLOCK_SIZE);
        return hashlib_add_block(table_ptr, new_block_size);
    }
    /* If we have REHASH_BLOCK_COUNT blocks, or the new block would be
     * too small, we simply rehash. */
    if ((new_block_size < MIN_BLOCK_SIZE) ||
        (table_ptr->num_blocks >= REHASH_BLOCK_COUNT))
    {
        TRACEMSG(1,(("Resize table forcing rehash; new_block_size = %" PRIu32
                     "; num_blocks = %u; REHASH_BLOCK_COUNT = %" PRIu32 "."),
                    new_block_size, table_ptr->num_blocks, REHASH_BLOCK_COUNT));
        rv = hashlib_rehash(table_ptr);
        if (rv != ERR_OUTOFMEMORY) {
            return rv;
        }
        /* rehashing failed.  try instead to add a new (small) block */
        table_ptr->rehash_failed = 1;
        if (new_block_size < MIN_BLOCK_SIZE) {
            new_block_size = MIN_BLOCK_SIZE;
        }
        TRACEMSG(1,("Rehash failed; creating new block instead..."));
    }
    /* Assert several global invariants */
    assert(new_block_size >= MIN_BLOCK_SIZE);
    assert(new_block_size <= HASH_GET_MAX_BLOCK_ENTRIES(table_ptr));
    assert(table_ptr->num_blocks < MAX_BLOCKS);

    /* Otherwise, add new a new block */
    return hashlib_add_block(table_ptr, new_block_size);
}


#if 0
static void
assert_not_already_there(
    HashTable          *table_ptr,
    uint8_t            *key_ptr)
{
    uint8_t *entry_ptr;
    HashBlock *block_ptr;
    int i;
    int rv;

    for (i = 0; i < (table_ptr->num_blocks-1); i++) {
        block_ptr = table_ptr->block_ptrs[i];
        rv = hashlib_block_find_entry(block_ptr, key_ptr, &entry_ptr);
        if (rv == OK) {
            getc(stdin);
        }
    }
}
#endif /* 0 */


#define HASHLIB_BLOCK_IS_FULL(blk_ptr)                  \
    ((blk_ptr)->num_entries >= (blk_ptr)->block_full)


int
hashlib_insert(
    HashTable          *table_ptr,
    const uint8_t      *key_ptr,
    uint8_t           **value_pptr)
{
    uint32_t i;
    int rv;
    HashBlock *block_ptr = NULL;
    uint8_t *current_entry_ptr = NULL;

#ifdef HASHLIB_RECORD_STATS
    hashlib_stats_inserts++;
#endif

    if (table_ptr->is_sorted) {
        assert(0 == table_ptr->is_sorted);
        return ERR_SORTTABLE;
    }

    /* Before doing anything else, see if we're ready to do a resize
     * by either adding a block or rehashing. */
    if (HASHLIB_BLOCK_IS_FULL(table_ptr->block_ptrs[table_ptr->num_blocks-1])){
        rv = hashlib_resize_table(table_ptr);
        if (rv != OK) {
            return rv;
        }
    }

    /* Look for the key in all blocks */
    for (i = 0; i < table_ptr->num_blocks; ++i) {
        block_ptr = table_ptr->block_ptrs[i];
        rv = hashlib_block_find_entry(block_ptr, key_ptr, &current_entry_ptr);
        if (rv == OK) {
            /* Found entry, use it */
            *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
            return OK_DUPLICATE;
        } else if (rv != ERR_NOTFOUND) {
            assert(0);
            /* An error of some sort occurred */
            return rv;
        }
    }

    /* Didn't find it, so do an insert into the last block by setting
     * the key AND increasing the count. The caller will set the
     * value. Note: block_ptr is pointing at the last block. */
    *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
    memcpy(HASH_GET_KEY(block_ptr, current_entry_ptr),
           key_ptr, block_ptr->key_width);
    block_ptr->num_entries++;

    return OK;
}



#if 0
/* NOTE: Since we're returning a reference to the value, the user
 * could mistakenly set the value to "empty" (e.g., put a null pointer
 * in an allocated entry).  This is problematic, since the internal
 * count will have been incremented even though in essence no entry
 * has been added.  I don't know the proper solution. */
static int
hashlib_block_insert(
    HashBlock          *block_ptr,
    uint8_t            *key_ptr,
    uint8_t           **value_pptr)
{
    uint8_t *current_entry_ptr = NULL;
    int rv;

    /* Assert block won't exceed load factor */
    assert(!HASHLIB_BLOCK_IS_FULL(block_ptr));
    /*
     * assert(!((block_ptr->num_entries / ((block_ptr->block_size) >> 8)) >
     *          block_ptr->load_factor));
     */

    /* Find a slot with the specified key, or a free slot in the hash
     * table */
    rv = hashlib_block_find_entry(block_ptr, key_ptr, &current_entry_ptr);
    if (rv == OK) {
        /* Return pointer to the value in the entry structure */
        *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
        return OK_DUPLICATE;
    } else if (rv == ERR_NOTFOUND) {
        /* Got an empty entry back -- fill it the key */
        memcpy(HASH_GET_KEY(block_ptr, current_entry_ptr),
               key_ptr, block_ptr->key_width);
        /* Return pointer to the value in the entry structure */
        *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
        /* Update the number of entries */
        (block_ptr->num_entries)++;
        return OK;
    } else {
        return rv;
    }
}
#endif /* 0 */


int
hashlib_lookup(
    const HashTable    *table_ptr,
    const uint8_t      *key_ptr,
    uint8_t           **value_pptr)
{
    uint8_t i;

#ifdef HASHLIB_RECORD_STATS
    hashlib_stats_lookups++;
#endif

    if (table_ptr->is_sorted) {
        assert(0 == table_ptr->is_sorted);
        return ERR_SORTTABLE;
    }

    /* Look in each block for the entry */
    for (i = 0; i < table_ptr->num_blocks; i++) {
        if (hashlib_block_lookup(table_ptr->block_ptrs[i], key_ptr, value_pptr)
            == OK)
        {
            return OK;
        }
    }
    return ERR_NOTFOUND;
}


static int
hashlib_block_lookup(
    const HashBlock    *block_ptr,
    const uint8_t      *key_ptr,
    uint8_t           **value_pptr)
{
    uint8_t *current_entry_ptr = NULL;
    int rv = hashlib_block_find_entry(block_ptr, key_ptr, &current_entry_ptr);

    if (rv == OK) {
        /* Return pointer to the value in the entry structure */
        *value_pptr = HASH_GET_VALUE(block_ptr, current_entry_ptr);
        return OK;
    } else {
        return ERR_NOTFOUND;
    }

}


/* If not found, points to insertion point */
static int
hashlib_block_find_entry(
    const HashBlock    *block_ptr,
    const uint8_t      *key_ptr,
    uint8_t           **entry_pptr)
{
#ifndef NDEBUG
    uint32_t num_tries = 0;
#endif
    uint32_t hash_index = 0;
    uint8_t *current_entry_ptr = NULL;
    uint32_t hash_value = 0;
    uint32_t hash_probe_increment = 0;

    /*
     *  This code computes the hash for the key, and finds the bucket
     *  at that hash-value.  If the bucket is empty, this function is
     *  done; it passes back a handle to that bucket and returns
     *  ERR_NOTFOUND.
     *
     *  If the bucket is not empty, the function checks to see if
     *  bucket's key matches the key passed into the function.  If the
     *  keys match, the entry is found; the function passes back
     *  a handle that bucket and returns OK.
     *
     *  If the keys do not match, it means multiple keys return the
     *  same hash value; that is, there is a collision.  The
     *  do..while() repeats and a new hash is computed---by
     *  incrementing the 'hash_probe_increment' value.
     *
     *  The process repeats until either an empty bucket is found or
     *  the keys match.
     *
     *  This collision resolution mechanism is what makes removal
     *  impossible---unless a "deleted-entry" value is used or the
     *  entire table is rehashed after a deletion.  It is also why the
     *  caller must never set a bucket's value to the no_value_ptr
     *  value.
     */
    do {
        HASH_COMPUTE_HASH(&hash_index, key_ptr, block_ptr->key_width,
                          block_ptr->block_size,
                          hash_value, hash_probe_increment);

        current_entry_ptr = HASH_GET_ENTRY(block_ptr, hash_index);

        /* Hit an empty entry, we're done. */
        if (HASH_ENTRY_ISEMPTY(block_ptr, current_entry_ptr)) {
            *entry_pptr = current_entry_ptr;
            return ERR_NOTFOUND;
        }
        assert(++num_tries < block_ptr->block_size);

    } while (!HASH_COMPARE_KEYS(HASH_GET_KEY(block_ptr, current_entry_ptr),
                                key_ptr, block_ptr->key_width));

    /* Found it. */
    *entry_pptr = current_entry_ptr;
    return OK;
}


static uint32_t
hashlib_block_count_nonempties(
    const HashBlock    *block_ptr)
{
    uint32_t i;
    uint32_t count = 0;
    uint8_t *entry_ptr;

    for (i = 0, entry_ptr = HASH_GET_ENTRY(block_ptr, 0);
         i < block_ptr->block_size;
         ++i, entry_ptr += HASH_GET_ENTRY_SIZE(block_ptr))
    {
        if (!HASH_ENTRY_ISEMPTY(block_ptr, entry_ptr)) {
            ++count;
        }
    }
    return count;
}


void
hashlib_dump_bytes(
    FILE               *fp,
    const uint8_t      *data_ptr,
    uint32_t            data_size)
{
    uint32_t j;

    for (j = 0; j < data_size; j++) {
        fprintf(fp, "%02x ", data_ptr[j]);
    }
}


static void
hashlib_dump_block_header(
    FILE               *fp,
    const HashBlock    *block_ptr)
{
    /* Dump header info */
    fprintf(fp, ("Block size: \t %" PRIu32 "\n"), block_ptr->block_size);
    fprintf(fp, ("Num entries:\t %" PRIu32 " (%2.0f%% full)\n"),
            block_ptr->num_entries,
            100 * (float) block_ptr->num_entries / block_ptr->block_size);
    fprintf(fp, "Key width:\t %u bytes\n", block_ptr->key_width);
    fprintf(fp, "Value width:\t %u bytes\n", block_ptr->value_width);
    fprintf(fp, "Load factor:\t %u = %2.0f%%\n",
            block_ptr->load_factor,
            100 * (float) block_ptr->load_factor / 255);
    fprintf(fp, "Empty value representation: ");
    hashlib_dump_bytes(fp, block_ptr->no_value_ptr, block_ptr->value_width);
    fprintf(fp, "\n");
}


static void
hashlib_dump_block(
    FILE               *fp,
    const HashBlock    *block_ptr)
{
    uint32_t i;                 /* Index of into hash table */
    uint32_t entry_index = 0;
    uint8_t *entry_ptr;

    hashlib_dump_block_header(fp, block_ptr);
    fprintf(fp, "Data Dump:\n");
    fprintf(fp, "----------\n");
    for (i = 0; i < block_ptr->block_size; i++) {
        entry_ptr = HASH_GET_ENTRY(block_ptr, i);
        /* Don't dump empty entries */
        if (HASH_ENTRY_ISEMPTY(block_ptr, entry_ptr)) {
            continue;
        }
        entry_index++;

        /* Dump hash index in table, the key and the value */
        fprintf(fp, ("%6" PRIu32 " (%" PRIu32 "). "), entry_index, i);
        hashlib_dump_bytes(fp, HASH_GET_KEY(block_ptr, entry_ptr),
                           block_ptr->key_width);
        fprintf(fp, " -- ");
        hashlib_dump_bytes(fp, HASH_GET_VALUE(block_ptr, entry_ptr),
                           block_ptr->value_width);
        fprintf(fp, "\n");
    }
}

#ifdef HASHLIB_RECORD_STATS
void
hashlib_clear_stats(
    void)
{
    hashlib_stats_inserts = 0;
    hashlib_stats_lookups = 0;
    hashlib_stats_rehashes = 0;
    hashlib_stats_rehash_inserts = 0;
    hashlib_stats_blocks_allocated = 0;
}


void
hashlib_dump_stats(
    FILE               *fp)
{
    fprintf(fp, "Accumulated statistics:\n");
    fprintf(fp, "  %" PRIu64 " total inserts.\n", hashlib_stats_inserts);
    fprintf(fp, "  %" PRIu64 " total lookups.\n", hashlib_stats_lookups);
    fprintf(fp, "  %" PRIu64 " total rehashes.\n", hashlib_stats_rehashes);
    fprintf(fp, "  %" PRIu64 " inserts due to rehashing.\n",
            hashlib_stats_rehash_inserts);
}
#endif /* HASHLIB_RECORD_STATS */


HASH_ITER
hashlib_create_iterator(
    const HashTable UNUSED(*table_ptr))
{
    HASH_ITER iter;

    memset(&iter, 0, sizeof(HASH_ITER));
    iter.block = HASH_ITER_BEGIN;
    return iter;
}


int
hashlib_iterate(
    const HashTable    *table_ptr,
    HASH_ITER          *iter_ptr,
    uint8_t           **key_pptr,
    uint8_t           **val_pptr)
{
#ifdef TRACEMSG_LEVEL
    static uint32_t so_far = 0;
#endif
    HashBlock *block_ptr;
    uint8_t *entry_ptr;

    if (iter_ptr->block == HASH_ITER_END) {
        return ERR_NOMOREENTRIES;
    }

    if (table_ptr->is_sorted && table_ptr->num_blocks > 1) {
        /* Use sorted iterator if we should */
        return hashlib_iterate_sorted(table_ptr, iter_ptr, key_pptr, val_pptr);
    }

    /* Start at the first entry in the first block or increment the
     * iterator to start looking at the next entry. */
    if (iter_ptr->block == HASH_ITER_BEGIN) {
        /* Initialize the iterator. */
        memset(iter_ptr, 0, sizeof(HASH_ITER));
#ifdef TRACEMSG_LEVEL
        so_far = 0;
#endif
    } else {
        (iter_ptr->index)++;
    }

    /* Walk through indices of current block until we find a
     * non-empty.  Once we reach the end of the block, move on to the
     * next block. */
    while (iter_ptr->block < table_ptr->num_blocks) {

        /* Select the current block */
        block_ptr = table_ptr->block_ptrs[iter_ptr->block];

        /* Find the next non-empty entry in the current block (if
         * there is one). */
        for (entry_ptr = HASH_GET_ENTRY(block_ptr, iter_ptr->index);
             iter_ptr->index < block_ptr->block_size;
             ++iter_ptr->index, entry_ptr += HASH_GET_ENTRY_SIZE(block_ptr))
        {
            if (!HASH_ENTRY_ISEMPTY(block_ptr, entry_ptr)) {
                /* We found an entry, return it */
                *key_pptr = HASH_GET_KEY(block_ptr, entry_ptr);
                *val_pptr = HASH_GET_VALUE(block_ptr, entry_ptr);
#ifdef TRACEMSG_LEVEL
                so_far++;
#endif
                return OK;
            }
        }

        /* At the end of the block. */
#ifdef TRACEMSG_LEVEL
        TRACEMSG(2,(("Iterate. Moving to next block. So far %" PRIu32),
                    so_far));
        so_far = 0;
#endif

        /* try the next block */
        iter_ptr->block++;
        iter_ptr->index = 0;
    }

    /* We're past the last entry of the last block, so we're done. */
    *key_pptr = NULL;
    *val_pptr = NULL;
    iter_ptr->block = HASH_ITER_END;
#ifdef TRACEMSG_LEVEL
    TRACEMSG(2,("Iterate. No more entries."));
#endif
    return ERR_NOMOREENTRIES;
}


static int
hashlib_iterate_sorted(
    const HashTable    *table_ptr,
    HASH_ITER          *iter_ptr,
    uint8_t           **key_pptr,
    uint8_t           **val_pptr)
{
    uint8_t *lowest_entry = NULL;
    int k;

    assert(iter_ptr->block != HASH_ITER_END);

    /* Start at the first entry in the first block or increment the
     * iterator to start looking at the next entry. */
    if (iter_ptr->block == HASH_ITER_BEGIN) {
        /* Initialize the iterator. */
        memset(iter_ptr, 0, sizeof(HASH_ITER));
    } else {
        /* Increment the pointer in the block from which we took the
         * entry last time. */
        ++iter_ptr->block_idx[iter_ptr->block];
    }

    /* Find the first available value across all blocks; this is our
     * arbitrary "lowest" value. */
    for (k = 0; k < table_ptr->num_blocks; ++k) {
        if (iter_ptr->block_idx[k] < table_ptr->block_ptrs[k]->num_entries) {
            iter_ptr->block = k;
            lowest_entry = HASH_GET_ENTRY(table_ptr->block_ptrs[k],
                                          iter_ptr->block_idx[k]);
            break;
        }
    }

    if (k == table_ptr->num_blocks) {
        /* We've processed all blocks.  Done. */
        *key_pptr = NULL;
        *val_pptr = NULL;
        iter_ptr->block = HASH_ITER_END;
        return ERR_NOMOREENTRIES;
    }

    /* Compare our arbitrary "lowest" with every remaining block to
     * find the actual lowest. */
    for ( ++k; k < table_ptr->num_blocks; ++k) {
        if ((iter_ptr->block_idx[k] < table_ptr->block_ptrs[k]->num_entries)
            && (table_ptr->cmp_fn(HASH_GET_ENTRY(table_ptr->block_ptrs[k],
                                                 iter_ptr->block_idx[k]),
                                  lowest_entry,
                                  table_ptr->user_data)
                < 0))
        {
            iter_ptr->block = k;
            lowest_entry = HASH_GET_ENTRY(table_ptr->block_ptrs[k],
                                          iter_ptr->block_idx[k]);
        }
    }

    /* return lowest */
    *key_pptr = HASH_GET_KEY(table_ptr, lowest_entry);
    *val_pptr = HASH_GET_VALUE(table_ptr, lowest_entry);
    return OK;
}


uint32_t
hashlib_count_buckets(
    const HashTable    *table_ptr)
{
    int i;
    uint32_t total = 0;

    for (i = 0; i < table_ptr->num_blocks; i++) {
        total += table_ptr->block_ptrs[i]->block_size;
    }
    return total;
}


uint32_t
hashlib_count_entries(
    const HashTable    *table_ptr)
{
    int i;
    uint32_t total = 0;

    for (i = 0; i < table_ptr->num_blocks; i++) {
        total += table_ptr->block_ptrs[i]->num_entries;
    }
    return total;
}


uint32_t
hashlib_count_nonempties(
    const HashTable    *table_ptr)
{
    int i;
    uint32_t total = 0;
    uint32_t bc;

    for (i = 0; i < table_ptr->num_blocks; i++) {
        bc = hashlib_block_count_nonempties(table_ptr->block_ptrs[i]);
        total += bc;
        TRACEMSG(2,(("nonempty count for block %d is %" PRIu32 "."), i, bc));
    }
    return total;
}


static int
hashlib_cmp_fn(
    const void         *a,
    const void         *b,
    void               *v_length)
{
    return memcmp(a, b, *(size_t*)v_length);
}


/* move the entries in each block to the front of the block, in
 * preparation for sorting the entries */
static void
hashlib_make_contiguous(
    HashTable          *table_ptr)
{
    HashBlock *block_ptr;
    uint32_t i, j;
    int k;
    uint8_t *entry_i;
    uint8_t *entry_j;
    uint32_t entry_size = HASH_GET_ENTRY_SIZE(table_ptr);
    uint32_t value_size = HASH_GET_VALUE_SIZE(table_ptr);

    for (k = 0; k < table_ptr->num_blocks; ++k) {
        block_ptr = table_ptr->block_ptrs[k];
        if (0 == block_ptr->num_entries) {
            continue;
        }

        /* 'j' starts from the front of the block and moves forward to
         * find empty entries.  'i' starts from the end of the block
         * and moves backward to find occupied entries.  We move
         * non-empty entries from 'i' to 'j' to get rid of holes in
         * the block.  Stop once i and j meet. */
        for (j = 0, entry_j = HASH_GET_ENTRY(block_ptr, 0);
             j < block_ptr->block_size;
             j++, entry_j += entry_size)
        {
            if (HASH_ENTRY_ISEMPTY(block_ptr, entry_j)) {
                break;
            }
        }

        for (i = block_ptr->block_size-1, entry_i =HASH_GET_ENTRY(block_ptr,i);
             i > j;
             --i, entry_i -= entry_size)
        {
            if (!HASH_ENTRY_ISEMPTY(block_ptr, entry_i)) {
                memcpy(entry_j, entry_i, entry_size);
                /* set i to the empty value */
                memcpy(HASH_GET_VALUE(block_ptr, entry_i),
                       block_ptr->no_value_ptr, value_size);
                /* find next empty value */
                for (++j, entry_j += entry_size;
                     j < i;
                     ++j, entry_j += entry_size)
                {
                    if (HASH_ENTRY_ISEMPTY(block_ptr, entry_j)) {
                        break;
                    }
                }
            }
        }

        assert(j <= block_ptr->num_entries);
    }
}


static int
hashlib_sort_entries_helper(
    HashTable          *table_ptr,
    int               (*cmp_fn)(const void *a, const void *b, void *user_data),
    void               *user_data)
{
    HashBlock *block_ptr;
    size_t entry_size = HASH_GET_ENTRY_SIZE(table_ptr);
    int k;

    if (!table_ptr->is_sorted) {
        /* first call; make the data in each block contiguous */
        hashlib_make_contiguous(table_ptr);
    }

    if (cmp_fn) {
        if (table_ptr->cmp_fn == hashlib_cmp_fn && table_ptr->user_data) {
            free(table_ptr->user_data);
        }
        table_ptr->cmp_fn = cmp_fn;
        table_ptr->user_data = user_data;
    } else if (table_ptr->cmp_fn != hashlib_cmp_fn) {
        table_ptr->cmp_fn = &hashlib_cmp_fn;
        table_ptr->user_data = malloc(sizeof(size_t));
        if (NULL == table_ptr->user_data) {
            return ERR_OUTOFMEMORY;
        }
        *(size_t*)table_ptr->user_data = HASH_GET_KEY_SIZE(table_ptr);
    }

    /* we use qsort to sort each block individually; when iterating,
     * return the lowest value among all sorted blocks. */
    for (k = 0; k < table_ptr->num_blocks; ++k) {
        block_ptr = table_ptr->block_ptrs[k];
        /* sort the block's entries */
        skQSort_r(block_ptr->_data_ptr, block_ptr->num_entries,
                  entry_size, table_ptr->cmp_fn, table_ptr->user_data);
    }

    table_ptr->is_sorted = 1;
    return 0;
}


int
hashlib_sort_entries_usercmp(
    HashTable          *table_ptr,
    int               (*cmp_fn)(const void *a, const void *b, void *user_data),
    void               *user_data)
{
    if (NULL == cmp_fn) {
        return ERR_BADARGUMENT;
    }
    return hashlib_sort_entries_helper(table_ptr, cmp_fn, user_data);
}

int
hashlib_sort_entries(
    HashTable          *table_ptr)
{
    return hashlib_sort_entries_helper(table_ptr, NULL, NULL);
}


void
hashlib_dump_table(
    FILE               *fp,
    const HashTable    *table_ptr)
{
    int i;

    hashlib_dump_table_header(fp, table_ptr);
    for (i = 0; i < table_ptr->num_blocks; i++) {
        fprintf(fp, "Block %d:\n", i);
        hashlib_dump_block(fp, table_ptr->block_ptrs[i]);
    }
}


void
hashlib_dump_table_header(
    FILE               *fp,
    const HashTable    *table_ptr)
{
    int i;
    HashBlock *block_ptr;
    uint32_t total_used_memory = 0;
    uint32_t total_data_memory = 0;

    /* Dump header info */
    fprintf(fp, "Key width:\t %d bytes\n", table_ptr->key_width);
    fprintf(fp, "Value width:\t %d bytes\n", table_ptr->value_width);
    if (table_ptr->value_type == HTT_INPLACE) {
        fprintf(fp, "Value type:\t in-place value\n");
    } else if (table_ptr->value_type == HTT_BYREFERENCE) {
        fprintf(fp, "Value type:\t reference\n");
    } else {
        fprintf(fp, "Value type:\t #ERROR\n");
    }
    fprintf(fp, "Empty value:\t");
    hashlib_dump_bytes(fp, table_ptr->no_value_ptr, table_ptr->value_width);
    fprintf(fp, "\n");
    fprintf(fp, ("App data size:\t %" PRIu32 " bytes\n"),
            table_ptr->appdata_size);
    fprintf(fp, "Load factor:\t %d = %2.0f%%\n",
            table_ptr->load_factor,
            100 * (float) table_ptr->load_factor / 255);
    fprintf(fp, ("Table has %" PRIu8 " blocks:\n"), table_ptr->num_blocks);
    for (i = 0; i < table_ptr->num_blocks; i++) {
        block_ptr = table_ptr->block_ptrs[i];
        total_data_memory +=
            HASH_GET_ENTRY_SIZE(block_ptr) * block_ptr->block_size;
        total_used_memory +=
            HASH_GET_ENTRY_SIZE(block_ptr) * block_ptr->num_entries;
        fprintf(fp, ("  Block #%d: %" PRIu32 "/%" PRIu32 " (%3.1f%%)\n"),
                i, block_ptr->num_entries, block_ptr->block_size,
                100 * ((float)block_ptr->num_entries) / block_ptr->block_size);
    }
    fprintf(fp, ("Total data memory:           %" PRIu32 " bytes\n"),
            total_data_memory);
    fprintf(fp, ("Total allocated data memory: %" PRIu32 " bytes\n"),
            total_used_memory);
    fprintf(fp, ("Excess data memory:          %" PRIu32 " bytes\n"),
            total_data_memory - total_used_memory);
    fprintf(fp, "\n");
}


#if 0
static int
hashlib_write_block_header(
    const HashBlock    *block_ptr,
    FILE               *output_fp)
{
    size_t write_count;

    /* Write out the block attributes */
    write_count = fwrite(&(block_ptr->block_size),
                         sizeof(block_ptr->block_size), 1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    write_count = fwrite(&(block_ptr->num_entries),
                         sizeof(block_ptr->num_entries), 1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }
    return OK;
}
#endif  /* 0 */


static int
hashlib_write_block(
    const HashBlock    *block_ptr,
    FILE               *output_fp)
{
    size_t write_count;

    /* Write out the actual data */
    write_count = fwrite(block_ptr->_data_ptr,
                         block_ptr->value_width + block_ptr->key_width,
                         block_ptr->block_size, output_fp);
    assert(write_count == block_ptr->block_size);
    if (write_count != block_ptr->block_size) {
        return ERR_FILEWRITEERROR;
    }

    return OK;
}


#if 0
static int
hashlib_read_block_header(
    BlockHeader        *header_ptr,
    FILE               *input_fp)
{
    size_t read_count;

    /* Read block attributes */
    read_count = fread(&(header_ptr->block_size),
                       sizeof(header_ptr->block_size),
                       1, input_fp);
    assert(read_count == 1);
    if (read_count != 1) {
        return ERR_FILEREADERROR;
    }
    read_count = fread(&(header_ptr->num_entries),
                       sizeof(header_ptr->num_entries),
                       1, input_fp);
    assert(read_count == 1);
    if (read_count != 1) {
        return ERR_FILEREADERROR;
    }
    return OK;
}
#endif  /* 0 */


static int
hashlib_read_block(
    HashBlock         **block_pptr,
    HashTable          *table_ptr,
    const BlockHeader  *block_header_ptr,
    FILE               *input_fp)
{
    HashBlock *block_ptr;
    size_t read_count;

    /* Create a new block to hold the data */
    block_ptr = hashlib_create_block(table_ptr, block_header_ptr->block_size);
    if (block_ptr == NULL) {
        return ERR_OUTOFMEMORY;
    }
    block_ptr->num_entries = block_header_ptr->num_entries;

    /* Read the table data right into the array directly */
    read_count = fread(block_ptr->_data_ptr,
                       table_ptr->key_width + table_ptr->value_width,
                       block_ptr->block_size,
                       input_fp);
    assert(read_count == block_ptr->block_size);
    if (read_count != block_ptr->block_size) {
        free(block_ptr);
        return ERR_FILEREADERROR;
    }

    /* Return the pointer to the block */
    *block_pptr = block_ptr;

    return OK;
}


int
hashlib_save_table(
    const HashTable    *table_ptr,
    const char         *filename_str,
    const uint8_t      *header_ptr,
    uint8_t             header_length)
{
    /* Open up a file for writing and serialize the data to the file */
    int rv;
    FILE *output_fp = fopen(filename_str, "wb");

    if (output_fp == NULL) {
        return ERR_FILEOPENERROR;
    }
    rv = hashlib_serialize_table(table_ptr, output_fp,
                                 header_ptr, header_length);

    fclose(output_fp);
    return rv;
}


int
hashlib_restore_table(
    HashTable         **table_pptr,
    const char         *filename_str,
    uint8_t            *header_ptr,
    uint8_t             header_length)
{
    int rv;

    /* Open up a file for reading and deserialize the data from the file */
    FILE *input_fp = fopen(filename_str, "rb");

    if (input_fp == NULL) {
        return ERR_FILEOPENERROR;
    }
    rv = hashlib_deserialize_table(table_pptr, input_fp,
                                   header_ptr, header_length);
    fclose(input_fp);
    return rv;
}


int
hashlib_serialize_table(
    const HashTable    *table_ptr,
    FILE               *output_fp,
    const uint8_t      *header_ptr,
    uint8_t             header_length)
{
    int rv = OK;
    int block_index;
    size_t write_count;

    /* For the moment, we only support serialization of values, not
     * references. */
    assert(table_ptr->value_type == HTT_INPLACE);
    if (table_ptr->value_type != HTT_INPLACE) {
        TRACEMSG(1,("ERROR:  Reference serialization not yet implemented."));
        return ERR_NOTSUPPORTED;
    }
    /* Write out the file's header (if any) */
    write_count = fwrite(header_ptr, 1, header_length, output_fp);
    assert(write_count == header_length);
    if (write_count != header_length) {
        return ERR_FILEWRITEERROR;
    }

    /* Write out table attributes */
    write_count = fwrite(&(table_ptr->key_width),
                         sizeof(table_ptr->key_width), 1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    write_count = fwrite(&(table_ptr->value_width),
                         sizeof(table_ptr->value_width), 1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    write_count = fwrite(&(table_ptr->load_factor),
                         sizeof(table_ptr->load_factor), 1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    write_count = fwrite(&(table_ptr->is_sorted),
                         sizeof(table_ptr->is_sorted), 1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    /* NOTE (2011-Jun): No handling of 'options' and del_value_ptr
     * fields, both of which are entirely unused. */

    /* Write out representation of no value */
    write_count = fwrite(table_ptr->no_value_ptr, table_ptr->value_width,
                         1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    /* Write out app data length */
    write_count = fwrite(&(table_ptr->appdata_size),
                         sizeof(table_ptr->appdata_size), 1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    /* Write out the app data */
    write_count = fwrite(table_ptr->appdata_ptr,
                         1, table_ptr->appdata_size, output_fp);
    assert(write_count == table_ptr->appdata_size);
    if (write_count != table_ptr->appdata_size) {
        return ERR_FILEWRITEERROR;
    }

    /* Write out number of blocks */
    write_count = fwrite(&(table_ptr->num_blocks),
                         sizeof(table_ptr->num_blocks), 1, output_fp);
    assert(write_count == 1);
    if (write_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    /* Write the block headers */
    for (block_index = 0; block_index < table_ptr->num_blocks; block_index++) {
        const HashBlock *block_ptr = table_ptr->block_ptrs[block_index];

        /* Write out the block attributes */
        write_count = fwrite(&(block_ptr->block_size),
                             sizeof(block_ptr->block_size), 1, output_fp);
        assert(write_count == 1);
        if (write_count != 1) {
            return ERR_FILEWRITEERROR;
        }

        write_count = fwrite(&(block_ptr->num_entries),
                             sizeof(block_ptr->num_entries), 1, output_fp);
        assert(write_count == 1);
        if (write_count != 1) {
            return ERR_FILEWRITEERROR;
        }
    }

    /* Write each of the blocks, one after the other */
    for (block_index = 0; block_index < table_ptr->num_blocks; block_index++) {
        rv = hashlib_write_block(table_ptr->block_ptrs[block_index],
                                 output_fp);
        if (rv != OK) {
            return rv;
        }
    }

    return OK;
}


int
hashlib_deserialize_table(
    HashTable         **table_pptr,
    FILE               *input_fp,
    uint8_t            *header_ptr,
    int                 header_length)
{
    HashTable *table_ptr;
    uint8_t key_width;
    uint8_t value_width;
    uint8_t load_factor;
    uint8_t is_sorted;
    uint32_t appdata_size;
    uint8_t *no_value_ptr;
    void *appdata_ptr;
    int block_index;
    BlockHeader block_headers[MAX_BLOCKS];
    size_t read_count;
    uint32_t i;

    assert(input_fp);

    memset(block_headers, 0, sizeof(block_headers));

    /* Read the file's header (the caller will interpret it) */
    read_count = fread(header_ptr, 1, header_length, input_fp);
    assert(read_count == (size_t) header_length);
    if (read_count != (size_t) header_length) {
        return ERR_FILEREADERROR;
    }

    /* Read the table attributes */
    read_count = fread(&key_width, sizeof(key_width), 1, input_fp);
    assert(read_count == 1);
    if (read_count != 1) {
        return ERR_FILEREADERROR;
    }

    read_count = fread(&value_width, sizeof(value_width), 1, input_fp);
    assert(read_count == 1);
    if (read_count != 1) {
        return ERR_FILEREADERROR;
    }

    read_count = fread(&load_factor, sizeof(load_factor), 1, input_fp);
    assert(read_count == 1);
    if (read_count != 1) {
        return ERR_FILEREADERROR;
    }

    read_count = fread(&is_sorted, sizeof(is_sorted), 1, input_fp);
    assert(read_count == 1);
    if (read_count != 1) {
        return ERR_FILEWRITEERROR;
    }

    /* NOTE (2011-Jun): No handling of 'options' and del_value_ptr
     * fields, both of which are entirely unused. */

    /* Read representation of an "empty" value */
    no_value_ptr = (uint8_t*)malloc(value_width);
    if (no_value_ptr == NULL) {
        return ERR_OUTOFMEMORY;
    }
    read_count = fread(no_value_ptr, value_width, 1, input_fp);
    assert(read_count == 1);
    if (read_count != 1) {
        free(no_value_ptr);
        return ERR_FILEREADERROR;
    }

    /* Read size of app data */
    read_count = fread(&appdata_size, sizeof(appdata_size), 1, input_fp);
    assert(read_count == 1);
    if (read_count != 1) {
        free(no_value_ptr);
        return ERR_FILEREADERROR;
    }
    if (appdata_size == 0) {
        appdata_ptr = NULL;
    } else {
        /* Allocate memory for the application data */
        appdata_ptr = malloc(appdata_size);
        if (appdata_ptr == NULL) {
            free(no_value_ptr);
            return ERR_OUTOFMEMORY;
        }
        /* Read application data */
        read_count = fread(appdata_ptr, 1, appdata_size, input_fp);
        assert(read_count == appdata_size);
        if (read_count != appdata_size) {
            free(no_value_ptr);
            free(appdata_ptr);
            return ERR_FILEREADERROR;
        }
    }

    /* Create a table structure using the values we just read */
    /* Allocate memory for the table and initialize attributes.  */
    table_ptr = (HashTable*)calloc(1, sizeof(HashTable));
    assert(table_ptr);
    if (table_ptr == NULL) {
        free(no_value_ptr);
        if (appdata_ptr) {
            free(appdata_ptr);
        }
        return ERR_OUTOFMEMORY;
    }
    /* Initialize the table structure */
    table_ptr->value_type = HTT_INPLACE;
    table_ptr->key_width = key_width;
    table_ptr->value_width = value_width;
    table_ptr->load_factor = load_factor;
    table_ptr->is_sorted = is_sorted;
    /* Application data */
    table_ptr->appdata_ptr = appdata_ptr;
    table_ptr->appdata_size = appdata_size;
    table_ptr->no_value_ptr = no_value_ptr;

    /* Determine if no_value_ptr contains a single byte value repeated
     * (such as all 0's or all 1's, so we can memset the memory in one
     * step.  Assume we can, and unset if we can't. */
    table_ptr->can_memset_val = 1;
    for (i = 1; i < table_ptr->value_width; ++i) {
        if (no_value_ptr[0] != no_value_ptr[i]) {
            table_ptr->can_memset_val = 0;
            break;
        }
    }

    /* Read the number of blocks */
    read_count = fread(&(table_ptr->num_blocks),
                       sizeof(table_ptr->num_blocks), 1, input_fp);
    if (read_count != 1) {
        hashlib_free_table(table_ptr);
        return ERR_FILEREADERROR;
    }
    if (table_ptr->num_blocks > MAX_BLOCKS) {
        TRACEMSG(1,(("ERROR: Serialized table num_blocks=%" PRIu32
                     " > MAX_BLOCKS=%u."),
                    table_ptr->num_blocks, MAX_BLOCKS));
        table_ptr->num_blocks = 0;
        hashlib_free_table(table_ptr);
        return ERR_NOTSUPPORTED;
    }

    /* Read the block header array */
    read_count = fread(block_headers, sizeof(BlockHeader),
                       table_ptr->num_blocks, input_fp);
    if (read_count != table_ptr->num_blocks) {
        table_ptr->num_blocks = 0;
        hashlib_free_table(table_ptr);
        return ERR_FILEREADERROR;
    }

    /* Read each of the blocks */
    for (block_index = 0; block_index < table_ptr->num_blocks; block_index++) {
        HashBlock *block_ptr;
        int rv;

        /* Read block based on block header */
        rv = hashlib_read_block(&block_ptr, table_ptr,
                                &(block_headers[block_index]),
                                input_fp);
        if (rv != OK) {
            assert(rv == OK);
            table_ptr->num_blocks = block_index;
            hashlib_free_table(table_ptr);
            return rv;
        }
        table_ptr->block_ptrs[block_index] = block_ptr;
    }

    *table_pptr = table_ptr;

    return OK;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
