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
** rwbagbuild can read an IP set and generate a bag with a default
** count for each IP address, or it can read a pipe-separated text
** file representing a bag.
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwbagbuild.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skbag.h>
#include <silk/skipaddr.h>
#include <silk/skipset.h>
#include <silk/sksite.h>
#include <silk/skstream.h>
#include <silk/skstringmap.h>
#include <silk/utils.h>


/* LOCAL DEFINES AND TYPEDEFS */

/* where to write --help output */
#define USAGE_FH stdout

/* What to do when malloc() fails */
#define EXIT_NO_MEMORY                                               \
    do {                                                             \
        skAppPrintOutOfMemory(NULL);                                 \
        exit(EXIT_FAILURE);                                          \
    } while(0)


/* LOCAL VARIABLES */

/* output stream */
static skstream_t *out_stream = NULL;

/* the compression method to use when writing the file.
 * sksiteCompmethodOptionsRegister() will set this to the default or
 * to the value the user specifies. */
static sk_compmethod_t comp_method;

/* input streams (for reading a textual bag or an ip set */
static skstream_t *bag_input = NULL;
static skstream_t *set_input = NULL;

/* counter */
static int f_use_default_count = 0;
static uint64_t default_count = 1;

/* delimiter for text input */
static char delimiter = '|';

/* key and counter type */
static skBagFieldType_t key_type = SKBAG_FIELD_CUSTOM;
static skBagFieldType_t counter_type = SKBAG_FIELD_CUSTOM;

/* string map of field types */
static sk_stringmap_t *field_map;


/* OPTIONS SETUP */

typedef enum {
    OPT_SET_INPUT,
    OPT_BAG_INPUT,
    OPT_DELIMITER,
    OPT_DEFAULT_COUNT,
    OPT_KEY_TYPE,
    OPT_COUNTER_TYPE,
    OPT_OUTPUT_PATH
} appOptionsEnum;

static struct option appOptions[] = {
    {"set-input",     REQUIRED_ARG, 0, OPT_SET_INPUT},
    {"bag-input",     REQUIRED_ARG, 0, OPT_BAG_INPUT},
    {"delimiter",     REQUIRED_ARG, 0, OPT_DELIMITER},
    {"default-count", REQUIRED_ARG, 0, OPT_DEFAULT_COUNT},
    {"key-type",      REQUIRED_ARG, 0, OPT_KEY_TYPE},
    {"counter-type",  REQUIRED_ARG, 0, OPT_COUNTER_TYPE},
    {"output-path",   REQUIRED_ARG, 0, OPT_OUTPUT_PATH},
    {0,0,0,0} /* sentinel entry */
};


static const char *appHelp[] = {
    "Create a bag from the specified IP set.",
    "Create a bag from a delimiter-separated text file.",
    ("Specify the delimiter separating the key and value\n"
     "\tfor the --bag-input switch. Def. '|'"),
    ("Set default count for each key in the new bag.\n"
     "\tDef. 1"),
    ("Set the key type to this value."),
    ("Set the counter type to this value."),
    ("Specify destination for the new bag. Def. stdout"),
    (char *)NULL
};


/* LOCAL FUNCTION PROTOTYPES */

static int  appOptionsHandler(clientData cData, int opt_index, char *opt_arg);
static void createFieldTypeStringMap(void);
static int
parseFieldType(
    const char         *string,
    int                 opt_index,
    skBagFieldType_t   *field_type);
static int
createBagFromTextBag(
    skBag_t            *bag,
    skstream_t         *stream);
