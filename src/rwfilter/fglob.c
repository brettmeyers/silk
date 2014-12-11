/*
** Copyright (C) 2001-2014 by Carnegie Mellon University.
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
**
**  10/14/2002
** Suresh L. Konda
**
** fglob.c
**
** routines for globbing files in the special dir hierarchy we have for
** packed files.
**
** There are three externally visible routines:
**  fglobSetup(): setup stuff
**  fglobNext():  call repeatedly till no more files
**  fglobTeardown(): clean up
**  fglobValid(): 1 if user specified any params; 0 else.
**
**  There is also a set of GetXX routines to gain access to the internals
**  of the fglob data structure.
**
** Details are given as block comments before these routines.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: fglob.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "rwfilter.h"


/* LOCAL TYPEDEFS AND DEFINES */

/* Output handle for --print-missing-files output */
#define MISSING_FH stderr

/*
 *  Structure for all pertinent information for a given find request.
 */
typedef struct fglobListStruct_st {
    /* flag: number of globbing options specified */
    int8_t          fg_user_option_count;
    /* flag: 1 if the globber was partly initialized; 2 if completely init'd */
    int8_t          fg_initialized;
    /* flag: 1 to print msg about existence of each file */
    int8_t          fg_missing;

    /* specified class/types */
    flowtypeID_t   *fg_flowtype_list;
    /* number of class/type pairs designated: length of fg_flowtype_list */
    int             fg_flowtype_count;
    /* current class/type when globbing; index into fg_flowtype_list */
    int             fg_flowtype_idx;

    /* list of sensor IDs wanted for each class/type */
    int           **fg_sensor_list;
    /* for each class/type, number of sensors: length of fg_sensor_list[i] */
    int            *fg_sensor_count;
    /* current sensor when globbing; index into
     * fg_sensor_list[fg_flowtype_idx] */
    int             fg_sensor_idx;

    /* start, end, current time---as milliseconds since the UNIX epoch
     * in UTC */
    sktime_t        fg_time_start;
    sktime_t        fg_time_end;
    sktime_t        fg_time_idx;

    /* user's input to various options.  This will be an array of
     * char*, index by the FGLOB_OPT_* enum.  The array will be
     * created once we know the number of options.*/
    char          **fg_option;

} fglobListStruct_t;



/* INTERNAL FUNCTIONS */

static int fglobAdjustCountersFlowtype(void);
static int fglobParseClassAndType(void);

static int fglobInit(void);
static int fglobLoadSiteConfig(void);
static int fglobInitTimes(void);
static int fglobHandler(clientData cData, int opt_index, char *opt_arg);
static void fglobEnableAllSensors(void);
static int fglobParseSensors(sk_bitmap_t **sensor_bits_ptr);


/* INTERNAL VARIABLES */

static fglobListStruct_t fglob_list;
static fglobListStruct_t * const fList = &fglob_list;




/* OPTIONS SETUP */

enum fglobOptionEnum {
    FGLOB_OPT_CLASS, FGLOB_OPT_TYPE, FGLOB_OPT_FLOWTYPES, FGLOB_OPT_SENSORS,
    FGLOB_OPT_START_DATE, FGLOB_OPT_END_DATE,
    FGLOB_OPT_PRINT_MISSING_FILES,
    FGLOB_OPT_DATA_ROOTDIR
};

static struct option fglobOptions[] = {
    {"class",                REQUIRED_ARG, 0, FGLOB_OPT_CLASS},
    {"type",                 REQUIRED_ARG, 0, FGLOB_OPT_TYPE},
    {"flowtypes",            REQUIRED_ARG, 0, FGLOB_OPT_FLOWTYPES},
    {"sensors",              REQUIRED_ARG, 0, FGLOB_OPT_SENSORS},
    {"start-date",           REQUIRED_ARG, 0, FGLOB_OPT_START_DATE},
    {"end-date",             REQUIRED_ARG, 0, FGLOB_OPT_END_DATE},
    {"print-missing-files",  NO_ARG,       0, FGLOB_OPT_PRINT_MISSING_FILES},
    {"data-rootdir",         REQUIRED_ARG, 0, FGLOB_OPT_DATA_ROOTDIR},
    {0, 0, 0, 0}
};


static const char *fglobHelp[] = {
    ("Class of data to process"),
    ("Type(s) of data to process within the specified class.  The\n"
     "\ttype names and default type(s) vary by class as shown in the table.\n"
     "\tUse 'all' to process every type for specified class.  The following\n"
     "\ttable lists \"* class (available types) Def. default types\":"),
    ("Comma separated list of class/type pairs to process.\n"
     "\tMay use 'all' for class and/or type.  This is an alternate way to\n"
     "\tspecify class/type; switch cannot be used with --class and --type"),
    ("Comma separated list of sensor names, sensor IDs, and ranges\n"
     "\tof sensor IDs.  Valid sensors vary by class.  Use 'rwsiteinfo' to\n"
     "\tsee a mapping of sensor names to IDs and classes."),
    ("First hour of data to process.  Specify date in\n"
     "\tYYYY/MM/DD[:HH] format; time is in "
#if  SK_ENABLE_LOCALTIME
     "localtime"
#else
     "UTC"
#endif
     ".  When no hour is\n"
     "\tspecified, the entire date is processed.  Def. Start of today"),
    ("Final hour of data to process specified as YYYY/MM/DD[:HH].\n"
     "\tWhen no hour specified, end of day is used unless start-date includes\n"
     "\tan hour.  When switch not specified, defaults to value in start-date"),
    ("Print the names of missing files to STDERR.\n"
     "\tDef. No"),
    ("Root of directory tree containing packed data"),
    (char*)NULL
};

/*
 *  is_ok = fglobSetup();
 *
 *    Initialize the values in the global fList structure: set
 *    everything to zero; set times to -1.  Register fglob options and
 *    handler.  Check the environment for variable giving the location
 *    of the data.
 *
 *    Return 0 if OK. 1 else.
 */
int
fglobSetup(
    void)
{
    size_t num_opts = sizeof(fglobOptions) / sizeof(struct option);

    /* verify same number of options and help strings */
    assert((sizeof(fglobHelp)/sizeof(char *)) ==
           (sizeof(fglobOptions)/sizeof(struct option)));

    /* zero out everything */
    memset(fList, 0, sizeof(fglobListStruct_t));

    /* Create an array to hold the option strings */
    fList->fg_option = (char**)calloc(num_opts, sizeof(char*));
    if (NULL == fList->fg_option) {
        skAppPrintOutOfMemory(NULL);
        return 1;
    }

    /* set start/end days/hours to be able to trap whether user gave
     * it explicitly
     */
    fList->fg_time_start = 0;
    fList->fg_time_end = 0;

    /* register the options */
    if (skOptionsRegister(fglobOptions, &fglobHandler, (clientData)fList)
        || sksiteOptionsRegister(SK_SITE_FLAG_CONFIG_FILE))
    {
        skAppPrintErr("Unable to register options");
        return 1;
    }

    return 0;                     /* OK */
}


