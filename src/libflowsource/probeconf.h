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
**  probeconf.h
**
**    Functions to parse a probe configuration file and use the
**    results.
**
*/

#ifndef _PROBECONF_H
#define _PROBECONF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_PROBECONF_H, "$SiLK: probeconf.h cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/silk_types.h>

/**
 *  @file
 *
 *    Functions to parse a probe configuration file and use the
 *    results.
 *
 *    This file is part of libflowsource.
 *
 *
 *    Lifecycle:
 *
 *    The application calls skpcSetup() to initialize the
 *    skpc data structures and memory.
 *
 *    The application should call skpcParse() to parse the
 *    application's configuration file.  skpcParse() will create
 *    sensors (if any) and probes.  The probes are created and checked
 *    for validity--this means they have all the data they require.
 *    If valid they are added to the list maintained by the skpc.
 *    If not valid, they are destroyed.
 *
 *    Once the probes have been created, the application can use
 *    skpcProbeIteratorBind() and skpcProbeIteratorNext() to process
 *    each probe.
 *
 *    Finally, the application calls skpcTeardown() to destroy
 *    the probes, sensors, and to fee all memory.
 *
 *    Note that skpc allows one to create a "temporary" sensor;
 *    i.e., a sensor that will only exist as long as the application
 *    is running; this is useful for testing a new sensor without
 *    requiring a complete recompile of SiLK.  However, "temporary"
 *    sensors will NOT be available to the analysis applications.  For
 *    the analysis applications to know about a sensor, it MUST be
 *    listed in the sensorInfo[] array.
 */


/**
 *    Values for the type of a probe.
 */
typedef enum {
    PROBE_ENUM_INVALID = 0,
    PROBE_ENUM_IPFIX = 10,
    PROBE_ENUM_NETFLOW_V5 = 5,
    PROBE_ENUM_NETFLOW_V9 = 9,
    PROBE_ENUM_SFLOW = 16,
    PROBE_ENUM_SILK = 15
} skpc_probetype_t;


/**
 *    Possible protocols
 */
typedef enum {
    SKPC_PROTO_UNSET = 0,
    SKPC_PROTO_TCP = 1,
    SKPC_PROTO_UDP = 2,
#if 0
    /* not sure if these should be here; we'll decide when we add SSL
     * support */
    SKPC_PROTO_DTLS_SCTP,
    SKPC_PROTO_TLS_TCP,
    SKPC_PROTO_DTLS_UDP,
#endif
    SKPC_PROTO_SCTP = 3
} skpc_proto_t;


/*
 *    Supported "quirks" for special record handling.
 */

/**
 *    Value returned by skpcProbeGetQuirks() to denote that no quirks
 *    are set.
 */
#define SKPC_QUIRK_NONE             0x00

/**
 *    Quirks flag to support checking for firewall event codes, such
 *    as those returned by the Cisco ASA series of routers.
 */
#define SKPC_QUIRK_FW_EVENT         0x01

/**
 *    Quirks flag to support flow records that do not contain a valid
 *    packets field, such as those from the Cisco ASA series of
 *    routers.
 */
#define SKPC_QUIRK_ZERO_PACKETS     0x02

/**
 *    Quirks flag to force processing of NetFlow v9/IPFIX records
 *    whose templates do not contain any IP addresses.
 */
#define SKPC_QUIRK_MISSING_IPS      0x04

/*#define SKPC_QUIRK_     0x08*/
/*#define SKPC_QUIRK_     0x10*/
/*#define SKPC_QUIRK_     0x20*/
/*#define SKPC_QUIRK_     0x40*/
/*#define SKPC_QUIRK_     0x80*/


/**
 *    The type for network ids
 */
typedef size_t skpc_network_id_t;

/**
 *    The maximum possible network ID
 */
#define SKPC_NETWORK_ID_MAX     ((skpc_network_id_t)254)

/**
 *    The invalid network ID
 */
#define SKPC_NETWORK_ID_INVALID ((skpc_network_id_t)255)


/**
 *   Which "side" of the record we look at when testing its flow
 *   interfaces, whether
 *
 *   -- its source is a particular network; i.e., it is COMING FROM an
 *      internet cloud.  For this case, look at its source IP or input
 *      SNMP interface.
 *
 *   -- its destination is a particular network; i.e., it is GOING TO
 *      a cloud.  For this case, look at the destination IP or output
 *      SNMP interface.
 */
typedef enum {
    SKPC_DIR_SRC = 0, SKPC_DIR_DST = 1
} skpc_direction_t;


/**
 *    The "type" of value that the probe stores in the input and
 *    output fields.
 *
 *    A value of 'SKPC_IFVALUE_SNMP' signifies that those fields hold
 *    the index of the interface (ifIndex) where the flows entered and
 *    left the router, respectively.
 *
 *    A value of 'SKPC_IFVALUE_VLAN' signifies that those fields hold
 *    the vlanIds for the source and destination networks,
 *    respectively.  If only vlan Id is available, the 'input' is set
 *    to that value and the 'output' is set to 0.
 */
typedef enum {
    SKPC_IFVALUE_SNMP = 0, SKPC_IFVALUE_VLAN = 1
} skpc_ifvaluetype_t;



/*  Forward declaration */
typedef struct skpc_sensor_st skpc_sensor_t;


/**
 *    The network definition.
 *
 *    Maps a name to an ID.
 */
