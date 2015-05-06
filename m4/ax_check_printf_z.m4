dnl Copyright (C) 2011-2015 by Carnegie Mellon University.
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

dnl RCSIDENT("$SiLK: ax_check_printf_z.m4 b7b8edebba12 2015-01-05 18:05:21Z mthomas $")

# ---------------------------------------------------------------------------
# AX_CHECK_PRINTF_Z
#
#    Determine what format string to use for size_t and ssize_t
#
AC_DEFUN([AX_CHECK_PRINTF_Z],[
    AC_MSG_CHECKING([whether printf understands the "z" modifier])
    sk_save_CFLAGS="${CFLAGS}"
    CFLAGS="${WARN_CFLAGS} ${sk_werror} ${CFLAGS}"
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([[
#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <stddef.h>
#else
#  ifdef  HAVE_STDLIB_H
#    include <stdlib.h>
#  endif
#  ifdef  HAVE_MALLOC_H
#    include <malloc.h>
#  endif
#endif
#ifdef    HAVE_STRING_H
#  if     !defined STDC_HEADERS && defined HAVE_MEMORY_H
#    include <memory.h>
#  endif
#  include <string.h>
#endif
#ifdef    HAVE_STRINGS_H
#  include <strings.h>
#endif
            ]], [[
char a[128];
char b[128];
size_t s = (size_t)0xfedcba98;
sprintf(a, "%zu", s);
sprintf(b, "%lu", (unsigned long)s);
return strcmp(a, b);
            ]])
    ],[
         AC_MSG_RESULT([yes])
         AC_DEFINE([HAVE_PRINTF_Z_FORMAT], [1], [Define to 1 if printf supports the "z" modifier])
    ],[
         AC_MSG_RESULT([no])
    ],[
         AC_MSG_RESULT([assuming no])
    ])
    CFLAGS="${sk_save_CFLAGS}"
])# AX_CHECK_PRINTF_Z

dnl Local Variables:
dnl mode:autoconf
dnl indent-tabs-mode:nil
dnl End:
