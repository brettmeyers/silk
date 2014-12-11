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
**  sksite.h
**
**    An interface to site-specific settings.
**
*/
#ifndef _SKSITE_H
#define _SKSITE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKSITE_H, "$SiLK: sksite.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/silk_types.h>

/**
 *  @file
 *
 *    The interface to site-specific settings, many of which are
 *    determined by the silk.conf file.
 *
 *    This file is part of libsilk.
 */


/**
 *    Name of environment variable that specfies the root directory of
 *    the repository of hourly SiLK Flow files.
 */
#define SILK_DATA_ROOTDIR_ENVAR "SILK_DATA_ROOTDIR"

/**
 *    Name of environment variable that specfies the full path to the
 *    silk.conf configuration file.
 */
#define SILK_CONFIG_FILE_ENVAR "SILK_CONFIG_FILE"


/* Add an option for processing the silk.conf configuration file */
#define SK_SITE_FLAG_CONFIG_FILE  0x01


/** Configuration and Initialization **********************************/

/**
 *    Initialize everything related to the site *except* the sensor
 *    configuration from the config file.  The parameter 'levels' is
 *    currently unused.
 *
 *    Application writers do not need to call this function as it is
 *    called by skAppRegister().
 */
int
sksiteInitialize(
    int                 levels);

/**
 *    Register options according to the value of 'flags'.
 *
 *    The parameter 'flags' is the bitsize-OR of the SK_SITE_FLAG_*
 *    values listed above.
 */
int
sksiteOptionsRegister(
    uint32_t            flags);

/**
 *    Print usage information for any options that were registered by
 *    sksiteOptionsRegister() to the file handle 'fh'.
 */
void
sksiteOptionsUsage(
    FILE               *fh);

/**
 *    Read the SiLK site configuration file (silk.conf) from the path
 *    set by sksiteSetConfigPath() or from one of the default
 *    locations:
 *
 *      -- location specified by SILK_CONFIG_FILE environment variable
 *      -- SILK_DATA_ROOTDIR/silk.conf
 *      -- SILK_PATH/share/silk/silk.conf, SILK_PATH/share/silk.conf
 *      -- bindir/../share/silk/silk.conf, bindir/../share/silk.conf
 *
 *    where bindir is location of current application.
 *
 *    Return 0 if the configuration has properly loaded; or non-zero
 *    otherwise.  Return -1 to indicate errors parsing the file, or -2
 *    if the site configuration file cannot be found.
 *
 *    If the function returns -2 to indicate the file was not found,
 *    you may called it again, presumably after calling
 *    sksiteSetConfigPath() or sksiteSetRootDir().
 *
 *    Once the function returns 0 or -1, nothing will happen on
 *    subsequent call to the function.
 *
 *    If verbose is 1, this function will report failures to open the
 *    file as errors.  If verbose is 0, only parsing failures will be
 *    reported.  (For example, if you intend to try opening several
 *    files in a row to find the correct file, verbose should be 0 to
 *    avoid several reports of open failures.)
 *
 *    NOTE: This function should be called single-threaded.
 */
int
sksiteConfigure(
    int                 verbose);

/**
 *    Set the path to the SiLK site configuration file.  Return 0
 *    unless 'filename' is too long or if the caller is attempting to
 *    invoke the function after the config file has been read.
 */
int
sksiteSetConfigPath(
    const char         *filename);

/**
 *    Fill 'buffer' with the path to the site configuration file.
 *    'buffer' is a character array whose length is 'bufsize'.
 *    Returns a pointer to 'buffer', or NULL if 'buffer' is too small
 *    to hold the path.
 *
 *    If the application has been configured, this function returns
 *    the path that sksiteConfigure() used.  Otherwise, the function
 *    returns the path that would be used by sksiteConfigure().
 */
char *
sksiteGetConfigPath(
    char               *buffer,
    size_t              bufsize);

/**
 *    Destroy all the data structures and deallocate all memory
 *    associated with the site.
 *
 *    Application writers do not need to call this function as it is
 *    called by skAppUnregister().
 */
void
sksiteTeardown(
    void);

/** Iterators *********************************************************/

/**
 *    Iterators for sensors
 */
typedef struct sensor_iter_st       sensor_iter_t;

/**
 *    Iterators for classes
 */
typedef struct class_iter_st        class_iter_t;

/**
 *    Iterators for sensor groups
 */
typedef struct sensorgroup_iter_st  sensorgroup_iter_t;

/**
 *    Iterators for flowtypes
 */
typedef struct flowtype_iter_st     flowtype_iter_t;

/**
 *  more_data = sksite<THING>InteratorNext(iter, &out_<THING>_id);
 *
 *    These take a pointer to an iterator and a pointer to the type
 *    being iterated over.  If the iterator is empty, 0 is returned.
 *    Otherwise, out_<THING>_id is set to the next valid ID and 1 is
 *    returned.
 */
int
sksiteSensorIteratorNext(
    sensor_iter_t      *iter,
    sensorID_t         *out_sensor_id);

int
sksiteClassIteratorNext(
    class_iter_t       *iter,
    classID_t          *out_class_id);

int
sksiteSensorgroupIteratorNext(
    sensorgroup_iter_t *iter,
    sensorgroupID_t    *out_sensorgroup_id);

int
sksiteFlowtypeIteratorNext(
    flowtype_iter_t    *iter,
    flowtypeID_t       *out_flowtype_id);


