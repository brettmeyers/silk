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
**  Prints information about SiLK site configurations
**
**  Michael Duggan
**  September 2011
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwsiteinfo.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skstream.h>
#include <silk/skstringmap.h>
#include <silk/sksite.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* max number of iterators is 3: flowtype, class, sensor */
#define RWS_MAX_ITERATOR_COUNT  3

/*
 *  Printer function typedef.
 *
 *    rwsiteinfo "prints" the output twice.  In the first pass, it
 *    uses a print function that does not produce output but instead
 *    determines the sizes of what it would print.  In the second
 *    pass, it uses an actual print function (fprintf) to produce the
 *    output.
 */
typedef int (*rws_fprintf_t)(FILE *f, const char *fmt, ...);


/* Sub-iterator types */
typedef enum {
    RWS_NULL,
    RWS_FLOWTYPE,
    RWS_CLASS,
    RWS_SENSOR,
    RWS_DEFAULT_CLASS,
    RWS_CLASS_FROM_SENSOR,
    RWS_SENSOR_FROM_CLASS,
    RWS_FLOWTYPE_FROM_CLASS,
    RWS_DEFAULT_FLOWTYPE_FROM_CLASS
} rws_iter_type_t;

/* Site iterator */
typedef struct rws_iter_st {
    /* The iterators */
    flowtype_iter_t flowtype_iter;
    class_iter_t    class_iter;
    sensor_iter_t   sensor_iter;

    /* The values */
    flowtypeID_t    flowtype_id;
    classID_t       class_id;
    sensorID_t      sensor_id;

    /* Order and type of iterators */
    rws_iter_type_t order[RWS_MAX_ITERATOR_COUNT];

    /* Number of iterators  */
    int             level;
    /* Highest bound iterator */
    int             bound;
    /* Highest started iterator */
    int             started;

    /* Emitted at a given level */
    int             emitted[RWS_MAX_ITERATOR_COUNT];
    /* Lowest level at which information is emitted */
    int             emit_level;

    /* Whether RWS_DEFAULT_FLOWTYPE_FROM_CLASS is one of the
     * iterators */
    unsigned        default_type : 1;
} rws_iter_t;


/* LOCAL VARIABLE DEFINITIONS */

/* Masks for flowtypes, classes, and sensors as set by the --classes,
 * --types, --flowtypes, and --sensors switches.  When the bitmap is
 * NULL, all values are printed; when the bitmap is not NULL, only
 * values where the bit is high are printed. */
static sk_bitmap_t *flowtype_mask = NULL;
static sk_bitmap_t *class_mask   = NULL;
static sk_bitmap_t *sensor_mask   = NULL;

/* paging program */
static const char *pager;

/* the output stream, set by --output-path or --pager */
static sk_fileptr_t output;

/* raw filter arguments */
static char *classes_arg   = NULL;
static char *types_arg     = NULL;
static char *flowtypes_arg = NULL;
static char *sensors_arg   = NULL;
static char *fields_arg    = NULL;

/* delimiters */
static char column_separator = '|';
static char list_separator   = ',';

/* how to format output */
static int no_columns         = 0;
static int no_final_delimiter = 0;
static int no_titles          = 0;

/* final delimiter */
static char final_delim[] = {'\0', '\0'};

/* Field types */
typedef enum {
    RWST_CLASS,
    RWST_TYPE,
    RWST_FLOWTYPE,
    RWST_FLOWTYPE_ID,
    RWST_SENSOR,
    RWST_SENSOR_ID,
    RWST_SENSOR_DESC,
    RWST_DEFAULT_CLASS,
    RWST_DEFAULT_TYPE,
    RWST_MARK_DEFAULTS,
    RWST_CLASS_LIST,
    RWST_TYPE_LIST,
    RWST_FLOWTYPE_LIST,
    RWST_FLOWTYPE_ID_LIST,
    RWST_SENSOR_LIST,
    RWST_SENSOR_ID_LIST,
    RWST_DEFAULT_CLASS_LIST,
    RWST_DEFAULT_TYPE_LIST,
    /* Number of field types */
    RWST_MAX_FIELD_COUNT
} rws_field_t;

/* Field names, types, descriptions, and titles.  MUST be in the same
 * order as the rws_field_t enumeration. */
static const sk_stringmap_entry_t field_map_entries[] = {
    {"class",              RWST_CLASS,
     "class name",                             "Class"},
    {"type",               RWST_TYPE,
     "type name",                              "Type"},
    {"flowtype",           RWST_FLOWTYPE,
     "flowtype name",                          "Flowtype"},
    {"id-flowtype",        RWST_FLOWTYPE_ID,
     "flowtype integer identifier",            "Flowtype-ID"},
    {"sensor",             RWST_SENSOR,
     "sensor name",                            "Sensor"},
    {"id-sensor",          RWST_SENSOR_ID,
     "sensor integer identifier",              "Sensor-ID"},
    {"describe-sensor",    RWST_SENSOR_DESC,
     "sensor description",                     "Sensor-Description"},
    {"default-class",      RWST_DEFAULT_CLASS,
     "default class name",                     "Default-Class"},
    {"default-type",       RWST_DEFAULT_TYPE,
     "default type name",                      "Default-Type"},
    {"mark-defaults",      RWST_MARK_DEFAULTS,
     "'+' for default classes, '*' for types", "Defaults"},
    {"class:list",         RWST_CLASS_LIST,
     "list of class names",                    "Class:list"},
    {"type:list",          RWST_TYPE_LIST,
     "list of type names",                     "Type:list"},
    {"flowtype:list",      RWST_FLOWTYPE_LIST,
     "list of flowtype names",                 "Flowtype:list"},
    {"id-flowtype:list",   RWST_FLOWTYPE_ID_LIST,
     "list of flowtype integer identifier",    "Flowtype-ID:list"},
    {"sensor:list",        RWST_SENSOR_LIST,
     "list of sensor names",                   "Sensor:list"},
    {"id-sensor:list",     RWST_SENSOR_ID_LIST,
     "list of sensor integer identifiers",     "Sensor-ID:list"},
    {"default-class:list", RWST_DEFAULT_CLASS_LIST,
     "list of default class names",            "Default-Class:list"},
    {"default-type:list",  RWST_DEFAULT_TYPE_LIST,
     "list of default type names",             "Default-Type:list"},
    SK_STRINGMAP_SENTINEL
};

/* Fields to print, in the order in which to print them */
static rws_field_t fields[RWST_MAX_FIELD_COUNT];

/* Number of fields in list */
static size_t num_fields = 0;

/* Width of the columns, where index is an rws_field_t. */
static int col_width[RWST_MAX_FIELD_COUNT];


/* OPTIONS SETUP */

typedef enum {
    OPT_FIELDS, OPT_CLASSES, OPT_TYPES, OPT_FLOWTYPES,
    OPT_SENSORS, OPT_NO_TITLES, OPT_NO_COLUMNS, OPT_COLUMN_SEPARATOR,
    OPT_NO_FINAL_DELIMITER, OPT_DELIMITED, OPT_LIST_DELIMETER,
    OPT_PAGER, OPT_DATA_ROOTDIR
} appOptionsEnum;

static struct option appOptions[] = {
    {"fields",              REQUIRED_ARG, 0, OPT_FIELDS},
    {"classes",             REQUIRED_ARG, 0, OPT_CLASSES},
    {"types",               REQUIRED_ARG, 0, OPT_TYPES},
    {"flowtypes",           REQUIRED_ARG, 0, OPT_FLOWTYPES},
    {"sensors",             REQUIRED_ARG, 0, OPT_SENSORS},
    {"no-titles",           NO_ARG,       0, OPT_NO_TITLES},
    {"no-columns",          NO_ARG,       0, OPT_NO_COLUMNS},
    {"column-separator",    REQUIRED_ARG, 0, OPT_COLUMN_SEPARATOR},
    {"no-final-delimiter",  NO_ARG,       0, OPT_NO_FINAL_DELIMITER},
    {"delimited",           OPTIONAL_ARG, 0, OPT_DELIMITED},
    {"list-delimiter",      REQUIRED_ARG, 0, OPT_LIST_DELIMETER},
    {"pager",               REQUIRED_ARG, 0, OPT_PAGER},
    {"data-rootdir",        REQUIRED_ARG, 0, OPT_DATA_ROOTDIR},
    {0,0,0,0}               /* sentinel entry */
};

