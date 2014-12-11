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
**  silk_types.h
**
**    A place to gather commonly used defines, typedefs, and enumerations.
**
*/

#ifndef _SILK_TYPES_H
#define _SILK_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SILK_TYPES_H, "$SiLK: silk_types.h 412b51a029ce 2014-01-28 22:59:06Z mthomas $");

/**
 *  @file
 *
 *    The typedefs and some common macros used throughout the SiLK
 *    code base.
 *
 *    This file is part of libsilk.
 */


/* *****  IP ADDRESS / SOCKADDR  ********************************** */

/**
 *    The IP address structure.  Most code should use skipaddr_t
 *    instead of ipUnion.  Users should not deference directly, use
 *    the macros specified in skipaddr.h to get/set the value.
 */
typedef union ipUnion_un {
    uint32_t    ipu_ipv4;
#if SK_ENABLE_IPV6
    uint8_t     ipu_ipv6[16];
#endif
} ipUnion;

/**
 *    An IP address structure that knows the version of IP address it
 *    contains.  Do not deference directly, instead use the macros
 *    specified in skipaddr.h to get/set the value
 */
typedef struct skipaddr_st {
    ipUnion     ip_ip;
#if SK_ENABLE_IPV6
    unsigned    ip_is_v6 :1;
#endif
} skipaddr_t;

/**
 *    Flags that determine the string representation of an IP as
 *    returned by skipaddrString() and other functions declared in
 *    utils.h
 */
typedef enum {
    /** Canonical form: dotted quad for IPv4 or hexadectet for
     * IPv6. This uses inet_ntop(3) which prints ::ffff:0:0/96 and
     * some parts of ::/96 using a mixture of IPv6 and IPv4. */
    SKIPADDR_CANONICAL,
    /** Similar to the canonical form, except the value is fully
     * padded; useful for string comparisons */
    SKIPADDR_ZEROPAD,
    /** Value as an integer number, printed as decimal */
    SKIPADDR_DECIMAL,
    /** Value as an integer number, printed as hexadecimal */
    SKIPADDR_HEXADECIMAL,
    /** Similar to the canonical format of IPv6, but never uses a
     * mixture of IPv4 and IPv6, and IPv4 addresses are always printed
     * in the ::ffff:0:0/96 netblock. */
    SKIPADDR_FORCE_IPV6
} skipaddr_flags_t;

/**
 *    How to handle IPv6 Flow records.
 */
typedef enum sk_ipv6policy_en {
    /** completely ignore IPv6 flows */
    SK_IPV6POLICY_IGNORE = -2,
    /** convert IPv6 flows to IPv4 if possible, else ignore */
    SK_IPV6POLICY_ASV4 = -1,
    /** mix IPv4 and IPv6 flows in the result--this is the default */
    SK_IPV6POLICY_MIX = 0,
    /** force IPv4 flows to be converted to IPv6 */
    SK_IPV6POLICY_FORCE = 1,
    /** only return IPv6 flows that were marked as IPv6 */
    SK_IPV6POLICY_ONLY = 2
} sk_ipv6policy_t;

/**
 *    Length of buffer required to hold an IPv6 address.  This is
 *    taken from INET6_ADDRSTRLEN used by inet_ntop(), which can
 *    return "0000:0000:0000:0000:0000:00FF:000.000.000.000"
 */
#define SK_NUM2DOT_STRLEN 46

/**
 *    A special structure of IP Addresses.  It is defined in utils.h
 */
typedef struct skIPWildcard_st skIPWildcard_t;


/**
 *    A union that encompasses the various struct sockaddr types.
 *    Macros and functions for manipulating these are in utils.h.
 */
typedef union sk_sockaddr_un {
    struct sockaddr     sa;
    struct sockaddr_in  v4;
    struct sockaddr_in6 v6;
    struct sockaddr_un  un;
} sk_sockaddr_t;

