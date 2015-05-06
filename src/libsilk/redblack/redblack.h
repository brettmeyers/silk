/*
 *    Redblack balanced tree algorithm
 *    Copyright (C) Damian Ivereigh 2000
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 2.1 of the License, or (at your option) any later
 *    version. See the file COPYING for details.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this program; if not, write to the Free
 *    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 *    USA.
 */

/*
 *    Includes modifications made by Carnegie Mellon Univeristy,
 *    December 3, 2014, to work in a multi-threaded environment.
 *
 *    Original source code is available from
 *    http://sourceforge.net/projects/libredblack/files/
 *
 *    Documentation for the original source code is available at
 *    http://libredblack.sourceforge.net/
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

RCSIDENTVAR(rcsID_REDBLACK_H, "$SiLK: redblack.h 9f00e19adb68 2014-12-03 23:15:22Z mthomas $");

/* Modes for rblookup */
#define RB_NONE -1      /* None of those below */

/**
 *    Returns node exactly matching the key. This is equivalent to
 *    rbfind
 */
#define RB_LUEQUAL 0

/**
 *    Returns the node exactly matching the specified key, if this is
 *    not found then the next node that is greater than the specified
 *    key is returned.
 */
#define RB_LUGTEQ 1

/**
 *    Returns the node exactly matching the specified key, if this is
 *    not found then the next node that is less than the specified key
 *    is returned.
 */
#define RB_LULTEQ 2

/**
 *    Returns the node that is just less than the specified key - not
 *    equal to. This mode is similar to RB_LUPREV except that the
 *    specified key need not exist in the tree.
 */
#define RB_LULESS 3

/**
 *    Returns the node that is just greater than the specified key -
 *    not equal to. This mode is similar to RB_LUNEXT except that the
 *    specified key need not exist in the tree.
 */
#define RB_LUGREAT 4

/**
 *    Looks for the key specified, if not found returns NULL. If the
 *    node is found returns the next node that is greater than the one
 *    found (or NULL if there is no next node). This is used to step
 *    through the tree in order.
 */
#define RB_LUNEXT 5

/**
 *    Looks for the key specified, if not found returns NULL. If the
 *    node is found returns the previous node that is less than the
 *    one found (or NULL if there is no previous node). This is used
 *    to step through the tree in reverse order.
 */
#define RB_LUPREV 6

/**
 *    Returns the first node in the tree (i.e. the one with the lowest
 *    key). The argument key is ignored.
 */
#define RB_LUFIRST 7

/**
 *    Returns the last node in the tree (i.e. the one with the largest
 *    key). The argument key is ignored.
 */
#define RB_LULAST 8


/**
 *    Values that the rbwalk() function passes to the 'action'
 *    callback function to denote the type of node being visited.
 *
 *    These values pinched from search.h.
 */
typedef enum {
    /** This is an internal node, and this is the first visit to the
     *  node; that is, the visit on the way down the tree */
    preorder,
    /** This is an internal node, and this is the second visit to the
     *  node; that is, the in-order visit */
    postorder,
    /** This is an internal node, and this is the final visit to the
     *  node; that is, the visit on the way back up the tree */
    endorder,
    /** This node is a leaf */
    leaf
} VISIT;


/**
 *    Support for visiting the nodes of the red-black tree using
 *    rbopenlist(), rbreadlist(), and rbcloselist().
 */
struct rblists {
    const struct rbnode *rootp;
    const struct rbnode *nextp;
    const struct rbnode *nullp;
};

#define RBLIST struct rblists

/**
 *    The redblack tree structure.
 */
struct rbtree {
    /** the caller's comparison routine */
    int (*rb_cmp)(const void *, const void *, const void *);
    /** config data to be passed to rb_cmp */
    const void *rb_config;
    /** the root of tree */
    struct rbnode *rb_root;
    /** the null pointer */
    struct rbnode *rb_null;
};


