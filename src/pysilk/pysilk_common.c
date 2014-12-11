/*
** Copyright (C) 2011-2014 by Carnegie Mellon University.
**
** @OPENSOURCE_HEADER_START@
**
** Use of the SILK system and related source code is subject to the terms
** of the following licenses:
**
** GNU Public License (GPL) Rights pursuant to Version 2, June 1991
** Government Purpose License Rights (GPLR) pursuant to DFARS 252.227.7013
**
** NO WARRANTY
**
** ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER
** PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY
** PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN
** "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY
** KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT
** LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE,
** MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE
** OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT,
** SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY
** TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF
** WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES.
** LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF
** CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON
** CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE
** DELIVERABLES UNDER THIS LICENSE.
**
** Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie
** Mellon University, its trustees, officers, employees, and agents from
** all claims or demands made against them (and any related losses,
** expenses, or attorney's fees) arising out of, or relating to Licensee's
** and/or its sub licensees' negligent use or willful misuse of or
** negligent conduct or willful misconduct regarding the Software,
** facilities, or other rights or assistance granted by Carnegie Mellon
** University under this License, including, but not limited to, any
** claims of product liability, personal injury, death, damage to
** property, or violation of any laws or regulations.
**
** Carnegie Mellon University Software Engineering Institute authored
** documents are sponsored by the U.S. Department of Defense under
** Contract FA8721-05-C-0003. Carnegie Mellon University retains
** copyrights in all material produced under this contract. The U.S.
** Government retains a non-exclusive, royalty-free license to publish or
** reproduce these documents, or allow others to do so, for U.S.
** Government purposes only pursuant to the copyright license under the
** contract clause at 252.227.7013.
**
** @OPENSOURCE_HEADER_END@
*/

/*
**  Common functions for pysilk and silkpython
**
*/


#include <Python.h>
#include <silk/silk.h>

RCSIDENT("$SiLK: pysilk_common.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "pysilk_common.h"

/* FUNCTION DEFINITIONS */

PyObject *
bytes_from_string(
    PyObject           *obj)
{
    PyObject *bytes;

    if (PyBytes_Check(obj)) {
        Py_INCREF(obj);
        return obj;
    }
    bytes = PyUnicode_AsASCIIString(obj);
    return bytes;
}

PyObject *
bytes_from_wchar(
    const wchar_t      *wc)
{
    PyObject *bytes;
    PyObject *pstr = PyUnicode_FromWideChar(wc, -1);
    if (pstr == NULL) {
        return NULL;
    }
    bytes = bytes_from_string(pstr);
    Py_DECREF(pstr);

    return bytes;
}

#if PY_VERSION_HEX < 0x02060000

PyObject *
string_to_unicode(
    const char         *s)
{
    return PyUnicode_DecodeUTF8(s, strlen(s), "strict");
}

SK_DIAGNOSTIC_FORMAT_NONLITERAL_PUSH

PyObject *
format_to_unicode(
    const char         *s,
    ...)
{
    va_list ap;
    PyObject *str;
    PyObject *uni;

    va_start(ap, s);
    str = PyString_FromFormatV(s, ap);
    va_end(ap);

    if (str == NULL) {
        return NULL;
    }
    uni = PyUnicode_FromObject(str);
    Py_DECREF(str);

    return uni;
}

SK_DIAGNOSTIC_FORMAT_NONLITERAL_POP

#endif

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
