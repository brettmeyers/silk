%{
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
**  Parser for silk toolset configuration file
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: sksiteconfig_parse.y cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "sksiteconfig.h"
#include <silk/sksite.h>

/* TYPEDEFS AND MACROS */

/* current version of the silk.conf file */
#define SKSITECONFIG_VERSION_CURRENT 2

/* default version to use if there is no 'version' command in the
 * file.  the default value is equivalent to the currect version, but
 * we use a different value to determine whether the 'version' has
 * been set explicitly */
#define SKSITECONFIG_VERSION_DEFAULT 0


/* EXPORTED VARIABLES */

/*
 * set to 1 to use the test handlers, which output what they think
 * they're seeing, for testing purposes.  This will be set to 1 when
 * the SKSITECONFIG_TESTING environment variable is set to a non-empty
 * value (whose first character is not '0')
 */
int sksiteconfig_testing = 0;


/* LOCAL VARIABLES */

/* current group or class being filled in--only one should be non-NULL
 * at a time. */
static char *current_group = NULL;
static sensorgroupID_t current_group_id = SK_INVALID_SENSORGROUP;
static char *current_class = NULL;
static classID_t current_class_id = SK_INVALID_CLASS;
static int site_file_version = SKSITECONFIG_VERSION_DEFAULT;


/* LOCAL FUNCTION PROTOTYPES */

/* Handle config file version */
static int do_version(int version);

/* Define sensor */
static void do_sensor(int id, char *name, char *description);

/* Define path-format */
static void do_path_format(char *fmt);

/* Define packing-logic */
static void do_packing_logic(char *fmt);

/* Include a file */
static void do_include(char *filename);

/* Begin defining a group */
static void do_group(char *groupname);

/* Add sensors to a group definition */
static void do_group_sensors(sk_vector_t *sensors);

/* Finish defining a group */
static void do_end_group(void);

/* Begin defining a class */
static void do_class(char *classname);

/* Add sensors to a class definition */
static void do_class_sensors(sk_vector_t *sensors);

/* Define type within a class definition */
static void do_class_type(int id, char *name, char *prefix);

/* Define the default types for a class */
static void do_class_default_types(sk_vector_t *types);

/* Finish defining a class */
static void do_end_class(void);

/* Set the default class */
static void do_default_class(char *classname);

/* Report an error while parsing (printf style) */
#define do_err sksiteconfigErr

/* Report a context error, like trying to define a sensor in a class */
static void do_err_ctx(const char *ctx, const char *cmd);

/* Report an argument error: too many, too few, or the wrong args */
static void do_err_args(const char *cmd);

/* Report an argument error: shouldn't be any arguments */
static void do_err_args_none(const char *cmd);


%}

%union {
    int integer;
    char *str;
    sk_vector_t *str_list;
}

%token TOK_NL

%token TOK_ATOM TOK_INTEGER TOK_STRING

%token TOK_CLASS TOK_DEF_CLASS TOK_DEF_TYPES TOK_END_CLASS TOK_END_GROUP
%token TOK_GROUP TOK_INCLUDE TOK_PATH_FORMAT TOK_PACKING_LOGIC
%token TOK_SENSOR TOK_SENSORS TOK_TYPE TOK_VERSION

%token ERR_UNK_CMD ERR_UNREC ERR_UNTERM_STRING ERR_STR_TOO_LONG
%token ERR_INVALID_OCTAL_ESCAPE

%type <str> TOK_ATOM
%type <str> TOK_INTEGER
%type <str> TOK_STRING
%type <str> ERR_UNK_CMD

%type <integer> int
%type <str> str
%type <str_list> str_list

%%

top_cmd_list:
    /* nothing */
  | top_cmd_list TOK_NL
  | top_cmd_list top_cmd
;

block_class:
    cmd_class class_cmd_list cmd_end_class
  | TOK_CLASS error TOK_NL        { do_err_args("class"); }
;

class_cmd_list:
    /* nothing */
  | class_cmd_list TOK_NL
  | class_cmd_list class_cmd
;

block_group:
    cmd_group group_cmd_list cmd_end_group
  | TOK_GROUP error TOK_NL        { do_err_args("group"); }
;

group_cmd_list:
    /* nothing */
  | group_cmd_list TOK_NL
  | group_cmd_list group_cmd
;



