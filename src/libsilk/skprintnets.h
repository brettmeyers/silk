/*
** Copyright (C) 2005-2015 by Carnegie Mellon University.
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
**  skprintnets.h
**
**    Utilities used by IPsets and Bags to group IPs into arbitrarily
**    sized netblocks for printing.  Each netblock keeps a count of
**    the number of smaller netblocks seen.  In the case of Bags, each
**    netblock sums the counters for the entries in that netblock.
**
*/
#ifndef _PRINT_NETWORK_STRUCTURE_H
#define _PRINT_NETWORK_STRUCTURE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_PRINT_NETWORK_STRUCTURE_H, "$SiLK: skprintnets.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/silk_types.h>

/**
 *    The context object for processing IP addresses.
 */
typedef struct skNetStruct_st skNetStruct_t;

/**
 *    Add the CIDR block 'ipaddr'/'prefix' to the network structure
 *    context object 'ns'.  It is an error to call this function on a
 *    network structure object configured to process counters.
 */
void
skNetStructureAddCIDR(
    skNetStruct_t      *ns,
    const skipaddr_t   *ipaddr,
    uint32_t            prefix);

/**
 *    Add the ('ipaddr', 'counter') pair to the network structure
 *    context object 'ns'.  It is an error to call this function on a
 *    network structure object that is not configured to process
 *    counters.
 */
void
skNetStructureAddKeyCounter(
    skNetStruct_t      *ns,
    const skipaddr_t   *ipaddr,
    const uint64_t     *counter);

/**
 *    Creates a new context object for processing IP addresses and
 *    stores that object in the location specified by 'ns'.
 *
 *    When 'has_count' is non-zero, the context object is configured
 *    to work with Bag files, and the caller must use
 *    skNetStructureAddKeyCounter() to add new (IP,counter) pairs to
 *    the context object for printing.
 *
 *    When 'has_count' is zero, the context object is configured to
 *    work with IPset files and the caller must use
 *    skNetStructureAddCIDR() to add a new CIDR block to the context
 *    object for printing.
 *
 *    Once all IPs have been processed, the caller must invoke
 *    skNetStructurePrintFinalize() to close any netblock that is
 *    still open and to print the total.
 *
 *    Text is printed in pipe-delimited columns by default.
 *
 *    By default, the context object prints to standard output.
 *
 *    Whether the network structure groups the IPs into IPv4 or IPv6
 *    netblocks is determined by the input passed to
 *    skNetStructureParse().  The default is to use the IPv4
 *    netblocks.
 *
 *    When configured to process IPv4 addresses, hosts are grouped by
 *    the /8, /16, /24, and /27 netblocks by default.  This may be
 *    changed by calling skNetStructureParse().
 *
 *    When configured to process IPv6 addresses, hosts are grouped by
 *    the /48 and /64 netblocks.  This may be changed by calling
 *    skNetStructureParse().
 *
 *    The default output prints the number of unique hosts seen and
 *    the number of each of the above netblocks that were seen.
 *
 *
 */
int
skNetStructureCreate(
    skNetStruct_t     **ns,
    int                 has_count);

/**
 *    Destroy the network structure context object pointed at by 'ns'
 *    and set 'ns' to NULL.  Does nothing if 'ns' or *ns is NULL.
 */
void
skNetStructureDestroy(
    skNetStruct_t     **ns);

/**
 *    Have the network structure context object 'ns' parse the user's
 *    configuration setting in input.  The input configures whether
 *    the network structure context object groups into IPv4 or IPv6
 *    netblocks and whether the be counted and/or printed.
 */
int
skNetStructureParse(
    skNetStruct_t      *ns,
    const char         *input);

/**
 *    Tell the network structure context object 'ns' that all IPs have
 *    been added and that it should finalize its output by closing any
 *    open netblocks and printing the results.
 */
void
skNetStructurePrintFinalize(
    skNetStruct_t      *ns);

/**
 *    Configure the network structure context object 'ns' to use
 *    'width' as the width of the column that contains the counter
 *    sum.  The value is only used when processing Bag files.
 */
void
skNetStructureSetCountWidth(
    skNetStruct_t      *ns,
    int                 width);

/**
 *    Configure the network structure context object 'ns' to print
 *    'delimiter' between columns and at the end of each row.
 */
void
skNetStructureSetDelimiter(
    skNetStruct_t      *ns,
    char                delimiter);

/**
 *    Configure the network structure context object 'ns' so it uses
 *    'format' when printing IP addresses, where 'format' will be
 *    passed to the skipaddrString() function.
 */
void
skNetStructureSetIpFormat(
    skNetStruct_t      *ns,
    uint32_t            format);

/**
 *    Configure the network structure context object 'ns' so it does
 *    not print the data in columns.
 */
void
skNetStructureSetNoColumns(
    skNetStruct_t      *ns);

/**
 *    Configure the network structure context object 'ns' so it does
 *    not print the final delimiter on each row.
 */
void
skNetStructureSetNoFinalDelimiter(
    skNetStruct_t      *ns);

/**
 *    Configure the network structure context object 'ns' to send its
 *    output to 'stream'.
 */
void
skNetStructureSetOutputStream(
    skNetStruct_t      *ns,
    skstream_t         *stream);

#ifdef __cplusplus
}
#endif
#endif /* _PRINT_NETWORK_STRUCTURE_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