static const char *appHelp[] = {
    ("Print the fields named in this comma-separated list. Choices:"),
    ("Restrict the output using classes named in this comma-\n"
     "\tseparated list. Use '@' to designate the default class.\n"
     "\tDef. Print data for all classes"),
    ("Restrict the output using the types named in this comma-\n"
     "\tseparated list. Use '@' to designate the default type(s) for a class.\n"
     "\tDef. Print data for all types"),
    ("Restrict the output using the class/type pairs named in\n"
     "\tthis comma-separated list. May use 'all' for class and/or type. This\n"
     "\tis an alternate way to specify class/type; switch may not be used\n"
     "\twith --class or --type. Def. Print data for all class/type pairs"),
    ("Restrict the output using the sensors named in this comma-\n"
     "\tseparated list. Sensors may be designated by name, ID (integer),\n"
     "\tand/or ranges of IDs. Def. Print data for all sensors"),
    ("Do not print column headers. Def. Print titles"),
    ("Disable fixed-width columnar output. Def. Columnar"),
    ("Use specified character between columns. Def. '|'"),
    ("Suppress column delimiter at end of line. Def. No"),
    ("Shortcut for --no-columns --no-final-del --column-sep=CHAR"),
    ("Use specified character between items in FIELD:list\n"
     "\tfields. Def. ','"),
    ("Program to invoke to page output. Def. $SILK_PAGER or $PAGER"),
    ("Root of directory tree containing packed data."),
    (char *)NULL
};



/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static sk_stringmap_t *createStringmap(void);
static int rws_parse_fields(void);
static int rws_parse_restrictions(void);
static int
rws_print_list_field(
    rws_fprintf_t       printer,
    FILE               *fd,
    rws_iter_t         *iter,
    rws_field_t         field,
    int                 width);
static int fprintf_size(FILE *stream, const char *format, ...);


/* FUNCTION DEFINITIONS */

/*
 *  appUsageLong();
 *
 *    Print complete usage information to USAGE_FH.  Pass this
 *    function to skOptionsSetUsageCallback(); skOptionsParse() will
 *    call this funciton and then exit the program when the --help
 *    option is given.
 */
static void
appUsageLong(
    void)
{
#define MIN_TEXT_ON_LINE  15
#define MAX_TEXT_ON_LINE  72
#define USAGE_MSG                                                            \
    ("--fields=<FIELDS> [SWITCHES]\n"                                        \
     "\tPrint selected information about the classes, types, flowtypes\n"    \
     "\tand sensors defined in the SiLK site configuration file.  By\n"      \
     "\tdefault, the selected information is printed for every class,\n"     \
     "\ttype, and sensor defined in the file; to restrict the output,\n"     \
     "\tspecify one or more of --classes, --types, --flowtypes, or\n"        \
     "\t--sensors.\n")

    FILE *fh = USAGE_FH;
    char *cp, *ep, *sp;
    char buf[2 * PATH_MAX];
    char path[PATH_MAX];
    int i;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);

    for (i = 0; appOptions[i].name; i++ ) {
        fprintf(fh, "--%s %s. ",
                appOptions[i].name, SK_OPTION_HAS_ARG(appOptions[i]));
        /* Print the static help text from the appHelp array */
        fprintf(fh, "%s\n", appHelp[i]);
        switch (appOptions[i].val) {
          case OPT_FIELDS:
            {
                sk_stringmap_t *map = createStringmap();
                if (map == NULL) {
                    skAppPrintErr("Error creating string map");
                    exit(EXIT_FAILURE);
                }
                skStringMapPrintUsage(map, fh, 8);
                skStringMapDestroy(map);
            }
            break;
          case OPT_DATA_ROOTDIR:
            /* put the text into a buffer, and then wrap the text in
             * the buffer at space characters. */
            snprintf(buf, sizeof(buf),
                     ("Currently '%s'. Def. $" SILK_DATA_ROOTDIR_ENVAR
                      " or '%s'"),
                     sksiteGetRootDir(path, sizeof(path)),
                     sksiteGetDefaultRootDir());
            sp = buf;
            while (strlen(sp) > MAX_TEXT_ON_LINE) {
                cp = &sp[MIN_TEXT_ON_LINE];
                while ((ep = strchr(cp+1, ' ')) != NULL) {
                    /* text is now too long */
                    if (ep - sp > MAX_TEXT_ON_LINE) {
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
            break;
          default:
            break;
        }
    }

    sksiteOptionsUsage(fh);
}


/*
 *  appTeardown()
 *
 *    Teardown all modules, close all files, and tidy up all
 *    application state.
 *
 *    This function is idempotent.
 */
static void
appTeardown(
    void)
{
    static int teardownFlag = 0;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* close the output file or process */
    if (output.of_name) {
        skFileptrClose(&output, &skAppPrintErr);
    }

    skBitmapDestroy(&flowtype_mask);
    skBitmapDestroy(&class_mask);
    skBitmapDestroy(&sensor_mask);

    skAppUnregister();
}


/*
 *  appSetup(argc, argv);
 *
 *    Perform all the setup for this application include setting up
 *    required modules, parsing options, etc.  This function should be
 *    passed the same arguments that were passed into main().
 *
 *    Returns to the caller if all setup succeeds.  If anything fails,
 *    this function will cause the application to exit with a FAILURE
 *    exit status.
 */
static void
appSetup(
    int                 argc,
    char              **argv)
{
    SILK_FEATURES_DEFINE_STRUCT(features);
    int arg_index;
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    /* initialize globals */
    memset(&output, 0, sizeof(output));
    output.of_fp = stdout;

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || sksiteOptionsRegister(SK_SITE_FLAG_CONFIG_FILE))
    {
        skAppPrintErr("Unable to register options");
        exit(EXIT_FAILURE);
    }

    /* register the teardown handler */
    if (atexit(appTeardown) < 0) {
        skAppPrintErr("Unable to register appTeardown() with atexit()");
        appTeardown();
        exit(EXIT_FAILURE);
    }

    /* parse the options */
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        skAppUsage();           /* never returns */
    }

    /* try to load site config file; if it fails, we will not be able
     * to resolve flowtype and sensor from input file names */
    if (sksiteConfigure(1)) {
        exit(EXIT_FAILURE);
    }

    /* parse fields */
    if (rws_parse_fields()) {
        exit(EXIT_FAILURE);
    }

    /* parse restrictions (--classes, --types, etc) */
    if (rws_parse_restrictions()) {
        exit(EXIT_FAILURE);
    }

    /* check for extraneous arguments */
    if (arg_index != argc) {
        skAppPrintErr("Too many arguments or unrecognized switch '%s'",
                      argv[arg_index]);
        skAppUsage();           /* never returns */
    }

    /* Initialize column widths */
    if (no_titles || no_columns) {
        memset(col_width, 0, sizeof(col_width));
    } else {
        size_t i;
        for (i = 0; i < RWST_MAX_FIELD_COUNT; i++) {
            col_width[i] = strlen((char *)field_map_entries[i].userdata);

            /* While looping through, verify that the
             * field_map_entries is in the same order as the
             * enumeration. */
            assert(i == field_map_entries[i].id);
        }
    }

    /* Set the final delimiter, if used */
    if (!no_final_delimiter) {
        final_delim[0] = column_separator;
    }

    rv = skFileptrOpenPager(&output, pager);
    if (rv && rv != SK_FILEPTR_PAGER_IGNORED) {
        skAppPrintErr("Unable to invoke pager");
    }

    return;  /* OK */
}