static int
createBagFromSet(
    skBag_t            *bag,
    skstream_t         *stream);


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
#define USAGE_MSG                                                           \
    ("{--set-input=FILE | --bag-input=FILE} [SWITCHES]\n"                   \
     "\tCreate a binary Bag file from either a binary IPset file or from\n" \
     "\ta textual input file.  Use \"stdin\" as the file name to read\n"    \
     "\tfrom the standard input.  The Bag is written to the standard\n"     \
     "\toutput or the location specified with the --output-path switch.\n")

    FILE *fh = USAGE_FH;
    const char *default_type = NULL;
    int i;

    createFieldTypeStringMap();
    if (field_map) {
        default_type = skStringMapGetFirstName(field_map, SKBAG_FIELD_CUSTOM);
    }
    if (!default_type) {
        default_type = "<ERROR>";
    }

    fprintf(fh, "%s %s", skAppName(), USAGE_MSG);
    fprintf(fh, "\nSWITCHES:\n");
    skOptionsDefaultUsage(fh);

    for (i = 0; appOptions[i].name; ++i) {
        fprintf(fh, "--%s %s. ", appOptions[i].name,
                SK_OPTION_HAS_ARG(appOptions[i]));
        switch (appOptions[i].val) {
          case OPT_KEY_TYPE:
            fprintf(fh, "%s Def. '%s'. Choices:\n",
                    appHelp[i], default_type);
            skStringMapPrintUsage(field_map, fh, 8);
            break;
          case OPT_COUNTER_TYPE:
            fprintf(fh, ("%s Def. '%s'.\n"
                         "\tChoices are the same as for the key-type\n"),
                    appHelp[i], default_type);
            break;
          default:
            fprintf(fh, "%s\n", appHelp[i]);
            break;
        }
    }

    skOptionsNotesUsage(fh);
    sksiteCompmethodOptionsUsage(fh);
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
    int rv;

    if (teardownFlag) {
        return;
    }
    teardownFlag = 1;

    if (field_map) {
        skStringMapDestroy(field_map);
        field_map = NULL;
    }

    /*
     * close pipes/files
     */
    if (out_stream) {
        rv = skStreamClose(out_stream);
        if (rv && rv != SKSTREAM_ERR_NOT_OPEN) {
            skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        }
        skStreamDestroy(&out_stream);
    }

    skStreamDestroy(&bag_input);
    skStreamDestroy(&set_input);

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

    /* create output stream */
    if (skStreamCreate(&out_stream, SK_IO_WRITE, SK_CONTENT_SILK)) {
        skAppPrintErr("Cannot create output stream");
        exit(EXIT_FAILURE);
    }

    /* register the options */
    if (skOptionsRegister(appOptions, &appOptionsHandler, NULL)
        || skOptionsNotesRegister(NULL)
        || sksiteCompmethodOptionsRegister(&comp_method))
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
    arg_index = skOptionsParse(argc, argv);
    if (arg_index < 0) {
        /* options parsing should print error */
        skAppUsage(); /* never returns */
    }

    /* check for input */
    if ( !set_input && !bag_input) {
        skAppPrintErr("Either --%s or --%s switch is required",
                      appOptions[OPT_SET_INPUT].name,
                      appOptions[OPT_BAG_INPUT].name);
        skAppUsage(); /* never returns */
    }

    /* Complain about extra args on command line */
    if (arg_index != argc) {
        skAppPrintErr("Too many or unrecognized argument specified: '%s'",
                      argv[arg_index]);
        exit(EXIT_FAILURE);
    }

    /* If output was never set, bind it to stdout */
    if (NULL == skStreamGetPathname(out_stream)) {
        rv = skStreamBind(out_stream, "stdout");
        if (rv) {
            skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
            exit(EXIT_FAILURE);
        }
    }

    /* Open output */
    if ((rv = skStreamSetCompressionMethod(out_stream, comp_method))
        || (rv = skStreamOpen(out_stream)))
    {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        exit(EXIT_FAILURE);
    }

    /* add notes if given */
    rv = skOptionsNotesAddToStream(out_stream);
    if (rv) {
        skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
        exit(EXIT_FAILURE);
    }
    skOptionsNotesTeardown();

    return;                     /* OK */
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
    int rv;

    switch ((appOptionsEnum)opt_index) {
      case OPT_SET_INPUT:
        if (set_input) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if (bag_input) {
            skAppPrintErr("May only specify one of --%s or --%s",
                          appOptions[OPT_SET_INPUT].name,
                          appOptions[OPT_BAG_INPUT].name);
            return 1;
        }
        if ((rv = skStreamCreate(&set_input, SK_IO_READ, SK_CONTENT_SILK))
            || (rv = skStreamBind(set_input, opt_arg))
            || (rv = skStreamOpen(set_input)))
        {
            skStreamPrintLastErr(set_input, rv, &skAppPrintErr);
            skStreamDestroy(&set_input);
            return 1;
        }
        break;

      case OPT_BAG_INPUT:
        if (bag_input) {
            skAppPrintErr("Invalid %s: Switch used multiple times",
                          appOptions[opt_index].name);
            return 1;
        }
        if (set_input) {
            skAppPrintErr("May only specify one of --%s or --%s",
                          appOptions[OPT_SET_INPUT].name,
                          appOptions[OPT_BAG_INPUT].name);
            return 1;
        }
        if ((rv = skStreamCreate(&bag_input, SK_IO_READ, SK_CONTENT_TEXT))
            || (rv = skStreamBind(bag_input, opt_arg))
            || (rv = skStreamOpen(bag_input)))
        {
            skStreamPrintLastErr(bag_input, rv, &skAppPrintErr);
            skStreamDestroy(&bag_input);
            return 1;
        }
        break;

      case OPT_OUTPUT_PATH:
        rv = skStreamBind(out_stream, opt_arg);
        if (rv) {
            skStreamPrintLastErr(out_stream, rv, &skAppPrintErr);
            return 1;
        }
        break;

      case OPT_DEFAULT_COUNT:
        rv = skStringParseUint64((uint64_t*)&default_count, opt_arg, 0, 0);
        if (rv) {
            skAppPrintErr("Invalid %s '%s': %s",
                          appOptions[opt_index].name, opt_arg,
                          skStringParseStrerror(rv));
            return 1;
        }
        f_use_default_count = 1;
        break;

      case OPT_DELIMITER:
        delimiter = opt_arg[0];
        if (delimiter == '#') {
            skAppPrintErr("The character '#' is the comment character;\n"
                          "\tit cannot be used as the delimiter.");
            return 1;
        }
        if (delimiter == '\n') {
            skAppPrintErr("Newline is not a valid delimiter.");
            return 1;
        }
        break;

      case OPT_KEY_TYPE:
        if (parseFieldType(opt_arg, opt_index, &key_type)) {
            return 1;
        }
        break;

      case OPT_COUNTER_TYPE:
        if (parseFieldType(opt_arg, opt_index, &counter_type)) {
            return 1;
        }
        break;
    }

    return 0; /* OK */
}


