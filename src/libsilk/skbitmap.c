/*
** Copyright (C) 2005-2014 by Carnegie Mellon University.
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
**  skbitmap.c
**
**    Bitmap creatation and deletion.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skbitmap.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/*
 *    Return number of 32bit words needed to hold a bitmap with
 *    'num_bits' elements.  Need to add 1 if number of bits is not an
 *    even multiple of 32.
 */
#define BITMAP_GET_WORD_COUNT(num_bits)         \
    (((num_bits) >> 5) + !!((num_bits) & 0x1F))


/* FUNCTION DEFINITIONS */

/*
 *    Return the number of trailing zeros in 'v'.  Note: This function
 *    assumes 'v' is non-zero; it will return 31 if 'v' is 0.
 */
static uint8_t
bitmapCountTrailingZeros(
    uint32_t            v)
{
    uint8_t c = 1;
    if ((v & 0xffff) == 0) {
        v >>= 16;
        c += 16;
    }
    if ((v & 0xff) == 0) {
        v >>= 8;
        c += 8;
    }
    if ((v & 0xf) == 0) {
        v >>= 4;
        c += 4;
    }
    if ((v & 0x3) == 0) {
        v >>= 2;
        c += 2;
    }
    return c - (uint8_t)(v & 0x1);
}


int
skBitmapBind(
    sk_bitmap_t        *bitmap,
    uint32_t            num_bits,
    uint32_t           *bitarray,
    size_t              sizeof_bitarray)
{
    uint32_t word_count = BITMAP_GET_WORD_COUNT(num_bits);

    if ( !(bitmap && num_bits && bitarray && sizeof_bitarray)) {
        return -1;
    }

    if (sizeof_bitarray < (word_count * sizeof(uint32_t))) {
        return -1;
    }

    memset(bitarray, 0, sizeof_bitarray);
    bitmap->map = bitarray;
    bitmap->num_bits = num_bits;
    bitmap->count = 0;

    return 0;
}


int
skBitmapCreate(
    sk_bitmap_t       **bitmap_out,
    uint32_t            num_bits)
{
    uint32_t word_count = BITMAP_GET_WORD_COUNT(num_bits);

    assert(bitmap_out);

    if (num_bits == 0) {
        return -1;
    }

    *bitmap_out = (sk_bitmap_t*)calloc(1, sizeof(sk_bitmap_t));
    if (*bitmap_out == NULL) {
        return -1;
    }

    (*bitmap_out)->map = (uint32_t*)calloc(word_count, sizeof(uint32_t));
    if (NULL == (*bitmap_out)->map) {
        free(*bitmap_out);
        return -1;
    }

    (*bitmap_out)->num_bits = num_bits;

    return 0;
}


void
skBitmapDestroy(
    sk_bitmap_t       **bitmap)
{
    if (!bitmap || !*bitmap) {
        return;
    }

    free((*bitmap)->map);
    (*bitmap)->map = NULL;
    free(*bitmap);
    *bitmap = NULL;
}


void
skBitmapClearAllBits(
    sk_bitmap_t        *bitmap)
{
    uint32_t word_count;

    assert(bitmap);

    word_count = BITMAP_GET_WORD_COUNT(bitmap->num_bits);

    memset(bitmap->map, 0, word_count * sizeof(uint32_t));
    bitmap->count = 0;
}


void
skBitmapSetAllBits(
    sk_bitmap_t        *bitmap)
{
    uint32_t word_count;

    assert(bitmap);

    word_count = BITMAP_GET_WORD_COUNT(bitmap->num_bits);

    if ((bitmap->num_bits & 0x1F) != 0) {
        /* last word is not fully used. handle it specially so we
         * don't set bits that are not in use */
        --word_count;
        SET_MASKED_BITS(bitmap->map[word_count], UINT32_MAX,
                        0, (bitmap->num_bits & 0x1F));
    }
    memset(bitmap->map, 0xFF, word_count * sizeof(uint32_t));
    bitmap->count = bitmap->num_bits;
}


uint32_t
skBitmapGetSizeF(
    const sk_bitmap_t  *bitmap)
{
    assert(bitmap);

    return skBitmapGetSizeFast(bitmap);
}


