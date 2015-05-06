/*
** Copyright (C) 2007-2015 by Carnegie Mellon University.
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
**  Miscellaneous functions for dealing with IP addresses.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: sku-ips.c 3d95ecc1dbb2 2015-02-13 19:50:40Z mthomas $");

#include <silk/skipaddr.h>
#include <silk/utils.h>


/* TYPEDEFS AND MACROS */

#define SILK_IPV6_POLICY_ENVAR  "SILK_IPV6_POLICY"


/* PUBLIC CONSTANTS USED IN MACROS */

const uint8_t sk_ipv6_zero[SK_IPV6_ZERO_LEN] =
    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
const uint8_t sk_ipv6_v4inv6[SK_IPV6_V4INV6_LEN] =
    {0,0,0,0, 0,0,0,0, 0,0,0xFF,0xFF};


/* LOCAL VARIABLES */

#if SK_ENABLE_IPV6
static const uint8_t max_ip6[16] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
#endif

/* masks of various sizes used when computing CIDR blocks */
static const uint32_t bitmask[] = {
    /*  0- 3 */ 0xffffffff, 0x7fffffff, 0x3fffffff, 0x1fffffff,
    /*  4- 7 */  0xfffffff,  0x7ffffff,  0x3ffffff,  0x1ffffff,
    /*  8-11 */   0xffffff,   0x7fffff,   0x3fffff,   0x1fffff,
    /* 12-15 */    0xfffff,    0x7ffff,    0x3ffff,    0x1ffff,
    /* 16-19 */     0xffff,     0x7fff,     0x3fff,     0x1fff,
    /* 20-23 */      0xfff,      0x7ff,      0x3ff,      0x1ff,
    /* 24-27 */       0xff,       0x7f,       0x3f,       0x1f,
    /* 28-31 */        0xf,        0x7,        0x3,        0x1,
    /* 32    */        0x0
};


/* this is used to find the log-base-2 of the CIDR block difference.
 * http://graphics.stanford.edu/~seander/bithacks.html */
static const uint8_t log_table_256[256] =
{
    /*   0- 15 */  0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    /*  16- 31 */  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    /*  32- 47 */  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    /*  48- 63 */  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    /*  64- 79 */  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    /*  80- 95 */  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    /*  96-111 */  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    /* 112-127 */  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    /* 128-143 */  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 144-159 */  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 160-175 */  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 176-191 */  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 192-207 */  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 208-223 */  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 224-239 */  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 240-255 */  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};



/* FUNCTION DEFINITIONS */


/* compute the log2() of 'value' */
int
skIntegerLog2(
    uint64_t            value)
{
    uint64_t tmp1;
    uint64_t tmp2;

    /* The comments use 63 for most significant bit and 0 for LSB */

    if ((tmp1 = value >> 32)) {          /* MSB is in bits 63-32 */
        if ((tmp2 = tmp1 >> 16)) {       /* MSB is in bits 63-48 */
            if ((tmp1 = tmp2 >> 8)) {    /* MSB is in bits 63-56 */
                return 56 + log_table_256[tmp1];
            } else {                     /* MSB is in bits 55-48 */
                return 48 + log_table_256[tmp2];
            }
        } else {                         /* MSB is in bits 47-32 */
            if ((tmp2 = tmp1 >> 8)) {    /* MSB is in bits 47-40 */
                return 40 + log_table_256[tmp2];
            } else {                     /* MSB is in bits 39-32 */
                return 32 + log_table_256[tmp1];
            }
        }
    } else {                             /* MSB is in bits 31- 0 */
        if ((tmp1 = value >> 16)) {    /* MSB is in bits 31-16 */
            if ((tmp2 = tmp1 >> 8)) {    /* MSB is in bits 31-24 */
                return 24 + log_table_256[tmp2];
            } else {                     /* MSB is in bits 23-16 */
                return 16 + log_table_256[tmp1];
            }
        } else {                         /* MSB is in bits 15- 0 */
            if ((tmp1 = value >> 8)) { /* MSB is in bits 15- 8 */
                return 8 + log_table_256[tmp1];
            } else {                     /* MSB is in bits  7- 0 */
                return log_table_256[value];
            }
        }
    }
}


/* compute a CIDR block */
int
skComputeCIDR(
    uint32_t            start_ip,
    uint32_t            end_ip,
    uint32_t           *new_start_ip)
{
    skipaddr_t start_addr;
    skipaddr_t end_addr;
    skipaddr_t new_start_addr;
    int prefix;

    skipaddrSetV4(&start_addr, &start_ip);
    skipaddrSetV4(&end_addr, &end_ip);
    if (new_start_ip) {
        prefix = skCIDRComputePrefix(&start_addr, &end_addr, &new_start_addr);
        if (-1 != prefix) {
            *new_start_ip = skipaddrGetV4(&new_start_addr);
        }
    } else {
        prefix = skCIDRComputePrefix(&start_addr, &end_addr, NULL);
    }

    return prefix;
}


/* compute the prefix to hold 'start_addr' through 'end_addr' */
int
skCIDRComputePrefix(
    const skipaddr_t   *start_addr,
    const skipaddr_t   *end_addr,
    skipaddr_t         *new_start_addr)
{
    int prefix = -1;

#if SK_ENABLE_IPV6
    if (skipaddrIsV6(start_addr) || skipaddrIsV6(end_addr)) {
        uint8_t start_ip6[16];
        uint8_t end_ip6[16];
        uint8_t range_start[16];
        int i;
        int tmp1;
        uint8_t tmp2 = 0;

        tmp1 = skipaddrCompare(start_addr, end_addr);
        if (tmp1 > 0) {
            /* bad range, start_addr > end_addr */
            return -1;
        }

        /* handle a range that contains a single IP */
        if (tmp1 == 0) {
            if (new_start_addr) {
                skipaddrClear(new_start_addr);
            }
            return 128;
        }

        skipaddrGetAsV6(start_addr, start_ip6);

        /* handle an odd start_addr */
        if (start_ip6[15] & 0x1) {
            if (new_start_addr) {
                skipaddrCopy(new_start_addr, start_addr);
                skipaddrIncrement(new_start_addr);
            }
            return 128;
        }

        skipaddrGetAsV6(end_addr, end_ip6);
        memset(range_start, 0, sizeof(range_start));

        /* find the most significant bit where start_addr and end_addr
         * differ */
        for (i = 0; i < 16; ++i) {
            if (start_ip6[i] != end_ip6[i]) {
                /* copy first i+1 bytes of start_ip6 into range_start */
                memcpy(range_start, start_ip6, i+1);

                /* find number of IPs in the range */
                tmp1 = end_ip6[i] - start_ip6[i];

                /* see if we need to add one */
                if ((i == 15)
                    || ((0 == memcmp(start_ip6 + i + 1, sk_ipv6_zero, (15-i)))
                        && (0 == memcmp(end_ip6 + i + 1, max_ip6, (15-i)))))
                {
                    ++tmp1;
                }
                if (256 == tmp1) {
                    tmp2 = 8;
                    prefix = 8 * i;
                } else {
                    tmp2 = log_table_256[tmp1];
                    prefix = 8 * (i + 1) - tmp2;
                    range_start[i] &= (0xFF << tmp2);
                }
                break;
            }
        }

        while (memcmp(range_start, start_ip6, sizeof(range_start)) < 0) {
            ++prefix;
            if (tmp2 != 0) {
                --tmp2;
            } else {
                ++i;
                tmp2 = 7;
            }
            range_start[i] = start_ip6[i] & (0xFF << tmp2);
        }

        if (new_start_addr) {
            /* compute the start of the next CIDR block, which is the
             * IP after the block we just finished.  In the case of
             * roll-over, the 'else' clause will be invoked, and we
             * will return 0. */
            range_start[i] |= ~(0xFF << tmp2);
            ++i;
            memset(range_start + i, 0xFF, (sizeof(range_start) - i));
            if (0 == memcmp(range_start, end_ip6, sizeof(range_start))) {
                skipaddrClear(new_start_addr);
            } else {
                skipaddrSetV6(new_start_addr, range_start);
                skipaddrIncrement(new_start_addr);
            }
        }

        return prefix;

    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        uint32_t start_ip4 = skipaddrGetV4(start_addr);
        uint32_t end_ip4 = skipaddrGetV4(end_addr);
        uint32_t range_start;

        if (end_ip4 < start_ip4) {
            return -1;
        }

        /* handle a range that contains a single IP */
        if (end_ip4 == start_ip4) {
            if (new_start_addr) {
                skipaddrClear(new_start_addr);
            }
            return 32;
        }
        /* handle an odd start_addr */
        if (start_ip4 & 0x1) {
            if (new_start_addr) {
                ++start_ip4;
                skipaddrSetV4(new_start_addr, &start_ip4);
            }
            return 32;
        }

        /* compute the log-base-2 (position of most significant bit)
         * of the number of IPs in the range, and subtract that from
         * 32 to get the widest possible CIDR block */
        prefix = 32 - skIntegerLog2(UINT64_C(1) + end_ip4 - start_ip4);

        /* tighten the range if we need to, in case the IPs don't fall
         * into a single CIDR block (e.g., 10.0.0.6--10.0.0.9) */
        while ((range_start = (start_ip4 & ~(bitmask[prefix]))) < start_ip4) {
            ++prefix;
        }

        /* assert that the CIDR block is within the limits */
        assert(range_start == start_ip4);
        assert((range_start | (bitmask[prefix])) <= end_ip4);

        if (new_start_addr) {
            /* compute the start of the next CIDR block, which is the
             * IP after the block we just finished.  In the case of
             * roll-over, the 'else' clause will be invoked, and we
             * will return 0. */
            start_ip4 = 1 + (range_start | (bitmask[prefix]));
            if (start_ip4 > end_ip4) {
                skipaddrClear(new_start_addr);
            } else {
                skipaddrSetV4(new_start_addr, &start_ip4);
            }
        }
    }

    return prefix;
}


