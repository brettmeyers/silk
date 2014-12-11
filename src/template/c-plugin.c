/*
 *    Use this file as a template to create a C plug-in for use with
 *    the SiLK tools rwfilter, rwcut, rwgroup, rwsort, rwstats, or
 *    rwuniq.
 */

#include <silk/silk.h>
#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/skplugin.h>
#include <silk/utils.h>


/* DEFINES AND TYPEDEFS */

/*
 *    These variables specify the version of the SiLK plug-in API.
 *    They are used in the call to skpinSimpleCheckVersion() below.
 *    See the description of that function for their meaning.
 */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0


/* LOCAL FUNCTION PROTOTYPES */

/* a convenience structure to define command line options.  you don't
 * have to use this. */
typedef struct option_info_st {
    /* this is mask specifying the applications for which this option
     * should be available.  use SKPLUGIN_FN_ANY for the option to be
     * available on all applications. */
    skplugin_fn_mask_t  apps;
    /* the name of the option (the command line switch) */
    const char         *name;
    /* whether it requires an argument, one of REQUIRED_ARG,
     * OPTIONAL_ARG, or NO_ARG */
    int                 has_arg;
    /* a unique value for this option */
    int                 val;
    /* the option's help string; printed when --help is given */
    char               *help;
} option_info_t;


/* LOCAL VARIABLES */

/*
 *    In this sample, we have the plug-in create two switches --one
 *    and --two, but --two is only available on rwfilter.  For their
 *    IDs, we define an enumeration.
 */
typedef enum options_enum_en {
    OPT_ONE, OPT_TWO
} options_enum;

static const option_info_t my_options[] = {
    {SKPLUGIN_FN_ANY,     "one", REQUIRED_ARG, OPT_ONE,
     "my first option"},
    {SKPLUGIN_FN_FILTER,  "two", REQUIRED_ARG, OPT_TWO,
     "my second option"},
    {0, 0, 0, 0, 0}             /* sentinel */
};




/* LOCAL FUNCTION PROTOTYPES */

static skplugin_err_t
optionsHandler(
    const char         *opt_arg,
    void               *reg_data);
static skplugin_err_t
initialize(
    void               *reg_data);
static skplugin_err_t
cleanup(
    void               *reg_data);
static skplugin_err_t
recToText(
    const rwRec        *rwrec,
    char               *text_value,
    size_t              text_size,
    void               *reg_data,
    void              **extra);
static skplugin_err_t
recToBin(
    const rwRec        *rwrec,
    uint8_t            *bin_value,
    void               *reg_data,
    void              **extra);
static skplugin_err_t
binToText(
    const uint8_t      *bin_value,
    char               *text_value,
    size_t              text_size,
    void               *reg_data);
static skplugin_err_t
addRecToBin(
    const rwRec        *rwrec,
    uint8_t            *bin_value,
    void               *reg_data,
    void              **extra);
static skplugin_err_t
binMerge(
    uint8_t            *dst_bin_value,
    const uint8_t      *src_bin_value,
    void               *reg_data);
static skplugin_err_t
binCompare(
    int                *cmp_result,
    const uint8_t      *value_a,
    const uint8_t      *value_b,
    void               *reg_data);
static skplugin_err_t
filter(
    const rwRec        *rwrec,
    void               *reg_data,
    void              **extra);
static skplugin_err_t
transform(
    rwRec              *rwrec,
    void               *reg_data,
    void              **extra);



/* FUNCTION DEFINITIONS */

/*
 *    This is the registration function.
 *
 *    When you provide "--plugin=my-plugin.so" on the command line to
 *    an application, the application calls this function to determine
 *    the new switches and/or fields that "my-plugin" provides.
 *
 *    This function is called with three arguments: the first two
 *    describe the version of the plug-in API, and the third is a
 *    pointer that is currently unused.
 */