/*
 *  status = appOptionsHandler(cData, opt_index, opt_arg);
 *
 *    This function is passed to skOptionsRegister(); it will be called
 *    by skOptionsParse() for each user-specified switch that the
 *    application has registered; it should handle the switch as
 *    required---typically by setting global variables---and return 1
 *    if the switch processing failed or 0 if it succeeded.  Returning
 *    a non-zero from from the handler causes skOptionsParse() to return
 *    a negative value.
 *
 *    The clientData in 'cData' is typically ignored; 'opt_index' is
 *    the index number that was specified as the last value for each
 *    struct option in appOptions[]; 'opt_arg' is the user's argument
 *    to the switch for options that have a REQUIRED_ARG or an
 *    OPTIONAL_ARG.
 */
static int
appOptionsHandler(
    clientData   UNUSED(cData),
    int                 opt_index,
    char               *opt_arg)
{
    switch ((appOptionsEnum)opt_index) {
      case OPT_DATA_ROOTDIR:
        if (!skDirExists(opt_arg)) {
            skAppPrintErr("Root data directory '%s' does not exist", opt_arg);
            return -1;
        }
        if (sksiteSetRootDir(opt_arg)) {
            skAppPrintErr("Unable to set root data directory to %s", opt_arg);
            return -1;
        }
        break;

      case OPT_CLASSES:
        if (classes_arg != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return -1;
        }
        classes_arg = opt_arg;
        break;

      case OPT_TYPES:
        if (types_arg != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return -1;
        }
        types_arg = opt_arg;
        break;

      case OPT_FLOWTYPES:
        if (flowtypes_arg != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return -1;
        }
        flowtypes_arg = opt_arg;
        break;

      case OPT_SENSORS:
        if (sensors_arg != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return -1;
        }
        sensors_arg = opt_arg;
        break;

      case OPT_FIELDS:
        if (fields_arg != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return -1;
        }
        fields_arg = opt_arg;
        break;

      case OPT_NO_TITLES:
        no_titles = 1;
        break;

      case OPT_NO_COLUMNS:
        no_columns = 1;
        break;

      case OPT_COLUMN_SEPARATOR:
        column_separator = opt_arg[0];
        break;

      case OPT_NO_FINAL_DELIMITER:
        no_final_delimiter = 1;
        break;

      case OPT_DELIMITED:
        no_columns = 1;
        no_final_delimiter = 1;
        if (opt_arg) {
            column_separator = opt_arg[0];
        }
        break;

      case OPT_LIST_DELIMETER:
        list_separator = opt_arg[0];
        break;

      case OPT_PAGER:
        pager = opt_arg;
        break;
    }

    return 0;  /* OK */
}


/*
 *  stringmap = createStringmap();
 *
 *    Create the string map that is used to parse the --fields
 *    paramater.
 */
static sk_stringmap_t *
createStringmap(
    void)
{
    sk_stringmap_t *field_map;

    /* Create the map */
    if (SKSTRINGMAP_OK != skStringMapCreate(&field_map)) {
        return NULL;
    }

    /* add entries */
    if (skStringMapAddEntries(field_map, -1, field_map_entries)
        != SKSTRINGMAP_OK)
    {
        skStringMapDestroy(field_map);
        return NULL;
    }

    return field_map;
}


/*
 *  status = rws_parse_fields();
 *
 *     Parse the --fields argument.  Return 0 on success, -1 on
 *     failure.
 */
static int
rws_parse_fields(
    void)
{
    sk_stringmap_t *field_map = NULL;
    sk_stringmap_iter_t *iter = NULL;
    sk_stringmap_entry_t *entry;
    char *errmsg;
    int rv = -1;

    if (fields_arg == NULL) {
        skAppPrintErr("The --%s switch is required",
                      appOptions[OPT_FIELDS].name);
        return rv;
    }
    field_map = createStringmap();
    if (NULL == field_map) {
        skAppPrintOutOfMemory(NULL);
        goto END;
    }

    /* parse the field-list */
    if (skStringMapParse(field_map, fields_arg, SKSTRINGMAP_DUPES_ERROR,
                         &iter, &errmsg))
    {
        skAppPrintErr("Invalid %s: %s",
                      appOptions[OPT_FIELDS].name, errmsg);
        goto END;
    }

    /* add the selected fields to the 'fields[]' array */
    while (skStringMapIterNext(iter, &entry, NULL) == SK_ITERATOR_OK) {
        assert(num_fields < RWST_MAX_FIELD_COUNT);
        fields[num_fields] = (rws_field_t)entry->id;
         ++num_fields;
    }

    rv = 0;

  END:
    if (iter != NULL) {
        skStringMapIterDestroy(iter);
    }
    if (field_map != NULL) {
        skStringMapDestroy(field_map);
    }
    return rv;
}


/*
 *  status = rws_parse_sensors(sn_bitmap);
 *
 *      Parse the --sensors argument from the global 'sensors_arg'.
 *      Set a bit in 'sn_bitmap' for each sensor we see.  Return 0 on
 *      success, or -1 if any invalid sensors are found.
 */
static int
rws_parse_sensors(
    sk_bitmap_t        *sn_bitmap)
{
    char *user_arg = NULL;
    char *user_arg_freeable = NULL;
    char *sensor_token = NULL;
    uint32_t min_sensor_id;
    uint32_t max_sensor_id;
    uint32_t val_min, val_max;
    sensorID_t sid = SK_INVALID_SENSOR;
    int p_err;
    int rv = 0;

    /* nothing to do when no --sensors switch is given */
    if (NULL == sensors_arg) {
        return 0;
    }

    assert(sn_bitmap);
    assert(skBitmapGetSize(sn_bitmap) > sksiteSensorGetMaxID());

    min_sensor_id = sksiteSensorGetMinID();
    max_sensor_id = sksiteSensorGetMaxID();

    /* create a copy of the input string and maintain a reference to
     * it so we can free it */
    user_arg = strdup(sensors_arg);
    user_arg_freeable = user_arg;
    if (user_arg == NULL) {
        skAppPrintOutOfMemory(NULL);
        rv = -1;
        goto END;
    }

    /* parse the sensors as a comma separated list of tokens */
    while ((sensor_token = strsep(&user_arg, ",")) != NULL) {
        /* check for empty token (e.g., double comma) */
        if ('\0' == *sensor_token) {
            continue;
        }

        /* look up sensor_token as a sensor name */
        sid = sksiteSensorLookup(sensor_token);
        if (SK_INVALID_SENSOR != sid) {
            /* found it. add it to our lists and continue */
            skBitmapSetBit(sn_bitmap, sid);
            continue;
        }

        /* parsing failed. does the token look like a number? */
        if (!isdigit((int)(*sensor_token))) {
            /* not a number: error */
            skAppPrintErr("Invalid %s '%s': Unknown sensor name",
                          appOptions[OPT_SENSORS].name, sensor_token);
            rv = -1;
            continue;
        }

        /* it is a digit: parse the token as a single number or a
         * range */
        p_err = skStringParseRange32(&val_min, &val_max, sensor_token,
                                     min_sensor_id, max_sensor_id,
                                     SKUTILS_RANGE_NO_OPEN);
        if (p_err != 0) {
            /* an error */
            skAppPrintErr("Invalid %s '%s': %s",
                          appOptions[OPT_SENSORS].name, sensor_token,
                          skStringParseStrerror(p_err));
            rv = -1;
            continue;
        }

        /* verify that start of range is valid */
        sid = (sensorID_t)val_min;
        if (!sksiteSensorExists(sid)) {
            skAppPrintErr(("Invalid %s: Value %" PRIu32
                           " is not a valid sensor id"),
                          appOptions[OPT_SENSORS].name, val_min);
            rv = -1;
            continue;
        }

        /* verify that end of range is valid */
        if (val_min != val_max && !sksiteSensorExists((sensorID_t)val_max)) {
            skAppPrintErr(("Invalid %s: Value %" PRIu32
                           " is not a valid sensor id"),
                          appOptions[OPT_SENSORS].name, val_max);
            rv = -1;
            continue;
        }

        /* add all sensor IDs in range */
        skBitmapRangeSet(sn_bitmap, sid, val_max);
    }

    /* did we find any sensors? */
    if ((0 == rv) && (0 == skBitmapGetHighCount(sn_bitmap))) {
        skAppPrintErr("Invalid %s: No valid sensors",
                      appOptions[OPT_SENSORS].name);
        rv = -1;
    }

  END:
    if (user_arg_freeable) {
        free(user_arg_freeable);
    }
    return rv;
}