top_cmd:
    block_class
  | cmd_default_class
  | block_group
  | cmd_include
  | cmd_path_format
  | cmd_packing_logic
  | cmd_sensor
  | cmd_version
  | err_top
;

class_cmd:
    cmd_class_default_types
  | cmd_class_sensors
  | cmd_class_type
  | err_cls
;

group_cmd:
    cmd_group_sensors
  | err_grp
;



err_top:
    TOK_END_CLASS error TOK_NL    { do_err_ctx("top level", "end class"); }
  | TOK_END_GROUP error TOK_NL    { do_err_ctx("top level", "end group"); }
  | TOK_SENSORS error TOK_NL      { do_err_ctx("top level", "sensors"); }
  | TOK_TYPE error TOK_NL         { do_err_ctx("top level", "type"); }
  | ERR_UNK_CMD error TOK_NL      { do_err("Unknown command '%s'", $1);
                                    free($1); }
  | ERR_UNREC error TOK_NL        { do_err("Unrecognizable command"); }
;

err_grp:
    TOK_CLASS error TOK_NL        { do_err_ctx("group", "class"); }
  | TOK_DEF_CLASS error TOK_NL    { do_err_ctx("group", "default-class"); }
  | TOK_END_CLASS error TOK_NL    { do_err_ctx("group", "end class"); }
  | TOK_GROUP error TOK_NL        { do_err_ctx("group", "group"); }
  | TOK_INCLUDE error TOK_NL      { do_err_ctx("group", "include"); }
  | TOK_PATH_FORMAT error TOK_NL  { do_err_ctx("group", "path-format"); }
  | TOK_PACKING_LOGIC error TOK_NL {do_err_ctx("group", "packing-logic"); }
  | TOK_SENSOR error TOK_NL       { do_err_ctx("group", "sensor"); }
  | TOK_TYPE error TOK_NL         { do_err_ctx("group", "type"); }
  | TOK_VERSION error TOK_NL      { do_err_ctx("group", "version"); }
  | ERR_UNK_CMD error TOK_NL      { do_err("Unknown command '%s'", $1);
                                    free($1); }
  | ERR_UNREC error TOK_NL        { do_err("Unrecognizable command"); }
;

err_cls:
    TOK_CLASS error TOK_NL        { do_err_ctx("class", "class"); }
  | TOK_DEF_CLASS error TOK_NL    { do_err_ctx("class", "default-class"); }
  | TOK_END_GROUP error TOK_NL    { do_err_ctx("class", "end group"); }
  | TOK_GROUP error TOK_NL        { do_err_ctx("class", "group"); }
  | TOK_INCLUDE error TOK_NL      { do_err_ctx("class", "include"); }
  | TOK_PATH_FORMAT error TOK_NL  { do_err_ctx("class", "path-format"); }
  | TOK_PACKING_LOGIC error TOK_NL {do_err_ctx("class","packing-logic");}
  | TOK_SENSOR error TOK_NL       { do_err_ctx("class", "sensor"); }
  | TOK_VERSION error TOK_NL      { do_err_ctx("class", "version"); }
  | ERR_UNK_CMD error TOK_NL      { do_err("Unknown command '%s'", $1); }
  | ERR_UNREC error TOK_NL        { do_err("Unrecognizable command"); }
;

cmd_class:
    TOK_CLASS str TOK_NL                { do_class($2); }
    /* error handling is in block_class so that the context stays at
       top-level when there's a problem with the command. */
;

cmd_default_class:
    TOK_DEF_CLASS str TOK_NL      { do_default_class($2); }
;

cmd_group:
    TOK_GROUP str TOK_NL          { do_group($2); }
    /* error handling is in block_class so that the context stays at
       top-level when there's a problem with the command. */
;

cmd_include:
    TOK_INCLUDE str TOK_NL        { do_include($2); }
  | TOK_INCLUDE error TOK_NL      { do_err_args("include"); }
;

cmd_path_format:
    TOK_PATH_FORMAT str TOK_NL    { do_path_format($2); }
  | TOK_PATH_FORMAT error TOK_NL  { do_err_args("path-format"); }
;

cmd_packing_logic:
    TOK_PACKING_LOGIC str TOK_NL   { do_packing_logic($2); }
  | TOK_PACKING_LOGIC error TOK_NL { do_err_args("packing-logic"); }
;

