/*
** Copyright (C) 2007-2014 by Carnegie Mellon University.
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
**  skipfix.c
**
**    SiLK Flow Record / IPFIX translation core
**
**    Brian Trammell
**    February 2007
*/

#define LIBFLOWSOURCE_SOURCE 1
#include <silk/silk.h>

RCSIDENT("$SiLK: skipfix.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/skipfix.h>
#include <silk/sklog.h>
#include <silk/skvector.h>
#include <silk/utils.h>

#ifdef SKIPFIX_TRACE_LEVEL
#define TRACEMSG_LEVEL 1
#endif
#define TRACEMSG(x)  TRACEMSG_TO_TRACEMSGLVL(1, x)
#include <silk/sktracemsg.h>

/* Uncomment to force fixbuf-1.3.0 compatibility */
/* #undef SK_HAVE_FBTEMPLATEGETCONTEXT */

/* LOCAL DEFINES AND TYPEDEFS */

/* The IPFIX Private Enterprise Number for CERT */
#define IPFIX_CERT_PEN  6871

/* Extenal Template ID used for SiLK Flows written by rwsilk2ipfix.
 * This is defined in skipfix.h. */
/* #define SKI_RWREC_TID        0xAFEA */

/* Internal Template ID for extended SiLK flows. */
#define SKI_EXTRWREC_TID        0xAFEB

/* Internal Template ID for TCP information. */
#define SKI_TCP_STML_TID        0xAFEC

/* Internal Template ID for NetFlowV9 Sampling Options Template */
#define SKI_NF9_SAMPLING_TID    0xAFED

/* Bit in Template ID that yaf sets for templates containing reverse
 * elements */
#define SKI_YAF_REVERSE_BIT     0x0010

/* Template ID used by yaf for a yaf stats option record */
#define SKI_YAF_STATS_TID       0xD000

/* Template ID used by yaf for a subTemplateMultiList containing only
 * forward TCP flags information. */
#define SKI_YAF_TCP_FLOW_TID    0xC003

/* Name of environment variable that, when set, cause SiLK to print
 * the templates that it receives to the log. */
#define SKI_ENV_PRINT_TEMPLATES  "SILK_IPFIX_PRINT_TEMPLATES"


/* Compatilibity for libfixbuf < 1.4.0 */
#ifndef FB_IE_INIT_FULL

#define FB_IE_INIT_FULL(_name_, _ent_, _num_, _len_, _flags_, _min_, _max_, _type_, _desc_) \
    FB_IE_INIT(_name_, _ent_, _num_, _len_, _flags_)

#define FB_IE_QUANTITY          0
#define FB_IE_TOTALCOUNTER      0
#define FB_IE_DELTACOUNTER      0
#define FB_IE_IDENTIFIER        0
#define FB_IE_FLAGS             0
#define FB_IE_LIST              0

#define FB_UNITS_BITS           0
#define FB_UNITS_OCTETS         0
#define FB_UNITS_PACKETS        0
#define FB_UNITS_FLOWS          0
#define FB_UNITS_SECONDS        0
#define FB_UNITS_MILLISECONDS   0
#define FB_UNITS_MICROSECONDS   0
#define FB_UNITS_NANOSECONDS    0
#define FB_UNITS_WORDS          0
#define FB_UNITS_MESSAGES       0
#define FB_UNITS_HOPS           0
#define FB_UNITS_ENTRIES        0

#endif  /* FB_IE_INIT_FULL */


/*
 *  val = CLAMP_VAL(val, max);
 *
 *    If 'val' is greater then 'max', return 'max'.  Otherwise,
 *    return (max & val).
 */
#define CLAMP_VAL(val, max) \
    (((val) > (max)) ? (max) : ((max) & (val)))

/* One more than UINT32_MAX */
#define ROLLOVER32 ((intmax_t)UINT32_MAX + 1)

/*
 *    For NetFlow V9, when the absolute value of the magnitude of the
 *    difference between the sysUpTime and the flowStartSysUpTime is
 *    greater than this value (in milliseconds), assume one of the
 *    values has rolled over.
 */
#define MAXIMUM_FLOW_TIME_DEVIATION  ((intmax_t)INT32_MAX)

/* Define the IPFIX information elements in IPFIX_CERT_PEN space for
 * SiLK */
static fbInfoElement_t ski_info_elements[] = {
    /* Extra fields produced by yaf for SiLK records */
    FB_IE_INIT("initialTCPFlags",              IPFIX_CERT_PEN, 14,  1,
               FB_IE_F_ENDIAN | FB_IE_F_REVERSIBLE),
    FB_IE_INIT("unionTCPFlags",                IPFIX_CERT_PEN, 15,  1,
               FB_IE_F_ENDIAN | FB_IE_F_REVERSIBLE),
    FB_IE_INIT("reverseFlowDeltaMilliseconds", IPFIX_CERT_PEN, 21,  4,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("silkFlowType",                 IPFIX_CERT_PEN, 30,  1,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("silkFlowSensor",               IPFIX_CERT_PEN, 31,  2,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("silkTCPState",                 IPFIX_CERT_PEN, 32,  1,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("silkAppLabel",                 IPFIX_CERT_PEN, 33,  2,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("flowAttributes",               IPFIX_CERT_PEN, 40,  2,
               FB_IE_F_ENDIAN | FB_IE_F_REVERSIBLE),

    /* Extra fields produced by yaf for yaf statistics */
    FB_IE_INIT("expiredFragmentCount",         IPFIX_CERT_PEN, 100, 4,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("assembledFragmentCount",       IPFIX_CERT_PEN, 101, 4,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("meanFlowRate",                 IPFIX_CERT_PEN, 102, 4,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("meanPacketRate",               IPFIX_CERT_PEN, 103, 4,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("flowTableFlushEventCount",     IPFIX_CERT_PEN, 104, 4,
               FB_IE_F_ENDIAN),
    FB_IE_INIT("flowTablePeakCount",           IPFIX_CERT_PEN, 105, 4,
               FB_IE_F_ENDIAN),
    FB_IE_NULL
};


/* These are IPFIX information elements either in the standard space
 * or specific to NetFlowV9.  However, these elements are not defined
 * in all versions of libfixbuf. */
static fbInfoElement_t ski_std_info_elements[] = {
#ifndef SK_HAVE_FBTEMPLATEGETCONTEXT
    /* the following are defined as of libfixbuf-1.4.0 */
    FB_IE_INIT_FULL("flowSamplerID", 0, 48, 1,
                    FB_IE_F_ENDIAN | FB_IE_IDENTIFIER,
                    0, 0, FB_UINT_8, NULL),
#endif  /* SK_HAVE_FBTEMPLATEGETCONTEXT */
    FB_IE_NULL
};


/* Bytes of padding to add to ski_rwrec_spec and ski_rwrec_st to get a
 * multiple of 64bits */
#define SKI_RWREC_PADDING  6

/*
 * This is an IPFIX encoding of a standard SiLK Flow record (rwRec);
 * it is used for export by rwsilk2ipfix where it has Template ID
 * SKI_RWREC_TID.
 *
 * This also becomes part of the "Extended" record, ski_extrwrec_spec,
 * that is used for import.
 *
 * Keep this in sync with the ski_rwrec_t below.  Use
 * SKI_RWREC_PADDING to pad to 64bits. */
static fbInfoElementSpec_t ski_rwrec_spec[] = {
    /* Millisecond start and end (epoch) (native time) */
    { (char*)"flowStartMilliseconds",              8, 0 },
    { (char*)"flowEndMilliseconds",                8, 0 },
    /* 4-tuple */
    { (char*)"sourceIPv6Address",                 16, 0 },
    { (char*)"destinationIPv6Address",            16, 0 },
    { (char*)"sourceIPv4Address",                  4, 0 },
    { (char*)"destinationIPv4Address",             4, 0 },
    { (char*)"sourceTransportPort",                2, 0 },
    { (char*)"destinationTransportPort",           2, 0 },
    /* Router interface information */
    { (char*)"ipNextHopIPv4Address",               4, 0 },
    { (char*)"ipNextHopIPv6Address",              16, 0 },
    { (char*)"ingressInterface",                   4, 0 },
    { (char*)"egressInterface",                    4, 0 },
    /* Counters (reduced length encoding for SiLK) */
    { (char*)"packetDeltaCount",                   8, 0 },
    { (char*)"octetDeltaCount",                    8, 0 },
    /* Protocol; sensor information */
    { (char*)"protocolIdentifier",                 1, 0 },
    { (char*)"silkFlowType",                       1, 0 },
    { (char*)"silkFlowSensor",                     2, 0 },
    /* Flags */
    { (char*)"tcpControlBits",                     1, 0 },
    { (char*)"initialTCPFlags",                    1, 0 },
    { (char*)"unionTCPFlags",                      1, 0 },
    { (char*)"silkTCPState",                       1, 0 },
    { (char*)"silkAppLabel",                       2, 0 },
    /* pad record to 64-bit boundary */
#if SKI_RWREC_PADDING != 0
    { (char*)"paddingOctets",  SKI_RWREC_PADDING, 0 },
#endif
    FB_IESPEC_NULL
};

/* Keep this in sync with the ski_rwrec_spec above. Pad to 64bits */
typedef struct ski_rwrec_st {
    uint64_t        flowStartMilliseconds;          /*   0-  7 */
    uint64_t        flowEndMilliseconds;            /*   8- 15 */

    uint8_t         sourceIPv6Address[16];          /*  16- 31 */
    uint8_t         destinationIPv6Address[16];     /*  32- 47 */

    uint32_t        sourceIPv4Address;              /*  48- 51 */
    uint32_t        destinationIPv4Address;         /*  52- 55 */

    uint16_t        sourceTransportPort;            /*  56- 57 */
    uint16_t        destinationTransportPort;       /*  58- 59 */

    uint32_t        ipNextHopIPv4Address;           /*  60- 63 */
    uint8_t         ipNextHopIPv6Address[16];       /*  64- 79 */
    uint32_t        ingressInterface;               /*  80- 83 */
    uint32_t        egressInterface;                /*  84- 87 */

    uint64_t        packetDeltaCount;               /*  88- 95 */
    uint64_t        octetDeltaCount;                /*  96-103 */

    uint8_t         protocolIdentifier;             /* 104     */
    flowtypeID_t    silkFlowType;                   /* 105     */
    sensorID_t      silkFlowSensor;                 /* 106-107 */

    uint8_t         tcpControlBits;                 /* 108     */
    uint8_t         initialTCPFlags;                /* 109     */
    uint8_t         unionTCPFlags;                  /* 110     */
    uint8_t         silkTCPState;                   /* 111     */
    uint16_t        silkAppLabel;                   /* 112-113 */
#if SKI_RWREC_PADDING != 0
    uint8_t         pad[SKI_RWREC_PADDING];         /* 114-119 */
#endif
} ski_rwrec_t;



/* Bytes of padding to add to ski_extrwrec_spec and ski_extrwrec_st to
 * get a multiple of 64bits */
#define SKI_EXTRWREC_PADDING  7

/* These are additional IPFIX fields (or different encodings of the
 * SiLK fields) that we may get from other flowmeters.  They will be
 * appended to the ski_rwrec_spec (above) to create the complete
 * ski_extrwrec_t (defined below).  This has Template ID
 * SKI_EXTRWREC_TID. */
static fbInfoElementSpec_t ski_extrwrec_spec[] = {
    /* Total counter support */
    { (char*)"packetTotalCount",                   8, 0 },
    { (char*)"octetTotalCount",                    8, 0 },
    { (char*)"initiatorPackets",                   8, 0 },
    { (char*)"initiatorOctets",                    8, 0 },
    /* Reverse counter support */
    { (char*)"reversePacketDeltaCount",            8, 0 },
    { (char*)"reverseOctetDeltaCount",             8, 0 },
    { (char*)"reversePacketTotalCount",            8, 0 },
    { (char*)"reverseOctetTotalCount",             8, 0 },
    { (char*)"responderPackets",                   8, 0 },
    { (char*)"responderOctets",                    8, 0 },
    /* Microsecond start and end (RFC1305-style) (extended time) */
    { (char*)"flowStartMicroseconds",              8, 0 },
    { (char*)"flowEndMicroseconds",                8, 0 },
    /* SysUpTime, used to handle Netflow v9 SysUpTime offset times */
    { (char*)"systemInitTimeMilliseconds",         8, 0 },
    /* Second start and end (extended time) */
    { (char*)"flowStartSeconds",                   4, 0 },
    { (char*)"flowEndSeconds",                     4, 0 },
    /* Flow durations (extended time) */
    { (char*)"flowDurationMicroseconds",           4, 0 },
    { (char*)"flowDurationMilliseconds",           4, 0 },
    /* Microsecond delta start and end (extended time) */
    { (char*)"flowStartDeltaMicroseconds",         4, 0 },
    { (char*)"flowEndDeltaMicroseconds",           4, 0 },
    /* Initial packet roundtrip */
    { (char*)"reverseFlowDeltaMilliseconds",       4, 0 },
    /* SysUpTime-based fields */
    { (char*)"flowStartSysUpTime",                 4, 0 },
    { (char*)"flowEndSysUpTime",                   4, 0 },
    /* Reverse flags */
    { (char*)"reverseTcpControlBits",              1, 0 },
    { (char*)"reverseInitialTCPFlags",             1, 0 },
    { (char*)"reverseUnionTCPFlags",               1, 0 },
    /* End reason */
    { (char*)"flowEndReason",                      1, 0 },
    /* Flow attributes */
    { (char*)"flowAttributes",                     2, 0 },
    { (char*)"reverseFlowAttributes",              2, 0 },
    /* Vlan IDs */
    { (char*)"vlanId",                             2, 0 },
    { (char*)"postVlanId",                         2, 0 },
    { (char*)"reverseVlanId",                      2, 0 },
    { (char*)"reversePostVlanId",                  2, 0 },
    /* ASN */
    { (char*)"bgpSourceAsNumber",                  4, 0 },
    { (char*)"bgpDestinationAsNumber",             4, 0 },
    /* MPLS */
    { (char*)"mplsTopLabelIPv4Address",            4, 0 },
    { (char*)"mplsTopLabelStackSection",           3, 0 },
    { (char*)"mplsLabelStackSection2",             3, 0 },
    { (char*)"mplsLabelStackSection3",             3, 0 },
    { (char*)"mplsLabelStackSection4",             3, 0 },
    { (char*)"mplsLabelStackSection5",             3, 0 },
    { (char*)"mplsLabelStackSection6",             3, 0 },
    { (char*)"mplsTopLabelPrefixLength",           1, 0 },
    { (char*)"mplsTopLabelType",                   1, 0 },
    /* Firewall events */
    { (char*)"firewallEvent",                      1, 0 },
    { (char*)"NF_F_FW_EVENT",                      1, 0 },
    { (char*)"NF_F_FW_EXT_EVENT",                  2, 0 },
    /* TOS */
    { (char*)"ipClassOfService",                   1, 0 },
    { (char*)"reverseIpClassOfService",            1, 0 },
    /* MAC Addresses */
    { (char*)"sourceMacAddress",                   6, 0 },
    { (char*)"destinationMacAddress",              6, 0 },
    /* Flow Sampler ID */
    { (char*)"flowSamplerID",                      2, 0 },
    /* flow direction */
    { (char*)"flowDirection",                      1, 0 },
    /* pad record to 64-bit boundary. */
#if SKI_EXTRWREC_PADDING != 0
    { (char*)"paddingOctets", SKI_EXTRWREC_PADDING, 0 },
#endif
    { (char*)"subTemplateMultiList",               0, 0 },
    FB_IESPEC_NULL
};

/* Keep in sync with the ski_extrwrec_spec[] above. */
typedef struct ski_extrwrec_st {
    ski_rwrec_t     rw;                             /*   0-119 */

    uint64_t        packetTotalCount;               /* 120-127 */
    uint64_t        octetTotalCount;                /* 128-135 */
    uint64_t        initiatorPackets;               /* 136-143 */
    uint64_t        initiatorOctets;                /* 144-151 */

    uint64_t        reversePacketDeltaCount;        /* 152-159 */
    uint64_t        reverseOctetDeltaCount;         /* 160-167 */
    uint64_t        reversePacketTotalCount;        /* 168-175 */
    uint64_t        reverseOctetTotalCount;         /* 176-183 */
    uint64_t        responderPackets;               /* 184-191 */
    uint64_t        responderOctets;                /* 192-199 */

    /* Time can be represented in many different formats: */

    /* start time as NTP (RFC1305); may either have end Time in same
     * format or as an flowDurationMicroseconds value. */
    uint64_t        flowStartMicroseconds;          /* 200-207 */
    uint64_t        flowEndMicroseconds;            /* 208-215 */

    /* SysUpTime: used for flow{Start,End}SysUpTime calculations.
     * Needed to support Netflow v9 in particular. */
    uint64_t        systemInitTimeMilliseconds;     /* 216-223 */

    /* start time and end times as seconds since UNIX epoch. no
     * flowDuration field */
    uint32_t        flowStartSeconds;               /* 224-227 */
    uint32_t        flowEndSeconds;                 /* 228-231 */

    /* elapsed time as either microsec or millisec.  used when the
     * flowEnd time is not given. */
    uint32_t        flowDurationMicroseconds;       /* 232-235 */
    uint32_t        flowDurationMilliseconds;       /* 236-239 */

    /* start time as delta (negative microsec offsets) from the export
     * time; may either have end time in same format or a
     * flowDurationMicroseconds value */
    uint32_t        flowStartDeltaMicroseconds;     /* 240-243 */
    uint32_t        flowEndDeltaMicroseconds;       /* 244-247 */

    /* start time of reverse flow, as millisec offset from start time
     * of forward flow */
    uint32_t        reverseFlowDeltaMilliseconds;   /* 248-251 */

    /* Start and end time as delta from the system init time.  Needed
     * to support Netflow v9. */
    uint32_t        flowStartSysUpTime;             /* 252-255 */
    uint32_t        flowEndSysUpTime;               /* 256-259 */

    /* Flags for the reverse flow: */
    uint8_t         reverseTcpControlBits;          /* 260     */
    uint8_t         reverseInitialTCPFlags;         /* 261     */
    uint8_t         reverseUnionTCPFlags;           /* 262     */

    uint8_t         flowEndReason;                  /* 263     */

    /* Flow attribute flags */
    uint16_t        flowAttributes;                 /* 264-265 */
    uint16_t        reverseFlowAttributes;          /* 266-267 */

    /* vlan IDs */
    uint16_t        vlanId;                         /* 268-269 */
    uint16_t        postVlanId;                     /* 270-271 */
    uint16_t        reverseVlanId;                  /* 272-273 */
    uint16_t        reversePostVlanId;              /* 274-275 */

    /* ASN */
    uint32_t        bgpSourceAsNumber;              /* 276-279 */
    uint32_t        bgpDestinationAsNumber;         /* 280-283 */

    /* MPLS */
    uint32_t        mplsTopLabelIPv4Address;        /* 284-287 */
    uint8_t         mplsLabels[18];                 /* 288-305 */
    uint8_t         mplsTopLabelPrefixLength;       /* 306     */
    uint8_t         mplsTopLabelType;               /* 307     */

    /* Firewall events */
    uint8_t         firewallEvent;                  /* 308     */
    uint8_t         NF_F_FW_EVENT;                  /* 309     */
    uint16_t        NF_F_FW_EXT_EVENT;              /* 310-311 */

    /* TOS */
    uint8_t         ipClassOfService;               /* 312     */
    uint8_t         reverseIpClassOfService;        /* 313     */

    /* MAC Addresses */
    uint8_t         sourceMacAddress[6];            /* 314-319 */
    uint8_t         destinationMacAddress[6];       /* 320-325 */

    /* Flow Sampler ID */
    uint16_t        flowSamplerID;                  /* 326-327 */

    /* Flow Direction */
    uint8_t         flowDirection;                  /* 328 */

    /* padding */
#if SKI_EXTRWREC_PADDING != 0
    uint8_t         pad[SKI_EXTRWREC_PADDING];      /* 329-335 */
#endif

    /* TCP flags from yaf (when it is run without --silk) */
    fbSubTemplateMultiList_t stml;

} ski_extrwrec_t;

/* Support for reading TCP flags from an IPFIX subTemplateMultiList as
 * exported by YAF.  This has Template ID SKI_TCP_STML_TID.
 *
 * Keep this in sync with the ski_tcp_stml_t defined below. */
static fbInfoElementSpec_t ski_tcp_stml_spec[] = {
    { (char*)"initialTCPFlags",                    1, 0 },
    { (char*)"unionTCPFlags",                      1, 0 },
    { (char*)"reverseInitialTCPFlags",             1, 0 },
    { (char*)"reverseUnionTCPFlags",               1, 0 },
    FB_IESPEC_NULL
};

/* Keep in sync with the ski_tcp_stml_spec[] defined above. */
typedef struct ski_tcp_stml_st {
    uint8_t         initialTCPFlags;
    uint8_t         unionTCPFlags;
    uint8_t         reverseInitialTCPFlags;
    uint8_t         reverseUnionTCPFlags;
} ski_tcp_stml_t;


#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
/*
 *    Define the list of information elements and the corresponding
 *    struct for reading NetFlowV9 Options Template records that
 *    contains sampling information.  These records use internal
 *    template ID SKI_NF9_SAMPLING_TID.
 */
#define SKI_NF9_SAMPLING_PADDING 4
static fbInfoElementSpec_t ski_nf9_sampling_spec[] = {
    { (char*)"samplingInterval",          4, 0 },    /* 34 */
    { (char*)"flowSamplerRandomInterval", 4, 0 },    /* 50 */
    { (char*)"samplingAlgorithm",         1, 0 },    /* 35 */
    { (char*)"flowSamplerMode",           1, 0 },    /* 49 */
    { (char*)"flowSamplerID",             2, 0 },    /* 48 */
#if SKI_NF9_SAMPLING_PADDING != 0
    { (char*)"paddingOctets",             SKI_NF9_SAMPLING_PADDING, 0 },
#endif
    FB_IESPEC_NULL
};

typedef struct ski_nf9_sampling_st {
    uint32_t    samplingInterval;
    uint32_t    flowSamplerRandomInterval;
    uint8_t     samplingAlgorithm;
    uint8_t     flowSamplerMode;
    uint16_t    flowSamplerID;
#if SKI_NF9_SAMPLING_PADDING != 0
    uint8_t     paddingOctets[SKI_NF9_SAMPLING_PADDING];
#endif
} ski_nf9_sampling_t;
#endif  /* SK_HAVE_FBTEMPLATEGETCONTEXT */


/* This lists statistics values that yaf may export. */
/* Keep this in sync with ski_yaf_stats_t defined in skipfix.h */
static fbInfoElementSpec_t ski_yaf_stats_option_spec[] = {
    { (char*)"systemInitTimeMilliseconds",         8, 0 },
    { (char*)"exportedFlowRecordTotalCount",       8, 0 },
    { (char*)"packetTotalCount",                   8, 0 },
    { (char*)"droppedPacketTotalCount",            8, 0 },
    { (char*)"ignoredPacketTotalCount",            8, 0 },
    { (char*)"notSentPacketTotalCount",            8, 0 },
    { (char*)"expiredFragmentCount",               4, 0 },
#if 0
    { (char*)"assembledFragmentCount",             4, 0 },
    { (char*)"flowTableFlushEventCount",           4, 0 },
    { (char*)"flowTablePeakCount",                 4, 0 },
    { (char*)"meanFlowRate",                       4, 0 },
    { (char*)"meanPacketRate",                     4, 0 },
    { (char*)"exporterIPv4Address",                4, 0 },
#endif  /* 0 */
    { (char*)"exportingProcessId",                 4, 0 },
#if SKI_YAF_STATS_PADDING != 0
    { (char*)"paddingOctets", SKI_YAF_STATS_PADDING,  0 },
#endif
    FB_IESPEC_NULL
};

/* Values for the flowEndReason. this first set is defined by the
 * IPFIX spec */
#define SKI_END_IDLE            1
#define SKI_END_ACTIVE          2
#define SKI_END_CLOSED          3
#define SKI_END_FORCED          4
#define SKI_END_RESOURCE        5

/* SiLK will ignore flows with a flowEndReason of
 * SKI_END_YAF_INTERMEDIATE_FLOW */
#define SKI_END_YAF_INTERMEDIATE_FLOW 0x1F

/* Mask for the values of flowEndReason: want to ignore the next bit */
#define SKI_END_MASK            0x1f

/* Bits from flowEndReason: whether flow is a continuation */
#define SKI_END_ISCONT          0x80


/* Bits from flowAttributes */
#define SKI_FLOW_ATTRIBUTE_UNIFORM_PACKET_SIZE 0x01


/*
 *    The skiRwNextRecord() function takes different actions depending
 *    on whether the template (fbTemplate_t) for the record it is
 *    processing contains various information elements.
 *
 *    When using a version of libfixbuf prior to 1.4.0, these searches
 *    use fbTemplateContainsElementByName(), which does a linear
 *    search over the template---for every record processed.
 *
 *    When using fixbuf-1.4.0 or later, the template is examined by
 *    skiTemplateCallbackCtx() when it is initially received, reducing
 *    the overhead of examining it for every record (at the expenses
 *    of looking for elements which skiRwNextRecord() may never
 *    actually need).  Overall this should be a benefit as long as the
 *    number of records received is much higher than the number of
 *    templates received (in the TCP case, the templates are only sent
 *    once).
 *
 *    The 'elem' variable is the single instance of the 'elem_st'
 *    struct.  For each of the information elements of interest, it
 *    specifies a bit position and the fbInfoElementSpec_t needed to
 *    query fbTemplateContainsElementByName.
 *
 *    The TEMPLATE_SET_BIT() macro sets one bit on the bitmap.  The
 *    macro takes the bitmap and the NAME of a member of the 'elem_st'
 *    structure to set.
 *
 *    The TEMPLATE_GET_BIT() macro checks whether a bit is set.  It
 *    takes the bitmap and the NAME of a member of the 'elem_st'
 *    structure.
 *
 *    The TEMPLATE_CONTAINS_BY_NAME() macro takes a template and the
 *    NAME of one of the members of the 'elem_st' struct.  It returns
 *    1 if the template contains the information element; 0 otherwise.
 *
 *    The TEMPLATE_SET_IF_CONTAINS() macro checks whether the template
 *    contains an elements and, if does, sets the bit on the bitmap.
 *    The macro takes the bitmap, the template, and the NAME of a
 *    member of the 'elem_st' structure.
 *
 *    The TMPL_HAS_ELEM() macro defined in skiRwNextRecord() takes a
 *    single argument which is the NAME of a member of the 'elem_st'
 *    structure.  For fixbuf-1.4, TMPL_HAS_ELEM() uses a local bitmap
 *    variable and calls TEMPLATE_GET_BIT().  For older versions of
 *    fixbuf, TMPL_HAS_ELEM() uses a local fbTemplate_t variable and
 *    calls TEMPLATE_CONTAINS_BY_NAME().
 */
typedef struct bit_ies_st {
    uint8_t             bit;
    fbInfoElementSpec_t ies;
} bit_ies_t;

static const struct elem_st {
    bit_ies_t   NF_F_FW_EVENT;
    bit_ies_t   NF_F_FW_EXT_EVENT;
    bit_ies_t   firewallEvent;
    bit_ies_t   flowStartSysUpTime;
    bit_ies_t   mplsTopLabelStackSection;
    bit_ies_t   postVlanId;
    bit_ies_t   reverseInitialTCPFlags;
    bit_ies_t   reverseTcpControlBits;
    bit_ies_t   reverseVlanId;
    bit_ies_t   systemInitTimeMilliseconds;

    bit_ies_t   destinationIPv4Address;
    bit_ies_t   sourceIPv4Address;

    bit_ies_t   destinationIPv6Address;
    bit_ies_t   sourceIPv6Address;

    bit_ies_t   flowSamplerMode;
    bit_ies_t   flowSamplerRandomInterval;

    bit_ies_t   samplingAlgorithm;
    bit_ies_t   samplingInterval;
} elem = {
    { 1, {(char*)"NF_F_FW_EVENT",                0, 0}},
    { 2, {(char*)"NF_F_FW_EXT_EVENT",            0, 0}},
    { 3, {(char*)"firewallEvent",                0, 0}},
    { 4, {(char*)"flowStartSysUpTime",           0, 0}},
    { 5, {(char*)"mplsTopLabelStackSection",     0, 0}},
    { 6, {(char*)"postVlanId",                   0, 0}},
    { 7, {(char*)"reverseInitialTCPFlags",       0, 0}},
    { 8, {(char*)"reverseTcpControlBits",        0, 0}},
    { 9, {(char*)"reverseVlanId",                0, 0}},
    {10, {(char*)"systemInitTimeMilliseconds",   0, 0}},

    /* source and destination IPv4 map to same bitmap position */
    {11, {(char*)"destinationIPv4Address",       0, 0}},
    {11, {(char*)"sourceIPv4Address",            0, 0}},

    /* source and destination IPv6 map to same bitmap position */
    {12, {(char*)"destinationIPv6Address",       0, 0}},
    {12, {(char*)"sourceIPv6Address",            0, 0}},

    /* flowSampler* elements map to same bitmap position */
    {13, {(char*)"flowSamplerMode",              0, 0}},
    {13, {(char*)"flowSamplerRandomInterval",    0, 0}},

    /* sampling* elements map to same bitmap position */
    {14, {(char*)"samplingAlgorithm",            0, 0}},
    {14, {(char*)"samplingInterval",             0, 0}}
};

#define TEMPLATE_SET_BIT(tsb_bitmap, tsb_member)        \
    tsb_bitmap |= (1 << (elem. tsb_member .bit))

#define TEMPLATE_GET_BIT(tgb_bitmap, tgb_member)        \
    ((tgb_bitmap) & (1 << elem. tgb_member .bit))

#define TEMPLATE_CONTAINS_BY_NAME(tcbn_tmpl, tcbn_member)               \
    fbTemplateContainsElementByName(                                    \
        tcbn_tmpl, (fbInfoElementSpec_t*)&elem. tcbn_member .ies)

#define TEMPLATE_SET_IF_CONTAINS(tsb_bitmap, tsb_tmpl, tsb_member)      \
    tsb_bitmap |= (TEMPLATE_CONTAINS_BY_NAME(tsb_tmpl, tsb_member)      \
                   << (elem. tsb_member .bit))



/*
 *    There is a single infomation model.
 */
static fbInfoModel_t *ski_model = NULL;

/*
 *    When processing files with fixbuf, the session object
 *    (fbSession_t) is owned the reader/write buffer (fBuf_t).
 *
 *    When doing network processing, the fBuf_t does not own the
 *    session.  We use this global vector to maintain those session
 *    pointers so they can be freed at shutdown.
 */
static sk_vector_t *session_list = NULL;

#if SK_HAVE_FBTEMPLATEGETCONTEXT
/*
 *    If non-zero, print the templates when they arrive.  This can be
 *    set by defining the environment variable specified in
 *    SKI_ENV_PRINT_TEMPLATES.
 */
static int print_templates = 0;
#endif


/* FUNCTION DEFINITIONS */

/*
 *    Return a pointer to the single information model.  If necessary,
 *    create and initialize it.
 */
static fbInfoModel_t *
skiInfoModel(
    void)
{
    if (!ski_model) {
        ski_model = fbInfoModelAlloc();
        fbInfoModelAddElementArray(ski_model, ski_info_elements);
        fbInfoModelAddElementArray(ski_model, ski_std_info_elements);

#if SK_HAVE_FBTEMPLATEGETCONTEXT
        {
            const char *env;

            env = getenv(SKI_ENV_PRINT_TEMPLATES);
            if (env && *env && strcmp("0", env)) {
                print_templates = 1;
            }
        }
#endif  /* SK_HAVE_FBTEMPLATEGETCONTEXT */
    }

    return ski_model;
}

/*
 *    Free the single information model.
 */
static void
skiInfoModelFree(
    void)
{
    if (ski_model) {
        fbInfoModelFree(ski_model);
        ski_model = NULL;
    }
}


#if SK_HAVE_FBTEMPLATEGETCONTEXT
/*
 *  skiPrintTemplate(session, template, template_id);
 *
 *    Function to print the contents of the 'template'.  The 'session'
 *    is used to get the domain which is printed with the
 *    'template_id'.
 *
 *    This function is normally invoked by the template callback when
 *    the 'print_templates' variable is true, and that variable is
 *    normally enabled by the environment variable named in
 *    SKI_ENV_PRINT_TEMPLATES.
 *
 *    This function requires libfixbuf 1.4.0 or later.
 */
static void
skiPrintTemplate(
    fbSession_t        *session,
    fbTemplate_t       *tmpl,
    uint16_t            tid)
{
    fbInfoElement_t *ie;
    uint32_t i;
    uint32_t count;
    uint32_t domain;

    domain = fbSessionGetDomain(session);
    count = fbTemplateCountElements(tmpl);

    INFOMSG(("Domain 0x%04X, TemplateID 0x%04X,"
             " Contains %" PRIu32 " Elements, Enabled by %s"),
            domain, tid, count, SKI_ENV_PRINT_TEMPLATES);

    for (i = 0; i < count && (ie = fbTemplateGetIndexedIE(tmpl, i)); ++i) {
        if (0 == ie->ent) {
            INFOMSG(("Domain 0x%04X, TemplateID 0x%04X, Position %3u,"
                     " Length %5u, IE %11u, Name %s"),
                    domain, tid, i, ie->len, ie->num, ie->ref.canon->ref.name);
        } else {
            INFOMSG(("Domain 0x%04X, TemplateID 0x%04X, Position %3u,"
                     " Length %5u, IE %5u/%5u, Name %s"),
                    domain, tid, i, ie->len, ie->ent, ie->num, ie->ref.canon->ref.name);
        }
    }
}
#endif  /* SK_HAVE_FBTEMPLATEGETCONTEXT */


/*
 *    The skiTemplateCallback() or skiTemplateCallbackCtx() callback
 *    is invoked whenever the session receives a new template.  The
 *    purpose of the callback is the tell fixbuf how to process items
 *    in a subTemplateMultiList.
 *
 *    We tell fixbuf to map from the two templates that yaf uses for
 *    TCP flags (one of which has reverse elements and one of which
 *    does not) to the struct used in this file.
 *
 *    For fixbuf-1.4.0, the skiTemplateCallbackCtx() also examines the
 *    template and sets a context pointer that contains high bits for
 *    certain information elements.  See the detailed comment above.
 */
#ifndef SK_HAVE_FBTEMPLATEGETCONTEXT
/*
 *     This function must have the signature defined by the
 *     'fbNewTemplateCallback_fn' typedef.  For versions of libfixbuf
 *     prior to 1.4.0.
 */
static void
skiTemplateCallback(
    fbSession_t            *session,
    uint16_t                tid,
    fbTemplate_t    UNUSED(*tmpl))
{
    TRACEMSG(("Template callback called for Template ID 0x%04X [%p]",
              tid, tmpl));

    if (SKI_YAF_TCP_FLOW_TID == (tid & ~SKI_YAF_REVERSE_BIT)) {
        fbSessionAddTemplatePair(session, tid, SKI_TCP_STML_TID);
    } else {
        /* ignore */
        fbSessionAddTemplatePair(session, tid, 0);
    }
}

#else /* SK_HAVE_FBTEMPLATEGETCONTEXT */

/*
 *     This function must have the signature defined by the
 *     'fbTemplateCtxCallback_fn' typedef.  For fixbuf-1.4.0 and
 *     later.
 */
static void
skiTemplateCallbackCtx(
    fbSession_t            *session,
    uint16_t                tid,
    fbTemplate_t           *tmpl,
    void                  **ctx,
    fbTemplateCtxFree_fn   *fn)
{
    uintptr_t bmap;

    TRACEMSG(("Template callback called for Template ID 0x%04X [%p]",
              tid, tmpl));

    if (SKI_YAF_TCP_FLOW_TID == (tid & ~SKI_YAF_REVERSE_BIT)) {
        fbSessionAddTemplatePair(session, tid, SKI_TCP_STML_TID);
        *ctx = NULL;
        *fn = NULL;

        if (print_templates) {
            skiPrintTemplate(session, tmpl, tid);
        }
    } else if (fbTemplateGetOptionsScope(tmpl)) {
        /* do not define any template pairs for this template */
        fbSessionAddTemplatePair(session, tid, 0);

        /* assume if it has the template ID used by the yaf stats
         * packet than that is what it is.  if not, check for
         * NetFlowV9 sampling values */
        if (tid == SKI_YAF_STATS_TID) {
            *ctx = NULL;
            *fn = NULL;
        } else if (TEMPLATE_CONTAINS_BY_NAME(tmpl, flowSamplerRandomInterval)
                   && TEMPLATE_CONTAINS_BY_NAME(tmpl, flowSamplerMode))
        {
            bmap = 1;
            TEMPLATE_SET_BIT(bmap, flowSamplerMode);
            *ctx = (void*)bmap;
            *fn = NULL;
            TRACEMSG(("Bitmap value for Template ID 0x%04X [%p] was set to %lx",
                      tid, tmpl, (unsigned long)bmap));
        } else if (TEMPLATE_CONTAINS_BY_NAME(tmpl, samplingInterval)
                   && TEMPLATE_CONTAINS_BY_NAME(tmpl, samplingAlgorithm))
        {
            bmap = 1;
            TEMPLATE_SET_BIT(bmap, samplingAlgorithm);
            *ctx = (void*)bmap;
            *fn = NULL;
            TRACEMSG(("Bitmap value for Template ID 0x%04X [%p] was set to %lx",
                      tid, tmpl, (unsigned long)bmap));
        } else {
            *ctx = NULL;
            *fn = NULL;
        }
    } else {
        /* do not define any template pairs for this template */
        fbSessionAddTemplatePair(session, tid, 0);

        /* fill 'bmap' based on the elements in the template */
        bmap = 1;
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, NF_F_FW_EVENT);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, NF_F_FW_EXT_EVENT);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, firewallEvent);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, flowStartSysUpTime);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, mplsTopLabelStackSection);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, postVlanId);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, reverseInitialTCPFlags);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, reverseTcpControlBits);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, reverseVlanId);
        TEMPLATE_SET_IF_CONTAINS(bmap, tmpl, systemInitTimeMilliseconds);

        /* special handling for IPs: Short circuit once one is
         * found; note that source and dest map to same  bit */
        if (TEMPLATE_CONTAINS_BY_NAME(tmpl, sourceIPv4Address)) {
            TEMPLATE_SET_BIT(bmap, sourceIPv4Address);
        } else if (TEMPLATE_CONTAINS_BY_NAME(tmpl, sourceIPv6Address)) {
            TEMPLATE_SET_BIT(bmap, sourceIPv6Address);
        } else if (TEMPLATE_CONTAINS_BY_NAME(tmpl, destinationIPv4Address)) {
            TEMPLATE_SET_BIT(bmap, destinationIPv4Address);
        } else if (TEMPLATE_CONTAINS_BY_NAME(tmpl, destinationIPv6Address)) {
            TEMPLATE_SET_BIT(bmap, destinationIPv6Address);
        }

        *ctx = (void*)bmap;
        *fn = NULL;
        TRACEMSG(("Bitmap value for Template ID 0x%04X [%p] was set to %lx",
                  tid, tmpl, (unsigned long)bmap));

        if (print_templates) {
            skiPrintTemplate(session, tmpl, tid);
        }
    }
}
#endif  /* #else of #ifndef SK_HAVE_FBTEMPLATEGETCONTEXT */


void
skiAddSessionCallback(
    fbSession_t        *session)
{
#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
    fbSessionAddTemplateCtxCallback(session, &skiTemplateCallbackCtx);
#else
    fbSessionAddTemplateCallback(session, &skiTemplateCallback);
#endif
}


static void
skiSessionsFree(
    void)
{
    size_t i;
    fbSession_t *session;

    if (session_list) {
        for (i = 0; i < skVectorGetCount(session_list); i++) {
            skVectorGetValue(&session, session_list, i);
            fbSessionFree(session);
        }
        skVectorDestroy(session_list);
        session_list = NULL;
    }
}


/*
 *    Initialize an fbSession object that reads from either the
 *    network or from a file.
 */
static int
skiSessionInitReader(
    fbSession_t        *session,
    GError            **err)
{
    fbInfoModel_t   *model = skiInfoModel();
    fbTemplate_t    *tmpl = NULL;

    /* Add the full record template */
    tmpl = fbTemplateAlloc(model);
    if (!fbTemplateAppendSpecArray(tmpl, ski_rwrec_spec, 0, err)) {
        goto ERROR;
    }
    if (!fbSessionAddTemplate(session, TRUE, SKI_RWREC_TID, tmpl, err)) {
        goto ERROR;
    }

    /* Add the extended record template */
    tmpl = fbTemplateAlloc(model);
    if (!fbTemplateAppendSpecArray(tmpl, ski_rwrec_spec, 0, err)) {
        goto ERROR;
    }
    if (!fbTemplateAppendSpecArray(tmpl, ski_extrwrec_spec, 0, err)) {
        goto ERROR;
    }
    if (!fbSessionAddTemplate(session, TRUE, SKI_EXTRWREC_TID, tmpl, err)) {
        goto ERROR;
    }

    /* Add the TCP record template */
    tmpl = fbTemplateAlloc(model);
    if (!fbTemplateAppendSpecArray(tmpl, ski_tcp_stml_spec, 0, err)) {
        goto ERROR;
    }
    if (!fbSessionAddTemplate(session, TRUE, SKI_TCP_STML_TID, tmpl, err)) {
        goto ERROR;
    }

    /* Add the yaf stats record template  */
    tmpl = fbTemplateAlloc(model);
    if (!fbTemplateAppendSpecArray(tmpl, ski_yaf_stats_option_spec, 0, err)) {
        goto ERROR;
    }
    if (!fbSessionAddTemplate(session, TRUE, SKI_YAF_STATS_TID, tmpl, err)) {
        goto ERROR;
    }

#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
    /* Add the netflow v9 sampling template  */
    tmpl = fbTemplateAlloc(model);
    if (!fbTemplateAppendSpecArray(tmpl, ski_nf9_sampling_spec, 0, err)) {
        goto ERROR;
    }
    if (!fbSessionAddTemplate(session, TRUE, SKI_NF9_SAMPLING_TID, tmpl, err)){
        goto ERROR;
    }
#endif  /* SK_HAVE_FBTEMPLATEGETCONTEXT */

    return 1;

  ERROR:
    fbTemplateFreeUnused(tmpl);
    return 0;
}


void
skiTeardown(
    void)
{
    skiInfoModelFree();
    skiSessionsFree();
}


/* **************************************************************
 * *****  Support for reading/import
 */


fbListener_t *
skiCreateListener(
    fbConnSpec_t           *spec,
    fbListenerAppInit_fn    appinit,
    fbListenerAppFree_fn    appfree,
    GError                **err)
{
    fbInfoModel_t   *model;
    fbSession_t     *session = NULL;

    /* The session is not owned by the buffer or the listener, so
     * maintain a vector of them for later destruction. */
    if (!session_list) {
        session_list = skVectorNew(sizeof(fbSession_t *));
        if (session_list == NULL) {
            return NULL;
        }
    }
    model = skiInfoModel();
    if (model == NULL) {
        return NULL;
    }
    session = fbSessionAlloc(model);
    if (session == NULL) {
        return NULL;
    }
    /* Initialize session for reading */
    if (!skiSessionInitReader(session, err)) {
        fbSessionFree(session);
        return NULL;
    }
    if (skVectorAppendValue(session_list, &session) != 0) {
        fbSessionFree(session);
        return NULL;
    }

    /* Invoke a callback when a new template arrives that tells fixbuf
     * how to map from the subTemplateMultiList used by YAF for TCP
     * information to our internal structure. */
    skiAddSessionCallback(session);

    /* Allocate a listener */
    return fbListenerAlloc(spec, session, appinit, appfree, err);
}


fBuf_t *
skiCreateReadBufferForFP(
    FILE               *fp,
    GError            **err)
{
    fbInfoModel_t  *model = NULL;
    fbCollector_t  *collector = NULL;
    fbSession_t    *session = NULL;
    fBuf_t         *fbuf = NULL;

    model = skiInfoModel();
    if (NULL == model) {
        return NULL;
    }

    collector = fbCollectorAllocFP(NULL, fp);
    if (NULL == collector) {
        return NULL;
    }

    /* Allocate a session.  The session will be owned by the fbuf, so
     * don't save it for later freeing. */
    session = fbSessionAlloc(model);
    if (session == NULL) {
        return NULL;
    }
    /* Initialize session for reading */
    if (!skiSessionInitReader(session, err)) {
        fbSessionFree(session);
        return NULL;
    }

    /* Create a buffer with the session and the collector */
    fbuf = fBufAllocForCollection(session, collector);
    if (NULL == fbuf) {
        fbSessionFree(session);
        return NULL;
    }

    /* Make certain the fbuf has an internal template */
    if (!fBufSetInternalTemplate(fbuf, SKI_RWREC_TID, err)) {
        fBufFree(fbuf);
        return NULL;
    }

    /* Invoke a callback when a new template arrives that tells fixbuf
     * how to map from the subTemplateMultiList used by YAF for TCP
     * information to our internal structure. */
    skiAddSessionCallback(session);

    return fbuf;
}


/* Convert an NTP timestamp (RFC1305) to epoch millisecond */
static uint64_t
skiNTPDecode(
    uint64_t            ntp)
{
    double          dntp;
    uint64_t        millis;

    if (!ntp) {
        return 0;
    }

    dntp = (ntp & UINT64_C(0xFFFFFFFF00000000)) >> 32;
    dntp += ((ntp & UINT64_C(0x00000000FFFFFFFF)) * 1.0) / (UINT64_C(2) << 32);
    millis = (uint64_t)(dntp * 1000);
    return millis;
}


/* Print a message saying why a flow was ignored */
static void
skiFlowIgnored(
    const ski_extrwrec_t   *fixrec,
    const char             *reason)
{
    char sipbuf[64];
    char dipbuf[64];

    if (!SK_IPV6_IS_ZERO(fixrec->rw.sourceIPv6Address)) {
#ifdef SK_HAVE_INET_NTOP
        if (!inet_ntop(AF_INET6, &fixrec->rw.sourceIPv6Address,
                       sipbuf, sizeof(sipbuf)))
#endif
        {
            strcpy(sipbuf, "unknown-v6");
        }
    } else {
        num2dot_r(fixrec->rw.sourceIPv4Address, sipbuf);
    }
    if (!SK_IPV6_IS_ZERO(fixrec->rw.destinationIPv6Address)) {
#ifdef SK_HAVE_INET_NTOP
        if (!inet_ntop(AF_INET6, &fixrec->rw.destinationIPv6Address,
                       dipbuf, sizeof(dipbuf)))
#endif
        {
            strcpy(dipbuf, "unknown-v6");
        }
    } else {
        num2dot_r(fixrec->rw.destinationIPv4Address, dipbuf);
    }

    INFOMSG(("IGNORED|%s|%s|%u|%u|%u|%" PRIu64 "|%" PRIu64 "|%s|"),
            sipbuf, dipbuf, fixrec->rw.sourceTransportPort,
            fixrec->rw.destinationTransportPort,fixrec->rw.protocolIdentifier,
            ((fixrec->rw.packetDeltaCount)
             ? fixrec->rw.packetDeltaCount
             : ((fixrec->packetTotalCount)
                ? fixrec->packetTotalCount
                : fixrec->initiatorPackets)),
            ((fixrec->rw.octetDeltaCount)
             ? fixrec->rw.octetDeltaCount
             : ((fixrec->octetTotalCount)
                ? fixrec->octetTotalCount
                : fixrec->initiatorOctets)),
            reason);
}


/* get the type of the next record */
ski_rectype_t
skiGetNextRecordType(
    fBuf_t             *fbuf,
    GError            **err)
{
    fbTemplate_t *tmpl;
    uint16_t tid;

    tmpl = fBufNextCollectionTemplate(fbuf, &tid, err);
    if (tmpl == NULL) {
        return SKI_RECTYPE_ERROR;
    }

    /* Handle Records that use an Options Template */
    if (fbTemplateGetOptionsScope(tmpl)) {
        if (tid == SKI_YAF_STATS_TID) {
            return SKI_RECTYPE_STATS;
        }
#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
        {
            uintptr_t bmap;
            bmap = (uintptr_t)fbTemplateGetContext(tmpl);
            if (bmap & ((1 << elem.samplingAlgorithm.bit)
                        | (1 << elem.flowSamplerMode.bit)))
            {
                return SKI_RECTYPE_NF9_SAMPLING;
            }
        }
#endif
        return SKI_RECTYPE_UNKNOWN;
    }
    return SKI_RECTYPE_FLOW;
}

gboolean
skiYafNextStats(
    fBuf_t                     *fbuf,
    const skpc_probe_t  UNUSED(*probe),
    ski_yaf_stats_t            *stats,
    GError                    **err)
{
    size_t len;

    /* Set internal template to read an yaf stats record */
    if (!fBufSetInternalTemplate(fbuf, SKI_YAF_STATS_TID, err)) {
        return FALSE;
    }

    memset(stats, 0, sizeof(*stats));
    len = sizeof(*stats);

    if (!fBufNext(fbuf, (uint8_t *)stats, &len, err)) {
        return FALSE;
    }

    return TRUE;
}

#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
gboolean
skiNextSamplingOptionsTemplate(
    fBuf_t                 *fbuf,
    const skpc_probe_t     *probe,
    GError                **err)
{
    fbTemplate_t *tmpl = NULL;
    ski_nf9_sampling_t rec;
    uintptr_t bmap;
    size_t len;

    /* Set internal template to read the options record */
    if (!fBufSetInternalTemplate(fbuf, SKI_NF9_SAMPLING_TID, err)) {
        return FALSE;
    }

    memset(&rec, 0, sizeof(rec));
    len = sizeof(rec);

    if (!fBufNext(fbuf, (uint8_t*)&rec, &len, err)) {
        return FALSE;
    }

    if (skpcProbeGetLogFlags(probe) & SOURCE_LOG_SAMPLING) {
        /* Get the template used for the last record */
        tmpl = fBufGetCollectionTemplate(fbuf, NULL);
        bmap = (uintptr_t)fbTemplateGetContext(tmpl);
        if (TEMPLATE_GET_BIT(bmap, samplingAlgorithm)) {
            INFOMSG("'%s': Sampling Algorithm %u; Sampling Interval %u",
                    skpcProbeGetName(probe), rec.samplingAlgorithm,
                    rec.samplingInterval);
        } else if (TEMPLATE_GET_BIT(bmap, flowSamplerMode)) {
            INFOMSG(("'%s': Flow Sampler Id %u; Flow Sampler Mode %u;"
                     " Flow Sampler Random Interval %u"),
                    skpcProbeGetName(probe), rec.flowSamplerID,
                    rec.flowSamplerMode, rec.flowSamplerRandomInterval);
        }
    }
    return TRUE;
}
#endif  /* SK_HAVE_FBTEMPLATEGETCONTEXT */

/* Get a record from the libfixbuf buffer 'fbuf' and fill in the
 * forward and reverse flow records, 'rec' and 'revRec'.  Return 1 if
 * the record is uni-flow, 2 if it is bi-flow, 0 if the record is to
 * be ignored, -1 if there is an error. */
int
skiRwNextRecord(
    fBuf_t                 *fbuf,
    const skpc_probe_t     *probe,
    skIPFIXSourceRecord_t  *forward_rec,
    skIPFIXSourceRecord_t  *reverse_rec,
    GError                **err)
{
    fbTemplate_t *tmpl = NULL;
    fbSubTemplateMultiListEntry_t *stml;
    ski_extrwrec_t fixrec;
    size_t len;
    uint16_t tid;
    uint64_t sTime, eTime;
    uint64_t pkts, bytes;
    uint64_t rev_pkts, rev_bytes;
    uint8_t tcp_state;
    uint8_t tcp_flags;
    int have_tcp_stml = 0;
    rwRec *rec;
#if SK_ENABLE_IPV6
    /* set to non-zero if no IPv4 addresses exist in the IPFIX
     * template for this record */
    int no_ipv4 = 0;
#endif
#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
    uintptr_t bmap;
#define TMPL_HAS_ELEM(the_member)               \
    TEMPLATE_GET_BIT(bmap, the_member)
#else
#define TMPL_HAS_ELEM(the_member)               \
    TEMPLATE_CONTAINS_BY_NAME(tmpl, the_member)
#endif

    assert(forward_rec);

    rec = skIPFIXSourceRecordGetRwrec(forward_rec);

    /* Clear out the record */
    RWREC_CLEAR(rec);

    /* Set internal template to read an extended flow record */
    if (!fBufSetInternalTemplate(fbuf, SKI_EXTRWREC_TID, err)) {
        return -1;
    }

    /* Get the next record */
    len = sizeof(fixrec);
    if (!fBufNext(fbuf, (uint8_t *)&fixrec, &len, err)) {
        return -1;
    }

    if ((fixrec.flowEndReason & SKI_END_MASK) == SKI_END_YAF_INTERMEDIATE_FLOW)
    {
        TRACEMSG(("Ignored YAF intermediate uniflow"));
        return 0;
    }

    /* Get the template used for the last record */
    tmpl = fBufGetCollectionTemplate(fbuf, &tid);

#ifdef SK_HAVE_FBTEMPLATEGETCONTEXT
    bmap = (uintptr_t)fbTemplateGetContext(tmpl);
    TRACEMSG(("Bitmap value for Template ID 0x%04X [%p] was read as %lx",
              tid, tmpl, (unsigned long)bmap));
#endif

    /* Ignore records with no IPs.  Ignore records that do not have
     * IPv4 addresses when SiLK was built without IPv6 support.  If we
     * are not using the template context, we need to check the
     * source and destination addresses. */
    if (TMPL_HAS_ELEM(sourceIPv4Address)) {
        /* we're good */
    } else if (TMPL_HAS_ELEM(sourceIPv6Address)) {
#if SK_ENABLE_IPV6
        /* we're good; skip the IPv4 check below */
        no_ipv4 = 1;
#else
        skiFlowIgnored(&fixrec, "IPv6 record");
        return 0;
#endif
#ifndef SK_HAVE_FBTEMPLATEGETCONTEXT
    } else if (TMPL_HAS_ELEM(destinationIPv4Address)) {
        /* we're good */
    } else if (TMPL_HAS_ELEM(destinationIPv6Address)) {
#if SK_ENABLE_IPV6
        /* we're good; skip the IPv4 check below */
        no_ipv4 = 1;
#else
        skiFlowIgnored(&fixrec, "IPv6 record");
        return 0;
#endif
#endif  /* #ifndef SK_HAVE_FBTEMPLATEGETCONTEXT */
    } else if ((skpcProbeGetQuirks(probe) & SKPC_QUIRK_MISSING_IPS) == 0) {
        skiFlowIgnored(&fixrec, "No IP addresses");
        return 0;
    }

    /*
     *  Handle records that represent a "firewall event" when the
     *  SKPC_QUIRK_FW_EVENT quirks value is set on the probe.  When
     *  the quirk is not set, process the records normally.
     *
     *  This code changed in SiLK 3.8.0.  Prior to SiLK 3.8.0, all
     *  firewall event status messages were dropped.
     *
     *  It seems that every record from a Cisco ASA has NF_F_FW_EVENT
     *  and NF_F_FW_EXT_EVENT information elements, so ignoring flow
     *  records with these elements means ignoring all flow records.
     *
     *  firewallEvent is an official IPFIX information element, IE 233
     *
     *  NF_F_FW_EVENT is Cisco IE 40005
     *
     *  NF_F_FW_EXT_EVENT is Cisco IE 33002.
     *
     *  Note that the Cisco IE numbers cannot be used in IPFIX because
     *  IPFIX would treat them as "reverse" records.
     *
     *  References (October 2013):
     *  http://www.cisco.com/en/US/docs/security/asa/asa82/netflow/netflow.html#wp1028202
     *  http://www.cisco.com/en/US/docs/security/asa/asa84/system/netflow/netflow.pdf
     *
     *  Values for the NF_F_FW_EXT_EVENT depend on the values for the
     *  NF_F_FW_EVENT.  The following lists the FW_EVENT with
     *  sub-bullets for the NF_F_FW_EXT_EVENT.
     *
     *  0.  Ignore -- This value indicates that a field must be
     *      ignored.
     *
     *      0.  Ignore -- This value indicates that the field must be
     *          ignored.
     *
     *  1.  Flow created -- This value indicates that a new flow was
     *      created.
     *
     *  2.  Flow deleted -- This value indicates that a flow was
     *      deleted.
     *
     *    >2000.  Values above 2000 represent various reasons why a
     *            flow was terminated.
     *
     *  3.  Flow denied -- This value indicates that a flow was
     *      denied.
     *
     *    >1000.  Values above 1000 represent various reasons why a
     *            flow was denied.
     *
     *     1001.  A flow was denied by an ingress ACL.
     *
     *     1002.  A flow was denied by an egress ACL.
     *
     *     1003.  The ASA denied an attempt to connect to the (ASA's)
     *            interface service.
     *
     *     1004.  The flow was denied because the first packet on the
     *            TCP was not a TCP SYN packet.
     *
     *  5.  Flow updated -- This value indicates that a flow update
     *      timer went off or a flow was torn down.
     *
     *  The IPFIX values for the firewallEvent IE follow those for
     *  NF_F_FW_EVENT (with IPFIX providing no explanation as to what
     *  the values mean! --- some standard) and IPFIX adds the value:
     *
     *  4.  Flow alert.
     *
     *  PROCESSING RULES:
     *
     *  The term "ignore" below means that a log message is written
     *  and that no SiLK flow record is created.
     *
     *  Ignore flow records where the "flow ignore" event is present.
     *
     *  Treat records where "flow deleted" is specified as actual flow
     *  records to be processed and stored.
     *
     *  Ignore "flow created" events, since we will handle these flows
     *  when the "flow deleted" event occurs.  Also, a short-lived
     *  flow record may produce a "flow deleted" event without a "flow
     *  created" event.
     *
     *  For a "flow denied" event, write a special value into the SiLK
     *  Flow record that the writing thread can use to categorize the
     *  record as innull/outnull.
     *
     *  It is unclear how to handle "flow updated" events. If the
     *  record is only being updated, presumably SiLK will get a "flow
     *  deleted" event in the future.  However, if the flow is being
     *  torn down, will the ASA send a separate "flow deleted" event?
     *  For now (as of SiLK 3.8.0), ignore "flow updated" events.
     *
     *  Ignore "flow alert" events.
     *
     *
     *  Firewall events, byte and packet counts, and the Cisco ASA:
     *
     *  1.  Flow created events have a byte and packet count of 0;
     *  this is fine since we are ignoring these flows.
     *
     *  2.  Flow deinied events have a byte and packet count of 0.
     *  SiLK will ignore these flows unless we doctor them to have a
     *  non-zero byte and packet count, which we do when the ASA hack
     *  is enabled.
     *
     *  3.  Flow deleted events have a packet count of 0, but we have
     *  code below to work around that when the ASA hack is enabled.
     *  The flows usally have a non-zero byte count.  HOWEVER, some
     *  flow records have a 0-byte count, and it is unclear what to
     *  with those---currently they are ignored.
     */
    if ((skpcProbeGetQuirks(probe) & SKPC_QUIRK_FW_EVENT)
        && (TMPL_HAS_ELEM(firewallEvent)
            || TMPL_HAS_ELEM(NF_F_FW_EVENT)
            || TMPL_HAS_ELEM(NF_F_FW_EXT_EVENT)))
    {
        char msg[64];
        uint8_t event = (fixrec.firewallEvent
                         ? fixrec.firewallEvent : fixrec.NF_F_FW_EVENT);
        if (SKIPFIX_FW_EVENT_DELETED == event) {
            /* flow deleted */
            TRACEMSG((("Processing flow deleted event as actual flow record;"
                       " firewallEvent=%u, NF_F_FW_EVENT=%u,"
                       " NF_F_FW_EXT_EVENT=%u"),
                      fixrec.firewallEvent, fixrec.NF_F_FW_EVENT,
                      fixrec.NF_F_FW_EXT_EVENT));
        } else if (SKIPFIX_FW_EVENT_DENIED == event) {
            /* flow denied */
            TRACEMSG((("Processing flow denied event as actual flow record;"
                       " firewallEvent=%u, NF_F_FW_EVENT=%u,"
                       " NF_F_FW_EXT_EVENT=%u"),
                      fixrec.firewallEvent, fixrec.NF_F_FW_EVENT,
                      fixrec.NF_F_FW_EXT_EVENT));
            if (SKIPFIX_FW_EVENT_DENIED_CHECK_VALID(fixrec.NF_F_FW_EXT_EVENT)){
                rwRecSetMemo(rec, fixrec.NF_F_FW_EXT_EVENT);
            } else {
                rwRecSetMemo(rec, event);
            }
        } else {
            /* flow created, flow updated, flow alert, or something
             * unexpected */
            if (skpcProbeGetLogFlags(probe) & SOURCE_LOG_FIREWALL) {
                snprintf(msg, sizeof(msg), "firewallEvent=%u,extended=%u",
                         event, fixrec.NF_F_FW_EXT_EVENT);
                skiFlowIgnored(&fixrec, msg);
            }
            return 0;
        }
    }

    /* FIXME.  What if the record has a flowDirection field that is
     * set to egress (0x01)?  Shouldn't we handle that by reversing
     * the record?  Or has fixbuf done that for us? */

    /* Get the forward and reverse packet and byte counts (run the
     * Gauntlet of Volume). */
    pkts = ((fixrec.rw.packetDeltaCount)
            ? fixrec.rw.packetDeltaCount
            : ((fixrec.packetTotalCount)
               ? fixrec.packetTotalCount
               : fixrec.initiatorPackets));
    bytes = ((fixrec.rw.octetDeltaCount)
             ? fixrec.rw.octetDeltaCount
             : ((fixrec.octetTotalCount)
                ? fixrec.octetTotalCount
                : fixrec.initiatorOctets));

    rev_pkts = ((fixrec.reversePacketDeltaCount)
                ? fixrec.reversePacketDeltaCount
                : ((fixrec.reversePacketTotalCount)
                   ? fixrec.reversePacketTotalCount
                   : fixrec.responderPackets));
    rev_bytes = ((fixrec.reverseOctetDeltaCount)
                 ? fixrec.reverseOctetDeltaCount
                 : ((fixrec.reverseOctetTotalCount)
                    ? fixrec.reverseOctetTotalCount
                    : fixrec.responderOctets));

    if (0 == bytes && 0 == rev_bytes) {
        /* flow denied events from the Cisco ASA have zero in the
         * bytes and packets field */
        if ((skpcProbeGetQuirks(probe) & SKPC_QUIRK_FW_EVENT)
            && 0 == pkts
            && SKIPFIX_FW_EVENT_DENIED == fixrec.NF_F_FW_EVENT)
        {
            TRACEMSG(("Setting forward bytes and packets to 1"
                      " for denied firewall event"));
            bytes = 1;
            pkts = 1;
        } else {
            skiFlowIgnored(&fixrec, "no forward/reverse octets");
            return 0;
        }
    }

    if (0 == pkts && 0 == rev_pkts) {
        if ((skpcProbeGetQuirks(probe) & SKPC_QUIRK_ZERO_PACKETS) == 0) {
            /* Ignore records with no volume. */
            skiFlowIgnored(&fixrec, "no forward/reverse packets");
            return 0;
        }

        /* attempt to handle NetFlowV9 records from an ASA router that
         * have no packet count.  The code assumes all records from an
         * ASA have a byte count, though this is not always true. */
        if (bytes) {
            /* there is a forward byte count */
            if (0 == pkts) {
                TRACEMSG(("Setting forward packets to 1"));
                pkts = 1;
            }
        }
        if (rev_bytes) {
            /* there is a reverse byte count */
            if (0 == rev_pkts) {
                TRACEMSG(("Setting reverse packets to 1"));
                rev_pkts = 1;
            }
        }
    }

    /* If the TCP flags are in a subTemplateMultiList, copy them from
     * the list and into the record.  The fixbuf.stml gets initialized
     * by the call to fBufNext().*/
    stml = NULL;
    while ((stml = fbSubTemplateMultiListGetNextEntry(&fixrec.stml, stml))) {
        if (SKI_TCP_STML_TID != stml->tmplID) {
            fbSubTemplateMultiListEntryNextDataPtr(stml, NULL);
        } else {
            ski_tcp_stml_t *tcp = NULL;
            tcp = ((ski_tcp_stml_t*)
                   fbSubTemplateMultiListEntryNextDataPtr(stml, tcp));
            fixrec.rw.initialTCPFlags = tcp->initialTCPFlags;
            fixrec.rw.unionTCPFlags = tcp->unionTCPFlags;
            fixrec.reverseInitialTCPFlags = tcp->reverseInitialTCPFlags;
            fixrec.reverseUnionTCPFlags = tcp->reverseUnionTCPFlags;
            have_tcp_stml = 1;
        }
    }
    fbSubTemplateMultiListClear(&fixrec.stml);

    if (pkts && bytes) {
        /* We have forward information. */
        TRACEMSG(("Read a forward record"));

        /* Handle the IP addresses */
#if SK_ENABLE_IPV6
        if (no_ipv4
            || !SK_IPV6_IS_ZERO(fixrec.rw.sourceIPv6Address)
            || !SK_IPV6_IS_ZERO(fixrec.rw.destinationIPv6Address))
        {
            /* Values found in IPv6 addresses--use them */
            rwRecSetIPv6(rec);
            rwRecMemSetSIPv6(rec, &fixrec.rw.sourceIPv6Address);
            rwRecMemSetDIPv6(rec, &fixrec.rw.destinationIPv6Address);
            rwRecMemSetNhIPv6(rec, &fixrec.rw.ipNextHopIPv6Address);
        } else
#endif /* SK_ENABLE_IPV6 */
        {
            /* Take values from IPv4 */
            rwRecSetSIPv4(rec, fixrec.rw.sourceIPv4Address);
            rwRecSetDIPv4(rec, fixrec.rw.destinationIPv4Address);
            rwRecSetNhIPv4(rec, fixrec.rw.ipNextHopIPv4Address);
        }

        /* Handle the Protocol and Ports */
        rwRecSetProto(rec, fixrec.rw.protocolIdentifier);
        rwRecSetSPort(rec, fixrec.rw.sourceTransportPort);
        rwRecSetDPort(rec, fixrec.rw.destinationTransportPort);

        /* Handle the SNMP or VLAN interfaces */
        if (SKPC_IFVALUE_VLAN == skpcProbeGetInterfaceValueType(probe)) {
            rwRecSetInput(rec, fixrec.vlanId);
            rwRecSetOutput(rec, fixrec.postVlanId);
        } else {
            rwRecSetInput(rec,
                          CLAMP_VAL(fixrec.rw.ingressInterface, UINT16_MAX));
            rwRecSetOutput(rec,
                           CLAMP_VAL(fixrec.rw.egressInterface, UINT16_MAX));
        }


        /* Store volume, clamping counts to 32 bits. */
        rwRecSetPkts(rec, CLAMP_VAL(pkts, UINT32_MAX));
        rwRecSetBytes(rec, CLAMP_VAL(bytes, UINT32_MAX));
    } else {
        if (0 == rev_pkts || 0 == rev_bytes) {
            skAbort();
        }

        /* We have no forward information, only reverse.  Write the
         * source and dest values from the IPFIX record to SiLK's dest
         * and source fields, respectively. */
        TRACEMSG(("Read a reversed record"));

        /* Store volume, clamping counts to 32 bits. */
        rwRecSetPkts(rec, CLAMP_VAL(rev_pkts, UINT32_MAX));
        rwRecSetBytes(rec, CLAMP_VAL(rev_bytes, UINT32_MAX));

        /* This cannot be a bi-flow.  Clear rev_pkts and rev_bytes
         * variables now. We check this in the reverse_rec code
         * below. */
        rev_pkts = rev_bytes = 0;

        /* Handle the IP addresses */
#if SK_ENABLE_IPV6
        if (no_ipv4
            || !SK_IPV6_IS_ZERO(fixrec.rw.sourceIPv6Address)
            || !SK_IPV6_IS_ZERO(fixrec.rw.destinationIPv6Address))
        {
            /* Values found in IPv6 addresses--use them */
            rwRecSetIPv6(rec);
            rwRecMemSetSIPv6(rec, &fixrec.rw.destinationIPv6Address);
            rwRecMemSetDIPv6(rec, &fixrec.rw.sourceIPv6Address);
            rwRecMemSetNhIPv6(rec, &fixrec.rw.ipNextHopIPv6Address);
        } else
#endif /* SK_ENABLE_IPV6 */
        {
            /* Take values from IPv4 */
            rwRecSetSIPv4(rec, fixrec.rw.destinationIPv4Address);
            rwRecSetDIPv4(rec, fixrec.rw.sourceIPv4Address);
            rwRecSetNhIPv4(rec, fixrec.rw.ipNextHopIPv4Address);
        }

        /* Handle the Protocol and Ports */
        rwRecSetProto(rec, fixrec.rw.protocolIdentifier);
        if (!rwRecIsICMP(rec)) {
            rwRecSetSPort(rec, fixrec.rw.destinationTransportPort);
            rwRecSetDPort(rec, fixrec.rw.sourceTransportPort);
        } else {
            /* For an ICMP record, put whichever Port field is
             * non-zero into the record's dPort field */
            rwRecSetSPort(rec, 0);
            rwRecSetDPort(rec, (fixrec.rw.destinationTransportPort
                                ? fixrec.rw.destinationTransportPort
                                : fixrec.rw.sourceTransportPort));
        }

        /* Handle the SNMP or VLAN interfaces */
        if (SKPC_IFVALUE_VLAN == skpcProbeGetInterfaceValueType(probe)) {
            if (TMPL_HAS_ELEM(reverseVlanId)) {
                /* If we have the reverse elements, use them */
                rwRecSetInput(rec, fixrec.reverseVlanId);
                rwRecSetOutput(rec, fixrec.reversePostVlanId);
            } else if (TMPL_HAS_ELEM(postVlanId)) {
                /* If we have a single vlanId, set 'input' to that value;
                 * otherwise, set 'input' to postVlanId and 'output' to
                 * vlanId. */
                rwRecSetInput(rec, fixrec.postVlanId);
                rwRecSetOutput(rec, fixrec.vlanId);
            } else {
                /* we have a single vlanId, so don't swap the values */
                rwRecSetInput(rec, fixrec.vlanId);
            }
        } else {
            rwRecSetInput(rec,
                          CLAMP_VAL(fixrec.rw.egressInterface, UINT16_MAX));
            rwRecSetOutput(rec,
                           CLAMP_VAL(fixrec.rw.ingressInterface, UINT16_MAX));
        }

    }


    /* Run the Gauntlet of Time - convert all the various ways an IPFIX
     * record's time could be represented into start and elapsed times. */
    if (fixrec.rw.flowStartMilliseconds) {
        TRACEMSG(("Setting time using flowStartMilliseconds"));
        /* Flow start time in epoch milliseconds */
        rwRecSetStartTime(rec, (sktime_t)fixrec.rw.flowStartMilliseconds);
        if (fixrec.rw.flowEndMilliseconds >= fixrec.rw.flowStartMilliseconds) {
            /* Flow end time in epoch milliseconds */
            rwRecSetElapsed(rec,(uint32_t)(fixrec.rw.flowEndMilliseconds
                                           -fixrec.rw.flowStartMilliseconds));
        } else {
            /* Flow duration in milliseconds */
            rwRecSetElapsed(rec, fixrec.flowDurationMilliseconds);
        }
    } else if (fixrec.flowStartMicroseconds) {
        TRACEMSG(("Setting time using flowStartMicroseconds"));
        /* Flow start time in NTP microseconds */
        sTime = skiNTPDecode(fixrec.flowStartMicroseconds);
        rwRecSetStartTime(rec, (sktime_t)sTime);
        if (fixrec.flowEndMicroseconds >= fixrec.flowStartMicroseconds) {
            /* Flow end time in NTP microseconds */
            rwRecSetElapsed(rec,
                            (uint32_t)(skiNTPDecode(fixrec.flowEndMicroseconds)
                                       - sTime));
        } else {
            /* Flow duration in microseconds */
            rwRecSetElapsed(rec, (fixrec.flowDurationMicroseconds / 1000));
        }
    } else if (fixrec.flowStartSeconds) {
        TRACEMSG(("Setting time using flowStartSeconds"));
        /* Seconds? Sure, why not. */
        rwRecSetStartTime(rec, sktimeCreate(fixrec.flowStartSeconds, 0));
        rwRecSetElapsed(rec, ((uint32_t)1000 * (fixrec.flowEndSeconds
                                                - fixrec.flowStartSeconds)));
    } else if (fixrec.flowStartDeltaMicroseconds) {
        TRACEMSG(("Setting time using flowStartDeltaMicroseconds"));
        /* Flow start time in delta microseconds */
        sTime = (fBufGetExportTime(fbuf) * 1000
                 - fixrec.flowStartDeltaMicroseconds / 1000);
        rwRecSetStartTime(rec, (sktime_t)sTime);
        if (fixrec.flowEndDeltaMicroseconds
            && (fixrec.flowEndDeltaMicroseconds
                <= fixrec.flowStartDeltaMicroseconds))
        {
            /* Flow end time in delta microseconds */
            eTime = (fBufGetExportTime(fbuf) * 1000
                     - fixrec.flowEndDeltaMicroseconds / 1000);
            rwRecSetElapsed(rec, (uint32_t)(eTime - sTime));
        } else {
            /* Flow duration in microseconds */
            rwRecSetElapsed(rec, (fixrec.flowDurationMicroseconds / 1000));
        }
    } else if (TMPL_HAS_ELEM(flowStartSysUpTime)) {
        /* Times based on flow generator system uptimes (Netflow v9) */
        intmax_t uptime, difference;
        sktime_t export_msec;

        TRACEMSG(("Setting time from flowStartSysUpTime %u  %u",
                  fixrec.flowStartSysUpTime, fixrec.flowEndSysUpTime));

        /* Set duration.  Our NetFlow v5 code checks the magnitude of
         * the difference between te eTime and sTime; this code is not
         * that complicated---we assume if eTime is less than sTime
         * then eTime has rolled over. */
        if (fixrec.flowStartSysUpTime <= fixrec.flowEndSysUpTime) {
            rwRecSetElapsed(rec, (fixrec.flowEndSysUpTime
                                  - fixrec.flowStartSysUpTime));
        } else {
            /* assume EndTime rolled-over and start did not */
            rwRecSetElapsed(rec, (ROLLOVER32 + fixrec.flowEndSysUpTime
                                  - fixrec.flowStartSysUpTime));
        }

        /* Set start time. */
        export_msec = sktimeCreate(fBufGetExportTime(fbuf), 0);
        if (!TMPL_HAS_ELEM(systemInitTimeMilliseconds)) {
            /* we do not know when the router booted.  assume end-time
             * is same as the record's export time and set start-time
             * accordingly. */
            TRACEMSG((("Setting times for NetFlowV9:"
                       " exportTime %" PRId64 ", bootTime N/A, upTime N/A"
                       ", flowStartSysUpTime %" PRIu32
                       ", flowEndSysUpTime %" PRIu32),
                      (int64_t)export_msec, fixrec.flowStartSysUpTime,
                      fixrec.flowEndSysUpTime));
            rwRecSetStartTime(rec, export_msec - rwRecGetElapsed(rec));
        } else {
            /* systemInitTimeMilliseconds is the absolute router boot
             * time (msec), and libfixbuf sets it by subtracting the
             * NFv9 uptime (msec) from the record's abolute export
             * time (sec). */
            uptime = export_msec - fixrec.systemInitTimeMilliseconds;
            TRACEMSG((("Setting times for NetFlowV9:"
                       " exportTime %" PRId64 ", bootTime %" PRIu64
                       ", upTime %" PRId64 ", flowStartSysUpTime %" PRIu32
                       ", flowEndSysUpTime %" PRIu32),
                      (int64_t)export_msec,
                      fixrec.systemInitTimeMilliseconds, (int64_t)uptime,
                      fixrec.flowStartSysUpTime,
                      fixrec.flowEndSysUpTime));

            difference = uptime - fixrec.flowStartSysUpTime;
            if (difference > MAXIMUM_FLOW_TIME_DEVIATION) {
                /* assume upTime is set before record is composed and
                 * that start-time has rolled over. */
                rwRecSetStartTime(rec, (fixrec.systemInitTimeMilliseconds
                                        + fixrec.flowStartSysUpTime
                                        + ROLLOVER32));
            } else if (-difference > MAXIMUM_FLOW_TIME_DEVIATION) {
                /* assume upTime is set after record is composed and
                 * that upTime has rolled over. */
                rwRecSetStartTime(rec, (fixrec.systemInitTimeMilliseconds
                                        + fixrec.flowStartSysUpTime
                                        - ROLLOVER32));
            } else {
                /* times look reasonable; assume no roll over */
                rwRecSetStartTime(rec, (fixrec.systemInitTimeMilliseconds
                                        + fixrec.flowStartSysUpTime));
            }
        }
    } else {
        TRACEMSG(("Setting time from export time %u",
                  fBufGetExportTime(fbuf)));
        /* No per-flow time information.
         * Assume message export is flow end time. */
        if (fixrec.flowDurationMilliseconds) {
            /* Flow duration in milliseconds */
            rwRecSetElapsed(rec, fixrec.flowDurationMilliseconds);
        } else if (fixrec.flowDurationMicroseconds) {
            /* Flow duration in microseconds */
            rwRecSetElapsed(rec, (fixrec.flowDurationMicroseconds / 1000));
        } else {
            /* Presume zero duration flow */
            rwRecSetElapsed(rec, 0);
        }
        /* Set start time based on export and elapsed time */
        rwRecSetStartTime(rec, sktimeCreate((fBufGetExportTime(fbuf)
                                             - rwRecGetElapsed(rec)), 0));
    }

    /* Copy the remainder of the record */
    rwRecSetFlowType(rec, fixrec.rw.silkFlowType);
    rwRecSetSensor(rec, fixrec.rw.silkFlowSensor);
    rwRecSetApplication(rec, fixrec.rw.silkAppLabel);

    tcp_state = fixrec.rw.silkTCPState;
    tcp_flags = (fixrec.rw.initialTCPFlags | fixrec.rw.unionTCPFlags);

    /* Ensure the SK_TCPSTATE_EXPANDED bit is properly set. */
    if (tcp_flags && IPPROTO_TCP == rwRecGetProto(rec)) {
        /* Flow is TCP and init|session flags had a value. */
        rwRecSetFlags(rec, tcp_flags);
        rwRecSetInitFlags(rec, fixrec.rw.initialTCPFlags);
        rwRecSetRestFlags(rec, fixrec.rw.unionTCPFlags);
        tcp_state |= SK_TCPSTATE_EXPANDED;
    } else {
        /* clear bit when not TCP or no separate init/session flags */
        tcp_state &= ~SK_TCPSTATE_EXPANDED;
        /* use whatever all-flags we were given; leave initial-flags
         * and session-flags unset */
        rwRecSetFlags(rec, fixrec.rw.tcpControlBits);
    }

    /* Process the flowEndReason and flowAttributes unless one of
     * those bits is already set (via silkTCPState). */
    if (!(tcp_state
          & (SK_TCPSTATE_FIN_FOLLOWED_NOT_ACK | SK_TCPSTATE_TIMEOUT_KILLED
             | SK_TCPSTATE_TIMEOUT_STARTED | SK_TCPSTATE_UNIFORM_PACKET_SIZE)))
    {
        /* Note active timeout */
        if ((fixrec.flowEndReason & SKI_END_MASK) == SKI_END_ACTIVE) {
            tcp_state |= SK_TCPSTATE_TIMEOUT_KILLED;
        }
        /* Note continuation */
        if (fixrec.flowEndReason & SKI_END_ISCONT) {
            tcp_state |= SK_TCPSTATE_TIMEOUT_STARTED;
        }
        /* Note flows with records of uniform size */
        if (fixrec.flowAttributes & SKI_FLOW_ATTRIBUTE_UNIFORM_PACKET_SIZE) {
            tcp_state |= SK_TCPSTATE_UNIFORM_PACKET_SIZE;
        }
        rwRecSetTcpState(rec, tcp_state);
    }

    rwRecSetTcpState(rec, tcp_state);


    /* Handle the reverse record if the caller provided one and if
     * there is one in the IPFIX record, which is indicated by the
     * value of 'rev_bytes'.*/
    if (0 == rev_bytes) {
        /* No data for reverse direction; just clear the record. */
        if (reverse_rec) {
            rwRec *revRec;
            revRec = skIPFIXSourceRecordGetRwrec(reverse_rec);
            RWREC_CLEAR(revRec);
        }
    } else if (reverse_rec) {
        rwRec *revRec;
        revRec = skIPFIXSourceRecordGetRwrec(reverse_rec);

        /* We have data for reverse direction. */
        TRACEMSG(("Handling reverse side of bi-flow"));

#define COPY_FORWARD_REC_TO_REVERSE 1
#if COPY_FORWARD_REC_TO_REVERSE
        /* Initialize the reverse record with the forward
         * record  */
        RWREC_COPY(revRec, rec);
#else
        /* instead of copying the forward record and changing
         * nearly everything, we could just set these fields on
         * the reverse record. */
        rwRecSetProto(revRec, fixrec.rw.protocolIdentifier);
        rwRecSetFlowType(revRec, fixrec.rw.silkFlowType);
        rwRecSetSensor(revRec, fixrec.rw.silkFlowSensor);
        rwRecSetTcpState(revRec, fixrec.rw.silkTCPState);
        rwRecSetApplication(revRec, fixrec.rw.silkAppLabel);
        /* does using the forward nexthop IP for the reverse
         * record make any sense?  Shouldn't we check for a
         * reverse next hop address? */
#if SK_ENABLE_IPV6
        if (rwRecIsIPv6(rec)) {
            rwRecSetIPv6(revRec);
            rwRecMemSetNhIPv6(revRec, &fixrec.rw.ipNextHopIPv6Address);
        } else
#endif
        {
            rwRecSetNhIPv4(revRec, &fixrec.rw.ipNextHopIPv4Address);
        }
#endif  /* #else clause of #if COPY_FORWARD_REC_TO_REVERSE */

        /* Reverse the IPs */
#if SK_ENABLE_IPV6
        if (rwRecIsIPv6(rec)) {
            rwRecMemSetSIPv6(revRec, &fixrec.rw.destinationIPv6Address);
            rwRecMemSetDIPv6(revRec, &fixrec.rw.sourceIPv6Address);
        } else
#endif
        {
            rwRecSetSIPv4(revRec, fixrec.rw.destinationIPv4Address);
            rwRecSetDIPv4(revRec, fixrec.rw.sourceIPv4Address);
        }

        /* Reverse the ports unless this is an ICMP record */
        if (!rwRecIsICMP(rec)) {
            rwRecSetSPort(revRec, rwRecGetDPort(rec));
            rwRecSetDPort(revRec, rwRecGetSPort(rec));
        }

        /* Reverse the SNMP or VLAN interfaces */
        if (SKPC_IFVALUE_VLAN != skpcProbeGetInterfaceValueType(probe)) {
            rwRecSetInput(revRec, rwRecGetOutput(rec));
            rwRecSetOutput(revRec, rwRecGetInput(rec));
        } else if (TMPL_HAS_ELEM(reverseVlanId)) {
            /* Reverse VLAN values exist.  Use them */
            rwRecSetInput(rec, fixrec.reverseVlanId);
            rwRecSetOutput(rec, fixrec.reversePostVlanId);
        } else if (TMPL_HAS_ELEM(postVlanId)) {
            /* Reverse the forward values */
            rwRecSetInput(rec, fixrec.postVlanId);
            rwRecSetOutput(rec, fixrec.vlanId);
        } else {
            /* we have a single vlanId, so don't swap the values */
            rwRecSetInput(rec, fixrec.vlanId);
        }

        /* Set volume.  We retrieved them above */
        rwRecSetPkts(revRec, CLAMP_VAL(rev_pkts, UINT32_MAX));
        rwRecSetBytes(revRec, CLAMP_VAL(rev_bytes, UINT32_MAX));

        /* Calculate reverse start time from reverse RTT */

        /* Reverse flow's start time must be increased and its
         * duration decreased by its offset from the forward
         * record  */
        rwRecSetStartTime(revRec, (rwRecGetStartTime(rec)
                                   + fixrec.reverseFlowDeltaMilliseconds));
        rwRecSetElapsed(revRec, (rwRecGetElapsed(rec)
                                 - fixrec.reverseFlowDeltaMilliseconds));

        /* Note: the value of the 'tcp_state' variable from above is
         * what is in rwRecGetTcpState(revRec). */

        /* Get reverse TCP flags from the IPFIX record if they are
         * available.  Otherwise, leave the flags unchanged (using
         * those from the forward direction). */
        tcp_flags =(fixrec.reverseInitialTCPFlags|fixrec.reverseUnionTCPFlags);

        if (tcp_flags && IPPROTO_TCP == rwRecGetProto(rec)) {
            /* Flow is TCP and init|session has a value. */
            TRACEMSG(("Using reverse TCP flags (initial|session)"));
            rwRecSetFlags(revRec, tcp_flags);
            rwRecSetInitFlags(revRec, fixrec.reverseInitialTCPFlags);
            rwRecSetRestFlags(revRec, fixrec.reverseUnionTCPFlags);
            tcp_state |= SK_TCPSTATE_EXPANDED;
        } else if (TMPL_HAS_ELEM(reverseTcpControlBits)) {
            /* Use whatever is in all-flags; clear any init/session
             * flags we got from the forward rec. */
            TRACEMSG(("Using reverse TCP flags (all only)"));
            rwRecSetFlags(revRec, fixrec.reverseTcpControlBits);
            rwRecSetInitFlags(revRec, 0);
            rwRecSetRestFlags(revRec, 0);
            tcp_state &= ~SK_TCPSTATE_EXPANDED;
        } else if (have_tcp_stml || (TMPL_HAS_ELEM(reverseInitialTCPFlags))) {
            /* If a reverseInitialTCPFlags Element existed on the
             * template; use it even though its value is 0. */
            TRACEMSG(("Setting all TCP flags to 0"));
            rwRecSetFlags(revRec, 0);
            rwRecSetInitFlags(revRec, 0);
            rwRecSetRestFlags(revRec, 0);
            tcp_state &= ~SK_TCPSTATE_EXPANDED;
        }
        /* else leave the flags unchanged */

        /* Handle reverse flow attributes */
        if (fixrec.reverseFlowAttributes
            & SKI_FLOW_ATTRIBUTE_UNIFORM_PACKET_SIZE)
        {
            /* ensure it is set */
            tcp_state |= SK_TCPSTATE_UNIFORM_PACKET_SIZE;
        } else {
            /* ensure it it not set */
            tcp_state &= ~SK_TCPSTATE_UNIFORM_PACKET_SIZE;
        }

        rwRecSetTcpState(revRec, tcp_state);


    }

    /* all done */
    return ((rev_bytes > 0) ? 2 : 1);
}


/* **************************************************************
 * *****  Support for writing/export
 */

fBuf_t *
skiCreateWriteBufferForFP(
    FILE               *fp,
    uint32_t            domain,
    GError            **err)
{
    fbInfoModel_t   *model = NULL;
    fbExporter_t    *exporter = NULL;
    fbSession_t     *session = NULL;
    fbTemplate_t    *tmpl = NULL;
    fBuf_t          *fbuf = NULL;

    model = skiInfoModel();
    if (NULL == model) {
        return NULL;
    }

    exporter = fbExporterAllocFP(fp);
    if (NULL == exporter) {
        return NULL;
    }

    /* Allocate a session.  The session will be owned by the fbuf, so
     * don't save it for later freeing. */
    session = fbSessionAlloc(model);
    if (session == NULL) {
        goto ERROR;
    }

    /* set observation domain */
    fbSessionSetDomain(session, domain);

    /* Add the full record template */
    tmpl = fbTemplateAlloc(model);
    if (!fbTemplateAppendSpecArray(tmpl, ski_rwrec_spec, 0, err)) {
        goto ERROR;
    }
    if (!fbSessionAddTemplate(session, TRUE, SKI_RWREC_TID, tmpl, err)) {
        goto ERROR;
    }
    if (!fbSessionAddTemplate(session, FALSE, SKI_RWREC_TID, tmpl, err)) {
        goto ERROR;
    }

    /* Create a buffer with the session and the exporter */
    fbuf = fBufAllocForExport(session, exporter);

    /* write RW base flow template */
    if (!fbSessionExportTemplates(session, err)) {
        goto ERROR;
    }

    /* set default templates */
    if (!fBufSetInternalTemplate(fbuf, SKI_RWREC_TID, err)) {
        goto ERROR;
    }
    if (!fBufSetExportTemplate(fbuf, SKI_RWREC_TID, err)) {
        goto ERROR;
    }

    /* done */
    return fbuf;

  ERROR:
    if (fbuf) {
        fBufFree(fbuf);
    } else {
        fbTemplateFreeUnused(tmpl);
        if (session) {
            fbSessionFree(session);
        }
    }
    return NULL;
}


/* Append SiLK Flow 'rec' to the libfixbuf buffer 'fbuf' */
gboolean
skiRwAppendRecord(
    fBuf_t             *fbuf,
    const rwRec        *rec,
    GError            **err)
{
    ski_rwrec_t fixrec;

    /* Convert time from start/elapsed to start and end epoch millis. */
    fixrec.flowStartMilliseconds = (uint64_t)rwRecGetStartTime(rec);
    fixrec.flowEndMilliseconds = ((uint64_t)fixrec.flowStartMilliseconds
                                  + rwRecGetElapsed(rec));

    /* Handle IP addresses */
#if SK_ENABLE_IPV6
    if (rwRecIsIPv6(rec)) {
        rwRecMemGetSIPv6(rec, fixrec.sourceIPv6Address);
        rwRecMemGetDIPv6(rec, fixrec.destinationIPv6Address);
        rwRecMemGetNhIPv6(rec, fixrec.ipNextHopIPv6Address);
        fixrec.sourceIPv4Address = 0;
        fixrec.destinationIPv4Address = 0;
        fixrec.ipNextHopIPv4Address = 0;
    } else
#endif
    {
        memset(fixrec.sourceIPv6Address, 0,
               sizeof(fixrec.sourceIPv6Address));
        memset(fixrec.destinationIPv6Address, 0,
               sizeof(fixrec.destinationIPv6Address));
        memset(fixrec.ipNextHopIPv6Address, 0,
               sizeof(fixrec.ipNextHopIPv6Address));
        fixrec.sourceIPv4Address = rwRecGetSIPv4(rec);
        fixrec.destinationIPv4Address = rwRecGetDIPv4(rec);
        fixrec.ipNextHopIPv4Address = rwRecGetNhIPv4(rec);
    }

    /* Copy rest of record */
    fixrec.sourceTransportPort = rwRecGetSPort(rec);
    fixrec.destinationTransportPort = rwRecGetDPort(rec);
    fixrec.ingressInterface = rwRecGetInput(rec);
    fixrec.egressInterface = rwRecGetOutput(rec);
    fixrec.packetDeltaCount = rwRecGetPkts(rec);
    fixrec.octetDeltaCount = rwRecGetBytes(rec);
    fixrec.protocolIdentifier = rwRecGetProto(rec);
    fixrec.silkFlowType = rwRecGetFlowType(rec);
    fixrec.silkFlowSensor = rwRecGetSensor(rec);
    fixrec.tcpControlBits = rwRecGetFlags(rec);
    fixrec.initialTCPFlags = rwRecGetInitFlags(rec);
    fixrec.unionTCPFlags = rwRecGetRestFlags(rec);
    fixrec.silkTCPState = rwRecGetTcpState(rec);
    fixrec.silkAppLabel = rwRecGetApplication(rec);

#if SKI_RWREC_PADDING != 0
    /* According to RFC5102, the value of the paddingOctets
     * Information Element "is always a sequence of 0x00 values." */
    memset(fixrec.pad, 0, SKI_RWREC_PADDING);
#endif

    /* Append the record to the buffer */
    if (!fBufAppend(fbuf, (uint8_t *)&fixrec, sizeof(fixrec), err)) {
        return FALSE;
    }

    /* all done */
    return TRUE;
}




void
skiCheckDataStructure(
    FILE               *fh)
{
    unsigned long pos;

#define PRINT_TITLE(s_)                                         \
    fprintf(fh, "===> %s\n%5s|%5s|%5s|%5s|%5s|%s\n", #s_,       \
            "begin", "end", "size", "alerr", "hole", "member")

#define PRINT_OFFSET(pos_, s_, mem_)                                    \
    {                                                                   \
        s_ x;                                                           \
        unsigned long off_ = (unsigned long)offsetof(s_, mem_);         \
        unsigned long sz_  = (unsigned long)sizeof(x.mem_);             \
        unsigned long end_ = off_ + sz_ - 1;                            \
        int align_ = ((off_ % sz_) == 0);                               \
        int hole_ = (pos_ != off_);                                     \
        pos_ += sz_;                                                    \
        fprintf(fh, "%5lu|%5lu|%5lu|%5s|%5s|%s\n",                      \
                off_, end_, sz_, (align_ ? "" : "alerr"),               \
                (hole_ ? "hole" : ""), #mem_);                          \
    }

    pos = 0;
    PRINT_TITLE(ski_rwrec_t);
    PRINT_OFFSET(pos, ski_rwrec_t, flowStartMilliseconds);
    PRINT_OFFSET(pos, ski_rwrec_t, flowEndMilliseconds);
    PRINT_OFFSET(pos, ski_rwrec_t, sourceIPv6Address);
    PRINT_OFFSET(pos, ski_rwrec_t, destinationIPv6Address);
    PRINT_OFFSET(pos, ski_rwrec_t, sourceIPv4Address);
    PRINT_OFFSET(pos, ski_rwrec_t, destinationIPv4Address);
    PRINT_OFFSET(pos, ski_rwrec_t, sourceTransportPort);
    PRINT_OFFSET(pos, ski_rwrec_t, destinationTransportPort);
    PRINT_OFFSET(pos, ski_rwrec_t, ipNextHopIPv4Address);
    PRINT_OFFSET(pos, ski_rwrec_t, ipNextHopIPv6Address);
    PRINT_OFFSET(pos, ski_rwrec_t, ingressInterface);
    PRINT_OFFSET(pos, ski_rwrec_t, egressInterface);
    PRINT_OFFSET(pos, ski_rwrec_t, packetDeltaCount);
    PRINT_OFFSET(pos, ski_rwrec_t, octetDeltaCount);
    PRINT_OFFSET(pos, ski_rwrec_t, protocolIdentifier);
    PRINT_OFFSET(pos, ski_rwrec_t, silkFlowType);
    PRINT_OFFSET(pos, ski_rwrec_t, silkFlowSensor);
    PRINT_OFFSET(pos, ski_rwrec_t, tcpControlBits);
    PRINT_OFFSET(pos, ski_rwrec_t, initialTCPFlags);
    PRINT_OFFSET(pos, ski_rwrec_t, unionTCPFlags);
    PRINT_OFFSET(pos, ski_rwrec_t, silkTCPState);
    PRINT_OFFSET(pos, ski_rwrec_t, silkAppLabel);
#if SKI_RWREC_PADDING != 0
    PRINT_OFFSET(pos, ski_rwrec_t, pad);
#endif

    pos = 0;
    PRINT_TITLE(ski_extrwrec_t);
    PRINT_OFFSET(pos, ski_extrwrec_t, rw);
    PRINT_OFFSET(pos, ski_extrwrec_t, packetTotalCount);
    PRINT_OFFSET(pos, ski_extrwrec_t, octetTotalCount);
    PRINT_OFFSET(pos, ski_extrwrec_t, initiatorPackets);
    PRINT_OFFSET(pos, ski_extrwrec_t, initiatorOctets);
    PRINT_OFFSET(pos, ski_extrwrec_t, reversePacketDeltaCount);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseOctetDeltaCount);
    PRINT_OFFSET(pos, ski_extrwrec_t, reversePacketTotalCount);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseOctetTotalCount);
    PRINT_OFFSET(pos, ski_extrwrec_t, responderPackets);
    PRINT_OFFSET(pos, ski_extrwrec_t, responderOctets);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowStartMicroseconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowEndMicroseconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, systemInitTimeMilliseconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowStartSeconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowEndSeconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowDurationMicroseconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowDurationMilliseconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowStartDeltaMicroseconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowEndDeltaMicroseconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseFlowDeltaMilliseconds);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowStartSysUpTime);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowEndSysUpTime);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseTcpControlBits);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseInitialTCPFlags);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseUnionTCPFlags);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowEndReason);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowAttributes);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseFlowAttributes);
    PRINT_OFFSET(pos, ski_extrwrec_t, vlanId);
    PRINT_OFFSET(pos, ski_extrwrec_t, postVlanId);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseVlanId);
    PRINT_OFFSET(pos, ski_extrwrec_t, reversePostVlanId);
    PRINT_OFFSET(pos, ski_extrwrec_t, bgpSourceAsNumber);
    PRINT_OFFSET(pos, ski_extrwrec_t, bgpDestinationAsNumber);
    PRINT_OFFSET(pos, ski_extrwrec_t, mplsTopLabelIPv4Address);
    PRINT_OFFSET(pos, ski_extrwrec_t, mplsLabels);
    PRINT_OFFSET(pos, ski_extrwrec_t, mplsTopLabelPrefixLength);
    PRINT_OFFSET(pos, ski_extrwrec_t, mplsTopLabelType);
    PRINT_OFFSET(pos, ski_extrwrec_t, firewallEvent);
    PRINT_OFFSET(pos, ski_extrwrec_t, NF_F_FW_EVENT);
    PRINT_OFFSET(pos, ski_extrwrec_t, NF_F_FW_EXT_EVENT);
    PRINT_OFFSET(pos, ski_extrwrec_t, ipClassOfService);
    PRINT_OFFSET(pos, ski_extrwrec_t, reverseIpClassOfService);
    PRINT_OFFSET(pos, ski_extrwrec_t, sourceMacAddress);
    PRINT_OFFSET(pos, ski_extrwrec_t, destinationMacAddress);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowSamplerID);
    PRINT_OFFSET(pos, ski_extrwrec_t, flowDirection);