/**
 *    A structure that represents multiple representations of an
 *    address and/or port.  Macros and functions for manipulating
 *    these are in utils.h.
 */
typedef struct sk_sockaddr_array_st {
    char          *name;
    sk_sockaddr_t *addrs;
    uint32_t       num_addrs;
} sk_sockaddr_array_t;



/* *****  FLOW RECORDS (RWREC)  *********************************** */

/**
 *    The generic SiLK Flow record returned from ANY file format
 *    containing packed SiLK Flow records.  It is defined in rwrec.h
 */
typedef struct rwGenericRec_V5_st rwGenericRec_V5;
typedef rwGenericRec_V5 rwRec;

/**
 *    The maximum size of a SiLK Flow record.
 */
#define SK_MAX_RECORD_SIZE 96

/**
 *    Number of possible SNMP interface index values
 */
#define SK_SNMP_INDEX_LIMIT   65536



/* *****  STREAM / FILE FORMATS  ********************************** */

/**
 *    Interface to a file containing SiLK Data---flow records or
 *    IPsets, etc---is an skstream_t.  See skstream.h.
 */
typedef struct skstream_st skstream_t;

/**
 *    Type to hold the ID of the various SiLK file formats.  The
 *    format IDs begin with FT_ and are listed in silk_files.h.
 */
typedef uint8_t  fileFormat_t;

/**
 *    The strlen() of the names of file formats will be this size or
 *    less.
 */
#define SK_MAX_STRLEN_FILE_FORMAT   32

/**
 *    A version of the file format.
 */
typedef uint8_t  fileVersion_t;

/**
 *    Value meaning that any file version is valid
 */
#define SK_RECORD_VERSION_ANY       ((fileVersion_t)0xFF)

/**
 *    The compression method used to write the data section of a file.
 *    The known compression methods are listed in silk_files.h.
 */
typedef uint8_t sk_compmethod_t;

/**
 *    The value for an invalid or unrecognized compression method
 */
#define SK_INVALID_COMPMETHOD       ((sk_compmethod_t)0xFF)

/**
 *    Values that specify how a stream/file is to be opened.
 */
typedef enum {
    SK_IO_READ = 1,
    SK_IO_WRITE = 2,
    SK_IO_APPEND = 4
} skstream_mode_t;

/**
 *    What type of content the stream contains
 */
typedef enum {
    /** stream contains line-oriented text */
    SK_CONTENT_TEXT = (1 << 0),
    /** stream contains a SiLK file header and SiLK Flow data */
    SK_CONTENT_SILK_FLOW = (1 << 1),
    /** stream contains a SiLK file header and data (non-Flow data) */
    SK_CONTENT_SILK = (1 << 2),
    /** stream contains binary data other than SiLK */
    SK_CONTENT_OTHERBINARY = (1 << 3)
} skcontent_t;



/* *****  CLASS / TYPE / SENSORS  ********************************* */

/* Most of the functions for manipulating these are declared in
 * sksite.h */

/**
 *    Type to hold a class ID.  A class is not actually stored in
 *    packed records (see flowtypeID_t).
 */
typedef uint8_t classID_t;

/**
 *    The maximum number of classes that may be allocated.  (All valid
 *    class IDs must be less than this number.)
 */
#define SK_MAX_NUM_CLASSES          ((classID_t)32)

/**
 *    The value for an invalid or unrecognized class.
 */
#define SK_INVALID_CLASS            ((classID_t)0xFF)

/**
 *    A flowtype is a class/type pair.  It has a unique name and
 *    unique ID.
 */
typedef uint8_t  flowtypeID_t;

/**
 *    The maximum number of flowtypes that may be allocated.  (All
 *    valid flowtype IDs must be less than this number.)
 */
#define SK_MAX_NUM_FLOWTYPES        ((flowtypeID_t)0xFF)

/**
 *    The value for an invalid or unrecognized flow-type value
 */
#define SK_INVALID_FLOWTYPE         ((flowtypeID_t)0xFF)

