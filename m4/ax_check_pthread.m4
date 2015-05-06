dnl Copyright (C) 2004-2015 by Carnegie Mellon University.
dnl
dnl @OPENSOURCE_HEADER_START@
dnl
dnl Use of the SILK system and related source code is subject to the terms
dnl of the following licenses:
dnl
dnl GNU Public License (GPL) Rights pursuant to Version 2, June 1991
dnl Government Purpose License Rights (GPLR) pursuant to DFARS 252.227.7013
dnl
dnl NO WARRANTY
dnl
dnl ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER
dnl PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY
dnl PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN
dnl "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY
dnl KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT
dnl LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE,
dnl MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE
dnl OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT,
dnl SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY
dnl TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF
dnl WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES.
dnl LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF
dnl CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON
dnl CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE
dnl DELIVERABLES UNDER THIS LICENSE.
dnl
dnl Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie
dnl Mellon University, its trustees, officers, employees, and agents from
dnl all claims or demands made against them (and any related losses,
dnl expenses, or attorney's fees) arising out of, or relating to Licensee's
dnl and/or its sub licensees' negligent use or willful misuse of or
dnl negligent conduct or willful misconduct regarding the Software,
dnl facilities, or other rights or assistance granted by Carnegie Mellon
dnl University under this License, including, but not limited to, any
dnl claims of product liability, personal injury, death, damage to
dnl property, or violation of any laws or regulations.
dnl
dnl Carnegie Mellon University Software Engineering Institute authored
dnl documents are sponsored by the U.S. Department of Defense under
dnl Contract FA8721-05-C-0003. Carnegie Mellon University retains
dnl copyrights in all material produced under this contract. The U.S.
dnl Government retains a non-exclusive, royalty-free license to publish or
dnl reproduce these documents, or allow others to do so, for U.S.
dnl Government purposes only pursuant to the copyright license under the
dnl contract clause at 252.227.7013.
dnl
dnl @OPENSOURCE_HEADER_END@

dnl RCSIDENT("$SiLK: ax_check_pthread.m4 b7b8edebba12 2015-01-05 18:05:21Z mthomas $")


# ---------------------------------------------------------------------------
# AX_CHECK_PTHREAD
#
#    Determine how to use pthreads.  In addition, determine whether
#    pthreads supports read/write locks.
#
#    Output variables: PTHREAD_LDFLAGS
#    Output definition: HAVE_PTHREAD_RWLOCK
#
AC_DEFUN([AX_CHECK_PTHREAD],[
    AC_SUBST(PTHREAD_LDFLAGS)

    AC_MSG_CHECKING([for pthread linker flags])

    # cache current LIBS
    sk_save_LIBS="$LIBS"

    # pthreads requires libexc on OSF/1 or TruUNIX or Tru64 or
    # whatever it is called by DEC or Compaq or HP or whatever they
    # are calling themselves now

    for sk_pthread in "X" "X-pthread" "X-lpthread" "X-lpthread -lexc"
    do
        sk_pthread=`echo $sk_pthread | sed 's/^X//'`
        LIBS="$sk_pthread $sk_save_LIBS"

        # This is a RUN because Solaris will successfully link the
        # program and just leave out the pthread calls!
        AC_RUN_IFELSE([
            AC_LANG_PROGRAM([
#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

static void *dummy(void *v)
{
  return v;
}
                ],[
pthread_t p;
int x = 0;
void *xp = NULL;

pthread_create(&p, NULL, dummy, &x);
pthread_join(p, &xp);
if (xp != &x) return 1;
])],[
            PTHREAD_LDFLAGS="$sk_pthread"
            if test "x$sk_pthread" = "x"
            then
                AC_MSG_RESULT([none required])
            else
                AC_MSG_RESULT([$PTHREAD_LDFLAGS])
            fi
            break])
    done


    AC_MSG_CHECKING([for pthread read/write locks])

    # add pthread library to the saved LIBS
    LIBS="$PTHREAD_LDFLAGS $sk_save_LIBS"

    # This is a RUN because Solaris will successfully link the
    # program and just leave out the pthread calls!
    AC_RUN_IFELSE([
        AC_LANG_PROGRAM([
#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#define FAILIF(x) ++test; if (x) { fprintf(stderr, "Failed test %d (%s) (rv = %d) on line %d\n", test, #x, rv, __LINE__); return 1; }
            ],[
    pthread_rwlock_t lock;
    int rv;
    int test;
    test = 0;
    rv = pthread_rwlock_init(&lock, NULL);
    FAILIF(rv != 0);
    rv = pthread_rwlock_tryrdlock(&lock);
    FAILIF(rv != 0);
    rv = pthread_rwlock_trywrlock(&lock);
    FAILIF(rv != EBUSY);
    rv = pthread_rwlock_tryrdlock(&lock);
    FAILIF(rv != 0);
    rv = pthread_rwlock_trywrlock(&lock);
    FAILIF(rv != EBUSY);
    rv = pthread_rwlock_unlock(&lock);
    FAILIF(rv != 0);
    rv = pthread_rwlock_trywrlock(&lock);
    FAILIF(rv != EBUSY);
    rv = pthread_rwlock_unlock(&lock);
    FAILIF(rv != 0);
    rv = pthread_rwlock_trywrlock(&lock);
    FAILIF(rv != 0);
    rv = pthread_rwlock_tryrdlock(&lock);
    FAILIF(rv != EDEADLK && rv != EBUSY);
    rv = pthread_rwlock_trywrlock(&lock);
    FAILIF(rv != EDEADLK && rv != EBUSY);
    rv = pthread_rwlock_unlock(&lock);
    FAILIF(rv != 0);
    rv = pthread_rwlock_tryrdlock(&lock);
    FAILIF(rv != 0);
    rv = pthread_rwlock_unlock(&lock);
    FAILIF(rv != 0);
    rv = pthread_rwlock_destroy(&lock);
    FAILIF(rv != 0);
    return 0;
            ])],[
           AC_MSG_RESULT([yes])
           AC_DEFINE([HAVE_PTHREAD_RWLOCK], 1,
                     [Define to 1 if your system has working pthread read/write locks])
            ],[
           AC_MSG_RESULT([no])
        ])

    # restore libs
    LIBS="$sk_save_LIBS"
])# AX_CHECK_PTHREAD

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
