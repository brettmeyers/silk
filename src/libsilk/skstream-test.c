/*
** Copyright (C) 2007-2014 by Carnegie Mellon University.
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
**  skstream-test.c
**
**    Test the binary capability of the skstream functions.
**
**  Mark Thomas
**  June 2007
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skstream-test.c 1c87fe1a22f7 2014-01-06 15:24:30Z mthomas $");

#include <silk/sksite.h>
#include <silk/utils.h>
#include <silk/skstream.h>


int main(int argc, char **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);
    uint8_t buffer[1 << 15];
    skstream_t *s_in = NULL;
    skstream_t *s_out = NULL;
    int rv;
    ssize_t got;
    ssize_t put;
    off_t len;
    int i;

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <dest>\n", skAppName());
        exit(EXIT_FAILURE);
    }

    if ((rv = (skStreamCreate(&s_in, SK_IO_READ, SK_CONTENT_OTHERBINARY)))
        || (rv = skStreamBind(s_in, argv[1]))
        || (rv = skStreamOpen(s_in)))
    {
        skStreamPrintLastErr(s_in, rv, &skAppPrintErr);
        goto END;
    }

    if ((rv = (skStreamCreate(&s_out, SK_IO_WRITE, SK_CONTENT_OTHERBINARY)))
        || (rv = skStreamBind(s_out, argv[2]))
        || (rv = skStreamOpen(s_out)))
    {
        skStreamPrintLastErr(s_out, rv, &skAppPrintErr);
        goto END;
    }

    do {
        got = skStreamRead(s_in, buffer, sizeof(buffer));
        if (got > 0) {
            put = skStreamWrite(s_out, buffer, got);
            if (got != put) {
                if (put < 0) {
                    skStreamPrintLastErr(s_out, put, &skAppPrintErr);
                } else {
                    skAppPrintErr("Warning: read %ld bytes and wrote %ld bytes",
                                  (long)got, (long)put);
                }
            }
        }
    } while (got > 0);

    if (got < 0) {
        skStreamPrintLastErr(s_in, got, &skAppPrintErr);
    }

    if (skStreamIsSeekable(s_out)) {
        /* get the current position in the output, write the buffer to
         * the output a couple of times, then truncate the output to
         * the current position */
        if ((rv = skStreamFlush(s_out))
            || ((len = skStreamTell(s_out)) == (off_t)-1))
        {
            skStreamPrintLastErr(s_out, rv, &skAppPrintErr);
            goto END;
        }

        memset(buffer, 0x55, sizeof(buffer));
        got = sizeof(buffer);

        for (i = 0; i < 2; ++i) {
            put = skStreamWrite(s_out, buffer, got);
            if (got != put) {
                skStreamPrintLastErr(s_out, rv, &skAppPrintErr);
                skAppPrintErr("Warning: have %ld bytes and wrote %ld bytes",
                              (long)got, (long)put);
            }
        }

        rv = skStreamTruncate(s_out, len);
        if (rv) {
            skStreamPrintLastErr(s_out, rv, &skAppPrintErr);
        }
    }

  END:
    rv = skStreamDestroy(&s_in);
    if (rv) {
        skStreamPrintLastErr(s_in, rv, &skAppPrintErr);
    }
    rv = skStreamClose(s_out);
    if (rv) {
        skStreamPrintLastErr(s_out, rv, &skAppPrintErr);
    }
    rv = skStreamDestroy(&s_out);
    if (rv) {
        skStreamPrintLastErr(s_out, rv, &skAppPrintErr);
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
