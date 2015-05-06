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

dnl RCSIDENT("$SiLK: ax_pkg_check_libfixbuf.m4 b7b8edebba12 2015-01-05 18:05:21Z mthomas $")


# ---------------------------------------------------------------------------
# AX_PKG_CHECK_LIBFIXBUF
#
#    Determine how to use libfixbuf.  Function takes two arguments:
#    minimum allowed version and too-new version.
#
#    Output variables:  FIXBUF_CFLAGS, FIXBUF_LDFLAGS,
#                       ENABLE_IPFIX
#
AC_DEFUN([AX_PKG_CHECK_LIBFIXBUF],[
    AC_SUBST([FIXBUF_CFLAGS])
    AC_SUBST([FIXBUF_LDFLAGS])

    # Either one or two arguments; first is minimum version required;
    # if a second is present, it represents a version that is "too
    # new"
    libfixbuf_required_version="$1"
    if test "x$2" = "x"
    then
        version_check="libfixbuf >= $1"
        report_version="libfixbuf.pc >= $1"
    else
        version_check="libfixbuf >= $1 libfixbuf < $2"
        report_version="libfixbuf.pc >= $1, libfixbuf.pc < $2"
    fi

    # the value in libfixbuf_have_version is printed as part of the
    # package summary
    libfixbuf_have_version=no
    ENABLE_IPFIX=0
    ENABLE_IPFIX_SFLOW=0

    # The configure switch
    sk_pkg_config=""
    AC_ARG_WITH([libfixbuf],[AS_HELP_STRING([--with-libfixbuf=DIR],
            [specify location of the libfixbuf IPFIX protocol package; find "libfixbuf.pc" in the directory DIR/ (i.e., prepend DIR to PKG_CONFIG_PATH).  The last component of DIR is likely "pkgconfig" [auto]])[]dnl
        ],[
            if test "x${withval}" != "xyes"
            then
                sk_pkg_config="${withval}"
            fi
    ])

    if test "x${sk_pkg_config}" = "xno"
    then
        AC_MSG_NOTICE([(${PACKAGE}) Building without IPFIX support at user request])
    else
        # prepend any argument to PKG_CONFIG_PATH
        if test "x${sk_pkg_config}" != "x"
        then
            sk_save_PKG_CONFIG_PATH="${PKG_CONFIG_PATH}"
            PKG_CONFIG_PATH="${sk_pkg_config}:${PKG_CONFIG_PATH}"
            export PKG_CONFIG_PATH
        fi

        # use pkg-config to check for libfixbuf existence
        PKG_CHECK_MODULES([LIBFIXBUF],
            [${version_check}],
            [ENABLE_IPFIX=1],[ENABLE_IPFIX=0])

        if test "x${ENABLE_IPFIX}" = "x0"
        then
            AC_MSG_NOTICE([(${PACKAGE}) Building without IPFIX support since pkg-config failed to find ${report_version}])
        else
            # verify that libfixbuf has any packages it depends on
            libfixbuf_reported_version=`${PKG_CONFIG} --modversion libfixbuf 2>/dev/null`
            if test "x${libfixbuf_reported_version}" = "x"
            then
                # PKG_CHECK_MODULES() says package is available, but
                # pkg-config does not find it; assume the user set the
                # LIBFIXBUF_LIBS/LIBFIXBUF_CFLAGS variables
                libfixbuf_reported_version=unknown
            else
                AC_MSG_CHECKING([presence of libfixbuf dependencies])
                echo "${as_me}:${LINENO}: \$PKG_CONFIG --libs libfixbuf >/dev/null 2>&AS_MESSAGE_LOG_FD" >&AS_MESSAGE_LOG_FD
                (${PKG_CONFIG} --libs libfixbuf) >/dev/null 2>&AS_MESSAGE_LOG_FD
                sk_pkg_status=$?
                echo "${as_me}:${LINENO}: \$? = ${sk_pkg_status}" >&AS_MESSAGE_LOG_FD

                if test 0 -eq ${sk_pkg_status}
                then
                    AC_MSG_RESULT([yes])
                else
                    AC_MSG_RESULT([no])
                    AC_MSG_NOTICE([(${PACKAGE}) Building without IPFIX support due to missing dependencies for libfixbuf. Details in config.log])
                    ENABLE_IPFIX=0
                fi
            fi
        fi

        # Restore the PKG_CONFIG_PATH to the saved value
        if test "x${sk_pkg_config}" != "x"
        then
            PKG_CONFIG_PATH="${sk_save_PKG_CONFIG_PATH}"
            export PKG_CONFIG_PATH
        fi
    fi

    # compile program that uses libfixbuf
    if test "x${ENABLE_IPFIX}" = "x1"
    then
        # Cache current values
        sk_save_LDFLAGS="${LDFLAGS}"
        sk_save_LIBS="${LIBS}"
        sk_save_CFLAGS="${CFLAGS}"
        sk_save_CPPFLAGS="${CPPFLAGS}"

        FIXBUF_LDFLAGS="${LIBFIXBUF_LIBS}"
        LIBS="${FIXBUF_LDFLAGS} ${LIBS}"

        FIXBUF_CFLAGS="${LIBFIXBUF_CFLAGS}"
        CPPFLAGS="${FIXBUF_CFLAGS} ${CPPFLAGS}"

        AC_MSG_CHECKING([usability of libfixbuf library and headers])
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM([
#include <fixbuf/public.h>
                ],[
fbInfoModel_t *m = fbInfoModelAlloc();
                 ])],[ENABLE_IPFIX=1],[ENABLE_IPFIX=0])

        if test "x${ENABLE_IPFIX}" = "x0"
        then
            AC_MSG_RESULT([no])
            AC_MSG_NOTICE([(${PACKAGE}) Building without IPFIX support since unable to compile program using libfixbuf. Details in config.log])
        else
            AC_MSG_RESULT([yes])
            AC_CHECK_FUNCS([fbTemplateGetContext fbCollectorSetSFlowTranslator])
            AC_CHECK_DECLS([FB_ENABLE_SCTP, HAVE_OPENSSL, HAVE_SPREAD])

            if test "x${ac_cv_func_fbCollectorSetSFlowTranslator}" = "xyes"
            then
                # version 1.6.0 or later
                libfixbuf_have_version="${libfixbuf_reported_version} >= 1.6.0"
                ENABLE_IPFIX_SFLOW=1
            elif test "x${ac_cv_func_fbTemplateGetContext}" = "xyes"
            then
                # version 1.4.0 or later
                libfixbuf_have_version="${libfixbuf_reported_version} >= 1.4.0"
            else
                libfixbuf_have_version="${libfixbuf_reported_version} >= 1.3.0"
            fi
        fi

        # Restore cached values
        LDFLAGS="${sk_save_LDFLAGS}"
        LIBS="${sk_save_LIBS}"
        CFLAGS="${sk_save_CFLAGS}"
        CPPFLAGS="${sk_save_CPPFLAGS}"
    fi

    if test "x${ENABLE_IPFIX}" = "x0"
    then
        FIXBUF_LDFLAGS=
        FIXBUF_CFLAGS=
    fi

    AM_CONDITIONAL(HAVE_FIXBUF, [test "x${ENABLE_IPFIX}" = "x1"])

    AC_DEFINE_UNQUOTED([ENABLE_IPFIX],[${ENABLE_IPFIX}],
        [Define to 1 to build with support for IPFIX.  Define to 0 otherwise.
         Requires libfixbuf-1.3 or later support and the
         <fixbuf/public.h> header file.])

    AC_DEFINE_UNQUOTED([ENABLE_IPFIX_SFLOW],[${ENABLE_IPFIX_SFLOW}],
        [Define to 1 to build with support for sFlow.  Define to 0 otherwise.
         Requires libfixbuf-1.6 support and the <fixbuf/public.h> header file.])

])# AX_PKG_CHECK_LIBFIXBUF


# ---------------------------------------------------------------------------
# AX_PKG_CHECK_LIBFIXBUF_PKGCONFIG
#
#    Run the part of --with-fixbuf that requires pkgconfig
#
AC_DEFUN([AX_PKG_CHECK_LIBFIXBUF_PKGCONFIG],[
])# AX_PKG_CHECK_LIBFIXBUF_PKGCONFIG

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