uint32_t
skBitmapGetHighCountF(
    const sk_bitmap_t  *bitmap)
{
    uint32_t i;
    uint32_t count = 0;
    uint32_t bits;

    assert(bitmap);

    /* check that bitmap->count is corrrect */
    for (i=BITMAP_GET_WORD_COUNT(bitmap->num_bits)-1; i < (uint32_t)-1; --i) {
        BITS_IN_WORD32(&bits, bitmap->map[i]);
        count += bits;
    }
    assert(count == bitmap->count);

    return skBitmapGetHighCountFast(bitmap);
}


int
skBitmapGetBitF(
    const sk_bitmap_t  *bitmap,
    uint32_t            pos)
{
    assert(bitmap);
    assert(pos < bitmap->num_bits);

    return skBitmapGetBitFast(bitmap, pos);
}


void
skBitmapSetBitF(
    sk_bitmap_t        *bitmap,
    uint32_t            pos)
{
    assert(bitmap);
    assert(pos < bitmap->num_bits);

    skBitmapSetBitFast(bitmap, pos);
}


void
skBitmapClearBitF(
    sk_bitmap_t        *bitmap,
    uint32_t            pos)
{
    assert(bitmap);
    assert(pos < bitmap->num_bits);

    skBitmapClearBitFast(bitmap, pos);
}


void
skBitmapComplement(
    sk_bitmap_t        *bitmap)
{
    uint32_t i;
    uint32_t bits;

    assert(bitmap);

    bitmap->count = 0;

    i = BITMAP_GET_WORD_COUNT(bitmap->num_bits) - 1;

    if (bitmap->num_bits & 0x1F) {
        /* last word is not fully used.  handle it specially so we
         * don't complement bits that are not in use */
        bitmap->map[i] = GET_MASKED_BITS(~bitmap->map[i], 0,
                                         (bitmap->num_bits & 0x1F));
        BITS_IN_WORD32(&bits, bitmap->map[i]);
        bitmap->count += bits;
        --i;
    }
    for ( ; i < (uint32_t)-1; --i) {
        bitmap->map[i] = ~bitmap->map[i];
        BITS_IN_WORD32(&bits, bitmap->map[i]);
        bitmap->count += bits;
    }
}


int
skBitmapIntersection(
    sk_bitmap_t        *dest,
    const sk_bitmap_t  *src)
{
    uint32_t i;
    uint32_t bits;

    assert(dest);
    assert(src);

    if (dest->num_bits != src->num_bits) {
        return -1;
    }

    dest->count = 0;
    for (i = BITMAP_GET_WORD_COUNT(src->num_bits) - 1; i < (uint32_t)-1; --i) {
        dest->map[i] &= src->map[i];
        BITS_IN_WORD32(&bits, dest->map[i]);
        dest->count += bits;
    }
    return 0;
}


int
skBitmapUnion(
    sk_bitmap_t        *dest,
    const sk_bitmap_t  *src)
{
    uint32_t i;
    uint32_t bits;

    assert(dest);
    assert(src);

    if (dest->num_bits != src->num_bits) {
        return -1;
    }

    dest->count = 0;
    for (i = BITMAP_GET_WORD_COUNT(src->num_bits) - 1; i < (uint32_t)-1; --i) {
        dest->map[i] |= src->map[i];
        BITS_IN_WORD32(&bits, dest->map[i]);
        dest->count += bits;
    }
    return 0;
}


