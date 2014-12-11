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

#include <silk/silk.h>

RCSIDENT("$SiLK: probeconf.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/libflowsource.h>
#include <silk/probeconf.h>
#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/sklog.h>
#include <silk/sksite.h>
#include "probeconfscan.h"

/*
 *  *****  Flow Type  **************************************************
 *
 *  The probe is used to determine the flow-type---as defined by in
 *  the silk.conf file---of a flow record (rwRec) read from that
 *  probe.
 *
 *  The skpcProbeDetermineFlowtype() function is defined in the
 *  probeconf-<$SILK_SITE>.c file.
 *
 */

/* LOCAL DEFINES AND TYPEDEFS */

/* Minimum version of libfixbuf required for IPFIX */
#define SKPC_LIBFIXBUF_VERSION_IPFIX        "1.3.0"

/* Minimum version of libfixbuf required for NetFlow V9 */
#define SKPC_LIBFIXBUF_VERSION_NETFLOWV9    "1.3.0"

/* Minimum version of libfixbuf required for sFlow */
#define SKPC_LIBFIXBUF_VERSION_SFLOW        "1.6.0"

/* Maximum valid value for a port 2^16 - 1 */
#define PORT_VALID_MAX 0xFFFF

/* Set ports to this invalid value initially */
#define PORT_NOT_SET   0xFFFFFFFF

/* Value to use for remaining IPs to say that it hasn't been set */
#define REMAINDER_NOT_SET  INT8_MAX


/* a map between probe types and printable names */
static struct probe_type_name_map_st {
    const char         *name;
    skpc_probetype_t    value;
} probe_type_name_map[] = {
    {"ipfix",       PROBE_ENUM_IPFIX},
    {"netflow-v5",  PROBE_ENUM_NETFLOW_V5},
    {"netflow-v9",  PROBE_ENUM_NETFLOW_V9},
    {"sflow",       PROBE_ENUM_SFLOW},
    {"silk",        PROBE_ENUM_SILK},

    /* legacy name for netflow-v5 */
    {"netflow",     PROBE_ENUM_NETFLOW_V5},

    /* sentinel */
    {NULL,          PROBE_ENUM_INVALID}
};


/* a map between protocols and printable names */
static struct skpc_protocol_name_map_st {
    const char     *name;
    uint8_t         num;
    skpc_proto_t    value;
} skpc_protocol_name_map[] = {
    {"sctp", 132, SKPC_PROTO_SCTP},
    {"tcp",    6, SKPC_PROTO_TCP},
    {"udp",   17, SKPC_PROTO_UDP},

    /* sentinel */
    {NULL,     0, SKPC_PROTO_UNSET}
};



/* LOCAL VARIABLES */

/* The probes that have been created and verified */
static sk_vector_t *skpc_probes = NULL;

/* The sensors that have been created and verified */
static sk_vector_t *skpc_sensors = NULL;

/* The networks that have been created */
static sk_vector_t *skpc_networks = NULL;

/* The groups that have been created */
static sk_vector_t *skpc_groups = NULL;

/* The IPWildcards that are added to the groups. */
static sk_vector_t *skpc_wildcards = NULL;

/* group containing the default non-routed NetFlow interface */
static skpc_group_t *nonrouted_group = NULL;


/* LOCAL FUNCTION PROTOTYPES */

static int
skpcGroupCheckInterface(
    const skpc_group_t *group,
    uint32_t            interface);
static int
skpcGroupCheckIp(
    const skpc_group_t *group,
    const skipaddr_t   *ip);
static int
skpcGroupComputeComplement(
    skpc_group_t       *group);
static uint32_t
skpcGroupGetItemCount(
    const skpc_group_t *group);


/* FUNCTION DEFINITIONS */


/*
 *  *****  Probe configuration  **************************************
 */

/* setup the probes */
int
skpcSetup(
    void)
{
    if (NULL == skpc_probes) {
        skpc_probes = skVectorNew(sizeof(skpc_probe_t*));
        if (NULL == skpc_probes) {
            goto ERROR;
        }
    }

    if (NULL == skpc_sensors) {
        skpc_sensors = skVectorNew(sizeof(skpc_sensor_t*));
        if (NULL == skpc_sensors) {
            goto ERROR;
        }
    }

    if (NULL == skpc_networks) {
        skpc_networks = skVectorNew(sizeof(skpc_network_t));
        if (NULL == skpc_networks) {
            goto ERROR;
        }
    }

    if (NULL == skpc_groups) {
        skpc_groups = skVectorNew(sizeof(skpc_group_t));
        if (NULL == skpc_groups) {
            goto ERROR;
        }
    }

    if (skpcParseSetup()) {
        goto ERROR;
    }

    return 0;

  ERROR:
    if (skpc_probes) {
        skVectorDestroy(skpc_probes);
    }
    if (skpc_sensors) {
        skVectorDestroy(skpc_sensors);
    }
    if (skpc_networks) {
        skVectorDestroy(skpc_networks);
    }
    if (skpc_groups) {
        skVectorDestroy(skpc_groups);
    }
    return -1;
}


/* destroy everything */
void
skpcTeardown(
    void)
{
    skpc_network_t *nwp;
    skpc_probe_t **probe;
    skpc_sensor_t **sensor;
    skpc_group_t **group;
    skIPWildcard_t **ipwild;
    size_t i;

    /* clean up the parser */
    skpcParseTeardown();

    /* Free all the networks */
    if (skpc_networks) {
        for (i = 0;
             (nwp = (skpc_network_t*)skVectorGetValuePointer(skpc_networks, i))
                 != NULL;
             ++i)
        {
            free(nwp->name);
        }

        /* destroy the vector itself */
        skVectorDestroy(skpc_networks);
        skpc_networks = NULL;
    }

    /* Free all the groups */
    if (skpc_groups) {
        for (i = 0;
             (group = (skpc_group_t**)skVectorGetValuePointer(skpc_groups, i))
                 != NULL;
             ++i)
        {
            skpcGroupDestroy(group);
        }
        /* destroy the vector itself */
        skVectorDestroy(skpc_groups);
        skpc_groups = NULL;
    }

    /* Free all the sensors */
    if (skpc_sensors) {
        for (i = 0;
             (sensor=(skpc_sensor_t**)skVectorGetValuePointer(skpc_sensors, i))
                 != NULL;
             ++i)
        {
            skpcSensorDestroy(sensor);
        }
        /* destroy the vector itself */
        skVectorDestroy(skpc_sensors);
        skpc_sensors = NULL;
    }

    /* Free all the probes */
    if (skpc_probes) {
        for (i = 0;
             (probe = (skpc_probe_t**)skVectorGetValuePointer(skpc_probes, i))
                 != NULL;
             ++i)
        {
            skpcProbeDestroy(probe);
        }
        /* destroy the vector itself */
        skVectorDestroy(skpc_probes);
        skpc_probes = NULL;
    }

    /* Free all the wildcards */
    if (skpc_wildcards) {
        for (i = 0;
             (ipwild = ((skIPWildcard_t**)
                        skVectorGetValuePointer(skpc_wildcards, i)))
                 != NULL;
             ++i)
        {
            free(*ipwild);
            *ipwild = NULL;
        }
        /* destroy the vector itself */
        skVectorDestroy(skpc_wildcards);
        skpc_wildcards = NULL;
    }
}


/* return a count of verified probes */
size_t
skpcCountProbes(
    void)
{
    assert(skpc_probes);
    return skVectorGetCount(skpc_probes);
}


int
skpcProbeIteratorBind(
    skpc_probe_iter_t  *probe_iter)
{
    if (probe_iter == NULL || skpc_probes == NULL) {
        return -1;
    }
    probe_iter->cur = 0;
    return 0;
}


int
skpcProbeIteratorNext(
    skpc_probe_iter_t      *probe_iter,
    const skpc_probe_t    **probe)
{
    if (probe_iter == NULL || probe == NULL) {
        return -1;
    }

    if (0 != skVectorGetValue((void*)probe, skpc_probes, probe_iter->cur)) {
        return 0;
    }

    ++probe_iter->cur;
    return 1;
}


/* return a probe having the given probe-name. */
const skpc_probe_t *
skpcProbeLookupByName(
    const char         *probe_name)
{
    const skpc_probe_t **probe;
    size_t i;

    assert(skpc_probes);

    /* check input */
    if (probe_name == NULL) {
        return NULL;
    }

    /* loop over all probes until we find one with given name */
    for (i = 0;
         (probe=(const skpc_probe_t**)skVectorGetValuePointer(skpc_probes, i))
             != NULL;
         ++i)
    {
        if (0 == strcmp(probe_name, (*probe)->probe_name)) {
            return *probe;
        }
    }

    return NULL;
}


/* return a count of verified sensors */
size_t
skpcCountSensors(
    void)
{
    assert(skpc_sensors);
    return skVectorGetCount(skpc_sensors);
}


int
skpcSensorIteratorBind(
    skpc_sensor_iter_t *sensor_iter)
{
    if (sensor_iter == NULL || skpc_sensors == NULL) {
        return -1;
    }
    sensor_iter->cur = 0;
    return 0;
}


int
skpcSensorIteratorNext(
    skpc_sensor_iter_t     *sensor_iter,
    const skpc_sensor_t   **sensor)
{
    if (sensor_iter == NULL || sensor == NULL) {
        return -1;
    }

    if (0 != skVectorGetValue((void*)sensor, skpc_sensors, sensor_iter->cur)) {
        return 0;
    }

    ++sensor_iter->cur;
    return 1;
}


/* append to sensor_vec sensors having the given sensor-name. */
int
skpcSensorLookupByName(
    const char         *sensor_name,
    sk_vector_t        *sensor_vec)
{
    const skpc_sensor_t **s;
    int count = 0;
    size_t i;

    assert(skpc_sensors);

    /* check input */
    if (sensor_name == NULL || sensor_vec == NULL) {
        return -1;
    }
    if (skVectorGetElementSize(sensor_vec) != sizeof(skpc_sensor_t*)) {
        return -1;
    }

    /* loop over all sensors looking for the given name */
    for (i = 0;
         (s = (const skpc_sensor_t**)skVectorGetValuePointer(skpc_sensors, i))
             != NULL;
         ++i)
    {
        if (0 == strcmp(sensor_name, (*s)->sensor_name)) {
            if (skVectorAppendValue(sensor_vec, s)) {
                return -1;
            }
            ++count;
        }
    }

    return count;
}


/* append to sensor_vec sensors having the given sensor-name. */
int
skpcSensorLookupByID(
    sensorID_t          sensor_id,
    sk_vector_t        *sensor_vec)
{
    const skpc_sensor_t **s;
    int count = 0;
    size_t i;

    assert(skpc_sensors);

    /* check input */
    if (sensor_vec == NULL) {
        return -1;
    }
    if (skVectorGetElementSize(sensor_vec) != sizeof(skpc_sensor_t*)) {
        return -1;
    }

    /* loop over all sensors looking for the given id */
    for (i = 0;
         (s = (const skpc_sensor_t**)skVectorGetValuePointer(skpc_sensors, i))
             != NULL;
         ++i)
    {
        if (sensor_id == (*s)->sensor_id) {
            if (skVectorAppendValue(sensor_vec, s)) {
                return -1;
            }
            ++count;
        }
    }

    return count;
}


/*
 *  *****  Network  ****************************************************
 */

/* Add a new id->name pair to the list of networks */
int
skpcNetworkAdd(
    skpc_network_id_t   id,
    const char         *name)
{
    skpc_network_t *nwp;
    skpc_network_t nw;
    size_t i;

    assert(skpc_networks);

    if (id > SKPC_NETWORK_ID_MAX) {
        return -4;
    }

    /* check for  */
    for (i = 0;
         (nwp = (skpc_network_t*)skVectorGetValuePointer(skpc_networks, i))
             != NULL;
         ++i)
    {
        if (id == nwp->id) {
            /* duplicate id */
            return -2;
        }
        if (0 == strcmp(name, nwp->name)) {
            /* duplicate name */
            return -3;
        }
    }

    /* create network and add it to the vector */
    nw.id = id;
    nw.name = strdup(name);
    if (nw.name == NULL) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    if (skVectorAppendValue(skpc_networks, &nw)) {
        free(nw.name);
        return -1;
    }

    return 0;
}


/* Find the network that has the given name 'name' */
const skpc_network_t *
skpcNetworkLookupByName(
    const char         *name)
{
    skpc_network_t *nwp;
    size_t i;

    assert(skpc_networks);

    for (i = 0;
         (nwp = (skpc_network_t*)skVectorGetValuePointer(skpc_networks, i))
             != NULL;
         ++i)
    {
        if (0 == strcmp(name, nwp->name)) {
            return nwp;
        }
    }
    return NULL;
}


/* Find the network that has the given ID 'id' */
const skpc_network_t *
skpcNetworkLookupByID(
    skpc_network_id_t   network_id)
{
    skpc_network_t *nwp;
    size_t i;

    assert(skpc_networks);
    assert(network_id <= SKPC_NETWORK_ID_INVALID);

    for (i = 0;
         (nwp = (skpc_network_t*)skVectorGetValuePointer(skpc_networks, i))
             != NULL;
         ++i)
    {
        if (network_id == nwp->id) {
            return nwp;
        }
    }
    return NULL;
}



/*
 *  *****  Probes  *****************************************************
 */

/* Create a probe */
int
skpcProbeCreate(
    skpc_probe_t      **probe)
{
    assert(probe);

    (*probe) = (skpc_probe_t*)calloc(1, sizeof(skpc_probe_t));
    if (NULL == (*probe)) {
        return -1;
    }

    (*probe)->probe_type = PROBE_ENUM_INVALID;
    (*probe)->protocol = SKPC_PROTO_UNSET;
    (*probe)->log_flags = SOURCE_LOG_ALL;

    return 0;
}


/* Destroy a probe and free all memory associated with it */
void
skpcProbeDestroy(
    skpc_probe_t      **probe)
{
    if (!probe || !(*probe)) {
        return;
    }

    if ((*probe)->sensor_list) {
        free((*probe)->sensor_list);
    }
    if ((*probe)->unix_domain_path) {
        free((*probe)->unix_domain_path);
    }
    if ((*probe)->file_source) {
        free((*probe)->file_source);
    }
    if ((*probe)->poll_directory) {
        free((*probe)->poll_directory);
    }
    if ((*probe)->probe_name) {
        free((*probe)->probe_name);
    }
    if ((*probe)->listen_addr) {
        skSockaddrArrayDestroy((*probe)->listen_addr);
    }
    if ((*probe)->accept_from_addr) {
        skSockaddrArrayDestroy((*probe)->accept_from_addr);
    }

    free(*probe);
    *probe = NULL;
}


/* Get and set the name of probe */
const char *
skpcProbeGetName(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->probe_name;
}

int
skpcProbeSetName(
    skpc_probe_t       *probe,
    const char         *name)
{
    const char *cp;
    char *copy;

    assert(probe);
    if (name == NULL || name[0] == '\0') {
        return -1;
    }

    /* check for illegal characters */
    cp = name;
    while (*cp) {
        if (*cp == '/' || isspace((int)*cp)) {
            return -1;
        }
        ++cp;
    }

    copy = strdup(name);
    if (copy == NULL) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    if (probe->probe_name) {
        free(probe->probe_name);
    }
    probe->probe_name = copy;
    return 0;
}


/* Get and set the probe type */
skpc_probetype_t
skpcProbeGetType(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->probe_type;
}

int
skpcProbeSetType(
    skpc_probe_t       *probe,
    skpc_probetype_t    probe_type)
{
    assert(probe);

    if (NULL == skpcProbetypeEnumtoName(probe_type)) {
        return -1;
    }
    probe->probe_type = probe_type;
    return 0;
}


/* Get and set the probe's protocol */
skpc_proto_t
skpcProbeGetProtocol(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->protocol;
}

int
skpcProbeSetProtocol(
    skpc_probe_t       *probe,
    skpc_proto_t        probe_protocol)
{
    assert(probe);

    if (NULL == skpcProtocolEnumToName(probe_protocol)) {
        return -1;
    }
    probe->protocol = probe_protocol;
    return 0;
}


/* Get and set probe log-flags */
uint8_t
skpcProbeGetLogFlags(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->log_flags;
}

int
skpcProbeSetLogFlags(
    skpc_probe_t       *probe,
    uint32_t            log_flags)
{
    assert(probe);
    if (log_flags > SOURCE_LOG_ALL) {
        return -1;
    }
    probe->log_flags = (uint8_t)(log_flags & 0xFF);
    return 0;
}


/* Get and set type of data in the input/output fields */

skpc_ifvaluetype_t
skpcProbeGetInterfaceValueType(
    const skpc_probe_t *probe)
{
    assert(probe);
    return ((probe->ifvalue_vlan) ? SKPC_IFVALUE_VLAN : SKPC_IFVALUE_SNMP);
}

int
skpcProbeSetInterfaceValueType(
    skpc_probe_t       *probe,
    skpc_ifvaluetype_t  interface_value_type)
{
    assert(probe);
    switch (interface_value_type) {
      case SKPC_IFVALUE_SNMP:
        probe->ifvalue_vlan = 0;
        break;
      case SKPC_IFVALUE_VLAN:
        probe->ifvalue_vlan = 1;
        break;
      default:
        return -1;              /* NOTREACHED */
    }
    return 0;
}


/* Get and set any "peculiar" data handling for this probe. */

uint32_t
skpcProbeGetQuirks(
    const skpc_probe_t *probe)
{
    assert(probe);
    return (uint32_t)probe->quirks;
}

int
skpcProbeAddQuirk(
    skpc_probe_t       *probe,
    const char         *quirk)
{
    assert(probe);

    if (0 == strcmp(quirk, "zero-packets")) {
        probe->quirks |= SKPC_QUIRK_ZERO_PACKETS;
        return 0;
    }
    if (0 == strcmp(quirk, "firewall-event")) {
        probe->quirks |= SKPC_QUIRK_FW_EVENT;
        return 0;
    }
    if (0 == strcmp(quirk, "missing-ips")) {
        probe->quirks |= SKPC_QUIRK_MISSING_IPS;
        return 0;
    }
    /* unrecognized quirk */
    return -1;
}

int
skpcProbeClearQuirks(
    skpc_probe_t       *probe)
{
    assert(probe);
    probe->quirks = 0;
    return 0;
}


/* Get and set host:port to listen on. */
int
skpcProbeGetListenOnSockaddr(
    const skpc_probe_t         *probe,
    const sk_sockaddr_array_t **addr)
{
    assert(probe);
    if (probe->listen_addr == NULL) {
        return -1;
    }

    if (addr) {
        *addr = probe->listen_addr;
    }
    return 0;
}

int
skpcProbeSetListenOnSockaddr(
    skpc_probe_t           *probe,
    sk_sockaddr_array_t    *addr)
{
    assert(probe);

    if (probe->listen_addr) {
        skSockaddrArrayDestroy(probe->listen_addr);
    }

    probe->listen_addr = addr;
    return 0;
}


/* Get and set the unix domain socket on which to listen. */
const char *
skpcProbeGetListenOnUnixDomainSocket(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->unix_domain_path;
}

int
skpcProbeSetListenOnUnixDomainSocket(
    skpc_probe_t       *probe,
    const char         *u_socket)
{
    char *copy;

    assert(probe);
    if (u_socket == NULL || u_socket[0] == '\0') {
        return -1;
    }

    copy = strdup(u_socket);
    if (copy == NULL) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    if (probe->unix_domain_path) {
        free(probe->unix_domain_path);
    }
    probe->unix_domain_path = copy;

    return 0;
}


/* Get and set the file name to read data from */
const char *
skpcProbeGetFileSource(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->file_source;
}

int
skpcProbeSetFileSource(
    skpc_probe_t       *probe,
    const char         *pathname)
{
    char *copy;

    assert(probe);
    if (pathname == NULL || pathname[0] == '\0') {
        return -1;
    }

    copy = strdup(pathname);
    if (copy == NULL) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    if (probe->file_source) {
        free(probe->file_source);
    }
    probe->file_source = copy;

    return 0;
}


/* Get and set the name of the directory to poll for new files */
const char *
skpcProbeGetPollDirectory(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->poll_directory;
}

int
skpcProbeSetPollDirectory(
    skpc_probe_t       *probe,
    const char         *pathname)
{
    char *copy;

    assert(probe);
    if (pathname == NULL || pathname[0] == '\0') {
        return -1;
    }

    copy = strdup(pathname);
    if (copy == NULL) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }

    if (probe->poll_directory) {
        free(probe->poll_directory);
    }
    probe->poll_directory = copy;

    return 0;
}


