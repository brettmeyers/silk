/*
** Copyright (C) 2004-2015 by Carnegie Mellon University.
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
**  addrtype.c
**
**    John McClary Prevost
**    May 25th, 2006
**
**    Support for "address type" lookups using the prefixmap data
**    structure.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: addrtype.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skplugin.h>
#include <silk/rwrec.h>
#include <silk/skprefixmap.h>


/* TYPEDEFS AND DEFINES */

#define ADDRTYPE_TEXT_WIDTH 1

/* internal identifiers for the fields/switches */
#define ADDRTYPE_STYPE  0
#define ADDRTYPE_DTYPE  1

/* Plugin protocol version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0

/* possible numerical values for the filters/fields */
typedef enum addrtype_en {
    ADDRTYPE_NONROUTABLE = 0,
    ADDRTYPE_INTERNAL    = 1,
    ADDRTYPE_EXTERNAL    = 2,
    ADDRTYPE_NONINTERNAL = 3,
    ADDRTYPE_UNSET       = INT8_MAX
} addrtype_t;


/* LOCAL VARIABLES */

/* the prefixmap used to look up addrtypes */
static skPrefixMap_t *addrtype_map = NULL;

/* which addrtype does the user want---for filtering */
static addrtype_t stype = ADDRTYPE_UNSET;
static addrtype_t dtype = ADDRTYPE_UNSET;

/* Options and Help for filtering */
static struct option plugin_options[] = {
    {"stype",       REQUIRED_ARG, 0, ADDRTYPE_STYPE},
    {"dtype",       REQUIRED_ARG, 0, ADDRTYPE_DTYPE},
    {0, 0, 0, 0}    /* sentinel */
};
static const char *plugin_help[] = {
    ("Source address mapped by \"address_types.pmap\" has this value.\n"
     "\tOne of: 0=non-routable; 1=internal; 2=external; 3=not-internal"),
    "Destination address maps to the specified type",
    NULL
};

/* fields for rwcut, rwuniq, etc */
static struct plugin_fields_st {
    const char     *name;
    uint32_t        val;
    const char     *aliases[3]; /* this size cannot be variable */
} plugin_fields[] = {
    {"sType", ADDRTYPE_STYPE, {"16", NULL}},
    {"dType", ADDRTYPE_DTYPE, {"17", NULL}},
    {NULL,    UINT32_MAX,     {NULL, NULL, NULL}}  /* sentinel */
};


/* LOCAL FUNCTION PROTOTYPES */

static skplugin_err_t addrtypeInit(void UNUSED(*x));
static skplugin_err_t addrtypeCleanup(void UNUSED(*x));
static skplugin_err_t
optionsHandler(
    const char         *opt_arg,
    void               *cbdata);
static skplugin_err_t
addrtypeFilter(
    const rwRec        *rwrec,
    void               *cbdata,
    void              **extra);
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

/* the registration function */
skplugin_err_t
skAddressTypesAddFields(
    uint16_t            major_version,
    uint16_t            minor_version,
    void        UNUSED(*pi_data))
{
    int i, j;
    skplugin_field_t *field = NULL;
    skplugin_callbacks_t regdata;
    skplugin_err_t rv;

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
    regdata.init         = addrtypeInit;
    regdata.cleanup      = addrtypeCleanup;
    regdata.column_width = ADDRTYPE_TEXT_WIDTH;
    regdata.bin_bytes    = sizeof(uint8_t);
    regdata.rec_to_text  = recToText;
    regdata.rec_to_bin   = recToBin;
    regdata.bin_to_text  = binToText;

    for (i = 0; plugin_fields[i].name; ++i) {
        rv = skpinRegField(&field, plugin_fields[i].name, NULL,
                           &regdata, (void*)&plugin_fields[i].val);
        if (SKPLUGIN_OK != rv) {
            return rv;
        }
        for (j = 0; plugin_fields[i].aliases[j]; ++j) {
            rv = skpinAddFieldAlias(field, plugin_fields[i].aliases[j]);
            if (SKPLUGIN_OK != rv) {
                return rv;
            }
        }
    }

    /* sanity check */
    assert((sizeof(plugin_options)/sizeof(struct option))
           == (sizeof(plugin_help)/sizeof(char*)));

    /* register the options to use for rwfilter.  when the option is
     * given, we will call skpinRegFilter() to register the filter
     * function. */
    for (i = 0; plugin_options[i].name; ++i) {
        rv = skpinRegOption2(plugin_options[i].name, plugin_options[i].has_arg,
                             plugin_help[i], NULL, &optionsHandler,
                             (void*)&plugin_options[i].val,
                             1, SKPLUGIN_FN_FILTER);
        if (SKPLUGIN_OK != rv && SKPLUGIN_ERR_DID_NOT_REGISTER != rv) {
            return rv;
        }
    }

    return SKPLUGIN_OK;
}