int
skCIDR2IPRange(
    const skipaddr_t   *ipaddr,
    uint32_t            cidr,
    skipaddr_t         *min_ip,
    skipaddr_t         *max_ip)
{
    uint32_t ip4;

#if SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr)) {
        uint8_t ip6[16];
        uint32_t i;

        if (cidr == 128) {
            /* don't use skipaddrCopy() in case caller supplied
             * pointers are the same */
            skipaddrGetV6(ipaddr, ip6);
            skipaddrSetV6(min_ip, ip6);
            skipaddrSetV6(max_ip, ip6);
            return 0;
        }

        if (cidr > 128) {
            return -1;
        }

        skipaddrGetV6(ipaddr, ip6);

        /* first byte of address where there is any effect */
        i = cidr >> 3;

        /* handle max: apply mask to this byte, remaining bits all 1 */
        ip6[i] |= (0xFF >> (cidr & 0x07));
        memset(&ip6[i+1], 0xFF, (15 - i));
        skipaddrSetV6(max_ip, ip6);

        /* handle min: apply mask, remaining bits all 0 */
        ip6[i] &= ~(0xFF >> (cidr & 0x07));
        memset(&ip6[i+1], 0, (15 - i));
        skipaddrSetV6(min_ip, ip6);

        return 0;
    }
#endif /* SK_ENABLE_IPV6 */

    if (cidr == 32) {
        ip4 = skipaddrGetV4(ipaddr);
        skipaddrSetV4(min_ip, &ip4);
        skipaddrSetV4(max_ip, &ip4);
        return 0;
    }

    if (cidr > 32) {
        return -1;
    }

    /* handle max IP */
    ip4 = (UINT32_MAX >> cidr) | skipaddrGetV4(ipaddr);
    skipaddrSetV4(max_ip, &ip4);

    /* handle min IP */
    ip4 &= ~(UINT32_MAX >> cidr);
    skipaddrSetV4(min_ip, &ip4);

    return 0;
}


int
skCIDRComputeStart(
    const skipaddr_t   *ipaddr,
    uint32_t            cidr,
    skipaddr_t         *min_ip)
{
    uint32_t ip4;

#if SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr)) {
        uint8_t ip6[16];
        uint32_t i;

        if (cidr == 128) {
            if (ipaddr != min_ip) {
                skipaddrCopy(min_ip, ipaddr);
            }
            return 0;
        }

        if (cidr > 128) {
            return -1;
        }

        skipaddrGetV6(ipaddr, ip6);

        /* first byte of address where there is any effect */
        i = cidr >> 3;

        /* handle min: apply mask to this byte, remaining bits all 0 */
        ip6[i] &= ~(0xFF >> (cidr & 0x07));
        memset(&ip6[i+1], 0, (15 - i));
        skipaddrSetV6(min_ip, ip6);

        return 0;
    }
#endif /* SK_ENABLE_IPV6 */

    if (cidr == 32) {
        ip4 = skipaddrGetV4(ipaddr);
        skipaddrSetV4(min_ip, &ip4);
        return 0;
    }

    if (cidr > 32) {
        return -1;
    }

    /* handle min IP */
    ip4 = ~(UINT32_MAX >> cidr) & skipaddrGetV4(ipaddr);
    skipaddrSetV4(min_ip, &ip4);

    return 0;
}


int
skCIDRComputeEnd(
    const skipaddr_t   *ipaddr,
    uint32_t            cidr,
    skipaddr_t         *max_ip)
{
    uint32_t ip4;

#if SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr)) {
        uint8_t ip6[16];
        uint32_t i;

        if (cidr == 128) {
            if (ipaddr != max_ip) {
                skipaddrCopy(max_ip, ipaddr);
            }
            return 0;
        }

        if (cidr > 128) {
            return -1;
        }

        skipaddrGetV6(ipaddr, ip6);

        /* first byte of address where there is any effect */
        i = cidr >> 3;

        /* handle max: apply mask to this byte, remaining bits all 1 */
        ip6[i] |= (0xFF >> (cidr & 0x07));
        memset(&ip6[i+1], 0xFF, (15 - i));
        skipaddrSetV6(max_ip, ip6);

        return 0;
    }
#endif /* SK_ENABLE_IPV6 */

    if (cidr == 32) {
        ip4 = skipaddrGetV4(ipaddr);
        skipaddrSetV4(max_ip, &ip4);
        return 0;
    }

    if (cidr > 32) {
        return -1;
    }

    /* handle max IP */
    ip4 = (UINT32_MAX >> cidr) | skipaddrGetV4(ipaddr);
    skipaddrSetV4(max_ip, &ip4);

    return 0;
}


#if SK_ENABLE_IPV6
int
skipaddrV6toV4(
    const skipaddr_t   *srcaddr,
    skipaddr_t         *dstaddr)
{
    uint32_t ipv4;

    if (!SK_IPV6_IS_V4INV6(srcaddr->ip_ip.ipu_ipv6)) {
        return -1;
    }
    memcpy(&ipv4, &(srcaddr->ip_ip.ipu_ipv6[12]), 4);
    ipv4 = ntohl(ipv4);
    skipaddrSetV4(dstaddr, &ipv4);
    return 0;
}


int
skipaddrGetAsV4(
    const skipaddr_t   *ipaddr,
    uint32_t           *ipv4)
{
    if (skipaddrIsV6(ipaddr)) {
        if (!SK_IPV6_IS_V4INV6(ipaddr->ip_ip.ipu_ipv6)) {
            return -1;
        }
        memcpy(ipv4, &(ipaddr->ip_ip.ipu_ipv6[12]), 4);
        *ipv4 = ntohl(*ipv4);
    } else {
        *ipv4 = skipaddrGetV4(ipaddr);
    }
    return 0;
}
#endif  /* SK_ENABLE_IPV6 */


