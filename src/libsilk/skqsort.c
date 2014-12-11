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
**   qsort()
**
**   A la Bentley and McIlroy in Software - Practice and Experience,
**   Vol. 23 (11) 1249-1265. Nov. 1993
**
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skqsort.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>


typedef long WORD;
#define W               sizeof(WORD)    /* must be a power of 2 */

/* The kind of swapping is given by a variable, swaptype, with value 0
 * for single-word swaps, 1 for general swapping by words, and 2 for
 * swapping by bytes.  */
#define SWAPINIT(a, es)                                                 \
    swaptype = ((((char*)a-(char*)0) | es) % W) ? 2 : (es > W) ? 1 : 0

#define exch(a, b, t)   (t = a, a = b, b = t)

/* choose between function call and in-line exchange */
#define swap(a, b)                                      \
    ((swaptype != 0)                                    \
     ? swapfunc((char*)(a), (char*)(b), es, swaptype)   \
     : (void)exch(*(WORD*)(a), *(WORD*)(b), t))

/* swap sequences of records */
#define vecswap(a, b, n)                                        \
    if (n > 0) swapfunc((char*)(a), (char*)(b), n, swaptype)

#define min(a, b)       a <= b ? a : b

/* return median value of 'a', 'b', and 'c'. */
static char *
med3(
    char               *a,
    char               *b,
    char               *c,
    int               (*cmp)(const void *, const void *, void *),
    void               *thunk)
{
    return ((cmp(a, b, thunk) < 0)
            ? ((cmp(b, c, thunk) < 0) ? b : ((cmp(a, c, thunk) < 0) ? c : a))
            : ((cmp(b, c, thunk) > 0) ? b : ((cmp(a, c, thunk) > 0) ? c : a)));
}


static void
swapfunc(
    char               *a,
    char               *b,
    size_t              n,
    int                 swaptype)
{
    if (swaptype <= 1) {
        WORD t;
        for ( ; n > 0; a += W, b += W, n -= W) {
            exch(*(WORD*)a, *(WORD*)b, t);
        }
    } else {
        char t;
        for ( ; n > 0; a += 1, b += 1, n -= 1) {
            exch(*a, *b, t);
        }
    }
}


void
skQSort_r(
    void               *a,
    size_t              n,
    size_t              es,
    int               (*cmp)(const void *, const void *, void *),
    void               *thunk)
{
    char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int r, swaptype;
    WORD t;
    size_t s;

  loop:
    SWAPINIT(a, es);
    if (n < 7) {
        /* use insertion sort on smallest arrays */
        for (pm = (char*)a + es; pm < (char*)a + n*es; pm += es)
            for (pl = pm; pl > (char*)a && cmp(pl-es, pl, thunk) > 0; pl -= es)
                swap(pl, pl-es);
        return;
    }

    /* determine the pivot, pm */
    pm = (char*)a + (n/2)*es;           /* small arrays middle element */
    if (n > 7) {
        pl = (char*)a;
        pn = (char*)a + (n - 1)*es;
        if (n > 40) {
            /* big arays.  Pseudomedian of 9 */
            s = (n / 8) * es;
            pl = med3(pl, pl + s, pl + 2 * s, cmp, thunk);
            pm = med3(pm - s, pm, pm + s, cmp, thunk);
            pn = med3(pn - 2 * s, pn - s, pn, cmp, thunk);
        }
        pm = med3(pl, pm, pn, cmp, thunk);      /* mid-size, med of 3 */
    }
    /* put pivot into position 0 */
    swap(a, pm);

    pa = pb = (char*)a + es;
    pc = pd = (char*)a + (n - 1) * es;
    for (;;) {
        while (pb <= pc && (r = cmp(pb, a, thunk)) <= 0) {
            if (r == 0) {
                swap(pa, pb);
                pa += es;
            }
            pb += es;
        }
        while (pc >= pb && (r = cmp(pc, a, thunk)) >= 0) {
            if (r == 0) {
                swap(pc, pd);
                pd -= es;
            }
            pc -= es;
        }
        if (pb > pc) {
            break;
        }
        swap(pb, pc);
        pb += es;
        pc -= es;
    }

    pn = (char*)a + n * es;
    s = min(pa - (char*)a, pb - pa);
    vecswap(a, pb - s, s);
    s = min((size_t)(pd - pc), pn - pd - es);
    vecswap(pb, pn - s, s);
    if ((s = pb - pa) > es) {
        skQSort_r(a, s/es, es, cmp, thunk);
    }
    if ((s = pd - pc) > es) {
        /* iterate rather than recurse */
        /* skQSort(pn - s, s/es, es, cmp, thunk); */
        a = pn - s;
        n = s / es;
        goto loop;
    }
}


void
skQSort(
    void               *a,
    size_t              n,
    size_t              es,
    int               (*cmp)(const void *, const void *))
{
    skQSort_r(a, n, es,(int (*)(const void *, const void *, void *))cmp, NULL);
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