/*
 *  fglobUsageClass(fh)
 *
 *    Print the usage for the --class switch to the specified file
 *    handle.
 */
static void
fglobUsageClass(
    FILE               *fh)
{
    classID_t class_id;
    class_iter_t ci;
    char class_name[SK_MAX_STRLEN_FLOWTYPE+1];
    int class_count = 0;

    /* CLASS: Use sksite to get list of classes */

    fprintf(fh, "%s", fglobHelp[FGLOB_OPT_CLASS]);

    class_id = sksiteClassGetDefault();
    if (class_id != SK_INVALID_CLASS) {
        sksiteClassGetName(class_name, sizeof(class_name), class_id);
        fprintf(fh, "  Def. %s", class_name);
    }

    sksiteClassIterator(&ci);
    while (sksiteClassIteratorNext(&ci, &class_id)) {
        sksiteClassGetName(class_name, sizeof(class_name), class_id);
        ++class_count;
        if (class_count == 1) {
            /* print first class */
            fprintf(fh, "\n\tAvailable classes: %s", class_name);
        } else {
            fprintf(fh, ",%s", class_name);
        }
    }
    fprintf(fh, "\n");
}


/*
 *  fglobUsageType(fh)
 *
 *    Print the usage for the --type switch to the specified file
 *    handle.
 */
static void
fglobUsageType(
    FILE               *fh)
{
    classID_t class_id;
    class_iter_t ci;
    flowtypeID_t flowtype_id;
    flowtype_iter_t fi;
    char name[SK_MAX_STRLEN_FLOWTYPE+1];
    char buf[128 + SK_MAX_STRLEN_FLOWTYPE];
    char *pos;
    int len;
    int flowtype_count;
    char *maybe_wrap;
    char old_char;
    const char *line_leader = "\t  * ";
    const char *cont_line_leader = "\t    ";
    const int wrap_col = 79 - 7;

    /* TYPE: For each class, read the list of types and
     * default list of types from sksite. */

    fprintf(fh, "%s\n", fglobHelp[FGLOB_OPT_TYPE]);

    /* loop over all the classes */
    sksiteClassIterator(&ci);
    while (sksiteClassIteratorNext(&ci, &class_id)) {
        pos = buf;

        /* print class name */
        sksiteClassGetName(name, sizeof(name), class_id);
        len = snprintf(pos, sizeof(buf)-(pos-buf), "%s%s", line_leader, name);
        pos += len;

        /* loop over the flowtypes in the class */
        flowtype_count = 0;
        sksiteClassFlowtypeIterator(class_id, &fi);
        while (sksiteFlowtypeIteratorNext(&fi, &flowtype_id)) {
            sksiteFlowtypeGetType(name, sizeof(name), flowtype_id);
            ++flowtype_count;
            maybe_wrap = pos;
            if (flowtype_count == 1) {
                /* print first type for this class */
                len = snprintf(pos, sizeof(buf)-(pos-buf), " (%s", name);
                pos += len;
            } else {
                /* print remaining types for the class */
                len = snprintf(pos, sizeof(buf)-(pos-buf), ",%s", name);
                pos += len;
            }
            /* check if we need to wrap the line */
            if ((pos - buf) > wrap_col) {
                if (flowtype_count > 1) {
                    ++maybe_wrap;
                }
                old_char = *maybe_wrap;
                *maybe_wrap = '\0';
                fprintf(fh, "%s\n", buf);
                *maybe_wrap = old_char;
                pos = buf;
                len = snprintf(pos, sizeof(buf)-(pos-buf), "%s%s",
                               cont_line_leader, maybe_wrap);
                pos += len;
            }
        }
        if (flowtype_count > 0) {
            len = snprintf(pos, sizeof(buf)-(pos-buf), ").");
            pos += len;
        }
        /* loop over the default flowtypes in the class */
        flowtype_count = 0;
        sksiteClassDefaultFlowtypeIterator(class_id, &fi);
        while (sksiteFlowtypeIteratorNext(&fi, &flowtype_id)) {
            sksiteFlowtypeGetType(name, sizeof(name), flowtype_id);
            ++flowtype_count;
            maybe_wrap = pos;
            if (flowtype_count == 1) {
                /* print first default type for this class */
                len = snprintf(pos, sizeof(buf)-(pos-buf), " Def. %s", name);
                pos += len;
            } else {
                len = snprintf(pos, sizeof(buf)-(pos-buf), ",%s", name);
                pos += len;
            }
            /* check if we need to wrap the line */
            if ((pos - buf) > wrap_col) {
                if (flowtype_count > 1) {
                    ++maybe_wrap;
                }
                old_char = *maybe_wrap;
                *maybe_wrap = '\0';
                fprintf(fh, "%s\n", buf);
                if (old_char == ' ') {
                    ++maybe_wrap;
                } else {
                    *maybe_wrap = old_char;
                }
                pos = buf;
                len = snprintf(pos, sizeof(buf)-(pos-buf), "%s%s",
                               cont_line_leader, maybe_wrap);
                pos += len;
            }
        }
        fprintf(fh, "%s\n", buf);
    }
}


/*
 *  fglobUsage(fh);
 *
 *    Print, to the given file handle, the usage for the command-line
 *    switches provided by the fglob library.
 */