/*
 *  status = addrtypeInit(data);
 *
 *    The initialization code for this plugin.  This is called by the
 *    plugin initialization code after option parsing and before data
 *    processing.
 */
static skplugin_err_t
addrtypeInit(
    void        UNUSED(*x))
{
    if (skAddressTypesSetup(NULL, &skAppPrintErr)) {
        return SKPLUGIN_ERR;
    }
    return SKPLUGIN_OK;
}


/*
 *   status = addrtypeCleanup(data);
 *
 *     Called by plugin interface code to tear down this plugin.
 */
static skplugin_err_t
addrtypeCleanup(
    void        UNUSED(*x))
{
    skAddressTypesTeardown();
    return SKPLUGIN_OK;
}


/*
 *  status = optionsHandler(opt_arg, &index);
 *
 *    Handles options for the plugin.  'opt_arg' is the argument, or
 *    NULL if no argument was given.  'index' is the enum passed in
 *    when the option was registered.
 *
 *    Returns SKPLUGIN_OK on success, or SKPLUGIN_ERR if there was a
 *    problem.
 */
static skplugin_err_t
optionsHandler(
    const char         *opt_arg,
    void               *cbdata)
{
    skplugin_callbacks_t regdata;
    unsigned int opt_index = *((unsigned int*)cbdata);
    uint32_t opt_val;
    int rv;

    rv = skStringParseUint32(&opt_val, opt_arg, ADDRTYPE_NONROUTABLE,
                             ADDRTYPE_NONINTERNAL);
    if (rv != 0) {
        skAppPrintErr("Invalid %s '%s': %s",
                      plugin_options[opt_index].name, opt_arg,
                      skStringParseStrerror(rv));
        return SKPLUGIN_ERR;
    }

    switch (opt_index) {
      case ADDRTYPE_STYPE:
        if (stype != ADDRTYPE_UNSET) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          plugin_options[opt_index].name);
            return SKPLUGIN_ERR;
        }
        stype = (addrtype_t)opt_val;
        break;

      case ADDRTYPE_DTYPE:
        if (dtype != ADDRTYPE_UNSET) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          plugin_options[opt_index].name);
            return SKPLUGIN_ERR;
        }
        dtype = (addrtype_t)opt_val;
        break;

      default:
        return SKPLUGIN_ERR_FATAL;
    }

    memset(&regdata, 0, sizeof(regdata));
    regdata.init = addrtypeInit;
    regdata.cleanup = addrtypeCleanup;
    regdata.filter = addrtypeFilter;
    return skpinRegFilter(NULL, &regdata, cbdata);
}


/*
 *  status = addrtypeFilter(rwrec, data, NULL);
 *
 *    The function implements filtering for the plugin.  Returns
 *    SKPLUGIN_FILTER_PASS if the record passes the filter,
 *    SKPLUGIN_FILTER_FAIL if it fails the filter.
 */