cmd_sensor:
    TOK_SENSOR int str TOK_NL             { do_sensor($2, $3, NULL); }
  | TOK_SENSOR int str TOK_STRING TOK_NL  { do_sensor($2, $3, $4); }
  | TOK_SENSOR error TOK_NL               { do_err_args("sensor"); }
;

cmd_version:
    TOK_VERSION int TOK_NL        { if (do_version($2)) { YYABORT; } }
  | TOK_VERSION error TOK_NL      { do_err_args("version"); }
;

cmd_group_sensors:
    TOK_SENSORS str_list TOK_NL   { do_group_sensors($2); }
  | TOK_SENSORS error TOK_NL      { do_err_args("sensors"); }
;

cmd_end_group:
    TOK_END_GROUP TOK_NL          { do_end_group(); }
  | TOK_END_GROUP error TOK_NL    { do_err_args_none("end group"); }
;

cmd_class_default_types:
    TOK_DEF_TYPES str_list TOK_NL { do_class_default_types($2); }
  | TOK_DEF_TYPES error TOK_NL    { do_err_args("default-types"); }
;

cmd_class_sensors:
    TOK_SENSORS str_list TOK_NL   { do_class_sensors($2); }
  | TOK_SENSORS error TOK_NL      { do_err_args("sensors"); }
;

cmd_class_type:
    TOK_TYPE int str TOK_NL       { do_class_type($2, $3, NULL); }
  | TOK_TYPE int str str TOK_NL   { do_class_type($2, $3, $4); }
  | TOK_TYPE error TOK_NL         { do_err_args("type"); }
;

cmd_end_class:
    TOK_END_CLASS TOK_NL          { do_end_class(); }
  | TOK_END_CLASS error TOK_NL    { do_err_args_none("end class"); }
;

/* For int, accept only things that look like integers, then atoi them. */
int:
    TOK_INTEGER                   { $$ = atoi($1); free($1); }
;

/* For str, take anything that looks like a quoted string, an identifier
 * (atom), or a number. */
str:
    TOK_ATOM
  | TOK_STRING
  | TOK_INTEGER
;

str_list:
    /* empty */                   { $$ = skVectorNew(sizeof(char*)); }
| str_list str                    { skVectorAppendValue($1, &$2); $$ = $1; }
;

%%

/* SUPPORTING CODE */

int
yyerror(
    char        UNUSED(*s))
{
    /* do nothing, we handle error messages ourselves */
    return 0;
}

/* Handle config file version */
static int
do_version(
    int                 version)
{
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "version \"%d\"\n", version);
    }
    if (( SKSITECONFIG_VERSION_DEFAULT != site_file_version )
        && ( version != site_file_version ))
    {
        sksiteconfigErr("Multiple version commands specified");
    }
    if ( version < 1 || version > SKSITECONFIG_VERSION_CURRENT ) {
        sksiteconfigErr("Unsupported version '%d'", version);
        return 1;
    }
    site_file_version = version;
    return 0;
}

/* Define sensor */
static void
do_sensor(
    int                 id,
    char               *name,
    char               *description)
{
    const int sensor_desc_first_version = 2;

    if ( sksiteconfig_testing ) {
        fprintf(stderr, "sensor %d \"%s\"", id, name);
        if ( description ) {
            fprintf(stderr, " \"%s\"", description);
        }
        fprintf(stderr, "\n");
    }
    if ( id >= SK_MAX_NUM_SENSORS ) {
        sksiteconfigErr("Sensor id '%d' is greater than maximum of %d",
                        id, SK_MAX_NUM_SENSORS-1);
    } else if ( strlen(name) > SK_MAX_STRLEN_SENSOR ) {
        sksiteconfigErr("Sensor name '%s' is longer than maximum of %d",
                        name, SK_MAX_STRLEN_SENSOR);
    } else if ( sksiteSensorExists(id) ) {
        sksiteconfigErr("A sensor with id '%d' already exists", id);
    } else if ( sksiteSensorLookup(name) != SK_INVALID_SENSOR ) {
        sksiteconfigErr("A sensor with name '%s' already exists", name);
    } else if ( sksiteSensorCreate(id, name) ) {
        sksiteconfigErr("Failed to create sensor");
    } else if ( description ) {
        if (( site_file_version != SKSITECONFIG_VERSION_DEFAULT )
            && ( site_file_version < sensor_desc_first_version ))
        {
            sksiteconfigErr(("Sensor descriptions only allowed when"
                             " file's version is %d or greater"),
                            sensor_desc_first_version);
        } else if ( sksiteSensorSetDescription(id, description) ) {
            sksiteconfigErr("Failed to set sensor description");
        }
    }
    if ( description ) {
        free(description);
    }
    free(name);
}

