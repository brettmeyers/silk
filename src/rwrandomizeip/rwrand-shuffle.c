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

/*
**  rwrand-shuffle.c
**
**    Functions to consistently randomize an IP address using a
**    shuffle table---which is actually 4 tables each having the 256
**    values of 0-255 that have been randomly shuffled.  Each
**    positional octet in the IP address uses one table to modify the
**    value appearing in that octet.
**
**    The rwrandShuffleInit() function is called by the main
**    rwrandomizeip application to initialize this back-end; it will
**    register switches.  If the user specifies one of these switches,
**    the optionHandler() function is called to handle it, and
**    rwrandomizeip will use this back-end to randomize each IP.
**
**    rwrandShuffleActivate() is called after options processing but
**    before reading the SiLK Flow records from the input.
**
**    rwrandShuffleRandIP() is called for each IP address to modify it.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: rwrand-shuffle.c b7b8edebba12 2015-01-05 18:05:21Z mthomas $");

#include "rwrandomizeip.h"
#include <silk/skstream.h>


/* LOCAL DEFINES AND TYPEDEFS */

#define OCTETS_PER_IP 4
#define VALUES_PER_OCTET 256


/* LOCAL VARIABLE DEFINITIONS */

/* whether the shuffle table has been initialized.  the --load and
 * --save switches will initialize the table; otherwise, we initialize
 * it before reading the first record. */
static uint8_t table_initialized = 0;

/* the table to use for the mapping the values in each octet to
 * another value. */
static uint8_t shuffle_table[OCTETS_PER_IP][VALUES_PER_OCTET];


/* OPTIONS SETUP */

typedef enum {
    OPT_CONSISTENT, OPT_SAVE_TABLE, OPT_LOAD_TABLE
} randOptionEnum;

static struct rand_option_st {
    const char *name;
    int         has_arg;
    int         id;
    const char *help;
}  rand_options[] = {
    {"consistent",  NO_ARG,       OPT_CONSISTENT,
     "Consistently randomize IP addresses. Def. No"},
    {"save-table",  REQUIRED_ARG, OPT_SAVE_TABLE,
     ("Consistently randomize IP addresses and save this\n"
      "\trun's randomization table for future use. Def. No")},
    {"load-table",  REQUIRED_ARG, OPT_LOAD_TABLE,
     "Consistently randomize IP addresses using a randomization\n"
     "\ttable from a previous run. Def. No"},
    {0,0,0,0}       /* sentinel entry */
};



/* LOCAL FUNCTION PROTOTYPES */

static int  optionHandler(char *opt_arg, void *data);
static int  rwrandShuffleActivate(void *data);
static void rwrandShuffleRandIP(uint32_t *ip);
static void createShuffleTable(void);
static int  loadShuffleFile(const char *filename);
static int  saveShuffleFile(const char *filename);


/* FUNCTION DEFINITIONS */

/*
 *  status = rwrandShuffleLoad();
 *
 *    This function is called by rwrandomizeip to initialize this
 *    back-end.
 */
int
rwrandShuffleLoad(
    void)
{
    int rv;
    int i;

    /* register the functions */
    rv = rwrandomizerRegister(&rwrandShuffleActivate, &rwrandShuffleRandIP,
                              NULL, NULL, NULL);
    if (rv) {
        return rv;
    }

    /* register the options */
    for (i = 0; rand_options[i].name; ++i) {
        rv = rwrandomizerRegisterOption(rand_options[i].name,
                                        rand_options[i].help, &optionHandler,
                                        &(rand_options[i].id),
                                        rand_options[i].has_arg);
        if (rv) {
            return rv;
        }
    }

    return rv;
}


/*
 *  status = rwrandShuffleActivate(data);
 *
 *    Verify that the shuffle table was initialized; if it wasn't,
 *    initialize it now.
 */
static int
rwrandShuffleActivate(
    void        UNUSED(*dummy))
{
    if (table_initialized) {
        return 0;
    }

    createShuffleTable();
    return 0;
}


/*
 *  status = optionHandler(cData, opt_index, opt_arg);
 *
 *    This function is passed to skOptionsRegister(); it will be called
 *    by skOptionsParse() for each user-specified switch that the
 *    application has registered; it should handle the switch as
 *    required---typically by setting global variables---and return 1
 *    if the switch processing failed or 0 if it succeeded.  Returning
 *    a non-zero from from the handler causes skOptionsParse() to return
 *    a negative value.
 *
 *    The clientData in 'cData' is typically ignored; 'opt_index' is
 *    the index number that was specified as the last value for each
 *    struct option in appOptions[]; 'opt_arg' is the user's argument
 *    to the switch for options that have a REQUIRED_ARG or an
 *    OPTIONAL_ARG.
 */
static int
optionHandler(
    char               *opt_arg,
    void               *data)
{
    int id = *((int*)data);

    switch ((randOptionEnum)id) {
      case OPT_CONSISTENT:
        /* nothing to do */
        break;

      case OPT_SAVE_TABLE:
        if (table_initialized) {
            skAppPrintErr("May only specify one of --%s or --%s.",
                          rand_options[OPT_SAVE_TABLE].name,
                          rand_options[OPT_LOAD_TABLE].name);
            return 1;
        }
        createShuffleTable();
        if (saveShuffleFile(opt_arg)) {
            return 1;
        }
        table_initialized = 1;
        break;

      case OPT_LOAD_TABLE:
        if (table_initialized) {
            skAppPrintErr("May only specify one of --%s or --%s.",
                          rand_options[OPT_SAVE_TABLE].name,
                          rand_options[OPT_LOAD_TABLE].name);
            return 1;
        }
        if (loadShuffleFile(opt_arg)) {
            return 1;
        }
        table_initialized = 1;
        break;
    }

    return 0;                     /* OK */
}


