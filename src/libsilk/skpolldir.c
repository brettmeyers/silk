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

/*
**  Library for polling directories for new files
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skpolldir.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skdeque.h>
#include <silk/redblack.h>
#include <silk/skvector.h>
#include <silk/skdllist.h>
#include <silk/skpolldir.h>
#include <silk/sktimer.h>
#include <silk/sklog.h>
#include <silk/utils.h>

#ifdef SKPOLLDIR_TRACE_LEVEL
#define TRACEMSG_LEVEL SKPOLLDIR_TRACE_LEVEL
#endif
#define TRACEMSG(tm_lvl, tm_msg)  TRACEMSG_TO_TRACEMSGLVL(tm_lvl, tm_msg)
#include <silk/sktracemsg.h>


/* LOCAL DEFINES AND TYPEDEFS */

/*
 *    Define SKPOLLDIR_USE_INODE to a non-zero value to include the
 *    inode in the key for the red-black tree.
 *
 *    The inode was added in an attempt to speed access in the
 *    red-black tree, but in testing on Linux RHEL 5.10 in a directory
 *    of 250,000, it only shaved about a tenth of a second from the
 *    times for directory-scan.
 */
/* #define SKPOLLDIR_USE_INODE 1 */
#ifndef SKPOLLDIR_USE_INODE
#define SKPOLLDIR_USE_INODE 0
#endif

/*
 *    Define SKPOLLDIR_TIMING to a non-zero value to time the
 *    directory-scan and mark-and-sweep operations.  Messages are
 *    written at --log-level=debug.
 */
/* #define SKPOLLDIR_TIMING 1 */
#ifndef SKPOLLDIR_TIMING
#define SKPOLLDIR_TIMING 0
#endif


/* The type of skPollDir_t objects */
typedef struct sk_polldir_st {
    /** name of directory to poll */
    char            *directory;
    /** the position of the basename in the complete file paths
     * returned to caller */
    size_t           filename_offset;
    /** a red-black is used to keep track of files that exist in the
     * directoy.  it holds pd_dirent_t objects */
    struct rbtree   *tree;
    /** once a file in the dirctory is quiescent, it is added to this
     * queue until requested by the caller.  the queue holds
     * pd_qentry_t objects */
    skDeque_t        queue;
    /** when the timer fires, it is time to poll the directory again */
    skTimer_t        timer;
    /** this is max number of seconds the NextFile() function will
     * wait for a file.  if 0, wait forever. */
    uint32_t         wait_next_file;
    /** current error.  this can be set while polling the directory or
     * when skPollDirStop() is called.  */
    skPollDirErr_t   error;
    /* if there is a system error during polling, this holds the
     * errno */
    int              sys_errno;
    unsigned         stopped : 1;
} sk_polldir_t;


/* A file entry; these are stored in the red-black tree for all files
 * that exist in the directory.  */
typedef struct pd_dirent_st {
    /* basename of the file */
    char        *name;
    /* size of the file */
    off_t        size;
#if SKPOLLDIR_USE_INODE
    /* the inode of the file */
    ino_t        inode;
#endif
    /* true if the file existed during the most recent scan */
    unsigned int seen   : 1;
    /* true if the file has been added to the deque */
    unsigned int queued : 1;
} pd_dirent_t;


/* The file entry for items stored in the deque. */
typedef struct pd_qentry_st {
    /* complete path to the file */
    char       *path;
    /* basename of the file; points into 'path' */
    char       *name;
} pd_qentry_t;



/* LOCAL VARIABLE DEFINITIONS */

/* variables and mutex to handle maximum file handle usage */
static pthread_mutex_t skp_fh_sem_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  skp_fh_sem_cond  = PTHREAD_COND_INITIALIZER;
static int skp_fh_max = SKPOLLDIR_DEFAULT_MAX_FILEHANDLES;
static int skp_fh_left = SKPOLLDIR_DEFAULT_MAX_FILEHANDLES;


/* FUNCTION DEFINITIONS */


/* Acquire the file handle semaphore */
static void
skp_fh_sem_aquire(
    void)
{
    pthread_mutex_lock(&skp_fh_sem_mutex);
    while (skp_fh_left <= 0) {
        pthread_cond_wait(&skp_fh_sem_cond, &skp_fh_sem_mutex);
    }
    --skp_fh_left;
    pthread_mutex_unlock(&skp_fh_sem_mutex);
}

