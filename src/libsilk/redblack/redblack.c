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

/* Implement the red/black tree structure. It is designed to emulate
** the standard tsearch() stuff. i.e. the calling conventions are
** exactly the same
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: redblack.c 9f00e19adb68 2014-12-03 23:15:22Z mthomas $");

#include <silk/redblack.h>


/* Uncomment this if you would rather use a raw sbrk to get memory
** (however the memory is never released again (only re-used). Can't
** see any point in using this these days.
*/
/* #define USE_SBRK */

enum nodecolour { BLACK, RED };

struct rbnode {
    struct rbnode      *left;       /* Left down */
    struct rbnode      *right;      /* Right down */
    struct rbnode      *up;         /* Up */
    enum nodecolour     colour;     /* Node colour */
    const void         *key;        /* Pointer to user's key (and data) */
};


#if defined(USE_SBRK)

static struct rbnode *rb_alloc();
static void rb_free(struct rbnode *);

#else

#define rb_alloc() ((struct rbnode *) malloc(sizeof(struct rbnode)))
#define rb_free(x) (free(x))

#endif

static struct rbnode *rb_traverse(int, const void *, struct rbtree *);
static struct rbnode *rb_lookup(int, const void *, struct rbtree *);
static void rb_destroy(struct rbnode *, const struct rbnode *);
static void
rb_left_rotate(
    struct rbnode **,
    struct rbnode *,
    const struct rbnode *);
static void
rb_right_rotate(
    struct rbnode **,
    struct rbnode *,
    const struct rbnode *);
static void
rb_delete(
    struct rbnode **,
    struct rbnode *,
    const struct rbnode *);
static void
rb_delete_fix(
    struct rbnode **,
    struct rbnode *,
    const struct rbnode *);
static struct rbnode *
rb_successor(
    const struct rbnode *,
    const struct rbnode *);
static struct rbnode *
rb_preccessor(
    const struct rbnode *,
    const struct rbnode *);
static void
rb_walk(
    const struct rbnode *,
    void (*)(const void *, const VISIT, const int, void *),
    void *,
    int,
    const struct rbnode *);
static RBLIST *rb_openlist(const struct rbnode *, const struct rbnode *);
static const void *rb_readlist(RBLIST *);
static void rb_closelist(RBLIST *);

/*
 *    Macros to set the members of a struct rbnode.
 */
#ifndef REDBLACK_DEBUG

#define NODE_SET_COLOUR(m_node, m_value) (m_node)->colour = (m_value)
#define NODE_SET_LEFT(m_node, m_value)   (m_node)->left   = (m_value)
#define NODE_SET_RIGHT(m_node, m_value)  (m_node)->right  = (m_value)
#define NODE_SET_UP(m_node, m_value)     (m_node)->up     = (m_value)

#else

#include <silk/sklog.h>

#define NODE_SET_COLOUR(m_node, m_value)                                \
    do {                                                                \
        if ((m_node) != RBNULL) {                                       \
            (m_node)->colour = (m_value);                               \
        } else if ((m_value) != BLACK) {                                \
            DEBUGMSG("%p %s:%d: Setting colour of %p to red",           \
                     (void*)pthread_self(), __FILE__, __LINE__,         \
                     (m_node));                                         \
            (m_node)->colour = (m_value);                               \
        } else if ((m_node)->colour != BLACK) {                         \
            DEBUGMSG("%p %s:%d: Restoring colour of %p to black",       \
                     (void*)pthread_self(), __FILE__, __LINE__,         \
                     (m_node));                                         \
            (m_node)->colour = (m_value);                               \
        }                                                               \
    } while(0)

