/*
** Copyright (C) 2001-2014 by Carnegie Mellon University.
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
**  rwipv6routingio.c
**
**    Routines to pack/unpack FT_RWIPV6ROUTING records.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwipv6routingio.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "skstream_priv.h"


/* Version to use when SK_RECORD_VERSION_ANY is specified */
#define DEFAULT_RECORD_VERSION 1


/* LOCAL FUNCTION PROTOTYPES */

static int
ipv6routingioRecordUnpack_V1(
    skstream_t         *rwIOS,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar);


static const uint8_t IP4in6_prefix[12] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};


/* ********************************************************************* */

/*
**  RWIPV6ROUTING VERSION 2
**
**    FT_RWIPV6ROUTING version 2 is identical to V1, expect must clear
**    the application field when unpacking.  Packing functions for V1
**    and V2 are identical.
*/

static int
ipv6routingioRecordUnpack_V2(
    skstream_t         *rwIOS,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    int rv;

    rv = ipv6routingioRecordUnpack_V1(rwIOS, rwrec, ar);
    rwRecSetApplication(rwrec, 0);
    return rv;
}


/* ********************************************************************* */

/*
**  RWIPV6ROUTING VERSION 1
**
**    int64_t       sTime;       //  0- 7  Flow start time as milliseconds
**                               //        since UNIX epoch
**
**    uint32_t      elapsed;     //  8-11  Duration of flow in milliseconds
**                               //        (Allows for a 49 day flow)
**
**    uint16_t      sPort;       // 12-13  Source port
**    uint16_t      dPort;       // 14-15  Destination port
**
**    uint8_t       proto;       // 16     IP protocol
**    flowtypeID_t  flow_type;   // 17     Class & Type info
**    sensorID_t    sID;         // 18-19  Sensor ID
**
**    uint8_t       flags;       // 20     OR of all flags (Netflow flags)
**    uint8_t       init_flags;  // 21     TCP flags in first packet
**                               //        or blank for "legacy" data
**    uint8_t       rest_flags;  // 22     TCP flags on non-initial packet
**                               //        or blank for "legacy" data
**    uint8_t       tcp_state;   // 23     TCP state machine info (below)
**
**    uint16_t      application; // 24-25  Indication of type of traffic
**    uint16_t      memo;        // 26-27  Application specific field
**
**    uint16_t      input;       // 28-29  Router incoming SNMP interface
**    uint16_t      output;      // 30-31  Router outgoing SNMP interface
**
**    uint32_t      pkts;        // 32-35  Count of packets
**    uint32_t      bytes;       // 36-39  Count of bytes
**
**    uint8_t[16]   sIP;         // 40-55  Source IP
**    uint8_t[16]   dIP;         // 56-71  Destination IP
**    uint8_t[16]   nhIP;        // 72-87  Router Next Hop IP
**
**
**  88 bytes on disk.
*/

#define RECLEN_RWIPV6ROUTING_V1 88


/*
 *    Byte swap the RWIPV6ROUTING v1 record 'ar' in place.
 */