skplugin_err_t
SKPLUGIN_SETUP_FN(
    uint16_t            major_version,
    uint16_t            minor_version,
    void               *plug_in_data)
{
    int i;
    skplugin_field_t *field;
    skplugin_err_t rv;
    skplugin_callbacks_t regdata;

    /* Check the plug-in API version */
    rv = skpinSimpleCheckVersion(major_version, minor_version,
                                 PLUGIN_API_VERSION_MAJOR,
                                 PLUGIN_API_VERSION_MINOR,
                                 skAppPrintErr);
    if (rv != SKPLUGIN_OK) {
        return rv;
    }

    /*
     *  Register the options.  Note that we pass the option identifier
     *  in the cddata field as a void*.
     */
    for (i = 0; my_options[i].name; ++i) {
        rv = skpinRegOption2(my_options[i].name,
                             my_options[i].has_arg,
                             my_options[i].help,
                             &optionsHandler, NULL,
                             (void*)&my_options[i].val,
                             1, my_options[i].apps);
        /* it's (probably) not an error if the option was not
         * registered.  in our example, option "two" is only
         * registered when the plug-in is loaded by rwfilter. */
        if (SKPLUGIN_OK != rv && SKPLUGIN_ERR_DID_NOT_REGISTER != rv) {
            return rv;
        }
    }


    /*
     *    All of the skplugin registration functions take a pointer to
     *    an skplugin_callbacks_t structure.  If the structure has
     *    valid values for the fields that an application requires,
     *    the application will register the field, the filter
     *    function, or the transform function.  If the structure does
     *    not have valid values for fields, the application ignores
     *    the registration call.
     *
     *    When defining multiple fields or filters within a single
     *    plug-in, there approaches:
     *
     *    1. You can create functions that operate on each field
     *    individually.  In this approach, you may have recToText1()
     *    and recToText2().
     *
     *    2. You can create a function that computes the value for
     *    multiple fields.  In this approach, the 'reg_data' field is
     *    often used to tell the function which field to compute.
     *
     *    Either approach is fine.  The first approach can be easier,
     *    but it can lead to a lot of duplicate code.
     */



    memset(&regdata, 0, sizeof(regdata));
    /* set the fields on the skplugin_callbacks_t structure.  This
     * example shows a value for each field, but you only need to set
     * the values that you require. */

    /* when special initialization is required by the 'filter' or
     * 'transform' functions or for the field in rwcut, rwgroup,
     * rwsort, rwstats, or rwuniq, specify a function that should be
     * called just before the application begins to process
     * records. */
    regdata.init           = &initialize;
    /* when special clean-up is required by the 'filter' or
     * 'transform' functions or for the field in rwcut, rwgroup,
     * rwsort, rwstats, or rwuniq, specify a function that
     * should be during shutdown. */
    regdata.cleanup        = &cleanup;
    /* when defining a field for rwcut, rwstats, or rwuniq, specify
     * the desired width of the output column to use for the textual
     * output. */
    regdata.column_width   = 0;
    /* when defining a field for rwgroup, rwsort, rwstats, or rwuniq,
     * specify the number of byte required to hold the binary
     * representation of the field. */
    regdata.bin_bytes      = 0;
    /* when defining a key field for rwcut, specify a function to
     * convert the rwRec to a textual field. */
    regdata.rec_to_text    = &recToText;
    /* when defining a key field for rwgroup, rwsort, rwstats, or
     * rwuniq, specify a function to convert the rwRec to a binary
     * value.  The length of the returned value should be exactly
     * bin_bytes. */
    regdata.rec_to_bin     = &recToBin;
    /* when defining an aggregate value field for rwstats or rwuniq,
     * specify a function to update a binary value with a value based
     * on the current rwRec. */
    regdata.add_rec_to_bin = &addRecToBin;
    /* when defining a key field or an aggregate value field for
     * rwstats or rwuniq, specify a function to convert the binary
     * value created by rec_to_bin (for keys) or add_rec_to_bin (for
     * aggregate values) to a textual field. */
    regdata.bin_to_text    = &binToText;
    /* when defining an aggregate value field for rwstats or rwuniq,
     * specify a function to merge two binary values created by
     * add_rec_to_bin. */
    regdata.bin_merge      = &binMerge;
    /* when defining an aggregate value field for rwstats, specify a
     * function to compare two the binary values created by
     * add_rec_to_bin.  This is required to sort the output in
     * rwstats. */
    regdata.bin_compare    = &binCompare;
    /* when defining an aggregate value field for rwstats or rwuniq,
     * specify the value to use to initialize the aggregate value.
     * This value should be exactly 'bin_bytes' long.  If not
     * specified, the value is zeroed. */
    regdata.initial        = NULL;
    /* when defining a partioning rule for use in rwfilter, specify a
     * function that determines whether a record passes or fails. */
    regdata.filter         = &filter;
    /* when defining a function that modifies SiLK records (for
     * example, in rwptoflow), specify a function that transforms the
     * SiLK record. */
    regdata.transform      = &transform;
    /* this will only be required for complicated plug-ins and is not
     * described here. */
    regdata.extra          = NULL;


    rv = skpinRegField(&field,              /* handle to new field */
                       "field_name",        /* field name */
                       "field description", /* field description */
                       &regdata,            /* skplugin_callbacks_t */
                       NULL);               /* reg_data */
    if (SKPLUGIN_OK != rv) {
        return rv;
    }

    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR;
}