/**
 *    rbinit() initialises the tree, stores a pointer to the
 *    comparison routine and any config data, which may be NULL if not
 *    required.  A pointer to type struct rbtree is returned and is
 *    used in subsequent calls into the redblack library.
 *
 *    A typical compare routine would be defined thus:
 *
 *    int cmp(const void *p1, const void *p2, const void *config);
 *
 *    The arguments p1 and p2 are the pointers to the data to be
 *    compared. It should return an integer which is negative, zero,
 *    or positive, depending on whether the first item is less than,
 *    equal to, or greater than the second. config is used to alter
 *    the behaviour of the compare routine in a fixed way. For example
 *    using the same compare routine with multiple trees - with this
 *    compare routine behaving differently for each tree. The config
 *    argument is just passed straight through to the compare routine
 *    and if not used may be set to NULL. N.B. It is very important
 *    that the compare routine be deterministic and stateless,
 *    i.e. always return the same result for the same given data.
 */
struct rbtree *
rbinit(
    int               (*rb_cmp)(
        const void  *p1,
        const void  *p2,
        const void  *config),
    const void         *config);

/**
 *    rbdelete() deletes an item from the tree. Its arguments are the
 *    same as for rbsearch().
 */
const void *
rbdelete(
    const void         *key,
    struct rbtree      *rb);

/**
 *    rbfind() searches the tree for an item. 'key' points to the item
 *    to be searched for. 'rb' points to the structure returned by
 *    rbinit(). If the item is found in the tree, then rbfind()
 *    returns a pointer to it. If it is not found, then rbfind()
 *    returns NULL.
 *
 *    See also rbsearch().
 */
const void *
rbfind(
    const void         *key,
    struct rbtree      *rb);

/**
 *    rblookup() allows the traversing of the tree. Which key is
 *    returned is determined by 'mode'. If requested 'key' cannot be
 *    found then rblookup() returns NULL. mode can be any one of the
 *    following:
 *
 *    RB_LUEQUAL
 *    RB_LUGTEQ
 *    RB_LULTEQ
 *    RB_LUGREAT
 *    RB_LULESS
 *    RB_LUNEXT
 *    RB_LUPREV
 *    RB_LUFIRST
 *    RB_LULAST
 */
const void *
rblookup(
    int                 mode,
    const void         *key,
    struct rbtree      *rb);

/**
 *    rbsearch() searches the tree for an item. 'key' points to the
 *    item to be searched for. 'rb' points to the structure returned
 *    by rbinit(). If the item is found in the tree, then rbsearch()
 *    returns a pointer to it. If it is not found, then rbsearch()
 *    adds it, and returns a pointer to the newly added item.
 *
 *    See also rbfind().
 */
const void *
rbsearch(
    const void         *key,
    struct rbtree      *rb);

/**
 *    rbdestroy() destroys any tree allocated by rbinit() and will
 *    free up any allocated nodes. N.B. The users data is not freed,
 *    since it is the users responsibility to store (and free) this
 *    data.
 */
void
rbdestroy(
    struct rbtree      *rb);

/**
 *    rbwalk() performs depth-first, left-to-right traversal of a
 *    binary tree. rbwalk() calls the user function 'action' each time
 *    a node is visited (that is, three times for an internal node,
 *    and once for a leaf). 'action', in turn, takes four
 *    arguments. The first is a pointer to the node being visited. The
 *    second is an integer which takes on the values preorder,
 *    postorder, and endorder depending on whether this is the first,
 *    second, or third visit to the internal node, or leaf if it is
 *    the single visit to a leaf node.  The third argument is the
 *    depth of the node, with zero being the root.  The fourth
 *    argument is a context pointer.
 */
void
rbwalk(
    const struct rbtree    *rb,
    void                  (*action)(
        const void     *key,
        const VISIT     visit,
        const int       depth,
        void           *rbwalk_ctx),
    void                    *rbwalk_ctx);

/**
 *    rbopenlist(), rbreadlist(), and rbcloselist() provide a simple
 *    way to read from a redblack binary tree created by rbinit().
 *
 *    rbopenlist() initialises the list and returns a RBLIST pointer
 *    that is used in subsequent calls to rbreadlist() and
 *    rbcloselist().
 */
RBLIST *
rbopenlist(
    const struct rbtree    *rb);

/**
 *    rbreadlist() returns a pointer to the node data. Each subsequent
 *    call returns the next node in the order specified by the tree.
 *
 *    See rbopenlist().
 */
const void *
rbreadlist(
    RBLIST              *rblp);

/**
 *    rbcloselist() simply frees up the memory used to allocate the
 *    RBLIST data pointer.
 *
 *    See rbopenlist().
 */
void
rbcloselist(
    RBLIST              *rblp);

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