uint32_t
skBitmapCountConsecutive(
    const sk_bitmap_t  *bitmap,
    uint32_t            begin_pos,
    uint32_t            state)
{
    uint32_t count = 0;
    uint32_t value;
    uint32_t limit;
    uint32_t i;

    assert(bitmap);
    if (begin_pos >= bitmap->num_bits) {
        return UINT32_MAX;
    }

    i = _BMAP_INDEX(begin_pos);
    limit = _BMAP_INDEX(bitmap->num_bits - 1);

    if (i == limit) {
        /* look at a single word */
        value = GET_MASKED_BITS((state ? ~bitmap->map[i] : bitmap->map[i]),
                                begin_pos & 0x1F, bitmap->num_bits-begin_pos);
        if (value) {
            return bitmapCountTrailingZeros(value);
        }
        return bitmap->num_bits - begin_pos;
    }

    if (begin_pos & 0x1F) {
        /* first bit is not least significant bit */
        value = GET_MASKED_BITS((state ? ~(bitmap->map[i]) : bitmap->map[i]),
                                begin_pos & 0x1F, 32 - (begin_pos & 0x1F));
        if (value) {
            return bitmapCountTrailingZeros(value);
        }
        count += 32 - (begin_pos & 0x1F);
        ++i;
    }

    if ((bitmap->num_bits & 0x1F) == 0) {
        /* no need to handle final word specially */
        ++limit;
    }

    if (state) {
        for (; i < limit; ++i) {
            if (UINT32_MAX == bitmap->map[i]) {
                count += 32;
            } else {
                return count + bitmapCountTrailingZeros(~bitmap->map[i]);
            }
        }
    } else {
        for (; i < limit; ++i) {
            if (0 == bitmap->map[i]) {
                count += 32;
            } else {
                return count + bitmapCountTrailingZeros(bitmap->map[i]);
            }
        }
    }

    if (bitmap->num_bits & 0x1F) {
        /* handle final partially filled word */
        value = GET_MASKED_BITS((state ? ~(bitmap->map[i]) : bitmap->map[i]),
                                0, (bitmap->num_bits & 0x1F));
        if (value) {
            return count + bitmapCountTrailingZeros(value);
        }
        count += (bitmap->num_bits & 0x1F);
    }

    return count;
}


int
skBitmapRangeSet(
    sk_bitmap_t        *bitmap,
    uint32_t            begin_pos,
    uint32_t            end_pos)
{
    uint32_t prev;
    uint32_t bits;
    uint32_t i;

    assert(bitmap);
    if (begin_pos > end_pos || end_pos >= bitmap->num_bits) {
        return -1;
    }

    i = _BMAP_INDEX(begin_pos);
    if (i == _BMAP_INDEX(end_pos)) {
        /* range is in a single word */
        prev = bitmap->map[i];
        SET_MASKED_BITS(bitmap->map[i], UINT32_MAX,
                        begin_pos & 0x1F, 1 + end_pos - begin_pos);
        BITS_IN_WORD32(&bits, prev ^ bitmap->map[i]);
        bitmap->count += bits;
        return 0;
    }

    prev = bitmap->map[i];
    SET_MASKED_BITS(bitmap->map[i], UINT32_MAX, begin_pos & 0x1F,
                    32 - (begin_pos & 0x1F));
    BITS_IN_WORD32(&bits, prev ^ bitmap->map[i]);
    bitmap->count += bits;

    for (++i; i < _BMAP_INDEX(end_pos); ++i) {
        BITS_IN_WORD32(&bits, bitmap->map[i]);
        bitmap->count += 32 - bits;
        bitmap->map[i] = UINT32_MAX;
    }

    prev = bitmap->map[i];
    SET_MASKED_BITS(bitmap->map[i], UINT32_MAX, 0, 1 + (end_pos & 0x1F));
    BITS_IN_WORD32(&bits, prev ^ bitmap->map[i]);
    bitmap->count += bits;
    return 0;
}


