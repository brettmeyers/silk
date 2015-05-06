/*
** Copyright (C) 2003-2015 by Carnegie Mellon University.
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
**  rwfileinfo
**
**  Prints information about an rw-file:
**    file type
**    file version
**    byte order
**    compression level
**    header size
**    record size
**    record count
**    file size
**    command line args used to generate file
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwfileinfo.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/sksite.h>
#include <silk/utils.h>
#include <silk/skstream.h>
#include <silk/skstringmap.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* when determining number of records in file, number of bytes to
 * request at one time */
#define RWINFO_BLOCK_SIZE 0x40000

/* format for a label */
#define LABEL_FMT "  %-20s"

/* format for a label that is a number, such as when printing command lines */
#define LABEL_NUM_FMT "%20d  "

/* A list of the properties that can be printed; keep in sync with the
 * info_props[] array below */
enum info_fileinfo_id {
    RWINFO_FORMAT,
    RWINFO_VERSION,
    RWINFO_BYTE_ORDER,
    RWINFO_COMPRESSION,
    RWINFO_HEADER_LENGTH,
    RWINFO_RECORD_LENGTH,
    RWINFO_COUNT_RECORDS,
    RWINFO_FILE_SIZE,
    RWINFO_COMMAND_LINES,
    RWINFO_RECORD_VERSION,
    RWINFO_SILK_VERSION,
    RWINFO_PACKED_FILE_INFO,
    RWINFO_PROBE_NAME,
    RWINFO_ANNOTIONS,
    RWINFO_PREFIX_MAP,
    RWINFO_IPSET,
    RWINFO_BAG,
    /* Last item is used to get a count of the above; it must be last */
    RWINFO_PROPERTY_COUNT
};

/* A property */
typedef struct info_property_st {
    /* property's label */
    const char   *label;
    /* nonzero if printed */
    unsigned      will_print :1;
} info_property_t;


/* LOCAL VARIABLES */

/* keep in sync with the info_fileinfo_id enum above */
static info_property_t info_props[RWINFO_PROPERTY_COUNT] = {
    {"format(id)",          1},
    {"version",             1},
    {"byte-order",          1},
    {"compression(id)",     1},
    {"header-length",       1},
    {"record-length",       1},
    {"count-records",       1},
    {"file-size",           1},
    {"command-lines",       1},
    {"record-version",      1},
    {"silk-version",        1},
    {"packed-file-info",    1},
    {"probe-name",          1},
    {"annotations",         1},
    {"prefix-map",          1},
    {"ipset",               1},
    {"bag",                 1}
};

/* whether to print the summary */
static int print_summary = 0;

/* whether to not print titles (0==print titles, 1==no titles) */
static int no_titles = 0;

/* for looping over files on the command line */
static sk_options_ctx_t *optctx = NULL;


/* OPTIONS SETUP */

/* Create constants for the option processor */
typedef enum rwinfoOptionIds {
    OPT_FIELDS,
    OPT_SUMMARY,
    OPT_NO_TITLES
} appOptionsEnum;

static struct option appOptions[] = {
    {"fields",          REQUIRED_ARG, 0, OPT_FIELDS},
    {"summary",         NO_ARG,       0, OPT_SUMMARY},
    {"no-titles",       NO_ARG,       0, OPT_NO_TITLES},
    {0,0,0,0}           /* sentinel entry */
};