/*
 *  rwrandShuffleRandIP(&ip)
 *
 *    Writes a new ip address to the location specified by 'ip' using
 *    the consistent mapping built using createShuffleTable().
 */
static void
rwrandShuffleRandIP(
    uint32_t           *ip)
{
    unsigned int tgtByte, i, mask;

    for (i = 0 ; i < OCTETS_PER_IP ; i++) {
        /* isolate the targeted byte. */
        mask = (0xFF << (i * 8));
        tgtByte = (*ip & mask) >> (i * 8);
        tgtByte = shuffle_table[i][tgtByte];
        *ip = (*ip & ~mask) | (tgtByte << (i * 8));
    }
}


/*
 *  createShuffleTable();
 *
 *    Generates a set of consistent mappings between IP octets.
 *    Each entry maps a corresponding octet value.  To actually
 *    generate the maps we start with a 0-255 table and then swap each
 *    value once.
 */
static void
createShuffleTable(
    void)
{
    unsigned int i,j, swapIndex;
    uint8_t stash;

    /*
     * First loop is pure initialization.  Set everything so
     * shuffle_table[i][j] = j.
     */
    for (i = 0; i < OCTETS_PER_IP; i++) {
        for (j = 0; j < VALUES_PER_OCTET; j++) {
            shuffle_table[i][j] = (uint8_t)j;
        }
    }

    for (i = 0; i < OCTETS_PER_IP; i++) {
        for (j = 0; j < VALUES_PER_OCTET; j++) {
            swapIndex = (unsigned int)((double)VALUES_PER_OCTET * random()
                                       / ((double)SK_MAX_RANDOM+1.0));
            stash = shuffle_table[i][swapIndex];
            shuffle_table[i][swapIndex] = shuffle_table[i][j];
            shuffle_table[i][j] = stash;
        }
    }
}


/*
 *  status = saveShuffleFile(filename);
 *
 *    Writes the contents of the shuffle_table array to the named
 *    file.
 *
 *    This routine is supposed to be called immediately after
 *    shuffling and before any filtering has been done.
 */
static int
saveShuffleFile(
    const char         *filename)
{
    skstream_t *stream = NULL;
    sk_file_header_t *hdr;
    ssize_t rv;
    int i;

    /* Prep and write the file's header information. */
    rv = skStreamCreate(&stream, SK_IO_WRITE, SK_CONTENT_SILK);
    if (rv) {
        return -1;
    }

    hdr = skStreamGetSilkHeader(stream);
    skHeaderSetFileFormat(hdr, FT_SHUFFLE);
    skHeaderSetRecordLength(hdr, 1);
    skHeaderSetRecordVersion(hdr, 0);
    skHeaderSetByteOrder(hdr, SILK_ENDIAN_BIG);
    skHeaderSetCompressionMethod(hdr, SK_COMPMETHOD_NONE);

    if ((rv = skStreamBind(stream, filename))
        || (rv = skStreamOpen(stream))
        || (rv = skStreamWriteSilkHeader(stream)))
    {
        goto END;
    }

    /* Write all values for Octet-0, then all for Octet-1, etc */
    for (i = 0; i < OCTETS_PER_IP; i++) {
        rv = skStreamWrite(stream, shuffle_table[i],
                           (sizeof(uint8_t) * VALUES_PER_OCTET));
        if (rv != (sizeof(uint8_t) * VALUES_PER_OCTET)) {
            goto END;
        }
    }

    /* Close the stream */
    rv = skStreamClose(stream);

  END:
    if (rv) {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        rv = -1;
    }
    skStreamDestroy(&stream);
    return rv;
}


/*
 *  status = loadShuffleFile(filename)
 *
 *    Loads a shuffle file off of disk by reading the octet streams,
 *    given the endian issue, this function will swap direction of the
 *    file was read in the opposite format.
 */
static int
loadShuffleFile(
    const char         *filename)
{
    skstream_t *stream = NULL;
    sk_file_header_t *hdr;
    int rv;
    int i;

    /* open the file and read the header */
    if ((rv = skStreamCreate(&stream, SK_IO_READ, SK_CONTENT_SILK))
        || (rv = skStreamBind(stream, filename))
        || (rv = skStreamOpen(stream))
        || (rv = skStreamReadSilkHeader(stream, &hdr)))
    {
        skStreamPrintLastErr(stream, rv, &skAppPrintErr);
        rv = -1;
        goto END;
    }

    /* verify the header */
    if (skStreamCheckSilkHeader(stream, FT_SHUFFLE, 0, 0, &skAppPrintErr)) {
        rv = -1;
        goto END;
    }

    if (SK_COMPMETHOD_NONE != skHeaderGetCompressionMethod(hdr)) {
        skAppPrintErr("%s: Randomization table compression is not supported",
                      skStreamGetPathname(stream));
        rv = -1;
        goto END;
    }

    /* Since we read bytes, the byte order doesn't matter */

    /* Read all values for Octet-0, then all for Octet-1, etc */
    for (i = 0; i < OCTETS_PER_IP; i++) {
        rv = skStreamRead(stream, shuffle_table[i],
                           (sizeof(uint8_t) * VALUES_PER_OCTET));
        if (rv != (sizeof(uint8_t) * VALUES_PER_OCTET)) {
            if (rv == -1) {
                skStreamPrintLastErr(stream, rv, &skAppPrintErr);
            }
            rv = -1;
            goto END;
        }
    }

    /* Close the stream */
    rv = skStreamClose(stream);

  END:
    skStreamDestroy(&stream);
    return rv;
}



/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
