/*
** Copyright (C) 2006-2014 by Carnegie Mellon University.
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
**  skite.c
**
**    Manages access to the classes, types, and sensors that are read
**    from the silk.conf file, as well as to similar values (file
**    formats and compression methods) that aren't strictly part of
**    silk.conf.
**
*/

/* use skCompressionMethods[] and fileOutputFormats[] from silk_files.h */
#define SKSITE_SOURCE 1
#include <silk/silk.h>

RCSIDENT("$SiLK: sksite.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/utils.h>
#include <silk/skstringmap.h>
#include "sksiteconfig.h"
#ifdef __CYGWIN__
#include "skcygwin.h"
#endif


/* TYPEDEFS AND DEFINES */

#define SILK_CONFIG_FILE_NAME "silk.conf"
#define SILK_DEFAULT_PATH_FORMAT "%T/%Y/%m/%d/%x"

/* if all other attempts to get a data root directory fail, use this */
#define FALLBACK_DATA_ROOTDIR "/data"

/* characters that may not appear in a flowtype (including a class
 * name and a type name) whitespace, '"', '\'', '\\', '/' */
#define SITE_BAD_CHARS_FLOWTYPE  "\t\n\v\f\r \b\a\"'\\/"

/* characters that may not appear in a sensor name */
#define SITE_BAD_CHARS_SENSOR    "_" SITE_BAD_CHARS_FLOWTYPE

/** Local Datatypes ***************************************************/

typedef struct sensor_struct_st {
    /* unique name for this sensor */
    const char         *sn_name;
    /* description of this sensor (for end-user use) */
    const char         *sn_description;
    /* vector of classes it belongs to */
    sk_vector_t        *sn_class_list;
    /* length of the name */
    size_t              sn_name_strlen;
    /* the sensor's id--must be its position in the array */
    sensorID_t          sn_id;
} sensor_struct_t;

typedef struct sensorgroup_struct_st {
    /* unique name for this group */
    const char         *sg_name;
    /* vector of sensors (by sensorID_t) in this group */
    sk_vector_t        *sg_sensor_list;
    /* length of the name */
    size_t              sg_name_strlen;
    /* the group's id--must be its position in the array */
    sensorgroupID_t     sg_id;
} sensorgroup_struct_t;

typedef struct class_struct_st {
    /* unique name for this class */
    const char         *cl_name;
    /* vector of sensors (by sensorID_t) in class */
    sk_vector_t        *cl_sensor_list;
    /* vector of flowtypes (by flowtypeID_t) in class */
    sk_vector_t        *cl_flowtype_list;
    /* vector of class's default flowtypes (by flowtypeID_t) */
    sk_vector_t        *cl_default_flowtype_list;
    /* length of the name */
    size_t              cl_name_strlen;
    /* the class's id--must be its position in the array */
    classID_t           cl_id;
} class_struct_t;

typedef struct flowtype_struct_st {
    /* unique name for this flowtype */
    const char         *ft_name;
    /* unique name for this flowtype within its class */
    const char         *ft_type;
    /* length of name */
    size_t              ft_name_strlen;
    /* length of type */
    size_t              ft_type_strlen;
    /* the class ID */
    classID_t           ft_class;
    /* the flowtype's id--must be its position in the array */
    flowtypeID_t        ft_id;
} flowtype_struct_t;

typedef struct fileformat_struct_st {
    /* the output format's id--must be its position in the array */
    fileFormat_t        ff_id;
    /* unique name of this output format */
    const char         *ff_name;
} fileformat_struct_t;

typedef struct compmethod_struct_st {
    /* the compression method's id--must be its position in the array */
    sk_compmethod_t     cm_id;
    /* unique name of this compression method */
    const char         *cm_name;
} compmethod_struct_t;

static struct fileformat_list_st {
    fileformat_struct_t    *fl_list;
    uint8_t                 fl_count;
} fileformats;

static struct compmethod_list_st {
    compmethod_struct_t    *cml_list;
    sk_compmethod_t         cml_count;
} compmethods;

static sk_compmethod_t default_compression = SK_ENABLE_OUTPUT_COMPRESSION;

/* The %-conversion characters supported by the path-format; exported
 * so it can be checked during silk.conf parsing */
const char path_format_conversions[] = "%CFHNTYdfmnx";


/** Options ***********************************************************/

typedef enum {
    OPT_SITE_CONFIG_FILE
} siteOptionsEnum;

/* switch */
static struct option siteOptions[] = {
    {"site-config-file",        REQUIRED_ARG, 0, OPT_SITE_CONFIG_FILE},
    {0,0,0,0}                   /* sentinel */
};


/** Config Storage ****************************************************/

#define MIN_FIELD_SIZE 3
#define INVALID_LABEL "?"


static char data_rootdir[PATH_MAX];
static char silk_config_file[PATH_MAX];
static char path_format[PATH_MAX];
static char packing_logic_path[PATH_MAX];


/* flags the caller passed to sksiteOptionsRegister() */
static uint32_t     site_opt_flags = 0;

/* 0 if not yet configured, 1 if configuration succeeded, -1 if it
 * failed due to parse errors.  Calling sksiteConfigure() with a
 * non-existent file does not change its value */
static int          configured = 0;

/* The list of sensors, plus the max field and max known ID.
 * Other types are broken down similarly. */
static sk_vector_t *sensor_list;        /* sensor_struct_t * */
static size_t       sensor_max_name_strlen = MIN_FIELD_SIZE;
static int          sensor_min_id = -1;
static int          sensor_max_id = -1;

/* Default class for fglob. */
static classID_t    default_class = SK_INVALID_CLASS;

static sk_vector_t *class_list;         /* class_struct_t * */
static size_t       class_max_name_strlen = MIN_FIELD_SIZE;
static int          class_max_id = -1;

static sk_vector_t *sensorgroup_list;   /* sensorgroup_struct_t * */
static size_t       sensorgroup_max_name_strlen = MIN_FIELD_SIZE;
static int          sensorgroup_max_id = -1;

static sk_vector_t *flowtype_list;      /* flowtype_struct_t * */
static size_t       flowtype_max_name_strlen = MIN_FIELD_SIZE;
static size_t       flowtype_max_type_strlen = MIN_FIELD_SIZE;
static int          flowtype_max_id = -1;

/** Local Function Prototypes *****************************************/

static int siteOptionsHandler(clientData cData, int opt_index, char *opt_arg);

static int siteInitializeFileformats(size_t sz);
static fileFormat_t siteFileformatAdd(const char *name, uint8_t id);

#define siteGetFileformatCount()                \
    (fileformats.fl_count)
#define siteFileformatGetNameFast(ffid)         \
    (fileformats.fl_list[(ffid)].ff_name)
#define siteFileformatGetName(ffid)             \
    (((ffid) >= siteGetFileformatCount())       \
     ? NULL                                     \
     : siteFileformatGetNameFast(ffid))

static int siteInitializeCompmethods(size_t sz);
static sk_compmethod_t siteCompmethodAdd(const char *name, uint8_t id);

#define siteGetCompmethodCount()                \
    (compmethods.cml_count)
#define siteCompmethodGetNameFast(cmid)         \
    (compmethods.cml_list[(cmid)].cm_name)
#define siteCompmethodGetName(cmid)             \
    (((cmid) >= siteGetCompmethodCount())       \
     ? NULL                                     \
     : siteCompmethodGetNameFast(cmid))

static void
siteSensorFree(
    sensor_struct_t    *sn);
static void
siteClassFree(
    class_struct_t     *cl);
static void
siteSensorgroupFree(
    sensorgroup_struct_t   *sg);
static void
siteFlowtypeFree(
    flowtype_struct_t  *ft);


/**********************************************************************/

int
sksiteInitialize(
    int          UNUSED(levels))
{
    static int initialized = 0;
    uint8_t i;
    uint8_t j;
    size_t numFormats;
    size_t numCompr;
    const char *silk_data_rootdir_env;
    int silk_data_rootdir_set = 0;

    if (initialized) {
        return 0;
    }
    initialized = 1;

    /* store the root_directory from configure, or the env var if given */
    silk_data_rootdir_env = getenv(SILK_DATA_ROOTDIR_ENVAR);
    if (silk_data_rootdir_env) {
        /* env var is defined, use it instead */
        while (isspace((int)*silk_data_rootdir_env)) {
            ++silk_data_rootdir_env;
        }
        if (*silk_data_rootdir_env) {
            if (sksiteSetRootDir(silk_data_rootdir_env)) {
                skAppPrintErr("Problem setting data root directory "
                              "from environment");
                skAbort();
            }
            silk_data_rootdir_set = 1;
        }
    }
    if (!silk_data_rootdir_set) {
        if (sksiteSetRootDir(sksiteGetDefaultRootDir())) {
            skAppPrintErr("Data root directory is too long");
            skAbort();
        }
    }

    /* compute number of file formats: FT_* macros; if the final entry
     * in the array is the empty string; remove it from format
     * count. */
    numFormats = (sizeof(fileOutputFormats)/sizeof(char*));
    if (fileOutputFormats[numFormats-1][0] == '\0') {
        --numFormats;
    }

    siteInitializeFileformats(numFormats);

    /* loop over fileOutputFormats[] and create the file formats */
    for (i = 0; i < numFormats && fileOutputFormats[i][0]; ++i) {
        siteFileformatAdd(fileOutputFormats[i], i);
    }
    if (siteGetFileformatCount() != numFormats) {
        skAppPrintErr("Inconsistency in fileOutputFormats[] array.\n"
                      "\tFix your site header and recompile.  Abort!");
        assert(siteGetFileformatCount() == numFormats);
        skAbort();
    }


    /* compute number of compression methods: SK_COMPMETHOD_* macros;
     * if the final entry in the array is the empty string; remove it
     * from method count. */
    numCompr = (sizeof(skCompressionMethods)/sizeof(char*));
    if (skCompressionMethods[numCompr-1][0] == '\0') {
        --numCompr;
    }

    siteInitializeCompmethods(numCompr);

    /* loop over skCompressionMethods[] to create the compr methods */
    for (j = 0; j < numCompr && skCompressionMethods[j][0]; ++j) {
        siteCompmethodAdd(skCompressionMethods[j], j);
    }
    if (siteGetCompmethodCount() != numCompr) {
        skAppPrintErr("Inconsistency in skCompressionMethods[] array.\n"
                      "\tFix your site header and recompile.  Abort!");
        assert(siteGetCompmethodCount() == numCompr);
        skAbort();
    }

    /* Basic initialization of site config data structures */

    strncpy(path_format, SILK_DEFAULT_PATH_FORMAT, sizeof(path_format));
    sensor_list = skVectorNew(sizeof(sensor_struct_t *));
    class_list = skVectorNew(sizeof(class_struct_t *));
    sensorgroup_list = skVectorNew(sizeof(sensorgroup_struct_t *));
    flowtype_list = skVectorNew(sizeof(flowtype_struct_t *));

    return 0;
}


int
sksiteOptionsRegister(
    uint32_t            flags)
{
    site_opt_flags = flags;

    /* Add a --site-config-file option if requested */
    if (site_opt_flags & SK_SITE_FLAG_CONFIG_FILE) {
        if (skOptionsRegister(siteOptions, &siteOptionsHandler, NULL))
        {
            return -1;
        }
    }
    return 0;
}


void
sksiteOptionsUsage(
    FILE               *fh)
{
#define MIN_TEXT_ON_LINE  15
#define MAX_TEXT_ON_LINE  72

    char *cp, *ep, *sp;
    char path[PATH_MAX];
    char buf[2 * PATH_MAX];

    /* print where we would get the silk.conf file, as well as the
     * other places we might look. */
    if (site_opt_flags & SK_SITE_FLAG_CONFIG_FILE) {
        fprintf(fh, "--%s %s. Location of the site configuration file.\n",
                siteOptions[OPT_SITE_CONFIG_FILE].name,
                SK_OPTION_HAS_ARG(siteOptions[OPT_SITE_CONFIG_FILE]));

        /* put the text into a buffer, and then wrap the text in the
         * buffer at space characters. */
        snprintf(buf, sizeof(buf),
                 ("Currently '%s'. Def. $" SILK_CONFIG_FILE_ENVAR ","
                  " $" SILK_DATA_ROOTDIR_ENVAR "/" SILK_CONFIG_FILE_NAME ","
                  " or '%s/" SILK_CONFIG_FILE_NAME "'"),
                 sksiteGetConfigPath(path, sizeof(path)),
                 sksiteGetDefaultRootDir());
        sp = buf;
        while (strlen(sp) > MAX_TEXT_ON_LINE) {
            cp = &sp[MIN_TEXT_ON_LINE];
            while ((ep = strchr(cp+1, ' ')) != NULL) {
                if (ep - sp > MAX_TEXT_ON_LINE) {
                    /* text is now too long */
                    if (cp == &sp[MIN_TEXT_ON_LINE]) {
                        /* this is the first space character we have
                         * on this line; so use it */
                        cp = ep;
                    }
                    break;
                }
                cp = ep;
            }
            if (cp == &sp[MIN_TEXT_ON_LINE]) {
                /* no space characters anywhere on the line */
                break;
            }
            assert(' ' == *cp);
            *cp = '\0';
            fprintf(fh, "\t%s\n", sp);
            sp = cp + 1;
        }
        if (*sp) {
            fprintf(fh, "\t%s\n", sp);
        }
    }
}


static int
siteOptionsHandler(
    clientData   UNUSED(cData),
    int                 opt_index,
    char               *opt_arg)
{
    switch ((siteOptionsEnum)opt_index) {
      case OPT_SITE_CONFIG_FILE:
        assert(site_opt_flags & SK_SITE_FLAG_CONFIG_FILE);
        if (configured) {
            skAppPrintErr("Ignoring --%s: site already configured",
                          siteOptions[opt_index].name);
        } else if (!skFileExists(opt_arg)) {
            skAppPrintErr("Invalid --%s: file '%s' does not exist",
                          siteOptions[opt_index].name, opt_arg);
            return 1;
        } else if (sksiteSetConfigPath(opt_arg)) {
            skAppPrintErr("Invalid --%s: path name '%s' is too long",
                          siteOptions[opt_index].name, opt_arg);
            return 1;
        }
        sksiteConfigure(1);
        break;
    }
    return 0;
}


static char *
siteFindConfigPath(
    char               *buffer,
    size_t              bufsize)
{
    const char *silk_config_file_env;
    int len;

    /* use environment variable if set; do not check for existence */
    silk_config_file_env = getenv(SILK_CONFIG_FILE_ENVAR);
    if (silk_config_file_env) {
        while (isspace((int)*silk_config_file_env)) {
            ++silk_config_file_env;
        }
        if (*silk_config_file_env) {
            if (bufsize <= strlen(silk_config_file_env)) {
                return NULL;
            }
            strncpy(buffer, silk_config_file_env, bufsize);
            return buffer;
        }
    }

    /* does it exist in SILK_DATA_ROOTDIR/silk.conf ? */
    len = snprintf(buffer, bufsize, "%s/%s",
                   data_rootdir, SILK_CONFIG_FILE_NAME);
    if ((size_t)len > bufsize) {
        return NULL;
    }
    if (skFileExists(buffer)) {
        return buffer;
    }

    /* not under SILK_DATA_ROOTDIR, try SILK_PATH/share/silk and
     * ../share/silk/silk.conf */
    if (skFindFile(SILK_CONFIG_FILE_NAME, buffer, bufsize, 0)) {
        return buffer;
    }

    /* it is not anywhere; return SILK_DATA_ROOTDIR/silk.conf */
    len = snprintf(buffer, bufsize, "%s/%s",
                   data_rootdir, SILK_CONFIG_FILE_NAME);
    assert((size_t)len < bufsize);
    return buffer;
}


int
sksiteConfigure(
    int                 verbose)
{
    /* once we've attempted to parse a file, this function no longer
     * attempts configuration */
    if (configured != 0) {
        return ((configured == -1) ? -1 : 0);
    }

    /* configuration hasn't happened yet.  attempt it. */
    if (silk_config_file[0]) {
        /* sksiteSetConfigPath() was called. does the file exist? */
        if (!skFileExists(silk_config_file)) {
            /* Missing file---do not modify 'configured' */
            if (verbose) {
                skAppPrintErr("Site configuration file not found");
            }
            return -2;
        }
    } else {
        /* no config file set yet.  try to find it.  only set
         * silk_config_file if we find an existing file */
        if (!siteFindConfigPath(silk_config_file, sizeof(silk_config_file))) {
            /* we only get NULL if 'silk_config_file' is too small */
            if (verbose) {
                skAppPrintErr("Error getting site configuration file");
            }
            silk_config_file[0] = '\0';
            return -2;
        }
        if (!(silk_config_file[0] && skFileExists(silk_config_file))) {
            /* Missing file---do not modify 'configured' */
            if (verbose) {
                skAppPrintErr("Site configuration file not found");
            }
            silk_config_file[0] = '\0';
            return -2;
        }
    }

    /* we have a file; attempt to parse it */
    if (sksiteconfigParse(silk_config_file, verbose)) {
        /* Failed */
        configured = -1;
    } else {
        /* Success */
        configured = 1;
    }
    return ((configured == -1) ? -1 : 0);
}


int
sksiteSetConfigPath(
    const char         *filename)
{
    if (configured) {
        return -1;
    }
    if (NULL == filename || '\0' == *filename
        || strlen(filename) >= sizeof(silk_config_file))
    {
        return -1;
    }
    strncpy(silk_config_file, filename, sizeof(silk_config_file));
    return 0;
}


char *
sksiteGetConfigPath(
    char               *buffer,
    size_t              bufsize)
{
    /* if the site-config file is set, return it. */
    if (silk_config_file[0] != '\0') {
        if (bufsize <= strlen(silk_config_file)) {
            return NULL;
        }
        strncpy(buffer, silk_config_file, bufsize);
        return buffer;
    }

    /* else, return result of attempting to find it */
    return siteFindConfigPath(buffer, bufsize);
}


void
sksiteTeardown(
    void)
{
    static int teardown = 0;
    size_t count;
    size_t i;

    if (teardown) {
        return;
    }
    teardown = 1;

    if (fileformats.fl_list) {
        free(fileformats.fl_list);
        fileformats.fl_list = NULL;
    }
    if (compmethods.cml_list) {
        free(compmethods.cml_list);
        compmethods.cml_list = NULL;
    }
    if (class_list) {
        class_struct_t *cl;
        count = skVectorGetCount(class_list);
        for (i = 0; i < count; ++i) {
            skVectorGetValue(&cl, class_list, i);
            siteClassFree(cl);
        }
        skVectorDestroy(class_list);
    }
    if (flowtype_list) {
        flowtype_struct_t *ft;
        count = skVectorGetCount(flowtype_list);
        for (i = 0; i < count; ++i) {
            skVectorGetValue(&ft, flowtype_list, i);
            siteFlowtypeFree(ft);
        }
        skVectorDestroy(flowtype_list);
    }
    if (sensorgroup_list) {
        sensorgroup_struct_t *sg;
        count = skVectorGetCount(sensorgroup_list);
        for (i = 0; i < count; ++i) {
            skVectorGetValue(&sg, sensorgroup_list, i);
            siteSensorgroupFree(sg);
        }
        skVectorDestroy(sensorgroup_list);
    }
    if (sensor_list) {
        sensor_struct_t *sn;
        count = skVectorGetCount(sensor_list);
        for (i = 0; i < count; ++i) {
            skVectorGetValue(&sn, sensor_list, i);
            siteSensorFree(sn);
        }
        skVectorDestroy(sensor_list);
    }
}


/** Iterators *********************************************************/

int
sksiteSensorIteratorNext(
    sensor_iter_t      *iter,
    sensorID_t         *out_sensor_id)
{
    sensor_struct_t *sn = NULL;

    if (iter->si_vector == NULL) {
        return 0;
    }
    if (iter->si_contains_pointers) {
        while (sn == NULL) {
            if (skVectorGetValue(&sn, iter->si_vector, iter->si_index)) {
                return 0;
            }
            if (sn == NULL) {
                ++iter->si_index;
            }
        }
        *out_sensor_id = sn->sn_id;
        ++iter->si_index;
        return 1;
    } else {
        if (skVectorGetValue(out_sensor_id, iter->si_vector, iter->si_index)) {
            return 0;
        }
        ++iter->si_index;
        return 1;
    }
}


int
sksiteClassIteratorNext(
    class_iter_t       *iter,
    classID_t          *out_class_id)
{
    class_struct_t *cl = NULL;

    if (iter->ci_vector == NULL) {
        return 0;
    }
    if (iter->ci_contains_pointers) {
        while (cl == NULL) {
            if (skVectorGetValue(&cl, iter->ci_vector, iter->ci_index)) {
                return 0;
            }
            if (cl == NULL) {
                ++iter->ci_index;
            }
        }
        *out_class_id = cl->cl_id;
        ++iter->ci_index;
        return 1;
    } else {
        if (skVectorGetValue(out_class_id, iter->ci_vector, iter->ci_index)) {
            return 0;
        }
        ++iter->ci_index;
        return 1;
    }
}


int
sksiteSensorgroupIteratorNext(
    sensorgroup_iter_t *iter,
    sensorgroupID_t    *out_group_id)
{
    sensorgroup_struct_t *sg = NULL;

    if (iter->gi_vector == NULL) {
        return 0;
    }
    if (iter->gi_contains_pointers) {
        while (sg == NULL) {
            if (skVectorGetValue(&sg, iter->gi_vector, iter->gi_index)) {
                return 0;
            }
            if (sg == NULL) {
                ++iter->gi_index;
            }
        }
        *out_group_id = sg->sg_id;
        ++iter->gi_index;
        return 1;
    } else {
        if (skVectorGetValue(out_group_id, iter->gi_vector, iter->gi_index)) {
            return 0;
        }
        ++iter->gi_index;
        return 1;
    }
}


int
sksiteFlowtypeIteratorNext(
    flowtype_iter_t    *iter,
    flowtypeID_t       *out_flowtype_id)
{
    flowtype_struct_t *ft = NULL;

    if (iter->fi_vector == NULL) {
        return 0;
    }
    if (iter->fi_contains_pointers) {
        while (NULL == ft) {
            if (skVectorGetValue(&ft, iter->fi_vector, iter->fi_index)) {
                return 0;
            }
            if (NULL == ft) {
                ++iter->fi_index;
            }
        }
        *out_flowtype_id = ft->ft_id;
        ++iter->fi_index;
        return 1;
    } else {
        if (skVectorGetValue(out_flowtype_id, iter->fi_vector, iter->fi_index))
        {
            return 0;
        }
        ++iter->fi_index;
        return 1;
    }
}

/** Sensors ***********************************************************/

int
sksiteSensorCreate(
    sensorID_t          sensor_id,
    const char         *sensor_name)
{
    sensor_struct_t *sn = NULL;
    size_t vcap = skVectorGetCapacity(sensor_list);

    /* check bounds and length/legality of name */
    if (sensor_id >= SK_MAX_NUM_SENSORS) {
        return -1;
    }
    if (sksiteSensorNameIsLegal(sensor_name) != 0) {
        return -1;
    }

    /* verify sensor does not exist */
    if (sksiteSensorExists(sensor_id)) {
        return -1;
    }
    if (sksiteSensorLookup(sensor_name) != SK_INVALID_SENSOR) {
        return -1;
    }

    if (sensor_id >= vcap) {
        if (skVectorSetCapacity(sensor_list, sensor_id + 1)) {
            goto alloc_error;
        }
    }

    sn = (sensor_struct_t *) calloc(1, sizeof(sensor_struct_t));
    if (sn == NULL) {
        goto alloc_error;
    }
    sn->sn_name = strdup(sensor_name);
    sn->sn_class_list = skVectorNew(sizeof(classID_t));
    if ((sn->sn_name == NULL) ||
        (sn->sn_class_list == NULL))
    {
        goto alloc_error;
    }

    sn->sn_id = sensor_id;
    sn->sn_name_strlen = strlen(sensor_name);
    if (sn->sn_name_strlen > sensor_max_name_strlen) {
        sensor_max_name_strlen = sn->sn_name_strlen;
    }

    if (sensor_id > sensor_max_id) {
        sensor_max_id = sensor_id;
    }
    if ((sensor_min_id == -1) || (sensor_id < sensor_min_id)) {
        sensor_min_id = sensor_id;
    }
    if (skVectorSetValue(sensor_list, sensor_id, &sn)) {
        goto alloc_error;
    }

    return 0;

  alloc_error:
    siteSensorFree(sn);
    return -1;
}


/*
 *  siteSensorFree(sn);
 *
 *    Free all memory associated with the Sensor 'sn'; 'sn' may be
 *    NULL.
 */
static void
siteSensorFree(
    sensor_struct_t    *sn)
{
    if (sn != NULL) {
        if (sn->sn_class_list != NULL) {
            skVectorDestroy(sn->sn_class_list);
        }
        if (sn->sn_name != NULL) {
            free((char*)sn->sn_name);
        }
        if (sn->sn_description != NULL) {
            free((char*)sn->sn_description);
        }
        free(sn);
    }
}


sensorID_t
sksiteSensorLookup(
    const char         *sensor_name)
{
    sensorID_t id;
    sensor_struct_t *sn;

    for (id = 0; 0 == skVectorGetValue(&sn, sensor_list, id); ++id) {
        if ((sn != NULL) && (strcmp(sn->sn_name, sensor_name) == 0)) {
            return id;
        }
    }
    return SK_INVALID_SENSOR;
}


int
sksiteSensorExists(
    sensorID_t          sensor_id)
{
    sensor_struct_t *sn;

    if (skVectorGetValue(&sn, sensor_list, sensor_id) || (NULL == sn)) {
        return 0;
    }
    return 1;
}


sensorID_t
sksiteSensorGetMinID(
    void)
{
    return sensor_min_id;
}


sensorID_t
sksiteSensorGetMaxID(
    void)
{
    return sensor_max_id;
}


size_t
sksiteSensorGetMaxNameStrLen(
    void)
{
    return sensor_max_name_strlen;
}


int
sksiteSensorGetName(
    char               *buffer,
    size_t              buffer_size,
    sensorID_t          sensor_id)
{
    sensor_struct_t *sn;

    if (sensor_id == SK_INVALID_SENSOR) {
        /* Invalid sensor, give message */
        return snprintf(buffer, buffer_size, "%s", INVALID_LABEL);
    } else if (skVectorGetValue(&sn, sensor_list, sensor_id) || (NULL == sn)) {
        /* Unknown sensor, give numeric value */
        return snprintf(buffer, buffer_size, "%u", (unsigned int)sensor_id);
    } else {
        /* Known sensor, give name */
        return snprintf(buffer, buffer_size, "%s", sn->sn_name);
    }
}


int
sksiteIsSensorInClass(
    sensorID_t          sensor_id,
    classID_t           class_id)
{
    class_iter_t ci;
    classID_t check_id;

    sksiteSensorClassIterator(sensor_id, &ci);
    while (sksiteClassIteratorNext(&ci, &check_id)) {
        if (check_id == class_id) {
            return 1;
        }
    }
    return 0;
}


void
sksiteSensorIterator(
    sensor_iter_t      *iter)
{
    iter->si_index = 0;
    iter->si_vector = sensor_list;
    iter->si_contains_pointers = 1;
}


void
sksiteSensorClassIterator(
    sensorID_t          sensor_id,
    class_iter_t       *iter)
{
    sensor_struct_t *sn = NULL;

    iter->ci_index = 0;
    iter->ci_contains_pointers = 0;
    if (skVectorGetValue(&sn, sensor_list, sensor_id) || (NULL == sn)) {
        iter->ci_vector = NULL;
    } else {
        iter->ci_vector = sn->sn_class_list;
    }
}


int
sksiteSensorNameIsLegal(
    const char         *name)
{
    size_t len;

    if (NULL == name) {
        return -1;
    }
    len = strcspn(name, SITE_BAD_CHARS_SENSOR);
    /* check that length is between 1 and SK_MAX_STRLEN_FLOWTYPE */
    if (len < 1) {
        return -2;
    }
    if (len > SK_MAX_STRLEN_SENSOR) {
        return -3;
    }
    /* check that name begins with a letter */
    if (!isalpha((int)*name)) {
        return -1;
    }
    /* check whether we matched an invalid character */
    if (name[len] != '\0') {
        return len;
    }

    return 0;
}


int
sksiteSensorGetClassCount(
    sensorID_t          sensor_id)
{
    sensor_struct_t *sn = NULL;

    if (skVectorGetValue(&sn, sensor_list, sensor_id) || (NULL == sn)) {
        return 0;
    }
    return skVectorGetCount(sn->sn_class_list);
}


const char *
sksiteSensorGetDescription(
    sensorID_t          sensor_id)
{
    sensor_struct_t *sn = NULL;

    if (skVectorGetValue(&sn, sensor_list, sensor_id) || (NULL == sn)) {
        return NULL;
    }
    return sn->sn_description;
}


int
sksiteSensorSetDescription(
    sensorID_t          sensor_id,
    const char         *sensor_description)
{
    sensor_struct_t *sn = NULL;

    if (skVectorGetValue(&sn, sensor_list, sensor_id) || (NULL == sn)) {
        return -1;
    }

    if (sn->sn_description) {
        free((char*)sn->sn_description);
    }
    if (NULL == sensor_description) {
        sn->sn_description = NULL;
    } else {
        sn->sn_description = strdup(sensor_description);
        if (NULL == sn->sn_description) {
            return -1;
        }
    }
    return 0;
}


/** Classes ***********************************************************/

static int
sksiteFlowtypeNameIsLegal(
    const char         *name)
{
    size_t len;

    if (NULL == name) {
        return -1;
    }
    len = strcspn(name, SITE_BAD_CHARS_FLOWTYPE);
    /* check that length is between 1 and SK_MAX_STRLEN_FLOWTYPE */
    if (len < 1) {
        return -2;
    }
    if (len > SK_MAX_STRLEN_FLOWTYPE) {
        return -3;
    }
    /* check that name begins with a letter */
    if (!isalpha((int)*name)) {
        return -1;
    }
    /* check whether we matched an invalid character */
    if (name[len] != '\0') {
        return len;
    }

    return 0;
}


int
sksiteClassCreate(
    classID_t           class_id,
    const char         *class_name)
{
    class_struct_t *cl = NULL;
    size_t vcap = skVectorGetCapacity(class_list);

    /* check bounds and length/legality of name */
    if (class_id >= SK_MAX_NUM_CLASSES) {
        return -1;
    }
    if (sksiteFlowtypeNameIsLegal(class_name) != 0) {
        return -1;
    }

    /* verify class does not exist */
    if (sksiteClassExists(class_id)) {
        return -1;
    }
    if (sksiteClassLookup(class_name) != SK_INVALID_CLASS) {
        return -1;
    }

    if (class_id >= vcap) {
        if (skVectorSetCapacity(class_list, class_id + 1)) {
            goto alloc_error;
        }
    }

    cl = (class_struct_t *) calloc(1, sizeof(class_struct_t));
    if (cl == NULL) {
        goto alloc_error;
    }
    cl->cl_name = strdup(class_name);
    cl->cl_sensor_list = skVectorNew(sizeof(sensorID_t));
    cl->cl_flowtype_list = skVectorNew(sizeof(flowtypeID_t));
    cl->cl_default_flowtype_list = skVectorNew(sizeof(flowtypeID_t));
    if ((cl->cl_name == NULL) ||
        (cl->cl_sensor_list == NULL) ||
        (cl->cl_flowtype_list == NULL) ||
        (cl->cl_default_flowtype_list == NULL))
    {
        goto alloc_error;
    }

    cl->cl_id = class_id;
    cl->cl_name_strlen = strlen(class_name);
    if (cl->cl_name_strlen > class_max_name_strlen) {
        class_max_name_strlen = cl->cl_name_strlen;
    }

    if (class_id > class_max_id) {
        class_max_id = class_id;
    }

    if (skVectorSetValue(class_list, class_id, &cl)) {
        goto alloc_error;
    }

    return 0;

  alloc_error:
    siteClassFree(cl);
    return -1;
}


/*
 *  siteClassFree(sg);
 *
 *    Free all memory associated with the Class 'cl'; 'cl' may
 *    be NULL.  Does not free any sensors or flowtypes.
 */
static void
siteClassFree(
    class_struct_t     *cl)
{
    if (cl != NULL) {
        if (cl->cl_default_flowtype_list != NULL) {
            skVectorDestroy(cl->cl_default_flowtype_list);
        }

        if (cl->cl_flowtype_list != NULL) {
            skVectorDestroy(cl->cl_flowtype_list);
        }

        if (cl->cl_sensor_list != NULL) {
            skVectorDestroy(cl->cl_sensor_list);
        }

        if (cl->cl_name != NULL) {
            free((char*)cl->cl_name);
        }
        free(cl);
    }
}

int
sksiteClassSetDefault(
    classID_t           class_id)
{
    if (0 == sksiteClassExists(class_id)) {
        return -1;
    }
    default_class = class_id;
    return 0;
}


classID_t
sksiteClassGetDefault(
    void)
{
    return default_class;
}


classID_t
sksiteClassLookup(
    const char         *class_name)
{
    classID_t id;
    class_struct_t *cl;

    for (id = 0; 0 == skVectorGetValue(&cl, class_list, id); ++id) {
        if ((cl != NULL) && (strcmp(cl->cl_name, class_name) == 0)) {
            return id;
        }
    }
    return SK_INVALID_CLASS;
}


int
sksiteClassExists(
    classID_t           class_id)
{
    class_struct_t *cl;

    if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        return 0;
    }
    return 1;
}