/*
 *    Iterators should be created on the stack, and their internal
 *    strucure is visible so they can be created on the stack.
 *    However, the caller should treat the internals as opaque.
 */

struct sensor_iter_st {
    /** vector of candidates */
    sk_vector_t        *si_vector;
    /** position in the vector */
    int                 si_index;
    /** 1 if vector contains pointers, 0 if it contains IDs */
    int                 si_contains_pointers;
};

struct class_iter_st {
    /** vector of candidates */
    sk_vector_t        *ci_vector;
    /** position in the vector */
    int                 ci_index;
    /** 1 if vector contains pointers, 0 if it contains IDs */
    int                 ci_contains_pointers;
};

struct sensorgroup_iter_st {
    /** vector of candidates */
    sk_vector_t        *gi_vector;
    /** position in the vector */
    int                 gi_index;
    /** 1 if vector contains pointers, 0 if it contains IDs */
    int                 gi_contains_pointers;
};

struct flowtype_iter_st {
    /** vector of candidates */
    sk_vector_t        *fi_vector;
    /** position in the vector */
    int                 fi_index;
    /** 1 if vector contains pointers, 0 if it contains IDs */
    int                 fi_contains_pointers;
};


/** Sensors ***********************************************************/

/**
 *    Create the sensor 'sensor_name' with id 'sensor_id'.  It is an
 *    error to create a sensor with an ID that is already allocated.
 *    If any error occurs (sensor ID already in use, illegal sensor
 *    name, out of memory), returns -1.  Otherwise returns 0 on
 *    success.
 */
int
sksiteSensorCreate(
    sensorID_t          sensor_id,
    const char         *sensor_name);

/**
 *    Find the sensor ID for a sensor given its name.  Returns
 *    SK_INVALID_SENSOR if no sensor is found with the given name.
 */
sensorID_t
sksiteSensorLookup(
    const char         *sensor_name);

/**
 *    Return 1 if a sensor with the given sensor ID exists, 0 if no
 *    such sensor has been defined.
 */
int
sksiteSensorExists(
    sensorID_t          sensor_id);

/**
 *    Returns the minimum sensor ID that has been allocated to a
 *    sensor.  Returns (sensorID_t)(-1) if no sensors yet exist.
 */
sensorID_t
sksiteSensorGetMinID(
    void);

/**
 *    Returns the maximum sensor ID that has been allocated to a
 *    sensor.  Add one to this value to get the lowest ID that is
 *    certain not to have been allocated.  (Note that not all IDs up
 *    to the maximum may have been allocated.)  Returns
 *    (sensorID_t)(-1) if no sensors yet exist.
 */
sensorID_t
sksiteSensorGetMaxID(
    void);

/**
 *    Returns the length of the longest currently known sensor name,
 *    or a minimum value.  This result is suitable for choosing the
 *    size of a display column for sensor names and/or IDs.
 */
size_t
sksiteSensorGetMaxNameStrLen(
    void);

/**
 *    Get the name of the sensor with the given sensor ID into the
 *    given buffer of size buffer_size.  If the name is longer than
 *    buffer_size, the value returns is truncated with a '\0' in the
 *    final position.
 *
 *    Returns the number of characters that would have been written if
 *    the buffer had been long enough.
 */
int
sksiteSensorGetName(
    char               *buffer,
    size_t              buffer_size,
    sensorID_t          sensor_id);

/**
 *    Returns 1 if the sensor with ID sensor_id is defined to be in
 *    the class with ID class_id.  Returns 0 otherwise.
 */
int
sksiteIsSensorInClass(
    sensorID_t          sensor_id,
    classID_t           class_id);

/**
 *    Sets iter to be an iterator that loops over all defined sensors.
 */
void
sksiteSensorIterator(
    sensor_iter_t      *iter);

/**
 *    Sets iter to be an iterator that loops over all of the classes
 *    that are possessed by the given sensor.
 */
void
sksiteSensorClassIterator(
    sensorID_t          sensor_id,
    class_iter_t       *iter);

/**
 *    Returns 0 if the given name is a legal sensor name containing
 *    no illegal characters and of the proper length.  Returns -1 if
 *    the name does not begin with an alpha character, -2 if the name
 *    is too short, -3 if it is too long.  A positive return value
 *    indicates that the character at position n is invalid.
 */
int
sksiteSensorNameIsLegal(
    const char         *name);

/**
 *    Returns the number of classes that the given sensor belongs to.
 */
int
sksiteSensorGetClassCount(
    sensorID_t          sensor_id);

/**
 *    Returns the description of the sensor.  These are specified in
 *    the silk.conf file and are solely for use by the end-user; the
 *    description can be printed with rwsiteinfo.  Returns NULL if no
 *    sensor has the ID 'sensor_id' or if the description has not been
 *    set.
 */
const char *
sksiteSensorGetDescription(
    sensorID_t          sensor_id);

/**
 *    Sets the description of the given sensor, removing any previous
 *    description.  Returns -1 if 'sensor_id' is not a valid sensor or
 *    on memory allocation errror.  Specify 'sensor_description' as
 *    NULL to clear the current description.
 */
int
sksiteSensorSetDescription(
    sensorID_t          sensor_id,
    const char         *sensor_description);

/** Classes ***********************************************************/