typedef struct skpc_network_st {
    char               *name;
    skpc_network_id_t   id;
} skpc_network_t;


/**
 *    The probe definition.
 *
 *    A probe tells how to collect data and the type of data.  For
 *    example, IPFIX data from machine 10.10.10.10 as TCP to port
 *    9999.  A probe is associated with one or more sensors.
 */
typedef struct skpc_probe_st {

    /** List of sensors to which this probe belongs, and a count of
     * those sensors */
    skpc_sensor_t     **sensor_list;
    size_t              sensor_count;

    /** The host:port combination on which this probe should listen for
     * data, as an IP address and a port-number. */
    sk_sockaddr_array_t *listen_addr;

    /** The host that this probe should accept connections from. */
    sk_sockaddr_array_t *accept_from_addr;

    /** The unix domain socket on which this probe should listen for
     * data, as a UNIX pathname */
    char               *unix_domain_path;

    /** A file name from which to read flow data */
    char               *file_source;

    /** A directory path name to poll in order to find files from which
     * to read flow data */
    char               *poll_directory;

    /** the name of the probe */
    char               *probe_name;

    /** Probe quirks */
    uint8_t             quirks;

    /** Probe type */
    skpc_probetype_t    probe_type;

    /** Probe protocol */
    skpc_proto_t        protocol;

    /** Probe logging flags */
    uint8_t             log_flags;

    /** Has probe been verified */
    unsigned            verified :1;

    /** Are we storing the vlanId (1) or SNMP (0) */
    unsigned            ifvalue_vlan :1;

} skpc_probe_t;


/**
 *  A 'group'
 *
 *    A 'group' contains either a list of interface numbers or a list
 *    of IPWildcards.  A group is created by giving it a list
 *    containing values or previously defined groups.
 */
typedef enum {
    SKPC_GROUP_UNSET,
    SKPC_GROUP_INTERFACE,
    SKPC_GROUP_IPBLOCK
} skpc_group_type_t;

typedef struct skpc_group_st {
    /** groups have an optional name */
    char               *g_name;
    /** the contents of the group */
    union skpc_group_value_un {
        /** A bitmap of SK_SNMP_INDEX_LIMIT bits. */
        sk_bitmap_t        *map;
        /** A list containing pointers to skIPWildcard_t. */
        skIPWildcard_t    **ipblock;
        /** Vectory of IPWildcards used while building group.  This is
         * replaced by the 'ipblock' once the group is frozen. */
        sk_vector_t        *vec;
    }                   g_value;
    /** number of items in the group */
    uint32_t            g_itemcount;
    /** the type of the group */
    skpc_group_type_t   g_type;
    /** once frozen, a group cannot be changed */
    int8_t              g_is_frozen;
} skpc_group_t;



/**
 *  The 'decider'.
 *
 *    This describes the logic that the sensor will use to determine
 *    (decide) the flowtype (class/type) of each flow.  The type will
 *    depend on whether the sensor.conf file lists ipblocks or
 *    interfaces for the sensor.
 */
typedef enum {
    /** no ipblock or interface values seen */
    SKPC_UNSET,
    /** *-interface (SNMP) value seen */
    SKPC_INTERFACE,
    /** *-ipblock value seen */
    SKPC_IPBLOCK,
    /** ipblock is inverted */
    SKPC_NEG_IPBLOCK,
    /** sensor.conf has "*-interface remainder" line */
    SKPC_REMAIN_INTERFACE,
    /** sensor.conf has "*-ipblock remainder" line */
    SKPC_REMAIN_IPBLOCK
} skpc_netdecider_type_t;

typedef struct skpc_netdecider_st {
    skpc_netdecider_type_t  nd_type;
    const skpc_group_t     *nd_group;
} skpc_netdecider_t;

/**    number of 'decider' types */
#define SKPC_NUM_NETDECIDER_TYPES  6


/**
 *  A filter
 *
 *    A filter is similar to the decider in that it accepts a list of
 *    interfaces or of ipblocks.  However, instead of being used to
 *    decide the flowtype, a filter is used to determine whether
 *    rwflowpack should even consider the flow.  A filter can match
 *    the 'source' (either source IP or input interface), the
 *    'destination' (either destination IP or output interface), or
 *    'any' (any of the above).  Filters are set in the sensor.conf
 *    file by using the 'discard-when' and 'discard-unless'
 *    statements.
 */
typedef enum {
    SKPC_FILTER_SOURCE, SKPC_FILTER_DESTINATION, SKPC_FILTER_ANY
} skpc_filter_type_t;

typedef struct skpc_filter_st {
    /** the value to use as the filter */
    const skpc_group_t     *f_group;
    /** the part of the flow record to use */
    skpc_filter_type_t      f_type;
    /** if non-zero, discard flows that match the value in 'f_group'.
     * if zero, discard flows that do NOT match the value */
    unsigned                f_discwhen :1;
    /** value in 'f_group' (0 == interfaces, 1 == ipwildcards) */
    unsigned                f_wildcard :1;
} skpc_filter_t;

/**    number of 'filter' types */
#define SKPC_NUM_FILTER_TYPES 3



/**
 *  The sensor definition.
 *
 *    The sensor takes the flows from one or more probes and
 *    determines how to pack them---i.e., their flowtype or
 *    class/type.
 */
struct skpc_sensor_st {

    /** An array of network-deciders, one for each of the networks
     * defined for this site.  For example, a normal border router
     * that has the INTERNAL, EXTERNAL, and NULL networks would have 3
     * valid elements. */
    skpc_netdecider_t  *decider;
    size_t              decider_count;

    /** An array of probes associated with this sensor and the number
     * of entries in that list */
    skpc_probe_t      **probe_list;
    size_t              probe_count;

    /** the name of the sensor */
    char               *sensor_name;

    /** An array of filters and the count. */
    skpc_filter_t      *filter;
    size_t              filter_count;

    /** A list (and a count of the elements in the list) that contains
     * the IP addresses of the ISP's this probe talks to. */
    uint32_t           *isp_ip_list;
    size_t              isp_ip_count;

    /** The source and destination networks, if they have been set to a
     * fixed value. */
    skpc_network_id_t   fixed_network[2];

    /** The sensor ID as defined in the silk.conf file. */
    sensorID_t          sensor_id;
};


/**
 *  Iterators over probes and sensors
 */
typedef struct skpc_probe_iter_st {
    size_t cur;
} skpc_probe_iter_t;

typedef struct skpc_sensor_iter_st {
    size_t cur;
} skpc_sensor_iter_t;



/*
 *  *****  Probe configuration  **************************************
 */


/**
 *    Initialize the probe configuration data structures.
 */
int
skpcSetup(
    void);


/**
 *    Destroy all probes and sensors and free all memory used by the
 *    probe configuration.
 */
void
skpcTeardown(
    void);


/**
 *    Parse the probe configuration file 'filename'.  This should only
 *    be called one time.
 *
 *    This function will parse the configuration file and create
 *    sensors and probes.
 */
int
skpcParse(
    const char         *filename,
    int               (*site_sensor_verify_fn)(skpc_sensor_t *sensor));


/**
 *    Return the count of created and verified probes.
 */
size_t
skpcCountProbes(
    void);


/**
 *    Bind 'probe_iter' to loop over all the probes that have been
 *    defined.  Returns 0 on success, or -1 on error.
 */
int
skpcProbeIteratorBind(
    skpc_probe_iter_t  *probe_iter);


/**
 *    If the probe iterator 'probe_iter' has exhausted all probes,
 *    leave 'probe' untouched and return 0; otherwise, fill 'probe'
 *    with a pointer to the next verified probe and return 1.  Returns
 *    -1 on error (such as NULL input).  The caller should not modify
 *    or free the probe.
 */
int
skpcProbeIteratorNext(
    skpc_probe_iter_t      *probe_iter,
    const skpc_probe_t    **probe);


/**
 *    Returns the probe named 'probe_name'.  Returns NULL if not
 *    found.  The caller should not modify nor free the return value.
 */
const skpc_probe_t *
skpcProbeLookupByName(
    const char         *probe_name);


/**
 *    Return the count of created and verified sensors.
 */
size_t
skpcCountSensors(
    void);


/**
 *    Bind 'sensor_iter' to loop over all the sensors that have been
 *    defined.  Returns 0 on success, or -1 on error.
 */
int
skpcSensorIteratorBind(
    skpc_sensor_iter_t *sensor_iter);


/**
 *    If the sensor iterator 'sensor_iter' has exhausted all sensors,
 *    leave 'sensor' untouched and return 0; otherwise, fill 'sensor'
 *    with a pointer to the next verified sensor and return 1.  Returns
 *    -1 on error (such as NULL input).  The caller should not modify
 *    or free the sensor.
 */
int
skpcSensorIteratorNext(
    skpc_sensor_iter_t     *sensor_iter,
    const skpc_sensor_t   **sensor);


/**
 *    Appends to 'sensor_vec' the sensors whose ID is 'sensor_id'.
 *    Returns the number of sensors added to 'sensor_vec'.  Returns -1
 *    to indicate invalid input or memory error appending to the
 *    vector.  'sensor_vec' should be a vector having elements of size
 *    sizeof(skpc_sensor_t*).  The caller should not modify nor free
 *    the values appended to the vector.
 */
int
skpcSensorLookupByID(
    sensorID_t          sensor_id,
    sk_vector_t        *sensor_vec);


/**
 *    Appends to 'sensor_vec' the sensors whose name is 'sensor_name'.
 *    Returns the number of sensors added to 'sensor_vec'.  Returns -1
 *    to indicate invalid input or memory error appending to the
 *    vector.  'sensor_vec' should be a vector having elements of size
 *    sizeof(skpc_sensor_t*).  The caller should not modify nor free
 *    the values appended to the vector.
 */
int
skpcSensorLookupByName(
    const char         *sensor_name,
    sk_vector_t        *sensor_vec);


/**
 *    Given a printable representation of a probe, return the probe
 *    type.
 *
 *    Return PROBE_ENUM_INVALID when given an unrecognized name.
 */
skpc_probetype_t
skpcProbetypeNameToEnum(
    const char         *name);

/**
 *    Return the printable respresentation of the probe type.
 *
 *    Return NULL when given an illegal value.
 */
const char *
skpcProbetypeEnumtoName(
    skpc_probetype_t    type);


/**
 *    Given a printable representation of a protocol, return the
 *    protocol.
 *
 *    Return SKPC_PROTO_UNSET when given an unrecognized name.
 */
skpc_proto_t
skpcProtocolNameToEnum(
    const char         *name);

