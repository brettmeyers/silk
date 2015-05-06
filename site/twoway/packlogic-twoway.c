/*
** Copyright (C) 2005-2015 by Carnegie Mellon University.
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
**  Support file for probeconf.c
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: packlogic-twoway.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwflowpack.h>
#include <silk/rwrec.h>
#include <silk/skheader.h>
#include <silk/sksite.h>
#include <silk/skvector.h>
#include <silk/utils.h>


/* TYPEDEFS AND MACROS */

/*
 *    Whether to split the web data separately from the other data;
 *    that is, whether to use the "inweb" and "outweb" flowtypes.
 *
 *    Web data is any flow seen on ports 80/tcp, 8080/tcp, 443/tcp.
 */
#ifndef SK_ENABLE_WEB_SPLIT
#  define SK_ENABLE_WEB_SPLIT   1
#endif

/*
 *    Whether to split ICMP data separately from the other data; that
 *    is, whether to use the "inicmp" and "outicmp" flowtypes.
 *
 *    ICMP data is any flow where proto == 1.
 */
#ifndef SK_ENABLE_ICMP_SPLIT
#  define SK_ENABLE_ICMP_SPLIT  0
#endif


/*
 *    Define integers to stand-in for each of the possible flowtypes
 *    that are defined in the silk.conf file.  These must match.
 */
#define RW_IN        0
#define RW_OUT       1
#define RW_IN_WEB    2
#define RW_OUT_WEB   3
#define RW_IN_NULL   4
#define RW_OUT_NULL  5
#define RW_INT2INT   6
#define RW_EXT2EXT   7
#define RW_IN_ICMP   8
#define RW_OUT_ICMP  9
#define RW_OTHER    10


/*
 *    These are the IDs of the networks that should be defined in the
 *    sensor.conf file.  We are in trouble if someone redefines these
 *    values.
 */
#define NUM_NETWORKS        3

#define NETWORK_NULL        ((skpc_network_id_t)0)
/* The SNMP interface on the probe to which non-routed traffic is
 * sent.  This is 0 on Cisco routers. */

#define NETWORK_EXTERNAL    ((skpc_network_id_t)1)
/* A bitmap where an ON bit represents an SNMP interface on the
 * probe that faces the external world.  Traffic entering the
 * router on this interface will be considered incoming. */

#define NETWORK_INTERNAL    ((skpc_network_id_t)2)
/* A bitmap where an ON bit represents an SNMP interface on the
 * probe that faces the internal world.  Traffic entering the
 * router on this interface will be considered outgoing. */


/* LOCAL VARIABLES */

/* the names that correspond to each network */
static const char *net_names[NUM_NETWORKS] = {
    "null",
    "external",
    "internal"
};

/*
 *    Define the file formats used to pack each flowtype.  If these do
 *    not line up with the type IDs defined in the config file, there
 *    will be problems.  Use the more compact formats for flows from
 *    NetFlow v5 based sources, and the expanded formats for flows
 *    from other sources.
 */
static struct filetypeFormats_st {
    fileFormat_t  netflow_v5;
    fileFormat_t  other;
} filetypeFormats[] = {
    /* in      */  {FT_RWSPLIT, FT_RWAUGMENTED},
    /* out     */  {FT_RWSPLIT, FT_RWAUGMENTED},
    /* inweb   */  {FT_RWWWW,   FT_RWAUGWEB},
    /* outweb  */  {FT_RWWWW,   FT_RWAUGWEB},
    /* innull  */  {FT_RWSPLIT, FT_RWAUGMENTED},
    /* outnull */  {FT_RWSPLIT, FT_RWAUGMENTED},
    /* int2int */  {FT_RWSPLIT, FT_RWAUGMENTED},
    /* ext2ext */  {FT_RWSPLIT, FT_RWAUGMENTED},
    /* inicmp  */  {FT_RWSPLIT, FT_RWAUGMENTED},
    /* outicmp */  {FT_RWSPLIT, FT_RWAUGMENTED},
    /* other   */  {FT_RWSPLIT, FT_RWAUGMENTED}
};


