/*
** Copyright (C) 2003-2014 by Carnegie Mellon University.
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
**  Reads PDUs from a router and writes the flows into packed files.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwflow_utils.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/sklog.h>
#include <silk/utils.h>
#include "rwflow_utils.h"

/* use TRACEMSG_LEVEL as our tracing variable */
#define TRACEMSG(lvl, msg) TRACEMSG_TO_TRACEMSGLVL(lvl, msg)
#include <silk/sktracemsg.h>


/* LOCAL VARIABLES */

/* where to store files on error */
static const char *error_directory = NULL;

/* where to archive files */
static const char *archive_directory = NULL;

/* command to run on archived files */
static const char *post_archive_command = NULL;

/* whether to remove files when archive_directory is NULL. */
static int remove_when_archive_null = 1;

/* by default, files are stored in subdirectories of the
 * archive_directory.  If the following value is non-zero,
 * subdirectories are not created. */
static int archive_flat = 0;


/* FUNCTION DEFINITIONS */

/*
 *  stream = openRepoStream(repo_file, &out_mode, no_lock, &shut_down_flag);
 *
 *    Either open an existing repository (hourly) data file or create
 *    a new repository file at the location specified by 'repo_file'.
 *    See the header for details.
 */
skstream_t *
openRepoStream(
    const char         *repo_file,
    skstream_mode_t    *out_mode,
    int                 no_lock,
    volatile int       *shut_down_flag)
{
    char buf[PATH_MAX];
    skstream_t *stream = NULL;
    ssize_t rv = SKSTREAM_OK;
    int filemod = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int flags;
    int fd = -1;

    /* Open an existing hourly file or create a new hourly file as
     * necessary. */
    if (skFileExists(repo_file)) {
        DEBUGMSG("Opening existing repository file '%s'", repo_file);

        /* Open existing file for read and write. */
        flags = O_RDWR | O_APPEND;
        fd = open(repo_file, flags, filemod);
        if (-1 == fd) {
            if (ENOENT != errno) {
                WARNINGMSG("Unable to open existing output file '%s': %s",
                           repo_file, strerror(errno));
                return NULL;
            }
            DEBUGMSG(("Existing file removed before opening;"
                      " attempting to open new file '%s'"), repo_file);
            flags = O_RDWR | O_CREAT | O_EXCL;
            fd = open(repo_file, flags, filemod);
            if (-1 == fd) {
                WARNINGMSG("Unable to open new output file '%s': %s",
                           repo_file, strerror(errno));
                return NULL;
            }
        }

    } else {
        INFOMSG("Opening new repository file '%s'", repo_file);

        /* Create directory for new file */
        if (!skDirname_r(buf, repo_file, sizeof(buf))) {
            WARNINGMSG("Unable to determine directory of '%s'",
                       repo_file);
            return NULL;
        }
        if (!skDirExists(buf)) {
            TRACEMSG(3, ("Creating directory '%s'...", buf));
            if (skMakeDir(buf)) {
                WARNINGMSG("Unable to create directory '%s': %s",
                           buf, strerror(errno));
                return NULL;
            }
        }

        /* Open new file. */
        flags = O_RDWR | O_CREAT | O_EXCL;
        fd = open(repo_file, flags, filemod);
        if (-1 == fd) {
            if (EEXIST != errno) {
                WARNINGMSG("Unable to open new output file '%s': %s",
                           repo_file, strerror(errno));
                return NULL;
            }
            DEBUGMSG(("Nonexistent file appeared before opening;"
                      " attempting to open existing file '%s'"), repo_file);
            flags = O_RDWR | O_APPEND;
            fd = open(repo_file, flags, filemod);
            if (-1 == fd) {
                WARNINGMSG("Unable to open new output file '%s': %s",
                           repo_file, strerror(errno));
                return NULL;
            }
        }
    }

    TRACEMSG(2, ("Flags are 0x%x for opened file '%s'", flags, repo_file));

    /* Lock the file */
    if (!no_lock) {
        TRACEMSG(1, ("Locking file '%s'", repo_file));
        while (skFileSetLock(fd, F_WRLCK, F_SETLKW) != 0) {
            if (*shut_down_flag) {
                TRACEMSG(1, ("Shutdown while locking '%s'", repo_file));
                goto ERROR;
            }
            switch (errno) {
              case EINTR:
                TRACEMSG(1, ("Interrupt while locking '%s'", repo_file));
                continue;
              case ENOLCK:
              case EINVAL:
                TRACEMSG(1, ("Errno %d while locking '%s'",errno,repo_file));
                NOTICEMSG("Unable to get write lock;"
                          " consider using the --no-file-locking switch");
                break;
              default:
                TRACEMSG(1, ("Errno %d while locking '%s'",errno,repo_file));
                break;
            }
            goto ERROR;
        }
    }

    /*
     * At this point we have the write lock.  Regardless of whether we
     * think the file is new or existing, we need to check for the
     * file header for a couple of reasons: (1)We may be opening a
     * 0-length file from a previously failed attempt. (2)We may open
     * a new file but another process can find the file, lock it, and
     * write the header to it prior to us locking the file.
     */

    /* Can we read the number of bytes in a SiLK file header?  The
     * header will be read and verified when the descriptor is bound
     * to an skstream. */
    rv = read(fd, buf, sizeof(sk_header_start_t));
    if (rv == (int)sizeof(sk_header_start_t)) {
        TRACEMSG(1, ("Read all header bytes from file '%s'", repo_file));
        /* file has enough bytes to contain a silk header; will treat
         * it as SK_IO_APPEND */
        if (!(flags & O_APPEND)) {
            /* add O_APPEND to the flags */
            DEBUGMSG("Found data in file; will append to '%s'", repo_file);
            flags = fcntl(fd, F_GETFL, 0);
            if (-1 == flags) {
                WARNINGMSG("Failed to get flags for file '%s': %s",
                           repo_file, strerror(errno));
                goto ERROR;
            }
            flags |= O_APPEND;
            TRACEMSG(2, ("Setting flags to 0x%x for '%s'", flags,repo_file));
            rv = fcntl(fd, F_SETFL, flags);
            if (-1 == rv) {
                WARNINGMSG("Failed to set flags for file '%s': %s",
                           repo_file, strerror(errno));
                goto ERROR;
            }
        }
        /* else, flags include O_APPEND, we are good. */

    } else if (0 == rv) {
        TRACEMSG(1, ("Read no header bytes from file '%s'", repo_file));
        /* file is empty; will treat it as SK_IO_WRITE */
        if (flags & O_APPEND) {
            /* must remove the O_APPEND flag */
            DEBUGMSG("Opened empty file; adding header to '%s'", repo_file);
            flags = fcntl(fd, F_GETFL, 0);
            if (-1 == flags) {
                WARNINGMSG("Failed to get flags for file '%s': %s",
                           repo_file, strerror(errno));
                goto ERROR;
            }
            flags &= ~O_APPEND;
            TRACEMSG(2, ("Setting flags to 0x%x for '%s'", flags,repo_file));
            rv = fcntl(fd, F_SETFL, flags);
            if (-1 == rv) {
                WARNINGMSG("Failed to set flags for file '%s': %s",
                           repo_file, strerror(errno));
                goto ERROR;
            }
        }
        /* else, flags do not include O_APPEND, we are good */

    } else if (-1 == rv) {
        WARNINGMSG("Error attempting to read file header from '%s': %s",
                   repo_file, strerror(errno));
        goto ERROR;
    } else {
        /* short read */
        WARNINGMSG("Read %" SK_PRIdZ "/%" SK_PRIuZ " bytes from '%s'",
                   rv, sizeof(sk_header_start_t), repo_file);
        goto ERROR;
    }

    TRACEMSG(2, ("Flags are 0x%x for opened file '%s'",
                 fcntl(fd, F_GETFL, 0), repo_file));

    *out_mode = ((flags & O_APPEND) ? SK_IO_APPEND : SK_IO_WRITE);

    /* File looks good; create an skstream */
    TRACEMSG(1, ("Creating %s skstream for '%s'",
                 ((SK_IO_APPEND == *out_mode) ? "APPEND" : "WRITE"),
                 repo_file));
    if ((rv = skStreamCreate(&stream, *out_mode, SK_CONTENT_SILK_FLOW))
        || (rv = skStreamBind(stream, repo_file))
        || (rv = skStreamFDOpen(stream, fd)))
    {
        /* NOTE: it is possible for skStreamFDOpen() to have stored
         * the value of 'fd' but return an error. */
        if (stream && skStreamGetDescriptor(stream) == fd) {
            fd = -1;
        }
        goto ERROR;
    }
    /* The stream controls this now */
    fd = -1;

    if (SK_IO_APPEND == *out_mode) {
        /* read the header---which also seeks to the end of the
         * file */
        rv = skStreamReadSilkHeader(stream, NULL);
        if (rv) {
            goto ERROR;
        }
    }

    return stream;

  ERROR:
    if (stream) {
        if (rv) {
            skStreamPrintLastErr(stream, rv, &WARNINGMSG);
        }
        skStreamDestroy(&stream);
    }
    if (-1 != fd) {
        close(fd);
    }
    return NULL;
}


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
    const char         *switch_name)
{
    const char *cp = command;

    while (NULL != (cp = strchr(cp, '%'))) {
        ++cp;
        switch (*cp) {
          case '%':
          case 's':
            ++cp;
            break;
          case '\0':
            if (switch_name) {
                skAppPrintErr(("Invalid %s '%s':"
                               " '%%' appears at end of string"),
                              switch_name, command);
            }
            return -1;
          default:
            if (switch_name) {
                skAppPrintErr("Invalid %s '%s': Unknown conversion '%%%c'",
                              switch_name, command, *cp);
            }
            return -1;
        }
    }
    return 0;
}


