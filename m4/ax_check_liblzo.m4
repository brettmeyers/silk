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

dnl RCSIDENT("$SiLK: ax_check_liblzo.m4 1c87fe1a22f7 2014-01-06 15:24:30Z mthomas $")


# ---------------------------------------------------------------------------
# AX_CHECK_LIBLZO
#
#    Try to find the lzo library.
#
#    Version 2 of LZO (released May-2005) puts the headers into an
#    "lzo" subdirectory (DarwinPorts uses an lzo2 subdirectory) while
#    version 1 did not.
#
#    Output defines: ENABLE_LZO, LZO_HEADER_NAME
#    Conditionals: HAVE_LZO
#
AC_DEFUN([AX_CHECK_LIBLZO],[
    ENABLE_LZO=0

    lzo_header_names="lzo2/lzo1x.h lzo/lzo1x.h lzo1x.h"
    lzo_library_names="lzo2 lzo"

    AC_ARG_WITH([lzo],
        AS_HELP_STRING([--with-lzo=LZO_DIR],
            [specify location of the LZO file compression library; find "lzo2/lzo1x.h", "lzo/lzo1x.h", or "lzo1x.h", in LZO_DIR/include/; find "liblzo2.so" or "liblzo.so" in LZO_DIR/lib/ [auto]]),
        [
            if test "x$withval" != "xyes"
            then
                lzo_dir="$withval"
                lzo_includes="$lzo_dir/include"
                lzo_libraries="$lzo_dir/lib"
            fi
    ])
    AC_ARG_WITH([lzo-includes],[AS_HELP_STRING([--with-lzo-includes=DIR],
            [find "lzo1x.h" in DIR/ (overrides LZO_DIR/include/ and disables searching lzo2 and lzo subdirectories)])[]dnl
        ],[
            if test "x$withval" = "xno"
            then
                lzo_dir=no
            elif test "x$withval" != "xyes"
            then
                lzo_includes="$withval"
                lzo_header_names="lzo1x.h"
            fi
    ])
    AC_ARG_WITH([lzo-libraries],[AS_HELP_STRING([--with-lzo-libraries=DIR],
            [find "liblzo2.so" or "liblzo.so" in DIR/ (overrides LZO_DIR/lib/)])[]dnl
        ],[
            if test "x$withval" = "xno"
            then
                lzo_dir=no
            elif test "x$withval" != "xyes"
            then
                lzo_libraries="$withval"
            fi
    ])

    ENABLE_LZO=0
    if test "x$lzo_dir" != "xno"
    then
        # Cache current values
        sk_save_LDFLAGS="$LDFLAGS"
        sk_save_LIBS="$LIBS"
        sk_save_CFLAGS="$CFLAGS"
        sk_save_CPPFLAGS="$CPPFLAGS"

        if test "x$lzo_libraries" != "x"
        then
            LZO_LDFLAGS="-L$lzo_libraries"
            LDFLAGS="$LZO_LDFLAGS $sk_save_LDFLAGS"
        fi

        if test "x$lzo_includes" != "x"
        then
            LZO_CFLAGS="-I$lzo_includes"
            CPPFLAGS="$LZO_CFLAGS $sk_save_CPPFLAGS"
        fi

        for sk_lzo_hdr in $lzo_header_names
        do
            AC_CHECK_HEADER([$sk_lzo_hdr], [
                sk_lzo_hdr="<$sk_lzo_hdr>"
                ENABLE_LZO=1
                break])
        done

        if test "x$ENABLE_LZO" = "x1"
        then
            #AC_MSG_CHECKING([for version of lzo])
            AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[
#include $sk_lzo_hdr
#if LZO_VERSION < 0x2000
#  error "version 1"
#endif
]])],
               , [lzo_library_names="lzo"])

            ENABLE_LZO=0

            # Loop over the library names
            AC_SEARCH_LIBS([lzo1x_1_15_compress],[$lzo_library_names],[ENABLE_LZO=1])
            if test "x$ENABLE_LZO" = "x1"
            then
                case "(X$ac_cv_search_lzo1x_1_15_compress" in *X-l*)
                    LZO_LDFLAGS="$LZO_LDFLAGS $ac_cv_search_lzo1x_1_15_compress" ;;
                esac
            fi
        fi

        if test "x$ENABLE_LZO" = "x1"
        then
            AC_MSG_CHECKING([usability of lzo library and headers])
            LDFLAGS="$sk_save_LDFLAGS"
            LIBS="$LZO_LDFLAGS $sk_save_LIBS"
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM([
#include $sk_lzo_hdr
                ],[
const lzo_bytep src;
lzo_bytep dst;
lzo_uint src_len;
lzo_uintp dst_len;
lzo_voidp wrkmem;

lzo1x_1_15_compress (src, src_len, dst, dst_len, wrkmem);
                ])], [AC_MSG_RESULT([yes])], [
                    AC_MSG_RESULT([no])
                    ENABLE_LZO=0])
        fi

        if test "x$ENABLE_LZO" = "x1"
        then
            [sk_lzo_asm_hdr=`echo "$sk_lzo_hdr" | sed 's/lzo1x/lzo_asm/' | sed 's/[<>]//g'`]
            AC_CHECK_HEADER($sk_lzo_asm_hdr, [
                sk_lzo_asm_hdr="<$sk_lzo_asm_hdr>"
                AC_CHECK_FUNCS([lzo1x_decompress_asm_fast_safe])
            ])
        fi

        # Restore cached values
        LDFLAGS="$sk_save_LDFLAGS"
        LIBS="$sk_save_LIBS"
        CFLAGS="$sk_save_CFLAGS"
        CPPFLAGS="$sk_save_CPPFLAGS"
    fi

    if test "x$ENABLE_LZO" = "x0"
    then
        LZO_LDFLAGS=
        LZO_CFLAGS=
    else
        AC_DEFINE_UNQUOTED([LZO_HEADER_NAME], [$sk_lzo_hdr],
            [When SK_ENABLE_LZO is set, this is the path to the lzo1x.h header file])
        AC_DEFINE_UNQUOTED([LZO_ASM_HEADER_NAME], [$sk_lzo_asm_hdr],
            [When SK_HAVE_LZO1X_DECOMPRESS_ASM_FAST_SAFE is defined, this is the path to the lzo_asm.h header file])
    fi

    AC_DEFINE_UNQUOTED([ENABLE_LZO], [$ENABLE_LZO],
        [Define to 1 to build with support for LZO compression.
         Define to 0 otherwise.  Requires the liblzo or liblzo2 library
         and the <lzo1x.h> header file.])
    AM_CONDITIONAL(HAVE_LZO,  [test "x$ENABLE_LZO" = x1])
])# AX_CHECK_LIBLZO

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
