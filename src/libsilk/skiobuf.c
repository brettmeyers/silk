/*
** Copyright (C) 2006-2015 by Carnegie Mellon University.
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
**  Routines for buffered file io.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skiobuf.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/utils.h>
#include "skiobuf.h"
#include "skstream_priv.h"  /* for declaration of skIOBufBindGzip() */

SK_DIAGNOSTIC_IGNORE_PUSH("-Wundef")

#if SK_ENABLE_LZO
#include SK_LZO_HEADER_NAME
#endif
#ifdef SK_HAVE_LZO1X_DECOMPRESS_ASM_FAST_SAFE
#include SK_LZO_ASM_HEADER_NAME
#endif

SK_DIAGNOSTIC_IGNORE_POP("-Wundef")


/*
 *  skiobuf Input/Output Format
 *
 *    For compressed streams, blocks are written to the storage
 *    medium.  Each block is written to the storage medium as an
 *    8-byte header, followed by the compressed block.  The format is
 *    as follows:
 *
 *    byte 0-3: 4-byte compressed size (network byte order)
 *    byte 4-7: 4-byte uncompressed size (network byte order)
 *    byte 8- : The compressed data (compressed size number of bytes)
 *
 *    The compressed size in bytes 0-3 is the size of the data portion
 *    only; it does not include the 8 byte header.
 *
 *    When reading, a compressed size of 0 is considered identical to
 *    an end-of-file.  This allows one to embed an skiobuf compressed
 *    stream within another stream of data.
 *
 *    For uncompressed streams, skiobuf merely acts as a buffered
 *    reader/writer.  Although bytes are read and written from or to
 *    the storage medium as blocks, no headers are read or written.
 */

/* Options for compression types. */
typedef union iobuf_opts_un {
#if SK_ENABLE_ZLIB
    /* zlib */
    struct {
        int level;
    } zlib;
#endif  /* SK_ENABLE_ZLIB */

#if SK_ENABLE_LZO
    /* lzo */
    struct {
        uint8_t *scratch;
    } lzo;
#endif  /* SK_ENABLE_LZO */

    char nothing;        /* Just to keep the union from being empty */
} iobuf_opts_t;


typedef struct compr_sizes_st {
    uint32_t compr_size;
    uint32_t uncompr_size;
} compr_sizes_t;


/* An IO buffer. */
struct sk_iobuf_st {
    uint8_t         compr_method;       /* Compression method */
    iobuf_opts_t    compr_opts;         /* Compression options */

    uint8_t        *compr_buf;          /* Compression buffer */
    uint8_t        *uncompr_buf;        /* Decompression buffer */

    uint32_t        compr_buf_size;     /* Size of compr buffer */
    uint32_t        uncompr_buf_size;   /* Size of uncompr buffer */

    uint32_t        block_size;         /* Block size */
    uint32_t        block_quantum;      /* quanta size block is divided into */

    off_t           block_pos;          /* Location of current block on disk */
    uint32_t        disk_block_size;    /* Size of current block on disk */
    uint32_t        pos;                /* Byte position in buffer */
    uint32_t        max_bytes;          /* Maximim bytes allowed in buf */

    void           *fd;                 /* File descriptor */
    skio_abstract_t io;                 /* IO information */

    off_t           total;              /* Total read or written */

    int             io_errno;           /* errno of error */
    uint32_t        error_line;         /* line number of error */

    unsigned        fd_valid : 1;       /* File descriptor valid? */
    unsigned        in_core  : 1;       /* Set if block is in memory */
    unsigned        uncompr  : 1;       /* Set if block is uncompressed */
    unsigned        no_seek  : 1;       /* Cannot use seek */
    unsigned        used     : 1;       /* Set after a read or write */
    unsigned        write    : 1;       /* Read or write */
    unsigned        eof      : 1;       /* End of file or flushed */
    unsigned        error    : 1;       /* Error state? */
    unsigned        interr   : 1;       /* Internal or external error? */
    unsigned        ioerr    : 1;       /* IO error */
};

/* Flag that gets passed to skio_uncompr(). */
typedef enum {
    SKIO_UNCOMP_NORMAL,         /* Normal read of next block */
    SKIO_UNCOMP_SKIP,           /* Only read sizes of next block */
    SKIO_UNCOMP_REREAD          /* Actually read a skipped block */
} skio_uncomp_t;


/* Method table for different types of compression */
typedef struct iobuf_methods_st {
    /* Initialization.  Should set the default opts. */
    int      (*init_method)       (iobuf_opts_t *opts);
    /* Deinitialization.  Should free any default opts. */
    int      (*uninit_method)     (iobuf_opts_t *opts);

    /* Should return the maximum compressed size given the block
     * size. */
    uint32_t (*compr_size_method) (uint32_t compr_size,
                                   const iobuf_opts_t *opts);

    /* The compression method.  Caller must set '*destlen' to the
     * length of the destination buffer before calling.  */
    int      (*compr_method)      (void       *dest,
                                   uint32_t   *destlen,
                                   const void *source,
                                   uint32_t    sourcelen,
                                   const iobuf_opts_t *opts);

    /* The decompression method.  Caller must set '*destlen' to the
     * length of the destination buffer before calling.  */
    int      (*uncompr_method)    (void       *dest,
                                   uint32_t   *destlen,
                                   const void *source,
                                   uint32_t    sourcelen,
                                   const iobuf_opts_t *opts);

    /* Whether this compression method expects the block sizes---that
     * is, a compr_sizes_t structure---before the compressed
     * blocks. */
    unsigned   block_numbers : 1;
} iobuf_methods_t;


