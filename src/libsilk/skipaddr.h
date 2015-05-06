/*
** Copyright (C) 2001-2015 by Carnegie Mellon University.
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
**  skipaddr.h
**
**    Macros and function declarations for handling IP addresses
**    (skipaddr_t and ipUnion).
**
*/
#ifndef _SKIPADDR_H
#define _SKIPADDR_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKIPADDR_H, "$SiLK: skipaddr.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/silk_types.h>

/*
**  For reference.  Defined in silk_types.h
**
**    typedef union ipUnion_un {
**        uint32_t    ipu_ipv4;
**    #if SK_ENABLE_IPV6
**        uint8_t     ipu_ipv6[16];
**    #endif
**    } ipUnion;
**
**
**    typedef struct skipaddr_st {
**        ipUnion     ip_ip;
**    #if SK_ENABLE_IPV6
**        unsigned    ip_is_v6 :1;
**    #endif
**    } skipaddr_t;
*/


/*
 *  is_zero = SK_IPV6_IS_ZERO(ipv6);
 *
 *    Return true if the specified ipv6 address is zero, where ipv6
 *    was defined as:
 *
 *    uint8_t ipv6[16];
 */
#define SK_IPV6_IS_ZERO(v6_byte_array)                                  \
    (0 == memcmp((v6_byte_array), sk_ipv6_zero, SK_IPV6_ZERO_LEN))
#define SK_IPV6_ZERO_LEN 16
extern const uint8_t sk_ipv6_zero[SK_IPV6_ZERO_LEN];


/*
 *  is_zero = SK_IPV6_IS_V4INV6(ipv6);
 *
 *    Return true if the specified ipv6 address represents an
 *    IPv6-encoded-IPv4 address, where ipv6 was defined as:
 *
 *    uint8_t ipv6[16];
 */
#define SK_IPV6_IS_V4INV6(v6_byte_array)                                   \
    (0 == memcmp((v6_byte_array), sk_ipv6_v4inv6, SK_IPV6_V4INV6_LEN))
#define SK_IPV6_V4INV6_LEN 12
extern const uint8_t sk_ipv6_v4inv6[SK_IPV6_V4INV6_LEN];


/* ****  ipUnion  **** */

/* Macros dealing with ipUnions are typically for use by other SiLK
 * macros and functions.  These macros are subject to change at any
 * time. */

/* Get and Set the V4 part of the address structure */
#define ipUnionGetV4(ipu)                       \
    ((ipu)->ipu_ipv4)

#define ipUnionSetV4(ipu, in_vp)                \
    memcpy(&((ipu)->ipu_ipv4), (in_vp), 4)

#define ipUnionApplyMaskV4(ipu, v4_mask)                \
    do { ((ipu)->ipu_ipv4) &= (v4_mask); } while(0)

/* Get the 'cidr' most significant bits of the V4 address */
#define ipUnionGetCIDRV4(ipu, cidr)                     \
    (((cidr) >= 32)                                     \
     ? ((ipu)->ipu_ipv4)                                \
     : (((ipu)->ipu_ipv4) & ~(UINT32_MAX >> cidr)))

/* Set the V4 address to its 'cidr' most significant bits.  Assumes
 * the following: 0 <= 'cidr' < 32 */
#define ipUnionApplyCIDRV4(ipu, cidr)                                   \
    do { (((ipu)->ipu_ipv4) &= ~(UINT32_MAX >> cidr)); } while(0)

#if SK_ENABLE_IPV6

/* Get and Set the V6 parts of the address structure */
#define ipUnionGetV6(ipu, out_vp)               \
    memcpy((out_vp), (ipu)->ipu_ipv6, 16)

#define ipUnionSetV6(ipu, in_vp)               \
    memcpy((ipu)->ipu_ipv6, (in_vp), 16)

/* Convert a pointer to a native uint32_t to an IPv6 byte array  */
#define ipUnionU32ToV6(src_u32, dst_v6)                                 \
    do {                                                                \
        uint32_t ipu32tov6 = htonl(*(src_u32));                         \
        memcpy((dst_v6), sk_ipv6_v4inv6, SK_IPV6_V4INV6_LEN);           \
        memcpy(((uint8_t*)(dst_v6)+SK_IPV6_V4INV6_LEN), &ipu32tov6, 4); \
    } while(0)