static const char *appHelp[] = {
    NULL, /* built dynamically */
    "Print a summary of total files, file sizes, and records",
    ("Do not print file names or field names; only print the\n"
     "\tvalues, one per line"),
    (char *)NULL /* sentinel entry */
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static int  parseFields(const char *field_str);
static int
printFileInfo(
    const char         *path,
    int64_t            *recs,
    int64_t            *bytes);


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
#define USAGE_MSG                                                       \
    ("[SWITCHES] <FILES>\n"                                             \
     "\tPrint information (type, version, etc.) about a SiLK Flow,\n"   \
     "\tIPset, or Bag file.  Use the fields switch to control what\n"   \
     "\tinformation is printed.\n")

    FILE *fh = USAGE_FH;
    int i, j;

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);
    for (i = 0; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch (appOptions[i].val) {
          case OPT_FIELDS:
            fprintf(fh,
                    ("List of fields to print; specify by name or by value.\n"
                     "\tDef. All fields.  Available fields:"));
            for (j = 0; j < RWINFO_PROPERTY_COUNT; ++j) {
                fprintf(fh, "\n\t  %3d %s", 1+j, info_props[j].label);
            }
            break;

          default:
            fprintf(fh, "%s", appHelp[i]);
            break;
        }
        fprintf(fh, "\n");
    }
    skOptionsCtxOptionsUsage(optctx, fh);
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

    skOptionsCtxDestroy(&optctx);
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
    int optctx_flags;
    int rv;

    /* verify same number of options and help strings */
    assert((sizeof(appHelp)/sizeof(char *)) ==
           (sizeof(appOptions)/sizeof(struct option)));

    /* register the application */
    skAppRegister(argv[0]);
    skAppVerifyFeatures(&features, NULL);
    skOptionsSetUsageCallback(&appUsageLong);

    optctx_flags = (SK_OPTIONS_CTX_INPUT_BINARY);

    /* register the options */
    if (skOptionsCtxCreate(&optctx, optctx_flags)
        || skOptionsCtxOptionsRegister(optctx)
        || skOptionsRegister(appOptions, &appOptionsHandler, NULL)
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

    /* parse options */
    rv = skOptionsCtxOptionsParse(optctx, argc, argv);
    if (rv < 0) {
        skAppUsage();  /* never returns */
    }

    /* try to load the site file to resolve sensor information */
    sksiteConfigure(0);

    return;  /* OK */
}


/*
 *  status = appOptionsHandler(cData, opt_index, opt_arg);
 *
 *    Called by skOptionsParse(), this handles a user-specified switch
 *    that the application has registered, typically by setting global
 *    variables.  Returns 1 if the switch processing failed or 0 if it
 *    succeeded.  Returning a non-zero from from the handler causes
 *    skOptionsParse() to return a negative value.
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
      case OPT_FIELDS:
        if (parseFields(opt_arg)) {
            return 1;
        }
        break;

      case OPT_SUMMARY:
        print_summary = 1;
        break;

      case OPT_NO_TITLES:
        no_titles = 1;
        break;
    }

    return 0;
}


/*
 *  ok = parseFields(field_list);
 *
 *    Parse the user's field list, setting the appropriate flags in
 *    the info_props[] vector.
 */
static int
parseFields(
    const char         *field_str)
{
    sk_stringmap_t *str_map = NULL;
    sk_stringmap_status_t rv_map;
    sk_stringmap_entry_t *map_entry;
    sk_stringmap_entry_t insert_entry[2];
    sk_stringmap_iter_t *iter = NULL;
    char *fieldnum_string = NULL;
    char *cp;
    size_t len;
    int sz;
    uint32_t i;
    int rv = -1;

    /* create a char array to hold the field values as strings */
    len = 11 * RWINFO_PROPERTY_COUNT;
    fieldnum_string = (char*)calloc(len, sizeof(char));
    if (fieldnum_string == NULL) {
        skAppPrintErr("Unable to create string");
        goto END;
    }

    /* create a stringmap of the available entries */
    if (SKSTRINGMAP_OK != skStringMapCreate(&str_map)) {
        skAppPrintErr("Unable to create stringmap");
        goto END;
    }

    cp = fieldnum_string;
    for (i = 0; i < RWINFO_PROPERTY_COUNT; ++i) {
        sz = snprintf(cp, len, ("%" PRIu32), (uint32_t)(i+1));
        if ((size_t)sz > len) {
            skAppPrintErr("Internal buffer too small");
            skAbort();
        }
        memset(insert_entry, 0, sizeof(insert_entry));
        insert_entry[0].name = info_props[i].label;
        insert_entry[0].id = i;
        insert_entry[1].name = cp;
        insert_entry[1].id = i;
        if (skStringMapAddEntries(str_map, 2, insert_entry)
            != SKSTRINGMAP_OK)
        {
            goto END;
        }
        cp += sz + 1;
        len -= sz + 1;
    }

    /* attempt to match */
    rv_map = skStringMapParse(str_map, field_str, SKSTRINGMAP_DUPES_KEEP,
                              &iter, &cp);
    if (rv_map) {
        skAppPrintErr("Invalid %s: %s", appOptions[OPT_FIELDS].name, cp);
        goto END;
    }

    /* turn off printing for all fields */
    for (i = 0; i < RWINFO_PROPERTY_COUNT; ++i) {
        info_props[i].will_print = 0;
    }

    /* enable fields user listed */
    while (skStringMapIterNext(iter, &map_entry, NULL) == SK_ITERATOR_OK) {
        info_props[map_entry->id].will_print = 1;
    }

    rv = 0;

  END:
    if (str_map) {
        skStringMapDestroy(str_map);
    }
    if (iter) {
        skStringMapIterDestroy(iter);
    }
    if (fieldnum_string) {
        free(fieldnum_string);
    }
    return rv;
}