classID_t
sksiteClassGetMaxID(
    void)
{
    return class_max_id;
}


size_t
sksiteClassGetMaxNameStrLen(
    void)
{
    return class_max_name_strlen;
}


int
sksiteClassGetName(
    char               *buffer,
    size_t              buffer_size,
    classID_t           class_id)
{
    class_struct_t *cl;

    if (class_id == SK_INVALID_CLASS) {
        /* Invalid class, give message */
        return snprintf(buffer, buffer_size, "%s", INVALID_LABEL);
    } else if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        /* Unknown class, give numeric value */
        return snprintf(buffer, buffer_size, "%u", (unsigned int)class_id);
    } else {
        /* Known value, print name */
        return snprintf(buffer, buffer_size, "%s", cl->cl_name);
    }
}


int
sksiteClassAddSensor(
    classID_t           class_id,
    sensorID_t          sensor_id)
{
    int i;
    class_struct_t *cl;
    sensor_struct_t *sn;
    sensorID_t id;

    if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        return -1;              /* Invalid class_id */
    }
    if (skVectorGetValue(&sn, sensor_list, sensor_id) || (NULL == sn)) {
        return -1;              /* Invalid sensor_id */
    }
    for (i = 0; 0 == skVectorGetValue(&id, cl->cl_sensor_list, i); ++i) {
        if (id == sensor_id) {
            return 0;
        }
    }
    /* XXX Recover and not add to class list either? */
    if (skVectorAppendValue(sn->sn_class_list, &class_id)) {
        return -1;
    }
    if (skVectorAppendValue(cl->cl_sensor_list, &sensor_id)) {
        return -1;
    }
    return 0;
}


