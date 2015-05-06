/*
** Copyright (C) 2005-2015 by Carnegie Mellon University.
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
**  rwgroup.h
**
**    See rwgroup.c for description.
**
*/
#ifndef _RWGROUP_H
#define _RWGROUP_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_RWGROUP_H, "$SiLK: rwgroup.h b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/rwascii.h>
#include <silk/rwrec.h>
#include <silk/skipaddr.h>
#include <silk/skplugin.h>
#include <silk/skstream.h>
#include <silk/utils.h>


/* TYPEDEFS AND DEFINES */


/*
 *    Maximum size of the --rec-treshold
 */
#define MAX_THRESHOLD 65535

/*
 *    Value indicating the delta_field value is unset
 */
#define DELTA_FIELD_UNSET   UINT32_MAX

/*
 *    Maximum number of fields that can come from plugins.  Allow four
 *    per plug-in.
 */
#define MAX_PLUGIN_KEY_FIELDS  32

/*
 *    Maximum bytes allotted to a "node", which is the complete rwRec
 *    and the bytes required by all keys that can come from plug-ins.
 *    Allow 8 bytes per field, plus enough space for an rwRec.
 */
#define MAX_NODE_SIZE  (256 + SK_MAX_RECORD_SIZE)

/* for key fields that come from plug-ins, this struct will hold
 * information about a single field */
typedef struct key_field_st {
    /* The plugin field handle, if kf_fxn is null */
    skplugin_field_t *kf_field_handle;
    /* the byte-offset for this field */
    size_t            kf_offset;
    /* the byte-width of this field */
    size_t            kf_width;
} key_field_t;


/* VARIABLES */

/* number of fields to group by; skStringMapParse() sets this */
extern uint32_t num_fields;

/* IDs of the fields to group by; skStringMapParse() sets it; values
 * are from the rwrec_printable_fields_t enum and from values that
 * come from plugins. */
extern uint32_t *id_fields;

/* the size of a "node".  Because the output from rwgroup is SiLK
 * records, the node size includes the complete rwRec, plus any binary
 * fields that we get from plug-ins to use as the key.  This node_size
 * value may increase when we parse the --fields switch. */
extern uint32_t node_size;

/* the columns that make up the key that come from plug-ins */
extern key_field_t key_fields[MAX_PLUGIN_KEY_FIELDS];

/* the number of these key_fields */
extern size_t key_num_fields;

/* input stream */
extern skstream_t *in_rwios;

/* output stream */
extern skstream_t *out_rwios;

/* the id of the field to match with fuzzy-ness */
extern uint32_t delta_field;

/* the amount of fuzzy-ness allowed */
extern uint64_t delta_value;

/* for IPv6, use a delta_value that is an skipaddr_t */
extern skipaddr_t delta_value_ip;

/* number of records to that must be in a group before the group is
 * printed. */
extern uint32_t threshold;

/* where to store the records while waiting to meet the threshold */
extern rwRec *thresh_buf;

/* the value to write into the next hop IP field */
extern skipaddr_t group_id;

/* whether the --summarize switch was given */
extern int summarize;

/* whether the --objective switch was given */
extern int objective;


void
appTeardown(
    void);
void
appSetup(
    int                 argc,
    char              **argv);


#ifdef __cplusplus
}
#endif
#endif /* _RWGROUP_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