char *
skipaddrString(
    char               *outbuf,
    const skipaddr_t   *ip,
    uint32_t            ip_flags)
{
#if SK_ENABLE_IPV6
    if (skipaddrIsV6(ip)) {
        /* to represent a 128 bit number, divide the number into 4
         * 64-bit values where each value represents 10 decimal digits
         * of the number, with position 0 holding the least
         * significant bits. */
        static const uint64_t map_ipv6_to_dec[][4] =
            {{                   1,                    0,  0, 0}, /* 1 <<  0 */
             {                 256,                    0,  0, 0}, /* 1 <<  8 */
             {               65536,                    0,  0, 0}, /* 1 << 16 */
             {            16777216,                    0,  0, 0}, /* 1 << 24 */
             { UINT64_C(4294967296),                   0,  0, 0}, /* 1 << 32 */
             { UINT64_C(9511627776),                 109,  0, 0}, /* 1 << 40 */
             { UINT64_C(4976710656),               28147,  0, 0}, /* 1 << 48 */
             { UINT64_C(4037927936),             7205759,  0, 0}, /* 1 << 56 */
             { UINT64_C(3709551616), UINT64_C(1844674407), 0, 0}, /* 1 << 64 */
             { UINT64_C(9645213696), UINT64_C(2236648286),      47, 0},/* 72 */
             { UINT64_C(9174706176), UINT64_C(2581961462),   12089, 0},/* 80 */
             { UINT64_C(8724781056),  UINT64_C(982134506), 3094850, 0},/* 88 */
             { UINT64_C(3543950336), UINT64_C(1426433759),
               /* */           UINT64_C(792281625),          0}, /* 1 <<  96 */
             { UINT64_C(7251286016), UINT64_C(5167042394),
               /* */          UINT64_C(2824096036),         20}, /* 1 << 104 */
             { UINT64_C(6329220096), UINT64_C(2762853049),
               /* */          UINT64_C(2968585348),       5192}, /* 1 << 112 */
             {  UINT64_C(280344576), UINT64_C(7290380706),
                /* */         UINT64_C(9957849158),    1329227}  /* 1 << 120 */
            };
        /* our 10 decimal digits */
        static const uint64_t lim = UINT64_C(10000000000);
        /* the decimal value being calculated */
        uint64_t decimal[5] = {0, 0, 0, 0, 0};
        /* when doing our own IPv6 printing, this holds each 16bit
         * section of the address */
        uint16_t hexdec[8];
        /* following two values used when finding the longest run of
         * 0s in an IPv6 address */
        unsigned int longest_zero_pos;
        unsigned int longest_zero_len;
        uint64_t tmp;
        char tmpbuf[SK_NUM2DOT_STRLEN];
        char *pos;
        unsigned int len;
        unsigned int i, j, k;

        switch ((skipaddr_flags_t)ip_flags) {
          case SKIPADDR_CANONICAL:
#ifdef SK_HAVE_INET_NTOP
#  if    SK_NUM2DOT_STRLEN < INET6_ADDRSTRLEN
#    error "SK_NUM2DOT_STRLEN is not big enough"
#  endif
            if (NULL == inet_ntop(AF_INET6, &(ip->ip_ip.ipu_ipv6), outbuf,
                                  SK_NUM2DOT_STRLEN))
            {
                outbuf[0] = '\0';
            }
            break;
#endif /* SK_HAVE_INET_NTOP */

          case SKIPADDR_FORCE_IPV6:
            /* do our own IPV6 printing */
            if (skipaddrIsZero(ip)) {
                snprintf(outbuf, SK_NUM2DOT_STRLEN, "::");
                break;
            }
            /* compute each hexadectet */
            hexdec[0] = (ip->ip_ip.ipu_ipv6[ 0]<<8) | ip->ip_ip.ipu_ipv6[ 1];
            hexdec[1] = (ip->ip_ip.ipu_ipv6[ 2]<<8) | ip->ip_ip.ipu_ipv6[ 3];
            hexdec[2] = (ip->ip_ip.ipu_ipv6[ 4]<<8) | ip->ip_ip.ipu_ipv6[ 5];
            hexdec[3] = (ip->ip_ip.ipu_ipv6[ 6]<<8) | ip->ip_ip.ipu_ipv6[ 7];
            hexdec[4] = (ip->ip_ip.ipu_ipv6[ 8]<<8) | ip->ip_ip.ipu_ipv6[ 9];
            hexdec[5] = (ip->ip_ip.ipu_ipv6[10]<<8) | ip->ip_ip.ipu_ipv6[11];
            hexdec[6] = (ip->ip_ip.ipu_ipv6[12]<<8) | ip->ip_ip.ipu_ipv6[13];
            hexdec[7] = (ip->ip_ip.ipu_ipv6[14]<<8) | ip->ip_ip.ipu_ipv6[15];
            /* find the starting position and length of the longest
             * run of 0s */
            longest_zero_pos = 8;
            longest_zero_len = 0;
            i = 0;
            while (i < 8) {
                if (hexdec[i]) {
                    ++i;
                } else {
                    j = i;
                    for (++i; i < 8 && (0 == hexdec[i]); ++i){};/*empty*/
                    if ((i - j) > longest_zero_len) {
                        longest_zero_len = i - j;
                        longest_zero_pos = j;
                    }
                }
            }
            /* print the ip; we build the result in pieces */
            if (0 == longest_zero_len) {
                snprintf(outbuf, SK_NUM2DOT_STRLEN, "%x:%x:%x:%x:%x:%x:%x:%x",
                         hexdec[0], hexdec[1], hexdec[2], hexdec[3],
                         hexdec[4], hexdec[5], hexdec[6], hexdec[7]);
                break;
            }
            i = 0;
            pos = outbuf;
            len = SK_NUM2DOT_STRLEN;
            while (i < 8) {
                if (i == longest_zero_pos) {
                    i += longest_zero_len;
                    if (8 == i) {
                        j = snprintf(pos, len, "::");
                    } else {
                        j = snprintf(pos, len, ":");
                    }
                } else if (0 == i) {
                    j = snprintf(pos, len, "%x", hexdec[i]);
                    ++i;
                } else {
                    j = snprintf(pos, len, ":%x", hexdec[i]);
                    ++i;
                }
                if (j >= len) {
                    skAbort();
                }
                pos += j;
                len -= j;
            }
            break;

          case SKIPADDR_ZEROPAD:
            /* Convert integer 0 to string
               "0000:0000:0000:0000:0000:0000:0000:0000" */
            snprintf(outbuf, SK_NUM2DOT_STRLEN,
                     ("%02x%02x:%02x%02x:%02x%02x:%02x%02x"
                      ":%02x%02x:%02x%02x:%02x%02x:%02x%02x"),
                     ip->ip_ip.ipu_ipv6[ 0], ip->ip_ip.ipu_ipv6[ 1],
                     ip->ip_ip.ipu_ipv6[ 2], ip->ip_ip.ipu_ipv6[ 3],
                     ip->ip_ip.ipu_ipv6[ 4], ip->ip_ip.ipu_ipv6[ 5],
                     ip->ip_ip.ipu_ipv6[ 6], ip->ip_ip.ipu_ipv6[ 7],
                     ip->ip_ip.ipu_ipv6[ 8], ip->ip_ip.ipu_ipv6[ 9],
                     ip->ip_ip.ipu_ipv6[10], ip->ip_ip.ipu_ipv6[11],
                     ip->ip_ip.ipu_ipv6[12], ip->ip_ip.ipu_ipv6[13],
                     ip->ip_ip.ipu_ipv6[14], ip->ip_ip.ipu_ipv6[15]);
            break;

          case SKIPADDR_HEXADECIMAL:
            /* Fill buffer with a string representation of an integer
             * in hexadecimal format; this works by printing the value
             * then stripping leading 0's. */
            if (skipaddrIsZero(ip)) {
                snprintf(outbuf, SK_NUM2DOT_STRLEN, "0");
                break;
            }
            snprintf(tmpbuf, sizeof(tmpbuf),
                     ("%x%02x%02x%02x%02x%02x%02x%02x"
                      "%02x%02x%02x%02x%02x%02x%02x%02x"),
                     ip->ip_ip.ipu_ipv6[ 0], ip->ip_ip.ipu_ipv6[ 1],
                     ip->ip_ip.ipu_ipv6[ 2], ip->ip_ip.ipu_ipv6[ 3],
                     ip->ip_ip.ipu_ipv6[ 4], ip->ip_ip.ipu_ipv6[ 5],
                     ip->ip_ip.ipu_ipv6[ 6], ip->ip_ip.ipu_ipv6[ 7],
                     ip->ip_ip.ipu_ipv6[ 8], ip->ip_ip.ipu_ipv6[ 9],
                     ip->ip_ip.ipu_ipv6[10], ip->ip_ip.ipu_ipv6[11],
                     ip->ip_ip.ipu_ipv6[12], ip->ip_ip.ipu_ipv6[13],
                     ip->ip_ip.ipu_ipv6[14], ip->ip_ip.ipu_ipv6[15]);
            i = strspn(tmpbuf, "0");
            /* checked for an IP of 0 above, so 'i' must be on some
             * non-zero hex-digit */
            assert('\0' != tmpbuf[i]);
            strncpy(outbuf, &tmpbuf[i], SK_NUM2DOT_STRLEN);
            break;

          case SKIPADDR_DECIMAL:
            /* Multiply each octet by the value in the map_ipv6_to_dec
             * look-up table and create the value in 'decimal' */
            for (i = 0; i < 16; ++i) {
                if (ip->ip_ip.ipu_ipv6[15-i]) {
                    for (j = 0; j < 4 && map_ipv6_to_dec[i][j] > 0; ++j) {
                        decimal[j] +=
                            ip->ip_ip.ipu_ipv6[15-i] * map_ipv6_to_dec[i][j];
                        if (decimal[j] >= lim) {
                            /* handle overflow */
                            tmp = decimal[j];
                            decimal[j] %= lim;
                            decimal[j+1] += tmp / lim;
                        }
                    }
                }
            }
            /* Final check for overflow and determine number of
             * 'decimal' elements that have a value */
            i = 0;
            for (j = 0; j < 4; ++j) {
                if (decimal[j] >= lim) {
                    i = k = j;
                    do {
                        tmp = decimal[k];
                        decimal[k] %= lim;
                        ++k;
                        decimal[k] += tmp / lim;
                    } while (decimal[k] >= lim && k < 4);
                } else if (decimal[j] > 0) {
                    i = j;
                }
            }
            /* Print it */
            switch (i) {
              case 0:
                snprintf(outbuf, SK_NUM2DOT_STRLEN,
                         "%" PRIu64, decimal[0]);
                break;
              case 1:
                snprintf(outbuf, SK_NUM2DOT_STRLEN,
                         "%" PRIu64 "%010" PRIu64,
                         decimal[1], decimal[0]);
                break;
              case 2:
                snprintf(outbuf, SK_NUM2DOT_STRLEN,
                         "%" PRIu64 "%010" PRIu64 "%010" PRIu64,
                         decimal[2], decimal[1], decimal[0]);
                break;
              case 3:
                snprintf(outbuf, SK_NUM2DOT_STRLEN,
                         "%" PRIu64 "%010" PRIu64 "%010" PRIu64 "%010" PRIu64,
                         decimal[3], decimal[2], decimal[1], decimal[0]);
                break;
              case 4:
                snprintf(outbuf, SK_NUM2DOT_STRLEN,
                         ("%" PRIu64 "%010" PRIu64 "%010" PRIu64
                          "%010" PRIu64 "%010" PRIu64),
                         decimal[4], decimal[3], decimal[2],
                         decimal[1], decimal[0]);
                break;
              default:
                skAbortBadCase(i);
            }
            break;
        }
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        /* address is IPv4 */
        switch ((skipaddr_flags_t)ip_flags) {
          case SKIPADDR_CANONICAL:
            /* Convert integer 0 to string "0.0.0.0" */
            snprintf(outbuf, SK_NUM2DOT_STRLEN, "%lu.%lu.%lu.%lu",
                     (unsigned long)((ip->ip_ip.ipu_ipv4 >> 24) & 0xFF),
                     (unsigned long)((ip->ip_ip.ipu_ipv4 >> 16) & 0xFF),
                     (unsigned long)((ip->ip_ip.ipu_ipv4 >> 8) & 0xFF),
                     (unsigned long)(ip->ip_ip.ipu_ipv4 & 0xFF));
            break;

          case SKIPADDR_ZEROPAD:
            /* Convert integer 0 to string "000.000.000.000" */
            snprintf(outbuf, SK_NUM2DOT_STRLEN, "%03lu.%03lu.%03lu.%03lu",
                     (unsigned long)((ip->ip_ip.ipu_ipv4 >> 24) & 0xFF),
                     (unsigned long)((ip->ip_ip.ipu_ipv4 >> 16) & 0xFF),
                     (unsigned long)((ip->ip_ip.ipu_ipv4 >> 8) & 0xFF),
                     (unsigned long)(ip->ip_ip.ipu_ipv4 & 0xFF));
            break;

          case SKIPADDR_DECIMAL:
            snprintf(outbuf, SK_NUM2DOT_STRLEN, ("%" PRIu32),
                     ip->ip_ip.ipu_ipv4);
            break;

          case SKIPADDR_HEXADECIMAL:
            snprintf(outbuf, SK_NUM2DOT_STRLEN, ("%" PRIx32),
                     ip->ip_ip.ipu_ipv4);
            break;

          case SKIPADDR_FORCE_IPV6:
            snprintf(outbuf, SK_NUM2DOT_STRLEN, "::ffff:%x:%x",
                     ip->ip_ip.ipu_ipv4 >> 16, ip->ip_ip.ipu_ipv4 & 0xFFFF);
            break;
        }
    }

    outbuf[SK_NUM2DOT_STRLEN-1] = '\0';
    return outbuf;
}


