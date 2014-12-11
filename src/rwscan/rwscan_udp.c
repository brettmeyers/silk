/*
** Copyright (C) 2006-2014 by Carnegie Mellon University.
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

#include <silk/silk.h>

RCSIDENT("$SiLK: rwscan_udp.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "rwscan.h"


void
increment_udp_counters(
    rwRec              *rwrec,
    event_metrics_t    *metrics)
{
    if (rwRecGetPkts(rwrec) < SMALL_PKT_CUTOFF) {
        metrics->flows_small++;
    }

    if ((rwRecGetBytes(rwrec) / rwRecGetPkts(rwrec)) > PACKET_PAYLOAD_CUTOFF) {
        metrics->flows_with_payload++;
    }

}

void
calculate_udp_metrics(
    rwRec              *event_flows,
    event_metrics_t    *metrics)
{
    uint32_t     i;
    uint32_t     class_c_next = 0, class_c_curr = 0;
    uint32_t     dip_next     = 0, dip_curr = 0;
    sk_bitmap_t *low_dp_bitmap;
    uint32_t     low_dp_hit = 0;

    sk_bitmap_t *sp_bitmap;

    uint32_t subnet_run = 1, max_subnet_run = 1;
    rwRec   *rwcurr     = NULL;
    rwRec   *rwnext     = NULL;

    skBitmapCreate(&low_dp_bitmap, 1024);
    skBitmapCreate(&sp_bitmap, UINT16_MAX);

    calculate_shared_metrics(event_flows, metrics);

    rwcurr = event_flows;
    rwnext = event_flows;

    skBitmapSetBit(low_dp_bitmap, rwRecGetDPort(rwcurr));
    dip_next     = rwRecGetDIPv4(rwnext);
    class_c_next = dip_next & 0xFFFFFF00;


    for (i = 0; i < metrics->event_size; ++i, ++rwcurr) {

        skBitmapSetBit(sp_bitmap, rwRecGetSPort(rwcurr));

        dip_curr     = dip_next;
        class_c_curr = class_c_next;

        if (i + 1 == metrics->event_size) {
            rwnext = NULL;
            dip_next = dip_curr - 1;
            class_c_next = class_c_curr - 0x100;

            if (subnet_run > max_subnet_run) {
                max_subnet_run = subnet_run;
            }
        } else {
            ++rwnext;

            dip_next     = rwRecGetDIPv4(rwnext);
            class_c_next = dip_next & 0xFFFFFF00;

            if (dip_curr == dip_next) {
                skBitmapSetBit(low_dp_bitmap, rwRecGetDPort(rwnext));
            } else if (class_c_curr == class_c_next) {
                if (dip_next - dip_curr == 1) {
                    ++subnet_run;
                } else if (subnet_run > max_subnet_run) {
                    max_subnet_run = subnet_run;
                    subnet_run = 1;
                }
            }
        }

        if (dip_curr != dip_next) {
            uint32_t j;
            uint32_t port_run = 0;

            /* determine longest consecutive run of low ports */
            for (j = 0; j < 1024; j++) {
                if (skBitmapGetBit(low_dp_bitmap, j)) {
                    ++port_run;
                } else if (port_run) {
                    if (port_run > metrics->proto.udp.max_low_port_run_length){
                        metrics->proto.udp.max_low_port_run_length = port_run;
                    }
                    port_run = 0;
                }
            }

            /* determine number of hits on low ports */
            low_dp_hit = skBitmapGetHighCount(low_dp_bitmap);
            if (low_dp_hit > metrics->proto.udp.max_low_dp_hit) {
                metrics->proto.udp.max_low_dp_hit = low_dp_hit;
            }

            /* reset */
            skBitmapClearAllBits(low_dp_bitmap);
            skBitmapSetBit(low_dp_bitmap, rwRecGetDPort(rwcurr));
        }

        if (class_c_curr != class_c_next) {
            if (max_subnet_run > metrics->proto.udp.max_class_c_dip_run_length)
            {
                metrics->proto.udp.max_class_c_dip_run_length = max_subnet_run;
            }
            max_subnet_run = 1;
        }
    }

    metrics->unique_sp_count = skBitmapGetHighCount(sp_bitmap);

    metrics->proto.udp.sp_dip_ratio =
        ((double) metrics->sp_count / metrics->unique_dsts);
    metrics->proto.udp.payload_ratio =
        ((double) metrics->flows_with_payload / metrics->event_size);
    metrics->proto.udp.unique_sp_ratio =
        ((double) metrics->unique_sp_count / metrics->event_size);
    metrics->proto.udp.small_ratio =
        ((double) metrics->flows_small / metrics->event_size);

    print_verbose_results((RWSCAN_VERBOSE_FH,
                           "\tudp (%.3f, %u, %u, %u, %.3f, %.3f, %.3f)",
                           metrics->proto.udp.small_ratio,
                           metrics->proto.udp.max_class_c_dip_run_length,
                           metrics->proto.udp.max_low_dp_hit,
                           metrics->proto.udp.max_low_port_run_length,
                           metrics->proto.udp.sp_dip_ratio,
                           metrics->proto.udp.payload_ratio,
                           metrics->proto.udp.unique_sp_ratio));

    skBitmapDestroy(&low_dp_bitmap);
    skBitmapDestroy(&sp_bitmap);
}

void
calculate_udp_scan_probability(
    event_metrics_t    *metrics)
{
    double y = 0;

    y = UDP_BETA0
        + UDP_BETA4 * metrics->proto.udp.small_ratio
        + UDP_BETA5 * metrics->proto.udp.max_class_c_dip_run_length
        + UDP_BETA8 * metrics->proto.udp.max_low_dp_hit
        + UDP_BETA10 * metrics->proto.udp.max_low_port_run_length
        + UDP_BETA13 * metrics->proto.udp.sp_dip_ratio
        + UDP_BETA15 * metrics->proto.udp.payload_ratio
        + UDP_BETA20 * metrics->proto.udp.unique_sp_ratio;

    metrics->scan_probability = exp(y) / (1.0 + exp(y));
    if (metrics->scan_probability > 0.5) {
        metrics->event_class = EVENT_SCAN;
    }
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
