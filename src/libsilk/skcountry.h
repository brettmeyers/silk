/*
** Copyright (C) 2004-2014 by Carnegie Mellon University.
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
**  skcountry.h
**
**    Functions for getting the two letter country code value for an
**    IP address.
**
**    Based on ccfilter_priv.h by John McClary Prevost, December 2004
**
**    Mark Thomas
**
*/
#ifndef _SKCOUNTRY_H
#define _SKCOUNTRY_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKCOUNTRY_H, "$SiLK: skcountry.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/silk_types.h>
#include <silk/skplugin.h>
#include <silk/skprefixmap.h>

/**
 *  @file
 *
 *    Functions for processing a specially designed binary prefix map
 *    file whose entries have a two-later country code as their value.
 *
 *    This file is part of libsilk.
 */


#define SK_COUNTRYCODE_INVALID      ((sk_countrycode_t)32383)

/**
 *    This contains the name of an environment variable.  If that
 *    variable is set, it should name the country code file to use.
 */
#define SK_COUNTRY_MAP_ENVAR        "SILK_COUNTRY_CODES"

/**
 *    If a country code data file name is not provided (neither in the
 *    environment nor via command line switches where
 *    supported/required) this is the name of mapping file.
 */
#define SK_COUNTRY_DEFAULT_MAP      "country_codes.pmap"


/**
 *    Abstract type for country code values
 */
typedef uint16_t sk_countrycode_t;


/**
 *    Return the maximum possible country code value.
 */
sk_countrycode_t
skCountryGetMaxCode(
    void);


/**
 *    Given a two letter Country Code in 'name', return the numerical
 *    value.  Returns SK_COUNTRYCODE_INVALID if 'name' is too long to
 *    be Country Code or contains illegal characters.  The returned
 *    value may not be a valid Country Code.
 */
sk_countrycode_t
skCountryNameToCode(
    const char         *name);


/**
 *    Given a numeric Country Code in 'code', fill 'name' with the two
 *    letter representation of the code, where 'name_len' is the
 *    number of characters in 'name'.
 *
 *    Return NULL if 'name' is NULL.  If 'code' is not a possible
 *    Country Code, writes "??" to name.
 */
char *
skCountryCodeToName(
    sk_countrycode_t    code,
    char               *name,
    size_t              name_len);


/**
 *    Return a handle to the prefix map supporting the Country Codes.
 */
const skPrefixMap_t *
skCountryGetPrefixMap(
    void);


/**
 *    Return 1 if the Country Code map contains IPv6 addresses.
 *    Return 0 if the Country Code map contains only IPv4 addresses.
 *    Return -1 if the Country Code map is not available.
 */
int
skCountryIsV6(
    void);


/**
 *    Find the Country Code for the IP address 'ipaddr' and return
 *    the numerical value.  The caller must invoke skCountrySetup()
 *    prior to calling this function.
 *
 *    Return SK_INVALID_COUNTRY_CODE if the Country Code map has not
 *    been loaded or if the Country Code map contains only IPv4
 *    addresses and 'ipaddr' is IPv6.
 *
 *    See also skCountryLookupName(), skCountryLookupCodeAndRange().
 */
sk_countrycode_t
skCountryLookupCode(
    const skipaddr_t   *ipaddr);


/**
 *    Find the Country Code for the IP address 'ipaddr' and return the
 *    numerical value.  The caller must invoke skCountrySetup() prior
 *    to calling this function.
 *
 *    In addition, set the values pointed at by 'start_range' and
 *    'end_range' to the starting and ending IP addresses of the CIDR
 *    block in the Country Code mapping file that contains 'ipaddr'.
 *
 *    Return SK_INVALID_COUNTRY_CODE and leave 'start_range' and
 *    'end_range' unchanged if the Country Code map has not been
 *    loaded or if the Country Code map contains only IPv4 addresses
 *    and 'ipaddr' is IPv6.
 *
 *    See also skCountryLookupCode(), skCountryLookupName().
 */
sk_countrycode_t
skCountryLookupCodeAndRange(
    const skipaddr_t   *ipaddr,
    skipaddr_t         *start_range,
    skipaddr_t         *end_range);


/**
 *    Find the Country Code for the IP address 'ipaddr' and return
 *    the numerical value.  The caller must invoke skCountrySetup()
 *    prior to calling this function.
 *
 *    Return NULL if 'name' is NULL.  If the Country Code map contains
 *    only IPv4 addresses and 'ipaddr' is IPv6 or if the address
 *    cannot be mapped for any other reason, write "??" to name.
 *
 *    See also skCountryLookupCode(), skCountryLookupCodeAndRange().
 */
char *
skCountryLookupName(
    const skipaddr_t   *ipaddr,
    char               *name,
    size_t              name_len);


/**
 *    Load the Country Code map for use by the skCountryLookupCode()
 *    and skCountryLookupName() functions.
 *
 *    Use the Country Code map name in 'map_name' if that value is
 *    provided.  If not, the environment variable named by
 *    SK_COUNTRY_ENVAR is used.  If that is empty, the
 *    SK_COUNTRY_DEFAULT_MAP is used.
 *
 *    Return 0 on success or non-zero if the map cannot be found or
 *    there is a problem reading the file.  On error, a messages will
 *    be printed using 'errfn' if non-NULL.
 *
 *    If the Country Code map was previously initialized, this
 *    function returns 0.  To load a different map, first destroy the
 *    current mapping by calling skCountryTeardown().
 */
int
skCountrySetup(
    const char         *map_name,
    sk_msg_fn_t         errfn);


/**
 *    Remove the Country Code mapping file from memory.
 */
void
skCountryTeardown(
    void);


/**
 *    Add support for the --scc and --dcc switches in rwfilter, and
 *    the 'scc' and 'dcc' fields in rwcut, rwgroup, rwsort, rwuniq,
 *    and rwstats.
 */
skplugin_err_t
skCountryAddFields(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*pi_data));


#ifdef __cplusplus
}
#endif
#endif /* _SKCOUNTRY_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
