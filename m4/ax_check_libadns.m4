dnl Copyright (C) 2004-2014 by Carnegie Mellon University.
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

dnl RCSIDENT("$SiLK: ax_check_libadns.m4 1c87fe1a22f7 2014-01-06 15:24:30Z mthomas $")

# ---------------------------------------------------------------------------
# AX_CHECK_LIBADNS
#
#    Try to find the ADNS (Asynchronous DNS) library.
#
#    Output variables: ADNS_CFLAGS ADNS_LDFLAGS
#    Output definition: HAVE_ADNS_H
#
AC_DEFUN([AX_CHECK_LIBADNS],[
    AC_SUBST(ADNS_CFLAGS)
    AC_SUBST(ADNS_LDFLAGS)

    AC_ARG_WITH([adns],[AS_HELP_STRING([--with-adns=ADNS_DIR],
            [specify location of the ADNS asynchronous DNS library; find "adns.h" in ADNS_DIR/include/; find "libadns.so" in ADNS_DIR/lib/ [auto]])[]dnl
        ],[
            if test "x$withval" != "xyes"
            then
                adns_dir="$withval"
                adns_includes="$adns_dir/include"
                adns_libraries="$adns_dir/lib"
            fi
    ])
    AC_ARG_WITH([adns-includes],[AS_HELP_STRING([--with-adns-includes=DIR],
            [find "adns.h" in DIR/ (overrides ADNS_DIR/include/)])[]dnl
        ],[
            if test "x$withval" = "xno"
            then
                adns_dir=no
            elif test "x$withval" != "xyes"
            then
                adns_includes="$withval"
            fi
    ])
    AC_ARG_WITH([adns-libraries],[AS_HELP_STRING([--with-adns-libraries=DIR],
            [find "libadns.so" in DIR/ (overrides ADNS_DIR/lib/)])[]dnl
        ],[
            if test "x$withval" = "xno"
            then
                adns_dir=no
            elif test "x$withval" != "xyes"
            then
                adns_libraries="$withval"
            fi
    ])

    ENABLE_ADNS=0
    if test "x$adns_dir" != "xno"
    then
        # Cache current values
        sk_save_LDFLAGS="$LDFLAGS"
        sk_save_LIBS="$LIBS"
        sk_save_CFLAGS="$CFLAGS"
        sk_save_CPPFLAGS="$CPPFLAGS"

        if test "x$adns_libraries" != "x"
        then
            ADNS_LDFLAGS="-L$adns_libraries"
            LDFLAGS="$ADNS_LDFLAGS $sk_save_LDFLAGS"
        fi

        if test "x$adns_includes" != "x"
        then
            ADNS_CFLAGS="-I$adns_includes"
            CPPFLAGS="$ADNS_CFLAGS $sk_save_CPPFLAGS"
        fi

        AC_CHECK_LIB([adns], [adns_init],
            [ENABLE_ADNS=1 ; ADNS_LDFLAGS="$ADNS_LDFLAGS -ladns"])

        if test "x$ENABLE_ADNS" = "x1"
        then
            AC_CHECK_HEADER([adns.h], , [
                AC_MSG_WARN([Found libadns but not adns.h.  Maybe you should install adns-devel?])
                ENABLE_ADNS=0])
        fi

        if test "x$ENABLE_ADNS" = "x1"
        then
            AC_MSG_CHECKING([usability of ADNS library and headers])
            LDFLAGS="$sk_save_LDFLAGS"
            LIBS="$ADNS_LDFLAGS $sk_save_LIBS"
            AC_LINK_IFELSE(
                [AC_LANG_PROGRAM([
#include <adns.h>
                    ],[
adns_state adns;
adns_query q;
int rv;

rv = adns_init(&adns, (adns_initflags)0, 0);
rv = adns_submit(adns, "255.255.255.255.in-addr.arpa", adns_r_ptr,
                 (adns_queryflags)(adns_qf_quoteok_cname|adns_qf_cname_loose),
                 NULL, &q);
                     ])],[
                AC_MSG_RESULT([yes])],[
                AC_MSG_RESULT([no])
                ENABLE_ADNS=0])
        fi

        # Restore cached values
        LDFLAGS="$sk_save_LDFLAGS"
        LIBS="$sk_save_LIBS"
        CFLAGS="$sk_save_CFLAGS"
        CPPFLAGS="$sk_save_CPPFLAGS"
    fi

    if test "x$ENABLE_ADNS" != "x1"
    then
        ADNS_LDFLAGS=
        ADNS_CFLAGS=
    else
        AC_DEFINE([HAVE_ADNS_H], 1,
            [Define to 1 include support for ADNS (asynchronous DNS).
             Requires the ADNS library and the <adns.h> header file.])
    fi
])# AX_CHECK_LIBADNS

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
