/*
** Copyright (C) 2010-2014 by Carnegie Mellon University.
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
**  sktracemsg.h
**
**    Facility for including low-level debugging (tracing) messages in
**    code.  These messages will be compiled away depending on the
**    value of a macro.
**
*/
#ifndef _SKTRACEMSG_H
#define _SKTRACEMSG_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKTRACEMSG_H, "$SiLK: sktracemsg.h 412b51a029ce 2014-01-28 22:59:06Z mthomas $");

/*
**    The tracemsg facility works as follows:
**
**    Any module (e.g., a library, functional unit in a library,
**    application) author wishing to support tracing should have
**    somthing like the following in the top of the C file:
**
**      #ifdef <MODULE>_TRACE_LEVEL
**      #define TRACEMSG_LEVEL <MODULE>_TRACE_LEVEL
**      #endif
**      #define TRACEMSG(lvl, msg)                      \
**          TRACEMSG_TO_TRACEMSGLVL(lvl, msg)
**      #include <silk/sktracemsg.h>
**
**    Application authors may wish to use the following construct
**    instead:
**
**      // use TRACEMSG_LEVEL as our tracing variable
**      #define TRACEMSG(lvl, msg) TRACEMSG_TO_TRACEMSGLVL(lvl, msg)
**      #include <silk/sktracemsg.h>
**
**    since the application sources are only used by the individual
**    application.
**
**    For modules using sklog.h, it is very important that sklog.h is
**    included BEFORE sktracemsg.h.  If a module would not normally
**    include sklog.h, it should not be included.
**
**    The module should only check the value of the
**    <MODULE>_TRACE_LEVEL macro, it should NOT define the macro, so
**    that the developer can specify it on the gcc, make, or configure
**    command line:
**
**      gcc -DMODULE_TRACE_LEVEL=3
**
**      make CFLAGS='-DMODULE_TRACE_LEVEL=3'
**
**      configure ... CFLAGS='-DMODULE_TRACE_LEVEL=3'
**
**    The developer can set the GLOBAL_TRACE_LEVEL macro to enable all
**    TRACEMSG() macros:
**
**      gcc -DGLOBAL_TRACE_LEVEL=9
**
**
**    The module author can include tracing messages in the code,
**    where a sample message is:
**
**      TRACEMSG(level, ("magic value is %d", magic));
**
**    Note that 'level' is a literal integer between 1 and 9
**    inclusive.  The message format string and any arguments it
**    requires should be included in a set of parentheses so that the
**    preprocessor treats them as one argument.  (Longing for the day
**    when we can assume every compiler supports variatic macros.)
**    The message should not include a final newline; it is the
**    responsibility of the printf-like function that TRACEMSG
**    eventually calls to add the newline.
**
**    Level 1 represents the least amount of debugging and level 9 the
**    most, so set the level of the tracing messages in the code as
**    appropriate.
**
**    An alternate way for the module author to support tracing
**    messages is for the individual TRACEMSG() macros not to include
**    a level, for example:
**
**      TRACEMSG(("magic value is %d", magic));
**
**    (Note that we still need to use double parens.)  In this case,
**    the definition of TRACEMSG should provide the level for all
**    message in this module, that is:
**
**      #ifdef <MODULE>_TRACE_LEVEL
**      #define TRACEMSG_LEVEL 1
**      #endif
**      #define TRACEMSG(msg) TRACEMSG_TO_TRACEMSGLVL(1, msg)
**
**
**    Although this header requires the module author to define TRACEMSG(),
**    it allows the author to define it in the best way for the
**    module.  The author of a complex module may also choose to have
**    to multiple sets of tracing messages:
**
**      #ifdef FOO_TRACE_LEVEL
**      #define TRACEMSG_LEVEL 1
**      #define FOOTRACE(x) TRACEMSG_TO_TRACEMSGLVL(1, x)
**      #else
**      #define FOOTRACE(x)
**      #endif
**      #ifdef BAR_TRACE_LEVEL
**      #define TRACEMSG_LEVEL 1
**      #define BARTRACE(x) TRACEMSG_TO_TRACEMSGLVL(1, x)
**      #else
**      #define BARTRACE(x)
**      #endif
**
**
**    One shortcoming of TRACEMSG() is there is no way to
**    automatically prepend the module's name to the message.  That
**    is, this will not work:
**
**    #define TRACEMSG(msg)                           \
**        TRACEMSG_TO_TRACEMSGLVL(1, "module:" msg)        // WRONG!!
**
**
**    Mark Thomas
**    April 2010
*/


