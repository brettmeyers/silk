/*
** Copyright (C) 2010-2014 by Carnegie Mellon University.
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
**  respoolreader.c
**
**    Helper file for rwflowpack.
**
**    Specify the functions that are used to poll a directory for SiLK
**    Flow files.  The records in these files will be copied into the
**    data repository based on the flowtype and sensor that appears in
**    each record---the flow records are not recategorized.  This
**    allows existing SiLK Flow records to be used to populate a new
**    repository.
**
**    This input_mode_type should only be used for the 'respool'
**    input-mode.
**
**    Because this input_mode_type short-circuits the categorization that
**    rwflowpack normally does, the second half of this file contains
**    functions to support the packing-logic in rwflowpack.
**
**    For a input_mode_type that does recategorize the records, see
**    dirreader.c.
**
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: respoolreader.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/skpolldir.h>
#include <silk/skstream.h>
#include "rwflowpack_priv.h"


/* MACROS and TYPEDEFS */

/* A name for this input mode. */
#define INPUT_MODE_TYPE_NAME  "SiLK File Respool Reader"


/* LOCAL VARIABLES */

/* The directory that the respool reader will poll for SiLK Flow files
 * to process */
static const char *incoming_directory = NULL;

/* Directory polling information */
static skPollDir_t     *polldir = NULL;
static uint32_t         polling_interval;


/* FUNCTION DECLARATIONS */

static void readerPrintStats(flow_proc_t *fproc);


/* FUNCTION DEFINITIONS */

/*
 *  status = readerGetRecord(&out_rwrec, &out_probe, flow_processor);
 *
 *    Invoked by input_mode_type->get_record_fn();
 */
