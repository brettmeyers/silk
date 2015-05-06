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
**  skcygwin.h
**
**    Support for getting the default SiLK Data Root directory from
**    the Windows Registry
**
**    July 2011
**
*/
#ifndef _SKCYGWIN_H
#define _SKCYGWIN_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKCYGWIN_H, "$SiLK: skcygwin.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#ifdef __CYGWIN__

/* registry location/key definitions */
#ifndef  NETSA_WINDOWSREG_REGHOME
#ifndef  SK_CYGWIN_TESTING

#define NETSA_WINDOWSREG_REGHOME        "Software\\CERT\\NetSATools"
#define SILK_WINDOWSREG_DATA_DIR_KEY    "SilkDataDir"
#define SILK_WINDOWSREG_DATA_DIR_KEY_PATH                               \
    (NETSA_WINDOWSREG_REGHOME "\\" SILK_WINDOWSREG_DATA_DIR_KEY)

#else  /* SK_CYGWIN_TESTING */

/* values for testing the code without having to modify registry */

#define NETSA_WINDOWSREG_REGHOME                        \
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
#define SILK_WINDOWSREG_DATA_DIR_KEY  "SystemRoot"
#define SILK_WINDOWSREG_DATA_DIR_KEY_PATH                               \
    (NETSA_WINDOWSREG_REGHOME "\\" SILK_WINDOWSREG_DATA_DIR_KEY)

#endif  /* SK_CYGWIN_TESTING */
#endif  /* NETSA_WINDOWSREG_REGHOME */


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
    size_t              bufsize);

#endif /* __CYGWIN__ */
#ifdef __cplusplus
}
#endif
#endif /* _SKCYGWIN_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