/* Write the V4 address into a V6 location. The two parameters can
 * point to the same ipUnion. */
#define ipUnionGetV4AsV6(ipu, ipv6)                               \
    ipUnionU32ToV6(&((ipu)->ipu_ipv4), ipv6)

/* Convert a V4 ipUnion to an V6 ipUnion. The two parameters can point
 * to the same ipUnion. */
#define ipUnion4to6(src_ipu, dst_ipu)                                   \
    ipUnionU32ToV6(&((src_ipu)->ipu_ipv4), (dst_ipu)->ipu_ipv6)

#define ipUnionApplyMaskV6(ipu, v6_mask)                                \
    do {                                                                \
        int ipuam;                                                      \
        for (ipuam = 0; ipuam < 16; ++ipuam) {                          \
            (ipu)->ipu_ipv6[ipuam] &= ((int8_t*)(v6_mask))[ipuam];      \
        }                                                               \
    } while(0)


/* Get the 'cidr' most significant bits of the V6 address */
#define ipUnionGetCIDRV6(ipu, out_vp, cidr)                             \
    if ((cidr) >= 128) {                                                \
        ipUnionGetV6((ipu), (out_vp));                                  \
    } else {                                                            \
        int ipugc6 = (cidr) >> 3;                                       \
        memcpy((out_vp), (ipu)->ipu_ipv6, ipugc6);                      \
        ((uint8_t*)(out_vp))[ipugc6] = ((ipu)->ipu_ipv6[ipugc6]         \
                                        & ~(0xFF >> (0x7 & (cidr))));   \
        memset(&((uint8_t*)(out_vp))[ipugc6+1], 0, 15 - ipugc6);        \
    }

/* Set the V6 address to its 'cidr' most significant bits.  assumes
 * the following: 0 <= cidr < 128 */
#define ipUnionApplyCIDRV6(ipu, cidr)                           \
    do {                                                        \
        int ipugc6 = (cidr) >> 3;                               \
        (ipu)->ipu_ipv6[ipugc6] &= ~(0xFF >> (0x7 & (cidr)));   \
        memset(&(ipu)->ipu_ipv6[ipugc6+1], 0, 15 - ipugc6);     \
    } while(0)
#endif /* SK_ENABLE_IPV6 */


/* ****  skipaddr_t  **** */

/*
 *  is_v6 = skipaddrIsV6(ipaddr).
 *
 *    Return 1 if the skipaddr_t 'ipaddr' is an IPv6 address.  Return
 *    0 otherwise.
 */
#if !SK_ENABLE_IPV6
#  define skipaddrIsV6(addr)   0
#else
#  define skipaddrIsV6(addr)   ((addr)->ip_is_v6)
#endif


/*
 *  skipaddrSetVersion(addr, is_v6);
 *
 *    If 'is_v6' is non-zero, specify that the skipaddr_t 'addr'
 *    contains an IPv6 address.  This does not modify the
 *    representation of the IP address.  See also skipaddrV4toV6().
 */
#if !SK_ENABLE_IPV6
#  define skipaddrSetVersion(addr, is_v6)
#else
#  define skipaddrSetVersion(addr, is_v6)     \
    do { (addr)->ip_is_v6 = !!(is_v6); } while(0)
#endif


/*
 *  skipaddrCopy(dst, src);
 *
 *    Copy the skipaddr_t pointed at by 'src' to the location of the
 *    skipaddr_t pointed at by 'dst'.
 */
#if !SK_ENABLE_IPV6
#  define skipaddrCopy(dst, src)                                \
    do { skipaddrGetV4(dst) = skipaddrGetV4(src); } while(0)
#else
#  define skipaddrCopy(dst, src)   memcpy((dst), (src), sizeof(skipaddr_t))
#endif


/*
 *  skipaddrClear(addr);
 *
 *    Set all bits in the skipaddr_t pointed at by 'addr' to 0.  This
 *    causes 'addr' to represent the IPv4 address 0.0.0.0.
 */
#if !SK_ENABLE_IPV6
#  define skipaddrClear(addr)   do { skipaddrGetV4(addr) = 0; } while(0)
#else
#  define skipaddrClear(addr)   memset((addr), 0, sizeof(skipaddr_t))
#endif