void
fglobUsage(
    FILE               *fh)
{
#define MIN_TEXT_ON_LINE  15
#define MAX_TEXT_ON_LINE  72

    char *cp, *ep, *sp;
    char buf[2 * PATH_MAX];
    char path[PATH_MAX];
    int have_config = 0;
    int opt;

    fprintf(fh, ("\nFILE SELECTION SWITCHES choose which files to read"
                 " from the data store:\n\n"));

    switch (sksiteConfigure(0)) {
      case 0:
        have_config = 1;
        break;
      case -1:
        fprintf(fh, "WARNING: site configuration file contains errors\n");
        break;
      case -2:
        fprintf(fh, "WARNING: site configuration file was not found\n");
        break;
      default:
        fprintf(fh, ("WARNING: site configuration file"
                     "was not found or contains errors\n"));
        break;
    }

    for (opt = 0; fglobOptions[opt].name; ++opt) {
        fprintf(fh, "--%s %s. ", fglobOptions[opt].name,
                SK_OPTION_HAS_ARG(fglobOptions[opt]));
        switch (fglobOptions[opt].val) {

          case FGLOB_OPT_CLASS:
            fglobUsageClass(fh);
            break;

          case FGLOB_OPT_TYPE:
            fglobUsageType(fh);
            break;

          case FGLOB_OPT_SENSORS:
            fprintf(fh, "%s", fglobHelp[opt]);
            if (have_config) {
                fprintf(fh, "  Valid IDs are %u--%u",
                        sksiteSensorGetMinID(), sksiteSensorGetMaxID());
            }
            fprintf(fh, "\n");
            break;

          case FGLOB_OPT_DATA_ROOTDIR:
            fprintf(fh, "%s.\n", fglobHelp[opt]);

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
            fprintf(fh, "%s\n", fglobHelp[opt]);
            break;
        }
    }

    sksiteOptionsUsage(fh);
}


/*
 *  fglobTeardown()
 *
 *    Free the elements in fList which were malloc'ed.  Do not free
 *    the fList struct itself---it is a global static.  Multiple calls
 *    to this function are handled gracefully.
 */
void
fglobTeardown(
    void)
{
    static int teardownFlag = 0;
    int i;

    /* Idempotency check. */
    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    /* free our data structures */
    if (fList->fg_flowtype_list) {
        free(fList->fg_flowtype_list);
        fList->fg_flowtype_list = NULL;
    }
    if (fList->fg_sensor_list) {
        for (i = sksiteFlowtypeGetMaxID(); i >= 0; --i) {
            if (fList->fg_sensor_list[i]) {
                free(fList->fg_sensor_list[i]);
            }
        }
        free(fList->fg_sensor_list);
        fList->fg_sensor_list = NULL;
    }
    if (fList->fg_sensor_count) {
        free(fList->fg_sensor_count);
        fList->fg_sensor_count = NULL;
    }

    /* free the options */
    if (fList->fg_option) {
        free(fList->fg_option);
        fList->fg_option = NULL;
    }

    return;
}


/*
 *  filename = fglobNext();
 *
 *    Return the name of next available file.  Returns NULL if all
 *    files have been processed.
 *
 *    Will complete the initialization of the library if required.
 */
char *
fglobNext(
    char               *buf,
    size_t              bufsize)
{
    int (*file_exists_fn)(const char *path);
    char *ext;

    {
        file_exists_fn = &skFileExists;
    }

    if (!fList->fg_initialized) {
        if (fglobInit()) {
            /* error */
            return NULL;
        }
    }

    /* keep adjusting the counters until we find a file that exists */
    while (fglobAdjustCountersFlowtype()) {
        /* Create the full path to the data file from the root dir,
         * pathPrefix, year, month, day, hour, flow-type dependent
         * file prefix, and sensor name.  Create the name with a '.gz'
         * extention, but hide the extension the first time we look
         * for the file. */
        if (!sksiteGeneratePathname(
                buf, bufsize, fList->fg_flowtype_list[fList->fg_flowtype_idx],
                fList->fg_sensor_list[fList->fg_flowtype_idx]
                [fList->fg_sensor_idx], fList->fg_time_idx, ".gz", NULL, NULL))
        {
            continue;
        }

        /* hide the compression extension */
        ext = &buf[strlen(buf) - 3];
        assert(*ext == '.');
        *ext = '\0';

        if (!file_exists_fn(buf)) {
            /* check for compressed version of file */
            *ext = '.';

            if (!file_exists_fn(buf)) {
                if (fList->fg_missing) {
                    *ext = '\0';
                    fprintf(MISSING_FH, "Missing %s\n", buf);
                }
                continue;
            }
        }

        return buf;
    }

    return NULL;
}


/*
 *  count = fglobFileCount();
 *
 *    Return an estimate (upper bound) of the number of files
 *    remaining to process.  The returned value assumes that a file
 *    exists for every valid hour-flowtype-sensor tuple which is left
 *    to process.
 */
int
fglobFileCount(
    void)
{
    int count = 0;
    sktime_t hours;
    int i;

    if (!fList->fg_initialized) {
        if (fglobInit()) {
            /* error */
            return -1;
        }
    }

    /* compute number of files we visit every hour, which is the
     * number of sensors that exist for every flowtype */
    for (i = 0; i < fList->fg_flowtype_count; ++i) {
        count += fList->fg_sensor_count[i];
    }

    /* compute number of hours we have left to process */
    hours = 1 + ((fList->fg_time_end - fList->fg_time_idx) / 3600000);

    /* files to visit is the product of those two values */
    count *= hours;

    /* if no files have been processed yet, just return */
    if (fList->fg_initialized < 2) {
        return count;
    }

    /* remove the files for the flowtypes we have already processed
     * in this hour */
    for (i = 0; i < fList->fg_flowtype_idx; ++i) {
        count -= fList->fg_sensor_count[i];
    }

    /* remove the files for the sensors associated with the current
     * flowtype which we have already processed  */
    count -= fList->fg_sensor_idx;

    return count;
}


/*
 *  is_ok = fglobInit();
 *
 *    This is an internal function and should be called to initialize
 *    things based on the options given by the user.  Hence this must
 *    be called after the options are parsed.  To make life easier for
 *    the application programmer, this routine is called by the first
 *    call to fglobNext().
 *
 *    Return 0 for OK. 1 else;
 */
static int
fglobInit(
    void)
{
    fList->fg_initialized = 1;

    /* load the site config */
    if (fglobLoadSiteConfig()) {
        /* error */
        return 1;
    }

    /* set up the times */
    if (fglobInitTimes()) {
        /* error */
        return 1;
    }

    /* parse the classes and types */
    if (fglobParseClassAndType()) {
        /* error */
        return 1;
    }

    /* now parse the sensor list and check w.r.t. the class/type. */
    if (fglobParseSensors(NULL)) {
        /* error */
        return 1;
    }

    return 0;
}


/*
 *  is_ok = fglobHandler(cData, opt_index, opt_arg);
 *
 *    Called by options processor.  Return 1 on error, 0 on success.
 */