/* Get and set host to accept connections from */
int
skpcProbeGetAcceptFromHost(
    const skpc_probe_t         *probe,
    const sk_sockaddr_array_t **addr)
{
    assert(probe);
    if (probe->accept_from_addr == NULL) {
        return -1;
    }

    if (addr) {
        *addr = probe->accept_from_addr;
    }
    return 0;
}

int
skpcProbeSetAcceptFromHost(
    skpc_probe_t           *probe,
    sk_sockaddr_array_t    *addr)
{
    assert(probe);
    if (addr == NULL) {
        return -1;
    }

    probe->accept_from_addr = addr;
    return 0;
}


size_t
skpcProbeGetSensorCount(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->sensor_count;
}


static int
skpcProbeAddSensor(
    skpc_probe_t       *probe,
    skpc_sensor_t      *sensor)
{
#if 0
    size_t i;

    /* 2011.12.09 Allow the same sensor to appear on a probe multiple
     * times, and assume the user is using a filter (e.g.,
     * discard-when) to avoid packing the flow record multiple
     * times. */
    /* each sensor may only appear on a probe one time */
    for (i = 0; i < probe->sensor_count; ++i) {
        if (skpcSensorGetID(probe->sensor_list[i])
            == skpcSensorGetID(sensor))
        {
            return -1;
        }
    }
#endif  /* 0 */

    if (probe->sensor_list == NULL) {
        /* create the sensor list; large enough to hold one sensor */
        assert(probe->sensor_count == 0);
        probe->sensor_list = (skpc_sensor_t**)malloc(sizeof(skpc_sensor_t*));
        if (probe->sensor_list == NULL) {
            skAppPrintOutOfMemory(NULL);
            return -1;
        }
    } else {
        /* grow the sensor list by one */
        skpc_sensor_t **old = probe->sensor_list;

        probe->sensor_list = ((skpc_sensor_t**)
                              realloc(probe->sensor_list,
                                      ((1 + probe->sensor_count)
                                       * sizeof(skpc_sensor_t*))));
        if (probe->sensor_list == NULL) {
            probe->sensor_list = old;
            skAppPrintOutOfMemory(NULL);
            return -1;
        }
    }
    probe->sensor_list[probe->sensor_count] = sensor;
    ++probe->sensor_count;

    return 0;
}