/**
 *    Create the class 'class_name' with id 'class_id'.  It is an
 *    error to create a class with an ID that is already allocated.
 *    If any error occurs (class ID already in use, out of memory),
 *    returns -1.  Otherwise returns 0 on success.
 */
int
sksiteClassCreate(
    classID_t           class_id,
    const char         *class_name);

/**
 *    Sets the default class for fglobbing.  Returns 0 on success, or
 *    -1 if 'class_id' is not a valid class.
 */
int
sksiteClassSetDefault(
    classID_t           class_id);

/**
 *    Returns the default class for fglobbing.
 */
classID_t
sksiteClassGetDefault(
    void);

/**
 *    Find the class ID for a class given its name.  Returns
 *    SK_INVALID_CLASS if no class is found with the given name.
 */
classID_t
sksiteClassLookup(
    const char         *class_name);

/**
 *    Return 1 if a class with the given class ID exists, 0 if no
 *    such class has been defined.
 */
int
sksiteClassExists(
    classID_t           class_id);

/**
 *    Returns the maximum class ID that has been allocated to a class.
 *    Add one to this value to get the lowest ID that is certain not
 *    to have been allocated.  (Note that not all IDs up to the
 *    maximum may have been allocated.)  Returns (classID_t)(-1) if
 *    no classes yet exist.
 */
classID_t
sksiteClassGetMaxID(
    void);

/**
 *    Returns the length of the longest currently known class name, or
 *    a minimum value.  This result is suitable for choosing the size
 *    of a display column for class names and/or IDs.
 */
size_t
sksiteClassGetMaxNameStrLen(
    void);

/**
 *    Get the name of the class with the given class ID into the
 *    given buffer of size buffer_size.  If the name is longer than
 *    buffer_size, the value returns is truncated with a '\0' in the
 *    final position.
 *
 *    Returns the number of characters that would have been written if
 *    the buffer had been long enough.
 */
int
sksiteClassGetName(
    char               *buffer,
    size_t              buffer_size,
    classID_t           class_id);

/**
 *    Adds the given sensor to the given class.  Returns 0 on success,
 *    -1 if an error occurred.
 */
int
sksiteClassAddSensor(
    classID_t           class_id,
    sensorID_t          sensor_id);

/**
 *    Adds every sensor in the given sensorgroup to the given class.
 *    Returns 0 on success, -1 if an error occurred.
 */
int
sksiteClassAddSensorgroup(
    classID_t           class_id,
    sensorgroupID_t     sensorgroup_id);

/**
 *    Sets iter to be an iterator that loops over all defined classes.
 */
void
sksiteClassIterator(
    class_iter_t       *iter);

/**
 *    Sets iter to be an iterator that loops over all sensors in the
 *    given class.
 */
void
sksiteClassSensorIterator(
    classID_t           class_id,
    sensor_iter_t      *iter);

/**
 *    Sets iter to be an iterator that loops over all flowtypes in the
 *    given class.
 */
void
sksiteClassFlowtypeIterator(
    classID_t           class_id,
    flowtype_iter_t    *iter);

/**
 *    Sets iter to be an iterator that loops over all default
 *    flowtypes for the given class.
 */
void
sksiteClassDefaultFlowtypeIterator(
    classID_t           class_id,
    flowtype_iter_t    *iter);

/**
 *    Returns the number of sensors in the given class.
 */
int
sksiteClassGetSensorCount(
    classID_t           class_id);

/**
 *    Adds the given flowtype to the list of default flowtypes for the
 *    given class.  The flowtype should be a part of this class
 *    already.  (i.e. it should have been created with class
 *    class_id.)
 *
 *    Returns 0 on success, -1 on any error.
 */
int
sksiteClassAddDefaultFlowtype(
    classID_t           class_id,
    flowtypeID_t        flowtype_id);

/** Sensorgroups ******************************************************/

/**
 *    Create the group 'sensorgroup_name' with id 'sensorgroup_id'.
 *    It is an error to create a group with an ID that is already
 *    allocated.  If any error occurs (class ID already in use, out of
 *    memory), returns -1.  Otherwise returns 0 on success.
 */
int
sksiteSensorgroupCreate(
    sensorgroupID_t     sensorgroup_id,
    const char         *sensorgroup_name);

/**
 *    Find the sensorgroup ID for a group given its name.  Returns
 *    SK_INVALID_SENSORGROUP if no group is found with the given name.
 */
sensorgroupID_t
sksiteSensorgroupLookup(
    const char         *sensorgroup_name);

/**
 *    Return 1 if a sensorgroup with the given ID exists, 0 if no such
 *    group has been defined.
 */
int
sksiteSensorgroupExists(
    sensorgroupID_t     sensorgroup_id);

/**
 *    Returns the maximum ID that has been allocated to a sensorgroup.
 *    Add one to this value to get the lowest ID that is certain not
 *    to have been allocated.  (Note that not all IDs up to the
 *    maximum may have been allocated.)  Returns
 *    (sensorgroupID_t)(-1) if no classes yet exist.
 */
sensorgroupID_t
sksiteSensorgroupGetMaxID(
    void);

/**
 *    Returns the length of the longest currently known group name, or
 *    a minimum value.  This result is suitable for choosing the size
 *    of a display column for group names and/or IDs.
 */
size_t
sksiteSensorgroupGetMaxNameStrLen(
    void);

