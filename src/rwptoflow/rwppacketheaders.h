/*
** Copyright (C) 2005-2014 by Carnegie Mellon University.
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
#ifndef _RWPPACKETHEADERS_H
#define _RWPPACKETHEADERS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWPMATCH_H, "$SiLK: rwppacketheaders.h 1c87fe1a22f7 2014-01-06 15:24:30Z mthomas $");

#ifdef SK_HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>
#else
#include <pcap.h>
#endif
#include <silk/utils.h>


/*
**  rwppacketheaders.h
**
**  Headers for ethernet, IP, ICMP, TCP, and UDP packets.
**
*/


/* mask with the IP header flags/fragment offset field to get the
 * fragment offset. */
#ifndef IPHEADER_FO_MASK
#define IPHEADER_FO_MASK 0x1FFF
#endif

/* mask with the IP header flags/fragment offset field to get the
 * 'more fragments' bit */
#ifndef IP_MF
#define IP_MF  0x2000
#endif


typedef struct eth_header_st {
    uint8_t     ether_dhost[6]; /* destination eth addr */
    uint8_t     ether_shost[6]; /* source ether addr    */
    uint16_t    ether_type;     /* packet type ID field */
} eth_header_t;

#ifndef ETHERTYPE_IP
#define ETHERTYPE_IP 0x0800
#endif

/* IPv4 header */
typedef struct ip_header_st {
    uint8_t     ver_ihl;        /*  0: version:4; header_length_in_words:4; */
    uint8_t     tos;            /*  1: type of service */
    uint16_t    tlen;           /*  2: total length (hdr + payload) */
    uint16_t    ident;          /*  4: identification */
    uint16_t    flags_fo;       /*  6: fragmentation: flags:3;offset:13; */
    uint8_t     ttl;            /*  8: time to live */
    uint8_t     proto;          /*  9: protocol */
    uint16_t    crc;            /* 10: checksum */
    uint32_t    saddr;          /* 12: source address */
    uint32_t    daddr;          /* 16: desitation address */
    /*                             20: variable length options */
} ip_header_t;


/* ICMP header */
typedef struct icmp_header_st {
    uint8_t     type;           /*  0: type of message */
    uint8_t     code;           /*  1: type sub-code */
    uint16_t    checksum;       /*  2: ones complement checksum */
    /*                              4: ICMP Message */
} icmp_header_t;


/* TCP header */
typedef struct tcp_header_st {
    uint16_t    sport;          /*  0: source port */
    uint16_t    dport;          /*  2: destination port */
    uint32_t    seqNum;         /*  4: sequence number */
    uint32_t    ackNum;         /*  8: acknowledgement number */
    uint8_t     offset;         /* 12: offset */
    uint8_t     flags;          /* 13: packet flags */
    uint16_t    window;         /* 14: window */
    uint16_t    checksum;       /* 16: checksum */
    uint16_t    urgentPtr;      /* 18: urgent pointer */
    /*                             20: Variable length options and padding */
} tcp_header_t;


/* UDP header */
typedef struct udp_header_st {
    uint16_t    sport;          /*  0: source port */
    uint16_t    dport;          /*  2: destination port */
    uint16_t    len;            /*  4: udp length */
    uint16_t    crc;            /*  6: udp checksum */
    /*                              8: UDP data */
} udp_header_t;


/* structure used when communicating with plug-ins */
typedef struct sk_pktsrc_st {
    /* the source of the packets */
    pcap_t                     *pcap_src;
    /* the pcap header as returned from pcap_next() */
    const struct pcap_pkthdr   *pcap_hdr;
    /* the packet as returned from pcap_next() */
    const u_char               *pcap_data;
} sk_pktsrc_t;

/*
 * rwptoflow hands the packet to the plugin as an "extra argument".
 * rwptoflow and its plugins must agree on the name of this argument.
 * The extra argument is specified in a NULL-terminated array of
 * argument names.
 */
#define RWP2F_EXTRA_ARGUMENTS {"ptoflow", NULL}


#ifdef __cplusplus
}
#endif
#endif /* _RWPPACKETHEADERS_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
