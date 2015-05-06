/*
** Copyright (C) 2001-2015 by Carnegie Mellon University.
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
#ifndef _RWASCII_H
#define _RWASCII_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWASCII_H, "$SiLK: rwascii.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/silk_types.h>
#include <silk/skstringmap.h>


/**
 *  @file
 *
 *    Functions for printing a SiLK Flow record (an rwRec) as text.
 *
 *    This file is part of libsilk.
 */


/* ***  Printing rwRec as ASCII  *** */

/**
 *    Number of fields we can print; should be one more than the last
 *    ID in rwrec_printable_fields_t
 */
#define  RWREC_PRINTABLE_FIELD_COUNT  26

/**
 *    Maximum width of the name of a field
 */
#define RWREC_PRINTABLE_MAX_NAME_LEN 16

/**
 *    An identifier for each field
 */
typedef enum {
    RWREC_FIELD_SIP,
    RWREC_FIELD_DIP,
    RWREC_FIELD_SPORT,
    RWREC_FIELD_DPORT,
    RWREC_FIELD_PROTO,
    RWREC_FIELD_PKTS,
    RWREC_FIELD_BYTES,
    RWREC_FIELD_FLAGS,
    RWREC_FIELD_STIME,
    RWREC_FIELD_ELAPSED,
    RWREC_FIELD_ETIME,
    RWREC_FIELD_SID,
    RWREC_FIELD_INPUT,
    RWREC_FIELD_OUTPUT,
    RWREC_FIELD_NHIP,
    RWREC_FIELD_INIT_FLAGS,
    RWREC_FIELD_REST_FLAGS,
    RWREC_FIELD_TCP_STATE,
    RWREC_FIELD_APPLICATION,
    RWREC_FIELD_FTYPE_CLASS,
    RWREC_FIELD_FTYPE_TYPE,
    RWREC_FIELD_STIME_MSEC,
    RWREC_FIELD_ETIME_MSEC,
    RWREC_FIELD_ELAPSED_MSEC,
    RWREC_FIELD_ICMP_TYPE,
    RWREC_FIELD_ICMP_CODE
} rwrec_printable_fields_t;


/**
 *    A type for printing records as ASCII.  Created via
 *    rwAsciiStreamCreate()
 */
typedef struct rwAsciiStream_st rwAsciiStream_t;


/**
 *    A callback function used by fields that are not built-in.  This
 *    callback will be invoked by rwAsciiPrintTitles(), or by the
 *    first call to print a record if titles have not yet been
 *    printed.
 *
 *    The function should fill 'text_buf' with the title of the field.
 *    'text_buf_size' is the size of the 'text_buf' buffer.  'cb_data'
 *    is the 'callback_data' that was specified when the callback was
 *    added.
 */
typedef void (*rwAsciiStreamGetTitle_t)(
    char        *text_buf,
    size_t       text_buf_size,
    void        *cb_data);


/**
 *    A callback function used by fields that are not built-in.  This
 *    callback will be invoked by rwAsciiPrintRec().
 *
 *    The function should fill 'text_buf' with the value of the field
 *    for the 'rwrec' passed to rwAsciiPrintRec().  'text_buf_size' is
 *    the size of the 'text_buf' buffer.  'cb_data' is the
 *    'callback_data' that was specified when the callback was added.
 *
 *    The return value of this function is ignored by rwAsciiStream.
 */
typedef int (*rwAsciiStreamGetValue_t)(
    const rwRec *rwrec,
    char        *text_buf,
    size_t       text_buf_size,
    void        *cb_data);


/**
 *    A callback function used by fields that are not built-in.  This
 *    callback will be invoked by rwAsciiPrintRecExtra().
 *
 *    The function should fill 'text_buf' with the value of the field
 *    for the 'rwrec' and 'extra' values passed to
 *    rwAsciiPrintRecExtra().  'text_buf_size' is the size of the
 *    'text_buf' buffer.  'cb_data' is the 'callback_data' that was
 *    specified when the callback was added.
 *
 *    The return value of this function is ignored by rwAsciiStream.
 */
typedef int (*rwAsciiStreamGetValueExtra_t)(
    const rwRec *rwrec,
    char        *text_buf,
    size_t       text_buf_size,
    void        *cb_data,
    void        *extra);


/**
 *  Create a new output rwAsciiStream for printing rwRec records in a
 *  human readable form. Store the newly allocated rwAsciiStream_t in
 *  the memory pointed to by **astream.  Return 0 on success or
 *  non-zero if allocation fails.
 *
 *  The caller may immediately call rwAsciiPrintRec() to print records
 *  to the 'astream', or the caller may use the functions listed below
 *  to change the defaults.  Calling an rwAsciiSet*() function once
 *  titles or records have been printed may result in strange output.
 *
 *  The defaults are (function that modifies this behavior):
 *  -- Output is sent to the standard output (rwAsciiSetOutputHandle)
 *  -- All fields are printed (rwAsciiAppendFields)
 *  -- Column titles are printed before first record (rwAsciiSetNoTitles)
 *  -- Fields are printed in a columnar format (rwAsciiSetNoColumns)
 *  -- Times are printed as "2009/09/09T09:09:09.009" (rwAsciiSetTimestampFlags)
 *  -- Delimiter between columns is '|' (rwAsciiSetDelimiter)
 *  -- A delimiter is printed after the final column
 *     (rwAsciiSetNoFinalDelimiter)
 *  -- A newline is printed after final column (rwAsciiSetNoNewline)
 *  -- IPs are printed in canonical form (rwAsciiSetIntegerIps,
 *     rwAsciiSetZeroPadIps)
 *  -- Sensor names are printed (rwAsciiSetIntegerSensors)
 *  -- Characters are used for TCP Flags (rwAsciiSetIntegerTcpFlags)
 *  -- No special handling of ICMP (rwAsciiSetIcmpTypeCode)
 */