/**
 *    Get the name of the group with the given ID into the given
 *    buffer of size buffer_size.  If the name is longer than
 *    buffer_size, the value returns is truncated with a '\0' in the
 *    final position.
 *
 *    Returns the number of characters that would have been written if
 *    the buffer had been long enough.
 */
int
sksiteSensorgroupGetName(
    char               *buffer,
    size_t              buffer_size,
    sensorgroupID_t     sensorgroup_id);

/**
 *    Adds the given sensor to the given class.  Returns 0 on success,
 *    -1 if an error occurred.
 */
int
sksiteSensorgroupAddSensor(
    sensorgroupID_t     sensorgroup_id,
    sensorID_t          sensor_id);

/**
 *    Adds every sensor in the sensorgroup 'src' to the sensorgroup
 *    'dest'.  Returns 0 on success, -1 if an error occurred.
 */
int
sksiteSensorgroupAddSensorgroup(
    sensorgroupID_t     dest,
    sensorgroupID_t     src);

/**
 *    Sets iter to be an iterator that loops over all defined
 *    sensorgroups.
 */
void
sksiteSensorgroupIterator(
    sensorgroup_iter_t *iter);

/**
 *    Sets iter to be an iterator that loops over all sensors in the
 *    given sensorgroup.
 */
void
sksiteSensorgroupSensorIterator(
    sensorgroupID_t     sensorgroup_id,
    sensorgroup_iter_t *iter);

/** Flowtypes *********************************************************/

/**
 *    Flowtypes represent individual class/type pairs.  Flowtype IDs
 *    are actually recorded in files, while class IDs and types are
 *    not.
 */

/**
 *    Create the flowtype 'flowtype_name' with id 'flowtype_id'.
 *    Associate it with the given class ID and type name.  It is an
 *    error to create a flowtype with an ID that is already allocated.
 *    If any error occurs (class ID already in use, out of memory),
 *    returns -1.  Otherwise returns 0 on success.
 */
int
sksiteFlowtypeCreate(
    flowtypeID_t        flowtype_id,
    const char         *flowtype_name,
    classID_t           class_id,
    const char         *type_name);

/**
 *    Find the flowtype ID for a flowtype given its name.  Returns
 *    SK_INVALID_FLOWTYPE if no flowtype is found with the given name.
 */
flowtypeID_t
sksiteFlowtypeLookup(
    const char         *flowtype_name);

/**
 *    Find the flowtype ID for a flowtype given its class and type.
 *    Returns SK_INVALID_FLOWTYPE if no flowtype is found with the
 *    given class and type.
 */
flowtypeID_t
sksiteFlowtypeLookupByClassType(
    const char         *class_name,
    const char         *type_name);

/**
 *    Find the flowtype ID for a flowtype given its class and type.
 *    Returns SK_INVALID_FLOWTYPE if no flowtype is found with the
 *    given class and type.
 */
flowtypeID_t
sksiteFlowtypeLookupByClassIDType(
    classID_t           class_id,
    const char         *type_name);

/**
 *    Return 1 if a flowtype with the given ID exists, 0 if no such
 *    flowtype has been defined.
 */
int
sksiteFlowtypeExists(
    flowtypeID_t        flowtype_id);

/**
 *    Returns the maximum flowtype ID that has been allocated.  Add
 *    one to this value to get the lowest ID that is certain not to
 *    have been allocated.  (Note that not all IDs up to the maximum
 *    may have been allocated.)  Returns (flowtypeID_t)(-1) if no
 *    flowtypes yet exist.
 */
flowtypeID_t
sksiteFlowtypeGetMaxID(
    void);

/**
 *    Get the class name of the flowtype with the given ID into the
 *    given buffer of size buffer_size.  If the name is longer than
 *    buffer_size, the value returns is truncated with a '\0' in the
 *    final position.
 *
 *    Returns the number of characters that would have been written if
 *    the buffer had been long enough.
 */
int
sksiteFlowtypeGetClass(
    char               *buffer,
    size_t              buffer_size,
    flowtypeID_t        flowtype_id);

/**
 *    Returns the class ID of this flowtype's class.  Returns
 *    SK_INVALID_CLASS if the flowtype does not exist.
 */
classID_t
sksiteFlowtypeGetClassID(
    flowtypeID_t        flowtype_id);

/**
 *    Returns the length of the longest currently known flowtype name,
 *    or a minimum value.  This result is suitable for choosing the
 *    size of a display column for flowtype names and/or IDs.
 */
size_t
sksiteFlowtypeGetMaxNameStrLen(
    void);

/**
 *    Get the name of the flowtype with the given ID into the given
 *    buffer of size buffer_size.  If the name is longer than
 *    buffer_size, the value returns is truncated with a '\0' in the
 *    final position.
 *
 *    Returns the number of characters that would have been written if
 *    the buffer had been long enough.
 */
int
sksiteFlowtypeGetName(
    char               *buffer,
    size_t              buffer_size,
    flowtypeID_t        flowtype_id);

/**
 *    Returns the length of the longest currently known flowtype type,
 *    or a minimum value.  This result is suitable for choosing the
 *    size of a display column for flowtype types.
 */
size_t
sksiteFlowtypeGetMaxTypeStrLen(
    void);