/*
 *  status = rws_parse_flowtypes(cl_bitmap, ft_bitmap);
 *
 *    Parse the --flowtypes argument from the global 'flowtypes_arg'.
 *    Set a bit on 'cl_bitmap' for each valid class and and a bit on
 *    'ft_bitmap' for each valid flowtype.  Return 0 on success, or -1
 *    on if any class/type value is not a valid pair.
 */
static int
rws_parse_flowtypes(
    sk_bitmap_t        *cl_bitmap,
    sk_bitmap_t        *ft_bitmap)
{
    char *user_arg = NULL;
    char *user_arg_freeable = NULL;
    char *class_name;
    char *type_name;
    flowtype_iter_t ft_iter;
    flowtypeID_t ft;
    classID_t class_id;
    int rv = 0;

    /* nothing to do when no --flowtypes switch is given */
    if (NULL == flowtypes_arg) {
        return 0;
    }

    /* create a modifiable version of the user's input. store a
     * pointer to it we can free it */
    user_arg = strdup(flowtypes_arg);
    user_arg_freeable = user_arg;
    if (NULL == user_arg) {
        skAppPrintOutOfMemory(NULL);
        rv = -1;
        goto END;
    }

    /* parse user's string as a comma separated list */
    while ((class_name = strsep(&user_arg, ",")) != NULL) {
        if (class_name[0] == '\0') {
            /* empty token (e.g., double comma) */
            continue;
        }

        /* break token into class and type separated by '/' */
        type_name = strchr(class_name, '/');
        if (type_name == NULL) {
            skAppPrintErr("Invalid %s: Missing '/' in token '%s'",
                          appOptions[OPT_FLOWTYPES].name, class_name);
            rv = -1;
            continue;
        }
        *type_name = '\0';
        ++type_name;

        /* find class and type.  if lookup fails, test for special
         * "all" keyword */
        ft = sksiteFlowtypeLookupByClassType(class_name, type_name);
        if (SK_INVALID_FLOWTYPE != ft) {
            /* Yay!  Class and type are specific */
            skBitmapSetBit(ft_bitmap, ft);
            skBitmapSetBit(cl_bitmap, sksiteFlowtypeGetClassID(ft));

        } else if (0 == strcmp(class_name, "all")) {
            if (0 == strcmp(type_name, "all")) {
                /* Use all classes and all types. */
                sksiteFlowtypeIterator(&ft_iter);
                while (sksiteFlowtypeIteratorNext(&ft_iter, &ft)) {
                    skBitmapSetBit(ft_bitmap, ft);
                    skBitmapSetBit(cl_bitmap, sksiteFlowtypeGetClassID(ft));
                }
            } else {
                /* Loop over all classes and add flowtype if type_name
                 * is valid for that class.  Don't complain unless the
                 * type is not valid for any class. */
                class_iter_t ci;
                int found_type = 0;

                sksiteClassIterator(&ci);
                while (sksiteClassIteratorNext(&ci, &class_id)) {
                    ft = sksiteFlowtypeLookupByClassIDType(class_id,type_name);
                    if (SK_INVALID_FLOWTYPE != ft) {
                        ++found_type;
                        skBitmapSetBit(ft_bitmap, ft);
                        skBitmapSetBit(cl_bitmap, class_id);
                    }
                }
                if (!found_type) {
                    skAppPrintErr(("Invalid %s:"
                                   " Type '%s' not valid for any class"),
                                  appOptions[OPT_FLOWTYPES].name, type_name);
                    rv = -1;
                }
            }

        } else if (0 == strcmp(type_name, "all")) {
            /* Use all types in the specified class */
            class_id = sksiteClassLookup(class_name);
            if (SK_INVALID_CLASS == class_id) {
                skAppPrintErr("Invalid %s: Invalid class '%s'",
                              appOptions[OPT_FLOWTYPES].name, class_name);
                rv = -1;
            } else {
                skBitmapSetBit(cl_bitmap, class_id);
                sksiteClassFlowtypeIterator(class_id, &ft_iter);
                while (sksiteFlowtypeIteratorNext(&ft_iter, &ft)) {
                    skBitmapSetBit(ft_bitmap, ft);
                }
            }

        } else {
            /* Invalid class/type */
            skAppPrintErr("Invalid %s: Unknown class/type pair '%s/%s'",
                          appOptions[OPT_FLOWTYPES].name,
                          class_name, type_name);
            rv = -1;
        }
    }

    if ((rv == 0) && (skBitmapGetHighCount(ft_bitmap) == 0)) {
        skAppPrintErr("Invalid %s: No valid class/type pairs",
                      appOptions[OPT_FLOWTYPES].name);
        rv = -1;
    }

  END:
    if (user_arg_freeable) {
        free(user_arg_freeable);
    }
    return rv;
}


/*
 *  status = rws_parse_classes_and_types();
 *
 *    Parse the --classes and/or --types arguments from the globals
 *    'classes_arg' and 'types_arg'.  Set a bit on 'cl_bitmap' for
 *    each valid class and and a bit on 'ft_bitmap' for each valid
 *    flowtype.  Return 0 on success, or -1 on if any class/type value
 *    is not a valid pair.
 */