enum internal_errors {
    ESKIO_BADOPT = 0,
    ESKIO_BADCOMPMETHOD,
    ESKIO_BLOCKSIZE,
    ESKIO_COMP,
    ESKIO_INITFAIL,
    ESKIO_MALLOC,
    ESKIO_NOFD,
    ESKIO_NOREAD,
    ESKIO_NOWRITE,
    ESKIO_SHORTREAD,
    ESKIO_SHORTWRITE,
    ESKIO_TOOBIG,
    ESKIO_UNCOMP,
    ESKIO_USED
};

static const char* internal_messages[] = {
    "Illegal compression or decompression option",    /* ESKIO_BADOPT */
    "Bad compression method",                         /* ESKIO_BADCOMPMETHOD */
    "Block size is too large",                        /* ESKIO_BLOCKSIZE */
    "Error during compression",                       /* ESKIO_COMP */
    "Compression initialization failed",              /* ESKIO_INITFAIL */
    "Out of memory",                                  /* ESKIO_MALLOC */
    "File descriptor is not set",                     /* ESKIO_NOFD */
    "Attempt to read from an IO buffer writer",       /* ESKIO_NOREAD */
    "Attempt to write to an IO buffer reader",        /* ESKIO_NOWRITE */
    "Could not read complete compressed block",       /* ESKIO_SHORTREAD */
    "Could not write complete compressed block",      /* ESKIO_SHORTWRITE */
    "Count is too large",                             /* ESKIO_TOOBIG */
    "Error during decompression",                     /* ESKIO_UNCOMP */
    "Parameter set on IO buffer after buffer has been used" /* ESKIO_USED */
};

#if SK_ENABLE_ZLIB
/* Forward declarations for zlib methods */
static int
zlib_init_method(
    iobuf_opts_t       *opts);
static uint32_t
zlib_compr_size_method(
    uint32_t            compr_size,
    const iobuf_opts_t *opts);
static int
zlib_compr_method(
    void               *dest,
    uint32_t           *destlen,
    const void         *source,
    uint32_t            sourcelen,
    const iobuf_opts_t *opts);
static int
zlib_uncompr_method(
    void               *dest,
    uint32_t           *destlen,
    const void         *source,
    uint32_t            sourcelen,
    const iobuf_opts_t *opts);
#endif  /* SK_ENABLE_ZLIB */


#if SK_ENABLE_LZO
/* Forward declarations for lzo methods */
static int
lzo_init_method(
    iobuf_opts_t       *opts);
static int
lzo_uninit_method(
    iobuf_opts_t       *opts);
static uint32_t
lzo_compr_size_method(
    uint32_t            compr_size,
    const iobuf_opts_t *opts);
static int
lzo_compr_method(
    void               *dest,
    uint32_t           *destlen,
    const void         *source,
    uint32_t            sourcelen,
    const iobuf_opts_t *opts);
static int
lzo_uncompr_method(
    void               *dest,
    uint32_t           *destlen,
    const void         *source,
    uint32_t            sourcelen,
    const iobuf_opts_t *opts);
#endif  /* SK_ENABLE_LZO */


#define SKIOBUF_METHOD_PLACEHOLDER  { NULL, NULL, NULL, NULL, NULL, 1 }

static const iobuf_methods_t methods[] = {
    /* NONE */
    { NULL, NULL, NULL, NULL, NULL, 0 },

#if  !SK_ENABLE_ZLIB
    SKIOBUF_METHOD_PLACEHOLDER,
#else
    /* ZLIB */
    { zlib_init_method, NULL,
      zlib_compr_size_method,
      zlib_compr_method, zlib_uncompr_method, 1},
#endif  /* SK_ENABLE_ZLIB */

#if  !SK_ENABLE_LZO
    SKIOBUF_METHOD_PLACEHOLDER
#else
    /* LZO1X */
    { lzo_init_method, lzo_uninit_method,
      lzo_compr_size_method,
      lzo_compr_method, lzo_uncompr_method, 1}
#endif  /* SK_ENABLE_LZO */

};

static int const num_methods = sizeof(methods) / sizeof(iobuf_methods_t);

#define SKIOBUF_INTERNAL_ERROR(fd, err)         \
    {                                           \
        if (!(fd)->error) {                     \
            (fd)->io_errno = (int)(err);        \
            (fd)->error = 1;                    \
            (fd)->interr = 1;                   \
            (fd)->error_line = __LINE__;        \
        }                                       \
        return -1;                              \
    }

#define SKIOBUF_EXTERNAL_ERROR(fd)              \
    {                                           \
        if (!(fd)->error) {                     \
            (fd)->io_errno = errno;             \
            (fd)->error = 1;                    \
            (fd)->error_line = __LINE__;        \
        }                                       \
        return -1;                              \
    }

#define SKIOBUF_IO_ERROR(fd)                    \
    {                                           \
        if (!(fd)->error) {                     \
            (fd)->io_errno = errno;             \
            (fd)->error = 1;                    \
            (fd)->ioerr = 1;                    \
            (fd)->fd_valid = 0;                 \
            (fd)->error_line = __LINE__;        \
        }                                       \
        return -1;                              \
    }


/* FUNCTION DEFINITIONS */