/**
 *    Return the printable respresentation of the protocol.
 *
 *    Return NULL when given an illegal value.
 */
const char *
skpcProtocolEnumToName(
    skpc_proto_t        proto);


/*
 *  *****  Networks  ***************************************************
 */


/**
 *    Add a (id, name) pair to the list of networks used when
 *    determining the flowtype (class/type) of a flow record.
 */
int
skpcNetworkAdd(
    skpc_network_id_t   network_id,
    const char         *name);

/**
 *    Return the network object that was created with the name
 *    attribute set to 'name'.  Returns NULL if no such network
 *    exists.
 */
const skpc_network_t *
skpcNetworkLookupByName(
    const char         *name);


/**
 *    Return the network object that was created with the id attribute
 *    set to 'id'.  Returns NULL if no such network exists.
 */
const skpc_network_t *
skpcNetworkLookupByID(
    skpc_network_id_t   network_id);



/*
 *  *****  Probes  *****************************************************
 *
 *
 *  Flows are stored by SENSOR; a SENSOR is a logical construct made
 *  up of one or more physical PROBES.
 *
 *  A probe collects flows in one of three ways:
 *
 *  1.  The probe can listen to network traffic.  For this case,
 *  skpcProbeGetListenOnSockaddr() will return the port on which to
 *  listen for traffic and the IP address that the probe should bind()
 *  to.  In addition, the skpcProbeGetAcceptFromHost() method will
 *  give the IP address from which the probe should accept
 *  connections.
 *
 *  2.  The probe can listen on a UNIX domain socket.  The
 *  skpcProbeGetListenOnUnixDomainSocket() method returns the pathname
 *  to the socket.
 *
 *  3.  The probe can read from a file.  The skpcProbeGetFileSource()
 *  method returns the name of the file.
 *
 *  A probe is not valid it has been set to use one and only one of
 *  these collection methods.
 *
 *  Once the probe has collected a flow, it needs to determine whether
 *  the flow represents incoming traffic, outgoing traffic, ACL
 *  traffic, etc.  The packLogicDetermineFlowtype() will take an
 *  'rwrec' and the probe where the record was collected and use the
 *  external, internal, and null interface values and the list of ISP
 *  IPs to update the 'flow_type' field on the rwrec.  The rwrec's
 *  sensor id ('sID') field is also updated.
 *
 */


/**
 *    Create a new probe and fill in 'probe' with the address of
 *    the newly allocated probe.
 */
int
skpcProbeCreate(
    skpc_probe_t      **probe);


/**
 *    Destroy the probe at '**probe' and free all memory.  Sets *probe
 *    to NULL.
 */
void
skpcProbeDestroy(
    skpc_probe_t      **probe);


/**
 *    Return the name of a probe.  The caller should not modify the
 *    name, and does not need to free() it.
 */
const char *
skpcProbeGetName(
    const skpc_probe_t *probe);


/**
 *    Set the name of a probe.  The probe name must
 *    meet all the requirements of a sensor name.  Each probe that is
 *    a collection point for a single sensor must have a unique name.
 *
 *    The function makes a copy of 'name' and returns 0 on
 *    success, non-zero on memory allocation failure.
 */
int
skpcProbeSetName(
    skpc_probe_t       *probe,
    const char         *name);


/**
 *    Return the type of the probe.  Before it is set by the user, the
 *    probe's type is PROBE_ENUM_INVALID.
 */
skpc_probetype_t
skpcProbeGetType(
    const skpc_probe_t *probe);

/**
 *    Set the probe's type.
 */
int
skpcProbeSetType(
    skpc_probe_t       *probe,
    skpc_probetype_t    probe_type);


/**
 *    Get the probe's protocol.  Before it is set by the user, the
 *    probe's protocol is SKPC_PROTO_UNSET.
 */
skpc_proto_t
skpcProbeGetProtocol(
    const skpc_probe_t *probe);

/**
 *    Set the probe's protocol.
 */
int
skpcProbeSetProtocol(
    skpc_probe_t       *probe,
    skpc_proto_t        skpc_protocol);


/**
 *    Get the probe's logging-flags.
 */
uint8_t
skpcProbeGetLogFlags(
    const skpc_probe_t *probe);

/**
 *    Set the probe's logging-flags; these
 *    logging flags refer to the log messages regarding missing and
 *    bad packet counts that some of the flow sources support.
 *
 *    The value of log_flags can be SOURCE_LOG_NONE to log nothing;
 *    SOURCE_LOG_ALL to log everything (the default); or a bitwise OR
 *    of the SOURCE_LOG_* values defined in libflowsource.h.
 */
int
skpcProbeSetLogFlags(
    skpc_probe_t       *probe,
    uint32_t            log_flags);


/**
 *    Determine whether the probe is currently configured to store
 *    SNMP interfaces or VLAN tags.
 */
skpc_ifvaluetype_t
skpcProbeGetInterfaceValueType(
    const skpc_probe_t *probe);


/**
 *    Set the type of value that the probe stores in the 'input' and
 *    'output' fields on the SiLK flow records; specifically SNMP
 *    values or VLAN tags.
 *
 *    If not set by the user, the probe stores the SNMP interfaces in
 *    the input and output fields.
 */
int
skpcProbeSetInterfaceValueType(
    skpc_probe_t       *probe,
    skpc_ifvaluetype_t  interface_value_type);


