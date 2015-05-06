/*
** Copyright (C) 2011-2015 by Carnegie Mellon University.
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
**  skcygwin.c
**
**    Support for getting the default SiLK Data Root directory from
**    the Windows Registry
**
**    July 2011
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skcygwin.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#ifdef __CYGWIN__
/*
 *    Microsoft says to define _WIN32_WINNT to get the right windows
 *    API version, but under Cygwin, you need to define WINVER - which
 *    are related, (but not the same?).  They are believed to be the
 *    same under Cygwin
 */
#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#include <windows.h>
#include "skcygwin.h"

/* LOCAL DEFINES AND TYPEDEFS */

/* used for converting a Windows path to a Cygwin path */
#define CYGWIN_PATH_PREFIX              "/cygdrive/"

/* path to return when the registry key does not exist */
#define SILK_DEFAULT_CYGWIN_DATA_DIR    CYGWIN_PATH_PREFIX "c/data"


/* sizes for buffers when querying the registry */
#define BUFFER_SIZE_INITIAL     8192
#define BUFFER_SIZE_INCREMENT   4096


/* FUNCTION DEFINITIONS */

/**
 *  windowsToCygwinPath
 *
 *    converts a "normal" windows path "C:\Windows\" into an
 *    equivalent cygwin path "/cygdrive/c/Windows/"
 *
 *    @param buf a character buffer to be filled with the cygwin path
 *
 *    @param bufsize the size of the buffer
 *
 *    @param win_path a character string containing a windows path
 *
 *    @return a pointer to buf on success; on error this function
 *    returns NULL
 */
static char *
windowsToCygwinPath(
    char               *buf,
    size_t              bufsize,
    const char         *win_path)
{
    int len;
    char *cp;
    char *ep;

    assert(buf);
    assert(win_path);

    len = snprintf(buf, bufsize, "%s%s",
                   CYGWIN_PATH_PREFIX, win_path);
    if ((size_t)len > bufsize) {
        /* registry value too large to fit in 'buf' */
        return NULL;
    }

    /* skip over the cygwin prefix so we're at the start of the windows path */
    cp = buf + strlen(CYGWIN_PATH_PREFIX);

    /* try to find the drive prefix, e.g. c: or d: or z: */
    ep = strchr(cp, ':');
    if (NULL == ep) {
        /* it's a relative path, run with it? */
        return NULL;
    }

    /* downcase drive letter */
    while (cp < ep) {
        *cp = tolower((int)*cp);
        ++cp;
    }

    /* change ':' to '/' */
    *cp = '/';

    /* convert remaining '\' to '/' */
    while ((cp = strchr(cp, '\\')) != NULL) {
        *cp = '/';
    }

    return buf;
}



/**
 *  skCygwinGetDataRootDir
 *
 *    Gets the data directory defined at INSTALLATION time on Windows
 *    machines via reading the windows registry.  Caches the result in
 *    a file static.
 *
 *    @param buf a character buffer to be filled with the directory
 *
 *    @param bufsize the size of the buffer
 *
 *    @return a pointer to buf is returned on success; on error this
 *    function returns NULL
 *
 *    @note must call skCygwinClean to get rid of the memory for the
 *    cached result
 */
const char *
skCygwinGetDataRootDir(
    char               *buf,
    size_t              bufsize)
{
    char *data_buffer;
    char *rv;
    DWORD buffer_size;
    DWORD rc;

    buffer_size = BUFFER_SIZE_INITIAL;
    data_buffer = (char*)malloc(sizeof(char) * buffer_size);
    if (NULL == data_buffer) {
        /* error couldn't allocate memory */
        return NULL;
    }

    /* keeping asking the registry for the value until we have a
     * buffer big enough to hold it */
    do {
        rc = RegQueryValueEx(HKEY_LOCAL_MACHINE,
                             SILK_WINDOWSREG_DATA_DIR_KEY_PATH, NULL,
                             NULL, (PVOID)data_buffer, &buffer_size);
        if (ERROR_MORE_DATA == rc) {
            /* buffer is not large enough; grow it.  also, since
             * 'buffer_size' contains the required size, I'm not sure
             * why we add additional space onto it.... */
            char *oldbuf = data_buffer;
            buffer_size += BUFFER_SIZE_INCREMENT;
            data_buffer = (char*)realloc(data_buffer, buffer_size);
            if (NULL == data_buffer) {
                /* realloc failed */
                free(oldbuf);
                return NULL;
            }
        }
    } while (ERROR_MORE_DATA == rc);

    if (ERROR_SUCCESS != rc) {
        free(data_buffer);
        return NULL;
    }

    if (0 == buffer_size) {
        /* What makes sense to do when we can't find the registry
         * entry?  In this case, we return a "sane" default for
         * windows. */
        free(data_buffer);
        strncpy(buf, SILK_DEFAULT_CYGWIN_DATA_DIR, bufsize);
        if ('\0' != buf[bufsize-1]) {
            return NULL;
        }
        return buf;
    }

    rv = windowsToCygwinPath(buf, bufsize, data_buffer);
    free(data_buffer);
    return rv;
}


#ifdef STANDALONE_TEST_HARNESS
/*
 */
int main(int UNUSED(argc), char UNUSED(**argv))
{
    char buf[PATH_MAX];
    char *b;

    b = skCygwinGetDataRootDir(buf, sizeof(buf));

    printf("registry string is\n    %s => \"%s\"\n",
           NETSA_WINDOWSREG_REGHOME, (b ? b : "NULL"));
    return 0;
}

#endif  /* STANDALONE_TEST_HARNESS */
#endif  /* __CYGWIN__ */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
