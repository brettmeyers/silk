/*
** Copyright (C) 2004-2015 by Carnegie Mellon University.
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
#ifndef _V5PDU_H
#define _V5PDU_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_V5PDU_H, "$SiLK: v5pdu.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");


/*
**  v5pdu.h
**
**  Structures defining Version 5 PDU NetFlow Records
**
*/

/*
** v5Header is 24 bytes, v5Record is 48 bytes.  Using the Ethernet MTU
** of 1500, we get get: ((1500 - 24)/48) => 30 v5Records per MTU, and
** the overall length of the PDU will be: (24 + (30*48)) => 1464 bytes
*/
#define V5PDU_LEN 1464
#define V5PDU_MAX_RECS 30

#define V5PDU_MAX_RECS_STR  "30"


typedef struct v5Header_st {
  uint16_t version;
  uint16_t count;
  uint32_t SysUptime;
  uint32_t unix_secs;
  uint32_t unix_nsecs;
  uint32_t flow_sequence;
  uint8_t  engine_type;
  uint8_t  engine_id;
  uint16_t sampling_interval;
} v5Header;

typedef struct v5Record_st {
  uint32_t  srcaddr;   /*  0- 3 */
  uint32_t  dstaddr;   /*  4- 7 */
  uint32_t  nexthop;   /*  8-11 */
  uint16_t  input;     /* 12-13 */
  uint16_t  output;    /* 14-15 */
  uint32_t  dPkts;     /* 16-19 */
  uint32_t  dOctets;   /* 20-23 */
  uint32_t  First;     /* 24-27 */
  uint32_t  Last;      /* 28-31 */
  uint16_t  srcport;   /* 32-33 */
  uint16_t  dstport;   /* 34-35 */
  uint8_t   pad1;      /* 36    */
  uint8_t   tcp_flags; /* 37    */
  uint8_t   prot;      /* 38    */
  uint8_t   tos;       /* 39    */
  uint16_t  src_as;    /* 40-41 */
  uint16_t  dst_as;    /* 42-43 */
  uint8_t   src_mask;  /* 44    */
  uint8_t   dst_mask;  /* 45    */
  uint16_t  pad2;      /* 46-47 */
} v5Record;

typedef struct v5PDU_st {
  v5Header hdr;
  v5Record data[V5PDU_MAX_RECS];
} v5PDU;

#ifdef __cplusplus
}
#endif
#endif /* _V5PDU_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