/*
 *  status = optionsHandler(opt_arg, reg_data);
 *
 *    Handles options for the plug-in.  Note that the skpinRegOption()
 *    function call above contains a pointer to this function.
 *
 *    This function is called when the application sees an option that
 *    the plug-in has registered.  'opt_arg' is the argument to the
 *    option (for example, if the user specifies --foo=23, then
 *    'opt_arg' will be the character buffer "23") or NULL if no
 *    argument was given.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the option.
 *
 *    Returns SKPLUGIN_OK on success, or SKPLUGIN_ERR if there was a
 *    problem.
 */
static skplugin_err_t
optionsHandler(
    const char         *opt_arg,
    void               *reg_data)
{
    options_enum opt_index = *((options_enum*)reg_data);
    skplugin_callbacks_t regdata;

    switch (opt_index) {
      case OPT_ONE:
        /* handle option "one" */
        break;

      case OPT_TWO:
        /* part of handling option "two" is to register a filter.
         * This is one way to write a plug-in that allows the user to
         * choose from among multiple filters */
        memset(&regdata, 0, sizeof(regdata));
        regdata.filter = &filter;
        return skpinRegFilter(NULL, &regdata, NULL);
    }

    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR;
}


/*
 *  status = initialize(reg_data);
 *
 *    This function only needs to be specified when special
 *    initialization code is required.  You do not have to have an
 *    initialize function.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 */
static skplugin_err_t
initialize(
    void               *reg_data)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;
}


/*
 *  status = cleanup(reg_data);
 *
 *    This function only needs to be specified when special cleanup
 *    code is required.  You do not have to have an cleanup function.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 */
static skplugin_err_t
cleanup(
    void               *reg_data)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;
}


/*
 *  status = recToText(rwrec, text_value, text_size, reg_data, extra);
 *
 *    A function similar to this is required when the plug-in will be
 *    used to create a key field for use by rwcut.
 *
 *    The function should use the SiLK flow record 'rwrec' to create a
 *    textual field.  The function should then write that textual
 *    value into the 'text_value' character buffer, writing no more
 *    than 'text_size' characters to it---including the final NUL.
 *    The value of 'text_size' will be at least one more than the
 *    'column_width' that was specified when the field was registered.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 *
 *    You will most likely not use the 'extra' parameter.
 */
static skplugin_err_t
recToText(
    const rwRec        *rwrec,
    char               *text_value,
    size_t              text_size,
    void               *reg_data,
    void              **extra)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;

#if 0
    /* key example: print lower of sPort or dPort */
    if (rwRecGetSPort(rwrec) < rwRecGetDPort(rwrec)) {
        snprintf(text_value, text_size, rwRecGetSPort(rwrec));
    } else {
        snprintf(text_value, text_size, rwRecGetDPort(rwrec));
    }
    return SKPLUGIN_OK;
#endif  /* 0 */
}


/*
 *  status = recToBin(rwrec, bin_value, reg_data, extra);
 *
 *    A function similar to this is required when the plug-in will be
 *    used to create a key field for use by rwgroup, rwsort, rwstats,
 *    or rwuniq.  (For rwstats and rwuniq, a binToText() function is
 *    also required.)
 *
 *    The function should use the SiLK flow record 'rwrec' to create a
 *    binary value.  The function should then write that binary value
 *    into the 'bin_value' buffer.  Since this value will be used for
 *    sorting, the binary value should be written in network byte
 *    order (big endian).
 *
 *    The function should write the same number of bytes as were
 *    specified in the 'bin_bytes' field when the field was
 *    registered.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 *
 *    You will most likely not use the 'extra' parameter.
 */