/**
 *    Return a bitmap that specifies any special (or "peculiar" or
 *    "quirky") data handling for the probe.
 */
uint32_t
skpcProbeGetQuirks(
    const skpc_probe_t *probe);


/**
 *    Add 'quirk' to the special data handling directives for 'probe'.
 *    Return 0 on success.  Return -1 if "quirk" is not recognized for
 *    this particular probe.  Return -2 if "quirk" conflicts with an
 *    existing "quirk" on 'probe'.
 */
int
skpcProbeAddQuirk(
    skpc_probe_t       *probe,
    const char         *quirk);


/**
 *    Clear all the "quirk" settings on 'probe'.
 */
int
skpcProbeClearQuirks(
    skpc_probe_t       *probe);


/**
 *    Get the port on which the probe listens for
 *    connections, and, for multi-homed hosts, the IP address that the
 *    probe should consider to be its IP.  The IP address is in host
 *    byte order.
 *
 *    When getting the information, the caller may pass in locations
 *    to be filled with the address and port; either parameter may be
 *    NULL to ignore that value.
 *
 *    The probe simply stores the address; it does not manipulate
 *    it in any way.
 *
 *    If the IP address to listen as is not set, the function
 *    returns INADDR_ANY in the 'out_addr'.  If the port has not been
 *    set, the 'get' function returns -1 and neither 'out' parameter
 *    is modified.
 */
int
skpcProbeGetListenOnSockaddr(
    const skpc_probe_t         *probe,
    const sk_sockaddr_array_t **addr);

/**
 *    Set the port on which the probe listens for
 *    connections, and, for multi-homed hosts, the IP address that the
 *    probe should consider to be its IP.  The IP address is in host
 *    byte order.
 *
 *    To specify the host(s) that may connect to this probe, use the
 *    skpcProbeSetAcceptFromHost() function.
 *
 *    When setting the information, the 'addr' must be non-zero and
 *    the port must be valid.
 *
 *    The probe simply stores the address; it does not manipulate
 *    it in any way.
 */
int
skpcProbeSetListenOnSockaddr(
    skpc_probe_t           *probe,
    sk_sockaddr_array_t    *addr);


/**
 *    Get the unix domain socket on which the
 *    probe listens for connections.
 *
 *    The caller should neither modify nor free the value returned by
 *    the 'get' method.  The 'get' method returns NULL if the 'set'
 *    method has not yet been called.
 */
const char *
skpcProbeGetListenOnUnixDomainSocket(
    const skpc_probe_t *probe);

/**
 *    Set the unix domain socket on which the
 *    probe listens for connections.
 *
 *    The 'set' method will make a copy of the 'u_socket' value.
 */
int
skpcProbeSetListenOnUnixDomainSocket(
    skpc_probe_t       *probe,
    const char         *u_socket);


/**
 *    Get the file name from which to read data.
 *
 *    The caller should neither modify nor free the value returned by
 *    the 'get' method.  The 'get' method returns NULL if the 'set'
 *    method has not yet been called.
 */
const char *
skpcProbeGetFileSource(
    const skpc_probe_t *probe);

/**
 *    Set the file name from which to read data.
 *
 *    The 'set' function will make a copy of the 'pathname' value.
 */
int
skpcProbeSetFileSource(
    skpc_probe_t       *probe,
    const char         *pathname);


/**
 *    Get the name of the directory to poll for
 *    files containing flow records.
 *
 *    The caller should neither modify nor free the value returned by
 *    the 'get' method.  The 'get' method returns NULL if the 'set'
 *    method has not yet been called.
 */
const char *
skpcProbeGetPollDirectory(
    const skpc_probe_t *probe);

/**
 *    Set the name of the directory to poll for
 *    files containing flow records.
 *
 *    The 'set' function will make a copy of the 'pathname' value.
 */
int
skpcProbeSetPollDirectory(
    skpc_probe_t       *probe,
    const char         *pathname);


/**
 *    Get the host the probe accepts connections
 *    from.  The value is in host byte order.
 *
 *    When getting the information, the caller should pass in a
 *    location to be filled with the address.
 *
 *    If the 'get' function is called before the 'set' function, NULL
 *    is returned and INADDR_ANY is stored in 'out_addr'.
 */
int
skpcProbeGetAcceptFromHost(
    const skpc_probe_t         *probe,
    const sk_sockaddr_array_t **addr);

/**
 *    Set the host the probe accepts connections
 *    from.  The value is in host byte order.
 *
 *    The probe simply stores the host_address; it does not manipulate
 *    it in any way.
 */
int
skpcProbeSetAcceptFromHost(
    skpc_probe_t           *probe,
    sk_sockaddr_array_t    *addr);


/**
 *    Return a count of sensors that are using this probe.
 */
size_t
skpcProbeGetSensorCount(
    const skpc_probe_t *probe);


/**
 *    Return 1 if probe has been verified; 0 otherwise.
 */
int
skpcProbeIsVerified(
    const skpc_probe_t *probe);


/**
 *    Verify the 'probe' is valid.  For example, that it's name is
 *    unique among all probes, and that if it is an IPFIX probe,
 *    verify that a listen-on-port has been specified.
 *
 *    When 'is_ephemeral' is specified, the function only verifies
 *    that is name is unique.  If the name is unique, the probe will
 *    be added to the global list of probes, but skpcProbeIsVerified()
 *    on the probe will return 0.
 *
 *    If valid, add the probe to the list of probes and return 0.
 *    Otherwise return non-zero.
 */