int
skBitmapRangeClear(
    sk_bitmap_t        *bitmap,
    uint32_t            begin_pos,
    uint32_t            end_pos)
{
    uint32_t prev;
    uint32_t bits;
    uint32_t i;

    assert(bitmap);
    if (begin_pos > end_pos || end_pos >= bitmap->num_bits) {
        return -1;
    }

    i = _BMAP_INDEX(begin_pos);
    if (i == _BMAP_INDEX(end_pos)) {
        /* range is in a single word */
        prev = bitmap->map[i];
        SET_MASKED_BITS(bitmap->map[i], 0,
                        begin_pos & 0x1F, 1 + end_pos - begin_pos);
        BITS_IN_WORD32(&bits, prev ^ bitmap->map[i]);
        bitmap->count -= bits;
        return 0;
    }

    prev = bitmap->map[i];
    SET_MASKED_BITS(bitmap->map[i], 0, begin_pos & 0x1F,
                    32 - (begin_pos & 0x1F));
    BITS_IN_WORD32(&bits, prev ^ bitmap->map[i]);
    bitmap->count -= bits;

    for (++i; i < _BMAP_INDEX(end_pos); ++i) {
        BITS_IN_WORD32(&bits, bitmap->map[i]);
        bitmap->count -= bits;
        bitmap->map[i] = 0;
    }

    prev = bitmap->map[i];
    SET_MASKED_BITS(bitmap->map[i], 0, 0, 1 + (end_pos & 0x1F));
    BITS_IN_WORD32(&bits, prev ^ bitmap->map[i]);
    bitmap->count -= bits;
    return 0;
}


uint32_t
skBitmapRangeCountHigh(
    sk_bitmap_t        *bitmap,
    uint32_t            begin_pos,
    uint32_t            end_pos)
{
    uint32_t value;
    uint32_t bits1;
    uint32_t bits2;
    uint32_t i;

    assert(bitmap);
    if (begin_pos > end_pos || end_pos >= bitmap->num_bits) {
        return UINT32_MAX;
    }

    i = _BMAP_INDEX(begin_pos);
    if (i == _BMAP_INDEX(end_pos)) {
        /* range is in a single word */
        value = GET_MASKED_BITS(bitmap->map[i], begin_pos & 0x1F,
                                1 + end_pos - begin_pos);
        BITS_IN_WORD32(&bits1, value);
        return bits1;
    }

    value = GET_MASKED_BITS(bitmap->map[i], begin_pos & 0x1F,
                            32 - (begin_pos & 0x1F));
    BITS_IN_WORD32(&bits1, value);
    value = GET_MASKED_BITS(bitmap->map[_BMAP_INDEX(end_pos)],
                            0, 1 + (end_pos & 0x1F));
    BITS_IN_WORD32(&bits2, value);

    return (bits1 + bits2 + 32 * (_BMAP_INDEX(end_pos) - i - 1));
}


void
skBitmapIteratorBind(
    const sk_bitmap_t  *bitmap,
    sk_bitmap_iter_t   *iter)
{
    assert(bitmap);
    assert(iter);

    memset(iter, 0, sizeof(sk_bitmap_iter_t));
    iter->bitmap = bitmap;
    skBitmapIteratorReset(iter);
}


int
skBitmapIteratorNext(
    sk_bitmap_iter_t   *iter,
    uint32_t           *pos)
{
    uint32_t word_count;

    assert(iter);
    assert(pos);

    word_count = BITMAP_GET_WORD_COUNT(iter->bitmap->num_bits);
    if (word_count == iter->map_idx) {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }

    if (iter->bitmap->map[iter->map_idx] >> iter->pos) {
        /* find next bit in current word */
        iter->pos += bitmapCountTrailingZeros(iter->bitmap->map[iter->map_idx]
                                              >> iter->pos);
        *pos = ((iter->map_idx << 5) | iter->pos);
        if (iter->pos < 31) {
            ++iter->pos;
        } else {
            ++iter->map_idx;
            iter->pos = 0;
        }
        return SK_ITERATOR_OK;
    }

    /* find next word with bits */
    for (++iter->map_idx; iter->map_idx < word_count; ++iter->map_idx) {
        if (iter->bitmap->map[iter->map_idx]) {
            iter->pos
                = bitmapCountTrailingZeros(iter->bitmap->map[iter->map_idx]);
            *pos = ((iter->map_idx << 5) | iter->pos);
            if (iter->pos < 31) {
                ++iter->pos;
            } else {
                ++iter->map_idx;
                iter->pos = 0;
            }
            return SK_ITERATOR_OK;
        }
    }

    return SK_ITERATOR_NO_MORE_ENTRIES;
}


void
skBitmapIteratorReset(
    sk_bitmap_iter_t   *iter)
{
    assert(iter);

    iter->map_idx = 0;
    iter->pos = 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
