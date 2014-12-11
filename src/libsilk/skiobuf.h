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
#ifndef _SKIOBUF_H
#define _SKIOBUF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKIOBUF_H, "$SiLK: skiobuf.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

/*
**  skiobuf.h
**
**  Routines for buffered file io.
**
*/

#if SK_ENABLE_ZLIB
#include <zlib.h>
#endif


#define SKIOBUF_DEFAULT_BLOCKSIZE SKSTREAM_DEFAULT_BLOCKSIZE
/*
 * The default uncompressed block size.
 */

#define SKIOBUF_MAX_BLOCKSIZE 0x100000 /* One megabyte */
/*
 * The maximum compressed or uncompressed block size.
 */


#define SKIOBUF_DEFAULT_RECORDSIZE 1
/*
 * The default record size.  A single record is guaranteed not to span
 * multiple blocks.
 */


struct sk_iobuf_st;
typedef struct sk_iobuf_st sk_iobuf_t;
/*
 * The type of IO buffer objects
 */


/*
 *  skIOBuf will wrap an abstract file descriptor 'fd' if it
 *  implements the following functions:
 */

typedef ssize_t (*skio_read_fn_t)(void *fd, void *dest, size_t count);
/*
 *    Implements a readn(2)-like call: skIOBuf is requesting that the
 *    'fd' add 'count' bytes of data into 'dest' for input, returning
 *    the number of bytes actually added.  Should return a number less
 *    than count for a short read, -1 for an error.
 */

typedef ssize_t (*skio_write_fn_t)(void *fd, const void *src, size_t count);
/*
 *    Implements a writen(2)-like call: skIOBuf is requesting that the
 *    'fd' accept 'count' bytes of data from 'src' for output,
 *    returning the number of bytes actually accepted.  Should return
 *    a number less than count for a short write (is this even
 *    meaningful?), -1 for an error.
 */

typedef off_t (*skio_seek_fn_t)(void *fd, off_t offset, int whence);
/*
 *    Implements an lseek(2)-like call: skIOBuf is requesting that the
 *    read pointer be positioned relative to 'whence' by 'offset'
 *    bytes, returning the new offset of the read pointer.  'whence'
 *    can be SEEK_SET, SEEK_CUR, or SEEK_END.  Returns (off_t)(-1) on
 *    error.  If seek cannot work on a particular fd because it is not
 *    a seekable file, set errno to ESPIPE.
 */

typedef int (*skio_flush_fn_t)(void *fd);
/*
 *    Implements an fflush(3)-like call: skIOBuf is requesting that
 *    the 'fd' syncronize its output buffers with the physical media.
 *    Returns 0 on success, or -1 on error.  This function may be
 *    NULL.
 */

typedef const char *(*skio_strerror_fn_t)(void *fd, int fd_errno);
/*
 *    Implements a strerror(3)-like call: skIOBuf is requesting a
 *    human-readable error message given the error code 'fd_errno'.
 */

typedef void (*skio_free_fn_t)(void *fd);
/*
 *    skIOBuf is requesting that the 'fd' deallocate itself.
 */


typedef struct skio_abstract_st {
    skio_read_fn_t     read;
    skio_write_fn_t    write;
    skio_seek_fn_t     seek;
    skio_flush_fn_t    flush;
    skio_free_fn_t     free_fd;
    skio_strerror_fn_t strerror;
} skio_abstract_t;
/*
 *   All necessary operations on an abstract file descriptor.
 */


sk_iobuf_t *
skIOBufCreateReader(
    void);
/*
 *    Creates a new IO buffer for reading.
 *
 *    Returns the new IO buffer or NULL on allocation error.
 */

sk_iobuf_t *
skIOBufCreateWriter(
    void);
/*
 *    Creates a new IO buffer for writing.
 *
 *    Returns the new IO buffer or NULL on allocation error.
 */


void
skIOBufDestroy(
    sk_iobuf_t         *buf);
/*
 *    Destroys the IO buffer.  If the IO buffer is a writer, the
 *    buffer will be flushed before destruction.
 */


int
skIOBufBind(
    sk_iobuf_t         *buf,
    int                 fd,
    int                 compmethod);
/*
 *    Binds the file descriptor 'fd' to the IO buffer.  If the IO
 *    buffer is a writer and is already associated with a file
 *    descriptor, the buffer will be flushed before binding.  Binding
 *    a file descriptor resets the write/read count of the IO buffer.
 *    'compmethod' is a valid compression method from silk_files.h.
 *
 *    Returns 0 on success, -1 on failure.
 */

int
skIOBufBindAbstract(
    sk_iobuf_t         *buf,
    void               *fd,
    int                 compmethod,
    skio_abstract_t    *fd_ops);