static fp_get_record_result_t
readerGetRecord(
    rwRec                  *out_rwrec,
    const skpc_probe_t    **out_probe,
    flow_proc_t            *fproc)
{
    skstream_t *stream = (skstream_t*)fproc->flow_src;
    skPollDirErr_t pderr;
    char path[PATH_MAX];
    char *filename;
    int rv;

    /* handle the common case: getting a record from an open file */
    if (stream) {
        rv = skStreamReadRecord(stream, out_rwrec);
        switch (rv) {
          case SKSTREAM_OK:
            return FP_RECORD;
          case SKSTREAM_ERR_EOF:
            break;
          default:
            skStreamPrintLastErr(stream, rv, &WARNINGMSG);
            break;
        }
    }

    /* either no open file yet, or just finished a file.  loop until
     * we get a new file containing records */

    for (;;) {
        /* if we have just finished with a stream, print its
         * statistics, close it, either archive the file or delete it,
         * and return FP_FILE_BREAK to the caller. */
        if (stream) {
            readerPrintStats(fproc);
            archiveDirectoryInsertOrRemove(skStreamGetPathname(stream), NULL);
            skStreamDestroy(&stream);

            /* Prepare for next file */
            fproc->flow_src = NULL;
            fproc->rec_count_total = 0;
            fproc->rec_count_bad = 0;

            return FP_FILE_BREAK;
        }

        /* Get next file from the directory poller */
        pderr = skPollDirGetNextFile(polldir, path, &filename);
        if (PDERR_NONE != pderr) {
            if (PDERR_STOPPED == pderr) {
                return FP_GET_ERROR;
            }
            CRITMSG("Error polling respool incoming directory: %s",
                    ((PDERR_SYSTEM == pderr)
                     ? strerror(errno)
                    : skPollDirStrError(pderr)));
            return FP_FATAL_ERROR;
        }

        INFOMSG((INPUT_MODE_TYPE_NAME " processing file '%s'"), filename);

        /* Open the source and attempt to get its first record.  If
         * successful, return that record. */
        rv = skStreamOpenSilkFlow(&stream, path, SK_IO_READ);
        if (SKSTREAM_OK == rv) {
            rv = skStreamReadRecord(stream, out_rwrec);
            if (SKSTREAM_OK == rv) {
                *out_probe = fproc->probe;
                fproc->flow_src = stream;
                return FP_RECORD;
            }
            if (SKSTREAM_ERR_EOF == rv) {
                /* valid file that contains no records. jump to the
                 * top of the loop to close and archive this file.  */
                fproc->flow_src = stream;
                continue;
            }
        }
        skStreamPrintLastErr(stream, rv, &WARNINGMSG);
        skStreamDestroy(&stream);

        /* Since we are here, there was a problem opening the file or
         * getting the first record from it.  In either case, we treat
         * it as an error. */

        NOTICEMSG("File '%s' does not appear to be a valid SiLK Flow file",
                  path);

        rv = errorDirectoryInsertFile(path);
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
 *  status = readerStart(flow_processor);
 *
 *    Invoked by input_mode_type->start_fn();
 */
static int
readerStart(
    flow_proc_t UNUSED(*fproc))
{
    /* Create a polldir object to set up directory polling */
    INFOMSG(("Creating " INPUT_MODE_TYPE_NAME " directory poller for '%s'"),
            incoming_directory);

    polldir = skPollDirCreate(incoming_directory, polling_interval);
    if (NULL == polldir) {
        CRITMSG("Could not initiate polling for %s", incoming_directory);
        return 1;
    }

    return 0;
}


/*
 *  readerStop(flow_processor);
 *
 *    Invoked by input_mode_type->stop_fn();
 */
static void
readerStop(
    flow_proc_t UNUSED(*fproc))
{
    if (polldir) {
        DEBUGMSG("Stopping " INPUT_MODE_TYPE_NAME " directory poller");
        skPollDirStop(polldir);
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
    skstream_t *stream = (skstream_t*)fproc->flow_src;

    INFOMSG(("%s: Recs %10" PRIu64),
            skStreamGetPathname(stream),
            skStreamGetRecordCount(stream));
}


/*
 *  status = readerSetup(&out_daemon_mode, probe_vector, options);
 *
 *    Invoked by input_mode_type->setup_fn();
 */
static int
readerSetup(
    fp_daemon_mode_t           *is_daemon,
    const sk_vector_t   UNUSED(*probe_vec),
    reader_options_t           *options)
{
    /* pull values out of options */
    incoming_directory   = options->respool.incoming_directory;
    polling_interval     = options->respool.polling_interval;

    *is_daemon = FP_DAEMON_ON;

    return 0;
}


/*
 *  readerCleanup();
 *
 *    Invoked by input_mode_type->cleanup_fn();
 */
static void
readerCleanup(
    void)
{
    if (polldir) {
        DEBUGMSG("Destroying " INPUT_MODE_TYPE_NAME " directory poller");
        skPollDirDestroy(polldir);
        polldir = NULL;
    }
}


/*
 *  status = respoolReaderInitialize(input_mode_type);
 *
 *    Fill in the name and the function pointers for the input_mode_type.
 */
int
respoolReaderInitialize(
    input_mode_type_t  *input_mode_type)
{
    /* Set my name */
    input_mode_type->reader_name = INPUT_MODE_TYPE_NAME;

    /* Set function pointers */
    input_mode_type->cleanup_fn    = &readerCleanup;
    input_mode_type->get_record_fn = &readerGetRecord;
    input_mode_type->setup_fn      = &readerSetup;
    input_mode_type->start_fn      = &readerStart;
    input_mode_type->stop_fn       = &readerStop;

    return 0;                     /* OK */
}


/*
 *  ***********************************************************************
 *
 *  PACKING LOGIC
 *
 *  ***********************************************************************
 */

static const char plugin_source[] = __FILE__;
static const char *plugin_path = plugin_source;


/* LOCAL FUNCTION PROTOTYPES */

static int  packLogicSetup(void);
static void packLogicTeardown(void);
static int  packLogicVerifySensor(skpc_sensor_t *sensor);
static int
packLogicDetermineFlowtype(
    const skpc_probe_t *probe,
    const rwRec        *rwrec,
    flowtypeID_t       *ftypes,
    sensorID_t         *sensorids);
static fileFormat_t
packLogicDetermineFileFormat(
    const skpc_probe_t *probe,
    flowtypeID_t        ftype);


/* FUNCTION DEFINITIONS */

/*
 *    Fill in 'packlogic' with pointers to the functions defined in
 *    this file.
 */
int
packLogicRespoolInitialize(
    packlogic_plugin_t *packlogic)
{
    assert(packlogic);

    if (packlogic->path) {
        plugin_path = packlogic->path;
    }

    packlogic->setup_fn =                &packLogicSetup;
    packlogic->teardown_fn =             &packLogicTeardown;
    packlogic->verify_sensor_fn =        &packLogicVerifySensor;
    packlogic->determine_flowtype_fn =   &packLogicDetermineFlowtype;
    packlogic->determine_fileformat_fn = &packLogicDetermineFileFormat;
    return 0;
}


/*
 *    Verify contents of silk.conf file matches the values we set here
 *    and set any globals we require.
 *
 *    Invoked from rwflowpack by packlogic->setup_fn
 */
static int
packLogicSetup(
    void)
{
    return 0;
}


/*
 *    Clean up any memory we allocated.
 *
 *    Invoked from rwflowpack by packlogic->teardown_fn
 */
static void
packLogicTeardown(
    void)
{
    return;
}


/*
 *    Verify sensor by its class.  Verify that the sensor supports the
 *    type(s) of its probe(s).  Verify that enough information is
 *    present on the sensor to categorize a flow record.
 *
 *    Invoked from rwflowpack by packlogic->verify_sensor_fn
 */
static int
packLogicVerifySensor(
    skpc_sensor_t   UNUSED(*sensor))
{
    return 0;
}


/*
 *  count = packLogicDetermineFlowtype(probe, &rwrec, ftypes[], sensorids[]);
 *
 *    Fill the 'ftypes' and 'sensorids' arrays with the list of
 *    flow_types and sensors to which the 'rwrec' probe, collected
 *    from the 'probe' sensor, should be packed.  Return the number of
 *    elements added to each array or -1 on error.
 *
 *    Invoked from rwflowpack by packlogic->determine_flowtype_fn
 */
static int
packLogicDetermineFlowtype(
    const skpc_probe_t  UNUSED(*probe),
    const rwRec                *rwrec,
    flowtypeID_t               *ftypes,
    sensorID_t                 *sensorids)
{
    ftypes[0] = rwRecGetFlowType(rwrec);
    sensorids[0] = rwRecGetSensor(rwrec);
    return 1;
}


/*
 *    Determine the file output format to use.
 *
 *    Invoked from rwflowpack by packlogic->determine_fileformat_fn
 */
static fileFormat_t
packLogicDetermineFileFormat(
    const skpc_probe_t  UNUSED(*probe),
    flowtypeID_t         UNUSED(ftype))
{
#if  SK_ENABLE_IPV6
    return FT_RWIPV6;
#else
    return FT_RWAUGMENTED;
#endif
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