/*
 *  *****  Verification  ***********************************************
 */



#if SK_ENABLE_IPFIX
/*
 *  is_valid = skpcProbeVerifyIPFIX(p);
 *
 *    Verify that probe has everything required to collect IPFIX data.
 */
static int
skpcProbeVerifyIPFIX(
    skpc_probe_t       *probe)
{
    /* skpcProbeVerify() should have verified that exactly one
     * collection mechanism is defined.  This function only needs to
     * ensure that this probe type supports that mechanism. */

    /* IPFIX does not support reading from files */
    if (probe->file_source != NULL) {
        return 0;
#if 0
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " the read-from-file clause"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
#endif
    }

    /* IPFIX does not support unix sockets */
    if (probe->unix_domain_path != NULL) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " the listen-on-unix-socket clause"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    /* Check for non-directory based options */
    if (probe->poll_directory == NULL) {
        /* Our IPFIX  support only allows UDP and TCP and has no default */
        switch (probe->protocol) {
          case SKPC_PROTO_UDP:
            break;
          case SKPC_PROTO_TCP:
            break;
          case SKPC_PROTO_UNSET:
            skAppPrintErr(("Error verifying probe '%s':\n"
                           "\tType '%s' probes must set"
                           " the protocol to 'tcp' or 'udp'"),
                          probe->probe_name,
                          skpcProbetypeEnumtoName(probe->probe_type));
            return -1;
          default:
            skAppPrintErr(("Error verifying probe '%s':\n"
                           "\tType '%s' probes only support"
                           " the 'udp' or 'tcp' protocol"),
                          probe->probe_name,
                          skpcProbetypeEnumtoName(probe->probe_type));
            return -1;
        }
    }


    return 0;
}
#endif  /* SK_ENABLE_IPFIX */


/*
 *  is_valid = skpcProbeVerifyNetflowV5(p);
 *
 *    Verify that probe has everything required to collect NetFlow-V5
 *    data.
 */
static int
skpcProbeVerifyNetflowV5(
    skpc_probe_t       *probe)
{
    /* skpcProbeVerify() should have verified that exactly one
     * collection mechanism is defined.  This function only needs to
     * ensure that this probe type supports that mechanism. */

    /* NetFlow does not support unix sockets */
    if (probe->unix_domain_path != NULL) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " the listen-on-unix-socket clause"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    /* NetFlow only supports the UDP protocol */
    if ((probe->listen_addr != NULL)
        && (probe->protocol != SKPC_PROTO_UDP))
    {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes only support"
                       " the 'udp' protocol"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    /* NetFlow v5 does not support VLAN interfaces */
    if (probe->ifvalue_vlan) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not have access"
                       " to vlan information"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    return 0;
}


#if SK_ENABLE_IPFIX
/*
 *  is_valid = skpcProbeVerifyNetflowV9(p);
 *
 *    Verify that probe has everything required to collect NetFlow-V9
 *    data.
 */
static int
skpcProbeVerifyNetflowV9(
    skpc_probe_t       *probe)
{
    /* skpcProbeVerify() should have verified that exactly one
     * collection mechanism is defined.  This function only needs to
     * ensure that this probe type supports that mechanism. */

    /* NetFlow v9 does not support reading from files */
    if (probe->file_source != NULL) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " the read-from-file clause"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    /* NetFlow v9 does not support unix sockets */
    if (probe->unix_domain_path != NULL) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " the listen-on-unix-socket clause"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    /* NetFlow v9 does not yet support directory polling */
    if (probe->poll_directory != NULL) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " the poll-directory clause"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    /* NetFlow only supports the UDP protocol */
    if ((probe->listen_addr != NULL)
        && (probe->protocol != SKPC_PROTO_UDP))
    {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes only support"
                       " the 'udp' protocol"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    return 0;
}
#endif  /* SK_ENABLE_IPFIX */


/*
 *  is_valid = skpcProbeVerifySilk(p);
 *
 *    Verify that probe has everything required to re-pack SiLK flow
 *    files.
 */
static int
skpcProbeVerifySilk(
    skpc_probe_t       *probe)
{
    /* The SiLK Flow file probe does not support reading from files */
    if (probe->file_source != NULL) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " the read-from-file clause"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    /* When re-packing SiLK Flow files, we do not support
     * network-based options.  */
    if (probe->unix_domain_path != NULL) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " the listen-on-unix-socket clause"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }
    if (probe->listen_addr != NULL) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tType '%s' probes do not support"
                       " listening on the network"),
                      probe->probe_name,
                      skpcProbetypeEnumtoName(probe->probe_type));
        return -1;
    }

    return 0;
}


/*
 *  is_valid = skpcProbeVerifyNetwork(p);
 *
 *    Verify that this network-based probe does not conflict with
 *    existing probes.
 */
static int
skpcProbeVerifyNetwork(
    const skpc_probe_t *probe)
{
    const skpc_probe_t **p;
    size_t i;

    /* Loop over all existing probes */
    for (i = 0;
         (p = (const skpc_probe_t**)skVectorGetValuePointer(skpc_probes, i))
             != NULL;
         ++i)
    {
        if (((*p)->protocol == probe->protocol)
            && skSockaddrArrayMatches((*p)->listen_addr,
                                      probe->listen_addr, 0))
        {
            /* Listen addresses match.  */

            /* Must have the same probe type */
            if (probe->probe_type != (*p)->probe_type) {
                skAppPrintErr(("Error verifying probe '%s':\n"
                               "\tThe listening port and address are the same"
                               " as probe '%s'\n\tand the probe types do not"
                               " match"),
                              probe->probe_name, (*p)->probe_name);
                return -1;
            }

            /* Check their accept_from addresses. */
            if (probe->accept_from_addr == NULL
                || (*p)->accept_from_addr == NULL
                || skSockaddrArrayMatches((*p)->accept_from_addr,
                                          probe->accept_from_addr,
                                          SK_SOCKADDRCOMP_NOPORT))
            {
                skAppPrintErr(("Error verifying probe '%s':\n"
                               "\tThe listening port and address are the same"
                               " as probe '%s';\n\tto distinguish each probe's"
                               " traffic, a unique value for the\n"
                               "\taccept-from-host clause is required on"
                               " each probe."),
                              probe->probe_name, (*p)->probe_name);
                return -1;
            }
        }
    }

    return 0;
}