int
sksiteClassAddSensorgroup(
    classID_t           class_id,
    sensorgroupID_t     group_id)
{
    int i;
    class_struct_t *cl;
    sensorgroup_struct_t *sg;
    sensorID_t id;

    if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        return -1;              /* Invalid class_id */
    }
    if (skVectorGetValue(&sg, sensorgroup_list, group_id) || (NULL == sg)) {
        return -1;              /* Invalid group_id */
    }
    for (i = 0; 0 == skVectorGetValue(&id, sg->sg_sensor_list, i); ++i) {
        if (sksiteClassAddSensor(class_id, id)) {
            return -1;
        }
    }
    return 0;
}


void
sksiteClassIterator(
    class_iter_t       *iter)
{
    iter->ci_index = 0;
    iter->ci_vector = class_list;
    iter->ci_contains_pointers = 1;
}


void
sksiteClassSensorIterator(
    classID_t           class_id,
    sensor_iter_t      *iter)
{
    class_struct_t *cl;

    iter->si_index = 0;
    iter->si_contains_pointers = 0;
    if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        iter->si_vector = NULL;
    } else {
        iter->si_vector = cl->cl_sensor_list;
    }
}


void
sksiteClassFlowtypeIterator(
    classID_t           class_id,
    flowtype_iter_t    *iter)
{
    class_struct_t *cl;

    memset(iter, 0, sizeof(flowtype_iter_t));
    if ((0 == skVectorGetValue(&cl, class_list, class_id)) && (NULL != cl)) {
        iter->fi_vector = cl->cl_flowtype_list;
    }
}


void
sksiteClassDefaultFlowtypeIterator(
    flowtypeID_t        class_id,
    flowtype_iter_t    *iter)
{
    class_struct_t *cl;

    if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        iter->fi_vector = NULL;
        return;
    }
    iter->fi_index = 0;
    iter->fi_vector = cl->cl_default_flowtype_list;
    iter->fi_contains_pointers = 0;
}


int
sksiteClassGetSensorCount(
    classID_t           class_id)
{
    class_struct_t *cl;

    if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        return 0;
    }
    return skVectorGetCount(cl->cl_sensor_list);
}


int
sksiteClassAddDefaultFlowtype(
    classID_t           class_id,
    flowtypeID_t        flowtype_id)
{
    int i;
    class_struct_t *cl;
    flowtype_struct_t *ft;
    flowtypeID_t id;

    if (skVectorGetValue(&ft, flowtype_list, flowtype_id) || (NULL == ft)) {
        return -1;
    }
    if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        return -1;
    }
    if (ft->ft_class != class_id) {
        return -1;
    }
    for (i=0; 0 == skVectorGetValue(&id, cl->cl_default_flowtype_list, i); ++i)
    {
        if (id == flowtype_id) {
            return 0;
        }
    }
    if (skVectorAppendValue(cl->cl_default_flowtype_list, &flowtype_id)) {
        return -1;
    }
    return 0;
}

/** Sensorgroups ******************************************************/

int
sksiteSensorgroupCreate(
    sensorgroupID_t     sensorgroup_id,
    const char         *sensorgroup_name)
{
    sensorgroup_struct_t *sg = NULL;
    size_t vcap = skVectorGetCapacity(sensorgroup_list);

    if (sensorgroup_id >= SK_MAX_NUM_SENSORGROUPS) {
        return -1;
    }

    /* verify sensorgroup does not exist */
    if (sksiteSensorgroupExists(sensorgroup_id)) {
        return -1;
    }
    if (sksiteSensorgroupLookup(sensorgroup_name) != SK_INVALID_SENSORGROUP) {
        return -1;
    }

    if (sensorgroup_id >= vcap) {
        if (skVectorSetCapacity(sensorgroup_list, sensorgroup_id + 1)) {
            goto alloc_error;
        }
    }

    sg = (sensorgroup_struct_t *) calloc(1, sizeof(sensorgroup_struct_t));
    if (sg == NULL) {
        goto alloc_error;
    }
    sg->sg_name = strdup(sensorgroup_name);
    sg->sg_sensor_list = skVectorNew(sizeof(sensorID_t));
    if ((sg->sg_name == NULL) ||
        (sg->sg_sensor_list == NULL))
    {
        goto alloc_error;
    }

    sg->sg_id = sensorgroup_id;
    sg->sg_name_strlen = strlen(sensorgroup_name);

    if (sg->sg_name_strlen > sensorgroup_max_name_strlen) {
        sensorgroup_max_name_strlen = sg->sg_name_strlen;
    }
    if (sensorgroup_id > sensorgroup_max_id) {
        sensorgroup_max_id = sensorgroup_id;
    }

    if (skVectorSetValue(sensorgroup_list, sensorgroup_id, &sg)) {
        goto alloc_error;
    }

    return 0;

  alloc_error:
    siteSensorgroupFree(sg);
    return -1;
}


/*
 *  siteSensorgroupFree(sg);
 *
 *    Free all memory associated with the Sensorgroup 'sg'; 'sg' may
 *    be NULL.
 */
static void
siteSensorgroupFree(
    sensorgroup_struct_t   *sg)
{
    if (sg != NULL) {
        if (sg->sg_sensor_list != NULL) {
            skVectorDestroy(sg->sg_sensor_list);
        }
        if (sg->sg_name != NULL) {
            free((char *)sg->sg_name);
        }
        free(sg);
    }
}


sensorgroupID_t
sksiteSensorgroupLookup(
    const char         *sensorgroup_name)
{
    sensorgroupID_t id;
    sensorgroup_struct_t *sg;

    for (id = 0; 0 == skVectorGetValue(&sg, sensorgroup_list, id); ++id) {
        if ((sg != NULL) && (strcmp(sg->sg_name, sensorgroup_name) == 0)) {
            return id;
        }
    }
    return SK_INVALID_SENSORGROUP;
}