/*
 *  runCommand(command, filename);
 *
 *    Spawn a new subprocess to run 'command'.  Formatting directives
 *    in 'command' may be expanded to hold to a 'filename'.
 */
void
runCommand(
    const char         *command,
    const char         *file)
{
    sigset_t sigs;
    pid_t pid;
    size_t len;
    size_t file_len;
    const char *cp;
    const char *sp;
    char *expanded_cmd;
    char *exp_cp;

    /* Parent (original process) forks to create Child 1 */
    pid = fork();
    if (-1 == pid) {
        ERRMSG("Could not fork to run command: %s", strerror(errno));
        return;
    }

    /* Parent reaps Child 1 and returns */
    if (0 != pid) {
        /* Wait for Child 1 to exit. */
        while (waitpid(pid, NULL, 0) == -1) {
            if (EINTR != errno) {
                NOTICEMSG("Error waiting for child %ld: %s",
                          (long)pid, strerror(errno));
                break;
            }
        }
        return;
    }

    /* Disable log rotation in Child 1 */
    sklogDisableRotation();

    /* Child 1 forks to create Child 2 */
    pid = fork();
    if (pid == -1) {
        ERRMSG("Child could not fork for to run command: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    /* Child 1 immediately exits, so Parent can stop waiting */
    if (pid != 0) {
        _exit(EXIT_SUCCESS);
    }

    /* Only Child 2 makes it here */

    /* Unmask signals */
    sigemptyset(&sigs);
    sigprocmask(SIG_SETMASK, &sigs, NULL);

    /* Determine length of buffer needed to hold the expanded command
     * string and allocate it. */
    len = strlen(command);
    file_len = strlen(file);
    cp = command;
    while (NULL != (cp = strchr(cp, '%'))) {
        ++cp;
        switch (*cp) {
          case '%':
            --len;
            break;
          case 's':
            len += file_len - 2;
            break;
          default:
            skAbortBadCase((int)(*cp));
        }
        ++cp;
    }
    expanded_cmd = (char*)malloc(len + 1);
    if (expanded_cmd == NULL) {
        WARNINGMSG("Unable to allocate memory to create command string");
        _exit(EXIT_FAILURE);
    }

    /* Copy command into buffer, handling %-expansions */
    cp = command;
    exp_cp = expanded_cmd;
    while (NULL != (sp = strchr(cp, '%'))) {
        /* copy text we just jumped over */
        strncpy(exp_cp, cp, sp - cp);
        exp_cp += (sp - cp);
        /* handle conversion */
        switch (*(sp+1)) {
          case '%':
            *exp_cp = '%';
            ++exp_cp;
            break;
          case 's':
            strcpy(exp_cp, file);
            exp_cp += file_len;
            break;
          default:
            skAbortBadCase((int)(*(sp+1)));
        }
        cp = sp + 2;
        assert(len >= (size_t)(exp_cp - expanded_cmd));
    }
    strcpy(exp_cp, cp);
    expanded_cmd[len] = '\0';

    DEBUGMSG("Invoking command: %s", expanded_cmd);

    /* Execute the command */
    if (execl("/bin/sh", "sh", "-c", expanded_cmd, (char*)NULL)
        == -1)
    {
        ERRMSG(("Error invoking /bin/sh: %s"),
               strerror(errno));
        _exit(EXIT_FAILURE);
    }

    /* Should never get here. */
    skAbort();
}


/* set the error directory */
void
errorDirectorySetPath(
    const char         *directory)
{
    error_directory = directory;
}


/* check whether the error directory is set */
int
errorDirectoryIsSet(
    void)
{
    return (NULL != error_directory);
}


/* move filename into the error-directory.  return 1 if
 * error-directory is not set. */
int
errorDirectoryInsertFile(
    const char         *filename)
{
    const char *c;
    char path[PATH_MAX];
    int rv;

    if (NULL == error_directory) {
        return 1;
    }

    /* basename */
    c = strrchr(filename, '/');
    if (c) {
        c++;
    } else {
        c = filename;
    }

    /* create destination path */
    rv = snprintf(path, sizeof(path), "%s/%s",
                  error_directory, c);
    if (sizeof(path) <= (size_t)rv) {
        WARNINGMSG("Error directory path too long");
        return -1;
    }

    /* move file */
    rv = skMoveFile(filename, path);
    if (rv != 0) {
        ERRMSG("Could not move '%s' to '%s': %s",
               filename, path, strerror(rv));
        return -1;
    }

    return 0;
}


/* do not create subdirectories of the archive_directory */
void
archiveDirectorySetFlat(
    void)
{
    archive_flat = 1;
}


/* set the archive directory */
void
archiveDirectorySetPath(
    const char         *directory)
{
    archive_directory = directory;
}


/* check whether the archive directory is set */
int
archiveDirectoryIsSet(
    void)
{
    if (NULL != archive_directory) {
        return 1;
    }
    if (NULL == post_archive_command) {
        return 0;
    }
    return -1;
}


/* set the command to run after archiving a file */
void
archiveDirectorySetPostCommand(
    const char         *command)
{
    post_archive_command = command;
}


/* do not remove files when archive-directory is NULL */
void
archiveDirectorySetNoRemove(
    void)
{
    remove_when_archive_null = 0;
}


/* put 'filename' into 'archive_dir/sub_dir/filename', or if
 * archive_dir is NULL, remove filename and return 1.  also call
 * runCommand() if post_archive_command is set. */
int
archiveDirectoryInsertOrRemove(
    const char         *filename,
    const char         *sub_directory)
{
    const char *c;
    char       *s;
    char        path[PATH_MAX];
    time_t      curtime;
    struct tm   ctm;
    int         rv;

    if (NULL == archive_directory) {
        if (remove_when_archive_null) {
            /* Remove file */
            if (unlink(filename) == -1) {
                WARNINGMSG("Could not remove '%s': %s",
                           filename, strerror(errno));
            }
        }
        return 1;
    }

    /* basename */
    c = strrchr(filename, '/');
    if (c) {
        c++;
    } else {
        c = filename;
    }

    if (archive_flat) {
        /* file goes directly into the archive_directory */
        rv = snprintf(path, sizeof(path), "%s/%s",
                      archive_directory, c);
        if (sizeof(path) <= (size_t)rv) {
            WARNINGMSG("Archive directory path too long");
            return -1;
        }
    } else {
        /* create destination path */
        if (sub_directory) {
            rv = snprintf(path, sizeof(path), "%s/%s/%s",
                          archive_directory, sub_directory, c);
        } else {
            /* create archive path based on current UTC time:
             * ARCHIVE/YEAR/MONTH/DAY/HOUR/FILE */
            curtime = time(NULL);
            gmtime_r(&curtime, &ctm);
            rv = snprintf(path, sizeof(path), "%s/%04d/%02d/%02d/%02d/%s",
                          archive_directory, (ctm.tm_year + 1900),
                          (ctm.tm_mon + 1), ctm.tm_mday, ctm.tm_hour, c);
        }

        if (sizeof(path) <= (size_t)rv) {
            WARNINGMSG("Archive directory path too long");
            return -1;
        }

        /* make the directory */
        s = strrchr(path, '/');
        *s = '\0';

        rv = skMakeDir(path);
        if (rv != 0) {
            ERRMSG("Could not create directory '%s': %s",
                   path, strerror(errno));
            return -1;
        }
        *s = '/';
    }

    /* move file */
    rv = skMoveFile(filename, path);
    if (rv != 0) {
        ERRMSG("Could not move '%s' to '%s': %s",
               filename, path, strerror(rv));
        return -1;
    }

    if (post_archive_command) {
        runCommand(post_archive_command, path);
    }

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