#define ipv6routingioRecordSwap_V1(ar)                                  \
    {                                                                   \
        SWAP_DATA64((ar) +  0);   /* sTime */                           \
        SWAP_DATA32((ar) +  8);   /* elapsed */                         \
        SWAP_DATA16((ar) + 12);   /* sPort */                           \
        SWAP_DATA16((ar) + 14);   /* dPort */                           \
        /* Two single bytes: (16)proto, (17)flow_type */                \
        SWAP_DATA16((ar) + 18);   /* sID */                             \
        /* Four single bytes: (20)flags, (21)init_flags,                \
         *                    (22)rest_flags, (23)tcp_state */          \
        SWAP_DATA16((ar) + 24);   /* application */                     \
        SWAP_DATA16((ar) + 26);   /* memo */                            \
        SWAP_DATA16((ar) + 28);   /* input */                           \
        SWAP_DATA16((ar) + 30);   /* output */                          \
        SWAP_DATA32((ar) + 32);   /* pkts */                            \
        SWAP_DATA32((ar) + 36);   /* bytes */                           \
        /* 48 bytes of sIP, dIP, nhIP always in network byte order */   \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
ipv6routingioRecordUnpack_V1(
    skstream_t         *rwIOS,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    uint32_t ip;

    /* swap if required */
    if (rwIOS->swapFlag) {
        ipv6routingioRecordSwap_V1(ar);
    }

    rwRecMemSetStartTime(rwrec, &ar[0]);
    rwRecMemSetElapsed(rwrec, &ar[8]);
    rwRecMemSetSPort(rwrec, &ar[12]);
    rwRecMemSetDPort(rwrec, &ar[14]);
    rwRecMemSetProto(rwrec, &ar[16]);
    rwRecMemSetFlowType(rwrec, &ar[17]);
    rwRecMemSetSensor(rwrec, &ar[18]);
    rwRecMemSetFlags(rwrec, &ar[20]);
    rwRecMemSetInitFlags(rwrec, &ar[21]);
    rwRecMemSetRestFlags(rwrec, &ar[22]);
    rwRecMemSetTcpState(rwrec, &ar[23]);
    rwRecMemSetApplication(rwrec, &ar[24]);
    rwRecMemSetMemo(rwrec, &ar[26]);
    rwRecMemSetInput(rwrec, &ar[28]);
    rwRecMemSetOutput(rwrec, &ar[30]);
    rwRecMemSetPkts(rwrec, &ar[32]);
    rwRecMemSetBytes(rwrec, &ar[36]);

    if (ar[23] & 0x80) {
        /* Record is IPv6 */
#if !SK_ENABLE_IPV6
        return SKSTREAM_ERR_UNSUPPORT_IPV6;
#else
        rwRecSetIPv6(rwrec);
        rwRecMemSetSIPv6(rwrec, &ar[40]);
        rwRecMemSetDIPv6(rwrec, &ar[56]);
        rwRecMemSetNhIPv6(rwrec, &ar[72]);
#endif /* SK_ENABLE_IPV6 */
    } else {
        /* Record is IPv4 */

        /* sIP */
        memcpy(&ip, &ar[52], sizeof(ip));
        rwRecSetSIPv4(rwrec, ntohl(ip));

        /* dIP */
        memcpy(&ip, &ar[68], sizeof(ip));
        rwRecSetDIPv4(rwrec, ntohl(ip));

        /* nhIP */
        memcpy(&ip, &ar[84], sizeof(ip));
        rwRecSetNhIPv4(rwrec, ntohl(ip));
    }

    RWREC_MAYBE_CLEAR_TCPSTATE_EXPANDED(rwrec);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
ipv6routingioRecordPack_V1(
    skstream_t             *rwIOS,
    const rwGenericRec_V5  *rwrec,
    uint8_t                *ar)
{
    uint32_t ip;

    rwRecMemGetStartTime(rwrec, &ar[0]);
    rwRecMemGetElapsed(rwrec, &ar[8]);
    rwRecMemGetSPort(rwrec, &ar[12]);
    rwRecMemGetDPort(rwrec, &ar[14]);
    rwRecMemGetProto(rwrec, &ar[16]);
    rwRecMemGetFlowType(rwrec, &ar[17]);
    rwRecMemGetSensor(rwrec, &ar[18]);
    rwRecMemGetFlags(rwrec, &ar[20]);
    rwRecMemGetInitFlags(rwrec, &ar[21]);
    rwRecMemGetRestFlags(rwrec, &ar[22]);
    rwRecMemGetTcpState(rwrec, &ar[23]);
    rwRecMemGetApplication(rwrec, &ar[24]);
    rwRecMemGetMemo(rwrec, &ar[26]);
    rwRecMemGetInput(rwrec, &ar[28]);
    rwRecMemGetOutput(rwrec, &ar[30]);
    rwRecMemGetPkts(rwrec, &ar[32]);
    rwRecMemGetBytes(rwrec, &ar[36]);

    if (rwRecIsIPv6(rwrec)) {
        /* Record is IPv6 */
#if !SK_ENABLE_IPV6
        return SKSTREAM_ERR_UNSUPPORT_IPV6;
#else
        ar[23] |= 0x80;
        rwRecMemGetSIPv6(rwrec, &ar[40]);
        rwRecMemGetDIPv6(rwrec, &ar[56]);
        rwRecMemGetNhIPv6(rwrec, &ar[72]);
#endif /* SK_ENABLE_IPV6 */
    } else {
        /* Record is IPv4, but encode as IPv6 */

        /* sIP */
        ip = htonl(rwRecGetSIPv4(rwrec));
        memcpy(&ar[40], IP4in6_prefix, sizeof(IP4in6_prefix));
        memcpy(&ar[52], &ip, sizeof(ip));

        /* dIP */
        ip = htonl(rwRecGetDIPv4(rwrec));
        memcpy(&ar[56], IP4in6_prefix, sizeof(IP4in6_prefix));
        memcpy(&ar[68], &ip, sizeof(ip));

        /* nhIP */
        ip = htonl(rwRecGetNhIPv4(rwrec));
        memcpy(&ar[72], IP4in6_prefix, sizeof(IP4in6_prefix));
        memcpy(&ar[84], &ip, sizeof(ip));
    }

    /* swap if required */
    if (rwIOS->swapFlag) {
        ipv6routingioRecordSwap_V1(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
 *  Return length of record of specified version, or 0 if no such
 *  version exists.  See skstream_priv.h for details.
 */
uint16_t
ipv6routingioGetRecLen(
    fileVersion_t       vers)
{
    switch (vers) {
      case 1:
      case 2:
        return RECLEN_RWIPV6ROUTING_V1;
      default:
        return 0;
    }
}


/*
 *  status = ipv6routingioPrepare(&rwIOSPtr);
 *
 *    Sets the record version to the default if it is unspecified,
 *    checks that the record format supports the requested record
 *    version, sets the record length, and sets the pack and unpack
 *    functions for this record format and version.
 */
int
ipv6routingioPrepare(
    skstream_t         *rwIOS)
{
#define FILE_FORMAT "FT_RWIPV6ROUTING"
    sk_file_header_t *hdr = rwIOS->silk_hdr;
    int rv = SKSTREAM_OK; /* return value */

    /* Set version if none was selected by caller */
    if ((rwIOS->io_mode == SK_IO_WRITE)
        && (skHeaderGetRecordVersion(hdr) == SK_RECORD_VERSION_ANY))
    {
        skHeaderSetRecordVersion(hdr, DEFAULT_RECORD_VERSION);
    }

    /* version check; set values based on version */
    switch (skHeaderGetRecordVersion(hdr)) {
      case 2:
        rwIOS->rwUnpackFn = &ipv6routingioRecordUnpack_V2;
        rwIOS->rwPackFn   = &ipv6routingioRecordPack_V1;
        break;
      case 1:
        rwIOS->rwUnpackFn = &ipv6routingioRecordUnpack_V1;
        rwIOS->rwPackFn   = &ipv6routingioRecordPack_V1;
        break;
      case 0:
      default:
        rv = SKSTREAM_ERR_UNSUPPORT_VERSION;
        goto END;
    }

    rwIOS->recLen = ipv6routingioGetRecLen(skHeaderGetRecordVersion(hdr));

    /* verify lengths */
    if (rwIOS->recLen == 0) {
        skAppPrintErr("Record length not set for %s version %u",
                      FILE_FORMAT, (unsigned)skHeaderGetRecordVersion(hdr));
        skAbort();
    }
    if (rwIOS->recLen != skHeaderGetRecordLength(hdr)) {
        if (0 == skHeaderGetRecordLength(hdr)) {
            skHeaderSetRecordLength(hdr, rwIOS->recLen);
        } else {
            skAppPrintErr(("Record length mismatch for %s version %u\n"
                           "\tcode = %" PRIu16 " bytes;  header = %lu bytes"),
                          FILE_FORMAT, (unsigned)skHeaderGetRecordVersion(hdr),
                          rwIOS->recLen,
                          (unsigned long)skHeaderGetRecordLength(hdr));
            skAbort();
        }
    }

  END:
    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