int
sksiteSensorgroupExists(
    sensorgroupID_t     sensorgroup_id)
{
    sensorgroup_struct_t *sg;

    if (skVectorGetValue(&sg, sensorgroup_list, sensorgroup_id)|| (NULL == sg))
    {
        return 0;
    }
    return 1;
}


sensorgroupID_t
sksiteSensorgroupGetMaxID(
    void)
{
    return sensorgroup_max_id;
}


size_t
sksiteSensorgroupGetMaxNameStrLen(
    void)
{
    return sensorgroup_max_name_strlen;
}


int
sksiteSensorgroupGetName(
    char               *buffer,
    size_t              buffer_size,
    sensorgroupID_t     group_id)
{
    sensorgroup_struct_t *sg;

    if (group_id == SK_INVALID_SENSORGROUP) {
        /* Invalid group, give message */
        return snprintf(buffer, buffer_size, "%s", INVALID_LABEL);
    } else if (skVectorGetValue(&sg, sensorgroup_list, group_id)
               || (NULL == sg))
    {
        /* Unknown sensorgroup, give numeric value */
        return snprintf(buffer, buffer_size, "%u", (unsigned int)group_id);
    } else {
        /* Known sensorgroup, give name */
        return snprintf(buffer, buffer_size, "%s", sg->sg_name);
    }
}


int
sksiteSensorgroupAddSensor(
    sensorgroupID_t     group_id,
    sensorID_t          sensor_id)
{
    int i;
    sensorgroup_struct_t *sg;
    sensor_struct_t *sn;
    sensorID_t id;

    if (skVectorGetValue(&sg, sensorgroup_list, group_id) || (NULL == sg)) {
        return -1;              /* Invalid group_id */
    }
    if (skVectorGetValue(&sn, sensor_list, sensor_id) || (NULL == sn)) {
        return -1;              /* Invalid sensor_id */
    }
    for (i = 0; 0 == skVectorGetValue(&id, sg->sg_sensor_list, i); ++i) {
        if (id == sensor_id) {
            return 0;           /* Already there */
        }
    }
    if (skVectorAppendValue(sg->sg_sensor_list, &sensor_id)) {
        return -1;              /* Memory failure */
    }
    return 0;
}


int
sksiteSensorgroupAddSensorgroup(
    sensorgroupID_t     dest,
    sensorgroupID_t     src)
{
    int i;
    sensorgroup_struct_t *sg_src;
    sensorgroup_struct_t *sg_dest;
    sensorID_t id;

    if (skVectorGetValue(&sg_src, sensorgroup_list, src) || (NULL == sg_src)) {
        return -1;              /* Invalid source group_id */
    }
    if (skVectorGetValue(&sg_dest, sensorgroup_list, dest)||(NULL == sg_dest)){
        return -1;              /* Invalid dest group_id */
    }
    for (i = 0; 0 == skVectorGetValue(&id, sg_src->sg_sensor_list, i); ++i) {
        if (sksiteSensorgroupAddSensor(dest, id)) {
            return -1;
        }
    }
    return 0;
}


void
sksiteSensorgroupIterator(
    sensorgroup_iter_t *iter)
{
    iter->gi_index = 0;
    iter->gi_vector = sensorgroup_list;
    iter->gi_contains_pointers = 1;
}


void
sksiteSensorgroupSensorIterator(
    sensorgroupID_t     group_id,
    sensorgroup_iter_t *iter)
{
    sensorgroup_struct_t *sg;

    if (skVectorGetValue(&sg, sensorgroup_list, group_id) || (NULL == sg)) {
        iter->gi_vector = NULL;
        return;
    }
    iter->gi_index = 0;
    iter->gi_vector = sg->sg_sensor_list;
    iter->gi_contains_pointers = 0;
}

/** Flowtypes *********************************************************/

int
sksiteFlowtypeCreate(
    flowtypeID_t        flowtype_id,
    const char         *flowtype_name,
    classID_t           class_id,
    const char         *type_name)
/*  const char         *prefix,
    fileFormat_t        file_format,
    fileVersion_t       file_version, */
{
    flowtype_struct_t *ft = NULL;
    class_struct_t *cl = NULL;
    size_t vcap = skVectorGetCapacity(flowtype_list);

    assert(flowtype_name);
    assert(type_name);

    /* check bounds and length/legality of name */
    if (flowtype_id >= SK_MAX_NUM_FLOWTYPES) {
        return -1;
    }
    if (sksiteFlowtypeNameIsLegal(flowtype_name) != 0) {
        return -1;
    }
    if (sksiteFlowtypeNameIsLegal(type_name) != 0) {
        return -1;
    }
#if 0
    /* treat "all" as a reserved type name */
    if (strcmp("all", type_name)) {
        return -1;
    }
#endif

    /* verify class exists */
    if (skVectorGetValue(&cl, class_list, class_id) || (NULL == cl)) {
        return -1;
    }

    /* verify flowtype does not exist, and verify type is unique on
     * this class */
    if (sksiteFlowtypeExists(flowtype_id)) {
        return -1;
    }
    if (sksiteFlowtypeLookup(flowtype_name) != SK_INVALID_FLOWTYPE) {
        return -1;
    }
    if (sksiteFlowtypeLookupByClassIDType(class_id, type_name)
        != SK_INVALID_FLOWTYPE)
    {
        return -1;
    }

    if (flowtype_id >= vcap) {
        if (skVectorSetCapacity(flowtype_list, flowtype_id + 1)) {
            goto alloc_error;
        }
    }
    ft = (flowtype_struct_t *) calloc(1, sizeof(flowtype_struct_t));
    if (ft == NULL) {
        goto alloc_error;
    }

    ft->ft_id = flowtype_id;
    ft->ft_name = strdup(flowtype_name);
    ft->ft_type = strdup(type_name);
    if (ft->ft_name == NULL) {
        goto alloc_error;
    }

    ft->ft_class = class_id;

    ft->ft_name_strlen = strlen(flowtype_name);
    if (ft->ft_name_strlen > flowtype_max_name_strlen) {
        flowtype_max_name_strlen = ft->ft_name_strlen;
    }
    ft->ft_type_strlen = strlen(type_name);
    if (ft->ft_type_strlen > flowtype_max_type_strlen) {
        flowtype_max_type_strlen = ft->ft_type_strlen;
    }

    /* Now register it on the list */

    if (skVectorAppendValue(cl->cl_flowtype_list, &flowtype_id)) {
        goto alloc_error;
    }

    if (flowtype_id > flowtype_max_id) {
        flowtype_max_id = flowtype_id;
    }

    if (skVectorSetValue(flowtype_list, flowtype_id, &ft)) {
        goto alloc_error;
    }

    return 0;

  alloc_error:
    siteFlowtypeFree(ft);
    return -1;
}


/*
 *  siteFlowtypeFree(sg);
 *
 *    Free all memory associated with the Flowtype 'ft'; 'ft' may
 *    be NULL.
 */
static void
siteFlowtypeFree(
    flowtype_struct_t  *ft)
{
    if (ft != NULL) {
        if (ft->ft_name != NULL) {
            free((char*)ft->ft_name);
        }
        if (ft->ft_type != NULL) {
            free((char*)ft->ft_type);
        }
        free(ft);
    }
}


flowtypeID_t
sksiteFlowtypeLookup(
    const char         *flowtype_name)
{
    flowtypeID_t id;
    flowtype_struct_t *ft;

    for (id = 0; 0 == skVectorGetValue(&ft, flowtype_list, id); ++id) {
        if ((ft != NULL) && (strcmp(ft->ft_name, flowtype_name) == 0)) {
            return id;
        }
    }
    return SK_INVALID_FLOWTYPE;
}


flowtypeID_t
sksiteFlowtypeLookupByClassType(
    const char         *class_name,
    const char         *type_name)
{
    flowtype_struct_t *ft;
    flowtype_iter_t iter;
    flowtypeID_t id;
    classID_t class_id;

    if (class_name == NULL || type_name == NULL) {
        return SK_INVALID_FLOWTYPE;
    }

    class_id = sksiteClassLookup(class_name);
    sksiteClassFlowtypeIterator(class_id, &iter);
    while (sksiteFlowtypeIteratorNext(&iter, &id)) {
        if (0 == skVectorGetValue(&ft, flowtype_list, id)) {
            if ((ft != NULL) && (strcmp(ft->ft_type, type_name) == 0)) {
                return id;
            }
        }
    }

    /* not found */
    return SK_INVALID_FLOWTYPE;
}


flowtypeID_t
sksiteFlowtypeLookupByClassIDType(
    classID_t           class_id,
    const char         *type_name)
{
    flowtype_struct_t *ft;
    flowtype_iter_t iter;
    flowtypeID_t id;

    if (type_name == NULL) {
        return SK_INVALID_FLOWTYPE;
    }

    sksiteClassFlowtypeIterator(class_id, &iter);
    while (sksiteFlowtypeIteratorNext(&iter, &id)) {
        if (0 == skVectorGetValue(&ft, flowtype_list, id)) {
            if ((ft != NULL) && (strcmp(ft->ft_type, type_name) == 0)) {
                return id;
            }
        }
    }

    /* not found */
    return SK_INVALID_FLOWTYPE;
}


int
sksiteFlowtypeExists(
    flowtypeID_t        flowtype_id)
{
    flowtype_struct_t *ft;

    if (skVectorGetValue(&ft, flowtype_list, flowtype_id) || (NULL == ft)) {
        return 0;
    }
    return 1;
}


flowtypeID_t
sksiteFlowtypeGetMaxID(
    void)
{
    return flowtype_max_id;
}


int
sksiteFlowtypeGetClass(
    char               *buffer,
    size_t              buffer_size,
    flowtypeID_t        flowtype_id)
{
    flowtype_struct_t *ft;

    if (skVectorGetValue(&ft, flowtype_list, flowtype_id) || (NULL == ft)) {
        /* Unknown flowtype */
        return snprintf(buffer, buffer_size, "%s", INVALID_LABEL);
    } else {
        /* Known flowtype; lookup the class */
        return sksiteClassGetName(buffer, buffer_size, ft->ft_class);
    }
}


classID_t
sksiteFlowtypeGetClassID(
    flowtypeID_t        flowtype_id)
{
    flowtype_struct_t *ft;

    if (skVectorGetValue(&ft, flowtype_list, flowtype_id) || (NULL == ft)) {
        return SK_INVALID_CLASS;
    }
    return ft->ft_class;
}


size_t
sksiteFlowtypeGetMaxNameStrLen(
    void)
{
    return flowtype_max_name_strlen;
}


int
sksiteFlowtypeGetName(
    char               *buffer,
    size_t              buffer_size,
    flowtypeID_t        flowtype_id)
{
    flowtype_struct_t *ft;

    if (flowtype_id == SK_INVALID_FLOWTYPE) {
        /* Invalid flowtype, give message */
        return snprintf(buffer, buffer_size, "%s", INVALID_LABEL);
    } else if (skVectorGetValue(&ft, flowtype_list, flowtype_id)
               || (NULL == ft))
    {
        /* Unknown flowtype, give numeric value */
        return snprintf(buffer, buffer_size, "%u", (unsigned int)flowtype_id);
    } else {
        /* Known filetype, give name */
        return snprintf(buffer, buffer_size, "%s", ft->ft_name);
    }
}


size_t
sksiteFlowtypeGetMaxTypeStrLen(
    void)
{
    return flowtype_max_type_strlen;
}


int
sksiteFlowtypeGetType(
    char               *buffer,
    size_t              buffer_size,
    flowtypeID_t        flowtype_id)
{
    flowtype_struct_t *ft;

    if (skVectorGetValue(&ft, flowtype_list, flowtype_id) || (NULL == ft)) {
        /* Unknown flowtype, give numeric flowtype value */
        return snprintf(buffer, buffer_size, "%u", (unsigned int)flowtype_id);
    } else {
        /* Known flowtype, give string flowtype value */
        return snprintf(buffer, buffer_size, "%s", ft->ft_type);
    }
}


void
sksiteFlowtypeIterator(
    flowtype_iter_t    *iter)
{
    iter->fi_index = 0;
    iter->fi_vector = flowtype_list;
    iter->fi_contains_pointers = 1;
}


void
sksiteFlowtypeAssert(
    const char         *pack_logic_file,
    flowtypeID_t        flowtype_id,
    const char         *class_name,
    const char         *type_name)
{
    classID_t class_id;
    flowtypeID_t check_id = SK_INVALID_FLOWTYPE;

    class_id = sksiteClassLookup(class_name);
    if (class_id == SK_INVALID_CLASS) {
        goto FAIL_ASSERT;
    }
    check_id = sksiteFlowtypeLookupByClassIDType(class_id, type_name);
    if (check_id == SK_INVALID_FLOWTYPE) {
        goto FAIL_ASSERT;
    }
    if (check_id != flowtype_id) {
        goto FAIL_ASSERT;
    }

    /* all is well */
    return;

  FAIL_ASSERT:
#define ERROR_MSG                                                       \
    "Mismatch in packing-logic [%s] versus site-config-file [%s]: "

    if (class_id == SK_INVALID_CLASS) {
        skAppPrintErr((ERROR_MSG
                       "Class '%s' does not exist in site-config-file"),
                      pack_logic_file, silk_config_file,
                      class_name);
    } else if (check_id == SK_INVALID_FLOWTYPE) {
        skAppPrintErr((ERROR_MSG
                       "No flowtype for class/type '%s/%s' exists in"
                       " site-config-file"),
                      pack_logic_file, silk_config_file,
                      class_name, type_name);
    } else if (check_id != flowtype_id) {
        skAppPrintErr((ERROR_MSG
                       "Flowtype ID for class/type '%s/%s' (%d) in"
                       " site-config-file does not match ID in"
                       " packing-logic (%d)"),
                      pack_logic_file, silk_config_file,
                      class_name, type_name, check_id, flowtype_id);
    }
    abort();
#undef ERROR_MSG
}

