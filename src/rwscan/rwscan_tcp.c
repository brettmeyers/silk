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

#include <silk/silk.h>

RCSIDENT("$SiLK: rwscan_tcp.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwscan.h"


void
add_count(
    uint32_t           *counts,
    uint32_t            value,
    uint32_t            max)
{
    if (value >= max - 1) {
        counts[max - 1]++;
    } else {
        counts[value]++;
    }
}

void
increment_tcp_counters(
    rwRec              *rwrec,
    event_metrics_t    *metrics)
{
    if (!(rwRecGetFlags(rwrec) & ACK_FLAG)) {
        metrics->flows_noack++;
    }

    if (rwRecGetPkts(rwrec) < SMALL_PKT_CUTOFF) {
        metrics->flows_small++;
    }

    if ((rwRecGetBytes(rwrec) / rwRecGetPkts(rwrec)) > PACKET_PAYLOAD_CUTOFF) {
        metrics->flows_with_payload++;
    }

    if (rwRecGetFlags(rwrec) == RST_FLAG
        || rwRecGetFlags(rwrec) == (SYN_FLAG | ACK_FLAG)
        || rwRecGetFlags(rwrec) == (RST_FLAG | ACK_FLAG))
    {
        metrics->flows_backscatter++;
    }
    add_count(metrics->tcp_flag_counts,
              rwRecGetFlags(rwrec),
              RWSCAN_MAX_FLAGS);

}

void
calculate_tcp_metrics(
    rwRec              *event_flows,
    event_metrics_t    *metrics)
{
    calculate_shared_metrics(event_flows, metrics);

    metrics->proto.tcp.noack_ratio =
        ((double) metrics->flows_noack / metrics->event_size);
    metrics->proto.tcp.small_ratio =
        ((double) metrics->flows_small / metrics->event_size);
    metrics->proto.tcp.sp_dip_ratio =
        ((double) metrics->sp_count / metrics->unique_dips);
    metrics->proto.tcp.payload_ratio =
        ((double) metrics->flows_with_payload / metrics->event_size);
    metrics->proto.tcp.unique_dip_ratio =
        ((double) metrics->unique_dips / metrics->event_size);
    metrics->proto.tcp.backscatter_ratio =
        ((double) metrics->flows_backscatter / metrics->event_size);

    print_verbose_results((RWSCAN_VERBOSE_FH,
                           "\ttcp (%.3f, %.3f, %.3f, %.3f, %.3f, %.3f)",
                           metrics->proto.tcp.noack_ratio,
                           metrics->proto.tcp.small_ratio,
                           metrics->proto.tcp.sp_dip_ratio,
                           metrics->proto.tcp.payload_ratio,
                           metrics->proto.tcp.unique_dip_ratio,
                           metrics->proto.tcp.backscatter_ratio));
}

void
calculate_tcp_scan_probability(
    event_metrics_t    *metrics)
{
    double y = 0;

    y = TCP_BETA0
        + TCP_BETA2 * metrics->proto.tcp.noack_ratio
        + TCP_BETA4 * metrics->proto.tcp.small_ratio
        + TCP_BETA13 * metrics->proto.tcp.sp_dip_ratio
        + TCP_BETA15 * metrics->proto.tcp.payload_ratio
        + TCP_BETA19 * metrics->proto.tcp.unique_dip_ratio
        + TCP_BETA21 * metrics->proto.tcp.backscatter_ratio;
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