/* Has probe been verified? */
int
skpcProbeIsVerified(
    const skpc_probe_t *probe)
{
    assert(probe);
    return probe->verified;
}


/*
 *    Verify that 'p' is a valid probe.
 */
int
skpcProbeVerify(
    skpc_probe_t       *probe,
    int                 is_ephemeral)
{
    size_t count;

    assert(probe);
    assert(skpc_probes);

    /* check name */
    if ('\0' == probe->probe_name[0]) {
        skAppPrintErr("Error verifying probe:\n\tProbe has no name.");
        return -1;
    }

    /* verify type is not invalid */
    if (probe->probe_type == PROBE_ENUM_INVALID) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tProbe's type is INVALID."),
                      probe->probe_name);
        return -1;
    }

    /* make certain no other probe has this name */
    if (skpcProbeLookupByName(probe->probe_name)) {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tA probe with this name is already defined"),
                      probe->probe_name);
        return -1;
    }

    /* if is_ephemeral is specified, add it to the global list of
     * probes but don't mark it as verified */
    if (is_ephemeral) {
        return skVectorAppendValue(skpc_probes, &probe);
    }

    /* when listen-as-host is specified, listen-on-port must be as
     * well */
    if ((probe->listen_addr != NULL)
        && (skSockaddrArraySize(probe->listen_addr) > 0)
        && (skSockaddrPort(skSockaddrArrayGet(probe->listen_addr, 0)) == 0))
    {
            skAppPrintErr(("Error verifying probe '%s':\n"
                           "\tThe listen-on-port clause is required when"
                           " listen-as-host is specified"),
                          probe->probe_name);
            return -1;
    }

    /* when listen-on-port is specifed, the protocol is also required */
    if ((probe->listen_addr != NULL)
        && (probe->protocol == SKPC_PROTO_UNSET))
    {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tThe protocol clause is required when"
                       " listen-on-port is specified"),
                      probe->probe_name);
        return -1;
    }

    /* when accept-from-host is specifed, listen-on-port must be
     * specified as well */
    if ((probe->accept_from_addr != NULL)
        && (probe->listen_addr == NULL))
    {
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tThe listen-on-port clause is required when"
                       " accept-from-host is specified"),
                      probe->probe_name);
        return -1;
    }

    /* check that one and only one of port, unix socket,
     * file-source, and poll-directory were given */
    count = 0;
    if (probe->listen_addr != NULL) {
        ++count;
    }
    if (probe->unix_domain_path != NULL) {
        ++count;
    }
    if (probe->file_source != NULL) {
        ++count;
    }
    if (probe->poll_directory != NULL) {
        ++count;
    }

    if (count != 1) {
        if (count == 0) {
            skAppPrintErr(("Error verifying probe '%s':\n"
                           "\tProbe needs a collection source; must give one"
                           " of listen-on-port,\n\tpoll-directory,"
                           " listen-on-unix-socket, or read-from-file."),
                          probe->probe_name);
        } else {
            skAppPrintErr(("Error verifying probe '%s':\n"
                           "\tMultiple collection sources; must give only one"
                           " of listen-on-port,\n\tpoll-directory,"
                           " listen-on-unix-socket, or read-from-file."),
                          probe->probe_name);
        }
        return -1;
    }

    /* when poll-directory is specified, no other probe can specify
     * that same directory */
    if (probe->poll_directory != NULL) {
        const skpc_probe_t **p;
        size_t i;

        /* loop over all probes checking the poll-directory */
        for (i = 0;
             (p = (const skpc_probe_t**)skVectorGetValuePointer(skpc_probes,i))
                 != NULL;
             ++i)
        {
            if ((*p)->poll_directory
                && (0 == strcmp(probe->poll_directory, (*p)->poll_directory)))
            {
                skAppPrintErr(("Error verifying probe '%s':\n"
                               "\tThe poll-directory must be unique, but"
                               " probe '%s' is\n\talso polling '%s'"),
                              probe->probe_name, (*p)->probe_name,
                              probe->poll_directory);
                return -1;
            }
        }
    }

    /* when listening on a port, make sure we're not tromping over
     * other probes' ports */
    if (probe->listen_addr != NULL && skpcProbeVerifyNetwork(probe)) {
        return -1;
    }

    /* verify the probe by its type */
    switch (probe->probe_type) {
      case PROBE_ENUM_NETFLOW_V5:
        if (0 != skpcProbeVerifyNetflowV5(probe)) {
            return -1;
        }
        break;


      case PROBE_ENUM_IPFIX:
#if !SK_ENABLE_IPFIX
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tIPFIX support requires libfixbuf-%s or later"
                       " and the required\n"
                       "\tlibfixbuf version was not included at compile time"),
                      probe->probe_name, SKPC_LIBFIXBUF_VERSION_IPFIX);
#else
        if (0 == skpcProbeVerifyIPFIX(probe)) {
            break;
        }
#endif /* SK_ENABLE_IPFIX */
        return -1;

      case PROBE_ENUM_NETFLOW_V9:
#if !SK_ENABLE_IPFIX
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tNetFlow v9 support requires libfixbuf-%s or later"
                       " and the required\n\t"
                       "libfixbuf version was not included at compile time"),
                      probe->probe_name, SKPC_LIBFIXBUF_VERSION_NETFLOWV9);
#else
        if (0 == skpcProbeVerifyNetflowV9(probe)) {
            break;
        }
#endif /* SK_ENABLE_IPFIX */
        return -1;

      case PROBE_ENUM_SFLOW:
#if !SK_ENABLE_IPFIX_SFLOW
        skAppPrintErr(("Error verifying probe '%s':\n"
                       "\tsFlow support requires libfixbuf-%s or later"
                       " and the required\n\t"
                       "libfixbuf version was not included at compile time"),
                      probe->probe_name, SKPC_LIBFIXBUF_VERSION_SFLOW);
#else
        /* sFlow probes have same requirements as NetFlow v9 */
        if (0 == skpcProbeVerifyNetflowV9(probe)) {
            break;
        }
#endif /* SK_ENABLE_IPFIX */
        return -1;

      case PROBE_ENUM_SILK:
        if (0 != skpcProbeVerifySilk(probe)) {
            return -1;
        }
        break;

      case PROBE_ENUM_INVALID:
        /* should have caught this above */
        skAbortBadCase(probe->probe_type);
    }

    /* probe is valid; add it to the global vector of probes */
    if (skVectorAppendValue(skpc_probes, &probe)) {
        return -1;
    }

    probe->verified = 1;
    return 0;
}


/*
 *  *****  Sensors  *****************************************************
 */

/* Create a sensor */
int
skpcSensorCreate(
    skpc_sensor_t     **sensor)
{
    assert(sensor);

    (*sensor) = (skpc_sensor_t*)calloc(1, sizeof(skpc_sensor_t));
    if (NULL == (*sensor)) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }

    (*sensor)->sensor_id = SK_INVALID_SENSOR;

    (*sensor)->fixed_network[0] = SKPC_NETWORK_ID_INVALID;
    (*sensor)->fixed_network[1] = SKPC_NETWORK_ID_INVALID;

    /* create the decider array; one for each network */
    (*sensor)->decider_count = skVectorGetCount(skpc_networks);
    if ((*sensor)->decider_count) {
        (*sensor)->decider = (skpc_netdecider_t*)calloc(
            (*sensor)->decider_count, sizeof(skpc_netdecider_t));
        if ((*sensor)->decider == NULL) {
            skAppPrintOutOfMemory(NULL);
            free(*sensor);
            return -1;
        }
    }

    return 0;
}


void
skpcSensorDestroy(
    skpc_sensor_t     **sensor)
{
    size_t i;

    if (!sensor || !(*sensor)) {
        return;
    }

    /* set the sensor's deciders' group to NULL, then destroy the
     * deciders */
    for (i = 0; i < (*sensor)->decider_count; ++i) {
        (*sensor)->decider[i].nd_group = NULL;
    }
    (*sensor)->decider_count = 0;

    if ((*sensor)->decider) {
        free((*sensor)->decider);
        (*sensor)->decider = NULL;
    }

    /* destroy the probe list */
    if ((*sensor)->probe_list) {
        free((*sensor)->probe_list);
        (*sensor)->probe_list = NULL;
        (*sensor)->probe_count = 0;
    }

    /* set the 'group' reference on all filters to NULL, then destroy
     * the filters array. */
    for (i = 0; i < (*sensor)->filter_count; ++i) {
        (*sensor)->filter[i].f_group = NULL;
    }
    (*sensor)->filter_count = 0;

    if ((*sensor)->filter) {
        free((*sensor)->filter);
        (*sensor)->filter = NULL;
    }

    /* destroy other attributes of the sensor */
    if ((*sensor)->isp_ip_count) {
        free((*sensor)->isp_ip_list);
        (*sensor)->isp_ip_list = NULL;
        (*sensor)->isp_ip_count = 0;
    }
    if ((*sensor)->sensor_name) {
        free((*sensor)->sensor_name);
    }

    /* destroy the sensor itself */
    free(*sensor);
    *sensor = NULL;
}


/* Get and set the name of this sensor. */
sensorID_t
skpcSensorGetID(
    const skpc_sensor_t    *sensor)
{
    assert(sensor);
    return sensor->sensor_id;
}

const char *
skpcSensorGetName(
    const skpc_sensor_t    *sensor)
{
    assert(sensor);
    return sensor->sensor_name;
}

