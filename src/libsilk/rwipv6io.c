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
**  rwipv6io.c
**
**    Routines to pack/unpack FT_RWIPV6 records.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwipv6io.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

/* #define RWPACK_BYTES_PACKETS          1 */
/* #define RWPACK_FLAGS_TIMES_VOLUMES    1 */
/* #define RWPACK_PROTO_FLAGS            1 */
/* #define RWPACK_SBB_PEF                1 */
/* #define RWPACK_TIME_BYTES_PKTS_FLAGS  1 */
#define RWPACK_TIMES_FLAGS_PROTO      1
#include "skstream_priv.h"
#include "rwpack.c"


/* Version to use when SK_RECORD_VERSION_ANY is specified */
#define DEFAULT_RECORD_VERSION 1


static const uint8_t IP4in6_prefix[12] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};


/* ********************************************************************* */

/*
**  RWIPV6 VERSION 2
**
**  in the following: EXPANDED == ((tcp_state & SK_TCPSTATE_EXPANDED) ? 1 : 0)
**
**    uint32_t      rflag_stime;     //  0- 3
**    // uint32_t     rest_flags: 8; //        is_tcp==0: Empty; else
**                                   //          EXPANDED==0:Empty
**                                   //          EXPANDED==1:TCPflags/!1st pkt
**    // uint32_t     is_tcp    : 1; //        1 if FLOW is TCP; 0 otherwise
**    // uint32_t     unused    : 1; //        Reserved
**    // uint32_t     stime     :22; //        Start time:msec offset from hour
**
**    uint8_t       proto_iflags;    //  4     is_tcp==0: Protocol; else:
**                                   //          EXPANDED==0:TCPflags/ALL pkts
**                                   //          EXPANDED==1:TCPflags/1st pkt
**    uint8_t       tcp_state;       //  5     TCP state machine info
**    uint16_t      application;     //  6- 7  Indication of type of traffic
**
**    uint16_t      sPort;           //  8- 9  Source port
**    uint16_t      dPort;           // 10-11  Destination port
**
**    uint32_t      elapsed;         // 12-15  Duration of the flow
**
**    uint32_t      pkts;            // 16-19  Count of packets
**    uint32_t      bytes;           // 20-23  Count of bytes
**
**    uint8_t[16]   sIP;             // 24-39  Source IP
**    uint8_t[16]   dIP;             // 40-55  Destination IP
**
**
**  56 bytes on disk.
*/

#define RECLEN_RWIPV6_V2 56


/*
 *    Byte swap the RWIPV6 v2 record 'ar' in place.
 */