static const char plugin_source[] = __FILE__;
static const char *plugin_path = plugin_source;


/* LOCAL FUNCTION PROTOTYPES */

static int  packLogicSetup(void);
static void packLogicTeardown(void);
static int  packLogicVerifySensor(skpc_sensor_t *sensor);
static int
packLogicDetermineFlowtype(
    const skpc_probe_t *probe,
    const rwRec        *rwrec,
    flowtypeID_t       *ftypes,
    sensorID_t         *sensorids);
static fileFormat_t
packLogicDetermineFileFormat(
    const skpc_probe_t *probe,
    flowtypeID_t        ftype);


/* FUNCTION DEFINITIONS */

/*
 *    Fill in 'packlogic' with pointers to the functions defined in
 *    this file.
 */
int
packLogicInitialize(
    packlogic_plugin_t *packlogic)
{
    assert(packlogic);

    if (packlogic->path) {
        plugin_path = packlogic->path;
    }

    packlogic->setup_fn =                &packLogicSetup;
    packlogic->teardown_fn =             &packLogicTeardown;
    packlogic->verify_sensor_fn =        &packLogicVerifySensor;
    packlogic->determine_flowtype_fn =   &packLogicDetermineFlowtype;
    packlogic->determine_fileformat_fn = &packLogicDetermineFileFormat;
    return 0;
}


/*
 *    Verify contents of silk.conf file matches the values we set here
 *    and set any globals we require.
 *
 *    Invoked from rwflowpack by packlogic->setup_fn
 */
static int
packLogicSetup(
    void)
{
    const size_t count = (sizeof(filetypeFormats)/sizeof(filetypeFormats[0]));
    uint32_t i;

#define FT_ASSERT(flowtype_id, flowtype_name)                           \
    sksiteFlowtypeAssert(plugin_path, (flowtype_id), "all", (flowtype_name))

    /* Make sure flowtype definitions match config file */
    FT_ASSERT(RW_IN,       "in");
    FT_ASSERT(RW_OUT,      "out");
    FT_ASSERT(RW_IN_WEB,   "inweb");
    FT_ASSERT(RW_OUT_WEB,  "outweb");
    FT_ASSERT(RW_IN_NULL,  "innull");
    FT_ASSERT(RW_OUT_NULL, "outnull");
    FT_ASSERT(RW_INT2INT,  "int2int");
    FT_ASSERT(RW_EXT2EXT,  "ext2ext");
    FT_ASSERT(RW_IN_ICMP,  "inicmp");
    FT_ASSERT(RW_OUT_ICMP, "outicmp");
    FT_ASSERT(RW_OTHER,    "other");

    /* Confirm that number of flowtypes is not greater than the size
     * of the filetypeFormats[] array; abort if it is.  Complain if
     * the array is too large, but continue processing.  */
    if (count <= sksiteFlowtypeGetMaxID()) {
        skAppPrintErr(("File formats not specified for some flowtypes.\n"
                       "\tModify filetypeFormats[] in %s,\n"
                       "\trecompile and try running again."),
                      plugin_path);
        skAbort();
    } else if (count != (1u + sksiteFlowtypeGetMaxID())) {
        skAppPrintErr(("Warning: Number of flowtypes does not equal number\n"
                       "\tof file formats in filetypeFormats[] in %s"),
                      plugin_path);
    }

    /* Define all of our networks */
    for (i = 0; i < NUM_NETWORKS; ++i) {
        if (skpcNetworkAdd(i, net_names[i])) {
            skAppPrintErr("Unable to add network %" PRIu32 "->%s",
                          i, net_names[i]);
            return -1;
        }
    }

    return 0;
}