int
skpcSensorSetName(
    skpc_sensor_t      *sensor,
    const char         *name)
{
    char *copy;

    assert(sensor);
    if (name == NULL || name[0] == '\0') {
        return -1;
    }

    copy = strdup(name);
    if (copy == NULL) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    if (sensor->sensor_name) {
        free(sensor->sensor_name);
    }
    sensor->sensor_name = copy;

    sensor->sensor_id = sksiteSensorLookup(name);
    return 0;
}


/* Count the number of SNMP interfaces that have been mapped to a
 * flowtype on 'sensor' excluding the network 'ignored_network_id' */
uint32_t
skpcSensorCountNetflowInterfaces(
    const skpc_sensor_t    *sensor,
    int                     ignored_network_id)
{
    uint32_t ifcount = 0;
    size_t i;

    for (i = 0; i < sensor->decider_count; ++i) {
        if (ignored_network_id == (int)i) {
            continue;
        }
        if (((sensor->decider[i].nd_type == SKPC_INTERFACE)
             || (sensor->decider[i].nd_type == SKPC_REMAIN_INTERFACE))
            && (sensor->decider[i].nd_group != NULL))
        {
            ifcount += skpcGroupGetItemCount(sensor->decider[i].nd_group);
        }
    }
    return ifcount;
}


/* Test rwrec against the interfaces (either SNMP or IP block) on the
 * sensor, or used the fixed network value if set. */
int
skpcSensorTestFlowInterfaces(
    const skpc_sensor_t    *sensor,
    const rwRec            *rwrec,
    skpc_network_id_t       network_id,
    skpc_direction_t        rec_dir)
{
    skipaddr_t ip;
    int found = 0;

    assert(sensor);
    assert(rwrec);
    assert(network_id <= SKPC_NETWORK_ID_MAX);
    assert(rec_dir == SKPC_DIR_SRC || rec_dir == SKPC_DIR_DST);

    /* use the fixed value if provided */
    if (sensor->fixed_network[rec_dir] == network_id) {
        return 1;
    }

    switch (sensor->decider[network_id].nd_type) {
      case SKPC_UNSET:
        break;

      case SKPC_INTERFACE:
      case SKPC_REMAIN_INTERFACE:
        /* An SNMP interface list was set for the network_id.  Test
         * the record's SNMP value against it.  Whether incoming or
         * outgoing depends on 'rec_dir'. */
        if (rec_dir == SKPC_DIR_SRC) {
            /* look where the record is coming from: its input SNMP
             * interface */
            if (skpcGroupCheckInterface(sensor->decider[network_id].nd_group,
                                        rwRecGetInput(rwrec)))
            {
                return 1;
            }
        } else {
            /* look where the record is going to: output SNMP */
            assert(rec_dir == SKPC_DIR_DST);
            if (skpcGroupCheckInterface(sensor->decider[network_id].nd_group,
                                        rwRecGetOutput(rwrec)))
            {
                return 1;
            }
        }
        return -1;

      case SKPC_NEG_IPBLOCK:
      case SKPC_REMAIN_IPBLOCK:
        /* want to find whether the IP is NOT in the list */
        found = 1;
        /* FALLTHROUGH */

      case SKPC_IPBLOCK:
        /* An IP block was set for 'network_id'.  Test the record's IP
         * against it.  Whether source IP or dest IP depends on
         * 'rec_dir'. */
        if (rec_dir == SKPC_DIR_SRC) {
            /* look where the record is coming from: its source IP */
            rwRecMemGetSIP(rwrec, &ip);
        } else {
            /* look where the record is going to: destination IP */
            assert(rec_dir == SKPC_DIR_DST);
            rwRecMemGetDIP(rwrec, &ip);
        }

        if (skpcGroupCheckIp(sensor->decider[network_id].nd_group, &ip)) {
            found = !found;
        }
        return ((found == 0) ? -1 : 1);
    }

    return 0;
}


/* Return non-zero if 'rwrec' matches ANY of the "discard-when"
 * filters on 'sensor' or if it does not match ALL of the
 * "discard-unless" filters. */
int
skpcSensorCheckFilters(
    const skpc_sensor_t    *sensor,
    const rwRec            *rwrec)
{
    skipaddr_t sip;
    skipaddr_t dip;
    const skpc_filter_t *filter;
    size_t discard;
    size_t j;

    assert(sensor);
    assert(rwrec);

    for (j = 0, filter = sensor->filter;
         j < sensor->filter_count;
         ++j, ++filter)
    {
        discard = !filter->f_discwhen;
        if (filter->f_wildcard) {
            switch (filter->f_type) {
              case SKPC_FILTER_SOURCE:
                rwRecMemGetSIP(rwrec, &sip);
                if (skpcGroupCheckIp(filter->f_group, &sip)) {
                    discard = !discard;
                }
                break;

              case SKPC_FILTER_DESTINATION:
                rwRecMemGetDIP(rwrec, &dip);
                if (skpcGroupCheckIp(filter->f_group, &dip)) {
                    discard = !discard;
                }
                break;

              case SKPC_FILTER_ANY:
                rwRecMemGetSIP(rwrec, &sip);
                rwRecMemGetDIP(rwrec, &dip);
                if (skpcGroupCheckIp(filter->f_group, &sip)
                    || skpcGroupCheckIp(filter->f_group, &dip))
                {
                    discard = !discard;
                }
                break;
            }
        } else {
            switch (filter->f_type) {
              case SKPC_FILTER_SOURCE:
                if (skpcGroupCheckInterface(filter->f_group,
                                            rwRecGetInput(rwrec)))
                {
                    discard = !discard;
                }
                break;

              case SKPC_FILTER_DESTINATION:
                if (skpcGroupCheckInterface(filter->f_group,
                                            rwRecGetOutput(rwrec)))
                {
                    discard = !discard;
                }
                break;

              case SKPC_FILTER_ANY:
                if (skpcGroupCheckInterface(filter->f_group,
                                            rwRecGetInput(rwrec))
                    || skpcGroupCheckInterface(filter->f_group,
                                               rwRecGetOutput(rwrec)))
                {
                    discard = !discard;
                }
                break;
            }
        }
        if (discard) {
            return 1;
        }
    }

    return 0;
}


int
skpcSensorSetNetwork(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id,
    skpc_direction_t    dir)
{
    const skpc_network_t *network;
    const char *prev_decider = NULL;

    assert(sensor);
    assert(network_id <= SKPC_NETWORK_ID_INVALID);
    assert(dir == SKPC_DIR_SRC || dir == SKPC_DIR_DST);

    /* get network */
    network = skpcNetworkLookupByID(network_id);
    if (network == NULL) {
        return -1;
    }

    /* verify that value not previously set */
    if (sensor->fixed_network[dir] != SKPC_NETWORK_ID_INVALID) {
        skAppPrintErr(("Error setting %s-network on sensor '%s':\n"
                       "\tCannot overwrite existing value"),
                      ((dir == 0) ? "source" : "destination"),
                      sensor->sensor_name);
        return -1;
    }

    /* verify that no ipblocks or interfaces have been set for this
     * network */
    switch (sensor->decider[network->id].nd_type) {
      case SKPC_UNSET:
        /* valid */
        break;

      case SKPC_INTERFACE:
      case SKPC_REMAIN_INTERFACE:
        prev_decider = "interface";
        break;

      case SKPC_NEG_IPBLOCK:
      case SKPC_REMAIN_IPBLOCK:
      case SKPC_IPBLOCK:
        prev_decider = "ipblock";
        break;
    }

    if (prev_decider) {
        skAppPrintErr(("Error setting %s-network on sensor '%s':\n"
                       "\tA %s-%s value has already been set"),
                      ((dir == 0) ? "source" : "destination"),
                      sensor->sensor_name, network->name, prev_decider);
        return -1;
    }

    sensor->fixed_network[dir] = network->id;
    return 0;
}


/* Set the list of IP blocks that represent a network */
int
skpcSensorSetIpBlocks(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id,
    const skpc_group_t *ip_group,
    int                 is_negatated)
{
    const skpc_network_t *network;
    size_t i;

    /* check input */
    assert(sensor);
    assert(network_id <= SKPC_NETWORK_ID_INVALID);

    /* check that group has data and that it is the correct type */
    if (ip_group == NULL) {
        return -1;
    }
    if (!skpcGroupIsFrozen(ip_group)
        || (skpcGroupGetItemCount(ip_group) == 0)
        || (SKPC_GROUP_IPBLOCK != skpcGroupGetType(ip_group)))
    {
        return -1;
    }

    /* get network */
    network = skpcNetworkLookupByID(network_id);
    if (network == NULL) {
        return -1;
    }
    assert(network->id < sensor->decider_count);

    /* cannot set ipblocks when the source/destination network has
     * been fixed to this network_id. */
    for (i = 0; i < 2; ++i) {
        if (sensor->fixed_network[i] == network_id) {
            skAppPrintErr(("Error setting IP blocks on sensor '%s':\n"
                           "\tAll flows are assumed to be %s the %s network"),
                          sensor->sensor_name,
                          ((i == 0) ? "coming from" : "going to"),
                          network->name);
            return -1;
        }
    }

    /* check that we're not attempting to change an existing value */
    if (sensor->decider[network->id].nd_type != SKPC_UNSET) {
        skAppPrintErr(("Error setting IP block on sensor '%s':\n"
                       "\tCannot overwrite existing %s network value"),
                      sensor->sensor_name, network->name);
        return -1;
    }

    sensor->decider[network->id].nd_group = ip_group;
    sensor->decider[network->id].nd_type =
        (is_negatated ? SKPC_NEG_IPBLOCK : SKPC_IPBLOCK);

    return 0;
}


