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
**  hashlib.h
**
**    Defines interface to core hash library functions.
**
*/

#ifndef _HASHLIB_H
#define _HASHLIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_HASHLIB_H, "$SiLK: hashlib.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/**
 *  @file
 *
 *    Implementation of a hashtable.
 *
 *    This file is part of libsilk.
 */

/*
 *  **************************************************************************
 *
 *  Configuration values for hashlib. These values should be modified
 *  for your platform:
 *
 *      MAX_MEMORY_BLOCK
 */

/**
 *    Maximum size in bytes that may be allocated with malloc. Must be a
 *    power of two.
 */
#define MAX_MEMORY_BLOCK (1<<28)

/*
 *  **************************************************************************
 */



/**
 *    Return codes for functions in hashlib. Note that >= 0 are
 *    success codes.
 */

/**  function was successful */
#define OK 0
/**  entry already exists */
#define OK_DUPLICATE 1
/**  could not find entry */
#define ERR_NOTFOUND -1
/**  no more entries available */
#define ERR_NOMOREENTRIES -2
/**  no longer in use */
#define ERR_INDEXOUTOFBOUNDS -3
/**  could not open a file */
#define ERR_FILEOPENERROR -4
/**  illegal argument value */
#define ERR_BADARGUMENT -5
/**  corrupt internal state */
#define ERR_INTERNALERROR -6
/**  operation not supported for this table*/
#define ERR_NOTSUPPORTED -7
/**  read error (corrupt data file) */
#define ERR_FILEREADERROR -8
/**  write error */
#define ERR_FILEWRITEERROR -9
/**  attempt to operate on a sorted table */
#define ERR_SORTTABLE -10
/**  attempted to aloc > max. # of blocks */
#define ERR_NOMOREBLOCKS -254
/**  a call to malloc failed */
#define ERR_OUTOFMEMORY -255

/* Types of hash tables */

/** Let the library manage the memory used to store values. */
#define HTT_INPLACE 0

/** Application-managed memory. Indicates the hash table will manage
 * pointer storage, _not_ data storage. */
#define HTT_BYREFERENCE 1

/** Indicates table allows deletion. Items are only removed from the table
 * after a rehash. Deleted items have the value del_value_ptr. */
#define HTT_ALLOWDELETION 1

/** Default load is 192 (75%). Generally, this is the value that should
 * be passed to hashlib_create_table for the load factor. */
#define DEFAULT_LOAD_FACTOR 192

/** Maximum number of blocks ever allocated. Once the primary block
 * reaches the maximum block size, new blocks will be allocated
 * until this maximum is reached. */
#define MAX_BLOCKS 8

/** Maximum byte-length of the key */
#define HASHLIB_MAX_KEY_WIDTH    UINT8_MAX
/** Maximum byte-lenght of the value */
#define HASHLIB_MAX_VALUE_WIDTH  UINT8_MAX


/* Data types: Note that
 * these data structures should generally be regarded as a read-only
 * by hash table users. Attriutes in HashTable duplicated in HashBlock
 * are copied upon block creation.*/

/**  a HashTable maintains a set of HashBlocks. */
typedef struct HashBlock_st HashBlock;

/**  the HashTable structure */
typedef struct HashTable_st {
    /**  HTT_INPLACE or HTT_BYREFERENCE */
    uint8_t value_type;
    /**  HTT_ALLOWDELETION or 0 */
    uint8_t options;
    /**  Size of a key in bytes */
    uint8_t key_width;
    /**  Size of a value in bytes */
    uint8_t value_width;
    /**  Point at which to resize (fraction of 255) */
    uint8_t load_factor;
    /**  Number of blocks */
    uint8_t num_blocks;
    /**  Non-zero if rehashing has failed in the past */
    uint8_t rehash_failed;
    /**  Non-zero if hash entries are sorted */
    uint8_t is_sorted;
    /**  Non-zero if we can memset new memory to a value */
    uint8_t can_memset_val;
    /**  Size of application data block */
    uint32_t appdata_size;
    /**  Application data */
    void    *appdata_ptr;
    /**  Pointer to representation of an empty value */
    uint8_t *no_value_ptr;
    /**  Pointer to rep. of a deleted value */
    uint8_t *del_value_ptr;
    /**  Comparison function to use for a sorted table */
    int    (*cmp_fn)(const void *, const void *, void *);
    /**  Caller's argument to the comparison function */
    void    *user_data;
    /**  The blocks */
    HashBlock *block_ptrs[MAX_BLOCKS];
} HashTable;