/**
 *    Get the type of the flowtype with the given ID into the given
 *    buffer of size buffer_size.  If the name is longer than
 *    buffer_size, the value returns is truncated with a '\0' in the
 *    final position.
 *
 *    Returns the number of characters that would have been written if
 *    the buffer had been long enough.
 */
int
sksiteFlowtypeGetType(
    char               *buffer,
    size_t              buffer_size,
    flowtypeID_t        flowtype_id);

/**
 *    Sets iter to be an iterator that loops over all defined
 *    flowtypes.
 */
void
sksiteFlowtypeIterator(
    flowtype_iter_t    *iter);

/**
 *    Asserts that the given flowtype exists, and is associated with
 *    the given class name and class type.  If this is not the case,
 *    an error message is printed and the program abort()s.  The
 *    'pack_logic_file' is the path to the file doing the assertion.
 */
void
sksiteFlowtypeAssert(
    const char         *pack_logic_file,
    flowtypeID_t        flowtype_id,
    const char         *class_name,
    const char         *type);

/** File Formats ******************************************************/

/**
 *    Get the name of the file format with the given ID into the given
 *    buffer of size buffer_size.  If the name is longer than
 *    buffer_size, the value returns is truncated with a '\0' in the
 *    final position.
 *
 *    Returns the number of characters that would have been written if
 *    the buffer had been long enough.
 */
int
sksiteFileformatGetName(
    char               *buffer,
    size_t              buffer_size,
    fileFormat_t        format_id);

/**
 *    Return 1 if 'format_id' is a valid file output format, or 0 if
 *    it is not.
 */
int
sksiteFileformatIsValid(
    fileFormat_t        format_id);

/**
 *    Returns the file output format associated with the name.  If the
 *    name is unknown, will return an invalid file format.
 */
fileFormat_t
sksiteFileformatFromName(
    const char         *name);


/*** Compression Methods ***********************************************/

/**    values returned by sksiteCompmethodCheck() */
#define SK_COMPMETHOD_IS_AVAIL  6
#define SK_COMPMETHOD_IS_VALID  2
#define SK_COMPMETHOD_IS_KNOWN  1

/**
 *    Check whether a compression method is valid and/or available.
 *
 *    If the compression method 'comp_method' is completely
 *    unrecognized, return 0.
 *
 *    Return SK_COMPMETHOD_IS_KNOWN when 'comp_method' is an
 *    "undecided" value (i.e., SK_COMPMETHOD_DEFAULT or
 *    SK_COMPMETHOD_BEST).  These compression methods should be
 *    considered valid for writing, as they will be converted to an
 *    appropriate type once the stream they are connected to is
 *    opened.
 *
 *    Return SK_COMPMETHOD_IS_VALID when 'comp_method' contains a
 *    known value other than an "undecided" value, but the compression
 *    method relies on an external library that is not part of this
 *    build of SiLK.
 *
 *    Return SK_COMPMETHOD_IS_AVAIL when 'comp_method' is a known
 *    value whose library is available.  These compression methods are
 *    valid for reading or for writing.
 *
 *    To determine whether 'comp_method' is valid for read, mask the
 *    output by 4.  To determine whether 'comp_method' is valid for
 *    write, mask the output of this function by 5. To determine
 *    whether 'comp_method' is an actual compression method (that is,
 *    not an "undecided" value), mask the output by 2.
 */
int
sksiteCompmethodCheck(
    sk_compmethod_t     comp_method);

/**
 *    Return the generically "best" compression method from all those
 *    that are available.
 */
sk_compmethod_t
sksiteCompmethodGetBest(
    void);

/**
 *    Return the default compression method.
 */
sk_compmethod_t
sksiteCompmethodGetDefault(
    void);

/**
 *    Given the compress method 'comp_method', write the name of that
 *    method into 'out_buffer' whose length is 'bufsize'.  The
 *    function returns a pointer to 'out_buffer', or NULL for an
 *    invalid compression method.
 */
int
sksiteCompmethodGetName(
    char               *buffer,
    size_t              buffer_size,
    sk_compmethod_t     comp_method);

/**
 *    Sets the default compression method.  Returns 0 on success, -1
 *    if the method is not available.
 */
int
sksiteCompmethodSetDefault(
    sk_compmethod_t     compression_method);

/**
 *    Add an option that will allow the user to set the compression
 *    method of binary output files.  After skOptionsParse()
 *    sucessfully returns, the variable pointed to by
 *    'compression_method' will contain the compression method to use.
 *
 *    If the user does not exercise the compression-method option,
 *    'compression_method' will be set to the default compression
 *    method.
 */
int
sksiteCompmethodOptionsRegister(
    sk_compmethod_t    *compression_method);

/**
 *    Include the compression method option in the list of options for
 *    --help output.
 */
void
sksiteCompmethodOptionsUsage(
    FILE               *fh);

/** Paths *************************************************************/

/**
 *    Return the default value for the data root directory that was
 *    specified when SiLK was built.
 *
 *    This function ignores any environment settings, and it should
 *    only be used to get the default.  This function may be called
 *    without initializing the site.
 */
const char *
sksiteGetDefaultRootDir(
    void);


/**
 *    Fill 'buffer' with the data root directory.  'buffer' is a
 *    character array whose length is 'bufsize'.  Returns a pointer to
 *    'buffer', or NULL if 'buffer' is too small to hold the root
 *    directory.
 */
char *
sksiteGetRootDir(
    char               *buffer,
    size_t              bufsize);