/* Create an IO buffer reader */
sk_iobuf_t *
skIOBufCreateReader(
    void)
{
    sk_iobuf_t *fd = NULL;

    fd = (sk_iobuf_t*)calloc(1, sizeof(sk_iobuf_t));
    if (fd == NULL) {
        return NULL;
    }

    fd->block_size = fd->uncompr_buf_size = SKIOBUF_DEFAULT_BLOCKSIZE;
    fd->block_quantum = SKIOBUF_DEFAULT_RECORDSIZE;
    fd->compr_method = SK_COMPMETHOD_NONE;

    return fd;
}

/* Create an IO buffer writer */
sk_iobuf_t *
skIOBufCreateWriter(
    void)
{
    sk_iobuf_t *fd = NULL;

    /* First, get a reader. */
    fd = skIOBufCreateReader();
    if (fd == NULL) {
        return NULL;
    }

    /* Next, turn it into a writer. */
    fd->write = 1;
    fd->pos = 0;

    return fd;
}


/* Destroy an IO buffer */
void
skIOBufDestroy(
    sk_iobuf_t         *fd)
{
    const iobuf_methods_t *method;

    assert(fd);

    /* Will set error on reader, but that's okay, since we free the
       structure immediately afterwards. */
    skIOBufFlush(fd);

    if (fd->compr_buf) {
        free(fd->compr_buf);
    }
    if (fd->uncompr_buf) {
        free(fd->uncompr_buf);
    }

    method = &methods[fd->compr_method];
    if (method->uninit_method) {
        method->uninit_method(&fd->compr_opts);
    }

    if (fd->io.free_fd) {
        fd->io.free_fd(fd->fd);
    }

    free(fd);
}


/* Set the buffer sizes based on the block_size and block_quantum */
static void
calculate_buffer_sizes(
    sk_iobuf_t         *fd)
{
    const iobuf_methods_t *method;

    method = &methods[fd->compr_method];

    fd->uncompr_buf_size = fd->block_size;
    fd->max_bytes = fd->block_size - (fd->block_size % fd->block_quantum);
    if (method->compr_size_method) {
        fd->compr_buf_size =
            method->compr_size_method(fd->block_size, &fd->compr_opts);
    } else {
        fd->compr_buf_size = fd->block_size;
    }

    if (fd->compr_buf) {
        free(fd->compr_buf);
        fd->compr_buf = NULL;
    }
    if (fd->uncompr_buf) {
        free(fd->uncompr_buf);
        fd->uncompr_buf = NULL;
    }
}


/* Prepare for binding a file descriptor */
static int
skio_bind_wrapper(
    sk_iobuf_t         *fd,
    int                 compmethod)
{
    off_t total;
    const iobuf_methods_t *method;

    /* if we are bound to something, uninitialize it */
    method = &methods[fd->compr_method];
    if (method->uninit_method) {
        if (method->uninit_method(&fd->compr_opts)) {
            return -1;
        }
    }

    method = &methods[compmethod];

    assert(fd);
    if (compmethod < 0 || compmethod >= num_methods) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BADCOMPMETHOD);
    }
#if !SK_ENABLE_ZLIB
    if (compmethod == SK_COMPMETHOD_ZLIB) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BADCOMPMETHOD);
    }
#endif
#if !SK_ENABLE_LZO
    if (compmethod == SK_COMPMETHOD_LZO1X) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BADCOMPMETHOD);
    }
#endif
    if (fd == NULL) {
        return -1;
    }

    if (fd->fd_valid) {
        total = skIOBufFlush(fd);
        if (total == -1) {
            SKIOBUF_INTERNAL_ERROR(fd, ESKIO_INITFAIL);
        }
    }

    fd->compr_method = compmethod;
    fd->total = 0;
    fd->used = 0;
    fd->error = 0;
    fd->interr = 0;
    fd->ioerr = 0;
    fd->io_errno = 0;
    fd->eof = 0;
    fd->uncompr = 0;
    fd->in_core = 1;            /* Set so first read doesn't try to
                                   complete a skip */

    if (method->init_method) {
        if (method->init_method(&fd->compr_opts)) {
            return -1;
        }
    }

    calculate_buffer_sizes(fd);
    if (fd->uncompr_buf_size > SKIOBUF_MAX_BLOCKSIZE) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BLOCKSIZE);
    }

    if (!fd->write) {
        fd->pos = fd->max_bytes;
    }

    return 0;
}


int
skIOBufBindAbstract(
    sk_iobuf_t         *fd,
    void               *file,
    int                 compmethod,
    skio_abstract_t    *fd_ops)
{
    int rv = skio_bind_wrapper(fd, compmethod);
    fd->io = *fd_ops;
    fd->fd = file;
    if (fd->io.seek == NULL) {
        fd->no_seek = 1;
    } else {
        fd->no_seek = 0;
    }
    fd->fd_valid = 1;
    return rv;
}