/*
 *  status = getNumberRecs(stream, record_size, &count);
 *
 *    Given 'stream' to the opened file, read the file to determine
 *    the number of 'record_size' records in the file, and set 'count'
 *    to that value.  If the file was successfully read, return 0.  If
 *    an error occurs while reading the file, return -1.
 */
static int
getNumberRecs(
    skstream_t         *stream,
    size_t              rec_size,
    int64_t            *count)
{
    size_t block_size = RWINFO_BLOCK_SIZE;
    int64_t bytes = 0;
    ssize_t saw;
    imaxdiv_t rec;
    int rv = 0;

    if (0 == rec_size) {
        rec_size = 1;
    }

    /* modify block_size to read an integer number of records */
    if (rec_size > block_size) {
        block_size = rec_size;
    } else {
        block_size -= block_size % rec_size;
    }

    /* get number of bytes in file */
    while ((saw = skStreamRead(stream, NULL, block_size)) > 0) {
        bytes += saw;
    }
    if (saw != 0) {
        skStreamPrintLastErr(stream, saw, &skAppPrintErr);
        rv = -1;
    }

    /* compute number of records */
    rec = imaxdiv(bytes, rec_size);
    if (rec.rem != 0) {
        skAppPrintErr("Short read (%" PRIdMAX "/%lu)",
                      rec.rem, (unsigned long)rec_size);
        rv = -1;
    }
    *count = (int64_t)rec.quot;
    return rv;
}


/*
 *  status = printFileInfo(info, &total_recs, &total_bytes);
 *
 *    Given the file information in the 'info' structure, print the
 *    fields requested by the user---given in the 'info_props'
 *    global---to the standard output.  Update the values pointed at
 *    by 'total_recs' and 'total_bytes' with the number of records and
 *    bytes in this file.  Return -1 if there is a problem opening or
 *    reading the file.  Return 0 otherwise.
 */