#define NODE_SET_HELPER(m_node, m_value, m_member)                      \
    do {                                                                \
        if ((m_node) != RBNULL) {                                       \
            (m_node)-> m_member = (m_value);                            \
        } else if ((m_value) != RBNULL) {                               \
            DEBUGMSG("%p %s:%d: Setting %s of %p to non-RBNULL value %p", \
                     (void*)pthread_self(), __FILE__, __LINE__,         \
                     #m_member, (m_node), (m_value));                   \
            (m_node)-> m_member = (m_value);                            \
        } else if ((m_node)-> m_member != RBNULL) {                     \
            DEBUGMSG("%p %s:%d: Restoring %s of %p to RBNULL value %p", \
                     (void*)pthread_self(), __FILE__, __LINE__,         \
                     #m_member, (m_node), (m_value));                   \
            (m_node)-> m_member = (m_value);                            \
        }                                                               \
    } while(0)

#define NODE_SET_LEFT(m_node, m_value)          \
    NODE_SET_HELPER(m_node, m_value, left)

#define NODE_SET_RIGHT(m_node, m_value)         \
    NODE_SET_HELPER(m_node, m_value, right)

#define NODE_SET_UP(m_node, m_value)            \
    NODE_SET_HELPER(m_node, m_value, up)

#endif  /* #else of #ifndef REDBLACK_DEBUG */


/*
** OK here we go, the balanced tree stuff. The algorithm is the
** fairly standard red/black taken from "Introduction to Algorithms"
** by Cormen, Leiserson & Rivest. Maybe one of these days I will
** fully understand all this stuff.
**
** Basically a red/black balanced tree has the following properties:-
** 1) Every node is either red or black (colour is RED or BLACK)
** 2) A leaf (RBNULL pointer) is considered black
** 3) If a node is red then its children are black
** 4) Every path from a node to a leaf contains the same no
**    of black nodes
**
** 3) & 4) above guarantee that the longest path (alternating
** red and black nodes) is only twice as long as the shortest
** path (all black nodes). Thus the tree remains fairly balanced.
*/

/*
 * Initialise a tree. Identifies the comparison routine and any config
 * data that must be sent to it when called.
 * Returns a pointer to the top of the tree.
 */
struct rbtree *
rbinit(
    int               (*cmp)(const void *p1, const void *p2, const void *cfg),
    const void         *config)
{
    struct rbnode *RBNULL;
    struct rbtree *retval;

    retval = (struct rbtree *)malloc(sizeof(struct rbtree));
    if (NULL == retval) {
        return(NULL);
    }

    /* Dummy (sentinel) node, so that we can make X->left->up = X
     * We then use this instead of NULL to mean the top or bottom
     * end of the rb tree. It is a black node.
     */
    RBNULL = retval->rb_null = (struct rbnode *)malloc(sizeof(struct rbnode));
    if (NULL == retval->rb_null) {
        free(retval);
        return NULL;
    }
    retval->rb_null->left   = RBNULL;
    retval->rb_null->right  = RBNULL;
    retval->rb_null->up     = RBNULL;
    retval->rb_null->colour = BLACK;
    retval->rb_null->key    = NULL;

    retval->rb_cmp = cmp;
    retval->rb_config = config;
    retval->rb_root = RBNULL;

    return(retval);
}

void
rbdestroy(
    struct rbtree      *rbinfo)
{
    const struct rbnode *RBNULL;

    if (rbinfo == NULL) {
        return;
    }
    RBNULL = rbinfo->rb_null;

    if (rbinfo->rb_root != RBNULL) {
        rb_destroy(rbinfo->rb_root, RBNULL);
    }

    free(rbinfo->rb_null);
    free(rbinfo);
}

const void *
rbsearch(
    const void         *key,
    struct rbtree      *rbinfo)
{
    const struct rbnode *RBNULL;
    struct rbnode *x;

    if (rbinfo == NULL) {
        return(NULL);
    }
    RBNULL = rbinfo->rb_null;

    x = rb_traverse(1, key, rbinfo);

    return((x == RBNULL) ? NULL : x->key);
}

const void *
rbfind(
    const void         *key,
    struct rbtree      *rbinfo)
{
    const struct rbnode *RBNULL;
    struct rbnode *x;

    if (rbinfo == NULL) {
        return(NULL);
    }
    RBNULL = rbinfo->rb_null;

    /* If we have a NULL root (empty tree) then just return */
    if (rbinfo->rb_root == RBNULL) {
        return(NULL);
    }

    x = rb_traverse(0, key, rbinfo);

    return((x == RBNULL) ? NULL : x->key);
}

const void *
rbdelete(
    const void         *key,
    struct rbtree      *rbinfo)
{
    const struct rbnode *RBNULL;
    struct rbnode *x;
    const void *y;

    if (rbinfo == NULL) {
        return(NULL);
    }
    RBNULL = rbinfo->rb_null;

    x = rb_traverse(0, key, rbinfo);

    if (x == RBNULL) {
        return(NULL);
    } else {
        y = x->key;
        rb_delete(&rbinfo->rb_root, x, RBNULL);
        return(y);
    }
}

void
rbwalk(
    const struct rbtree    *rbinfo,
    void                  (*action)(
        const void *,
        const VISIT,
        const int,
        void *),
    void                   *arg)
{
    if (rbinfo == NULL) {
        return;
    }

    rb_walk(rbinfo->rb_root, action, arg, 0, rbinfo->rb_null);
}

RBLIST *
rbopenlist(
    const struct rbtree    *rbinfo)
{
    if (rbinfo == NULL) {
        return(NULL);
    }

    return(rb_openlist(rbinfo->rb_root, rbinfo->rb_null));
}

const void *
rbreadlist(
    RBLIST             *rblistp)
{
    if (rblistp == NULL) {
        return(NULL);
    }

    return(rb_readlist(rblistp));
}

void
rbcloselist(
    RBLIST             *rblistp)
{
    if (rblistp == NULL) {
        return;
    }

    rb_closelist(rblistp);
}

const void *
rblookup(
    int                 mode,
    const void         *key,
    struct rbtree      *rbinfo)
{
    const struct rbnode *RBNULL;
    struct rbnode *x;

    /* If we have a NULL root (empty tree) then just return NULL */
    if (rbinfo == NULL || rbinfo->rb_root == NULL) {
        return(NULL);
    }
    RBNULL = rbinfo->rb_null;

    x = rb_lookup(mode, key, rbinfo);

    return((x == RBNULL) ? NULL : x->key);
}

/* --------------------------------------------------------------------- */

/* Search for and if not found and insert is true, will add a new
** node in. Returns a pointer to the new node, or the node found
*/
static struct rbnode *
rb_traverse(
    int                 insert,
    const void         *key,
    struct rbtree      *rbinfo)
{
    struct rbnode *RBNULL = rbinfo->rb_null;
    struct rbnode *x, *y, *z;
    int cmp;
    int found = 0;
    /* int cmpmods(); */

    y = RBNULL; /* points to the parent of x */
    x = rbinfo->rb_root;

    /* walk x down the tree */
    while (x != RBNULL && found == 0) {
        y = x;
        /* printf("key=%s, x->key=%s\n", key, x->key); */
        cmp = (*rbinfo->rb_cmp)(key, x->key, rbinfo->rb_config);
        if (cmp < 0) {
            x = x->left;
        } else if (cmp > 0) {
            x = x->right;
        } else {
            found = 1;
        }
    }

    if (found || !insert) {
        return(x);
    }

    if ((z = rb_alloc()) == NULL) {
        /* Whoops, no memory */
        return(RBNULL);
    }

    z->key = key;
    NODE_SET_UP(z, y);
    if (y == RBNULL) {
        rbinfo->rb_root = z;
    } else {
        cmp = (*rbinfo->rb_cmp)(z->key, y->key, rbinfo->rb_config);
        if (cmp < 0) {
            NODE_SET_LEFT(y, z);
        } else {
            NODE_SET_RIGHT(y, z);
        }
    }

    NODE_SET_LEFT(z, RBNULL);
    NODE_SET_RIGHT(z, RBNULL);

    /* colour this new node red */
    NODE_SET_COLOUR(z, RED);

    /* Having added a red node, we must now walk back up the tree balancing
    ** it, by a series of rotations and changing of colours
    */
    x = z;

    /* While we are not at the top and our parent node is red
    ** N.B. Since the root node is garanteed black, then we
    ** are also going to stop if we are the child of the root
    */

    while (x != rbinfo->rb_root && (x->up->colour == RED)) {
        /* if our parent is on the left side of our grandparent */
        if (x->up == x->up->up->left) {
            /* get the right side of our grandparent (uncle?) */
            y = x->up->up->right;
            if (y->colour == RED) {
                /* make our parent black */
                NODE_SET_COLOUR(x->up, BLACK);
                /* make our uncle black */
                NODE_SET_COLOUR(y, BLACK);
                /* make our grandparent red */
                NODE_SET_COLOUR(x->up->up, RED);

                /* now consider our grandparent */
                x = x->up->up;
            } else {
                /* if we are on the right side of our parent */
                if (x == x->up->right) {
                    /* Move up to our parent */
                    x = x->up;
                    rb_left_rotate(&rbinfo->rb_root, x, RBNULL);
                }

                /* make our parent black */
                NODE_SET_COLOUR(x->up, BLACK);
                /* make our grandparent red */
                NODE_SET_COLOUR(x->up->up, RED);
                /* right rotate our grandparent */
                rb_right_rotate(&rbinfo->rb_root, x->up->up, RBNULL);
            }
        } else {
            /* everything here is the same as above, but
            ** exchanging left for right
            */

            y = x->up->up->left;
            if (y->colour == RED) {
                NODE_SET_COLOUR(x->up, BLACK);
                NODE_SET_COLOUR(y, BLACK);
                NODE_SET_COLOUR(x->up->up, RED);

                x = x->up->up;
            } else {
                if (x == x->up->left) {
                    x = x->up;
                    rb_right_rotate(&rbinfo->rb_root, x, RBNULL);
                }

                NODE_SET_COLOUR(x->up, BLACK);
                NODE_SET_COLOUR(x->up->up, RED);
                rb_left_rotate(&rbinfo->rb_root, x->up->up, RBNULL);
            }
        }
    }

    /* Set the root node black */
    NODE_SET_COLOUR(rbinfo->rb_root, BLACK);

    return(z);
}

/* Search for a key according to mode (see redblack.h)
*/
static struct rbnode *
rb_lookup(
    int                 mode,
    const void         *key,
    struct rbtree      *rbinfo)
{
    struct rbnode *RBNULL = rbinfo->rb_null;
    struct rbnode *x, *y;
    int cmp = 0;
    int found = 0;

    y = RBNULL; /* points to the parent of x */
    x = rbinfo->rb_root;

    if (mode == RB_LUFIRST) {
        /* Keep going left until we hit a NULL */
        while (x != RBNULL) {
            y = x;
            x = x->left;
        }

        return(y);
    } else if (mode == RB_LULAST) {
        /* Keep going right until we hit a NULL */
        while (x != RBNULL) {
            y = x;
            x = x->right;
        }

        return(y);
    }

    /* walk x down the tree */
    while (x != RBNULL && found == 0) {
        y = x;
        /* printf("key=%s, x->key=%s\n", key, x->key); */
        cmp = (*rbinfo->rb_cmp)(key, x->key, rbinfo->rb_config);
        if (cmp < 0) {
            x = x->left;
        } else if (cmp > 0) {
            x = x->right;
        } else {
            found = 1;
        }
    }

    if (found
        && (mode == RB_LUEQUAL || mode == RB_LUGTEQ || mode == RB_LULTEQ))
    {
        return(x);
    }

    if (!found
        && (mode == RB_LUEQUAL || mode == RB_LUNEXT || mode == RB_LUPREV))
    {
        return(RBNULL);
    }

    if (mode == RB_LUGTEQ || (!found && mode == RB_LUGREAT)) {
        if (cmp > 0) {
            return(rb_successor(y, RBNULL));
        } else {
            return(y);
        }
    }

    if (mode == RB_LULTEQ || (!found && mode == RB_LULESS)) {
        if (cmp < 0) {
            return(rb_preccessor(y, RBNULL));
        } else {
            return(y);
        }
    }

    if (mode == RB_LUNEXT || (found && mode == RB_LUGREAT)) {
        return(rb_successor(x, RBNULL));
    }

    if (mode == RB_LUPREV || (found && mode == RB_LULESS)) {
        return(rb_preccessor(x, RBNULL));
    }

    /* Shouldn't get here */
    return(RBNULL);
}

/*
 * Destroy all the elements blow us in the tree
 * only useful as part of a complete tree destroy.
 */
static void
rb_destroy(
    struct rbnode          *x,
    const struct rbnode    *RBNULL)
{
    if (x != RBNULL) {
        if (x->left != RBNULL) {
            rb_destroy(x->left, RBNULL);
        }
        if (x->right != RBNULL) {
            rb_destroy(x->right, RBNULL);
        }
        rb_free(x);
    }
}

/*
** Rotate our tree thus:-
**
**             X        rb_left_rotate(X)--->            Y
**           /   \                                     /   \
**          A     Y     <---rb_right_rotate(Y)        X     C
**              /   \                               /   \
**             B     C                             A     B
**
** N.B. This does not change the ordering.
**
** We assume that neither X or Y is NULL
*/

static void
rb_left_rotate(
    struct rbnode         **rootp,
    struct rbnode          *x,
    const struct rbnode    *RBNULL)
{
    struct rbnode *y;

    assert(x != RBNULL);
    assert(x->right != RBNULL);

    y = x->right; /* set Y */

    /* Turn Y's left subtree into X's right subtree (move B)*/
    NODE_SET_RIGHT(x, y->left);

    /* If B is not null, set it's parent to be X */
    if (y->left != RBNULL) {
        NODE_SET_UP(y->left, x);
    }

    /* Set Y's parent to be what X's parent was */
    NODE_SET_UP(y, x->up);

    /* if X was the root */
    if (x->up == RBNULL) {
        *rootp = y;
    } else {
        /* Set X's parent's left or right pointer to be Y */
        if (x == x->up->left) {
            NODE_SET_LEFT(x->up, y);
        } else {
            NODE_SET_RIGHT(x->up, y);
        }
    }

    /* Put X on Y's left */
    NODE_SET_LEFT(y, x);

    /* Set X's parent to be Y */
    NODE_SET_UP(x, y);
}

static void
rb_right_rotate(
    struct rbnode         **rootp,
    struct rbnode          *y,
    const struct rbnode    *RBNULL)
{
    struct rbnode *x;

    assert(y != RBNULL);
    assert(y->left != RBNULL);

    x = y->left; /* set X */

    /* Turn X's right subtree into Y's left subtree (move B) */
    NODE_SET_LEFT(y, x->right);

    /* If B is not null, set it's parent to be Y */
    if (x->right != RBNULL) {
        NODE_SET_UP(x->right, y);
    }

    /* Set X's parent to be what Y's parent was */
    NODE_SET_UP(x, y->up);

    /* if Y was the root */
    if (y->up == RBNULL) {
        *rootp = x;
    } else {
        /* Set Y's parent's left or right pointer to be X */
        if (y == y->up->left) {
            NODE_SET_LEFT(y->up, x);
        } else {
            NODE_SET_RIGHT(y->up, x);
        }
    }

    /* Put Y on X's right */
    NODE_SET_RIGHT(x, y);

    /* Set Y's parent to be X */
    NODE_SET_UP(y, x);
}

/* Return a pointer to the smallest key greater than x
*/
static struct rbnode *
rb_successor(
    const struct rbnode    *x,
    const struct rbnode    *RBNULL)
{
    struct rbnode *y;

    if (x->right != RBNULL) {
        /* If right is not NULL then go right one and
        ** then keep going left until we find a node with
        ** no left pointer.
        */
        for (y = x->right; y->left != RBNULL; y = y->left) ;
    } else {
        /* Go up the tree until we get to a node that is on the
        ** left of its parent (or the root) and then return the
        ** parent.
        */
        y = x->up;
        while (y != RBNULL && x == y->right) {
            x = y;
            y = y->up;
        }
    }
    return(y);
}

/* Return a pointer to the largest key smaller than x
*/
static struct rbnode *
rb_preccessor(
    const struct rbnode    *x,
    const struct rbnode    *RBNULL)
{
    struct rbnode *y;

    if (x->left != RBNULL) {
        /* If left is not NULL then go left one and
        ** then keep going right until we find a node with
        ** no right pointer.
        */
        for (y = x->left; y->right != RBNULL; y = y->right) ;
    } else {
        /* Go up the tree until we get to a node that is on the
        ** right of its parent (or the root) and then return the
        ** parent.
        */
        y = x->up;
        while (y != RBNULL && x == y->left) {
            x = y;
            y = y->up;
        }
    }
    return(y);
}

/* Delete the node z, and free up the space
*/
static void
rb_delete(
    struct rbnode         **rootp,
    struct rbnode          *z,
    const struct rbnode    *RBNULL)
{
    struct rbnode *x, *y;

    if (z->left == RBNULL || z->right == RBNULL) {
        y = z;
    } else {
        y = rb_successor(z, RBNULL);
    }

    if (y->left != RBNULL) {
        x = y->left;
    } else {
        x = y->right;
    }

    NODE_SET_UP(x, y->up);

    if (y->up == RBNULL) {
        *rootp = x;
    } else {
        if (y == y->up->left) {
            NODE_SET_LEFT(y->up, x);
        } else {
            NODE_SET_RIGHT(y->up, x);
        }
    }

    if (y != z) {
        z->key = y->key;
    }

    if (y->colour == BLACK) {
        rb_delete_fix(rootp, x, RBNULL);
    }

    rb_free(y);
}

/* Restore the reb-black properties after a delete */
static void
rb_delete_fix(
    struct rbnode         **rootp,
    struct rbnode          *x,
    const struct rbnode    *RBNULL)
{
    struct rbnode *w;

    while (x != *rootp && x->colour == BLACK) {
        if (x == x->up->left) {
            w = x->up->right;
            if (w->colour == RED) {
                NODE_SET_COLOUR(w, BLACK);
                NODE_SET_COLOUR(x->up, RED);
                rb_left_rotate(rootp, x->up, RBNULL);
                w = x->up->right;
            }

            if (w->left->colour == BLACK && w->right->colour == BLACK) {
                NODE_SET_COLOUR(w, RED);
                x = x->up;
            } else {
                if (w->right->colour == BLACK) {
                    NODE_SET_COLOUR(w->left, BLACK);
                    NODE_SET_COLOUR(w, RED);
                    rb_right_rotate(rootp, w, RBNULL);
                    w = x->up->right;
                }

                NODE_SET_COLOUR(w, x->up->colour);
                NODE_SET_COLOUR(x->up, BLACK);
                NODE_SET_COLOUR(w->right, BLACK);
                rb_left_rotate(rootp, x->up, RBNULL);
                x = *rootp;
            }
        } else {
            w = x->up->left;
            if (w->colour == RED) {
                NODE_SET_COLOUR(w, BLACK);
                NODE_SET_COLOUR(x->up, RED);
                rb_right_rotate(rootp, x->up, RBNULL);
                w = x->up->left;
            }

            if (w->right->colour == BLACK && w->left->colour == BLACK) {
                NODE_SET_COLOUR(w, RED);
                x = x->up;
            } else {
                if (w->left->colour == BLACK) {
                    NODE_SET_COLOUR(w->right, BLACK);
                    NODE_SET_COLOUR(w, RED);
                    rb_left_rotate(rootp, w, RBNULL);
                    w = x->up->left;
                }

                NODE_SET_COLOUR(w, x->up->colour);
                NODE_SET_COLOUR(x->up, BLACK);
                NODE_SET_COLOUR(w->left, BLACK);
                rb_right_rotate(rootp, x->up, RBNULL);
                x = *rootp;
            }
        }
    }

    NODE_SET_COLOUR(x, BLACK);
}

static void
rb_walk(
    const struct rbnode    *x,
    void                  (*action)(
        const void *,
        const VISIT,
        const int,
        void *),
    void                   *arg,
    int                     level,
    const struct rbnode    *RBNULL)
{
    if (x == RBNULL) {
        return;
    }

    if (x->left == RBNULL && x->right == RBNULL) {
        /* leaf */
        (*action)(x->key, leaf, level, arg);
    } else {
        (*action)(x->key, preorder, level, arg);

        rb_walk(x->left, action, arg, level+1, RBNULL);

        (*action)(x->key, postorder, level, arg);

        rb_walk(x->right, action, arg, level+1, RBNULL);

        (*action)(x->key, endorder, level, arg);
    }
}

static RBLIST *
rb_openlist(
    const struct rbnode    *rootp,
    const struct rbnode    *RBNULL)
{
    RBLIST *rblistp;

    rblistp = (RBLIST *)malloc(sizeof(RBLIST));
    if (!rblistp) {
        return(NULL);
    }

    rblistp->rootp = rootp;
    rblistp->nextp = rootp;
    rblistp->nullp = RBNULL;

    if (rootp != RBNULL) {
        while (rblistp->nextp->left != RBNULL) {
            rblistp->nextp = rblistp->nextp->left;
        }
    }

    return(rblistp);
}

static const void *
rb_readlist(
    RBLIST             *rblistp)
{
    const void *key = NULL;
    const struct rbnode *RBNULL;

    if (rblistp != NULL) {
        RBNULL = rblistp->nullp;
        if (rblistp->nextp != RBNULL) {
            key = rblistp->nextp->key;
            rblistp->nextp = rb_successor(rblistp->nextp, RBNULL);
        }
    }

    return(key);
}

static void
rb_closelist(
    RBLIST             *rblistp)
{
    if (rblistp) {
        free(rblistp);
    }
}

#if defined(USE_SBRK)
/* Allocate space for our nodes, allowing us to get space from
** sbrk in larger chucks.
*/
static struct rbnode *rbfreep = NULL;

#define RBNODEALLOC_CHUNK_SIZE 1000
static struct rbnode *
rb_alloc()
{
    struct rbnode *x;
    int i;

    if (rbfreep == NULL) {
        /* must grab some more space */
        rbfreep = ((struct rbnode *)
                   sbrk(sizeof(struct rbnode) * RBNODEALLOC_CHUNK_SIZE));

        if (rbfreep == (struct rbnode *) -1) {
            return(NULL);
        }

        /* tie them together in a linked list (use the up pointer) */
        for (i = 0, x = rbfreep; i < RBNODEALLOC_CHUNK_SIZE - 1; i++, x++) {
            x->up = (x + 1);
        }
        NODE_SET_UP(x, NULL);
    }

    x = rbfreep;
    rbfreep = rbfreep->up;
    return(x);
}

/* free (dealloc) an rbnode structure - add it onto the front of the list
** N.B. rbnode need not have been allocated through rb_alloc()
*/
static void
rb_free(struct rbnode *x)
{
    NODE_SET_UP(x, rbfreep);
    rbfreep = x;
}

#endif

#if 0
int
rb_check(struct rbnode *rootp)
{
    if (rootp == NULL || rootp == RBNULL) {
        return(0);
    }

    if (rootp->up != RBNULL) {
        fprintf(stderr, "Root up pointer not RBNULL");
        dumptree(rootp, 0);
        return(1);
    }

    if (rb_check1(rootp)) {
        dumptree(rootp, 0);
        return(1);
    }

    if (count_black(rootp) == -1) {
        dumptree(rootp, 0);
        return(-1);
    }

    return(0);
}

int
rb_check1(struct rbnode *x)
{
    if (x->left == NULL || x->right == NULL) {
        fprintf(stderr, "Left or right is NULL");
        return(1);
    }

    if (x->colour == RED) {
        if (x->left->colour != BLACK && x->right->colour != BLACK) {
            fprintf(stderr, "Children of red node not both black, x=%ld", x);
            return(1);
        }
    }

    if (x->left != RBNULL) {
        if (x->left->up != x) {
            fprintf(stderr, "x->left->up != x, x=%ld", x);
            return(1);
        }

        if (rb_check1(x->left)) {
            return(1);
        }
    }

    if (x->right != RBNULL) {
        if (x->right->up != x) {
            fprintf(stderr, "x->right->up != x, x=%ld", x);
            return(1);
        }

        if (rb_check1(x->right)) {
            return(1);
        }
    }
    return(0);
}

count_black(struct rbnode *x)
{
    int nleft, nright;

    if (x == RBNULL) {
        return(1);
    }

    nleft = count_black(x->left);
    nright = count_black(x->right);

    if (nleft == -1 || nright == -1) {
        return(-1);
    }

    if (nleft != nright) {
        fprintf(stderr, "Black count not equal on left & right, x=%ld", x);
        return(-1);
    }

    if (x->colour == BLACK) {
        nleft++;
    }

    return(nleft);
}

dumptree(struct rbnode *x, int n)
{
    char *prkey();

    if (x != NULL && x != RBNULL) {
        n++;
        fprintf(stderr, "Tree: %*s %ld: left=%ld, right=%ld, colour=%s, key=%s",
                n,
                "",
                x,
                x->left,
                x->right,
                (x->colour == BLACK) ? "BLACK" : "RED",
                prkey(x->key));

        dumptree(x->left, n);
        dumptree(x->right, n);
    }
}
#endif


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