/** Compatibility Functions *******************************************/


int
sksiteParseSensorList(
    sk_vector_t        *out_sensors,
    const char         *name_list)
{
    sensorID_t sensor_id;
    char *cp;
    char *ep;
    char *name_list_copy = NULL;
    int matches = 0;
    int rv;

    if ((name_list == NULL) || (out_sensors == NULL)) {
        rv = -1;
        goto END;
    }
    if (skVectorGetElementSize(out_sensors) != sizeof(sensorID_t)) {
        rv = -1;
        goto END;
    }
    name_list_copy = strdup(name_list);
    if (name_list_copy == NULL) {
        rv = -1;
        goto END;
    }
    cp = name_list_copy;
    while (*cp) {
        ep = strchr(cp, ',');
        if (ep == cp) {
            /* double comma, ignore */
            ++cp;
            continue;
        }
        if (ep == NULL) {
            /* no more commas, jump to end of string */
            ep = cp + strlen(cp);
        } else {
            *ep = '\0';
            ++ep;
        }
        sensor_id = sksiteSensorLookup(cp);
        if (sensor_id == SK_INVALID_SENSOR) {
            rv = 0;
            goto END;
        }
        /* add to vector */
        if (skVectorAppendValue(out_sensors, &sensor_id)) {
            rv = -1;
            goto END;
        }
        cp = ep;
        ++matches;
    }
    rv = matches;
  END:
    if (name_list_copy != NULL) {
        free(name_list_copy);
    }
    return rv;
}

/** File Output Formats ***********************************************/

static int
siteInitializeFileformats(
    size_t              sz)
{
    if (sz > UINT8_MAX) {
        return -1;
    }
    fileformats.fl_list = ((fileformat_struct_t*)
                           calloc(1+sz, sizeof(fileformat_struct_t)));
    fileformats.fl_count = (uint8_t)sz;
    if (fileformats.fl_list == NULL) {
        return -1;
    }
    return 0;
}


static fileFormat_t
siteFileformatAdd(
    const char         *name,
    uint8_t             id)
{
    fileformat_struct_t *ffi;

    /* check length of file format */
    if (strlen(name) > SK_MAX_STRLEN_FILE_FORMAT) {
        skAppPrintErr(("File format name '%s' is longer than allowed (%u)\n"
                       "\tFix you site header and recompile.  Abort!"),
                      name, SK_MAX_STRLEN_FILE_FORMAT);
        assert(strlen(name) <= SK_MAX_STRLEN_FILE_FORMAT);
        skAbort();
    }

    /* check id is o.k. */
    if (id >= siteGetFileformatCount()) {
        skAppPrintErr(("File format id '%u' is larger than allowed (%u)\n"
                       "\tFix you site header and recompile.  Abort!"),
                      id, siteGetFileformatCount());
        assert(id < siteGetFileformatCount());
        skAbort();
    }

    ffi = &fileformats.fl_list[id];
    ffi->ff_id = (fileFormat_t)id;
    ffi->ff_name = name;

    return ffi->ff_id;
}


int
sksiteFileformatGetName(
    char               *buffer,
    size_t              buffer_size,
    fileFormat_t        id)
{
    const char *name = siteFileformatGetName(id);

    if (name == NULL) {
        /* Unknown file format, give integer */
        return snprintf(buffer, buffer_size, "%s[%u]", INVALID_LABEL, id);
    } else {
        /* Known file format, give name */
        return snprintf(buffer, buffer_size, "%s", name);
    }
}


fileFormat_t
sksiteFileformatFromName(
    const char         *name)
{
    uint8_t i;

    for (i = 0; i < fileformats.fl_count; ++i) {
        if (strcmp(name, fileformats.fl_list[i].ff_name) == 0) {
            return fileformats.fl_list[i].ff_id;
        }
    }

    return siteGetFileformatCount();
}


int
sksiteFileformatIsValid(
    fileFormat_t        id)
{
    return (id < siteGetFileformatCount());
}


/** Compression Methods ***********************************************/


#define COMPMETHOD_STRING_BEST "best"

static int
siteInitializeCompmethods(
    size_t              sz)
{
    if (sz > UINT8_MAX) {
        return -1;
    }
    compmethods.cml_list = ((compmethod_struct_t*)
                            calloc(1+sz, sizeof(compmethod_struct_t)));
    compmethods.cml_count = (uint8_t)sz;
    if (compmethods.cml_list == NULL) {
        return -1;
    }
    return 0;
}


static sk_compmethod_t
siteCompmethodAdd(
    const char         *name,
    uint8_t             comp_method)
{
    compmethod_struct_t *cmi;

#if 0
    /* check length of compression method name */
    if (strlen(name) > SK_MAX_STRLEN_FILE_FORMAT) {
        skAppPrintErr(("Compression method name '%s'"
                       " is longer than allowed (%u)\n"
                       "\tFix you site header and recompile.  Abort!"),
                      name, SK_MAX_STRLEN_FILE_FORMAT);
        assert(strlen(cmi->cm_name) <= SK_MAX_STRLEN_FILE_FORMAT);
        skAbort();
    }
#endif

    /* check whether 'comp_method' is o.k. */
    if (comp_method >= siteGetCompmethodCount()) {
        skAppPrintErr(("Compression method id '%u'"
                       " is larger than allowed (%u)\n"
                       "\tFix your site header and recompile.  Abort!"),
                      comp_method, siteGetCompmethodCount());
        assert(comp_method < siteGetCompmethodCount());
        skAbort();
    }

    cmi = &compmethods.cml_list[comp_method];
    cmi->cm_id = (sk_compmethod_t)comp_method;
    cmi->cm_name = name;

    return cmi->cm_id;
}


int
sksiteCompmethodGetName(
    char               *buffer,
    size_t              buffer_size,
    sk_compmethod_t     comp_method)
{
    const char *name = siteCompmethodGetName(comp_method);

    if (NULL == name) {
        if (SK_COMPMETHOD_DEFAULT == comp_method) {
            name = siteCompmethodGetName(sksiteCompmethodGetDefault());
        } else if (SK_COMPMETHOD_BEST == comp_method) {
            name = COMPMETHOD_STRING_BEST;
        }
    }

    if (NULL == name) {
        /* Unknown compression method, give integer */
        return snprintf(buffer, buffer_size, "%u", comp_method);
    }
    /* Known compression method, give name */
    return snprintf(buffer, buffer_size, "%s", name);
}


int
sksiteCompmethodCheck(
    sk_compmethod_t     comp_method)
{
    switch (comp_method) {
      case SK_COMPMETHOD_DEFAULT:
      case SK_COMPMETHOD_BEST:
        return SK_COMPMETHOD_IS_KNOWN;

      case SK_COMPMETHOD_NONE:
#if SK_ENABLE_ZLIB
      case SK_COMPMETHOD_ZLIB:
#endif
#if SK_ENABLE_LZO
      case SK_COMPMETHOD_LZO1X:
#endif
        return SK_COMPMETHOD_IS_AVAIL;
    }

    if (((uint8_t)comp_method) < siteGetCompmethodCount()) {
        return SK_COMPMETHOD_IS_VALID;
    }
    return 0;
}


sk_compmethod_t
sksiteCompmethodGetBest(
    void)
{
#if   SK_ENABLE_LZO
    return SK_COMPMETHOD_LZO1X;
#elif SK_ENABLE_ZLIB
    return SK_COMPMETHOD_ZLIB;
#else
    return SK_COMPMETHOD_NONE;
#endif
}


sk_compmethod_t
sksiteCompmethodGetDefault(
    void)
{
    /* return the value from configure */
    return default_compression;
}


int
sksiteCompmethodSetDefault(
    sk_compmethod_t     comp_method)
{
    if (SK_COMPMETHOD_IS_AVAIL != sksiteCompmethodCheck(comp_method)) {
        return -1;
    }
    default_compression = comp_method;
    return 0;
}


/* ======================================================================== */


typedef enum {
    OPT_COMPRESSION_METHOD
} site_compmethod_opts_enum_t;

static struct option site_compmethod_opts[] = {
    {"compression-method",  REQUIRED_ARG, 0, OPT_COMPRESSION_METHOD},
    {0,0,0,0}               /* sentinel entry */
};


static int
siteCompmethodOptionsHandler(
    clientData          cData,
    int                 opt_index,
    char               *opt_arg)
{
    sk_compmethod_t *compression_method = (sk_compmethod_t*)cData;
    sk_stringmap_t *str_map = NULL;
    sk_stringmap_status_t rv_map;
    sk_stringmap_entry_t *map_entry;
    sk_stringmap_entry_t insert_entry;
    sk_compmethod_t cm;
    int rv = 1;

    if (opt_index != OPT_COMPRESSION_METHOD) {
        skAbort();
    }

    /* create a stringmap of the available entries */
    if (SKSTRINGMAP_OK != skStringMapCreate(&str_map)) {
        skAppPrintErr("Unable to create stringmap");
        goto END;
    }
    memset(&insert_entry, 0, sizeof(insert_entry));
    insert_entry.name = COMPMETHOD_STRING_BEST;
    insert_entry.id = (sk_stringmap_id_t)SK_COMPMETHOD_BEST;
    if (skStringMapAddEntries(str_map, 1, &insert_entry) != SKSTRINGMAP_OK) {
        goto END;
    }
    for (cm = 0; cm < siteGetCompmethodCount(); ++cm) {
        if (SK_COMPMETHOD_IS_AVAIL != sksiteCompmethodCheck(cm)) {
            continue;
        }
        memset(&insert_entry, 0, sizeof(insert_entry));
        insert_entry.name = siteCompmethodGetNameFast(cm);
        insert_entry.id = (sk_stringmap_id_t)cm;
        if (skStringMapAddEntries(str_map, 1, &insert_entry)
            != SKSTRINGMAP_OK)
        {
            goto END;
        }
    }

    /* attempt to match */
    rv_map = skStringMapGetByName(str_map, opt_arg, &map_entry);
    switch (rv_map) {
      case SKSTRINGMAP_OK:
        *compression_method = (uint8_t)map_entry->id;
        rv = 0;
        break;

      case SKSTRINGMAP_PARSE_AMBIGUOUS:
        skAppPrintErr("%s value '%s' is ambiguous",
                      site_compmethod_opts[OPT_COMPRESSION_METHOD].name,
                      opt_arg);
        goto END;

      case SKSTRINGMAP_PARSE_NO_MATCH:
        skAppPrintErr("%s value '%s' does not match any known method",
                      site_compmethod_opts[OPT_COMPRESSION_METHOD].name,
                      opt_arg);
        goto END;

      default:
        skAppPrintErr("Unexpected return value from string-map parser (%d)",
                      rv_map);
        goto END;
    }

  END:
    if (str_map) {
        skStringMapDestroy(str_map);
    }
    return rv;
}


int
sksiteCompmethodOptionsRegister(
    sk_compmethod_t    *compression_method)
{
    *compression_method = SK_COMPMETHOD_DEFAULT;

    return skOptionsRegister(site_compmethod_opts,
                             &siteCompmethodOptionsHandler,
                             compression_method);
}


void
sksiteCompmethodOptionsUsage(
    FILE               *fh)
{
    int i;
    sk_compmethod_t cm;

    for (i = 0; site_compmethod_opts[i].name; ++i) {
        fprintf(fh, "--%s %s. ", site_compmethod_opts[i].name,
                SK_OPTION_HAS_ARG(site_compmethod_opts[i]));
        switch ((site_compmethod_opts_enum_t)(site_compmethod_opts[i].val)) {
          case OPT_COMPRESSION_METHOD:
            fprintf(fh, ("Set compression for binary output file(s).\n"
                         "\tDef. %s. Choices: %s [=%s]"),
                    siteCompmethodGetNameFast(sksiteCompmethodGetDefault()),
                    COMPMETHOD_STRING_BEST,
                    siteCompmethodGetNameFast(sksiteCompmethodGetBest()));
            for (cm = 0; cm < siteGetCompmethodCount(); ++cm) {
                if (SK_COMPMETHOD_IS_AVAIL != sksiteCompmethodCheck(cm)) {
                    continue;
                }
                fprintf(fh, ", %s", siteCompmethodGetNameFast(cm));
            }
            break;
        }
        fprintf(fh, "\n");
    }
}


/** Paths *************************************************************/

const char *
sksiteGetDefaultRootDir(
    void)
{
    static char default_rootdir[PATH_MAX];
#ifdef SILK_DATA_ROOTDIR
    const char *root = "" SILK_DATA_ROOTDIR "";
#else
    const char *root = NULL;
#endif

#ifdef __CYGWIN__
    if (skCygwinGetDataRootDir(default_rootdir, sizeof(default_rootdir))
        != NULL)
    {
        return default_rootdir;
    }
#endif
    if (default_rootdir[0]) {
        return default_rootdir;
    }
    if (root && root[0] == '/') {
        strncpy(default_rootdir, root, sizeof(default_rootdir));
    } else {
        strncpy(default_rootdir, FALLBACK_DATA_ROOTDIR,
                sizeof(default_rootdir));
    }
    default_rootdir[sizeof(default_rootdir)-1] = '\0';
    return default_rootdir;
}


char *
sksiteGetRootDir(
    char               *buffer,
    size_t              bufsize)
{
    if (bufsize < 1+strlen(data_rootdir)) {
        return NULL;
    }
    strncpy(buffer, data_rootdir, bufsize);
    return buffer;
}


int
sksiteSetRootDir(
    const char         *rootdir)
{
    if (rootdir == NULL || rootdir[0] == '\0')  {
        return -1;
    }
    if (strlen(rootdir) >= sizeof(data_rootdir)) {
        return -1;
    }
    strncpy(data_rootdir, rootdir, sizeof(data_rootdir));
    return 0;
}


