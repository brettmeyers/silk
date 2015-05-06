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
**  skpolldir.h
**
**    Library for polling directories for new files.
**
*/
#ifndef _POLLDIR_H
#define _POLLDIR_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_POLLDIR_H, "$SiLK: skpolldir.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/**
 *  @file
 *
 *    Implementation of a data structure for polling a directory for
 *    newly arrived files.
 *
 *    This file is part of libsilk-thrd.
 *
 *
 *    An skPollDir_t object is created with skPollDirCreate().  The
 *    directory to poll and the interval between polls of that
 *    directory are specified as parameters.
 *
 *    If during a poll of the directory skPollDir_t notices a new file
 *    (for example, a file has been added to the directory),
 *    skPollDir_t notes the name and size of that file.  (skPollDir_t
 *    ignores files whose size is 0 bytes and files whose names begin
 *    with a dot.)  On its next directory poll, skPollDir_t checks
 *    whether that file has the same size as on the previous poll.  If
 *    it does, the file's name is added to a list of "ready" files; if
 *    the file's size is different, skPollDir_t records the new size
 *    and checks the file again on its next poll.
 *
 *    Because files must exist at constant size for two directory
 *    polls, it will take skPollDir_t at least INTERVAL seconds to
 *    notice a file, a maximum of 2*INTERVAL seconds, and an average
 *    of 1.5*INTERVAL seconds.
 *
 *    Note that skPollDir_t does not check the timestamps on the
 *    files.  If a file is moved out of the directory being polled and
 *    put back into the directory between polls, skPollDir_t does not
 *    notice the "new" file because it believes it is looking at the
 *    same file.  If the file is moved out of the directory,
 *    skPollDir_t polls the directory, and then the file is put back
 *    into the directory, skPollDir_t will treat the file as a new
 *    file.
 *
 *    A call to skPollDirGetNextFile() gets a value from the "ready"
 *    list and returns the file name to the caller.  If there are no
 *    files in the "ready" list, skPollDirGetNextFile() blocks (see
 *    next paragraph).  Before skPollDirGetNextFile() returns a file
 *    name to the caller, it verifies that the file still exists.  If
 *    the file has been removed from the file system,
 *    skPollDirGetNextFile() goes to the next file on the "ready"
 *    list.
 *
 *    The skPollDirSetFileTimeout() function allows the caller to
 *    specify the number of seconds skPollDirGetNextFile() will block
 *    waiting for a file to appear on the "ready" list.  The default
 *    is for skPollDirGetNextFile() to wait indefinitely.
 *
 *    A call to skPollDirStop() causes skPollDir_t to stop polling the
 *    directory and unblocks any calls to skPollDirGetNextFile().
 *
 *    A call to skPollDirDestroy() destroys the skPollDir_t object.
 *    Calling skPollDirDestroy() while skPollDirGetNextFile() calls
 *    are still active is undefined.
 *
 *    To poll its directory, each skPollDir_t requires a file handle.
 *    When many skPollDir_t objects have been created, they could
 *    exhaust the number of file handles if they all polled
 *    simultaneously.  To prevent this, there is limit on the number
 *    of file handles the skpolldir library will use.  The default
 *    limit is SKPOLLDIR_DEFAULT_MAX_FILEHANDLES; the caller may
 *    adjust this value by calling skPollDirSetMaximumFileHandles().
 *
 *
 *    Possible extensions: Have skPollDir_t respect advisory locks
 *    (c.f., flock(2)) that exist on the files in the directory.
 *
 *
 */

/**
 *    Default maximum number of filehandles for skpolldir to allow to
 *    be used simultaneously.
 */
#define SKPOLLDIR_DEFAULT_MAX_FILEHANDLES 32

/**
 *    The type of directory polling objects.
 */
typedef struct sk_polldir_st skPollDir_t;

/**
 *    The type of polldir errors.
 */
typedef enum {
    PDERR_NONE = 0,
    PDERR_STOPPED,
    PDERR_MEMORY,
    PDERR_SYSTEM,
    PDERR_TIMEDOUT
} skPollDirErr_t;

