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
#ifndef _PYSILK_COMMON_H
#define _PYSILK_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_PYSILK_COMMON_H, "$SiLK: pysilk_common.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

/*
**  pysilk_common.h
**
**  Stuff shared in common between the pysilk module and the
**  silkpython module.
**
*/

/* Assumes that Python.h has been included before this */
#ifndef PY_MAJOR_VERSION
#error "Python.h was not included before pysilk_common.h"
#endif

/* Python 3 versus Python 2.x (x >= 6) */
#if PY_MAJOR_VERSION >= 3
/* Python 3.x version */
#  define IS_STRING(o) PyUnicode_Check(o)
#  define IS_INT(o) (PyLong_Check(o) && !PyBool_Check(o))
#  define PyInt_FromLong(o) PyLong_FromLong(o)
#  define PyInt_AsLong(o) PyLong_AsLong(o)
#  define LONG_AS_UNSIGNED_LONGLONG(o) PyLong_AsUnsignedLongLong(o)
#  define COBJ_CHECK(o) PyCapsule_CheckExact(o)
#  define COBJ_PTR(o) PyCapsule_GetPointer(o, NULL)
#  define COBJ_SETPTR(o, ptr) PyCapsule_SetPointer(o, ptr)
#  define COBJ_SETPTR_FAILED(rv) (rv != 0)
#  define COBJ_CREATE(ptr) PyCapsule_New(ptr, NULL, NULL)
#  define BYTES_FROM_XCHAR(s) bytes_from_wchar(s)
#  define BYTES_AS_STRING(s) PyBytes_AS_STRING(s)
#  define STRING_FROM_STRING(s) PyUnicode_FromString(s)
#  define BUILTINS "builtins"
typedef void *cmpfunc;
#else  /* PY_MAJOR_VERSION < 3 */
#  if PY_VERSION_HEX < 0x02060000
/* Python 2.[45] version */
#    define PyBytes_AS_STRING PyString_AS_STRING
#    define PyBytes_AsString PyString_AsString
#    define PyBytes_Check PyString_Check
#    define PyBytes_GET_SIZE PyString_GET_SIZE
#    define PyBytes_FromStringAndSize PyString_FromStringAndSize
#    define PyUnicode_FromString string_to_unicode
#    define PyUnicode_FromFormat format_to_unicode
#    define Py_TYPE(o) ((o)->ob_type)
#    define PyVarObject_HEAD_INIT(a, b) PyObject_HEAD_INIT(a) b,
#  endif  /* PY_VERSION_HEX < 0x02060000 */
/* Python 2.x version */
#  define IS_STRING(o) (PyBytes_Check(o) || PyUnicode_Check(o))
#  define IS_INT(o) ((PyInt_Check(o) && !PyBool_Check(o)) || PyLong_Check(o))
#  define LONG_AS_UNSIGNED_LONGLONG(o)          \
    (PyLong_Check(o) ?                          \
     PyLong_AsUnsignedLongLong(o) :             \
     PyLong_AsUnsignedLong(o))
#  define COBJ_CHECK(o) PyCObject_Check(o)
#  define COBJ_PTR(o) PyCObject_AsVoidPtr(o)
#  define COBJ_SETPTR(o, ptr) PyCObject_SetVoidPtr(o, ptr)
#  define COBJ_SETPTR_FAILED(rv) (rv == 0)
#  define COBJ_CREATE(ptr) PyCObject_FromVoidPtr(ptr, NULL)
#  define BYTES_FROM_XCHAR(s) PyString_FromString(s)
#  define STRING_FROM_STRING(s) PyString_FromString(s)
#  define BUILTINS "__builtin__"
#  define PyUnicode_InternFromString PyUnicode_FromString
#endif  /* PY_MAJOR_VERSION */

PyObject *
bytes_from_string(
    PyObject           *obj);
PyObject *
bytes_from_wchar(
    const wchar_t      *wc);
#if PY_VERSION_HEX < 0x02060000
PyObject *
string_to_unicode(
    const char         *s);
PyObject *
format_to_unicode(
    const char         *s,
    ...);
#endif

#ifdef __cplusplus
}
#endif
#endif /* _PYSILK_COMMON_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