/* Define path-format */
static void
do_path_format(
    char               *fmt)
{
    const char *cp;
    int final_x = 0;

    if ( sksiteconfig_testing ) {
        fprintf(stderr, "path-format \"%s\"\n", fmt);
    }
    cp = fmt;
    while (NULL != (cp = strchr(cp, '%'))) {
        ++cp;
        if (!*cp) {
            sksiteconfigErr("The path-format '%s' ends with a single '%%'",
                            fmt);
            break;
        }
        if (NULL == strchr(path_format_conversions, *cp)) {
            sksiteconfigErr(
                "The path-format '%s' contains an unknown conversion '%%%c'",
                fmt, *cp);
        } else if ('x' == *cp && '\0' == *(cp+1)) {
            /* Found %x at the end; confirm that either the entire fmt
             * is "%x" or that "%x" is preceeded by '/'  */
            if ((cp-1 == fmt) || ('/' == *(cp-2))) {
                final_x = 1;
            }
        }
        ++cp;
    }
    if (!final_x) {
        sksiteconfigErr("The path-format '%s' does not end with '/%%x'", fmt);
    }
    if ( sksiteSetPathFormat(fmt) ) {
        sksiteconfigErr("Failed to set path-format");
    }
    free(fmt);
}

/* Define the packing-logic file */
static void
do_packing_logic(
    char               *fmt)
{
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "packing-logic \"%s\"\n", fmt);
    }
    if ( sksiteSetPackingLogicPath(fmt) ) {
        sksiteconfigErr("Failed to set packing-logic");
    }
    free(fmt);
}

/* Include a file */
static void
do_include(
    char               *filename)
{
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "include \"%s\"\n", filename);
    }
    sksiteconfigIncludePush(filename);
}

/* Begin defining a group */
static void
do_group(
    char               *groupname)
{
    assert(current_group == NULL);
    assert(current_class == NULL);
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "group \"%s\"\n", groupname);
    }
    current_group = groupname;
    current_group_id = sksiteSensorgroupLookup(current_group);
    if ( current_group_id == SK_INVALID_SENSORGROUP ) {
        current_group_id = sksiteSensorgroupGetMaxID() + 1;
        if ( sksiteSensorgroupCreate(current_group_id, groupname) ) {
            current_group_id = SK_INVALID_SENSORGROUP;
            sksiteconfigErr("Failed to create sensorgroup");
        }
    }
}

/* Add sensors to a group definition */
static void
do_group_sensors(
    sk_vector_t        *sensors)
{
    size_t i;
    size_t len;
    char *str;
    sensorID_t sensor_id;
    sensorgroupID_t sensorgroup_id;

    assert(current_group != NULL);
    assert(current_class == NULL);
    len = skVectorGetCount(sensors);
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "[group \"%s\"] sensors", current_group);
        for ( i = 0; i < len; i++ ) {
            skVectorGetValue(&str, sensors, i);
            fprintf(stderr, " %s", str);
        }
        fprintf(stderr, "\n");
    }
    if ( current_group_id != SK_INVALID_SENSORGROUP ) {
        for ( i = 0; i < len; i++ ) {
            skVectorGetValue(&str, sensors, i);
            if ( str[0] == '@' ) {
                sensorgroup_id = sksiteSensorgroupLookup(&str[1]);
                if ( sensorgroup_id == SK_INVALID_SENSORGROUP ) {
                    sksiteconfigErr(("Cannot add group to group '%s':"
                                     " group '%s' is not defined"),
                                    current_group, str);
                } else {
                    sksiteSensorgroupAddSensorgroup(current_group_id,
                                                    sensorgroup_id);
                }
            } else {
                sensor_id = sksiteSensorLookup(str);
                if ( sensor_id == SK_INVALID_SENSOR ) {
                    sksiteconfigErr(("Cannot add sensor to group '%s':"
                                     " sensor '%s' is not defined"),
                                    current_group, str);
                } else {
                    sksiteSensorgroupAddSensor(current_group_id, sensor_id);
                }
            }
        }
    }
    /* free the vector and its contents */
    for (i = 0; i < len; ++i) {
        skVectorGetValue(&str, sensors, i);
        free(str);
    }
    skVectorDestroy(sensors);
}