/* release the file handle semaphore */
static void
skp_fh_sem_release(
    void)
{
    pthread_mutex_lock(&skp_fh_sem_mutex);
    ++skp_fh_left;
    pthread_cond_signal(&skp_fh_sem_cond);
    pthread_mutex_unlock(&skp_fh_sem_mutex);
}

/* Change the maximum number of filehandles we can use. */
int
skPollDirSetMaximumFileHandles(
    int                 max_fh)
{
    if (max_fh < 1) {
        return -1;
    }

    pthread_mutex_lock(&skp_fh_sem_mutex);
    skp_fh_left += max_fh - skp_fh_max;
    skp_fh_max = max_fh;
    pthread_mutex_unlock(&skp_fh_sem_mutex);

    return 0;
}

/* Comparison function for pd_dirent_t items (used by red black tree) */
static int
compare(
    const void         *va,
    const void         *vb,
    const void  UNUSED(*unused))
{
    const pd_dirent_t *a = (const pd_dirent_t *)va;
    const pd_dirent_t *b = (const pd_dirent_t *)vb;

#if SKPOLLDIR_USE_INODE
    if (a->inode < b->inode) {
        return -1;
    }
    if (a->inode > b->inode) {
        return 1;
    }
#endif

    return strcmp(a->name, b->name);
}


/* Walk through the tree removing files that were not seen.  Marks
 * files that were not removed as unseen in preperation for the next
 * pass. */
static void
remove_unseen(
    sk_polldir_t       *pd)
{
    RBLIST *list = NULL;
    pd_dirent_t *x;
    sk_vector_t *dellist = NULL;
    int rv;
    size_t i;
#if SKPOLLDIR_TIMING
    clock_t t1, t2 = 0, t3 = 0;
#endif

    TRACEMSG(1, ("polldir %p: Starting mark and sweep", pd));
#if SKPOLLDIR_TIMING
    t1 = clock();
#endif

    dellist = skVectorNew(sizeof(pd_dirent_t *));

    list = rbopenlist(pd->tree);
    if (list == NULL) {
        pd->error = PDERR_MEMORY;
        goto end;
    }

    /* Loop through all files */
    while ((x = (pd_dirent_t *)rbreadlist(list))) {
        if (x->seen) {
            /* Seen.  Reset to zero for next pass. */
            x->seen = 0;
        } else {
            TRACEMSG(2, (("polldir %p: File '%s' was not noticed."
                          " Removing from consideration."),
                         pd, x->name));

            /* Not seen.  Add to delete list. */
            rv = skVectorAppendValue(dellist, &x);
            if (-1 == rv) {
                pd->error = PDERR_MEMORY;
                goto end;
            }
        }
    }

#if SKPOLLDIR_TIMING
    t2 = clock();
#endif

    /* Remove items in the delete list */
    for (i = 0; i < skVectorGetCount(dellist); i++) {
        rv = skVectorGetValue(&x, dellist, i);
        assert(0 == rv);
        rbdelete(x, pd->tree);
        free(x->name);
        free(x);
    }

  end:
#if SKPOLLDIR_TIMING
    t3 = clock();
#endif
    TRACEMSG(1, (("polldir %p: Finished mark and sweep."
                  " Removed %" SK_PRIuZ " nodes"),
                 pd, skVectorGetCount(dellist)));

#if SKPOLLDIR_TIMING
    DEBUGMSG(("polldir %p: Mark and sweep required %f seconds:"
              " rbtree scan %f secs; node delete %f secs"),
             pd, ((double)(t3 - t1) / CLOCKS_PER_SEC),
             ((double)(t2 - t1) / CLOCKS_PER_SEC),
             ((double)(t3 - t2) / CLOCKS_PER_SEC));
#endif
    if (list) {
        rbcloselist(list);
    }

    if (dellist) {
        skVectorDestroy(dellist);
    }
}


/* Free all pd_dirent_t structures in the tree. */
static void
free_tree_nodes(
    sk_polldir_t       *pd)
{
    RBLIST *list;
    pd_dirent_t *x;

    list = rbopenlist(pd->tree);
    if (list == NULL) {
        pd->error = PDERR_MEMORY;
        goto end;
    }

    while ((x = (pd_dirent_t *)rbreadlist(list))) {
        free(x->name);
        free(x);
    }

  end:
    rbcloselist(list);
}