/*
 *  ipv4 = skipaddrGetV4(addr);
 *
 *    Treat the skipaddr_t 'addr' as containing an IPv4 address and
 *    return that value in native (host) byte order.  To properly
 *    handle the cases where 'addr' contains an IPv6 address, use
 *    skipaddrGetAsV4().
 */
#define skipaddrGetV4(addr)    (ipUnionGetV4(&((addr)->ip_ip)))


/*
 *  skipaddrSetV4(addr, src);
 *
 *    Copy the uint32_t refereneced by 'src' into the skipaddr_t
 *    'addr' and set the 'addr' as containing an IPv4 address.  'src'
 *    should be in native (host) byte order.
 */
#if !SK_ENABLE_IPV6
#  define skipaddrSetV4(addr, in_vp)            \
    ipUnionSetV4(&((addr)->ip_ip), in_vp)
#else
#define skipaddrSetV4(addr, in_vp)                                      \
    do {                                                                \
        skipaddrClear(addr);                                            \
        ipUnionSetV4(&((addr)->ip_ip), (in_vp));                        \
    } while(0)
#endif


#if SK_ENABLE_IPV6

/*
 *  skipaddrGetAsV6(addr, dst);
 *
 *    Copy an IPv6 representation of the skipaddr_t 'addr' to the
 *    uint8_t[16] referenced by 'dst'.  If 'addr' contains an IPv4
 *    address, the result will contain an IPv6 representation of the
 *    address.
 */
#define skipaddrGetAsV6(addr, out_vp)                        \
    if (skipaddrIsV6(addr)) {                                \
        ipUnionGetV6(&((addr)->ip_ip), (out_vp));            \
    } else {                                                 \
        ipUnionGetV4AsV6(&((addr)->ip_ip), (out_vp));        \
    }


/*
 *  skipaddrGetV6(addr, dst);
 *
 *    Treat 'addr' as containing an IPv6 address and copy that value
 *    into the uint8_t[16] refereneced by 'dst'.  To properly handle
 *    the cases where 'addr' contains an IPv4 address, use
 *    skipaddrGetAsV6().
 */
#define skipaddrGetV6(addr, out_vp)             \
    ipUnionGetV6(&((addr)->ip_ip), (out_vp))


/*
 *  skipaddrSetV6(addr, src);
 *
 *    Copy the uint8_t[16] refereneced by 'src' into the skipaddr_t
 *    'addr' and set the 'addr' as containing an IPv6 address.
 */
#define skipaddrSetV6(addr, in_vp)                                      \
    do {                                                                \
        ipUnionSetV6(&((addr)->ip_ip), (in_vp));                        \
        (addr)->ip_is_v6 = 1;                                           \
    } while(0)


/*
 *  skipaddrSetV6FromUint32(addr, src);
 *
 *    Treat the uint32_t refereneced by 'src' as an IPv4 address in
 *    native (host) byte order, convert it to an IPv6 address, and
 *    store the result in skipaddr_t 'addr'.
 */
#define skipaddrSetV6FromUint32(addr, in_vp)            \
    do {                                                \
        uint8_t v6fromu32[16];                          \
        ipUnionU32ToV6((in_vp), v6fromu32);             \
        ipUnionSetV6(&((addr)->ip_ip), v6fromu32);      \
        (addr)->ip_is_v6 = 1;                           \
    } while(0)

/*
 *  skipaddrV4toV6(srcaddr, dstaddr);
 *
 *    Assume the skipaddr_t 'srcaddr' contains an IPv4 address,
 *    convert that address to an IPv6 address, and store the result in
 *    the skipaddr_t 'dstaddr'. 'srcaddr' and 'dstaddr' may point to
 *    the same skipaddr_t.
 */
#define skipaddrV4toV6(srcaddr, dstaddr)                              \
    do {                                                              \
        ipUnion4to6(&((srcaddr)->ip_ip), &((dstaddr)->ip_ip));        \
        (dstaddr)->ip_is_v6 = 1;                                      \
    } while(0)


/*
 *  ok = skipaddrV6toV4(srcaddr, dstaddr);
 *
 *    Assume the skipaddr_t 'srcaddr' contains an IPv6 address. If
 *    that address is an IPv6-representation of an IPv4 address,
 *    convert the address to IPv4, store the result in the skipaddr_t
 *    'dstaddr', and return 0.  Otherwise, return -1 and leave
 *    'dstaddr' unchanged. 'srcaddr' and 'dstaddr' may point to the
 *    same skipaddr_t.
 */
