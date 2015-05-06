/*
** Copyright (C) 2007-2015 by Carnegie Mellon University.
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

#include <silk/silk.h>

RCSIDENT("$SiLK: rwipautils.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwipa.h"


#define IPA_CONFIG_FILE "silk-ipa.conf"
#define IPA_CONFIG_LINE_LENGTH 1024


char *
get_ipa_config(
    void)
{
    skstream_t *conf_stream = NULL;
    char filename[PATH_MAX];
    char line[IPA_CONFIG_LINE_LENGTH];
    char *ipa_url = NULL;
    int rv;

    /* Read in the data file */
    if (NULL == skFindFile(IPA_CONFIG_FILE, filename, sizeof(filename), 1)) {
        skAppPrintErr("Could not locate config file '%s'.",
                      IPA_CONFIG_FILE);
        return NULL;
    }

    /* open input */
    if ((rv = skStreamCreate(&conf_stream, SK_IO_READ, SK_CONTENT_TEXT))
        || (rv = skStreamBind(conf_stream, filename))
        || (rv = skStreamSetCommentStart(conf_stream, "#"))
        || (rv = skStreamOpen(conf_stream)))
    {
        skStreamPrintLastErr(conf_stream, rv, &skAppPrintErr);
        skStreamDestroy(&conf_stream);
        exit(EXIT_FAILURE);
    }
    while (skStreamGetLine(conf_stream, line, sizeof(line), NULL)
           == SKSTREAM_OK)
    {
        /* FIXME: smarter config file reading, please */
        if (strlen(line) > 0) {
            ipa_url = strdup(line);
            break;
        }
    }

    skStreamDestroy(&conf_stream);
    /* Should be free()d by the caller */
    return ipa_url;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