static int
fglobHandler(
    clientData   UNUSED(cData),
    int                 opt_index,
    char               *opt_arg)
{
    fList->fg_user_option_count++;

    switch (opt_index) {
      case FGLOB_OPT_CLASS:
      case FGLOB_OPT_TYPE:
      case FGLOB_OPT_FLOWTYPES:
      case FGLOB_OPT_SENSORS:
      case FGLOB_OPT_START_DATE:
      case FGLOB_OPT_END_DATE:
        if (fList->fg_option[opt_index] != NULL) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          fglobOptions[opt_index].name);
            return 1;
        }
        fList->fg_option[opt_index] = opt_arg;
        break;

      case FGLOB_OPT_PRINT_MISSING_FILES:
        fList->fg_missing = 1;
        break;

      case FGLOB_OPT_DATA_ROOTDIR:
        if (!skDirExists(opt_arg)) {
            skAppPrintErr("Root data directory '%s' does not exist", opt_arg);
            return 1;
        }
        if (sksiteSetRootDir(opt_arg)) {
            skAppPrintErr("Unable to set root data directory to %s", opt_arg);
            return 1;
        }
        break;

      default:
        /* programmer error */
        skAbortBadCase(opt_index);
    }

    return 0;                     /* OK */
}


/*
 *  have_more_data = fglobAdjustCountersFlowtype();
 *
 *    Go to the next (or first if fg_initialized is < 2) sensor, or
 *    type, or class, or hour, or day.  Return 0 if there is nothing
 *    else to iterate over, or 1 otherwise.
 *
 *    fglobInit() must be called before calling this function.
 *
 *    When fg_initialized < 2, the function assumes this is the first
 *    time it has been called and it will complete the initialization
 *    of the counters that was started by fglobInit(); otherwise it
 *    will increment the counters.
 */
static int
fglobAdjustCountersFlowtype(
    void)
{
    if (fList->fg_initialized < 2) {
        /* was init called? */
        assert(fList->fg_initialized);

        /* This is the first call into this function.  All of the
         * indexes are at zero; there is nothing we need to lookup, so
         * simply set the "we've been here" flag and return. */
        fList->fg_initialized = 2;

        return 1;
    }

    /* This is not the first call into this function; need to
     * increment or reset the appropriate counters. */

    /* Make certain we haven't reached the end of the data. */
    if (fList->fg_time_idx > fList->fg_time_end) {
        return 0;
    }

    /* First, see if we can increment the sensor */
    fList->fg_sensor_idx++;
    if(fList->fg_sensor_idx < fList->fg_sensor_count[fList->fg_flowtype_idx]){
        /* we're good: looking at next sensor--same class, type & time */
        return 1;
    }
    /* On last sensor; reset and try class/type */
    fList->fg_sensor_idx = 0;

    /* Try to increment the class/type */
    if (fList->fg_flowtype_count > 1) {
        fList->fg_flowtype_idx++;
        if (fList->fg_flowtype_idx < fList->fg_flowtype_count) {
            /* we're good: looking at next class/type -- same time */
            return 1;
        }
        /* On last class/type; reset and try time */
        fList->fg_flowtype_idx = 0;
    }

    /* Finally, increment the time: go to next hour */
    fList->fg_time_idx += 3600000;
    if (fList->fg_time_idx > fList->fg_time_end)
    {
        /* We're done. */
        return 0;
    }

    return 1;
}


/*
 *  status = fglobLoadSiteConfig();
 *
 *    Load the site configuration file and allocate the arrays of
 *    flowtypes and sensors that depend on the number of items
 *    defined.  Return 0 for success, non-zero on error.
 */
static int
fglobLoadSiteConfig(
    void)
{
    int flowtype_count;
    int sensor_count;
    int i;

    /* load the site config */
    if (sksiteConfigure(1)) {
        /* error */
        return 1;
    }

    /* return if we have already allocated everything */
    if (fList->fg_sensor_count != NULL) {
        return 0;
    }

    /* Allocate memory for lists, based on sizes from sksite */
    flowtype_count = sksiteFlowtypeGetMaxID() + 1;
    sensor_count = sksiteSensorGetMaxID() + 1;

    fList->fg_flowtype_list
        = (flowtypeID_t*)calloc(flowtype_count, sizeof(flowtypeID_t));
    if (fList->fg_flowtype_list == NULL) {
        skAppPrintOutOfMemory(NULL);
        return 1;
    }
    fList->fg_sensor_list = (int**)calloc(flowtype_count, sizeof(int*));
    if (fList->fg_sensor_list == NULL) {
        skAppPrintOutOfMemory(NULL);
        return 1;
    }
    for (i = 0; i < flowtype_count; ++i) {
        fList->fg_sensor_list[i] = (int*)calloc(sensor_count + 1, sizeof(int));
        if (fList->fg_sensor_list[i] == NULL) {
            skAppPrintOutOfMemory(NULL);
            return 1;
        }
    }

    fList->fg_sensor_count = (int*)calloc(flowtype_count, sizeof(int));
    if (fList->fg_sensor_count == NULL) {
        skAppPrintOutOfMemory(NULL);
        return 1;
    }

    return 0;
}


/*
 *  is_ok = fglobInitTimes();
 *
 *    Verify the user's time inputs and initialize the start and end
 *    times based on the input.  Return 0 if all is well; otherwise
 *    print an error and return 1.
 */
