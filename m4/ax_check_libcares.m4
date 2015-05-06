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

dnl RCSIDENT("$SiLK: ax_check_libcares.m4 b7b8edebba12 2015-01-05 18:05:21Z mthomas $")

# ---------------------------------------------------------------------------
# AX_CHECK_LIBCARES
#
#    A bit of confusion here: The package we need is c-ares.  The
#    header is named <ares.h>, but the library is libcares.
#
#    Try to find the CARES (Asynchronous DNS) library.
#
#    Output variables: CARES_CFLAGS CARES_LDFLAGS
#    Output definition: HAVE_CARES_H
#
AC_DEFUN([AX_CHECK_LIBCARES],[
    AC_SUBST(CARES_CFLAGS)
    AC_SUBST(CARES_LDFLAGS)

    AC_ARG_WITH([c-ares],[AS_HELP_STRING([--with-c-ares=CARES_DIR],
            [specify location of the c-ares asynchronous DNS library; find "ares.h" in CARES_DIR/include/; find "libcares.so" in CARES_DIR/lib/ [auto]])[]dnl
        ],[
            if test "x$withval" != "xyes"
            then
                cares_dir="$withval"
                cares_includes="$cares_dir/include"
                cares_libraries="$cares_dir/lib"
            fi
    ])
    AC_ARG_WITH([c-ares-includes],[AS_HELP_STRING([--with-c-ares-includes=DIR],
            [find "ares.h" in DIR/ (overrides CARES_DIR/include/)])[]dnl
        ],[
            if test "x$withval" = "xno"
            then
                cares_dir=no
            elif test "x$withval" != "xyes"
            then
                cares_includes="$withval"
            fi
    ])
    AC_ARG_WITH([c-ares-libraries],[AS_HELP_STRING([--with-c-ares-libraries=DIR],
            [find "libcares.so" in DIR/ (overrides CARES_DIR/lib/)])[]dnl
        ],[
            if test "x$withval" = "xno"
            then
                cares_dir=no
            elif test "x$withval" != "xyes"
            then
                cares_libraries="$withval"
            fi
    ])

    ENABLE_CARES=0
    if test "x$cares_dir" != "xno"
    then
        # Cache current values
        sk_save_LDFLAGS="$LDFLAGS"
        sk_save_LIBS="$LIBS"
        sk_save_CFLAGS="$CFLAGS"
        sk_save_CPPFLAGS="$CPPFLAGS"

        if test "x$cares_libraries" != "x"
        then
            CARES_LDFLAGS="-L$cares_libraries"
            LDFLAGS="$CARES_LDFLAGS $sk_save_LDFLAGS"
        fi

        if test "x$cares_includes" != "x"
        then
            CARES_CFLAGS="-I$cares_includes"
            CPPFLAGS="$CARES_CFLAGS $sk_save_CPPFLAGS"
        fi

        AC_CHECK_LIB([cares], [ares_init],
            [ENABLE_CARES=1 ; CARES_LDFLAGS="$CARES_LDFLAGS -lcares"])

        if test "x$ENABLE_CARES" = "x1"
        then
            AC_CHECK_HEADER([ares.h], , [
                AC_MSG_WARN([Found libcares but not ares.h.  Maybe you should install c-ares-devel?])
                ENABLE_CARES=0])
        fi

        if test "x$ENABLE_CARES" = "x1"
        then
            AC_MSG_CHECKING([usability of C-ARES library and headers])
            LDFLAGS="$sk_save_LDFLAGS"
            LIBS="$CARES_LDFLAGS $sk_save_LIBS"
            AC_LINK_IFELSE(
                [AC_LANG_PROGRAM([
#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#include <ares.h>
                    ],[
ares_channel ares;
int rv;

rv = ares_init(&ares);
rv = ares_fds(ares, NULL, NULL);
ares_process(ares, NULL, NULL);
ares_cancel(ares);
                     ])],[
                AC_MSG_RESULT([yes])],[
                AC_MSG_RESULT([no])
                ENABLE_CARES=0])
        fi

        # Restore cached values
        LDFLAGS="$sk_save_LDFLAGS"
        LIBS="$sk_save_LIBS"
        CFLAGS="$sk_save_CFLAGS"
        CPPFLAGS="$sk_save_CPPFLAGS"
    fi

    if test "x$ENABLE_CARES" != "x1"
    then
        CARES_LDFLAGS=
        CARES_CFLAGS=
    else
        AC_DEFINE([HAVE_CARES_H], 1,
            [Define to 1 include support for C-ARES (asynchronous DNS).
             Requires the CARES library and the <ares.h> header file.])
    fi
])# AX_CHECK_LIBCARES

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