/* Actually poll the directory.  This is the function that the
 * skTimer_t will call. */
static skTimerRepeat_t
pollDir(
    void               *vpd)
{
    sk_polldir_t *pd = (sk_polldir_t *)vpd;
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    int rv;
    char path[PATH_MAX];
    pd_dirent_t *node, *found;
    pd_qentry_t *item;
    skDQErr_t err;
#if SKPOLLDIR_TIMING
    clock_t t1, t2 = 0;
#endif

    skp_fh_sem_aquire();
    TRACEMSG(1, ("polldir %p: Starting directory scan of '%s'",
                 pd, pd->directory));

#if SKPOLLDIR_TIMING
    t1 = clock();
#endif

    node = NULL;
    dir = opendir(pd->directory);
    if (NULL == dir) {
        pd->error = PDERR_SYSTEM;
        pd->sys_errno = errno;
        goto cleanup_and_exit;
    }

    rv = snprintf(path, sizeof(path), "%s/", pd->directory);
    assert((size_t)rv == pd->filename_offset);

    /* Loop over all files in the directory */
    while (PDERR_NONE == pd->error &&
           (entry = readdir(dir)))
    {

        /* Ignore dot files. */
        if (entry->d_name[0] == '.') {
            TRACEMSG(2, ("polldir %p: File '%s' was ignored (dotfile)",
                         pd, entry->d_name));
            continue;
        }

        assert(strlen(entry->d_name) + pd->filename_offset < sizeof(path));
        strcpy(path + pd->filename_offset, entry->d_name);

        /* Ignore files that are empty, or that aren't either regular
           files or symbolic links. */
        rv = stat(path, &st);
        if (rv == -1
            || !(st.st_mode & (S_IFREG | S_IFLNK))
            || (0 == st.st_size))
        {
            TRACEMSG(2, ("polldir %p: File '%s' was ignored (%s)",
                         pd, entry->d_name,
                         ((0 == st.st_size) ? "zero-bytes" : "non-file")));
            continue;
        }

        /* Allocate an entry, if one doesn't already exist. */
        if (node == NULL) {
            node = (pd_dirent_t*)malloc(sizeof(pd_dirent_t));
            if (NULL == node) {
                TRACEMSG(1, ("polldir %p: Node allocation error for '%s'",
                             pd, entry->d_name));
                pd->error = PDERR_MEMORY;
                continue;
            }
            node->queued = 0;
        }

        node->name = entry->d_name;
#if SKPOLLDIR_USE_INODE
        node->inode = st.st_ino;
#endif

        /* Find or insert the entry into the tree.  */
        found = (pd_dirent_t *)rbsearch(node, pd->tree);

        if (found == node) {
            /* New node has been added to the tree */
            TRACEMSG(2, (("polldir %p: File '%s' noticed with size %" PRId64
                          " for the first time"),
                         pd, node->name, st.st_size));

            found->size = st.st_size;

            /* Make the name persistent */
            found->name = strdup(found->name);
            if (NULL == found->name) {
                TRACEMSG(1, ("polldir %p: Name allocation error for '%s'",
                             pd, node->name));
                pd->error = PDERR_MEMORY;
                continue;
            }

            /* Allocate a new node on the next run through the loop. */
            node = NULL;

        } else if (!found->queued) {
            /* Node already existed but not yet queued. */
            if (st.st_size != found->size) {
                /* file size still changing */
                TRACEMSG(2, (("polldir %p: File '%s' was noticed with size %"
                              PRId64 "; different from previous %" PRId64),
                            pd, node->name, st.st_size, found->size));
                found->size = st.st_size;
            } else {
                /* Size has stabalized, add to queue */
                TRACEMSG(2, (("polldir %p: File '%s' was noticed with size %"
                              PRId64 "; size is stable.  Queuing"),
                             pd, node->name, st.st_size));
                found->queued = 1;

                item = (pd_qentry_t *)malloc(sizeof(pd_qentry_t));
                if (NULL == item) {
                    TRACEMSG(1, ("polldir %p: Entry allocation error for '%s'",
                                 pd, node->name));
                    pd->error = PDERR_MEMORY;
                    continue;
                }
                item->path = strdup(path);
                if (NULL == item->path) {
                    TRACEMSG(1, ("polldir %p: Path allocation error for '%s'",
                                 pd, node->name));
                    free(item);
                    pd->error = PDERR_MEMORY;
                    continue;
                }
                item->name = item->path + pd->filename_offset;

                err = skDequePushFront(pd->queue, item);
                if (err != SKDQ_SUCCESS) {
                    TRACEMSG(1, ("polldir %p: Deque allocation error for '%s'",
                                 pd, node->name));
                    free(item->path);
                    free(item);
                    pd->error = PDERR_MEMORY;
                    continue;
                }
            }
#ifdef SKPOLLDIR_TRACE_LEVEL
        } else {
            TRACEMSG(2, (("polldir %p: File '%s' with size %" PRId64
                          "; prevously queued with size %" PRId64),
                         pd, node->name, st.st_size, found->size));
#endif  /* ENABLE_TRACEMSG */
        }

        /* Mark this file as seen this time around */
        found->seen = 1;
    }
    closedir(dir);

  cleanup_and_exit:
#if SKPOLLDIR_TIMING
    t2 = clock();
#endif
    TRACEMSG(1, ("polldir %p: Finished directory scan of '%s'",
                 pd, pd->directory));
    skp_fh_sem_release();

#if SKPOLLDIR_TIMING
    DEBUGMSG("polldir %p: Directory scan required %f seconds",
             pd, ((double)(t2 - t1) / CLOCKS_PER_SEC));
#endif

    /* Free node, if left over. */
    if (node) {
        free(node);
    }

    /* Remove entries we did not see, and re-mark tree as unseen. */
    if (PDERR_NONE == pd->error) {
        remove_unseen(pd);
    }

    /* Repeat if no error */
    if (PDERR_NONE == pd->error) {
        return SK_TIMER_REPEAT;
    }

    skDequeUnblock(pd->queue);

    return SK_TIMER_END;
}


