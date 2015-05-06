/*
** Copyright (C) 2006-2015 by Carnegie Mellon University.
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
** rwaugsnmpoutio.c
**
**      routines to do io stuff with augsnmpout records.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwaugsnmpoutio.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

/* #define RWPACK_BYTES_PACKETS          1 */
#define RWPACK_FLAGS_TIMES_VOLUMES    1
#define RWPACK_PROTO_FLAGS            1
/* #define RWPACK_SBB_PEF                1 */
#define RWPACK_TIME_BYTES_PKTS_FLAGS  1
#define RWPACK_TIMES_FLAGS_PROTO      1
#include "skstream_priv.h"
#include "rwpack.c"


/* Version to use when SK_RECORD_VERSION_ANY is specified */
#define DEFAULT_RECORD_VERSION 4


/* ********************************************************************* */

/*
**  RWAUGSNMPOUT VERSION 5
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
**    uint32_t      sIP;             // 24-27  Source IP
**    uint32_t      dIP;             // 28-31  Destination IP
**
**    uint16_t      output;          // 32-33  Router outgoing SNMP interface
**
**
**  34 bytes on disk.
*/

#define RECLEN_RWAUGSNMPOUT_V5 34


/*
 *    Byte swap the RWAUGSNMPOUT v5 record 'ar' in place.
 */
