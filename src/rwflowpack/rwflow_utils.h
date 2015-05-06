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
#ifndef _RWFLOW_UTILS_H
#define _RWFLOW_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWFLOW_UTILS_H, "$SiLK: rwflow_utils.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/*
**  rwflow_utils.h
**
**    Definitions and prototypes for functions shared between
**    rwflowpack and rwflowappend.
*/

#include <silk/skstream.h>


/*
 *  stream = openRepoStream(repo_file, &out_mode, no_lock, &shut_down_flag);
 *
 *    Either open an existing repository (hourly) data file or create
 *    a new repository file at the location specified by 'repo_file'.
 *    Return the opened stream on success; return NULL on failure.
 *
 *    The location pointed to be 'out_mode' will be set to
 *    SK_IO_APPEND if an existing file was opened, or SK_IO_WRITE if a
 *    new file was opened.
 *
 *    When a file is successfully opened, the function will obtain a
 *    write lock on the file unless the 'no_lock' argument is
 *    non-zero.
 *
 *    The caller must provide the location of the variable that
 *    denotes when the daemon is shutting down in the 'shut_down_flag'
 *    argument.  The function will check this variable while obtaining
 *    the write lock.
 *
 *    For existing files opened for append, the file descriptor is
 *    positioned at the end of the file.  For new files, the file
 *    descriptor is at position 0 and the file is empty.
 */
skstream_t *
openRepoStream(
    const char         *repo_file,
    skstream_mode_t    *out_mode,
    int                 no_lock,
    volatile int       *shut_down_flag);


/*
 *  status = verifyCommandString(command, switch_name);
 *
 *    Verify that the command string specified in 'command' does not
 *    contain unknown conversions.  If 'command' is valid, return 0.
 *
 *    If 'command' is not valid, print an error that 'switch_name' is
 *    invalid and return -1.  If 'switch_name' is NULL, no error is
 *    printed.
 */
int
verifyCommandString(
    const char         *command,
    const char         *switch_name);


/*
 *  runCommand(command, file);
 *
 *    Run the command specified in 'command' in a subprocess.  The
 *    string "%s" in 'command' will be replaced by 'filename'.
 */
void
runCommand(
    const char         *command,
    const char         *file);


/*
 *  errorDirectorySetPath(directory);
 *
 *    Set 'directory' as the name of the error directory.  Overrides
 *    any previous value.  If 'directory' is NULL, unsets the
 *    error-directory.
 *
 *    The caller should ensure 'directory' is an existing directory
 *    prior to calling this function.
 *
 *    This function simply maintains a pointer to the specified value
 *    'directory'.  The caller should ensure 'directory' is valid
 *    throughout the life of the application.
 */
void
errorDirectorySetPath(
    const char         *directory);


/*
 *  is_set = errorDirectoryIsSet();
 *
 *    Return 1 if an error directory has been specified.  Return 0
 *    otherwise.
 */
int
errorDirectoryIsSet(
    void);


/*
 *  status = errorDirectoryInsertFile(filename);
 *
 *    Move 'filename' to the error-directory.  Return 0 on success.
 *    Return 1 if the error-directory is not set.  Return -1 on error.
 */
int
errorDirectoryInsertFile(
    const char         *filename);


/*
 *  archiveDirectorySetFlat();
 *
 *    On a call to archiveDirectoryInsertOrRemove(), do not create
 *    subdirectories under the archive-directory.
 */
void
archiveDirectorySetFlat(
    void);


/*
 *  archiveDirectorySetPath(directory);
 *
 *    Set 'directory' as the name of the archive directory.  Overrides
 *    any previous value.  If 'directory' is NULL, unsets the
 *    archive-directory.
 *
 *    The caller should ensure 'directory' is an existing directory
 *    prior to calling this function.
 *
 *    This function simply maintains a pointer to the specified value
 *    'directory'.  The caller should ensure 'directory' is valid
 *    throughout the life of the application.
 */
void
archiveDirectorySetPath(
    const char         *directory);


/*
 *  archiveDirectorySetPostCommand(command);
 *
 *    Specify 'command' as a command to run on files that get copied
 *    into the archive-directory.  The string "%s" in command will be
 *    replaced with the path to the archived file.
 */
void
archiveDirectorySetPostCommand(
    const char         *command);


/*
 *  archiveDirectorySetNoRemove();
 *
 *    Normally, calling archiveDirectoryInsertOrRemove(f) always
 *    effects the file 'f'.  The file 'f' is either moved into the
 *    archive-directory or 'f' is completely removed if the
 *    archive-directory is not set.  However, if this function has
 *    been called and if archive-directory is not set, the file 'f'
 *    will not be removed.
 */
void
archiveDirectorySetNoRemove(
    void);


/*
 *  is_set = archiveDirectoryIsSet();
 *
 *    Return 1 if an archive directory has been specified.  Return -1
 *    if a post-archive-command has been set but the archive-directory
 *    is not set.  Return 0 if neither an archive-directory nor a
 *    post-archive-command has been specified.
 */
int
archiveDirectoryIsSet(
    void);


/*
 *  status = archiveDirectoryInsertOrRemove(filename, sub_directory);
 *
 *    When an archive-directory has NOT been specified, this function
 *    removes 'filename' (unless archiveDirectorySetNoRemove() was
 *    called) and returns 1.
 *
 *    When an archive-directory has been specified and
 *    archiveDirectorySetFlat() has been called, this function moves
 *    'filename' into the archive-directory itself, regardless of the
 *    value of 'sub_directory'.
 *
 *    When an archive-directory has been specified and
 *    archiveDirectorySetFlat() has NOT been called, this function
 *    moves 'filename' into a subdirectory under the
 *    archive-directory.  If 'sub_directory' is NULL, a subdirectory
 *    based on the current time is created.  Otherwise,
 *    'sub_directory' is treated as being relative to
 *    archive-directory and that subdirectory is created.  Note that
 *    'sub_directory' may contain directory separators.
 *
 *    If a post-archiving command has been specified, that command is
 *    run on the file after is has been copied into the
 *    archive-directory.
 *
 *    If everything succeeds, return 0.  If there is an error creating
 *    a subdirectory or moving a file, this function returns -1.
 *    Errors cased by the post-command are ignored.
 */
int
archiveDirectoryInsertOrRemove(
    const char         *filename,
    const char         *sub_directory);


#ifdef __cplusplus
}
#endif
#endif /* _RWFLOW_UTILS_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