int
rwAsciiStreamCreate(
    rwAsciiStream_t   **astream);

/**
 *    Free all memory associated with the 'astream'.  It is the
 *    caller's responsibility to fflush() the underlying file pointer.
 *    Does nothing if 'astream' or the location it points to is NULL.
 */
void
rwAsciiStreamDestroy(
    rwAsciiStream_t   **astream);

/**
 *    Print 'rwrec' in a human-readable form to 'astream'.  Will print
 *    the column titles when the 'astream' is configured to have
 *    titles and the titles have not yet been printed.
 */
void
rwAsciiPrintRec(
    rwAsciiStream_t    *astream,
    const rwRec        *rwrec);


/**
 *    Similar to rwAsciiPrintRec(), except it includes an extra
 *    argument that will be passed unchanged to any callback functions
 *    registered with rwAsciiAppendCallbackFieldExtra().
 */
void
rwAsciiPrintRecExtra(
    rwAsciiStream_t    *astream,
    const rwRec        *rwrec,
    void               *extra);

/**
 *    Print the column titles when the 'astream' is configured to have
 *    titles and they have not been printed.
 */
void
rwAsciiPrintTitles(
    rwAsciiStream_t    *astream);

/**
 *    Configure the 'astream' to print the output to 'fh'.  If 'fh' is
 *    NULL, stdout is used.
 */
void
rwAsciiSetOutputHandle(
    rwAsciiStream_t    *astream,
    FILE               *fh);


/**
 *    Configure the 'astream' to print the built-in fields listed in
 *    'field_list', which is an array contains 'field_count' values
 *    from the 'rwrec_printable_fields_t' enumeration.  These fields
 *    will be printed after any fields that the 'astream' has already
 *    been instructed to print.
 *
 *    If 'field_list' is NULL or 'field_count' is 0, return non-zero.
 *    Return non-zero if the list of fields can be increased to handle
 *    the added fields.
 */
int
rwAsciiAppendFields(
    rwAsciiStream_t    *astream,
    const uint32_t     *field_list,
    uint32_t            field_count);


/**
 *    Like rwAsciiAppendFields(), but appends a single field.
 */
int
rwAsciiAppendOneField(
    rwAsciiStream_t    *astream,
    const uint32_t      field_id);


/**
 *    Configure the 'astream' to generate a textual field value for a
 *    SiLK Flow record by invoking callbacks.
 *
 *    The 'get_title_fn' callback is used to get the title of field.
 *    The stream will invoke get_title_fn(buf, bufsize, callback_data)
 *    to generate the title for the field.
 *
 *    'get_value_fn' is used to get the value of the field.  The
 *    stream will invoke get_value_fn(rec, buf, bufsize,
 *    callback_data) to generate the value for the feild given the
 *    specified record 'rec'.
 *
 *    'callback_data' will be passed unchanged into the callbacks for
 *    the callbacks to use as they wish.  It may be NULL.
 *
 *    'width' is the width the column should have when columnar output
 *    is active.  Note that the bufsize passed into the callbacks is
 *    the size of 'buf', not the column width.
 *
 *    This field will be printed after any fields that the 'astream'
 *    has already been instructed to print.
 *
 *    If the callbacks are NULL, return non-zero.  Return non-zero if
 *    the list of fields cannot be increased to handle this field.
 */
int
rwAsciiAppendCallbackField(
    rwAsciiStream_t            *astream,
    rwAsciiStreamGetTitle_t     get_title_fn,
    rwAsciiStreamGetValue_t     get_value_fn,
    void                       *callback_data,
    uint32_t                    width);


/**
 *    Similar to rwAsciiAppendCallbackField(), except the callback to
 *    get the value, 'get_value_extra_fn', accepts an additional
 *    argument, which is the 'extra' argument passed to the
 *    rwAsciiPrintRecExtra() function.
 */
int
rwAsciiAppendCallbackFieldExtra(
    rwAsciiStream_t                *astream,
    rwAsciiStreamGetTitle_t         get_title_fn,
    rwAsciiStreamGetValueExtra_t    get_value_extra_fn,
    void                           *callback_data,
    uint32_t                        width);


/**
 *    Configure the 'astream' not to print titles before the first
 *    record of output.
 */
void
rwAsciiSetNoTitles(
    rwAsciiStream_t    *astream);