int
skpcProbeVerify(
    skpc_probe_t       *probe,
    int                 is_ephemeral);




/*
 *  *****  Sensors  ****************************************************
 */


/**
 *    Create a new sensor and fill in 'sensor' with the address of
 *    the newly allocated sensor.
 */
int
skpcSensorCreate(
    skpc_sensor_t     **sensor);


/**
 *    Destroy the sensor at '**sensor' and free all memory.  Sets *sensor
 *    to NULL.
 */
void
skpcSensorDestroy(
    skpc_sensor_t     **sensor);


/**
 *    Get the numeric ID of the sensor, as determined by the silk.conf
 *    file.
 */
sensorID_t
skpcSensorGetID(
    const skpc_sensor_t    *sensor);

/**
 *    Get the name of the sensor.  The caller should not modify the
 *    name, and does not need to free() it.
 */
const char *
skpcSensorGetName(
    const skpc_sensor_t    *sensor);

/**
 *    Set the name of a sensor.  The function makes a copy of 'name'
 *    and returns 0 on success, non-zero on memory allocation failure.
 */
int
skpcSensorSetName(
    skpc_sensor_t      *sensor,
    const char         *name);


/**
 *    Add a new filter (discard-when, discard-unless) to 'sensor'.
 *    'filter_type' specifies what part of the record to match
 *    (source, destination, or any).
 *
 *    If 'is_discardwhen_list' is non-zero, the caller is adding a
 *    filter that, when matched, causes rwflowpack to discard the
 *    flow.  Otherwise, not matching the filter causes the flow to be
 *    discarded.
 *
 *    When 'is_wildcard_list' is non-zero, the type of 'group' must be
 *    SKPC_GROUP_IPBLOCK and it must contain skIPWildcard_t* objects.
 *    Otherwise, the type of 'group' must be SKPC_GROUP_INTERFACE and
 *    it must contain numbers to treat as SNMP interface IDs.  The
 *    'group' must be frozen and it must contain values, otherwise the
 *    function returns -1.
 *
 *    Return 0 on success.  Return -1 on memory allocation error or
 *    when a filter for the specified 'filter_type' already exists.
 */
int
skpcSensorAddFilter(
    skpc_sensor_t      *sensor,
    const skpc_group_t *group,
    skpc_filter_type_t  filter_type,
    int                 is_discardwhen_list,
    int                 is_wildcard_list);


/**
 *    Get the IP addresses of the ISP routers
 *    that this sensor receives data from.
 *
 *    The method returns the length of the list of ISP-IPs.  If
 *    the 'out_ip_list' parameter is provided, it will be modified to point to
 *    the list of IP addresses (a C-array).  The caller should NOT
 *    modify the ISP-IP list that she receives, nor should she free()
 *    it.  When the 'get' method is called before the 'set' method, 0
 *    is returned and the out parameter is not modified.
 */
uint32_t
skpcSensorGetIspIps(
    const skpc_sensor_t    *sensor,
    const uint32_t        **out_ip_list);

/**
 *    Set the IP addresses of the ISP routers
 *    that this sensor receives data from.  When flows are sent to the
 *    null-interface, these IP addresses are used to distinguish
 *    between flows that were ACL'ed and those that were probably
 *    IP-routing messages sent from the ISP to this sensor.
 *
 *    The method takes a vector of uint32_t's containing the
 *    list of IP addresses.  The function will copy the values from
 *    the vector.  It returns 0 on success, or -1 on memory allocation
 *    errors.
 */
int
skpcSensorSetIspIps(
    skpc_sensor_t      *sensor,
    const sk_vector_t  *isp_ip_vec);


/**
 *    Specify that the 'dir' of all traffic should be considered
 *    the 'network_id'.  The list of 'network_id's is defined by the
 *    packing logic plug-in that is specified on the rwflowpack
 *    command line.
 *
 *    Here, "network" refers to one of the domains that are being
 *    monitored by the router or other flow collection software.  For
 *    example, a border router joins the internal and external
 *    networks, and a flow whose source IP is specified in the list of
 *    external network addresses will be considered incoming.
 *
 *    For example, to configure 'sensor' so that all its traffic is
 *    incoming (coming from the external network and going to the
 *    internal network), one would call this function twice, once to
 *    set the SKPC_DIR_SRC to 'external' and again to set the
 *    SKPC_DIR_DST to 'internal.
 *
 *    This function conflicts with skpcSensorSetIpBlocks() and
 *    skpcSensorSetInterfaces().
 */
int
skpcSensorSetNetwork(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id,
    skpc_direction_t    dir);


/**
 *    Function to set the list of IPs associated with the network
 *    'network_id'.  The list of 'network_id's is defined by the
 *    packing logic plug-in that is specified on the rwflowpack
 *    command line.
 *
 *    Here, "network" refers to one of the domains that are being
 *    monitored by the router or other flow collection software.  For
 *    example, a border router joins the internal and external
 *    networks, and a flow whose source IP is specified in the list of
 *    external network addresses will be considered incoming.
 *
 *    The 'ip_group' must be frozen and have type SKPC_GROUP_IPBLOCK.
 *    The skIPWildcard_t's contained in the group will be assigned to
 *    the specified 'network_id'.
 *
 *    The 'is_negated' value, if non-zero, means that the list of IPs
 *    to assign to 'network_id' are all those NOT given in the
 *    'ip_group'.
 *
 *    The function returns 0 on success, or -1 if the group is NULL,
 *    not frozen, or empty.
 *
 *    The skpcSensorSetIpBlocks() function will return an error if
 *    skpcSensorSetInterfaces() function is called before it, or if
 *    the skpcSensotSetNetwork() function has already assigned all
 *    source or destination traffic to this 'network_id'.
 */
