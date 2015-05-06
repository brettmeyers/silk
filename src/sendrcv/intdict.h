/*
** Copyright (C) 2006-2015 by Carnegie Mellon University.
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
#ifndef _INTDICT_H
#define _INTDICT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_INTDICT_H, "$SiLK: intdict.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/*
**  intdict.h
**
**  Integer dictionaries
**
*/


typedef int32_t intkey_t;

struct int_dict_st;
typedef struct int_dict_st int_dict_t;

struct int_dict_iter_st;
typedef struct int_dict_iter_st int_dict_iter_t;

int_dict_t *
int_dict_create(
    size_t              value_size);

void
int_dict_destroy(
    int_dict_t         *dict);

void *
int_dict_get(
    int_dict_t         *dict,
    intkey_t            key,
    void               *value);

void *
int_dict_get_first(
    int_dict_t         *dict,
    intkey_t           *key,
    void               *value);

void *
int_dict_get_last(
    int_dict_t         *dict,
    intkey_t           *key,
    void               *value);

void *
int_dict_get_next(
    int_dict_t         *dict,
    intkey_t           *key,
    void               *value);

void *
int_dict_get_prev(
    int_dict_t         *dict,
    intkey_t           *key,
    void               *value);

int
int_dict_set(
    int_dict_t         *dict,
    intkey_t            key,
    void               *value);

int
int_dict_del(
    int_dict_t         *dict,
    intkey_t            key);

unsigned int
int_dict_get_count(
    int_dict_t         *dict);

int_dict_iter_t *
int_dict_open(
    int_dict_t         *dict);

void *
int_dict_next(
    int_dict_iter_t    *iter,
    intkey_t           *key,
    void               *value);

void
int_dict_close(
    int_dict_iter_t    *iter);

#ifdef __cplusplus
}
#endif
#endif /* _INTDICT_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