/* Set the specified interface to all IPs not covered by other interfaces */
int
skpcSensorSetToRemainderIpBlocks(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id)
{
    const skpc_network_t *network;
    int i;

    assert(sensor);
    assert(network_id <= SKPC_NETWORK_ID_INVALID);

    /* get network */
    network = skpcNetworkLookupByID(network_id);
    if (network == NULL) {
        return -1;
    }

    assert(network->id < sensor->decider_count);

    /* cannot set ipblocks when the source/destination network has
     * been fixed to this network_id. */
    for (i = 0; i < 2; ++i) {
        if (sensor->fixed_network[i] == network_id) {
            skAppPrintErr(("Error setting IP block on sensor '%s':\n"
                           "\tAll flows are assumed to be %s the %s network"),
                          sensor->sensor_name,
                          ((i == 0) ? "coming from" : "going to"),
                          network->name);
            return -1;
        }
    }

    /* check that we're not attempting to change an existing value */
    if (sensor->decider[network->id].nd_type != SKPC_UNSET) {
        skAppPrintErr(("Error setting IP block on sensor '%s':\n"
                       "\tCannot overwrite existing %s network value"),
                      sensor->sensor_name, network->name);
        return -1;
    }

    sensor->decider[network->id].nd_type = SKPC_REMAIN_IPBLOCK;

    return 0;
}


int
skpcSensorSetDefaultNonrouted(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id)
{
    sk_vector_t *ifvec = NULL;
    const uint32_t default_nonrouted = 0;
    int rv = -1;

    assert(sensor);
    assert(network_id <= SKPC_NETWORK_ID_INVALID);

    if (NULL == nonrouted_group) {
        ifvec = skVectorNew(sizeof(uint32_t));
        if (skpc_probes == NULL) {
            skAppPrintOutOfMemory(NULL);
            goto END;
        }
        if (skVectorAppendValue(ifvec, &default_nonrouted)) {
            skAppPrintOutOfMemory(NULL);
            goto END;
        }

        if (skpcGroupCreate(&nonrouted_group)) {
            skAppPrintOutOfMemory(NULL);
            goto END;
        }
        skpcGroupSetType(nonrouted_group, SKPC_GROUP_INTERFACE);
        if (skpcGroupAddValues(nonrouted_group, ifvec)) {
            skAppPrintOutOfMemory(NULL);
            goto END;
        }
        skpcGroupFreeze(nonrouted_group);
    }

    rv = skpcSensorSetInterfaces(sensor, network_id, nonrouted_group);
  END:
    if (ifvec) {
        skVectorDestroy(ifvec);
    }
    return rv;
}


/* Set the list of SNMP interfaces on the specified interface group */
int
skpcSensorSetInterfaces(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id,
    const skpc_group_t *if_group)
{
    const skpc_network_t *network;
    size_t i;

    /* check input */
    assert(sensor);
    assert(network_id <= SKPC_NETWORK_ID_INVALID);

    /* check that group has data and that it is the correct type */
    if (if_group == NULL) {
        return -1;
    }
    if (!skpcGroupIsFrozen(if_group)
        || (skpcGroupGetItemCount(if_group) == 0)
        || (SKPC_GROUP_INTERFACE != skpcGroupGetType(if_group)))
    {
        return -1;
    }

    /* get network */
    network = skpcNetworkLookupByID(network_id);
    if (network == NULL) {
        return -1;
    }
    assert(network->id < sensor->decider_count);

    /* cannot set interfaces when the source/destination network has
     * been fixed to this network_id. */
    for (i = 0; i < 2; ++i) {
        if (sensor->fixed_network[i] == network_id) {
            skAppPrintErr(("Error setting interfaces on sensor '%s':\n"
                           "\tAll flows are assumed to be %s the %s network"),
                          sensor->sensor_name,
                          ((i == 0) ? "coming from" : "going to"),
                          network->name);
            return -1;
        }
    }

    /* check that we're not attempting to change an existing value */
    if (sensor->decider[network->id].nd_type != SKPC_UNSET) {
        skAppPrintErr(("Error setting interfaces on sensor '%s':\n"
                       "\tCannot overwrite existing %s network value"),
                      sensor->sensor_name, network->name);
        return -1;
    }

    sensor->decider[network->id].nd_group = if_group;
    sensor->decider[network->id].nd_type = SKPC_INTERFACE;

    return 0;
}


/* Set the SNMP interface list for the specified 'network' to all
 * interfaces not mentioned in other lists. */
int
skpcSensorSetToRemainderInterfaces(
    skpc_sensor_t      *sensor,
    skpc_network_id_t   network_id)
{
    const skpc_network_t *network;
    int i;

    assert(sensor);
    assert(network_id <= SKPC_NETWORK_ID_INVALID);

    /* get network */
    network = skpcNetworkLookupByID(network_id);
    if (network == NULL) {
        return -1;
    }

    assert(network->id < sensor->decider_count);

    /* cannot set interfaces when the source/destination network has
     * been fixed to this network_id. */
    for (i = 0; i < 2; ++i) {
        if (sensor->fixed_network[i] == network_id) {
            skAppPrintErr(("Error setting interfaces on sensor '%s':\n"
                           "\tAll flows are assumed to be %s the %s network"),
                          sensor->sensor_name,
                          ((i == 0) ? "coming from" : "going to"),
                          network->name);
            return -1;
        }
    }
    /* check that we're not attempting to change an existing value */
    if (sensor->decider[network->id].nd_type != SKPC_UNSET) {
        skAppPrintErr(("Error setting IP block on sensor '%s':\n"
                       "\tCannot overwrite existing %s network value"),
                      sensor->sensor_name, network->name);
        return -1;
    }

    sensor->decider[network->id].nd_type = SKPC_REMAIN_INTERFACE;

    return 0;
}


static int
skpcSensorComputeRemainingInterfaces(
    skpc_sensor_t      *sensor)
{
    size_t remain_network = SKPC_NETWORK_ID_INVALID;
    size_t i;
    skpc_group_t *group = NULL;

    /* determine which network has claimed the 'remainder' */
    for (i = 0; i < sensor->decider_count; ++i) {
        if (sensor->decider[i].nd_type == SKPC_REMAIN_INTERFACE) {
            if (remain_network != SKPC_NETWORK_ID_INVALID) {
                /* cannot have more than one "remainder" */
                skAppPrintErr(("Cannot verify sensor '%s':\n"
                               "\tMultiple network values claim 'remainder'"),
                              sensor->sensor_name);
                return -1;
            }
            remain_network = i;
        }
    }

    if (remain_network == SKPC_NETWORK_ID_INVALID) {
        /* no one is set to remainder; return */
        return 0;
    }

    /* create a new group */
    if (skpcGroupCreate(&group)) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    skpcGroupSetType(group, SKPC_GROUP_INTERFACE);

    sensor->decider[remain_network].nd_group = group;

    /* add all existing groups to the new group */
    for (i = 0; i < sensor->decider_count; ++i) {
        if (sensor->decider[i].nd_type == SKPC_INTERFACE) {
            if (skpcGroupAddGroup(group, sensor->decider[i].nd_group)) {
                skAppPrintOutOfMemory(NULL);
                return -1;
            }
        }
    }

    /* take the complement of the group, then freeze it */
    skpcGroupComputeComplement(group);
    skpcGroupFreeze(group);

    return 0;
}


static int
skpcSensorComputeRemainingIpBlocks(
    skpc_sensor_t      *sensor)
{
    size_t remain_network = SKPC_NETWORK_ID_INVALID;
    int has_ipblocks = 0;
    skpc_group_t *group = NULL;
    size_t i;

    /* determine which network has claimed the 'remainder'. At the
     * same time, verify that at least one network has 'ipblocks' */
    for (i = 0; i < sensor->decider_count; ++i) {
        if (sensor->decider[i].nd_type == SKPC_REMAIN_IPBLOCK) {
            if (remain_network != SKPC_NETWORK_ID_INVALID) {
                /* cannot have more than one "remainder" */
                skAppPrintErr(("Cannot verify sensor '%s':\n"
                               "\tMultiple network values claim 'remainder'"),
                              sensor->sensor_name);
                return -1;
            }
            remain_network = i;
        } else if (sensor->decider[i].nd_type == SKPC_IPBLOCK) {
            has_ipblocks = 1;
        }
    }

    if (remain_network == SKPC_NETWORK_ID_INVALID) {
        /* no one is set to remainder; return */
        return 0;
    }

    /* need to have existing IPblocks to set a remainder */
    if (has_ipblocks == 0) {
        const skpc_network_t *network = skpcNetworkLookupByID(remain_network);
        skAppPrintErr(("Cannot verify sensor '%s':\n"
                       "\tCannot set %s-ipblocks to remaining IP because\n"
                       "\tno other interfaces hold IP blocks"),
                      sensor->sensor_name, network->name);
        return -1;
    }

    /* create a new group */
    if (skpcGroupCreate(&group)) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    skpcGroupSetType(group, SKPC_GROUP_IPBLOCK);

    sensor->decider[remain_network].nd_group = group;

    /* add all existing groups to the new group */
    for (i = 0; i < sensor->decider_count; ++i) {
        if (sensor->decider[i].nd_type == SKPC_IPBLOCK) {
            if (skpcGroupAddGroup(group, sensor->decider[i].nd_group)) {
                skAppPrintOutOfMemory(NULL);
                return -1;
            }
        }
    }

    /* freze the group */
    skpcGroupFreeze(group);

    return 0;
}