static skplugin_err_t
addrtypeFilter(
    const rwRec            *rwrec,
    void                   *idx,
    void           UNUSED(**extra))
{
    skipaddr_t addr;
    uint32_t code;
    uint32_t wanted;

    switch (*((unsigned int*)(idx))) {
      case ADDRTYPE_STYPE:
        rwRecMemGetSIP(rwrec, &addr);
        wanted = stype;
        break;
      case ADDRTYPE_DTYPE:
        rwRecMemGetDIP(rwrec, &addr);
        wanted = dtype;
        break;
      default:
        return SKPLUGIN_ERR_FATAL;
    }

    code = skPrefixMapFindValue(addrtype_map, &addr);

    /* Given that this IP returns 'code', see how this compare against
     * what the user wanted. */
    if (ADDRTYPE_NONINTERNAL == wanted) {
        /* anything except internal is fine */
        if (ADDRTYPE_INTERNAL == code) {
            return SKPLUGIN_FILTER_FAIL;
        }
    } else if (wanted != code) {
        return SKPLUGIN_FILTER_FAIL;
    }
    return SKPLUGIN_FILTER_PASS;
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
    skipaddr_t addr;
    uint32_t code;

    switch (*((unsigned int*)(idx))) {
      case ADDRTYPE_STYPE:
        rwRecMemGetSIP(rwrec, &addr);
        break;
      case ADDRTYPE_DTYPE:
        rwRecMemGetDIP(rwrec, &addr);
        break;
      default:
        return SKPLUGIN_ERR_FATAL;
    }

    code = skPrefixMapFindValue(addrtype_map, &addr);
    snprintf(text_value, text_size, ("%" PRIu32), code);
    return SKPLUGIN_OK;
}


/*
 *  status = recToBin(rwrec, bin_val, &index, NULL);
 *
 *    Given the SiLK Flow record 'rwrec', lookup the Address Type
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
    skipaddr_t addr;
    uint32_t code;

    switch (*((unsigned int*)(idx))) {
      case ADDRTYPE_STYPE:
        rwRecMemGetSIP(rwrec, &addr);
        break;
      case ADDRTYPE_DTYPE:
        rwRecMemGetDIP(rwrec, &addr);
        break;
      default:
        return SKPLUGIN_ERR_FATAL;
    }

    code = skPrefixMapFindValue(addrtype_map, &addr);
    *bin_value = (uint8_t)code;
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
    snprintf(text_value, text_size, "%u", *bin_value);
    return SKPLUGIN_OK;
}


skPrefixMap_t *
skAddressTypesGetPmap(
    void)
{
    return addrtype_map;
}


int
skAddressTypesSetup(
    const char         *map_name,
    sk_msg_fn_t         errfn)
{
    char filename[PATH_MAX];
    skPrefixMapErr_t map_error;
    int check_pwd = 1;
    int found = 0;

    if (addrtype_map) {
        return 0;
    }

    if (!map_name) {
        map_name = getenv(SK_ADDRTYPE_MAP_ENVAR);
        if (!map_name || !map_name[0]) {
            map_name = SK_ADDRTYPE_DEFAULT_MAP;
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
            errfn("Could not locate AddressTypes data file '%s'",
                  map_name);
        }
        return -1;
    }

    /* Read in the data file */
    map_error = skPrefixMapLoad(&addrtype_map, filename);
    if (SKPREFIXMAP_OK != map_error) {
        if (errfn) {
            errfn("Failed to load AddressTypes data file '%s': %s",
                  filename, skPrefixMapStrerror(map_error));
        }
        return -1;
    }

    if (skPrefixMapGetContentType(addrtype_map)==SKPREFIXMAP_CONT_PROTO_PORT){
        skPrefixMapDelete(addrtype_map);
        addrtype_map = NULL;
        if (errfn) {
            errfn(("Failed to load AddressTypes data file '%s': "
                   "Map contains protocol/port pairs"),
                  filename);
        }
        return -1;
    }

    return 0;
}


void
skAddressTypesTeardown(
    void)
{
    if (addrtype_map) {
        skPrefixMapDelete(addrtype_map);
        addrtype_map = NULL;
    }
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