/*
 *    Binds the abstract file descriptor 'fd' to the IO buffer.  If
 *    the IO buffer is a writer and is already associated with a file
 *    descriptor, the buffer will be flushed before binding.  Binding
 *    a file descriptor resets the write/read count of the IO buffer.
 *    'compmethod' is a valid compression method from silk_files.h.
 *    'fd_ops' is a pointer to a struct of file operations on the
 *    abstract file descriptor 'fd'.
 */

ssize_t
skIOBufRead(
    sk_iobuf_t         *buf,
    void               *data,
    size_t              count);
/*
 *    Reads 'count' uncompressed bytes into 'data' from the IO buffer
 *    'buf'.  'data' can be NULL.
 *
 *    Returns the number of uncompressed bytes read on success, -1 on
 *    failure.  Will return 0 on a short read (less than the requested
 *    number of bytes) if it has reached the end of a stream
 *    (compressed section or file).  This function will only return a
 *    short read if the end of the stream has been reached.
 */

ssize_t
skIOBufReadToChar(
    sk_iobuf_t         *buf,
    void               *data,
    size_t              count,
    int                 c);
/*
 *    Reads uncompressed bytes into 'data' from the IO buffer 'buf'
 *    until the character 'c' is reached (inclusive), or 'count'
 *    number of bytes have been written.  'data' can be NULL.
 *
 *    Returns the number of uncompressed bytes read on success, -1 on
 *    failure.  Will return 0 on a short read (less than the requested
 *    number of bytes) if it has reached the end of a stream
 *    (compressed section or file).
 */

ssize_t
skIOBufWrite(
    sk_iobuf_t         *buf,
    const void         *data,
    size_t              count);
/*
 *    Writes 'count' uncompressed bytes from 'data' into the IO buffer
 *    'buf'.
 *
 *    Returns the number of uncompressed bytes written on success, -1
 *    on failure.  This function will never return a number of bytes
 *    less than 'count'.
 */

off_t
skIOBufFlush(
    sk_iobuf_t         *buf);
/*
 *    Flushes an IO buffer writer.  This does not close the buffer or
 *    the underlying file descriptor.
 *
 *    Returns the number of compressed bytes written to the underlying
 *    file descriptor since it was bound to the IO buffer on success.
 *    Returns -1 on failure.
 */

off_t
skIOBufTotal(
    sk_iobuf_t         *buf);
/*
 *    Returns the compressed number of bytes that have been actually
 *    been read/written from/to the underlying file descriptor.  -1 on
 *    error.
 */

off_t
skIOBufTotalUpperBound(
    sk_iobuf_t         *buf);
/*
 *    Returns an upper bound on the number of compressed bytes that
 *    would be written to the underlying file descriptor since the
 *    binding of the buffer if the buffer were flushed.  -1 on error.
 */

int
skIOBufSetBlockSize(
    sk_iobuf_t         *buf,
    uint32_t            size);
/*
 *     Sets the block size for the IO buffer.  This function can only
 *     be called immediately after creation or binding of the IO
 *     buffer.  Returns 0 on success, -1 on error.
 */

uint32_t
skIOBufUpperCompBlockSize(
    sk_iobuf_t         *fd);
/*
 *     Returns the maximum possible compressed block size.
 */

int
skIOBufSetRecordSize(
    sk_iobuf_t         *buf,
    uint32_t            size);
/*
 *     Sets the record size for the IO buffer.  This function can only
 *     be called immediately after creation or binding of the IO
 *     buffer.  Returns 0 on success, -1 on error.
 */

#if SK_ENABLE_ZLIB
int
skIOBufSetZlibLevel(
    sk_iobuf_t         *buf,
    int                 level);
/*
 *    Sets the compression level for zlib-based IO buffers.  Returns 0
 *    on success, -1 on error.
 */

int
skIOBufBindGzip(
    sk_iobuf_t         *buf,
    gzFile              fd,
    int                 compmethod);
/*
 *    Binds the gzip file descriptor 'fd' to the IO buffer.  If the IO
 *    buffer is a writer and is already associated with a file
 *    descriptor, the buffer will be flushed before binding.  Binding
 *    a file descriptor resets the write/read count of the IO buffer.
 *    'compmethod' is a valid compression method from silk_files.h.
 *
 *    Returns 0 on success, -1 on failure.
 */
#endif  /* SK_ENABLE_ZLIB */

const char *
skIOBufStrError(
    sk_iobuf_t         *buf);
/*
 *    Returns a string representing the error state of the IO buffer
 *    'buf'.  This is a static string similar to that used by
 *    strerror.  This function also resets the error state of the IO
 *    buffer.
 */

#ifdef __cplusplus
}
#endif
#endif /* _SKIOBUF_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