/**
 *    Configure the 'astream' to put the character 'delimiter' between
 *    columns.  Does not effect the columnar setting.
 */
void
rwAsciiSetDelimiter(
    rwAsciiStream_t    *astream,
    char                delimiter);

/**
 *    Configure the 'astream' not to use fixed size columns.  This
 *    avoids extra whitespace, but makes the output difficult for
 *    humans to read.
 */
void
rwAsciiSetNoColumns(
    rwAsciiStream_t    *astream);

/**
 *    Configure the 'astream' not to print a newline after the final
 *    field.
 */
void
rwAsciiSetNoNewline(
    rwAsciiStream_t    *astream);

/**
 *    Configure the 'astream' not to print a delimiter after the final
 *    field.
 */
void
rwAsciiSetNoFinalDelimiter(
    rwAsciiStream_t    *astream);

/**
 *    Set the argument that the 'astream' will pass to
 *    skipaddrString() to print the IP addresses.
 */
void
rwAsciiSetIPFormatFlags(
    rwAsciiStream_t    *astream,
    uint32_t            ip_format);

/**
 *    Configure how the 'astream' handles IPv6 flows.
 *
 *    CURRENTLY THIS IS USED ONLY TO DETERMINE THE COLUMN WIDTH OF IP
 *    COLUMNS.
 */
void
rwAsciiSetIPv6Policy(
    rwAsciiStream_t    *astream,
    sk_ipv6policy_t     policy);

/**
 *    DEPRECATED as of SiLK 3.7.
 *
 *    Configure the 'astream' to print IP addresses as integers (0)
 *    instead of in the canonical format.
 */
#define rwAsciiSetIntegerIps(a_a)                       \
    rwAsciiSetIPFormatFlags((a_a), SKIPADDR_DECIMAL)

/**
 *    DEPRECATED as of SiLK 3.7.
 *
 *    Configure the 'astream' to print each octet of an IP with extra
 *    0's; e.g., print "10.1.2.3" as "010.001.002.003".
 */
#define rwAsciiSetZeroPadIps(a_a)                       \
    rwAsciiSetIPFormatFlags((a_a), SKIPADDR_ZEROPAD)

/**
 *    Set the argument that the 'astream' will pass to sktimestamp() to
 *    print the times.
 */
void
rwAsciiSetTimestampFlags(
    rwAsciiStream_t    *astream,
    uint32_t            time_flags);

/**
 *    Configure the 'astream' to print sensors as integers.  Usually
 *    the name of the sensor is printed.
 */
void
rwAsciiSetIntegerSensors(
    rwAsciiStream_t    *astream);

/**
 *    Configure the 'astream' to print TCP Flags as integers.  Usually
 *    an character is printed (F,S,R,P,A,U,E,C) for each flag.  This
 *    affects all TCP Flag fields (flags,initialFlags,sessionFlags).
 */
void
rwAsciiSetIntegerTcpFlags(
    rwAsciiStream_t    *astream);

/**
 *    Configure the 'astream' to use a slightly different output for
 *    ICMP or ICMPv6 records (IPv4 record with proto==1, or IPv6
 *    record with proto==58); when active and an ICMP/ICMPv6 record is
 *    given, the sPort and dPort columns will hold the ICMP type and
 *    code, respectively.
 *
 *    This behavior is deprecated as of SiLK 3.8.1 and will be removed
 *    in SiLK 4.0.
 */
void
rwAsciiSetIcmpTypeCode(
    rwAsciiStream_t    *astream);

/**
 *    Put the first 'buf_len'-1 characters of the name of the
 *    field/column denoted by 'field_id' into the buffer 'buf'.  The
 *    caller should ensure that 'buf' is 'buf_len' characters long.
 *
 *    A 'buf' of at least RWREC_PRINTABLE_MAX_NAME_LEN characters will
 *    be large enough to hold the entire field name.
 */
void
rwAsciiGetFieldName(
    char                       *buf,
    size_t                      buf_len,
    rwrec_printable_fields_t    field_id);

/**
 *    Call flush() on the I/O object that 'astream' wraps.
 */
int
rwAsciiFlush(
    rwAsciiStream_t    *astream);

/**
 *    Appends all printable fields to the string map pointed to by
 *    'field_map'.  If *'field_map' is NULL, a new string map is
 *    created, and it is the caller's responsibilty to destroy it.
 *
 *    Returns the result of creating the string map or appending
 *    values to it.
 */
sk_stringmap_status_t
rwAsciiFieldMapAddDefaultFields(
    sk_stringmap_t    **field_map);

/**
 *    Appends the deprecated "icmpTypeCode" field (and its alias "25")
 *    to the existing string map pointed to by 'field_map'.  The field
 *    is assigned the ID given in 'id'.
 *
 *    Returns the result of appending entries to the string map.
 */
sk_stringmap_status_t
rwAsciiFieldMapAddIcmpTypeCode(
    sk_stringmap_t     *field_map,
    uint32_t            id);

#ifdef __cplusplus
}
#endif
#endif /* _RWASCII_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