/* Finish defining a group */
static void
do_end_group(
    void)
{
    assert(current_group != NULL);
    assert(current_class == NULL);
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "[group \"%s\"] end group\n", current_group);
    }
    free(current_group);
    current_group = NULL;
}

/* Begin defining a class */
static void
do_class(
    char               *classname)
{
    assert(current_group == NULL);
    assert(current_class == NULL);
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "class \"%s\"\n", classname);
    }
    current_class = classname;
    current_class_id = sksiteClassLookup(current_class);
    /* We're okay on "duplicates": just more info on existing class */
    if ( current_class_id == SK_INVALID_CLASS ) {
        if ( strlen(classname) > SK_MAX_STRLEN_FLOWTYPE ) {
            sksiteconfigErr(("The class-name '%s'"
                             " is longer than the maximum of %d"),
                            classname, SK_MAX_STRLEN_FLOWTYPE);
        }
        current_class_id = sksiteClassGetMaxID() + 1;
        if ( sksiteClassCreate(current_class_id, classname) ) {
            current_class_id = SK_INVALID_CLASS;
            sksiteconfigErr("Failed to create class");
        }
    }
}

/* Add sensors to a class definition */
static void
do_class_sensors(
    sk_vector_t        *sensors)
{
    size_t i;
    size_t len;
    char *str;
    sensorID_t sensor_id;
    sensorgroupID_t sensorgroup_id;

    assert(current_class != NULL);
    assert(current_group == NULL);
    len = skVectorGetCount(sensors);
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "[class \"%s\"] sensors", current_class);
        for ( i = 0; i < len; i++ ) {
            skVectorGetValue(&str, sensors, i);
            fprintf(stderr, " %s", str);
        }
        fprintf(stderr, "\n");
    }
    if ( current_class_id != SK_INVALID_CLASS ) {
        for ( i = 0; i < len; i++ ) {
            skVectorGetValue(&str, sensors, i);
            if ( str[0] == '@' ) {
                sensorgroup_id = sksiteSensorgroupLookup(&str[1]);
                if ( sensorgroup_id == SK_INVALID_SENSORGROUP ) {
                    sksiteconfigErr(("Cannot add group to class '%s':"
                                     " group '%s' is not defined"),
                                    current_class, str);
                } else {
                    sksiteClassAddSensorgroup(current_class_id,
                                              sensorgroup_id);
                }
            } else {
                sensor_id = sksiteSensorLookup(str);
                if ( sensor_id == SK_INVALID_SENSOR ) {
                    sksiteconfigErr(("Cannot add sensor to class '%s':"
                                     " sensor '%s' is not defined"),
                                    current_class, str);
                } else {
                    sksiteClassAddSensor(current_class_id, sensor_id);
                }
            }
        }
    }
    /* free the vector and its contents */
    for (i = 0; i < len; ++i) {
        skVectorGetValue(&str, sensors, i);
        free(str);
    }
    skVectorDestroy(sensors);
}