int
sksiteSetPathFormat(
    const char         *format)
{
    if ((format == NULL) || (format[0] == '\0')) {
        return -1;
    }
    if (strlen(format) + 1 > sizeof(path_format)) {
        return -1;
    }
    strncpy(path_format, format, sizeof(path_format));
    path_format[sizeof(path_format)-1] = '\0';
    return 0;
}


char *
sksiteGetPackingLogicPath(
    char               *buffer,
    size_t              bufsize)
{
    if (packing_logic_path[0] == '\0') {
        return NULL;
    }
    if (bufsize < 1+strlen(packing_logic_path)) {
        return NULL;
    }
    strncpy(buffer, packing_logic_path, bufsize);
    return buffer;
}


int
sksiteSetPackingLogicPath(
    const char         *pathname)
{
    if ((pathname == NULL) || (pathname[0] == '\0')) {
        return -1;
    }
    if (strlen(pathname) + 1 > sizeof(packing_logic_path)) {
        return -1;
    }
    strncpy(packing_logic_path, pathname, sizeof(packing_logic_path));
    packing_logic_path[sizeof(packing_logic_path)-1] = '\0';
    return 0;
}


char *
sksiteGeneratePathname(
    char               *buffer,
    size_t              bufsize,
    flowtypeID_t        flowtype_id,
    sensorID_t          sensor_id,
    sktime_t            timestamp,
    const char         *suffix,
    char              **reldir_begin,
    char              **filename_begin)
{
    /* convert sktime_t to time_t platforms */
    const time_t tt = (time_t)(timestamp / 1000);
    struct tm trec;
    char ftype_name_buffer[SK_MAX_STRLEN_FLOWTYPE+1];
    char sensor_name_buffer[SK_MAX_STRLEN_SENSOR+1];
    const char *suf = NULL;
    const char *pos;
    const char *next_pos;
    char *buf;
    size_t len;

    if (buffer == NULL || bufsize == 0) {
        return NULL;
    }

    if (!sksiteFlowtypeExists(flowtype_id)) {
        return NULL;
    }

    if (!sksiteSensorExists(sensor_id)) {
        return NULL;
    }

    /* set 'suf' to the suffix if it was provided and not the empty
     * string; ignore the leading '.' if suffix, it is added later. */
    if (suffix && *suffix) {
        suf = suffix;
        if (*suf == '.') {
            ++suf;
        }
    }

    gmtime_r(&tt, &trec);

    buf = buffer;

    /* First, add the data_rootdir */
    len = snprintf(buf, bufsize, "%s/", data_rootdir);
    if (len >= bufsize) {
        return NULL;
    }
    buf += len;
    bufsize -= len;

    /* Apply the format */
    pos = path_format;
    while (NULL != (next_pos = strchr(pos, '%'))) {
        assert(strchr(path_format_conversions, next_pos[1]));
        /* copy text we just jumped over */
        len = next_pos - pos;
        if (len >= bufsize) {
            return NULL;
        }
        strncpy(buf, pos, len);
        buf += len;
        bufsize -= len;
        /* handle conversion */
        pos = next_pos + 1;
        switch (*pos) {
          case '%':
            if (bufsize >= 1) {
                buf[0] = '%';
            }
            len = 1;
            break;
          case 'C':
            len = sksiteFlowtypeGetClass(buf, bufsize, flowtype_id);
            break;
          case 'F':
            len = sksiteFlowtypeGetName(buf, bufsize, flowtype_id);
            break;
          case 'H':
            len = snprintf(buf, bufsize, "%02d", trec.tm_hour);
            break;
          case 'N':
            len = sksiteSensorGetName(buf, bufsize, sensor_id);
            break;
          case 'T':
            len = sksiteFlowtypeGetType(buf, bufsize, flowtype_id);
            break;
          case 'Y':
            len = snprintf(buf, bufsize, "%04d", trec.tm_year + 1900);
            break;
          case 'd':
            len = snprintf(buf, bufsize, "%02d", trec.tm_mday);
            break;
          case 'f':
            len = snprintf(buf, bufsize, "%u", flowtype_id);
            break;
          case 'm':
            len = snprintf(buf, bufsize, "%02d", trec.tm_mon + 1);
            break;
          case 'n':
            len = snprintf(buf, bufsize, "%u", sensor_id);
            break;
          case 'x':
            sksiteFlowtypeGetName(ftype_name_buffer, sizeof(ftype_name_buffer),
                                  flowtype_id);
            sksiteSensorGetName(sensor_name_buffer,
                                sizeof(sensor_name_buffer), sensor_id);
            len = snprintf(buf, bufsize, "%s-%s_%04d%02d%02d.%02d",
                           ftype_name_buffer, sensor_name_buffer,
                           trec.tm_year + 1900,
                           trec.tm_mon + 1, trec.tm_mday, trec.tm_hour);
            break;
          default:
            skAbortBadCase((int)*pos);
        }
        if (len >= bufsize) {
            return NULL;
        }
        ++pos;
        buf += len;
        bufsize -= len;
    }
    /* handle remaining text (since %x is always last, this should
     * never be needed) */
    len = snprintf(buf, bufsize, "%s", pos);
    if (len >= bufsize) {
        return NULL;
    }
    buf += len;
    bufsize -= len;

    /* Optionally add suffix */
    if (suf) {
        len = snprintf(buf, bufsize, ".%s", suf);
        if (len >= bufsize) {
            return NULL;
        }
        buf += len;
        bufsize -= len;
    }

    /* And finally, add NUL (probably not necessary since NUL should
     * have been added at each step above) */
    if (bufsize == 0) {
        return NULL;
    }
    *buf = '\0';

    if (reldir_begin) {
        *reldir_begin = &buffer[1+strlen(data_rootdir)];
    }
    if (filename_begin) {
        *filename_begin = strrchr(buffer, '/') + 1;
    }

    return buffer;
}


flowtypeID_t
sksiteParseFilename(
    flowtypeID_t       *out_flowtype,
    sensorID_t         *out_sensor,
    sktime_t           *out_timestamp,
    const char        **out_suffix,
    const char         *filename)
{
    char buf[PATH_MAX];
    const char *cp;
    const char *sp;
    char *ep;
    flowtypeID_t ft;
    unsigned long temp1, temp2;

    /* check input */
    if (!filename) {
        return SK_INVALID_FLOWTYPE;
    }

    /* copy file portion of filename into buf */
    sp = skBasename_r(buf, filename, sizeof(buf));
    if (sp == NULL) {
        /* input name too long */
        return SK_INVALID_FLOWTYPE;
    }

    /* find the flowtype/sensor separator, which is a hyphen, e.g.,
     * "in-S2".  The while() loop is here to support flowtypes that
     * contain hyphens.  (For this to work correctly, we really should
     * make certain we do not allow one flowtype that is a substring of
     * another at the '-', e.g., "in" and "in-web"). */
    cp = sp;
    while ((ep = strchr(cp, '-')) != NULL) {
        *ep = '\0';

        /* see if file type exists */
        ft = sksiteFlowtypeLookup(sp);
        if (ft != SK_INVALID_FLOWTYPE) {
            /* it does */
            ++ep;
            break;
        }
        /* else we failed; restore 'ep' and move 'cp' to the character
         * after 'ep' and try again. */
        *ep = '-';
        cp = ep + 1;
    }
    if (NULL == ep) {
        return SK_INVALID_FLOWTYPE;
    }
    if (out_flowtype) {
        *out_flowtype = ft;
    }

    /* find the sensor/timestamp separator, which is an underscore,
     * e.g., "S2_20120926".  Sensors may not contain an underscore. */
    sp = ep;
    ep = strchr(sp, '_');
    if (NULL == ep) {
        return SK_INVALID_FLOWTYPE;
    }
    *ep = '\0';
    ++ep;

    if (out_sensor) {
        *out_sensor = sksiteSensorLookup(sp);
    }

    /* move to start of time; convert "YYYYMMDD." into a single
     * integer, then pull out each part */
    sp = ep;
    errno = 0;
    temp1 = strtoul(sp, &ep, 10);
    if (sp == ep || *ep != '.' || (temp1 == ULONG_MAX && errno == ERANGE)
        || (temp1 < 19700101 || temp1 >= 20380119))
    {
        return SK_INVALID_FLOWTYPE;
    }

    /* now handle the hour "HH." or "HH\0" */
    sp = ep + 1;
    errno = 0;
    temp2 = strtoul(sp, &ep, 10);
    if (sp == ep || (*ep != '.' && *ep != '\0')
        || (temp2 == ULONG_MAX && errno == ERANGE) || (temp2 > 23))
    {
        return SK_INVALID_FLOWTYPE;
    }

    if (out_timestamp) {
        struct tm trec;
        time_t t;

        memset(&trec, 0, sizeof(struct tm));
        trec.tm_mday = temp1 % 100;
        temp1 /= 100;
        trec.tm_mon  = temp1 % 100 - 1;
        trec.tm_year = (temp1 / 100) - 1900;
        trec.tm_hour = temp2;
        t = timegm(&trec);
        if (t == (time_t)(-1)) {
            return SK_INVALID_FLOWTYPE;
        }
        *out_timestamp = sktimeCreate(t, 0);
    }

    if (out_suffix) {
        *out_suffix = &filename[ep-buf];
    }

    return ft;
}


char *
sksiteParseGeneratePath(
    char               *buffer,
    size_t              bufsize,
    const char         *filename,
    const char         *suffix,
    char              **reldir_begin,
    char              **filename_begin)
{
    flowtypeID_t flowtype;
    sensorID_t sensor;
    sktime_t timestamp;
    const char *old_suffix;
    char new_suffix[PATH_MAX];

    if (sksiteParseFilename(&flowtype, &sensor, &timestamp, &old_suffix,
                            filename)
        == SK_INVALID_FLOWTYPE)
    {
        return NULL;
    }

    if (*old_suffix != '\0' && suffix == NULL) {
        /* there was a suffix on 'filename' and the caller didn't
         * provide a new suffix; append old suffix to new name */
        strncpy(new_suffix, old_suffix, sizeof(new_suffix));
        if (new_suffix[sizeof(new_suffix)-1] != '\0') {
            /* suffix too long */
            return NULL;
        }
        suffix = new_suffix;
    }

    return sksiteGeneratePathname(buffer, bufsize, flowtype, sensor, timestamp,
                                  suffix, reldir_begin, filename_begin);
}


/** Special Support Functions *****************************************/

/* typedef struct sksite_error_iterator_st sksite_error_iterator_t; */
struct sksite_error_iterator_st {
    sk_vector_t  *error_vector;
    size_t        pos;
};

typedef struct sksite_validation_error_st {
    int           error_code;
    const char   *error_string;
} sksite_validation_error_t;


void
sksiteErrorIteratorReset(
    sksite_error_iterator_t    *iter)
{
    iter->pos = UINT32_MAX;
}

int
sksiteErrorIteratorNext(
    sksite_error_iterator_t    *iter)
{
    if (iter->pos == UINT32_MAX) {
        if (0 == skVectorGetCount(iter->error_vector)) {
            return SK_ITERATOR_NO_MORE_ENTRIES;
        }
        iter->pos = 0;
        return SK_ITERATOR_OK;
    }
    if (iter->pos + 1 >= skVectorGetCount(iter->error_vector)) {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }
    ++iter->pos;
    return SK_ITERATOR_OK;
}

void
sksiteErrorIteratorFree(
    sksite_error_iterator_t    *iter)
{
    if (iter) {
        if (iter->error_vector) {
            skVectorDestroy(iter->error_vector);
        }
        free(iter);
    }
}


/*
 *  ok = siteErrorIterCreate(&iter, vec);
 *
 *    Create a new error iterator at the location specified by 'iter'.
 *    Have the iterator iterate over the elements in 'vec', a vector
 *    containing sksite_validation_error_t's.  'vec' must not be NULL.
 *
 *    Return 0 on success, or -1 on allocation error.
 */
static int
siteErrorIterCreate(
    sksite_error_iterator_t   **iter,
    sk_vector_t                *vec)
{
    assert(iter);
    assert(vec);
    assert(sizeof(sksite_validation_error_t) == skVectorGetElementSize(vec));

    *iter = (sksite_error_iterator_t*)malloc(sizeof(sksite_error_iterator_t));
    if (*iter == NULL) {
        return -1;
    }
    (*iter)->error_vector = vec;
    sksiteErrorIteratorReset(*iter);
    return 0;
}


/*
 *  name = siteErrorIterGetter(iter, action, &error_code);
 *
 *    Gets the current value for the error iterator, where the type of
 *    returned value depends on the 'action', as specified here:
 *
 *    1. Set 'error_code' to the numeric error code, a value defined
 *    in the sksite_validate_enum_t enumeration, and return the token
 *    that caused the error to be set.  When 'action' is 1,
 *    'error_code' must not be NULL.
 *
 *    2. Return the token that caused the error to be set.  Ignores
 *    paramter 'error_cde'
 *
 *    3. Return a pointer to a static buffer, where the buffer
 *    contains an appropriate error message given the code.  Ignores
 *    paramter 'error_code'.
 */
