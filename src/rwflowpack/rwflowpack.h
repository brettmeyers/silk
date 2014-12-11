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
**  rwflowpack.h
**
**    This header defines the structure of function pointers used by a
**    packing-logic plug-in for rwflowpack.
**
*/
#ifndef _RWFLOWPACK_H
#define _RWFLOWPACK_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWFLOWPACK_H, "$SiLK: rwflowpack.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/silk_types.h>
#include <silk/libflowsource.h>

/**
 *  @file
 *
 *    Interface between rwflowpack and the packing logic plug-in that
 *    is used to decide how to pack each flow record.
 *
 *    This file is part of libflowsource.
 */


/**
 *    The maximum number of flowtype/sensors that a single flow can be
 *    packed to at one time.  Used to set array sizes.
 */
#define MAX_SPLIT_FLOWTYPES 16


/**
 *    Name of the function that rwflowpack calls when the plug-in is
 *    first loaded.
 */
#define SK_PACKLOGIC_INIT  "packLogicInitialize"


/**
 *    packing logic plug-in
 */
typedef struct packlogic_plugin_st packlogic_plugin_t;
struct packlogic_plugin_st {
    /**
     *    handle returned by dlopen()
     */
    void           *handle;

    /**
     *    path to the plugin
     */
    char           *path;

    /**
     *  Site-specific initialization function called when the plug-in
     *  is first loaded during options processing.  This funciton is
     *  called with this structure as its argument; it should set the
     *  function pointers listed below.
     */
    int           (*initialize_fn)(packlogic_plugin_t *packlogic);

    /**
     *  Site-specific setup function, called after the site
     *  configuration file (silk.conf) has been loaded but before
     *  parsing the sensor.conf file.
     */
    int           (*setup_fn)(void);

    /**
     *  Site-specific teardown function.
     */
    void          (*teardown_fn)(void);


    /**
     *  Site-specific function to verify that a sensor has all the
     *  information it requires to pack flow records.
     */
    int           (*verify_sensor_fn)(skpc_sensor_t *sensor);

    /**
     *  A function that determines the flow type(s) and sensorID(s) of
     *  a flow record 'rwrec' and was collected from the 'probe'.  The
     *  function will compare the SNMP interfaces on the record with
     *  those specified in the probe for external, internal, and null
     *  flows.
     *
     *  'ftypes' and 'sensorids' should each be arrays having enough
     *  space to hold the expected number of flow_types and sensorIDs;
     *  NUM_FLOW_TYPES is the maximum that could be returned.  These
     *  arrays will be populated with the flow_type and sensorID
     *  pair(s) into which the record should be packed.
     *
     *  Excepting errors, the return value is always the number of
     *  flow_type/sensorID pairs into which the record should be
     *  packed.
     *
     *  A return value of 0 indicates no packing rules existed for
     *  this record from this probe; a value of -1 indicates an error
     *  condition.
     */
    int           (*determine_flowtype_fn)(const skpc_probe_t   *probe,
                                           const rwRec          *rwrec,
                                           flowtypeID_t         *ftypes,
                                           sensorID_t           *sensorids);

    /**
     *  A function that determines the file format to use for records
     *  whose flowtype is 'ftype'.  The 'probe' parameter contains the
     *  probe where records are collected.
     */
    fileFormat_t  (*determine_fileformat_fn)(const skpc_probe_t *probe,
                                             flowtypeID_t        ftype);
};


/**
 *    Function that must exist in the packing logic plug-in.  This
 *    function should set the function pointers on 'packlogic'.
 */
int
packLogicInitialize(
    packlogic_plugin_t *packlogic);


#ifdef __cplusplus
}
#endif
#endif /* _RWFLOWPACK_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