/* Define type within a class definition */
static void
do_class_type(
    int                 id,
    char               *type,
    char               *name)
{
    char flowtype_name_buf[SK_MAX_STRLEN_FLOWTYPE+1];

    assert(current_class != NULL);
    assert(type);

    if ( sksiteconfig_testing ) {
        fprintf(stderr, "[class \"%s\"] type %d %s",
                current_class, id, type);
        if (name) {
            fprintf(stderr, " %s", name);
        }
        fprintf(stderr, "\n");
    }

    if ( strlen(type) > SK_MAX_STRLEN_FLOWTYPE ) {
        sksiteconfigErr(("The type-name '%s'"
                         " is longer than the maximum of %d"),
                        type, SK_MAX_STRLEN_FLOWTYPE);
    }
    if ( name ) {
        if ( strlen(name) > SK_MAX_STRLEN_FLOWTYPE ) {
            sksiteconfigErr(("The flowtype-name '%s'"
                             " is longer than the maximum of %d"),
                            name, SK_MAX_STRLEN_FLOWTYPE);
        }
    } else {
        if ( snprintf(flowtype_name_buf, SK_MAX_STRLEN_FLOWTYPE,
                      "%s%s", current_class, type)
             > SK_MAX_STRLEN_FLOWTYPE )
        {
            sksiteconfigErr(("The generated flowtype-name '%s%s'"
                             " is longer than the maximum of %d"),
                            current_class, type, SK_MAX_STRLEN_FLOWTYPE);
        }
        name = flowtype_name_buf;
    }
    if ( current_class_id != SK_INVALID_CLASS ) {
        if ( id >= SK_MAX_NUM_FLOWTYPES ) {
            sksiteconfigErr("Type id '%d' is greater than maximum of %d",
                            id, SK_MAX_NUM_FLOWTYPES-1);
        } else if ( sksiteFlowtypeExists(id) ) {
            sksiteconfigErr("A type with id '%d' already exists", id);
        } else if ( sksiteFlowtypeLookup(name) != SK_INVALID_FLOWTYPE ) {
            sksiteconfigErr("A type with prefix '%s' already exists", name);
        } else if ( sksiteFlowtypeLookupByClassIDType(current_class_id, type)
                    != SK_INVALID_CLASS ) {
            sksiteconfigErr("The type '%s' for class '%s' already exists",
                            type, current_class);
        } else if ( sksiteFlowtypeCreate(id, name, current_class_id, type) ) {
            sksiteconfigErr("Failed to create type");
        }
    }
    free(type);
    if ( name != flowtype_name_buf ) {
        free(name);
    }
}

/* Set the default types within a class definition */
static void
do_class_default_types(
    sk_vector_t        *types)
{
    size_t i;
    size_t len;
    char *str;
    flowtypeID_t flowtype_id;

    assert(current_class != NULL);
    assert(current_group == NULL);
    len = skVectorGetCount(types);
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "[class \"%s\"] default-types", current_class);
        for ( i = 0; i < len; i++ ) {
            skVectorGetValue(&str, types, i);
            fprintf(stderr, " %s", str);
        }
        fprintf(stderr, "\n");
    }
    if ( current_class_id != SK_INVALID_CLASS ) {
        for ( i = 0; i < len; i++ ) {
            skVectorGetValue(&str, types, i);
            flowtype_id =
                sksiteFlowtypeLookupByClassIDType(current_class_id, str);
            if ( flowtype_id == SK_INVALID_FLOWTYPE ) {
                sksiteconfigErr(("Cannot set default type in class '%s':"
                                 " type '%s' is not defined"),
                                current_class, str);
            } else if ( sksiteClassAddDefaultFlowtype(current_class_id,
                                                      flowtype_id) )
            {
                sksiteconfigErr("Failed to add default type");
            }
        }
    }
    /* free the vector and its contents */
    for (i = 0; i < len; ++i) {
        skVectorGetValue(&str, types, i);
        free(str);
    }
    skVectorDestroy(types);
}

/* Finish defining a class */
static void
do_end_class(
    void)
{
    assert(current_class != NULL);
    assert(current_group == NULL);
    if ( sksiteconfig_testing ) {
        fprintf(stderr, "[class \"%s\"] end class\n", current_class);
    }
    free(current_class);
    current_class = NULL;
}

static void
do_default_class(
    char               *name)
{
    classID_t class_id;

    if ( sksiteconfig_testing ) {
        fprintf(stderr, "default-class \"%s\"\n", name);
    }
    class_id = sksiteClassLookup(name);
    if ( class_id == SK_INVALID_CLASS ) {
        sksiteconfigErr("Cannot set default class: class '%s' is not defined",
                        name);
    } else {
        if ( sksiteClassSetDefault(class_id) ) {
            sksiteconfigErr("Failed to set default class");
        }
    }
    free(name);
}

/* Report a context error, like trying to define a sensor in a class */
static void
do_err_ctx(
    const char         *ctx,
    const char         *cmd)
{
    do_err("Command '%s' not allowed in %s", cmd, ctx);
}

/* Report an argument error: too many, too few, or the wrong args */
static void
do_err_args(
    const char         *cmd)
{
    do_err("Bad arguments to command '%s'", cmd);
}

/* Report an argument error: shouldn't be any arguments */
static void
do_err_args_none(
    const char         *cmd)
{
    do_err("Command '%s' does take arguments", cmd);
}


/*
** Local variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