/**
 *    Sets the data root directory to 'rootdir', overriding the value
 *    that was set when SiLK was configured.  Returns 0 on success, or
 *    -1 if 'rootdir' is NULL, the empty string, or longer than
 *    PATH_MAX characters.
 */
int
sksiteSetRootDir(
    const char         *rootdir);

/**
 *    Sets the path format for output files, overriding the current
 *    value.  Returns 0 on success, or -1 if 'format' is NULL, the
 *    empty string, or longer than PATH_MAX characters.
 *
 *    NOTE: At the moment, all path formats are required to end in %x,
 *    so that filenames have the default format.  This restriction may
 *    be lifted in the future.
 *
 *    The following strings have special meaning in path formats:
 *
 *     %C - Class name
 *     %F - Flowtype name
 *     %H - Hour, two digits, zero-padded
 *     %N - Sensor name
 *     %T - Type name
 *     %Y - Year, four digits, zero-padded
 *     %d - Day of month, two digits, zero-padded
 *     %f - Flowtype ID
 *     %m - Month of year, two digits, zero-padded
 *     %n - Sensor ID
 *     %x - Default filename, which is the same as "%F-%N_%Y%m%d.%H"
 *     %% - a single percent sign
 *
 *    The default path format is "%T/%Y/%m/%d/%x"
 */
int
sksiteSetPathFormat(
    const char         *format);


/**
 *    Fill 'buffer' with the path to the packing logic file.  'buffer'
 *    is a character array whose length is 'bufsize'.  Returns NULL if
 *    the packing-logic path was not set.  Otherwise, returns a
 *    pointer to 'buffer', or NULL if 'buffer' is too small to hold
 *    the value.
 */
char *
sksiteGetPackingLogicPath(
    char               *buffer,
    size_t              bufsize);


/**
 *    Set the packing-logic value to 'pathname'.  The packer,
 *    rwflowpack, may use this value to determine into which class and
 *    type a flow belongs.  Return 0 on success, or -1 on failure.
 */
int
sksiteSetPackingLogicPath(
    const char         *pathname);


/**
 *    Fill in 'buffer', a C-array of size 'bufsize', with the complete
 *    path of the file having the specified 'flowtype_id',
 *    'sensor_id', and 'timestamp'.  The name will be of the form
 *    specified by the current path format.
 *
 *    When 'suffix' is non-NULL and not the empty string, it will be
 *    appended to the name of the generated file.  If 'suffix' does
 *    not begin with a period ("."), one will be added between the
 *    filename and the suffix.
 *
 *    When 'reldir_begin' is non-NULL, its value is set to the
 *    beginning of the relative directory; i.e., to the character
 *    following the '/' that ends the root-directory.  For the default
 *    path format of "%T/%Y/%m/%d/%x", 'reldir_begin' would point to
 *    the start of the type name (%T).
 *
 *    When 'filename_begin' is non-NULL, its value is set to the
 *    beginning of the filename; i.e., to the character following the
 *    final '/'.
 *
 *    Return a pointer to 'buffer' or NULL on error.
 */
char *
sksiteGeneratePathname(
    char               *buffer,
    size_t              bufsize,
    flowtypeID_t        flowtype,
    sensorID_t          sensor,
    sktime_t            timestamp,
    const char         *suffix,
    char              **reldir_begin,
    char              **filename_begin);


/**
 *    Extract the flowtype, sensor, and timestamp from 'filename', the
 *    name of or a path to a SiLK Flow file in the "%x" format, and
 *    put the values into 'out_flowtype', 'out_sensor' and
 *    'out_timestamp', respectively.  Any of those values may be NULL,
 *    indicating that the caller does not want the value returned.
 *    The function returns the flowtype, or SK_INVALID_FLOWTYPE if the
 *    name is not in the proper form or if the flowtype is not
 *    recognized.  An unrecognized sensor name is allowed.
 *
 *    The standard format for the name of a SiLK flow file is:
 *
 *    flowtypeString-SensorName_YYYYMMDD.HH[.*]
 *
 *    with a hyphen separating the flowtype from the sensor, and an
 *    underscore separating the sensor from the time.  The filename
 *    may include a suffix made up of a period and additional text.
 *    When 'filename' has a suffix and 'out_suffix' is non-NULL,
 *    'out_suffix' will be set to the location of the '.' in
 *    'filename' that begins the suffix.
 *
 *    The function will take the basename of 'filename', so 'filename'
 *    may include directory components.
 *
 */
flowtypeID_t
sksiteParseFilename(
    flowtypeID_t       *out_flowtype,
    sensorID_t         *out_sensor,
    sktime_t           *out_timestamp,
    const char        **out_suffix,
    const char         *filename);

/**
 *    Extract the flowtype, sensor, and timestamp from 'filename', the
 *    name of a SiLK Flow file, and use those values to fill in
 *    'buffer', a C-array of size 'bufsize', with the complete path of
 *    the file having that flowtype, sensor, and timestamp.
 *
 *    Returns a pointer to 'buffer' on success, or NULL if an error
 *    occurs or if the filename is not of the expected form.
 *
 *    When 'suffix' is NULL and 'filename' has a suffix, the path name
 *    put into 'buffer' will have the same suffix appended.  To have
 *    the suffix removed from the new path, pass in the empty string
 *    as the value of 'suffix'.
 *
 *    The parameters 'buffer' and 'filename' may point to the same
 *    location.
 *
 *    This function works as a convenience wrapper around
 *    sksiteParseFilename() and sksiteGeneratePathname().  See the
 *    documentation of those functions for additional information.
 */