/**
 *    Create a directory polling object.
 *
 *    Creates a polldir object for directory.  The poll_interval is
 *    the number of seconds between polls of the directory for new
 *    files.
 */
skPollDir_t *
skPollDirCreate(
    const char         *directory,
    uint32_t            poll_interval);

/**
 *    Stop polling a directory.
 *
 *    Calling this function will cause skPollDirGetNextFile() to
 *    return PDERR_STOPPED.
 */
void
skPollDirStop(
    skPollDir_t        *pd);

/**
 *    Destroy a directory polling object.  Does nothing if the object
 *    is NULL.
 */
void
skPollDirDestroy(
    skPollDir_t        *pd);

/**
 *    Get the next added filename entry to a directory.
 *
 *    This function blocks on the existance of a new file entry to the
 *    polldir directory.  The buffer 'path' will be filled with the
 *    full path to the file entry.  It is up to the caller to make
 *    sure that 'path' is at least PATH_MAX bytes in size.  If
 *    'filename_ptr' is non-NULL, it will set to the position in
 *    'path' where the filename begins.
 *
 *    Under normal circumstances, this function will return
 *    PDERR_NONE.
 *
 *    If a polldir timeout has been set by skPollDirSetFileTimeout(),
 *    this function will return PDERR_TIMEDOUT if no file is seen
 *    within the specified number of seconds.
 *
 *    In the event that the polldir was stopped using skPollDirStop(),
 *    this will return PDERR_STOPPED.
 *
 *    If a system error occurred, the function will return
 *    PDERR_SYSTEM, and it will set errno to the appropriate system
 *    error value.
 *
 *    If there was a memory error, this function will return
 *    PDERR_MEMORY.
 */
skPollDirErr_t
skPollDirGetNextFile(
    skPollDir_t        *pd,
    char               *path,
    char              **filename_ptr);

/**
 *    Puts a file back on the polldir object so it can be retrieved
 *    again.
 *
 *    A file that is "put back" will be the next file that
 *    skPollDirGetNextFile() will retrieve.  skPollDirGetNextFile()
 *    checks for the file's existence before returning it, so if
 *    'filename' does not exist, the file will not be returned.
 *
 *    Please note that 'filename' is the name of the file.  The file
 *    is expected to be in the same directory that the skPollDir_t
 *    object is polling.
 *
 *    If there was a memory error, this function will return
 *    PDERR_MEMORY.
 *
 */
skPollDirErr_t
skPollDirPutBackFile(
    skPollDir_t        *pd,
    const char         *filename);

/**
 *    Return a string describing an error.
 */
const char *
skPollDirStrError(
    skPollDirErr_t      err);

/**
 *    Get the name of the directory being polled by a polldir object.
 */
const char *
skPollDirGetDir(
    skPollDir_t        *pd);

/**
 *    Set the maximum number of seconds skPollDirGetNextFile() will
 *    wait for a file to appear in the directoy.  When a non-zero
 *    value is specified for 'timeout_seconds', skPollDirGetNextFile()
 *    returns PDERR_TIMEDOUT if no file is available after the
 *    specified number of seconds.
 *
 *    If this function has not been called or is called with a
 *    'timeout_seconds' value of 0, the skPollDirGetNextFile() call
 *    will wait indefinitely, or until explicitly stopped with
 *    skPollDirStop().
 *
 *    Calling this function has no effect on any blocking calls to
 *    skPollDirGetNextFile().  This function has no relation to the
 *    polling interval.
 */
void
skPollDirSetFileTimeout(
    skPollDir_t        *pd,
    uint32_t            timeout_seconds);

/**
 *    Set the maximum number of file handles that can be open
 *    simultaneously across all skPollDir_t objects.  Return 0 unless
 *    'max_fh' is a non-positive integer value.
 */
int
skPollDirSetMaximumFileHandles(
    int                 max_fh);


#ifdef __cplusplus
}
#endif
#endif /* _POLLDIR_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
