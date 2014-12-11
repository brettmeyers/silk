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
**  skcountry.c
**
**    John McClary Prevost
**    December 6th, 2004
**
**    Country code lookups using the prefixmap data structure.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: skcountry.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/rwrec.h>
#include <silk/skcountry.h>
#include <silk/skipaddr.h>
#include <silk/skprefixmap.h>


/* TYPEDEFS AND MACROS */

#define MIN_COUNTRY_CODE  (((uint32_t)' ' << 8) | (uint32_t)' ')

#define MAX_COUNTRY_CODE  (((uint32_t)'~' << 8) | (uint32_t)'~')

#define WRITE_INVALID_CC_STRING(out, out_len)   \
    (void)snprintf((out), (out_len), "??")


/* LOCAL VARIABLES */

/* the prefixmap used to look up country codes */
static skPrefixMap_t *ccmap = NULL;


/* FUNCTION DEFINITIONS */


sk_countrycode_t
skCountryGetMaxCode(
    void)
{
    return SK_COUNTRYCODE_INVALID;
}


sk_countrycode_t
skCountryNameToCode(
    const char         *name)
{
    sk_countrycode_t code;

    if (strlen(name) < 2) {
        return SK_COUNTRYCODE_INVALID;
    }

    code = (sk_countrycode_t)((tolower(name[0]) << 8) | tolower(name[1]));
    if (code < MIN_COUNTRY_CODE || code > MAX_COUNTRY_CODE) {
        return SK_COUNTRYCODE_INVALID;
    }

    return code;
}


char *
skCountryCodeToName(
    sk_countrycode_t    code,
    char               *name,
    size_t              name_len)
{
    if (!name || name_len < 2) {
        if (name && name_len == 1) {
            name[0] = '\0';
            return name;
        }
        return NULL;
    }

    if (code < MIN_COUNTRY_CODE || code > MAX_COUNTRY_CODE) {
        WRITE_INVALID_CC_STRING(name, name_len);
        return name;
    }

    switch (name_len) {
      case 0:
      case 1:
        skAbortBadCase(name_len);
      case 2:
        name[0] = (code >> 8);
        name[1] = '\0';
        break;
      default:
        name[0] = (code >> 8);
        name[1] = (code & 0xFF);
        name[2] = '\0';
        break;
    }
    return name;
}


const skPrefixMap_t *
skCountryGetPrefixMap(
    void)
{
    return ccmap;
}


int
skCountryIsV6(
    void)
{
    if (!ccmap) {
        return -1;
    }
    return (skPrefixMapGetContentType(ccmap) == SKPREFIXMAP_CONT_ADDR_V6);
}


sk_countrycode_t
skCountryLookupCode(
    const skipaddr_t   *ipaddr)
{
    uint32_t code;

    if (!ccmap) {
        return SK_COUNTRYCODE_INVALID;
    }

    code = skPrefixMapFindValue(ccmap, ipaddr);
    if (code == SKPREFIXMAP_NOT_FOUND) {
        return SK_COUNTRYCODE_INVALID;
    }
    return (sk_countrycode_t)code;
}


char *
skCountryLookupName(
    const skipaddr_t   *ipaddr,
    char               *name,
    size_t              name_len)
{
    uint32_t code;

    if (!ccmap) {
        goto ERROR;
    }

    code = skPrefixMapFindValue(ccmap, ipaddr);
    if (code > MAX_COUNTRY_CODE) {
        goto ERROR;
    }

    return skCountryCodeToName((sk_countrycode_t)code, name, name_len);

  ERROR:
    if (!name || name_len == 0) {
        return NULL;
    }
    WRITE_INVALID_CC_STRING(name, name_len);
    return name;
}


sk_countrycode_t
skCountryLookupCodeAndRange(
    const skipaddr_t   *ipaddr,
    skipaddr_t         *start_range,
    skipaddr_t         *end_range)
{
    uint32_t code;

    if (!ccmap) {
        return SK_COUNTRYCODE_INVALID;
    }

    code = skPrefixMapFindRange(ccmap, ipaddr, start_range, end_range);
    if (code == SKPREFIXMAP_NOT_FOUND) {
        return SK_COUNTRYCODE_INVALID;
    }
    return (sk_countrycode_t)code;
}