static int
fglobInitTimes(
    void)
{
    sktime_t start_val;
    sktime_t end_val;
    int start_precision = -1;
    int end_precision = -1;
    time_t t;
    int rv;
#if  SK_ENABLE_LOCALTIME
    struct tm work_tm;
    int work_hour;
#endif

    /* If no start time, make certain there was no end time, then look
     * at everything from today in local time. */
    if (!fList->fg_option[FGLOB_OPT_START_DATE]) {
        if (fList->fg_option[FGLOB_OPT_END_DATE]) {
            skAppPrintErr("Cannot use --%s without --%s",
                          fglobOptions[FGLOB_OPT_END_DATE].name,
                          fglobOptions[FGLOB_OPT_START_DATE].name);
            return 1;
        }

        time(&t);

        /* Take the floor of the time in hours */
        t = t - (t % 3600);

        /* Set the ending time to current hour */
        fList->fg_time_end = sktimeCreate(t, 0);

        /* Go to start of today */
#if   SK_ENABLE_LOCALTIME
        localtime_r(&t, &work_tm);
        work_tm.tm_hour = 0;
        t = mktime(&work_tm);
#else
        t = t - (t % 86400);
#endif

        /* Set the starting time */
        fList->fg_time_start = sktimeCreate(t, 0);

        /* Set the current time to the start time */
        fList->fg_time_idx = fList->fg_time_start;
        return 0;
    }

    /* Parse the starting time */
    rv = skStringParseDatetime(&start_val,
                               fList->fg_option[FGLOB_OPT_START_DATE],
                               &start_precision);
    if (rv) {
        skAppPrintErr("Invalid %s '%s': %s",
                      fglobOptions[FGLOB_OPT_START_DATE].name,
                      fList->fg_option[FGLOB_OPT_START_DATE],
                      skStringParseStrerror(rv));
        return 1;
    }
    if (start_precision > 4) {
        skAppPrintErr("Warning: %s precision greater than hours ignored",
                      fglobOptions[FGLOB_OPT_START_DATE].name);
    }

    /* Take to start of hour */
    fList->fg_time_start = start_val - (start_val % 3600000);

    if (fList->fg_option[FGLOB_OPT_END_DATE]) {
        rv = skStringParseDatetime(&end_val,
                                   fList->fg_option[FGLOB_OPT_END_DATE],
                                   &end_precision);
        if (rv) {
            skAppPrintErr("Invalid %s '%s': %s",
                          fglobOptions[FGLOB_OPT_END_DATE].name,
                          fList->fg_option[FGLOB_OPT_END_DATE],
                          skStringParseStrerror(rv));
            return 1;
        }

        /* Force end time to start of hour */
        fList->fg_time_end = end_val - (end_val % 3600000);

        /* Make any required adjustements to end-time */
        if (start_precision == 3) {
            /* when no starting hour given, we look at the full days,
             * regardless of the precision of the ending time; go to
             * the last hour of the ending day. */
            if (end_precision >= 4) {
                skAppPrintErr(("Warning: %s precision greater than days\n"
                               "\tignored when %s has no hour"),
                              fglobOptions[FGLOB_OPT_END_DATE].name,
                              fglobOptions[FGLOB_OPT_START_DATE].name);
            }
            if (skDatetimeCeiling(&end_val, &end_val, start_precision)) {
                return 1;
            }
            fList->fg_time_end = end_val - (end_val % 3600000);
        } else if (end_precision < 4) {
            /* starting time has an hour but ending time does not; use
             * same hour for ending time */
#if  SK_ENABLE_LOCALTIME
            /* get starting hour */
            t = start_val / 1000;
            localtime_r(&t, &work_tm);
            work_hour = work_tm.tm_hour;
            /* break apart end time */
            t = end_val / 1000;
            localtime_r(&t, &work_tm);
            /* set end hour to start hour and re-combine */
            work_tm.tm_hour = work_hour;
            t = mktime(&work_tm);
            fList->fg_time_end = sktimeCreate((t - (t % 3600)), 0);
#else
            fList->fg_time_end = (fList->fg_time_end
                                  - (fList->fg_time_end % 86400000)
                                  + (fList->fg_time_start % 86400000));
#endif
        } else if (end_precision > 4) {
            skAppPrintErr("Warning: %s precision greater than hours ignored",
                          fglobOptions[FGLOB_OPT_END_DATE].name);
        }

    } else if (start_precision >= 4) {
        /* no ending time was given and the starting time contains an
         * hour; we only look at that single hour */
        fList->fg_time_end = fList->fg_time_start;
    } else {
        /* no ending time was given and the starting time was to the
         * day; look at that entire day */
        if (skDatetimeCeiling(&end_val, &start_val, start_precision)) {
            return 1;
        }
        /* Force end time to start of hour */
        fList->fg_time_end = end_val - (end_val % 3600000);
    }

    if (fList->fg_time_end < fList->fg_time_start) {
        skAppPrintErr("%s of %s is earlier than\n\t%s of %s",
                      fglobOptions[FGLOB_OPT_END_DATE].name,
                      fList->fg_option[FGLOB_OPT_END_DATE],
                      fglobOptions[FGLOB_OPT_START_DATE].name,
                      fList->fg_option[FGLOB_OPT_START_DATE]);
        return 1;
    }

    /* Set the current time to the starting value. */
    fList->fg_time_idx = fList->fg_time_start;

    return 0;
}


/*
 *  fglobEnableAllSensors();
 *
 *    For each of the configured flowtypes, add every sensor that is
 *    valid for that flowtype.
 */
static void
fglobEnableAllSensors(
    void)
{
    sensor_iter_t si;
    sensorID_t sensor_id;
    class_iter_t ci;
    classID_t class_of_sensor;
    int k;

    /* loop 'sensor_id' over all the sensors in silk.conf */
    sksiteSensorIterator(&si);
    while (sksiteSensorIteratorNext(&si, &sensor_id)) {

        /* loop 'class_of_sensor' over all classes 'sensor_id' is a
         * member of */
        sksiteSensorClassIterator(sensor_id, &ci);
        while (sksiteClassIteratorNext(&ci, &class_of_sensor)) {

            /* loop 'k' over all the flowtypes we are using */
            for (k = 0; k < fList->fg_flowtype_count; ++k) {

                /* if the class of the flowtype equals
                 * 'class_of_sensor', we will search over that
                 * sensor */
                if (sksiteFlowtypeGetClassID(fList->fg_flowtype_list[k])
                    == class_of_sensor)
                {
                    /* add this sensor to this flowtype */
                    fList->fg_sensor_list[k][fList->fg_sensor_count[k]++] =
                        sensor_id;
                }
            }
        }
    }
}


/*
 *  fglobAddSensor(sensor_bits, sensor_id, warn_unused);
 *
 *    Check whether 'sensor_id' has been set in the 'sensor_bits'
 *    bitmap.  If so, return immediately.
 *
 *    Otherwise, add it to the bitmap.  In addition, for the classes
 *    that have been specified that 'sensor_id' is a member of, add
 *    sensor_id to the list of sensors to process for that flowtype.
 *    If 'sensor_id' does not belong to any specified flowtypes, print
 *    a warning if 'warn_unused' is non-zero.
 */
static void
fglobAddSensor(
    sk_bitmap_t        *sensor_bits,
    sensorID_t          sensor_id,
    int                 warn_unused)
{
    int found_sensor = 0;
    class_iter_t ci;
    classID_t class_of_sensor;
    int k;

    /* return if we have already processed this sensor */
    if (skBitmapGetBit(sensor_bits, sensor_id)) {
        return;
    }

    skBitmapSetBit(sensor_bits, sensor_id);

    /* loop 'class_of_sensor' over all classes that 'sensor_id' is a
     * member of */
    sksiteSensorClassIterator(sensor_id, &ci);
    while (sksiteClassIteratorNext(&ci, &class_of_sensor)) {
        /* loop 'k' over all the flowtypes we are using */
        for (k = 0; k < fList->fg_flowtype_count; ++k) {

            /* if the class of the flowtype equals 'class_of_sensor',
             * we will search over that sensor */
            if (sksiteFlowtypeGetClassID(fList->fg_flowtype_list[k])
                == class_of_sensor)
            {
                /* add this sensor to this flowtype */
                fList->fg_sensor_list[k][fList->fg_sensor_count[k]++] =
                    sensor_id;
                ++found_sensor;
            }
        }
    }

    /* warn about unused sensor */
    if ((0 == found_sensor) && warn_unused) {
        char sensor_name[SK_MAX_STRLEN_SENSOR+1];

        if (sksiteSensorExists(sensor_id)) {
            sksiteSensorGetName(sensor_name, sizeof(sensor_name), sensor_id);
            skAppPrintErr(("Ignoring sensor %s (ID=%u) that is"
                           " not used by specified flowtype%s"),
                          sensor_name, sensor_id,
                          ((fList->fg_flowtype_count > 1) ? "s" : ""));
        }
    }
}