static void
createFieldTypeStringMap(
    void)
{
    skBagFieldType_t field_type;
    skBagFieldTypeIterator_t iter;
    sk_stringmap_entry_t sm_entry;
    sk_stringmap_status_t sm_err;
    char field_name[SKBAG_MAX_FIELD_BUFLEN];

    if (field_map) {
        return;
    }

    memset(&sm_entry, 0, sizeof(sm_entry));

    /* create a stringmap of the available resolvers */
    sm_err = skStringMapCreate(&field_map);
    if (SKSTRINGMAP_OK != sm_err) {
        skAppPrintErr("Unable to create string map: %s",
                      skStringMapStrerror(sm_err));
        return;
    }

    skBagFieldTypeIteratorBind(&iter);
    while (skBagFieldTypeIteratorNext(&iter, &field_type, NULL,
                                      field_name, sizeof(field_name))
           == SKBAG_OK)
    {
        sm_entry.id = field_type;
        sm_entry.name = field_name;
        sm_err = skStringMapAddEntries(field_map, 1, &sm_entry);
        if (SKSTRINGMAP_OK != sm_err) {
            skAppPrintErr("Unable to add string map entry: %s",
                          skStringMapStrerror(sm_err));
            skStringMapDestroy(field_map);
            field_map = NULL;
            return;
        }
    }
}


static int
parseFieldType(
    const char         *string,
    int                 opt_index,
    skBagFieldType_t   *field_type)
{
    sk_stringmap_status_t sm_err;
    sk_stringmap_entry_t *sm_entry;

    createFieldTypeStringMap();

    /* attempt to match */
    sm_err = skStringMapGetByName(field_map, string, &sm_entry);
    switch (sm_err) {
      case SKSTRINGMAP_OK:
        *field_type = (skBagFieldType_t)sm_entry->id;
        return 0;

      case SKSTRINGMAP_PARSE_AMBIGUOUS:
        skAppPrintErr("Invalid %s: Field '%s' is ambiguous",
                      appOptions[opt_index].name, string);
        break;

      case SKSTRINGMAP_PARSE_NO_MATCH:
        skAppPrintErr("Invalid %s: Field '%s' is not recognized",
                      appOptions[opt_index].name, string);
        break;

      default:
        skAppPrintErr("Unexpected return value from string-map parser (%d)",
                      sm_err);
        break;
    }

    return -1;
}