int
skipaddrV6toV4(
    const skipaddr_t   *srcaddr,
    skipaddr_t         *dstaddr);
#endif /* SK_ENABLE_IPV6 */


/*
 *  cmp = skipaddrCompare(addr1, addr2);
 *
 *    Compare the value of the skipaddr_t objects 'addr1' and 'addr2'.
 *
 *    Return 0 if 'addr1' is equal to 'addr2'; return a value less
 *    than 0 if 'addr1' is less than 'addr2'; return a value greater
 *    than 0 if 'addr1' is greater than 'addr2'.
 *
 *    When IPv6 is enabled and either address is IPv6, the comparison
 *    is done as if both addresses were IPv6.
 */
#if !SK_ENABLE_IPV6
#  define skipaddrCompare(addr1, addr2)                         \
    (((addr1)->ip_ip.ipu_ipv4 < (addr2)->ip_ip.ipu_ipv4)        \
     ? -1                                                       \
     : (((addr1)->ip_ip.ipu_ipv4 > (addr2)->ip_ip.ipu_ipv4)     \
        ? 1                                                     \
        : 0))
#else
int
skipaddrCompare(
    const skipaddr_t   *addr1,
    const skipaddr_t   *addr2);
#endif /* SK_ENABLE_IPV6 */


/*
 *  skipaddrMask(ipaddr, mask_ip);
 *
 *    Apply the bit-mask in the skipaddr_t 'mask_ip' to the skipaddr_t
 *    'ipaddr'.
 *
 *    When both addresses are either IPv4 or IPv6, applying the mask
 *    is straightforward.
 *
 *    If 'ipaddr' is IPv6 but 'mask_ip' is IPv4, 'mask_ip' is
 *    converted to IPv6 and then the mask is applied.  This may result
 *    in an odd result.
 *
 *    If 'ipaddr' is IPv4 and 'mask_ip' is IPv6, 'ipaddr' will remain
 *    an IPv4 address if masking 'mask_ip' with 'sk_ipv6_v4inv6'
 *    results in 'sk_ipv6_v4inv6' (namely, if bytes 10 and 11 of
 *    'mask_ip' are 0xFFFF).  Otherwise, 'ipaddr' is converted to an
 *    IPv6 address and the mask is performed in IPv6 space, which may
 *    result in an odd result.
 */
#if !SK_ENABLE_IPV6
#  define skipaddrMask(ipaddr, mask_ip)                         \
    do {                                                        \
        (ipaddr)->ip_ip.ipu_ipv4 &= (mask_ip)->ip_ip.ipu_ipv4;  \
    } while(0)
#else
void
skipaddrMask(
    skipaddr_t         *ipaddr,
    const skipaddr_t   *mask_ip);
#endif /* SK_ENABLE_IPV6 */


/*
 *  skipaddrApplyCIDR(ipaddr, cidr_prefix);
 *
 *    Apply the numeric CIDR prefix 'cidr_prefix' to the skipaddr_t
 *    'ipaddr', zeroing all but the most significant 'cidr_prefix'
 *    bits.
 *
 *    If a CIDR prefix too large for the address is given, it will be
 *    ignored.
 */
#if !SK_ENABLE_IPV6
#  define skipaddrApplyCIDR(ipaddr, cidr)               \
    if ((cidr) >= 32) { /* no-op */ } else {            \
        ipUnionApplyCIDRV4(&(ipaddr)->ip_ip, cidr);     \
    }
#else
#  define skipaddrApplyCIDR(ipaddr, cidr)                       \
    if (skipaddrIsV6(ipaddr)) {                                 \
        if ((cidr) < 128) {                                     \
            ipUnionApplyCIDRV6(&((ipaddr)->ip_ip), cidr);       \
        }                                                       \
    } else {                                                    \
        if ((cidr) < 32) {                                      \
            ipUnionApplyCIDRV4(&((ipaddr)->ip_ip), cidr);       \
        }                                                       \
    }
#endif