/**
 *    The strlen() of the names of flowtypes, classes, and types will
 *    be this size or less.  Add 1 to allow for the NUL byte.
 */
#define SK_MAX_STRLEN_FLOWTYPE      32

/**
 *    Type to hold a sensor ID.  Usually, a sensor is a router or
 *    other flow collector.
 */
typedef uint16_t sensorID_t;

/**
 *    The maximum number of sensors that may be allocated.  (All valid
 *    sensor IDs must be less than this number.
 */
#define SK_MAX_NUM_SENSORS          ((sensorID_t)0xFFFF)

/**
 *    The value for an invalid or unrecognized sensor.
 */
#define SK_INVALID_SENSOR           ((sensorID_t)0xFFFF)

/**
 *    The maximum length of a sensor name, not including the final
 *    NUL.
 */
#define SK_MAX_STRLEN_SENSOR        64

/**
 *    Type to hold a sensor group ID.  This is not actually stored in
 *    packed records.
 */
typedef uint8_t sensorgroupID_t;

/**
 *    The maximum number of sensorgroups that may be allocated.  (All
 *    valid sensorgroup IDs must be less than this number.)
 */
#define SK_MAX_NUM_SENSORGROUPS     ((sensorgroupID_t)0xFF)

/**
 *    The value for an invalid or unrecognized sensor.
 */
#define SK_INVALID_SENSORGROUP      ((sensorgroupID_t)0xFF)



/* *****  BITMPAP / LINKED-LIST / VECTOR  ************************* */

/**
 *    Bitmap of integers.  It is defined in utils.h.
 */
typedef struct sk_bitmap_st sk_bitmap_t;


/**
 *    Signature of a doubly-linked list.  See skdllist.h.
 */
struct sk_dllist_st;
typedef struct sk_dllist_st      sk_dllist_t;

/**
 *    Signature of an iterator for a doubly linked list
 */
struct sk_dll_iter_st;
typedef struct sk_dll_iter_st sk_dll_iter_t;
struct sk_dll_iter_st {
    void           *data;
    sk_dll_iter_t  *link[2];
};

/**
 *    A stringmap maps strings to integer IDs.  It is used for parsing
 *    the user's argument to --fields.  See skstringmap.h.
 */
typedef sk_dllist_t sk_stringmap_t;

/**
 *    Growable array.  See skvector.h.
 */
typedef struct sk_vector_st sk_vector_t;



/* *****  MISCELLANEOUS  ****************************************** */

/**
 *    sktime_t is milliseconds since the UNIX epoch.  Macros and
 *    functions for manipulating these are in utils.h.
 */
typedef int64_t sktime_t;

/**
 *    Minimum size of buffer to pass to sktimestamp_r().
 */
#define SKTIMESTAMP_STRLEN 28


/**
 *    An enumeration type for endianess.
 */
typedef enum silk_endian_en {
    SILK_ENDIAN_BIG,
    SILK_ENDIAN_LITTLE,
    SILK_ENDIAN_NATIVE,
    SILK_ENDIAN_ANY
} silk_endian_t;


/**
 *    The status of an iterator.
 */
typedef enum skIteratorStatus_en {
    /** More entries */
    SK_ITERATOR_OK=0,
    /** No more entries */
    SK_ITERATOR_NO_MORE_ENTRIES
} skIteratorStatus_t;

/**
 *    The type of message functions.  These should use the same
 *    semantics as printf.
 */
typedef int (*sk_msg_fn_t)(const char *, ...)
    SK_CHECK_TYPEDEF_PRINTF(1, 2);

/**
 *    The type of message functions with the arguments expanded to a
 *    variable argument list.
 */
typedef int (*sk_msg_vargs_fn_t)(const char *, va_list)
    SK_CHECK_TYPEDEF_PRINTF(1, 0);

#ifdef __cplusplus
}
#endif
#endif /* _SILK_TYPES_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