void
skPollDirStop(
    sk_polldir_t       *pd)
{
    assert(pd);

    pd->stopped = 1;

    TRACEMSG(1, ("polldir %p: Being told to stop", pd));

    /* Stop the timer */
    if (pd->timer) {
        skTimerDestroy(pd->timer);
        pd->timer = NULL;
    }

    /* Unblock the queue */
    pd->error = PDERR_STOPPED;
    skDequeUnblock(pd->queue);
}


/* Destroy a polldir object */
void
skPollDirDestroy(
    sk_polldir_t       *pd)
{
    pd_qentry_t *item;

    assert(pd);

    skPollDirStop(pd);

    TRACEMSG(1, ("polldir %p: Being destroyed", pd));

    /* Empty and destroy the tree */
    if (pd->tree) {
        free_tree_nodes(pd);
        rbdestroy(pd->tree);
        pd->tree = NULL;
    }

    /* Free the directory name */
    if (pd->directory) {
        free(pd->directory);
        pd->directory = NULL;
    }

    /* Empty and destoy the queue */
    if (pd->queue) {
        while (skDequePopFrontNB(pd->queue, (void **)&item) == SKDQ_SUCCESS) {
            free(item->path);
            free(item);
        }

        skDequeDestroy(pd->queue);
        pd->queue = NULL;
    }

    /* Finally, free the polldir object */
    free(pd);
}


/* Create a directory polling object. */
sk_polldir_t *
skPollDirCreate(
    const char         *directory,
    uint32_t            poll_interval)
{
    sk_polldir_t *pd;
    int           rv;

    assert(directory);

    if (!skDirExists(directory)) {
        return NULL;
    }

    pd = (sk_polldir_t*)calloc(1, sizeof(sk_polldir_t));
    if (NULL == pd) {
        return NULL;
    }

    pd->queue = skDequeCreate();
    if (NULL == pd->queue) {
        goto err;
    }
    pd->directory = strdup(directory);
    if (NULL == pd->directory) {
        goto err;
    }
    pd->filename_offset = strlen(directory) + 1;
    if (pd->filename_offset >= PATH_MAX) {
        goto err;
    }

    pd->tree = rbinit(compare, NULL);
    if (NULL == pd->tree) {
        goto err;
    }

    TRACEMSG(1, ("polldir %p: Created to scan '%s' every %" PRIu32 " seconds",
                 pd, directory, poll_interval));

    /* Initial population of tree */
    pollDir(pd);

    /* Start timer */
    rv = skTimerCreate(&pd->timer, poll_interval, pollDir, pd);
    if (0 != rv) {
        goto err;
    }

    return pd;

  err:
    /* Clean up after any errors */
    skPollDirDestroy(pd);
    return NULL;
}