/*
 *    Clean up any memory we allocated.
 *
 *    Invoked from rwflowpack by packlogic->teardown_fn
 */
static void
packLogicTeardown(
    void)
{
    return;
}


/*
 *    Verify sensor by its class.  Verify that the sensor supports the
 *    type(s) of its probe(s).  Verify that enough information is
 *    present on the sensor to categorize a flow record.
 *
 *    Invoked from rwflowpack by packlogic->verify_sensor_fn
 */
static int
packLogicVerifySensor(
    skpc_sensor_t      *sensor)
{
    unsigned int nd_type_count[SKPC_NUM_NETDECIDER_TYPES];
    unsigned int block_count;
    unsigned int set_count;
    unsigned int if_count;
    unsigned int i;
    skpc_probe_t *probe;
    sk_vector_t *probe_vec;
    uint32_t probe_count;
    uint32_t c;

    /* There is a single class, so no per-class verification is
     * necessary.  Make certain each sensor has snmp interface values,
     * ipblocks, or IPsets to categorize each flow. */

    /* get the probes for the sensor */
    probe_vec = skVectorNew(sizeof(skpc_probe_t*));
    if (probe_vec == NULL) {
        return -1;
    }
    probe_count = skpcSensorGetProbes(sensor, probe_vec);

    /* verify each probe */
    for (c = 0; c < probe_count; ++c) {
        skVectorGetValue(&probe, probe_vec, c);

        /* make certain the probe's type is valid */
        switch (probe->probe_type) {
          case PROBE_ENUM_NETFLOW_V5:
          case PROBE_ENUM_NETFLOW_V9:
          case PROBE_ENUM_IPFIX:
          case PROBE_ENUM_SFLOW:
          case PROBE_ENUM_SILK:
            /* supported probe types */
            break;

          default:
            assert(skpcProbetypeEnumtoName(probe->probe_type));
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tThe probe type '%s' is not supported in the"
                           " packing-logic\n\tfile '%s'"),
                          sensor->sensor_name,
                          skpcProbetypeEnumtoName(probe->probe_type),
                          plugin_path);
            skVectorDestroy(probe_vec);
            return -1;
        }
    }
    skVectorDestroy(probe_vec);

    /* If the source and destination networks are set, we're
     * good to go.  Ideally, there should be a way to say that all
     * traffic is from a particular network, but that we want to
     * categorize it by where it goes.  For example, consider a router
     * where we only monitor incoming traffic, but we still want to
     * distinguish ACLed traffic from routed. */
    if ((sensor->fixed_network[0] != SKPC_NETWORK_ID_INVALID)
        && (sensor->fixed_network[1] != SKPC_NETWORK_ID_INVALID))
    {
        return 0;
    }

    /*
     * Verify that we have enough information to determine the
     * flowtype for every flow.  These are the rules:
     *
     * 1. One of NET-interface, NET-ipblock, or NET-ipset must be
     * specified, where NET is either "internal" or "external".
     *
     * 2. A null-interface is always allowed.  Otherwise, each sensor
     * must only use one of ipblocks, ipsets, and interfaces.
     *
     * 3. Only one network may claim the remainder.
     *
     * 4. Using 'remainder' for an ipblock or an ipset requires that
     * another NET has set an IPblock or an IPset.  (Not required for
     * interfaces.)
     *
     * 5. If only one of internal-* or external-* is set, set the
     * other to the remaining values, unless the 'remainder' is
     * already claiming them.
     */

    /* for this sensor, count how many of each decider type (e.g.,
     * SKPC_IBLOCK (NET-ipblock)) have been specified */
    memset(nd_type_count, 0, sizeof(nd_type_count));
    for (i = 0; i < NUM_NETWORKS; ++i) {
        assert(sensor->decider[i].nd_type < SKPC_NUM_NETDECIDER_TYPES);
        ++nd_type_count[sensor->decider[i].nd_type];
    }

    /* get number of deciders for ipblocks and for interfaces */
    if_count = (nd_type_count[SKPC_INTERFACE]
                + nd_type_count[SKPC_REMAIN_INTERFACE]);
    block_count = (nd_type_count[SKPC_IPBLOCK]
                   + nd_type_count[SKPC_REMAIN_IPBLOCK]);
    set_count = (nd_type_count[SKPC_IPSET]
                 + nd_type_count[SKPC_REMAIN_IPSET]);

    if (nd_type_count[SKPC_NEG_IPBLOCK]) {
        /* this should never happen, since there is no way to set this
         * from the sensor.conf file  */
        skAppPrintErr("Negated IPblock logic not implemented");
        exit(EXIT_FAILURE);
    }
    if (nd_type_count[SKPC_NEG_IPSET]) {
        /* this should never happen, since there is no way to set this
         * from the sensor.conf file  */
        skAppPrintErr("Negated IPset logic not implemented");
        exit(EXIT_FAILURE);
    }

    /* make certain ipblocks, ipsets, or interfaces are specified, and
     * make certain something in addition to null-* is specified */
    if ((block_count + if_count + set_count == 0)
        || ((block_count + if_count + set_count == 1)
            && (sensor->decider[NETWORK_NULL].nd_type != SKPC_UNSET)))
    {
        skAppPrintErr(("Cannot verify sensor %s:\n"
                       "\tMust specify source-network and"
                       " destination-network, or at least one\n"
                       "\tof %s- and %s-interface, %s- and %s-ipblock,"
                       " or %s- and %s-ipset"),
                      sensor->sensor_name,
                      net_names[NETWORK_EXTERNAL],net_names[NETWORK_INTERNAL],
                      net_names[NETWORK_EXTERNAL],net_names[NETWORK_INTERNAL],
                      net_names[NETWORK_EXTERNAL],net_names[NETWORK_INTERNAL]);
        return -1;
    }

    /* only one 'remainder' is allowed */
    if ((nd_type_count[SKPC_REMAIN_IPBLOCK]
         + nd_type_count[SKPC_REMAIN_INTERFACE]
         + nd_type_count[SKPC_REMAIN_IPSET])
        > 1)
    {
        skAppPrintErr(("Cannot verify sensor '%s':\n"
                       "\tOnly one network value may use 'remainder'"),
                      sensor->sensor_name);
        return -1;
    }

    /* handle case where NET-ipblocks are set */
    if (block_count) {
        if (block_count == NUM_NETWORKS) {
            /* all networks were specified. nothing else to check */
            assert(if_count == 0);
            assert(set_count == 0);
            return 0;
        }
        /* block_count is either 1 or 2 */
        assert(block_count <= 2);

        if (set_count) {
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix <NET>-ipblock and <NET>-ipset"),
                          sensor->sensor_name);
            return -1;
        }

        /* only valid mix of NET-ipblock and NET-interface is for the
         * interfaces to be on the NULL network */
        if (if_count) {
            switch (sensor->decider[NETWORK_NULL].nd_type) {
              case SKPC_INTERFACE:
              case SKPC_REMAIN_INTERFACE:
                --if_count;
                break;

              default:
                break;
            }
            if (if_count) {
                skAppPrintErr(("Cannot verify sensor '%s':\n"
                               "\tCannot mix <NET>-interface"
                               " and <NET>-ipblock"),
                              sensor->sensor_name);
                return -1;
            }
        }

        /* if an ipblock has claimed the 'remainder', verify we have
         * IPs specified elsewhere and return */
        if (nd_type_count[SKPC_REMAIN_IPBLOCK] == 1) {
            /* need at least one IP address to be specified for
             * 'remainder' to work */
            if (block_count == 1) {
                skAppPrintErr(("Cannot verify sensor '%s':\n"
                               "\tCannot set ipblocks to remainder when"
                               " no other networks hold IP blocks"),
                              sensor->sensor_name);
                return -1;
            }
            return 0;
        }

        /* if either EXTERNAL or INTERNAL is unset, set to remainder */
        if (sensor->decider[NETWORK_EXTERNAL].nd_type == SKPC_UNSET) {
            assert(sensor->decider[NETWORK_INTERNAL].nd_type == SKPC_IPBLOCK);
            sensor->decider[NETWORK_EXTERNAL].nd_type = SKPC_REMAIN_IPBLOCK;
        }
        if (sensor->decider[NETWORK_INTERNAL].nd_type == SKPC_UNSET) {
            assert(sensor->decider[NETWORK_EXTERNAL].nd_type == SKPC_IPBLOCK);
            sensor->decider[NETWORK_INTERNAL].nd_type = SKPC_REMAIN_IPBLOCK;
        }

        return 0;
    }

    /* handle case where NET-ipsets are set */
    if (set_count) {
        if (set_count == NUM_NETWORKS) {
            /* all networks were specified. nothing else to check */
            assert(if_count == 0);
            assert(block_count == 0);
            return 0;
        }
        /* set_count is either 1 or 2 */
        assert(set_count <= 2);

        if (block_count) {
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix <NET>-ipset and <NET>-ipblock"),
                          sensor->sensor_name);
            return -1;
        }

        /* only valid mix of NET-ipset and NET-interface is for the
         * interfaces to be on the NULL network */
        if (if_count) {
            switch (sensor->decider[NETWORK_NULL].nd_type) {
              case SKPC_INTERFACE:
              case SKPC_REMAIN_INTERFACE:
                --if_count;
                break;

              default:
                break;
            }
            if (if_count) {
                skAppPrintErr(("Cannot verify sensor '%s':\n"
                               "\tCannot mix <NET>-interface"
                               " and <NET>-ipset"),
                              sensor->sensor_name);
                return -1;
            }
        }

        /* if an ipset has claimed the 'remainder', verify we have
         * IPs specified elsewhere and return */
        if (nd_type_count[SKPC_REMAIN_IPSET] == 1) {
            /* need at least one IP address to be specified for
             * 'remainder' to work */
            if (set_count == 1) {
                skAppPrintErr(("Cannot verify sensor '%s':\n"
                               "\tCannot set ipsets to remainder when"
                               " no other networks hold IP sets"),
                              sensor->sensor_name);
                return -1;
            }
            return 0;
        }

        /* if either EXTERNAL or INTERNAL is unset, set to remainder */
        if (sensor->decider[NETWORK_EXTERNAL].nd_type == SKPC_UNSET) {
            assert(sensor->decider[NETWORK_INTERNAL].nd_type == SKPC_IPSET);
            sensor->decider[NETWORK_EXTERNAL].nd_type = SKPC_REMAIN_IPSET;
        }
        if (sensor->decider[NETWORK_INTERNAL].nd_type == SKPC_UNSET) {
            assert(sensor->decider[NETWORK_EXTERNAL].nd_type == SKPC_IPSET);
            sensor->decider[NETWORK_INTERNAL].nd_type = SKPC_REMAIN_IPSET;
        }

        return 0;
    }

    /* handle case where NET-interfaces are set */
    if (0 == if_count || block_count > 0 || set_count > 0) {
        skAppPrintErr("Programmer error");
        skAbort();
    }

    if (if_count == NUM_NETWORKS) {
        /* all networks were specified. nothing else to check */
        return 0;
    }
    /* if_count is either 1 or 2. */
    assert(if_count <= 2);

    /* if someone has claimed the 'remainder', there is nothing else
     * to do. */
    if (nd_type_count[SKPC_REMAIN_INTERFACE] == 1) {
        /* unlike the ipblock case, 'remainder' by itself is legal */
        return 0;
    }

    /* if either EXTERNAL or INTERNAL is unset, set to remainder */
    if (sensor->decider[NETWORK_EXTERNAL].nd_type == SKPC_UNSET) {
        assert(sensor->decider[NETWORK_INTERNAL].nd_type==SKPC_INTERFACE);
        sensor->decider[NETWORK_EXTERNAL].nd_type = SKPC_REMAIN_INTERFACE;
    }
    if (sensor->decider[NETWORK_INTERNAL].nd_type == SKPC_UNSET) {
        assert(sensor->decider[NETWORK_EXTERNAL].nd_type==SKPC_INTERFACE);
        sensor->decider[NETWORK_INTERNAL].nd_type = SKPC_REMAIN_INTERFACE;
    }

    return 0;
}