#define ipv6ioRecordSwap_V2(ar)                            \
    {                                                           \
        SWAP_DATA32((ar) +  0);   /* rflag_stime */             \
        /* two single bytes (4)tcp_state, (5)proto_iflags */    \
        SWAP_DATA16((ar) +  6);   /* application */             \
        SWAP_DATA16((ar) +  8);   /* sPort */                   \
        SWAP_DATA16((ar) + 10);   /* dPort */                   \
        SWAP_DATA32((ar) + 12);   /* elapsed */                 \
        SWAP_DATA32((ar) + 16);   /* pkts */                    \
        SWAP_DATA32((ar) + 20);   /* bytes */                   \
        /* 32 bytes of sIP, dIP always in network byte order */ \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
ipv6ioRecordUnpack_V2(
    skstream_t         *rwIOS,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    uint32_t ip;

    /* swap if required */
    if (rwIOS->swapFlag) {
        ipv6ioRecordSwap_V2(ar);
    }

    /* Start time, TCP flags, Protocol, TCP State */
    rwpackUnpackTimesFlagsProto(rwrec, ar, rwIOS->hdr_starttime);

    /* application */
    rwRecMemSetApplication(rwrec, &ar[ 6]);

    /* sPort, dPort */
    rwRecMemSetSPort(rwrec, &ar[ 8]);
    rwRecMemSetDPort(rwrec, &ar[10]);

    /* Elapsed */
    rwRecMemSetElapsed(rwrec, &ar[12]);

    /* packets, bytes */
    rwRecMemSetPkts(rwrec,  &ar[16]);
    rwRecMemSetBytes(rwrec, &ar[20]);

    /* sIP, dIP */
    if (ar[5] & 0x80) {
        /* Record is IPv6 */
#if !SK_ENABLE_IPV6
        return SKSTREAM_ERR_UNSUPPORT_IPV6;
#else
        rwRecSetIPv6(rwrec);
        rwRecMemSetSIPv6(rwrec, &ar[24]);
        rwRecMemSetDIPv6(rwrec, &ar[40]);
#endif /* SK_ENABLE_IPV6 */
    } else {
        /* Record is IPv4 */
        memcpy(&ip, &ar[24+12], sizeof(ip));
        rwRecSetSIPv4(rwrec, ntohl(ip));

        memcpy(&ip, &ar[40+12], sizeof(ip));
        rwRecSetDIPv4(rwrec, ntohl(ip));
    }

    /* sensor, flow_type from file name/header */
    rwRecSetSensor(rwrec, rwIOS->hdr_sensor);
    rwRecSetFlowType(rwrec, rwIOS->hdr_flowtype);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
ipv6ioRecordPack_V2(
    skstream_t             *rwIOS,
    const rwGenericRec_V5  *rwrec,
    uint8_t                *ar)
{
    uint32_t ip;
    int rv;

    /* Start time, TCP Flags, Protocol, TCP State */
    rv = rwpackPackTimesFlagsProto(rwrec, ar, rwIOS->hdr_starttime);
    if (rv) {
        return rv;
    }

    /* application */
    rwRecMemGetApplication(rwrec, &ar[6]);

    /* sPort, dPort */
    rwRecMemGetSPort(rwrec, &ar[ 8]);
    rwRecMemGetDPort(rwrec, &ar[10]);

    /* Elapsed */
    rwRecMemGetElapsed(rwrec, &ar[12]);

    /* packets, bytes */
    rwRecMemGetPkts(rwrec,  &ar[16]);
    rwRecMemGetBytes(rwrec, &ar[20]);

    /* sIP, dIP */
    if (rwRecIsIPv6(rwrec)) {
        /* Record is IPv6 */
#if !SK_ENABLE_IPV6
        return SKSTREAM_ERR_UNSUPPORT_IPV6;
#else
        ar[ 5] |= 0x80;
        rwRecMemGetSIPv6(rwrec, &ar[24]);
        rwRecMemGetDIPv6(rwrec, &ar[40]);
#endif /* SK_ENABLE_IPV6 */
    } else {
        /* Record is IPv4, but encode as IPv6 */
        ip = htonl(rwRecGetSIPv4(rwrec));
        memcpy(&ar[24], IP4in6_prefix, sizeof(IP4in6_prefix));
        memcpy(&ar[24+12], &ip, sizeof(ip));

        ip = htonl(rwRecGetDIPv4(rwrec));
        memcpy(&ar[40], IP4in6_prefix, sizeof(IP4in6_prefix));
        memcpy(&ar[40+12], &ip, sizeof(ip));
    }

    /* swap if required */
    if (rwIOS->swapFlag) {
        ipv6ioRecordSwap_V2(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
**  RWIPV6 VERSION 1
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
**    uint32_t      pkts;        // 28-31  Count of packets
**    uint32_t      bytes;       // 32-35  Count of bytes
**
**    uint8_t[16]   sIP;         // 36-51  Source IP
**    uint8_t[16]   dIP;         // 52-67  Destination IP
**
**
**  68 bytes on disk.
*/

#define RECLEN_RWIPV6_V1 68


/*
 *    Byte swap the RWIPV6 v1 record 'ar' in place.
 */
#define ipv6ioRecordSwap_V1(ar)                                 \
    {                                                           \
        SWAP_DATA64((ar) +  0);   /* sTime */                   \
        SWAP_DATA32((ar) +  8);   /* elapsed */                 \
        SWAP_DATA16((ar) + 12);   /* sPort */                   \
        SWAP_DATA16((ar) + 14);   /* dPort */                   \
        /* Two single bytes: (16)proto, (17)flow_type */        \
        SWAP_DATA16((ar) + 18);   /* sID */                     \
        /* Four single bytes: (20)flags, (21)init_flags,        \
         *                    (22)rest_flags, (23)tcp_state */  \
        SWAP_DATA16((ar) + 24);   /* application */             \
        SWAP_DATA16((ar) + 26);   /* memo */                    \
        SWAP_DATA32((ar) + 28);   /* pkts */                    \
        SWAP_DATA32((ar) + 32);   /* bytes */                   \
        /* 32 bytes of sIP, dIP always in network byte order */ \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
ipv6ioRecordUnpack_V1(
    skstream_t         *rwIOS,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    uint32_t ip;

    /* swap if required */
    if (rwIOS->swapFlag) {
        ipv6ioRecordSwap_V1(ar);
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
    rwRecMemSetPkts(rwrec, &ar[28]);
    rwRecMemSetBytes(rwrec, &ar[32]);

    if (ar[23] & 0x80) {
        /* Record is IPv6 */
#if !SK_ENABLE_IPV6
        return SKSTREAM_ERR_UNSUPPORT_IPV6;
#else
        rwRecSetIPv6(rwrec);
        rwRecMemSetSIPv6(rwrec, &ar[36]);
        rwRecMemSetDIPv6(rwrec, &ar[52]);
#endif /* SK_ENABLE_IPV6 */
    } else {
        /* Record is IPv4 */

        /* sIP */
        memcpy(&ip, &ar[48], sizeof(ip));
        rwRecSetSIPv4(rwrec, ntohl(ip));

        /* dIP */
        memcpy(&ip, &ar[64], sizeof(ip));
        rwRecSetDIPv4(rwrec, ntohl(ip));
    }

    RWREC_MAYBE_CLEAR_TCPSTATE_EXPANDED(rwrec);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
ipv6ioRecordPack_V1(
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
    rwRecMemGetPkts(rwrec, &ar[28]);
    rwRecMemGetBytes(rwrec, &ar[32]);

    if (rwRecIsIPv6(rwrec)) {
        /* Record is IPv6 */
#if !SK_ENABLE_IPV6
        return SKSTREAM_ERR_UNSUPPORT_IPV6;
#else
        ar[23] |= 0x80;
        rwRecMemGetSIPv6(rwrec, &ar[36]);
        rwRecMemGetDIPv6(rwrec, &ar[52]);
#endif /* SK_ENABLE_IPV6 */
    } else {
        /* Record is IPv4, but encode as IPv6 */

        /* sIP */
        ip = htonl(rwRecGetSIPv4(rwrec));
        memcpy(&ar[36], IP4in6_prefix, sizeof(IP4in6_prefix));
        memcpy(&ar[48], &ip, sizeof(ip));

        /* dIP */
        ip = htonl(rwRecGetDIPv4(rwrec));
        memcpy(&ar[52], IP4in6_prefix, sizeof(IP4in6_prefix));
        memcpy(&ar[64], &ip, sizeof(ip));
    }

    /* swap if required */
    if (rwIOS->swapFlag) {
        ipv6ioRecordSwap_V1(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
 *  Return length of record of specified version, or 0 if no such
 *  version exists.  See skstream_priv.h for details.
 */
uint16_t
ipv6ioGetRecLen(
    fileVersion_t       vers)
{
    switch (vers) {
      case 1:
        return RECLEN_RWIPV6_V1;
      case 2:
        return RECLEN_RWIPV6_V2;
      default:
        return 0;
    }
}


/*
 *  status = ipv6ioPrepare(&rwIOSPtr);
 *
 *    Sets the record version to the default if it is unspecified,
 *    checks that the record format supports the requested record
 *    version, sets the record length, and sets the pack and unpack
 *    functions for this record format and version.
 */
int
ipv6ioPrepare(
    skstream_t         *rwIOS)
{
#define FILE_FORMAT "FT_RWIPV6"
    sk_file_header_t *hdr = rwIOS->silk_hdr;
    int rv = SKSTREAM_OK; /* return value */

    assert(skHeaderGetFileFormat(hdr) == FT_RWIPV6);

    /* Set version if none was selected by caller */
    if ((rwIOS->io_mode == SK_IO_WRITE)
        && (skHeaderGetRecordVersion(hdr) == SK_RECORD_VERSION_ANY))
    {
        skHeaderSetRecordVersion(hdr, DEFAULT_RECORD_VERSION);
    }

    /* version check; set values based on version */
    switch (skHeaderGetRecordVersion(hdr)) {
      case 2:
        rwIOS->rwUnpackFn = &ipv6ioRecordUnpack_V2;
        rwIOS->rwPackFn   = &ipv6ioRecordPack_V2;
        break;
      case 1:
        rwIOS->rwUnpackFn = &ipv6ioRecordUnpack_V1;
        rwIOS->rwPackFn   = &ipv6ioRecordPack_V1;
        break;
      case 0:
      default:
        rv = SKSTREAM_ERR_UNSUPPORT_VERSION;
        goto END;
    }

    rwIOS->recLen = ipv6ioGetRecLen(skHeaderGetRecordVersion(hdr));

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