char *
sksiteParseGeneratePath(
    char               *buffer,
    size_t              bufsize,
    const char         *filename,
    const char         *suffix,
    char              **reldir_begin,
    char              **filename_begin);

/** Special Support Functions *****************************************/

/**
 *    Treats 'string' as a comma separated list of tokens representing
 *    the NAMES of sensors---does not accept sensor IDs.  For each
 *    token that is a valid sensor name, the function appends an
 *    element to the 'out_sensors' vector, where the element holds the
 *    sensorID_t that the token maps to.
 *
 *    Returns the number of matches found, 0 for no matches, or -1 for
 *    memory error, NULL input, or if the vector's element's size is
 *    not sizeof(sensorID_t).
 *
 *    If out_sensors is NULL, the function still returns the number of
 *    matches found.
 */
int
sksiteParseSensorList(
    sk_vector_t        *out_sensors,
    const char         *string);


typedef enum sksite_validate_enum_en {
    /** No delimiter present in class/type pair */
    SKSITE_ERR_MISSING_DELIM,
    /** unknown class name */
    SKSITE_ERR_UNKNOWN_CLASS,
    /** unknown type name */
    SKSITE_ERR_UNKNOWN_TYPE,
    /** unknown sensor name */
    SKSITE_ERR_UNKNOWN_SENSOR,
    /** unknown sensor numeric id */
    SKSITE_ERR_UNKNOWN_SENSOR_ID,
    /** unknown type for given class */
    SKSITE_ERR_TYPE_NOT_IN_CLASS,
    /** sensor not available in given class(es) */
    SKSITE_ERR_SENSOR_NOT_IN_CLASS
} sksite_validate_enum_t;


typedef struct sksite_error_iterator_st sksite_error_iterator_t;


/**
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
 *    remain valid while iterating over the errors.  The caller must
 *    invoke sksiteErrorIteratorFree() to free the iterator.
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
    sksite_error_iterator_t   **error_iter);


/**
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
 *    remain valid while iterating over the errors.  The caller must
 *    invoke sksiteErrorIteratorFree() to free the iterator.
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
    sksite_error_iterator_t   **invalid_sensors_vec);


/**
 *    Moves the iterator to the next error, or initializes the
 *    iterator returned by sksiteValidateFlowtypes() or
 *    sksiteValidateSensors().
 *
 *    The function returns SK_ITERATOR_OK if there are more errors, or
 *    SK_ITERATOR_NO_MORE_ENTRIES if all errors have been seen.
 */
int
sksiteErrorIteratorNext(
    sksite_error_iterator_t    *iter);


/**
 *    Frees all memory associated with the iterator.
 */
void
sksiteErrorIteratorFree(
    sksite_error_iterator_t    *iter);


/**
 *    Resets the iterator so that it may be iterated over again.
 */
void
sksiteErrorIteratorReset(
    sksite_error_iterator_t    *iter);


/**
 *    Returns the error code associated with the current error.  The
 *    error code is a value from sksite_validate_enum_t.  This
 *    function does not advance the iterator.
 */
int
sksiteErrorIteratorGetCode(
    const sksite_error_iterator_t  *iter);


/**
 *    Returns the current token---that is, the class/type pair or the
 *    sensor that was found to be invalid.  This token is a pointer
 *    into the character pointer array that was passed to
 *    sksiteValidateFlowtypes() or sksiteValidateSensors().  This
 *    function does not advance the iterator.
 */
const char *
sksiteErrorIteratorGetToken(
    const sksite_error_iterator_t  *iter);


/**
 *    Returns a message explaining the current error.  The message
 *    will include the token---that is, the class/type pair or the
 *    sensor that was found to be invalid.  This function does not
 *    advance the iterator.
 */
const char *
sksiteErrorIteratorGetMessage(
    const sksite_error_iterator_t  *iter);



/** DATA_ROOTDIR Repository Iteration (fglob) *************************/

typedef struct sksite_repo_iter_st sksite_repo_iter_t;

typedef struct sksite_fileattr_st {
    sktime_t        timestamp;
    sensorID_t      sensor;
    flowtypeID_t    flowtype;
    /* debating whether to include a uint32_t here for "user_data".
     * On 64bit platform, the struct will be padded by an additional 4
     * bytes regardless.  The additional value could be used as a
     * record offset if we ever wanted to provide a pointer to an
     * individual record in a file, or it could be used for
     * information about the location of the file. */
} sksite_fileattr_t;


#define RETURN_MISSING (1 << 0)


