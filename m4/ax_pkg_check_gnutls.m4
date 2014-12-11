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

dnl RCSIDENT("$SiLK: ax_pkg_check_gnutls.m4 1c87fe1a22f7 2014-01-06 15:24:30Z mthomas $")


# ---------------------------------------------------------------------------
# AX_PKG_CHECK_GNUTLS
#
#    Determine how to use GNUTLS.
#
#    Output variables:  GNUTLS_CFLAGS, GNUTLS_LDFLAGS, ENABLE_GNUTLS
#
#    Output definitions: HAVE_DECL_GNUTLS_CERT_EXPIRED
#
AC_DEFUN([AX_PKG_CHECK_GNUTLS],[
    AC_SUBST(GNUTLS_CFLAGS)
    AC_SUBST(GNUTLS_LDFLAGS)

    gnutls_required_version="$1"
    gnutls_declined=no
    if test "x$2" = "x"
    then
        version_check="gnutls >= ${gnutls_required_version}"
    else
        version_check="gnutls >= ${gnutls_required_version} gnutls < $2"
    fi

    ENABLE_GNUTLS=0

    m4_ifdef([PKG_CHECK_MODULES], [AX_PKG_CHECK_GNUTLS_PKGCONFIG])

    if test "x${ENABLE_GNUTLS}" = "x1"
    then
        # Cache current values
        sk_save_LDFLAGS="${LDFLAGS}"
        sk_save_LIBS="${LIBS}"
        sk_save_CFLAGS="${CFLAGS}"
        sk_save_CPPFLAGS="${CPPFLAGS}"

        # Add libgcrypt flags to gnutls flags
        GNUTLS_CFLAGS="${GNUTLS_CFLAGS} ${LIBGCRYPT_CFLAGS}"
        GNUTLS_LDFLAGS="${GNUTLS_LIBS} ${LIBGCRYPT_LDFLAGS}"

        LIBS="${GNUTLS_LDFLAGS} ${LIBS}"
        CFLAGS="${GNUTLS_CFLAGS} ${CFLAGS}"

        AC_MSG_CHECKING([usability of gnutls library and headers])
        AC_LINK_IFELSE(
                [AC_LANG_PROGRAM([
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gcrypt.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

gnutls_certificate_credentials_t cred;
GCRY_THREAD_OPTION_PTHREAD_IMPL;
                    ],[
gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
gnutls_global_init();
                     ])],[
                AC_MSG_RESULT([yes])
                ENABLE_GNUTLS=1
                ],[
                AC_MSG_RESULT([no])
                ENABLE_GNUTLS=0])

        if test "x${ENABLE_GNUTLS}" = "x1"
        then
            # This function went away in GnuTLS 3.0.0
            AC_CHECK_FUNCS([gnutls_transport_set_lowat])

            # This was added in GnuTLS 2.6.6
            AC_CHECK_DECLS([GNUTLS_CERT_EXPIRED], [], [], [[#include <gnutls/gnutls.h>]])
        fi

        # Restore cached values
        LDFLAGS="${sk_save_LDFLAGS}"
        LIBS="${sk_save_LIBS}"
        CFLAGS="${sk_save_CFLAGS}"
        CPPFLAGS="${sk_save_CPPFLAGS}"
    fi

    if test "x${ENABLE_GNUTLS}" = "x0"
    then
        GNUTLS_LDFLAGS=
        GNUTLS_CFLAGS=
        if test "x${gnutls_declined}" = "xyes"
        then
            AC_MSG_NOTICE([(${PACKAGE}) Building without GnuTLS support at user request])
        else
            AC_MSG_NOTICE([(${PACKAGE}) Building without GnuTLS support since pkg-config failed to find gnutls.pc >= ${gnutls_required_version}])
        fi
    fi

    AC_DEFINE_UNQUOTED([ENABLE_GNUTLS], [${ENABLE_GNUTLS}],
        [Define to 1 build with support for GnuTLS.  Define to 0 otherwise.
         Requires the GnuTLS library and the <gnutls/gnutls.h> header file.])
])# AX_PKG_CHECK_GNUTLS


# ---------------------------------------------------------------------------
# AX_PKG_CHECK_GNUTLS_PKGCONFIG
#
#    Run the part of --with-gnutls that requires pkgconfig
#
AC_DEFUN([AX_PKG_CHECK_GNUTLS_PKGCONFIG],[
    sk_pkg_config=""
    AC_ARG_WITH([gnutls],[AS_HELP_STRING([--with-gnutls=DIR],
            [specify location of the GnuTLS transport security package; find "gnutls.pc" in the directory DIR/ (i.e., prepend DIR to PKG_CONFIG_PATH).  The last component of DIR is likely "pkgconfig" [auto]])[]dnl
        ],[
            if test "x${withval}" != "xyes"
            then
                sk_pkg_config="${withval}"
            fi
    ])

    if test "x${sk_pkg_config}" = "xno"
    then
        gnutls_declined=yes
    else
        AX_CHECK_LIBGCRYPT

        if test "x${sk_pkg_config}" != "x"
        then
            sk_save_PKG_CONFIG_PATH="${PKG_CONFIG_PATH}"
            PKG_CONFIG_PATH="${sk_pkg_config}:${PKG_CONFIG_PATH}"
            export PKG_CONFIG_PATH
        fi

        # check for gnutls existence
        PKG_CHECK_MODULES([GNUTLS],
            [${version_check}],
            [ENABLE_GNUTLS=1], [ENABLE_GNUTLS=0])

        # verify that gnutls has the packages it needs
        if test "x${ENABLE_GNUTLS}" = "x1"
        then
            AC_MSG_CHECKING([presence of gnutls dependencies])
            echo "${as_me}:${LINENO}: \$PKG_CONFIG --libs gnutls >/dev/null 2>&AS_MESSAGE_LOG_FD" >&AS_MESSAGE_LOG_FD
            (${PKG_CONFIG} --libs gnutls) >/dev/null 2>&AS_MESSAGE_LOG_FD
            sk_pkg_status=$?
            echo "${as_me}:${LINENO}: \$? = ${sk_pkg_status}" >&AS_MESSAGE_LOG_FD

            if test 0 -eq ${sk_pkg_status}
            then
                AC_MSG_RESULT([yes])
            else
                AC_MSG_RESULT([no])
                AC_MSG_NOTICE([(${PACKAGE}) Building without GnuTLS support due to missing dependencies for gnutls. Details in config.log])
                ENABLE_GNUTLS=0
            fi
        fi

        if test "x${sk_pkg_config}" != "x"
        then
            PKG_CONFIG_PATH="${sk_save_PKG_CONFIG_PATH}"
            export PKG_CONFIG_PATH
        fi
    fi
])# AX_PKG_CHECK_GNUTLS_PKGCONFIG


# ---------------------------------------------------------------------------
# AX_CHECK_LIBGCRYPT
#
#    Determine how to use libgcrypt.  Used when configuring gnutls
#    since pkgconfig/gnutls.pc may not include -lgcrypt (which makes
#    me wonder what the point on pkgconfig is).
#
AC_DEFUN([AX_CHECK_LIBGCRYPT],[
    AC_ARG_WITH([libgcrypt],[AS_HELP_STRING([--with-libgcrypt-config=CONFIG_PROG],
            [specify location of the libgcrypt cryptographic library configuration program; find "libgcrypt-config" at CONFIG_PROG [auto]])[]dnl
        ],[
            if test "x${withval}" != "xyes"
            then
                libgcrypt_config="${withval}"
            fi
    ])

    ENABLE_LIBGCRYPT=0
    if test "x${libgcrypt_config}" = "x"
    then
        AC_PATH_PROG([IGNORE_LIBGCRYPT_CONFIG], [libgcrypt-config], [no])
        libgcrypt_config=${IGNORE_LIBGCRYPT_CONFIG}
    fi

    if test "x${libgcrypt_config}" != "xno"
    then
        # Just extract the values but don't check them here; we'll let
        # gnutls do the checking
        LIBGCRYPT_LDFLAGS=`${libgcrypt_config} --libs 2>/dev/null`
        LIBGCRYPT_CFLAGS=`${libgcrypt_config} --cflags 2>/dev/null`
    fi
])# AX_CHECK_LIBGCRYPT

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