static int
createBagFromTextBag(
    skBag_t            *bag,
    skstream_t         *stream)
{
#if SK_ENABLE_IPV6
    /* types of keys seen in the input */
    struct key_types {
        unsigned    num    :1;
        unsigned    ipv4   :1;
        unsigned    ipv6   :1;
    } key_types;
    skBagTypedKey_t key;
#endif  /* SK_ENABLE_IPV6 */
    skBagTypedKey_t ipkey;
    skBagTypedCounter_t counter;
    char *sz_key;
    char *sz_counter;
    skIPWildcardIterator_t iter;
    skIPWildcard_t ipwild;
    char line[128];
    int lc = 0;
    skBagErr_t err;
    int rv;

#if SK_ENABLE_IPV6
    /* initialize types of keys */
    memset(&key_types, 0, sizeof(key_types));

    /* set the types for the key and counter once */
    key.type = SKBAG_KEY_U32;
#endif
    ipkey.type = SKBAG_KEY_IPADDR;
    counter.type = SKBAG_COUNTER_U64;

    /* set counter to the default */
    counter.val.u64 = default_count;

    if (skStreamSetCommentStart(stream, "#")) {
        return 1;
    }

    /* read until end of file */
    while ((rv = skStreamGetLine(stream, line, sizeof(line), &lc))
           != SKSTREAM_ERR_EOF)
    {
        switch (rv) {
          case SKSTREAM_OK:
            /* good, we got our line */
            break;
          case SKSTREAM_ERR_LONG_LINE:
            /* bad: line was longer than sizeof(line_buf) */
            skAppPrintErr("Input line %d too long. ignored",
                          lc);
            continue;
          default:
            /* unexpected error */
            skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            return 1;
        }

        /* search for pipe, and break line into key and counter */
        sz_key = line;
        sz_counter = strchr(line, delimiter);
        if (sz_counter) {
            /* change pipe into NULL char, to terminate first string */
            *sz_counter = '\0';
        }

        if (f_use_default_count == 1) {
            /* already set to the default */
        } else if (sz_counter == NULL) {
            /* not a pipe delimited line; use default count */
            counter.val.u64 = default_count;
        } else {
            /* move counter to point to next char (start of counter) */
            sz_counter++;

            rv = skStringParseUint64(&counter.val.u64, sz_counter, 0, 0);
            if (rv < 0) {
                /* parse error */
                skAppPrintErr("Error parsing count on line %d: %s",
                              lc, skStringParseStrerror(rv));
                return 1;
            }

            if ((rv > 0) && (sz_counter[rv] != delimiter)) {
                /* unrecognized stuff after count */
                skAppPrintErr("Error parsing line %d: Extra text after count",
                              lc);
                return 1;
            }

            /* ignore trailing delimiter and everything after it */
        }

        /* parse key section of bag line */

#if !SK_ENABLE_IPV6
        /* parse as an integer, an IP, a CIDR block, or an IP wildcard */
        rv = skStringParseIPWildcard(&ipwild, sz_key);
        if (rv != 0) {
            /* not parsable */
            skAppPrintErr("Error parsing IP on line %d: %s",
                          lc, skStringParseStrerror(rv));
            return 1;
        }
        /* Add IPs from wildcard to the bag */
        skIPWildcardIteratorBind(&iter, &ipwild);
        while (skIPWildcardIteratorNext(&iter, &ipkey.val.addr)
               == SK_ITERATOR_OK)
        {
            err = skBagCounterAdd(bag, &ipkey, &counter, NULL);
            if (err != SKBAG_OK) {
                skAppPrintErr("Error adding value to bag: %s",
                              skBagStrerror(err));
                return 1;
            }
        }

#else  /* SK_ENABLE_IPV6 */

        /* do not allow a mix of integer keys with IPv6 addresses */

        /* first, attempt to parse as a number */
        rv = skStringParseUint32(&key.val.u32, sz_key,
                                 SKBAG_KEY_MIN, SKBAG_KEY_MAX);
        if (0 == rv) {
            if (key_types.ipv6) {
                skAppPrintErr(("Error on line %d:"
                               " May not mix integer keys with IPv6 keys"),
                              lc);
                return 1;
            }
            key_types.num = 1;
            err = skBagCounterAdd(bag, &key, &counter, NULL);
            if (err != SKBAG_OK) {
                skAppPrintErr("Error adding value to bag: %s",
                              skBagStrerror(err));
                return 1;
            }
        } else {
            /* parse as an IP, a CIDR block, or an IP wildcard */
            rv = skStringParseIPWildcard(&ipwild, sz_key);
            if (rv != 0) {
                /* not parsable */
                skAppPrintErr("Error parsing IP on line %d: %s",
                              lc, skStringParseStrerror(rv));
                return 1;
            }
            if (skIPWildcardIsV6(&ipwild)) {
                if (key_types.num) {
                    skAppPrintErr(("Error on line %d:"
                                   " May not mix integer keys with IPv6 keys"),
                                  lc);
                    return 1;
                }
                key_types.ipv6 = 1;
            } else {
                key_types.ipv4 = 1;
            }

            /* Add IPs from wildcard to the bag */
            skIPWildcardIteratorBind(&iter, &ipwild);
            while (skIPWildcardIteratorNext(&iter, &ipkey.val.addr)
                   == SK_ITERATOR_OK)
            {
                err = skBagCounterAdd(bag, &ipkey, &counter, NULL);
                if (err != SKBAG_OK) {
                    skAppPrintErr("Error adding value to bag: %s",
                                  skBagStrerror(err));
                    return 1;
                }
            }
        }

#endif  /* #else of #if !SK_ENABLE_IPV6 */

    }

    return 0;
}