static int
printFileInfo(
    const char         *path,
    int64_t            *recs,
    int64_t            *bytes)
{
    char buf[1024];
    int count;
    int64_t rec_count;
    skstream_t *stream = NULL;
    sk_file_header_t *hdr;
    sk_header_entry_t *he;
    sk_hentry_iterator_t iter;
    int rv = SKSTREAM_OK;
    int retval = 0;

    if (SKSTREAM_OK == rv) {
        rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK);
    }
    if (SKSTREAM_OK == rv) {
        rv = skStreamBind(stream, path);
    }
    if (SKSTREAM_OK == rv) {
        rv = skStreamOpen(stream);
    }
    if (SKSTREAM_OK == rv) {
        rv = skStreamReadSilkHeaderStart(stream);
    }

    /* Give up if we can't read the beginning of the silk header */
    if (rv != SKSTREAM_OK) {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        skStreamDestroy(&stream);
        return -1;
    }

    /* print file name */
    if (!no_titles) {
        printf("%s:\n", path);
    }

    /* read the header */
    rv = skStreamReadSilkHeader(stream, &hdr);
    switch (rv) {
      case SKSTREAM_OK:
        break;
      case SKHEADER_ERR_LEGACY:
        /* unrecognized file format.  disable printing of record
         * version and record size */
        info_props[RWINFO_HEADER_LENGTH].will_print = 0;
        info_props[RWINFO_RECORD_LENGTH].will_print = 0;
        info_props[RWINFO_RECORD_VERSION].will_print = 0;
        info_props[RWINFO_SILK_VERSION].will_print = 0;
        info_props[RWINFO_COUNT_RECORDS].will_print = 0;
        break;
      default:
        /* print an error but continue */
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        retval = -1;
        break;
    }

    if (info_props[RWINFO_FORMAT].will_print) {
        sksiteFileformatGetName(buf, sizeof(buf),
                                skHeaderGetFileFormat(hdr));
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_FORMAT].label);
        }
        printf("%s(0x%02x)\n", buf, skHeaderGetFileFormat(hdr));
    }

    if (info_props[RWINFO_VERSION].will_print) {
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_VERSION].label);
        }
        printf("%u\n", skHeaderGetFileVersion(hdr));
    }

    if (info_props[RWINFO_BYTE_ORDER].will_print) {
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_BYTE_ORDER].label);
        }
        printf("%s\n", ((skHeaderGetByteOrder(hdr) == SILK_ENDIAN_BIG)
                        ? "BigEndian"
                        : "littleEndian"));
    }

    if (info_props[RWINFO_COMPRESSION].will_print) {
        sksiteCompmethodGetName(buf, sizeof(buf),
                                skHeaderGetCompressionMethod(hdr));
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_COMPRESSION].label);
        }
        printf("%s(%u)\n", buf, skHeaderGetCompressionMethod(hdr));
    }

    if (info_props[RWINFO_HEADER_LENGTH].will_print) {
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_HEADER_LENGTH].label);
        }
        printf("%u\n", (unsigned int)skHeaderGetLength(hdr));
    }

    if (info_props[RWINFO_RECORD_LENGTH].will_print) {
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_RECORD_LENGTH].label);
        }
        printf("%u\n", (unsigned int)skHeaderGetRecordLength(hdr));
    }

    if (info_props[RWINFO_RECORD_VERSION].will_print) {
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_RECORD_VERSION].label);
        }
        printf("%d\n", skHeaderGetRecordVersion(hdr));
    }

    if (info_props[RWINFO_SILK_VERSION].will_print) {
        uint32_t vers = skHeaderGetSilkVersion(hdr);
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_SILK_VERSION].label);
        }
        if (vers == 0) {
            printf("0\n");
        } else {
            printf(("%" PRId32 ".%" PRId32 ".%" PRId32 "\n"),
                   (vers / 1000000), (vers / 1000 % 1000), (vers % 1000));
        }
    }

    if (info_props[RWINFO_COUNT_RECORDS].will_print) {
        rv = getNumberRecs(stream, skHeaderGetRecordLength(hdr), &rec_count);
        if (rv) {
            retval = -1;
        }
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_COUNT_RECORDS].label);
        }
        printf(("%" PRId64 "\n"), rec_count);
        *recs += rec_count;
    }

    if (info_props[RWINFO_FILE_SIZE].will_print) {
        int64_t sz = (int64_t)skFileSize(path);
        if (!no_titles) {
            printf(LABEL_FMT, info_props[RWINFO_FILE_SIZE].label);
        }
        printf(("%" PRId64 "\n"), sz);
        *bytes += sz;
    }

    if (info_props[RWINFO_PACKED_FILE_INFO].will_print) {
        count = 0;
        skHeaderIteratorBindType(&iter, hdr, SK_HENTRY_PACKEDFILE_ID);
        while ((he = skHeaderIteratorNext(&iter)) != NULL) {
            if (!no_titles) {
                if (count == 0) {
                    printf(LABEL_FMT,
                           info_props[RWINFO_PACKED_FILE_INFO].label);
                } else {
                    printf(LABEL_FMT, "");
                }
            }
            ++count;
            skHentryPackedfilePrint(he, stdout);
            printf("\n");
        }
    }

    if (info_props[RWINFO_PROBE_NAME].will_print) {
        count = 0;
        skHeaderIteratorBindType(&iter, hdr, SK_HENTRY_PROBENAME_ID);
        while ((he = skHeaderIteratorNext(&iter)) != NULL) {
            if (!no_titles) {
                if (count == 0) {
                    printf(LABEL_FMT, info_props[RWINFO_PROBE_NAME].label);
                } else {
                    printf(LABEL_FMT, "");
                }
            }
            ++count;
            skHentryProbenamePrint(he, stdout);
            printf("\n");
        }
    }

    if (info_props[RWINFO_PREFIX_MAP].will_print) {
        count = 0;
        skHeaderIteratorBindType(&iter, hdr, SK_HENTRY_PREFIXMAP_ID);
        while ((he = skHeaderIteratorNext(&iter)) != NULL) {
            if (!no_titles) {
                if (count == 0) {
                    printf(LABEL_FMT, info_props[RWINFO_PREFIX_MAP].label);
                } else {
                    printf(LABEL_FMT, "");
                }
            }
            ++count;
            skHentryPrefixmapPrint(he, stdout);
            printf("\n");
        }
    }

    if (info_props[RWINFO_IPSET].will_print) {
        count = 0;
        skHeaderIteratorBindType(&iter, hdr, SK_HENTRY_IPSET_ID);
        while ((he = skHeaderIteratorNext(&iter)) != NULL) {
            if (!no_titles) {
                if (count == 0) {
                    printf(LABEL_FMT, info_props[RWINFO_IPSET].label);
                } else {
                    printf(LABEL_FMT, "");
                }
            }
            ++count;
            skHentryIPSetPrint(he, stdout);
            printf("\n");
        }
    }

    if (info_props[RWINFO_BAG].will_print) {
        count = 0;
        skHeaderIteratorBindType(&iter, hdr, SK_HENTRY_BAG_ID);
        while ((he = skHeaderIteratorNext(&iter)) != NULL) {
            if (!no_titles) {
                if (count == 0) {
                    printf(LABEL_FMT, info_props[RWINFO_BAG].label);
                } else {
                    printf(LABEL_FMT, "");
                }
            }
            ++count;
            skHentryBagPrint(he, stdout);
            printf("\n");
        }
    }

    if (info_props[RWINFO_COMMAND_LINES].will_print) {
        count = 0;
        skHeaderIteratorBindType(&iter, hdr, SK_HENTRY_INVOCATION_ID);
        while ((he = skHeaderIteratorNext(&iter)) != NULL) {
            if (count == 0 && !no_titles) {
                printf((LABEL_FMT "\n"),
                       info_props[RWINFO_COMMAND_LINES].label);
            }
            ++count;
            if (!no_titles) {
                printf(LABEL_NUM_FMT, count);
            }
            skHentryInvocationPrint(he, stdout);
            printf("\n");
        }
    }

    if (info_props[RWINFO_ANNOTIONS].will_print) {
        count = 0;
        skHeaderIteratorBindType(&iter, hdr, SK_HENTRY_ANNOTATION_ID);
        while ((he = skHeaderIteratorNext(&iter)) != NULL) {
            if (count == 0 && !no_titles) {
                printf((LABEL_FMT "\n"), info_props[RWINFO_ANNOTIONS].label);
            }
            ++count;
            if (!no_titles) {
                printf(LABEL_NUM_FMT, count);
            }
            skHentryAnnotationPrint(he, stdout);
            printf("\n");
        }
    }

    skStreamDestroy(&stream);
    return retval;
}


/*
 *  For each file, get the file's info then print it
 */
int main(int argc, char **argv)
{
    int64_t total_files = 0;
    int64_t total_bytes = 0;
    int64_t total_recs = 0;
    int rv = EXIT_SUCCESS;
    char *path;

    appSetup(argc, argv);       /* never returns on error */

    while (skOptionsCtxNextArgument(optctx, &path) == 0) {
        if (printFileInfo(path, &total_recs, &total_bytes)) {
            rv = EXIT_FAILURE;
        }
        ++total_files;
    }

    if (print_summary) {
        if (!no_titles) {
            printf("**SUMMARY**:\n");
            printf(LABEL_FMT, "number-files");
        }
            printf(("%" PRId64 "\n"), total_files);
        if (info_props[RWINFO_COUNT_RECORDS].will_print) {
            if (!no_titles) {
                printf(LABEL_FMT, "total-records");
            }
            printf(("%" PRId64 "\n"), total_recs);
        }
        if (info_props[RWINFO_FILE_SIZE].will_print) {
            if (!no_titles) {
                printf(LABEL_FMT, "all-file-sizes");
            }
            printf(("%" PRId64 "\n"), total_bytes);
        }
    }

    /* done */
    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
