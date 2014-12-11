/*
** Copyright (C) 2008-2014 by Carnegie Mellon University.
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
**  skoptions-notes.c
**
**    Provide support for the --note-add, --note-file-add, and
**    --note-strip switches.
**
*/


#include <silk/silk.h>

RCSIDENT("$SiLK: skoptions-notes.c cd598eff62b9 2014-09-21 19:31:29Z mthomas $");

#include <silk/utils.h>
#include <silk/skstream.h>
#include <silk/skvector.h>


/* LOCAL DEFINES AND TYPEDEFS */

typedef enum {
    OPT_NOTE_STRIP,
    OPT_NOTE_ADD,
    OPT_NOTE_FILE_ADD
} noteopt_type_t;

typedef struct noteopt_arg_st {
    noteopt_type_t  type;
    const char     *arg;
} noteopt_arg_t;


/* LOCAL VARIABLE DEFINITIONS */

/* a vector of noteopt_arg_t that is filled in by the user's use of
 * the --note-add and --note-file-add switches. */
static sk_vector_t *noteopt_vec = NULL;

/* whether the application wants to ignore the --note-strip option */
static int noteopt_strip_ignored = 0;



/* OPTIONS SETUP */

static struct option noteopt_options[] = {
    {"note-strip",          NO_ARG,       0, OPT_NOTE_STRIP},
    {"note-add",            REQUIRED_ARG, 0, OPT_NOTE_ADD},
    {"note-file-add",       REQUIRED_ARG, 0, OPT_NOTE_FILE_ADD},
    {0,0,0,0}               /* sentinel */
};

static const char *noteopt_help[] = {
    "Do not copy notes from the input files to the output file",
    ("Store the textual argument in the output SiLK file's header\n"
     "\tas an annotation. Switch may be repeated to add multiple annotations"),
    ("Store the content of the named text file in the output\n"
     "\tSiLK file's header as an annotation.  Switch may be repeated."),
    (char*)NULL
};


/* FUNCTION DEFINITIONS */


static int
noteopt_handler(
    clientData          cData,
    int                 opt_index,
    char               *opt_arg)
{
    int *note_strip = (int*)cData;
    noteopt_arg_t note;

    switch ((noteopt_type_t)opt_index) {
      case OPT_NOTE_ADD:
      case OPT_NOTE_FILE_ADD:
        note.type = (noteopt_type_t)opt_index;
        note.arg = opt_arg;
        if (noteopt_vec == NULL) {
            noteopt_vec = skVectorNew(sizeof(noteopt_arg_t));
            if (noteopt_vec == NULL) {
                skAppPrintOutOfMemory(NULL);
                return 1;
            }
        }
        if (skVectorAppendValue(noteopt_vec, &note)) {
            skAppPrintOutOfMemory(NULL);
            return 1;
        }
        break;

      case OPT_NOTE_STRIP:
        assert(noteopt_strip_ignored == 0);
        *note_strip = 1;
        break;

    }

    return 0;
}


int
skOptionsNotesRegister(
    int                *note_strip)
{
    if (note_strip == NULL) {
        noteopt_strip_ignored = 1;
    }

    assert((sizeof(noteopt_options)/sizeof(struct option))
           == (sizeof(noteopt_help)/sizeof(char*)));

    return skOptionsRegister(&noteopt_options[noteopt_strip_ignored],
                             &noteopt_handler, (clientData)note_strip);
}


void
skOptionsNotesTeardown(
    void)
{
    if (noteopt_vec) {
        skVectorDestroy(noteopt_vec);
    }
    noteopt_vec = NULL;
}


void
skOptionsNotesUsage(
    FILE               *fh)
{
    int i;

    for (i = noteopt_strip_ignored; noteopt_options[i].name; ++i) {
        fprintf(fh, "--%s %s. %s\n", noteopt_options[i].name,
                SK_OPTION_HAS_ARG(noteopt_options[i]), noteopt_help[i]);
    }
}


int
skOptionsNotesAddToStream(
    skstream_t         *stream)
{
    sk_file_header_t *hdr = skStreamGetSilkHeader(stream);
    noteopt_arg_t *note;
    size_t i;
    int rv = 0;

    if (noteopt_vec) {
        for (i = 0;
             ((note = (noteopt_arg_t*)skVectorGetValuePointer(noteopt_vec, i))
              != NULL);
             ++i)
        {
            if (note->type == OPT_NOTE_ADD) {
                rv = skHeaderAddAnnotation(hdr, note->arg);
            } else {
                rv = skHeaderAddAnnotationFromFile(hdr, note->arg);
            }
            if (rv) {
                return rv;
            }
        }
    }

    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/