int
skCountrySetup(
    const char         *map_name,
    sk_msg_fn_t         errfn)
{
    char filename[PATH_MAX];
    skPrefixMapErr_t map_error;
    const char *errmsg;
    int check_pwd = 1;
    int found = 0;

    if (ccmap) {
        return 0;
    }

    if (!map_name) {
        map_name = getenv(SK_COUNTRY_MAP_ENVAR);
        if (!map_name || !map_name[0]) {
            map_name = SK_COUNTRY_DEFAULT_MAP;
            /* don't check pwd if we use the default map name */
            check_pwd = 0;
        }
    }

    /* if name explicitly given, see if the file exists.  this will
     * support relative paths that skFindFile() does not. */
    if (check_pwd) {
        if (skFileExists(map_name)) {
            strncpy(filename, map_name, sizeof(filename));
            filename[sizeof(filename)-1] = '\0';
            found = 1;
        }
    }

    /* Locate the data file */
    if (!found
        && (NULL == skFindFile(map_name, filename, sizeof(filename), 1)))
    {
        if (errfn) {
            errfn("Could not locate Country Code data file '%s'",
                  map_name);
        }
        return -1;
    }

    /* Read in the data file */
    map_error = skPrefixMapLoad(&ccmap, filename);
    switch (map_error) {
      case SKPREFIXMAP_OK:
        if (SKPREFIXMAP_CONT_PROTO_PORT == skPrefixMapGetContentType(ccmap)) {
            skPrefixMapDelete(ccmap);
            ccmap = NULL;
            errmsg = "Map contains protocol/port pairs";
            break;
        }
        return 0;
      case SKPREFIXMAP_ERR_ARGS:
        errmsg = "Invalid arguments";
        break;
      case SKPREFIXMAP_ERR_MEMORY:
        errmsg = "Out of memory";
        break;
      case SKPREFIXMAP_ERR_IO:
        errmsg = "I/O error";
        break;
      default:
        errmsg = "Unknown error";
        break;
    }

    if (errfn) {
        errfn("Failed to load Country Code data file '%s': %s",
              filename, errmsg);
    }
    return -1;
}


void
skCountryTeardown(
    void)
{
    if (ccmap) {
        skPrefixMapDelete(ccmap);
        ccmap = NULL;
    }
}



/* **************************************************************** */
/* Country Code "Plug-In" Support */
/* **************************************************************** */


/* TYPEDEFS AND DEFINES */

#define CCFILTER_TEXT_WIDTH 3

#define CCFILTER_SCC  1
#define CCFILTER_DCC  2

/* Plugin protocol version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0


/* LOCAL VARIABLES */

/* fields for rwcut, rwuniq, etc */
static struct plugin_fields_st {
    const char *name;
    const char *alias;
    uint32_t    val;
    const char *description;
} plugin_fields[] = {
    {"scc", "18", CCFILTER_SCC, "Country code of source address"},
    {"dcc", "19", CCFILTER_DCC, "Country code of destination address"},
    {NULL,  NULL, UINT32_MAX, NULL}         /* sentinel */
};


/* PRIVATE FUNCTION PROTOTYPES */

static skplugin_err_t ccInit(void UNUSED(*x));
static skplugin_err_t ccCleanup(void UNUSED(*x));
static skplugin_err_t
recToText(
    const rwRec        *rwrec,
    char               *dest,
    size_t              width,
    void               *cbdata,
    void              **extra);
static skplugin_err_t
recToBin(
    const rwRec        *rec,
    uint8_t            *dest,
    void               *cbdata,
    void              **extra);
static skplugin_err_t
binToText(
    const uint8_t      *bin,
    char               *dest,
    size_t              width,
    void               *cbdata);


/* FUNCTION DEFINITIONS */