static struct policies_st {
    sk_ipv6policy_t     policy;
    const char         *name;
    const char         *description;
} policies[] = {
    {SK_IPV6POLICY_IGNORE, "ignore",
     "Completely ignore IPv6 flows"},
    {SK_IPV6POLICY_ASV4,   "asv4",
     "Convert IPv6 flows to IPv4 if possible, else ignore"},
    {SK_IPV6POLICY_MIX,    "mix",
     "Process a mixture of IPv4 and IPv6 flows"},
    {SK_IPV6POLICY_FORCE,  "force",
     "Force IPv4 flows to be converted to IPv6"},
    {SK_IPV6POLICY_ONLY,   "only",
     "Only process flows that were marked as IPv6"}
};


/* Parse an IPv6 policy.  Return 0 if it is valid, -1 otherwise. */
int
skIPv6PolicyParse(
    sk_ipv6policy_t    *ipv6_policy,
    const char         *policy_name,
    const char         *option_name)
{
    size_t len = strlen(policy_name);
    size_t i;

    for (i = 0; i < sizeof(policies)/sizeof(struct policies_st); ++i) {
        if (len < strlen(policies[i].name)) {
            if (0 == strncmp(policies[i].name, policy_name, len)) {
                *ipv6_policy = policies[i].policy;
                return 0;
            }
        } else if (0 == strcmp(policies[i].name, policy_name)) {
            *ipv6_policy = policies[i].policy;
            return 0;
        }
    }

    if (option_name) {
        skAppPrintErr("Invalid %s '%s'", option_name, policy_name);
    }
    return -1;
}


/* store the default policy from the application */
static sk_ipv6policy_t ipv6_default;

/* support for the option */
#define OPT_IPV6_POLICY  0

static struct option ipv6_policy_options[] = {
    {"ipv6-policy",         REQUIRED_ARG, 0, OPT_IPV6_POLICY},
    {0,0,0,0}               /* sentinel */
};

/* handler for the option */
static int
ipv6PolicyHandler(
    clientData          cData,
    int                 opt_index,
    char               *opt_arg)
{
    sk_ipv6policy_t *ipv6_policy = (sk_ipv6policy_t*)cData;

    switch (opt_index) {
      case OPT_IPV6_POLICY:
        if (skIPv6PolicyParse(ipv6_policy, opt_arg,
                              ipv6_policy_options[opt_index].name))
        {
            return 1;
        }
        break;

      default:
        skAbortBadCase(opt_index);
    }

    return 0;
}


int
skIPv6PolicyOptionsRegister(
    sk_ipv6policy_t    *ipv6_policy)
{
    sk_ipv6policy_t tmp_policy;
    char *env;

    assert(ipv6_policy);

    /* store the default policy wanted by the application */
    ipv6_default = *ipv6_policy;

    /* get the default from the environment */
    env = getenv(SILK_IPV6_POLICY_ENVAR);
    if (env) {
        if (skIPv6PolicyParse(&tmp_policy, env, SILK_IPV6_POLICY_ENVAR) == 0) {
            *ipv6_policy = tmp_policy;
        }
    }

#if !SK_ENABLE_IPV6
    /* Force an IPv4-only SiLK to ignore any IPv6 flows */
    ipv6_default = SK_IPV6POLICY_IGNORE;
    *ipv6_policy = ipv6_default;

    /* Register the option for compatibility with an IPv6-enabled
     * silk, but pass 'ipv6_default' as the clientData, so the user's
     * value does not modify the value the application uses. */
    return skOptionsRegister(ipv6_policy_options, &ipv6PolicyHandler,
                             (clientData)&ipv6_default);
#else
    /* add the option */
    return skOptionsRegister(ipv6_policy_options, &ipv6PolicyHandler,
                             (clientData)ipv6_policy);
#endif  /* SK_ENABLE_IPV6 */
}