static int
bagFromSetCallback(
    skipaddr_t         *ipaddr,
    uint32_t     UNUSED(prefix),
    void               *v_bag)
{
    skBagTypedKey_t k;
    skBagTypedCounter_t c;

    k.type = SKBAG_KEY_IPADDR;
    skipaddrCopy(&k.val.addr, ipaddr);
    c.type = SKBAG_COUNTER_U64;
    c.val.u64 = default_count;

    return skBagCounterSet((skBag_t*)v_bag, &k, &c);
}


static int
createBagFromSet(
    skBag_t            *bag,
    skstream_t         *stream)
{
    sk_ipv6policy_t ipv6policy = SK_IPV6POLICY_IGNORE;
    skipset_t *set = NULL;
    int rv;

    /* Read IPset from file */
    rv = skIPSetRead(&set, stream);
    if (rv) {
        if (SKIPSET_ERR_FILEIO == rv) {
            skStreamPrintLastErr(stream, skStreamGetLastReturnValue(stream),
                                 &skAppPrintErr);
        } else {
            skAppPrintErr("Unable to read IPset from '%s': %s",
                          skStreamGetPathname(stream), skIPSetStrerror(rv));
        }
        return 1;
    }

    if (skIPSetContainsV6(set)) {
        /* have the IPset convert everything to IPv6 */
        ipv6policy = SK_IPV6POLICY_FORCE;
        if (SKBAG_FIELD_CUSTOM == key_type) {
            skBagModify(bag, SKBAG_FIELD_ANY_IPv6, skBagCounterFieldType(bag),
                        SKBAG_OCTETS_FIELD_DEFAULT, SKBAG_OCTETS_NO_CHANGE);
        }
    } else if (SKBAG_FIELD_CUSTOM == key_type) {
        skBagModify(bag, SKBAG_FIELD_ANY_IPv4, skBagCounterFieldType(bag),
                    SKBAG_OCTETS_FIELD_DEFAULT, SKBAG_OCTETS_NO_CHANGE);
    }

    rv = skIPSetWalk(set, 0, ipv6policy, &bagFromSetCallback, (void*)bag);
    skIPSetDestroy(&set);

    return ((rv == 0) ? 0 : 1);
}


int main(int argc, char **argv)
{
    skBag_t *bag = NULL;
    skBagErr_t err;
    int rv = EXIT_FAILURE;

    appSetup(argc, argv); /* never returns on error */

    /* Create new bag */
    err = skBagCreateTyped(&bag, key_type, counter_type,
                           ((SKBAG_FIELD_CUSTOM == key_type)
                            ? sizeof(uint32_t) : SKBAG_OCTETS_FIELD_DEFAULT),
                           ((SKBAG_FIELD_CUSTOM == counter_type)
                            ? sizeof(uint64_t) : SKBAG_OCTETS_FIELD_DEFAULT));
    if (SKBAG_OK != err) {
        skAppPrintErr("Unable to create bag: %s", skBagStrerror(err));
        exit(EXIT_FAILURE);
    }

    /* Process input */
    if (set_input) {
        /* Handle set-file input */
        if (createBagFromSet(bag, set_input)) {
            skAppPrintErr("Error creating bag from set");
            goto END;
        }
    } else if (bag_input) {
        /* Handle bag-file input */
        if (createBagFromTextBag(bag, bag_input)) {
            skAppPrintErr("Error creating bag from text bag");
            goto END;
        }
    } else {
        skAbort();
    }

    /* write output */
    err = skBagWrite(bag, out_stream);
    if (SKBAG_OK != err) {
        if (SKBAG_ERR_OUTPUT == err) {
            skStreamPrintLastErr(out_stream,
                                 skStreamGetLastReturnValue(out_stream),
                                 &skAppPrintErr);
        } else {
            skAppPrintErr("Error writing bag to '%s': %s",
                          skStreamGetPathname(out_stream), skBagStrerror(err));
        }
        goto END;
    }

    rv = EXIT_SUCCESS;

  END:
    skBagDestroy(&bag);
    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
