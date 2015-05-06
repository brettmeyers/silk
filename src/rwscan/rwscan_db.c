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

RCSIDENT("$SiLK: rwscan_db.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwscan_db.h"

#define RWSCAN_TIME_BUFFER_SIZE 32


static field_def_t field_defs[] = {
    {RWSCAN_FIELD_SIP,       "sip",        16},
    {RWSCAN_FIELD_PROTO,     "proto",       6},
    {RWSCAN_FIELD_STIME,     "stime",      24},
    {RWSCAN_FIELD_ETIME,     "etime",      24},
    {RWSCAN_FIELD_FLOWS,     "flows",      10},
    {RWSCAN_FIELD_PKTS,      "packets",    10},
    {RWSCAN_FIELD_BYTES,     "bytes",      10},
    {RWSCAN_FIELD_MODEL,     "scan_model", 12},
    {RWSCAN_FIELD_SCAN_PROB, "scan_prob",  10},
    {(field_id_t)0,          0,             0}
};

int
write_scan_header(
    FILE               *out,
    uint8_t             no_columns,
    char                delimiter,
    uint8_t             model_fields)
{
    unsigned int i;
    int width;

    for (i = 0; field_defs[i].id != 0; ++i) {
        assert(i < RWSCAN_MAX_FIELD_DEFS);
        width = (no_columns) ? 0 : (field_defs[i].width);
        if ((field_defs[i].id == RWSCAN_FIELD_MODEL
             || field_defs[i].id == RWSCAN_FIELD_SCAN_PROB)
            && (!model_fields))
        {
            continue;
        }
        if (i != 0) {
            fprintf(out, "%c", delimiter);
        }
        fprintf(out, "%*s", width, field_defs[i].label);
    }
    if (!options.no_final_delimiter) {
        fprintf(out, "%c", delimiter);
    }
    fprintf(out, "\n");
    return 0;
}

int
write_scan_record(
    scan_info_t        *rec,
    FILE               *out,
    uint8_t             no_columns,
    char                delimiter,
    uint8_t             model_fields)
{
    unsigned int  i;
    int  width;
    char stimestr[RWSCAN_TIME_BUFFER_SIZE];
    char etimestr[RWSCAN_TIME_BUFFER_SIZE];
    char sipstr[SK_NUM2DOT_STRLEN+1];

    timestamp_to_datetime(stimestr, rec->stime);
    timestamp_to_datetime(etimestr, rec->etime);

    for (i = 0; field_defs[i].id != 0; ++i) {
        assert(i < RWSCAN_MAX_FIELD_DEFS);
        if ((field_defs[i].id == RWSCAN_FIELD_MODEL
             || field_defs[i].id == RWSCAN_FIELD_SCAN_PROB)
            && (!model_fields))
        {
            continue;
        }
        if (i != 0) {
            fprintf(out, "%c", delimiter);
        }

        width = (no_columns) ? 0 : (field_defs[i].width);

        switch (field_defs[i].id) {
          case RWSCAN_FIELD_SIP:
            if (options.integer_ips) {
                fprintf(out, "%*u", width, rec->ip);
            } else {
                fprintf(out, "%*s", width, num2dot_r(rec->ip, sipstr));
            }
            break;
          case RWSCAN_FIELD_PROTO:
            fprintf(out, "%*u", width, rec->proto);
            break;
          case RWSCAN_FIELD_STIME:
            fprintf(out, "%*s", width, stimestr);
            break;
          case RWSCAN_FIELD_ETIME:
            fprintf(out, "%*s", width, etimestr);
            break;
          case RWSCAN_FIELD_FLOWS:
            fprintf(out, "%*u", width, rec->flows);
            break;
          case RWSCAN_FIELD_PKTS:
            fprintf(out, "%*u", width, rec->pkts);
            break;
          case RWSCAN_FIELD_BYTES:
            fprintf(out, "%*u", width, rec->bytes);
            break;
          case RWSCAN_FIELD_MODEL:
            if (options.model_fields) {
                fprintf(out, "%*d", width, rec->model);
            }
            break;
          case RWSCAN_FIELD_SCAN_PROB:
            if (options.model_fields) {
                fprintf(out, "%*f", width, rec->scan_prob);
            }
            break;
          default:
            skAbortBadCase(field_defs[i].id); /* NOTREACHED */
        }
    }
    if (!options.no_final_delimiter) {
        fprintf(out, "%c", delimiter);
    }
    fprintf(out, "\n");

    return 0;
}


int
timestamp_to_datetime(
    char               *buf,
    uint32_t            timestamp)
{
    struct tm date_tm;
    time_t    t = (time_t)timestamp;

    gmtime_r(&t, &date_tm);
    snprintf(buf, RWSCAN_TIME_BUFFER_SIZE,
             "%04d-%02d-%02d %02d:%02d:%02d", 1900 + date_tm.tm_year,
             1 + date_tm.tm_mon, date_tm.tm_mday, date_tm.tm_hour,
             date_tm.tm_min, date_tm.tm_sec);
    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
