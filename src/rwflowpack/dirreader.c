/*
** Copyright (C) 2008-2015 by Carnegie Mellon University.
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
**  dirreader.c
**
**    Helper file for rwflowpack.
**
**    Specify the functions that are used to read poll a directory for
**    files containing PDU (NetFlow v5), IPFIX, or SiLK flow records.
**
**    This input_mode_type is used by the 'stream' input-mode.
**
**    Any SiLK Flow records read by this input_mode_type will be
**    completely repacked.  See respoolreader.c for a input_mode_type that
**    does not repack the records.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: dirreader.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include <silk/skpolldir.h>
#include <silk/skstream.h>
#include "rwflowpack_priv.h"


/* LOCAL DEFINES AND TYPEDEFS */

/* A name for this input mode. */
#define INPUT_MODE_TYPE_NAME  "Directory Reader"

/* There will be one of these per probe that processes files */
typedef struct dir_source_st {
    char                pathname[PATH_MAX];
    char               *filename;
    skPollDir_t        *polldir;
    union dir_src_un {
#if SK_ENABLE_IPFIX
        skIPFIXSource_t    *ipfix;
#endif
        skPDUSource_t      *pdu;
        skstream_t         *silk;
        void               *any;
    }                   src;
    skpc_probetype_t    probe_type;
} dir_source_t;


/* LOCAL VARIABLES */

/* The polling interval */
static uint32_t polling_interval;


/* FUNCTION DECLARATIONS */

static void readerPrintStats(flow_proc_t *fproc);


/* FUNCTION DEFINITIONS */

/*
 *  status = readerGetRecord(out_rwrec, out_probe, flow_processor);
 *
 *    Invoked by input_mode_type->get_record_fn();
 *
 *    Fill 'out_rwrec' with a SiLK Flow record from the underlying
 *    {NetFlow,SiLK,IPFIX} file-based flowsource object, fill
 *    'out_probe' with the probe where the flow was collected, and
 *    return FP_RECORD.
 *
 *    If we are at the end of a file, close it and return
 *    FP_FILE_BREAK.
 *
 *    If there is no flowsource object, create one by pulling a file
 *    off the poll-directory queue, blocking until a file is
 *    available.  Return FP_GET_ERROR when the poll-directory stops.
 *    Otherwise, if we cannot pull a new file name from the
 *    poll-directory, return FP_FATAL_ERROR.
 *
 *    This function may modify the source object that 'fproc' contains.
 *
 *    If we cannot open the file or get the first record from it,
 *    return FP_FATAL_ERROR unless the --error-dir is set, in which
 *    case move the file there and attempt to get another file.
 */