static int
rws_parse_classes_and_types(
    sk_bitmap_t        *cl_bitmap,
    sk_bitmap_t        *ft_bitmap)
{
    sk_bitmap_iter_t bmap_iter;
    uint32_t bmap_val;
    class_iter_t ci;
    flowtype_iter_t ft_iter;
    char *user_arg = NULL;
    char *user_arg_freeable = NULL;
    char *class_token = NULL;
    char *type_token = NULL;
    classID_t class_id;
    flowtypeID_t ft;
    int found_type;
    int rv = 0;

    if (NULL == classes_arg) {
        /* temporarily enable all classes */
        sksiteClassIterator(&ci);
        while (sksiteClassIteratorNext(&ci, &class_id)) {
            skBitmapSetBit(cl_bitmap, class_id);
        }

    } else {
        /* create a copy of the input string and maintain a reference to
         * it so we can free it */
        user_arg = strdup(classes_arg);
        user_arg_freeable = user_arg;
        if (user_arg == NULL) {
            skAppPrintOutOfMemory(NULL);
            rv = -1;
            goto END;
        }

        /* parse the classes as a comma separated list of tokens */
        while ((class_token = strsep(&user_arg, ",")) != NULL) {
            /* check for empty token (e.g., double comma) */
            if ('\0' == *class_token) {
                continue;
            }

            /* Handle default class token */
            if (strcmp(class_token, "@") == 0) {
                class_id = sksiteClassGetDefault();
                if (SK_INVALID_CLASS != class_id) {
                    skBitmapSetBit(cl_bitmap, class_id);
                }
                continue;
            }

            /* look up class_token as a class name */
            class_id = sksiteClassLookup(class_token);
            if (SK_INVALID_CLASS != class_id) {
                /* found it. add it to our lists and continue */
                skBitmapSetBit(cl_bitmap, class_id);
                continue;
            }

            skAppPrintErr("Invalid %s '%s': Unknown class name",
                          appOptions[OPT_CLASSES].name, class_token);
            rv = -1;
            continue;
        }

        free(user_arg_freeable);
        user_arg_freeable = NULL;

        /* did we find any classes? */
        if ((0 == rv) && (0 == skBitmapGetHighCount(cl_bitmap))) {
            skAppPrintErr("Invalid %s: No valid classes",
                          appOptions[OPT_CLASSES].name);
            rv = -1;
        }

        if (NULL == types_arg) {
            if (rv != 0) {
                goto END;
            }
            /* there is no --types; enable all flowtypes that exist in
             * the specified class(es) */
            skBitmapIteratorBind(cl_bitmap, &bmap_iter);
            while (skBitmapIteratorNext(&bmap_iter, &bmap_val)==SK_ITERATOR_OK)
            {
                class_id = (classID_t)bmap_val;
                sksiteClassFlowtypeIterator(class_id, &ft_iter);
                while (sksiteFlowtypeIteratorNext(&ft_iter, &ft)) {
                    skBitmapSetBit(ft_bitmap, ft);
                }
            }
            goto END;
        }
    }

    /* create a modifiable version of the user's input. store a
     * pointer to it we can free */
    user_arg = strdup(types_arg);
    user_arg_freeable = user_arg;
    if (NULL == user_arg) {
        skAppPrintOutOfMemory(NULL);
        rv = -1;
        goto END;
    }

    /* parse user's string as a comma separated list */
    while ((type_token = strsep(&user_arg, ",")) != NULL) {
        found_type = 0;
        if (type_token[0] == '\0') {
            /* empty token (e.g., double comma) */
            continue;
        }

        /* check for the type in all the classes we found above */
        skBitmapIteratorBind(cl_bitmap, &bmap_iter);
        while (skBitmapIteratorNext(&bmap_iter, &bmap_val)==SK_ITERATOR_OK) {
            class_id = (classID_t)bmap_val;

            /* find class and type */
            ft = sksiteFlowtypeLookupByClassIDType(class_id, type_token);
            if (SK_INVALID_FLOWTYPE != ft) {
                /* Yay!  Class and type are specific */
                skBitmapSetBit(ft_bitmap, ft);
                ++found_type;

            } else if (0 == strcmp(type_token, "all")) {
                /* Use all types in the specified class */
                sksiteClassFlowtypeIterator(class_id, &ft_iter);
                while (sksiteFlowtypeIteratorNext(&ft_iter, &ft)) {
                    ++found_type;
                    skBitmapSetBit(ft_bitmap, ft);
                }

            } else if (0 == strcmp(type_token, "@")) {
                sksiteClassDefaultFlowtypeIterator(class_id, &ft_iter);
                while (sksiteFlowtypeIteratorNext(&ft_iter, &ft)) {
                    ++found_type;
                    skBitmapSetBit(ft_bitmap, ft);
                }
            }
        }

        if (!found_type) {
            skAppPrintErr("Invalid %s: Type '%s' not valid for %s",
                          appOptions[OPT_TYPES].name, type_token,
                          (classes_arg ? "specified classes" : "any class"));
            rv = -1;
        }
    }

    if ((rv == 0) && (skBitmapGetHighCount(ft_bitmap) == 0)) {
        skAppPrintErr("Invalid --%s: No valid types",
                      appOptions[OPT_TYPES].name);
        rv = -1;
    }

    if ((rv == 0) && (NULL == classes_arg)) {
        /* no --classes were specified.  reset the cl_bitmap based on
         * the flowtypes we found */
        skBitmapClearAllBits(cl_bitmap);

        skBitmapIteratorBind(ft_bitmap, &bmap_iter);
        while (skBitmapIteratorNext(&bmap_iter, &bmap_val)==SK_ITERATOR_OK) {
            ft = (flowtypeID_t)bmap_val;
            skBitmapSetBit(cl_bitmap, sksiteFlowtypeGetClassID(ft));
        }
    }

  END:
    if (user_arg_freeable) {
        free(user_arg_freeable);
    }
    return rv;
}


/*
 *  status = rws_parse_restrictions();
 *
 *    Parse the --classes, --types, --flowtypes, and --sensors options
 *    and create/fill bitmaps that restrict the output.
 *
 *    Returns 0 on success, -1 on error.
 */
static int
rws_parse_restrictions(
    void)
{
    sk_bitmap_iter_t bmap_iter;
    uint32_t bmap_val;
    sk_bitmap_t *cl_mask = NULL;
    sk_bitmap_t *ft_mask = NULL;
    sk_bitmap_t *sn_mask = NULL;
    class_iter_t class_iter;
    flowtype_iter_t ft_iter;
    sensor_iter_t sensor_iter;
    classID_t class_id;
    sensorID_t sensor_id;
    flowtypeID_t flowtype_id;
    int sensors_only = 0;
    int rv = 0;

    if (!classes_arg && !types_arg && !flowtypes_arg) {
        if (!sensors_arg) {
            /* nothing to do */
            return 0;
        }
        /* else, only need to process --sensors */
        sensors_only = 1;
    }

    /* create the global bitmaps for all classes, all flowtypes, and
     * all sensors */
    if (skBitmapCreate(&class_mask, 1 + sksiteClassGetMaxID())) {
        skAppPrintOutOfMemory("class bitmap");
        return -1;
    }
    if (skBitmapCreate(&flowtype_mask, 1 + sksiteFlowtypeGetMaxID())) {
        skAppPrintOutOfMemory("flowtype bitmap");
        return -1;
    }
    if (skBitmapCreate(&sensor_mask, 1 + sksiteSensorGetMaxID())) {
        skAppPrintOutOfMemory("sensor bitmap");
        return -1;
    }

    if (!sensors_only && sensors_arg) {
        /* need to create temporary bitmaps */
        if (skBitmapCreate(&cl_mask, 1 + sksiteClassGetMaxID())) {
            skAppPrintOutOfMemory("class bitmap");
            rv = -1;
            goto END;
        }
        if (skBitmapCreate(&ft_mask, 1 + sksiteFlowtypeGetMaxID())) {
            skAppPrintOutOfMemory("flowtype bitmap");
            rv = -1;
            goto END;
        }
        if (skBitmapCreate(&sn_mask, 1 + sksiteSensorGetMaxID())) {
            skAppPrintOutOfMemory("sensor bitmap");
            rv = -1;
            goto END;
        }
    } else {
        /* else we can point the temporaries at the real thing */
        cl_mask = class_mask;
        ft_mask = flowtype_mask;
        sn_mask = sensor_mask;
    }

    if (sensors_arg) {
        rv = rws_parse_sensors(sensor_mask);

        if (sensors_only && rv != 0) {
            goto END;
        }

        /* set class_mask and flowtype_mask based on the sensors we saw */
        skBitmapIteratorBind(sensor_mask, &bmap_iter);
        while (skBitmapIteratorNext(&bmap_iter, &bmap_val)==SK_ITERATOR_OK) {
            sensor_id = (sensorID_t)bmap_val;
            sksiteSensorClassIterator(sensor_id, &class_iter);
            while (sksiteClassIteratorNext(&class_iter, &class_id)) {
                skBitmapSetBit(cl_mask, class_id);
                sksiteClassFlowtypeIterator(class_id, &ft_iter);
                while (sksiteFlowtypeIteratorNext(&ft_iter, &flowtype_id)) {
                    skBitmapSetBit(ft_mask, flowtype_id);
                }
            }
        }

        if (sensors_only) {
            return 0;
        }
    }

    /* handle case when --flowtypes is given */
    if (flowtypes_arg) {
        if (classes_arg || types_arg) {
            skAppPrintErr(("Cannot use --%s when either --%s or --%s is"
                           " specified"),
                          appOptions[OPT_FLOWTYPES].name,
                          appOptions[OPT_CLASSES].name,
                          appOptions[OPT_TYPES].name);
            goto END;
        }
        rv |= rws_parse_flowtypes(class_mask, flowtype_mask);
        if (rv != 0) {
            goto END;
        }
    } else {
        assert(classes_arg || types_arg);
        rv |= rws_parse_classes_and_types(class_mask, flowtype_mask);
        if (rv != 0) {
            goto END;
        }
    }

    /* set sensor_mask based on the classes we saw */
    skBitmapIteratorBind(class_mask, &bmap_iter);
    while (skBitmapIteratorNext(&bmap_iter, &bmap_val)==SK_ITERATOR_OK) {
        class_id = (classID_t)bmap_val;
        sksiteClassSensorIterator(class_id, &sensor_iter);
        while (sksiteSensorIteratorNext(&sensor_iter, &sensor_id)) {
            skBitmapSetBit(sn_mask, sensor_id);
        }
    }

    /* perform the intersection of the masks with the temporaries */
    if (sn_mask && sn_mask != sensor_mask) {
        skBitmapIntersection(sensor_mask, sn_mask);
        skBitmapDestroy(&sn_mask);
    }
    if (cl_mask && cl_mask != class_mask) {
        skBitmapIntersection(class_mask, cl_mask);
        skBitmapDestroy(&cl_mask);
    }
    if (ft_mask && ft_mask != flowtype_mask) {
        skBitmapIntersection(flowtype_mask, ft_mask);
        skBitmapDestroy(&ft_mask);
    }

  END:
    if (sn_mask && sn_mask != sensor_mask) {
        skBitmapDestroy(&sn_mask);
    }
    if (cl_mask && cl_mask != class_mask) {
        skBitmapDestroy(&cl_mask);
    }
    if (ft_mask && ft_mask != flowtype_mask) {
        skBitmapDestroy(&ft_mask);
    }
    return rv;
}


