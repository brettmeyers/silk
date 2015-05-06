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

RCSIDENT("$SiLK: packlogic-generic.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwflowpack.h>
#include <silk/rwrec.h>
#include <silk/skheader.h>
#include <silk/sksite.h>
#include <silk/skvector.h>
#include <silk/utils.h>


/* TYPEDEFS AND MACROS */

/*
 *    Define integers to stand-in for each of the possible flowtypes
 *    that are defined in the silk.conf file.  These must match.
 */
#define RW_IN       0
#define RW_OUT      1
#define RW_IN_WEB   2
#define RW_OUT_WEB  3
#define RW_IN_NULL  4
#define RW_OUT_NULL 5


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
    /* outnull */  {FT_RWSPLIT, FT_RWAUGMENTED}
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
    skpc_probe_t *probe;
    sk_vector_t *probe_vec;
    uint32_t count;

    /* There is a single class, so no per-class verification is
     * necessary.  Make certain we have either snmp interface values
     * or ip-blocks depending on the type of probe(s) associated with
     * this sensor. */

    /* get the probes for the sensor */
    probe_vec = skVectorNew(sizeof(skpc_probe_t*));
    if (probe_vec == NULL) {
        return -1;
    }
    count = skpcSensorGetProbes(sensor, probe_vec);

    /* this packing logic only supports a single probe per sensor */
    if (count != 1) {
        skAppPrintErr(("Cannot verify sensor '%s':\n"
                       "\tOnly one probe per sensor is supported"
                       " by the packing-logic\n\tfile '%s'"),
                      sensor->sensor_name,
                      plugin_path);
        return -1;
    }
    skVectorGetValue(&probe, probe_vec, 0);
    skVectorDestroy(probe_vec);

    /* make certain the probe's type is valid */
    switch (probe->probe_type) {
      case PROBE_ENUM_NETFLOW_V5:
      case PROBE_ENUM_NETFLOW_V9:
      case PROBE_ENUM_IPFIX:
        /* expected probe types */
        break;

      default:
        assert(skpcProbetypeEnumtoName(probe->probe_type));
        skAppPrintErr(("Cannot verify sensor '%s':\n"
                       "\tThe probe type '%s' is not supported in the"
                       " packing-logic\n\tfile '%s'"),
                      sensor->sensor_name,
                      skpcProbetypeEnumtoName(probe->probe_type),
                      plugin_path);
        return -1;
    }


    /* Verify that we have enough information to determine the
     * flowtype for every flow.  These are the rules:
     *
     * 1. One of external-interface, external-ipblock, or
     * external-ipset must be specified.
     *
     * 2. You cannot mix interfaces, ipblocks, and ipsets, with the
     * excption that a null-interface which is always allowed.
     *
     * 3. Only one network may claim the remainder.
     *
     * 4. Using remainder for an ipblock or ipset requires that
     * another interface has set an IPblock or an IPset.
     */
    switch (sensor->decider[NETWORK_EXTERNAL].nd_type) {
      case SKPC_UNSET:
        /* It is an error when neither SNMP interfaces nor IP-blocks
         * were specified for the external network. */
        skAppPrintErr(("Cannot verify sensor '%s':\n"
                       "\tMust specify %s-interface, %s-ipblock, or %s-ipset"),
                      sensor->sensor_name,
                      net_names[NETWORK_EXTERNAL],
                      net_names[NETWORK_EXTERNAL],
                      net_names[NETWORK_EXTERNAL]);
        return -1;

      case SKPC_NEG_IPBLOCK:
        skAppPrintErr("Negated IPblock logic not implemented");
        exit(EXIT_FAILURE);
      case SKPC_NEG_IPSET:
        skAppPrintErr("Negated IPset logic not implemented");
        exit(EXIT_FAILURE);

      case SKPC_IPBLOCK:
        /* Fine as long as INTERNAL is either empty or also contains
         * IPblocks */
        switch (sensor->decider[NETWORK_INTERNAL].nd_type) {
          case SKPC_UNSET:
          case SKPC_IPBLOCK:
          case SKPC_REMAIN_IPBLOCK:
            /* These are fine */
            break;

          case SKPC_NEG_IPBLOCK:
            skAppPrintErr("Negated IPblock logic not implemented");
            exit(EXIT_FAILURE);
          case SKPC_NEG_IPSET:
            skAppPrintErr("Negated IPset logic not implemented");
            exit(EXIT_FAILURE);

          case SKPC_INTERFACE:
          case SKPC_REMAIN_INTERFACE:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-ipblock and %s-interface"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;

          case SKPC_IPSET:
          case SKPC_REMAIN_IPSET:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-ipblock and %s-ipset"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;
        }
        break;

      case SKPC_REMAIN_IPBLOCK:
        switch (sensor->decider[NETWORK_INTERNAL].nd_type) {
          case SKPC_UNSET:
            /* Accept for now, though this will be an error if
             * NETWORK_NULL does not define an IPblock */
            break;

          case SKPC_NEG_IPBLOCK:
            skAppPrintErr("Negated IPblock logic not implemented");
            exit(EXIT_FAILURE);
          case SKPC_NEG_IPSET:
            skAppPrintErr("Negated IPset logic not implemented");
            exit(EXIT_FAILURE);

          case SKPC_REMAIN_IPBLOCK:
            /* Cannot have multiple things requesting "remainder" */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tOnly one network value may use 'remainder'"),
                          sensor->sensor_name);
            return -1;

          case SKPC_IPBLOCK:
            /* This is fine */
            break;

          case SKPC_INTERFACE:
          case SKPC_REMAIN_INTERFACE:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-ipblock and %s-interface"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;

          case SKPC_IPSET:
          case SKPC_REMAIN_IPSET:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-ipblock and %s-ipset"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;
        }
        break;

      case SKPC_INTERFACE:
      case SKPC_REMAIN_INTERFACE:
        /* Fine as long as INTERNAL and NULL are either empty or also
         * contain interfaces */
        switch (sensor->decider[NETWORK_INTERNAL].nd_type) {
          case SKPC_UNSET:
          case SKPC_INTERFACE:
          case SKPC_REMAIN_INTERFACE:
            switch (sensor->decider[NETWORK_NULL].nd_type) {
              case SKPC_IPBLOCK:
              case SKPC_NEG_IPBLOCK:
              case SKPC_REMAIN_IPBLOCK:
                /* Bad mix */
                skAppPrintErr(("Cannot verify sensor '%s':\n"
                               "\tCannot mix %s-interface and %s-ipblock"),
                              sensor->sensor_name,
                              net_names[NETWORK_EXTERNAL],
                              net_names[NETWORK_NULL]);
                return -1;
              case SKPC_IPSET:
              case SKPC_NEG_IPSET:
              case SKPC_REMAIN_IPSET:
                /* Bad mix */
                skAppPrintErr(("Cannot verify sensor '%s':\n"
                               "\tCannot mix %s-interface and %s-ipset"),
                              sensor->sensor_name,
                              net_names[NETWORK_EXTERNAL],
                              net_names[NETWORK_NULL]);
                return -1;
              default:
                break;
            }
            break;

          case SKPC_IPBLOCK:
          case SKPC_NEG_IPBLOCK:
          case SKPC_REMAIN_IPBLOCK:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-interface and %s-ipblock"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;

          case SKPC_IPSET:
          case SKPC_NEG_IPSET:
          case SKPC_REMAIN_IPSET:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-interface and %s-ipset"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;
        }
        break;

      case SKPC_IPSET:
        /* Fine as long as INTERNAL is either empty or also contains
         * IPsets */
        switch (sensor->decider[NETWORK_INTERNAL].nd_type) {
          case SKPC_UNSET:
          case SKPC_IPSET:
          case SKPC_REMAIN_IPSET:
            /* These are fine */
            break;

          case SKPC_NEG_IPSET:
            skAppPrintErr("Negated IPset logic not implemented");
            exit(EXIT_FAILURE);
          case SKPC_NEG_IPBLOCK:
            skAppPrintErr("Negated IPblock logic not implemented");
            exit(EXIT_FAILURE);

          case SKPC_INTERFACE:
          case SKPC_REMAIN_INTERFACE:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-ipset and %s-interface"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;

          case SKPC_IPBLOCK:
          case SKPC_REMAIN_IPBLOCK:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-ipset and %s-ipblock"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;
        }
        break;

      case SKPC_REMAIN_IPSET:
        switch (sensor->decider[NETWORK_INTERNAL].nd_type) {
          case SKPC_UNSET:
            /* Accept for now, though this will be an error if
             * NETWORK_NULL does not define an IPset */
            break;

          case SKPC_NEG_IPSET:
            skAppPrintErr("Negated IPset logic not implemented");
            exit(EXIT_FAILURE);
          case SKPC_NEG_IPBLOCK:
            skAppPrintErr("Negated IPblock logic not implemented");
            exit(EXIT_FAILURE);

          case SKPC_REMAIN_IPSET:
            /* Cannot have multiple things requesting "remainder" */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tOnly one network value may use 'remainder'"),
                          sensor->sensor_name);
            return -1;

          case SKPC_IPSET:
            /* This is fine */
            break;

          case SKPC_INTERFACE:
          case SKPC_REMAIN_INTERFACE:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-ipset and %s-interface"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;

          case SKPC_IPBLOCK:
          case SKPC_REMAIN_IPBLOCK:
            /* Bad mix */
            skAppPrintErr(("Cannot verify sensor '%s':\n"
                           "\tCannot mix %s-ipset and %s-ipblock"),
                          sensor->sensor_name,
                          net_names[NETWORK_EXTERNAL],
                          net_names[NETWORK_INTERNAL]);
            return -1;
        }
        break;
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

    /* index into output arrays and count to be returned */
    size_t sensor_count;

    assert(ftypes);
    assert(sensorids);

    memo = rwRecGetMemo(rwrec);

    /* loop over all sensors that use the 'probe' */
    for (sensor_count = 0; sensor_count < probe->sensor_count; ++sensor_count) {
        sensor = probe->sensor_list[sensor_count];
        sensorids[sensor_count] = sensor->sensor_id;

        if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                              NETWORK_EXTERNAL, SKPC_DIR_SRC))
        {
            /* Flow came from the outside */

            if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                  NETWORK_NULL, SKPC_DIR_DST))
            {
                /* Flow went to the null destination */
                ftypes[sensor_count] = RW_IN_NULL;
            } else {
                /* Assume flow went to the inside: incoming */
                if (rwRecIsWeb(rwrec)) {
                    ftypes[sensor_count] = RW_IN_WEB;
                } else {
                    ftypes[sensor_count] = RW_IN;
                }
            }
        } else {
            /* Flow came from the inside */

            if (1 == skpcSensorTestFlowInterfaces(sensor, rwrec,
                                                  NETWORK_NULL, SKPC_DIR_DST))
            {
                /* Flow went to the null destination */
                ftypes[sensor_count] = RW_OUT_NULL;
            } else {

                /* Assume flow went to the outside: outgoing */
                if (rwRecIsWeb(rwrec)) {
                    ftypes[sensor_count] = RW_OUT_WEB;
                } else {
                    ftypes[sensor_count] = RW_OUT;
                }
            }
        }

        if (skpcProbeGetQuirks(probe) & SKPC_QUIRK_FW_EVENT) {
            /* Check whether libskipfix stored a "flow denied"
             * firewallEvent, NF_F_FW_EVENT, or NF_F_FW_EXT_EVENT.  If so,
             * make certain flowtype is NULL */
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
                    /* arrived from the outside */
                    ftypes[sensor_count] = RW_IN_NULL;
                    break;
                  case RW_OUT:
                  case RW_OUT_WEB:
                    /* arrived from the inside */
                    ftypes[sensor_count] = RW_OUT_NULL;
                    break;
                  default:
                    skAbortBadCase(ftypes[sensor_count]);
                }
                break;
            }
        }
    }

    return probe->sensor_count;
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