static fp_get_record_result_t
readerGetRecord(
    rwRec                  *out_rwrec,
    const skpc_probe_t    **out_probe,
    flow_proc_t            *fproc)
{
    dir_source_t *dir_source = (dir_source_t *)fproc->flow_src;
    skPollDirErr_t pderr;
    skFlowSourceParams_t params;
    int rv;

    /* handle the common case: getting a record from an open file */
    if (dir_source->src.any) {
        /* try to get a record and return it if successful */
        switch (dir_source->probe_type) {
#if SK_ENABLE_IPFIX
          case PROBE_ENUM_IPFIX:
            if (0 == skIPFIXSourceGetGeneric(dir_source->src.ipfix,out_rwrec)){
                *out_probe = fproc->probe;
                return FP_RECORD;
            }
            break;
#endif  /* SK_ENABLE_IPFIX */

          case PROBE_ENUM_NETFLOW_V5:
            if (0 == skPDUSourceGetGeneric(dir_source->src.pdu, out_rwrec)) {
                *out_probe = fproc->probe;
                return FP_RECORD;
            }
            break;

          case PROBE_ENUM_SILK:
            rv = skStreamReadRecord(dir_source->src.silk, out_rwrec);
            switch (rv) {
              case SKSTREAM_OK:
                *out_probe = fproc->probe;
                return FP_RECORD;
              case SKSTREAM_ERR_EOF:
                break;
              default:
                skStreamPrintLastErr(dir_source->src.silk, rv, &WARNINGMSG);
                break;
            }
            break;

          default:
            skAbortBadCase(dir_source->probe_type);
        }
    }

    /* either no open file yet, or just finished a file.  loop until
     * we get a new file containing records */

    *out_probe = NULL;

    for (;;) {
        /* if we have just finished with a source, print its
         * statistics, close it, either archive the file or delete it,
         * and return FP_FILE_BREAK to the caller. */
        if (dir_source->src.any) {
            readerPrintStats(fproc);

            switch (dir_source->probe_type) {
#if SK_ENABLE_IPFIX
              case PROBE_ENUM_IPFIX:
                skIPFIXSourceDestroy(dir_source->src.ipfix);
                break;
#endif
              case PROBE_ENUM_NETFLOW_V5:
                skPDUSourceDestroy(dir_source->src.pdu);
                break;
              case PROBE_ENUM_SILK:
                skStreamDestroy(&dir_source->src.silk);
                break;
              default:
                skAbortBadCase(dir_source->probe_type);
            }
            dir_source->src.any = NULL;
            flowpackReleaseFileHandle();

            archiveDirectoryInsertOrRemove(dir_source->pathname, NULL);

            return FP_FILE_BREAK;
        }

        /* Prepare for next file */
        fproc->rec_count_total = 0;
        fproc->rec_count_bad = 0;

        /* Get next file from the directory poller */
        pderr = skPollDirGetNextFile(dir_source->polldir, dir_source->pathname,
                                     &dir_source->filename);
        if (PDERR_NONE != pderr) {
            if (PDERR_STOPPED == pderr) {
                return FP_GET_ERROR;
            }
            CRITMSG("Error polling directory for probe %s: %s",
                    skpcProbeGetName(fproc->probe),
                    ((PDERR_SYSTEM == pderr)
                     ? strerror(errno)
                    : skPollDirStrError(pderr)));
            return FP_FATAL_ERROR;
        }

        /* Get a file handle. Check return status in case we started
         * shutting down while waiting for a handle. */
        if (flowpackAcquireFileHandle()) {
            return FP_GET_ERROR;
        }

        params.path_name = dir_source->pathname;
        INFOMSG(("'%s': " INPUT_MODE_TYPE_NAME " processing %s file '%s'"),
                skpcProbeGetName(fproc->probe),
                skpcProbetypeEnumtoName(dir_source->probe_type),
                dir_source->filename);

        /* Open the source and attempt to get its first record.  If
         * successful, return that record. */
        switch (dir_source->probe_type) {
#if SK_ENABLE_IPFIX
          case PROBE_ENUM_IPFIX:
            {
                skIPFIXSource_t *ipfixsource;
                ipfixsource = skIPFIXSourceCreate(fproc->probe, &params);
                if (ipfixsource) {
                    if (0 == skIPFIXSourceGetGeneric(ipfixsource, out_rwrec)) {
                        dir_source->src.ipfix = ipfixsource;
                        *out_probe = fproc->probe;
                        return FP_RECORD;
                    }
                    /* unable to get first record */
                    skIPFIXSourceDestroy(ipfixsource);
                }
            }
            break;
#endif  /* SK_ENABLE_IPFIX */

          case PROBE_ENUM_NETFLOW_V5:
            {
                skPDUSource_t *pdusource;
                pdusource = skPDUSourceCreate(fproc->probe, &params);
                if (pdusource) {
                    if (0 == skPDUSourceGetGeneric(pdusource, out_rwrec)) {
                        dir_source->src.pdu = pdusource;
                        *out_probe = fproc->probe;
                        return FP_RECORD;
                    }
                    /* unable to get first record */
                    skPDUSourceDestroy(pdusource);
                }
            }
            break;

          case PROBE_ENUM_SILK:
            {
                skstream_t *rwios;
                rv = skStreamOpenSilkFlow(&rwios, dir_source->pathname,
                                          SK_IO_READ);
                if (SKSTREAM_OK == rv) {
                    rv = skStreamReadRecord(rwios, out_rwrec);
                    if (SKSTREAM_OK == rv) {
                        *out_probe = fproc->probe;
                        dir_source->src.silk = rwios;
                        return FP_RECORD;
                    }
                    if (SKSTREAM_ERR_EOF == rv) {
                        /* valid file that contains no records. jump
                         * to the top of the while to close and
                         * archive this file.  */
                        dir_source->src.silk = rwios;
                        continue;
                    }
                }
                skStreamPrintLastErr(rwios, rv, &WARNINGMSG);
                skStreamDestroy(&rwios);
            }
            break;

          default:
            skAbortBadCase(dir_source->probe_type);
        }

        /* Since we are here, there was a problem opening the file or
         * getting the first record from it.  In either case, we treat
         * it as an error. */

        flowpackReleaseFileHandle();

        NOTICEMSG("File '%s' does not appear to be a valid %s file",
                  dir_source->pathname,
                  skpcProbetypeEnumtoName(dir_source->probe_type));

        rv = errorDirectoryInsertFile(dir_source->pathname);
        if (rv != 0) {
            /* either no --error-dir (rv == 1) or problem moving the
             * file (rv == -1).  either way, we return an error code
             * to the caller. */
            return FP_FATAL_ERROR;
        }
        /* else, moved file to error dir.  try another file */
    }
}