/*
 *    The following can be used to enable all TRACEMSG messages.  Do
 *    not define it here; let the developer set it on the gcc or make
 *    command line.
 */
/* #define GLOBAL_TRACE_LEVEL 9 */
#ifdef GLOBAL_TRACE_LEVEL
#  undef  TRACEMSG_LEVEL
#  define TRACEMSG_LEVEL GLOBAL_TRACE_LEVEL
#endif


/*
 *    The following macro converts a level and a message to a call to
 *    the TRACEMSGx() macro for level 'x'.
 *
 *    A module wishing to use TRACEMSG should define a macro in one of
 *    the two forms shown here and described in detail above:
 *
 *    #define TRACEMSG(l, m) TRACEMSG_TO_TRACEMSGLVL(l, m)
 *
 *    #define TRACEMSG(m) TRACEMSG_TO_TRACEMSGLVL(1, m)
 *
 */
#define TRACEMSG_TO_TRACEMSGLVL(trace_level, trace_msg)        \
    TRACEMSG ## trace_level (trace_msg)


/*
 *    The function that the macro invokes to print the message.  This
 *    can be defined to any printf-like function (in fact, sklog.h
 *    defines this to DEBUGMSG).  The function should print its args
 *    and include a final newline.  skTraceMsg() is declared in
 *    utils.h and defined in sku-app.c.
 */
#ifndef TRACEMSG_FUNCTION
#  define TRACEMSG_FUNCTION skTraceMsg
#endif


/*    Make certain the macro has a value */
#ifndef TRACEMSG_LEVEL
#  define TRACEMSG_LEVEL 0
#endif

/*    Compile away all TRACEMSG messages by default */
#define TRACEMSG0(trace_msg)    /* this never gets redefined */
#define TRACEMSG1(trace_msg)
#define TRACEMSG2(trace_msg)
#define TRACEMSG3(trace_msg)
#define TRACEMSG4(trace_msg)
#define TRACEMSG5(trace_msg)
#define TRACEMSG6(trace_msg)
#define TRACEMSG7(trace_msg)
#define TRACEMSG8(trace_msg)
#define TRACEMSG9(trace_msg)

/*    Enable the TRACEMSG for all appropriate levels */
#if TRACEMSG_LEVEL >= 1
#  undef  TRACEMSG1
#  define TRACEMSG1(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif
#if TRACEMSG_LEVEL >= 2
#  undef  TRACEMSG2
#  define TRACEMSG2(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif
#if TRACEMSG_LEVEL >= 3
#  undef  TRACEMSG3
#  define TRACEMSG3(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif
#if TRACEMSG_LEVEL >= 4
#  undef  TRACEMSG4
#  define TRACEMSG4(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif
#if TRACEMSG_LEVEL >= 5
#  undef  TRACEMSG5
#  define TRACEMSG5(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif
#if TRACEMSG_LEVEL >= 6
#  undef  TRACEMSG6
#  define TRACEMSG6(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif
#if TRACEMSG_LEVEL >= 7
#  undef  TRACEMSG7
#  define TRACEMSG7(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif
#if TRACEMSG_LEVEL >= 8
#  undef  TRACEMSG8
#  define TRACEMSG8(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif
#if TRACEMSG_LEVEL >= 9
#  undef  TRACEMSG9
#  define TRACEMSG9(trace_msg)  TRACEMSG_FUNCTION trace_msg
#endif

#ifdef __cplusplus
}
#endif
#endif /* _SKTRACEMSG_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
