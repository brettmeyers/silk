/*
   Redblack balanced tree algorithm
   Copyright (C) Damian Ivereigh 2000

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version. See the file COPYING for details.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Header file for redblack.c, should be included by any code that
** uses redblack.c since it defines the functions
*/

/* Stop multiple includes */
#ifndef _REDBLACK_H
#define _REDBLACK_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_REDBLACK_H, "$SiLK: redblack.h 412b51a029ce 2014-01-28 22:59:06Z mthomas $");

/* Modes for rblookup */
#define RB_NONE -1	    /* None of those below */
#define RB_LUEQUAL 0	/* Only exact match */
#define RB_LUGTEQ 1		/* Exact match or greater */
#define RB_LULTEQ 2		/* Exact match or less */
#define RB_LULESS 3		/* Less than key (not equal to) */
#define RB_LUGREAT 4	/* Greater than key (not equal to) */
#define RB_LUNEXT 5		/* Next key after current */
#define RB_LUPREV 6		/* Prev key before current */
#define RB_LUFIRST 7	/* First key in index */
#define RB_LULAST 8		/* Last key in index */

/* For rbwalk - pinched from search.h */
typedef enum
{
  preorder,
  postorder,
  endorder,
  leaf
}
VISIT;

struct rblists {
const struct rbnode *rootp;
const struct rbnode *nextp;
};

#define RBLIST struct rblists

struct rbtree {
		/* comparison routine */
int (*rb_cmp)(const void *, const void *, const void *);
		/* config data to be passed to rb_cmp */
const void *rb_config;
		/* root of tree */
struct rbnode *rb_root;
};

struct rbtree *rbinit(int (*)(const void *, const void *, const void *),
		 const void *);
const void *rbdelete(const void *, struct rbtree *);
const void *rbfind(const void *, struct rbtree *);
const void *rblookup(int, const void *, struct rbtree *);
const void *rbsearch(const void *, struct rbtree *);
void rbdestroy(struct rbtree *);
void rbwalk(const struct rbtree *,
		void (*)(const void *, const VISIT, const int, void *),
		void *);
RBLIST *rbopenlist(const struct rbtree *);
const void *rbreadlist(RBLIST *);
void rbcloselist(RBLIST *);

/* Some useful macros */
#define rbmin(rbinfo) rblookup(RB_LUFIRST, NULL, (rbinfo))
#define rbmax(rbinfo) rblookup(RB_LULAST, NULL, (rbinfo))

#ifdef __cplusplus
}
#endif
#endif /* _REDBLACK_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