/* Handle actual read and decompression of a block */
static int32_t
skio_uncompr(
    sk_iobuf_t         *fd,
    skio_uncomp_t       mode)
{
    uint32_t comp_block_size, uncomp_block_size, new_block_size;
    /* to support buffer overruns when decompressing */
    uint32_t padded_uncomp_block_size;
    ssize_t readlen;
    uint8_t *bufpos;
    const iobuf_methods_t *method;

    assert(fd);

    /* assert that the mode is being used correctly */
    assert((mode == SKIO_UNCOMP_NORMAL) ||
           (mode == SKIO_UNCOMP_SKIP) ||
           (mode == SKIO_UNCOMP_REREAD && !fd->uncompr));

    /* Alias our methods. */
    method = &methods[fd->compr_method];

    /* When reading a new block, reset the block */
    if (mode != SKIO_UNCOMP_REREAD) {
        fd->in_core = 0;
        fd->uncompr = 0;
    }

    /* Determine our block sizes */
    if (mode == SKIO_UNCOMP_REREAD) {
        /* The sizes have already been read.  Get them from the fd */
        comp_block_size = fd->disk_block_size;
        uncomp_block_size = fd->max_bytes;
        new_block_size = fd->max_bytes;
        padded_uncomp_block_size = fd->max_bytes;
    } else if (!method->block_numbers) {
        /* Without block numbers, assume fd->max_bytes for
         * everything. */
        comp_block_size = fd->max_bytes;
        uncomp_block_size = fd->max_bytes;
        new_block_size = fd->max_bytes;
        padded_uncomp_block_size = fd->max_bytes;
    } else {
        /* Read in the compressed block sizes */
        readlen = fd->io.read(fd->fd, &comp_block_size,
                              sizeof(comp_block_size));
        if (readlen == -1) {
            SKIOBUF_IO_ERROR(fd);
        }
        if (readlen == 0) {
            /* We've reached eof. */
            fd->eof = 1;
            return 0;
        }
        fd->total += readlen;
        if ((size_t)readlen < sizeof(comp_block_size)) {
            SKIOBUF_INTERNAL_ERROR(fd, ESKIO_SHORTREAD);
        }

        /* If we have reached the end of the compressed stream, we
         * have the bytes we have. */
        if (comp_block_size == 0) {
            fd->eof = 1;
            return 0;
        }

        /* Read in the uncompressed block sizes */
        readlen = fd->io.read(fd->fd, &uncomp_block_size,
                              sizeof(uncomp_block_size));
        if (readlen == -1) {
            SKIOBUF_IO_ERROR(fd);
        }
        fd->total += readlen;
        if ((size_t)readlen < sizeof(uncomp_block_size)) {
            /* We've reached eof, though we weren't expecting to */
            fd->eof = 1;
            SKIOBUF_INTERNAL_ERROR(fd, ESKIO_SHORTREAD);
        }

        comp_block_size = ntohl(comp_block_size);
        uncomp_block_size = new_block_size = ntohl(uncomp_block_size);

        /*
         *   Some decompression algorithms require more space than the
         *   size of the decompressed data since they write data in
         *   4-byte chunks.  In particular, lzo1x_decompress_asm_fast
         *   has this requirement.  Account for that padding here.
         */
        padded_uncomp_block_size = 3 + uncomp_block_size;
    }

    /* Make sure block sizes aren't too large */
    if (comp_block_size > SKIOBUF_MAX_BLOCKSIZE ||
        padded_uncomp_block_size > SKIOBUF_MAX_BLOCKSIZE)
    {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BLOCKSIZE);
    }

    /* Save the size of the disk block size */
    fd->disk_block_size = comp_block_size;

    /* Reallocate buffers if necessary */
    if (method->uncompr_method != NULL &&
        (comp_block_size > fd->compr_buf_size ||
         fd->compr_buf == NULL))
    {
        assert(mode != SKIO_UNCOMP_REREAD);

        if (fd->compr_buf) {
            free(fd->compr_buf);
        }
        fd->compr_buf = (uint8_t*)malloc(comp_block_size);
        if (fd->compr_buf == NULL) {
            SKIOBUF_INTERNAL_ERROR(fd, ESKIO_MALLOC);
        }
        fd->compr_buf_size = comp_block_size;
    }
    if (padded_uncomp_block_size > fd->uncompr_buf_size ||
        fd->uncompr_buf == NULL)
    {
        assert(mode != SKIO_UNCOMP_REREAD);

        if (fd->uncompr_buf) {
            free(fd->uncompr_buf);
        }
        fd->uncompr_buf = (uint8_t*)malloc(padded_uncomp_block_size);
        if (fd->uncompr_buf == NULL) {
            SKIOBUF_INTERNAL_ERROR(fd, ESKIO_MALLOC);
        }
        fd->uncompr_buf_size = padded_uncomp_block_size;
    }

    /* Skip over data if we can */
    if (mode == SKIO_UNCOMP_SKIP && !fd->no_seek) {
        off_t end;
        off_t pos;

        errno = 0;

        /* Save current read position */
        fd->block_pos = fd->io.seek(fd->fd, 0, SEEK_CUR);
        if (fd->block_pos == (off_t)(-1)) {
            if (errno == ESPIPE) {
                fd->no_seek = 1;
                goto exit_skip;
            }
            SKIOBUF_IO_ERROR(fd);
        }
        /* Get EOF position */
        end = fd->io.seek(fd->fd, 0, SEEK_END);
        if (end == (off_t)(-1)) {
            SKIOBUF_IO_ERROR(fd);
        }
        /* Move to next block location */
        pos = fd->io.seek(fd->fd, fd->block_pos + comp_block_size,
                                    SEEK_SET);
        if (pos == (off_t)(-1)) {
            SKIOBUF_IO_ERROR(fd);
        }
        /* If next block is past EOF, read the last block */
        if (end < pos) {
            mode = SKIO_UNCOMP_REREAD;
            fd->pos = 0;
        }
    }
  exit_skip:

    /* If rereading, set our file position correctly. */
    if (mode == SKIO_UNCOMP_REREAD && !fd->in_core) {
        off_t pos;

        pos = fd->io.seek(fd->fd, fd->block_pos, SEEK_SET);
        if (pos == (off_t)(-1)) {
            SKIOBUF_IO_ERROR(fd);
        }
    }

    /* Read data when we must */
    if (mode == SKIO_UNCOMP_NORMAL ||
        (mode == SKIO_UNCOMP_REREAD && !fd->in_core) ||
        (mode == SKIO_UNCOMP_SKIP && fd->no_seek))
    {
        bufpos = method->uncompr_method ? fd->compr_buf : fd->uncompr_buf;
        readlen = fd->io.read(fd->fd, bufpos, comp_block_size);
        fd->in_core = 1;

        if (readlen == -1) {
            SKIOBUF_IO_ERROR(fd);
        }
        fd->total += readlen;
        if ((size_t)readlen < comp_block_size) {
            if (method->block_numbers) {
                SKIOBUF_INTERNAL_ERROR(fd, ESKIO_SHORTREAD);
            }
            fd->eof = 1;
            new_block_size = readlen;
            comp_block_size = readlen;
        }
    }

    /* Decompress it if we are not skipping it */
    if (mode != SKIO_UNCOMP_SKIP) {
        if (!method->uncompr_method) {
            /* If no decompression method, already uncompressed */
            fd->uncompr = 1;
        } else {
            /* Decompress */
            assert(fd->in_core);

            new_block_size = fd->uncompr_buf_size;
            if (method->uncompr_method(fd->uncompr_buf, &new_block_size,
                                       fd->compr_buf, comp_block_size,
                                       &fd->compr_opts))
            {
                SKIOBUF_INTERNAL_ERROR(fd, ESKIO_UNCOMP);
            }

            /* Verify the block's uncompressed size */
            if (new_block_size != uncomp_block_size) {
                SKIOBUF_INTERNAL_ERROR(fd, ESKIO_UNCOMP);
            }

            fd->uncompr = 1;
        }
    }

    /* Register the new data in the struct */
    fd->max_bytes = new_block_size;
    if (mode != SKIO_UNCOMP_REREAD) {
        /* Don't reset pos in an reread block */
        fd->pos = 0;
    }

    return new_block_size;
}


