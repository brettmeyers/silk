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

dnl RCSIDENT("$SiLK: ax_pkg_check_libipa.m4 1c87fe1a22f7 2014-01-06 15:24:30Z mthomas $")


# ---------------------------------------------------------------------------
# AX_PKG_CHECK_LIBIPA
#
#    Determine how to use IPA
#
#    Output variables: LIBIPA_CFLAGS, LIBIPA_LDFLAGS
#    Output definitions: ENABLE_IPA
#
AC_DEFUN([AX_PKG_CHECK_LIBIPA],[
    AC_SUBST([LIBIPA_CFLAGS])
    AC_SUBST([LIBIPA_LDFLAGS])

    libipa_required_version="$1"
    libipa_declined=no
    ENABLE_IPA=0

    m4_ifdef([PKG_CHECK_MODULES], [AX_PKG_CHECK_LIBIPA_PKGCONFIG])

    if test "x${ENABLE_IPA}" = "x1"
    then
        # Cache current values
        sk_save_LDFLAGS="${LDFLAGS}"
        sk_save_LIBS="${LIBS}"
        sk_save_CFLAGS="${CFLAGS}"
        sk_save_CPPFLAGS="${CPPFLAGS}"

        LIBIPA_LDFLAGS="${LIBIPA_LIBS}"
        LIBS="${LIBIPA_LDFLAGS} ${LIBS}"

        CPPFLAGS="${LIBIPA_CFLAGS} ${CPPFLAGS}"

        AC_MSG_CHECKING([usability of libipa library and headers])
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM([
#include <ipa/ipa.h>
                ],[
IPAContext *ctx;
                 ])],[
            AC_MSG_RESULT([yes])
            ENABLE_IPA=1
            ],[
            AC_MSG_RESULT([no])
            ENABLE_IPA=0])

        # Restore cached values
        LDFLAGS="${sk_save_LDFLAGS}"
        LIBS="${sk_save_LIBS}"
        CFLAGS="${sk_save_CFLAGS}"
        CPPFLAGS="${sk_save_CPPFLAGS}"
    fi

    if test "x${ENABLE_IPA}" = "x0"
    then
        LIBIPA_LDFLAGS=
        LIBIPA_CFLAGS=
        if test "x${libipa_declined}" = "xyes"
        then
            AC_MSG_NOTICE([(${PACKAGE}) Building without IPA support at user request])
        else
            AC_MSG_NOTICE([(${PACKAGE}) Building without IPA support since pkg-config failed to find libipa.pc >= ${libipa_required_version}])
        fi
    fi

    AC_DEFINE_UNQUOTED([ENABLE_IPA], [${ENABLE_IPA}],
        [Define to 1 to build with support for IPA.  Define to 0 otherwise.
         Requires the libipa library and the <ipa/ipa.h> header file.])
    AM_CONDITIONAL(HAVE_IPA, [test "x${ENABLE_IPA}" = "x1"])
])# AX_PKG_CHECK_LIBIPA


# ---------------------------------------------------------------------------
# AX_PKG_CHECK_LIBIPA_PKGCONFIG
#
#    Run the part of --with-ipa that requires pkgconfig
#
AC_DEFUN([AX_PKG_CHECK_LIBIPA_PKGCONFIG],[
    sk_pkg_config=""
    AC_ARG_WITH([libipa],[AS_HELP_STRING([--with-libipa=DIR],
            [specify location of the IPA IP-address annotation package; find "libipa.pc" in the directory DIR/ (i.e., prepend DIR to PKG_CONFIG_PATH).  The last component of DIR is likely "pkgconfig" [auto]])[]dnl
        ],[
            if test "x${withval}" != "xyes"
            then
                sk_pkg_config="${withval}"
            fi
    ])

    if test "x${sk_pkg_config}" = "xno"
    then
        libipa_declined=yes
    else
        if test "x${sk_pkg_config}" != "x"
        then
            sk_save_PKG_CONFIG_PATH="${PKG_CONFIG_PATH}"
            PKG_CONFIG_PATH="${sk_pkg_config}:${PKG_CONFIG_PATH}"
            export PKG_CONFIG_PATH
        fi

        # check for libipa existence
        PKG_CHECK_MODULES([LIBIPA],
            [libipa >= ${libipa_required_version}],
            [ENABLE_IPA=1], [ENABLE_IPA=0])

        # verify that libipa has the packages it needs
        if test "x${ENABLE_IPA}" = "x1"
        then
            AC_MSG_CHECKING([presence of libipa dependencies])
            echo "${as_me}:${LINENO}: \$PKG_CONFIG --libs libipa >/dev/null 2>&AS_MESSAGE_LOG_FD" >&AS_MESSAGE_LOG_FD
            (${PKG_CONFIG} --libs libipa) >/dev/null 2>&AS_MESSAGE_LOG_FD
            sk_pkg_status=$?
            echo "${as_me}:${LINENO}: \$? = ${sk_pkg_status}" >&AS_MESSAGE_LOG_FD

            if test 0 -eq ${sk_pkg_status}
            then
                AC_MSG_RESULT([yes])
            else
                AC_MSG_RESULT([no])
                AC_MSG_NOTICE([(${PACKAGE}) Building without IPA support due to missing dependencies for libipa. Details in config.log])
                ENABLE_IPA=0
            fi
        fi

        if test "x${sk_pkg_config}" != "x"
        then
            PKG_CONFIG_PATH="${sk_save_PKG_CONFIG_PATH}"
            export PKG_CONFIG_PATH
        fi
    fi
])# AX_PKG_CHECK_LIBIPA_PKGCONFIG

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