int
skpcSensorSetIpBlocks(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id,
    const skpc_group_t *ip_group,
    int                 is_negated);


/**
 *    Sets the list of IPs that are part of the network 'network_id'
 *    to all IPs that not assigned to another interface.  The list of
 *    'network_id's is defined by the packing logic plug-in that is
 *    specified on the rwflowpack command line.
 */
int
skpcSensorSetToRemainderIpBlocks(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id);


/**
 *    Function to set the list of SNMP interfaces associated with the
 *    network 'network_id'.  The list of 'network_id's is defined by
 *    the packing logic plug-in that is specified on the rwflowpack
 *    command line.
 *
 *    Here, "network" refers to one of the domains that are being
 *    monitored by the router or other flow collection software.  For
 *    example, a border router joins the internal and external
 *    networks, and a flow whose interface ID is specified in the list
 *    of external interfaces will be considered incoming.
 *
 *    The 'if_group' must be frozen and have type
 *    SKPC_GROUP_INTERFACE.  The uint32_t's contained in the group
 *    will be treated as SNMP values and will be assigned to the
 *    specified 'network_id'.
 *
 *    The function returns 0 on success, or -1 if the group is NULL,
 *    not frozen, or empty.
 *
 *    The skpcSensorSetInterfaces() function will return an error if
 *    skpcSensorSetIpBlocks() function is called before it, or if
 *    the skpcSensotSetNetwork() function has already assigned all
 *    source or destination traffic to this 'network_id'.
 */
int
skpcSensorSetInterfaces(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id,
    const skpc_group_t *if_group);


/**
 *    Sets the group of SNMP interfaces that connect to the network
 *    whose ID is 'network_id' to 0, the SNMP interface value used by
 *    Cisco to designate a non-routed flow.  The network may not have
 *    been previously set.  Return 0 on success, or non-zero on
 *    failure.
 */
int
skpcSensorSetDefaultNonrouted(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id);


/**
 *    Sets the SNMP interfaces associated with 'network_id' to all
 *    SNMP interfaces that have not been not assigned to other
 *    interfaces.  The list of 'network_id's is defined by the
 *    packing logic plug-in that is specified on the rwflowpack
 *    command line.
 */
int
skpcSensorSetToRemainderInterfaces(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id);


/**
 *    Appends to 'out_probe_vec' all the probes defined on 'sensor'.
 *    Returns the number of probes defined on 'sensor'.  Returns 0 if
 *    no probes are defined or if there is a memory error appending
 *    the probes to the vector.  If 'out_probe_vec' is NULL, the count
 *    of probes on sensors is returned.  'out_probe_vec' should be a
 *    vector having elements of size sizeof(skpc_probe_t*).  The
 *    caller should not modify nor free the values appended to the
 *    vector.
 */
uint32_t
skpcSensorGetProbes(
    const skpc_sensor_t    *sensor,
    sk_vector_t            *out_probe_vec);


/**
 *    Copy the probes listed in 'probe_vec' onto 'sensor'.
 *
 *    Return 0 on success, or -1 if 'probe_vec' is NULL or empty, or
 *    if memory allocation fails.
 */
int
skpcSensorSetProbes(
    skpc_sensor_t      *sensor,
    const sk_vector_t  *probe_vec);


/**
 *    Count the number of SNMP interfaces that have been mapped to a
 *    flowtype on 'sensor'.  Will exclude the network ID
 *    'ignored_network_id'; pass SKPC_NETWORK_ID_INVALID or a negative
 *    value in that parameter to ensure that all SNMP interfaces are
 *    counted.
 */
uint32_t
skpcSensorCountNetflowInterfaces(
    const skpc_sensor_t    *sensor,
    int                     ignored_network_id);


/**
 *    Test 'rwrec' against the 'network_id' interfaces---either the
 *    SNMP values or the IP-block values---on the 'sensor'.  The value
 *    'rec_dir' tells the function whether to check if the rwrec was
 *    coming from the specified 'network_id' or going to that network.
 *
 *    The function returns 1 if there is a match, -1 if there was not
 *    a match, and 0 if neither an IP block list nor an SNMP interface
 *    list was defined for the 'network_id'.
 *
 *    If 'rec_dir' is SKPC_DIR_SRC, the function checks the record's
 *    sIP against the list of IP blocks for the 'network_id'.  If no
 *    IP blocks are defined for the specified 'network_id' on this
 *    'sensor', the function checks the record's SNMP input interface
 *    against the list of SNMP interfaces for the 'network_id'.  When
 *    'rec_dir' is SKPC_DIR_DST, the record's dIP and SNMP output
 *    values are checked.
 */
int
skpcSensorTestFlowInterfaces(
    const skpc_sensor_t    *sensor,
    const rwRec            *rwrec,
    skpc_network_id_t       network_id,
    skpc_direction_t        rec_dir);


