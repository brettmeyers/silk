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
#ifndef _STREAM_CACHE_H
#define _STREAM_CACHE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_STREAM_CACHE_H, "$SiLK: stream-cache.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skstream.h>

/*
**  stream-cache.h
**
**    A simple interface for maintaining a list of open file handles
**    so we can avoid a lot of open/close cycles.  File handles are
**    indexed by the timestamp of the file, the sensor_id, and the
**    flowtype (class/type) of the data they contain.
*/


#define STREAM_CACHE_MINIMUM_SIZE 2
/*
 *    Smallest maximum cache size.  Code that handles removing items
 *    from end of list assumes at least two entries in the list.
 */

#define STREAM_CACHE_INACTIVE_TIMEOUT  (5 * 60 * 1000)
/*
 *    When skStreamFlush() is called, streams that have not been
 *    written to in the last STREAM_CACHE_INACTIVE_TIMEOUT
 *    milliseconds will be closed.
 */


/* The stream cache object. */
struct stream_cache_st;
typedef struct stream_cache_st stream_cache_t;


/*
 *  The cache_key_t is used as the key to the stream.
 */
typedef struct cache_key_st {
    /* the hour that this file is for */
    sktime_t        time_stamp;
    /* the sensor that this file is for */
    sensorID_t      sensor_id;
    /* the flowtype (class/type) that this file is for */
    flowtypeID_t    flowtype_id;
} cache_key_t;


/*
 *  The cache_entry_t contains information about the file, the file
 *  handle, and the number of records in the file.
 *
 *  Users of the stream-cache should view the cache_entry_t as opaque.
 *  Use the macros and functions to access it.
 */
typedef struct cache_entry_st {
    /* the number of records in the file as of opening or the most
     * recent flush, used for log messages */
    uint64_t        rec_count;
    /* when this entry was last accessed */
    sktime_t        last_accessed;
    /* the key */
    cache_key_t     key;
    /* the open file handle */
    skstream_t     *stream;
} cache_entry_t;


/*
 *  rwio = cache_open_fn_t(key, caller_data);
 *
 *    This function is used by skCacheLookupOrOpenAdd() when the
 *    stream associated with 'key' is not in the cache.  This function
 *    should open an existing file or create a new file as
 *    appriopriate.  The 'caller_data' is for the caller to use as she
 *    sees fit.  The stream does nothing with this value.
 *
 *    This function should return NULL if there was an error opening
 *    the file.
 */
typedef skstream_t *(*cache_open_fn_t)(
    const cache_key_t  *key,
    void               *caller_data);


/*
 *  status = skCacheAdd(cache, stream, key, &entry);
 *
 *    Add 'stream' to the stream cache 'cache' keyed by 'key' and put
 *    the cache-entry associated with the stream into the locatation
 *    pointed at by 'entry'.  The entry is returned in a locked state.
 *    The caller should call skCacheEntryRelease() to unlock the entry
 *    once processing is complete.
 *
 *    After this call, the cache will own the stream and will free it
 *    when the cache is full or when skCacheCloseAll() or
 *    skCacheDestroy() is called.
 *
 *    Return 0 on success, or -1 if there was a problem initializing
 *    the entry.  When the cache is full, adding a stream to the cache
 *    will cause a current stream to close.  If closing the stream
 *    fails, the new stream is still added to the cache, but 1 is
 *    returned to indicate the error.
 */
int
skCacheAdd(
    stream_cache_t     *cache,
    skstream_t         *stream,
    const cache_key_t  *key,
    cache_entry_t     **entry);


/*
 *  status = skCacheCloseAll(cache);
 *
 *    Close all the streams in the cache and remove them from the
 *    cache.  For each file, log the number of records processed since
 *    the most recent flush or open.  Returns zero if all streams were
 *    successfully flushed and closed.  Returns -1 if calling the
 *    skStreamClose() function for any stream returns non-zero, though
 *    all streams will still be closed and destroyed.
 */
int
skCacheCloseAll(
    stream_cache_t     *cache);


/*
 *  cache = skCacheCreate(max_size, open_callback);
 *
 *    Create a stream_cache capable of keeping 'max_size' files open.
 *    The 'open_callback' is the function that the stream_cache will
 *    use when skCacheLookupOrOpenAdd() is called.  If the caller does
 *    not use that function, the 'open_callback' may be NULL.
 *
 *    Returns NULL if memory cannot be allocated.
 */
stream_cache_t *
skCacheCreate(
    int                 max_size,
    cache_open_fn_t     open_fn);


/*
 *  status = skCacheDestroy(cache);
 *
 *    Close all streams and free all memory associated with the
 *    streams.  Free the memory associated with the cache.  The cache
 *    pointer is invalid after a call to this function.
 *
 *    As part of its processing, this function calls
 *    skCacheCloseAll(), and that function's return value is the
 *    return value of this function.
 */
int
skCacheDestroy(
    stream_cache_t     *cache);


/*
 *  stream = skCacheEntryGetStream(entry);
 *
 *    Returns the stream associated with a stream entry.
 */
#define skCacheEntryGetStream(entry) ((entry)->stream)


/*
 *  skCacheEntryRelease(entry);
 *
 *    Releases (unlocks) a stream entry.
 */
#define skCacheEntryRelease(entry)


/*
 *  status = skCacheFlush(cache);
 *
 *    Flush all the streams in the cache, and log the number of
 *    records processed since the most recent flush or open.  Returns
 *    zero if all streams were successfully flushed.  Returns -1 if
 *    calling the skStreamFlush() function for any stream returns
 *    non-zero, though all streams will still be flushed.
 */
int
skCacheFlush(
    stream_cache_t     *cache);


/*
 *  status = skCacheLockAndCloseAll(cache);
 *
 *    Identical to skCacheCloseAll(), except that it keeps a lock on
 *    the cache.  The caller should call skCacheUnlock() to unlock the
 *    cache.
 */
int
skCacheLockAndCloseAll(
    stream_cache_t     *cache);


/*
 *  entry = skCacheLookup(cache, key);
 *
 *    Return the stream entry associated with the specified 'key'.
 *    Return NULL if no stream entry for the specified 'key' is
 *    found.  The entry is returned in a locked state.  The caller
 *    should call skCacheEntryRelease() once the caller has finished
 *    with the entry.
 */
cache_entry_t *
skCacheLookup(
    stream_cache_t     *cache,
    const cache_key_t  *key);


/*
 *  status = skCacheLookupOrOpenAdd(cache, key, caller_data, &entry);
 *
 *    If a stream entry associated with 'key' already exists in the
 *    cache, set 'entry' to that location and return 0.
 *
 *    Otherwise, the cache calls the 'open_callback' that was
 *    registered when the cache was created.  The arguments to that
 *    function will be the 'key' and specified 'caller_data'.  If the
 *    open_callback returns NULL, this function returns -1.
 *    Otherwise, the stream is added to the cache as if skCacheAdd()
 *    had been called, and this function's return status will reflect
 *    the result of that call.
 *
 *    The entry is returned in a locked state.  The caller should call
 *    skCacheEntryRelease() once the caller has finished with the
 *    entry.
 */
int
skCacheLookupOrOpenAdd(
    stream_cache_t     *cache,
    const cache_key_t  *key,
    void               *caller_data,
    cache_entry_t     **entry);


/*
 *  skCacheUnlock(cache);
 *
 *    Unlocks a cache locked by skCacheLockAndCloseAll().
 */
void
skCacheUnlock(
    stream_cache_t     *cache);

#define skCacheUnlock(c)


#ifdef __cplusplus
}
#endif
#endif /* _STREAM_CACHE_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