void
skIPv6PolicyUsage(
    FILE               *fh)
{
    size_t i;

    fprintf(fh, "--%s %s. ",
            ipv6_policy_options[OPT_IPV6_POLICY].name,
            SK_OPTION_HAS_ARG(ipv6_policy_options[OPT_IPV6_POLICY]));
#if !SK_ENABLE_IPV6
    fprintf(fh, ("No IPv6 support available; IPv6 flows are always ignored\n"
                 "\tregardless of the value passed to this switch."
                 " Legal values:\n"));
#else
    fprintf(fh, "Set policy for IPv4/IPv6 flows. ");
    for (i = 0; i < sizeof(policies)/sizeof(struct policies_st); ++i) {
        if (ipv6_default != policies[i].policy) {
            continue;
        }
        fprintf(fh, "Def. %s. ", policies[i].name);
        break;
    }
    fprintf(fh, "Choices:\n");
#endif
    for (i = 0; i < sizeof(policies)/sizeof(struct policies_st); ++i) {
        fprintf(fh, "\t%-6s  - %s\n",
                policies[i].name, policies[i].description);
    }
}


#if SK_ENABLE_IPV6
int
skipaddrCompare(
    const skipaddr_t   *addr1,
    const skipaddr_t   *addr2)
{
    skipaddr_t tmp;

    if (addr1->ip_is_v6) {
        if (addr2->ip_is_v6) {
            return memcmp(addr1->ip_ip.ipu_ipv6, addr2->ip_ip.ipu_ipv6, 16);
        }
        skipaddrV4toV6(addr2, &tmp);
        return memcmp(addr1->ip_ip.ipu_ipv6, tmp.ip_ip.ipu_ipv6, 16);
    }
    if (addr2->ip_is_v6) {
        skipaddrV4toV6(addr1, &tmp);
        return memcmp(tmp.ip_ip.ipu_ipv6, addr2->ip_ip.ipu_ipv6, 16);
    }
    /* both addresses are IPv4 */
    if (addr1->ip_ip.ipu_ipv4 < addr2->ip_ip.ipu_ipv4) {
        return -1;
    }
    if (addr1->ip_ip.ipu_ipv4 > addr2->ip_ip.ipu_ipv4) {
        return 1;
    }
    return 0;
}


/* apply the bit-mask in 'mask_ip' to 'ipaddr' */
void
skipaddrMask(
    skipaddr_t         *ipaddr,
    const skipaddr_t   *mask_ip)
{
    skipaddr_t tmp;

    if (ipaddr->ip_is_v6) {
        if (mask_ip->ip_is_v6) {
            /* both addresses are IPv6 */
            ipUnionApplyMaskV6(&ipaddr->ip_ip, mask_ip->ip_ip.ipu_ipv6);
            return;
        }
        /* convert mask to IPv6. This result will be strange */
        skipaddrV4toV6(mask_ip, &tmp);
        ipUnionApplyMaskV6(&ipaddr->ip_ip, tmp.ip_ip.ipu_ipv6);
        return;
    }
    if (mask_ip->ip_is_v6) {
        /* 'ipaddr' is IPv4 and 'mask_ip' is IPv6. if bytes 10 and 11
         * of 'mask_ip' are 0xFFFF, then an IPv4 address will result;
         * otherwise, we'll get something strange */
        if (0 == memcmp(&mask_ip->ip_ip.ipu_ipv6[10], &sk_ipv6_v4inv6[10], 2)){
            uint32_t mask_v4;
            memcpy(&mask_v4, &mask_ip->ip_ip.ipu_ipv6[12], 4);
            ipUnionApplyMaskV4(&ipaddr->ip_ip, ntohl(mask_v4));
            return;
        }
        skipaddrV4toV6(ipaddr, ipaddr);
        ipUnionApplyMaskV6(&ipaddr->ip_ip, mask_ip->ip_ip.ipu_ipv6);
        return;
    }
    /* both addresses are IPv4 */
    ipUnionApplyMaskV4(&ipaddr->ip_ip, ipUnionGetV4(&mask_ip->ip_ip));
    return;
}

#endif /* SK_ENABLE_IPV6 */


/* *************    IP WILDCARDS   ******************* */


void
skIPWildcardClear(
    skIPWildcard_t     *ipwild)
{
    assert(ipwild);
    memset(ipwild, 0, sizeof(skIPWildcard_t));
    memset(ipwild->m_min, 0xFF, sizeof(ipwild->m_min));
}



#if SK_ENABLE_IPV6
int
skIPWildcardCheckIp(
    const skIPWildcard_t   *ipwild,
    const skipaddr_t       *ipaddr)
{
    assert(ipwild);
    assert(ipaddr);

    if (skIPWildcardIsV6(ipwild)) {
        uint8_t ip6[16];
        int i;

        if (skipaddrIsV6(ipaddr)) {
            skipaddrGetV6(ipaddr, ip6);
        } else {
            skipaddr_t tmpip;
            skipaddrV4toV6(ipaddr, &tmpip);
            skipaddrGetV6(&tmpip, ip6);
        }

        for (i = 0; i < 8; ++i) {
            if (!_IPWILD_BLOCK_IS_SET(ipwild, i, ip6[2*i] << 8 | ip6[2*i+1])) {
                return 0;
            }
        }

        return 1;
    }

    if (skipaddrIsV6(ipaddr)) {
        skipaddr_t tmpip;
        const uint8_t ip4in6[12] = {0x0, 0x0, 0x0, 0x0,  0x0,  0x0,
                                    0x0, 0x0, 0x0, 0x0, 0xFF, 0xFF};
        uint8_t ip6[16];
        uint32_t ip4;

        skipaddrGetV6(ipaddr, ip6);
        if (memcmp(ip6, ip4in6, sizeof(ip4in6)) != 0) {
            return 0;
        }

        memcpy(&ip4, &ip6[12], 4);
        ip4 = ntohl(ip4);
        skipaddrSetV4(&tmpip, &ip4);
        return _IPWILD_IPv4_IS_SET(ipwild, &tmpip);
    }

    return _IPWILD_IPv4_IS_SET(ipwild, ipaddr);
}


/* Bind iterator to an ipwildcard, forcing IPv6 addresses */
int
skIPWildcardIteratorBindV6(
    skIPWildcardIterator_t *out_iter,
    const skIPWildcard_t   *ipwild)
{
    if (skIPWildcardIteratorBind(out_iter, ipwild)) {
        return -1;
    }
    out_iter->force_ipv6 = 1;
    out_iter->force_ipv4 = 0;
    return 0;
}


/* Bind iterator to an ipwildcard, forcing IPv4 addresses */
int
skIPWildcardIteratorBindV4(
    skIPWildcardIterator_t *out_iter,
    const skIPWildcard_t   *ipwild)
{
    assert(out_iter);
    if (NULL == ipwild) {
        return -1;
    }

    out_iter->ipwild = ipwild;
    out_iter->force_ipv6 = 0;
    /* only set force_ipv4 when wildcard is IPv6 */
    out_iter->force_ipv4 = skIPWildcardIsV6(ipwild);
    skIPWildcardIteratorReset(out_iter);

    return 0;
}
#endif /* SK_ENABLE_IPV6 */


/* Bind iterator to an ipwildcard */
int
skIPWildcardIteratorBind(
    skIPWildcardIterator_t *out_iter,
    const skIPWildcard_t   *ipwild)
{
    assert(out_iter);
    if (ipwild == NULL) {
        return -1;
    }

    out_iter->ipwild = ipwild;
    out_iter->force_ipv6 = 0;
    out_iter->force_ipv4 = 0;
    skIPWildcardIteratorReset(out_iter);

    return 0;
}


/* helper function for skIPWildcardIteratorNext() and
 * skIPWildcardIteratorNextCidr() */