/*
 *  is_ok = fglobParseSensors(sensor_bitmap_ptr);
 *
 *    Set the list of sensors to process based on the user's --sensor
 *    input that is specified in the fList global variable.  If the
 *    user did not specify --sensors, use all sensors for the
 *    specified flowtypes.
 *
 *    The sensor list can be a combination of named sensors and lone
 *    integers or integer ranges, separated by commas:
 *    "S01,8,3-6,S02".
 *
 *    For each sensor parsed, the fglobAddSensor() function will be
 *    called with that sensor's ID.
 *
 *    If parsing is successful and if 'sensor_bitmap_ptr' is non-NULL,
 *    an sk_bitmap_t will be created in the location it specifies.  It
 *    is the caller's responsibility to destroy the bitmap.  The
 *    'sensor_bitmap_ptr' should only be non-NULL when a --sensors
 *    value has been specified.
 *
 *    Function returns 0 on success or 1 on error: malloc error, bad
 *    sensor name, number out of range.
 */
static int
fglobParseSensors(
    sk_bitmap_t       **sensor_bits_ptr)
{
    char *user_sensors = NULL;
    char *user_sensors_freeable = NULL;
    char *sensor_token = NULL;
    sk_bitmap_t *sensor_bits = NULL;
    uint32_t min_sensor_id;
    uint32_t max_sensor_id;
    uint32_t val_min, val_max;
    sensorID_t sid = SK_INVALID_SENSOR;
    int p_err;
    int i;
    int rv = 0;

    /* enable all sensors if no --sensors line was given */
    if (NULL == fList->fg_option[FGLOB_OPT_SENSORS]) {
        assert(NULL == sensor_bits_ptr);
        fglobEnableAllSensors();
        return 0;
    }

    min_sensor_id = sksiteSensorGetMinID();
    max_sensor_id = sksiteSensorGetMaxID();

    /* create a bitmap for all the sensors */
    if (skBitmapCreate(&sensor_bits, 1 + max_sensor_id)) {
        skAppPrintOutOfMemory(NULL);
        rv = 1;
        goto END;
    }

    /* create a copy of the input string and maintain a reference to
     * it so we can free it */
    user_sensors = strdup(fList->fg_option[FGLOB_OPT_SENSORS]);
    user_sensors_freeable = user_sensors;
    if (user_sensors == NULL) {
        skAppPrintOutOfMemory(NULL);
        rv = 1;
        goto END;
    }

    /* parse the sensors as a comma separated list of tokens */
    while ((sensor_token = strsep(&user_sensors, ",")) != NULL) {
        /* check for empty token (e.g., double comma) */
        if ('\0' == *sensor_token) {
            continue;
        }

        /* lookup sensor_token as a sensor name */
        sid = sksiteSensorLookup(sensor_token);
        if (SK_INVALID_SENSOR != sid) {
            /* found it. add it to our lists and continue */
            fglobAddSensor(sensor_bits, sid, (sensor_bits_ptr == NULL));
            continue;
        }

        /* parsing failed. does the token look like a number? */
        if (!isdigit((int)(*sensor_token))) {
            /* not a number: error */
            skAppPrintErr("Invalid %s '%s': Unknown sensor name",
                          fglobOptions[FGLOB_OPT_SENSORS].name, sensor_token);
            rv = 1;
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
                          fglobOptions[FGLOB_OPT_SENSORS].name, sensor_token,
                          skStringParseStrerror(p_err));
            rv = 1;
            continue;
        }

        /* verify that start of range is valid */
        sid = (sensorID_t)val_min;
        if (!sksiteSensorExists(sid)) {
            skAppPrintErr(("Invalid %s: Value %" PRIu32
                           " is not valid sensor id"),
                          fglobOptions[FGLOB_OPT_SENSORS].name, val_min);
            rv = 1;
            continue;
        }

        /* verify that end of range is valid */
        if (val_min != val_max && !sksiteSensorExists((sensorID_t)val_max)) {
            skAppPrintErr(("Invalid %s: Value %" PRIu32
                           " is not valid sensor id"),
                          fglobOptions[FGLOB_OPT_SENSORS].name, val_max);
            rv = 1;
            continue;
        }

        /* add all sensor IDs in range */
        for ( ; sid <= (sensorID_t)val_max; ++sid) {
            fglobAddSensor(sensor_bits, sid, (sensor_bits_ptr == NULL));
        }
    }

    /* did we find any sensors? */
    if ((0 == rv) && (0 == skBitmapGetHighCount(sensor_bits))) {
        skAppPrintErr("Invalid %s: No valid sensors",
                      fglobOptions[FGLOB_OPT_FLOWTYPES].name);
        rv = 1;
    }

    /* does each class/type have sensors? */
    if ((0 == rv) && skBitmapGetHighCount(sensor_bits)
        && (sensor_bits_ptr == NULL))
    {
        /* For any class/type combinations for which we didn't find a
         * sensor, print an error. */
        char class_name[SK_MAX_STRLEN_FLOWTYPE+1];
        char type_name[SK_MAX_STRLEN_FLOWTYPE+1];

        for (i = 0; i < fList->fg_flowtype_count; ++i) {
            if (0 == fList->fg_sensor_count[i]) {
                sksiteFlowtypeGetClass(class_name, sizeof(class_name),
                                       fList->fg_flowtype_list[i]);
                sksiteFlowtypeGetType(type_name, sizeof(type_name),
                                      fList->fg_flowtype_list[i]);
                skAppPrintErr(("No corresponding sensors given for"
                               " class/type pair '%s/%s'"),
                              class_name, type_name);
                rv = 1;
                break;
            }
        }
    }

  END:
    if (user_sensors_freeable) {
        free(user_sensors_freeable);
    }

    if (sensor_bits_ptr && (0 == rv)) {
        *sensor_bits_ptr = sensor_bits;
    } else if (sensor_bits) {
        skBitmapDestroy(&sensor_bits);
    }

    return rv;
}