/**
 *    Check whether 'rwrec' matches the filters specified on 'sensor'.
 *    Return 0 if the flow should be packed, or non-zero to discard
 *    the flow.
 *
 *    When 'rwrec' matches any "discard-when" filter on 'sensor', this
 *    function returns non-zero.  Otherwise, if no "discard-unless"
 *    filters exist, the function returns 0.
 *
 *    If any "discard-unless" filters exist on 'sensor', 'rwrec' must
 *    match EVERY one of them for the function to return 0.
 */
int
skpcSensorCheckFilters(
    const skpc_sensor_t    *sensor,
    const rwRec            *rwrec);


/**
 *    Verify that 'sensor' is valid.  For example, that if its probe
 *    is an IPFIX probe, the sensor has ipblocks are defined, etc.  If
 *    'site_sensor_verify_fn' is defined, it will be called to verify
 *    the sensor for the current site.  That function should return 0
 *    if the sensor is valid, or non-zero if not valid.
 *
 *    Returns 0 if it is valid, non-zero otherwise.
 */
int
skpcSensorVerify(
    skpc_sensor_t      *sensor,
    int               (*site_sensor_verify_fn)(skpc_sensor_t *sensor));



/*
 *  *****  Groups  *****************************************************
 */


/**
 *    Create a new group and fill in 'group' with the address of the
 *    newly allocated group.
 */
int
skpcGroupCreate(
    skpc_group_t      **group);


/**
 *    Destroy the group at '**group' and free all memory.  Sets *group
 *    to NULL.
 */
void
skpcGroupDestroy(
    skpc_group_t      **group);


/**
 *    Specifies that no changes can be made to the group (other than
 *    destroying it).  Returns 0 on success, or -1 if there is a
 *    memory allocation failure.  (Freezing a group may allocate
 *    memory as data is rearranged in the group.)
 *
 *    Freezing a frozen group is no-op, and the function returns 0.
 */
int
skpcGroupFreeze(
    skpc_group_t       *group);


/**
 *    Get the name of a group.
 *
 *    A group can be anonymous---that is, not have a name.  These
 *    groups are created when an interface or ipblock list is specified
 *    outside of a "group" block; for example, by listing integers (or
 *    multiple groups) on in "internal-interfaces" statement.  For
 *    these groups, the name is NULL.
 *
 *    The function returns the name of the group.  The caller
 *    should not modify the name, and does not need to free() it.
 */
const char *
skpcGroupGetName(
    const skpc_group_t *group);

/**
 *    Set the name of a group.  A group name must
 *    meet all the requirements of a sensor name.
 *
 *    The set function makes a copy of 'name' and returns 0 on
 *    success, non-zero on memory allocation failure.  The set
 *    function also returns non-zero if the group is frozen (see
 *    skpcGroupFreeze()).
 */
int
skpcGroupSetName(
    skpc_group_t       *group,
    const char         *group_name);


/**
 *    Get the groups's type; that is, whether it stores IP-blocks or
 *    interface values.
 *
 *    Until it has been set, the group's type is SKPC_GROUP_UNSET.
 */
skpc_group_type_t
skpcGroupGetType(
    const skpc_group_t *group);

/**
 *    Set the groups's type.
 *
 *    The set function returns -1 if the type of the group has been
 *    previously set or if the group is frozen (see
 *    skpcGroupFreeze()).
 */
int
skpcGroupSetType(
    skpc_group_t       *group,
    skpc_group_type_t   group_type);


/**
 *    Add the values in 'vec' to the group 'group'.  If the type of
 *    'group' is SKPC_GROUP_INTERFACE, 'vec' should contain
 *    uint32_t's.  If the type of 'group' is SKPC_GROUP_IPBLOCK, 'vec'
 *    should contain pointers to skIPWildcard_t.
 *
 *    When 'vec' contains skIPWildcard_t*, this function assumes
 *    ownership of the data and will free the skIPWildcard_t when
 *    skpcTeardown() is called.
 *
 *    The function returns 0 on success.  It also returns 0 when 'vec'
 *    is NULL or contains no elements.
 *
 *    Return -1 if the group is frozen, if the group's type is
 *    SKPC_GROUP_UNSET, if the size of elements in the 'vec' is not
 *    consistent with the group's type, or if there is a memory
 *    allocation error.
 */
int
skpcGroupAddValues(
    skpc_group_t       *group,
    const sk_vector_t  *vec);


/**
 *    Add the contents of group 'g' to group 'group'.
 *
 *    Return 0 on success.  Also return 0 if 'g' is NULL or if
 *    contains no items.
 *
 *    Return -1 if 'group' is frozen, if 'g' is NOT frozen, if the
 *    groups' types are different, or if there is a memory allocation
 *    error.
 */
int
skpcGroupAddGroup(
    skpc_group_t       *group,
    const skpc_group_t *g);


/**
 *    Return 1 if 'group' is frozen; 0 otherwise.
 */
int
skpcGroupIsFrozen(
    const skpc_group_t *group);


/**
 *    Returns the group named 'group_name'.  Returns NULL if not found
 *    of if 'group_name' is NULL.  The returned group is frozen.
 */
skpc_group_t *
skpcGroupLookupByName(
    const char         *group_name);


#ifdef __cplusplus
}
#endif
#endif /* _PROBECONF_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