/*
 *  count = packLogicDetermineFlowtype(probe, &rwrec, ftypes[], sensorids[]);
 *
 *    Fill the 'ftypes' and 'sensorids' arrays with the list of
 *    flow_types and sensors to which the 'rwrec' probe, collected
 *    from the 'probe' sensor, should be packed.  Return the number of
 *    elements added to each array or -1 on error.
 *
 *    Invoked from rwflowpack by packlogic->determine_flowtype_fn
 */
static int
packLogicDetermineFlowtype(
    const skpc_probe_t *probe,
    const rwRec        *rwrec,
    flowtypeID_t       *ftypes,
    sensorID_t         *sensorids)
{
    skpc_sensor_t *sensor;
    uint16_t memo;
    size_t i;

    /* index into output arrays and count to be returned */
    int sensor_count = 0;

    assert(ftypes);
    assert(sensorids);

    memo = rwRecGetMemo(rwrec);

    /* loop over all sensors that use the 'probe' */
    for (i = 0; i < probe->sensor_count; ++i) {
        sensor = probe->sensor_list[i];

        /* check whether to discard the flow */
        if (sensor->filter_count && skpcSensorCheckFilters(sensor, rwrec)) {
            continue;
        }

        sensorids[sensor_count] = sensor->sensor_id;

        if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                              NETWORK_EXTERNAL, SKPC_DIR_SRC))
        {
            /* Flow reached the monitoring point from the outside, and ... */
            if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                  NETWORK_NULL, SKPC_DIR_DST))
            {
                /* ... Flow went to the null destination */
                ftypes[sensor_count] = RW_IN_NULL;
            } else if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                         NETWORK_INTERNAL,
                                                         SKPC_DIR_DST))
            {
                /* ... Flow entered the monitored network: incoming */
#if     SK_ENABLE_ICMP_SPLIT
                if (rwRecIsICMP(rwrec)) {
                    ftypes[sensor_count] = RW_IN_ICMP;
                } else
#endif
#if     SK_ENABLE_WEB_SPLIT
                if (rwRecIsWeb(rwrec)) {
                    ftypes[sensor_count] = RW_IN_WEB;
                } else
#endif
                {
                    ftypes[sensor_count] = RW_IN;
                }
            } else if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                         NETWORK_EXTERNAL,
                                                         SKPC_DIR_DST))
            {
                /* ... Flow went back out the way it came in */
                ftypes[sensor_count] = RW_EXT2EXT;
            } else {
                /* ... Flow left the monitor through an unknown interface */
                ftypes[sensor_count] = RW_OTHER;
            }
        } else if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                     NETWORK_INTERNAL,
                                                     SKPC_DIR_SRC))
        {
            /* Flow reached the monitoring point from the inside of
             * network, and ... */
            if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                  NETWORK_NULL, SKPC_DIR_DST))
            {
                /* ... Flow went to the null destination */
                ftypes[sensor_count] = RW_OUT_NULL;
            } else if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                         NETWORK_EXTERNAL,
                                                         SKPC_DIR_DST))
            {
                /* ... Flow left the monitored network: outgoing */
#if     SK_ENABLE_ICMP_SPLIT
                if (rwRecIsICMP(rwrec)) {
                    ftypes[sensor_count] = RW_OUT_ICMP;
                } else
#endif
#if     SK_ENABLE_WEB_SPLIT
                if (rwRecIsWeb(rwrec)) {
                    ftypes[sensor_count] = RW_OUT_WEB;
                } else
#endif
                {
                    ftypes[sensor_count] = RW_OUT;
                }
            } else if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                         NETWORK_INTERNAL,
                                                         SKPC_DIR_DST))
            {
                /* ... Flow went back into the monitored network */
                ftypes[sensor_count] = RW_INT2INT;
            } else {
                /* ... Flow went to an unknown interface */
                ftypes[sensor_count] = RW_OTHER;
            }
        } else {
            /* Flow originated from an unknown interface */
            ftypes[sensor_count] = RW_OTHER;
        }

        if (skpcProbeGetQuirks(probe) & SKPC_QUIRK_FW_EVENT) {
            /* Check whether libskipfix stored a "flow denied"
             * firewallEvent, NF_F_FW_EVENT, or NF_F_FW_EXT_EVENT.  If so,
             * make certain flowtype is NULL; however, if flowtype is
             * RW_OTHER leave it as is. */
            switch (memo) {
              case 0:
                break;
              case SKIPFIX_FW_EVENT_DENIED_INGRESS:
                ftypes[sensor_count] = RW_IN_NULL;
                break;
              case SKIPFIX_FW_EVENT_DENIED_EGRESS:
                ftypes[sensor_count] = RW_OUT_NULL;
                break;
              case SKIPFIX_FW_EVENT_DENIED:
              case SKIPFIX_FW_EVENT_DENIED_SERV_PORT:
              case SKIPFIX_FW_EVENT_DENIED_NOT_SYN:
                switch (ftypes[sensor_count]) {
                  case RW_IN_NULL:
                  case RW_OUT_NULL:
                    /* type is already null */
                    break;
                  case RW_IN:
                  case RW_IN_WEB:
                  case RW_IN_ICMP:
                  case RW_EXT2EXT:
                    /* arrived from the outside */
                    ftypes[sensor_count] = RW_IN_NULL;
                    break;
                  case RW_OUT:
                  case RW_OUT_WEB:
                  case RW_OUT_ICMP:
                  case RW_INT2INT:
                    /* arrived from the inside */
                    ftypes[sensor_count] = RW_OUT_NULL;
                    break;
                  case RW_OTHER:
                    /* hopefully the type is already "unusual" enough that
                     * there is no need to categorize it as denied. */
                    break;
                  default:
                    skAbortBadCase(ftypes[sensor_count]);
                }
                break;
              default:
                break;
            }
        }
        ++sensor_count;

    } /* for (sensors-per-probe) */

    return sensor_count;
}


/*
 *    Determine the file output format to use.
 *
 *    Invoked from rwflowpack by packlogic->determine_fileformat_fn
 */
#if  SK_ENABLE_IPV6

static fileFormat_t
packLogicDetermineFileFormat(
    const skpc_probe_t  UNUSED(*probe),
    flowtypeID_t         UNUSED(ftype))
{
    return FT_RWIPV6;
}

#else

static fileFormat_t
packLogicDetermineFileFormat(
    const skpc_probe_t *probe,
    flowtypeID_t        ftype)
{
    assert(ftype < (sizeof(filetypeFormats)/sizeof(filetypeFormats[0])));

    if (skpcProbeGetQuirks(probe) & SKPC_QUIRK_ZERO_PACKETS) {
        /* Use a format that does not use bytes/packet ratio */
        return FT_RWGENERIC;
    }

    switch (probe->probe_type) {
      case PROBE_ENUM_NETFLOW_V5:
        return filetypeFormats[ftype].netflow_v5;

      default:
        return filetypeFormats[ftype].other;
    }
}

#endif  /* #else of #if SK_ENABLE_IPV6 */


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