/**
 *    Create a new iterator that will return files from the data
 *    store.  The iterator will be created at the location specified
 *    by 'iter'.
 *
 *    Each file in the data store is located by a triple comprised of
 *    the flowtype, sensor, and start-hour.  The list of flowtypes to
 *    iterate over is specified by 'ft_vec', a vector containing
 *    flowtypeID_t's.  The list of sensors to iterate over is
 *    specified by 'sen_vec', a vector containing sensorID_t's.  If
 *    'sen_vec' is NULL, the iterator uses all sensors that are valid
 *    for the given flowtypes.  The time range to iterate over is
 *    specified by 'start_time' to 'end_time'.
 *
 *    This function makes no effort to ensure that the values in
 *    'ft_vec' and 'sen_vec' are unique.  Duplicate values in those
 *    arrays will cause a file to be visited multiple times.
 *
 *    'flags' contain addition information.  Normally, the iterator
 *    does not return files that do not exist in the repository.
 *    However, when RETURN_MISSING is set in 'flags', the iterator
 *    will return missing files in addition to existing files.
 *
 *    This function returns 0 on success, or -1 for failure.  Failures
 *    include: 'iter' being NULL; 'ft_vec' being NULL or having the
 *    wrong sized elements; 'sen_vec' having the wrong sized elements;
 *    'end_time' less than 'start_time'; memory allocation error.
 *
 *    The caller may use any combination of
 *    sksiteRepoIteratorGetFileattrs(),
 *    sksiteRepoIteratorRemaingFileattrs(), or the various
 *    sksiteRepoIteratorNext*() functions to iterate over the files.
 *    Note that each call to one of these functions moves the
 *    iterator.
 *
 *    The caller should use 'sksiteRepoIteratorDestroy()' to destroy
 *    the iterator once she has finished with the iterator.
 */
int
sksiteRepoIteratorCreate(
    sksite_repo_iter_t    **iter,
    const sk_vector_t      *flowtypes_vec,
    const sk_vector_t      *sensor_vec,
    sktime_t                start_time,
    sktime_t                end_time,
    uint32_t                flags);


/**
 *    Destroy an iterator created by sksiteRepoIteratorCreate(), and
 *    set 'iter' to NULL.  The function is a no-op if 'iter' or
 *    '*iter' is NULL.
 */
void
sksiteRepoIteratorDestroy(
    sksite_repo_iter_t    **iter);


/**
 *    Fill 'attr_array' with the next 'max_count' file attributes read
 *    from the iterator 'iter'.  Return the number of file attributes
 *    added to 'attr_array'.  A number less than 'max_count' indicates
 *    all files have been iterated over.
 *
 *    This function does not provide any mechanism for the caller to
 *    determine whether an entry in the 'attr_array' represents a
 *    missing file, when 'iter' is iterating over missing files.
 */
size_t
sksiteRepoIteratorGetFileattrs(
    sksite_repo_iter_t *iter,
    sksite_fileattr_t  *attr_array,
    size_t              attr_max_count);

/**
 *    Put the file attributes of the next file from the data store
 *    into the location specified by 'file_attr'.  If 'is_missing' is
 *    not-NULL, it will be set to 0 if the file exists in the data
 *    store, or 1 if the file is missing from the data store.
 *
 *    Return SK_ITERATOR_OK when a file exists, or
 *    SK_ITERATOR_NO_MORE_ENTRIES when the iterator has visited all
 *    files.
 */
int
sksiteRepoIteratorNextFileattr(
    sksite_repo_iter_t *iter,
    sksite_fileattr_t  *fileattr,
    int                *is_missing);

/**
 *    Put the file path of the next file from the data store into the
 *    location specified by 'path'.  The value 'path_len' should
 *    indicate the number of characters 'path' can hold.  If
 *    'is_missing' is not-NULL, it will be set to 0 if the file exists
 *    in the data store, or 1 if the file is missing from the data
 *    store.
 *
 *    Return SK_ITERATOR_OK when a file exists, or
 *    SK_ITERATOR_NO_MORE_ENTRIES when the iterator has visited all
 *    files.
 */
int
sksiteRepoIteratorNextPath(
    sksite_repo_iter_t *iter,
    char               *path,
    size_t              path_len,
    int                *is_missing);

/**
 *    Create a new stream holding the next file in the data store at
 *    the location specified by 'stream'.  If the file exists, the
 *    stream will be opened at the header of the file will be read.
 *    If 'is_missing' is not-NULL, it will be set to 0 if the file
 *    exists in the data store, or 1 if the file is missing from the
 *    data store.
 *
 *    If a file exists in the data store but there is an error opening
 *    the file, the stream is not returned by the iterator.  If 'err_fn'
 *    is not-NULL, a message will be printed using that function.
 *
 *    Return SK_ITERATOR_OK when a file exists, or
 *    SK_ITERATOR_NO_MORE_ENTRIES when the iterator has visited all
 *    files.
 */
int
sksiteRepoIteratorNextStream(
    sksite_repo_iter_t     *iter,
    skstream_t            **stream,
    int                    *is_missing,
    sk_msg_fn_t             err_fn);

/**
 *    Append all remaining file attributes to the 'attr_vec' vector.
 *    'attr_vec' should be a vector where the element size is
 *    sizeof(sksite_fileattr_t).  Return 0 on success, or -1 on
 *    failure.
 *
 *    This function does not provide any mechanism for the caller to
 *    determine whether an entry in the 'attr_vec' represents a
 *    missing file, when 'iter' is iterating over missing files.
 */
int
sksiteRepoIteratorRemainingFileattrs(
    sksite_repo_iter_t *iter,
    sk_vector_t        *fileattr_vec);

/**
 *    Reset the state of the iterator 'iter' so that all files may be
 *    iterated over again.
 */
void
sksiteRepoIteratorReset(
    sksite_repo_iter_t *iter);


#ifdef __cplusplus
}
#endif
#endif /* _SKSITE_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