/* Puts a file back on the queue */
skPollDirErr_t
skPollDirPutBackFile(
    sk_polldir_t       *pd,
    const char         *filename)
{
    pd_qentry_t *item;
    char path[PATH_MAX];
    skDQErr_t err;
    int rv;

    assert(pd);
    assert(filename);

    rv = snprintf(path, sizeof(path), "%s/%s", pd->directory, filename);
    if ((size_t)rv >= sizeof(path)) {
        return PDERR_MEMORY;
    }

    item = (pd_qentry_t *)malloc(sizeof(*item));
    if (NULL == item) {
        return PDERR_MEMORY;
    }

    item->path = strdup(path);
    if (NULL == item->path) {
        free(item);
        return PDERR_MEMORY;
    }

    item->name = item->path + pd->filename_offset;

    err = skDequePushFront(pd->queue, item);
    if (err != SKDQ_SUCCESS) {
        free(item->path);
        free(item);
        return PDERR_MEMORY;
    }

    return PDERR_NONE;
}


/* Get the next added entry to a directory. */
skPollDirErr_t
skPollDirGetNextFile(
    sk_polldir_t       *pd,
    char               *path,
    char              **filename)
{
    pd_qentry_t *item = NULL;
    skDQErr_t err;

    assert(pd);
    assert(path);

    for (;;) {
        item = NULL;
        if (pd->wait_next_file) {
            err = skDequePopBackTimed(pd->queue, (void **)&item,
                                      pd->wait_next_file);
        } else {
            err = skDequePopBack(pd->queue, (void **)&item);
        }
        TRACEMSG(2, ("polldir %p: Deque return value is %d", pd, (int)err));
        if (SKDQ_SUCCESS != err) {
            if (pd->error == PDERR_NONE) {
                if (err == SKDQ_TIMEDOUT) {
                    return PDERR_TIMEDOUT;
                }
                /* This should not happen */
                CRITMSG(("%s:%d Invalid error condition in polldir;"
                         " deque returned %d"),
                        __FILE__, __LINE__, err);
                skAbort();
            }
            if (item) {
                free(item->path);
                free(item);
            }
            if (pd->error == PDERR_SYSTEM) {
                errno = pd->sys_errno;
            }
            return pd->error;
        }

        assert(item->path);

        if (skFileExists(item->path)) {
            /* File exists, so return it. */
            assert(strlen(item->path) < PATH_MAX);
            strcpy(path, item->path);
            if (filename) {
                *filename = path + (item->name - item->path);
            }

            free(item->path);
            free(item);
            break;
        }

        TRACEMSG(2, ("polldir %p: File '%s' was deleted before it"
                     " could be delivered",
                     pd, item->name));

        free(item->path);
        free(item);
        item = NULL;
    }

    TRACEMSG(2, ("polldir %p: File '%s' was delivered", pd, item->name));

    return PDERR_NONE;
}


/* Get the directory being polled by a polldir object. */
const char *
skPollDirGetDir(
    sk_polldir_t       *pd)
{
    assert(pd);
    return pd->directory;
}


void
skPollDirSetFileTimeout(
    sk_polldir_t       *pd,
    uint32_t            timeout_seconds)
{
    assert(pd);
    pd->wait_next_file = timeout_seconds;
}


/* Return a string describing an error */
const char *
skPollDirStrError(
    skPollDirErr_t      err)
{
    switch (err) {
      case PDERR_NONE:
        return "No error";
      case PDERR_STOPPED:
        return "Polldir stopped";
      case PDERR_MEMORY:
        return "Memory allocation error";
      case PDERR_SYSTEM:
        return "System error";
      case PDERR_TIMEDOUT:
        return "Polldir timed out";
    }

    return "Invalid error identifier"; /* NOTREACHED */
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