/*
 *  rws_iter_bind(&iter, level);
 *
 *    Bind the sub-iterator of a site iterator at the given level.
 */
static void
rws_iter_bind(
    rws_iter_t         *iter,
    int                 level)
{
    assert(iter);
    assert(iter->level >= level);

    /* Negative level is for a non-iterable iterator.  This is used
     * for options that require no level iteration, like
     * --fields=class:list. */
    if (level >= 0) {
        switch (iter->order[level]) {
          case RWS_FLOWTYPE:
            sksiteFlowtypeIterator(&iter->flowtype_iter);
            iter->flowtype_id = SK_INVALID_FLOWTYPE;
            break;
          case RWS_CLASS:
            sksiteClassIterator(&iter->class_iter);
            iter->class_id = SK_INVALID_CLASS;
            break;
          case RWS_DEFAULT_CLASS:
            assert(level == 0);
            iter->class_id = SK_INVALID_CLASS;
            break;
          case RWS_SENSOR:
            sksiteSensorIterator(&iter->sensor_iter);
            iter->sensor_id = SK_INVALID_SENSOR;
            break;
          case RWS_FLOWTYPE_FROM_CLASS:
            sksiteClassFlowtypeIterator(iter->class_id, &iter->flowtype_iter);
            iter->flowtype_id = SK_INVALID_FLOWTYPE;
            break;
          case RWS_CLASS_FROM_SENSOR:
            sksiteSensorClassIterator(iter->sensor_id, &iter->class_iter);
            iter->class_id = SK_INVALID_CLASS;
            break;
          case RWS_SENSOR_FROM_CLASS:
            sksiteClassSensorIterator(iter->class_id, &iter->sensor_iter);
            iter->sensor_id = SK_INVALID_SENSOR;
            break;
          case RWS_DEFAULT_FLOWTYPE_FROM_CLASS:
            sksiteClassDefaultFlowtypeIterator(
                iter->class_id, &iter->flowtype_iter);
            iter->flowtype_id = SK_INVALID_FLOWTYPE;
            iter->default_type = 1;
            break;
          case RWS_NULL:
            skAbortBadCase(iter->order[level]);
            break;
        }
    }

    /* We are now bound at this level */
    iter->bound = level;

    /* But mark as not having stared iteration yet */
    iter->started = level - 1;

    /* Mark that we haven't emitted any data at this level yet */
    iter->emitted[level] = 0;
}


/*
 *  status = rws_iter_next(iter, level);
 *
 *    Site iterator iteration, at a particular sub-iterator level.
 *    Returns 1 on success, 0 if the iteration is complete at the
 *    given level.
 */
static int
rws_iter_next(
    rws_iter_t         *iter,
    const int           level)
{
    int rv;

    assert(iter);
    assert(iter->level >= level);

    /* Prevent re-iteration after already having completed
     * iteration */
    if (iter->bound < level) {
        assert(level == 0);
        return 0;
    }
    if (level >= RWS_MAX_ITERATOR_COUNT) {
        skAbort();
    }

    do {
        /* Iterate at the next level if we've already started
         * iteration on this level */
        if (iter->started >= level && level < iter->level) {
            if (iter->bound == level) {
                rws_iter_bind(iter, level + 1);
            }
            rv = rws_iter_next(iter, level + 1);
            if (rv != 0) {
                /* Leaf iter succeeded */
                return 1;
            }
        }

        /* Iterate at the current level */
        switch (iter->order[level]) {
          case RWS_DEFAULT_CLASS:
            /* Pseudo-iterator for default class: toggle between the
             * default-class and SK_INVALID_CLASS */
            if (iter->class_id == SK_INVALID_CLASS) {
                iter->class_id = sksiteClassGetDefault();
            } else {
                iter->class_id = SK_INVALID_CLASS;
            }
            rv = (iter->class_id != SK_INVALID_CLASS);
            break;

          case RWS_FLOWTYPE:
          case RWS_FLOWTYPE_FROM_CLASS:
          case RWS_DEFAULT_FLOWTYPE_FROM_CLASS:
            /* Flowtype iteration */
            while ((rv = sksiteFlowtypeIteratorNext(&iter->flowtype_iter,
                                                    &iter->flowtype_id))
                   && flowtype_mask != NULL
                   && !skBitmapGetBit(flowtype_mask, iter->flowtype_id))
                ; /* empty */
            /* Set class from flowtype */
            if (rv) {
                iter->class_id = sksiteFlowtypeGetClassID(iter->flowtype_id);
            }
            break;

          case RWS_CLASS:
          case RWS_CLASS_FROM_SENSOR:
            /* Class iteration */
            while ((rv = sksiteClassIteratorNext(&iter->class_iter,
                                                 &iter->class_id))
                   && class_mask != NULL
                   && !skBitmapGetBit(class_mask, iter->class_id))
                ; /* empty */
            break;

          case RWS_SENSOR:
          case RWS_SENSOR_FROM_CLASS:
            /* Sensor iteration */
            while ((rv = sksiteSensorIteratorNext(&iter->sensor_iter,
                                                  &iter->sensor_id))
                   && sensor_mask != NULL
                   && !skBitmapGetBit(sensor_mask, iter->sensor_id))
                ; /* empty */
            break;

          default:
            skAbortBadCase(iter->order[level]);
        }

        /* Mark that we've started iterating at the current level */
        if (iter->started < level) {
            iter->started = level;
        }

        /* If iteration failed, but we haven't emitted anything at
         * this level yet, pretend it succeeded this once. */
        if (rv && level >= iter->emit_level) {
            iter->emitted[level] = 1;
        } else if (!rv && level != 0
                   && !iter->emitted[level]
                   && iter->emitted[level - 1])
        {
            iter->emitted[level] = 1;
            rv = 1;
        }

        if (rv && level == iter->level) {
            /* return success at leaf */
            return 1;
        }

        /* Iterate until a leaf iteration succeeds, or we fail to
         * iterate at this level. */
    } while (rv);

    /* Iteration is over.  We are no longer bound at this level.  */
    iter->bound = level - 1;

    return 0;
}


/*
 *  len = rws_print_field(printer, fd, iter, field, width);
 *
 *    Print a 'width'-wide column containing the value for 'field'
 *    using the fprintf-style function 'printer' to the FILE pointer
 *    'fd', where 'iter' is the current context.
 *
 *    Return the number of characters printed.
 */