/* Read data from an IO buffer.  If 'c' is non-null, stop when the
 * char '*c' is encountered. */
static ssize_t
iobufRead(
    sk_iobuf_t         *fd,
    void               *buf,
    size_t              count,
    const int          *c)
{
    ssize_t       total   = 0;
    int           found_c = 0;
    skio_uncomp_t mode;

    assert(fd);

    /* Take care of boundary conditions */
    if (fd == NULL) {
        return -1;
    }
    if (fd->error) {
        return -1;
    }
    if (fd->write) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_NOREAD);
    }
    if (!fd->fd_valid) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_NOFD);
    }
    if (count == 0) {
        return 0;
    }

    assert(count <= SSIZE_MAX);
    if (count > SSIZE_MAX) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_TOOBIG);
    }

    /* If we don't need the bytes, skip them */
    if (buf == NULL && c == NULL) {
        mode = SKIO_UNCOMP_SKIP;
    } else {
        mode = SKIO_UNCOMP_NORMAL;
    }

    /* Transfer the bytes */
    while (count && !found_c) {
        size_t   left = fd->max_bytes - fd->pos;
        size_t   num;
        void    *bufpos;

        /* If we have no bytes, we must get some. */
        if (left == 0) {
            int32_t uncompr_size;

            if (fd->eof) {
                break;
            }
            uncompr_size = skio_uncompr(fd, mode);
            if (uncompr_size == -1) {
                /* In an error condition, return those bytes we have
                 * successfully read.  A subsequent call to
                 * iobufRead() will return -1 because fd->error will
                 * be set. */
                return total ? total : -1;
            }
            fd->used = 1;
            left = fd->max_bytes;
            if (uncompr_size == 0) {
                assert(fd->eof);
                break;
            }

        } else if (!fd->uncompr && mode == SKIO_UNCOMP_NORMAL) {
            /* Read and/or uncompress real data, if needed. */
            int32_t rv = skio_uncompr(fd, SKIO_UNCOMP_REREAD);
            if (rv == -1) {
                return total ? total : -1;
            }
            if (rv == 0) {
                assert(fd->eof);
                break;
            }
        }

        /* Calculate how many bytes to read from our current buffer */
        num = (count < left) ? count : left;

        /* Copy the bytes, and update the data */
        bufpos = &fd->uncompr_buf[fd->pos];
        if (buf != NULL) {
            if (c != NULL) {
                void *after = memccpy(buf, bufpos, *c, num);
                if (after != NULL) {
                    found_c = 1;
                    num = ((uint8_t *)after) - ((uint8_t *)buf);
                }
            } else {
                memcpy(buf, bufpos, num);
            }
            buf = (uint8_t *)buf + num;
        } else if (c != NULL) {
            void *after = memchr(bufpos, *c, num);
            if (after != NULL) {
                found_c = 1;
                num = ((uint8_t *)after) - ((uint8_t *)bufpos) + 1;
            }
        }
        fd->pos += num;
        total += num;
        count -= num;
    }

    return total;
}

/* Standard read function for skiobuf */
ssize_t
skIOBufRead(
    sk_iobuf_t         *fd,
    void               *buf,
    size_t              count)
{
    return iobufRead(fd, buf, count, NULL);
}