static skIteratorStatus_t
ipwildcardIterNext(
    skIPWildcardIterator_t *iter,
    skipaddr_t             *ipaddr,
    uint32_t               *prefix,
    int                     want_cidr)
{
    uint32_t cidr_adjust;
    uint32_t tmp;
    unsigned int check_ints;
    unsigned int idx;
    unsigned int i;
    unsigned int j;

    assert(iter);
    assert(ipaddr);
    assert(prefix);

    /* check the stopping condition */
    if (iter->no_more_entries) {
        return SK_ITERATOR_NO_MORE_ENTRIES;
    }

    /* the iterator is already set to the single IP to return or to
     * the first IP of the CIDR block to return; get that first IP,
     * and set prefix assuming a single IP */
#if SK_ENABLE_IPV6
    if (skIPWildcardIsV6(iter->ipwild)) {
        if (iter->force_ipv4) {
            uint32_t ip4 = (iter->i_block[6] << 16) | iter->i_block[7];
            skipaddrSetV4(ipaddr, &ip4);
            *prefix = 32;
        } else {
            uint8_t ip6[16];

            ip6[ 0] = (uint8_t)(iter->i_block[0] >> 8);
            ip6[ 1] = (uint8_t)(iter->i_block[0] & 0xFF);
            ip6[ 2] = (uint8_t)(iter->i_block[1] >> 8);
            ip6[ 3] = (uint8_t)(iter->i_block[1] & 0xFF);
            ip6[ 4] = (uint8_t)(iter->i_block[2] >> 8);
            ip6[ 5] = (uint8_t)(iter->i_block[2] & 0xFF);
            ip6[ 6] = (uint8_t)(iter->i_block[3] >> 8);
            ip6[ 7] = (uint8_t)(iter->i_block[3] & 0xFF);
            ip6[ 8] = (uint8_t)(iter->i_block[4] >> 8);
            ip6[ 9] = (uint8_t)(iter->i_block[4] & 0xFF);
            ip6[10] = (uint8_t)(iter->i_block[5] >> 8);
            ip6[11] = (uint8_t)(iter->i_block[5] & 0xFF);
            ip6[12] = (uint8_t)(iter->i_block[6] >> 8);
            ip6[13] = (uint8_t)(iter->i_block[6] & 0xFF);
            ip6[14] = (uint8_t)(iter->i_block[7] >> 8);
            ip6[15] = (uint8_t)(iter->i_block[7] & 0xFF);
            skipaddrSetV6(ipaddr, ip6);
            *prefix = 128;
        }
    } else if (iter->force_ipv6) {
        uint32_t ip4 = ((iter->i_block[0] << 24) |
                        (iter->i_block[1] << 16) |
                        (iter->i_block[2] <<  8) |
                        (iter->i_block[3]));
        skipaddrSetV6FromUint32(ipaddr, &ip4);
        *prefix = 128;
    } else
#endif
    {
        uint32_t ip4 = ((iter->i_block[0] << 24) |
                        (iter->i_block[1] << 16) |
                        (iter->i_block[2] <<  8) |
                        (iter->i_block[3]));
        skipaddrSetV4(ipaddr, &ip4);
        *prefix = 32;
    }

    /* skip the for() loop if not looking for a CIDR block */
    if (!want_cidr) {
        goto NEXT_IP;
    }

    /* determine the end of this CIDR block by counting the number of
     * consecutive high bits in the bitmap */
    i = iter->ipwild->num_blocks;
    while (i > 0) {
        --i;
        /* 'idx' is array position of the uint32_t we are currently
         * looking at in the bitmap for this octet/hexadectet */
        idx = _BMAP_INDEX(iter->i_block[i]);
        /* this is the CIDR block adjustment to make due to this
         * octet/hexadectet only */
        cidr_adjust = 0;
        /* switch based on the number of additional bits we can
         * consider in the current uint32_t */
        switch (iter->i_block[i] & 0x1F) {
          case 0:
            /* can consider all */
            if (iter->ipwild->m_blocks[i][idx] == 0xFFFFFFFF) {
                /* all bits in this uint32_t are high.  Check for a
                 * longer run of high bits by checking the value in
                 * adjacent uint32_t's. 'check_ints' is the number of
                 * values to check.  this calculation is based on the
                 * number of trailing 0's in the number of available
                 * uint32_t's. */
                tmp = ((iter->i_block[i] >> 5)
                       | (1 << (2 * iter->ipwild->num_blocks - 5)));
                if (tmp & 0x1) {
                    check_ints = 0;
                } else {
                    check_ints = (1 << 1);
                    if ((tmp & 0xff) == 0) {
                        tmp >>= 8;
                        check_ints <<= 8;
                    }
                    if ((tmp & 0xf) == 0) {
                        tmp >>= 4;
                        check_ints <<= 4;
                    }
                    if ((tmp & 0x3) == 0) {
                        tmp >>= 2;
                        check_ints <<= 2;
                    }
                    check_ints >>= (tmp & 0x1);
                }
                /* move to next uint32_t; start counting from 1 since
                 * we already checked one uint32_t. */
                ++idx;
                for (j = 1; j < check_ints; ++j, ++idx) {
                    if (iter->ipwild->m_blocks[i][idx] != 0xFFFFFFFF) {
                        break;
                    }
                }
                /* cidr_adjust is at least 5 plus an amount determined
                 * by the most significant bit in 'j' */
                cidr_adjust = 5 + skIntegerLog2(j);
                break;
            }
            /* FALLTHROUGH */

          case 16:
            /* we can consider at least 16 bits in this uint32_t */
            if (((iter->ipwild->m_blocks[i][idx]
                  >> (iter->i_block[i] & 0x1F)) & 0xFFFF) == 0xFFFF)
            {
                cidr_adjust = 4;
                break;
            }
            /* FALLTHROUGH */

          case 8: case 24:
            /* we can consider at least 8 bits in this uint32_t */
            if (((iter->ipwild->m_blocks[i][idx]
                  >> (iter->i_block[i] & 0x1F)) & 0xFF) == 0xFF)
            {
                cidr_adjust = 3;
                break;
            }
            /* FALLTHROUGH */

          case 4: case 12: case 20: case 28:
            if (((iter->ipwild->m_blocks[i][idx]
                  >> (iter->i_block[i] & 0x1F)) & 0xF) == 0xF)
            {
                cidr_adjust = 2;
                break;
            }
            /* FALLTHROUGH */

          case  2: case  6: case 10: case 14:
          case 18: case 22: case 26: case 30:
            if (((iter->ipwild->m_blocks[i][idx]
                  >> (iter->i_block[i] & 0x1F)) & 0x3) == 0x3)
            {
                cidr_adjust = 1;
                break;
            }
            /* FALLTHROUGH */

          default:
            break;
        }

        /* adjust the prefix value based on this octet/hexadectet */
        *prefix -= cidr_adjust;
        iter->i_block[i] += (1 << cidr_adjust) - 1;
        assert(iter->i_block[i] <= iter->ipwild->m_max[i]);

        /* do the consecutive bits run into the next
         * octet/hexadectet? */
        if (cidr_adjust < 2u * iter->ipwild->num_blocks) {
            /* no.  we've found the largest block */
            break;
        }
    }

  NEXT_IP:
    /* found the CIDR block to return this time.  move the iterator to
     * the start of the next IP or CIDR block for the next query */
    i = iter->ipwild->num_blocks;
    while (i > 0) {
        --i;
        /* is the i'th octet/hexadectet at its maximum? */
        if (iter->i_block[i] >= iter->ipwild->m_max[i]) {
            /* yes; reset the counter for this octet/hexadectet and
             * try the next most significant octet/hexadectet */
            iter->i_block[i] = iter->ipwild->m_min[i];

            if (iter->force_ipv4 && i <= 6) {
                break;
            }
            continue;
        }
        /* else it is not at the max, and we can increment this
         * octet/hexadectet.  find the next high bit */

        /* assert that our stopping condition is valid */
        assert(_IPWILD_BLOCK_IS_SET(iter->ipwild, i, iter->ipwild->m_max[i]));
        ++iter->i_block[i];
        if (_IPWILD_BLOCK_IS_SET(iter->ipwild, i, iter->i_block[i])) {
            /* found it */
            return SK_ITERATOR_OK;
        }

        /* jump over multiple 0 bits. get current uint32_t */
        tmp = iter->ipwild->m_blocks[i][_BMAP_INDEX(iter->i_block[i])];
        if (iter->i_block[i] & 0x1F) {
            /* this uint32_t contains the CIDR block that is being
             * returned; need to shift off bits we have already
             * handled */
            tmp >>= (iter->i_block[i] & 0x1F);
            if (0 == tmp) {
                /* no more high bits in this uint32_t */
                iter->i_block[i] += 32 - (iter->i_block[i] & 0x1F);
                tmp = iter->ipwild->m_blocks[i][_BMAP_INDEX(iter->i_block[i])];
            }
        }
        while (0 == tmp) {
            /* skip over uint32_t's that are 0 */
            iter->i_block[i] += 32;
            tmp = iter->ipwild->m_blocks[i][_BMAP_INDEX(iter->i_block[i])];
        }

        /* this uint32_t contains a high bit */
        if (0 == (tmp & 0x1)) {
            ++iter->i_block[i];
            if ((tmp & 0xFFFF) == 0) {
                tmp >>= 16;
                iter->i_block[i] += 16;
            }
            if ((tmp & 0xFF) == 0) {
                tmp >>= 8;
                iter->i_block[i] += 8;
            }
            if ((tmp & 0xF) == 0) {
                tmp >>= 4;
                iter->i_block[i] += 4;
            }
            if ((tmp & 0x3) == 0) {
                tmp >>= 2;
                iter->i_block[i] += 2;
            }
            iter->i_block[i] -= (tmp & 0x1);
        }
        assert(_IPWILD_BLOCK_IS_SET(iter->ipwild,i,iter->i_block[i]));
        return SK_ITERATOR_OK;
    }

    /* we're done. make the next call to Next() fail */
    iter->no_more_entries = 1;

    return SK_ITERATOR_OK;
}


