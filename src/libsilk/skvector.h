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
**  skvector.h
**
**    Grow-able arrays
**
*/
#ifndef _SKVECTOR_H
#define _SKVECTOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKVECTOR_H, "$SiLK: skvector.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/silk_types.h>

/**
 *  @file
 *
 *    Implementation of a simple growable array.
 *
 *    This file is part of libsilk.
 */

/* typedef struct sk_vector_st sk_vector_t; // silk_types.h */


/**
 *    Creates a new vector whose elements are each element_size bytes.
 *
 *    Does not allocate any space for the elements.
 */
sk_vector_t *
skVectorNew(
    size_t              element_size);


/**
 *    Creates a new vector whose elements are each 'element_size'
 *    bytes.  The vector allocates enough space for 'count' elements,
 *    and copies ('count' * 'element_size') bytes from 'array' into
 *    the vector.
 *
 *    Returns the new vector or NULL on allocation error.
 */
sk_vector_t *
skVectorNewFromArray(
    size_t              element_size,
    const void         *array,
    size_t              count);

/**
 *    Creates a new copy of a vector, or NULL on allocation error.
 */
sk_vector_t *
skVectorClone(
    const sk_vector_t  *v);


/**
 *    Destroys the vector, v, freeing all memory that the vector manages.
 */
void
skVectorDestroy(
    sk_vector_t        *v);


/**
 *    Sets the capacity of the vector to 'v', growing or shrinking the
 *    spaced allocated for the elements as required.  When the vector
 *    grows, the newly allocated space is zeroed.  Returns 0 on
 *    success, or -1 for an allocation error.
 */
int
skVectorSetCapacity(
    sk_vector_t        *v,
    size_t              capacity);


/**
 *    Zero's all memory for the elements and sets the count of
 *    elements in the vector to zero.
 */
void
skVectorClear(
    sk_vector_t        *v);


/**
 *    Returns the element size that was specified when the vector was
 *    created.
 */
size_t
skVectorGetElementSize(
    const sk_vector_t  *v);


/**
 *    Returns the capacity of the vector, i.e., the number of elements
 *    the vector can hold without requiring a re-allocation.
 */
size_t
skVectorGetCapacity(
    const sk_vector_t  *v);


/**
 *    Returns the number elements that have been added to the vector.
 */
size_t
skVectorGetCount(
    const sk_vector_t  *v);


/**
 *    Copies the data at 'value' into the vector 'v' at position
 *    skVectorGetCount(v).  Returns 0 on success, or -1 for an
 *    allocation error.
 */
int
skVectorAppendValue(
    sk_vector_t        *v,
    const void         *value);


/**
 *    Copies the data from 'src' into the vector 'dst' at position
 *    skVectorGetCount(v).  Returns 0 on success, or -1 for an
 *    allocation error.
 */
int
skVectorAppendVector(
    sk_vector_t        *dst,
    const sk_vector_t  *src);

/**
 *    Copies the data from 'array' into the vector 'v' at position
 *    skVectorGetCount(v).  Returns 0 on success, or -1 for an
 *    allocation error.
 */
int
skVectorAppendFromArray(
    sk_vector_t        *v,
    const void         *array,
    size_t              count);

/**
 *    Copies the data at 'value' into the vector 'v' at 'position',
 *    where 0 denotes the first position in the vector.
 *
 *    'position' must be within the current capacity of the vector;
 *    i.e., the vector will not grow to support data at 'position'.
 *    If 'position' is too large, -1 is returned.
 *
 *    This call may affect the count of elements in the vector.
 */
int
skVectorSetValue(
    sk_vector_t        *v,
    size_t              position,
    const void         *value);


/**
 *    Copies the data in vector 'v' at 'position' to the location
 *    pointed at by 'out_element'.  The first position in the vector
 *    is position 0.  Returns 0 on success, or -1 if 'position' is
 *    larger than skVectorGetCount(v).
 *
 *    Equivalent to
 *
 *    -1 + skVectorGetMultipleValues(out_element, v, position, 1)
 */
int
skVectorGetValue(
    void               *out_element,
    const sk_vector_t  *v,
    size_t              position);


/**
 *    Returns a pointer to the data item in vector 'v' at 'position'.
 *    The first position in the vector is position 0.  Returns NULL if
 *    'position' is larger than skVectorGetCount(v).
 *
 *    The caller should not cache this value, since any addition to
 *    the vector may result in a re-allocation that could make the
 *    pointer invalid.
 */
void *
skVectorGetValuePointer(
    const sk_vector_t  *v,
    size_t              position);


/**
 *    Copies up to 'num_elements' data elements starting at
 *    'start_position' from vector 'v' to the location pointed at by
 *    'out_array'.  Returns fewer than 'num_elements' if the end of
 *    the vector is reached.  The first position in the vector is
 *    position 0.
 *
 *    It is the caller's responsibility to ensure that 'out_array' can
 *    hold 'num_elements' elements of size skVectorGetElementSize(v).
 *
 *    Returns the number of elements copied into the array.  Returns
 *    0 if 'position' is larger than skVectorGetCount(v).
 */
size_t
skVectorGetMultipleValues(
    void               *out_array,
    const sk_vector_t  *v,
    size_t              start_position,
    size_t              num_elements);


/**
 *    Copies the data in the vector 'v' to the C-array 'out_array'.
 *    This is equivalent to:
 *
 *    skVectorGetMultipleValues(out_array, v, 0, skVectorGetCount(v));
 *
 *    It is the caller's responsibility to ensure that 'out_array' is
 *    large enough to hold skVectorGetCount(v) elements of size
 *    skVectorGetElementSize(v).
 */
void
skVectorToArray(
    void               *out_array,
    const sk_vector_t  *v);


/**
 *    Allocates an array large enough to hold all the elements of the
 *    vector 'v', copies the elements from 'v' into the array, and
 *    returns the array.
 *
 *    The caller must free() the array when it is no longer required.
 *
 *    The function returns NULL if the vector is empty or if the array
 *    could not be allocated.
 *
 *    This function is equivalent to:
 *
 *    a = malloc(skVectorGetElementSize(v) * skVectorGetCount(v));
 *    skVectorGetMultipleValues(a, v, 0, skVectorGetCount(v));
 */
void *
skVectorToArrayAlloc(
    const sk_vector_t  *v);


#ifdef __cplusplus
}
#endif
#endif /* _SKVECTOR_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