/*
 *  fglobAddFlowtype(flowtype_id);
 *
 *    Append 'flowtype_id' to the list of flowtypes to use.  However,
 *    if 'flowtype_id' has been previously added to the list of
 *    flowtypes, ignore it.
 */
static void
fglobAddFlowtype(
    flowtypeID_t        flowtype_id)
{
    int j;

    assert(flowtype_id != SK_INVALID_FLOWTYPE);

    for (j = 0; j < fList->fg_flowtype_count; ++j) {
        if (flowtype_id == fList->fg_flowtype_list[j]) {
            return;
        }
    }

    fList->fg_flowtype_list[fList->fg_flowtype_count] = flowtype_id;
    ++fList->fg_flowtype_count;
}


/*
 *  ok = fglobParseFlowtypes();
 *
 *    Parse the list of comma-separated class/type pairs listed in the
 *    --flowtypes switch.  Return 0 on success, or 1 on failure.
 */
static int
fglobParseFlowtypes(
    void)
{
    char *user_flowtypes = NULL;
    char *user_flowtypes_freeable = NULL;
    char *class_name;
    char *type_name;
    flowtype_iter_t fi;
    flowtypeID_t ft;
    classID_t class_id;
    int rv = 0;

    /* create a modifiable version of the user's input. store a
     * pointer to it we can free */
    user_flowtypes = strdup(fList->fg_option[FGLOB_OPT_FLOWTYPES]);
    user_flowtypes_freeable = user_flowtypes;
    if (NULL == user_flowtypes) {
        skAppPrintOutOfMemory(NULL);
        rv = 1;
        goto END;
    }

    /* parse user's string as a comma separated list */
    while ((class_name = strsep(&user_flowtypes, ",")) != NULL) {

        /* break token into class and type separated by '/' */
        type_name = strchr(class_name, '/');
        if (NULL == type_name) {
            if (class_name[0] == '\0') {
                /* empty token (e.g., double comma) */
                continue;
            }
            skAppPrintErr("Invalid %s: Missing '/' in token '%s'",
                          fglobOptions[FGLOB_OPT_FLOWTYPES].name, class_name);
            rv = 1;
            continue;
        }
        *type_name = '\0';
        ++type_name;

        /* find class and type.  if lookup fails, test for special
         * "all" keyword */
        ft = sksiteFlowtypeLookupByClassType(class_name, type_name);
        if (SK_INVALID_FLOWTYPE != ft) {
            /* Yay!  Class and type are specific */
            fglobAddFlowtype(ft);

        } else if (0 == strcmp(class_name, "all")) {
            if (0 == strcmp(type_name, "all")) {
                /* Use all classes and all types.  Clear flowtype
                 * count, then add all flowtypes */
                fList->fg_flowtype_count = 0;
                sksiteFlowtypeIterator(&fi);
                while (sksiteFlowtypeIteratorNext(&fi, &ft)) {
                    fglobAddFlowtype(ft);
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
                        fglobAddFlowtype(ft);
                    }
                }
                if (!found_type) {
                    skAppPrintErr(("Invalid %s:"
                                   " Type '%s' not valid for any class"),
                                  fglobOptions[FGLOB_OPT_FLOWTYPES].name,
                                  type_name);
                    rv = 1;
                }
            }
        } else if (0 == strcmp(type_name, "all")) {
            /* Use all types in the specified class */
            class_id = sksiteClassLookup(class_name);
            if (SK_INVALID_CLASS == class_id) {
                skAppPrintErr("Invalid %s: Invalid class '%s'",
                              fglobOptions[FGLOB_OPT_FLOWTYPES].name,
                              class_name);
                rv = 1;
            } else {
                sksiteClassFlowtypeIterator(class_id, &fi);
                while (sksiteFlowtypeIteratorNext(&fi, &ft)) {
                    fglobAddFlowtype(ft);
                }
            }
        } else {
            /* Invalid class/type */
            skAppPrintErr("Invalid %s: Unknown class/type pair '%s/%s'",
                          fglobOptions[FGLOB_OPT_FLOWTYPES].name,
                          class_name, type_name);
            rv = 1;
        }
    }

    if ((rv == 0) && (fList->fg_flowtype_count == 0)) {
        skAppPrintErr("No valid class/type pairs given in --%s",
                      fglobOptions[FGLOB_OPT_FLOWTYPES].name);
        rv = 1;
    }
    if (rv == 1) {
        skAppPrintErr("Use the --help option to see valid classes and types");
    }

  END:
    if (user_flowtypes_freeable) {
        free(user_flowtypes_freeable);
    }
    return rv;
}


/*
 *  is_ok = fglobParseClassAndType();
 *
 *    Will parse the user's --class and --type input, or use the
 *    defaults if no class and/or type was given.  Return 0 on
 *    success, 1 on failure.
 *
 *    Will also verify that each listed type belongs to a class, and
 *    each class has at least one type.
 */
