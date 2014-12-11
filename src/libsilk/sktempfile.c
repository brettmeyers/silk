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
**  sktempfile.c
**
**    Functions to handle temp file creation and access.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: sktempfile.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>
#include <silk/skvector.h>
#include <silk/sktempfile.h>


/* TYPDEFS AND DEFINES */

/* Print debugging messages when this environment variable is set to a
 * positive integer. */
#define SKTEMPFILE_DEBUG_ENVAR "SILK_TEMPFILE_DEBUG"


/* typedef struct sk_tempfilectx_st sk_tempfilectx_t; */
struct sk_tempfilectx_st {
    /* template used to make temporary files. */
    char tf_template[PATH_MAX];

    /* names of temporary files */
    sk_vector_t *tf_names;

    /* whether to enable debugging */
    unsigned     print_debug :1;
};


/*
 *  TEMPFILE_DEBUGn(td_ctx, td_fmt, ...);
 *
 *    Print a message when the print_debug member is active.  Use a
 *    value of n that matches the number of arguments to the format.
 *
 *    TEMPFILE_DEBUG1(tmpctx, "one is %d", 1);
 */
#define TEMPFILE_DEBUG0(td_ctx, td_fmt)                         \
    if (!(td_ctx)->print_debug) { /* no-op */ } else {          \
        skAppPrintErr(SKTEMPFILE_DEBUG_ENVAR ": " td_fmt);      \
    }
#define TEMPFILE_DEBUG1(td_ctx, td_fmt, td_arg1)                \
    if (!(td_ctx)->print_debug) { /* no-op */ } else {          \
        skAppPrintErr((SKTEMPFILE_DEBUG_ENVAR ": " td_fmt),     \
                      (td_arg1));                               \
    }
#define TEMPFILE_DEBUG2(td_ctx, td_fmt, td_arg1, td_arg2)       \
    if (!(td_ctx)->print_debug) { /* no-op */ } else {          \
        skAppPrintErr((SKTEMPFILE_DEBUG_ENVAR ": " td_fmt),     \
                      (td_arg1), (td_arg2));                    \
    }
#define TEMPFILE_DEBUG3(td_ctx, td_fmt, td_arg1, td_arg2, td_arg3)      \
    if (!(td_ctx)->print_debug) { /* no-op */ } else {                  \
        skAppPrintErr((SKTEMPFILE_DEBUG_ENVAR ": " td_fmt),             \
                      (td_arg1), (td_arg2), (td_arg3));                 \
    }
#define TEMPFILE_DEBUG4(td_ctx, td_fmt, td_arg1, td_arg2, td_arg3, td_arg4) \
    if (!(td_ctx)->print_debug) { /* no-op */ } else {                  \
        skAppPrintErr((SKTEMPFILE_DEBUG_ENVAR ": " td_fmt),             \
                      (td_arg1), (td_arg2), (td_arg3), (td_arg4));      \
    }


/* EXPORTED VARIABLES */

/* placeholder for files that have been removed from the vector */
const char * const sktempfile_null = "NULL";


/* LOCAL VARIABLES */



/* FUNCTION DEFINITIONS */


/* find the tmpdir to use, initialize template, create vector */
int
skTempFileInitialize(
    sk_tempfilectx_t  **tmpctx,
    const char         *user_temp_dir,
    const char         *prefix_name,
    sk_msg_fn_t         err_fn)
{
    sk_tempfilectx_t *t;
    const char *tmp_dir = NULL;
    const char *env_value;
    uint32_t debug_lvl;
    int rv;

    if (NULL == prefix_name) {
        prefix_name = skAppName();
    }

    tmp_dir = skTempDir(user_temp_dir, err_fn);
    if (NULL == tmp_dir) {
        return -1;
    }

    t = (sk_tempfilectx_t*)calloc(1, sizeof(sk_tempfilectx_t));
    if (NULL == t) {
        return -1;
    }

    rv = snprintf(t->tf_template, sizeof(t->tf_template),
                  "%s/%s.%d.XXXXXXXX", tmp_dir, prefix_name, (int)getpid());
    if ((size_t)rv >= sizeof(t->tf_template) || rv < 0) {
        if (err_fn) {
            err_fn("Error initializing template for temporary file names");
        }
        free(t);
        return -1;
    }

    /* initialize tmp_names */
    t->tf_names = skVectorNew(sizeof(char*));
    if (NULL == t->tf_names) {
        if (err_fn) {
            err_fn("Unable to allocate vector for temporary file names");
        }
        free(t);
        return -1;
    }

    /* Check for debugging */
    env_value = getenv(SKTEMPFILE_DEBUG_ENVAR);
    if ((env_value != NULL)
        && (0 == skStringParseUint32(&debug_lvl, env_value, 1, 0)))
    {
        t->print_debug = 1;
    }

    TEMPFILE_DEBUG1(t, "Initialization complete for '%s'", t->tf_template);

    *tmpctx = t;
    return 0;
}


/* remove all temp files and destroy vector */
void
skTempFileTeardown(
    sk_tempfilectx_t  **tmpctx)
{
    sk_tempfilectx_t *t;
    int i;

    if (tmpctx && *tmpctx) {
        t = *tmpctx;
        *tmpctx = NULL;

        if (t->tf_names) {
            for (i = skVectorGetCount(t->tf_names)-1; i >= 0; --i) {
                skTempFileRemove(t, i);
            }
            skVectorDestroy(t->tf_names);
        }

        TEMPFILE_DEBUG1(t, "Teardown complete for '%s'", t->tf_template);

        free(t);
    }
}