/* add a new discard-{when,unless} list to 'sensor' */
int
skpcSensorAddFilter(
    skpc_sensor_t      *sensor,
    const skpc_group_t *group,
    skpc_filter_type_t  filter_type,
    int                 is_discardwhen_list,
    int                 is_wildcard_list)
{
    const char *filter_name;
    skpc_filter_t *filter;
    size_t j;
    int rv = -1;

    assert(sensor);

    /* check that group has data and that it is the correct type */
    if (group == NULL) {
        return -1;
    }
    if (!skpcGroupIsFrozen(group)
        || (skpcGroupGetItemCount(group) == 0)
        || (skpcGroupGetType(group) != (is_wildcard_list
                                        ? SKPC_GROUP_IPBLOCK
                                        : SKPC_GROUP_INTERFACE)))
    {
        return -1;
    }

    /* verify we are not attempting to overwrite a value */
    for (j = 0, filter = sensor->filter;
         j < sensor->filter_count;
         ++j, ++filter)
    {
        if (filter->f_type != filter_type) {
            continue;
        }
        if ((is_wildcard_list && !filter->f_wildcard)
            || (!is_wildcard_list && filter->f_wildcard))
        {
            continue;
        }

        /* error */
        switch (filter_type) {
          case SKPC_FILTER_ANY:
            filter_name = "any";
            break;
          case SKPC_FILTER_DESTINATION:
            filter_name = "destination";
            break;
          case SKPC_FILTER_SOURCE:
            filter_name = "source";
            break;
          default:
            skAbortBadCase(filter_type);
        }
        skAppPrintErr(("Error setting discard-%s list on sensor '%s':\n"
                       "\tCannot overwrite existing %s-%s list"),
                      (is_discardwhen_list ? "when" : "unless"),
                      sensor->sensor_name, filter_name,
                      (is_wildcard_list ? "ipblocks" : "interfaces"));
        return -1;
    }

    /* if this is the first filter, allocate space for all the filters
     * on this sensor */
    if (NULL == sensor->filter) {
        assert(0 == sensor->filter_count);
        /* allow room for both interface-filters and ipblock-filters */
        sensor->filter = (skpc_filter_t*)calloc(2 * SKPC_NUM_FILTER_TYPES,
                                                sizeof(skpc_filter_t));
        if (NULL == sensor->filter) {
            skAppPrintOutOfMemory(NULL);
            goto END;
        }
    }

    assert(sensor->filter_count < (2 * SKPC_NUM_FILTER_TYPES));

    filter = &sensor->filter[sensor->filter_count];

    filter->f_group = group;
    filter->f_type = filter_type;
    filter->f_wildcard = (is_wildcard_list ? 1 : 0);
    filter->f_discwhen = (is_discardwhen_list ? 1 : 0);

    ++sensor->filter_count;

    rv = 0;

  END:
    return rv;
}


/* Get and set the list of IPs of the ISP */
uint32_t
skpcSensorGetIspIps(
    const skpc_sensor_t    *sensor,
    const uint32_t        **out_ip_list)
{
    assert(sensor);

    if (sensor->isp_ip_count > 0) {
        if (out_ip_list != NULL) {
            *out_ip_list = sensor->isp_ip_list;
        }
    }
    return sensor->isp_ip_count;
}

int
skpcSensorSetIspIps(
    skpc_sensor_t      *sensor,
    const sk_vector_t  *isp_ip_vec)
{
    size_t count;
    uint32_t *copy;

    /* check input */
    assert(sensor);
    if (isp_ip_vec == NULL) {
        return -1;
    }
    count = skVectorGetCount(isp_ip_vec);
    if (count == 0) {
        return -1;
    }

    /* copy the values out of the vector */
    copy = (uint32_t*)malloc(count * skVectorGetElementSize(isp_ip_vec));
    if (copy == NULL) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    skVectorToArray(copy, isp_ip_vec);

    /* free the existing list and set value on the sensor */
    if (sensor->isp_ip_count) {
        free(sensor->isp_ip_list);
    }
    sensor->isp_ip_list = copy;
    sensor->isp_ip_count = count;

    return 0;
}


uint32_t
skpcSensorGetProbes(
    const skpc_sensor_t    *sensor,
    sk_vector_t            *out_probe_vec)
{
    assert(sensor);

    if (sensor->probe_count != 0 && out_probe_vec != NULL) {
        if (skVectorAppendFromArray(out_probe_vec, sensor->probe_list,
                                    sensor->probe_count))
        {
            skAppPrintOutOfMemory(NULL);
            return 0;
        }
    }
    return sensor->probe_count;
}

int
skpcSensorSetProbes(
    skpc_sensor_t      *sensor,
    const sk_vector_t  *probe_vec)
{
    size_t count;
    void *copy;

    /* check input */
    assert(sensor);
    if (probe_vec == NULL) {
        return -1;
    }
    count = skVectorGetCount(probe_vec);
    if (count == 0) {
        return -1;
    }

    /* copy the values out of the vector */
    if (0 == sensor->probe_count) {
        /* create a new array */
        copy = malloc(count * skVectorGetElementSize(probe_vec));
        if (NULL == copy) {
            skAppPrintOutOfMemory(NULL);
            return -1;
        }
        sensor->probe_list = (skpc_probe_t**)copy;
        sensor->probe_count = count;
    } else {
        /* grow current array */
        copy = sensor->probe_list;
        sensor->probe_list
            = (skpc_probe_t**)realloc(copy, (skVectorGetElementSize(probe_vec)
                                             * (sensor->probe_count + count)));
        if (NULL == sensor->probe_list) {
            sensor->probe_list = (skpc_probe_t**)copy;
            skAppPrintOutOfMemory(NULL);
            return -1;
        }
        /* point 'copy' at new memory location */
        copy = sensor->probe_list + sensor->probe_count;
        sensor->probe_count += count;
    }

    skVectorToArray(copy, probe_vec);

    return 0;
}


int
skpcSensorVerify(
    skpc_sensor_t      *sensor,
    int               (*site_sensor_verify_fn)(skpc_sensor_t *sensor))
{
    uint32_t i;

    assert(sensor);

    if (sensor->sensor_id == SK_INVALID_SENSOR) {
        skAppPrintErr(("Error verifying sensor '%s'\n"
                       "\tSensor is not defined in site file silk.conf"),
                      sensor->sensor_name);
        return -1;
    }

#if 0
    /* 2008.05.16 --- Allow sensors to be defined multiple times.  The
     * add-sensor-to-probe call below will fail if we attempt to
     * define two sensors with the same name that each process the
     * same probe. */

    /* make certain no other sensor has this name */
    if (skpcSensorLookupByName(sensor->sensor_name)) {
        skAppPrintErr(("Error verifying sensor '%s':\n"
                       "\tA sensor with this name is already defined"),
                      sensor->sensor_name);
        return -1;
    }
#endif  /* 0 */

    /* verifying the sensor for this site (i.e., by its class) */
    if (site_sensor_verify_fn != NULL) {
        if (0 != site_sensor_verify_fn(sensor)) {
            return -1;
        }
    }

    /* if any network decider is set to 'remainder', update the sensor */
    if (skpcSensorComputeRemainingInterfaces(sensor)) {
        return -1;
    }
    if (skpcSensorComputeRemainingIpBlocks(sensor)) {
        return -1;
    }

    /* add a link on each probe to this sensor */
    for (i = 0; i < sensor->probe_count; ++i) {
        if (skpcProbeAddSensor(sensor->probe_list[i], sensor)) {
            skAppPrintErr(("Error verifying sensor '%s':\n"
                           "\tCannot link probe '%s' to this sensor"),
                          sensor->sensor_name,
                          sensor->probe_list[i]->probe_name);
            return -1;
        }
    }

    /* sensor is valid; add it to the global vector of sensors */
    if (skVectorAppendValue(skpc_sensors, &sensor)) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }

    return 0;
}


/*
 *  *****  Groups  ***********************************************************
 */

/* create a new group */
int
skpcGroupCreate(
    skpc_group_t      **group)
{
    skpc_group_t *g;

    assert(group);

    g = (skpc_group_t*)calloc(1, sizeof(skpc_group_t));
    if (NULL == g) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }

    g->g_type = SKPC_GROUP_UNSET;

    *group = g;

    return 0;
}

/* destroy a group */
void
skpcGroupDestroy(
    skpc_group_t      **group)
{
    if (!group || !(*group)) {
        return;
    }

    switch ((*group)->g_type) {
      case SKPC_GROUP_UNSET:
        break;
      case SKPC_GROUP_INTERFACE:
        if ((*group)->g_value.map) {
            skBitmapDestroy(&((*group)->g_value.map));
            (*group)->g_value.map = NULL;
        }
        break;
      case SKPC_GROUP_IPBLOCK:
        if ((*group)->g_is_frozen) {
            if ((*group)->g_value.ipblock) {
                free((*group)->g_value.ipblock);
                (*group)->g_value.ipblock = NULL;
            }
        } else if ((*group)->g_value.vec) {
            skVectorDestroy((*group)->g_value.vec);
            (*group)->g_value.vec = NULL;
        }
        break;
    }

    if ((*group)->g_name) {
        free((*group)->g_name);
        (*group)->g_name = NULL;
    }

    free(*group);
    *group = NULL;
}


/* mark group as unchangeable, convert ipblock vector to array, and
 * count number of items  */
int
skpcGroupFreeze(
    skpc_group_t       *group)
{
    size_t count;
    skIPWildcard_t **ipwild_list;
    sk_vector_t *ipblock_vec;

    assert(group);
    if (group->g_is_frozen) {
        return 0;
    }

    if (SKPC_GROUP_UNSET == group->g_type) {
        /* nothing else do */
        goto END;
    }

    if (SKPC_GROUP_INTERFACE == group->g_type) {
        group->g_itemcount = skBitmapGetHighCount(group->g_value.map);
        goto END;
    }

    assert(SKPC_GROUP_IPBLOCK == group->g_type);

    /* convert the vector to an array */
    ipblock_vec = group->g_value.vec;
    count = skVectorGetCount(ipblock_vec);

    /* allocate memory for the list of ip-wildcards */
    ipwild_list = (skIPWildcard_t**)malloc(count * sizeof(skIPWildcard_t*));
    if (NULL == ipwild_list) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }

    /* convert vector to an array */
    skVectorToArray(ipwild_list, ipblock_vec);

    /* finished with the vector */
    skVectorDestroy(ipblock_vec);

    group->g_value.ipblock = ipwild_list;
    group->g_itemcount = count;

  END:
    group->g_is_frozen = 1;
    if (skVectorAppendValue(skpc_groups, &group)) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    return 0;
}