/*
 *  ok = skipaddrGetAsV4(addr, &ipv4);
 *
 *    If the skipaddr_t 'addr' contains an IPv4 address or an
 *    IPv6-representation of an IPv4 address, set the value pointed at
 *    by the uint32_t 'ipv4' to the IPv4 address (in native (host)
 *    byte order) and return 0.  Otherwise leave the value pointed at
 *    by 'ipv4' unchanged and return -1.
 */
#if !SK_ENABLE_IPV6
/* use comma expression so expression evaluates to 0 */
#  define skipaddrGetAsV4(addr, ipv4_ptr)       \
    ((*(ipv4_ptr) = skipaddrGetV4(addr)), 0)
#else
int
skipaddrGetAsV4(
    const skipaddr_t   *addr,
    uint32_t           *ipv4);
#endif


/*
 *  skipaddrIncrement(ipaddr);
 *
 *    Add one to the integer representation of the IP address in the
 *    skipaddr_t 'ipaddr'.  If overflow occurs, wrap the value back to
 *    0.
 *
 *
 *  skipaddrDecrement(ipaddr);
 *
 *    Subtract one from the integer representation of the IP address
 *    in the skipaddr_t 'ipaddr'.  If underflow occurs, wrap the value
 *    back to the maximum.
 */
#if !SK_ENABLE_IPV6
#define skipaddrIncrement(addr)                 \
    ((void)(++(addr)->ip_ip.ipu_ipv4))
#define skipaddrDecrement(addr)                 \
    ((void)(--(addr)->ip_ip.ipu_ipv4))
#else
#define skipaddrIncrement(addr)                                         \
    if (!skipaddrIsV6(addr)) {                                          \
        ++(addr)->ip_ip.ipu_ipv4;                                       \
    } else {                                                            \
        int incr_idx;                                                   \
        for (incr_idx = 15; incr_idx >= 0; --incr_idx) {                \
            if (UINT8_MAX != (addr)->ip_ip.ipu_ipv6[incr_idx]) {        \
                ++(addr)->ip_ip.ipu_ipv6[incr_idx];                     \
                break;                                                  \
            }                                                           \
            (addr)->ip_ip.ipu_ipv6[incr_idx] = 0;                       \
        }                                                               \
    }
#define skipaddrDecrement(addr)                                         \
    if (!skipaddrIsV6(addr)) {                                          \
        --(addr)->ip_ip.ipu_ipv4;                                       \
    } else {                                                            \
        int decr_idx;                                                   \
        for (decr_idx = 15; decr_idx >= 0; --decr_idx) {                \
            if (0 != (addr)->ip_ip.ipu_ipv6[decr_idx]) {                \
                --(addr)->ip_ip.ipu_ipv6[decr_idx];                     \
                break;                                                  \
            }                                                           \
            (addr)->ip_ip.ipu_ipv6[decr_idx] = UINT8_MAX;               \
        }                                                               \
    }
#endif  /* SK_ENABLE_IPV6 */


/*
 *  is_zero = skipaddrIsZero(ipaddr);
 *
 *    Return 1 if the IP address in the skipaddr_t 'ipaddr' contains
 *    no high bits.  Return 0 otherwise.
 *
 *    skipaddrIsZero(skipaddrClear(ipaddr)) returns 1.
 */
#if !SK_ENABLE_IPV6
#  define skipaddrIsZero(addr) (0 == (addr)->ip_ip.ipu_ipv4)
#else
#  define skipaddrIsZero(addr)                                          \
    (skipaddrIsV6(addr)                                                 \
     ? SK_IPV6_IS_ZERO((addr)->ip_ip.ipu_ipv6)                          \
     : (0 == (addr)->ip_ip.ipu_ipv4))
#endif


/* An skcidr_t holds an IP address and the number of subnet bits */
typedef union skcidr_un {
    struct cidr_un_v4 {
        /* whether this value contains an IPv6 mask */
        uint8_t  is_ipv6;
        /* length of the subnet (in bits). */
        uint8_t  cidr_length;
        /* placeholders for alignment */
        uint8_t  unused2;
        uint8_t  unused3;
        /* the base IP of the CIDR block */
        uint32_t ip;
        /* pre-computed mask where the upper length bits are high */
        uint32_t mask;
    } v4;
#if SK_ENABLE_IPV6
    struct cidr_un_v6 {
        /* whether this value contains an IPv6 mask */
        uint8_t  is_ipv6;
        /* length of the subnet (in bits). */
        uint8_t  cidr_length;
        /* number of bytes to memcmp() when comparing IP to CIDR */
        uint8_t  byte_length;
        /* pre-computed mask to use when comparing the
         * "ip[byte_length-1]" byte */
        uint8_t  mask;
        /* the base IP of the CIDR block */
        uint8_t  ip[16];
    } v6;
#endif  /* SK_ENABLE_IPV6 */
} skcidr_t;