#if SKI_EXTRWREC_PADDING != 0
    PRINT_OFFSET(pos, ski_extrwrec_t, pad);
#endif

    pos = 0;
    PRINT_TITLE(ski_yaf_stats_t);
    PRINT_OFFSET(pos, ski_yaf_stats_t, systemInitTimeMilliseconds);
    PRINT_OFFSET(pos, ski_yaf_stats_t, exportedFlowRecordTotalCount);
    PRINT_OFFSET(pos, ski_yaf_stats_t, packetTotalCount);
    PRINT_OFFSET(pos, ski_yaf_stats_t, droppedPacketTotalCount);
    PRINT_OFFSET(pos, ski_yaf_stats_t, ignoredPacketTotalCount);
    PRINT_OFFSET(pos, ski_yaf_stats_t, notSentPacketTotalCount);
    PRINT_OFFSET(pos, ski_yaf_stats_t, expiredFragmentCount);
#if 0
    PRINT_OFFSET(pos, ski_yaf_stats_t, assembledFragmentCount);
    PRINT_OFFSET(pos, ski_yaf_stats_t, flowTableFlushEventCount);
    PRINT_OFFSET(pos, ski_yaf_stats_t, flowTablePeakCount);
    PRINT_OFFSET(pos, ski_yaf_stats_t, meanFlowRate);
    PRINT_OFFSET(pos, ski_yaf_stats_t, meanPacketRate);
    PRINT_OFFSET(pos, ski_yaf_stats_t, exporterIPv4Address);
#endif  /* 0 */
    PRINT_OFFSET(pos, ski_yaf_stats_t, exportingProcessId);
#if SKI_YAF_STATS_PADDING != 0
    PRINT_OFFSET(pos, ski_yaf_stats_t, pad);
#endif
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