/* get or set the name of the group */
const char *
skpcGroupGetName(
    const skpc_group_t *group)
{
    assert(group);
    return group->g_name;
}

int
skpcGroupSetName(
    skpc_group_t       *group,
    const char         *name)
{
    const char *cp;
    char *copy;

    assert(group);
    if (group->g_is_frozen) {
        return -1;
    }

    if (name == NULL || name[0] == '\0') {
        return -1;
    }

    /* check for illegal characters */
    cp = name;
    while (*cp) {
        if (*cp == '/' || isspace((int)*cp)) {
            return -1;
        }
        ++cp;
    }

    copy = strdup(name);
    if (copy == NULL) {
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    if (group->g_name) {
        free(group->g_name);
    }
    group->g_name = copy;
    return 0;
}


/* get or set the type of the group */
skpc_group_type_t
skpcGroupGetType(
    const skpc_group_t *group)
{
    assert(group);
    return group->g_type;
}

int
skpcGroupSetType(
    skpc_group_t       *group,
    skpc_group_type_t   group_type)
{
    assert(group);
    if (group->g_is_frozen) {
        return -1;
    }
    if (SKPC_GROUP_UNSET != group->g_type) {
        return -1;
    }

    switch (group_type) {
      case SKPC_GROUP_UNSET:
        return -1;

      case SKPC_GROUP_INTERFACE:
        if (skBitmapCreate(&group->g_value.map, SK_SNMP_INDEX_LIMIT)) {
            return -1;
        }
        break;

      case SKPC_GROUP_IPBLOCK:
        group->g_value.vec = skVectorNew(sizeof(skIPWildcard_t**));
        if (NULL == group->g_value.vec) {
            return -1;
        }
        break;
    }

    group->g_type = group_type;
    return 0;
}


/* add the values from 'vec' to 'group' */
int
skpcGroupAddValues(
    skpc_group_t       *group,
    const sk_vector_t  *vec)
{
    size_t count;
    size_t i;
    uint32_t *num;

    assert(group);
    if (group->g_is_frozen) {
        return -1;
    }

    if (NULL == vec) {
        return 0;
    }

    count = skVectorGetCount(vec);
    if (0 == count) {
        return 0;
    }

    switch (group->g_type) {
      case SKPC_GROUP_UNSET:
        return -1;

      case SKPC_GROUP_INTERFACE:
        /* check that vector has data of the correct type (size) */
        if (skVectorGetElementSize(vec) != sizeof(uint32_t)) {
            return -1;
        }
        for (i = 0; i < count; ++i) {
            num = (uint32_t*)skVectorGetValuePointer(vec, i);
            skBitmapSetBit(group->g_value.map, *num);
        }
        break;

      case SKPC_GROUP_IPBLOCK:
        /* check that vector has data of the correct type (size) */
        if (skVectorGetElementSize(vec) != sizeof(skIPWildcard_t*)) {
            return -1;
        }
        /* add IPWildcards to the group */
        if (skVectorAppendVector(group->g_value.vec, vec)) {
            skAppPrintOutOfMemory(NULL);
            return -1;
        }
        /* store the IPWildcards in skpc_wildcards for memory cleanup */
        if (!skpc_wildcards) {
            skpc_wildcards = skVectorNew(sizeof(skIPWildcard_t**));
            if (!skpc_wildcards) {
                skAppPrintOutOfMemory(NULL);
                return -1;
            }
        }
        if (skVectorAppendVector(skpc_wildcards, vec)) {
            skAppPrintOutOfMemory(NULL);
            return -1;
        }
        break;
    }

    return 0;
}


/* add the values from the existing group 'g' to group 'group' */
int
skpcGroupAddGroup(
    skpc_group_t       *group,
    const skpc_group_t *g)
{
    assert(group);
    if (group->g_is_frozen) {
        return -1;
    }

    /* verify group 'g' */
    if (NULL == g) {
        return 0;
    }
    if (!g->g_is_frozen) {
        return -1;
    }
    if (0 == g->g_itemcount) {
        return 0;
    }

    if (g->g_type != group->g_type) {
        return -1;
    }

    switch (group->g_type) {
      case SKPC_GROUP_UNSET:
        return -1;

      case SKPC_GROUP_INTERFACE:
        skBitmapUnion(group->g_value.map, g->g_value.map);
        break;

      case SKPC_GROUP_IPBLOCK:
        /* count number of IPWildcards */
        if (skVectorAppendFromArray(group->g_value.vec, g->g_value.ipblock,
                                    g->g_itemcount))
        {
            return -1;
        }
        break;
    }

    return 0;
}


/* is group frozen? */
int
skpcGroupIsFrozen(
    const skpc_group_t *group)
{
    assert(group);
    return group->g_is_frozen;
}


/* find the group named 'group_name' */
skpc_group_t *
skpcGroupLookupByName(
    const char         *group_name)
{
    skpc_group_t **group;
    size_t i;

    assert(skpc_groups);

    /* check input */
    if (group_name == NULL) {
        return NULL;
    }

    /* loop over all groups until we find one with given name */
    for (i = 0;
         (group = (skpc_group_t**)skVectorGetValuePointer(skpc_groups, i))
             != NULL;
         ++i)
    {
        if ((NULL != (*group)->g_name)
            && (0 == strcmp(group_name, (*group)->g_name)))
        {
            if (skpcGroupFreeze(*group)) {
                return NULL;
            }
            return *group;
        }
    }

    return NULL;
}


/*
 *  status = skpcGroupComputeComplement(group);
 *
 *    If the type of 'group' is SKPC_GROUP_INTERFACE, modify the group
 *    contain the complement of its current interface list.  Return 0
 *    on success.
 *
 *    Return -1 if 'group' is frozen or has a type other than
 *    SKPC_GROUP_INTERFACE.
 */
static int
skpcGroupComputeComplement(
    skpc_group_t       *group)
{
    assert(group);
    if (group->g_is_frozen) {
        return -1;
    }

    /* can only complement interfaces */
    if (SKPC_GROUP_INTERFACE != group->g_type) {
        return -1;
    }

    skBitmapComplement(group->g_value.map);
    return 0;
}


/*
 *  found = skpcGroupCheckInterface(group, interface);
 *
 *    Return 1 if 'group' contains the value 'interface'; 0 otherwise.
 */
static int
skpcGroupCheckInterface(
    const skpc_group_t *group,
    uint32_t            interface)
{
    assert(group->g_type == SKPC_GROUP_INTERFACE);
    return skBitmapGetBit(group->g_value.map, interface);
}


/*
 *  found = skpcGroupCheckIp(group, ip);
 *
 *    Return 1 if 'group' contains the IP Address 'ip'; 0 otherwise.
 */
static int
skpcGroupCheckIp(
    const skpc_group_t *group,
    const skipaddr_t   *ip)
{
    size_t i;

    assert(group->g_type == SKPC_GROUP_IPBLOCK);
    for (i = 0; i < group->g_itemcount; ++i) {
        if (skIPWildcardCheckIp(group->g_value.ipblock[i], ip)) {
            return 1;
        }
    }
    return 0;
}


/*
 *  count = skpcGroupGetItemCount(group);
 *
 *    Return the count of the number of items in 'group'.  Returns 0
 *    if 'group' is not frozen.
 */
static uint32_t
skpcGroupGetItemCount(
    const skpc_group_t *group)
{
    assert(group);
    return group->g_itemcount;
}


/*
 *  *****  Probes Types  *****************************************************
 */

/* return an enum value given a probe type name */
skpc_probetype_t
skpcProbetypeNameToEnum(
    const char         *name)
{
    struct probe_type_name_map_st *entry;

    if (name) {
        for (entry = probe_type_name_map; entry->name; ++entry) {
            if (0 == strcmp(name, entry->name)) {
                return entry->value;
            }
        }
    }
    return PROBE_ENUM_INVALID;
}


/* return the name given a probe type number */
const char *
skpcProbetypeEnumtoName(
    skpc_probetype_t    type)
{
    struct probe_type_name_map_st *entry;

    for (entry = probe_type_name_map; entry->name; ++entry) {
        if (type == entry->value) {
            return entry->name;
        }
    }
    return NULL;
}


/*
 *  *****  Probes Protocols  *************************************************
 */

/* return an protocol enum value given a probe protocol name */
skpc_proto_t
skpcProtocolNameToEnum(
    const char         *name)
{
    struct skpc_protocol_name_map_st *entry;
    uint32_t num;

    if (NULL != name) {
        for (entry = skpc_protocol_name_map; entry->name; ++entry) {
            if (0 == strcmp(name, entry->name)) {
                return entry->value;
            }
        }
        if (isdigit((int)*name)) {
            /* attempt to parse as a number */
            if (0 == skStringParseUint32(&num, name, 0, 255)) {
                for (entry = skpc_protocol_name_map; entry->name; ++entry) {
                    if (num == entry->num) {
                        return entry->value;
                    }
                }
            }
        }
    }

    return SKPC_PROTO_UNSET;
}


/* return a name given a probe protocol enum */
const char *
skpcProtocolEnumToName(
    skpc_proto_t        protocol)
{
    struct skpc_protocol_name_map_st *entry;

    for (entry = skpc_protocol_name_map; entry->name; ++entry) {
        if (protocol == entry->value) {
            return entry->name;
        }
    }
    return NULL;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