#define augsnmpoutioRecordSwap_V5(ar)                           \
    {                                                           \
        SWAP_DATA32((ar) +  0);   /* rflag_stime */             \
        /* two single bytes (4)proto_iflags, (5)tcp_state */    \
        SWAP_DATA16((ar) +  6);   /* application */             \
        SWAP_DATA16((ar) +  8);   /* sPort */                   \
        SWAP_DATA16((ar) + 10);   /* dPort */                   \
        SWAP_DATA32((ar) + 12);   /* elapsed */                 \
        SWAP_DATA32((ar) + 16);   /* pkts */                    \
        SWAP_DATA32((ar) + 20);   /* bytes */                   \
        SWAP_DATA32((ar) + 24);   /* sIP */                     \
        SWAP_DATA32((ar) + 28);   /* dIP */                     \
        SWAP_DATA16((ar) + 32);   /* output */                  \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
augsnmpoutioRecordUnpack_V5(
    skstream_t         *rwIOS,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    /* swap if required */
    if (rwIOS->swapFlag) {
        augsnmpoutioRecordSwap_V5(ar);
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
    rwRecMemSetSIPv4(rwrec, &ar[24]);
    rwRecMemSetDIPv4(rwrec, &ar[28]);

    /* output */
    rwRecMemSetOutput(rwrec, &ar[32]);

    /* sensor, flow_type from file name/header */
    rwRecSetSensor(rwrec, rwIOS->hdr_sensor);
    rwRecSetFlowType(rwrec, rwIOS->hdr_flowtype);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
augsnmpoutioRecordPack_V5(
    skstream_t             *rwIOS,
    const rwGenericRec_V5  *rwrec,
    uint8_t                *ar)
{
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
    rwRecMemGetSIPv4(rwrec, &ar[24]);
    rwRecMemGetDIPv4(rwrec, &ar[28]);

    /* output */
    rwRecMemGetOutput(rwrec, &ar[32]);

    /* swap if required */
    if (rwIOS->swapFlag) {
        augsnmpoutioRecordSwap_V5(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
**  RWAUGSNMPOUT VERSION 4
**
**  in the following: EXPANDED == ((tcp_state & SK_TCPSTATE_EXPANDED) ? 1 : 0)
**
**    uint32_t      stime_bb1;       //  0- 3
**    // uint32_t     stime     :22  //        Start time:msec offset from hour
**    // uint32_t     bPPkt1    :10; //        Whole bytes-per-packet (hi 10)
**
**    uint32_t      bb2_elapsed;     //  4- 7
**    // uint32_t     bPPkt2    : 4; //        Whole bytes-per-packet (low 4)
**    // uint32_t     bPPFrac   : 6; //        Fractional bytes-per-packet
**    // uint32_t     elapsed   :22; //        Duration of flow in msec
**
**    uint32_t      pro_flg_pkts;    //  8-11
**    // uint32_t     prot_flags: 8; //        is_tcp==0: IP protocol
**                                   //        is_tcp==1 &&
**                                   //          EXPANDED==0:TCPflags/All pkts
**                                   //          EXPANDED==1:TCPflags/1st pkt
**    // uint32_t     pflag     : 1; //        'pkts' requires multiplier?
**    // uint32_t     is_tcp    : 1; //        1 if flow is TCP; 0 otherwise
**    // uint32_t     padding   : 2; //
**    // uint32_t     pkts      :20; //        Count of packets
**
**    uint8_t       tcp_state;       // 12     TCP state machine info
**    uint8_t       rest_flags;      // 13     is_tcp==0: Flow's reported flags
**                                   //        is_tcp==1 &&
**                                   //          EXPANDED==0:Empty
**                                   //          EXPANDED==1:TCPflags/!1st pkt
**    uint16_t      application;     // 14-15  Type of traffic
**
**    uint16_t      sPort;           // 16-17  Source port
**    uint16_t      dPort;           // 18-19  Destination port
**
**    uint32_t      sIP;             // 20-23  Source IP
**    uint32_t      dIP;             // 24-27  Destination IP
**
**    uint16_t      output;          // 28-29  Router outgoing SNMP interface
**
**
**  30 bytes on disk.
*/

#define RECLEN_RWAUGSNMPOUT_V4 30


/*
 *    Byte swap the RWAUGSNMPOUT v4 record 'ar' in place.
 */
#define augsnmpoutioRecordSwap_V4(ar)                           \
    {                                                           \
        SWAP_DATA32((ar) +  0);   /* stime_bb1 */               \
        SWAP_DATA32((ar) +  4);   /* bb2_elapsed */             \
        SWAP_DATA32((ar) +  8);   /* pro_flg_pkts */            \
        /* two single bytes (12)tcp_state, (13)rest_flags */    \
        SWAP_DATA16((ar) + 14);   /* application */             \
        SWAP_DATA16((ar) + 16);   /* sPort */                   \
        SWAP_DATA16((ar) + 18);   /* dPort */                   \
        SWAP_DATA32((ar) + 20);   /* sIP */                     \
        SWAP_DATA32((ar) + 24);   /* dIP */                     \
        SWAP_DATA16((ar) + 28);   /* output */                  \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
augsnmpoutioRecordUnpack_V4(
    skstream_t         *rwIOS,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    /* swap if required */
    if (rwIOS->swapFlag) {
        augsnmpoutioRecordSwap_V4(ar);
    }

    /* sTime, elapsed, pkts, bytes, proto, tcp-flags, state, application */
    rwpackUnpackFlagsTimesVolumes(rwrec, ar, rwIOS->hdr_starttime, 16, 0);

    /* sPort, dPort */
    rwRecMemSetSPort(rwrec, &ar[16]);
    rwRecMemSetDPort(rwrec, &ar[18]);

    /* sIP, dIP */
    rwRecMemSetSIPv4(rwrec, &ar[20]);
    rwRecMemSetDIPv4(rwrec, &ar[24]);

    /* output */
    rwRecMemSetOutput(rwrec, &ar[28]);

    /* sensor, flow_type from file name/header */
    rwRecSetSensor(rwrec, rwIOS->hdr_sensor);
    rwRecSetFlowType(rwrec, rwIOS->hdr_flowtype);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
augsnmpoutioRecordPack_V4(
    skstream_t             *rwIOS,
    const rwGenericRec_V5  *rwrec,
    uint8_t                *ar)
{
    int rv = SKSTREAM_OK; /* return value */

    /* sTime, elapsed, pkts, bytes, proto, tcp-flags, state, application */
    rv = rwpackPackFlagsTimesVolumes(ar, rwrec, rwIOS->hdr_starttime, 16);
    if (rv) {
        return rv;
    }

    /* sPort, dPort */
    rwRecMemGetSPort(rwrec, &ar[16]);
    rwRecMemGetDPort(rwrec, &ar[18]);

    /* sIP, dIP */
    rwRecMemGetSIPv4(rwrec, &ar[20]);
    rwRecMemGetDIPv4(rwrec, &ar[24]);

    /* output */
    rwRecMemGetOutput(rwrec, &ar[28]);

    /* swap if required */
    if (rwIOS->swapFlag) {
        augsnmpoutioRecordSwap_V4(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
**  RWAUGSNMPOUT VERSION 1
**  RWAUGSNMPOUT VERSION 2
**  RWAUGSNMPOUT VERSION 3
**
**  in the following: EXPANDED == ((tcp_state & SK_TCPSTATE_EXPANDED) ? 1 : 0)
**
**    uint32_t      sIP;             //  0- 3  Source IP
**    uint32_t      dIP;             //  4- 7  Destination IP
**
**    uint16_t      sPort;           //  8- 9  Source port
**    uint16_t      dPort;           // 10-11  Destination port
**
**    uint32_t      pkts_stime;      // 12-15
**    // uint32_t     pkts      :20; //        Count of packets
**    // uint32_t     sTime     :12; //        Start time--offset from hour
**
**    uint32_t      bbe;             // 16-19
**    // uint32_t     bPPkt     :14; //        Whole bytes-per-packet
**    // uint32_t     bPPFrac   : 6; //        Fractional bytes-per-packet
**    // uint32_t     elapsed   :12; //        Duration of flow
**
**    uint32_t      msec_flags       // 20-23
**    // uint32_t     sTime_msec:10; //        Fractional sTime (millisec)
**    // uint32_t     elaps_msec:10; //        Fractional elapsed (millisec)
**    // uint32_t     pflag     : 1; //        'pkts' requires multiplier?
**    // uint32_t     is_tcp    : 1; //        1 if flow is TCP; 0 otherwise
**    // uint32_t     padding   : 2; //        padding/reserved
**    // uint32_t     prot_flags: 8; //        is_tcp==0: IP protocol
**                                   //        is_tcp==1 &&
**                                   //          EXPANDED==0:TCPflags/All pkts
**                                   //          EXPANDED==1:TCPflags/1st pkt
**
**    uint16_t      application;     // 24-25  Type of traffic
**
**    uint8_t       tcp_state;       // 26     TCP state machine info
**    uint8_t       rest_flags;      // 27     is_tcp==0: Flow's reported flags
**                                   //        is_tcp==1 &&
**                                   //          EXPANDED==0:Empty
**                                   //          EXPANDED==1:TCPflags/!1st pkt
**
**    uint16_t      output;          // 28-29  Router outgoing SNMP interface
**
**
**  30 bytes on disk.
*/

#define RECLEN_RWAUGSNMPOUT_V1 30
#define RECLEN_RWAUGSNMPOUT_V2 30
#define RECLEN_RWAUGSNMPOUT_V3 30


/*
 *    Byte swap the RWAUGSNMPOUT v1 record 'ar' in place.
 */
#define augsnmpoutioRecordSwap_V1(ar)                           \
    {                                                           \
        SWAP_DATA32((ar) +  0);   /* sIP */                     \
        SWAP_DATA32((ar) +  4);   /* dIP */                     \
        SWAP_DATA16((ar) +  8);   /* sPort */                   \
        SWAP_DATA16((ar) + 10);   /* dPort */                   \
        SWAP_DATA32((ar) + 12);   /* pkts_stime */              \
        SWAP_DATA32((ar) + 16);   /* bbe */                     \
        SWAP_DATA32((ar) + 20);   /* msec_flags */              \
        SWAP_DATA16((ar) + 24);   /* application */             \
        /* Two single bytes: (26)tcp_state, (27)rest_flags */   \
        SWAP_DATA16((ar) + 28);   /* output */                  \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
augsnmpoutioRecordUnpack_V1(
    skstream_t         *rwIOS,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    uint32_t msec_flags;
    uint8_t is_tcp, prot_flags;

    /* swap if required */
    if (rwIOS->swapFlag) {
        augsnmpoutioRecordSwap_V1(ar);
    }

    /* sIP, dIP, sPort, dPort */
    rwRecMemSetSIPv4(rwrec, &ar[0]);
    rwRecMemSetDIPv4(rwrec, &ar[4]);
    rwRecMemSetSPort(rwrec, &ar[8]);
    rwRecMemSetDPort(rwrec, &ar[10]);

    /* msec times, proto or flags */
    memcpy(&msec_flags, &ar[20], 4);

    /* application */
    rwRecMemSetApplication(rwrec, &ar[24]);

    /* sTime, pkts, bytes, elapsed, proto, tcp-flags, bpp */
    rwpackUnpackTimeBytesPktsFlags(rwrec, rwIOS->hdr_starttime,
                                   (uint32_t*)&ar[12], (uint32_t*)&ar[16],
                                   &msec_flags);

    /* extra TCP information */
    is_tcp = (uint8_t)GET_MASKED_BITS(msec_flags, 10, 1);
    prot_flags = (uint8_t)GET_MASKED_BITS(msec_flags, 0, 8);
    rwpackUnpackProtoFlags(rwrec, is_tcp, prot_flags, ar[26], ar[27]);

    /* output interfaces */
    rwRecMemSetOutput(rwrec, &ar[28]);

    /* sensor, flow_type from file name/header */
    rwRecSetSensor(rwrec, rwIOS->hdr_sensor);
    rwRecSetFlowType(rwrec, rwIOS->hdr_flowtype);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
augsnmpoutioRecordPack_V1(
    skstream_t             *rwIOS,
    const rwGenericRec_V5  *rwrec,
    uint8_t                *ar)
{
    int rv = SKSTREAM_OK; /* return value */
    uint32_t msec_flags;
    uint8_t is_tcp, prot_flags;

    /* sTime, pkts, bytes, elapsed, proto, tcp-flags, bpp */
    rv = rwpackPackTimeBytesPktsFlags((uint32_t*)&ar[12], (uint32_t*)&ar[16],
                                      &msec_flags,
                                      rwrec, rwIOS->hdr_starttime);
    if (rv) {
        return rv;
    }

    rwpackPackProtoFlags(&is_tcp, &prot_flags, &ar[26], &ar[27], rwrec);

    /* msec_flags: sTime_msec:10; elaps_msec:10; pflag:1;
     *             is_tcp:1; pad:2; prot_flags:8; */
    /* overwrite the least significant 11 bits */
    msec_flags = ((msec_flags & (MASKARRAY_21 << 11))
                  | (is_tcp ? (1 << 10) : 0)
                  | prot_flags);

    /* sIP, dIP, sPort, dPort */
    rwRecMemGetSIPv4(rwrec, &ar[0]);
    rwRecMemGetDIPv4(rwrec, &ar[4]);
    rwRecMemGetSPort(rwrec, &ar[8]);
    rwRecMemGetDPort(rwrec, &ar[10]);

    /* msec_flags */
    memcpy(&ar[20], &msec_flags, 4);

    /* application */
    rwRecMemGetApplication(rwrec, &ar[24]);

    /* output interfaces */
    rwRecMemGetOutput(rwrec, &ar[28]);

    /* swap if required */
    if (rwIOS->swapFlag) {
        augsnmpoutioRecordSwap_V1(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
 *  Return length of record of specified version, or 0 if no such
 *  version exists.  See skstream_priv.h for details.
 */
uint16_t
augsnmpoutioGetRecLen(
    fileVersion_t       vers)
{
    switch (vers) {
      case 1:
        return RECLEN_RWAUGSNMPOUT_V1;
      case 2:
        return RECLEN_RWAUGSNMPOUT_V2;
      case 3:
        return RECLEN_RWAUGSNMPOUT_V3;
      case 4:
        return RECLEN_RWAUGSNMPOUT_V4;
      case 5:
        return RECLEN_RWAUGSNMPOUT_V5;
      default:
        return 0;
    }
}


/*
 *  status = augsnmpoutioPrepare(&rwIOSPtr);
 *
 *    Sets the record version to the default if it is unspecified,
 *    checks that the record format supports the requested record
 *    version, sets the record length, and sets the pack and unpack
 *    functions for this record format and version.
 */
int
augsnmpoutioPrepare(
    skstream_t         *rwIOS)
{
#define FILE_FORMAT "FT_RWAUGSNMPOUT"
    sk_file_header_t *hdr = rwIOS->silk_hdr;
    int rv = SKSTREAM_OK; /* return value */

    assert(skHeaderGetFileFormat(hdr) == FT_RWAUGSNMPOUT);

    /* Set version if none was selected by caller */
    if ((rwIOS->io_mode == SK_IO_WRITE)
        && (skHeaderGetRecordVersion(hdr) == SK_RECORD_VERSION_ANY))
    {
        skHeaderSetRecordVersion(hdr, DEFAULT_RECORD_VERSION);
    }

    /* version check; set values based on version */
    switch (skHeaderGetRecordVersion(hdr)) {
      case 5:
        rwIOS->rwUnpackFn = &augsnmpoutioRecordUnpack_V5;
        rwIOS->rwPackFn   = &augsnmpoutioRecordPack_V5;
        break;
      case 4:
        rwIOS->rwUnpackFn = &augsnmpoutioRecordUnpack_V4;
        rwIOS->rwPackFn   = &augsnmpoutioRecordPack_V4;
        break;
      case 3:
      case 2:
      case 1:
        /* V1 and V2 differ only in the padding of the header */
        /* V2 and V3 differ only in that V3 supports compression on
         * read and write; V2 supports compression only on read */
        rwIOS->rwUnpackFn = &augsnmpoutioRecordUnpack_V1;
        rwIOS->rwPackFn   = &augsnmpoutioRecordPack_V1;
        break;
      case 0:
      default:
        rv = SKSTREAM_ERR_UNSUPPORT_VERSION;
        goto END;
    }

    rwIOS->recLen = augsnmpoutioGetRecLen(skHeaderGetRecordVersion(hdr));

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