/* create new file */
FILE *
skTempFileCreate(
    sk_tempfilectx_t   *tmpctx,
    int                *tmp_idx,
    char              **out_name)
{
    int saved_errno;
    FILE *fp = NULL;
    char *name;
    int fd;

    /* copy template name */
    name = strdup(tmpctx->tf_template);
    if (NULL == name) {
        return NULL;
    }

    /* open file */
    fd = mkstemp(name);
    if (fd == -1) {
        saved_errno = errno;
        TEMPFILE_DEBUG2(tmpctx, "Failed to mkstemp('%s'): %s",
                        name, strerror(errno));
        free(name);
        errno = saved_errno;
        return NULL;
    }

    /* convert descriptor to pointer */
    fp = fdopen(fd, "w");
    if (fp == NULL) {
        saved_errno = errno;
        TEMPFILE_DEBUG3(tmpctx, "Failed to fdopen(%d ['%s']): %s",
                        fd, name, strerror(errno));
        close(fd);
        free(name);
        errno = saved_errno;
        return NULL;
    }

    /* append to vector */
    if (skVectorAppendValue(tmpctx->tf_names, &name)) {
        saved_errno = errno;
        TEMPFILE_DEBUG1(tmpctx, "Failed to skVectorAppendValue(): %s",
                        strerror(errno));
        close(fd);
        free(name);
        errno = saved_errno;
        return NULL;
    }
    *tmp_idx = skVectorGetCount(tmpctx->tf_names) - 1;

    TEMPFILE_DEBUG2(tmpctx, "Created new temp %d => '%s'", *tmp_idx, name);

    if (out_name) {
        *out_name = name;
    }
    return fp;
}


/* get name of file */
const char *
skTempFileGetName(
    const sk_tempfilectx_t *tmpctx,
    int                     tmp_idx)
{
    char **name;

    name = (char**)skVectorGetValuePointer(tmpctx->tf_names, tmp_idx);
    if (name && *name) {
        return *name;
    }
    return sktempfile_null;
}


/* open existing file */
FILE *
skTempFileOpen(
    const sk_tempfilectx_t *tmpctx,
    int                     tmp_idx)
{
    const char *name;

    name = skTempFileGetName(tmpctx, tmp_idx);
    TEMPFILE_DEBUG2(tmpctx, "Opening existing temp %d => '%s'", tmp_idx, name);

    if (name == sktempfile_null) {
        return NULL;
    }
    return fopen(name, "r");
}


/* remove file */
void
skTempFileRemove(
    sk_tempfilectx_t   *tmpctx,
    int                 tmp_idx)
{
    const char *name;

    name = skTempFileGetName(tmpctx, tmp_idx);
    if (name == sktempfile_null) {
        TEMPFILE_DEBUG2(tmpctx, "Removing temp %d => '%s'", tmp_idx, name);
        return;
    }

    TEMPFILE_DEBUG3(tmpctx, "Removing temp %d => '%s' of size %" PRId64,
                    tmp_idx, name, (int64_t)skFileSize(name));
    if (-1 == unlink(name)) {
        if (skFileExists(name)) {
            TEMPFILE_DEBUG2(tmpctx, "Failed to unlink('%s'): %s",
                            name, strerror(errno));
        }
    }

    free((char*)name);
    skVectorSetValue(tmpctx->tf_names, tmp_idx, &sktempfile_null);
}


/*
 *  ok = skTempFileWriteBuffer(&tmp_idx, buffer, size, count);
 *
 *    Print 'count' records having size 'size' bytes from 'buffer'
 *    into a newly created temporary file.  Index the temp file by
 *    'tmp_idx'.  Returns 0 on success, or -1 if a temp file cannot be
 *    created, or the write or close calls fail.
 */
int
skTempFileWriteBuffer(
    sk_tempfilectx_t   *tmpctx,
    int                *tmp_idx,
    const void         *rec_buffer,
    uint32_t            rec_size,
    uint32_t            rec_count)
{
    int saved_errno = 0;
    FILE* temp_filep = NULL;
    char *name;
    int rv = -1; /* return value */

    temp_filep = skTempFileCreate(tmpctx, tmp_idx, &name);
    if (temp_filep == NULL) {
        saved_errno = errno;
        goto END;
    }

    TEMPFILE_DEBUG3(tmpctx, "Writing %" PRIu32 " records to temp %d => '%s'",
                    rec_count, *tmp_idx, name);
    if (rec_count != fwrite(rec_buffer, rec_size, rec_count, temp_filep)) {
        /* error writing */
        saved_errno = errno;
        TEMPFILE_DEBUG2(tmpctx, "Failed to fwrite('%s'): %s",
                        name, strerror(errno));
        goto END;
    }

    /* Success so far */
    rv = 0;

  END:
    /* close the file if open */
    if (temp_filep) {
        if (rv != 0) {
            /* already an error, so just close the file */
            fclose(temp_filep);
        } else if (fclose(temp_filep) == EOF) {
            saved_errno = errno;
            TEMPFILE_DEBUG2(tmpctx, "Failed to fclose('%s'): %s",
                            name, strerror(errno));
            rv = -1;
        }
    }
    errno = saved_errno;
    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