static skplugin_err_t
recToBin(
    const rwRec        *rwrec,
    uint8_t            *bin_value,
    void               *reg_data,
    void              **extra)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;

#if 0
    /* key example: encode lower of sPort or dPort */
    uint16_t port;

    if (rwRecGetSPort(rwrec) < rwRecGetDPort(rwrec)) {
        port = htons(rwRecGetSPort(rwrec));
    } else {
        port = htons(rwRecGetDPort(rwrec));
    }
    memcpy(bin_value, &port, sizeof(port));
    return SKPLUGIN_OK;
#endif  /* 0 */
}


/*
 *  status = binToText(bin_value, text_value, text_size, reg_data);
 *
 *    A function similar to this is required when the plug-in will be
 *    used to create a key field or an aggregate value for use by
 *    rwstats or rwuniq.
 *
 *    The function should use the 'bin_value' buffer to create a
 *    textual value.  This function should write the textual value
 *    into the 'text_value' character buffer, writing no more than
 *    'text_size' characters to it---including the final NUL.  The
 *    value of 'text_size' will be at least one more than the
 *    'column_width' that was specified when the field was registered.
 *
 *    For key fields, the contents of 'bin_value' will be the result
 *    of a prior call to the recToBin() function.
 *
 *    For aggregate value fields, the contents of 'bin_value' will be
 *    the result of calls to addRecToBin() and binMerge().
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 */
static skplugin_err_t
binToText(
    const uint8_t      *bin_value,
    char               *text_value,
    size_t              text_size,
    void               *reg_data)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;

#if 0
    /* key example: print lower port encoded by recToBin */
    uint16_t port;

    memcpy(&port, bin_value, sizeof(port));
    snprintf(text_value, text_size, ntohs(port));
    return SKPLUGIN_OK;

    /* value example:  sum of durations */
    uint32_t dur;

    memcpy(&dur, bin_value, sizeof(dur));
    snprintf(text_value, text_size, dur);
    return SKPLUGIN_OK;
#endif  /* 0 */
}


/*
 *  status = addRecToBin(rwrec, bin_value, reg_data, extra);
 *
 *    A function similar to this is required when the plug-in will be
 *    used to create an aggregate value field for use by rwstats or or
 *    rwuniq.  The binToText() and binMerge() functions are also
 *    required.
 *
 *    The function should use the SiLK flow record 'rwrec' to create a
 *    binary value.  The function should then  ADD or MERGE that binary
 *    value into the value currently in the 'bin_value' buffer.  For
 *    example, if your plug-in is counting bytes, it should add the
 *    bytes from the current 'rwRec' into the value in 'bin_value'.
 *
 *    The length of 'bin_value' is determined by the 'bin_bytes' field
 *    when the field was registered.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 *
 *    You will most likely not use the 'extra' parameter.
 */
static skplugin_err_t
addRecToBin(
    const rwRec        *rwrec,
    uint8_t            *bin_value,
    void               *reg_data,
    void              **extra)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;

#if 0
    /* value example:  sum of duration for all flows matching key */
    uint32_t dur;

    memcpy(&dur, bin_value, sizeof(dur));
    dur += rwRecGetElapsed(rwrec);
    memcpy(bin_value, &dur, sizeof(dur));
    return SKPLUGIN_OK;
#endif  /* 0 */
}


/*
 *  status = binMerge(dst_bin_value, src_bin_value, reg_data);
 *
 *    A function similar to this is required when the plug-in will be
 *    used to create an aggregate value field for use by rwstats or or
 *    rwuniq.  The binToText() and addRecToBin() functions are also
 *    required.
 *
 *    This function is used to combine two binary aggregate values,
 *    created by prior calls to addRecToBin(), into a single binary
 *    value.  When called, both 'dst_bin_value' and 'src_bin_value'
 *    will have valid binary values.  This function should combine the
 *    values (by adding or merging them as appropriate) and put the
 *    resulting value into 'dst_bin_value'.
 *
 *    The length of 'bin_value' is determined by the 'bin_bytes' field
 *    when the field was registered.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 *
 *    When rwstats or rwuniq run out of RAM, they write their current
 *    (key,value) pair to temporary files disk; once all records have
 *    been processed, the (key,value) pairs in the temporary files
 *    must be merged.  This function will be called to merge values
 *    for entries with identical keys.
 */