/*
 *  status = readerStart(flow_processor)
 *
 *    Invoked by input_mode_type->start_fn();
 */
static int
readerStart(
    flow_proc_t        *fproc)
{
    const char *dir;
    dir_source_t *dir_source;

    /* Should be NULL when starting */
    assert(fproc->flow_src == NULL);

    /* create a new dir_source object and add it to this file's global
     * queue or dir_sources. */
    dir_source = (dir_source_t *)calloc(1, sizeof(*dir_source));
    if (dir_source == NULL) {
        /* memory error.  Fail */
        skAppPrintOutOfMemory(NULL);
        return -1;
    }
    dir_source->probe_type = skpcProbeGetType(fproc->probe);

    /* Set up directory polling */
    dir = skpcProbeGetPollDirectory(fproc->probe);
    assert(dir != NULL);

    INFOMSG(("Creating " INPUT_MODE_TYPE_NAME
             " poller for %s probe %s on '%s'"),
            skpcProbetypeEnumtoName(dir_source->probe_type),
            skpcProbeGetName(fproc->probe), dir);

    dir_source->polldir = skPollDirCreate(dir, polling_interval);
    if (dir_source->polldir == NULL) {
        ERRMSG("Could not initiate polling on directory %s", dir);
        free(dir_source);
        return -1;
    }

    fproc->flow_src = dir_source;

    return 0;
}


/*
 *  readerStop(flow_processor);
 *
 *    Invoked by input_mode_type->stop_fn();
 */
static void
readerStop(
    flow_proc_t        *fproc)
{
    dir_source_t *dir_source = (dir_source_t *)fproc->flow_src;

    if (dir_source) {
        if (dir_source->polldir) {
            DEBUGMSG("Stopping polling of %s",
                     skPollDirGetDir(dir_source->polldir));
            skPollDirStop(dir_source->polldir);
        }
    }
}


/*
 *  readerFree(flow_processor);
 *
 *    Invoked by input_mode_type->free_fn();
 */
static void
readerFree(
    flow_proc_t        *fproc)
{
    dir_source_t *dir_source = (dir_source_t*)fproc->flow_src;

    if (dir_source) {
        if (dir_source->src.any) {
            switch (dir_source->probe_type) {
#if SK_ENABLE_IPFIX
              case PROBE_ENUM_IPFIX:
                skIPFIXSourceDestroy(dir_source->src.ipfix);
                break;
#endif
              case PROBE_ENUM_NETFLOW_V5:
                skPDUSourceDestroy(dir_source->src.pdu);
                break;
              case PROBE_ENUM_SILK:
                skStreamDestroy(&dir_source->src.silk);
                break;
              default:
                skAbortBadCase(dir_source->probe_type);
            }
            dir_source->src.any = NULL;
        }
        if (dir_source->polldir) {
            DEBUGMSG("Destroying directory poller");
            skPollDirDestroy(dir_source->polldir);
        }
        free(dir_source);
        fproc->flow_src = NULL;
    }
}