/* Read until buffer full or character in 'c' encountered */
ssize_t
skIOBufReadToChar(
    sk_iobuf_t         *fd,
    void               *buf,
    size_t              count,
    int                 c)
{
    return iobufRead(fd, buf, count, &c);
}


/* Handle actual compression and write of a block */
static int32_t
skio_compr(
    sk_iobuf_t         *fd)
{
    uint32_t compr_size, uncompr_size;
    uint32_t size;
    ssize_t writelen;
    uint8_t *bufpos;
    const iobuf_methods_t *method;
    uint32_t extra;
    uint32_t offset;

    assert(fd);

    method = &methods[fd->compr_method];
    uncompr_size = fd->pos;

    extra = fd->pos % fd->block_quantum;
    /* Programmer's error if we don't have complete records */
    assert(extra == 0);
    /* If assertions aren't turned on, at least pad the record */
    if (extra != 0) {
        memset(&fd->uncompr_buf[fd->pos], 0, extra);
        uncompr_size += extra;
    }

    /* Extra bit added on for block sizes */
    offset = method->block_numbers ? sizeof(compr_sizes_t) : 0;

    /* Compress the block */
    if (method->compr_method) {

        /* Create the compression buffer, if necessary */
        if (fd->compr_buf == NULL) {
            fd->compr_buf = (uint8_t*)malloc(fd->compr_buf_size + offset);
            if (fd->compr_buf == NULL) {
                SKIOBUF_INTERNAL_ERROR(fd, ESKIO_MALLOC);
            }
        }

        compr_size = fd->compr_buf_size;
        if (method->compr_method(fd->compr_buf + offset, &compr_size,
                                 fd->uncompr_buf, uncompr_size,
                                 &fd->compr_opts) != 0)
        {
            SKIOBUF_INTERNAL_ERROR(fd, ESKIO_COMP);
        }
        bufpos = fd->compr_buf;
    } else {
        compr_size = fd->pos;
        bufpos = fd->uncompr_buf;
    }

    size = compr_size + offset;

    if (method->block_numbers) {
        compr_sizes_t *sizes = (compr_sizes_t *)fd->compr_buf;

        /* Write out the block numbers */
        sizes->compr_size = htonl(compr_size);
        sizes->uncompr_size = htonl(uncompr_size);
    }

    /* Write out compressed data */
    writelen = fd->io.write(fd->fd, bufpos, size);
    if (writelen == -1) {
        SKIOBUF_IO_ERROR(fd);
    }
    fd->total += writelen;
    if ((size_t)writelen < size) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_SHORTWRITE);
    }

    fd->pos = 0;

    return (int32_t)writelen;
}


/* Write data to an IO buffer */
ssize_t
skIOBufWrite(
    sk_iobuf_t         *fd,
    const void         *buf,
    size_t              count)
{
    ssize_t total = 0;

    assert(fd);

    /* Take care of boundary conditions */
    if (count == 0) {
        return 0;
    }
    if (fd == NULL) {
        return -1;
    }
    if (!fd->write) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_NOWRITE);
    }
    if (!fd->fd_valid) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_NOFD);
    }
    assert(count <= SSIZE_MAX);
    if (count > SSIZE_MAX) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_TOOBIG);
    }

    fd->used = 1;

    /* If the buffer hasn't been created yet, create it. */
    if (fd->uncompr_buf == NULL) {
        fd->uncompr_buf = (uint8_t*)malloc(fd->uncompr_buf_size);
        if (fd->uncompr_buf == NULL) {
            SKIOBUF_INTERNAL_ERROR(fd, ESKIO_MALLOC);
        }
    }

    /* Transfer the bytes */
    while (count) {
        size_t left = fd->max_bytes - fd->pos;
        size_t num;

        /* If we have filled the buffer, we must write it out. */
        if (left == 0) {
            int32_t compr_size = skio_compr(fd);

            if (compr_size == -1) {
                return -1;
            }

            left = fd->max_bytes;
        }

        /* Calculate how many bytes to write into our current buffer */
        num = (count < left) ? count : left;

        /* Copy the bytes, and update the data */
        memcpy(&fd->uncompr_buf[fd->pos], buf, num);
        fd->pos += num;
        total += num;
        count -= num;
        buf = (uint8_t *)buf + num;
    }

    return total;
}


/* Finish writing to an IO buffer */
off_t
skIOBufFlush(
    sk_iobuf_t         *fd)
{
    assert(fd);
    if (fd == NULL) {
        return -1;
    }
    if (!fd->write) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_NOWRITE);
    }
    if (!fd->fd_valid) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_NOFD);
    }

    if (fd->pos) {
        int32_t compr_size = skio_compr(fd);

        if (compr_size == -1) {
            return -1;
        }
    }

    if (fd->io.flush) {
        fd->io.flush(fd->fd);
    }

    return fd->total;
}


/* Total number of bytes read/written from/to file descriptor */
off_t
skIOBufTotal(
    sk_iobuf_t         *fd)
{
    assert(fd);
    if (fd == NULL) {
        return -1;
    }

    return fd->total;
}


/* Maximum total number of bytes in a compressed block */
uint32_t
skIOBufUpperCompBlockSize(
    sk_iobuf_t         *fd)
{
    const iobuf_methods_t *method;
    uint32_t total;

    assert(fd);
    assert(fd->write);

    method = &methods[fd->compr_method];

    if (method->compr_size_method) {
        total = method->compr_size_method(fd->max_bytes, &fd->compr_opts);
    } else {
        total = fd->max_bytes;
    }

    if (method->block_numbers) {
        total += sizeof(compr_sizes_t);
    }

    return total;
}