static int
rws_print_field(
    rws_fprintf_t       printer,
    FILE               *fd,
    rws_iter_t         *iter,
    rws_field_t         field,
    int                 width)
{
    /* buffer large enough to hold a single sensor name or flowtype
     * name */
    char buf[SK_MAX_STRLEN_SENSOR + SK_MAX_STRLEN_FLOWTYPE];
    const char *s;
    int rv = 0;

    switch (field) {
      case RWST_CLASS:
        if (iter->class_id != SK_INVALID_CLASS) {
            sksiteClassGetName(buf, sizeof(buf), iter->class_id);
            rv = printer(fd, "%*s", width, buf);
        }
        break;

      case RWST_DEFAULT_TYPE:
        if (!iter->default_type) {
            if (iter->class_id != SK_INVALID_CLASS
                && iter->flowtype_id != SK_INVALID_FLOWTYPE)
            {
                flowtype_iter_t fi;
                flowtypeID_t ft;

                /* See if this type is a default for this class */
                sksiteClassDefaultFlowtypeIterator(iter->class_id, &fi);
                while (sksiteFlowtypeIteratorNext(&fi, &ft)) {
                    if (iter->flowtype_id == ft) {
                        sksiteFlowtypeGetType(buf, sizeof(buf), ft);
                        rv = printer(fd, "%*s", width, buf);
                        break;
                    }
                }
            }
            break;
        }
        /* Fall through */
      case RWST_TYPE:
        if (iter->flowtype_id != SK_INVALID_FLOWTYPE) {
            sksiteFlowtypeGetType(buf, sizeof(buf), iter->flowtype_id);
            rv = printer(fd, "%*s", width, buf);
        }
        break;

      case RWST_FLOWTYPE:
        if (iter->flowtype_id != SK_INVALID_FLOWTYPE) {
            sksiteFlowtypeGetName(buf, sizeof(buf), iter->flowtype_id);
            rv = printer(fd, "%*s", width, buf);
        }
        break;

      case RWST_FLOWTYPE_ID:
        if (iter->flowtype_id != SK_INVALID_FLOWTYPE) {
            rv = printer(fd, "%*" PRIu8, width, iter->flowtype_id);
        }
        break;

      case RWST_SENSOR:
        if (iter->sensor_id != SK_INVALID_SENSOR) {
            sksiteSensorGetName(buf, sizeof(buf), iter->sensor_id);
            rv = printer(fd, "%*s", width, buf);
        }
        break;

      case RWST_SENSOR_ID:
        if (iter->sensor_id != SK_INVALID_SENSOR) {
            rv = printer(fd, "%*" PRIu16, width, iter->sensor_id);
        }
        break;

      case RWST_SENSOR_DESC:
        if (iter->sensor_id != SK_INVALID_SENSOR) {
            s = sksiteSensorGetDescription(iter->sensor_id);
            if (s != NULL) {
                rv = printer(fd, "%*s", width, s);
            }
        }
        break;

      case RWST_DEFAULT_CLASS:
      case RWST_DEFAULT_CLASS_LIST:
        if (field == RWST_DEFAULT_CLASS_LIST
            || iter->class_id == sksiteClassGetDefault())
        {
            classID_t cid = sksiteClassGetDefault();
            if (cid != SK_INVALID_CLASS) {
                sksiteClassGetName(buf, sizeof(buf), cid);
                rv = printer(fd, "%*s", width, buf);
            }
        }
        break;

      case RWST_MARK_DEFAULTS:
        {
            char mark[3] = {' ', ' ', '\0'};
            int i = 0;
            if (iter->class_id != SK_INVALID_CLASS) {
                if (no_columns) {
                    memset(mark, 0, sizeof(mark));
                }
                if (iter->class_id == sksiteClassGetDefault())
                {
                    mark[0] = '+';
                    ++i;
                }
                if (iter->flowtype_id != SK_INVALID_FLOWTYPE) {
                    flowtype_iter_t fi;
                    flowtypeID_t ft;

                    /* See if this type is a default for this class */
                    sksiteClassDefaultFlowtypeIterator(iter->class_id, &fi);
                    while (sksiteFlowtypeIteratorNext(&fi, &ft)) {
                        if (iter->flowtype_id == ft) {
                            mark[no_columns ? i : 1] = '*';
                            ++i;
                            break;
                        }
                    }
                }
            }
            rv = printer(fd, "%*s", width, mark);
        }
        break;

      case RWST_CLASS_LIST:
        rv = rws_print_list_field(printer, fd, iter, RWST_CLASS, width);
        break;
      case RWST_TYPE_LIST:
        rv = rws_print_list_field(printer, fd, iter, RWST_TYPE, width);
        break;
      case RWST_FLOWTYPE_LIST:
        rv = rws_print_list_field(printer, fd, iter, RWST_FLOWTYPE, width);
        break;
      case RWST_FLOWTYPE_ID_LIST:
        rv = rws_print_list_field(printer, fd, iter, RWST_FLOWTYPE_ID, width);
        break;
      case RWST_SENSOR_LIST:
        rv = rws_print_list_field(printer, fd, iter, RWST_SENSOR, width);
        break;
      case RWST_SENSOR_ID_LIST:
        rv = rws_print_list_field(printer, fd, iter, RWST_SENSOR_ID, width);
        break;
      case RWST_DEFAULT_TYPE_LIST:
        rv = rws_print_list_field(printer, fd, iter, RWST_DEFAULT_TYPE, width);
        break;

      case RWST_MAX_FIELD_COUNT:
        skAbortBadCase(field);
    }

    /* Fill in any missing spaces. */
    if (rv < width) {
        printer(fd, "%*s", width - rv, "");
    }

    return rv;
}


/*
 *  len = rws_print_list_field(printer, fd, iter, field, width);
 *
 *    Print a 'width'-wide column containing the value for 'field'
 *    (where 'field' represents a "FIELD:list" field) using the
 *    fprintf-style function 'printer' to the FILE pointer 'fd', where
 *    'iter' is the current context.
 *
 *    Return the number of characters printed.
 */
static int
rws_print_list_field(
    rws_fprintf_t       printer,
    FILE               *fd,
    rws_iter_t         *iter,
    rws_field_t         field,
    int                 width)
{
    int total = 0;
    int len;
    rws_iter_t subiter;
    int first;

    /* Create a site iterator containing a single sub-iterator of the
     * correct type.  */
    memset(&subiter, 0, sizeof(subiter));
    switch (field) {
      case RWST_CLASS:
        if (iter->sensor_id == SK_INVALID_SENSOR) {
            subiter.order[0] = RWS_CLASS;
        } else {
            subiter.order[0] = RWS_CLASS_FROM_SENSOR;
        }
        subiter.sensor_id = iter->sensor_id;
        break;
      case RWST_TYPE:
      case RWST_FLOWTYPE:
      case RWST_FLOWTYPE_ID:
        if (iter->class_id == SK_INVALID_CLASS) {
            subiter.order[0] = RWS_FLOWTYPE;
        } else {
            subiter.order[0] = RWS_FLOWTYPE_FROM_CLASS;
        }
        subiter.class_id = iter->class_id;
        subiter.flowtype_id = iter->flowtype_id;
        break;
      case RWST_SENSOR:
      case RWST_SENSOR_ID:
        if (iter->class_id == SK_INVALID_CLASS) {
            subiter.order[0] = RWS_SENSOR;
        } else {
            subiter.order[0] = RWS_SENSOR_FROM_CLASS;
        }
        subiter.class_id = iter->class_id;
        break;
      case RWST_DEFAULT_TYPE:
        if (iter->class_id == SK_INVALID_CLASS) {
            return 0;
        }
        subiter.order[0] = RWS_DEFAULT_FLOWTYPE_FROM_CLASS;
        subiter.class_id = iter->class_id;
        break;
      default:
        skAbortBadCase(field);
    }
    rws_iter_bind(&subiter, 0);

    /* Call ourself with the fake-fprintf to determine the number of
     * printed characters in this field, and then print the padding */
    if (width != 0) {
        len = rws_print_list_field(fprintf_size, NULL, iter, field, 0);
        if (len < width) {
            total += printer(fd, "%*s", width - len, "");
        }
    }

    /* Iterate over the fields, printing each */
    first = 1;
    while (rws_iter_next(&subiter, 0)) {
        if (!first) {
            len = printer(fd, "%c", list_separator);
            total += len;
        }
        len = rws_print_field(printer, fd, &subiter, field, 0);
        total += len;
        first = 0;
    }

    return total;
}


