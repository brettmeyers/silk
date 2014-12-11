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

RCSIDENT("$SiLK: rwscan_icmp.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include "rwscan.h"


void
increment_icmp_counters(
    rwRec              *rwrec,
    event_metrics_t    *metrics)
{
    uint8_t type = 0, code = 0;

    type = rwRecGetIcmpType(rwrec);
    code = rwRecGetIcmpCode(rwrec);

    if ((type == 8 || type == 13 || type == 15 || type == 17)
        && (code == 0))
    {
        metrics->flows_icmp_echo++;
    }
}


void
calculate_icmp_metrics(
    rwRec              *event_flows,
    event_metrics_t    *metrics)
{
    uint32_t i;
    uint32_t class_c_next = 0, class_c_curr = 0;
    uint32_t dip_next     = 0, dip_curr = 0;

    uint8_t  run               = 1, max_run_curr = 1;
    uint32_t class_c_run       = 1, max_class_c_run = 1;
    uint8_t  class_c_dip_count = 1, max_class_c_dip_count = 1;

    rwRec *rwcurr = NULL;
    rwRec *rwnext = NULL;

    calculate_shared_metrics(event_flows, metrics);

    for (i = 0; i < metrics->event_size; i++) {
        rwcurr = &(event_flows[i]);
        rwnext =
            (i + 1 < (metrics->event_size)) ? &(event_flows[i + 1]) : NULL;

        dip_curr     = rwRecGetDIPv4(rwcurr);
        class_c_curr = dip_curr & 0xFFFFFF00;

        if (rwnext != NULL) {
            dip_next     = rwRecGetDIPv4(rwnext);
            class_c_next = dip_next & 0xFFFFFF00;
        }

        if ((rwnext != NULL) && (class_c_curr == class_c_next)) {
            if (dip_curr != dip_next) {
                class_c_dip_count++;
                if (dip_next - dip_curr == 1) {
                    run++;
                } else {
                    if (run > max_run_curr) {
                        max_run_curr = run;
                    }
                    run = 1;
                }
            }
        } else {
            if (((class_c_next - class_c_curr) >> 8) == 1) {
                class_c_run++;
            } else {
                if (class_c_run > max_class_c_run) {
                    max_class_c_run = class_c_run;
                }
                class_c_run = 1;
            }

            if (max_run_curr >
                metrics->proto.icmp.max_class_c_dip_run_length)
            {
                metrics->proto.icmp.max_class_c_dip_run_length = max_run_curr;
            }

            if (class_c_dip_count > max_class_c_dip_count) {
                max_class_c_dip_count = class_c_dip_count;
            }
            class_c_dip_count = 1;
        }
    }

    metrics->proto.icmp.max_class_c_subnet_run_length = max_class_c_run;

    metrics->proto.icmp.echo_ratio =
        ((double) metrics->flows_icmp_echo / metrics->event_size);
    metrics->proto.icmp.max_class_c_dip_count = max_class_c_dip_count;
    metrics->proto.icmp.total_dip_count       = metrics->unique_dsts;

    print_verbose_results((RWSCAN_VERBOSE_FH, "\ticmp (%u, %u, %u, %u, %.3f)",
                           metrics->proto.icmp.max_class_c_subnet_run_length,
                           metrics->proto.icmp.max_class_c_dip_run_length,
                           metrics->proto.icmp.max_class_c_dip_count,
                           metrics->proto.icmp.total_dip_count,
                           metrics->proto.icmp.echo_ratio));
}


void
calculate_icmp_scan_probability(
    event_metrics_t    *metrics)
{
    double y = 0;

    y = ICMP_BETA0
        + ICMP_BETA1 * metrics->proto.icmp.max_class_c_subnet_run_length
        + ICMP_BETA5 * metrics->proto.icmp.max_class_c_dip_run_length
        + ICMP_BETA6 * metrics->proto.icmp.max_class_c_dip_count
        + ICMP_BETA11 * metrics->proto.icmp.total_dip_count
        + ICMP_BETA22 * metrics->proto.icmp.echo_ratio;

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