/**  HashTable iteration object */
typedef struct HashIter_st {
    /* Current block. Negative value is beginning or end */
    int         block;
    /* Current index into block */
    uint32_t    index;
    /* When working with a sorted table, index into each block */
    uint32_t    block_idx[MAX_BLOCKS];
} HASH_ITER;


/**
 *    Creates a new hash table. The initial table includes a single
 *    block big enough to accomodate estimated_size entries with less
 *    than the specified load_factor.
 *
 *    Parameters:
 *
 *    key_width:      The width of a key in bytes.
 *    value_width:    The width of a value in bytes
 *    data_type:      One of HTT_INPLACE or HTT_BYREFERENCE, indicating
 *                    whether the application data is stored in the table,
 *                    or the value is a pointer to application data. Not
 *                    currently used (will be used by serialization code).
 *    no_value_ptr:   A sequence of value_width bytes used to represent
 *                    "no value" (i.e., an empty entry).  The hash table
 *                    makes a copy of this value.  If 'no_value_ptr' is
 *                    NULL, values will be initialized to all 0.
 *    app_data_ptr:   Pointer to application data. Generally used by a
 *                    wrapper to store wrapper-specific data.
 *    app_data_size:  The size of the data referenced by app_data_ptr in
 *                    bytes.
 *    estimated_size: An estimate of the number of unique entries that will
 *                    ultimately be inserted into the table.
 *    load_factor:    Generally, simply use DEFAULT_LOAD_FACTOR here. This
 *                    specifies what load level triggers the allocation of
 *                    a new hash block or a rehash (i.e., the maximum load
 *                    for a block). This is a percentage expressed as a
 *                    fraction of 255.
 *
 *    Returns:
 *
 *    A pointer to the new table.
 *    Will return NULL in the case of a memory allocation error.
 */
HashTable *
hashlib_create_table(
    uint8_t             key_width,
    uint8_t             value_width,
    uint8_t             data_type,
    uint8_t            *no_value_ptr,
    uint8_t            *app_data_ptr,
    uint32_t            app_data_size,
    uint32_t            estimated_size,
    uint8_t             load_factor);


/**
 *    Modifies the hash table so that hashlib_iterate() returns
 *    the entries sorted by their key.
 *
 *    NOTE THAT hashlib_iterate() WILL CONTINUE TO USE 'cmp_fn' AND
 *    'user_data'.  THE 'cmp_fn' AND 'user_data' MUST REMAIN VALID THE
 *    UNTIL THE TABLE IS DESTROYED!!!
 *
 *    The comparison function will be passed three parameters; the
 *    first two parameters are hash entry keys.  'cmp_fn' must return
 *    a value less than 0 if key 'a' is less than key 'b', greater
 *    than 0 if key 'a' is greater than key 'b', or 0 if the two keys
 *    are identical.  The 'user_data' parameter will be passed
 *    unchanged to the comparison function, which allows the
 *    comparison function to use additional data without using global
 *    variables.
 *
 *    NOTE: Once the hash table has been sorted, insert, lookup, and
 *    rehash are invalid.  One may only iterate over a sorted table or
 *    delete it.
 *
 *    Parameters:
 *
 *    table_ptr:      A pointer to the table to sort.
 *
 *    cmp_fn:         The comparison function to use to compare keys
 *
 *    user_data:      Value passed unchange to 'cmp_fn'.
 *
 *    Returns:
 *
 *    Returns OK in the case when the data in the table has been
 *    successfully sorted.
 */
int
hashlib_sort_entries_usercmp(
    HashTable      *table_ptr,
    int           (*cmp_fn)(const void *a, const void *b, void *user_data),
    void           *user_data);