/*
 *  in_cidr = skcidrCheckIP(cidr, ipaddr);
 *
 *    Return a true value if 'ipaddr' is contained in the CIDR block
 *    represented by 'cidr'.  Return false otherwise.
 */
#if !SK_ENABLE_IPV6
#define skcidrCheckIP(cidr, ipaddr)                             \
    ((skipaddrGetV4(ipaddr) & cidr->v4.mask) == (cidr)->v4.ip)
#else
int
skcidrCheckIP(
    const skcidr_t     *cidr,
    const skipaddr_t   *ipaddr);
#endif


/*
 *  skcidrClear(cidr);
 *
 *    Set all bits in the skcidr_t pointed at by 'cidr' to 0.
 */
#define skcidrClear(cidr)           memset(cidr, 0, sizeof(skcidr_t))


/*
 *  skcidrGetIPAddr(cidr, ipaddr);
 *
 *    Fill 'ipaddr' with the IP address contained by 'cidr'.
 */
void
skcidrGetIPAddr(
    const skcidr_t     *cidr,
    skipaddr_t         *ipaddr);


/*
 *  len = skcidrGetLength(cidr);
 *
 *    Return the length of the subnet represented by 'cidr'.
 */
#define skcidrGetLength(cidr)       ((cidr)->v4.cidr_length)


/*
 *  skcidrIsV6(cidr);
 *
 *    Return 1 if the skcidr_t pointed at by 'cidr' contains IPv6
 *    data.
 */
#if !SK_ENABLE_IPV6
#  define skcidrIsV6(cidr)          (0)
#else
#  define skcidrIsV6(cidr)          ((cidr)->v4.is_ipv6)
#endif


/*
 *  ok = skcidrSetFromIPAddr(cidr, ipaddr, length);
 *
 *    Set 'cidr' to the CIDR block that represents a subnet of
 *    'length' bits that has the IP in 'ipaddr' as its base.  Returns
 *    -1 if 'length' is too long for the given 'ipaddr'; returns 0
 *    otherwise.
 */
int
skcidrSetFromIPAddr(
    skcidr_t           *cidr,
    const skipaddr_t   *ipaddr,
    uint32_t            cidr_len);


/*
 *  ok = skcidrSetV4(cidr, ipv4, length);
 *
 *    Similar to skcidrSetFromIPAddr(), except takes an integer
 *    representation of an IPv4 address.
 */
int
skcidrSetV4(
    skcidr_t           *cidr,
    uint32_t            ipv4,
    uint32_t            cidr_len);

#if SK_ENABLE_IPV6
/*
 *  ok = skcidrSetV6(cidr, ipv6, length);
 *
 *    Similar to skcidrSetFromIPAddr(), except takes an array
 *    representation of an IPv6 address.
 */
int
skcidrSetV6(
    skcidr_t           *cidr,
    const uint8_t      *ipv6,
    uint32_t            cidr_len);
#endif


/*
 *  ok = skipaddrToSockaddr(dest, len, src);
 *
 *    Clears 'dest', and then sets the family and address of 'dest'
 *    from 'src'.  'len' is the length of 'dest'.  Returns -1 if 'len'
 *    is too small, 0 otherwise.
 */
int
skipaddrToSockaddr(
    struct sockaddr    *dest,
    size_t              len,
    const skipaddr_t   *src);

/*
 *  ok = skipaddrFromSockaddr(dest, src)
 *
 *    Sets 'dest' to the address in 'src'.  Returns -1 if 'src' does
 *    not represent an IP address, 0 otherwise.
 */
int
skipaddrFromSockaddr(
    skipaddr_t             *dest,
    const struct sockaddr  *src);


#ifdef __cplusplus
}
#endif
#endif /* _SKIPADDR_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