/* Maximum total number of bytes written to file descriptor after a flush */
off_t
skIOBufTotalUpperBound(
    sk_iobuf_t         *fd)
{
    const iobuf_methods_t *method;
    off_t total;

    assert(fd);
    if (fd == NULL) {
        return -1;
    }
    if (!fd->write) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_NOWRITE);
    }

    method = &methods[fd->compr_method];

    total = fd->total + fd->pos;
    if (method->block_numbers) {
        total += sizeof(compr_sizes_t);
    }
    if (method->compr_size_method) {
        total += (method->compr_size_method(fd->max_bytes, &fd->compr_opts)
                  - fd->max_bytes);
    }

    return total;
}


/* Sets the block size */
int
skIOBufSetBlockSize(
    sk_iobuf_t         *fd,
    uint32_t            size)
{
    assert(fd);
    if (fd == NULL) {
        return -1;
    }
    if (fd->used) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_USED);
    }

    if (size > SKIOBUF_MAX_BLOCKSIZE) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BLOCKSIZE);
    }

    fd->block_size = size;
    calculate_buffer_sizes(fd);
    if (fd->uncompr_buf_size > SKIOBUF_MAX_BLOCKSIZE) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BLOCKSIZE);
    }

    return 0;
}


/* Sets the write quantum */
int
skIOBufSetRecordSize(
    sk_iobuf_t         *fd,
    uint32_t            size)
{
    assert(fd);
    if (fd == NULL) {
        return -1;
    }
    if (fd->used) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_USED);
    }

    fd->block_quantum = size;
    calculate_buffer_sizes(fd);
    if (fd->uncompr_buf_size > SKIOBUF_MAX_BLOCKSIZE) {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BLOCKSIZE);
    }

    return 0;
}


/* Create an error message */
const char *
skIOBufStrError(
    sk_iobuf_t         *fd)
{
    static char buf[256];
    static const char *message;

    message = buf;
    if (!fd->error) {
        message = "No error";
    } else if (fd->interr) {
        snprintf(buf, sizeof(buf), "%s",
                 internal_messages[fd->io_errno]);
    } else if (fd->ioerr) {
        snprintf(buf, sizeof(buf), "%s",
                 fd->io.strerror(fd->fd, fd->io_errno));
    } else {
        snprintf(buf, sizeof(buf), "%s",
                 strerror(fd->io_errno));
    }

    fd->error = 0;
    fd->interr = 0;
    fd->ioerr = 0;
    fd->io_errno = 0;

    return message;
}


/*  SIMPLE READ/WRITE methods */

/* skio_abstract_t.read; Wrapper around simple read */
static ssize_t
raw_read(
    void               *vfd,
    void               *dest,
    size_t              count)
{
    int *fd = (int *)vfd;
    return skreadn(*fd, dest, count);
}


/* skio_abstract_t.write; Wrapper around simple write */
static ssize_t
raw_write(
    void               *vfd,
    const void         *src,
    size_t              count)
{
    int *fd = (int *)vfd;
    return skwriten(*fd, src, count);
}


/* skio_abstract_t.seek; Wrapper around simple lseek */
static off_t
raw_seek(
    void               *vfd,
    off_t               offset,
    int                 whence)
{
    int *fd = (int *)vfd;
    return lseek(*fd, offset, whence);
}


/* skio_abstract_t.strerror; Wrapper around simple strerror */
static const char *
raw_strerror(
    void        UNUSED(*vfd),
    int                 io_errno)
{
    return strerror(io_errno);
}


/* Bind an IO buffer */
int
skIOBufBind(
    sk_iobuf_t         *fd,
    int                 file,
    int                 compmethod)
{
    skio_abstract_t io;
    int *fh;

    fh = (int*)malloc(sizeof(*fh));
    if (fh == NULL) {
        return -1;
    }
    *fh = file;
    io.read = &raw_read;
    io.write = &raw_write;
    io.seek = &raw_seek;
    io.strerror = &raw_strerror;
    io.flush = NULL;
    io.free_fd = &free;
    return skIOBufBindAbstract(fd, fh, compmethod, &io);
}


#if SK_ENABLE_ZLIB

/* ZLIB methods */

/* iobuf_methods_t.init_method */
static int
zlib_init_method(
    iobuf_opts_t       *opts)
{
    opts->zlib.level = Z_DEFAULT_COMPRESSION;
    return 0;
}


/* iobuf_methods_t.compr_size_method */
static uint32_t
zlib_compr_size_method(
    uint32_t                    compr_size,
    const iobuf_opts_t  UNUSED(*opts))
{
#ifdef SK_HAVE_COMPRESSBOUND
    return compressBound(compr_size);
#else
    return compr_size + compr_size / 1000 + 12;
#endif
}


/* iobuf_methods_t.compr_method */
static int
zlib_compr_method(
    void               *dest,
    uint32_t           *destlen,
    const void         *source,
    uint32_t            sourcelen,
    const iobuf_opts_t *opts)
{
    uLongf dl;
    uLong  sl;
    int    rv;

    assert(sizeof(sl) >= sizeof(sourcelen));
    assert(sizeof(dl) >= sizeof(*destlen));

    sl = sourcelen;
    dl = *destlen;
    rv = compress2((Bytef*)dest, &dl, (const Bytef*)source, sl,
                   opts->zlib.level);
    *destlen = dl;

    rv = (rv == Z_OK) ? 0 : -1;

    return rv;
}