/**
 *    A wrapper around hashlib_sort_entries_usercmp() that uses
 *    memcmp() to compare the keys.
 */
int
hashlib_sort_entries(
    HashTable          *table_ptr);


/*
 * NOT IMPLEMENTED
 */
int
hashlib_mark_deleted(
    HashTable          *table_ptr,
    const uint8_t      *key_ptr);


/**
 *    Inserts a new entry into the hash table, and returns a pointer
 *    to the memory in the hash table used to store the value. The
 *    application should store the value associated with the entry in
 *    *(value_pptr). If the entry already exists, *(value_pptr) will
 *    point to the value currently associated with the given key.
 *
 *    NOTE: An application should never store the sequence of bytes
 *    used to represent "no value" here.
 *
 *    Parameters:
 *
 *    table_ptr:      A pointer to the table to modify.
 *    key_ptr:        A pointer to a sequence of bytes (table_ptr->key_width
 *                    bytes in length) corresponding to the key.
 *    value_pptr:     A reference to a pointer that points to existing value
 *                    memory associated with a key, or a newly created entry.
 *
 *    Returns:
 *
 *    OK in the case when a new entry has been added successfully,
 *
 *    OK_DUPLICATE if an entry with the given key already exists. May
 *    return
 *
 *    ERR_OUTOFMEMORY in the case of a memory allocation failure.
 *
 *    ERR_SORTTABLE if hashlib_sort_entries() has been called on the table.
 */
int
hashlib_insert(
    HashTable          *table_ptr,
    const uint8_t      *key_ptr,
    uint8_t           **value_pptr);


/**
 *    Looks up an entry with the given key in the hash table. If an
 *    entry with the given key does not exist, value_pptr is
 *    untouched. Otherwise, a reference to the value memory is
 *    returned in *value_pptr as above.
 *
 *    Parameters:
 *
 *    table_ptr:      A pointer to the table to modify.
 *    key_ptr:        A pointer to a sequence of bytes (table_ptr->key_width
 *                    bytes in length) corresponding to the key.
 *    value_pptr:     A reference to a pointer that points to existing value
 *                    memory associated with a key (if in the table).
 *
 *    Returns:
 *
 *    OK if the entry exists.
 *
 *    ERR_NOTFOUND if the entry does not exist in the table.
 *
 *    ERR_SORTTABLE if hashlib_sort_entries() has been called on the table.
 */
int
hashlib_lookup(
    const HashTable    *table_ptr,
    const uint8_t      *key_ptr,
    uint8_t           **value_pptr);


/**
 *    Creates an iterator. Calling this function is the first step in
 *    iterating over the contents of a table. See hashlib_iterate.
 *
 *    table_ptr:      A pointer to the table to iterate over.
 *
 *    Returns:
 *
 *    An iterator to use in subsequent calls to hashlib_iterate().
 */
HASH_ITER
hashlib_create_iterator(
    const HashTable    *table_ptr);


/**
 *    Retrieves next available entry during iteration. After calling
 *    hashlib_create_iterator, this function should be called
 *    repeatedly until ERR_NOMOREENTRIES is returned. References to
 *    the key and value associated with the entry are returned as
 *    *(key_pptr) and *(val_pptr).
 *
 *    Parameters:
 *
 *    table_ptr:      A pointer to the current table being iterated over.
 *    iter_ptr:       A pointer to the iterator being used.
 *    key_pptr:       A reference to a pointer whose value will be set
 *                    to the key in the current entry (i.e., *key_pptr
 *                    will point to the key stored in the table).
 *    val_pptr:       A reference to a pointer whose value will be set
 *                    to the value in the current entry (i.e., *val_pptr
 *                    will point to the value in the table).
 *
 *    Returns:
 *
 *    OK until the end of the table is reached.
 *
 *    ERR_NOMOREENTRIES to indicate the iterator has visited all entries.
 *
 *    ERR_SORTTABLE if hashlib_sort_entries() has been called on this table.
 */
