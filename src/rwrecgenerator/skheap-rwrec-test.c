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
**  skheap-rwrec-test.c
**
**     Test the skheap-rwrec code.
**
**  Michael Duggan
**  May 2011
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skheap-rwrec-test.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skstream.h>
#include <silk/utils.h>
#include "skheap-rwrec.h"

int main(int UNUSED(argc), char **argv)
{
#define DATA_SIZE 30
    skrwrecheap_t *heap;
    int data[DATA_SIZE] = {
        201, 34, 202, 56, 203,  2,
        204, 65, 205,  3, 206,  5,
        207,  8, 208, 74, 209, 32,
        210, 78, 211, 79, 212, 80,
        213,  5, 214,  5, 215,  1};
    rwRec recs[DATA_SIZE];
    const rwRec *last, *cur;
    int rv;
    int i;

    /* register the application */
    skAppRegister(argv[0]);

    memset(recs, 0, sizeof(recs));
    for (i = 0; i < DATA_SIZE; i++) {
        rwRecSetElapsed(&recs[i], data[i]);
        rwRecSetProto(&recs[i], data[i]);
    }

    heap = skRwrecHeapCreate(1);
    if (heap == NULL) {
        skAppPrintErr("Failed to create heap");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < DATA_SIZE; i++) {
        rv = skRwrecHeapInsert(heap, &recs[i]);
        if (rv != 0) {
            skAppPrintErr("Failed to insert element");
            exit(EXIT_FAILURE);
        }
    }

    last = skRwrecHeapPeek(heap);
    if (last == NULL) {
        skAppPrintErr("Heap unexpectedly empty");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < DATA_SIZE; i++) {
        cur = skRwrecHeapPop(heap);
        if (cur == NULL) {
            skAppPrintErr("Heap unexpectedly empty");
            exit(EXIT_FAILURE);
        }
        if (i != 0 && cur == last) {
            skAppPrintErr("Unexpected duplicate");
            exit(EXIT_FAILURE);
        }
        if (rwRecGetProto(cur) < rwRecGetProto(last)) {
            skAppPrintErr("Incorrect ordering");
            exit(EXIT_FAILURE);
        }
        printf("%" PRIu32 "\n", rwRecGetProto(cur));
        last = cur;
    }
    if (skRwrecHeapPeek(heap) != NULL) {
        skAppPrintErr("Heap unexpectedly non-empty");
        exit(EXIT_FAILURE);
    }
    if (skRwrecHeapPop(heap) != NULL) {
        skAppPrintErr("Heap unexpectedly non-empty");
        exit(EXIT_FAILURE);
    }

    printf("Success!\n");

    return EXIT_SUCCESS;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