/* Get next entry in tree */
skIteratorStatus_t
skIPWildcardIteratorNext(
    skIPWildcardIterator_t *iter,
    skipaddr_t             *ipaddr)
{
    uint32_t prefix;

    return ipwildcardIterNext(iter, ipaddr, &prefix, 0);
}


/* get the next CIDR block in the wildcard */
skIteratorStatus_t
skIPWildcardIteratorNextCidr(
    skIPWildcardIterator_t *iter,
    skipaddr_t             *ipaddr,
    uint32_t               *prefix)
{
    return ipwildcardIterNext(iter, ipaddr, prefix, 1);
}


/* Reset iterator */
void
skIPWildcardIteratorReset(
    skIPWildcardIterator_t *iter)
{
    int i;

    assert(iter);

#if SK_ENABLE_IPV6
    if (iter->force_ipv4) {
        /* must ensure that wildcard contains 0:0:0:0:0:FFFF:x:x */
        assert(skIPWildcardIsV6(iter->ipwild));
        for (i = 0; i < 5; ++i) {
            if (!_IPWILD_BLOCK_IS_SET(iter->ipwild, i, 0)) {
                iter->no_more_entries = 1;
                return;
            }
            assert(0 == iter->ipwild->m_min[i]);
            iter->i_block[i] = iter->ipwild->m_min[i];
        }
        if (!_IPWILD_BLOCK_IS_SET(iter->ipwild, 5, 0xFFFF)) {
            iter->no_more_entries = 1;
            return;
        }
        assert(UINT16_MAX == iter->ipwild->m_max[5]);
        iter->i_block[5] = iter->ipwild->m_max[5];

        iter->i_block[6] = iter->ipwild->m_min[6];
        iter->i_block[7] = iter->ipwild->m_min[7];

        iter->no_more_entries = 0;
        return;
    }
#endif  /* SK_ENABLE_IPV6 */

    iter->no_more_entries = 0;
    for (i = 0; i < iter->ipwild->num_blocks; ++i) {
        iter->i_block[i] = iter->ipwild->m_min[i];
    }
}


/* ******************************************************************** */
/* skcidr_t                                                             */
/* ******************************************************************** */


#if SK_ENABLE_IPV6
/* check whether 'ipaddr' is in 'cidr' */
int
skcidrCheckIP(
    const skcidr_t     *cidr,
    const skipaddr_t   *ipaddr)
{
    uint8_t  ipv6[16];
    uint32_t ipv4;

    if (skcidrIsV6(cidr)) {
        skipaddrGetAsV6(ipaddr, ipv6);

        return (0 == memcmp(cidr->v6.ip, ipv6, cidr->v6.byte_length)
                && ((0 == cidr->v6.mask)
                    || ((cidr->v6.mask & ipv6[cidr->v6.byte_length])
                        == cidr->v6.ip[cidr->v6.byte_length])));
    }
    if (0 == skipaddrGetAsV4(ipaddr, &ipv4)) {
        return ((ipv4 & cidr->v4.mask) == cidr->v4.ip);
    }
    return 0;
}
#endif  /* SK_ENABLE_IPV6 */

/* Fill 'ipaddr' with the IP address in 'cidr' */
void
skcidrGetIPAddr(
    const skcidr_t     *cidr,
    skipaddr_t         *ipaddr)
{
#if SK_ENABLE_IPV6
    if (skcidrIsV6(cidr)) {
        skipaddrSetV6(ipaddr, cidr->v6.ip);
        return;
    }
#endif
    skipaddrSetV4(ipaddr, &(cidr->v4.ip));
}


/* Fill 'cidr' using the 'ipaddr' and 'cidr_len' */
int
skcidrSetFromIPAddr(
    skcidr_t           *cidr,
    const skipaddr_t   *ipaddr,
    uint32_t            cidr_len)
{
#if SK_ENABLE_IPV6
    if (skipaddrIsV6(ipaddr)) {
        uint8_t tmp_ip[16];
        skipaddrGetV6(ipaddr, tmp_ip);
        return skcidrSetV6(cidr, tmp_ip, cidr_len);
    }
#endif
    return skcidrSetV4(cidr, skipaddrGetV4(ipaddr), cidr_len);
}


/* Fill 'cidr' using the 'ipv4' and 'cidr_len' */
int
skcidrSetV4(
    skcidr_t           *cidr,
    uint32_t            ipv4,
    uint32_t            cidr_len)
{
    if (cidr_len > 32) {
        return -1;
    }

    skcidrClear(cidr);
    cidr->v4.cidr_length = cidr_len;
    cidr->v4.mask = ((cidr_len == 32)
                     ? UINT32_MAX
                     : ~(UINT32_MAX >> cidr_len));
    cidr->v4.ip = ipv4 & cidr->v4.mask;
    return 0;
}


#if SK_ENABLE_IPV6
/* Fill 'cidr' using the 'ipv6' and 'cidr_len' */
int
skcidrSetV6(
    skcidr_t           *cidr,
    const uint8_t      *ipv6,
    uint32_t            cidr_len)
{
    if (cidr_len > 128) {
        return -1;
    }

    skcidrClear(cidr);
    cidr->v6.is_ipv6 = 1;
    cidr->v6.cidr_length = cidr_len;
    cidr->v6.byte_length = cidr_len >> 3;
    cidr->v6.mask = 0xFF & ~(0xFF >> (cidr_len & 0x7));
    memcpy(cidr->v6.ip, ipv6, cidr->v6.byte_length);
    if (cidr->v6.mask) {
        cidr->v6.ip[cidr->v6.byte_length]
            = ipv6[cidr->v6.byte_length] & cidr->v6.mask;
    }
    return 0;
}
#endif  /* SK_ENABLE_IPV6 */


/* ******************************************************************** */
/* sockaddr                                                             */
/* ******************************************************************** */


/* Fill 'dest' of length 'len' with the address in 'src' */
int
skipaddrToSockaddr(
    struct sockaddr    *dest,
    size_t              len,
    const skipaddr_t   *src)
{
    assert(src);
    assert(dest);

#if SK_ENABLE_IPV6
    if (skipaddrIsV6(src)) {
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)dest;
        if (len < sizeof(*v6)) {
            return -1;
        }
        memset(v6, 0, sizeof(*v6));
        v6->sin6_family = AF_INET6;
        skipaddrGetV6(src, v6->sin6_addr.s6_addr);
    } else