int
hashlib_iterate(
    const HashTable    *table_ptr,
    HASH_ITER          *iter_ptr,
    uint8_t           **key_pptr,
    uint8_t           **val_pptr);


/**
 *    Frees the memory associated with a table. Does _not_ free the
 *    application data memory referenced by app_data_ptr.  Does
 *    nothing if 'table_ptr' is NULL.
 *
 *    Parameters:
 *
 *    table_ptr:      The table to free.
 */
void
hashlib_free_table(
    HashTable          *table_ptr);


/**
 *    Force a rehash of a table. Generally, this will be used when a
 *    series of insertions has been completed before a very large
 *    number of lookups (relative to the number of inserts). That is,
 *    when it is likely that the rehash cost is less than the cost
 *    associated with performing searches over multiple blocks.  This
 *    is an advanced function. As a rule, it never make sense to call
 *    this function when the table holds a single block.
 *
 *    Parameters:
 *
 *    table_ptr:      The table to rehash.
 *
 *    Returns:
 *
 *    Returns either OK or ERR_OUTOFMEMORY in the case of a memory
 *    allocation failure.
 *
 *    ERR_SORTTABLE if hashlib_sort_entries() has been called on the table.
 */
int
hashlib_rehash(
    HashTable          *table_ptr);


/**
 *    Parameters:
 *
 *    table_ptr:      A pointer to a table.
 *
 *    Returns:
 *
 *    Returns the total number of buckets that have been allocated.
 */
uint32_t
hashlib_count_buckets(
    const HashTable    *table_ptr);


/**
 *    Parameters:
 *
 *    table_ptr:      A pointer to a table.
 *
 *    Returns:
 *
 *    Returns the total number of entries in the table.  This function
 *    sums the current entry count for each of the blocks that make up
 *    the table.  The return value should be equal to
 *    hashlib_count_nonempties().
 */
uint32_t
hashlib_count_entries(
    const HashTable    *table_ptr);


/**
 *    Parameters:
 *
 *    table_ptr:      A pointer to a table.
 *
 *    Returns:
 *
 *    Returns the total number of entries in the table.  This function
 *    does a complete scan of the table, looking at each bucket to see
 *    if it is empty.
 */
uint32_t
hashlib_count_nonempties(
    const HashTable    *table_ptr);


/*
 * NOT IMPLEMENTED
 */
uint32_t
hashlib_count_nondeleted(
    const HashTable    *table_ptr);



void
hashlib_dump_table_header(
    FILE               *fp,
    const HashTable    *table_ptr);
void
hashlib_dump_table(
    FILE               *fp,
    const HashTable    *table_ptr);
void
hashlib_dump_bytes(
    FILE               *fp,
    const uint8_t      *data_ptr,
    uint32_t            data_size);

/* Serialization */
int
hashlib_save_table(
    const HashTable    *table_ptr,
    const char         *filename_str,
    const uint8_t      *header_ptr,
    uint8_t             header_length);
int
hashlib_restore_table(
    HashTable         **table_pptr,
    const char         *filename_str,
    uint8_t            *header_ptr,
    uint8_t             header_length);

int
hashlib_serialize_table(
    const HashTable    *table_ptr,
    FILE               *output_fp,
    const uint8_t      *header_ptr,
    uint8_t             header_length);

int
hashlib_deserialize_table(
    HashTable         **table_pptr,
    FILE               *input_fp,
    uint8_t            *header_ptr,
    int                 header_length);

/* #define HASHLIB_RECORD_STATS */

/* Statistics */
#ifdef HASHLIB_RECORD_STATS
extern void
hashlib_clear_stats(
    void);
extern void
hashlib_dump_stats(
    FILE               *fp);
extern uint64_t hashlib_stats_rehashes;
extern uint64_t hashlib_stats_rehash_inserts;
extern uint64_t hashlib_stats_inserts;
extern uint64_t hashlib_stats_lookups;
extern uint32_t hashlib_stats_blocks_allocated;
#endif /* HASHLIB_RECORD_STATS */

#ifdef __cplusplus
}
#endif
#endif /* _HASHLIB_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