/* iobuf_methods_t.uncompr_method */
static int
zlib_uncompr_method(
    void                       *dest,
    uint32_t                   *destlen,
    const void                 *source,
    uint32_t                    sourcelen,
    const iobuf_opts_t  UNUSED(*opts))
{
    uLongf dl;
    uLong  sl;
    int    rv;

    assert(sizeof(sl) >= sizeof(sourcelen));
    assert(sizeof(dl) >= sizeof(*destlen));

    sl = sourcelen;
    dl = *destlen;
    rv = uncompress((Bytef*)dest, &dl, (Bytef*)source, sl);
    *destlen = dl;

    rv = (rv == Z_OK) ? 0 : -1;

    return rv;
}


/* skio_abstract_t.strerror */
static const char *
gz_strerror(
    void               *vfd,
    int                 io_errno)
{
    int gzerr;
    gzFile fd = (gzFile)vfd;
    const char *errstr;

    errstr =  gzerror(fd, &gzerr);
    if (gzerr == Z_ERRNO) {
        errstr = strerror(io_errno);
    }
    return errstr;
}


/* skio_abstract_t.flush */
static int
gz_flush(
    void               *vfd)
{
    gzFile fd = (gzFile)vfd;
    return gzflush(fd, Z_SYNC_FLUSH);
}


/* Bind an gzip IO buffer */
int
skIOBufBindGzip(
    sk_iobuf_t         *fd,
    gzFile              file,
    int                 compmethod)
{
    skio_abstract_t io;

    io.read = (skio_read_fn_t)&gzread;
    io.write = (skio_write_fn_t)&gzwrite;
    io.seek = NULL;
    io.strerror = &gz_strerror;
    io.flush = &gz_flush;
    io.free_fd = NULL;
    return skIOBufBindAbstract(fd, file, compmethod, &io);
}


int
skIOBufSetZlibLevel(
    sk_iobuf_t         *fd,
    int                 level)
{
    assert(fd);
    if (fd == NULL) {
        return -1;
    }

    if (!(level >= Z_BEST_SPEED && level <= Z_BEST_COMPRESSION) &&
        level != Z_NO_COMPRESSION && level != Z_DEFAULT_COMPRESSION)
    {
        SKIOBUF_INTERNAL_ERROR(fd, ESKIO_BADOPT);
    }

    fd->compr_opts.zlib.level = level;

    return 0;
}

#endif  /* SK_ENABLE_ZLIB */


#if SK_ENABLE_LZO

/* LZO methods */

/* iobuf_methods_t.init_method */
static int
lzo_init_method(
    iobuf_opts_t       *opts)
{
    static int initialized = 0;

    if (initialized) {
        if (lzo_init()) {
            return -1;
        }
        initialized = 1;
    }

    opts->lzo.scratch = (uint8_t *)calloc(LZO1X_1_15_MEM_COMPRESS, 1);
    if (opts->lzo.scratch == NULL) {
        return -1;
    }

    return 0;
}


/* iobuf_methods_t.uninit_method */
static int
lzo_uninit_method(
    iobuf_opts_t       *opts)
{
    assert(opts->lzo.scratch != NULL);
    free(opts->lzo.scratch);
    opts->lzo.scratch = NULL;
    return 0;
}


/* iobuf_methods_t.compr_size_method */
static uint32_t
lzo_compr_size_method(
    uint32_t                    compr_size,
    const iobuf_opts_t  UNUSED(*opts))
{
    /* The following formula is in the lzo faq:
       http://www.oberhumer.com/opensource/lzo/lzofaq.php */
    return (compr_size + (compr_size >> 4) + 64 + 3);
}


/* iobuf_methods_t.compr_method */
static int
lzo_compr_method(
    void               *dest,
    uint32_t           *destlen,
    const void         *source,
    uint32_t            sourcelen,
    const iobuf_opts_t *opts)
{
    lzo_uint sl, dl;
    int rv;

    assert(sizeof(sl) >= sizeof(sourcelen));
    assert(sizeof(dl) >= sizeof(*destlen));

    dl = *destlen;
    sl = sourcelen;
    rv = lzo1x_1_15_compress((const unsigned char*)source, sl,
                             (unsigned char*)dest, &dl, opts->lzo.scratch);
    *destlen = dl;

    return rv;
}


/* iobuf_methods_t.uncompr_method */
static int
lzo_uncompr_method(
    void                       *dest,
    uint32_t                   *destlen,
    const void                 *source,
    uint32_t                    sourcelen,
    const iobuf_opts_t  UNUSED(*opts))
{
    lzo_uint sl, dl;
    int rv;

    assert(sizeof(sl) >= sizeof(sourcelen));
    assert(sizeof(dl) >= sizeof(*destlen));

    dl = *destlen;
    sl = sourcelen;
#ifdef SK_HAVE_LZO1X_DECOMPRESS_ASM_FAST_SAFE
    rv = lzo1x_decompress_asm_fast_safe(source, sl, dest, &dl, NULL);
#else
    rv = lzo1x_decompress_safe((const unsigned char*)source, sl,
                               (unsigned char*)dest, &dl, NULL);
#endif
    *destlen = dl;

    return rv;
}

#endif  /* SK_ENABLE_LZO */


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