static int
fglobParseClassAndType(
    void)
{
    char *user_types = NULL;
    char *user_types_freeable = NULL;
    char *type_token;
    int rv = 0; /* return value */
    classID_t class_id;
    char path[PATH_MAX];
    char class_name_buf[SK_MAX_STRLEN_FLOWTYPE+1];
    char *class_name;
    flowtype_iter_t fi;
    flowtypeID_t flowtype_id;

    /* have we been here before? */
    if (fList->fg_flowtype_count > 0) {
        return 0;
    }

    /* handle case when --flowtypes is given */
    if (fList->fg_option[FGLOB_OPT_FLOWTYPES]) {
        if (fList->fg_option[FGLOB_OPT_CLASS]
            || fList->fg_option[FGLOB_OPT_TYPE])
        {
            skAppPrintErr(("Cannot use --%s when either --%s or --%s are"
                           " specified"),
                          fglobOptions[FGLOB_OPT_FLOWTYPES].name,
                          fglobOptions[FGLOB_OPT_CLASS].name,
                          fglobOptions[FGLOB_OPT_TYPE].name);
            return 1;
        }
        return fglobParseFlowtypes();
    }

    /* parse --class or use default */
    class_name = fList->fg_option[FGLOB_OPT_CLASS];
    if (NULL == class_name) {
        /* no class given, use default class and get its name */
        class_id = sksiteClassGetDefault();
        if (SK_INVALID_CLASS == class_id) {
            skAppPrintErr(("No --class given and no default class"
                           " specified in %s"),
                          sksiteGetConfigPath(path, sizeof(path)));
            return 1;
        }
        sksiteClassGetName(class_name_buf, sizeof(class_name_buf), class_id);
        class_name = class_name_buf;
    } else {
        class_id = sksiteClassLookup(class_name);
        if (SK_INVALID_CLASS == class_id) {
            if (strchr(class_name, ',')) {
                skAppPrintErr(("Invalid --%s: Use --%s to"
                               " process multiple classes"),
                              fglobOptions[FGLOB_OPT_CLASS].name,
                              fglobOptions[FGLOB_OPT_FLOWTYPES].name);
            } else if (0 == strcmp(class_name, "all")) {
                skAppPrintErr(("Invalid --%s: Use --%s to"
                               " process all classes"),
                              fglobOptions[FGLOB_OPT_CLASS].name,
                              fglobOptions[FGLOB_OPT_FLOWTYPES].name);
            } else {
                skAppPrintErr(("Invalid --%s: Cannot find class '%s'\n"
                               "\tUse the --help option to see valid classes"),
                              fglobOptions[FGLOB_OPT_CLASS].name,
                              class_name);
            }
            return 1;
        }
    }

    /* if user didn't give --type, use default types for the class */
    if (NULL == fList->fg_option[FGLOB_OPT_TYPE]) {
        sksiteClassDefaultFlowtypeIterator(class_id, &fi);
        while (sksiteFlowtypeIteratorNext(&fi, &flowtype_id)) {
            fglobAddFlowtype(flowtype_id);
        }
        if (fList->fg_flowtype_count == 0) {
            skAppPrintErr(("No --type given and no default types"
                           " specified for class %s in %s"),
                          class_name, sksiteGetConfigPath(path, sizeof(path)));
            return 1;
        }
        return 0;
    }

    /* if user said --type=all, use all types for each class */
    if (0 == strcmp(fList->fg_option[FGLOB_OPT_TYPE], "all")) {
        sksiteClassFlowtypeIterator(class_id, &fi);
        while (sksiteFlowtypeIteratorNext(&fi, &flowtype_id)) {
            fglobAddFlowtype(flowtype_id);
        }
        return 0;
    }

    /* create a modifiable version of the user's input. store a
     * pointer to it we can free */
    user_types = strdup(fList->fg_option[FGLOB_OPT_TYPE]);
    user_types_freeable = user_types;
    if (NULL == user_types) {
        skAppPrintOutOfMemory(NULL);
        return 1;
    }

    /* parse user's string as a comma separated list */
    while ((type_token = strsep(&user_types, ",")) != NULL) {
        /* check for empty token (e.g., double comma) */
        if ('\0' == *type_token) {
            continue;
        }

        flowtype_id = sksiteFlowtypeLookupByClassIDType(class_id, type_token);
        if (SK_INVALID_FLOWTYPE != flowtype_id) {
            fglobAddFlowtype(flowtype_id);
        } else {
            skAppPrintErr("Invalid %s: type '%s' not valid for class %s",
                          fglobOptions[FGLOB_OPT_TYPE].name,
                          type_token,class_name);
            rv = 1;
        }
    }

    if ((rv == 0) && (fList->fg_flowtype_count == 0)) {
        skAppPrintErr("Invalid %s: No valid types specified for class %s",
                      fglobOptions[FGLOB_OPT_TYPE].name, class_name);
        rv = 1;
    }
    if (rv == 1) {
        skAppPrintErr("Use the --help option to see valid classes and types");
    }
    if (user_types_freeable) {
        free(user_types_freeable);
    }

    return rv;
}


/*
 *  is_used = fglobValid();
 *
 *    Return 1 if fglob is to be used; 0 if it is not to be used, or
 *    -1 if there were errors with classes, types, sensors, or
 *    times.
 */
int
fglobValid(
    void)
{
    if (fList->fg_user_option_count == 0) {
        /* no fglob options given */
        return 0;
    }

    if (!fList->fg_initialized) {
        if (fglobInit()) {
            return -1;
        }
    }
    return 1;
}


/*
 *  option_count = fglobSetFilters(sensor_bitmap, file_info_bitmap);
 *
 *    This function is used when filtering a previous data pull.  It
 *    allows the --class, --type, and --sensor switches to work over
 *    this data.
 *
 *    Fill in the 'sensor_bitmap' with the values the user provided to
 *    fglob's --sensor switch as defined in the silk.conf file.
 *
 *    Fill in the 'file_info_bitmap' with the values the user provided
 *    to fglob's --class and --type switches as defined in the
 *    silk.conf file.
 */
int
fglobSetFilters(
    sk_bitmap_t       **sensor_bitmap,
    sk_bitmap_t       **flowtype_bitmap)
{
    int i;
    int rv = 0; /* no filters, no errors */

    assert(sensor_bitmap);
    assert(flowtype_bitmap);

    /* Do we have sensor info? */
    if (fList->fg_option[FGLOB_OPT_SENSORS]) {
        /* ensure site configuration file */
        if (fglobLoadSiteConfig()) {
            return -1;
        }

        if (fglobParseSensors(sensor_bitmap)) {
            return -1;
        }

        rv += 1;
    }

    if (fList->fg_option[FGLOB_OPT_CLASS]
        || fList->fg_option[FGLOB_OPT_TYPE]
        || fList->fg_option[FGLOB_OPT_FLOWTYPES])
    {
        /* ensure site configuration file */
        if (fglobLoadSiteConfig()) {
            return -1;
        }

        /* User gave us at least class/type information */
        if (fglobParseClassAndType()) {
            /* error */
            return -2;
        }

        if (skBitmapCreate(flowtype_bitmap, sksiteFlowtypeGetMaxID()+1)) {
            return -1;
        }

        for (i = 0; i < fList->fg_flowtype_count; ++i) {
            skBitmapSetBit(*flowtype_bitmap, fList->fg_flowtype_list[i]);
        }

        rv += 2;
    }

    /* Adjust the value of the 'fg_user_option_count' member */
    if (fList->fg_option[FGLOB_OPT_CLASS]) {
        --fList->fg_user_option_count;
    }
    if (fList->fg_option[FGLOB_OPT_TYPE]) {
        --fList->fg_user_option_count;
    }
    if (fList->fg_option[FGLOB_OPT_FLOWTYPES]) {
        --fList->fg_user_option_count;
    }
    if (fList->fg_option[FGLOB_OPT_SENSORS]) {
        --fList->fg_user_option_count;
    }

    return rv;
}




/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
