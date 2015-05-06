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

dnl RCSIDENT("$SiLK: ax_check_libpcap.m4 b7b8edebba12 2015-01-05 18:05:21Z mthomas $")


# ---------------------------------------------------------------------------
# AX_CHECK_LIBPCAP
#
#    Determine how to use pcap.  Output variable: PCAP_LDFLAGS
#    Output definition: HAVE_PCAP_H, HAVE_PCAPPCAP_H
#
AC_DEFUN([AX_CHECK_LIBPCAP],[
    AC_SUBST(PCAP_LDFLAGS)

    AC_ARG_WITH([pcap],[AS_HELP_STRING([--with-pcap=PCAP_DIR],
            [specify location of the PCAP packet capture library; find "pcap.h" in PCAP_DIR/include/; find "libpcap.so" in PCAP_DIR/lib/ [auto]])[]dnl
        ],[
            if test "x$withval" != "xyes"
            then
                pcap_dir="$withval"
                pcap_includes="$pcap_dir/include"
                pcap_libraries="$pcap_dir/lib"
            fi
    ])
    AC_ARG_WITH([pcap-includes],[AS_HELP_STRING([--with-pcap-includes=DIR],
            [find "pcap.h" in DIR/ (overrides PCAP_DIR/include/)])[]dnl
        ],[
            if test "x$withval" = "xno"
            then
                pcap_dir=no
            elif test "x$withval" != "xyes"
            then
                pcap_includes="$withval"
            fi
    ])
    AC_ARG_WITH([pcap-libraries],[AS_HELP_STRING([--with-pcap-libraries=DIR],
            [find "libpcap.so" in DIR/ (overrides PCAP_DIR/lib/)])[]dnl
        ],[
            if test "x$withval" = "xno"
            then
                pcap_dir=no
            elif test "x$withval" != "xyes"
            then
                pcap_libraries="$withval"
            fi
    ])

    ENABLE_PCAP=0
    if test "x$pcap_dir" != "xno"
    then
        # Cache current values
        sk_save_LDFLAGS="$LDFLAGS"
        sk_save_LIBS="$LIBS"
        sk_save_CFLAGS="$CFLAGS"
        sk_save_CPPFLAGS="$CPPFLAGS"

        if test "x$pcap_libraries" != "x"
        then
            PCAP_LDFLAGS="-L$pcap_libraries"
            LDFLAGS="$PCAP_LDFLAGS $sk_save_LDFLAGS"
        fi

        if test "x$pcap_includes" != "x"
        then
            PCAP_CFLAGS="-I$pcap_includes"
            CPPFLAGS="$PCAP_CFLAGS $sk_save_CPPFLAGS"
        fi

        # look for -lpcap or -lwpcap
        AC_SEARCH_LIBS([pcap_open_dead], [pcap wpcap], [ENABLE_PCAP=1])
        if test "x$ENABLE_PCAP" = "x1"
        then
            case "(X$ac_cv_search_pcap_open_dead" in *X-l*)
                PCAP_LDFLAGS="$PCAP_LDFLAGS $ac_cv_search_pcap_open_dead" ;;
            esac
        fi

        if test "x$ENABLE_PCAP" != "x1"
        then
            AC_MSG_NOTICE([(${PACKAGE}) Not building rwp* tools due to missing pcap library])
        else
            # look for header as pcap.h and pcap/pcap.h
            ENABLE_PCAP=0
            AC_CHECK_HEADERS([pcap.h pcap/pcap.h], [ENABLE_PCAP=1 ; break])

            if test "x$ENABLE_PCAP" != "x1"
            then
                AC_MSG_NOTICE([(${PACKAGE}) Not building rwp* tools: Found libpcap but not pcap.h. Maybe you need to install pcap-devel?])
            fi
        fi

        if test "x$ENABLE_PCAP" = "x1"
        then
            AC_MSG_CHECKING([usability of PCAP library and headers])
            LDFLAGS="$sk_save_LDFLAGS"
            LIBS="$PCAP_LDFLAGS $sk_save_LIBS"
            AC_LINK_IFELSE(
                [AC_LANG_PROGRAM([
#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else
#include <pcap.h>
#endif
                    ],[
pcap_t *pc;
u_char *pkt;
struct pcap_pkthdr hdr;
pcap_dumper_t *dump;

pc = pcap_open_dead(0, 0);
dump = pcap_dump_open(pc, "/dev/null");
pkt = (u_char*)(pcap_next(pc, &hdr));
pcap_dump((u_char*)dump, &hdr, pkt);
pcap_dump_flush(dump);
                     ])],[
                AC_MSG_RESULT([yes])],[
                AC_MSG_RESULT([no])
                ENABLE_PCAP=0])
        fi

        if test "x$ENABLE_PCAP" != "x1"
        then
            AC_MSG_NOTICE([(${PACKAGE}) Not building rwp* tools due to error using pcap library and header. Details in config.log])
        fi

        # Restore cached values
        LDFLAGS="$sk_save_LDFLAGS"
        LIBS="$sk_save_LIBS"
        CFLAGS="$sk_save_CFLAGS"
        CPPFLAGS="$sk_save_CPPFLAGS"
    fi

    if test "x$ENABLE_PCAP" != "x1"
    then
        PCAP_LDFLAGS=
        PCAP_CFLAGS=
    else
        AC_DEFINE([HAVE_PCAP_H], 1,
            [Define to 1 if you have the <pcap.h> header file.])
    fi

    AM_CONDITIONAL(HAVE_PCAP, [test "x$ENABLE_PCAP" = x1])

])# AX_CHECK_LIBPCAP

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