#endif  /* SK_ENABLE_IPV6 */
    {
        struct sockaddr_in *v4 = (struct sockaddr_in *)dest;
        if (len < sizeof(*v4)) {
            return -1;
        }
        memset(v4, 0, sizeof(*v4));
        v4->sin_family = AF_INET;
        v4->sin_addr.s_addr = htonl(skipaddrGetV4(src));
    }

    return 0;
}

/* Fill 'dest' with address in 'src' */
int
skipaddrFromSockaddr(
    skipaddr_t             *dest,
    const struct sockaddr  *src)
{
    assert(dest);
    assert(src);

    switch (src->sa_family) {
      case AF_INET:
        {
            in_addr_t addr;
            const struct sockaddr_in *v4 = (const struct sockaddr_in *)src;
            addr = ntohl(v4->sin_addr.s_addr);
            skipaddrSetV4(dest, &addr);
        }
        break;
#if SK_ENABLE_IPV6
      case AF_INET6:
        {
            const struct sockaddr_in6 *v6 = (const struct sockaddr_in6 *)src;
            skipaddrSetV6(dest, v6->sin6_addr.s6_addr);
        }
        break;
#endif
      default:
        return -1;
    }

    return 0;
}


int
skSockaddrCompare(
    const sk_sockaddr_t    *a,
    const sk_sockaddr_t    *b,
    int                     flags)
{
    uint16_t pa, pb;
    sk_sockaddr_t temp;

    if (a == b) {
        return 0;
    }
    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    if (!(flags & SK_SOCKADDRCOMP_NOPORT)) {
        switch (a->sa.sa_family) {
          case AF_INET:
            pa = a->v4.sin_port;
            break;
          case AF_INET6:
            pa = a->v6.sin6_port;
            break;
          case AF_UNIX:
            pa = 0;
            break;
          default:
            skAbortBadCase(a->sa.sa_family);
        }
        switch (b->sa.sa_family) {
          case AF_INET:
            pb = b->v4.sin_port;
            break;
          case AF_INET6:
            pb = b->v6.sin6_port;
            break;
          case AF_UNIX:
            pb = 0;
            break;
          default:
            skAbortBadCase(b->sa.sa_family);
        }
        if (pa < pb) {
            return -1;
        }
        if (pa > pb) {
            return 1;
        }
    }
    if (!(flags & SK_SOCKADDRCOMP_NOT_V4_AS_V6)) {
        /* If necessary, convert V4 addresses to V6. */
        if (a->sa.sa_family == AF_INET6 && b->sa.sa_family == AF_INET) {
            temp.sa.sa_family = AF_INET6;
            memcpy(temp.v6.sin6_addr.s6_addr,
                   sk_ipv6_v4inv6, SK_IPV6_V4INV6_LEN);
            memcpy(temp.v6.sin6_addr.s6_addr + SK_IPV6_V4INV6_LEN,
                   &b->v4.sin_addr.s_addr, 4);
            b = &temp;
        } else if (a->sa.sa_family == AF_INET && b->sa.sa_family == AF_INET6) {
            temp.sa.sa_family = AF_INET6;
            memcpy(temp.v6.sin6_addr.s6_addr,
                   sk_ipv6_v4inv6, SK_IPV6_V4INV6_LEN);
            memcpy(temp.v6.sin6_addr.s6_addr + SK_IPV6_V4INV6_LEN,
                   &a->v4.sin_addr.s_addr, 4);
            a = &temp;
        }
    }
    if (a->sa.sa_family < b->sa.sa_family) {
        return -1;
    }
    if (a->sa.sa_family > b->sa.sa_family) {
        return 1;
    }
    if (flags & SK_SOCKADDRCOMP_NOADDR) {
        return 0;
    }
    switch (a->sa.sa_family) {
      case AF_INET:
        return memcmp(&a->v4.sin_addr.s_addr, &b->v4.sin_addr.s_addr,
                      sizeof(a->v4.sin_addr.s_addr));
      case AF_INET6:
        return memcmp(a->v6.sin6_addr.s6_addr, b->v6.sin6_addr.s6_addr,
                      sizeof(a->v6.sin6_addr.s6_addr));
      case AF_UNIX:
        return strncmp(a->un.sun_path, b->un.sun_path,
                       sizeof(a->un.sun_path));
      default:
        skAbortBadCase(a->sa.sa_family);
    }
}

int
skSockaddrString(
    char                   *buffer,
    int                     size,
    const sk_sockaddr_t    *addr)
{
    /* Must be large enough to hold UNIX domain socket path. */
    char sabuf[PATH_MAX];
    skipaddr_t ipaddr;
    uint16_t port;
    int rv;

    switch (addr->sa.sa_family) {
      case AF_INET6:
        if (0 == memcmp(addr->v6.sin6_addr.s6_addr, in6addr_any.s6_addr,
                        sizeof(addr->v6.sin6_addr.s6_addr)))
        {
            sabuf[0] = '*';
            sabuf[1] = '\0';
        } else {
#if SK_ENABLE_IPV6
            skipaddrFromSockaddr(&ipaddr, &addr->sa);
            skipaddrString(sabuf, &ipaddr, SKIPADDR_CANONICAL);
#elif defined(SK_HAVE_INET_NTOP)
            if (NULL == inet_ntop(AF_INET6, &addr->v6.sin6_addr,
                                  sabuf, sizeof(sabuf)))
            {
                strcpy(sabuf, "<unknown-ipv6>");
            }
#else  /* !SK_ENABLE_IPV6 && !defined(SK_HAVE_INET_NTOP) */
            strcpy(sabuf, "<unknown-ipv6>");
#endif  /* SK_ENABLE_IPV6 */
        }
        port = ntohs(addr->v6.sin6_port);
        if (port) {
            rv = snprintf(buffer, size, ("[%s]:%" PRIu16), sabuf, port);
        } else {
            rv = snprintf(buffer, size, "%s", sabuf);
        }
        break;
      case AF_INET:
        if (addr->v4.sin_addr.s_addr == INADDR_ANY) {
            sabuf[0] = '*';
            sabuf[1] = '\0';
        } else {
            skipaddrFromSockaddr(&ipaddr, &addr->sa);
            skipaddrString(sabuf, &ipaddr, SKIPADDR_CANONICAL);
        }
        port = ntohs(addr->v4.sin_port);
        if (port) {
            rv = snprintf(buffer, size, ("%s:%" PRIu16), sabuf, port);
        } else {
            rv = snprintf(buffer, size, "%s", sabuf);
        }
        break;
      case AF_UNIX:
        rv = snprintf(buffer, size, "%s", addr->un.sun_path);
        break;
      default:
        skAbortBadCase(addr->sa.sa_family);
    }

    return rv;
}

int
skSockaddrArrayContains(
    const sk_sockaddr_array_t  *array,
    const sk_sockaddr_t        *addr,
    int                         flags)
{
    uint32_t i;

    if (array == NULL || addr == NULL) {
        return 0;
    }
    for (i = 0; i < skSockaddrArraySize(array); i++) {
        if (skSockaddrCompare(skSockaddrArrayGet(array, i),
                              addr, flags) == 0)
        {
            return 1;
        }
    }

    return 0;
}

int
skSockaddrArrayEqual(
    const sk_sockaddr_array_t  *a,
    const sk_sockaddr_array_t  *b,
    int                         flags)
{
    uint32_t i;

    if (a == NULL) {
        return b == NULL;
    }
    if (b == NULL) {
        return 0;
    }
    if (skSockaddrArraySize(a) != skSockaddrArraySize(b)) {
        return 0;
    }
    for (i = 0; i < skSockaddrArraySize(a); i++) {
        if (!skSockaddrArrayContains(b, skSockaddrArrayGet(a, i), flags)) {
            return 0;
        }
    }
    return 1;
}

int
skSockaddrArrayMatches(
    const sk_sockaddr_array_t  *a,
    const sk_sockaddr_array_t  *b,
    int                         flags)
{
    uint32_t i, j;

    if (a == NULL) {
        return b == NULL;
    }
    if (b == NULL) {
        return 0;
    }
    for (i = 0; i < skSockaddrArraySize(a); ++i) {
        for (j = 0; j < skSockaddrArraySize(b); ++j) {
            if (skSockaddrCompare(skSockaddrArrayGet(a, i),
                                  skSockaddrArrayGet(b, j),
                                  flags) == 0)
            {
                return 1;
            }
        }
    }
    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