/*
 *  readerPrintStats(flow_processor);
 *
 *    Invoked by input_mode_type->print_stats_fn();
 */
static void
readerPrintStats(
    flow_proc_t        *fproc)
{
    dir_source_t *dir_source = (dir_source_t *)fproc->flow_src;

    switch (dir_source->probe_type) {
#if SK_ENABLE_IPFIX
      case PROBE_ENUM_IPFIX:
        skIPFIXSourceLogStatsAndClear(dir_source->src.ipfix);
        break;
#endif
      case PROBE_ENUM_NETFLOW_V5:
        skPDUSourceLogStatsAndClear(dir_source->src.pdu);
        if (fproc->rec_count_bad) {
            INFOMSG(("'%s': Records categorized %" PRIu64 ", dropped %" PRIu64),
                    dir_source->filename,
                    (fproc->rec_count_total - fproc->rec_count_bad),
                    fproc->rec_count_bad);
        }
        break;

      case PROBE_ENUM_SILK:
        INFOMSG(("%s: Recs %10" PRIu64),
                dir_source->filename,
                skStreamGetRecordCount(dir_source->src.silk));
        break;

      default:
        skAbortBadCase(dir_source->probe_type);
    }
}


/*
 *  status = readerSetup(&out_daemon_mode, probe_vector, options);
 *
 *    Invoked by input_mode_type->setup_fn();
 */
static int
readerSetup(
    fp_daemon_mode_t   *is_daemon,
    const sk_vector_t  *probe_vec,
    reader_options_t   *options)
{
    skpc_probe_t **probe;
    const char *dir;
    size_t j;

    /* this function should only be called if we actually have probes
     * to process */
    if (0 == skVectorGetCount(probe_vec)) {
        skAppPrintErr("readerSetup() called with zero length probe vector");
        return 1;
    }

    /* make certain the directory for each probe exists */
    for (j = 0;
         NULL!=(probe = (skpc_probe_t**)skVectorGetValuePointer(probe_vec, j));
         ++j)
    {
        dir = skpcProbeGetPollDirectory(*probe);
        if (!skDirExists(dir)) {
            skAppPrintErr("Probe %s polls a nonexistent directory '%s'",
                          skpcProbeGetName(*probe), dir);
            return 1;
        }
    }

    polling_interval = options->stream_polldir.polling_interval;

    /* This reader does run as a daemon */
    *is_daemon = FP_DAEMON_ON;

    return 0;
}


/*
 *  yes_or_no = readerWantProbe(probe);
 *
 *    Invoked by input_mode_type->want_probe_fn();
 */
static int
readerWantProbe(
    skpc_probe_t       *probe)
{
    /* probe must have a directory to read */
    if (skpcProbeGetPollDirectory(probe)) {
        /* check the type of the probe */
        switch (skpcProbeGetType(probe)) {
#if SK_ENABLE_IPFIX
          case PROBE_ENUM_IPFIX:
#endif
          case PROBE_ENUM_NETFLOW_V5:
          case PROBE_ENUM_SILK:
            return 1;
          default:
            break;
        }
    }
    return 0;
}


/*
 *  status = dirReaderInitialize(input_mode_type);
 *
 *    Fill in the name and the function pointers for the input_mode_type.
 */
int
dirReaderInitialize(
    input_mode_type_t  *input_mode_type)
{
    /* Set my name */
    input_mode_type->reader_name = INPUT_MODE_TYPE_NAME;

    /* Set function pointers */
    input_mode_type->free_fn       = &readerFree;
    input_mode_type->get_record_fn = &readerGetRecord;
    input_mode_type->setup_fn      = &readerSetup;
    input_mode_type->start_fn      = &readerStart;
    input_mode_type->stop_fn       = &readerStop;
    input_mode_type->want_probe_fn = &readerWantProbe;

    return 0;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