/*
 *  rws_print_row(iter);
 *
 *    Print a single row from an iterator.
 */
static void
rws_print_row(
    rws_iter_t         *iter)
{
    size_t i;

    for (i = 0; i < num_fields; ++i) {
        if (i > 0) {
            fprintf(output.of_fp, "%c", column_separator);
        }
        rws_print_field(fprintf, output.of_fp, iter,
                        fields[i], col_width[fields[i]]);
    }
    fprintf(output.of_fp, "%s\n", final_delim);
}


SK_DIAGNOSTIC_FORMAT_NONLITERAL_PUSH
/*
 *  len = fprintf_size(fs, format, ...);
 *
 *    Used to calculate print sizes.  This fprintf() variation does
 *    not produce outout, it simply returns how many charcters it
 *    WOULD have printed.
 */
static int
fprintf_size(
    FILE        UNUSED(*stream),
    const char         *format,
    ...)
{
    char buf;
    int len;
    va_list ap;
    va_start(ap, format);
    len = vsnprintf(&buf, 0, format, ap);
    va_end(ap);
    return len;
}
SK_DIAGNOSTIC_FORMAT_NONLITERAL_POP


/*
 *  rws_calcsize_row(iter);
 *
 *    Updates column sizes based on a single row that would be printed
 *    from the given iterator.
 */
static void
rws_calcsize_row(
    rws_iter_t         *iter)
{
    size_t i;
    int len;

    for (i = 0; i < num_fields; i++) {
        len = rws_print_field(fprintf_size, NULL, iter, fields[i], 0);
        if (len > col_width[fields[i]]) {
            col_width[fields[i]] = len;
        }
    }
}


/*
 *  rws_print_titles();
 *
 *    Print the column headings unless --no-titles was specified.
 */
static void
rws_print_titles(
    void)
{
    size_t i;

    if (no_titles) {
        return;
    }

    for (i = 0; i < num_fields; ++i) {
        if (i > 0) {
            fprintf(output.of_fp, "%c", column_separator);
        }
        fprintf(output.of_fp, "%*s", col_width[fields[i]],
                (char *)field_map_entries[fields[i]].userdata);
    }
    fprintf(output.of_fp, "%s\n", final_delim);
}


/*
 *  status = rws_setup_iter_from_fields(iter);
 *
 *    Set up the iterator for iterating based on the field list.
 *
 *    Return 0 for normal iteration.  Return -1 for no output (other
 *    than titles).  Return 1 for outputting a single non-iterated
 *    entry.
 */
static int
rws_setup_iter_from_fields(
    rws_iter_t         *iter)
{
    size_t i;
    /* Next iterator level to initialize */
    int level = 0;
    /* Boolean: class iterator set? */
    int class_set = 0;
    /* Boolean: flowtype iterator set? */
    int flowtype_set = 0;
    /* Boolean: sensor iterator set? */
    int sensor_set = 0;
    /* Default-type iterator set at this level (0 == none, since
     * default-type cannot exist at level 0. */
    int default_type = 0;
    /* Boolean: could be a non-iterable singleton */
    int singleton = 0;

    memset(iter, 0, sizeof(*iter));
    iter->level = -1;

    for (i = 0; i < num_fields; i++) {
        switch (fields[i]) {
          case RWST_CLASS:
          case RWST_DEFAULT_CLASS:
          case RWST_DEFAULT_TYPE_LIST:
            /* Need classes to generate default types */
            if (class_set || flowtype_set) {
                break;
            }
            if (sensor_set) {
                iter->order[level] = RWS_CLASS_FROM_SENSOR;
            } else if (fields[i] == RWST_DEFAULT_CLASS) {
                iter->order[level] = RWS_DEFAULT_CLASS;
            } else {
                iter->order[level] = RWS_CLASS;
            }
            iter->level = level;
            ++level;
            class_set = 1;
            break;
          case RWST_TYPE:
          case RWST_FLOWTYPE:
          case RWST_FLOWTYPE_ID:
            if (flowtype_set) {
                break;
            }
            if (default_type) {
                /* Replace default type, as flowtype and default type
                 * is nonsensical */
                iter->order[default_type] =
                    class_set ? RWS_FLOWTYPE_FROM_CLASS : RWS_FLOWTYPE;
                default_type = 0;
            } else {
                iter->order[level] =
                    class_set ? RWS_FLOWTYPE_FROM_CLASS : RWS_FLOWTYPE;
                iter->level = level;
                ++level;
            }
            flowtype_set = 1;
            break;
          case RWST_SENSOR:
          case RWST_SENSOR_ID:
          case RWST_SENSOR_DESC:
            if (sensor_set) {
                break;
            }
            iter->order[level] = class_set ? RWS_SENSOR_FROM_CLASS : RWS_SENSOR;
            iter->level = level;
            ++level;
            sensor_set = 1;
            break;
          case RWST_DEFAULT_TYPE:
            assert(!default_type);
            if (flowtype_set) {
                break;
            }
            if (!class_set) {
                /* Default-type needs a class iterator, so add one */
                iter->order[level] =
                    sensor_set ? RWS_CLASS_FROM_SENSOR : RWS_CLASS;
                if (level == 0) {
                    iter->emit_level = 1;
                }
                ++level;
                class_set = 1;
            }
            iter->order[level] = RWS_DEFAULT_FLOWTYPE_FROM_CLASS;
            iter->level = level;
            ++level;
            default_type = level;
            break;
          case RWST_CLASS_LIST:
          case RWST_TYPE_LIST:
          case RWST_FLOWTYPE_LIST:
          case RWST_FLOWTYPE_ID_LIST:
          case RWST_SENSOR_LIST:
          case RWST_SENSOR_ID_LIST:
          case RWST_DEFAULT_CLASS_LIST:
            /* These fields can generate null-iterators with a single
             * row of output. */
            singleton = 1;
            break;
          case RWST_MARK_DEFAULTS:
            break;
          case RWST_MAX_FIELD_COUNT:
          default:
            skAbortBadCase(fields[i]);
        }
    }

    /* Bind the iterator (at level -1 if this is a null-iterator) */
    iter->flowtype_id = SK_INVALID_FLOWTYPE;
    iter->class_id    = SK_INVALID_CLASS;
    iter->sensor_id   = SK_INVALID_SENSOR;
    rws_iter_bind(iter, level ? 0 : -1);

    if (level != 0) {
        /* Return 0 if this is not a null iterator */
        return 0;
    }

    /* Return 1 if this is a singleton null-iterator.  -1 otherwise */
    return singleton ? 1 : -1;
}


int main(int argc, char **argv)
{
    rws_iter_t iter;
    rws_iter_t calciter;
    int rv;

    appSetup(argc, argv);       /* never returns on error */

    /* Set up site iterator */
    rv = rws_setup_iter_from_fields(&iter);
    if (rv == -1) {
        /* Nothing to do */
        rws_print_titles();
        return EXIT_SUCCESS;
    }

    if (!no_columns) {
        /* Calculate column sizes by generating the output using a
         * fake printer function that notes the column widths but does
         * not print the output. */
        calciter = iter;
        if (rv == 1) {
            /* Null iterator case */
            rws_calcsize_row(&calciter);
        } else {
            while (rws_iter_next(&calciter, 0)) {
                rws_calcsize_row(&calciter);
            }
        }
    }

    /* Print titles */
    rws_print_titles();

    /* Print rows */
    if (rv == 1) {
        /* Null iterator case */
        rws_print_row(&iter);
    } else {
        while (rws_iter_next(&iter, 0)) {
            rws_print_row(&iter);
        }
    }

    return EXIT_SUCCESS;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