static const char *
siteErrorIterGetter(
    const sksite_error_iterator_t  *iter,
    int                             action,
    int                            *error_code)
{
    static char err_buf[1024];
    sksite_validation_error_t err;

    /* if action is 1, error_code not be NULL */
    assert(1 != action || NULL != error_code);

    if (NULL == iter) {
        return NULL;
    }
    if (skVectorGetValue(&err, iter->error_vector, iter->pos)) {
        return NULL;
    }

    if (1 == action) {
        *error_code = err.error_code;
        return err.error_string;
    }
    if (2 == action) {
        return err.error_string;
    }
    assert(3 == action);

    switch (err.error_code) {
      case SKSITE_ERR_MISSING_DELIM:
        snprintf(err_buf, sizeof(err_buf),
                 ("Invalid flowtype '%s': "
                  "The value does not contain the specified delimiter"),
                 err.error_string);
        break;
      case SKSITE_ERR_UNKNOWN_TYPE:
        snprintf(err_buf, sizeof(err_buf),
                 "Invalid flowtype '%s': The type is not recognized",
                 err.error_string);
        break;
      case SKSITE_ERR_UNKNOWN_CLASS:
        snprintf(err_buf, sizeof(err_buf),
                 "Invalid flowtype '%s': The class is not recognized",
                 err.error_string);
        break;
      case SKSITE_ERR_UNKNOWN_SENSOR:
        snprintf(err_buf, sizeof(err_buf),
                 "Invalid sensor '%s': The sensor is not recognized",
                 err.error_string);
        break;
      case SKSITE_ERR_UNKNOWN_SENSOR_ID:
        snprintf(err_buf, sizeof(err_buf),
                 "Invalid sensor ID %s: No sensor has that ID ",
                 err.error_string);
        break;
      case SKSITE_ERR_TYPE_NOT_IN_CLASS:
        snprintf(err_buf, sizeof(err_buf),
                 "Invalid flowtype '%s': The type does not exist in the class",
                 err.error_string);
        break;
      case SKSITE_ERR_SENSOR_NOT_IN_CLASS:
        snprintf(err_buf, sizeof(err_buf),
                 "Sensor '%s' is not a member of the specified class",
                 err.error_string);
        break;
      default:
        break;
    }

    err_buf[sizeof(err_buf)-1] = '\0';
    return err_buf;
}


int
sksiteErrorIteratorGetCode(
    const sksite_error_iterator_t  *iter)
{
    int error_code = -1;

    if (NULL == siteErrorIterGetter(iter, 1, &error_code)) {
        return -1;
    }
    return error_code;
}


const char *
sksiteErrorIteratorGetToken(
    const sksite_error_iterator_t  *iter)
{
    return siteErrorIterGetter(iter, 2, NULL);
}


const char *
sksiteErrorIteratorGetMessage(
    const sksite_error_iterator_t  *iter)
{
    return siteErrorIterGetter(iter, 3, NULL);
}


/*
 *  ok = sksiteValidateFlowtypes(ft_vec,ft_count,ft_strings,delim,error_iter);
 *
 *    Validate the class/type pairs specified in the character pointer
 *    array 'ft_strings'.  Each value in the array should contain a
 *    valid class name and type name, with the names separated by the
 *    character 'delim'.  The class name and/or the type name may be
 *    "all".
 *
 *    If 'ft_count' is non-negative, it is used as the number of
 *    entries in 'ft_strings'.  If 'ft_count' is negative,
 *    'ft_strings' is treated a NULL-terminated array.
 *
 *    The valid flowtype IDs are appended to the 'ft_vec' vector,
 *    unless the flowtype ID is already present in 'ft_vec'.
 *
 *    If 'error_iter' is non-NULL and an invalid class/type pair is
 *    encountered, a new sksite_error_iterator_t is allocated at the
 *    specified location, and an appropriate error code is added to
 *    the iterator, along with a pointer into the 'ft_strings' array.
 *    The caller must ensure that entries in the 'ft_strings' array
 *    remain valid while iterating over the errors.
 *
 *    The function returns 0 if all flowtypes were valid.  A return
 *    value of -1 indicates invalid input---for example, 'ft_vec'
 *    elements are not the correct size. A positive return value
 *    indicates the number of invalid class/type pairs.
 */
int
sksiteValidateFlowtypes(
    sk_vector_t                *flowtypes_vec,
    int                         flowtype_count,
    const char                **flowtype_strings,
    char                        delimiter,
    sksite_error_iterator_t   **error_iter)
{
    BITMAP_DECLARE(flowtype_seen, SK_MAX_NUM_FLOWTYPES);
    char class_name[2 + SK_MAX_STRLEN_FLOWTYPE];
    const char *type_name;
    const char *ft_string;
    flowtype_iter_t fi;
    flowtypeID_t ft;
    classID_t class_id;
    sk_vector_t *invalid_vec = NULL;
    sksite_validation_error_t err;
    int invalid_count = 0;
    size_t num_flowtypes;
    size_t i;
    int rv = -1;

    sksiteConfigure(0);

    /* get number of flowtypes */
    if (flowtype_count >= 0) {
        num_flowtypes = flowtype_count;
    } else {
        /* null terminated array; count entries */
        num_flowtypes = 0;
        while (flowtype_strings[num_flowtypes]) {
            ++num_flowtypes;
        }
    }
    if (0 == num_flowtypes) {
        return 0;
    }

    /* check the incoming vector */
    if (NULL == flowtypes_vec
        || skVectorGetElementSize(flowtypes_vec) != sizeof(flowtypeID_t))
    {
        goto END;
    }

    /* 'flowtype_seen' keeps track of which flowtypes we have seen;
     * initialize it with values from the incoming vector */
    BITMAP_INIT(flowtype_seen);
    for (i = 0; 0 == skVectorGetValue(&ft, flowtypes_vec, i); ++i) {
        BITMAP_SETBIT(flowtype_seen, ft);
    }

    /* create the array that holds invalid tokens */
    if (error_iter) {
        invalid_vec = skVectorNew(sizeof(sksite_validation_error_t));
        if (NULL == invalid_vec) {
            goto END;
        }
    }

    /* process each string in 'flowtype_strings' */
    for (i = 0; i < num_flowtypes; ++i) {
        ft_string = flowtype_strings[i];

        /* copy class part of the string into a separate buffer */
        if ('\0' == delimiter) {
            type_name = ft_string + strlen(ft_string);
        } else {
            type_name = strchr(ft_string, delimiter);
            if (NULL == type_name) {
                ++invalid_count;
                if (invalid_vec) {
                    err.error_code = SKSITE_ERR_MISSING_DELIM;
                    err.error_string = ft_string;
                    if (skVectorAppendValue(invalid_vec, &err)) {
                        goto END;
                    }
                }
                continue;
            }
        }
        if ((type_name - ft_string) > ((int)sizeof(class_name) - 1)) {
            ++invalid_count;
            if (invalid_vec) {
                err.error_code = SKSITE_ERR_UNKNOWN_CLASS;
                err.error_string = ft_string;
                if (skVectorAppendValue(invalid_vec, &err)) {
                    goto END;
                }
                continue;
            }
        }
        strncpy(class_name, ft_string, sizeof(class_name));
        class_name[type_name - ft_string] = '\0';
        ++type_name;

        /* find class and type.  if lookup fails, test for special
         * "all" keyword */
        ft = sksiteFlowtypeLookupByClassType(class_name, type_name);
        if (SK_INVALID_FLOWTYPE != ft) {
            /* Class and type are specific */
            if (!BITMAP_GETBIT(flowtype_seen, ft)) {
                BITMAP_SETBIT(flowtype_seen, ft);
                if (skVectorAppendValue(flowtypes_vec, &ft)) {
                    goto END;
                }
            }

        } else if (0 == strcmp(class_name, "all")) {
            if (0 == strcmp(type_name, "all")) {
                /* Use all classes and all types. */
                sksiteFlowtypeIterator(&fi);
                while (sksiteFlowtypeIteratorNext(&fi, &ft)) {
                    if (!BITMAP_GETBIT(flowtype_seen, ft)) {
                        BITMAP_SETBIT(flowtype_seen, ft);
                        if (skVectorAppendValue(flowtypes_vec, &ft)) {
                            goto END;
                        }
                    }
                }
            } else {
                /* Loop over all classes and add flowtype if type_name
                 * is valid for that class.  Don't complain unless the
                 * type in not valid for any class. */
                class_iter_t ci;
                int found_type = 0;

                sksiteClassIterator(&ci);
                while (sksiteClassIteratorNext(&ci, &class_id)) {
                    ft = sksiteFlowtypeLookupByClassIDType(class_id,type_name);
                    if (SK_INVALID_FLOWTYPE != ft) {
                        ++found_type;
                        if (!BITMAP_GETBIT(flowtype_seen, ft)) {
                            BITMAP_SETBIT(flowtype_seen, ft);
                            if (skVectorAppendValue(flowtypes_vec, &ft)) {
                                goto END;
                            }
                        }
                    }
                }
                if (!found_type) {
                    ++invalid_count;
                    if (invalid_vec) {
                        err.error_code = SKSITE_ERR_UNKNOWN_TYPE;
                        err.error_string = ft_string;
                        if (skVectorAppendValue(invalid_vec, &err)) {
                            goto END;
                        }
                    }
                }
            }

        } else if (0 == strcmp(type_name, "all")) {
            /* Use all types in the specified class */
            class_id = sksiteClassLookup(class_name);
            if (SK_INVALID_CLASS == class_id) {
                ++invalid_count;
                if (invalid_vec) {
                    err.error_code = SKSITE_ERR_UNKNOWN_CLASS;
                    err.error_string = ft_string;
                    if (skVectorAppendValue(invalid_vec, &err)) {
                        goto END;
                    }
                }
            } else {
                sksiteClassFlowtypeIterator(class_id, &fi);
                while (sksiteFlowtypeIteratorNext(&fi, &ft)) {
                    if (!BITMAP_GETBIT(flowtype_seen, ft)) {
                        BITMAP_SETBIT(flowtype_seen, ft);
                        if (skVectorAppendValue(flowtypes_vec, &ft)) {
                            goto END;
                        }
                    }
                }
            }

        } else {
            /* Invalid class/type */
            ++invalid_count;
            if (invalid_vec) {
                if (SK_INVALID_CLASS == sksiteClassLookup(class_name)) {
                    err.error_code = SKSITE_ERR_UNKNOWN_CLASS;
                } else {
                    err.error_code = SKSITE_ERR_TYPE_NOT_IN_CLASS;
                }
                err.error_string = ft_string;
                if (skVectorAppendValue(invalid_vec, &err)) {
                    goto END;
                }
            }
        }
    }

    /* create the error iterator if we encountered invalid tokens */
    if (NULL != error_iter && invalid_count > 0) {
        if (siteErrorIterCreate(error_iter, invalid_vec)) {
            goto END;
        }
        /* set to NULL so we don't destroy it below */
        invalid_vec = NULL;
    }

    rv = 0;

  END:
    if (invalid_vec) {
        skVectorDestroy(invalid_vec);
    }
    if (0 == rv) {
        return invalid_count;
    }
    return rv;
}


/*
 *  ok = sksiteValidateSensors(s_vec, ft_vec, s_count, s_strings, error_iter);
 *
 *    Validate the sensor names and/or sensor IDs listed in the
 *    character pointer array 's_strings'.  Each value in the array
 *    should contain a valid sensor name or a valid sensor numeric ID.
 *
 *    If 's_count' is non-negative, it is used as the number of
 *    entries in 's_strings'.  If 's_count' is negative, 's_strings'
 *    is treated a NULL-terminated array.
 *
 *    The valid sensor IDs are appended to the 's_vec' vector,
 *    unless the sensor ID is already present in 's_vec'.
 *
 *    If 'ft_vec' is non-NULL, it should point to a vector containing
 *    flowtype IDs, and only sensors that exist in the specified
 *    flowtypes will be added to 's_vec'.  Other sensors are treated
 *    as invalid.
 *
 *    If 'error_iter' is non-NULL and an invalid sensor is
 *    encountered, a new sksite_error_iterator_t is allocated at the
 *    specified location, and an appropriate error code is added to
 *    the iterator, along with a pointer into the 's_strings' array.
 *    The caller must ensure that entries in the 's_strings' array
 *    remain valid while iterating over the errors.
 *
 *    The function returns 0 if all sensors were valid.  A return
 *    value of -1 indicates invalid input---for example, 's_vec'
 *    elements are not the correct size. A positive return value
 *    indicates the number of invalid sensors.
 */