/* the registration function called by skplugin.c */
skplugin_err_t
skCountryAddFields(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*pi_data))
{
    int i;
    skplugin_field_t *field;
    skplugin_err_t rv = SKPLUGIN_OK;
    skplugin_callbacks_t regdata;

    /* Check API version */
    rv = skpinSimpleCheckVersion(major_version, minor_version,
                                 PLUGIN_API_VERSION_MAJOR,
                                 PLUGIN_API_VERSION_MINOR,
                                 skAppPrintErr);
    if (rv != SKPLUGIN_OK) {
        return rv;
    }

    /* register the fields to use for rwcut, rwuniq, rwsort */
    memset(&regdata, 0, sizeof(regdata));
    regdata.init         = ccInit;
    regdata.cleanup      = ccCleanup;
    regdata.column_width = CCFILTER_TEXT_WIDTH;
    regdata.bin_bytes    = sizeof(sk_countrycode_t);
    regdata.rec_to_text  = recToText;
    regdata.rec_to_bin   = recToBin;
    regdata.bin_to_text  = binToText;

    for (i = 0; plugin_fields[i].name; ++i) {
        rv = skpinRegField(&field, plugin_fields[i].name,
                           plugin_fields[i].description,
                           &regdata, (void*)&plugin_fields[i].val);
        if (SKPLUGIN_OK != rv) {
            return rv;
        }
        rv = skpinAddFieldAlias(field, plugin_fields[i].alias);
        if (SKPLUGIN_OK != rv) {
            return rv;
        }
    }

    return rv;
}


/*
 *  status = ccInit(data);
 *
 *    The initialization code for this plugin.  This is called by the
 *    plugin initialization code after option parsing and before data
 *    processing.
 */
static skplugin_err_t
ccInit(
    void        UNUSED(*x))
{
    /* Read in the data file */
    if (skCountrySetup(NULL, &skAppPrintErr)) {
        return SKPLUGIN_ERR;
    }

    return SKPLUGIN_OK;
}


/*
 *   status = ccCleanup(data);
 *
 *     Called by plugin interface code to tear down this plugin.
 */
static skplugin_err_t
ccCleanup(
    void        UNUSED(*x))
{
    skCountryTeardown();
    return SKPLUGIN_OK;
}


/*
 *  status = recToText(rwrec, text_val, text_len, &index, NULL);
 *
 *    Given the SiLK Flow record 'rwrec', lookup the Country Code
 *    specified by '*index', and write a textual representation of
 *    that value into 'text_val', a buffer of 'text_len' characters.
 */
static skplugin_err_t
recToText(
    const rwRec            *rwrec,
    char                   *text_value,
    size_t                  text_size,
    void                   *idx,
    void           UNUSED(**extra))
{
    skipaddr_t ipaddr;

    switch (*((unsigned int*)(idx))) {
      case CCFILTER_SCC:
        rwRecMemGetSIP(rwrec, &ipaddr);
        break;
      case CCFILTER_DCC:
        rwRecMemGetDIP(rwrec, &ipaddr);
        break;
      default:
        return SKPLUGIN_ERR_FATAL;
    }

    skCountryLookupName(&ipaddr, text_value, text_size);
    return SKPLUGIN_OK;
}


/*
 *  status = recToBin(rwrec, bin_val, &index, NULL);
 *
 *    Given the SiLK Flow record 'rwrec', lookup the Country Code
 *    specified by '*index', and write a binary representation of
 *    that value into 'bin_val'.
 */
static skplugin_err_t
recToBin(
    const rwRec            *rwrec,
    uint8_t                *bin_value,
    void                   *idx,
    void           UNUSED(**extra))
{
    skipaddr_t ipaddr;
    sk_countrycode_t cc;

    switch (*((unsigned int*)(idx))) {
      case CCFILTER_SCC:
        rwRecMemGetSIP(rwrec, &ipaddr);
        break;
      case CCFILTER_DCC:
        rwRecMemGetDIP(rwrec, &ipaddr);
        break;
      default:
        return SKPLUGIN_ERR_FATAL;
    }

    cc = htons(skCountryLookupCode(&ipaddr));
    memcpy(bin_value, &cc, sizeof(sk_countrycode_t));

    return SKPLUGIN_OK;
}


/*
 *  status = binToText(bin_val, text_val, text_len, &index);
 *
 *    Given the buffer 'bin_val' which was filled by calling
 *    recToBin(), write a textual representation of that value into
 *    'text_val', a buffer of 'text_len' characters.
 */
static skplugin_err_t
binToText(
    const uint8_t          *bin_value,
    char                   *text_value,
    size_t                  text_size,
    void            UNUSED(*idx))
{
    sk_countrycode_t cc;

    memcpy(&cc, bin_value, sizeof(sk_countrycode_t));

    skCountryCodeToName(ntohs(cc), text_value, text_size);
    return SKPLUGIN_OK;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