static skplugin_err_t
binMerge(
    uint8_t            *dst_bin_value,
    const uint8_t      *src_bin_value,
    void               *reg_data)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;

#if 0
    /* value example:  sum of duration for all flows matching key */
    uint32_t dst_dur;
    uint32_t src_dur;

    memcpy(&dst_dur, dst_bin_value, sizeof(dst_dur));
    memcpy(&src_dur, src_bin_value, sizeof(src_dur));
    dst_dur += src_dur;
    memcpy(dst_bin_value, &dst_dur, sizeof(dst_dur));
    return SKPLUGIN_OK;
#endif  /* 0 */
}


/*
 *  status = binCompare(cmp_result, bin_value_a, bin_value_b, reg_data);
 *
 *    A function similar to this is required when the plug-in will be
 *    used to create an aggregate value field for use by rwstats.  The
 *    binToText(), addRecToBin(), and binMerge() functions are also
 *    required.
 *
 *    This function is used to compare two binary aggregate values,
 *    created by prior calls to addRecToBin().  This is required to
 *    sort the value fields in rwstats to produce a Top-N list.  The
 *    function should set 'cmp_result' to a value less than 0, equal
 *    to 0, or greater than 0 when 'bin_value_a' is less then, equal
 *    to, or greater than 'bin_value_b', respectively.
 *
 *    The length of 'bin_value' is determined by the 'bin_bytes' field
 *    when the field was registered.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 */
static skplugin_err_t
binCompare(
    int                *cmp_result,
    const uint8_t      *bin_value_a,
    const uint8_t      *bin_value_b,
    void               *reg_data)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;

#if 0
    /* value example:  sum of duration for all flows matching key */
    uint32_t dur_a;
    uint32_t dur_b;

    memcpy(&dur_a, bin_value_a, sizeof(dur_a));
    memcpy(&dur_b, bin_value_b, sizeof(dur_b));
    if (dur_a < dur_b) {
        *cmp_result = -1;
    } else if (dur_a > dur_b) {
        *cmp_result = 1;
    } else {
        *cmp_result = 0;
    }
    return SKPLUGIN_OK;
#endif  /* 0 */
}


/*
 *  status = filter(rwrec, reg_data, extra);
 *
 *    A function similar to this is required when the plug-in will be
 *    used to partition fields into PASS and FAIL streams in rwfilter.
 *
 *    The function should examine the SiLK flow record and return
 *    SKPLUGIN_FILTER_PASS to write the rwRec to the
 *    pass-destination(s) or SKPLUGIN_FILTER_FAIL to write it to the
 *    fail-destination(s).
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 *
 *    You will most likely not use the 'extra' parameter.
 */
static skplugin_err_t
filter(
    const rwRec        *rwrec,
    void               *reg_data,
    void              **extra)
{
    /* return one of the following */
    return SKPLUGIN_FILTER_FAIL;
    return SKPLUGIN_FILTER_PASS;

#if 0
    /* example: pass ICMP or ICMPv6 flows */
    if (IPPROTO_ICMP == rwRecGetProto(rwrec)
        || IPPROTO_ICMPV6 == rwRecGetProto(rwrec))
    {
        return SKPLUGIN_FILTER_PASS;
    }
    return SKPLUGIN_FILTER_FAIL;
#endif  /* 0 */
}


/*
 *  status = transform(rwrec, reg_data, extra);
 *
 *    A function similar to this is required when the plug-in will be
 *    used to modify the SiLK Flow records.
 *
 *    The function can modify the SiLK flow record in place.  One use
 *    for this is to modify records during their creation by the
 *    rwptoflow application.
 *
 *    The 'reg_data' will be the value you specified when you
 *    registered the field.  You do not have to use that value.
 *
 *    You will most likely not use the 'extra' parameter.
 */
static skplugin_err_t
transform(
    rwRec              *rwrec,
    void               *reg_data,
    void              **extra)
{
    /* return one of the following */
    return SKPLUGIN_OK;
    return SKPLUGIN_ERR_FATAL;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