int
sksiteValidateSensors(
    sk_vector_t                *sensors_vec,
    const sk_vector_t          *flowtypes_vec,
    int                         sensor_count,
    const char                **sensor_strings,
    sksite_error_iterator_t   **error_iter)
{
    BITMAP_DECLARE(classes, SK_MAX_NUM_FLOWTYPES);
    const char *sen_string = NULL;
    sk_bitmap_t *sensor_bits = NULL;
    uint32_t min_sensor_id;
    uint32_t max_sensor_id;
    uint32_t tmp32;
    sensorID_t sid = SK_INVALID_SENSOR;
    flowtypeID_t ft;
    int found_sensor;
    sk_vector_t *invalid_vec = NULL;
    sksite_validation_error_t err;
    class_iter_t ci;
    classID_t class_of_sensor;
    int invalid_count = 0;
    size_t num_sensors;
    size_t i;
    int rv = -1;

    min_sensor_id = sksiteSensorGetMinID();
    max_sensor_id = sksiteSensorGetMaxID();
    BITMAP_INIT(classes);

    /* get number of sensors */
    if (sensor_count >= 0) {
        num_sensors = sensor_count;
    } else {
        /* null terminated array; count entries */
        num_sensors = 0;
        while (sensor_strings[num_sensors]) {
            ++num_sensors;
        }
    }
    if (0 == num_sensors) {
        return 0;
    }

    /* check the incoming vector */
    if (NULL == sensors_vec
        || skVectorGetElementSize(sensors_vec) != sizeof(sensorID_t))
    {
        goto END;
    }

    /* if flowtypes_vec was given, we limit the sensors to the sensors
     * that appear in those classes */
    if (NULL == flowtypes_vec) {
        /* accept all sensors */
        memset(classes, 0xFF, sizeof(classes));
    } else {
        if (skVectorGetElementSize(flowtypes_vec) != sizeof(flowtypeID_t)) {
            goto END;
        }
        for (i = 0; 0 == skVectorGetValue(&ft, flowtypes_vec, i); ++i) {
            BITMAP_SETBIT(classes, sksiteFlowtypeGetClassID(ft));
        }
    }

    /* create a bitmap for all the sensors */
    if (skBitmapCreate(&sensor_bits, 1 + max_sensor_id)) {
        goto END;
    }

    /* Sets bits in 'sensor_bits' for IDs already present in the vector */
    for (i = 0; 0 == skVectorGetValue(&sid, sensors_vec, i); ++i) {
        skBitmapSetBit(sensor_bits, sid);
    }

    /* create the array that holds invalid tokens */
    if (error_iter) {
        invalid_vec = skVectorNew(sizeof(sksite_validation_error_t));
        if (NULL == invalid_vec) {
            goto END;
        }
    }

    /* process at each string in 'sensor_strings' */
    for (i = 0; i < num_sensors; ++i) {
        sen_string = sensor_strings[i];

        /* lookup sen_string as a sensor name; if that fails, try it
         * as a sensor id */
        sid = sksiteSensorLookup(sen_string);
        if (SK_INVALID_SENSOR == sid) {
            if (0 != skStringParseUint32(&tmp32, sen_string,
                                         min_sensor_id, max_sensor_id))
            {
                ++invalid_count;
                if (invalid_vec) {
                    err.error_code = SKSITE_ERR_UNKNOWN_SENSOR;
                    err.error_string = sen_string;
                    if (skVectorAppendValue(invalid_vec, &err)) {
                        goto END;
                    }
                }
                continue;
            }
            sid = (sensorID_t)tmp32;
            if (!sksiteSensorExists(sid)) {
                ++invalid_count;
                if (invalid_vec) {
                    err.error_code = SKSITE_ERR_UNKNOWN_SENSOR_ID;
                    err.error_string = sen_string;
                    if (skVectorAppendValue(invalid_vec, &err)) {
                        goto END;
                    }
                }
                continue;
            }
        }

        if (!skBitmapGetBit(sensor_bits, sid)) {
            if (NULL == flowtypes_vec) {
                if (skVectorAppendValue(sensors_vec, &sid)) {
                    goto END;
                }
            } else {
                /* loop 'class_of_sensor' over all classes that 'sid'
                 * is a member of */
                found_sensor = 0;
                sksiteSensorClassIterator(sid, &ci);
                while (sksiteClassIteratorNext(&ci, &class_of_sensor)) {
                    if (BITMAP_GETBIT(classes, class_of_sensor)) {
                        found_sensor = 1;
                        skBitmapSetBit(sensor_bits, sid);
                        if (skVectorAppendValue(sensors_vec, &sid)) {
                            goto END;
                        }
                        break;
                    }
                }

                /* warn about unused sensor */
                if (0 == found_sensor) {
                    ++invalid_count;
                    if (invalid_vec) {
                        err.error_code = SKSITE_ERR_SENSOR_NOT_IN_CLASS;
                        err.error_string = sen_string;
                        if (skVectorAppendValue(invalid_vec, &err)) {
                            goto END;
                        }
                    }
                }
            }
        }
    }

    /* create the error iterator if we encountered invalid tokens */
    if (NULL != error_iter && invalid_count > 0) {
        if (siteErrorIterCreate(error_iter, invalid_vec)) {
            goto END;
        }
        /* set to NULL so we don't destroy it below */
        invalid_vec = NULL;
    }

    rv = 0;

  END:
    if (sensor_bits) {
        skBitmapDestroy(&sensor_bits);
    }
    if (invalid_vec) {
        skVectorDestroy(invalid_vec);
    }
    if (0 == rv) {
        return invalid_count;
    }
    return rv;
}


/** DATA_ROOTDIR File Iteration (fglob) *******************************/

/* typedef struct sksite_repo_iter_st sksite_repo_iter_t; */
struct sksite_repo_iter_st {
    sk_vector_t    *sen_ft_vec;
    size_t          sensor_idx;
    sktime_t        time_start;
    sktime_t        time_end;
    sktime_t        time_idx;
    uint32_t        flags;
    uint8_t         first_call;
};

typedef struct sensor_flowtype_st {
    sensorID_t      sensor;
    flowtypeID_t    flowtype;
} sensor_flowtype_t;


/*
 *  more_files = siteRepoIterIncrement(iter, &attr);
 *
 *    Increment the file iterator so that it points to the next file,
 *    and set the values in 'attr' to the tuple for that file.
 *
 *    Return 1 if the iterator moved to the next file, or 0 if there
 *    are no more files.
 */
static int
siteRepoIterIncrement(
    sksite_repo_iter_t *iter,
    sksite_fileattr_t  *attr)
{
    sensor_flowtype_t sen_ft;

    /* Make certain we haven't reached the end of the data. */
    if (iter->time_idx > iter->time_end) {
        return 0;
    }

    if (iter->first_call) {
        iter->first_call = 0;
        assert(iter->sensor_idx == 0);
        if (skVectorGetValue(&sen_ft, iter->sen_ft_vec, iter->sensor_idx) == 0)
        {
            attr->sensor = sen_ft.sensor;
            attr->flowtype = sen_ft.flowtype;
            attr->timestamp = iter->time_idx;
            return 1;
        }
        /* Empty iterator */
        return 0;
    }

    /* First, see if we can increment the sensor/flowtype pair */
    ++iter->sensor_idx;
    if (skVectorGetValue(&sen_ft, iter->sen_ft_vec, iter->sensor_idx) == 0) {
        attr->sensor = sen_ft.sensor;
        attr->flowtype = sen_ft.flowtype;
        attr->timestamp = iter->time_idx;
        return 1;
    }
    /* On last sensor/flowtype; reset and try time */
    iter->sensor_idx = 0;

    /* Finally, increment the time: go to next hour */
    iter->time_idx += 3600000;
    if (iter->time_idx > iter->time_end) {
        /* We're done. */
        return 0;
    }

    if (skVectorGetValue(&sen_ft, iter->sen_ft_vec, iter->sensor_idx) != 0) {
        return 0;
    }
    attr->sensor = sen_ft.sensor;
    attr->flowtype = sen_ft.flowtype;
    attr->timestamp = iter->time_idx;
    return 1;
}


/*
 *  more_files = siteRepoIterNext(iter, &attr, name, name_len, &is_missing);
 *
 *    Increment the file iterator so that it points to the next file,
 *    set the values in 'attr' to the tuple for that file, set 'name'
 *    to the pathname to that file, and, if 'is_missing' is not NULL,
 *    set 'is_missing' to 0 if the file exists, or 1 if it does not.
 *
 *    Return SK_ITERATOR_OK if the iterator moved to the next file, or
 *    SK_ITERATOR_NO_MORE_ENTRIES if there are no more files.
 */
static int
siteRepoIterNext(
    sksite_repo_iter_t *iter,
    sksite_fileattr_t  *attr,
    char               *name,
    size_t              name_len,
    int                *is_missing)
{
    const char suffix[] = ".gz";
    char *cp;

    assert(iter);
    assert(attr);
    assert(name);

    while (siteRepoIterIncrement(iter, attr)) {

        /* check whether file exists */
        if (NULL == sksiteGeneratePathname(name, name_len, attr->flowtype,
                                           attr->sensor, attr->timestamp,
                                           suffix, NULL, NULL))
        {
            /* error */
            continue;
        }

        cp = &name[strlen(name) + 1 - sizeof(suffix)];
        *cp = '\0';
        if (skFileExists(name)) {
            if (is_missing) {
                *is_missing = 0;
            }
            return SK_ITERATOR_OK;
        }
        *cp = '.';
        if (skFileExists(name)) {
            if (is_missing) {
                *is_missing = 0;
            }
            return SK_ITERATOR_OK;
        }
        *cp = '\0';
        if (iter->flags & RETURN_MISSING) {
            if (is_missing) {
                *is_missing = 1;
            }
            return SK_ITERATOR_OK;
        }
    }

    return SK_ITERATOR_NO_MORE_ENTRIES;
}


int
sksiteRepoIteratorCreate(
    sksite_repo_iter_t    **iter,
    const sk_vector_t      *flowtypes_vec,
    const sk_vector_t      *sensor_vec,
    sktime_t                start_time,
    sktime_t                end_time,
    uint32_t                flags)
{
    sensor_flowtype_t sen_ft;
    sensor_iter_t sen_iter;
    flowtypeID_t ft;
    classID_t class_id;
    sensorID_t sid;
    size_t i, j;

    if (NULL == iter
        || NULL == flowtypes_vec
        || sizeof(flowtypeID_t) != skVectorGetElementSize(flowtypes_vec)
        || end_time < start_time)
    {
        return -1;
    }
    if (sensor_vec
        && sizeof(sensorID_t) != skVectorGetElementSize(sensor_vec))
    {
        return -1;
    }

    memset(&sen_ft, 0, sizeof(sensor_flowtype_t));

    *iter = (sksite_repo_iter_t*)calloc(1, sizeof(sksite_repo_iter_t));
    if (NULL == *iter) {
        return -1;
    }

    (*iter)->sen_ft_vec = skVectorNew(sizeof(sensor_flowtype_t));
    if (NULL == (*iter)->sen_ft_vec) {
        sksiteRepoIteratorDestroy(iter);
        return -1;
    }

    (*iter)->time_start = start_time;
    (*iter)->time_end = end_time;
    (*iter)->flags = flags;

    for (i = 0; 0 == skVectorGetValue(&ft, flowtypes_vec, i); ++i) {
        class_id = sksiteFlowtypeGetClassID(ft);
        if (NULL == sensor_vec) {
            sksiteClassSensorIterator(class_id, &sen_iter);
            while (sksiteSensorIteratorNext(&sen_iter, &sid)) {
                sen_ft.sensor = sid;
                sen_ft.flowtype = ft;
                if (skVectorAppendValue((*iter)->sen_ft_vec, &sen_ft)) {
                    sksiteRepoIteratorDestroy(iter);
                    return -1;
                }
            }
        } else {
            for (j = 0; 0 == skVectorGetValue(&sid, sensor_vec, j); ++j) {
                if (sksiteIsSensorInClass(sid, class_id)) {
                    sen_ft.sensor = sid;
                    sen_ft.flowtype = ft;
                    if (skVectorAppendValue((*iter)->sen_ft_vec, &sen_ft)) {
                        sksiteRepoIteratorDestroy(iter);
                        return -1;
                    }
                }
            }
        }
    }

    sksiteRepoIteratorReset(*iter);

    return 0;
}


void
sksiteRepoIteratorDestroy(
    sksite_repo_iter_t    **iter)
{
    if (iter && *iter) {
        if ((*iter)->sen_ft_vec) {
            skVectorDestroy((*iter)->sen_ft_vec);
        }
        memset(*iter, 0, sizeof(sksite_repo_iter_t));
        free(*iter);
    }
}


int
sksiteRepoIteratorNextFileattr(
    sksite_repo_iter_t *iter,
    sksite_fileattr_t  *fileattr,
    int                *is_missing)
{
    char path[PATH_MAX];

    return siteRepoIterNext(iter, fileattr, path, sizeof(path), is_missing);
}

int
sksiteRepoIteratorNextPath(
    sksite_repo_iter_t *iter,
    char               *path,
    size_t              path_len,
    int                *is_missing)
{
    sksite_fileattr_t attr;

    return siteRepoIterNext(iter, &attr, path, path_len, is_missing);
}


int
sksiteRepoIteratorNextStream(
    sksite_repo_iter_t     *iter,
    skstream_t            **stream,
    int                    *is_missing,
    sk_msg_fn_t             err_fn)
{
    char path[PATH_MAX];
    sksite_fileattr_t attr;
    int file_missing;
    int rv;

    if (NULL == is_missing) {
        is_missing = &file_missing;
    }

    do {
        rv = siteRepoIterNext(iter, &attr, path, sizeof(path), is_missing);
        if (0 != rv) {
            return rv;
        }

        if (*is_missing) {
            if ((rv = skStreamCreate(stream, SK_IO_READ, SK_CONTENT_SILK_FLOW))
                || (rv = skStreamBind(*stream, path)))
            {
                if (err_fn) {
                    skStreamPrintLastErr(*stream, rv, err_fn);
                }
                skStreamDestroy(stream);
            }
        } else {
            rv = skStreamOpenSilkFlow(stream, path, SK_IO_READ);
            if (0 != rv) {
                if (err_fn) {
                    skStreamPrintLastErr(*stream, rv, err_fn);
                }
                skStreamDestroy(stream);
            }
        }
    } while (0 != rv);

    return rv;
}


size_t
sksiteRepoIteratorGetFileattrs(
    sksite_repo_iter_t *iter,
    sksite_fileattr_t  *attr_array,
    size_t              attr_max_count)
{
    char path[PATH_MAX];
    int is_missing;
    size_t count = 0;
    sksite_fileattr_t *attr = attr_array;
    int rv;

    while (attr_max_count > 0) {
        --attr_max_count;
        rv = siteRepoIterNext(iter, attr, path, sizeof(path), &is_missing);
        if (rv) {
            return count;
        }
        ++count;
        ++attr;
    }
    return count;
}

int
sksiteRepoIteratorRemainingFileattrs(
    sksite_repo_iter_t *iter,
    sk_vector_t        *fileattr_vec)
{
    char path[PATH_MAX];
    int is_missing;
    sksite_fileattr_t attr;

    if (NULL == fileattr_vec
        || sizeof(sksite_fileattr_t) != skVectorGetElementSize(fileattr_vec))
    {
        return -1;
    }

    while (siteRepoIterNext(iter, &attr, path, sizeof(path), &is_missing)
           == SK_ITERATOR_OK)
    {
        if (skVectorAppendValue(fileattr_vec, &attr)) {
            return -1;
        }
    }
    return 0;
}


void
sksiteRepoIteratorReset(
    sksite_repo_iter_t *iter)
{
    assert(iter);

    iter->time_idx = iter->time_start;
    iter->sensor_idx = 0;
    iter->first_call = 1;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
